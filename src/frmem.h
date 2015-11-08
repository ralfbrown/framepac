/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frmem.h		memory allocation routines		*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,1998,1999,2000,2003,2004,2005,	*/
/*		2006,2007,2008,2009,2010,2012,2013,2014,2015		*/
/*	   Ralf Brown/Carnegie Mellon University			*/
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

#ifndef __FRMEM_H_INCLUDED
#define __FRMEM_H_INCLUDED

#ifndef __FRCOMMON_H_INCLUDED
#include "frcommon.h"
#endif

#ifndef __FRTHREAD_H_INCLUDED
#include "frthread.h"
#endif

#include "framerr.h"
#include <stdlib.h>

#include "memcheck.h"

/************************************************************************/
/*    Manifest constants						*/
/************************************************************************/

#ifdef __GNUC__
//#define FrTHREE_ARG_NEW	// this version of G++ supports three-arg new[]
#endif

// we allocate a bunch of FrCons, FrFrame, FrSlot, FrFacet, etc. objects at
// a time in order to reduce the overhead (in both memory and time)
// of making individual requests to the system memory allocation routines
// for each object.  Up to a point, bigger blocks save time and memory
// overhead, but excessive values can waste both time and memory preallocating
// objects which are never used.
//
// The suballocation blocks will be allocated in such a manner that
// their *ends* are aligned at the given granularity; a field in the
// control structure will allow unaligned starts to accomodate
// memory-allocator overhead and partial blocks resulting from
// unaligned allocations returned by the system allocator.
//
// The blocks are smaller for MS-DOS than other OSs because of the restricted
// amount of memory available in MS-DOS's sub-megabyte address space.
//
// (with the current implementation, 4080 is an exact multiple of the
//  size of the object types FrCons/FrSlot/FrFacet/FrString/FrFloat
//  under MS-DOS)
//
// Note: various code requires that the alignment sizes be powers of two, and
// that the block size be a multiple of FrALIGN_SIZE.
//
#if defined(_MSC_VER)   // Visual C++
#  define FrALIGN_SIZE	   4		// strictest alignment of any type
#  define FrALIGN_SIZE_PTR 4		// alignment size for pointers
#  define FrALIGN_SIZE_SBRK 131072UL	// allocation granularity for sbrk()
#  define FrALLOC_GRANULARITY 16384	// allocation granularity of big_malloc
#elif defined(__386__)    // x86, primarily Watcom C++32
#  if defined(__WINDOWS__) || defined(__NT__)
#    define FrALIGN_SIZE_SBRK 131072UL	// allocation granularity for sbrk()
#  else
#    define FrALIGN_SIZE_SBRK 4096	// allocation granularity for sbrk()
#  endif
#  if defined(__linux__)
#    define FrMMAP_ALLOCSIZE (4*1024*1024UL)
#    define FrMMAP_MERGEABLE		// anonymous mmap areas can be merged/subdivided
#  endif /* __linux__ */
#  if defined(__886__)
#    define FrALIGN_SIZE	8	// strictest alignment of any type
#    define FrALIGN_SIZE_PTR	8	// alignment size for pointers
#    define FrALLOC_GRANULARITY 65536	// allocation granularity of big_malloc
#    define FrALLOC_PAGESIZE 65536	// size of pages used for suballocation
#    define FrALLOC_BIGMAX  (1UL << 30) // biggest block we ordinarily allocate (1GB)
#  else
#    define FrALIGN_SIZE	4	// strictest alignment of any type
#    define FrALIGN_SIZE_PTR 	4	// alignment size for pointers
#    define FrALLOC_GRANULARITY 16384	// allocation granularity of big_malloc
#  endif /* __886__ */
#elif defined(__MSDOS__)  // 16-bit DOS compiler
#  define FrALIGN_SIZE	   4		// strictest alignment of any type
#  define FrALIGN_SIZE_PTR 4		// alignment size for pointers
#  define FrALLOC_PAGESIZE 4096		// small pages due to small memory
#elif defined(__linux__)  // Linux on x86
#  define FrALIGN_SIZE	   4		// strictest alignment of any type
#  define FrALIGN_SIZE_PTR 4		// alignment size for pointers
//#  define FrALIGN_SIZE_SBRK 4096	// allocation granularity for sbrk()
#  define FrALLOC_GRANULARITY 16384	// allocation granularity for big_malloc
#elif defined(__alpha__)
#  define FrALIGN_SIZE	   8		// strictest alignment of any type
#  define FrALIGN_SIZE_PTR 8		// alignment size for pointers
//#  define FrALIGN_SIZE_SBRK 4096	// allocation granularity for sbrk()
#  define FrALLOC_GRANULARITY 16384	// allocation granularity for big_malloc
#else			  // conservative, for generic platform
#  define FrALIGN_SIZE	   8		// SPARC needs doubles on 8-byte bound
#  define FrALIGN_SIZE_PTR 4		// pointers are OK on 4-byte boundaries
#  define FrPOINTERS_MUST_ALIGN		// data accesses must be aligned
#  define FrALLOC_GRANULARITY 16384	// allocation granularity for big_malloc
#endif

#ifndef FrMAX_AUTO_SUBALLOC
#  define FrMAX_AUTO_SUBALLOC (32*FrALIGN_SIZE)
#endif

#define FrMAX_MEMPOOL_NAME 32

//-------------------
// set default values
//-------------------

// if the sbrk() granularity has not been explicitly set above, use the
// default value, which is the strictest type-alignment size
//
#ifndef FrALIGN_SIZE_SBRK
#  if FrALIGN_SIZE >= 512
#    define FrALIGN_SIZE_SBRK FrALIGN_SIZE
#  else
#    define FrALIGN_SIZE_SBRK 512
#  endif
#endif

#ifndef FrALLOC_GRANULARITY
#  define FrALLOC_GRANULARITY 4096	// granularity of big_malloc() for sbrk
#endif

#ifndef FrALLOC_PAGESIZE
#  define FrALLOC_PAGESIZE 16384	// size of pages used for suballocation
#endif

#ifndef FrSBRK_ALLOCSIZE		// minimum size fo ask for in sbrk()
#  define FrSBRK_ALLOCSIZE (4*FrALLOC_GRANULARITY)
#endif

#ifndef FrMMAP_ALLOCSIZE		// minimum size to ask for in mmap()
#  define FrMMAP_ALLOCSIZE (4*1024*1024)
#endif

#ifndef FrMMAP_GRANULARITY
#  if FrALLOC_GRANULARITY > 16384
#     define FrMMAP_GRANULARITY FrALLOC_GRANULARITY
#else
#     define FrMMAP_GRANULARITY 16384
#  endif
#endif

// if mmap areas can't be split and merged, ensure that we allocate in
//   large enough chunks to keep arena-bookkeeping overhead low
#ifndef FrMMAP_MERGEABLE
#  if FrMMAP_ALLOCSIZE < (32*1024*1024)
#     undef FrMMAP_ALLOCSIZE
#     define FrMMAP_ALLOCSIZE (32*1024*1024)
#  endif
#endif

// Set the size of the biggest block we expect to allocate with big_malloc
//   (we won't fail if this is too small, but we will have to search more
//   blocks in the highest bin).  Smaller values mean less freelist
//   maintenance overhead.
#ifndef FrALLOC_BIGMAX
#  define FrALLOC_BIGMAX   (1UL << 27)	   // 128MB
#endif

#define FrAllocator_max ((FrFOOTER_OFS / 4) - 1)

//-------------------
// sanity checks
//-------------------

#if FrALLOC_GRANULARITY % FrALIGN_SIZE != 0
#  error FrALLOC_GRANULARITY must be a multiple of the alignment size
#endif

#if FrMMAP_ALLOCSIZE % 32768 != 0
#  error FrMMAP_ALLOCSIZE must be a multiple of 32K
#endif

#if FrMMAP_GRANULARITY % 4096 != 0
#  error FrMMAP_GRANULARITY must be a multiple of 4K
#endif

#if FrMMAP_GRANULARITY % FrALLOC_GRANULARITY != 0
#  error FrMMAP_GRANULARITY must be a multiple of the allocation granularity
#endif

//----------------------------------------------------------------------
// *** nothing below this point should require reconfiguration ***

#define FrSUBALLOC_GRAN (FrALIGN_SIZE)
#define FrAUTO_SUBALLOC_BINS (FrMAX_AUTO_SUBALLOC/FrSUBALLOC_GRAN)

#define FrFIRST_SUBALLOC_LIST (FrAllocator_max/FrALIGN_SIZE+1)
#define FrTOTAL_ALLOC_LISTS (FrFIRST_SUBALLOC_LIST+FrAUTO_SUBALLOC_BINS+1)

// convert an arbitrary suballocated object's address into a pointer
//   to the granularity-aligned memory block containing it
#define FrBLOCK_PTR(addr) ((char*)(((unsigned long)(addr)) & ~(unsigned long)(FrALLOC_GRANULARITY-1)))
// return a pointer to the block footer for the block containing an
//   arbitrary suballocated object
#define FrFOOTER_OFS (FrALLOC_GRANULARITY - sizeof(FrMemFooter))
#define FrFOOTER_PTR(addr) ((FrMemFooter*)(FrBLOCK_PTR(addr) + FrFOOTER_OFS))

#define FrPAGE_OFFSET(obj) (((unsigned long)(obj)) & (FrALLOC_GRANULARITY-1))

#define FrREDZONE_VALUE (0x0123456789ABCDEFULL)

#define FrBLOCKING_SIZE (FrFOOTER_OFS - 96) //FIXME: approximate (underestimate)

/************************************************************************/
/*	Readability macros						*/
/************************************************************************/

#if defined(FrMULTITHREAD)
#  define if_MULTITHREAD(x) x
#else
#  define if_MULTITHREAD(x)
#endif /* FrMULTITHREAD_CHECKS */

#if defined(FrMEMWRITE_CHECKS)
#  define if_MEMWRITE_CHECKS(x) x
#  define if_MEMWRITE_CHECKS_else(x,y) x
#else
#  define if_MEMWRITE_CHECKS(x)
#  define if_MEMWRITE_CHECKS_else(x,y) y
#endif /* FrMEMWRITE_CHECKS */

#ifdef NVALGRIND
# define MKVALID(var) var
# define MK_VALID(var)
#else
# define MKVALID(var) ((void)VALGRIND_MAKE_MEM_DEFINED(&var,sizeof(var))) ; var
# define MK_VALID(var) ((void)VALGRIND_MAKE_MEM_DEFINED(&var,sizeof(var))) ;
#endif /* NVALGRIND */

// round up to the next multiple of some power of two
#define round_up(val,align) ((val+align-1)&(~(align-1)))

/************************************************************************/
/************************************************************************/

#if (FrALLOC_GRANULARITY-1) > UINT16_MAX
typedef uint32_t FrSubAllocOffset ;
#else
typedef uint16_t FrSubAllocOffset ;
#endif

#ifdef FrMEMWRITE_CHECKS
// a little magic to prevent linking against a library compiled with a different
//   value for FrMEMWRITE_CHECKS
class FrFreeListMW ;
typedef class FrFreeListMW FrFreeList ;
#else
class FrFreeList ;
#endif /* FrMEMWRITE_CHECKS */

//----------------------------------------------------------------------

class FrMemFooter
   {
   private:
      if_MEMWRITE_CHECKS(uint64_t m_redzone ;)	// guard area to check for overruns
      if_MULTITHREAD(pthread_t	   m_owner ;)	// who can allocate from this block?
      FrMemFooter *m_next ;
      FrMemFooter *m_mynext ;		// next block owned by same thread
      FrSubAllocOffset m_unavailable ;  // bytes at start of aligned block
				        // which are not available to us,
				        // e.g. due to memory allocator
				        // structures
      FrSubAllocOffset m_start ;        // offset of first byte actually in use
      				        // (in a suballocated block, this permits
      				        // offset-shifting to reduce cache-line
      				        // collisions)
      FrSubAllocOffset m_freecount ;	// number of unallocated objects on page
      					// (used during compaction)
   public:
      void init(FrMemFooter *nxt = 0, FrSubAllocOffset unavail = 0)
	 {
	 MK_VALID(*this) ;
	 if_MEMWRITE_CHECKS(m_redzone = FrREDZONE_VALUE) ;
	 m_next = nxt ; m_unavailable = unavail ; m_start = unavail ;
	 m_mynext = 0 ;
	 clearFreeCount() ; // keep Purify/Valgrind happy
#ifdef FrMULTITHREAD
	 m_owner = pthread_self() ;
	 // reserve space for the freelist pointer
	 m_start += round_up(sizeof(FrFreeList*),FrALIGN_SIZE) ;
#endif /* FrMULTITHREAD */
	 return ;
	 }
      FrMemFooter(FrMemFooter *nxt = 0, FrSubAllocOffset unavail = 0)
	 { init(nxt,unavail) ; }
      ~FrMemFooter() {}

      // accessors
      FrMemFooter *next() const { MK_VALID(m_next) return m_next ; }
      size_t listlength() const ;
      size_t listlengthlocal() const ;
      FrSubAllocOffset arenaStart() const { MK_VALID(m_unavailable) return m_unavailable ; }
      char *arenaStartPtr() const { return FrBLOCK_PTR(this) + arenaStart() ; }
      FrSubAllocOffset objectStart() const { MK_VALID(m_start) return m_start ; }
      char *objectStartPtr() const
	 { return FrBLOCK_PTR(this) + objectStart() ; }
      FrSubAllocOffset totalBytes() const
	 { return FrFOOTER_OFS - arenaStart() ; }
      FrSubAllocOffset availableBytes() const
	 { return FrFOOTER_OFS - objectStart() ; }
      FrSubAllocOffset totalObjects(unsigned objsize) const
	 { return availableBytes() / objsize ; }
      FrSubAllocOffset freeObjects() const { MK_VALID(m_freecount) return m_freecount ; }
      bool emptyPage(unsigned objsize) const { return freeObjects() >= totalObjects(objsize) ; }
      FrMemFooter *myNext() const { MK_VALID(m_mynext) return m_mynext ; }
#ifdef FrMULTITHREAD
      pthread_t owner() const { MK_VALID(m_owner) return m_owner ; }
      bool myMemory() const { MK_VALID(m_owner) return pthread_equal(m_owner,pthread_self()) ; }
      FrFreeList *freelistHead() const
	 {
	 (void)VALGRIND_MAKE_MEM_DEFINED(arenaStartPtr(),sizeof(FrFreeList*)) ;
	 return *((FrFreeList**)arenaStartPtr()) ; 
	 }
      FrFreeList *localFreelist() const
	 {
	 (void)VALGRIND_MAKE_MEM_DEFINED(arenaStartPtr(),sizeof(FrFreeList*)) ;
	 return *((FrFreeList**)arenaStartPtr()) ; 
	 }
#else
      int owner() const { return 0 ; }
      bool myMemory() const { return true ; }
      FrFreeList *freelistHead() const { return 0 ; }
      FrFreeList *localFreelist() const { return 0 ; }
#endif /* FrMULTITHREAD */
      bool hadOverrun() const
	 { MK_VALID(m_redzone) return if_MEMWRITE_CHECKS_else(m_redzone == FrREDZONE_VALUE,false); }
      // the following is used by FrSymbolTable's palloc(), which
      //   adjusts the object-start pointer to record which part of
      //   the page has already been allocated
      unsigned freecount() const { return objectStart() - arenaStart() ; }

      // manipulators
      void next(FrMemFooter *nxt) { MKVALID(m_next) = nxt ; }
      void myNext(FrMemFooter *nxt) { MKVALID(m_mynext) = nxt ; }
      void freelistHead(FrFreeList *fl) ;
      void setArenaStart(unsigned unavail)
	 { debug_assert(unavail < FrALLOC_GRANULARITY/2) ; m_unavailable = (FrSubAllocOffset)unavail ; }
      void setObjectStart(unsigned start)
	 {
	 debug_assert(start >= arenaStart() if_MULTITHREAD(+ sizeof(FrSubAllocOffset))) ;
	 MKVALID(m_start) = (FrSubAllocOffset)start ;
	 }
      void adjObjectStartDown(unsigned amount)
	 { MKVALID(m_start) -= (FrSubAllocOffset)amount ; debug_assert(objectStart() >= arenaStart()) ; }
      static void pushOnFreelist(FrFreeList *item) ;
      FrFreeList *grabLocalFreelist() ;
      void clearFreeCount() { MKVALID(m_freecount) = 0 ; }
      void incrFreeCount() { MKVALID(m_freecount) += 1 ; }
      void markEmpty() { MKVALID(m_freecount) = FrFOOTER_OFS ; }
      void setOwner(pthread_t new_owner)
	 {
	 (void)new_owner ;
	 if_MULTITHREAD(m_owner = new_owner) ;
	 }
      void acceptOwnership() { if_MULTITHREAD(m_owner = pthread_self()) ; }
   } ;

//----------------------------------------------------------------------

#ifdef FrMEMWRITE_CHECKS
class FrFreeListMW
   {
   private:
      uint64_t m_marker ;
   public:
      void clearMarker() { MKVALID(m_marker) = 0 ; }
      void setMarker() { MKVALID(m_marker) = FrREDZONE_VALUE ; }
      bool markerIntact() const { MK_VALID(m_marker) return m_marker == FrREDZONE_VALUE ; }
#else // !FrMEMWRITE_CHECKS
class FrFreeList
   {
   public:
      void clearMarker() {}
      void setMarker() {}
      bool markerIntact() const { return true ; }
#endif /* FrMEMWRITE_CHECKS */
   public:
      FrFreeList *m_next ;
   public:
      FrFreeList() { next(0) ; }
      FrFreeList *next() const { MK_VALID(m_next) return m_next ; }
      void next(FrFreeList *n) { MKVALID(m_next) = n ; }
      size_t listlength() const ;
      FrFreeList *nextInPage(FrMemFooter *page) const
	 {
	 FrSubAllocOffset ofs = *((FrSubAllocOffset*)this) ;
	 return (ofs < FrFOOTER_OFS) ? (FrFreeList*)(((char*)FrBLOCK_PTR(page))+ofs) : 0 ;
	 }
      void nextInPage(FrSubAllocOffset ofs) { *((FrSubAllocOffset*)this) = ofs ; }
   } ;

//----------------------------------------------------------------------

class FrFreeListCL : public FrFreeList
   {
   private:
#ifdef FrMULTITHREAD
      // avoid false sharing of freelist heads by separating them into
      //   individual cache lines
      char m_pad[Fr_cacheline_size - sizeof(FrFreeList)] ;
#endif /* FrMULTITHREAD */
   } ;

//----------------------------------------------------------------------

class FrFreeHdr
   {
   public:
      FrFreeHdr *next ;			// this MUST be the first field!
      FrFreeHdr *prev ;
   public:
      void initEmpty() { next = this ; prev = this ; }
      FrFreeHdr *nextFree() const { return next ; }
      FrFreeHdr *prevFree() const { return prev ; }
      void setNextFree(FrFreeHdr *n) { MKVALID(next) = n ; }
      void setPrevFree(FrFreeHdr *p) { MKVALID(prev) = p ; }
   } ;
// code in frmem.C uses the fact that "next" is at offset 0 in the structure
// to allow the freelist "prev" pointer to point at the simple FrFreeHdr*
// that points at the freelist, thus avoiding a null check when unlinking
// blocks from the freelist

class FrAllocator ;
class FrMemoryPool ;

typedef void FrCompactFunc(FrMemFooter *pages, FrFreeList **freelist,
			   FrMemFooter **freed) ;
typedef int FrGCFunc(FrAllocator *allocator) ;
typedef bool FrMemIterFunc(void *object, va_list args) ;

/************************************************************************/
/*	Global Variables						*/
/************************************************************************/

extern class FrMemoryPool FramepaC_mempool ;

extern bool memory_errors_are_fatal ;
extern uint64_t FrMalloc_requests[] ;	// only valid if FrMEMUSE_STATS

extern void *FramepaC_memerr_addr ;

/************************************************************************/
/*    Procedural interface to memory allocation				*/
/************************************************************************/

extern void *FrMalloc(size_t numbytes)
     _fnattr_malloc _fnattr_alloc_size1(1) _fnattr_warn_unused_result ;
extern void *FrCalloc(size_t nitems, size_t size)
      _fnattr_malloc _fnattr_alloc_size2(1,2) _fnattr_warn_unused_result ;
extern void *FrCalloc(size_t nitems, size_t size,
		      FrMemoryPool *mp)
      _fnattr_malloc _fnattr_alloc_size2(1,2) _fnattr_warn_unused_result ;
void *FrRealloc(void *block, size_t newsize, bool copy_contents = true)
      _fnattr_alloc_size1(2) _fnattr_warn_unused_result ;
void *FrRealloc(void *block, size_t newsize, bool copy_contents,
		FrMemoryPool *mp)
      _fnattr_alloc_size1(2) _fnattr_warn_unused_result ;
void FrFree(void *block) ;
void *realloc3(void *block, size_t newsize, bool copy_contents = true)
      _fnattr_alloc_size1(2) _fnattr_warn_unused_result ;

void *_FrMallocDebug(size_t numbytes, const char *file, size_t line)
     _fnattr_malloc _fnattr_alloc_size1(1) _fnattr_warn_unused_result ;
void *_FrCallocDebug(size_t nitems,size_t size, const char *file,size_t line)
     _fnattr_malloc _fnattr_alloc_size2(1,2) _fnattr_warn_unused_result ;
void *_FrReallocDebug(void *blk, size_t newsize, bool copy_contents,
		      const char *file,size_t line) ;
void _FrFreeDebug(void *block, const char *file, size_t line) ;

#ifdef FrMEMLEAK_CHECKS
#define FrMalloc(size) _FrMallocDebug(size,__FILE__,__LINE__)
#define FrCalloc(n,size) _FrCallocDebug(n,size,__FILE__,__LINE__)
inline FrRealloc(void *blk,size_t newsize,bool copy=true)
  { return _FrReallocDebug(blk,newsize,copy,__FILE__,__LINE__) ; }
#define FrFree(blk) _FrFreeDebug(blk,__FILE__,__LINE__)
#endif /* FrMEMLEAK_CHECKS */

// control over how aggressively memory is returned to the operating system
//   A level of 0 means memory is never ever returned prior to program
//   termination.
unsigned FrMallocReclaimLevel() ;		// get current level
bool FrMallocReclaimLevel(unsigned level) ;	// set new level
unsigned FrMallocReclaimLevelMax() ;		// get maximum valid level
bool FrMallocAllowFragmentingArenas(bool) ;	// OK to partially return mem to OS?

void FrMalloc_gc() ;		// reclaim unused FrMalloc blocks
void FramepaC_gc() ;		// reclaim unused FrMalloc/FrAllocator blocks

size_t FrMaxSmallAlloc() 	// largest request allocated in a small block
     _fnattr_const ;

// integrity checks
bool FramepaC_memory_chain_OK() ;
bool FramepaC_memory_freelist_OK() ;
bool check_FrMalloc(FrMemoryPool *mpi) ;

// informational displays
void FrMemoryStats(ostream &out, bool verbose = false) ;
void FrMemoryStats() ; // use cerr
void FrMemoryStats(bool verb) ; // use cerr
void FrMemoryAllocReport(ostream &out) ;
void FrMemoryAllocReport() ; // use cerr
void FrShowMemory(ostream &out) ;
void FrShowMemory() ;

/************************************************************************/
/*	Debugging Support						*/
/************************************************************************/

#if defined(PURIFY)
#undef FrMalloc
#define FrMalloc ::malloc
#undef FrCalloc
#define FrCalloc ::calloc
#undef FrRealloc
#define FrRealloc ::realloc3
#undef FrFree
#define FrFree ::free
#endif /* PURIFY */

/************************************************************************/
/*    Convenience Macros						*/
/************************************************************************/

#define FrNew(type) ((type*)FrMalloc(sizeof(type)))
#define FrNewN(type,n) ((type*)FrMalloc((n)*sizeof(type)))
#define FrNewC(type,n) ((type*)FrCalloc((n),sizeof(type)))
#define FrNewR(type,ptr,n) ((type*)FrRealloc(ptr,(n)*sizeof(type),true))

/************************************************************************/
/*	local stack allocation similar to alloca, but allowing		*/
/*	arbitrary sizes by falling back on regular memory allocator	*/
/************************************************************************/

#define FrLocalAlloc(type,name,stackcount,count) \
   type name##buffer[stackcount] ; \
   type *name ; \
   if ((count) <= stackcount) \
      name = name##buffer ; \
   else \
      name = FrNewN(type,count) ;

#define FrLocalAllocC(type,name,stackcount,count) \
   type name##buffer[stackcount] ; \
   type *name ; \
   if ((count) <= stackcount) \
      { \
      name = name##buffer ; \
      memset(name,'\0',(count)*sizeof(type)) ; \
      } \
   else \
      name = FrNewC(type,count) ;

#define FrLocalFree(name) \
   if (name != name##buffer) FrFree(name) ;

/************************************************************************/
/*	declarations for FrMemoryBinnedFreelist			  	*/
/************************************************************************/

// originally 4 and 2 instead of 3 and 3
#define FrMEMPOOLINFO_FACTORBITS 3
#define FrBIGPOOLINFO_FACTORBITS 3
//#define FrMEMPOOLINFO_FACTORBITS 3
//#define FrBIGPOOLINFO_FACTORBITS 4

#define FrMEMPOOLINFO_FACTOR (1<<FrMEMPOOLINFO_FACTORBITS)
#define FrBIGPOOLINFO_FACTOR (1<<FrBIGPOOLINFO_FACTORBITS)

#if FrALIGN_SIZE >= 16
#define FrMEMPOOLINFO_SCALE 4
#elif FrALIGN_SIZE >= 8
#define FrMEMPOOLINFO_SCALE 3
#elif FrALIGN_SIZE >= 4
#define FrMEMPOOLINFO_SCALE 2
#else
#define FrMEMPOOLINFO_SCALE 1
#endif

//NOTE: FrMEMPOOLINFO_BINS must be big enough to hold all the bins needed by
//  *any* instantiation of FrMemoryBinnedFreelist
#if FrBIGPOOLINFO_FACTORBITS > FrMEMPOOLINFO_FACTORBITS
#  define FrPOOLINFO_FACTORBITS FrBIGPOOLINFO_FACTORBITS
#  define FrPOOLINFO_FACTOR FrBIGPOOLINFO_FACTOR
#else
#  define FrPOOLINFO_FACTORBITS FrMEMPOOLINFO_FACTORBITS
#  define FrPOOLINFO_FACTOR FrMEMPOOLINFO_FACTOR
#endif
#define FrMEMPOOLINFO_BINS \
     (FrPOOLINFO_FACTOR*(sizeof(unsigned short)*CHAR_BIT \
			    - FrMEMPOOLINFO_SCALE \
			    - 2*FrPOOLINFO_FACTORBITS + 2))

template <class T_hdr, unsigned N_factor> class FrMemoryBinnedFreelist
   {
   private:
      static size_t   s_maxbin ;		// maximum bin number in use
      static size_t   s_granularity ;
      static size_t   s_granularity_shift ;
      static unsigned s_N_factor_shift ;
      static size_t   s_binsizes[FrMEMPOOLINFO_BINS+1] ;
   protected:
      T_hdr	      m_freelist ;
      FrMutex 	      m_mutex ;
      FrMemoryBinnedFreelist *m_self ;	// verifies initialization by pointing
					//   to containing instance
      T_hdr	     *m_freelist_ptrs[FrMEMPOOLINFO_BINS] ;
      uint16_t	      m_index[FrMEMPOOLINFO_BINS] ;
   protected:
      void setMaximumBin(size_t max) { s_maxbin = max ; }
      void setGranularity(size_t gr)
	 { s_granularity = gr ;
	   s_granularity_shift = 0 ;
	   while ((gr&1) == 0) { s_granularity_shift++; gr>>=1; }
	 }
      void setBinSize(size_t N, size_t sz) { s_binsizes[N] = sz ; }
      static void markAsFree(T_hdr *block) ;
      static void markAsUsed(T_hdr *block) ;
      static void markAsReserved(T_hdr *block) ;
      static bool blockInUse(const T_hdr *block) ;
      static bool blockReserved(const T_hdr *block) ;
   public:
      FrMemoryBinnedFreelist(size_t minbinsize, size_t maxbinsize,
			     size_t gran) ;
      ~FrMemoryBinnedFreelist() ;
      void init(size_t minbinsize, size_t maxbinsize,
		size_t gran) ;
      void initBinSizes(size_t minbinsize, size_t maxbinsize,
			size_t gran) ;

      // accessors
      bool initialized() const { return m_self == this ; }
      FrMutex &mutex() { return m_mutex ; }
      T_hdr *freelistHead() { return &m_freelist ; }
      static size_t numBins() { return FrMEMPOOLINFO_BINS ; }
      static size_t granularity() { return s_granularity ; }
      static size_t granularityShift() { return s_granularity_shift ; }
      static size_t maximumBin() { return s_maxbin ; }
      static size_t bin(size_t size) ;
      static size_t binsize(size_t bin_num) { return s_binsizes[bin_num] ; }
      static size_t blockSize(const T_hdr *block) ;
      static size_t blockBin(const T_hdr *block) ;
      static void splitblock(size_t totalsize, size_t &desired) ;

      // manipulators
      T_hdr *pop(size_t size) ;
      void add(T_hdr *block) ;
      void remove(T_hdr *block,size_t bin_num) ;
      void remove(T_hdr *block) ;
      size_t removeIfFree(T_hdr *block) ;

      // debug
      bool check_idx() const ;
      bool containsDuplicates() const ;
      bool freelistOK() const ;
   } ;

/************************************************************************/
/*	declarations for class FrMemoryPool			  	*/
/************************************************************************/

typedef FrMemoryBinnedFreelist<FrFreeHdr,FrMEMPOOLINFO_FACTOR> FrMemPoolFreelist ;

class FrMemoryPool
   {
   protected:
      static FrCriticalSection          s_critsect ;
      static FrPER_THREAD FrMemPoolFreelist *s_poolinfo ;
      static FrMemFooter               *s_global_pagelist ;  // adoptable still-in-use pages
      static FrMemFooter               *s_global_freepages ;
      static FrPER_THREAD FrFreeListCL *s_foreign_frees ;
      static FrPER_THREAD FrMemFooter  *s_pagelist_malloc ;  // list of pages in use
      static FrPER_THREAD FrMemFooter  *s_freepages ;   // list of freed pages
      static FrPER_THREAD size_t        s_freecount ;   // number of freed pages
      static size_t                     s_freelimit ;   // max freed pages to cache

      FrMemoryPool *m_next ;
      FrMemoryPool *m_prev ;
      size_t        m_refcount ;
      char          m_typename[FrMAX_MEMPOOL_NAME] ;
   protected:
      void reclaimForeignFrees() ;		// get back all pending var-sized frees
      static void orphanBusyPages() ;
      void adoptOrphanedPages() ;
   public:
      void *operator new(size_t size) { return FrMalloc(size) ; }
      void operator delete(void *blk) { FrFree(blk) ; }
      FrMemoryPool(const char *name = 0) ;
      ~FrMemoryPool() ;
      void init(const char *name) ;
      void init() ;

      void *allocate(size_t size) _fnattr_malloc ;
      void release(void *item) ;
      void *reallocate(void *item,size_t size, bool copydata = true)
	 _fnattr_warn_unused_result ;

      void releaseForeign(void *item) ;		// item was allocated by another thread

      // manipulation functions
      void setName(const char *name) ;
      void incRefCount() { m_refcount++ ; }
      bool decRefCount() ;

      // synchronization
      static void enterCritical() { s_critsect.acquire() ; }
      static void leaveCritical() { s_critsect.release() ; }
      template <typename T> static T atomicSwap(T &var, const T value)
	 { return s_critsect.swap(var,value) ; }
      template <typename T> static void atomicPush(T **var, T *node)
	 { s_critsect.push(var,node) ; }
      template <typename T> static void atomicPush(T *var, T node)
	 { s_critsect.push(var,node) ; }
      template <typename T> static void atomicPush(T **var, T *nodes, T *tail)
	 { s_critsect.push(var,nodes,tail) ; }
      template <typename T> static T *atomicPop(T **var)
	 { return s_critsect.pop(var) ; }

      // accessors
      bool initialized() const { return m_refcount > 0 ; }
      const char *typeName() const { return m_typename ; }
      FrMemoryPool *nextPool() const { return m_next ; }
      FrMemoryPool *prevPool() const { return m_prev ; }

      static size_t numPages() ;
      static bool haveFreePages() { return s_freepages != 0 ; }
      static size_t numFreePages() ;
      static const FrMemFooter *localMallocPageList() { return s_pagelist_malloc ; }
      static const FrMemFooter *globalPageList() { return s_global_pagelist ; }
      static const FrMemFooter *localFreePages() { return s_freepages ; }
      static const FrMemFooter *globalFreePages() { return s_global_freepages ; }
      static FrFreeList *foreignFreelistHead() ;
      long bytesAllocated() const ;

      // manipulators for per-thread blocks
      static void addMallocPage(FrMemFooter *page) ;
      static void removeMallocPage(FrMemFooter *page) ;
      static FrMemFooter *grabMallocPages() ;
      void removeMallocBlock(FrMemFooter *oldblock, FrMemFooter *pred) ;
      static void addFreePage(FrMemFooter *page) ;
      static FrMemFooter *grabAllFreePages() ;
      static FrMemFooter *popFreePage() ;
      static bool reclaimFreePages() ;
      bool checkFreelist() ;

      static void threadCleanup() ;

      // manipulators for global blocks
      static void addGlobalPage(FrMemFooter *page) ;
      static void addGlobalPages(FrMemFooter *pages, FrMemFooter *last) ;
      static FrMemFooter *grabGlobalPages() ;
      static void addGlobalFreePage(FrMemFooter *page) ;
      static void addGlobalFreePages(FrMemFooter *pages, FrMemFooter *last) ;
      static FrMemFooter *grabGlobalFreePages() ;

      // pass-throughs to the freelist manager
      static void add(FrFreeHdr *hdr) { s_poolinfo->add(hdr) ; }
      static void remove(FrFreeHdr *hdr) { s_poolinfo->remove(hdr) ; }

      // debugging
      void check() ;
   } ;

/************************************************************************/
/*	declarations for class FrSlabPool			  	*/
/************************************************************************/

// this class basically just serves as a marker to identify a set of pages
//   containing the same size of object
class FrSlabPool
   {
   protected:
      static bool	s_initialized ;
      FrMemFooter      *m_reclaimlist ;
      size_t		m_pagecount ;
      unsigned short	m_size_bin ; // may not need this field if only accessed for info display
      FrSubAllocOffset	m_objsize ;
      bool		m_malloc ;
      bool		m_pending_frees ;
   public:
      void *operator new(size_t size) { return FrMalloc(size) ; }
      void operator delete(void *blk) { FrFree(blk) ; }
      FrSlabPool()
	 {
	 // Default memory zeroing is all the init we need.
	 // Trying to do an explicit init causes problems due to use
	 //   caused by memory allocation requests which occur
	 //   *before* the requisite global ctor is called!
	 }
      FrSlabPool(FrSubAllocOffset objsize, bool malloc_header)
	 { init(objsize,malloc_header) ; }
      ~FrSlabPool() ;

      // accessors
      size_t pageCount() const { return m_pagecount ; }
      const char *typeName() const ;
      unsigned objectSize() const { return m_objsize ; }
      bool mallocHeader() const { return m_malloc ; }
      static unsigned binSize(unsigned bin_num, bool malloc_header) ;
      static unsigned sizeBin(FrSubAllocOffset objsize, bool malloc_header) ;
      //unsigned sizeBin() const { return sizeBin(m_objsize,m_malloc) ; }
      unsigned sizeBin() const { return m_size_bin ; }
      bool pendingFrees() const { return m_pending_frees ; }

      // manipulators
      static void slabsInitialized() { s_initialized = true ; }
      void init(FrSubAllocOffset objsize, bool malloc_header) ;
      void addToReclaimList(FrMemFooter *pages, FrMemFooter *last) ;
      FrMemFooter *popReclaimed() ;
      void addPage() { m_pagecount++ ; }
      void removePage() { m_pagecount-- ; }
      void removePages(size_t count) { m_pagecount -= count ; }
      void addPendingFree() { m_pending_frees = true ; }
      // we don't bother synchronizing the clear, since an occasional lost 'set'
      //   will merely result in delayed reclamation of free objects
      void clearPendingFrees() { m_pending_frees = false ; }

      //
      static FrFreeList *subdividePage(FrMemFooter *page, FrFreeList *freelist, FrSubAllocOffset objsize,
				       bool malloc_header, size_t &objcount) ;
      FrFreeList *subdividePage(FrMemFooter *page, FrFreeList *freelist, size_t &objcount) const
	 { return subdividePage(page,freelist,objectSize(),mallocHeader(),objcount) ; }
   } ;

/************************************************************************/
/*	declarations for class FrAllocator				*/
/************************************************************************/

#ifdef FrMEMUSE_STATS
#  define INC_REQUESTS m_total_requests++
#else
#  define INC_REQUESTS
#endif /* FrMEMUSE_STATS */

#  define MEMWRITE_CHECK \
      if_MEMWRITE_CHECKS(if (!freelist()->markerIntact()) FrProgError("memory overwrite!") ;)
#  define MEMWRITE_CLEAR(item) \
      if_MEMWRITE_CHECKS(((FrFreeList*)item)->setMarker())

class FrAllocator
   {
   protected:
      static FrSlabPool s_slabpools[FrTOTAL_ALLOC_LISTS] ;
      static FrPER_THREAD FrFreeListCL *s_foreign_freelists ;
      static FrPER_THREAD FrFreeList *s_freelists[FrTOTAL_ALLOC_LISTS] ;
      static FrPER_THREAD uint32_t s_freecounts[FrTOTAL_ALLOC_LISTS] ;
      static FrPER_THREAD FrMemFooter *s_pagelists[FrTOTAL_ALLOC_LISTS] ;
      char               m_typename[FrMAX_MEMPOOL_NAME] ;
      uint64_t           m_total_requests ;
      FrCompactFunc     *m_compact_func ;
      FrGCFunc          *gc_func ;
      FrGCFunc          *async_gc_func ;
      FrAllocator       *m_next ;		// maintain a list of all instances
      FrAllocator       *m_prev ;
      unsigned int	 m_size_bin ;
      FrSubAllocOffset	 m_objsize ;
      FrSubAllocOffset	 m_usersize ;
      bool 		 m_compacting ;
      bool		 m_malloc ;  // do we have malloc header on each object?
   protected: // methods
      FrAllocator() {} // array init, need to finalize later
      void *allocate_more() ;
      void setSizeBin() ;
      FrMemFooter *&pageList() const { return s_pagelists[m_size_bin] ; }
      void pushPage(FrMemFooter *page)
	 {
	 page->myNext(pageList()) ; pageList() = page ;
	 }
      static void pushPage(FrMemFooter *page, size_t bin_num)
	 {
	 page->myNext(pageList(bin_num)) ; pageList(bin_num) = page ;
	 }
      FrSlabPool *slabpool() const { return &s_slabpools[m_size_bin] ; }
      FrFreeList *freelist() const { return s_freelists[m_size_bin] ; }
      static FrFreeList *&freelistPtr(unsigned bin_num) { return s_freelists[bin_num] ; }
      FrFreeList *&freelistPtr() const { return s_freelists[m_size_bin] ; }
      static FrFreeListCL *foreignFreelistHead(size_t size_bin) ;
      void setFreelist(FrFreeList *fl) { s_freelists[m_size_bin] = fl ; }
      static void setFreelist(FrFreeList *fl, size_t size_bin) { s_freelists[size_bin] = fl ; }
      FrFreeList *grabFreelist() ;
      static FrFreeList *reclaimForeignFrees(size_t size_bin) ;	// get back pending frees
      static bool reclaimPageFrees(FrMemFooter *page, size_t size_bin) ;
      bool reclaimPageFrees(FrMemFooter *page) { return reclaimPageFrees(page,m_size_bin) ; }
      void releaseForeign(void *item) ;
   public:
      FrAllocator(const char *name, int size, FrCompactFunc *func = 0,
		  bool malloc_headers = false) ;
      ~FrAllocator() ;
#if defined(PURIFY)
      // override the suballocator and use standard allocations instead
      void *allocate() _fnattr_hot _fnattr_malloc { return malloc(objectSize()) ; }
      void release(void *item) { free(item) ; }
#else /* !PURIFY */
      void *allocate() _fnattr_hot _fnattr_malloc
	 {
	 void *item = freelist() ;
	 INC_REQUESTS ;
	 if (unlikely(!item))
	    item = allocate_more() ;
	 MEMWRITE_CHECK ;
	 FrFreeList *fl = (FrFreeList*)item ;
	 setFreelist(fl->next()) ;
	 s_freecounts[m_size_bin]-- ;
	 VALGRIND_MEMPOOL_ALLOC(&s_slabpools[m_size_bin],item,dataSize()) ;
	 return item ;
	 }
      void release(void *item) _fnattr_hot
	 {
	 MEMWRITE_CLEAR(item) ;
	 s_freecounts[m_size_bin]++ ;
#ifdef FrMULTITHREAD
	 FrMemFooter *footer = FrFOOTER_PTR(item) ;
	 if (unlikely(!footer->myMemory()))
	    {
	    releaseForeign(item) ;
	    return ;
	    }
#endif /* FrMULTITHREAD */
	 // no synchronization needed here, since only the thread
	 //   which originally allocated the block can free it to the
	 //   main thread-local free list
//!!!	 VALGRIND_MEMPOOL_FREE(&s_slabpools[m_size_bin],item) ;
	 FrFreeList *fl = (FrFreeList*)item ;
	 (void)VALGRIND_MAKE_MEM_UNDEFINED(&fl->m_next,sizeof(fl->m_next)) ;
	 fl->next(freelist()) ;
	 (void)VALGRIND_MAKE_MEM_DEFINED(&item,sizeof(FrFreeList*)) ;
	 setFreelist(fl) ;
	 }
#endif /* PURIFY */
      void *allocate(size_t size) _fnattr_malloc
	 {
	 if (size > objectSize())
	    return FrMalloc(size) ;
	 else
	    return allocate() ;
	 }
      void release(void *item,size_t size)
	 {
	 if (size > objectSize())
	    FrFree(item) ;
	 else
	    release(item) ;
	 }

      // manipulation functions
      void setName(const char *name) ;
      void flagAsMalloc() { m_malloc = true ; }
      void set_gc_funcs(FrGCFunc *gc, FrGCFunc *async_gc) ;
      int async_gc() ;
      int gc() ;
      size_t compact(bool force = false) ;
      size_t reclaim() ;
      static size_t reclaim(unsigned size_bin) ;
      static size_t reclaimAll() ;
      static size_t reclaimFreeBlocks() ;
      void preAllocate() ;
      // the following two functions are NOPs unless FrMEMWRITE_CHECKS is enabled
      bool iterateVA(FrMemIterFunc *fn, va_list args) ;
      bool iterate(FrMemIterFunc *fn, ...) ;

      // access to state
      const char *typeName() const { return m_typename ; }
      size_t objects_allocated() const ;
      size_t freelist_length() const ;
      unsigned long requests_made() const { return m_total_requests ; }
      unsigned int objectSize() const { return m_objsize ; }
      unsigned int dataSize() const { return m_usersize ; }
      unsigned int objectsPerPage() const { return (FrFOOTER_OFS-48) / objectSize() ; } // approximate
      bool mallocHeader() const { return m_malloc ; }
      FrAllocator *nextAllocator() const { return m_next ; }
      FrAllocator *prevAllocator() const { return m_prev ; }

      // statistics support
      static FrSlabPool *slabpool(unsigned bin_num) { return &s_slabpools[bin_num] ; }
      static FrFreeList *freelist(unsigned bin_num) { return s_freelists[bin_num] ; }
      static FrMemFooter *&pageList(unsigned bin_num) { MK_VALID(s_pagelists[bin_num]) return s_pagelists[bin_num] ; }

      // threading support
      static size_t reclaimAllForeignFrees() ;
      static void threadSetup() ;
      static void threadInitOnce() ;
      static void threadCleanup(void * = 0) ;
   } ;

#undef INC_REQUESTS
#undef MEMWRITE_CHECK
#undef MKVALID
#undef MK_VALID

/************************************************************************/

#endif /* !__FRMEM_H_INCLUDED */

// end of file frmem.h //
