/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frfloat.cpp	   class FrFloat, + I/O				*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,1998,2000,2001,2003,2006,2009	*/
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
#  pragma implementation "frfloat.h"
#endif

#include "frfloat.h"
#include "frprintf.h"
#include "frpcglbl.h"

#ifdef FrSTRICT_CPLUSPLUS
#  include <cstdio>
#  include <cstdlib>
#  include <iomanip>
#  include <string>
#else
#  include <iomanip.h>
#  include <stdio.h>
#  include <stdlib.h>
#  include <string.h>
#endif /* FrSTRICT_CPLUSPLUS */

/************************************************************************/
/*	Manifest Constants						*/
/************************************************************************/

#ifdef __GNUC__
#  define FLOAT_FORMAT "%.16g"
#else
#  define FLOAT_FORMAT "%.16lg"
#endif

/************************************************************************/
/*	Forward Declarations						*/
/************************************************************************/

extern FrNumber *(*_FramepaC_make_Float)(const char *str, char **stopper)  ;
extern FrNumber *(*_FramepaC_make_Rational)(long num, long denom) ;
extern double (*_FramepaC_long_to_double)(long value) ;

FrNumber *make_Float(const char *str, char **stopper) ;
FrNumber *make_Rational(long num, long denom) ;
double long_to_double(long num) ;

/************************************************************************/
/*	Types local to this module					*/
/************************************************************************/

class FrFloatInitializer
   {
   public:
      FrFloatInitializer()
        { _FramepaC_make_Float = make_Float ;
  	  _FramepaC_make_Rational = make_Rational ;
	  _FramepaC_long_to_double = long_to_double ; }
   } ;

/************************************************************************/
/*	Global data limited to this module			      	*/
/************************************************************************/

static const char str_FrFloat[] = "FrFloat" ;

/************************************************************************/
/*    Global variables for class FrFloat			      	*/
/************************************************************************/

FrAllocator FrFloat::allocator(str_FrFloat,sizeof(FrFloat)) ;

static FrFloatInitializer init ;

/************************************************************************/
/*    Member functions for class FrFloat			      	*/
/************************************************************************/

FrObjectType FrFloat::objType() const
{
   return OT_FrFloat ;
}

//----------------------------------------------------------------------

const char *FrFloat::objTypeName() const
{
   (void)&init ; // just to have a ref, to keep compiler happy
   return str_FrFloat ;
}

//----------------------------------------------------------------------

void FrFloat::freeObject()
{
   delete this ;
   return ;
}

//----------------------------------------------------------------------

bool FrFloat::floatp() const
{
   return true ;
}

//----------------------------------------------------------------------

FrObjectType FrFloat::objSuperclass() const
{
   return OT_FrNumber ;
}

//----------------------------------------------------------------------

unsigned long FrFloat::hashValue() const
{
   return (unsigned long)m_value ;
}

//----------------------------------------------------------------------

long int FrFloat::intValue() const
{
   return (long int) m_value ;
}

//----------------------------------------------------------------------

double FrFloat::floatValue() const
{
   return m_value ;
}

//----------------------------------------------------------------------

double FrFloat::real() const
{
   return floatValue() ;
}

//----------------------------------------------------------------------

double FrFloat::fraction() const
{
   return (floatValue() - intValue()) ;
}

//----------------------------------------------------------------------

int FrFloat::compare(const FrObject *obj) const
{
   if (!obj)
      return 1 ;     // anything is greater than NIL / empty-list
   if (!obj->numberp())
      return -1 ;    // sort all non-numbers after numbers
//   else if (objectType() < obj->objectType())
//      return -((FrNumber*)obj)->compare(this) ;
   else
      {
      double diff = floatValue() - ((FrNumber*)obj)->floatValue() ;
      if (diff < 0.0)
	 return -1 ;
      else if (diff > 0.0)
	 return +1 ;
      else
	 return 0 ;
      }
}

//----------------------------------------------------------------------

FrObject *FrFloat::copy() const
{
   if (this)
      return new FrFloat(floatValue()) ;
   else
      return 0 ;
}

//----------------------------------------------------------------------

FrObject *FrFloat::deepcopy() const
{
   if (this)
      return new FrFloat(floatValue()) ;
   else
      return 0 ;
}

//----------------------------------------------------------------------

ostream &FrFloat::printValue(ostream &output) const
{
   return output << setprecision(16) << floatValue() ;
}

//----------------------------------------------------------------------

size_t FrFloat::displayLength() const
{
   return Fr_sprintf(0,0,FLOAT_FORMAT,floatValue()) ;
}

//----------------------------------------------------------------------

char *FrFloat::displayValue(char *buffer) const
{
   return buffer + Fr_sprintf(buffer,FrMAX_DOUBLE_STRING,
			      FLOAT_FORMAT,floatValue()) ;
}

//----------------------------------------------------------------------

static bool dump_unfreed(void *obj, va_list args)
{
   FrFloat *number = (FrFloat*)obj ;
   FrVarArg(ostream *,out) ;
   if (number && out)
      (*out) << ' ' << number << flush ;
   return true ;			// continue iterating
}

void FrFloat::dumpUnfreed(ostream &out)
{
   allocator.iterate(dump_unfreed,&out) ;
   out << endl ;
   return ;
}

/************************************************************************/
/*									*/
/************************************************************************/

FrNumber *make_Float(const char *str, char **stopper)
{
   return new FrFloat(strtod(str,stopper)) ;
}

//----------------------------------------------------------------------

FrNumber *make_Rational(long num, long denom)
{
   return new FrFloat(num / (double)denom) ;
}

//----------------------------------------------------------------------

double long_to_double(long value)
{
   return (double)value ;
}

// end of file frfloat.cpp //
