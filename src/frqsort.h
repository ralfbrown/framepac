/************************************************************************/
/*									*/
/*  FramepaC								*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frqsort.h		templatized Quick-Sort and Merge-Sort	*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 2000,2001,2002,2004,2007,2008,2012			*/
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

#ifndef __FRQSORT_H_INCLUDED
#define __FRQSORT_H_INCLUDED

#include <stdlib.h>

//    requires:  static int T::compare(const T&, const T&) ;
//		 static void T::swap(T&, T&) ;
template <class T> void FrQuickSort(T elts[], size_t num_elts)
{
   while (num_elts > 2)
      {
      // find a partitioning element
      T *mid = &elts[num_elts/2] ;
      T *right = &elts[num_elts-1] ;
      if (T::compare(elts[0],*mid) > 0)
	 T::swap(elts[0],*mid) ;
      if (T::compare(elts[0],*right) > 0)
	 T::swap(elts[0],*right) ;
      if (T::compare(*mid,*right) > 0)
	 T::swap(*mid,*right) ;
      if (num_elts <= 3)
	 return ;			// we already sorted the entire range
      // partition the array
      T::swap(*mid,*right) ;		// put partition elt at end of array
      T *partval = right ;
      T *left = elts ;
      right-- ;
      do {
	 while (left <= right && T::compare(*left,*partval) <= 0)
	    left++ ;
	 while (left <= right && T::compare(*partval,*right) <= 0)
	    right-- ;
	 T::swap(*left,*right) ;
	 } while (left < right) ;
      size_t part = left - elts ;
      T::swap(*left,*right) ;
      // move partitioning element to its final location
      T::swap(*left,elts[num_elts-1]) ;
      // sort each half of the array
      if (part >= num_elts-part)
	 {
	 // left half is larger, so recurse on smaller right half
	 size_t half = num_elts-part-1 ;
	 if (half > 1)			// no need to recurse if only one elt
	    FrQuickSort(elts+part+1,half) ;
	 num_elts = part ;
	 }
      else
	 {
	 // right half is larger, so recurse on smaller left half
	 if (part > 1)			// no need to recurse if only one elt
	    FrQuickSort(elts,part) ;
	 elts += part+1 ;
	 num_elts -= part+1 ;
	 }
      }
   // when we get to this point, we have at most two items left;
   // a single item is already sorted by definition, and two items need at
   //   most a simple swap
   if (num_elts == 2 && T::compare(elts[0],elts[1]) > 0)
      T::swap(elts[0],elts[1]) ;
   return ;
}

//----------------------------------------------------------------------

//    requires:  static void T::swap(T&, T&) ;
template <class T> void FrQuickSort(T elts[], size_t num_elts,
				    int compare(const T*,const T*))
{
   while (num_elts > 2)
      {
      // find a partitioning element
      T *mid = &elts[num_elts/2] ;
      T *right = &elts[num_elts-1] ;
      if (compare(&elts[0],mid) > 0)
	 T::swap(elts[0],*mid) ;
      if (compare(&elts[0],right) > 0)
	 T::swap(elts[0],*right) ;
      if (compare(mid,right) > 0)
	 T::swap(*mid,*right) ;
      if (num_elts <= 3)
	 return ;			// we already sorted the entire range
      // partition the array
      T::swap(*mid,*right) ;		// put partition elt at end of array
      T *partval = right ;
      T *left = elts ;
      right-- ;
      do {
	 while (left <= right && compare(left,partval) <= 0)
	    left++ ;
	 while (left <= right && compare(partval,right) <= 0)
	    right-- ;
	 T::swap(*left,*right) ;
	 } while (left < right) ;
      size_t part = left - elts ;
      T::swap(*left,*right) ;
      // move partitioning element to its final location
      T::swap(*left,elts[num_elts-1]) ;
      // sort each half of the array
      if (part >= num_elts-part)
	 {
	 // left half is larger, so recurse on smaller right half
	 size_t half = num_elts-part-1 ;
	 if (half > 1)			// no need to recurse if only one elt
	    FrQuickSort(elts+part+1,half,compare) ;
	 num_elts = part ;
	 }
      else
	 {
	 // right half is larger, so recurse on smaller left half
	 if (part > 1)			// no need to recurse if only one elt
	    FrQuickSort(elts,part,compare) ;
	 elts += part+1 ;
	 num_elts -= part+1 ;
	 }
      }
   // when we get to this point, we have at most two items left;
   // a single item is already sorted by definition, and two items need at
   //   most a simple swap
   if (num_elts == 2 && compare(&elts[0],&elts[1]) > 0)
      T::swap(elts[0],elts[1]) ;
   return ;
}

//----------------------------------------------------------------------

template <class T> void FrQuickSortPtr(T *elts[], size_t num_elts,
				       int compare(const T*,const T*))
{
#define swap(x,y) { T* tmp = *(x) ; *(x) = *(y) ; *(y) = tmp ; }
   while (num_elts > 2)
      {
      // find a partitioning element
      T **mid = &elts[num_elts/2] ;
      T **right = &elts[num_elts-1] ;
      if (compare(elts[0],*mid) > 0)
	 swap(&elts[0],mid) ;
      if (compare(elts[0],*right) > 0)
	 swap(&elts[0],right) ;
      if (compare(*mid,*right) > 0)
	 swap(mid,right) ;
      if (num_elts <= 3)
	 return ;			// we already sorted the entire range
      // partition the array
      swap(mid,right) ;			// put partition elt at end of array
      T *partval = *right ;
      T **left = elts ;
      right-- ;
      do {
	 while (left <= right && compare(*left,partval) <= 0)
	    left++ ;
	 while (left <= right && compare(partval,*right) <= 0)
	    right-- ;
	 swap(left,right) ;
	 } while (left < right) ;
      size_t part = left - elts ;
      swap(left,right) ;
      // move partitioning element to its final location
      swap(left,&elts[num_elts-1]) ;
      // sort each half of the array
      if (part >= num_elts-part)
	 {
	 // left half is larger, so recurse on smaller right half
	 size_t half = num_elts-part-1 ;
	 if (half > 1)			// no need to recurse if only one elt
	    FrQuickSortPtr(elts+part+1,half,compare) ;
	 num_elts = part ;
	 }
      else
	 {
	 // right half is larger, so recurse on smaller left half
	 if (part > 1)			// no need to recurse if only one elt
	    FrQuickSortPtr(elts,part,compare) ;
	 elts += part+1 ;
	 num_elts -= part+1 ;
	 }
      }
   // when we get to this point, we have at most two items left;
   // a single item is already sorted by definition, and two items need at
   //   most a simple swap
   if (num_elts == 2 && compare(elts[0],elts[1]) > 0)
      swap(&elts[0],&elts[1]) ;
#undef swap
   return ;
}

//----------------------------------------------------------------------

//    requires:  static int T::compare(const T&, const T&) ;
//               T* T::next() const
//		 void T::setNext(T*)
template <class T> T *FrMergeSortedLists(T *list1, T *list2)
{
   if (!list2)
      return list1 ;
//   assert(list1 != 0) ;
   T *result ;
   if (T::compare(*list1,*list2) <= 0)
      {
      result = list1 ;
      list1 = list1->next() ;
      }
   else
      {
      result = list2 ;
      list2 = list2->next() ;
      }
   T *prev = result ;
   while (list1 && list2)
      {
      if (T::compare(*list1,*list2) <= 0)
	 {
	 prev->setNext(list1) ;		// glue item onto end of results list
	 prev = list1 ;
	 list1 = list1->next() ;	// advance down list1
	 }
      else
	 {
	 prev->setNext(list2) ;		// glue item onto end of results list
	 prev = list2 ;
	 list2 = list2->next() ;	// advance down list2
	 }
      }
   if (list1)
      prev->setNext(list1) ;
   else
      prev->setNext(list2) ;
   return result ;
}

//----------------------------------------------------------------------

//    requires:	 T* T::next() const
//		 void T::setNext(T*)
template <class T> T *FrMergeSortedLists(T *list1, T *list2,
					 int compare(const T&,const T&))
{
   if (!list2)
      return list1 ;
//   assert(list1 != 0) ;
   T *result ;
   if (compare(*list1,*list2) <= 0)
      {
      result = list1 ;
      list1 = list1->next() ;
      }
   else
      {
      result = list2 ;
      list2 = list2->next() ;
      }
   T *prev = result ;
   while (list1 && list2)
      {
      if (compare(*list1,*list2) <= 0)
	 {
	 prev->setNext(list1) ;		// glue item onto end of results list
	 prev = list1 ;
	 list1 = list1->next() ;	// advance down list1
	 }
      else
	 {
	 prev->setNext(list2) ;		// glue item onto end of results list
	 prev = list2 ;
	 list2 = list2->next() ;	// advance down list2
	 }
      }
   if (list1)
      prev->setNext(list1) ;
   else
      prev->setNext(list2) ;
   return result ;
}

//----------------------------------------------------------------------

//    requires:	 T* T::next() const
//		 void T::setNext(T*)
template <class T> T *FrMergeSortedListsPtr(T *list1, T *list2,
					    int compare(const T*,const T*))
{
   if (!list2)
      return list1 ;
//   assert(list1 != 0) ;
   T *result ;
   if (compare(list1,list2) <= 0)
      {
      result = list1 ;
      list1 = list1->next() ;
      }
   else
      {
      result = list2 ;
      list2 = list2->next() ;
      }
   T *prev = result ;
   while (list1 && list2)
      {
      if (compare(list1,list2) <= 0)
	 {
	 prev->setNext(list1) ;		// glue item onto end of results list
	 prev = list1 ;
	 list1 = list1->next() ;	// advance down list1
	 }
      else
	 {
	 prev->setNext(list2) ;		// glue item onto end of results list
	 prev = list2 ;
	 list2 = list2->next() ;	// advance down list2
	 }
      }
   if (list1)
      prev->setNext(list1) ;
   else
      prev->setNext(list2) ;
   return result ;
}

//----------------------------------------------------------------------

//    requires:  static int T::compare(const T&, const T&) ;
//               T* T::next() const
//		 void T::setNext(T*)
template <class T> T *FrMergeSort(T *list)
{
   if (list == 0)			// empty list?
      return list ;			// if yes, it's already sorted
   T *sublists[CHAR_BIT*sizeof(size_t)] ;
   sublists[0] = 0 ;
   size_t maxbits = 0 ;
   // scan down the given list, creating sorted sublists in powers of two
   while (list)
      {
      // chop the head node off the list
      T *sublist = list ;
      list = list->next() ;
      sublist->setNext(0) ;

      // merge the head node with sucessively longer sublists until we
      //   reach a power of two for which there currently is no sublist
      size_t i ;
      for (i = 0 ; i <= maxbits && sublists[i] ; i++)
	 {
	 sublist = FrMergeSortedLists(sublists[i],sublist) ;
	 sublists[i] = 0 ;
	 }
      sublists[i] = sublist ;
      if (i > maxbits)
	 maxbits++ ;
      }
   // at this point, we just need to merge together all the remaining
   //   sublists
   T *result = sublists[0] ;
   for (size_t i = 1 ; i <= maxbits ; i++)
      {
      if (sublists[i])
	 result = FrMergeSortedLists(sublists[i],result) ;
      }
   return result ;
}

//----------------------------------------------------------------------

//    requires:  T* T::next() const
//		 void T::setNext(T*)
template <class T> T *FrMergeSort(T *list, int compare(const T&,const T&))
{
   if (!list)				// empty list?
      return list ;			// if yes, it's already sorted
   T *sublists[CHAR_BIT*sizeof(size_t)] ;
   sublists[0] = 0 ;
   size_t maxbits = 0 ;
   // scan down the given list, creating sorted sublists in powers of two
   while (list)
      {
      // chop the head node off the list
      T *sublist = list ;
      list = list->next() ;
      sublist->setNext((T*)0) ;

      // merge the head node with sucessively longer sublists until we
      //   reach a power of two for which there currently is no sublist
      size_t i ;
      for (i = 0 ; i <= maxbits && sublists[i] ; i++)
	 {
	 sublist = FrMergeSortedLists(sublists[i],sublist,compare) ;
	 sublists[i] = 0 ;
	 }
      sublists[i] = sublist ;
      if (i > maxbits)
	 maxbits++ ;
      }
   // at this point, we just need to merge together all the remaining
   //   sublists
   T *result = sublists[0] ;
   for (size_t i = 1 ; i <= maxbits ; i++)
      {
      if (sublists[i])
	 result = FrMergeSortedLists(sublists[i],result,compare) ;
      }
   return result ;
}

//----------------------------------------------------------------------

//    requires:  T* T::next() const
//		 void T::setNext(T*)
template <class T> T *FrMergeSortPtr(T *list, int compare(const T*,const T*))
{
   if (!list)				// empty list?
      return list ;			// if yes, it's already sorted
   T *sublists[CHAR_BIT*sizeof(size_t)] ;
   sublists[0] = 0 ;
   size_t maxbits = 0 ;
   // scan down the given list, creating sorted sublists in powers of two
   while (list)
      {
      // chop the head node off the list
      T *sublist = list ;
      list = list->next() ;
      sublist->setNext(0) ;

      // merge the head node with sucessively longer sublists until we
      //   reach a power of two for which there currently is no sublist
      size_t i ;
      for (i = 0 ; i <= maxbits && sublists[i] ; i++)
	 {
	 sublist = FrMergeSortedListsPtr(sublists[i],sublist,compare) ;
	 sublists[i] = 0 ;
	 }
      sublists[i] = sublist ;
      if (i > maxbits)
	 maxbits++ ;
      }
   // at this point, we just need to merge together all the remaining
   //   sublists
   T *result = sublists[0] ;
   for (size_t i = 1 ; i <= maxbits ; i++)
      {
      if (sublists[i])
	 result = FrMergeSortedListsPtr(sublists[i],result,compare) ;
      }
   return result ;
}

//----------------------------------------------------------------------

//    requires:  T* T::next() const
//		 void T::setNext(T*)
//  DEPRECATED
template <class T> T *FrMergeSort2(T *list, int compare(const T*,const T*))
{
   return FrMergeSortPtr(list,compare) ;
}

#endif /* !__FRQSORT_H_INCLUDED */

// end of file frqsort.h //
