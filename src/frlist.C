/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frlist.cpp	class FrCons and class FrList			*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,1998,1999,2000,2001,2002,2003,	*/
/*		2004,2005,2007,2008,2009,2010,2013,2015			*/
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
#  pragma implementation "frlist.h"
#endif

#include "frlist.h"
#include "frqsort.h"
#include "frutil.h"
#include "frpcglbl.h"

#ifdef FrSTRICT_CPLUSPLUS
#  include <iomanip>
#  include <string>
#else
#  include <iomanip.h>
#  include <string.h>
#endif /* FrSTRICT_CPLUSPLUS */

/************************************************************************/
/*    Global data local to this module					*/
/************************************************************************/

static const char str_FrCons[] = "FrCons" ;

/************************************************************************/
/*    Global variables for classes FrCons and FrList			*/
/************************************************************************/

FrAllocator FrCons::allocator(str_FrCons,sizeof(FrCons)) ;

static FrObject *dummy_pointer = 0 ;

//----------------------------------------------------------------------

/************************************************************************/
/*    Helper functions							*/
/************************************************************************/

#define update_listhash FramepaC_update_ulong_hash

/************************************************************************/
/*    Member functions for class FrCons				*/
/************************************************************************/

FrObjectType FrCons::objType() const
{
   return OT_Cons ;
}

//----------------------------------------------------------------------

const char *FrCons::objTypeName() const
{
   return str_FrCons ;
}

//----------------------------------------------------------------------

FrObjectType FrCons::objSuperclass() const
{
   return OT_FrObject ;
}

//----------------------------------------------------------------------

void FrCons::freeObject()
{
   delete this ;
}

//----------------------------------------------------------------------

void FrCons::reserve(size_t N)
{
   size_t objs_per_block = FrFOOTER_OFS / allocator.objectSize() - 20 ;
   for (size_t i = allocator.objects_allocated() ;
	i < N ;
	i += objs_per_block)
      {
      allocator.preAllocate() ;
      }
   return ;
}

//----------------------------------------------------------------------

bool FrCons::consp() const
{
   return true ;
}

//----------------------------------------------------------------------

FrObject *FrCons::car() const
{
   return m_ptr1 ;
}

//----------------------------------------------------------------------

FrObject *FrCons::cdr() const
{
   return m_ptr2 ;
}

//----------------------------------------------------------------------

FrObject *FrCons::reverse()
{
   // swap the car() and cdr() of the cons cell
   FrObject *p = m_ptr2 ;
   m_ptr1 = m_ptr2 ;
   m_ptr2 = p ;
   return this ;
}

//----------------------------------------------------------------------

FrObject *FrCons::getNth(size_t N) const
{
   const FrObject *result ;
   if (N == 0)
      result = first() ;
   else if (N == 1)
      result = m_ptr2 ;
   else
      result = 0 ;
   return result ? result->deepcopy() : 0 ;
}

//----------------------------------------------------------------------

bool FrCons::setNth(size_t N, const FrObject *newelt)
{
   if (N == 0)
      {
      free_object(m_ptr1) ;
      m_ptr1 = newelt ? newelt->deepcopy() : 0 ;
      return true ;
      }
   else if (N == 1)
      {
      free_object(m_ptr2) ;
      m_ptr2 = newelt ? newelt->deepcopy() : 0 ;
      return true ;
      }
   else
      return false ;
}

//----------------------------------------------------------------------

bool FrCons::equal(const FrObject *obj) const
{
   if (this == obj)
      return true ;	   // equal if comparing to ourselves
   else if (!obj || !(obj->consp()))
      return false ;	   // not equal if other object is not a FrCons or is
			   //	empty ('this' can't be 0 because virtual fn)
   else
      {
      FrObject *o1 = first() ;
      FrObject *o2 = ((FrCons *)obj)->first() ;
      // are the cars of the two conses equal?
      if (o1 != o2 && (!o1 || !o2 || !o1->equal(o2)))
	 return false ;
      o1 = m_ptr2 ;
      o2 = ((FrCons *)obj)->m_ptr2 ;
      // are the cdrs of the two conses equal?
      if (o1 != o2 && (!o1 || !o2 || !o1->equal(o2)))
	 return false ;
      else
	 return true ;
      }
}

//----------------------------------------------------------------------

FrObject *FrCons::copy() const
{
   return new FrCons(first(),m_ptr2) ;
}

//----------------------------------------------------------------------

FrObject *FrCons::deepcopy() const
{
   return new FrCons(m_ptr1?m_ptr1->deepcopy():0,m_ptr2?m_ptr2->deepcopy():0);
}

//----------------------------------------------------------------------

unsigned long FrCons::hashValue() const
{
   // the hash value of a cons is a combination of the hash values of car
   // and cdr elements
   const FrObject *item = first() ;
   unsigned long hash = update_listhash(0,item ? item->hashValue() : 0) ; ;
   item = consCdr() ;
   hash = update_listhash(hash,item ? item->hashValue() : 0) ;
   return hash ;
}

//----------------------------------------------------------------------

int FrCons::compare(const FrObject *obj) const
{
   if (!obj)
      return 1 ;     // anything is greater than NIL / empty-list
   else if (!obj->consp())
      return -1 ;    // non-lists should sort after lists
   int cmp = m_ptr1 ? m_ptr1->compare(((FrCons*)obj)->first()) : -1 ;
   if (!cmp)
      cmp = m_ptr2 ? m_ptr2->compare(((FrCons*)obj)->m_ptr2) : -1 ;
   return cmp ;
}

//----------------------------------------------------------------------

bool FrCons::iterateVA(FrIteratorFunc func, va_list args) const
{
   FrSafeVAList(args) ;
   bool success = func(first(),FrSafeVarArgs(args)) ;
   FrSafeVAListEnd(args) ;
   if (success)
      return func(m_ptr2,args) ;
   else
      return false ;
}

//----------------------------------------------------------------------

static bool dump_unfreed(void *obj, va_list args)
{
   FrCons *cons = (FrCons*)obj ;
   FrVarArg(ostream *,out) ;
   if (cons && out)
      (*out) << cons << endl ;
   return true ;			// continue iterating
}

void FrCons::dumpUnfreed(ostream &out)
{
   allocator.iterate(dump_unfreed,&out) ;
   return ;
}

/**********************************************************************/
/*    Member functions for class FrList			      */
/**********************************************************************/

FrObjectType FrList::objType() const
{
   return OT_List ;
}

//----------------------------------------------------------------------

const char *FrList::objTypeName() const
{
   return "FrList" ;
}

//----------------------------------------------------------------------

FrObjectType FrList::objSuperclass() const
{
   return OT_Cons ;
}

//----------------------------------------------------------------------

FrList::FrList(const FrObject *one, const FrObject *two)
{
   replaca(one) ;
   replacd(new FrList(two)) ;
   return ;
}

//----------------------------------------------------------------------

FrList::FrList(const FrObject *one, const FrObject *two,
	   const FrObject *three)
{
   replaca(one) ;
   replacd(new FrList(two,three)) ;
   return ;
}

//----------------------------------------------------------------------

FrList::FrList(const FrObject *one, const FrObject *two,
	   const FrObject *three, const FrObject *four)
{
   replaca(one) ;
   replacd(new FrList(two,three,four)) ;
   return ;
}

//----------------------------------------------------------------------

FrList::~FrList()
{
   free_object(first()) ;
   if (m_ptr2)
      delete (FrList*)m_ptr2 ;
   return ;
}

//----------------------------------------------------------------------

void FrList::freeObject()
{
   if (this)
      {
      FrCons *head = (FrCons *)this ;

      for (;;)
	 {
	 FrList *next = ((FrList *)head)->rest() ;
	 if (head->first())
	    head->first()->freeObject() ;
#ifdef FrOBJECT_VIRTUAL_DTOR
	 head->replaca(0) ;
	 head->replacd(0) ;
#endif /* FrOBJECT_VIRTUAL_DTOR */
	 delete head ;
	 if (next)
	    {
	    if (next->consp())
	       head = next ;
	    else
	       {
	       next->freeObject() ;
	       break ;
	       }
	    }
	 else
	    break ;
	 }
      }
   return ;
}

//----------------------------------------------------------------------

FrList *FrList::member(const FrObject *item) const
{
   const FrList *list = this ;

   while (list)
      {
      if (list->first() == item)
	 break ;
      list = (FrList*)list->m_ptr2 ;
      }
   return (FrList *)list ;
}

//----------------------------------------------------------------------

FrList *FrList::member(const FrObject *item,FrCompareFunc cmp) const
{
   const FrList *list = this ;

   while (list)
      {
      if (cmp(item,list->first()))
	 break ;
      list = (FrList*)list->m_ptr2 ;
      }
   return (FrList *)list ;
}

//----------------------------------------------------------------------
// check whether two lists are equivalent except for the order of their
// elements

bool FrList::equiv(const FrList *l) const
{
   if (l == this)	   // lists are equivalent if they are the same list
      return true ;
   const FrList *lst = this ;
   if (!l || !lst || listlength() != l->listlength())
      return false ;	   // can't be equiv if only one is empty
			   //  or lists differ in length
   else
      {
      for ( ; lst ; lst = lst->rest())
	 {
	 if (!l->member(lst->first(),::equal))
	    return false ;
	 }
      // at this point, we know that every item in 'this' is also in the
      //  other list, but there may have been duplicates in 'this', so we
      //  also need to check in the other direction
      for ( ; l ; l = l->rest())
	 {
	 if (!member(l->first(),::equal))
	    return false ;
	 }
      return true ;
      }
}

//----------------------------------------------------------------------

FrObject *FrList::copy() const
{
   if (this)
      {
      FrList *oldlist, *newlist, *tmp ;

      tmp = newlist = new FrList ;
      oldlist = (FrList *)m_ptr2 ;
      tmp->replaca(first()) ;
      while (oldlist)
	 {
	 if (oldlist->consp())
	    {
	    tmp->replacd(new FrList) ;
	    FrObject *frst = oldlist->first() ;
	    tmp = (FrList*)tmp->m_ptr2 ;
	    oldlist = (FrList*)oldlist->m_ptr2 ;
	    tmp->replaca(frst) ;
	    }
	 else
	    {
	    tmp->replacd(oldlist) ;
	    return newlist ;
	    }
	 }
      tmp->replacd(0) ;
      return newlist ;
      }
   else
      return 0 ;
}


//----------------------------------------------------------------------

FrObject *FrList::deepcopy() const
{
   if (this)
      {
      FrList *oldlist, *newlist, *tmp ;

      tmp = newlist = new FrList ;
      oldlist = (FrList *)m_ptr2 ;
      tmp->replaca(first() ? first()->deepcopy() : 0) ;
      while (oldlist)
	 {
	 if (oldlist->consp())
	    {
	    tmp->replacd(new FrList) ;
	    FrObject *frst = oldlist->first() ;
	    tmp = (FrList*)tmp->m_ptr2 ;
	    oldlist = (FrList*)oldlist->m_ptr2 ;
	    tmp->replaca(frst ? frst->deepcopy() : 0) ;
	    }
	 else
	    {
	    tmp->replacd(oldlist->deepcopy()) ;
	    return newlist ;
	    }
	 }
      tmp->replacd(0) ;
      return newlist ;
      }
   else
      return 0 ;
}

//----------------------------------------------------------------------

unsigned long FrList::hashValue() const
{
   // the hash value of a list is a combination of the hash values of all
   // its elements
   unsigned long hash = 0 ;
   const FrList *l ;
   for (l = this ; l && l->consp() ; l = l->rest())
      {
      FrObject *head = l->first() ;
      hash = update_listhash(hash,head ? head->hashValue() : 0) ;
      }
   if (l)
      {
      // add in the final dotted element
      hash = update_listhash(hash,l->hashValue()) ;
      }
   return hash ;
}

//----------------------------------------------------------------------

void FrList::eraseList(bool deep)
{
   if (this)
      {
      FrCons *head = (FrCons *)this ;
      for (;;)
	 {
	 FrList *next = ((FrList *)head)->rest() ;
	 if (deep && head->first())
	    head->first()->freeObject() ;
#ifdef FrOBJECT_VIRTUAL_DTOR
	 head->replaca(0) ;
	 head->replacd(0) ;
#endif /* FrOBJECT_VIRTUAL_DTOR */
	 delete head ;
	 if (next)
	    {
	    if (next->consp())
	       head = next ;
	    else
	       {
	       if (deep) next->freeObject() ;
	       break ;
	       }
	    }
	 else
	    break ;
	 }
      }
   return ;
}

//----------------------------------------------------------------------

FrObject *FrList::reverse()
{
   return listreverse(this) ;
}

//----------------------------------------------------------------------

FrObject *FrList::getNth(size_t N) const
{
   const FrObject *result = nth(N) ;
   return result ? result->deepcopy() : 0 ;
}

//----------------------------------------------------------------------

bool FrList::setNth(size_t N, const FrObject *newelt)
{
   FrList *head = nthcdr(N) ;
   if (head)
      {
      free_object(head->first()) ;
      head->replaca(newelt ? newelt->deepcopy() : 0) ;
      return true ;
      }
   else
      return false ;
}

//----------------------------------------------------------------------

int FrList::position(const FrObject *item) const
{
   const FrList *list = this ;
   int pos = 0 ;

   for ( ; list ; list = list->rest())
      {
      if (list->first() == item)
	 return pos ;
      pos++ ;
      }
   return -1 ;				// not found!
}

//----------------------------------------------------------------------

int FrList::position(const FrObject *item,FrCompareFunc cmp) const
{
   const FrList *list = this ;
   int pos = 0 ;

   for ( ; list ; list = list->rest())
      {
      if (cmp(item,list->first()))
	 return pos ;
      pos++ ;
      }
   return -1 ;				// not found!
}

//----------------------------------------------------------------------

size_t FrList::locate(const FrObject *item, size_t startpos) const
{
   const FrList *l = this ;
   size_t pos ;
   if (startpos != (size_t)-1)
      l = l->nthcdr(startpos+1) ;
   if (item && item->consp())
      {
      // check for sublist of this
      FrList *sublist = (FrList*)item ;
      size_t sublen = (size_t)sublist->listlength() ;
      size_t remain = (size_t)l->listlength() ;
      if (sublen > remain)
	 return (size_t)-1 ;		// can't possibly find a match
      remain -= sublen ;
      pos = 0 ;
      bool match = false ;
      for ( ; l && pos <= remain ; l = l->rest(), pos++)
	 {
	 match = true ;
	 sublist = (FrList*)item ;
	 for (const FrList *cand = l ;
	      sublist ;
	      cand = cand->rest(), sublist = sublist->rest())
	    {
	    if (cand->first() != sublist->first())
	       {
	       match = false ;
	       break ;
	       }
	    }
	 if (match)
	    break ;
	 }
      if (!match)
	 pos = (size_t)-1 ;
      }
   else
      {
      // scan for a matching element in the list
      pos = (size_t)l->position(item) ;
      }
   if (pos == (size_t)-1)
      return (size_t)-1 ;
   else if (startpos == (size_t)-1)
      return pos ;
   else
      return pos + startpos + 1 ;
}

//----------------------------------------------------------------------

size_t FrList::locate(const FrObject *item, FrCompareFunc cmp,
		       size_t startpos) const
{
   const FrList *l = this ;
   size_t pos ;
   if (startpos != (size_t)-1)
      l = l->nthcdr(startpos+1) ;
   if (item && item->consp())
      {
      FrList *sublist = (FrList*)item ;
      size_t sublen = (size_t)sublist->listlength() ;
      size_t remain = (size_t)l->listlength() ;
      if (sublen > remain)
	 return (size_t)-1 ;		// can't possibly find a match
      remain -= sublen ;
      pos = 0 ;
      bool match = false ;
      for ( ; l && pos <= remain ; l = l->rest(), pos++)
	 {
	 match = true ;
	 sublist = (FrList*)item ;
	 for (const FrList *cand = l ;
	      sublist ;
	      cand = cand->rest(), sublist = sublist->rest())
	    {
	    if (!cmp(cand->first(),sublist->first()))
	       {
	       match = false ;
	       break ;
	       }
	    }
	 if (match)
	    break ;
	 }
      if (!match)
	 pos = (size_t)-1 ;
      }
   else
      {
      // scan for a matching element in the list
      pos = (size_t)l->position(item,cmp) ;
      }
   if (pos == (size_t)-1)
      return (size_t)-1 ;
   else if (startpos == (size_t)-1)
      return pos ;
   else
      return pos + startpos + 1 ;
}

//----------------------------------------------------------------------

FrObject *FrList::insert(const FrObject *newelts, size_t pos, bool cp)
{
   if (pos == 0)
      {
      // add to beginning of list
      if (cp && newelts)
	 newelts = newelts->deepcopy() ;
      if (!newelts || !newelts->consp())
	 newelts = new FrList(newelts) ;
      return ((FrList*)newelts)->nconc(this) ;
      }
   FrList *l = this ;
   FrList *prev = 0 ;
   while (pos-- > 0)
      {
      prev = l ;
      if (!l)
	 break ;
      l = l->rest() ;
      }
   if (prev)				//  are we still within the list?
      {
      if (cp && newelts)
	 newelts = newelts->deepcopy() ;
      if (!newelts || !newelts->consp())
	 newelts = new FrList(newelts) ;
      prev->replacd(((FrList*)newelts)->nconc(l)) ;
      }
   return this ;
}

//----------------------------------------------------------------------

FrObject *FrList::elide(size_t start, size_t end)
{
   if (end < start)
      end = start ;
   FrList *l = this ;
   FrList *prev = 0 ;
   size_t pos ;
   for (pos = 0 ; l && pos < start ; pos++)
      {
      prev = l ;
      l = l->rest() ;
      }
   if (l)
      {
      // if the start position is not past the end of the list, start removing
      // elements until we reach the end position or the end of the list
      for ( ; l && pos <= end ; pos++)
	 {
	 FrList *tmp = l ;
	 l = l->rest() ;
//	 delete tmp->first() ;
	 free_object(tmp->first()) ;
	 tmp->replaca(0) ;
	 tmp->replacd(0) ;
	 delete tmp ;
	 }
      if (prev)
	 prev->replacd(l) ;
      else
	 return l ;
      }
   return this ;
}

//----------------------------------------------------------------------

FrObject *FrList::removeDuplicates() const
{
   FrList *result = 0 ;
   FrList **end = &result ;
   for (const FrList *l = this ; l ; l = l->rest())
      {
      FrObject *f = l->first() ;
      if (!result->member(f))
	 {
         result->pushlistend(f?f->deepcopy():0,end) ;
	 *end = 0 ;
	 }
      }
   return result ;
}

//----------------------------------------------------------------------

FrObject *FrList::removeDuplicates(FrCompareFunc cmp) const
{
   FrList *result = 0 ;
   FrList **end = &result ;
   for (const FrList *l = this ; l ; l = l->rest())
      {
      FrObject *f = l->first() ;
      if (!result->member(f,cmp))
	 {
         result->pushlistend(f?f->deepcopy():0,end) ;
	 *end = 0 ;
	 }
      }
   return result ;
}

//----------------------------------------------------------------------

FrList *FrList::last() const
{
   FrList *l = (FrList *)this ;
   if (l)
      {
      FrList *next ;
      while ((next = l->rest()) != 0)
	 {
	 l = next ;
	 if (!next->consp())
	    break ;
	 }
      }
   return l ;
}

//----------------------------------------------------------------------

FrList *FrList::nconc(FrList *appendage)
{
   FrList *list = (FrList *)this ;
   if (list)
      {
      FrList *next ;
      while ((next = list->rest()) != 0)
	 list = next ;
      list->replacd(appendage) ;
      return this ;
      }
   else
      return appendage ;
}

//----------------------------------------------------------------------

void FrList::nconc(FrList *appendage, FrList **&end)
{
   *end = appendage ;
   if (appendage)
      {
      while (appendage->m_ptr2)
	 appendage = (FrList*)appendage->m_ptr2 ;
      FrObject **endptr = &appendage->m_ptr2 ;
      end = (FrList**)endptr ;
      }
   return ;
}

//----------------------------------------------------------------------

size_t FrList::length() const
{
   size_t len = 0 ;
   const FrList *list = this ;

   while (list && list->consp())
      {
      len++ ;
      list = list->rest() ;
      }
   return len ;
}

//----------------------------------------------------------------------

size_t FrList::listlength() const
{
   size_t len = 0 ;
   const FrList *list = this ;

   while (list && list->consp())
      {
      len++ ;
      list = list->rest() ;
      }
   return len ;
}

//----------------------------------------------------------------------

size_t FrList::simplelistlength() const
{
   size_t len = 0 ;
   const FrList *list = this ;

   while (list)
      {
      len++ ;
      list = list->rest() ;
      }
   return len ;
}

//----------------------------------------------------------------------

int FrList::compare(const FrObject *obj) const
{
   if (!obj)
      return 1 ;     // anything is greater than NIL / empty-list
   else if (!obj->consp())
      return -1 ;    // non-lists should sort after lists
   const FrList *l1 = this ;
   while (l1 && obj)
      {
      int cmp = l1->first() ? l1->first()->compare(((FrList*)obj)->first())
			 : (((FrList*)obj)->first() ? -1 : 0) ;
      if (cmp)
	 return cmp ;
      l1 = l1->rest() ;
      obj = ((FrList*)obj)->rest() ;
      if (!l1)
	 return obj ? -1 : 0 ;
      else if (!l1->consp())
	 return obj ? -obj->compare(l1) : -1 ;
      else if (!obj || !obj->consp())
	 return +1 ;			// this list longer than other list
      }
   if (l1)
      return +1 ;			// other list ended first
   else if (obj)
      return -1 ;			// this list ended first
   return 0 ;				// lists compared equal
}

//----------------------------------------------------------------------

bool FrList::iterateVA(FrIteratorFunc func, va_list args) const
{
   for (const FrList *l = this ; l ; l = l->rest())
      {
      FrSafeVAList(args) ;
      bool success = func(l->first(),FrSafeVarArgs(args)) ;
      FrSafeVAListEnd(args) ;
      if (!success)
	 return false ;
      }
   return true ;
}

//----------------------------------------------------------------------

FrCons *FrList::assoc(const FrObject *item) const
{
   for (const FrList *alist = this ; alist ; alist = alist->rest())
      {
      FrObject *head = alist->first() ;
      if (head && head->consp() && ((FrCons *)head)->first() == item)
	 return (FrCons *)head ;
      }
   return 0 ;  // not found
}

//----------------------------------------------------------------------

FrCons *FrList::assoc(const FrObject *item, FrCompareFunc cmp) const
{
   for (const FrList *alist = this ; alist ; alist = alist->rest())
      {
      FrObject *head = alist->first() ;

      if (head && head->consp() && cmp(((FrCons *)head)->first(),item))
	 return (FrCons *)head ;
      }
   return 0 ;  // not found
}

//----------------------------------------------------------------------

FrObject *FrList::subseq(size_t start, size_t stop) const
{
   FrList *seq = nthcdr(start) ;	// skip to start of requested subseq.
   FrList *result = 0 ;
   FrList **end = &result ;
   for (size_t i = start ; seq && i <= stop ; i++)
      {
      FrObject *obj = seq->first() ;
      FrList::pushlistend(obj ? obj->deepcopy() : 0, end) ;
      seq = seq->rest() ;
      }
   // terminate the list
   *end = 0 ;
   return result ;
}

//----------------------------------------------------------------------

FrList *FrList::shallowsubseq(size_t start, size_t stop) const
{
   FrList *seq = nthcdr(start) ;	// skip to start of requested subseq.
   FrList *result ;
   FrList **end = &result ;
   for (int i = stop-start ; seq && i >= 0 ; i--)
      {
      FrList::pushlistend(seq->first(),end) ;
      seq = seq->rest() ;
      }
   *end = 0 ;
   return result ;
}

//----------------------------------------------------------------------

FrList * __FrCDECL makelist(const FrObject *obj1, ...)
{
   FrList *list, *tail ;
   va_list objs ;
   FrObject *next ;

   list = new FrList(obj1) ;
   tail = list ;
   va_start(objs,obj1) ;
   while ((next = va_arg(objs,FrObject *)) != 0)
      {
      tail->replacd(new FrList(next)) ;
      tail = tail->rest() ;
      }
   va_end(objs) ;
   return list ;
}

//----------------------------------------------------------------------

FrList *pushlist(const FrObject *newitem, FrList *&list)
{
   FrList *newlist = new FrList ;
   newlist->replacd(list) ;
   newlist->replaca(newitem) ;
   list = newlist ;
   return newlist ;
}

//----------------------------------------------------------------------

void FrList::pushlistend(const FrObject *newitem, FrList **&end)
{
   // prior to first call, 'end' must be initialized to point at the pointer
   //   for the list to be constructed; after last call, use "*end=0;" to
   //   terminate the resulting list
   FrList *newlist = new FrList ;
   newlist->replaca(newitem) ;
   *end = newlist ;
   FrObject **endptr = &newlist->m_ptr2 ;
   end = (FrList**)endptr ;
   return ;
}

//----------------------------------------------------------------------

FrList *pushlistnew(const FrObject *newitem, FrList *&list)
{
   for (const FrList *l = list ; l ; l = l->rest())
      {
      if (l->first() == newitem)
	 return list ;
      }
   FrList *newlist = new FrList ;
   newlist->replacd(list) ;
   newlist->replaca(newitem) ;
   list = newlist ;
   return newlist ;
}

//----------------------------------------------------------------------

FrList *pushlistnew(const FrObject *newitem, FrList *&list,
		     FrCompareFunc cmp)
{
   for (const FrList *l = list ; l ; l = l->rest())
      {
      if (cmp(l->first(),newitem))
	 return list ;
      }
   FrList *newlist = new FrList ;
   newlist->replacd(list) ;
   newlist->replaca(newitem) ;
   list = newlist ;
   return newlist ;
}

//----------------------------------------------------------------------

FrObject *poplist(FrList *&list)
{
   if (list && list->consp())
      {
      FrObject *head = list->first() ;
      FrCons *c = (FrCons *)list ;
      list = list->rest() ;
#ifdef FrOBJECT_VIRTUAL_DTOR
      c->replaca(0) ;
      c->replacd(0) ;
#endif /* FrOBJECT_VIRTUAL_DTOR */
      delete c ;
      return head ;
      }
   else
      return 0 ;
}

//----------------------------------------------------------------------

FrObject *poplist(FrList *&list, bool destructive)
{
   FrObject *head ;
   FrCons *c ;

   if (list && list->consp())
      {
      head = list->first() ;
      if (destructive)
	 {
	 c = (FrCons *)list ;
	 list = list->rest() ;
#ifdef FrOBJECT_VIRTUAL_DTOR
	 c->replaca(0) ;
	 c->replacd(0) ;
#endif /* FrOBJECT_VIRTUAL_DTOR */
	 delete c ;
	 }
      else
	 list = list->rest() ;
      return head ;
      }
   else
      return 0 ;
}

//----------------------------------------------------------------------
// destructively reverse the given list, returning a pointer to the
// resulting reversed list

FrList *listreverse(FrList *l)
{
   FrList *prev = 0 ;
   FrList *curr ;

   curr = l ;
   while (curr)
      {
      l = l->rest() ;
      curr->replacd(prev) ;
      prev = curr ;
      curr = l ;
      }
   return prev ;
}

//----------------------------------------------------------------------

FrList *listremove(const FrList *l, const FrObject *item)
{
   FrList *prev = 0, *curr = (FrList *)l ;

   while (curr)
      {
      if (curr->first() == item)
	 {
	 if (prev)
	    prev->replacd(curr->rest()) ;
	 else
	    l = l->rest() ;
#ifdef FrOBJECT_VIRTUAL_DTOR
	 curr->replaca(0) ;
	 curr->replacd(0) ;
#endif /* FrOBJECT_VIRTUAL_DTOR */
	 delete (FrCons *)curr ;
	 return (FrList *)l ;
	 }
      prev = curr ;
      curr = curr->rest() ;
      }
   return (FrList *)l ;
}

//----------------------------------------------------------------------

FrList *listremove(const FrList *l, const FrObject *item,FrCompareFunc cmp)
{
   FrList *prev = 0, *curr = (FrList *)l ;

   while (curr)
      {
      if (cmp(curr->first(),item))
	 {
	 if (prev)
	    prev->replacd(curr->rest()) ;
	 else
	    l = l->rest() ;
	 free_object(curr->first()) ;
#ifdef FrOBJECT_VIRTUAL_DTOR
	 curr->replaca(0) ;
	 curr->replacd(0) ;
#endif /* FrOBJECT_VIRTUAL_DTOR */
	 delete (FrCons *)curr ;
	 return (FrList *)l ;
	 }
      prev = curr ;
      curr = curr->rest() ;
      }
   return (FrList *)l ;
}

//----------------------------------------------------------------------

static FrList *merge(FrList *list1, FrList *list2,
		      ListSortCmpFunc *cmpfunc)
{
   if (!list2)
      return list1 ;
//   assert(list1 != 0) ;
   FrList *result ;
   if (cmpfunc(list1->first(),list2->first()) <= 0)
      {
      result = list1 ;
      list1 = list1->rest() ;
      }
   else
      {
      result = list2 ;
      list2 = list2->rest() ;
      }
   FrList *prev = result ;
   while (list1 && list2)
      {
      if (cmpfunc(list1->first(),list2->first()) <= 0)
	 {
	 prev->replacd(list1) ;		// glue item onto end of results list
	 prev = list1 ;
	 list1 = list1->rest() ;	// advance down list1
	 }
      else
	 {
	 prev->replacd(list2) ;		// glue item onto end of results list
	 prev = list2 ;
	 list2 = list2->rest() ;	// advance down list2
	 }
      }
   if (list1)
      prev->replacd(list1) ;
   else
      prev->replacd(list2) ;
   return result ;
}

//----------------------------------------------------------------------

FrList *FrList::sort(ListSortCmpFunc *cmpfunc)
{
   FrList *list = (FrList*)this ;
   if (!list)				// empty list?
      return list ;			// if yes, it's already sorted
   FrList *sublists[CHAR_BIT*sizeof(size_t)] ;
   sublists[0] = 0 ;
   size_t maxbits = 0 ;
   // scan down the given list, creating sorted sublists in powers of two
   while (list && maxbits < lengthof(sublists))
      {
      // chop the head node off the list
      FrList *sublist = list ;
      list = list->rest() ;
      sublist->replacd(0) ;

      // merge the head node with sucessively longer sublists until we
      //   reach a power of two for which there currently is no sublist
      size_t i ;
      for (i = 0 ; i <= maxbits && sublists[i] ; i++)
	 {
	 if (sublist)
	    sublist = merge(sublists[i],sublist,cmpfunc) ;
	 else
	    sublist = sublists[i] ;
	 sublists[i] = 0 ;
	 }
      sublists[i] = sublist ;
      if (i > maxbits)
	 maxbits++ ;
      }
   // at this point, we just need to merge together all the remaining
   //   sublists
   FrList *result = sublists[0] ;
   for (size_t i = 1 ; i <= maxbits ; i++)
      {
      if (sublists[i])
	 result = merge(sublists[i],result,cmpfunc) ;
      }
   return result ;
}

//----------------------------------------------------------------------

FrList *FrList::nthcdr(size_t n) const
{
   const FrList *l = this ;

   while (n > 0 && l)
      {
      l = l->rest() ;
      n-- ;
      }
   return (FrList *)l ;
}

//----------------------------------------------------------------------

FrObject *FrList::nth(size_t n) const
{
   const FrList *l = this ;

   while (n > 0 && l)
      {
      l = l->rest() ;
      n-- ;
      }
   return l ? l->first() : 0 ;
}

//----------------------------------------------------------------------

FrObject *&FrList::operator[] (size_t n) const
{
   const FrList *l = this ;

   while (n > 0 && l)
      {
      l = l->rest() ;
      n-- ;
      }
   return l ? *((FrObject**)(&l->m_ptr1)) : dummy_pointer ;
}

//----------------------------------------------------------------------

FrObject *FrList::second() const
{
   const FrList *l = this ;
   return (l && l->rest()) ? l->rest()->first() : 0 ;
}

//----------------------------------------------------------------------

FrObject *FrList::third() const
{
   const FrList *l = this ;
   if (l && l->rest())
      {
      l = l->rest()->rest() ;
      return l ? l->first() : 0 ;
      }
   else
      return 0 ;
}

/**********************************************************************/
/*	 Input/Output functions for FrList			      */
/**********************************************************************/

static void print_list(FrCons *list, ostream &output, size_t firstindent,
		       size_t indent)
{
   size_t loc = firstindent+1 ;
   output << setw(loc) << "(" ;
   bool firstelt = true ;
   while (list)
      {
      if (list->consp())
	 {
	 FrObject *first = list->first() ;
	 if (first)
	    {
	    int len = first->displayLength() ;
	    loc += len ;
	    if (first->consp())
	       {
	       if (!firstelt && loc > FramepaC_display_width)
		  {
		  output << '\n' ;
		  loc = indent+len+1 ;
		  print_list((FrCons*)first,output,indent+2,indent+1) ;
		  }
	       else
		  print_list((FrCons*)first,output,0,indent+1) ;
	       }
	    else
	       {
	       if (!firstelt && loc > FramepaC_display_width)
		  {
		  output << '\n' << setw(indent+1) << " " ;
		  loc = indent+len+1 ;
		  }
	       first->printValue(output) ;
	       }
	    }
	 else
	    {
	    output << "()" ;
	    loc += 2 ;
	    }
	 FrObject *rest = ((FrList*)list)->rest() ;
	 if (rest && loc < FramepaC_display_width)
	    {
	    output << ' ' ;
	    loc++ ;
	    }
	 list = (FrCons *)rest ;
	 }
      else
	 {
	 output << ". " ;
	 list->printValue(output) ;
	 list = 0 ;
	 }
      firstelt = false ;
      }
   output << ')' ;
}

//----------------------------------------------------------------------

ostream &FrCons::printValue(ostream &output) const
{
   print_list((FrCons*)this,output,
	      0,FramepaC_initial_indent) ;
   return output ;
}

//----------------------------------------------------------------------

size_t FrCons::displayLength() const
{
   FrCons *current = (FrCons *)&(*this) ;
   size_t len = 2 ;

   while (current)
      {
      if (current->consp())
	 {
	 if (current->first())
	    len += current->first()->displayLength() ;
	 else
	    len += 2 ;	// "()"
	 if (current->m_ptr2)
	    len++ ;	// separating space
	 current = (FrCons *)current->m_ptr2 ;
	 }
      else
	 {
	 len += 2 ;	// separating dot and space
	 len += current->displayLength() ;
	 current = 0 ;
	 }
      }
   return len ;
}

//----------------------------------------------------------------------

char *FrCons::displayValue(char *buffer) const
{
   FrCons *current = (FrCons *)&(*this) ;

   *buffer++ = '(' ;
   while (current)
      {
      if (current->consp())
	 {
	 if (current->first())
	    buffer = current->first()->displayValue(buffer) ;
	 else
	    {
	    *buffer++ = '(' ;
	    *buffer++ = ')' ;
	    }
	 if (current->m_ptr2)
	    *buffer++ = ' ' ;
	 current = (FrCons *)current->m_ptr2 ;
	 }
      else
	 {
	 *buffer++ = '.' ;
	 *buffer++ = ' ' ;
	 buffer = current->displayValue(buffer) ;
	 current = 0 ;
	 }
      }
   *buffer++ = ')' ;
   *buffer = '\0' ;			// ensure proper string termination
   return buffer ;
}

// end of file frlist.cpp //
