/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC								*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File: frbpriq.cpp	      class FrBoundedPriQueue			*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 2002,2003,2009,2013,2015				*/
/*	    Ralf Brown/Carnegie Mellon University			*/
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

#if defined(__GNUC__)
#  pragma implementation "frbpriq.h"
#endif

#include "frbpriq.h"

#ifdef FrSTRICT_CPLUSPLUS
#  include <cstring>
#  include <iostream>
#else
#  include <iostream.h>
#  include <string.h>
#endif

/************************************************************************/
/*	Global Variables						*/
/************************************************************************/

FrAllocator FrBoundedPriQueue::allocator("FrBoundedPriQueue",
					  sizeof(FrBoundedPriQueue)) ;
FrAllocator *FrBoundedPriQueue::e_allocator = nullptr ;
FrAllocator *FrBoundedPriQueue::p_allocator = nullptr ;
size_t FrBoundedPriQueue::alloc_size = 0 ;
double FrBoundedPriQueue::null_priority = 0.0 ;

/************************************************************************/
/*	Methods for class FrBoundedPriQueue				*/
/************************************************************************/

FrBoundedPriQueue::FrBoundedPriQueue(size_t max_size, bool descending,
				     bool copy_data)
{
   if (max_size < 2)
      max_size = 2 ;
   q_size = max_size ;
   q_head = 0 ;
   q_tail = 0 ;
   if (p_allocator && q_size == alloc_size)
      priorities = (double*)p_allocator->allocate() ;
   else
      priorities = FrNewN(double,q_size) ;
   if (e_allocator && q_size == alloc_size)
      entries = (FrObject**)e_allocator->allocate() ;
   else
      entries = FrNewN(FrObject*,q_size) ;
   sort_descending = descending ;
   copy_objects = copy_data ;
   if (!priorities || !entries)
      q_size = 0 ;
   return ;
}

//----------------------------------------------------------------------

FrBoundedPriQueue::FrBoundedPriQueue(const FrBoundedPriQueue *old)
{
   q_size = old->q_size ;
   q_head = 0 ;
   q_tail = old->q_tail - old->q_head ;
   sort_descending = old->sort_descending ;
   copy_objects = old->copy_objects ;
   if (p_allocator && q_size == alloc_size)
      priorities = (double*)p_allocator->allocate() ;
   else
      priorities = FrNewN(double,q_size) ;
   if (e_allocator && q_size == alloc_size)
      entries = (FrObject**)e_allocator->allocate() ;
   else
      entries = FrNewN(FrObject*,q_size) ;
   if (priorities)
      memcpy(priorities,old->priorities+old->q_head,
	     q_tail*sizeof(priorities[0])) ;
   if (entries)
      memcpy(entries,old->entries+old->q_head,q_tail*sizeof(entries[0])) ;
   if (!priorities || !entries)
      q_size = 0 ;
   return ;
}

//----------------------------------------------------------------------

FrBoundedPriQueue::~FrBoundedPriQueue()
{
   if (p_allocator && q_size == alloc_size)
      p_allocator->release(priorities) ;
   else
      FrFree(priorities) ;
   if (e_allocator && q_size == alloc_size)
      e_allocator->release(entries) ;
   else
      FrFree(entries) ;
   q_size = q_head = q_tail = 0 ;
   priorities = 0 ;
   entries = nullptr ;
   sort_descending = true ;
   copy_objects = false ;
   return ;
}

//----------------------------------------------------------------------

FrObjectType FrBoundedPriQueue::objType() const
{
   return OT_FrBoundedPriQueue ;
}

//----------------------------------------------------------------------

FrObjectType FrBoundedPriQueue::objSuperclass() const
{
   return OT_FrObject ;
}

//----------------------------------------------------------------------

const char *FrBoundedPriQueue::objTypeName() const
{
   return "FrBoundedPriQueue" ;
}

//----------------------------------------------------------------------

ostream &FrBoundedPriQueue::printValue(ostream &output) const
{
   output << '#' << objTypeName() << '(' << q_size << '/' << q_tail-q_head
	  << '/' << (sort_descending ? 'D' : 'A') << '/'
	  << (copy_objects ? '1' : '0') ;
   for (size_t i = q_head ; i < q_tail ; i++)
      output << ' ' << priorities[i] << '/' << entries[i] ;
   output << ')' ;
   return output ;
}

//----------------------------------------------------------------------

bool FrBoundedPriQueue::setAllocators(size_t queue_size)
{
   if (e_allocator)
      {
      delete e_allocator ;
      e_allocator = nullptr ;
      }
   if (p_allocator)
      {
      delete p_allocator ;
      p_allocator = nullptr ;
      }
   bool good = false ;
   alloc_size = 0 ;
   if (queue_size > 0 && queue_size < FrFOOTER_OFS / (4*sizeof(double)))
      {
      e_allocator = new FrAllocator("FrBndPriQue ent",
				    queue_size*sizeof(FrObject*)) ;
      p_allocator = new FrAllocator("FrBndPriQue pri",
				    queue_size*sizeof(double)) ;
      good = (e_allocator && p_allocator) ;
      }
   else if (queue_size > 0)
      good = true ;
   return good ;
}

//----------------------------------------------------------------------

double FrBoundedPriQueue::lastPriority() const
{
   if (q_tail > q_head)
      return priorities[q_tail-1] ;
   else
      return null_priority ;
}

//----------------------------------------------------------------------

size_t FrBoundedPriQueue::location(const FrObject *obj) const
{
   if (copy_objects)
      {
      for (size_t i = q_head ; i < q_tail ; i++)
	 {
	 if (::equal(obj,entries[i]))
	    return i - q_head ;
	 }
      }
   else
      {
      for (size_t i = q_head ; i < q_tail ; i++)
	 {
	 if (obj == entries[i])
	    return i - q_head ;
	 }
      }
   return FrBPQ_NOT_FOUND ;
}

//----------------------------------------------------------------------

size_t FrBoundedPriQueue::location(const FrObject *obj,
				    FrCompareFunc cmp_fn) const
{
   if (!cmp_fn)
      return location(obj) ;
   for (size_t i = q_head ; i < q_tail ; i++)
      {
      if (cmp_fn(obj,entries[i]))
	 return i - q_head ;
      }
   return FrBPQ_NOT_FOUND ;
}

//----------------------------------------------------------------------

size_t FrBoundedPriQueue::location(double priority) const
{
   if (q_tail > q_head)
      {
      // perform a binary search for the first matching entry
      size_t lo = q_head ;
      size_t hi = q_tail ;
      if (sort_descending)
	 {
	 while (hi > lo)
	    {
	    size_t mid = (hi + lo) >> 1 ;
	    double midpri = priorities[mid] ;
	    if (priority < midpri)
	       lo = mid + 1 ;
	    else // if (priority >= midpri)
	       hi = mid ;
	    }
	 }
      else // ascending
	 {
	 while (hi > lo)
	    {
	    size_t mid = (hi + lo) >> 1 ;
	    double midpri = priorities[mid] ;
	    if (priority > midpri)
	       lo = mid + 1 ;
	    else // if (priority <= midpri)
	       hi = mid ;
	    }
	 }
      if (priorities[lo] == priority)
	 return lo - q_head ;
      }
   return FrBPQ_NOT_FOUND ;
}

//----------------------------------------------------------------------

size_t FrBoundedPriQueue::insertionPoint(double priority) const
{
   if (q_tail == q_head)
      return q_head ;
   // perform a binary search for the entry
   size_t lo = q_head ;
   size_t hi = q_tail ;
   if (sort_descending)
      {
      // insert after all other entries with same or higher priority value
      while (hi > lo)
	 {
	 size_t mid = (hi + lo) >> 1 ;
	 double midpri = priorities[mid] ;
	 if (priority <= midpri)
	    lo = mid + 1 ;
	 else // if (priority > midpri)
	    hi = mid ;
	 }
      }
   else // ascending
      {
      // insert after all other entries with same or lower priority value
      while (hi > lo)
	 {
	 size_t mid = (hi + lo) >> 1 ;
	 double midpri = priorities[mid] ;
	 if (priority >= midpri)
	    lo = mid + 1 ;
	 else // if (priority < midpri)
	    hi = mid ;
	 }
      }
   return lo ;				// lo == hi is now insertion point
}

//----------------------------------------------------------------------

bool FrBoundedPriQueue::push(FrObject *obj, double priority)
{
   if (q_head > 0)
      {
      size_t pos = insertionPoint(priority) ;
      // shift everything before insertion point down one position
      for (size_t i = q_head - 1 ; i < pos ; i++)
	 {
	 priorities[i] = priorities[i+1] ;
	 entries[i] = entries[i+1] ;
	 }
      q_head-- ;
      pos-- ;
      priorities[pos] = priority ;
      entries[pos] = (copy_objects && obj) ? obj->deepcopy() : obj ;
      return true ;
      }
   else
      {
      size_t pos = insertionPoint(priority) ;
      if (pos >= q_size)		// lower priority than anything in Q?
	 return false ;
      // shift everything after insertion point up one position
      if (q_tail >= q_size)		// if Q already full, lose last entry
	 {
	 q_tail-- ;
	 if (copy_objects)
	    free_object(entries[q_tail]) ;
	 }
      for (size_t i = q_tail ; i > pos ; i--)
	 {
	 priorities[i] = priorities[i-1] ;
	 entries[i] = entries[i-1] ;
	 }
      q_tail++ ;
      priorities[pos] = priority ;
      entries[pos] = (copy_objects && obj) ? obj->deepcopy() : obj ;
      return true ;
      }
}

//----------------------------------------------------------------------

bool FrBoundedPriQueue::push(FrObject *obj, double priority, size_t &pos)
{
   if (q_head > 0)
      {
      pos = insertionPoint(priority) ;
      // shift everything before insertion point down one position
      for (size_t i = q_head - 1 ; i < pos ; i++)
	 {
	 priorities[i] = priorities[i+1] ;
	 entries[i] = entries[i+1] ;
	 }
      q_head-- ;
      pos-- ;
      priorities[pos] = priority ;
      entries[pos] = (copy_objects && obj) ? obj->deepcopy() : obj ;
      return true ;
      }
   else
      {
      pos = insertionPoint(priority) ;
      if (pos >= q_size)		// lower priority than anything in Q?
	 {
	 pos = FrBPQ_NOT_FOUND ;
	 return false ;
	 }
      // shift everything after insertion point up one position
      if (q_tail >= q_size)		// if Q already full, lose last entry
	 {
	 q_tail-- ;
	 if (copy_objects)
	    free_object(entries[q_tail]) ;
	 }
      for (size_t i = q_tail ; i > pos ; i--)
	 {
	 priorities[i] = priorities[i-1] ;
	 entries[i] = entries[i-1] ;
	 }
      q_tail++ ;
      priorities[pos] = priority ;
      entries[pos] = (copy_objects && obj) ? obj->deepcopy() : obj ;
      return true ;
      }
}

//----------------------------------------------------------------------

FrObject *FrBoundedPriQueue::pop()
{
   if (q_tail > q_head)
      return entries[q_head++] ;
   else
      return 0 ;
}

//----------------------------------------------------------------------

FrObject *FrBoundedPriQueue::pop(double &priority)
{
   if (q_tail > q_head)
      {
      priority = priorities[q_head] ;
      return entries[q_head++] ;
      }
   else
      {
      priority = null_priority ;
      return 0 ;
      }
}

//----------------------------------------------------------------------

bool FrBoundedPriQueue::removeLoc(size_t pos)
{
   if (pos == FrBPQ_NOT_FOUND)
      return false ;
   pos += q_head ;
   if (copy_objects)
      free_object(entries[pos]) ;
   if (pos == q_head)
      q_head++ ;
   else
      {
      q_tail-- ;
      for (size_t i = pos ; i < q_tail ; i++)
	 {
	 priorities[i] = priorities[i+1] ;
	 entries[i] = entries[i+1] ;
	 }
      }
   return true ;
}

//----------------------------------------------------------------------

bool FrBoundedPriQueue::remove(const FrObject *obj)
{
   return removeLoc(location(obj)) ;
}

//----------------------------------------------------------------------

bool FrBoundedPriQueue::remove(double priority)
{
   return removeLoc(location(priority)) ;
}


// end of frbpriq.cpp //

