/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frobject.cpp	    class FrObject				*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,1998,2001,2003,2004,2006,2009,	*/
/*		2015 Ralf Brown/Carnegie Mellon University		*/
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
#  pragma implementation "frobject.h"
#endif

#include "framerr.h"
#include "frobject.h"
#include "frpcglbl.h"
#include "frutil.h"

#ifdef FrSTRICT_CPLUSPLUS
#  include <cstdlib>
using namespace std ;
#else
#  include <iostream.h>
#  include <stdlib.h>
#endif /* FrSTRICT_CPLUSPLUS */

#if defined(unix) || defined(__linux__) || defined(__GNUC__)
#include <sys/time.h>
#include <sys/resource.h>
#endif

/**********************************************************************/
/*    Global variables local to this module			      */
/**********************************************************************/

static const char stringNIL[] = "NIL" ;

/**********************************************************************/
/*    Global variables which are visible from other modules	      */
/**********************************************************************/

FrObject NIL ;

bool FramepaC_typesafe_virtual = true ;

#ifdef FrSYMBOL_VALUE
FrObject UNBOUND ;
#endif

FramepaC_bgproc_funcptr FramepaC_bgproc_func ;

size_t FramepaC_display_width = 79 ;
size_t FramepaC_initial_indent = 0 ;

/**********************************************************************/
/*    Global data limited to this module			      */
/**********************************************************************/

static const char errmsg_nomem_in_display[] =
   "in FrObject::print()" ;

/**********************************************************************/
/*    Utility Functions						      */
/**********************************************************************/

#ifdef FrNEED_ULTOA
char *ultoa(unsigned long value,char *buffer,int radix)
{
   register char *s ;

   if (buffer)
      {
      s = buffer ;
      do {
	 *s++ = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"[(int)(value%radix)] ;
	 value /= radix ;
	 } while (value != 0) ;
      *s-- = '\0' ;
      for (char *tmp = buffer ; tmp < s ; tmp++, s--)
	 {
	 char c = *s ;
	
	 *s = *tmp ;
	 *tmp = c ;
	 }
      }
   return buffer ;
}
#endif /* FrNEED_ULTOA */

//----------------------------------------------------------------------

size_t Fr_number_length(unsigned long number)
{
   char num[FrMAX_ULONG_STRING] ;

   ultoa(number,num,10) ;
   return strlen(num) ;
}

/**********************************************************************/
/*    Member functions for class FrObject			      */
/**********************************************************************/

#if 0
FrObject::~FrObject()
{
   // don't need to do anything, only want override to use proper 'delete'
   // Note: this slows things down, so left out in the interest of maximum
   //   speed.  Use freeObject() instead of delete to type-safe deletion.
}
#endif

//----------------------------------------------------------------------

const char *FrObject::objTypeName() const
{
   return "FrObject" ;
}

//----------------------------------------------------------------------

FrObjectType FrObject::objType() const
{
   return OT_FrObject ;
}

//----------------------------------------------------------------------

FrObjectType FrObject::objSuperclass() const
{
   return OT_FrObject ;
}

//----------------------------------------------------------------------

bool FrObject::atomp() const
{
   return false ;
}

//----------------------------------------------------------------------

bool FrObject::consp() const
{
   return false ;
}

//----------------------------------------------------------------------

bool FrObject::symbolp() const
{
   return false ;
}

//----------------------------------------------------------------------

bool FrObject::numberp() const
{
   return false ;
}

//----------------------------------------------------------------------

bool FrObject::floatp() const
{
   return false ;
}

//----------------------------------------------------------------------

bool FrObject::stringp() const
{
   return false ;
}

//----------------------------------------------------------------------

bool FrObject::framep() const
{
   return false ;
}

//----------------------------------------------------------------------

bool FrObject::structp() const
{
   return false ;
}

//----------------------------------------------------------------------

bool FrObject::hashp() const
{
   return false ;
}

//----------------------------------------------------------------------

bool FrObject::queuep() const
{
   return false ;
}

//----------------------------------------------------------------------

bool FrObject::arrayp() const
{
   return false ;
}

//----------------------------------------------------------------------

bool FrObject::widgetp() const
{
   return false ;
}

//----------------------------------------------------------------------

bool FrObject::vectorp() const
{
   return false ;
}

//----------------------------------------------------------------------

const char *FrObject::printableName() const
{
   return 0 ;
}

//----------------------------------------------------------------------

FrSymbol *FrObject::coerce2symbol(FrCharEncoding) const
{
   return 0 ;
}

//----------------------------------------------------------------------

unsigned long FrObject::hashValue() const
{
   return (unsigned long)this ;
}

//----------------------------------------------------------------------

void FrObject::freeObject()
{
   delete this ;
}

//----------------------------------------------------------------------

size_t FrObject::length() const
{
   return (size_t)-1 ;   // function not applicable to this object
}

//----------------------------------------------------------------------

FrObject *FrObject::car() const
{
   // only FrCons and subclasses have car and cdr, so the default behavior
   // is to return NIL
   return 0 ;
}

//----------------------------------------------------------------------

FrObject *FrObject::cdr() const
{
   // only FrCons and subclasses have car and cdr, so the default behavior
   // is to return NIL
   return 0 ;
}

//----------------------------------------------------------------------

FrObject *FrObject::reverse()
{
   // for any classes which don't support reversing, return the object itself
   return this ;
}

//----------------------------------------------------------------------

FrObject *FrObject::getNth(size_t) const
{
   // for any non-compound classes, return 0
   if (FramepaC_typesafe_virtual)
      FrInvalidVirtualFunction("FrObject::getNth") ;
   return 0 ;
}

//----------------------------------------------------------------------

bool FrObject::setNth(size_t, const FrObject *)
{
   // non-compound objects are never updated, so indicate failure to update
   if (FramepaC_typesafe_virtual)
      FrInvalidVirtualFunction("FrObject::setNth") ;
   return false ;
}

//----------------------------------------------------------------------

size_t FrObject::locate(const FrObject *, size_t /* start */) const
{
   // non-compound objects can never locate any component object
   if (FramepaC_typesafe_virtual)
      FrInvalidVirtualFunction("FrObject::locate") ;
   return (size_t)-1 ;
}

//----------------------------------------------------------------------

size_t FrObject::locate(const FrObject *, FrCompareFunc, size_t) const
{
   // non-compound objects can never locate any component object
   if (FramepaC_typesafe_virtual)
      FrInvalidVirtualFunction("FrObject::locate") ;
   return (size_t)-1 ;
}

//----------------------------------------------------------------------

FrObject *FrObject::insert(const FrObject *, size_t, bool)
{
   // non-compound objects can never have anything inserted, so return
   // the unchanged object
   if (FramepaC_typesafe_virtual)
      FrInvalidVirtualFunction("FrObject::insert") ;
   return this ;
}

//----------------------------------------------------------------------

FrObject *FrObject::elide(size_t, size_t)
{
   // non-compound objects can never have anything elided, so return
   // the unchanged object
   if (FramepaC_typesafe_virtual)
      FrInvalidVirtualFunction("FrObject::elide") ;
   return this ;
}

//----------------------------------------------------------------------

FrObject *FrObject::removeDuplicates() const
{
   // non-compound objects can never have anything duplicated, so return
   // the unchanged object
   if (FramepaC_typesafe_virtual)
      FrInvalidVirtualFunction("FrObject::removeDuplicates") ;
   return this->deepcopy() ;
}

//----------------------------------------------------------------------

FrObject *FrObject::removeDuplicates(FrCompareFunc) const
{
   // non-compound objects can never have anything duplicated, so return
   // the unchanged object
   if (FramepaC_typesafe_virtual)
      FrInvalidVirtualFunction("FrObject::removeDuplicates") ;
   return this->deepcopy() ;
}

//----------------------------------------------------------------------

bool FrObject::expand(size_t)
{
   // non-compound objects can never be expanded, so the expansion fails
   if (FramepaC_typesafe_virtual)
      FrInvalidVirtualFunction("FrObject::expand") ;
   return false ;
}

//----------------------------------------------------------------------

bool FrObject::expandTo(size_t)
{
   // non-compound objects can never be expanded, so the expansion fails
   if (FramepaC_typesafe_virtual)
      FrInvalidVirtualFunction("FrObject::expandTo") ;
   return false ;
}

//----------------------------------------------------------------------

FrObject *FrObject::subseq(size_t /*start*/, size_t /*stop*/) const
{
   // non-compound objects have no sequence from which to take a part
   if (FramepaC_typesafe_virtual)
      FrInvalidVirtualFunction("FrObject::subseq") ;
   return 0 ;
}

//----------------------------------------------------------------------

long int FrObject::intValue() const
{
   if (FramepaC_typesafe_virtual)
      FrInvalidVirtualFunction("FrObject::intValue") ;
   return 0L ;
}

//----------------------------------------------------------------------

double FrObject::floatValue() const
{
   if (FramepaC_typesafe_virtual)
      FrInvalidVirtualFunction("FrObject::floatValue") ;
   return 0.0 ;
}

//----------------------------------------------------------------------

double FrObject::real() const
{
   if (FramepaC_typesafe_virtual)
      FrInvalidVirtualFunction("FrObject::real") ;
   return 0.0 ;
}

//----------------------------------------------------------------------

double FrObject::imag() const
{
   if (FramepaC_typesafe_virtual)
      FrInvalidVirtualFunction("FrObject::fraction") ;
   return 0.0 ;
}

//----------------------------------------------------------------------

double FrObject::fraction() const
{
   if (FramepaC_typesafe_virtual)
      FrInvalidVirtualFunction("FrObject::fraction") ;
   return 0.0 ;
}

//----------------------------------------------------------------------

bool FrObject::equal(const FrObject *obj) const
{
   // the default 'equal' is a simple identity, that is, both objects
   // must be one and the same, checked by comparing pointers.
   return (this == obj) ;
}

//----------------------------------------------------------------------

int FrObject::compare(const FrObject *obj) const
{
   if (!obj)
      return 1 ;     // anything is greater than NIL / empty-list
   if (obj->objType() != OT_FrObject)
      return -(obj->compare(this)) ;
   else if ((long int)this < (long int)obj)
      return -1 ;
   else if ((long int)this > (long int)obj)
      return 1 ;
   else
      return 0 ;
}

//----------------------------------------------------------------------

bool FrObject::iterateVA(FrIteratorFunc, va_list) const
{
   return true ;  // nothing to iterate, so all iterations were successful
}

//----------------------------------------------------------------------

bool __FrCDECL FrObject::iterate(FrIteratorFunc func, ...) const
{
   va_list args ;
   va_start(args,func) ;
   bool result = this ? iterateVA(func,args) : true ;
   va_end(args) ;
   return result ;
}

//----------------------------------------------------------------------

size_t FrObject_string_length(const FrObject *obj)
{
   if (obj)
      return obj->displayLength() ;
   else
      return 2 ;
}

//----------------------------------------------------------------------

FrObject *FrObject::copy() const
{
   return (FrObject *)this ;
}

//----------------------------------------------------------------------

FrObject *FrObject::deepcopy() const
{
   return (FrObject *)this ;
}

//----------------------------------------------------------------------

ostream &FrObject::printValue(ostream &output) const
{
   if (this == &NIL)
      return output << stringNIL ;
   else
      return output << objTypeName() << "<" << (uintptr_t)this << ">" ;
}

//----------------------------------------------------------------------

size_t FrObject::displayLength() const
{
   if (this == &NIL)
      return 3 ;
   else
      return strlen(objTypeName()) + Fr_number_length((uintptr_t)this) + 2 ;
}

//----------------------------------------------------------------------

char *FrObject::displayValue(char *buffer) const
{
   if (this == &NIL)
      {
      memcpy(buffer,stringNIL,4) ;
      return buffer+3 ;
      }
   else
      {
      const char *type_name = objTypeName() ;
      size_t len = strlen(type_name) ;
      memcpy(buffer,type_name,len) ;
      buffer += len ;
      *buffer++ = '<' ;
      ultoa((unsigned long)this,buffer,10) ;
      buffer = strchr(buffer,'\0') ;
      memcpy(buffer,">",2) ;
      return buffer+1 ;
      }
}

//----------------------------------------------------------------------

char *FrObject::print(char *buffer) const
{
   if (this)
      return displayValue(buffer) ;
   else
      {
      memcpy(buffer,"()",3) ;
      return buffer+2 ;
      }
}

//----------------------------------------------------------------------

char *FrObject::print() const
{
   if (this)
      {
      char *result = FrNewN(char,displayLength()+1) ;
      if (result)
	 {
	 (void)displayValue(result) ;
	 return result ;
	 }
      else
	 FrNoMemory(errmsg_nomem_in_display) ;
      }
   else
      {
      return FrDupString("()") ;
      }
   return 0 ;
}

//----------------------------------------------------------------------

ostream &FrObject::print(ostream &out) const
{
   if (this)
      printValue(out) ;
   else
      out << "()" ;
   return out ;
}

//----------------------------------------------------------------------

size_t FrObject::printSize() const
{
   if (this)
      return displayLength() ;
   else
      return 2 ;
}

//----------------------------------------------------------------------

istream &operator >> (istream &input, FrObject *&obj)
{
   FramepaC_bgproc() ;
   obj = read_FrObject(input) ;
   return input ;
}

//----------------------------------------------------------------------

void FrObject::_() const
{
   print(cerr) ;
   cerr << endl ;
}

/**********************************************************************/
/*	 overloaded operators (non-member functions)		      */
/**********************************************************************/

ostream &operator << (ostream &output,const FrObject *object)
{
   if (object)
      return object->printValue(output) ;
   else
      return output << "()" ;
}

/************************************************************************/
/*    Member functions for class FrAtom					*/
/************************************************************************/

const char *FrAtom::objTypeName() const
{
   return "FrAtom" ;
}

//----------------------------------------------------------------------

FrObjectType FrAtom::objType() const
{
   return OT_FrAtom ;
}

//----------------------------------------------------------------------

FrObjectType FrAtom::objSuperclass() const
{
   return OT_FrObject ;
}

//----------------------------------------------------------------------

bool FrAtom::atomp() const
{
   return true ;
}

/**********************************************************************/
/*	 procedural interface					      */
/**********************************************************************/

void _(const FrObject*obj) { cout << obj << endl ; }

//----------------------------------------------------------------------

size_t FrDisplayWidth(size_t new_width)
{
   size_t old_width = FramepaC_display_width ;
   if (new_width > 1)
      FramepaC_display_width = new_width ;
   return old_width ;
}

// end of file frobject.cpp //
