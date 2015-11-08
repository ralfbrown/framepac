/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC								*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File: frbpriq.h	      class FrBoundedPriQueue			*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 2002,2009 Ralf Brown/Carnegie Mellon University	*/
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

#ifndef __FRBPRIQ_H_INCLUDED
#define __FRBPRIQ_H_INCLUDED

#ifndef __FROBJECT_H_INCLUDED
#include "frobject.h"
#endif

#if defined(__GNUC__)
#  pragma interface
#endif

/************************************************************************/
/*	Manifest Constants						*/
/************************************************************************/

#define FrBPQ_NOT_FOUND ((size_t)~0)

/************************************************************************/
/*	Types								*/
/************************************************************************/

class FrBoundedPriQueue : public FrObject
   {
   private: // data
      static FrAllocator allocator ;
      static FrAllocator *e_allocator ;
      static FrAllocator *p_allocator ;
      static size_t alloc_size ;
      static double null_priority ;
      double *priorities ;
      FrObject **entries ;
      size_t q_size ;
      size_t q_head ;
      size_t q_tail ;
      bool sort_descending ;
      bool copy_objects ;
   protected: // methods
      size_t insertionPoint(double priority) const ;
      bool removeLoc(size_t loc) ;
   public: // methods
      //      void *operator new(size_t) { return allocator.allocate() ; }
      //      void operator delete(void *blk) { allocator.release(blk) ; }
      FrBoundedPriQueue(size_t max_size, bool descending = true,
			 bool copy = true) ;
      FrBoundedPriQueue(const FrBoundedPriQueue *) ;
      virtual ~FrBoundedPriQueue() ;
      static bool setAllocators(size_t queue_size) ;

      virtual FrObjectType objType() const ;
      virtual const char *objTypeName() const ;
      virtual FrObjectType objSuperclass() const ;
      //virtual FrReader *objReader() const { return 0 ; }
      virtual ostream &printValue(ostream &output) const ;
      //virtual char *displayValue(char *buffer) const ;
      //virtual size_t displayLength() const ;
      //virtual FrObject *copy() const ;
      //virtual FrObject *deepcopy() const ;

      // accessors
      size_t queueLength() const { return q_tail - q_head ; }
      const FrObject *first() const
	  { return (q_tail > q_head) ? entries[q_head] : 0 ; }
      const FrObject *first(double &priority) const
	  { if (q_tail > q_head)
	       { priority = priorities[q_head] ; return entries[q_head] ; }
 	    else
	       { priority = null_priority ; return 0 ; }
	  }
      double firstPriority() const
          { return (q_tail > q_head) ? priorities[q_head] : null_priority ; }
      double lastPriority() const ;
      size_t location(const FrObject *obj) const ;
      size_t location(const FrObject *obj, FrCompareFunc cmp) const ;
      size_t location(double priority) const ;
      static double nullPriority() { return null_priority ; }

      // modifiers
      bool push(FrObject *obj, double priority) ;
      bool push(FrObject *obj, double priority, size_t &position) ;
      FrObject *pop() ;
      FrObject *pop(double &priority) ;
      bool remove(const FrObject *obj) ;
      bool remove(double priority) ;
      static void nullPriority(double pri) { null_priority = pri ; }
   } ;

typedef class FrBoundedPriQueue FrBoundedPriQueue ;

#endif /* !__FRBPRIQ_H_INCLUDED */

// end of frbpriq.h //





