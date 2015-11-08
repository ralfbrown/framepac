/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC								*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frbwtloc.cpp	    FrBWTLocationList				*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 2004,2005,2006,2007,2009,2012				*/
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
/* 	Methods for class FrBWTLocationList				*/
/************************************************************************/

FrBWTLocationList::FrBWTLocationList()
{
   m_ranges = 0 ;
   m_numranges = 0 ;
   m_totalsize = 0 ;
//   m_startpos = 0 ;
   return ;
}

//----------------------------------------------------------------------

FrBWTLocationList::FrBWTLocationList(const FrBWTLocation &loc)
{
   m_ranges = FrNew(FrBWTLocation) ;
   if (m_ranges)
      {
      new (m_ranges) FrBWTLocation(loc) ;
      m_numranges = 1 ;
      m_totalsize = loc.rangeSize() ;
      }
   else
      {
      m_numranges = 0 ;
      m_totalsize = 0 ;
      }
//   m_startpos = 0 ;
   return ;
}

//----------------------------------------------------------------------

FrBWTLocationList::FrBWTLocationList(const FrBWTLocationList &loc)
{
   m_ranges = FrNewN(FrBWTLocation,loc.m_numranges) ;
   if (m_ranges)
      {
      m_numranges = loc.m_numranges ;
      m_totalsize = loc.m_totalsize ;
      memcpy(m_ranges,loc.m_ranges,m_numranges * sizeof(FrBWTLocation)) ;
      }
   else
      {
      m_numranges = 0 ;
      m_totalsize = 0 ;
      }
//   m_startpos = loc.m_startpos ;
   return ;
}

//----------------------------------------------------------------------

FrBWTLocationList &FrBWTLocationList::operator = (const FrBWTLocationList &loc)
{
   m_ranges = FrNewN(FrBWTLocation,loc.m_numranges) ;
   if (m_ranges)
      {
      m_numranges = loc.m_numranges ;
      m_totalsize = loc.m_totalsize ;
      memcpy(m_ranges,loc.m_ranges,m_numranges * sizeof(FrBWTLocation)) ;
      }
   else
      {
      m_numranges = 0 ;
      m_totalsize = 0 ;
      }
//   m_startpos = loc.m_startpos ;
   return *this ;
}

//----------------------------------------------------------------------

FrBWTLocationList::~FrBWTLocationList()
{
   FrFree(m_ranges) ;
   m_ranges = 0 ;
   m_numranges = 0 ;
   m_totalsize = 0 ;
   return ;
}

//----------------------------------------------------------------------

size_t FrBWTLocationList::totalExtent() const
{
   if (m_numranges > 0)
      return m_ranges[m_numranges-1].pastEnd() - m_ranges[0].first() ;
   else
      return 0 ;
}

//----------------------------------------------------------------------

FrBWTLocation FrBWTLocationList::range(size_t which) const
{
   FrBWTLocation r(~0,0) ;
   if (which < m_numranges)
      r = m_ranges[which] ;
   return r ;
}

//----------------------------------------------------------------------

size_t FrBWTLocationList::rangeSize(size_t which) const
{
   if (which < m_numranges)
      return m_ranges[which].rangeSize() ;
   else
      return 0 ;
}

//----------------------------------------------------------------------

bool FrBWTLocationList::covered(uint32_t location) const
{
   size_t lo = 0 ;
   size_t hi = m_numranges ;
   while (hi > lo)
      {
      size_t mid = (lo + hi) / 2 ;
      FrBWTLocation *midloc = &m_ranges[mid] ;
      if (location < midloc->first())
	 hi = mid ;
      else if (location > midloc->last())
	 lo = mid + 1 ;
      else
	 return true ;			// 'location' is within 'midloc'
      }
   // if we get here, there is no range containing 'location'
   return false ;
}

//----------------------------------------------------------------------

void FrBWTLocationList::precedingRange(uint32_t location,
				       size_t &first, size_t &pastend) const
{
   if (m_numranges)
      {
      if (location > m_ranges[m_numranges-1].last())
	 {
	 first = m_ranges[m_numranges-1].first() ;
	 pastend = m_ranges[m_numranges-1].pastEnd() ;
	 return ;
	 }
      if (location > m_ranges[0].first())
	 {
	 size_t lo = 0 ;
	 size_t hi = m_numranges - 1 ;
	 while (hi > lo)
	    {
	    size_t mid = (lo + hi) / 2 ;
	    if (location > m_ranges[mid+1].first())
	       lo = mid + 1 ;
	    else
	       hi = mid ;
	    }
	 first = m_ranges[lo].first() ;
	 pastend = m_ranges[lo].pastEnd() ;
	 return ;
	 }
      }
   // if we get here, there is no range prior to 'location'
   first = ~0 ;
   pastend = 0 ;
   return ;
}

//----------------------------------------------------------------------

bool FrBWTLocationList::insert(FrBWTLocation loc)
{
   if (loc.isEmpty())
      return true ;			// trivially successful
   size_t lo = 0 ;
   size_t hi = m_numranges ;
   while (hi > lo)
      {
      size_t mid = (lo + hi) / 2 ;
      FrBWTLocation *midloc = &m_ranges[mid] ;
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
   FrBWTLocation *new_ranges = FrNewR(FrBWTLocation,m_ranges,m_numranges+1) ;
   if (new_ranges)
      {
      m_ranges = new_ranges ;
      for (size_t i = m_numranges ; i > lo ; i--)
	 m_ranges[i] = m_ranges[i-1] ;
      new (m_ranges + lo) FrBWTLocation(loc) ;
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

bool FrBWTLocationList::append(const FrBWTLocationList *locs)
{
   if (locs)
      {
      for (size_t i = 0 ; i < locs->numRanges() ; i++)
	 append(locs->range(i)) ;
      }
   return true ;
}

//----------------------------------------------------------------------

bool FrBWTLocationList::append(FrBWTLocation loc)
{
   if (loc.isEmpty())
      return true ;			// nothing to do, trivially successful
   if (m_numranges > 0 && m_ranges[m_numranges-1].inOrAbutsRange(loc.first()))
      {
      FrBWTLocation *lastrange = &m_ranges[m_numranges-1] ;
      if (lastrange->pastEnd() < loc.pastEnd())
	 {
	 m_totalsize += (loc.pastEnd() - lastrange->pastEnd()) ;
	 lastrange->adjustEnd(loc.pastEnd()) ;
	 }
      return true ;
      }
   FrBWTLocation *new_ranges = FrNewR(FrBWTLocation,m_ranges,m_numranges+1) ;
   if (new_ranges)
      {
      m_ranges = new_ranges ;
      new (m_ranges + m_numranges) FrBWTLocation(loc) ;
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

bool FrBWTLocationList::append(uint32_t loc)
{
   if (m_numranges > 0 && m_ranges[m_numranges-1].pastEnd() == loc)
      {
      m_ranges[m_numranges-1].adjustEnd(loc+1) ;
      m_totalsize++ ;
      return true ;
      }
   FrBWTLocation *new_ranges = FrNewR(FrBWTLocation,m_ranges,m_numranges+1) ;
   if (new_ranges)
      {
      m_ranges = new_ranges ;
      new (m_ranges + m_numranges) FrBWTLocation(loc,loc+1) ;
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

bool FrBWTLocationList::merge(const FrBWTLocationList &loc)
{
   // this isn't the most efficient implementation, but it will do the job
   //   for now
   for (size_t i = 0 ; i < loc.numRanges() ; i++)
      {
      if (!insert(loc.range(i)))
	 return false ;
      }
   return true ;
}

//----------------------------------------------------------------------

bool FrBWTLocationList::remove(size_t which)
{
   if (which < m_numranges)
      {
      if (which + 1 < m_numranges)
	 memcpy(m_ranges + which, m_ranges + which + 1,
		(m_numranges - which) * sizeof(FrBWTLocation)) ;
      m_numranges-- ;
      return true ;
      }
   // if we get here, there was some problem removing the specified range
   return false ;
}

//----------------------------------------------------------------------

static int compare_ranges(const void *r1, const void *r2)
{
   const FrBWTLocation *range1 = (FrBWTLocation*)r1 ;
   const FrBWTLocation *range2 = (FrBWTLocation*)r2 ;
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

void FrBWTLocationList::clear()
{
   delete m_ranges ;	m_ranges = 0 ;
   m_numranges = 0 ;
   m_totalsize = 0 ;
   return ;
}

//----------------------------------------------------------------------

void FrBWTLocationList::sort()
{
   qsort(m_ranges,m_numranges,sizeof(FrBWTLocation),compare_ranges) ;
   return ;
}

/************************************************************************/
/*	Methods for class FrBWTIndex					*/
/************************************************************************/

FrBWTLocationList *FrBWTIndex::extendMatch(const FrBWTLocationList *matches,
					   uint32_t *nextIDs, size_t num_IDs,
					   bool IDs_are_sorted,
					   FrBWTLocationList *unextendable)
    const
{
   FrBWTLocationList *newlist = new FrBWTLocationList ;
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
   if (!IDs_are_sorted)
      FrSortWordIDs(nextIDs,num_IDs) ;
   for (size_t i = 0 ; i < num_IDs ; i++)
      {
      FrBWTLocation nextrange(unigram(nextIDs[i])) ;
      for (size_t j = 0 ; j < matches->numRanges() ; j++)
	 {
	 FrBWTLocation oldrange = matches->range(j) ;
	 FrBWTLocation range(extendMatch(oldrange,nextrange)) ;
	 if (range.nonEmpty())
	    {
	    newlist->append(range) ;
	    // the next existing range will come later, so that also means
	    //   that the next extended match will follow the current one,
	    //   which means we can limit the search on the next iteration
	    nextrange.adjustStart(range.pastEnd()) ;
	    if (unextendable)
	       {
	       if (range.first() > oldrange.first())
		  unextendable->append(FrBWTLocation(oldrange.first(),
						     range.first())) ;
	       if (range.pastEnd() < oldrange.pastEnd())
		  unextendable->append(FrBWTLocation(range.pastEnd(),
						     oldrange.pastEnd())) ;
	       }
	    }
	 else
	    {
	    if (unextendable)
	       unextendable->append(oldrange) ;
	    if (nextrange.inRange(range.first()))
	       nextrange.adjustStart(range.first()) ;
	    }
	 }
      }
   return newlist ;
}

//----------------------------------------------------------------------

FrBWTLocationList *FrBWTIndex::extendMatch(const FrBWTLocationList *matches,
					   FrBWTLocation next,
					   FrBWTLocationList *unextendable)
    const
{
   FrBWTLocationList *newlist = new FrBWTLocationList ;
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
	 newlist->append(range) ;
	 // the next existing range will come later, so that also means
	 //   that the next extended match will follow the current one,
	 //   which means we can limit the search on the next iteration
	 next.adjustStart(range.pastEnd()) ;
	 if (unextendable)
	    {
	    if (range.first() > oldrange.first())
	       unextendable->append(FrBWTLocation(oldrange.first(),
						  range.first())) ;
	    if (range.pastEnd() < oldrange.pastEnd())
	       unextendable->append(FrBWTLocation(range.pastEnd(),
						  oldrange.pastEnd())) ;
	    }
	 }
      else
	 {
	 if (unextendable)
	    unextendable->append(oldrange) ;
	 if (next.inRange(range.first()))
	    next.adjustStart(range.first()) ;
	 }
      }
   return newlist ;
}

//----------------------------------------------------------------------

FrBWTLocationList *FrBWTIndex::restrictMatch(const FrBWTLocationList *matches,
					     FrBWTLocation next) const
{
   FrBWTLocationList *newlist = new FrBWTLocationList ;
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
	 // chop off the 'next' word to get the subrange of 'oldrange' which
	 //   is within the context of 'next'
	 size_t r_first = getSuccessor(range.first()) ;
	 size_t r_pastend = getSuccessor(range.last()) + 1 ;
	 if (r_pastend - r_first == range.rangeSize())
	    {
	    FrBWTLocation new_range(r_first,r_pastend) ;
	    newlist->append(new_range) ;
	    }
	 else
	    {
	    for (size_t j = range.first() ; j < range.pastEnd() ; j++)
	       {
	       newlist->append(getSuccessor(j)) ;
	       }
	    }
	 // the next existing range will come later, so that also means
	 //   that the next extended match will follow the current one,
	 //   which means we can limit the search on the next iteration
	 next.adjustStart(range.pastEnd()) ;
	 }
      else
	 {
	 if (next.inRange(range.first()))
	    next.adjustStart(range.first()) ;
	 }
      }
   return newlist ;
}

//----------------------------------------------------------------------

FrBWTLocationList *FrBWTIndex::extendMatch(const FrBWTLocationList *matches,
					   FrBWTLocation next,
					   size_t next_words,
					   FrBWTLocationList *unextendable)
    const
{
   if (next_words == 1)
      return extendMatch(matches,next,unextendable) ;
   FrBWTLocationList *newlist = new FrBWTLocationList ;
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
      FrBWTLocation range(extendMatch(oldrange,next,next_words)) ;
      if (range.nonEmpty())
	 {
	 newlist->append(range) ;
	 // the next existing range will come later, so that also means
	 //   that the next extended match will follow the current one,
	 //   which means we can limit the search on the next iteration
	 next.adjustStart(range.pastEnd()) ;
	 if (unextendable)
	    {
	    if (range.first() > oldrange.first())
	       unextendable->append(FrBWTLocation(oldrange.first(),
						  range.first())) ;
	    if (range.pastEnd() < oldrange.pastEnd())
	       unextendable->append(FrBWTLocation(range.pastEnd(),
						  oldrange.pastEnd())) ;
	    }
	 }
      else
	 {
	 if (unextendable)
	    unextendable->append(oldrange) ;
	 if (next.inRange(range.first()))
	    next.adjustStart(range.first()) ;
	 }
      }
   return newlist ;
}

//----------------------------------------------------------------------

FrBWTLocationList *
   FrBWTIndex::extendMatchGap(const FrBWTLocationList *matches,
			      FrBWTLocation next, size_t gap_size,
			      FrBWTLocationList * /*unextendable*/)
const
{
   FrBWTLocationList *newlist = new FrBWTLocationList ;
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
	    newlist->append(loc) ;
	 }
      }
   return newlist ;
}

//----------------------------------------------------------------------

FrBWTLocationList *
   FrBWTIndex::extendMatchGap(const FrBWTLocationList *matches,
			      uint32_t next_ID, size_t gap_size,
			      FrBWTLocationList *unextendable)
const
{
   return extendMatchGap(matches,unigram(next_ID),gap_size,unextendable) ;
}

//----------------------------------------------------------------------

// end of file frbwtloc.cpp //
