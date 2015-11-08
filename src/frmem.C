/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frmem.cpp		memory allocation routines		*/
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

#if __GNUC__ >= 3
namespace std {
#include <sys/types.h>		    	// needed by new.h
}
using namespace std ;
#endif /* __GNUC__ >= 3 */

#include <errno.h>
#include <memory.h>		        // for GCC 4.3
#include "frballoc.h"
#include "fr_mem.h"
#include "frmembin.h"			// template FrMemoryBinnedFreelist
#include "framerr.h"
#ifndef NDEBUG
//#  define NDEBUG			// comment out to enable assertions
#endif /* !NDEBUG */
#include "frassert.h"
#include "frlru.h"
#include "frprintf.h"
#include "memcheck.h"

#ifdef FrSTRICT_CPLUSPLUS
#  include <cstdio>
#  include <cstdlib>
#  include <new>
#  include <string>			// needed on SunOS, others?
#else
#  include <new.h>
#  include <stdio.h>
#  include <stdlib.h>
#  include <string.h>			// needed on SunOS, others?
#endif /* FrSTRICT_CPLUSPLUS */

#ifndef NDEBUG
# undef _FrCURRENT_FILE
static const char _FrCURRENT_FILE[] = __FILE__ ; // save memory
#endif /* NDEBUG */

// if frmem.h redefines the following, undefine them
#undef FrMalloc
#undef FrCalloc
#undef FrRealloc
#undef FrFree

/************************************************************************/
/*	Readability macros						*/
/************************************************************************/

/************************************************************************/
/*	Portability definitions						*/
/************************************************************************/

// BorlandC uses a weird type for the __new_handler pointer
// and also calls it by a slightly different name than GNU C++ v2.5.8
#ifdef __BORLANDC__
typedef pvf new_handler_t ;
#define __new_handler _new_handler

// Watcom C++ uses yet another type for the new handler....
#elif defined(__WATCOMC__)
typedef PFV new_handler_t ;

#elif defined(_MSC_VER)
typedef _PNH new_handler_t ;

// newer versions of GNU C++ 2.x switched types on us, then 3.0 went back....
#elif (__GNUC__ == 2 && __GNUC_MINOR__ >= 70 && __GNUC_MINOR__ < 95)
typedef fvoid_t *new_handler_t ;

#elif defined(__GNUC__)
typedef void (*new_handler_t)() ;

// if not Borland/Watcom/Gnu, define the new handler's type from scratch
#else
typedef void (*new_handler_t)() ;
#endif /* __BORLANDC__ ... */

// Porting between different behavior handling lack of memory:
// BorlandC's builtin ::new loops until either _new_handler is 0 or
// the memory allocation succeeds (if _new_handler is 0, it returns 0).
// Gnu C++ 2.5.8's builtin ::new merely calls the _new_handler and
// returns whatever it happens to return.
#ifdef __GNUC__
void *dummy_new_handler() { return 0 ; }
#  define FR_NEW_HANDLER ((new_handler_t)dummy_new_handler)
#else
#  define FR_NEW_HANDLER ((new_handler_t)0)
#endif

#ifdef FrREPLACE_MALLOC
//  inline new_handler_t set_new_handler(new_handler_t handler)
//     { (void)handler ; return 0 ; }
#elif defined(_MSC_VER)
  inline new_handler_t set_new_handler(new_handler_t handler)
     { return (new_handler_t)_set_new_handler((_PNH)handler) ; }
#elif defined(FrNEW_THROWS_EXCEPTIONS)
  // won't be using set_new_handler....
#elif !defined(__WATCOMC__)
  inline new_handler_t set_new_handler(new_handler_t handler)
     {
     new_handler_t orig_handler = __new_handler ;
     __new_handler = handler ;
     return orig_handler ;
     }
#endif /* !__WATCOMC__ */

/************************************************************************/
/*	Manifest constants for this module				*/
/************************************************************************/

/************************************************************************/
/*	 Global Data for this module					*/
/************************************************************************/

if_MULTITHREAD(size_t FrCriticalSection::s_collisions = 0 ;)

#if defined(FrMEMORY_CHECKS) && defined(FrPOINTERS_MUST_ALIGN)
static const char misaligned_str[] = "(misaligned)" ;
#endif /* FrMEMORY_CHECKS && FrPOINTERS_MUST_ALIGN */

/************************************************************************/
/*	 Global Variables for this module				*/
/************************************************************************/

bool memory_errors_are_fatal = FrMEM_ERRS_FATAL ;

int FramepaC_symbol_blocks = 0 ;
FrMemoryPool *FramepaC_memory_pools = 0 ;

//----------------------------------------------------------------------
// global variables for FrSlabPool

// end FrSlabPool globals
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// global variables for FrMalloc

FrMemoryPool FramepaC_mempool("FrMalloc") ;

// end FrMalloc globals
//----------------------------------------------------------------------

/************************************************************************/
/*	Forward declarations						*/
/************************************************************************/

/************************************************************************/
/*	Specializations of template FrMemoryBinnedFreelist		*/
/************************************************************************/

template <>
size_t FrMemPoolFreelist::blockSize(const FrFreeHdr *block)
{
   MK_VALID(HDR(block).size) ;
   return HDRBLKSIZE(block) ;
}

//----------------------------------------------------------------------

template <>
size_t FrMemPoolFreelist::blockBin(const FrFreeHdr *block)
{
#ifdef NVALGRIND
   return bin(HDRBLKSIZE(block)) ;
#else
   MK_VALID(HDR(block).size) ;
   size_t b = bin(HDRBLKSIZE(block)) ;
   MKNOACCESS(HDR(block).size) ;
   return b ;
#endif /* NVALGRIND */
}

//----------------------------------------------------------------------

template <>
void FrMemPoolFreelist::markAsUsed(FrFreeHdr *block)
{
   HDRMARKUSED(block) ;
   return ;
}

//----------------------------------------------------------------------

template <>
void FrMemPoolFreelist::markAsFree(FrFreeHdr *block)
{
   MK_VALID(HDR(block).size) ;
   HDRMARKFREE(block) ;
   MKNOACCESS(HDR(block).size) ;
   return ;
}

//----------------------------------------------------------------------

template <>
bool FrMemPoolFreelist::blockInUse(const FrFreeHdr *block)
{
   MK_VALID(HDR(block).size) ;
   return HDRBLKUSED(block) != 0 ;
}

/************************************************************************/
/*	FramepaC equivalents for standard malloc() family		*/
/************************************************************************/

void *FrMalloc(size_t size)
{
#ifdef PURIFY
   return ::malloc(size) ;
#else
   return FramepaC_mempool.allocate(size) ;
#endif /* PURIFY */
}

//----------------------------------------------------------------------

void *_FrMallocDebug(size_t size, const char *file, size_t line)
{
   void *blk = FrMalloc(size) ;
   if (!blk)
      FrWarningVA("out of memory at line %lu of %s",(unsigned long)line,file) ;
   return blk ;
}

//----------------------------------------------------------------------

void *FrCalloc(size_t nitems, size_t size, FrMemoryPool *mp)
{
#ifdef PURIFY
   (void)mp ;
   return ::calloc(nitems,size) ;
#else
   size_t blocksize = nitems*size ;
   void *block = mp->allocate(blocksize) ;

   if (block)
      {
      memset(block,'\0',blocksize) ;
      }
   return block ;
#endif /* PURIFY */
}

//----------------------------------------------------------------------

void *FrCalloc(size_t nitems, size_t size)
{
#ifdef PURIFY
   return ::calloc(nitems,size) ;
#else
   size_t blocksize = nitems*size ;
   void *block = FramepaC_mempool.allocate(blocksize) ;
   if (block)
      {
      memset(block,'\0',blocksize) ;
      }
   return block ;
#endif /* PURIFY */
}

//----------------------------------------------------------------------

void *_FrCallocDebug(size_t n,size_t size, const char *file, size_t line)
{
   void *blk = FrCalloc(n,size) ;
   if (!blk)
      FrWarningVA("out of memory at line %lu of %s",(unsigned long)line,file) ;
   return blk ;
}

//----------------------------------------------------------------------

void *FrRealloc(void *block, size_t newsize, bool copydata)
{
#ifdef PURIFY
   (void)copydata ;
   return ::realloc(block,newsize) ;
#else
   if (!FramepaC_mempool.initialized())
      FramepaC_mempool.init("FrMalloc") ;
   return FramepaC_mempool.reallocate(block,newsize,copydata) ;
#endif /* PURIFY */
}

//----------------------------------------------------------------------

void *FrRealloc(void *block, size_t newsize, bool copydata, FrMemoryPool *mp)
{
#ifdef PURIFY
   (void)copydata ;
   (void)mp ;
   return ::realloc(block,newsize) ;
#else
   return mp->reallocate(block,newsize,copydata) ;
#endif /* PURIFY */
}

//----------------------------------------------------------------------
// support for Purify builds

void *realloc3(void *block, size_t newsize, bool copydata)
{
   (void)copydata ;
   return realloc(block,newsize) ;
}

//----------------------------------------------------------------------

void *_FrReallocDebug(void *blk, size_t size, bool copy,
		      const char *file, size_t line)
{
   void *new_blk = FrRealloc(blk,size,copy) ;
   if (!new_blk)
      FrWarningVA("out of memory at line %lu of %s",(unsigned long)line,file) ;
   return new_blk ;
}

//----------------------------------------------------------------------

void FrFree(void *block)
{
//it's an error to ever call FrFree before the first FrMalloc call
//   if (!FramepaC_mempool.initialized())
//      FramepaC_mempool.init("FrMalloc") ;
   return FramepaC_mempool.release(block) ;
}

//----------------------------------------------------------------------

void _FrFreeDebug(void *blk, const char *file, size_t line)
{
   (void)file; (void)line;
   FrFree(blk) ;
   return ;
}

//----------------------------------------------------------------------

bool FramepaC_memory_freelist_OK()
{
   bool OK = true ;
#ifdef FrREPLACE_MALLOC
   extern bool FramepaC_bigmalloc_freelist_OK() ;
   OK = FramepaC_bigmalloc_freelist_OK() ;
#endif /* FrREPLACE_MALLOC */
   return OK ;
}

//----------------------------------------------------------------------

bool FramepaC_memory_chain_OK()
{
#ifdef FrREPLACE_MALLOC
   extern bool FramepaC_bigmalloc_chain_OK() ;
   if (!FramepaC_bigmalloc_chain_OK())
      return false ;
#endif /* FrREPLACE_MALLOC */
   return FramepaC_memory_freelist_OK() ;
}

/************************************************************************/
/*	big-allocation functions when *not* replacing system malloc()	*/
/************************************************************************/

#if !defined(FrREPLACE_MALLOC) && !defined(PURIFY)

void *big_malloc_sys(size_t size, bool)
{
   void *blk ;
   size_t align = 512 ;
   if (size > 512)
      align = FrALLOC_GRANULARITY ;
   int errcode = posix_memalign(&blk, align, size) ;
   return errcode == 0 ? blk : 0 ; 
}

void big_free_sys(void *blk)
{ 
   ::free(blk) ;
   return ;
}
#endif /* !FrREPLACE_MALLOC && !PURIFY */

/************************************************************************/
/*	Configuration Functions						*/
/************************************************************************/

unsigned FrMallocReclaimLevel()
{
#ifdef FrREPLACE_MALLOC
   unsigned FrMallocReclaimLevel__internal() ;
   return FrMallocReclaimLevel__internal() ;
#else
   return 0 ;
#endif /* FrREPLACE_MALLOC */
}

//----------------------------------------------------------------------

unsigned FrMallocReclaimLevelMax()
{
#ifdef FrREPLACE_MALLOC
   unsigned FrMallocReclaimLevelMax__internal() ;
   return FrMallocReclaimLevelMax__internal() ;
#else
   return 0 ;
#endif /* FrREPLACE_MALLOC */
}

//----------------------------------------------------------------------

bool FrMallocReclaimLevel(unsigned level)
{
#ifdef FrREPLACE_MALLOC
   unsigned FrMallocReclaimLevel__internal(unsigned) ;
   return FrMallocReclaimLevel__internal(level) ;
#else
   (void)level ;
   return false ;
#endif /* FrREPLACE_MALLOC */
}

//----------------------------------------------------------------------

bool FrMallocAllowFragmentingArenas(bool allow)
{
#ifdef FrREPLACE_MALLOC
   bool FrMallocAllowFragmentingArenas__internal(bool allow) ;
   return FrMallocAllowFragmentingArenas__internal(allow) ;
#else
   (void)allow ;
   return false ;
#endif /* FrREPLACE_MALLOC */
}

/************************************************************************/
/*	Methods for class FrFreeList					*/
/************************************************************************/

size_t FrFreeList::listlength() const
{
   size_t count = 0 ;
   const FrFreeList *fl = this ;
   while (fl)
      {
      count++ ;
      fl = fl->next() ;
      }
   return count  ;
}

/************************************************************************/
/*	Methods for class FrMemFooter					*/
/************************************************************************/

void FrMemFooter::freelistHead(FrFreeList *fl)
{
   FrFreeList **f = (FrFreeList**)arenaStartPtr() ;
   MK_VALID(*f) ;
   *f = fl ;
   MKNOACCESS(*f) ;
   return ;
}

//----------------------------------------------------------------------

size_t FrMemFooter::listlength() const
{
   size_t count = 0 ;
   const FrMemFooter *fl = this ;
   while (fl)
      {
      count++ ;
      fl = fl->next() ;
      }
   return count  ;
}

//----------------------------------------------------------------------

size_t FrMemFooter::listlengthlocal() const
{
   size_t count = 0 ;
   const FrMemFooter *fl = this ;
   while (fl)
      {
      count++ ;
      fl = fl->myNext() ;
      }
   return count  ;
}

//----------------------------------------------------------------------

void FrMemFooter::pushOnFreelist(FrFreeList *item)
{
   FrMemFooter *page = FrFOOTER_PTR(item) ;
   item->next(page->localFreelist()) ;
   page->freelistHead(item) ;
   return ;
}

//----------------------------------------------------------------------

FrFreeList *FrMemFooter::grabLocalFreelist()
{
   FrFreeList *fl = localFreelist() ;
   freelistHead(0) ;
   return fl ;
}

/************************************************************************/
/*	Methods for class FrSlabPool					*/
/************************************************************************/

unsigned FrSlabPool::sizeBin(FrSubAllocOffset objsize, bool malloc)
{
   if (malloc)
      return FrFIRST_SUBALLOC_LIST + ((objsize + FrSUBALLOC_GRAN - 1) / FrSUBALLOC_GRAN) ;
   else
      return (objsize + FrALIGN_SIZE - 1) / FrALIGN_SIZE ;
}

//----------------------------------------------------------------------

unsigned FrSlabPool::binSize(unsigned bin_num, bool malloc)
{
   if (malloc)
      {
      return FrSUBALLOC_GRAN * (bin_num - FrFIRST_SUBALLOC_LIST) ;
      }
   else
      {
      return FrALIGN_SIZE * bin_num ;
      }
}

//----------------------------------------------------------------------

void FrSlabPool::init(FrSubAllocOffset objsize, bool malloc_header)
{
#ifdef VALGRIND
   if (m_objsize == 0)
      {
      VALGRIND_CREATE_MEMPOOL(this,0,0) ;
      }
#endif /* VALGRIND */
   m_objsize = objsize ;
   m_malloc = malloc_header ;
   m_size_bin = sizeBin(objsize,malloc_header) ; // field may not be needed
   return ;
}

//----------------------------------------------------------------------

FrSlabPool::~FrSlabPool()
{
#ifdef VALGRIND
   if (m_objsize > 0)
      {
// don't tell VG we're destroying the memory pool, because some global
//   objects end up freeing memory blocks in their dtors....
//      VALGRIND_DESTROY_MEMPOOL(this) ;
      }
#endif /* VALGRIND */
   m_objsize = 0 ;
   m_size_bin = 0 ;
   return ;
}

//----------------------------------------------------------------------

const char *FrSlabPool::typeName() const
{
   static char namebuf[40] ;
   
   if (objectSize() == 0)
      strcpy(namebuf,"Unused") ;
   else if (m_malloc)
      Fr_sprintf(namebuf,sizeof(namebuf),"FrMalloc <= %d",
		 (int)(m_objsize - sizeof(FrSubAllocOffset))) ;
   else
      Fr_sprintf(namebuf,sizeof(namebuf),"FrAllocator(%d)",
		 m_objsize) ;
   return namebuf ;
}

//----------------------------------------------------------------------

void FrSlabPool::addToReclaimList(FrMemFooter *pages, FrMemFooter *last)
{
   (void)pages; (void)last ;
   if_MULTITHREAD(FrMemoryPool::atomicPush(&m_reclaimlist,pages,last) ;)
   return ;
}

//----------------------------------------------------------------------

FrMemFooter *FrSlabPool::popReclaimed()
{
#ifdef FrMULTITHREAD
   return FrMemoryPool::atomicPop(&m_reclaimlist) ;
#else
   return 0 ;
#endif /* FrMULTITHREAD */
}

//----------------------------------------------------------------------
// thread-safety note: caller must ensure serialized access to
//   'freelist' if it is not thread-local

static FrFreeList *divide_malloc_block(FrMemFooter *page,
				       FrFreeList *freelist, size_t size,
				       size_t &objcount)
{
   // build a new free list inside the array we just allocated, adding it to
   //   the front of the existing free list
   FrFreeList *last = (FrFreeList*)(((char*)page)-2*size) ;
   FrFreeList *fr ;
   FrSubAllocOffset netsize = size - sizeof(FrSubAllocOffset) ;
   for (fr = (FrFreeList*)page->objectStartPtr() ; fr <= last ; fr = fr->next())
      {
      objcount++ ;
      fr->next((FrFreeList*)(((char*)fr)+size)) ;
      MEMWRITE_CLEAR(fr) ;		// ensure proper memory init for
      					// running with FrMEMWRITE_CHECK or
      					// under Purify/Valgrind
      MK_VALID(HDR(fr).size) ;
      HDRSETSIZE(fr,netsize) ;
      }
   MK_VALID(HDR(fr).size) ;
   HDRSETSIZEUSED(fr,netsize) ;
   objcount++ ;
   fr->next(freelist) ;
   MEMWRITE_CLEAR(fr) ;			// ensure proper memory init
   // set up the Valgrind invariant that the superblock(s) for a
   //   memory pool are NOACCESS
   (void)VALGRIND_MAKE_MEM_NOACCESS(page->arenaStartPtr(),page->totalBytes()+sizeof(FrMemFooter)) ;
   return (FrFreeList*)page->objectStartPtr() ;
}

//----------------------------------------------------------------------
// thread-safety note: caller must ensure serialized access to
//   'freelist' if it is not thread-local

static FrFreeList *divide_obj_block(FrMemFooter *page,
				    FrFreeList *freelist, size_t size,
				    size_t &objcount)
{
   // build a new free list inside the array we just allocated, adding it to
   //   the front of the existing free list
   FrFreeList *last = (FrFreeList*)(((char*)page)-2*size) ;
   FrFreeList *fr ;
   for (fr = (FrFreeList*)page->objectStartPtr() ; fr <= last ; fr = fr->next())
      {
      objcount++ ;
      fr->next((FrFreeList*)(((char*)fr)+size)) ;
      MEMWRITE_CLEAR(fr) ;		// ensure proper memory init for
      					// running with FrMEMWRITE_CHECK or
      					// under Purify/Valgrind
      }
   objcount++ ;
   fr->next(freelist) ;
   MEMWRITE_CLEAR(fr) ;			// ensure proper memory init
   // set up the Valgrind invariant that the superblock(s) for a
   //   memory pool are NOACCESS
   (void)VALGRIND_MAKE_MEM_NOACCESS(page->arenaStartPtr(),page->totalBytes()+sizeof(FrMemFooter)) ;
   return (FrFreeList*)page->objectStartPtr() ;
}

//----------------------------------------------------------------------
// thread-safety note: caller must ensure serialized access to
//   'freelist' if it is not thread-local

FrFreeList *FrSlabPool::subdividePage(FrMemFooter *page, FrFreeList *freelist,
				      FrSubAllocOffset objsize, bool malloc_header,
				      size_t &objcount)
{
   size_t size_bin = sizeBin(objsize,malloc_header) ;
   objsize = binSize(size_bin,malloc_header) ;
   unsigned hdrsize = malloc_header ? sizeof(FrSubAllocOffset) : 0 ;
   unsigned align = FrALIGN_SIZE ;
   unsigned netsize = objsize - hdrsize ;
#if FrALIGN_SIZE_PTR < FrALIGN_SIZE
   if (netsize < align)
      align = FrALIGN_SIZE_PTR ;
#else // allow 'int' alignment for very small items
   if (netsize < align && netsize <= sizeof(int))
      align = sizeof(int) ;
#endif
   // adjust the object-start pointer to accomodate the malloc header, if present
   unsigned objstart = round_up(page->objectStart() + hdrsize,align) ;
   page->setObjectStart(objstart) ;
   // reduce cache-line collisions by using the internal fragmentation to shift
   //   the locations of the objects in different blocks
   unsigned unavail = objsize ? page->availableBytes() % objsize : 0 ; // internal fragmentation
   if (unavail)
      {
      unsigned jitter = (((unsigned long)page) / FrALLOC_GRANULARITY * 2) % unavail ; 
      // ensure proper alignment of allocated objects
      jitter = round_up(jitter,align) ;
      // record the starting address we used for future accesses
      page->setObjectStart(page->objectStart() + jitter) ;
      }
   if (malloc_header)
      return divide_malloc_block(page, freelist, objsize, objcount) ;
   else
      return divide_obj_block(page, freelist, objsize, objcount) ;
}

// end of file frmem.cpp //
