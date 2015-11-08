/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC								*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frnumber.cpp	   classes FrNumber and FrInteger		*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,1998,2000,2001,2003,2009		*/
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

#if defined(__GNUC__)
#  pragma implementation "frnumber.h"
#endif

#include "frnumber.h"
#include "frpcglbl.h"

#ifdef FrSTRICT_CPLUSPLUS
#  include <cstdlib>	// for ultoa() under WatcomC++
#else
#  include <stdlib.h>	// for ultoa() under WatcomC++
#endif /* FrSTRICT_CPLUSPLUS */

/************************************************************************/
/*	Global variables visible from other modules		      	*/
/************************************************************************/

FrNumber *(*_FramepaC_make_Float)(const char *str, char **stopper) = 0 ;
FrNumber *(*_FramepaC_make_Rational)(long num, long denom) = 0 ;
FrInteger64 *(*_FramepaC_make_Int64)(const char *str, char **stopper,
				      long radix) = 0 ;
double (*_FramepaC_long_to_double)(long value) = 0 ;

/************************************************************************/
/*	Global data limited to this module			      	*/
/************************************************************************/

static const char str_FrInteger[] = "FrInteger" ;

/**********************************************************************/
/*    Member functions for class FrNumber			      */
/**********************************************************************/

FrObjectType FrNumber::objType() const
{
   return OT_FrNumber ;
}

//----------------------------------------------------------------------

const char *FrNumber::objTypeName() const
{
   return "FrNumber" ;
}

//----------------------------------------------------------------------

FrObjectType FrNumber::objSuperclass() const
{
   return OT_FrAtom ;
}

//----------------------------------------------------------------------

bool FrNumber::numberp() const
{
   return true ;
}

//----------------------------------------------------------------------

long int FrNumber::intValue() const
{
   if (FramepaC_typesafe_virtual)
      FrInvalidVirtualFunction("FrNumber::intValue") ;
   return 0L ;
}

//----------------------------------------------------------------------

int64_t FrNumber::int64value() const
{
   return intValue() ;
}

//----------------------------------------------------------------------

double FrNumber::floatValue() const
{
   if (FramepaC_typesafe_virtual)
      FrInvalidVirtualFunction("FrNumber::floatValue") ;
   return 0.0 ;
}

//----------------------------------------------------------------------

double FrNumber::real() const
{
   if (FramepaC_typesafe_virtual)
      FrInvalidVirtualFunction("FrNumber::real") ;
   return 0.0 ;
}

//----------------------------------------------------------------------

double FrNumber::imag() const
{
   return 0.0 ;
}

//----------------------------------------------------------------------

double FrNumber::fraction() const
{
   if (FramepaC_typesafe_virtual)
      FrInvalidVirtualFunction("FrNumber::fraction") ;
   return 0.0 ;
}

//----------------------------------------------------------------------

int FrNumber::compare(const FrObject *) const
{
   if (FramepaC_typesafe_virtual)
      FrInvalidVirtualFunction("FrNumber::compare") ;
   return 0 ;
}

//----------------------------------------------------------------------

bool FrNumber::equal(const FrObject *obj) const
{
   if (this == obj)
      return true ;	   // equal if comparing to ourselves
   else
      return compare(obj) == 0 ;
}

//----------------------------------------------------------------------

FrObject *FrNumber::copy() const
{
   if (FramepaC_typesafe_virtual)
      FrInvalidVirtualFunction("FrNumber::copy") ;
   return 0 ;
}

//----------------------------------------------------------------------

FrObject *FrNumber::deepcopy() const
{
   if (FramepaC_typesafe_virtual)
      FrInvalidVirtualFunction("FrNumber::deepcopy") ;
   return 0 ;
}

//----------------------------------------------------------------------

void FrNumber::freeObject()
{
   delete this ;
   return ;
}

//----------------------------------------------------------------------

FrSymbol *FrNumber::coerce2symbol(FrCharEncoding) const
{
   char *buf = print() ;
   if (buf)
      {
      FrSymbol *sym = FrSymbolTable::add(buf) ;
      FrFree(buf) ;
      return sym ;
      }
   else
      return 0 ;
}

/**********************************************************************/
/*    Global variables for class FrInteger			      */
/**********************************************************************/

FrAllocator FrInteger::allocator(str_FrInteger,sizeof(FrInteger)) ;

/**********************************************************************/
/*    Member functions for class FrInteger			      */
/**********************************************************************/

FrObjectType FrInteger::objType() const
{
   return OT_FrInteger ;
}

//----------------------------------------------------------------------

const char *FrInteger::objTypeName() const
{
   return str_FrInteger ;
}

//----------------------------------------------------------------------

void FrInteger::freeObject()
{
   delete this ;
   return ;
}

//----------------------------------------------------------------------

FrObjectType FrInteger::objSuperclass() const
{
   return OT_FrNumber ;
}

//----------------------------------------------------------------------

unsigned long FrInteger::hashValue() const
{
   return (unsigned long)m_value ;
}

//----------------------------------------------------------------------

long int FrInteger::intValue() const
{
   return m_value ;
}

//----------------------------------------------------------------------

double FrInteger::floatValue() const
{
   if (_FramepaC_long_to_double)
      return _FramepaC_long_to_double(intValue()) ;
   else
      return 0.0 ;
}

//----------------------------------------------------------------------

double FrInteger::real() const
{
   return floatValue() ;
}

//----------------------------------------------------------------------

double FrInteger::fraction() const
{
   return 0.0 ;
}

//----------------------------------------------------------------------

int FrInteger::compare(const FrObject *obj) const
{
   if (!obj)
      return 1 ;     // anything is greater than NIL / empty-list
   if (!obj->numberp())
      return -1 ;    // sort all non-numbers after numbers
   else if (objType() < obj->objType())
      return -((FrNumber*)obj)->compare(this) ;
   else
      {
      long diff = intValue() - ((FrNumber*)obj)->intValue() ;
      if (diff < 0)
	 return -1 ;
      else if (diff > 0)
	 return +1 ;
      else
	 return 0 ;
      }
}

//----------------------------------------------------------------------

FrObject *FrInteger::copy() const
{
   if (this)
      return new FrInteger(intValue()) ;
   else
      return 0 ;
}

//----------------------------------------------------------------------

FrObject *FrInteger::deepcopy() const    // note: identical to copy()
{
   if (this)
      return new FrInteger(intValue()) ;
   else
      return 0 ;
}

//----------------------------------------------------------------------

ostream &FrInteger::printValue(ostream &output) const
{
   return output << intValue() ;
}

//----------------------------------------------------------------------

size_t FrInteger::displayLength() const
{
   if (intValue() < 0)
      return Fr_number_length(-intValue())+1 ;
   else
      return Fr_number_length(intValue()) ;
}

//----------------------------------------------------------------------

char *FrInteger::displayValue(char *buffer) const
{
   if (intValue() < 0)
      {
      *buffer++ = '-' ;
      ultoa(-intValue(),buffer,10) ;
      return buffer + strlen(buffer) ;
      }
   else
      {
      ultoa(intValue(),buffer,10) ;
      return buffer + strlen(buffer) ;
      }
}

//----------------------------------------------------------------------

static bool dump_unfreed(void *obj, va_list args)
{
   FrInteger *number = (FrInteger*)obj ;
   FrVarArg(ostream *,out) ;
   if (number && out)
      (*out) << ' ' << number << flush ;
   return true ;			// continue iterating
}

void FrInteger::dumpUnfreed(ostream &out)
{
   allocator.iterate(dump_unfreed,&out) ;
   out << endl ;
   return ;
}

// end of file frnumber.cpp //
