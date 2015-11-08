/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC								*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frbwtlc2.cpp	    FrBWTLocLenList				*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 2004,2005,2006,2009,2012				*/
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

#include "frbwt.h"

#ifdef FrSTRICT_CPLUSPLUS
#  include <cstdlib>
#  include <cstring>
#else
#  include <stdlib.h>
#  include <string.h>	// for GCC 2.x, memcpy()
#endif

/************************************************************************/
/*	Global Variables for class FrBWTLocLenList			*/
/************************************************************************/

FrAllocator FrBWTLocLenList::allocator("FrBWTLocLenList",
				       sizeof(FrBWTLocLenList)) ;

/************************************************************************/
/************************************************************************/

FrBWTLocLenList::FrBWTLocLenList()
{
   m_ranges = 0 ;
   m_numranges = 0 ;
   m_totalsize = 0 ;
   return ;
}

//----------------------------------------------------------------------

FrBWTLocLenList::FrBWTLocLenList(const FrBWTLocLen &loc)
{
   m_ranges = FrNew(FrBWTLocLen) ;
   if (m_ranges)
      {
      new (m_ranges) FrBWTLocLen(loc) ;
      m_numranges = 1 ;
      m_totalsize = loc.rangeSize() ;
      }
   else
      {
      m_numranges = 0 ;
      m_totalsize = 0 ;
      }
   return ;
}

//----------------------------------------------------------------------

FrBWTLocLenList::FrBWTLocLenList(const FrBWTLocLenList &loc)
{
   m_ranges = FrNewN(FrBWTLocLen,loc.m_numranges) ;
   if (m_ranges)
      {
      m_numranges = loc.m_numranges ;
      m_totalsize = loc.m_totalsize ;
      memcpy(m_ranges,loc.m_ranges,m_numranges * sizeof(FrBWTLocLen)) ;
      }
   else
      {
      m_numranges = 0 ;
      m_totalsize = 0 ;
      }
   return ;
}

//----------------------------------------------------------------------

FrBWTLocLenList &FrBWTLocLenList::operator = (const FrBWTLocLenList &loc)
{
   m_ranges = FrNewN(FrBWTLocLen,loc.m_numranges) ;
   if (m_ranges)
      {
      m_numranges = loc.m_numranges ;
      m_totalsize = loc.m_totalsize ;
      memcpy(m_ranges,loc.m_ranges,m_numranges * sizeof(FrBWTLocLen)) ;
      }
   else
      {
      m_numranges = 0 ;
      m_totalsize = 0 ;
      }
   return *this ;
}

//----------------------------------------------------------------------

FrBWTLocLenList::~FrBWTLocLenList()
{
   FrFree(m_ranges) ;
   m_ranges = 0 ;
   m_numranges = 0 ;
   m_totalsize = 0 ;
   return ;
}

//----------------------------------------------------------------------

size_t FrBWTLocLenList::totalExtent() const
{
   if (m_numranges > 0)
      return m_ranges[m_numranges-1].pastEnd() - m_ranges[0].first() ;
   else
      return 0 ;
}

//----------------------------------------------------------------------

FrBWTLocation FrBWTLocLenList::range(size_t which) const
{
   FrBWTLocation r(~0,0) ;
   if (which < m_numranges)
      r = m_ranges[which] ;
   return r ;
}

//----------------------------------------------------------------------

size_t FrBWTLocLenList::rangeSize(size_t which) const
{
   if (which < m_numranges)
      return m_ranges[which].rangeSize() ;
   else
      return 0 ;
}

//----------------------------------------------------------------------

size_t FrBWTLocLenList::sourceLength(size_t which) const
{
   if (which < m_numranges)
      return 0 ; //FIXME: m_ranges[which].sourceLength() ;
   else
      return 0 ;
}

//----------------------------------------------------------------------

bool FrBWTLocLenList::insert(FrBWTLocLen loc)
{
   if (loc.isEmpty())
      return true ;			// trivially successful
   size_t lo = 0 ;
   size_t hi = m_numranges ;
   while (hi > lo)
      {
      size_t mid = (lo + hi) / 2 ;
      FrBWTLocLen *midloc = &m_ranges[mid] ;
      if (loc.pastEnd() < midloc->first())
	 hi = mid ;
      else if (loc.first() > midloc->pastEnd())
	 lo = mid + 1 ;
      else
	 {
	 // we've found an existing range that overlaps or immediately
	 //  abuts the new one, so expand the existing range
	 if (loc.first() < midloc->first())
	    {
	    m_totalsize += (midloc->first() - loc.first()) ;
	    midloc->adjustStart(loc.first()) ;
	    }
	 if (loc.pastEnd() > midloc->pastEnd())
	    {
	    m_totalsize += (loc.pastEnd() - midloc->pastEnd()) ;
	    midloc->adjustEnd(loc.pastEnd()) ;
	    }
	 return true ;
	 }
      }
   // we need to insert a new range at 'lo'
   FrBWTLocLen *new_ranges = FrNewR(FrBWTLocLen,m_ranges,m_numranges+1) ;
   if (new_ranges)
      {
      m_ranges = new_ranges ;
      for (size_t i = m_numranges ; i > lo ; i--)
	 m_ranges[i] = m_ranges[i-1] ;
      new (m_ranges + lo) FrBWTLocLen(loc) ;
      m_numranges++ ;
      m_totalsize += loc.rangeSize() ;
      return true ;
      }
   else
      FrNoMemory("while expanding list of matches") ;
   // if we get here, there was some problem inserting the new range
   return false ;
}

//----------------------------------------------------------------------

bool FrBWTLocLenList::append(FrBWTLocation loc, size_t srclen)
{
   if (loc.isEmpty())
      return true ;			// nothing to do, trivially successful
   if (m_numranges > 0 && m_ranges[m_numranges-1].inRange(loc.first()))
      {
      FrBWTLocLen *lastrange = &m_ranges[m_numranges-1] ;
      if (lastrange->pastEnd() < loc.pastEnd())
	 {
	 m_totalsize += (loc.pastEnd() - lastrange->pastEnd()) ;
	 lastrange->adjustEnd(loc.pastEnd()) ;
	 }
      return true ;
      }
   //FIXME
   (void)srclen;
   return false ;
}

//----------------------------------------------------------------------

bool FrBWTLocLenList::append(FrBWTLocLen loc)
{
   if (loc.isEmpty())
      return true ;			// nothing to do, trivially successful
   if (m_numranges > 0 && m_ranges[m_numranges-1].inRange(loc.first()))
      {
      FrBWTLocLen *lastrange = &m_ranges[m_numranges-1] ;
      if (lastrange->pastEnd() < loc.pastEnd())
	 {
	 m_totalsize += (loc.pastEnd() - lastrange->pastEnd()) ;
	 lastrange->adjustEnd(loc.pastEnd()) ;
	 }
      return true ;
      }
   FrBWTLocLen *new_ranges = FrNewR(FrBWTLocLen,m_ranges,m_numranges+1) ;
   if (new_ranges)
      {
      m_ranges = new_ranges ;
      new (m_ranges + m_numranges) FrBWTLocLen(loc) ;
      m_numranges++ ;
      m_totalsize += loc.rangeSize() ;
      return true ;
      }
   else
      FrNoMemory("while appending to list of matches") ;
   // if we get here, there was some problem inserting the new range
   return false ;
}

//----------------------------------------------------------------------

bool FrBWTLocLenList::append(uint32_t loc, size_t srclen)
{
   if (m_numranges > 0 && m_ranges[m_numranges-1].pastEnd() == loc)
      {
      m_ranges[m_numranges-1].adjustEnd(loc+1) ;
      m_totalsize++ ;
      return true ;
      }
   FrBWTLocLen *new_ranges = FrNewR(FrBWTLocLen,m_ranges,m_numranges+1) ;
   if (new_ranges)
      {
      m_ranges = new_ranges ;
      new (m_ranges + m_numranges) FrBWTLocLen(loc,loc+1,srclen) ;
      m_numranges++ ;
      m_totalsize++ ;
      return true ;
      }
   else
      FrNoMemory("while appending to list of matches") ;
   // if we get here, there was some problem inserting the new range
   return false ;
}

//----------------------------------------------------------------------

bool FrBWTLocLenList::remove(size_t which)
{
   if (which < m_numranges)
      {
      if (which + 1 < m_numranges)
	 memcpy(m_ranges + which, m_ranges + which + 1,
		(m_numranges - which) * sizeof(FrBWTLocLen)) ;
      m_numranges-- ;
      return true ;
      }
   // if we get here, there was some problem removing the specified range
   return false ;
}

//----------------------------------------------------------------------

static int compare_ranges(const void *r1, const void *r2)
{
   const FrBWTLocLen *range1 = (FrBWTLocLen*)r1 ;
   const FrBWTLocLen *range2 = (FrBWTLocLen*)r2 ;
   if (range1->first() < range2->first())
      return -1 ;
   else if (range1->first() > range2->first())
      return +1 ;
   else if (range1->pastEnd() < range2->pastEnd())
      return -1 ;
   else if (range1->pastEnd() > range2->pastEnd())
      return +1 ;
   return 0 ;
}

//----------------------------------------------------------------------

void FrBWTLocLenList::clear()
{
   delete m_ranges ;	m_ranges = 0 ;
   m_numranges = 0 ;
   m_totalsize = 0 ;
   return ;
}

//----------------------------------------------------------------------

void FrBWTLocLenList::sort()
{
   qsort(m_ranges,m_numranges,sizeof(FrBWTLocLen),compare_ranges) ;
   return ;
}

/************************************************************************/
/*	Methods for class FrBWTIndex					*/
/************************************************************************/

FrBWTLocLenList *FrBWTIndex::extendMatch(const FrBWTLocLenList *matches,
					 uint32_t *nextIDs, size_t num_IDs,
					 size_t *source_lengths,
					 FrBWTLocLenList *unextendable) const
{
   FrBWTLocLenList *newlist = new FrBWTLocLenList ;
   if (!newlist)
      {
      FrNoMemory("while extending match") ;
      return 0 ;
      }
   if (!matches || matches->totalMatches() == 0 || num_IDs == 0)
      return newlist ;
   // naive implementation: after ensuring that the IDs are in ascending
   //   order, simply apply each ID in order to each range in 'matches',
   //   and append the result to 'newlist'
   //
   // a more efficient implementation would take into account that
   //   the subranges defined by the different IDs are themselves ascending,
   //   as are the ranges defined by the existing matches
   for (size_t i = 0 ; i < num_IDs ; i++)
      {
      FrBWTLocation nextrange(unigram(nextIDs[i])) ;
      size_t newlen = source_lengths[i] ;
      for (size_t j = 0 ; j < matches->numRanges() ; j++)
	 {
	 FrBWTLocation oldrange = matches->range(j) ;
	 FrBWTLocation range(extendMatch(oldrange,nextrange)) ;
	 if (range.nonEmpty())
	    {
	    newlist->append(range,matches->sourceLength(j)+newlen) ;
	    // the next existing range will come later, so that also means
	    //   that the next extended match will follow the current one,
	    //   which means we can limit the search on the next iteration
	    nextrange.adjustStart(range.pastEnd()) ;
	    if (unextendable)
	       {
	       if (range.first() > oldrange.first())
		  unextendable->append(FrBWTLocation(oldrange.first(),
						     range.first()),
				       matches->sourceLength(j)) ;
	       if (range.pastEnd() < oldrange.pastEnd())
		  unextendable->append(FrBWTLocation(range.pastEnd(),
						     oldrange.pastEnd()),
				       matches->sourceLength(j)) ;
	       }
	    }
	 else
	    {
	    if (unextendable)
	       unextendable->append(oldrange,matches->sourceLength(j)) ;
	    if (nextrange.inRange(range.first()))
	       nextrange.adjustStart(range.first()) ;
	    }
	 }
      }
   return newlist ;
}

//----------------------------------------------------------------------

FrBWTLocLenList *FrBWTIndex::extendMatch(const FrBWTLocLenList *matches,
					 FrBWTLocation next, size_t newlen,
					 FrBWTLocLenList *unextendable) const
{
   FrBWTLocLenList *newlist = new FrBWTLocLenList ;
   if (!newlist)
      {
      FrNoMemory("while extending match") ;
      return 0 ;
      }
   if (!matches || matches->totalMatches() == 0 || next.isEmpty())
      return newlist ;
   for (size_t i = 0 ; i < matches->numRanges() ; i++)
      {
      FrBWTLocation oldrange = matches->range(i) ;
      FrBWTLocation range(extendMatch(oldrange,next)) ;
      if (range.nonEmpty())
	 {
	 newlist->append(range,matches->sourceLength(i)+newlen) ;
	 // the next existing range will come later, so that also means
	 //   that the next extended match will follow the current one,
	 //   which means we can limit the search on the next iteration
	 next.adjustStart(range.pastEnd()) ;
	 if (unextendable)
	    {
	    if (range.first() > oldrange.first())
	       unextendable->append(FrBWTLocation(oldrange.first(),
						  range.first()),
				    matches->sourceLength(i)) ;
	    if (range.pastEnd() < oldrange.pastEnd())
	       unextendable->append(FrBWTLocation(range.pastEnd(),
						  oldrange.pastEnd()),
				    matches->sourceLength(i)) ;
	    }
	 }
      else
	 {
	 if (unextendable)
	    unextendable->append(oldrange,matches->sourceLength(i)) ;
	 if (next.inRange(range.first()))
	    next.adjustStart(range.first()) ;
	 }
      }
   return newlist ;
}

//----------------------------------------------------------------------

FrBWTLocLenList *
   FrBWTIndex::extendMatchGap(const FrBWTLocLenList *matches,
			      FrBWTLocation next, size_t gap_size,
			      FrBWTLocLenList * /*unextendable*/) const
{
   FrBWTLocLenList *newlist = new FrBWTLocLenList ;
   if (!newlist)
      {
      FrNoMemory("while extending match") ;
      return 0 ;
      }
   if (!matches || matches->totalMatches() == 0 || next.isEmpty())
      return newlist ;
   // this is not the most efficient implementation, particularly for
   // gap sizes > 1 (should take advantage of spatial locality in the
   // index)
   size_t succ_num = gap_size + 1 ;
   for (size_t loc = next.first() ; loc < next.pastEnd() ; loc++)
      {
      for (size_t i = 0 ; i < matches->numRanges() ; i++)
	 {
	 FrBWTLocation oldrange = matches->range(i) ;
	 if (oldrange.inRange(getNthSuccessor(loc,succ_num)))
	    newlist->append(loc,matches->sourceLength(i)+1) ;
	 }
      }
   return newlist ;
}

//----------------------------------------------------------------------

FrBWTLocLenList *
   FrBWTIndex::extendMatchGap(const FrBWTLocLenList *matches,
			      uint32_t next_ID, size_t gap_size,
			      FrBWTLocLenList *unextendable) const
{
   return extendMatchGap(matches,unigram(next_ID),gap_size,unextendable) ;
}

//----------------------------------------------------------------------

// end of file frbwtlc2.cpp //
