/************************************************************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File: frlist.h	class FrCons and class FrList			*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,1998,2001,2004,2007,2008,2009,	*/
/*		2010,2015 Ralf Brown/Carnegie Mellon University		*/
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

#ifndef __FRLIST_H_INCLUDED
#define __FRLIST_H_INCLUDED

#ifndef __FROBJECT_H_INCLUDED
#include "frobject.h"
#endif

#if defined(__GNUC__)
#  pragma interface
#endif

/**********************************************************************/
/*    types used by this module					      */
/**********************************************************************/

typedef FrObject *(*FrListMapFunc)(const FrObject *obj, va_list args) ;

/**********************************************************************/
/*	declaration of class FrCons				      */
/**********************************************************************/

class FrCons : public FrObject
   {
   private:
      static FrAllocator allocator ;
   protected:
      FrObject *m_ptr1, *m_ptr2 ;
      FrCons() {} ;
   public:
      void *operator new(size_t) { return allocator.allocate() ; }
      void operator delete(void *blk,size_t) { allocator.release(blk) ; }
      FrCons(const FrObject *frst, const FrObject *rest = 0)
           { replaca(frst) ; replacd(rest) ; }
      virtual void freeObject() ;
      virtual bool consp() const ;
      virtual FrObjectType objType() const ;
      virtual const char *objTypeName() const ;
      virtual FrObjectType objSuperclass() const ;
      virtual ostream &printValue(ostream &output) const ;
      virtual char *displayValue(char *buffer) const ;
      virtual size_t displayLength() const ;
      virtual FrObject *copy() const ;
      virtual FrObject *deepcopy() const ;
      virtual unsigned long hashValue() const ;
      virtual FrObject *reverse() ;
      virtual FrObject *car() const ;
      virtual FrObject *cdr() const ;
      virtual bool equal(const FrObject *obj) const ;
      virtual int compare(const FrObject *obj) const ;
      virtual bool iterateVA(FrIteratorFunc func, va_list args) const ;
      virtual FrObject *getNth(size_t N) const ;
      virtual bool setNth(size_t N, const FrObject *elt) ;
      FrObject *first() const { return m_ptr1 ; }
      FrObject *consCdr() const { return m_ptr2 ; }
      void replaca(const FrObject *newcar) { m_ptr1 = (FrObject*)newcar ; }
      void replacd(const FrObject *newcdr) { m_ptr2 = (FrObject*)newcdr ; }

      static void reserve(size_t N) ;

      // debugging
      static void dumpUnfreed(ostream &out) ;
   } ;

/**********************************************************************/
/*	declaration of class FrList				      */
/**********************************************************************/

typedef int ListSortCmpFunc(const FrObject *,const FrObject *) ;

class FrList : public FrCons
   {
   private:
      // none, all inherited from FrCons
   public:
      FrList() {} // for internal use! (caller must do both replaca & replacd)
      FrList(const FrObject *one)
            { replaca(one) ; replacd(0) ; }
      FrList(const FrObject *one, const FrObject *two) ;
      FrList(const FrObject *one, const FrObject *two,
	    const FrObject *three) ;
      FrList(const FrObject *one, const FrObject *two,
	    const FrObject *three, const FrObject *four) ;
      virtual ~FrList() ;
      virtual FrObjectType objType() const ;
      virtual const char *objTypeName() const ;
      virtual FrObjectType objSuperclass() const ;
      virtual void freeObject() ;
      virtual FrReader *objReader() const ;
      virtual FrObject *copy() const ;
      virtual FrObject *deepcopy() const ;
      virtual unsigned long hashValue() const ;
      virtual FrObject *reverse() ;
      virtual size_t length() const ;
      virtual int compare(const FrObject *obj) const ;
      virtual bool iterateVA(FrIteratorFunc func, va_list args) const ;
      virtual FrObject *subseq(size_t start, size_t stop) const ;
      virtual FrObject *getNth(size_t N) const ;
      virtual bool setNth(size_t N, const FrObject *elt) ;
      virtual size_t locate(const FrObject *item,
			    size_t start = (size_t)-1) const ;
      virtual size_t locate(const FrObject *item,
			    FrCompareFunc func,
			    size_t start = (size_t)-1) const ;
      virtual FrObject *insert(const FrObject *,size_t location,
			        bool copyitem = true) ;
      virtual FrObject *elide(size_t start, size_t end) ;
      virtual FrObject *removeDuplicates() const ;
      virtual FrObject *removeDuplicates(FrCompareFunc) const ;
      bool equiv(const FrList *l) const ;
      void eraseList(bool deep) ;
      FrList *rest() const { return (FrList *)m_ptr2 ; }
      FrList *last() const ;
      FrList *nconc(FrList *appendage) ;
      void nconc(FrList *appendage, FrList **&end) ;
      FrList * __FrCDECL mapcar(FrListMapFunc map, ...) const ;
      FrList * __FrCDECL mapcan(FrListMapFunc map, ...) const ;
      size_t listlength() const ;
      size_t simplelistlength() const ; // doesn't handle dotted lists
      FrList *shallowsubseq(size_t start, size_t stop) const ;
      FrList *member(const FrObject *item) const ;
      FrList *member(const FrObject *item, FrCompareFunc cmp) const ;
      int position(const FrObject *item) const ;
      int position(const FrObject *item, bool from_end) const ;
      int position(const FrObject *item, FrCompareFunc cmp) const ;
      int position(const FrObject *item, FrCompareFunc cmp,
		   bool from_end) const ;
      FrCons *assoc(const FrObject *item) const ;
      FrCons *assoc(const FrObject *item,FrCompareFunc cmp) const ;
      bool contains(const FrList *sublist) const ;
      bool contains(const FrList *sublist, FrCompareFunc cmp) const ;
      FrList *difference(const FrList *l2) const ;
      FrList *difference(const FrList *l2,FrCompareFunc cmp) const ;
      FrList *ndifference(const FrList *l2) ;
      FrList *ndifference(const FrList *l2,FrCompareFunc cmp) ;
      FrList *listunion(const FrList *l2) const ;
      FrList *listunion(const FrList *l2, FrCompareFunc cmp) const ;
      FrList *nlistunion (const FrList *l2) ;
      FrList *nlistunion (const FrList *l2, FrCompareFunc cmp) ;
      FrList *intersection(const FrList *l2) const ;
      FrList *intersection(const FrList *l2, FrCompareFunc cmp) const ;
      FrList *sort(ListSortCmpFunc *cmpfunc) ;
      FrList *nthcdr(size_t n) const ;
      FrObject *nth(size_t n) const ;
      FrObject *second() const ;
      FrObject *third() const ;
      bool subsetOf(const FrList *l2) const ;
      bool subsetOf(const FrList *l2, FrCompareFunc cmp) const ;
      bool intersect(const FrList *l2) const ;
      bool intersect(const FrList *l2, FrCompareFunc cmp) const ;
      //note: pushlistend requires 'end' to be initialized to the pointer to
      //  hold the resulting list, and '*end' to be set to 0 after the final
      //  call to properly terminate the list
      static void pushlistend(const FrObject *newitem, FrList **&end) ;
   //overloaded operators
      FrList *operator - (const FrList *l) const
	 { return difference(l) ; }
      FrList *operator + (const FrList *l) const
	 { return listunion(l) ; }
      FrList *operator * (const FrList *l) const
	 { return intersection(l) ; }
      FrObject *&operator [] (size_t pos) const ;
   } ;

//----------------------------------------------------------------------
// non-member functions related to class FrList

FrList *pushlist(const FrObject *newitem, FrList *&list) ;
FrList *pushlistnew(const FrObject *newitem, FrList *&list) ;
FrList *pushlistnew(const FrObject *newitem, FrList *&list,
		     FrCompareFunc cmp) ;
FrObject *poplist(FrList *&list) ;
FrObject *poplist(FrList *&list, bool destructive) ;
FrList * __FrCDECL makelist(const FrObject *obj1, ...) ;
FrList *listreverse(FrList *list) ;
FrList *listremove(const FrList *l, const FrObject *item) ;
FrList *listremove(const FrList *l, const FrObject *item,FrCompareFunc cmp) ;

FrList *FrFlattenList(const FrList *l) ;
FrList *FrFlattenListInPlace(FrList *l, bool remove_NILs = false) ;

//----------------------------------------------------------------------

inline FrList *nconc(FrList *l1, FrList *l2)
   { return l1->nconc(l2) ; }

inline FrList *FrCopyList(const FrList *l)
   { return l ? (FrList*)l->deepcopy() : 0 ; }

#endif /* !__FRLIST_H_INCLUDED */

// end of file frlist.h //
