/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frmalloc.cpp		memory allocation routines		*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,1998,1999,2000,2001,2002,2003,	*/
/*		 2004,2005,2006,2007,2009,2010,2013,2014,2015		*/
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

#include <errno.h>
#include <memory.h>
#include <sys/types.h>			// for sbrk() [on most systems]
#include "fr_mem.h"
#include "frballoc.h"
#include "frmembin.h"
#include "frmmap.h"
#include "framerr.h"
#ifndef NDEBUG
//#  define NDEBUG			// comment out to enable assertions
#endif /* !NDEBUG */
#include "frassert.h"
#include "frconfig.h"
#include "frprintf.h"
#include "memcheck.h"

#ifdef __BORLANDC__
#include <alloc.h>	  		// for sbrk()
#endif /* __BORLANDC__ */

#ifdef _AIX
#include <unistd.h>			// for sbrk()
#endif /* _AIX */

#if defined(__GNUC__) && __GNUC__ < 3
# include <unistd.h>			// for sbrk(), sysconf()
#elif defined(unix) || defined(__linux__) || defined(__GNUC__)
# include <unistd.h>			// for sysconf()
#endif

#ifdef FrMMAP_SUPPORTED
#if defined(unix) || defined(__linux__) || defined(__GNUC__)
#  include <sys/mman.h>
#elif defined(__WINDOWS__) || defined(__NT__)
#  include <windows.h>
#endif /* unix, Windows|NT */
#endif /* FrMMAP_SUPPORTED */

#ifndef NDEBUG
# undef _FrCURRENT_FILE
static const char _FrCURRENT_FILE[] = __FILE__ ; // save memory
#endif /* NDEBUG */

#if !defined(_GNUC__) && !defined(__attribute__)
#  define __attribute__(x)
#endif

#undef big_malloc
#undef big_free

/************************************************************************/
/*	Portability definitions						*/
/************************************************************************/

// support for using mmap() when sbrk() space is exhausted
#if !defined(MAP_ANONYMOUS) && defined(MAP_ANON)
#  define MAP_ANONYMOUS MAP_ANON
#endif
#ifndef MAP_FAILED
#  define MAP_FAILED ((char*)-1)
#endif /* ! MAP_FAILED */
#ifdef MAP_ANONYMOUS
#  define ALLOC_MMAP(where, size, prot, flags) \
	mmap((where),(size),(prot),MAP_ANONYMOUS|(flags),-1,0)
#endif
#if defined(MAP_HUGETLB)
#  if FrMMAP_ALLOCSIZE < 4*1024*1024
#    undef FrMMAP_ALLOCSIZE
#    define FrMMAP_ALLOCSIZE (4*1024*1024)
#  endif
#else
# define MAP_HUGETLB 0
#endif /* MAP_HUGETLB */

#if defined(__SUNOS__) || defined(__SOLARIS__) || defined(__alpha__)
// the Sun man page says sbrk() is declared in sys/types.h, but it isn't!
extern "C" caddr_t sbrk(int incr) ;
#endif /* __SUNOS__ || __SOLARIS__ */

#if defined(__linux__) && __GNUC__ == 3 && __GNUC_MINOR__ < 3
extern "C" caddr_t sbrk(int incr) ;
#endif /* __linux__ && !__886__ */

#ifdef _MSC_VER
#include <windows.h>
void *sbrk(int size)
{
   if (size > 0)
      return HeapAlloc(GetProcessHeap(),0,size) ;
   else
      return (void*)-1 ;
}
#endif /* _MSC_VER */

/************************************************************************/
/*	Manifest constants for this module				*/
/************************************************************************/

/************************************************************************/
/*	Types for this module						*/
/************************************************************************/

class FrMmapOptions
   {
   public:
      size_t reclaim_size ;
      size_t madvise_size ;
      bool   split_arena ;
   } ;

/************************************************************************/
/*	 Global Data for this module					*/
/************************************************************************/

#if defined(FrMEMORY_CHECKS)
static const char big_malloc_str[] = "big_malloc" ;
static const char big_free_str[] = "big_free" ;
static const char big_realloc_str[] = "big_realloc" ;
#endif /* FrMEMORY_CHECKS */
static const char FrRealloc_str[] = "FrRealloc" ;
static const char FrFree_str[] = "FrFree" ;

static const char outside_heap_str[] = " (outside heap)" ;

#if defined(ALLOC_MMAP) && MAP_HUGETLB != 0 && 0
static bool try_hugetlb = false ; // defaults off for now, as it's MUCH slower
#else
static const bool try_hugetlb = false ;
#endif

bool allow_fragmenting_arenas = true ;

#ifdef FrMMAP_MERGEABLE
bool merge_mmap = true ;
#else
bool merge_mmap = false ;
#endif /* FrMMAP_MERGEABLE */

#define MMAP_RECLAIM_LEVELS 8
static const FrMmapOptions mmap_options[MMAP_RECLAIM_LEVELS] =
   { 
      { (size_t)~0, (size_t)~0, false },
      { 16*FrMMAP_ALLOCSIZE, (size_t)~0, true },
      { 8*FrMMAP_ALLOCSIZE, 6*FrMMAP_ALLOCSIZE, false },
      { 4*FrMMAP_ALLOCSIZE, FrMMAP_ALLOCSIZE, false },
      { 2*FrMMAP_ALLOCSIZE, 8*FrALLOC_GRANULARITY, false },
      { 2*FrMMAP_ALLOCSIZE, 8*FrALLOC_GRANULARITY, true },
      { FrMMAP_ALLOCSIZE+1, FrALLOC_GRANULARITY+1, true },
      { FrMMAP_ALLOCSIZE+1, 0, true }
   } ;
#ifdef AGGRESSIVE_RECLAIM
static unsigned mmap_reclaim_level = MMAP_RECLAIM_LEVELS-1 ;
#elif defined(FrMMAP_MERGEABLE)
static unsigned mmap_reclaim_level = 1 ;  // be very conservative to minimize perf impact
#else
static unsigned mmap_reclaim_level = 4 ;  // reclaim user allocs above min mmap
#endif

/************************************************************************/
/*	 Global Variables for this module				*/
/************************************************************************/

//----------------------------------------------------------------------
// global variables for big_malloc

typedef FrMemoryBinnedFreelist<FrBigAllocHdr,FrBIGPOOLINFO_FACTOR> FrBigAllocFreelist ;

FrBigAllocFreelist FramepaC_bigalloc_pool(FrALLOC_GRANULARITY, FrALLOC_BIGMAX,
					  FrALLOC_GRANULARITY) ;

static FrBigAllocHdr FramepaC_arenas(false) ;
FrBigAllocHdr *FramepaC_memory_start = 0 ;
FrBigAllocHdr *memory_sbrk_last = 0 ;
void *FramepaC_sbrk_end = 0 ;
FrBigAllocHdr *FramepaC_mmap_start = 0 ;
FrBigAllocHdr *FramepaC_memory_end = 0 ;

static size_t total_allocations = 0 ;
size_t mmap_alloc_threshold = 4 * FrMMAP_ALLOCSIZE ;

#if defined(unix) || defined(__linux__) || defined(__GNUC__)
static size_t mmap_granularity = 0 ;
#else
static size_t mmap_granularity = FrMMAP_GRANULARITY ;
#endif

static bool can_return_sbrk = true ;

int FramepaC_memerr_type = 0 ;
void *FramepaC_memerr_addr = 0 ;

// end big_malloc globals
//----------------------------------------------------------------------

// Watcom C++ v11.x places some variables into the modules containing malloc()
// and free() which are referenced from elsewhere, thus causing linker warnings
// about duplicated functions
#if defined(__WATCOMC__) && __WATCOMC__ >= 1100 && defined(FrREPLACE_MALLOC)
extern "C" {
void *__nheapbeg = 0 ;
void *__MiniHeapRover = 0 ;
void *__MiniHeapFreeRover = 0 ;
size_t __LargestSizeB4MiniHeapRover = 0 ;
}
#endif /* __WATCOMC__ >= 1100 && FrREPLACE_MALLOC */

/************************************************************************/
/*	FramepaC replacements for standard memory allocation functions	*/
/************************************************************************/

#if defined(FrREPLACE_MALLOC) && !defined(PURIFY)

#if !defined(FrBUGFIX_XLIB_ALLOC)
# if defined(_MSC_VER)
_CRTIMP void * __cdecl malloc(size_t size) { return FrMalloc(size) ; }
_CRTIMP void * __cdecl realloc(void *blk, size_t size) { return FrRealloc(blk,size) ; }
_CRTIMP void * __cdecl calloc(size_t n, size_t size) { return FrCalloc(n,size) ; }
_CRTIMP int    __cdecl cfree(void *blk) { FrFree(blk) ; return 1 ; }

# else // !_MSC_VER

#if __WATCOMC__ >= 1200 // OpenWatcom
namespace std {
#endif /* __WATCOMC__ */

extern "C" void *malloc(size_t size) __THROW _fnattr_malloc ;
void *malloc(size_t size) __THROW
   { return FrMalloc(size) ; }

extern "C" void *realloc(void *blk, size_t size) __THROW ;
void *realloc(void *blk, size_t size) __THROW
{ return FrRealloc(blk,size,true) ; }

extern "C" void *calloc(size_t n, size_t size) __THROW _fnattr_malloc ;
void *calloc(size_t n, size_t size) __THROW
   { return FrCalloc(n,size) ; }

#ifndef __linux__
extern "C" int cfree(void *blk) ;
int cfree(void *blk) { FrFree(blk) ; return 1 ; }
#endif /* !__linux */

extern "C" void free(void *blk) ;

#if __WATCOMC__ >= 1200 // OpenWatcom
}
#endif /* __WATCOMC__ */

# endif /* _MSC_VER */

#endif /* FrBUGFIX_XLIB_ALLOC */

#ifdef __WATCOMC__
// Watcom C 10.0a's runtime lib frees non-dynamic memory on exit!
#ifndef FrBUGFIX_XLIB_ALLOC
#if __WATCOMC__ >= 1200 // OpenWatcom
namespace std {
#endif /* __WATCOMC__ */
void free(void *blk) { if (!FramepaC_invalid_address(blk)) FrFree(blk) ; }
#if __WATCOMC__ >= 1200 // OpenWatcom
}
#endif /* __WATCOMC__ */
#endif /* FrBUGFIX_XLIB_ALLOC */

// _nmalloc and _nfree are aliases for malloc and free located in the same
// object modules in the Watcom run-time library.
extern "C" void *_nmalloc(size_t) ;
extern "C" void _nfree(void*) ;
void *_nmalloc(size_t size) { return FrMalloc(size) ; }
void _nfree(void *blk) { FrFree(blk) ; }

#else

#ifndef FrBUGFIX_XLIB_ALLOC
#  ifdef _MSC_VER
   _CRTIMP void __cdecl free(void *blk) { FrFree(blk) ; }
#  else
   void free(void *blk) { FrFree(blk) ; }
#  endif /* _MSC_VER */
#endif /* FrBUGFIX_XLIB_ALLOC */

#endif /* __WATCOMC__ */
#endif /* FrREPLACE_MALLOC && !PURIFY */

/************************************************************************/
/*	Specializations of template FrMemoryBinnedFreelist		*/
/************************************************************************/

template <>
size_t FrBigAllocFreelist::blockSize(const FrBigAllocHdr *block)
{
   return block->size() ;
}

//----------------------------------------------------------------------

template <>
size_t FrBigAllocFreelist::blockBin(const FrBigAllocHdr *block)
{
   return bin(block->size()) ;
}

//----------------------------------------------------------------------

template <>
void FrBigAllocFreelist::markAsUsed(FrBigAllocHdr *block)
{
   (void)VALGRIND_MAKE_MEM_DEFINED(block,sizeof(FrBigAllocHdrBase)) ;
   block->status = Block_In_Use ;
   (void)VALGRIND_MAKE_MEM_NOACCESS(block,sizeof(FrBigAllocHdrBase)) ;
   return ;
}

//----------------------------------------------------------------------

template <>
void FrBigAllocFreelist::markAsFree(FrBigAllocHdr *block)
{
   (void)VALGRIND_MAKE_MEM_DEFINED(block,sizeof(FrBigAllocHdrBase)) ;
   block->status = Block_Free ;
   (void)VALGRIND_MAKE_MEM_NOACCESS(block,sizeof(FrBigAllocHdrBase)) ;
   return ;
}

//----------------------------------------------------------------------

template <>
void FrBigAllocFreelist::markAsReserved(FrBigAllocHdr *block)
{
   (void)VALGRIND_MAKE_MEM_DEFINED(block,sizeof(FrBigAllocHdrBase)) ;
   block->status = Block_Reserved ;
   (void)VALGRIND_MAKE_MEM_NOACCESS(block,sizeof(FrBigAllocHdrBase)) ;
   return ;
}

//----------------------------------------------------------------------

template <>
bool FrBigAllocFreelist::blockInUse(const FrBigAllocHdr *block)
{
#ifdef VALGRIND
   (void)VALGRIND_MAKE_MEM_DEFINED(block,sizeof(FrBigAllocHdrBase)) ;
   bool inuse = block->status == Block_In_Use ;
   (void)VALGRIND_MAKE_MEM_NOACCESS(block,sizeof(FrBigAllocHdrBase)) ;
   return inuse ;
#else
   return block->status == Block_In_Use ;
#endif /* VALGRIND */
}

//----------------------------------------------------------------------

template <>
bool FrBigAllocFreelist::blockReserved(const FrBigAllocHdr *block)
{
#ifdef VALGRIND
   (void)VALGRIND_MAKE_MEM_DEFINED(block,sizeof(FrBigAllocHdrBase)) ;
   bool res = block->status == Block_Reserved ;
   (void)VALGRIND_MAKE_MEM_NOACCESS(block,sizeof(FrBigAllocHdrBase)) ;
   return res ;
#else
   return block->status == Block_Reserved ;
#endif /* VALGRIND */
}

/************************************************************************/
/*	Methods for class FrBigAllocHdr					*/
/************************************************************************/

void FrBigAllocHdrBase::initEmpty(size_t sz)
{
   if_DEBUG(memsource = MEMSRC_SBRK ;) // don't fail VALID checks during init
   setSize(sz) ;
   setLastOffset((size_t)0) ;
   status = Block_In_Use ;
   markAsFirst() ;
   last(true) ;
   suballocated = false ;
   prevArena(this) ;
   nextArena(this) ;
   if_DEBUG_else(,memsource = MEMSRC_INVALID) ;
   return ;
}

//----------------------------------------------------------------------

void FrBigAllocHdrBase::init(MemorySource src, size_t sz)
{
   if_DEBUG(memsource = MEMSRC_SBRK ;) // don't fail VALID checks during init
   setSize(sz) ;
   setLastOffset((size_t)0) ;
   status = Block_Free ;
   markAsFirst() ;
   last(true) ;
   suballocated = false ;
   memsource = src ;
   return ;
}

//----------------------------------------------------------------------

FrBigAllocHdr *FrBigAllocHdrBase::splitBlock(size_t newsize, bool add_to_freelist)
{
   if (newsize >= size() || newsize < sizeof(FrBigAllocHdr))
      {
      if_DEBUG(this->invalidateUnused()) ;
      return 0 ;
      }
   size_t leftover = size() - newsize ;
   if (leftover < FrALLOC_GRANULARITY)
      {
      if_DEBUG(this->invalidateUnused()) ;
      return 0 ;
      }
   MK_VALID(*this)
   // grab some values out of the block header before we overwrite them
   FrBigAllocHdr *nxt = next() ;
   size_t origsize = size() ;
   bool are_last = last() ;
   // update the block's size
   setSize(newsize) ;
   // the block being split is no longer the last one in the arena
   last(false) ;
   // point at the leftover bit after resizing the block
   FrBigAllocHdr *frag = next() ;
   // and initialize its header
   (void)VALGRIND_MAKE_MEM_UNDEFINED(frag,sizeof(FrBigAllocHdrBase)) ;
   frag->memsource = memsource ;
   frag->setSize(leftover) ;
   frag->setPrevSize(newsize) ;
   frag->status = Block_Free ;
   frag->last(are_last) ;
   MKVALID(frag->suballocated) = false ;
   if_DEBUG(frag->invalidateUnused() ;)
   if (origsize == 0) abort();
   if (are_last)
      {
      // update the arena's pointer to the last block
      FrBigAllocHdr *arena = thisArena() ;
      arena->setLastBlock(frag) ;
      }
   else
      {
      // update the back link from our successor
      nxt->setPrevSize(leftover) ;
      }
   if_DEBUG(this->invalidateUnused()) ;
   if (add_to_freelist)
      {
      FramepaC_bigalloc_pool.add(frag) ;
      }
   MKNOACCESS(*this)
   return frag ;
}

//----------------------------------------------------------------------

FrBigAllocHdr *FrBigAllocHdrBase::mergePredecessor()
{
   FrBigAllocHdrBase *prv = prev() ;
   if (!prv)
      return (FrBigAllocHdr*)this ;
   (void)VALGRIND_MAKE_MEM_DEFINED(prv,sizeof(FrBigAllocHdrBase)) ;
   if (prv->status == Block_Free)
      {
      FramepaC_bigalloc_pool.remove((FrBigAllocHdr*)prv) ;
      prv->growSize(size()) ;
      if (last())
	 {
	 // update the arena's last-block pointer
	 FrBigAllocHdr *arena = thisArena() ;
	 arena->setLastBlock(prv) ;
	 prv->last(true) ;
	 }
      else
	 {
	 // otherwise, update the back-link from our successor
	 next()->setPrevSize(prv->size()) ;
	 }
      if_DEBUG(invalidate()) ;
      (void)VALGRIND_MAKE_MEM_NOACCESS(prv,sizeof(FrBigAllocHdrBase)) ;
      return (FrBigAllocHdr*)prv ;
      }
   (void)VALGRIND_MAKE_MEM_NOACCESS(prv,sizeof(FrBigAllocHdrBase)) ;
   return (FrBigAllocHdr*)this ;
}

//----------------------------------------------------------------------

bool FrBigAllocHdrBase::mergeSuccessor(bool unlinksucc, bool unlinkself)
{
   if (!last())
      {
      FrBigAllocHdrBase *nxt = next() ;
      (void)VALGRIND_MAKE_MEM_DEFINED(nxt,sizeof(*nxt)) ;
      if (nxt->status == Block_Free)
	 {
	 if (status == Block_Free && unlinkself)
	    {
	    FramepaC_bigalloc_pool.remove((FrBigAllocHdr*)this) ;
	    }
	 if (unlinksucc)
	    {
	    FramepaC_bigalloc_pool.remove((FrBigAllocHdr*)nxt) ;
	    }
	 // merge in the info from the successor block
	 bool are_last = nxt->last() ;
	 growSize(nxt->size()) ;
	 if (are_last)
	    {
	    // if we are now the last block in the arena, update the
	    //   arena's last-block pointer
	    FrBigAllocHdr *arena = nxt->thisArena() ; //'nxt' has the direct pointer
	    arena->setLastBlock(this) ;
	    last(true) ;
	    }
	 else
	    {
	    // otherwise, update the back-link from our new successor
	    next()->setPrevSize(size()) ;
	    }
	 if_DEBUG(nxt->invalidate() ;)
	 (void)VALGRIND_MAKE_MEM_NOACCESS(nxt,sizeof(*nxt)) ;
	 return true ;
	 }
      (void)VALGRIND_MAKE_MEM_NOACCESS(nxt,sizeof(*nxt)) ;
      }
   return false ;
}

//----------------------------------------------------------------------

FrBigAllocHdr *FrBigAllocHdrBase::thisArena() const
{
   // to be able to find the start of the arena quickly, we point the
   //   last subblock's reverse arena chain at the first subblock
   if (last())
      return (FrBigAllocHdr*)(((char*)this) - m_lastoffset) ;
   if (first())
      return (FrBigAllocHdr*)this ;
   // all other subblocks require a search of the arena list
   FrBigAllocHdr *arena ;
   for (arena = FramepaC_arenas.nextArena() ;
	arena != &FramepaC_arenas ;
	arena = arena->nextArena())
      {
      if (this >= arena && this < arena->arenaEnd())
	 return arena ;
      }
   arena = 0 ;
   assert(arena != 0) ;
   return arena ;
}

//----------------------------------------------------------------------

bool FrBigAllocHdrBase::shrinkArena(FrBigAllocHdr *new_last_block)
{
   new_last_block->last(true) ;
   setLastBlock(new_last_block) ;
   return true ;
}

//----------------------------------------------------------------------

FrBigAllocHdr *FrBigAllocHdrBase::splitArena(FrBigAllocHdr *last_subblock, bool relink)
{
   assert(last_subblock >= this && (char*)last_subblock < ((char*)this)+arenaSize()) ;
   FrBigAllocHdr *frag = last_subblock->next() ;
   // next() returns 0 if the subblock is the last one, in which case there's nothing to do
   if (frag)
      {
      FrBigAllocHdr *orig_last = lastBlock() ;
      // shrink the arena
      setLastBlock(last_subblock) ;
      // set the new arena's size
      frag->setLastBlock(orig_last) ;
      // mark the given block as the last in its arena
      last_subblock->last(true) ;
      // mark the first subblock of the new arena as the first in its
      //   arena
      frag->markAsFirst() ;
      if (relink)
	 {
	 // remove the original arena from the arena chain and add the new
	 //   fragment in its place
	 FrBigAllocHdr *prev_arena = prevArena() ;
	 FrBigAllocHdr *next_arena = nextArena() ;
	 prev_arena->nextArena(frag) ;
	 frag->prevArena(prev_arena) ;
	 frag->nextArena(next_arena) ;
	 next_arena->prevArena(frag) ;
	 }
      else
	 {
	 frag->prevArena(0) ;
	 frag->nextArena(0) ;
	 }
      }
   return frag ;
}

//----------------------------------------------------------------------

void FrBigAllocHdrBase::linkSuccessorArena(FrBigAllocHdr *arena)
{
   FrBigAllocHdr *nxt = nextArena() ;
   if (!nxt)
      nxt = &FramepaC_arenas ;
   nextArena(arena) ;
   arena->prevArena((FrBigAllocHdr*)this) ;
   arena->nextArena(nxt) ;
   nxt->prevArena(arena) ;
   return ;
}

//----------------------------------------------------------------------

bool FrBigAllocHdrBase::mergeSuccessorArena(FrBigAllocHdr *next_arena,
					    bool is_linked)
{
   // is the successor immediately adjacent?
   if (next_arena != arenaEnd())
      return false ;
   if (is_linked)
      {
      // unlink the successor arena from the list of arenas
      next_arena->unlinkArena() ;
      // update the global arena chain
      if (FramepaC_arenas.prevArena() == next_arena)
	 FramepaC_arenas.prevArena(this) ;
      }
   // get the last block of each of the two arenas
   FrBigAllocHdr *lastblock = lastBlock() ;
   FrBigAllocHdr *nextlast = next_arena->lastBlock() ;
   // grow the first arena by making the second one's last block the
   //   first one's last block
   setLastBlock(nextlast) ;
   // link the second arena's first block back to the first arena's
   //   last block
   next_arena->setPrevSize(lastblock->size()) ;
   // first arena's last block is no longer the last block
   lastblock->last(false) ;
   if_DEBUG(lastblock->invalidateUnused() ;)
   return true ;
}

//----------------------------------------------------------------------

void FrBigAllocHdrBase::unlinkArena()
{
   assert(first()) ;
   FrBigAllocHdr *prev_arena = prevArena() ;
   FrBigAllocHdr *next_arena = nextArena() ;
   prev_arena->nextArena(next_arena) ;
   next_arena->prevArena(prev_arena) ;
   return ;
}

//----------------------------------------------------------------------

void FrBigAllocHdrBase::invalidateUnused()
{
   if (!first())
      {
      m_nextarena = 0 ;
      m_prevarena = 0 ;
#ifdef VALGRIND_MAKE_MEM_UNDEFINED
      (void)VALGRIND_MAKE_MEM_UNDEFINED(&m_nextarena,sizeof(m_nextarena)) ;
      (void)VALGRIND_MAKE_MEM_UNDEFINED(&m_prevarena,sizeof(m_prevarena)) ;
#endif
      if (!last())
	 {
	 m_lastoffset = 0 ;
#ifdef VALGRIND_MAKE_MEM_UNDEFINED
	 (void)VALGRIND_MAKE_MEM_UNDEFINED(&m_lastoffset,sizeof(m_lastoffset)) ;
#endif
	 }
      }
   return ;
}

//----------------------------------------------------------------------

void FrBigAllocHdrBase::invalidate()
{
   m_nextarena = 0 ;
   m_prevarena = 0 ;
   m_lastoffset = 0 ;
   status = Block_Reserved ;
   memsource = MEMSRC_INVALID ;
   m_size = 0 ;
   m_prevsize = ~0 ;
#ifdef VALGRIND_MAKE_MEM_UNDEFINED
   (void)VALGRIND_MAKE_MEM_UNDEFINED(this,sizeof(*this)) ;
#endif
   return ;
}

//----------------------------------------------------------------------

bool FrBigAllocHdrBase::checkArenaChain()
{
   // check the memory limits for consistency
   if (FramepaC_memory_start > FramepaC_memory_end)
      return false ;
   if (memory_sbrk_last > FramepaC_memory_end ||
       FramepaC_sbrk_end > FramepaC_memory_end)
      return false ;
   if (memory_sbrk_last && FramepaC_mmap_start &&
       memory_sbrk_last >= FramepaC_mmap_start)
      return false ;
   // walk the arena chain, checking for out-of-order arenas,
   //   incorrect reverse links, and (if merging is permitted)
   //   contiguous adjacent arenas
   for (FrBigAllocHdr *arena = FramepaC_arenas.nextArena() ;
	arena != &FramepaC_arenas ;
	arena = arena->nextArena())
      {
      FrBigAllocHdr *next = arena->nextArena() ;
      if (!next)
	 {
	 cerr << "invalid arena forward link" << endl ;
	 return false ;
	 }
      if (next->prevArena() != arena)
	 {
	 cerr << "invalid arena reverse link" << endl ;
	 return false ;
	 }
      if (next != &FramepaC_arenas && next <= arena)
	 {
	 cerr << "mis-ordered arenas" << endl ;
	 printArenaChain() ;
	 return false ;
	 }
      if (merge_mmap && arena->arenaEnd() == next)
	 {
	 cerr << "mergeable arenas" << endl ;
	 return false ;
	 }
      }
   return true ;
}

//----------------------------------------------------------------------

void FrBigAllocHdrBase::printArenaChain()
{
   cerr << "Valid address ranges are " << FramepaC_memory_start << "-"
	<< (void*)(((char*)FramepaC_sbrk_end)-1) << " and " << FramepaC_mmap_start << "-"
	<< (void*)(((char*)FramepaC_memory_end)-1) << endl ;
   cerr << "Arena list:" ;
   MemorySource prevsrc = MEMSRC_INVALID ;
   for (FrBigAllocHdr *arena = FramepaC_arenas.nextArena() ;
	arena != &FramepaC_arenas ;
	arena = arena->nextArena())
      {
      cerr << ' ' ;
      if (arena->memsource == MEMSRC_MMAP && prevsrc != MEMSRC_MMAP)
	 {
	 cerr << "mmap:" ;
	 if (arena != FramepaC_mmap_start)
	    cerr << "!!" ;
	 }
      if (arena < arena->prevArena())
	 cerr << "<<" ;
      cerr << arena << "-" << (void*)(((char*)arena->arenaEnd())-1) ;
      if (arena == memory_sbrk_last)
	 cerr << ":sbrk" ;
      prevsrc = arena->memsource ;
      }
   cerr << endl ;
   return ;
}

/************************************************************************/
/*	Methods for class FrBigAllocHdrFree				*/
/************************************************************************/

void FrBigAllocHdr::initEmpty()
{
   FrBigAllocHdrBase::initEmpty(sizeof(*this)) ;
   setNextFree(this) ;
   setPrevFree(this) ;
   return ;
}

//----------------------------------------------------------------------

void FrBigAllocHdr::invalidateUnused()
{
   this->FrBigAllocHdrBase::invalidateUnused() ;
   if (status == Block_In_Use)
      {
      nextfree = 0 ;
      prevfree = 0 ;
#ifdef VALGRIND_MAKE_MEM_UNDEFINED
      (void)VALGRIND_MAKE_MEM_UNDEFINED(&nextfree,sizeof(nextfree)) ;
      (void)VALGRIND_MAKE_MEM_UNDEFINED(&prevfree,sizeof(prevfree)) ;
#endif
      }
   return ;
}

//----------------------------------------------------------------------

void FrBigAllocHdr::invalidate()
{
   this->FrBigAllocHdrBase::invalidate() ;
#if 0
   nextfree = 0 ;
   prevfree = 0 ;
#ifdef VALGRIND_MAKE_MEM_UNDEFINED
      VALGRIND_MAKE_MEM_UNDEFINED(&nextfree,sizeof(nextfree)) ;
      VALGRIND_MAKE_MEM_UNDEFINED(&prevfree,sizeof(prevfree)) ;
#endif
#endif
   return ;
}

/************************************************************************/
/*	Error reporting							*/
/************************************************************************/

static void memory_corrupted(const void *blk, bool in_near, const char *where)
   _fnattr_noreturn ;
static void memory_corrupted(const void *blk, bool in_near, const char *where)
{
   FrProgErrorVA("Memory chain corrupted %s 0x%lX! (found by %s)\n"
		 "  This is typically the result of dereferencing a bad pointer or\n"
		 "  overrunning an array.",
		 in_near ? "in block" : "near",
		 (long int)blk, where) ;
   exit(127) ;
}

// to prevent automatic inlining by the compiler and to permit
//   possible extensions with other handlers, we indirect
//   memory_corrupted() through a pointer
void (*FramepaC_memory_corrupted)(const void *blk, bool, const char *)
    = memory_corrupted ;

//----------------------------------------------------------------------

static void mem_err(const char *msg)
{
   if (memory_errors_are_fatal)
      FrProgError(msg) ;
   else
      FrWarningVA("Memory Error: %s",msg) ;
}

// to prevent automatic inlining by the compiler and to permit
//   possible extensions with other handlers, we indirect mem_err()
//   through a pointer
void (*FramepaC_mem_err)(const char *msg) = mem_err ;

//----------------------------------------------------------------------

void FramepaC_bad_pointer(void *ptr, const char *comment, const char *where)
{
   char buf[200] ;
   Fr_sprintf(buf,sizeof(buf),"%s got bad pointer 0x%lX%s.",
	      where, (long int)ptr, comment) ;
   FramepaC_mem_err(buf) ;
   return ;
}

/************************************************************************/
/*	Error checking							*/
/************************************************************************/

bool FramepaC_invalid_address(const void *block)
{
   return (block < FramepaC_memory_start || block >= FramepaC_memory_end
#ifdef ALLOC_MMAP
	   || (block >= FramepaC_sbrk_end && block < FramepaC_mmap_start)
#endif /* ALLOC_MMAP */
      ) ;
}

//----------------------------------------------------------------------

bool FramepaC_check_in_heap(void *block, const char *msg)
{
   if (FramepaC_invalid_address(block))
      {
      FramepaC_bad_pointer(block,outside_heap_str,msg) ;
      FrBigAllocHdr::printArenaChain() ;
      return false ;
      }
   return true ;
}

//----------------------------------------------------------------------

bool FramepaC_bigmalloc_freelist_OK()
{
   bool OK = true ;
   FrCRITSECT_ENTER(FramepaC_bigalloc_pool.mutex()) ;
   // check the freelist
   FrBigAllocHdr *head = FramepaC_bigalloc_pool.freelistHead() ;
   FrBigAllocHdr *nextfree ;
   for (const FrBigAllocHdr *block = head->nextFree() ;
	block && block != head ;
	block = nextfree)
      {
      // is the block at a valid address?
      if (FramepaC_invalid_address(block))
	 {
	 OK = false ; // chain corrupted!
	 break ;
	 }
      MK_VALID(block->nextfree) ;
      MK_VALID(block->prevfree) ;
      nextfree = block->nextfree ;
      FrBigAllocHdr *prevfree = block->prevfree ;
      // are the forward and backward links correct?
      // must be non-NULL, must point at a block that points back at us,
      //  and can't point at ourself (except for the list head, which is
      //  never processed here)
      if (!nextfree || !prevfree)
	 {
	 OK = false ; // chain corrupted!
	 break ;
	 }
      MK_VALID(nextfree->prevfree) ;
      MK_VALID(prevfree->nextfree) ;
      if (nextfree->prevfree != block ||
	  prevfree->nextfree != block ||
	 nextfree == block || prevfree == block)
	 {
	 OK =  false ; // chain corrupted!
	 break ;
	 }
      MKNOACCESS(nextfree->prevfree) ;
      MKNOACCESS(prevfree->nextfree) ;
      (void)VALGRIND_MAKE_MEM_NOACCESS(block,sizeof(FrBigAllocHdrBase)) ;
      }
   FrCRITSECT_LEAVE(FramepaC_bigalloc_pool.mutex()) ;
   return OK ;
}

//----------------------------------------------------------------------

bool FramepaC_bigmalloc_chain_OK()
{
   size_t freeblocks = 0 ;
   size_t alloc = 0 ;
   bool OK = true ;

   FrCRITSECT_ENTER(FramepaC_bigalloc_pool.mutex()) ;
   FrBigAllocHdr *next_arena ;
   for (FrBigAllocHdrBase *arena = FramepaC_arenas.nextArena() ;
	arena && arena != &FramepaC_arenas && OK ;
	arena = next_arena)
      {
      FrBigAllocHdr *block = (FrBigAllocHdr*)arena ;
      FrBigAllocHdr *pastend = (FrBigAllocHdr*)(((char*)arena) + arena->arenaSize()) ;
      // iterate over the blocks in the current memory arena
      do {
         // check the reverse link
         if ((block->prevsize() == 0 && !block->first()) ||
	     (!block->first() && block == arena))
	    {
	    FramepaC_memerr_type = 1 ;
	    OK = false ;
	    break ;
	    }
	 // check the forward link
	 if (block->size() < sizeof(FrBigAllocHdr) ||
	     block->next() > pastend ||
	     (block->last() && (((char*)block)+block->size()) != (char*)pastend))
	    {
	    FramepaC_memerr_type = 2 ;
	    OK = false ;
	    break ;
	    }
	 // field range checks
	 (void)VALGRIND_MAKE_MEM_DEFINED(block,sizeof(FrBigAllocHdrBase)) ;
	 if (block->status > Block_Last_Status ||
	     block->suballocated > true ||
	     (block->first() && block->arenaSize() < block->size()))
	    {
	    FramepaC_memerr_type = 3 ;
	    OK = false ;
	    break ;
	    }
	 // arena information checks
	 if (block->first())
	    {
	    if (block->memsource > MEMSRC_LASTSRC ||
		block->memsource == MEMSRC_INVALID ||
		block->arenaSize() < block->size())
	       {
	       FramepaC_memerr_type = 4 ;
	       OK = false ;
	       break ;
	       }
	    }
	 if (block->last())
	    {

	    }
	 // accumulate statistics
	 alloc += block->size() ;
	 if (block->status == Block_Free)
	    freeblocks++ ;
#ifdef VALGRIND
	 FrBigAllocHdr *nextblock = block->next() ;
	 (void)VALGRIND_MAKE_MEM_NOACCESS(block,sizeof(FrBigAllocHdrBase)) ;
	 block = nextblock ;
#else
	 block = block->next() ;
#endif	 
         } while (block) ;
      next_arena = arena->nextArena() ;
      MKNOACCESS(*arena)
      }
   if (OK)
      {
      FramepaC_memerr_addr = 0 ;
      FramepaC_memerr_type = 0 ;
      if (alloc != total_allocations)
	 {
	 FramepaC_memerr_type = 98 ;
	 OK = false ;			// internal inconsistency!
	 }
      }
   FrCRITSECT_LEAVE(FramepaC_bigalloc_pool.mutex()) ;
   return OK ;
}

//----------------------------------------------------------------------

#if defined(FrMEMORY_CHECKS)
static void _check_memory(void *blk, const char *caller, const char *where)
{
   if (FramepaC_invalid_address(blk))
      {
      FramepaC_bad_pointer(blk,outside_heap_str,caller) ;
      if (memory_errors_are_fatal)
	 exit(127) ;
      }
   else if (!FramepaC_bigmalloc_chain_OK())
      FramepaC_memory_corrupted(blk,false,where) ;
}

// to prevent automatic inlining by Watcom C++ or Visual C++ and to permit
// possible extensions with other handlers, we indirect check_memory() through
// a pointer
static void (*check_memory)(void *blk, const char *,const char *) = _check_memory ;
#endif /* FrMEMORY_CHECKS */

/************************************************************************/
/*	Configuration Functions						*/
/************************************************************************/

unsigned FrMallocReclaimLevel__internal()
{
   return mmap_reclaim_level ;
}

//----------------------------------------------------------------------

unsigned FrMallocReclaimLevelMax__internal()
{
   return MMAP_RECLAIM_LEVELS - 1 ;
}

//----------------------------------------------------------------------

bool FrMallocReclaimLevel__internal(unsigned level)
{
   if (level < MMAP_RECLAIM_LEVELS)
      {
      mmap_reclaim_level = level ;
      return true ;
      }
   return false ;
}

//----------------------------------------------------------------------

bool FrMallocAllowFragmentingArenas__internal(bool allow)
{
   bool prev = allow_fragmenting_arenas ;
   allow_fragmenting_arenas = allow ;
   return prev ;
}

/************************************************************************/
/************************************************************************/

// find the highest allocation arena below the given block
static FrBigAllocHdr *find_predecessor(FrBigAllocHdr *block)
{
//   assert(block!=0);
   FrBigAllocHdr *head = &FramepaC_arenas ;
   // if we're above the highest previous allocation, by definition it's
   //   our predecessor in address order, so just return the last block
   //   in the memory chain
   FrBigAllocHdr *pred = head->prevArena() ;
   if (block >= pred)
      return pred ;
   // if the block is below the start of our heap, we have to insert
   //   it as the very first block, which makes the list head its
   //   predecessor
   pred = head->nextArena() ;
   if (block < pred)
      return head ;
   if (memory_sbrk_last)
      {
      if (block > memory_sbrk_last)
	 {
	 if (block < FramepaC_mmap_start)
	    {
	    // work *backwards* from first mmap block; this should find the 
	    //   predecessor in a single step
	    FrBigAllocHdr *prev = FramepaC_mmap_start->prevArena() ;
	    if (prev && prev < block)
	       return prev ;
	    }
	 else if (!FramepaC_mmap_start)
	    {
	    // we're above the highest previous sbrk arena and there are
	    //   no mmap arenas, so the highest sbrk arena is our
	    //   predecessor in address order
	    return memory_sbrk_last ;
	    }
	 }
      }
   // OK, we'll have to bite the bullet and scan the entire arena chain.
   while (pred != head)
      {
      FrBigAllocHdr *next = pred->nextArena() ;
      if (next > block)
	 break ;
      pred = next ;
      }
   if (pred != head)
      return pred ;
   // not found
   assert(0);
   return 0 ;
}

//----------------------------------------------------------------------

static bool adjust_sbrk(FrBigAllocHdr *&block, size_t gran = FrALLOC_GRANULARITY)
{
   int adjust = (((long int)block)&(gran-1)) ;
   if (adjust)			// misaligned block?
      {
      void *prev_sbrk = sbrk(0) ;
      if (sbrk(gran - adjust) == prev_sbrk)
	 {
	 block = (FrBigAllocHdr*)(((char*)block) + gran - adjust) ;
	 return true ;
	 }
      else
	 {
	 errno = ENOMEM ;
	 block = 0 ;
	 return false ;
	 }
      }
   return false ;
}

//----------------------------------------------------------------------

static FrBigAllocHdr *try_sbrk(size_t sbsize)
{
   if (sbsize < FrSBRK_ALLOCSIZE)
      sbsize = FrSBRK_ALLOCSIZE ;
#if FrALIGN_SIZE_SBRK > FrALLOC_GRANULARITY
   sbsize = round_up(sbsize,FrALIGN_SIZE_SBRK) ;
#endif
   FrBigAllocHdr *block = (FrBigAllocHdr*)sbrk(sbsize) ;
   if ((void*)block == (void*)-1)
      return 0 ;
   total_allocations += sbsize ;
   block->init(MEMSRC_SBRK,sbsize) ;
   FrBigAllocHdr *pred ;
   if (block >= FramepaC_sbrk_end)
      {
      pred = memory_sbrk_last ? memory_sbrk_last : &FramepaC_arenas ;
      }
   else
      {
      // the new allocation was below an existing one!  (!@$% Windows XP)
      // find the highest existing arena below the new allocation
      pred = find_predecessor(block) ;
      }
   // is the new allocation contiguous with the prior one?
   if (pred->mergeSuccessorArena(block,false))
      {
      // check whether the last block in the old arena is free;
      //   if so, merge it into the new allocation
      block = block->mergePredecessor() ;
      }
   else
      {
      // if not contiguous, set things up as a discontiguous block
      if (adjust_sbrk(block))
	 {
	 if (block)
	    block->init(MEMSRC_SBRK,sbsize) ;
	 else
	    {
	    total_allocations -= sbsize ; // couldn't allocate after all
	    return 0 ;
	    }
	 }
      // insert the block into the list of arenas
      pred->linkSuccessorArena(block) ;
      // memory_start and sbrk_last can only change when we add a new
      //   arena, so update them here instead of below with the other
      //   limits
      if (block > memory_sbrk_last)
	 memory_sbrk_last = block ;
      FramepaC_memory_start = FramepaC_arenas.nextArena() ;
      }
   // update memory limits
   FramepaC_sbrk_end = memory_sbrk_last->arenaEnd() ;
   FramepaC_memory_end = FramepaC_arenas.prevArena()->arenaEnd() ;
   return block ;
}

//----------------------------------------------------------------------

static void init_mmap_granularity()
{
#if defined(unix) || defined(__linux__) || defined(__GNUC__)
   long ps = sysconf(_SC_PAGESIZE) ;
   mmap_granularity = (ps > 0) ? (size_t)ps : FrMMAP_GRANULARITY ;
#endif /* unix */
   return ;
}

//----------------------------------------------------------------------

static FrBigAllocHdr *try_mmap(size_t mmsize, bool &use_tail, int extra_flags = 0)
{
   use_tail = false ;
   if (mmap_granularity == 0)
      init_mmap_granularity() ;
#if !defined(ALLOC_MMAP)
   (void)mmsize; (void)extra_flags;
   return 0 ;
#else
   // enforce a minimum size for mmap() requests; we'll split off any
   //   excess and use it for future memory requests
   if (mmsize < FrMMAP_ALLOCSIZE)
      mmsize = FrMMAP_ALLOCSIZE ;
   // add in some slack to allow us to align things properly
   if (mmap_granularity < FrALLOC_GRANULARITY)
      {
      mmsize = mmsize + FrALLOC_GRANULARITY ;
      }
   mmsize = round_up(mmsize,mmap_granularity) ;
   // request the memory
   FrBigAllocHdr *block =
      (FrBigAllocHdr*)ALLOC_MMAP(0, mmsize, PROT_READ|PROT_WRITE,
				 MAP_PRIVATE|extra_flags) ;
   if (block == MAP_FAILED || block == 0)
      return 0 ;
   // check the allocation's alignment
   size_t runt =
      FrALLOC_GRANULARITY - (((unsigned long)block) % FrALLOC_GRANULARITY) ;
   runt %= FrALLOC_GRANULARITY ;
   FrBigAllocHdr *aligned = (FrBigAllocHdr*)(((char*)block) + runt) ;
   bool misaligned = (runt != 0) ;
#if defined(linux) || defined(FrMMAP_MERGEABLE)
   if (misaligned)
      {
      // we can just unmap the extra pages and adjust the block pointer and size
      //   accordingly
      // With Linux 3.4, MAP_PRIVATE mappings proceed downward from high addresses,
      //   so a single unmapping will align all future maps!
      if (munmap(block,runt) == 0)
	 {
	 block = aligned ;
	 mmsize -= runt ;
	 misaligned = false ;
	 }
      }
#endif
   block->init(MEMSRC_MMAP,mmsize) ;
   total_allocations += mmsize ;
   // locate the highest arena below the allocation we just got
   FrBigAllocHdr *pred = find_predecessor(block) ;
   bool must_link = true ;
   if (merge_mmap && !misaligned)
      {
      FrBigAllocHdrBase *arena = block ;
      // try to merge the new allocation with the prior and following arenas
      (void)VALGRIND_MAKE_MEM_DEFINED(pred,sizeof(FrBigAllocHdrBase)) ;
      if (pred->memsource == MEMSRC_MMAP &&
	  pred->mergeSuccessorArena(block,false))
	 {
	 must_link = false ;		// we're in the arena chain now
	 arena = pred ;
	 // check whether the last block in the old arena is free;
	 //   if so, merge it into the new allocation
	 block = block->mergePredecessor() ;
	 }
      FrBigAllocHdr *nxt = pred->nextArena() ;
      if (nxt->memsource == MEMSRC_MMAP &&
	  arena->mergeSuccessorArena(nxt,true))
	 {
	 // check whether the first block in the old successor arena
	 //   is free; if so, merge it into the new allocation
	 block->mergeSuccessor(true,false) ;
	 // if we're expanding downward (as Linux does for mmap), the
	 //   caller should use the *end* of the returned block so
	 //   that the next expansion has a chance to merge free space
	 if (arena == block)
	    {
	    use_tail = true ;
	    }
	 }
      }
   if (must_link)
      {
      // insert the block into the list of arenas
      pred->linkSuccessorArena(block) ;
      if (FramepaC_mmap_start == 0 || block < FramepaC_mmap_start)
	 FramepaC_mmap_start = block ;
      // memory_start can only change when we add a new arena, so update
      //   it here instead of below
      FramepaC_memory_start = FramepaC_arenas.nextArena() ;
      }
   (void)VALGRIND_MAKE_MEM_NOACCESS(pred,sizeof(FrBigAllocHdrBase)) ;
   // update memory limits
   FramepaC_memory_end = FramepaC_arenas.prevArena()->arenaEnd() ;
   // if the mmap arena is not aligned with our internal page size,
   //   split off a runt block at the beginning to align all further
   //   chunks allocated out of the arena
   if (misaligned)
      {
      // fortunately, mmap guarantees page alignment, which is
      // much larger than a FrBigAllocHdr, so we can always split
//FIXME: re-align by splitting off runt block
      FrProgError("not implemented yet") ;
      (void)aligned;
      }
   return block ;
#endif /* ALLOC_MMAP */
}

//----------------------------------------------------------------------

void *big_malloc(size_t size, bool prefer_mmap)
{
   (void)prefer_mmap ;
   if (!FramepaC_bigalloc_pool.initialized())
      {
      FramepaC_bigalloc_pool.init(FrALLOC_GRANULARITY, FrALLOC_BIGMAX,
				  FrALLOC_GRANULARITY) ;
      FramepaC_arenas.initEmpty() ;
      // align the heap to our granularity; we'll just throw out the
      //   first partial block
      void *heap = sbrk(0) ;
      int partial = ((uintptr_t)heap) & (FrALLOC_GRANULARITY-1) ;
      if (partial)
	 sbrk(FrALLOC_GRANULARITY - partial) ;
      if (!memory_sbrk_last)
	 FramepaC_sbrk_end = sbrk(0) ;
      }
   size = round_up(size+sizeof(FrBigAllocHdrBase),FrALLOC_GRANULARITY) ;
   size_t realsize = size ;
   bool use_tail = false ;
   FrCRITSECT_ENTER(FramepaC_bigalloc_pool.mutex()) ;
   // grab a block of the appropriate size off of our free list, if available
   FrBigAllocHdr *block = FramepaC_bigalloc_pool.pop(size) ;
   if (!block)
      {
      // there was no free block large enough, so get some more memory
      //   from the system
      bool tried_sbrk = false ;
#if defined(ALLOC_MMAP)
      if (!prefer_mmap && size <= FrMMAP_ALLOCSIZE && // prefer mmap() for large allocs
	  total_allocations < mmap_alloc_threshold)
#endif /* ALLOC_MMAP */
	 {
	 block = try_sbrk(size) ;
	 tried_sbrk = true ;
	 }
      if (!block)
	 {
	 if (try_hugetlb)
	    block = try_mmap(size,use_tail,MAP_HUGETLB) ;
	 if (!block)
	    block = try_mmap(size,use_tail) ;
	 else if (!tried_sbrk)
	    {
	    // fall back on sbrk() allocation
	    block = try_sbrk(size) ;
	    tried_sbrk = true ;
	    }
	 if (!block)
	    {
	    errno = ENOMEM ;
	    FrCRITSECT_LEAVE(FramepaC_bigalloc_pool.mutex()) ;
	    return 0 ;		// unable to allocate more memory
	    }
	 }
      }
   (void)VALGRIND_MAKE_MEM_DEFINED(block,sizeof(FrBigAllocHdrBase)) ;
   block->suballocated = false ;
   if (use_tail)
      {
      size_t extra = block->size() - realsize ;
      FrBigAllocHdr *rest = block->splitBlock(extra,false) ;
      if (rest)
	 {
	 FramepaC_bigalloc_pool.add(block) ;
	 (void)VALGRIND_MAKE_MEM_NOACCESS(block,sizeof(FrBigAllocHdrBase)) ;
	 block = rest ;
	 }
      }
   else
      {
      block->splitBlock(realsize) ;
      }
   (void)VALGRIND_MAKE_MEM_DEFINED(block,sizeof(FrBigAllocHdrBase)) ;
   block->status = Block_In_Use ;    // no longer free
   FrCRITSECT_LEAVE(FramepaC_bigalloc_pool.mutex()) ;
   (void)VALGRIND_MAKE_MEM_UNDEFINED(block->body(),block->bodySize()) ;
   (void)VALGRIND_MAKE_MEM_NOACCESS(block,sizeof(FrBigAllocHdrBase)) ;
   VALGRIND_MALLOCLIKE_BLOCK(block->body(),block->bodySize(),0,false) ;
   return block->body() ;
}

//----------------------------------------------------------------------

static bool reclaim_mmap_block(FrBigAllocHdr *block, bool in_freelist)
{
   (void)VALGRIND_MAKE_MEM_DEFINED(block,sizeof(FrBigAllocHdrBase)) ;
   if (block->memsource != MEMSRC_MMAP)
      {
      (void)VALGRIND_MAKE_MEM_NOACCESS(block,sizeof(FrBigAllocHdrBase)) ;
      return false ;
      }
//FIXME: update checks to allow reserved alignment blocks at start and end of mmap allocation
   bool full_arena = block->first() && block->last() ;
   size_t size = block->size() ;
   size_t minsize = mmap_options[mmap_reclaim_level].reclaim_size ;
   bool reclaimable = size >= minsize ;
   // if the block is both 'first' and 'last', then it encompasses the
   //   entire mmap allocation, so we can ignore the size threshold for
   //   an explicit GC
   if (reclaimable || (/*in_freelist &&*/ full_arena))
      {
      bool can_reclaim = false ;
      if (full_arena)
	 {
	 if (block == FramepaC_mmap_start)
	    {
	    if (block == FramepaC_arenas.prevArena())
	       FramepaC_mmap_start = 0 ; // this was the only mmap arena
	    else
	       FramepaC_mmap_start = block->nextArena() ;
	    }
	 block->unlinkArena() ;
	 FramepaC_memory_end = FramepaC_arenas.prevArena()->arenaEnd() ; 
	 can_reclaim = true ;
	 }
      else if (merge_mmap)
	 {
	 if (block->first())
	    {
	    // split the arena and keep the latter part
	    if (FramepaC_mmap_start == block)
	       {
	       FramepaC_mmap_start = block->next() ;
	       }
	    (void)block->splitArena(block,true) ;
	    can_reclaim = true ;
	    }
	 else if (block->last())
	    {
	    // split the arena and keep the former part
	    (void)block->thisArena()->shrinkArena(block->prev()) ;
	    can_reclaim = true ;
	    }
	 else if (allow_fragmenting_arenas &&
		  mmap_options[mmap_reclaim_level].split_arena)
	    {
	    // The block is in the middle of the arena, so split
	    //   twice.  Unfortunately, this operation is not
	    //   constant-time, and the more subarenas are created,
	    //   the slower further splitting becomes.
	    FrBigAllocHdr *arena = block->thisArena() ;
	    // ensure that the leftover bits are big enough to bother with
	    if ((size_t)((char*)block - (char*)arena) >= minsize &&
		(size_t)((char*)arena->arenaEnd() - (char*)block->next()) >= minsize)
	       {
	       FrBigAllocHdr *rsplit = arena->splitArena(block,false) ;
	       if (rsplit)
		  {
		  arena->shrinkArena(block->prev()) ;
		  // link the portion after the block into the arena chain
		  arena->linkSuccessorArena(rsplit) ;
		  can_reclaim = true ;
		  }
	       }
	    }
	 }
      if (can_reclaim)
	 {
	 if (in_freelist)
	    {
	    FramepaC_bigalloc_pool.remove(block) ;
	    }
	 if (munmap(block,size) != 0)
	    {
	    // OOPS!
	    abort() ;
	    }
	 total_allocations -= size ;
	 // update memory limits
	 FramepaC_memory_start = FramepaC_arenas.nextArena() ;
	 FramepaC_memory_end = FramepaC_arenas.prevArena()->arenaEnd() ;
	 return true ;
	 }
      }
   bool madvisable = size >= mmap_options[mmap_reclaim_level].madvise_size ;
   // if the block is in the freelist, we are performing an explicit GC,
   //   so ignore the size threshold
   if (madvisable || in_freelist)
      {
      if (size >= 2 * mmap_granularity)
	 {
	 // tell the OS it can discard the pages of the block, except
	 //   for the first one (since it contains the still-valid
	 //   block header)
	 FrDontNeedMemory((char*)block + mmap_granularity, size - mmap_granularity) ;
	 }
      }
   (void)VALGRIND_MAKE_MEM_NOACCESS(block,sizeof(FrBigAllocHdrBase)) ;
   return false ; // no address space was returned to OS
}

//----------------------------------------------------------------------

void big_free(void *block)
{
   FrBigAllocHdr *hdr = FrBigAllocHdr::header(block) ;
#if defined(FrMEMORY_CHECKS)
   {
   size_t blocksize = hdr->size() ;
   (void)VALGRIND_MAKE_MEM_DEFINED(hdr,sizeof(FrBigAllocHdrBase)) ;
   if (hdr->status != Block_In_Use ||
       blocksize < sizeof(FrBigAllocHdr) ||
       FramepaC_invalid_address(hdr))
      {
      check_memory(block,FrFree_str,big_free_str) ;
      return ;
      }
   }
#endif /* FrMEMORY_CHECKS */
   VALGRIND_FREELIKE_BLOCK(block,0) ;
   FrCRITSECT_ENTER(FramepaC_bigalloc_pool.mutex()) ;
   (void)hdr->mergeSuccessor(true,false) ;
   hdr = hdr->mergePredecessor() ;
   (void)VALGRIND_MAKE_MEM_DEFINED(hdr,sizeof(FrBigAllocHdrBase)) ;
   hdr->status = Block_Free ;
   hdr->suballocated = false ;
   (void)VALGRIND_MAKE_MEM_NOACCESS(hdr,sizeof(FrBigAllocHdrBase)) ;
   // check whether this is a mmap area that has become completely free
   if (!reclaim_mmap_block(hdr,false))
      {
      // add block to freelist for future re-use
      FramepaC_bigalloc_pool.add(hdr) ;
      }
   FrCRITSECT_LEAVE(FramepaC_bigalloc_pool.mutex()) ;
   return ;
}

//----------------------------------------------------------------------

void *big_realloc(void *blk,size_t newsize)
{
   FrBigAllocHdr *block = FrBigAllocHdr::header(blk) ;
   if (!FramepaC_bigalloc_pool.initialized())
      {
      FramepaC_bigalloc_pool.init(FrALLOC_GRANULARITY, FrALLOC_BIGMAX,
				  FrALLOC_GRANULARITY) ;
      FramepaC_arenas.initEmpty() ;
      }
#ifdef FrMEMORY_CHECKS
   MK_VALID(*block) ;
   if (block->status != Block_In_Use ||
       FramepaC_invalid_address(block))
      {
      check_memory(blk,FrRealloc_str,big_realloc_str) ;
      errno = EFAULT ;
      return 0 ;
      }
#endif /* FrMEMORY_CHECKS */
   size_t origsize = block->size() ;
   size_t size = round_up(newsize + sizeof(FrBigAllocHdrBase),
			  FrALLOC_GRANULARITY) ;
   FrCRITSECT_ENTER(FramepaC_bigalloc_pool.mutex()) ;
   // merge block with following one, if free
   (void)block->mergeSuccessor(true,false) ;
   // check whether expanded block is big enough to hold requested size
   if (block->size() < size && block->last())
      {
      // the block is too small, but it's the last one in the arena
      size_t leftover = size - block->size() ;
      FrBigAllocHdr *newblock = 0 ;
      if (block->memsource == MEMSRC_SBRK &&
	  block >= memory_sbrk_last)
	 {	       // last sbrk() memory block?
	 // try to expand the arena containing the block
	 newblock = try_sbrk(leftover) ;
	 }
      else if (block->memsource == MEMSRC_MMAP &&
	       block >= FramepaC_arenas.prevArena())
	 {
	 // block is in the highest-addressed mmap() arena
	 bool use_tail ;
	 newblock = try_mmap(leftover,use_tail) ;
	 }
      if (newblock && !block->mergeSuccessor(false,false))
	 {
	 FramepaC_bigalloc_pool.add(newblock) ;
	 }
      }
   if (block->size() >= size)
      {
      // we've managed to make the block big enough, so trim off any excess
      block->splitBlock(size,true) ;
      VALGRIND_RESIZEINPLACE_BLOCK(blk,origsize-sizeof(FrBigAllocHdrBase),newsize,0) ;
      FrCRITSECT_LEAVE(FramepaC_bigalloc_pool.mutex()) ;
      return block->body() ;
      }
   FrCRITSECT_LEAVE(FramepaC_bigalloc_pool.mutex()) ;
   // if we get here, we were unable to resize the existing block, so allocate
   // a new one and copy over the contents
   void *newblock = big_malloc(newsize,true) ;
   if (newblock)
      {
      if (origsize > newsize)
	 origsize = newsize ;
      memcpy(newblock,block->body(),origsize) ;
      big_free(blk) ;
      }
   return newblock ;
}

//----------------------------------------------------------------------

bool big_reclaim()
{
   bool reclaimed = false ;
   FrCRITSECT_ENTER(FramepaC_bigalloc_pool.mutex()) ;
   FrBigAllocHdr *prev ;
#if defined(ALLOC_MMAP)
   // scan down the list of free blocks, looking for those which have
   //   MEMSRC_MMAP, and attempt to reclaim them
   // if big_free has been doing its job, there will be few or none to
   //   reclaim
   FrBigAllocHdr *head = FramepaC_bigalloc_pool.freelistHead() ;
   MK_VALID(head->prevfree) ;
   for (FrBigAllocHdr *block = head->prevfree ;
	block && block != head ;
	block = prev)
      {
      MK_VALID(block->prevfree) ;
      prev = block->prevfree ;
      MKNOACCESS(block->prevfree) ;
      reclaim_mmap_block(block,true) ;
      }
   MKNOACCESS(head->prevfree) ;
#endif /* ALLOC_MMAP */
   // check if we can reduce our sbrk() memory size -- requires that the
   //  highest large memory block be free
   if (memory_sbrk_last && can_return_sbrk)
      {
      FrBigAllocHdr *block = memory_sbrk_last->lastBlock() ;
      (void)VALGRIND_MAKE_MEM_DEFINED(block,sizeof(*block)) ;
      size_t size = block->size() ;
      (void)VALGRIND_MAKE_MEM_DEFINED(memory_sbrk_last,sizeof(FrBigAllocHdrBase)) ;
      if (block->status == Block_Free && size >= FrMMAP_ALLOCSIZE/2 &&
	  memory_sbrk_last->memsource == MEMSRC_SBRK)
	 {
	 FramepaC_bigalloc_pool.remove(block) ;
	 if (block->first())
	    {
	    // if the entire arena is free, release it
	    prev = memory_sbrk_last->prevArena() ;
	    memory_sbrk_last->unlinkArena();
	    memory_sbrk_last = (prev == &FramepaC_arenas) ? 0 : prev ;
	    }
	 else
	    {
	    // reduce the arena size
	    memory_sbrk_last->shrinkArena(block->prev()) ;
	    }
	 // reduce memory allocation
	 // we must check the return value because some systems (e.g. DOS/4GW)
	 // will not honor negative increments to sbrk()
	 if (sbrk(-((long int)size)) != (void*)-1)
	    {
	    // adjust the various end-of-memory markers
	    total_allocations -= size ;
	    // indicate success
	    reclaimed = true ;
	    }
	 else
	    {
	    // unable to release to OS, so don't try again
	    can_return_sbrk = false ;
	    }
	 FramepaC_memory_start = FramepaC_arenas.nextArena() ;
	 FramepaC_memory_end = FramepaC_arenas.prevArena()->arenaEnd() ;
	 }
      (void)VALGRIND_MAKE_MEM_NOACCESS(block,sizeof(*block)) ;
      }
   FrCRITSECT_LEAVE(FramepaC_bigalloc_pool.mutex()) ;
   return reclaimed ;
}

//----------------------------------------------------------------------

bool FrIterateMemoryArenas(bool (*fn)(FrBigAllocHdr*,void*),void *user_data)
{
   for (FrBigAllocHdr *arena = FramepaC_arenas.nextArena() ;
	arena != &FramepaC_arenas ;
	arena = arena->nextArena())
      {
      if (!fn(arena,user_data))
	 return false ;
      }
   return true ;
}

// end of file frmalloc.cpp //
