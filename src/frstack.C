/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frstack.cpp	class FrStack					*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,1998,2001,2009,2011		*/
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

#if defined(__GNUC__)
#  pragma implementation "frstack.h"
#endif

#include "frstack.h"
#include "frpcglbl.h"

/************************************************************************/
/*	Manifest constants						*/
/************************************************************************/

#define FRSTACK_INTRO "Stack"

/************************************************************************/
/*	Global variables for class FrStack				*/
/************************************************************************/

static FrObject *read_Stack(istream &input, const char *) ;
static FrObject *string_to_Stack(const char *&input, const char *) ;
static bool verify_Stack(const char *&input, const char *, bool strict) ;

static FrReader FrStack_reader(string_to_Stack, read_Stack, verify_Stack,
			       FrREADER_LEADIN_LISPFORM,FRSTACK_INTRO) ;

static const char frstack_intro[] = "#" FRSTACK_INTRO ;

/************************************************************************/
/*    Member functions for class FrStack				*/
/************************************************************************/

FrObjectType FrStack::objType() const
{
   return OT_FrStack ;
}

//----------------------------------------------------------------------

const char *FrStack::objTypeName() const
{
   return "FrStack" ;
}

//----------------------------------------------------------------------

FrObjectType FrStack::objSuperclass() const
{
   return OT_FrStack ;
}

//----------------------------------------------------------------------

void FrStack::freeObject()
{
   delete this ;
}

//----------------------------------------------------------------------

FrObject *FrStack::copy() const
{
   return new FrStack(qhead) ;
}

//----------------------------------------------------------------------

FrObject *FrStack::deepcopy() const
{
   return new FrStack(qhead) ;
}

//----------------------------------------------------------------------

ostream &FrStack::printValue(ostream &out) const
{
   size_t orig_indent = FramepaC_initial_indent ;
   FramepaC_initial_indent += 3 ;
   out << frstack_intro << qhead ;
   FramepaC_initial_indent = orig_indent ;
   return out ;
}

//----------------------------------------------------------------------

size_t FrStack::displayLength() const
{
   return qhead->displayLength() + sizeof(frstack_intro)-1 ;
}

//----------------------------------------------------------------------

char *FrStack::displayValue(char *buffer) const
{
   memcpy(buffer,frstack_intro,sizeof(frstack_intro)) ;
   buffer += sizeof(frstack_intro)-1 ;
   return qhead->displayValue(buffer) ;
}

//----------------------------------------------------------------------

static FrObject *read_Stack(istream &input, const char *)
{
   FrObject *result = read_FrObject(input) ;
   if (result && result->consp())
      return new FrStack((FrList*)result) ;
   else
      return new FrStack(0) ;
}

//----------------------------------------------------------------------

static FrObject *string_to_Stack(const char *&input, const char *)
{
   FrObject *result = string_to_FrObject(input) ;
   if (result && result->consp())
      return new FrStack((FrList*)result) ;
   else
      return new FrStack(0) ;
}


//----------------------------------------------------------------------

static bool verify_Stack(const char *&input, const char *, bool strict)
{
   return valid_FrObject_string(input,strict) ;
}

// end of file frstack.cpp //
