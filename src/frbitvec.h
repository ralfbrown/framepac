/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frbitvec.cpp	class FrBitVector				*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1997,2001,2004,2009					*/
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

#ifndef __FRBITVEC_H_INCLUDED
#define __FRBITVEC_H_INCLUDED

#ifndef __FROBJECT_H_INCLUDED
#include "frobject.h"
#endif

#if defined(__GNUC__)
#  pragma interface
#endif

//----------------------------------------------------------------------

#define FrDEFAULT_BITVECTOR_LEN (8*FrBITS_PER_UINT)

bool booleanValue(const FrObject *obj) ;

class FrBitVector : public FrObject
   {
   protected:
      size_t _length ;
      size_t size ;
      unsigned int *vector ;
   protected:
      FrBitVector(size_t len, unsigned int *vect) ;
   public:
      void *operator new(size_t size) { return FrMalloc(size) ; }
      void operator delete(void *block) { FrFree(block) ; }
      FrBitVector(size_t len = 0) ;
      FrBitVector(size_t len, const FrList *init) ;
      FrBitVector(const FrBitVector &vect) ;
      virtual ~FrBitVector() ;
      virtual FrObjectType objType() const ;
      virtual const char *objTypeName() const ;
      virtual FrObjectType objSuperclass() const ;
      virtual ostream &printValue(ostream &output) const ;
      virtual char *displayValue(char *buffer) const ;
      virtual size_t displayLength() const ;
      virtual FrObject *copy() const ;
      virtual FrObject *deepcopy() const ;
      virtual unsigned long hashValue() const ;
      bool __FrCDECL iterate(FrIteratorFunc func, ...) const ;
      virtual bool vectorp() const ;
      virtual void freeObject() ;
      virtual size_t length() const ;
      virtual bool equal(const FrObject *obj) const ;
      virtual int compare(const FrObject *obj) const ;
      virtual bool iterateVA(FrIteratorFunc func, va_list) const ;
      virtual FrObject *subseq(size_t start, size_t stop) const ;
      virtual FrObject *reverse() ;
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
      virtual bool expand(size_t increment) ;
      virtual bool expandTo(size_t newsize) ;

      FrBitVector *intersection(const FrBitVector *othervector) const ;
      FrBitVector *vectorunion(const FrBitVector *othervector) const ;
      FrBitVector *difference(const FrBitVector *othervector) const ;
      size_t intersectionBits(const FrBitVector *othervector) const ;
      size_t vectorunionBits(const FrBitVector *othervector) const ;
      size_t differenceBits(const FrBitVector *othervector) const ;
      bool intersects(const FrBitVector *othervector) const ;

      void clear() ;
      void negate() ;

   // access to internal state
      size_t vectorlength() const { return _length ; }
      bool setBit(size_t N, bool newvalue = true) ;
      bool setRange(size_t firstN, size_t lastN, bool newvalue = true) ;
      bool getBit(size_t N) const ;
      size_t countBits() const ;

   // overloaded operators for class FrBitVector
      FrBitVector *operator * (const FrBitVector &othervect) const
	    { return intersection(&othervect) ; }
      FrBitVector *operator + (const FrBitVector &othervect) const
	    { return vectorunion(&othervect) ; }
      FrBitVector *operator - (const FrBitVector &othervect) const
	    { return difference(&othervect) ; }
   } ;

typedef FrBitVector FrBitVector ;

#endif /* !__FRBITVEC_H_INCLUDED */

// end of file frbitvec.h //
