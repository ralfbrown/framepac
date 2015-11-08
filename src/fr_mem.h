/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File fr_mem.h		private memory allocation declarations	*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,1998,2004,2006,2007,2009,2010,	*/
/*		2013 Ralf Brown/Carnegie Mellon University		*/
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

#ifndef __FR_MEM_H_INCLUDED
#define __FR_MEM_H_INCLUDED

#ifndef __FRMEM_H_INCLUDED
#include "frmem.h"
#endif

/************************************************************************/
/*	Compilation Options						*/
/************************************************************************/

// Should we aggressively return suballocated pages to the global
//   memory pool for use by any allocation?  This slightly slows down
//   allocations but reduces memory wastage, especially in
//   multi-threaded situations
#ifdef FrMULTITHREAD
//#define AGGRESSIVE_RECLAIM
#else
//#define AGGRESSIVE_RECLAIM		// less useful for single-threading
#endif /* FrMULTITHREAD */

/************************************************************************/
/*	Manifest constants						*/
/************************************************************************/

#define FrMALLOC_BINS	11   // # of separate sizes to report for memory stats

#define MAX_SMALL_ALLOC ((3*FrFOOTER_OFS/4)-2*FrALIGN_SIZE-HDRSIZE)
#define MAX_SMALL_BLOCK ((3*FrFOOTER_OFS/4)-FrALIGN_SIZE)

#define MAX_AUTO_SUBALLOC ((int)(FrMAX_AUTO_SUBALLOC - sizeof(FrSubAllocOffset)))

/************************************************************************/
/*	Convenience macros						*/
/************************************************************************/

#ifdef NVALGRIND
# define MKVALID(var) var
# define MK_VALID(var)
# define MKUNDEF(var)
# define MKNOACCESS(var)
#else
# define MKVALID(var) ((void)VALGRIND_MAKE_MEM_DEFINED(&var,sizeof(var))) ; var
# define MK_VALID(var) ((void)VALGRIND_MAKE_MEM_DEFINED(&var,sizeof(var))) ;
# define MKUNDEF(var) ((void)VALGRIND_MAKE_MEM_UNDEFINED(&var,sizeof(var))) ;
# define MKNOACCESS(var) ((void)VALGRIND_MAKE_MEM_NOACCESS(&var,sizeof(var))) ;
#endif /* NVALGRIND */

/************************************************************************/
/*	Types								*/
/************************************************************************/

class FrBigAllocHdr ;

class FrMallocHdr
   {
   public:
      FrSubAllocOffset prevsize ;
      FrSubAllocOffset size ;   // low bit indicates whether block is in use
   } ;

//----------------------------------------------------------------------

class FrMallocBigHdr
   {
   public:
      int size ;
   } ;

//----------------------------------------------------------------------

class FrSubAllocator : public FrAllocator
   {
   public:
      void *operator new (size_t, void *where) { return where ; } // placement new
      FrSubAllocator() {} // array init, need to finalize individually later
      FrSubAllocator(int size, const char *name = 0,
		     FrCompactFunc *func = 0) ;
      ~FrSubAllocator() {}

      void setObjectSize(size_t s) { m_objsize = s ; setSizeBin() ; }
   } ;

/************************************************************************/
/*	Static members for class FrMemoryBinnedFreelist			*/
/************************************************************************/

template<class T_hdr, unsigned N_factor>
size_t FrMemoryBinnedFreelist<T_hdr,N_factor>::s_maxbin = 0 ;
template<class T_hdr, unsigned N_factor>
size_t FrMemoryBinnedFreelist<T_hdr,N_factor>::s_granularity = 1 ;
template<class T_hdr, unsigned N_factor>
size_t FrMemoryBinnedFreelist<T_hdr,N_factor>::s_granularity_shift = 0 ;
template<class T_hdr, unsigned N_factor>
unsigned  FrMemoryBinnedFreelist<T_hdr,N_factor>::s_N_factor_shift = 0 ;
template<class T_hdr, unsigned N_factor>
size_t FrMemoryBinnedFreelist<T_hdr,N_factor>::s_binsizes[FrMEMPOOLINFO_BINS+1] ;

/************************************************************************/
/*	Global Variables						*/
/************************************************************************/

extern FrMemoryPool *FramepaC_memory_pools ;
extern FrBigAllocHdr *FramepaC_memory_start ;
extern FrBigAllocHdr *FramepaC_memory_end ;
extern FrAllocator *FramepaC_allocators ;
extern int FramepaC_symbol_blocks ;
#ifdef FrREPLACE_MALLOC
extern FrMemoryBinnedFreelist<FrBigAllocHdr,FrBIGPOOLINFO_FACTOR>
	    FramepaC_bigalloc_pool ;
#endif /* FrREPLACE_MALLOC */

/************************************************************************/
/*    Procedural interface to memory allocation				*/
/************************************************************************/

extern void (*FramepaC_gc_handler)() ;
extern void (*FramepaC_auto_gc_handler)() ;

bool FrIterateMemoryArenas(bool (*fn)(FrBigAllocHdr*,void*),void* user_data) ;

#if !defined(FrREPLACE_MALLOC) && !defined(PURIFY)
#define big_malloc big_malloc_sys
#define big_free big_free_sys
#endif /* !FrREPLACE_MALLOC */

void *big_malloc(size_t, bool prefer_mmap = false) _fnattr_malloc ;
void *big_realloc(void *block,size_t newsize) ;
void big_free(void *) ;
bool big_reclaim() ;

/************************************************************************/
/*    Convenience Macros						*/
/************************************************************************/

#define memory_last (FramepaC_bigalloc_pool.freelistHead()->prev())

#define HDRSIZE sizeof(FrMallocHdr)
#define HDR(ptr) (((FrMallocHdr*)(ptr))[-1])
#define HDRADDR(ptr) (((FrMallocHdr*)(ptr))-1)
#define HDRBLKSIZE(ptr) (HDR(ptr).size & FRFREEHDR_SIZE_MASK)
#define HDRBLKUSED(ptr) (HDR(ptr).size & FRFREEHDR_USED_BIT)
#define HDRSETSIZE(ptr,sz) (HDR(ptr).size = (FrSubAllocOffset)(sz))
#define HDRSETSIZEUSED(ptr,sz)(HDR(ptr).size = (FrSubAllocOffset)(sz) | FRFREEHDR_USED_BIT)
#define HDRMARKUSED(ptr) (HDR(ptr).size |= FRFREEHDR_USED_BIT)
#define HDRMARKFREE(ptr) (HDR(ptr).size &= ~FRFREEHDR_USED_BIT)

#define MIN_ALLOC (HDRSIZE + 2*sizeof(void*))
#define FRFREEHDR_USED_BIT (1)
#define FRFREEHDR_SIZE_MASK (~ FRFREEHDR_USED_BIT)
#define FRFREEHDR_BIGBLOCK (USHRT_MAX & FRFREEHDR_SIZE_MASK)

#define BIGHDRSIZE (round_up(HDRSIZE+sizeof(FrMallocBigHdr),FrALIGN_SIZE))
#define BIGHDR(ptr) (((FrMallocBigHdr*)(((char*)ptr)-HDRSIZE))[-1])

#endif /* !__FR_MEM_H_INCLUDED */

// end of file fr_mem.h //
