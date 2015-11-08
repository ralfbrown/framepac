/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frballoc.h		private memory allocation declarations	*/
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

#ifndef __FRBALLOC_H_INCLUDED
#define __FRBALLOC_H_INCLUDED

#include "fr_mem.h" //!!!

/************************************************************************/
/*	Types								*/
/************************************************************************/

enum BlockStatus
   {
      Block_Reserved,
      Block_In_Use,
      Block_Free,
      Block_Last_Status = Block_Free
   } ;

enum MemorySource
   {
      MEMSRC_INVALID,
      MEMSRC_SBRK,
      MEMSRC_MMAP,
      MEMSRC_LASTSRC=MEMSRC_MMAP
   } ;

class FrBigAllocHdr ;

#ifdef DEBUG
# define CHKVALID assert(memsource!=MEMSRC_INVALID); 
#else
# define CHKVALID
#endif

class FrBigAllocHdrBase
   {
   protected:
      FrBigAllocHdr *m_nextarena ;	// only valid if 'first'
      FrBigAllocHdr *m_prevarena ;	// only valid if 'first'
      size_t         m_lastoffset ;     // if 'first'/'last', offset of last block in arena
      size_t         m_size ;		// size of current allocation block
      size_t         m_prevsize ;	// size of previous allocation block, used
					//   to find that block; 0 if 'first'
   public:
#if FrALIGN_SIZE > 4
      // when we have to pad the structure anyway, don't pack the
      //   fields, just ensure that the enums occupy only one byte
      BlockStatus status : 8 ;
      MemorySource memsource : 8 ;    // 1 = sbrk, 2 = mmap
      bool m_last ;		      // next block is non-contiguous
      bool suballocated ;
#else
      unsigned char status : 2 ;
      unsigned char memsource : 3 ;   // 1 = sbrk, 2 = mmap
      bool m_last : 1 ;		      // next block is non-contiguous
      bool suballocated : 1 ;
#endif
   public: // methods
      // construction
      void initEmpty(size_t arena_size = sizeof(FrBigAllocHdrBase)) ;
      void init(MemorySource src, size_t arenasize) ;

      // access to state
      size_t first() const { CHKVALID MK_VALID(m_prevsize) return m_prevsize == 0 ; }
      size_t last() const { CHKVALID MK_VALID(m_last) return m_last ; }
      size_t size() const { CHKVALID MK_VALID(m_size) return m_size ; }
      size_t bodySize() const
	 { CHKVALID MK_VALID(m_size)
	   return m_size - sizeof(FrBigAllocHdrBase) ; }
      size_t prevsize() const { CHKVALID MK_VALID(m_prevsize) return m_prevsize ; }
      FrBigAllocHdr *lastBlock() const
	 { CHKVALID MK_VALID(m_lastoffset)
	   return (FrBigAllocHdr*)(((char*)this) + m_lastoffset) ; }
      FrBigAllocHdr *next() const
	 { CHKVALID return last() ? 0 : (FrBigAllocHdr*)(((char*)this) + size()) ; }
      FrBigAllocHdr *prev() const
	 { CHKVALID return first() ? 0 : (FrBigAllocHdr*)(((char*)this) - prevsize()) ; }
      FrBigAllocHdr *thisArena() const ; // fast from first or last block, slow from others in between
      FrBigAllocHdr *nextArena() const
	 { CHKVALID MK_VALID(m_nextarena) return m_nextarena ; }
      FrBigAllocHdr *prevArena() const
	 { CHKVALID MK_VALID(m_prevarena) return m_prevarena ; }
      size_t arenaSize() const 
	 { CHKVALID MK_VALID(m_lastoffset)
	   FrBigAllocHdrBase *tail = (FrBigAllocHdrBase*)lastBlock() ;
	   return m_lastoffset + tail->size() ; }
      FrBigAllocHdr *arenaEnd() const { CHKVALID return (FrBigAllocHdr*)(((char*)this) + arenaSize()) ; }

      // manipulators
      void last(bool l) { CHKVALID MKVALID(m_last) = l ; }
      void setSize(size_t sz) { CHKVALID MKVALID(m_size) = sz ; }
      void growSize(size_t sz) { CHKVALID MKVALID(m_size) += sz ; }
      void setPrevSize(size_t sz) { CHKVALID MKVALID(m_prevsize) = sz ; }
      void markAsFirst() { CHKVALID setPrevSize(0) ; }
      void setLastOffset(size_t ofs) { CHKVALID MKVALID(m_lastoffset) = ofs ; }
      void setLastBlock(FrBigAllocHdrBase *blk)
	 { CHKVALID setLastOffset(((char*)blk) - ((char*)this)) ;
	    MKVALID(blk->m_lastoffset) = m_lastoffset ; }

      FrBigAllocHdr *splitBlock(size_t newsize, bool add_to_freelist = true) ;
      FrBigAllocHdr *mergePredecessor() ;
      bool mergeSuccessor(bool unlinksucc = true, bool unlinkself = true) ;

      FrBigAllocHdr *splitArena(FrBigAllocHdr *last_subblock, bool relink) ;
      bool shrinkArena(FrBigAllocHdr *new_last_subblock) ;
      bool mergeSuccessorArena(FrBigAllocHdr *arena, bool is_linked) ;
      void updateArenaPointers() ;
      void updateArenaPointers(FrBigAllocHdr *merged_arena) ;
      void linkSuccessorArena(FrBigAllocHdr *arena) ;
      void unlinkArena() ;

      void nextArena(FrBigAllocHdrBase *n) { CHKVALID MKVALID(m_nextarena) = (FrBigAllocHdr*)n ; }
      void prevArena(FrBigAllocHdrBase *p) { CHKVALID MKVALID(m_prevarena) = (FrBigAllocHdr*)p ; }

      // utility functions to move between header and body
      static FrBigAllocHdr *header(void *body)
	 { return (FrBigAllocHdr*)(((char*)body)-sizeof(FrBigAllocHdrBase)); }
      char *body() const
	 { return ((char*)this) + sizeof(FrBigAllocHdrBase) ; }

      // debug support
      void invalidate() ;
      void invalidateUnused() ;
      static bool checkArenaChain() ;
      static void printArenaChain() ;
   } ;

#undef CHKVALID

//----------------------------------------------------------------------

class FrBigAllocHdr : public FrBigAllocHdrBase
   {
   public:
      // since the free-list pointers are only valid for free blocks, separate
      //   them into a subclass so that they can be overlaid by user data on
      //   allocated blocks
      FrBigAllocHdr *nextfree, *prevfree ;

   public: // methods
      // construction
      FrBigAllocHdr() { initEmpty() ; }
      FrBigAllocHdr(bool) {}  // do-nothin ctor to avoid clobbering a forcibly-initialized instance
      void initEmpty() ;

      // access to state
      FrBigAllocHdr *nextFree() const { MK_VALID(nextfree) return nextfree ; }
      FrBigAllocHdr *prevFree() const { MK_VALID(prevfree) return prevfree ; }

      // manipulators
      void setNextFree(FrBigAllocHdr *n) { MKVALID(nextfree) = n ; }
      void setPrevFree(FrBigAllocHdr *p) { MKVALID(prevfree) = p ; } 
      void insertInFreelist(FrBigAllocHdr *pred)
	 {
	 prevfree = pred ;
	 nextfree = pred->nextFree() ;
	 pred->nextfree = this ;
	 nextfree->prevfree = this ;
	 }
      void unlinkFromFreelist()
	 {
	 nextfree->prevfree = prevFree() ;
	 prevfree->nextfree = nextFree() ;
	 }
      void replaceInFreelist(FrBigAllocHdr *other)
	 {
	 other->nextfree = nextFree() ;
	 other->prevfree = prevFree() ;
	 nextfree->prevfree = other ;
	 prevfree->nextfree = other ;
	 }

      // debug support
      void invalidate() ;
      void invalidateUnused() ;
   } ;

#endif /* !__FRBALLOC_H_INCLUDED */

// end of file frballoc.h //
