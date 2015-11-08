/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frlist2.cpp	class FrCons and class FrList			*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,1998,2001,2002,2009		*/
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

#include "frlist.h"

#ifdef __WATCOMC__
inline bool NONNULLthis(const void *x) { return (bool)(x!=0) ; }
#else
# define NONNULLthis(t) (t!=0)
#endif /* __WATCOMC__ */

/**********************************************************************/
/*    Member functions for class FrList			      */
/**********************************************************************/

bool FrList::contains(const FrList *sublist) const
{
   if (!sublist)
      return true ;		// trivially successful
   const FrObject *frst = sublist->first() ;
   const FrList *rst = sublist->rest() ;
   for (const FrList *l = this ; l ; l = l->rest())
      {
      if (l->first() == frst)
	 {
	 const FrList *l1 = l->rest() ;
	 const FrList *l2 = rst ;
	 for ( ; l1 && l2 ; l1 = l1->rest(), l2 = l2->rest())
	    if (l1->first() != l2->first())
	       break ;
	 if (l2 == 0)		// entire sublist matched?
	    return true ;
	 }
      }
   // if we get here, there was no match for the substring
   return false ;
}

//----------------------------------------------------------------------

bool FrList::contains(const FrList *sublist, FrCompareFunc cmp) const
{
   if (!sublist)
      return true ;		// trivially successful
   const FrObject *frst = sublist->first() ;
   const FrList *rst = sublist->rest() ;
   for (const FrList *l = this ; l ; l = l->rest())
      {
      if (cmp(l->first(),frst))
	 {
	 const FrList *l1 = l->rest() ;
	 const FrList *l2 = rst ;
	 for ( ; l1 && l2 ; l1 = l1->rest(), l2 = l2->rest())
	    if (!cmp(l1->first(),l2->first()))
	       break ;
	 if (l2 == 0)		// entire sublist matched?
	    return true ;
	 }
      }
   // if we get here, there was no match for the substring
   return false ;
}

//----------------------------------------------------------------------

int FrList::position(const FrObject *item, bool from_end) const
{
   int pos = 0 ;
   int pos_last = -1 ;

   for (const FrList *list = this ; list ; list = list->rest())
      {
      if (list->first() == item)
	 {
	 if (from_end)
	    pos_last = pos ;
	 else
	    return pos ;
	 }
      pos++ ;
      }
   return pos_last ;
}

//----------------------------------------------------------------------

int FrList::position(const FrObject *item,FrCompareFunc cmp,
		      bool from_end) const
{
   int pos = 0 ;
   int pos_last = -1 ;

   for (const FrList *list = this ; list ; list = list->rest())
      {
      if (cmp(item,list->first()))
	 {
	 if (from_end)
	    pos_last = pos ;
	 else
	    return pos ;
	 }
      pos++ ;
      }
   return pos_last ;
}

//----------------------------------------------------------------------

FrList *FrList::difference(const FrList *l2) const
{
   const FrList *l1 = this ;
   FrList *result = 0 ;
   FrList **end = &result ;

   if (!l2)
      return l1 ? (FrList*)l1->deepcopy() : 0 ;
   for ( ; l1; l1 = l1->rest())
      {
      FrObject *obj = l1->first() ;
      if (!l2->member(obj))
	 FrList::pushlistend(obj?obj->deepcopy():obj,end) ;
      }
   *end = 0 ;
   return result ;
}

//----------------------------------------------------------------------

FrList *FrList::difference(const FrList *l2, FrCompareFunc cmp) const
{
   const FrList *l1 = this ;
   FrList *result = 0 ;
   FrList **end = &result ;

   if (!l2)
      return l1 ? (FrList*)l1->deepcopy() : 0 ;
   for ( ; l1 ; l1 = l1->rest())
      {
      FrObject *obj = l1->first() ;
      if (!l2->member(obj,cmp))
	 FrList::pushlistend(obj?obj->deepcopy():obj,end) ;
      }
   *end = 0 ;
   return result ;
}

//----------------------------------------------------------------------

FrList *FrList::ndifference(const FrList *l2)
{
   FrList *l1 = (FrList*)this ;

   if (!l2)
      return l1 ;
   FrList *result = 0 ;
   FrList **end = &result ;
   while (l1)
      {
      FrObject *obj = poplist(l1) ;
      if (!l2->member(obj))
	 result->pushlistend(obj,end) ;
      else if (obj)
	 obj->freeObject() ;
      }
   *end = 0 ;
   return result ;
}

//----------------------------------------------------------------------

FrList *FrList::ndifference(const FrList *l2, FrCompareFunc cmp)
{
   FrList *l1 = (FrList*)this ;

   if (!l2)
      return l1 ;
   FrList *result = 0 ;
   FrList **end = &result ;
   while (l1)
      {
      FrObject *obj = poplist(l1) ;
      if (!l2->member(obj,cmp))
	 result->pushlistend(obj,end) ;
      else if (obj)
	 obj->freeObject() ;
      }
   *end = 0 ;				// properly terminate the result list
   return result ;
}

//----------------------------------------------------------------------

FrList *FrList::intersection(const FrList *l2) const
{
   const FrList *l1 = this ;
   FrList *result = 0 ;
   FrList **end = &result ;

   while (l1)
      {
      FrObject *obj = l1->first() ;
      if (l2->member(obj))
	 result->pushlistend(obj?obj->deepcopy():obj,end) ;
      l1 = l1->rest() ;
      }
   *end = 0 ;				// properly terminate the result list
   return result ;
}

//----------------------------------------------------------------------

FrList *FrList::intersection(const FrList *l2, FrCompareFunc cmp) const
{
   const FrList *l1 = this ;
   FrList *result = 0 ;
   FrList **end = &result ;

   while (l1)
      {
      FrObject *obj = l1->first() ;
      if (l2->member(obj,cmp))
	 result->pushlistend(obj?obj->deepcopy():obj,end) ;
      l1 = l1->rest() ;
      }
   *end = 0 ;				// properly terminate the result list
   return result ;
}

//----------------------------------------------------------------------

FrList *FrList::listunion(const FrList *l2) const
{
   FrList *result = 0 ;
   FrList **end = &result ;

   while (l2)
      {
      FrObject *obj = l2->first() ;
      if (!member(obj))
	 result->pushlistend(obj?obj->deepcopy():obj,end) ;
      l2 = l2->rest() ;
      }
   *end = 0 ;				// properly terminate the result list
   return NONNULLthis(this) ? result->nconc((FrList*)deepcopy()) : result ;
}

//----------------------------------------------------------------------

FrList *FrList::listunion(const FrList *l2, FrCompareFunc cmp) const
{
   FrList *result = 0 ;
   FrList **end = &result ;

   while (l2)
      {
      FrObject *obj = l2->first() ;
      if (!member(obj,cmp))
	 result->pushlistend(obj?obj->deepcopy():obj,end) ;
      l2 = l2->rest() ;
      }
   *end = 0 ;				// properly terminate the result list
   return NONNULLthis(this) ? result->nconc((FrList*)deepcopy()) : result ;
}

//----------------------------------------------------------------------

FrList *FrList::nlistunion(const FrList *l2)
{
   FrList *result = 0 ;
   FrList **end = &result ;
   while (l2)
      {
      FrObject *obj = l2->first() ;
      if (!member(obj))
	 result->pushlistend(obj?obj->deepcopy():obj,end) ;
      l2 = l2->rest() ;
      }
   *end = 0 ;				// properly terminate the result list
   return result->nconc(this) ;
}

//----------------------------------------------------------------------

FrList *FrList::nlistunion(const FrList *l2, FrCompareFunc cmp)
{
   FrList *result = 0 ;
   FrList **end = &result ;
   for ( ; l2 ; l2 = l2->rest())
      {
      FrObject *obj = l2->first() ;
      if (!member(obj,cmp))
	 result->pushlistend(obj?obj->deepcopy():obj,end) ;
      }
   *end = 0 ;				// properly terminate the result list
   return result->nconc(this) ;
}

//----------------------------------------------------------------------

bool FrList::subsetOf(const FrList *l2) const
{
   // not a subset if any element of current list is not in other list
   for (const FrList *l1 = this ; l1 ; l1 = l1->rest())
      if (!l2->member(l1->first()))
	 return false ;
   return true ;
}

//----------------------------------------------------------------------

bool FrList::subsetOf(const FrList *l2, FrCompareFunc cmp) const
{
   // not a subset if any element of current list is not in other list
   for (const FrList *l1 = this ; l1 ; l1 = l1->rest())
      if (!l2->member(l1->first(),cmp))
	 return false ;
   return true ;
}

//----------------------------------------------------------------------

bool FrList::intersect(const FrList *l2) const
{
   // the two lists intersect if any element of current list is in other list
   for (const FrList *l1 = this ; l1 ; l1 = l1->rest())
      if (l2->member(l1->first()))
	 return true ;
   return false ;

}

//----------------------------------------------------------------------

bool FrList::intersect(const FrList *l2, FrCompareFunc cmp) const
{
   // the two lists intersect if any element of current list is in other list
   for (const FrList *l1 = this ; l1 ; l1 = l1->rest())
      if (l2->member(l1->first(),cmp))
	 return true ;
   return false ;
}

//----------------------------------------------------------------------

FrList * __FrCDECL FrList::mapcar(FrListMapFunc map, ...) const
{
   FrList *result = 0 ;
   FrList **end = &result ;

   for (const FrList *l = this ; l ; l = l->rest())
      {
      // note: under Watcom C++, use of the va_list inside the called
      // function affects its value here!  So, we have to reset it each
      // time through the loop
      va_list args ;
      va_start(args,map) ;
      FrObject *elt = map(l->first(),args) ;
      va_end(args) ;
      if (elt == l->first())
	 elt = elt->deepcopy() ;
      result->pushlistend(elt,end) ;
      }
   *end = 0 ;				// properly terminate the result
   return result ;
}

//----------------------------------------------------------------------

FrList * __FrCDECL FrList::mapcan(FrListMapFunc map, ...) const
{
   FrList *result = 0, *tail = 0 ;

   for (const FrList *l = this ; l ; l = l->rest())
      {
      // note: under Watcom C++, use of the va_list inside the called
      // function affects its value here!  So, we have to reset it each
      // time through the loop
      va_list args ;
      va_start(args,map) ;
      FrObject *elt = map(l->first(),args) ;
      va_end(args) ;
      if (elt)
	 {
	 if (!elt->consp())
	    elt = new FrList(elt) ;
	 if (tail)
	    tail->replacd(elt) ;
	 else
	    tail = result = (FrList*)elt ;
	 tail = tail->last() ;
	 }
      }
   return result ;
}

// end of file frlist2.cpp //
