/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frstack.h	   class FrStack				*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1997,1998,2001,2009					*/
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

#ifndef __FRSTACK_H_INCLUDED
#define __FRSTACK_H_INCLUDED

#ifndef __FRQUEUE_H_INCLUDED
#include "frqueue.h"
#endif

#if defined(__GNUC__)
#  pragma interface
#endif

/**********************************************************************/
/*	declaration of class FrStack				      */
/**********************************************************************/

class FrStack : public FrQueue
   {
   public:
      FrStack() {}
      FrStack(const FrList *items) : FrQueue(items) {}
      virtual ~FrStack() {}
      virtual FrObjectType objType() const ;
      virtual const char *objTypeName() const ;
      virtual FrObjectType objSuperclass() const ;
      virtual void freeObject() ;
      // virtual bool queuep() const ;
      virtual FrObject *copy() const ;
      virtual FrObject *deepcopy() const ;
      virtual ostream &printValue(ostream &out) const ;
      virtual size_t displayLength() const ;
      virtual char *displayValue(char *buffer) const ;
      // virtual unsigned long hashValue() const ;
      // virtual size_t length() const ;
      // virtual FrObject *reverse() ;
      // virtual FrObject *subseq(size_t start, size_t stop) const ;
      // virtual FrObject *car() const ;
      // virtual bool iterateVA(FrIteratorFunc func, va_list) const ;
      // virtual FrObject *getNth(size_t N) const ;
      // virtual bool setNth(size_t N, const FrObject *elt) ;
      // virtual size_t locate(const FrObject *item,
      //		       size_t start = (size_t)-1) const ;
      // virtual size_t locate(const FrObject *item,
      //		    FrCompareFunc func,
      //		    size_t start = (size_t)-1) const ;
      // virtual FrObject *insert(const FrObject *,size_t location,
      //			   bool copyitem = true) ;
      // virtual FrObject *elide(size_t start, size_t end) ;
      // virtual FrObject *removeDuplicates() const ;
      // virtual FrObject *removeDuplicates(FrCompareFunc) const ;
      void add(const FrObject *item, bool copy_item = true)
	 { FrQueue::addFront(item,copy_item) ; }
      void addBottom(const FrObject *item, bool copy_item = true)
	 { FrQueue::add(item,copy_item) ; }
      // bool remove(const FrObject *item) ;
      // bool remove(const FrObject *item,FrCompareFunc cmp) ;
      // FrObject *find(const FrObject *item, FrCompareFunc cmp) const ;
      // FrObject *pop() ;
      // FrObject *peek() const { return qhead ? qhead->first() : 0 ; }
      // void clear() ;
      // size_t queueLength() const { return qlength ; }
      size_t stackHeight() const { return qlength ; }
   } ;

#endif /* !__FRSTACK_H_INCLUDED */

// end of file frstack.h //
