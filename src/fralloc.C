/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File fralloc.cpp		slab allocation routines		*/
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
//#include "frmembin.h"
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
/*	 Global Variables for this module				*/
/************************************************************************/

FrAllocator *FramepaC_allocators = nullptr ;

//----------------------------------------------------------------------
// global variables for FrAllocator

FrSlabPool FrAllocator::s_slabpools[FrTOTAL_ALLOC_LISTS] ;
FrPER_THREAD FrFreeListCL *FrAllocator::s_foreign_freelists = nullptr ;
FrPER_THREAD FrFreeList *FrAllocator::s_freelists[FrTOTAL_ALLOC_LISTS] /* = { 0 } */ ;
FrPER_THREAD uint32_t FrAllocator::s_freecounts[FrTOTAL_ALLOC_LISTS] /* = { 0 } */ ;
FrPER_THREAD FrMemFooter *FrAllocator::s_pagelists[FrTOTAL_ALLOC_LISTS] /* = { 0 } */ ;

FrThreadOnce FrAllocator_setup ;
FrThreadKey FrAllocator_key ;

// to avoid infinite regress of having to allocate memory while
//   initializing the memory allocator, pre-reserve a few threads' worth
//   of freelist heads.
#define NUM_FREELIST_BUFFERS 4
static FrFreeListCL foreign_freelist_buffers[NUM_FREELIST_BUFFERS*(FrTOTAL_ALLOC_LISTS+1)] /* = { 0 } */ ;
// the +1 above reserves a pointer for use in chaining the buffers on thread death
static FrFreeListCL *free_foreign_freelist_buffers = nullptr ;
static bool foreign_freelist_buffers_init = false ;

// end FrAllocator globals
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// global variables for FrSubAllocator

// end FrSubAllocator globals
//----------------------------------------------------------------------

/************************************************************************/
/*	Helper functions						*/
/************************************************************************/

//----------------------------------------------------------------------

static void FramepaC_auto_gc_handler_func()
{
   FrAllocator *alloc = FramepaC_allocators ;
   while (alloc)
      {
      alloc->async_gc() ;
      alloc->compact(false) ;
      alloc = alloc->nextAllocator() ;
      }
   return ;
}

//----------------------------------------------------------------------

static void FramepaC_gc_handler_func()
{
   FrAllocator *alloc = FramepaC_allocators ;
   while (alloc)
      {
      alloc->gc() ;
      FrAllocator *next = alloc->nextAllocator() ;
      alloc->compact(true) ;
      assert(next != alloc) ;
      alloc = next ; 
      }
   FrAllocator::reclaimFreeBlocks() ;
   return ;
}

void chkalloc()
{
   FrAllocator *alloc = FramepaC_allocators ;
   while (alloc)
      {
      FrAllocator *next = alloc->nextAllocator() ;
      assert(next != alloc) ;
      alloc = next ; 
      }
   return ;
}

//----------------------------------------------------------------------

static FrFreeList *release_free_items_on_empty_pages(FrFreeList *freelist, size_t objsize,
						     size_t &num_freed)
{
   FrFreeList *prev_fl = nullptr ;
   FrFreeList *next_fl = nullptr ;
   for (FrFreeList *fl = freelist ; fl ; fl = next_fl)
      {
      next_fl = fl->next() ;
      if (FrFOOTER_PTR(fl)->emptyPage(objsize))
	 {
	 num_freed++ ;
	 if (prev_fl)
	    prev_fl->next(next_fl) ;
	 else
	    freelist = next_fl ;
	 }
      else
	 prev_fl = fl ;
      }
   return freelist ;
}

//----------------------------------------------------------------------

static size_t reclaim_free_items(FrMemFooter **pages,
				 FrFreeList **freelist,
				 size_t objsize,
				 size_t &totalfree,
				 FrFreeListCL *pending_frees = 0)
{
   if (objsize == 0)
      return 0 ;
   size_t numfreed = 0 ;
   // grab the list of pending frees and add them to our local free list
   if (pending_frees)
      {
      FrFreeList *fl = pending_frees->next() ;
      pending_frees->next(0) ;
      while (fl)
	 {
	 FrFreeList *nxt = fl->next() ;
	 fl->next(*freelist) ;
	 (*freelist) = fl ;
	 fl = nxt ;
	 }
      }
   // zero out the free counts on all pages
   for (FrMemFooter *page = *pages ; page ; page = page->myNext())
      {
      page->clearFreeCount() ;
      }
   // now traverse our local free list, updating corresponding page free counts
   (void)VALGRIND_MAKE_MEM_DEFINED(freelist,sizeof(FrFreeList*)) ;
   for (FrFreeList *fl = *freelist ; fl ; fl = fl->next())
      {
      FrMemFooter *page = FrFOOTER_PTR(fl) ;
      page->incrFreeCount() ;
      totalfree++ ;
      (void)VALGRIND_MAKE_MEM_NOACCESS(fl,sizeof(*fl)) ;
      }
   // traverse the free list a second time, removing any items on
   //   pages which are completely empty
   size_t items_freed = 0 ;
   *freelist = release_free_items_on_empty_pages(*freelist,objsize,items_freed) ;
   // finally, scan the list of pages again, releasing any which are now empty
   FrMemFooter *prev = nullptr ;
   FrMemFooter *nxt = nullptr ;
   for (FrMemFooter *page = *pages ; page ; page = nxt)
      {
      nxt = page->myNext() ;
      if (page->emptyPage(objsize))
	 {
	 long count = page->freeObjects() ; 
	 numfreed++ ;
	 totalfree -= count ;
	 // unlink the page from the local list
	 if (prev)
	    prev->myNext(nxt) ;
	 else
	    (*pages) = nxt ;
	 // and return the page to the per-thread free list
	 FrMemoryPool::addFreePage(page) ;
	 }
      else
	 prev = page ;
      }
   if (numfreed)
      FrMemoryPool::reclaimFreePages() ;
   return numfreed ;
}

//----------------------------------------------------------------------

#ifdef FrMULTITHREAD
static void move_to_local_freelists(FrFreeList *freelist)
{
   if (!freelist)
      return ;
   FrFreeList *next = nullptr ;
   for (FrFreeList *fl = freelist ; fl ; fl = next)
      {
      next = fl->next() ;
      FrMemFooter::pushOnFreelist(fl) ;
      }
   return ;
}
#endif /* FrMULTITHREAD */

/************************************************************************/
/*	Methods for class FrSubAllocator				*/
/************************************************************************/

FrSubAllocator::FrSubAllocator(int size, const char *name,
			       FrCompactFunc *func)
   : FrAllocator(name,size,func,true)
{
   return ;
}

/************************************************************************/
/*	class FrAllocator						*/
/************************************************************************/

FrAllocator::FrAllocator(const char *name, int size, FrCompactFunc *func,
			 bool malloc_headers)
{
   if (size < (int)sizeof(FrFreeList))
      size = sizeof(FrFreeList) ;
   if ((unsigned)size > FrAllocator_max)
      FrProgErrorVA("excessively large object size (%d) in FrAllocator ctor",size) ;
   setName(name) ;
#ifdef FrREPLACE_MALLOC
   // round the size up to the slab allocation granularity, so that we
   //   can correctly share slabs between FrAllocator instances
   size = FrSlabPool::binSize(FrSlabPool::sizeBin(size,malloc_headers),malloc_headers) ;
#endif /* FrREPLACE_MALLOC */
   m_objsize = size ;
   m_malloc = malloc_headers ;
   m_usersize = size - (m_malloc ? sizeof(FrSubAllocOffset) : 0) ;
   setSizeBin() ;
   m_compact_func = func ;
   gc_func = nullptr ;
   async_gc_func = nullptr ;
   m_compacting = false ;
   m_total_requests = 0 ;
   FramepaC_mempool.enterCritical() ;
   // link into the list of all FrAllocator instances
   m_prev = nullptr ;
   m_next = FramepaC_allocators ;
   if (!malloc_headers)
      {
      FramepaC_allocators = this ;
      if (nextAllocator())
	 m_next->m_prev = this ;
      }
   if (!FramepaC_gc_handler)
      {
      FramepaC_gc_handler = FramepaC_gc_handler_func ;
      FramepaC_auto_gc_handler = FramepaC_auto_gc_handler_func ;
      }
   FramepaC_mempool.leaveCritical() ;
   return ;
}

//----------------------------------------------------------------------

FrAllocator::~FrAllocator()
{
   if (!m_malloc)
      {
      FramepaC_mempool.enterCritical() ;
      if (nextAllocator())
	 m_next->m_prev = prevAllocator() ;
      if (prevAllocator())
	 m_prev->m_next = nextAllocator() ;
      else
	 FramepaC_allocators = nextAllocator() ;
      FramepaC_mempool.leaveCritical() ;
      }
   return ;
}

//----------------------------------------------------------------------

void FrAllocator::releaseForeign(void *item)
{
   FrFreeList *obj = (FrFreeList*)item ;
#ifdef FrMULTITHREAD
   FrMemFooter *page = FrFOOTER_PTR(item) ;
   FrFreeList *fl = page->freelistHead() ;
   if (fl) // CAUSES MEMORY LEAK!  Band-aid to avoid crash on freeing mem allocated by dead thread
   FrMemoryPool::atomicPush(&fl->m_next,obj) ;
#else
   // we should never be called (since there's no 'foreign' free in
   //   single-threaded mode), but just in case....
   FrFreeList *fl = freelist() ;
   obj->next(fl) ;
   setFreelist(obj) ;
#endif /* FrMULTITHREAD */
   return ;
}

//----------------------------------------------------------------------

void FrAllocator::setName(const char *name)
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

void FrAllocator::setSizeBin()
{
   m_size_bin = FrSlabPool::sizeBin(objectSize(),m_malloc) ;
   s_slabpools[m_size_bin].init(objectSize(),m_malloc) ;
   return ;
}


//----------------------------------------------------------------------

void FrAllocator::threadInitOnce()
{
#ifdef FrMULTITHREAD
   FrThread::createKey(FrAllocator_key,FrAllocator::threadCleanup) ;
#endif /* FrMULTITHREAD */
   return ;
}

//----------------------------------------------------------------------

void FrAllocator::threadSetup()
{
   FrAllocator_setup.doOnce(threadInitOnce) ;
#ifdef FrMULTITHREAD
   // check whether we've set the key; if not, we haven't initialized
   //   the thread-local data yet
   if (!FrThread::getKey(FrAllocator_key))
      {
      // set a non-NULL value for the key in the current thread, so
      //   that our cleanup function gets called on thread termination
      FrThread::setKey(FrAllocator_key,s_freelists) ;
      }
#endif /* FrMULTITHREAD */
   return ;
}

//----------------------------------------------------------------------

void FrAllocator::threadCleanup(void *)
{
#ifdef FrMULTITHREAD
   size_t numfreed = 0 ;
   for (size_t i = 0 ; i < lengthof(s_pagelists) ; i++)
      {
      numfreed += reclaim(i) ;
      if (pageList(i))
	 {
	 // we still have unfreed objects on one or more pages,
	 //   so transfer those pages to the global list
	 // start by moving any remaining freelist items into the
	 //   page-local freelists
	 FrMemFooter *pages = pageList(i) ;
	 FrMemFooter *last = pages ;
	 *pageList(i) = nullptr ;
	 for (FrMemFooter *page = pages ; page ; page = page->myNext())
	    {
	    // zero out the page-local freelist on all pages
	    page->freelistHead(0) ;
	    last = page ;
	    }
	 move_to_local_freelists(s_freelists[i]) ;
	 s_freelists[i] = nullptr ;
	 // make pages visible to other threads as adoptable pages
	 s_slabpools[i].addToReclaimList(pages,last) ;
	 }
      }
   // put the array of 'foreign' freelist heads on a list for
   //   reclamation by the next thread creation; we can't free it
   //   because that could cause access to invalid memory if any
   //   blocks allocated by this thread are still in use and get freed
   //   later
//FIXME: make push thread-safe
   FrFreeListCL *buffer = s_foreign_freelists - 1 ;
   buffer->m_next = free_foreign_freelist_buffers ;
   free_foreign_freelist_buffers = buffer ;
   // run parent's cleanup routine
   FrMemoryPool::threadCleanup() ;
#endif /* FrMULTITHREAD */
   return ;
}

//----------------------------------------------------------------------

FrFreeList *FrAllocator::grabFreelist()
{
   // the freelist is per-thread, so we don't need any atomic or synchronized
   //  operations
   FrFreeList *fl = freelist() ;
   setFreelist(0) ;
   return fl ;
}

//----------------------------------------------------------------------

bool FrAllocator::reclaimPageFrees(FrMemFooter *page, size_t size_bin)
{
   // grab any list of free pages that were stuffed into this page
   //   when it was orphaned by the original owner
   FrFreeList *fl = page->grabLocalFreelist() ;
   if (fl)
      {
      // and add them to our own free list
      FrFreeList *nxt ;
      do {
         nxt = fl->next() ;
	 fl->next(freelist(size_bin)) ;
	 setFreelist(fl,size_bin) ;
	 fl = nxt ;
      } while (fl) ;
      return true ;
      }
   return false ;
}

//----------------------------------------------------------------------

FrFreeListCL *FrAllocator::foreignFreelistHead(size_t size_bin)
{
   if (!s_foreign_freelists)
      {
      if (!foreign_freelist_buffers_init)
	 {
	 for (int i = 0 ; i < NUM_FREELIST_BUFFERS ; i++)
	    {
	    FrFreeListCL *buffer = &foreign_freelist_buffers[i*(FrTOTAL_ALLOC_LISTS+1)] ;
	    buffer->m_next = free_foreign_freelist_buffers ;
	    free_foreign_freelist_buffers = buffer ;
	    }
	 foreign_freelist_buffers_init = true ;
	 }
//FIXME: make popping from freelist thread-safe
      if (free_foreign_freelist_buffers)
	 {
	 s_foreign_freelists = free_foreign_freelist_buffers ;
	 free_foreign_freelist_buffers = (FrFreeListCL*)free_foreign_freelist_buffers->m_next ;
	 }
      else
	 {
	 s_foreign_freelists = new FrFreeListCL[FrTOTAL_ALLOC_LISTS+1] ;
	 // NOTE: the above allocation will never be freed (to avoid
	 //   access to invalid memory if a memory block is freed by
	 //   another thread after the allocating thread dies), so
	 //   tell your memory-leak checker to ignore it
	 }
      if (s_foreign_freelists)
	 s_foreign_freelists += 1 ; // skip the entry used for chaining buffers
      }
   return &s_foreign_freelists[size_bin] ;
}

//----------------------------------------------------------------------

FrFreeList *FrAllocator::reclaimForeignFrees(size_t size_bin)
{
#ifdef FrMULTITHREAD
   FrFreeListCL *head = foreignFreelistHead(size_bin) ;
   FrFreeList *fl = FrMemoryPool::atomicSwap(head->m_next,(FrFreeList*)0) ;
#ifdef VALGRIND
   for (FrFreeList *f = fl ; f ; f = f->next())
      {
      VALGRIND_MEMPOOL_FREE(&s_slabpools[size_bin],f) ;
      }
#endif /* VALGRIND */
   return fl ;
#else
   (void)size_bin ;
   return 0 ;
#endif /* FrMULTITHREAD */
}

//----------------------------------------------------------------------

size_t FrAllocator::reclaimAllForeignFrees()
{
   size_t reclaimed = 0 ;
#ifdef FrMULTITHREAD
   for (size_t i = 0 ; i < FrTOTAL_ALLOC_LISTS ; i++)
      {
      FrFreeList *fl = reclaimForeignFrees(i) ;
      if (fl)
	 {
	 FrFreeList *last = fl ;
	 reclaimed++ ;
	 while (last->next())
	    {
	    last = last->next() ;
	    reclaimed++ ;
	    }
	 last->next(s_freelists[i]) ;
	 s_freelists[i] = fl ;
	 }
      }
#endif /* FrMULTITHREAD */
   return reclaimed ;
}

//----------------------------------------------------------------------

void *FrAllocator::allocate_more()
{
   FrMemFooter *newpage = nullptr ;
#ifdef FrMULTITHREAD
   // are there any pending cross-thread frees?
   FrFreeList *fl = reclaimForeignFrees(m_size_bin) ;
   if (fl)
      {
      s_freelists[m_size_bin] = fl ;
      return fl ;
      }
   // can we adopt a page that was orphaned by another thread's termination?
   while ((newpage = slabpool()->popReclaimed()) != 0)
      {
      // adopt the page
      newpage->acceptOwnership() ;
      pushPage(newpage) ;     // add to thread-local list for FrAllocator
      // chop out the page's freelist, if any
      if (reclaimPageFrees(newpage))
	 return freelist() ;
      }
#else
   FrFreeList *fl = nullptr ;
#endif /* FrMULTITHREAD */
   newpage = FrMemoryPool::popFreePage() ;
   if (!newpage)
      {
      // we didn't have a free page handy, so try to reclaim a page
      //   from one of the other slabs
//FIXME: scan other slabs for available pages
      newpage = allocate_block("FrAllocator") ;  // grab free page if avail, otherwise alloc one
      }
   // the rest of this function operates entirely on thread-local data,
   //   so no synchronization is needed
   if (newpage)
      {
      newpage->freelistHead(foreignFreelistHead(m_size_bin)) ;
      size_t objcount = 0 ;
      FrFreeList *objs = FrSlabPool::subdividePage(newpage,freelist(),objectSize(),mallocHeader(),
						   objcount) ;
      s_freecounts[m_size_bin] += objcount ;
      setFreelist(objs) ;
      pushPage(newpage) ;		    // add to thread-local list for FrAllocator
      slabpool()->addPage() ;
      }
   fl = freelist() ;
   if (!fl)
      FrNoMemoryVA("in %s::new",typeName()) ;
   return fl ;
}

//----------------------------------------------------------------------

size_t FrAllocator::objects_allocated() const
{
   size_t count = 0 ;
   for (const FrMemFooter *page = pageList() ; page ; page = page->next())
      {
      count += page->totalObjects(objectSize()) ;
      }
   return count ;
}

//----------------------------------------------------------------------

size_t FrAllocator::freelist_length() const
{
   int count = 0 ;
   FrFreeList *fl = freelist() ;
   FrFreeList *trailer = fl ;

   while (fl)
      {
      count++ ;
      fl = fl->next() ;
      if (trailer == fl)
	 (void)FrAssertionFailed("loop in free list",__FILE__,__LINE__) ;
      if (fl)
	 {
	 count++ ;
	 fl = fl->next() ;
	 if (trailer == fl)
	    (void)FrAssertionFailed("loop in free list",__FILE__,__LINE__) ;
	 trailer = trailer->next() ;
	 }
      }
   return count ;
}

//----------------------------------------------------------------------

void FrAllocator::set_gc_funcs(FrGCFunc *gc_fn, FrGCFunc *async_gc_fn)
{
   gc_func = gc_fn ;
   async_gc_func = async_gc_fn ;
   return ;
}

//----------------------------------------------------------------------

int FrAllocator::async_gc()
{
   int reclaimed(0) ;
   if (async_gc_func)
      reclaimed = async_gc_func(this) ;
   return reclaimed ;			// indicate number of objects freed
}

//----------------------------------------------------------------------

int FrAllocator::gc()
{
   int reclaimed(0) ;
   if (gc_func)
      reclaimed = gc_func(this) ;
   return reclaimed ;			// indicate number of objects freed
}

//----------------------------------------------------------------------

size_t FrAllocator::reclaim(unsigned size_bin)
{
   size_t numfreed(0) ;
   FrMemFooter *pages = pageList(size_bin) ;
   if (!pages)
      {
      // nothing to be compacted, so bail out now
      return numfreed ;
      }
   size_t totalfree(0) ;
//   size_t sz = (i >= FrFIRST_SUBALLOC_LIST)
//      ? (i-FrFIRST_SUBALLOC_LIST) * FrSUBALLOC_GRAN 
//      : i * FrALIGN_SIZE ;
   size_t freed = reclaim_free_items(&pageList(size_bin),&freelistPtr(size_bin),
				     slabpool(size_bin)->objectSize(),totalfree,
				     foreignFreelistHead(size_bin)) ;
   s_freecounts[size_bin] = totalfree ;
   numfreed += freed ;
   slabpool(size_bin)->removePages(freed) ;
   return numfreed ;
}

//----------------------------------------------------------------------

size_t FrAllocator::reclaimAll()
{
   int freed = reclaimAllForeignFrees() ;
   for (size_t i = 0 ; i < FrTOTAL_ALLOC_LISTS ; i++)
      {
      // can we adopt any pages that were orphaned by another thread's termination?
      FrMemFooter *newpage ;
      while ((newpage = slabpool(i)->popReclaimed()) != 0)
	 {
	 // adopt the page
	 newpage->acceptOwnership() ;
	 pushPage(newpage,i) ;     // add to thread-local list for FrAllocator
	 // chop out the page's freelist, if any, and add it to our own
	 reclaimPageFrees(newpage,i) ;
	 }
      // now reclaim any completely-empty pages
      freed += reclaim(i) ;
      }
   return freed ;
}

//----------------------------------------------------------------------

size_t FrAllocator::compact(bool force)
{
   if (m_compacting)	// avoid reentering ourself if the user's compaction
      return 0 ;	//   function causes another compaction
   if (objectSize() < sizeof(FrFreeList) ||
       (m_malloc && objectSize() < sizeof(FrFreeList) + sizeof(FrSubAllocOffset)))
      return 0 ;			// sorry, can't compact -- too small
   m_compacting = true ;
   size_t numfreed(0) ;
   size_t totalfree = freelist_length() ;

   // only do actual compaction if there are enough free items that we
   // can free a page after compacting the cells which are in use --
   // unless we've been told to force the compaction (e.g. the
   // compaction function frees some items)
   if (m_compact_func)
      {
      if (force || totalfree >= (size_t)(FrFOOTER_OFS/objectSize()))
	 {
	 FrFreeList *fl = freelist() ;
	 FrMemFooter *freedpages = nullptr ;
	 m_compact_func(pageList(),&fl,&freedpages);
	 setFreelist(fl) ;
	 // flag the freed page(s) for reclamation
	 for (FrMemFooter *page = freedpages ; page ; page = page->next())
	    {
	    page->markEmpty() ;
	    }
	 // scan the item free list and remove those on pages we're
	 //   about to eliminate
	 setFreelist(release_free_items_on_empty_pages(freelist(),objectSize(),numfreed)) ;
	 // any pages to be freed get put back on the shared free list
	 while (freedpages)
	    {
	    numfreed++ ;
	    FrMemFooter *curr = freedpages ;
	    freedpages = freedpages->next() ;
	    FrMemoryPool::addFreePage(curr) ;
	    }
	 slabpool()->removePages(numfreed) ;
	 }
      }
   m_compacting = false ;
   return numfreed ;
}

//----------------------------------------------------------------------

size_t FrAllocator::reclaimFreeBlocks()
{
   size_t reclaimed(0) ;
   FrMemFooter *pages = FrMemoryPool::grabAllFreePages() ;
   FrMemFooter *last = pages ;
   if (pages)
      {
      reclaimed = 1 ;
      while (last->next())
	 {
	 last = last->next() ;
	 reclaimed++ ;
	 }
      FrMemoryPool::addGlobalFreePages(pages,last) ;
      }
   return reclaimed ;
}

//----------------------------------------------------------------------

void FrAllocator::preAllocate()
{
   (void)allocate_more() ;
   return ;
}

//----------------------------------------------------------------------

bool FrAllocator::iterateVA(FrMemIterFunc *func, va_list args)
{
   if (!func)
      return true ;			// trivially successful
#ifdef FrMEMWRITE_CHECKS
   // all the free items have been given a red-zone marker when 
   //   FrMEMWRITE_CHECKS is enabled; this allows us to scan the
   //   memory pages and pick out the allocated items based on
   //   a missing red-zone marker
   bool success = true ;
   for (const FrMemFooter *page = pageList() ; page ; page = page->next())
      {
      const char *last = ((char*)page) - objectSize() ;
      for (char *obj = page->objectStartPtr() ; obj <= last ; obj += objectSize())
	 {
	 if (!((FrFreeList*)obj)->markerIntact())
	    {
	    FrSafeVAList(args) ;
	    success = func(obj,FrSafeVarArgs(args)) ;
	    FrSafeVAListEnd(args) ;
	    if (!success)
	       break ;
	    }
	 }
      }
   return success ;
#else
   (void)args ;
   return false ;
#endif /* FrMEMWRITE_CHECKS */
}

//----------------------------------------------------------------------

bool FrAllocator::iterate(FrMemIterFunc *func, ...)
{
   va_list args ;
   va_start(args,func) ;
   bool success = this ? iterateVA(func,args) : true ;
   va_end(args) ;
   return success ;
}

// end of file fralloc.cpp //
