/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frmemp.cpp		memory-pool allocation routines		*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,1998,1999,2000,2002,2003,2004,	*/
/*		2006,2007,2008,2009,2010,2011,2012,2013,2014,2015	*/
/*		 Ralf Brown/Carnegie Mellon University			*/
/*	This program is free software; you can redistribute it and/or	*/
/*	modify it under the terms of the GNU Lesser General Public 	*/
/*	License as published by the Free Software Foundation, 		*/
/*	version 3.							*/
/*									*/
/*	This program is distributed in the hope that it will be		*/
/*	useful, but WITHOUT ANY WARRANTY; without even the implied	*/
/*	warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR		*/
/*	PURPOSE.  See the GNU Lesser General Public License for more 	*/
/*	details.							*/
/*									*/
/*	You should have received a copy of the GNU Lesser General	*/
/*	Public License (file COPYING) and General Public License (file	*/
/*	GPL.txt) along with this program.  If not, see			*/
/*	http://www.gnu.org/licenses/					*/
/*									*/
/************************************************************************/

#if __GNUC__ >= 3
namespace std {
#include <sys/types.h>		    	// needed by new.h
}
using namespace std ;
#endif /* __GNUC__ >= 3 */

#if !defined(__GNUC__) || __GNUC__ < 4
#include <new.h>
#endif

#include <errno.h>
#include <string.h>			/* needed on SunOS */
#include "fr_mem.h"
#include "frballoc.h"
#include "frmembin.h"
#ifndef NDEBUG
#  define NDEBUG			// comment out to enable assertions
#endif /* !NDEBUG */
#include "frassert.h"
#include "frprintf.h"
#include "frthread.h"
#include "frpcglbl.h"
#include "memcheck.h"			// for VALGRIND_MAKE_*

#ifndef NDEBUG
# undef _FrCURRENT_FILE
static const char _FrCURRENT_FILE[] = __FILE__ ; // save memory
#endif /* NDEBUG */

/************************************************************************/
/*	Manifest constants for this module				*/
/************************************************************************/

#if FrMAX_AUTO_SUBALLOC > 32
#define MPI_MIN_BINSIZE FrMAX_AUTO_SUBALLOC
#else
#define MPI_MIN_BINSIZE 32
#endif

#ifdef AGGRESSIVE_RECLAIM
#  define MAX_FREE_PAGES_CACHED 0
#else
#  define MAX_FREE_PAGES_CACHED 1000
#endif /* AGGRESSIVE_RECLAIM */

/************************************************************************/
/*	Readability macros						*/
/************************************************************************/

#if defined(FrMEMUSE_STATS) && !defined(HELGRIND)
#  define if_MEMUSE_STATS(x) x
#else
#  define if_MEMUSE_STATS(x)
#endif /* FrMEMUSE_STATS */

/************************************************************************/
/*	Types local to this module					*/
/************************************************************************/

// a basically trivial wrapper around FrSubAllocOffset to make it compatible with atomicPush/atomicPop
class FrSubAllocOffsetList
   {
   private:
      FrSubAllocOffset m_offset ;
   public:
      FrSubAllocOffsetList(FrSubAllocOffset ofs = FrFOOTER_OFS) { m_offset = ofs ; }

      FrSubAllocOffset next() const { return m_offset ; }
      void next(FrSubAllocOffset nxt) { m_offset = nxt ; }
      void next(FrSubAllocOffsetList &nxt) { m_offset = nxt.m_offset ; }
      FrSubAllocOffset *valuePtr() { return &m_offset ; }
      const FrSubAllocOffset *valuePtr() const { return &m_offset ; }
      FrSubAllocOffset value() const { return m_offset ; }
   } ;

/************************************************************************/
/*	External and forward declarations				*/
/************************************************************************/

extern void FramepaC_bad_pointer(void *ptr, const char *comment,
				 const char *where);
extern void (*FramepaC_memory_corrupted)(const void *blk, bool,
					 const char *) ;
extern void (*FramepaC_mem_err)(const char *msg) ;

bool FramepaC_check_in_heap(void *block, const char *msg) ;

// forward
void FramepaC_auto_gc() ;
static bool FramepaC_reclaim_free_blocks() ;

/************************************************************************/
/*	 Global Variables for this module				*/
/************************************************************************/

static FrSubAllocator auto_suballocators[FrAUTO_SUBALLOC_BINS+1] ;

uint64_t FrMalloc_requests[FrAUTO_SUBALLOC_BINS+2] = { 0, 0 } ;
uint64_t FrMalloc_bytes = 0 ;
uint64_t FrMalloc_bytes_suballoc = 0 ;
uint64_t FrRealloc_requests = 0 ;

static bool gc_in_progress = false ;
void (*FramepaC_gc_handler)() = 0 ;
void (*FramepaC_auto_gc_handler)() = 0 ;

#if defined(FrREPLACE_MALLOC)
static const char outside_heap_str[] = " (outside heap)" ;
#endif /* FrREPLACE_MALLOC */
static const char FrRealloc_str[] = "FrRealloc" ;
static const char FrFree_str[] = "FrFree" ;

#ifdef FrREPLACE_MALLOC
static unsigned big_alloc_overhead = sizeof(FrBigAllocHdrBase) ;
#else
static unsigned big_alloc_overhead = 0 ; // posix_memalign forces page-offset of 0
#endif

bool FramepaC_memwrite_checks = if_MEMWRITE_CHECKS_else(true,false) ;

//----------------------------------------------------------------------
// global variables for FrMemoryPool

FrCriticalSection FrMemoryPool::s_critsect ;
FrMemFooter *FrMemoryPool::s_global_pagelist = 0 ;
FrMemFooter *FrMemoryPool::s_global_freepages = 0 ;
FrPER_THREAD FrFreeListCL *FrMemoryPool::s_foreign_frees = 0 ;
FrPER_THREAD FrMemPoolFreelist *FrMemoryPool::s_poolinfo = 0 ;
FrPER_THREAD FrMemFooter *FrMemoryPool::s_pagelist_malloc = 0 ;
FrPER_THREAD FrMemFooter *FrMemoryPool::s_freepages = 0 ;
FrPER_THREAD size_t FrMemoryPool::s_freecount = 0 ;
size_t FrMemoryPool::s_freelimit = MAX_FREE_PAGES_CACHED ;

static bool static_freelist_assigned = false ;
static char static_freelist[sizeof(FrMemPoolFreelist)] ;

// to avoid infinite regress of having to allocate memory while
//   initializing the memory allocator, pre-reserve a few threads' worth
//   of freelist heads.
#define NUM_FREELIST_BUFFERS 4
static FrFreeListCL foreign_freelist_buffers[NUM_FREELIST_BUFFERS*2] /* = { 0 } */ ;
// the *2 above reserves a pointer for use in chaining the buffers on thread death
static FrFreeListCL *free_foreign_buffers = 0 ;
static bool foreign_freelist_buffers_init = false ;

// end FrMemoryPool globals
//----------------------------------------------------------------------

/************************************************************************/
/*	Specializations of template FrMemoryBinnedFreelist		*/
/************************************************************************/

bool FramepaC_invalid_address(const void *block) ;

template <>
bool FrMemPoolFreelist::freelistOK() const
{
   const FrFreeHdr *next ;
   bool OK = true ;
   for (const FrFreeHdr *flist = m_freelist.next ;
	flist && flist != &m_freelist ;
	flist = next)
      {
#ifdef FrREPLACE_MALLOC
      if (FramepaC_invalid_address(flist))
	 {
	 OK = false ;
	 break ;
	 }
#endif /* FrREPLACE_MALLOC */
      MK_VALID(flist->next) ;
      next = flist->next ;
      const FrFreeHdr *prev = flist->prev ;
      // do we have valid forward and backward links?
      if (!next || next->prev != flist ||
	  !prev || prev->next != flist)
	 {
	 OK = false ; // freelist corrupted!
	 break ;
	 }
      }
   return OK ;
}

/************************************************************************/
/*	Helper functions						*/
/************************************************************************/

static void allocate_poolinfo(FrMemPoolFreelist *&mpi)
{
   // we have a chicken-and-egg problem in that we can't use a 'normal'
   //  allocation to allocate the first instance, since that would call
   //  into the memory allocator we're trying to initialize

   // to save memory, use a static buffer for the very first instance
   //   (since we will always need one)
   if (!static_freelist_assigned)
      {
      // because this assignment occurs during program startup before
      //   any additional threads are spawned, we don't need
      //   synchronization on the guard variable
      static_freelist_assigned = true ;
      mpi = new (&static_freelist) FrMemPoolFreelist(MPI_MIN_BINSIZE,MAX_SMALL_BLOCK,FrALIGN_SIZE) ;
      VALGRIND_CREATE_MEMPOOL(mpi,0,0) ;
      return ;
      }
   // bypass the slab allocator; this wastes a bit of memory, but any
   //   program using multiple threads will be using a fair amount of
   //   memory anyway due to per-thread pools
   void *block = big_malloc(sizeof(FrMemPoolFreelist)) ;
   if (block)
      {
      VALGRIND_FREELIKE_BLOCK(block,0) ;
      mpi = new (block) FrMemPoolFreelist(MPI_MIN_BINSIZE,MAX_SMALL_BLOCK,FrALIGN_SIZE) ;
      VALGRIND_CREATE_MEMPOOL(mpi,0,0) ;
      }
   return  ;
}

//----------------------------------------------------------------------

static void free_poolinfo(FrMemPoolFreelist *&mpi)
{
   if (mpi == 0)
      return ;
   if (mpi != (FrMemPoolFreelist*)&static_freelist)
      {
      VALGRIND_DESTROY_MEMPOOL(mpi) ;
      mpi->~FrMemPoolFreelist() ;
      big_free(mpi) ;
      mpi = 0 ;
      return ;
      }
   VALGRIND_DESTROY_MEMPOOL(&static_freelist) ;
   FrMemPoolFreelist *fl = (FrMemPoolFreelist*)&static_freelist ;
   fl->~FrMemPoolFreelist() ;
   static_freelist_assigned = false ;
   return ;
}

//----------------------------------------------------------------------

inline size_t suballoc_bin(size_t size)
{
   if (size < sizeof(FrFreeList))
      size = sizeof(FrFreeList) ;
   return ((size+sizeof(FrSubAllocOffset))+(FrSUBALLOC_GRAN-1)) / FrSUBALLOC_GRAN ;
}

//----------------------------------------------------------------------

inline void make_fragment(void *block, size_t oldsize, size_t newsize,
			  FrMemoryPool *mpi)
{
   size_t leftover = oldsize - newsize ;
   char *frag = (((char *)block) + newsize) ;
   char *next = (((char *)block) + oldsize) ;
   // make a valid block out of the leftover fragment and add it
   //   to the freelist
   MKUNDEF(HDR(frag)) ;
   HDR(frag).prevsize = (FrSubAllocOffset)newsize ;
   HDRSETSIZEUSED(frag,leftover) ;
   MKUNDEF(HDR(next).prevsize) ;
   HDR(next).prevsize = (FrSubAllocOffset)leftover ;
   mpi->add((FrFreeHdr*)frag) ;
   return ;
}

/************************************************************************/
/*	Consistency Checks						*/
/************************************************************************/

#if defined(FrMEMORY_CHECKS) && defined(FrREPLACE_MALLOC)
#define check_in_heap(block,msg,retval) \
   if (!FramepaC_check_in_heap(block,msg)) return retval ;
#else
#define check_in_heap(block,msg,retval)
#endif /* FrMEMORY_CHECKS && FrREPLACE_MALLOC */

#if defined(FrMEMORY_CHECKS) && defined(FrPOINTERS_MUST_ALIGN)
#define check_alignment(block,msg,retval)	  \
   if (((long int)block & (FrALIGN_SIZE-1)) != 0)	\
      {							\
      FramepaC_bad_pointer(block,misaligned_str,msg) ;	\
      errno = EFAULT ;					\
      return retval ;					\
      }
#else
#define check_alignment(block,msg,retval)
#endif /* FrMEMORY_CHECKS && FrPOINTERS_MUST_ALIGN */

//----------------------------------------------------------------------

static bool check_FrMalloc_block(const FrMemFooter *block, void *userptr = 0,
				 bool *found = 0)
{
   const char *frag = block->objectStartPtr() ;
   const char *prev = frag ;
   const char *last = ((char*)block) ;
   const char *next ;
   if (found)
      *found = false ;
   int size ;
   do {
      if (userptr && userptr == frag && found)
	 *found = true ;
      MK_VALID(HDR(frag).prevsize) ;
      if (HDR(frag).prevsize != (frag-prev))
	 {
	 FrMessage("bad prev-subblock size") ;
	 return false ;	   // corrupted: bad previous-subblock size
	 }
      MK_VALID(HDR(frag).size) ;
      size = HDRBLKSIZE(frag) ;
      if (size > (last-frag) || (size & (FrALIGN_SIZE-1)) != 0)
	 {
	 FrMessage("bad subblock size") ;
	 return false ;	   // corrupted: bad subblock size
	 }
      next = frag + size ;
      if (size)
	 {
	 MK_VALID(HDR(next).prevsize) ;
	 if (HDR(next).prevsize != size)
	    {
	    FrMessage("link mismatch (next)") ;
	    return false ; // corrupted: link mismatch with following subblock
	    }
	 }
      else if (frag != last)
	 {
	 FrMessage("last block in wrong place") ;
	 return false ;	   // corrupted: last block doesn't end in right place
	 }
      prev = frag ;
      frag = next ;
      } while (size && frag < last) ;
   return frag == last ;  // block checks out OK
}

//----------------------------------------------------------------------

bool check_FrMalloc(FrMemoryPool *mpi)
{
   // check FrMalloc memory chain, and then report either that the pointer
   // is bad or that it has already been freed
   for (const FrMemFooter *block = mpi->localMallocPageList() ;
	block ;
	block = block->next())
      {
      if (!check_FrMalloc_block(block))
	 return false ;	   // block bad, so FrMalloc chain is corrupt!
      }
   return mpi->checkFreelist() ;
}

/************************************************************************/
/*	General allocation functions					*/
/************************************************************************/

FrMemFooter *FramepaC_allocate_block(const char *errloc)
{
   FrMemFooter *newobj = FrMemoryPool::popFreePage() ;
   if (newobj)
      {
      newobj->init(0,newobj->arenaStart()) ;
      }
   else // !newobj
      {
      if (FramepaC_memwrite_checks &&
	  (!FramepaC_memory_chain_OK() ||
	   !check_FrMalloc(&FramepaC_mempool)))
	 {
	 FramepaC_memwrite_checks = false ;
	 FrProgErrorVA("memory corruption detected while allocating block! [%s]",
		       errloc?errloc:"location not specified") ;
	 }
      char *page = (char*)big_malloc(FrALLOC_GRANULARITY-big_alloc_overhead) ;
      if (!page)
	 {
	 // try to reclaim some memory
	 FramepaC_reclaim_free_blocks() ;
#ifdef FrLRU_DISCARD
	 discard_LRU_frames() ;
#endif /* FrLRU_DISCARD */
	 FramepaC_auto_gc() ;
	 newobj = FrMemoryPool::popFreePage() ;
	 if (newobj)
	    {
	    newobj->init(0,newobj->arenaStart()) ;
	    }
	 else
	    {
	    page = (char*)big_malloc(FrALLOC_GRANULARITY-big_alloc_overhead) ;
	    }
	 }
      if (page)				// successfully allocated?
	 {
	 newobj = FrFOOTER_PTR(page) ;
	 newobj->init(0,FrPAGE_OFFSET(page)) ;
	 }
      else if (!newobj)			// out of memory?
	 {
	 return 0 ;
	 }
      }
#ifdef FrREPLACE_MALLOC
   FrBigAllocHdr *hdr = FrBigAllocHdr::header(newobj->arenaStartPtr()) ;
   MKVALID(hdr->suballocated) = true ;
#endif /* FrREPLACE_MALLOC */
   (void)VALGRIND_MAKE_MEM_NOACCESS(newobj,sizeof(FrMemFooter)) ;
   return newobj ;
}

//----------------------------------------------------------------------

static bool in_free_subblock(const FrMemFooter *block, void *userptr)
{
   // assume that the block is OK, so we just follow the subblock chain and
   // see whether the user pointer is inside a free subblock
   const char *frag = (((char*)block) + FrALIGN_SIZE) ;
   const char *last = ((char*)block) - FrALIGN_SIZE ;
   int size ;

   do {
      size = HDR(frag).size ;
      if ((size & FRFREEHDR_USED_BIT) != 0)
	 size &= FRFREEHDR_SIZE_MASK ;
      else if (userptr >= (void*)frag && userptr <= (void*)(frag+size))
	 return true ;	 // yes, we're inside a free block!
      frag += size ;
      } while (size && frag <= last) ;
   return false ;  // not inside free memory
}

//----------------------------------------------------------------------

static bool _check_free_or_bad(const FrMemoryPool *mpi,
			       const char *where, void *blk)
{
#ifdef FrREPLACE_MALLOC
   if (!FramepaC_check_in_heap(blk,where))
      return true ;
#endif /* FrREPLACE_MALLOC */
   // check FrMalloc memory chain, and then report either that the pointer
   // is bad or that it has already been freed
   for (const FrMemFooter *block=mpi->localMallocPageList() ; block ; block = block->next())
      {
      bool found ;
      if (blk >= (void*)block &&
	  blk <= (void*)(((char*)block)+sizeof(FrMemFooter)))
	 {
	 if (!check_FrMalloc_block(block,blk,&found))
	    FramepaC_memory_corrupted(block,true,where) ;
	 else if (!found)
	    {
	    const char *comment = (in_free_subblock(block,blk)
				   ? " (possibly re-freed)" : "") ;
	    FramepaC_bad_pointer(blk, comment, where) ;
	    }
	 else
	    {
	    char buf[200] ;
	    Fr_sprintf(buf,sizeof(buf),"gave already-free block 0x%lX to %s",
		       (long int)blk,where) ;
	    FramepaC_mem_err(buf) ;
	    }
	 return false ;
	 }
      }
   // memory checks out OK, so pointer must be bad
   FramepaC_bad_pointer(blk,"",where) ;
   return true ;
}

// to prevent automatic inlining by Watcom C++ or Visual C++ and to permit
// possible extensions with other handlers, we indirect check_free_or_bad()
// through a pointer
static bool (*check_free_or_bad)(const FrMemoryPool *,const char *,void *blk) = _check_free_or_bad ;

/************************************************************************/
/*	"Garbage Collection"  (memory compaction)			*/
/************************************************************************/

void compact_FrMalloc()
{
   if (!gc_in_progress && FramepaC_memwrite_checks &&
       (!FramepaC_memory_chain_OK() || !check_FrMalloc(&FramepaC_mempool)))
      FrProgError("memory corruption detected while compacting FrMalloc memory!") ;
   // Since we already reclaim empty pages in FrMemoryPool::release, we just need
   //   to grab the list of free pages and return them to the global allocator
   FrMemFooter *page = FrMemoryPool::grabAllFreePages() ;
   while (page)
      {
      FrMemFooter *next = page->next() ;
      // return the page for allocation by any thread
      big_free(page->arenaStartPtr()) ;
      page = next ;
      }
   return ;
}

//----------------------------------------------------------------------
// return any blocks on the global freelist to the system's memory allocator

static bool FramepaC_reclaim_free_blocks()
{
   bool reclaimed = FrMemoryPool::reclaimFreePages() ;
#if defined(FrREPLACE_MALLOC)
   if (big_reclaim())
      reclaimed = true ;
#endif /* FrREPLACE_MALLOC */
   return reclaimed ;
}

//----------------------------------------------------------------------

void FramepaC_auto_gc()
{
   if (gc_in_progress)
      return ;
   gc_in_progress = true ;
   if (FramepaC_memwrite_checks &&
       (!FramepaC_memory_chain_OK() || !check_FrMalloc(&FramepaC_mempool)))
      FrProgError("memory corruption detected while starting GC!") ;
   if (FramepaC_auto_gc_handler)
      FramepaC_auto_gc_handler() ;
   compact_FrMalloc() ;
   gc_in_progress = false ;
   return ;
}

//----------------------------------------------------------------------

void FramepaC_gc()
{
   if (gc_in_progress)
      return ;
   gc_in_progress = true ;
   if (FramepaC_memwrite_checks &&
       (!FramepaC_memory_chain_OK() || !check_FrMalloc(&FramepaC_mempool)))
      FrProgError("memory corruption detected while starting GC!") ;
   FrAllocator::reclaimAll() ;
   if (FramepaC_gc_handler)
      FramepaC_gc_handler() ;
   compact_FrMalloc() ;
   FramepaC_reclaim_free_blocks() ;
   if (FramepaC_memwrite_checks &&
       (!FramepaC_memory_chain_OK() || !check_FrMalloc(&FramepaC_mempool)))
      FrProgError("memory corruption detected at end of GC!") ;
   gc_in_progress = false ;
   return ;
}

//----------------------------------------------------------------------

void FrMalloc_gc()
{
   void (*old_gc)() = FramepaC_gc_handler ;
   FramepaC_gc_handler = 0 ;
   FramepaC_gc() ;
   FramepaC_gc_handler = old_gc ;
   return ;
}

/************************************************************************/
/*	Methods for class FrMemoryPool					*/
/************************************************************************/

FrMemoryPool::FrMemoryPool(const char *name)
{
   // the default memory pool might get forcibly initialized before its
   //   constructor has a chance to run, so don't run the initialization
   //   again if that happens
   if (!initialized())
      {
      FramepaC_mempool.enterCritical() ;
      m_next = FramepaC_memory_pools ;
      FramepaC_memory_pools = this ;
      if (m_next)
	 m_next->m_prev = this ;
      FramepaC_mempool.leaveCritical() ;
      }
   if (this != &FramepaC_mempool || !initialized())
      {
      init(name) ;
      }
   return ;
}

//----------------------------------------------------------------------

FrMemoryPool::~FrMemoryPool()
{
   reclaimFreePages() ;
   FramepaC_mempool.enterCritical() ;
   if (nextPool())
      nextPool()->m_prev = prevPool() ;
   if (prevPool())
      prevPool()->m_next = nextPool() ;
   else
      FramepaC_memory_pools = nextPool() ;
   FramepaC_mempool.leaveCritical() ;
   return ;
}

//----------------------------------------------------------------------

void FrMemoryPool::orphanBusyPages()
{
   FrMemFooter *pages = grabMallocPages() ;
   FrMemFooter *last = pages ;
   if (pages)
      {
      while (last->next())
	 last = last->next() ;
      addGlobalPages(pages,last) ;
      }
   return ;
}

//----------------------------------------------------------------------

void FrMemoryPool::threadCleanup()
{
   FramepaC_mempool.reclaimForeignFrees() ;
   reclaimFreePages() ;
   // move any remaining pages (containing yet-to-be-freed blocks) to
   //   the global reclaim list
   orphanBusyPages() ;
   free_poolinfo(s_poolinfo) ;
#ifdef FrMULTITHREAD
   // put the 'foreign' freelist head on a list for reclamation by the
   //   next thread creation; we can't free it because that could
   //   cause access to invalid memory if any blocks allocated by this
   //   thread are still in use and get freed later
//FIXME: make push thread-safe
   FrFreeListCL *buffer = s_foreign_frees - 1 ;
   buffer->next(free_foreign_buffers) ;
   free_foreign_buffers = buffer ;
#endif /* FrMULTITHREAD */
   return  ;
}

//----------------------------------------------------------------------

FrFreeList *FrMemoryPool::foreignFreelistHead()
{
   if (!s_foreign_frees)
      {
      if (!foreign_freelist_buffers_init)
	 {
	 for (int i = 0 ; i < NUM_FREELIST_BUFFERS ; i++)
	    {
	    FrFreeListCL *buffer = &foreign_freelist_buffers[2*i] ;
	    buffer->next(free_foreign_buffers) ;
	    free_foreign_buffers = buffer ;
	    }
	 foreign_freelist_buffers_init = true ;
	 }
//FIXME: make popping from freelist thread-safe
      if (free_foreign_buffers)
	 {
	 s_foreign_frees = free_foreign_buffers ;
	 free_foreign_buffers = (FrFreeListCL*)free_foreign_buffers->next() ;
	 }
      else
	 {
	 s_foreign_frees = new FrFreeListCL[2] ;
	 // NOTE: the above allocation will never be freed (to avoid
	 //   access to invalid memory if a memory block is freed by
	 //   another thread after the allocating thread dies), so
	 //   tell your memory-leak checker to ignore it
	 }
      if (s_foreign_frees)
	 s_foreign_frees += 1 ; // skip the entry used for chaining buffers
      }
   return s_foreign_frees ;
}

//----------------------------------------------------------------------

void FrMemoryPool::setName(const char *name)
{
   if (name)
      {
      strncpy(m_typename,name,sizeof(m_typename)) ;
      m_typename[sizeof(m_typename)-1] = '\0' ;
      }
   else
      m_typename[0] = '\0' ;
   return ;
}

//----------------------------------------------------------------------

void FrMemoryPool::init(const char *name)
{
   setName(name) ;
   m_prev = 0 ;
   m_refcount = 1 ;
   (void)VALGRIND_MAKE_MEM_DEFINED(this,sizeof(FrMemoryPool)) ;
   return ;
}

//----------------------------------------------------------------------

bool FrMemoryPool::decRefCount()
{
   if (--m_refcount == 0)
      {
      delete this ;
      return true ;
      }
   return false ;
}

//----------------------------------------------------------------------

void FrMemoryPool::addMallocPage(FrMemFooter *page)
{
   // since the list is per-thread, we don't need any synchronization
   FrMemFooter *next = s_pagelist_malloc ;
   page->next(next) ;
   s_pagelist_malloc = page ;
   // we abuse the otherwise-idle myNext pointer as the backlink
   //   to make a doubly-linked list for easy arbitrary removal
   page->myNext(0) ;
   if (next)
      next->myNext(page) ;
   return ;
}

//----------------------------------------------------------------------

FrMemFooter *FrMemoryPool::popFreePage()
{
   // since the list is per-thread, we don't need any synchronization
   FrMemFooter *page = s_freepages ;
   if (page)
      {
      --s_freecount ;
      s_freepages = page->next() ;
      }
   return page ;
}

//----------------------------------------------------------------------

void FrMemoryPool::reclaimForeignFrees()
{
   // reclaim any pending variable-size malloc frees
   FrFreeList *fl = atomicSwap(foreignFreelistHead()->m_next,(FrFreeList*)0) ;
   while (fl)
      {
      FrFreeList *next = fl->next() ;
      release(fl) ;
      fl = next ;
      }
   return ;
}

//----------------------------------------------------------------------

void FrMemoryPool::adoptOrphanedPages()
{
   FrMemFooter *pages = grabGlobalPages() ;
   while (pages)
      {
      FrMemFooter *nxt = pages->next() ;
      pages->acceptOwnership() ;
      addMallocPage(pages) ;
      pages = nxt ;
      }
   return ;
}

//----------------------------------------------------------------------

void *FrMemoryPool::allocate(size_t size)
{
   if (expected(size <= MAX_AUTO_SUBALLOC))
      {
      size_t bin_num = suballoc_bin(size) ;
      if (unlikely(auto_suballocators[bin_num].objectSize() == 0))
	 {
	 new (&auto_suballocators[bin_num]) FrSubAllocator(bin_num * FrSUBALLOC_GRAN,
						       "small FrMalloc") ;
	 }
      if_MEMUSE_STATS(FrMalloc_requests[bin_num]++) ;
      if_MEMUSE_STATS(FrMalloc_bytes_suballoc += size) ;
      void *blk = auto_suballocators[bin_num].allocate() ;
      (void)VALGRIND_MAKE_MEM_DEFINED(&HDR(blk),HDRSIZE) ;
      HDRMARKUSED(blk) ;
      return blk ;
      }
   if (unlikely(!initialized()))
      init("FrMalloc") ;
   if_MEMUSE_STATS(FrMalloc_requests[FrAUTO_SUBALLOC_BINS+1]++) ;
   if_MEMUSE_STATS(FrMalloc_bytes += size) ;
   if (size <= MAX_SMALL_ALLOC)
      {
      if (unlikely(!s_poolinfo))
	 allocate_poolinfo(s_poolinfo) ;
      size_t user_size = size ;
      if (unlikely(FramepaC_memwrite_checks && !check_FrMalloc(this)))
	 FrProgError("memory corruption detected by FrMalloc!") ;
      // round request up to necessary alignment size
      size = round_up(user_size + HDRSIZE,FrALIGN_SIZE) ;
      void *block = s_poolinfo->pop(size) ;
      if (unlikely(!block))
	 {
	 reclaimForeignFrees() ;
	 block = s_poolinfo->pop(size) ;
	 }
      int freesize ;
      if (unlikely(!block))
	 {
	 // adopt page(s) orphaned by another thread's termination
	 adoptOrphanedPages() ;
	 block = s_poolinfo->pop(size) ;
	 }
      if (unlikely(!block))
	 {
	 FrMemFooter *newblock = allocate_block("in FrMalloc()") ;
	 if (!newblock)
	    {
	    errno = ENOMEM ;
	    return 0 ;
	    }
	 addMallocPage(newblock) ;
	 FrSubAllocOffset start = round_up(sizeof(FrMallocHdr),FrALIGN_SIZE) ;
	 newblock->setObjectStart(newblock->objectStart() + start) ;
	 newblock->freelistHead(foreignFreelistHead()) ;
	 block = (FrFreeHdr*)newblock->objectStartPtr() ;
	 FrSubAllocOffset last = FrFOOTER_OFS - newblock->objectStart() ;
	 (void)VALGRIND_MAKE_MEM_NOACCESS(newblock,sizeof(*newblock)) ;
	 MK_VALID(HDR(block).prevsize) ;
	 HDR(block).prevsize = 0 ;
	 char *lastelt = ((char*)block) + last ;
	 MK_VALID(HDR(block).size) ;
	 HDRSETSIZEUSED(block,last) ;
	 HDRSETSIZEUSED(lastelt,0) ;     // end marker
	 freesize = last ;
	 HDR(lastelt).prevsize = (FrSubAllocOffset)last ;
	 }
      else // (block != 0)
	 {
	 MK_VALID(HDR(block).size) ;
	 freesize = HDRBLKSIZE(block) ;
	 HDRMARKUSED(block) ;
	 ASSERT((size_t)freesize >= size,"found too-small block") ;
	 }
      // we now have a block big enough to hold the requested allocation, so
      // split it and store the left-over fragment on the appropriate free list
      s_poolinfo->splitblock(freesize,size) ;
      // split off excess if possible
      if (size < (size_t)freesize)
	 {
	 make_fragment(block,freesize,size,this) ;
	 // set block's size, mark as used
	 MK_VALID(HDR(block).size) ;
	 HDRSETSIZEUSED(block,size) ;
	 }
      MKNOACCESS(HDR(block)) ;
#ifdef VALGRIND
      VALGRIND_MAKE_MEM_UNDEFINED((char*)block,user_size) ;
      size -= HDRSIZE ;
      if (size > user_size)
	 {
	 VALGRIND_MAKE_MEM_NOACCESS((char*)block+user_size,size-user_size) ;
	 }
      VALGRIND_MEMPOOL_ALLOC(s_poolinfo,block,user_size) ;
#endif /* VALGRIND */
      return block ;
      }
   else // size > MAX_SMALL_ALLOC
      {
      size += BIGHDRSIZE ;
      void *blk = big_malloc(size) ;
      if (!blk)
	 {
	 FramepaC_reclaim_free_blocks() ;
	 FramepaC_auto_gc() ;
	 blk = big_malloc(size) ;
	 }
      if (blk)
	 {
	 blk = (((char*)blk)+BIGHDRSIZE) ;
	 HDR(blk).prevsize = 0 ;       		// insert a FrMalloc header
	 HDR(blk).size = FRFREEHDR_BIGBLOCK ; 	// flag it as a system block
	 BIGHDR(blk).size = size ;
	 }
      MKNOACCESS(HDR(blk)) ;
      return blk ;
      }
}

//----------------------------------------------------------------------

#if defined(_MSC_VER) && _MSC_VER >= 800
// turn off Visual C++'s complaint about short += (short)int losing data....
#pragma warning(disable : 4244)
#endif /* _MSC_VER */

void *FrMemoryPool::reallocate(void *block,size_t newsize, bool copydata)
{
   if (block)
      {
      if_MEMUSE_STATS(FrRealloc_requests++) ;
      check_alignment(block,FrRealloc_str,0) ;
      check_in_heap(block,FrRealloc_str,block) ;
      MK_VALID(HDR(block).size) ;
      unsigned int oldsize = HDRBLKSIZE(block) ;
      if (oldsize == FRFREEHDR_BIGBLOCK)  // big blocks must always get new alloc
	 {
#ifdef FrREPLACE_MALLOC
	 if (newsize > MAX_SMALL_ALLOC)
	    {
	    newsize += BIGHDRSIZE ;
	    void *cp = big_realloc(((char*)block)-BIGHDRSIZE,newsize) ;
	    if (cp)
	       {
	       cp = (void*)(((char*)cp) + BIGHDRSIZE) ;
	       BIGHDR(cp).size = newsize ;
	       }
	    return cp ;
	    }
#elif defined(FrTHREE_ARG_NEW)
	 // some versions of Gnu C have a special three-arg new() which will
	 // realloc a block
	 if (newsize > MAX_SMALL_ALLOC)
	    {
	    char *cp = new(((char*)block)-BIGHDRSIZE,newsize+BIGHDRSIZE) char ;
	    cp += BIGHDRSIZE ;
	    return cp ;
	    }
#endif /* FrREPLACE_MALLOC */
	 // get true size from header
	 oldsize = BIGHDR(block).size - BIGHDRSIZE ;
	 }
      else if (HDRBLKUSED(block) == 0)
	 {
	 check_free_or_bad(this,FrRealloc_str,block) ;
	 errno = EFAULT ;
	 return 0 ;
	 }
      else if (newsize <= MAX_AUTO_SUBALLOC &&
	       oldsize <= MAX_AUTO_SUBALLOC)
	 {
	 if (suballoc_bin(oldsize) == suballoc_bin(newsize))
	    {
	    return block ;
	    }
	 }
      else if ((newsize <= MAX_AUTO_SUBALLOC &&
		oldsize > MAX_AUTO_SUBALLOC)
	       ||
	       (newsize > MAX_AUTO_SUBALLOC &&
		oldsize <= MAX_AUTO_SUBALLOC))
	 {
	 // switching between auto_suballoc and normal FrMalloc means
	 //    we have to perform a fresh allocation (below)....
	 }
      else
	 {
	 // round request up to necessary alignment size, ensuring minimum size
	 //  requirement is met
	 size_t user_size = newsize ;
	 if (newsize < MIN_ALLOC-HDRSIZE)
	    newsize = round_up(MIN_ALLOC,FrALIGN_SIZE) ;
	 else
	    newsize = round_up(user_size+HDRSIZE,FrALIGN_SIZE) ;
	 if (newsize == (size_t)oldsize)
	    {
#ifdef VALGRIND	    
	    // no need to actually resize the allocation, but let
	    //   Valgrind know about the changed amount of valid
	    //   memory in the block
	    VALGRIND_MEMPOOL_CHANGE(s_poolinfo,block,block,user_size) ;
	    VALGRIND_MAKE_MEM_DEFINED(block,user_size) ;
	    if (user_size < (size_t)oldsize)
	       {
	       VALGRIND_MAKE_MEM_NOACCESS((char*)block+user_size, oldsize - user_size) ;
	       }
#endif /* VALGRIND */
	    return block ;
	    }
	 FrFreeHdr *next = (FrFreeHdr*)(((char *)block) + oldsize) ;
#ifdef FrMEMORY_CHECKS
	 if (HDR(next).prevsize != oldsize)
	    {
	    check_free_or_bad(this,FrRealloc_str,block) ;
	    errno = EFAULT ;
	    return 0 ;
	    }
#endif /* FrMEMORY_CHECKS */
#ifdef VALGRIND
	 unsigned int origsize = oldsize ;
#endif /* VALGRIND */
	 unsigned int nextsize = s_poolinfo->removeIfFree(next) ;
	 if (nextsize > 0)     // merge in free block after ours
	    {
	    FrFreeHdr *nextnext = (FrFreeHdr*)(((char*)next)+nextsize) ;
	    oldsize += (unsigned int)nextsize ;
	    // add other block's size to ours
	    MK_VALID(HDR(block).size) ;
	    HDRSETSIZEUSED(block,oldsize) ;
	    HDR(nextnext).prevsize = oldsize ;
	    }
	 if (newsize <= (size_t)oldsize)     // possible to resize as desired?
	    {
	    s_poolinfo->splitblock(oldsize,newsize) ;
	    // split off excess if possible
	    if (newsize < (size_t)oldsize)
	       {
	       make_fragment(block,oldsize,newsize,this) ;
	       // set block's size, mark as used
	       MK_VALID(HDR(block).size) ;
	       HDRSETSIZEUSED(block,newsize) ;
	       }
#ifdef VALGRIND
	    VALGRIND_MEMPOOL_CHANGE(s_poolinfo,block,block,user_size) ;
	    if (user_size >= origsize)
	       {
	       VALGRIND_MAKE_MEM_DEFINED(block,origsize) ;
	       VALGRIND_MAKE_MEM_UNDEFINED((char*)block+origsize,user_size-origsize) ;
	       }
	    else
	       {
	       // it's possible for the application to request an
	       //   increased allocation which is still smaller than
	       //   the previous true allocation, in which case we
	       //   need to make the newly-added bytes valid
	       if (newsize >= origsize)
		  {
		  VALGRIND_MAKE_MEM_DEFINED(block,user_size) ;
		  }
	       VALGRIND_MAKE_MEM_NOACCESS((char*)block+user_size,origsize-user_size) ;
	       }
#endif /* VALGRIND */
	    return block ;
	    }
	 }
      // if we get here, we were unable to resize the existing block
      void *newblock ;
      if ((newblock = allocate(newsize)) == 0)
	 {
	 return 0 ;			// errno already set to ENOMEM
	 }
      if (copydata)
	 {
	 if (newsize < (size_t)oldsize)
	    oldsize = newsize ;
	 memcpy(newblock,block,oldsize) ;
	 }
      release(block) ;
      return newblock ;
      }
   else // block == 0
      {
      return allocate(newsize) ;
      }
}

#if defined(_MSC_VER) && _MSC_VER >= 800
// re-enable Visual C++'s loss-of-data complaint for short += int
#pragma warning(enable : 4244)
#endif /* _MSC_VER */

//----------------------------------------------------------------------

void FrMemoryPool::releaseForeign(void *block)
{
   if (block)
      {
#ifdef FrMULTITHREAD
      FrMemFooter *page = FrFOOTER_PTR(block) ;
      FrFreeList *f = (FrFreeList*)block ;
      FrFreeList *fl = page->freelistHead() ;
      if (fl)
	 atomicPush(&fl->m_next,f) ;
      else
	 FrProgError("NULL freelist pointer") ;
#else
      // block is always in the same thread
      release(block) ;
#endif /* FrMULTITHREAD */
      }
   return ;
}

//----------------------------------------------------------------------

void FrMemoryPool::release(void *block)
{
   if (block)
      {
      check_alignment(block,FrFree_str,) ;
      check_in_heap(block,FrFree_str,) ;
      MK_VALID(HDR(block).size) ;
      unsigned size = HDR(block).size ;
      if (size == FRFREEHDR_BIGBLOCK)
	 big_free(((char*)block)-BIGHDRSIZE) ;
      else if (expected((size & FRFREEHDR_USED_BIT) != 0))// make sure block is in use...
	 {
	 size &= FRFREEHDR_SIZE_MASK ;
	 if (expected(size <= MAX_AUTO_SUBALLOC))
	    {
	    size_t bin_num = suballoc_bin(size) ;
	    HDRMARKFREE(block) ;
	    if (expected(auto_suballocators[bin_num].objectSize() > 0))
	       auto_suballocators[bin_num].release(block) ;
	    else
	       FrProgErrorVA("attempted to free small block (size %d) for which\n"
			     "no suballocator has been set up!",(int)size) ;
	    return ;
	    }
	 FrMemFooter *page = FrFOOTER_PTR(block) ;
	 if (unlikely(!page->myMemory()))
	    {
	    releaseForeign(block) ;
	    return ;
	    }
	 // mark the block free
	 FrFreeHdr *next = (FrFreeHdr*)(((char*)block) + size) ;
	 VALGRIND_MEMPOOL_FREE(s_poolinfo,block) ;
#ifdef FrMEMORY_CHECKS
	 MK_VALID(HDR(next).prevsize) ;
	 if (unlikely(HDR(next).prevsize != size))
	    {
	    check_free_or_bad(this,FrFree_str,block) ;
	    errno = EFAULT ;
	    return ;
	    }
#endif /* FrMEMORY_CHECKS */
	 // merge with the following block if it is also free
	 unsigned nextsize = s_poolinfo ? s_poolinfo->removeIfFree(next) : 0 ;
	 if (nextsize > 0)
	    {
	    next = (FrFreeHdr*)(((char*)next) + nextsize) ;
	    size += nextsize ;
	    MK_VALID(HDR(block).size) ;
	    HDRSETSIZEUSED(block,size) ;
	    MK_VALID(HDR(next).prevsize) ;
	    HDR(next).prevsize = size ;
	    }
	 MK_VALID(HDR(block).prevsize) ;
	 unsigned prevsize = HDR(block).prevsize ;
	 // coalesce with previous block if it is also free
	 if (prevsize != 0)
	    {
	    FrFreeHdr *prev = (FrFreeHdr*)(((char*)block) - prevsize) ;
	    MK_VALID(HDR(prev)) ;
	    unsigned int psize = HDR(prev).size ;
	    if (psize == prevsize)
	       {
	       s_poolinfo->remove(prev,FrMemPoolFreelist::bin(prevsize)) ;
	       HDR(next).prevsize += (FrSubAllocOffset)prevsize ;
	       HDR(prev).size += size ;
	       HDRMARKUSED(prev) ;
	       block = prev ;
	       }
#ifdef FrMEMORY_CHECKS
	    else if ((psize & FRFREEHDR_SIZE_MASK) != prevsize)
	       {
	       check_free_or_bad(this,FrFree_str,block) ;
	       errno = EFAULT ;
	       return ;
	       }
#endif /* FrMEMORY_CHECKS */
	    }
	 if (HDR(block).prevsize == 0 && HDR(next).size == FRFREEHDR_USED_BIT)
	    {
	    // the page is now empty
	    // unlink from thread-local list of pages
	    removeMallocPage(page) ;
	    addFreePage(page) ;
	    }
	 else if (s_poolinfo)
	    add((FrFreeHdr*)block) ;
	 return ;
	 }
      else			// the block is already free or bad...
	 {
	 if (size <= MAX_AUTO_SUBALLOC)
	    {
	    FramepaC_bad_pointer(block,"",FrFree_str) ;
	    return ;
	    }
	 check_free_or_bad(this,FrFree_str,block) ;
	 return ;
	 }
      }
   return ;
}

//----------------------------------------------------------------------

void FrMemoryPool::check()
{
   if (!s_poolinfo)
      return ;
   if (s_poolinfo->containsDuplicates())
      {
      (void)FrAssertionFailed("duplicate block found by FrMemoryPool::check",__FILE__,__LINE__) ;
      }
   if (!s_poolinfo->check_idx())
      {
      (void)FrAssertionFailed("bad index or size found by FrMemoryPool::check",__FILE__,__LINE__) ;
      }
   return ;
}

//----------------------------------------------------------------------

void FrMemoryPool::removeMallocBlock(FrMemFooter *oldblock,
					 FrMemFooter *pred)
{
   // since the list is per-thread, we don't need any synchronization
   FrMemFooter *nxt = oldblock->next() ;
   if (pred)
      pred->next(nxt) ;
   else
      s_pagelist_malloc = nxt ;
   return ;
}

//----------------------------------------------------------------------

void FrMemoryPool::removeMallocPage(FrMemFooter *page)
{
   // since the list is per-thread, we don't need any synchronization
   FrMemFooter *next = page->next() ;
   // we abused the myNext pointer as the 'prev' pointer for the
   //   doubly-linked list
   FrMemFooter *prev = page->myNext() ;
   if (next)
      next->myNext(prev) ;
   if (prev)
      prev->next(next) ;
   else
      s_pagelist_malloc = next ;
   return ;
}

//----------------------------------------------------------------------

FrMemFooter *FrMemoryPool::grabMallocPages()
{
   // since the list is per-thread, we don't need any synchronization
   FrMemFooter *pages = s_pagelist_malloc ;
   s_pagelist_malloc = 0 ;
   return pages ;
}

//----------------------------------------------------------------------

void FrMemoryPool::addFreePage(FrMemFooter *page)
{
   if (s_freecount >= s_freelimit)
      {
      // return the page for use by any allocation in any thread
      big_free(page->arenaStartPtr()) ;
      }
   else
      {
      // return page for use by any suballocator in current thread
      // since the list is per-thread, we don't need any synchronization
      page->next(s_freepages) ;
      s_freepages = page ;
      s_freecount++ ;
      return ;
      }
}

//----------------------------------------------------------------------

FrMemFooter *FrMemoryPool::grabAllFreePages()
{
   // since the list is per-thread, we don't need any synchronization
   s_freecount = 0 ;
   FrMemFooter *pages = s_freepages ;
   s_freepages = 0 ;
   return pages ;
}

//----------------------------------------------------------------------

void FrMemoryPool::addGlobalPage(FrMemFooter *page)
{
   atomicPush(&s_global_pagelist,page) ;
   return ;
}

//----------------------------------------------------------------------

void FrMemoryPool::addGlobalPages(FrMemFooter *pages, FrMemFooter *last)
{
   atomicPush(&s_global_pagelist,pages,last) ;
   return ;
}

//----------------------------------------------------------------------

FrMemFooter *FrMemoryPool::grabGlobalPages()
{
   // avoid the expensive atomic operation if the list is currently empty
   if (!s_global_pagelist)
      return 0 ;
   return atomicSwap(s_global_pagelist,(FrMemFooter*)0) ;
}

//----------------------------------------------------------------------

void FrMemoryPool::addGlobalFreePage(FrMemFooter *page)
{
   atomicPush(&s_global_freepages,page) ;
   return ;
}

//----------------------------------------------------------------------

void FrMemoryPool::addGlobalFreePages(FrMemFooter *pages, FrMemFooter *last)
{
   atomicPush(&s_global_freepages,pages,last) ;
   return ;
}

//----------------------------------------------------------------------

FrMemFooter *FrMemoryPool::grabGlobalFreePages()
{
   // avoid the expensive atomic operation if the list is currently empty
#if !defined(HELGRIND)
   if (!s_global_freepages)
      return 0 ;
#endif /* !HELGRIND */
   return atomicSwap(s_global_freepages,(FrMemFooter*)0) ;
}

//----------------------------------------------------------------------

#if 0
FrMemFooter *FrMemoryPool::popGlobalFreePage()
{
   // to properly sync with addFreeBlock() and avoid ABA problems,
   //   atomically grab the entire list of blocks, pop the first one,
   //   and then add back the remainder in a single operation
   FrMemFooter *blocks = grabAllFreePages() ;
   FrMemFooter *block = blocks ;
   if (block)
      {
      blocks = block->next() ;
      block->next(0) ;
      if (blocks)
	 {
	 FrMemFooter *tail = blocks ;
	 while (tail->next())
	    tail = tail->next() ;
	 addFreeBlocks(blocks,tail) ;
	 }
      }
   return block ;
}
#endif /* 0 */

//----------------------------------------------------------------------

long FrMemoryPool::bytesAllocated() const
{
   long bytes = 0 ;
   for (const FrMemFooter *p = localMallocPageList() ; p ; p = p->next())
      {
      bytes += (FrALLOC_GRANULARITY - p->arenaStart()) ;
      }
   return bytes ;
}

//----------------------------------------------------------------------

size_t FrMemoryPool::numPages()
{
   size_t count = 0 ;
   for (const FrMemFooter *p = localMallocPageList() ; p ; p = p->next())
      count++ ;
   return count ;
}

//----------------------------------------------------------------------

size_t FrMemoryPool::numFreePages()
{
   size_t count = 0 ;
   for (const FrMemFooter *f = localFreePages() ; f ; f = f->next())
      count++ ;
   for (const FrMemFooter *f = globalFreePages() ; f ; f = f->next())
      count++ ;
   return count ;
}

//----------------------------------------------------------------------

bool FrMemoryPool::reclaimFreePages()
{
   // atomically grab the entire list of free blocks at once; we can
   //   then process them at our leisure without needing to block
   //   other threads
   FrMemFooter *block = grabAllFreePages() ;
   bool reclaimed = false ;
   FrMemFooter *next ;
   for ( ; block ; block = next)
      {
      next = block->next() ;
      big_free(block->arenaStartPtr()) ;
      reclaimed = true ;
      }	
   block = grabGlobalFreePages() ;
   for ( ; block ; block = next)
      {
      next = block->next() ;
      big_free(block->arenaStartPtr()) ;
      reclaimed = true ;
      }	
   return reclaimed ;
}

//----------------------------------------------------------------------

bool FrMemoryPool::checkFreelist()
{
   if (!s_poolinfo)
      return true ;
   return s_poolinfo->freelistOK() ;
}

//----------------------------------------------------------------------

size_t FrMaxSmallAlloc()
{
   return MAX_SMALL_ALLOC ;
}

// end of file frmemp.cpp //
