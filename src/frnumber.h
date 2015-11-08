/****************************** -*- C++ -*- *****************************/
/*								        */
/*  FramepaC  -- frame manipulation in C++			        */
/*  Version 2.01						        */
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*								        */
/*  File frnumber.h	classes FrNumber, FrInteger, and FrInteger64	*/
/*  LastEdit: 08nov2015						        */
/*								        */
/*  (c) Copyright 1994,1995,1996,1997,1998,2000,2001,2006,2009,2010	*/
/*		 Ralf Brown/Carnegie Mellon University			*/
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

#ifndef __FRNUMBER_H_INCLUDED
#define __FRNUMBER_H_INCLUDED

#ifndef __FROBJECT_H_INCLUDED
#include "frobject.h"
#endif

#if defined(__GNUC__)
#  pragma interface
#endif

/************************************************************************/
/************************************************************************/

class FrNumber : public FrAtom
   {
   private:
      // none
   public:
      virtual bool numberp() const ;
      virtual FrObjectType objType() const ;
      virtual const char *objTypeName() const ;
      virtual FrObjectType objSuperclass() const ;
      virtual FrReader *objReader() const ;
      virtual long int intValue() const ;
      virtual int64_t int64value() const ;
      virtual double floatValue() const ;
      virtual double fraction() const ;
      virtual double real() const ;
      virtual double imag() const ;
      virtual bool equal(const FrObject *obj) const ;
      virtual int compare(const FrObject *obj) const ;
      virtual FrObject *copy() const ;
      virtual FrObject *deepcopy() const ;
      virtual void freeObject() ;
      virtual FrSymbol *coerce2symbol(FrCharEncoding enc) const ;
   //overloaded operators
      operator int () const { return (int)intValue() ; }
      operator long () const { return intValue() ; }
#if LONG_MAX <= 2147483647L
      operator int64_t () const { return int64value() ; }
#endif
      operator double () const { return floatValue() ; }
      int operator == (const double testval) const
	 { return floatValue() == testval ; }
      int operator == (const long testval) const
	 { return intValue() == testval ; }
      int operator == (const FrNumber &testval) const
	 { return compare(&testval) == 0 ; }
      int operator != (const double testval) const
	 { return floatValue() != testval ; }
      int operator != (const long testval) const
	 { return intValue() != testval ; }
      int operator != (const FrNumber &testval) const
	 { return compare(&testval) != 0 ; }
      int operator < (const double testval) const
	 { return floatValue() < testval ; }
      int operator < (const long testval) const
	 { return intValue() < testval ; }
      int operator < (const FrNumber &testval) const
	 { return compare(&testval) < 0 ; }
      int operator <= (const double testval) const
	 { return floatValue() <= testval ; }
      int operator <= (const long testval) const
	 { return intValue() <= testval ; }
      int operator <= (const FrNumber &testval) const
	 { return compare(&testval) <= 0 ; }
      int operator > (const double testval) const
	 { return floatValue() > testval ; }
      int operator > (const long testval) const
	 { return intValue() > testval ; }
      int operator > (const FrNumber &testval) const
	 { return compare(&testval) > 0 ; }
      int operator >= (const double testval) const
	 { return floatValue() >= testval ; }
      int operator >= (const long testval) const
	 { return intValue() >= testval ; }
      int operator >= (const FrNumber &testval) const
	 { return compare(&testval) >= 0 ; }
   } ;

/************************************************************************/
/************************************************************************/

class FrInteger : public FrNumber
   {
   private:
      static FrAllocator allocator ;
      long int m_value ;
   public:
      void *operator new(size_t) { return allocator.allocate() ; }
      void operator delete(void *blk) { allocator.release(blk) ; return ; }
      FrInteger(long int val = 0) { m_value = val ; }
      virtual ~FrInteger() {}
      virtual FrObjectType objType() const ;
      virtual const char *objTypeName() const ;
      virtual FrObjectType objSuperclass() const ;
      virtual void freeObject() ;
      virtual int compare(const FrObject *obj) const ;
      virtual FrObject *copy() const ;
      virtual FrObject *deepcopy() const ;
      virtual ostream &printValue(ostream &output) const ;
      virtual char *displayValue(char *buffer) const ;
      virtual size_t displayLength() const ;
      virtual long int intValue() const ;
      virtual double real() const ;
      virtual double floatValue() const ;
      virtual double fraction() const ;
      virtual unsigned long hashValue() const ;

      // manipulators
      FrInteger &operator += (long int adj) { m_value += adj ; return *this ; }
      FrInteger &operator -= (long int adj) { m_value -= adj ; return *this ; }
      FrInteger &operator *= (long int adj) { m_value *= adj ; return *this ; }
      FrInteger &operator /= (long int adj) { m_value /= adj ; return *this ; }

      // debugging
      static void dumpUnfreed(ostream &out) ;
   } ;

/************************************************************************/
/************************************************************************/

class FrInteger64 : public FrNumber
   {
   private:
      static FrAllocator allocator ;
      int64_t m_value ;
   public:
      void *operator new(size_t) { return allocator.allocate() ; }
      void operator delete(void *blk) { allocator.release(blk) ; }
      FrInteger64(int64_t val = 0) { m_value = val ; }
      virtual ~FrInteger64() {}
      virtual FrObjectType objType() const ;
      virtual const char *objTypeName() const ;
      virtual FrObjectType objSuperclass() const ;
      virtual FrObject *copy() const ;
      virtual FrObject *deepcopy() const ;
      virtual void freeObject() ;
      virtual int compare(const FrObject *obj) const ;
      virtual ostream &printValue(ostream &output) const ;
      virtual char *displayValue(char *buffer) const ;
      virtual size_t displayLength() const ;
      virtual long int intValue() const ;
      virtual int64_t int64value() const ;
      virtual double real() const ;
      virtual double floatValue() const ;
      virtual double fraction() const ;
      virtual unsigned long hashValue() const ;
   } ;

#endif /* !__FRNUMBER_H_INCLUDED */

// end of file frnumber.h //
