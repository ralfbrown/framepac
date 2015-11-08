/****************************** -*- C++ -*- *****************************/
/*								        */
/*  FramepaC							        */
/*  Version 2.01						        */
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*								        */
/*  File frfloat.h	class FrFloat					*/
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

#ifndef __FRFLOAT_H_INCLUDED
#define __FRFLOAT_H_INCLUDED

#ifndef __FRNUMBER_H_INCLUDED
#include "frnumber.h"
#endif

#if defined(__GNUC__)
#  pragma interface
#endif

/************************************************************************/
/************************************************************************/

class FrFloat : public FrNumber
   {
   private:
      static FrAllocator allocator ;
      double m_value ;
   public:
      void *operator new(size_t) { return allocator.allocate() ; }
      void operator delete(void *blk) { allocator.release(blk) ; }
      FrFloat(double val = 0.0) { m_value = val ; }
      virtual ~FrFloat() {}
      virtual FrObjectType objType() const ;
      virtual const char *objTypeName() const ;
      virtual FrObjectType objSuperclass() const ;
      virtual void freeObject() ;
      virtual bool floatp() const ;
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
      FrFloat &operator += (long int adj) { m_value += adj ; return *this ; }
      FrFloat &operator -= (long int adj) { m_value -= adj ; return *this ; }
      FrFloat &operator *= (long int adj) { m_value *= adj ; return *this ; }
      FrFloat &operator /= (long int adj) { m_value /= adj ; return *this ; }

      // debugging
      static void dumpUnfreed(ostream &out) ;
   } ;

#endif /* !__FRFLOAT_H_INCLUDED */

// end of file frfloat.h //
