/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frnum64.cpp	   class FrInteger64				*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1998,2002,2003,2006,2009,2010,2012			*/
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

#include "frnumber.h"
#include "frpcglbl.h"

#ifdef FrSTRICT_CPLUSPLUS
#  include <cstdio>
#  include <cstdlib>
#  include <string>
#else
#  include <stdio.h>
#  include <stdlib.h>
#  include <string.h>
#endif /* FrSTRICT_CPLUSPLUS */

/************************************************************************/
/*	Forward Declarations						*/
/************************************************************************/

extern FrInteger64 *(*_FramepaC_make_Int64)(const char *str, char **stopper,
					     long radix) ;

FrInteger64 *make_Int64(const char *str, char **stopper, long radix) ;

/************************************************************************/
/*	Types local to this module					*/
/************************************************************************/

class FrInt64Initializer
   {
   public:
      FrInt64Initializer() { _FramepaC_make_Int64 = make_Int64 ; }
   } ;

/************************************************************************/
/*	Global data limited to this module			      	*/
/************************************************************************/

static const char str_FrInt64[] = "FrInteger64" ;

/************************************************************************/
/*    Global variables for class FrInteger64			      	*/
/************************************************************************/

FrAllocator FrInteger64::allocator(str_FrInt64,sizeof(FrInteger64)) ;

static FrInt64Initializer init ;

/************************************************************************/
/*    Member functions for class FrInteger64			      	*/
/************************************************************************/

FrObjectType FrInteger64::objType() const
{
   return OT_FrInteger64 ;
}

//----------------------------------------------------------------------

const char *FrInteger64::objTypeName() const
{
   (void)&init ; // just to have a ref, to keep compiler happy
   return str_FrInt64 ;
}

//----------------------------------------------------------------------

void FrInteger64::freeObject()
{
   delete this ;
}

//----------------------------------------------------------------------

FrObjectType FrInteger64::objSuperclass() const
{
   return OT_FrNumber ;
}

//----------------------------------------------------------------------

unsigned long FrInteger64::hashValue() const
{
   return (unsigned long)m_value ;
}

//----------------------------------------------------------------------

long int FrInteger64::intValue() const
{
   return (long int)m_value ;
}

//----------------------------------------------------------------------

int64_t FrInteger64::int64value() const
{
   return m_value ;
}

//----------------------------------------------------------------------

double FrInteger64::floatValue() const
{
   return (double)m_value ;
}

//----------------------------------------------------------------------

double FrInteger64::real() const
{
   return (double)m_value ;
}

//----------------------------------------------------------------------

double FrInteger64::fraction() const
{
   return 0.0 ;
}

//----------------------------------------------------------------------

int FrInteger64::compare(const FrObject *obj) const
{
   if (!obj)
      return 1 ;     // anything is greater than NIL / empty-list
   if (!obj->numberp())
      return -1 ;    // sort all non-numbers after numbers
   else if (objType() < obj->objType())
      return -((FrNumber*)obj)->compare(this) ;
   else
      {
      int64_t diff = m_value - ((FrNumber*)obj)->int64value() ;
      if (diff < 0L)
	 return -1 ;
      else if (diff > 0L)
	 return +1 ;
      else
	 return 0 ;
      }
}

//----------------------------------------------------------------------

FrObject *FrInteger64::copy() const
{
   if (this)
      return new FrInteger64(int64value()) ;
   else
      return 0 ;
}

//----------------------------------------------------------------------

FrObject *FrInteger64::deepcopy() const
{
   if (this)
      return new FrInteger64(int64value()) ;
   else
      return 0 ;
}

//----------------------------------------------------------------------

ostream &FrInteger64::printValue(ostream &output) const
{
   return output << m_value ;
}

//----------------------------------------------------------------------

size_t FrInteger64::displayLength() const
{
   int64_t tmp(m_value) ;
   size_t digits = 0 ;
   if (m_value < 0L)
      {
      digits++ ;
      tmp = -tmp ;
      }
   do {
      digits++ ;
      tmp /= 10L ;
      } while (tmp != 0L) ;
   return digits ;
}

//----------------------------------------------------------------------

char *FrInteger64::displayValue(char *buffer) const
{
   char buf[FrMAX_INT64_STRING+1] ;
   bool neg = (m_value < 0L) ;
   int64_t tmp(m_value) ;
   size_t digits = 0 ;
   if (neg)
      tmp = -tmp ;
   do {
      buf[digits++] = (char)((uint16_t)(tmp % 10L)) ;
      tmp /= 10L ;
      } while (tmp != 0L) ;
   if (neg)
      *buffer++ = '-' ;
   while (digits > 0)
      {
      digits-- ;
      *buffer++ = buf[digits] ;
      }
   *buffer = '\0' ;
   return buffer;
}

/************************************************************************/
/*									*/
/************************************************************************/

FrInteger64 *make_Int64(const char *str, char **stopper, long radix)
{
   bool neg = false ;
   if (*str == '-')
      {
      neg = true ;
      str++ ;
      }
   else if (*str == '+')
      str++ ;
   static int64_t value ;
   value = 0 ;
   while (Fr_isdigit(*str))
      {
      value = (value * radix) + (*str - '0') ;
      str++ ;
      }
   if (stopper)
      *stopper = (char*)str ;
   if (neg)
      value = -value ;
   return new FrInteger64(value) ;
}

// end of file frnum64.cpp //
