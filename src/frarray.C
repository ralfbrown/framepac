/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frarray.cpp	    class FrArray				*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1996,1997,1998,1999,2001,2004,2009,2011,2015		*/
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
#  pragma implementation "frarray.h"
#endif

#include <stdlib.h>
#include "framerr.h"
#include "frarray.h"
#include "frcmove.h"
#include "frreader.h"
#include "frpcglbl.h"

#ifdef FrSTRICT_CPLUSPLUS
#  include <iomanip>
#else
#  include <iomanip.h>
#endif

/**********************************************************************/
/*    Global variables local to this module			      */
/**********************************************************************/

static FrObject *read_FrArray(istream &input, const char *) ;
static FrObject *string_to_FrArray(const char *&input, const char *) ;
static bool verify_FrArray(const char *&input, const char *, bool strict) ;

static FrReader FrArray_reader(string_to_FrArray, read_FrArray, verify_FrArray,
			       FrREADER_LEADIN_LISPFORM,"A") ;

static FrObject *read_vector(istream &input, const char *) ;
static FrObject *string_to_vector(const char *&input, const char *) ;
static bool verify_vector(const char *&input, const char *, bool strict) ;

static FrReader vector_reader(string_to_vector, read_vector, verify_vector,
			      FrREADER_LEADIN_LISPFORM,"") ;

static FrObject *dummy_element = nullptr ;

static const char errmsg_bounds[]
      = "array access out of bounds" ;

/**********************************************************************/
/*    Global variables which are visible from other modules	      */
/**********************************************************************/

FrAllocator FrArray::allocator("FrArray",sizeof(FrArray)) ;

bool FrArray::range_errors = false ;

/**********************************************************************/
/*    Helper functions						      */
/**********************************************************************/

#define update_arrayhash FramepaC_update_ulong_hash

/**********************************************************************/
/*    Member functions for class FrArray			      */
/**********************************************************************/

FrArray::FrArray(size_t size, const FrList *init)
{
   m_array = FrNewN(FrObject*,size) ;
   if (m_array)
      {
      m_length = size ;
      // copy the initializer, if any, into the array
      if (init)
	 {
	 if (init->consp())
	    {
	    const FrList *next ;
	    for (size_t i = 0 ; i < size ; i++, init = next)
	       {
	       next = init->rest() ;
	       FrObject *item = init->first() ;
	       if (!next)
		  next = init ;		// repeat last item ad nauseam
	       m_array[i] = item ? item->deepcopy() : 0 ;
	       }
	    }
	 else
	    {
	    for (size_t i = 0 ; i < size ; i++)
	       m_array[i] = init->deepcopy() ;
	    }
	 }
      else
	 {
	 for (size_t i = 0 ; i < size ; i++)
	    {
	    m_array[i] = nullptr ;
	    }
	 }
      }
   else
      m_length = 0 ;
   return ;
}

//----------------------------------------------------------------------

FrArray::FrArray(size_t size, const FrList *init, bool copyitems)
{
   m_array = FrNewN(FrObject*,size) ;
   if (m_array)
      {
      m_length = size ;
      // copy the initializer, if any, into the array
      if (init)
	 {
	 if (init->consp())
	    {
	    const FrList *next ;
	    for (size_t i = 0 ; i < size ; i++, init = next)
	       {
	       FrObject *item = init->first() ;
	       next = init->rest() ;
	       m_array[i] = (item && copyitems) ? item->deepcopy() : item ;
	       if (!next)
		  {
		  next = init ;		// repeat last item ad nauseam
		  copyitems = true ;
		  }
	       }
	    }
	 else
	    {
	    for (size_t i = 0 ; i < size ; i++)
	       m_array[i] = init->deepcopy() ;
	    }
	 }
      else
	 {
	 for (size_t i = 0 ; i < size ; i++)
	    {
	    m_array[i] = nullptr ;
	    }
	 }
      }
   else
      m_length = 0 ;
   return ;
}

//----------------------------------------------------------------------

FrArray::FrArray(size_t size, const FrObject **init, bool copyitems)
{
   m_array = FrNewN(FrObject*,size) ;
   if (m_array)
      {
      m_length = size ;
      if (copyitems)
	 {
	 for (size_t i = 0 ; i < size ; i++)
	    {
	    m_array[i] = init[i] ? init[i]->deepcopy() : nullptr ;
	    }
	 }
      else
	 {
	 for (size_t i = 0 ; i < size ; i++)
	    {
	    m_array[i] = (FrObject*)init[i] ;
	    }
	 }
      }
   else
      m_length = 0 ;
   return ;
}

//----------------------------------------------------------------------

FrArray::FrArray()
{
   m_length = 0 ;
   m_array = nullptr ;
   return ;
}

//----------------------------------------------------------------------

FrArray::~FrArray()
{
   if (m_array)
      {
      for (size_t pos = 0 ; pos < arrayLength() ; pos++)
	 free_object(m_array[pos]) ;
      FrFree(m_array) ;
      }
   return ;
}

//----------------------------------------------------------------------

const char *FrArray::objTypeName() const
{
   return "FrArray" ;
}

//----------------------------------------------------------------------

FrObjectType FrArray::objType() const
{
   return OT_FrArray ;
}

//----------------------------------------------------------------------

FrObjectType FrArray::objSuperclass() const
{
   return OT_FrObject ;
}

//----------------------------------------------------------------------

bool FrArray::arrayp() const
{
   return true ;
}

//----------------------------------------------------------------------

unsigned long FrArray::hashValue() const
{
   // the hash value of an array is a combination of the hash values of all
   // its elements
   unsigned long hash = 0 ;
   if (arrayLength() > 0)
      {
      for (size_t pos = 0 ; pos < arrayLength() ; pos++)
	 {
	 FrObject *elt = m_array[pos] ;
	 hash = update_arrayhash(hash,elt ? elt->hashValue() : 0) ;
	 }
      }
   return hash ;
}

//----------------------------------------------------------------------

void FrArray::freeObject()
{
   delete this ;
   return ;
}

//----------------------------------------------------------------------

size_t FrArray::length() const
{
   return arrayLength() ;
}

//----------------------------------------------------------------------

FrObject *FrArray::car() const
{
   return (arrayLength() > 0) ? m_array[0] : 0 ;
}

//----------------------------------------------------------------------

FrObject *FrArray::reverse()
{
   if (arrayLength() > 1)
      {
      for (size_t pos = 0 ; pos < arrayLength()/2 ; pos++)
	 {
	 FrObject *tmp = m_array[pos] ;
	 m_array[pos] = m_array[arrayLength()-pos-1] ;
	 m_array[arrayLength()-pos-1] = tmp ;
	 }
      }
   return this ;
}

//----------------------------------------------------------------------

FrObject *FrArray::getNth(size_t N) const
{
   if (N < arrayLength())
      return m_array[N] ;
   else if (range_errors)
      FrWarning(errmsg_bounds) ;
   return 0 ;
}

//----------------------------------------------------------------------

FrObject *&FrArray::operator [] (size_t N)
{
   if (N < arrayLength())
      return m_array[N] ;
   else if (range_errors)
      FrWarning(errmsg_bounds) ;
   return dummy_element ;
}

//----------------------------------------------------------------------

const FrObject *&FrArray::operator [] (size_t N) const
{
   if (N < arrayLength())
      return *(const FrObject**)&m_array[N] ;
   else if (range_errors)
      FrWarning(errmsg_bounds) ;
   return *(const FrObject**)&dummy_element ;
}

//----------------------------------------------------------------------

bool FrArray::setNth(size_t N, const FrObject *elt)
{
   if (N < arrayLength())
      {
      m_array[N] = elt ? elt->deepcopy() : 0 ;
      return true ;
      }
   else if (range_errors)
      FrWarning(errmsg_bounds) ;
   return false ;
}

//----------------------------------------------------------------------

size_t FrArray::locate(const FrObject *item, size_t start) const
{
   if (!m_array)
      return (size_t)-1 ;
   if (start == (size_t)-1)
      start = 0 ;
   else
      start++ ;
   if (item && item->consp())
      {
      for (size_t pos = start ; pos < arrayLength() ; pos++)
	 {
//!!!
	 if (m_array[pos] == ((FrList*)item)->first())
	    return pos ;
	 }
      }
   else
      {
      for (size_t pos = start ; pos < arrayLength() ; pos++)
	 {
	 if (m_array[pos] == item)
	    return pos ;
	 }
      }
   return (size_t)-1 ;
}

//----------------------------------------------------------------------

size_t FrArray::locate(const FrObject *item, FrCompareFunc cmp,
			size_t start) const
{
   if (!m_array)
      return (size_t)-1 ;
   if (start == (size_t)-1)
      start = 0 ;
   else
      start++ ;
   if (item && item->consp())
      {
      for (size_t pos = start ; pos < arrayLength() ; pos++)
	 {
//!!!
	 if (cmp(m_array[pos],((FrList*)item)->first()))
	    return pos ;
	 }
      }
   else
      {
      for (size_t pos = start ; pos < arrayLength() ; pos++)
	 {
	 if (cmp(m_array[pos],item) == 0)
	    return pos ;
	 }
      }
   return (size_t)-1 ;
}

//----------------------------------------------------------------------

bool FrArray::expand(size_t increment)
{
   size_t newsize = arrayLength() + increment ;
   if (newsize < arrayLength())		// did we somehow wrap around int?
      return false ;			//    if yes, can't expand
   FrObject **newarr = FrNewR(FrObject*,m_array,newsize) ;
   if (!newarr)
      {
      FrNoMemory("while expanding FrArray") ;
      return false ;
      }
   m_array = newarr ;
   for (size_t pos = arrayLength() ; pos < newsize ; pos++)
      {
      m_array[pos] = nullptr ;
      }
   m_length = newsize ;
   return true ;
}

//----------------------------------------------------------------------

bool FrArray::expandTo(size_t newsize)
{
   if (arrayLength() < newsize)
      return expand(newsize-arrayLength()) ;
   return true ;			// trivially successful
}

//----------------------------------------------------------------------

FrObject *FrArray::insert(const FrObject *item, size_t pos, bool copyitem)
{
   if (pos > arrayLength() || !m_array)
      {
      if (range_errors)
	 FrWarning(errmsg_bounds) ;
      return this ;			// can't insert there!
      }
   size_t numitems ;
   if (item && item->consp())
      {
      numitems = ((FrList*)item)->listlength() ;
      if (numitems == 1)
	 item = ((FrList*)item)->first() ;
      }
   else
      numitems = 1 ;
   expand(numitems) ;
   // make room for the new items
   if (arrayLength() > 0)
      for (size_t i = arrayLength()-1 ; i >= pos+numitems  ; i--)
	 m_array[i] = m_array[i-numitems] ;
   // and now copy them into the hole we just made
   if (numitems == 1)
      m_array[pos] = (copyitem && item) ? item->deepcopy() : (FrObject*)item ;
   else
      {
      for (size_t i = pos ; i < pos+numitems ; i++)
	 {
	 m_array[i] = ((FrList*)item)->first() ;
	 item = ((FrList*)item)->rest() ;
	 }
      }
   return this ;
}

//----------------------------------------------------------------------

FrObject *FrArray::elide(size_t start, size_t end)
{
   if (start >= arrayLength())
      {
      if (range_errors)
	 FrWarning(errmsg_bounds) ;
      return this ;			// can't elide that!
      }
   if (end < start)
      end = start ;
   else if (end >= arrayLength())
      end = arrayLength()-1 ;
   size_t elide_size = end-start+1 ;
   // remove the requested section of the array
   for (size_t i = start ; i <= end ; i++)
      if (m_array[i])
	 m_array[i]->freeObject() ;
   for (size_t pos = start ; pos < arrayLength()-elide_size ; pos++)
      m_array[pos] = m_array[pos+elide_size] ;
   // resize our storage downward
   m_length -= elide_size ;
   FrObject **newarray = FrNewR(FrObject*,m_array,arrayLength()) ;
   if (newarray || arrayLength() == 0)
      m_array = newarray ;
   return this ;
}

//----------------------------------------------------------------------

FrObject *FrArray::removeDuplicates() const
{
   FrArray *result = new FrArray(arrayLength()) ;
   if (result)
      {
      if (arrayLength() > 0 && m_array)
	 {
	 size_t i ;
	 size_t j = 0 ;
	 for (i = 0 ; i < arrayLength() ; i++)
	    {
	    FrObject *obj = m_array[i] ;
	    if (result->locate(obj) != (size_t)-1)
	       (*result)[j++] = obj ? obj->deepcopy() : 0 ;
	    }
	 result->elide(j,arrayLength()) ;
	 }
      }
   else
      FrNoMemory("in FrArray::removeDuplicates") ;
   return result ;
}

//----------------------------------------------------------------------

FrObject *FrArray::removeDuplicates(FrCompareFunc cmp) const
{
   FrArray *result = new FrArray(arrayLength()) ;
   if (result)
      {
      if (arrayLength() > 0 && m_array)
	 {
	 size_t i ;
	 size_t j = 0 ;
	 for (i = 0 ; i < arrayLength() ; i++)
	    {
	    FrObject *obj = m_array[i] ;
	    if (result->locate(obj,cmp) != (size_t)-1)
	       (*result)[j++] = obj ? obj->deepcopy() : 0 ;
	    }
	 result->elide(j,arrayLength()) ;
	 }
      }
   else
      FrNoMemory("in FrArray::removeDuplicates") ;
   return result ;
}

//----------------------------------------------------------------------

FrObject *FrArray::subseq(size_t start, size_t stop) const
{
   if (stop < start)
      stop = start ;
   if (stop >= arrayLength())
      stop = arrayLength() - 1 ;
   if (m_array && start < arrayLength() && start <= stop)
      return new FrArray(stop-start+1,(const FrObject**)m_array+start,true) ;
   else
      {
      if (range_errors)
	 FrWarning(errmsg_bounds) ;
      return 0 ;			// can't take that subsequence!
      }
}

//----------------------------------------------------------------------

bool FrArray::equal(const FrObject *obj) const
{
   if (obj == this)
      return true ;
   else if (!obj || !obj->arrayp())
      return false ;			// can't be equal to non-array
   else
      {
      FrArray *arr = (FrArray*)obj ;
      if (arrayLength() != arr->arrayLength())
	 return false ;
      for (size_t i = 0 ; i < arrayLength() ; i++)
	 {
	 if (!equal_inline(m_array[i],arr->m_array[i]))
	    return false ;
	 }
      return true ;
      }
}

//----------------------------------------------------------------------

int FrArray::compare(const FrObject *obj) const
{
   if (!obj)
      return 1 ;     // anything is greater than NIL / empty-list
   else if (obj->arrayp())
      {
      FrArray *arr = (FrArray*)obj ;
      size_t min_length = FrMin(arr->arrayLength(),arrayLength()) ;
      for (size_t i = 0 ; i < min_length ; i++)
	 {
	 int cmp ;
	 if (m_array[i])
	    cmp = m_array[i]->compare(arr->m_array[i]) ;
	 else if (arr->m_array[i])
	    cmp = -(arr->m_array[i]->compare(m_array[i])) ;
	 else
	    continue ;
	 if (cmp)
	    return cmp ;
	 }
      if (arrayLength() == arr->arrayLength())
	 return 0 ;
      else
	 return arrayLength() > arr->arrayLength() ? 1 : -1 ; // longer array comes later
      }
   else
      return -1 ;			// non-arrays come after arrays
}

//----------------------------------------------------------------------

bool FrArray::iterateVA(FrIteratorFunc func, va_list args) const
{
   bool success = true ;
   for (size_t pos = 0 ; pos < arrayLength() && success ; pos++)
      {
      FrSafeVAList(args) ;
      if (!func(m_array[pos],FrSafeVarArgs(args)))
	 success = false ;
      FrSafeVAListEnd(args) ;	
      }
   return success ;
}

//----------------------------------------------------------------------

FrObject *FrArray::copy() const
{
   return new FrArray(arrayLength(),(const FrObject**)m_array,false) ;
}

//----------------------------------------------------------------------

FrObject *FrArray::deepcopy() const
{
   return new FrArray(arrayLength(),(const FrObject**)m_array,true) ;
}

//----------------------------------------------------------------------

ostream &FrArray::printValue(ostream &output) const
{
   output << "#A(" ;
   size_t loc = FramepaC_initial_indent + 3 ;
   size_t orig_indent = FramepaC_initial_indent ;
   for (size_t pos = 0 ; pos < arrayLength() ; pos++)
      {
      FramepaC_initial_indent = loc ;
      size_t len = m_array[pos] ? m_array[pos]->displayLength()+1 : 3 ;
      loc += len ;
      if (loc > FramepaC_display_width && pos != 0)
	 {
	 loc = orig_indent+3 ;
	 output << '\n' << setw(loc) << " " ;
	 FramepaC_initial_indent = loc ;
	 loc += len ;
	 }
      if (m_array[pos])
	 m_array[pos]->printValue(output) ;
      else
	 output << "()" ;
      if (pos < arrayLength()-1)
	 output << ' ' ;
      }
   FramepaC_initial_indent = orig_indent ;
   return output << ")" ;
}

//----------------------------------------------------------------------

char *FrArray::displayValue(char *buffer) const
{
   memcpy(buffer,"#A(",3) ;
   buffer += 3 ;
   for (size_t pos = 0 ; pos < arrayLength() ; pos++)
      {
      if (m_array[pos])
	 buffer = m_array[pos]->displayValue(buffer) ;
      else
	 {
	 *buffer++ = '(' ;
	 *buffer++ = ')' ;
	 }
      if (pos < arrayLength()-1)
	 *buffer++ = ' ' ;		// separate the elements
      }
   *buffer++ = ')' ;
   *buffer = '\0' ;			// ensure proper string termination
   return buffer ;
}

//----------------------------------------------------------------------

size_t FrArray::displayLength() const
{
   size_t len = 4 ;			// for the #A()
   if (arrayLength() > 1)
      len += (arrayLength()-1) ;		// also count the blanks we add
   for (size_t pos = 0 ; pos < arrayLength() ; pos++)
      {
      if (m_array[pos])
	 len += m_array[pos]->displayLength() ;
      else
	 len += 2 ;
      }
   return len ;
}

/************************************************************************/
/************************************************************************/

static FrObject *read_FrArray(istream &input, const char *)
{
   FrObject *initializer = read_FrObject(input) ;
   if (initializer)
      {
      FrArray *array ;
      if (initializer->consp())
	 array = new FrArray(((FrList*)initializer)->listlength(),
			      (FrList*)initializer,false) ;
      else
	 {
	 initializer = new FrList(initializer) ;
	 array = new FrArray(1,(FrList*)initializer,false) ;
	 }
      ((FrList*)initializer)->eraseList(false) ;
      return array ;
      }
   else
      return new FrArray(0) ;
}

//----------------------------------------------------------------------

static FrObject *read_vector(istream &input, const char *digits)
{
   bool widechar = read_extended_strings(false) ; // use Lisp compatibility
   FrObject *initializer = read_FrObject(input) ;
   read_extended_strings(widechar) ;
   FrArray *array ;
   if (digits && *digits)
      {
      size_t len = atoi(digits) ;
      array = new FrArray(len,(FrList*)initializer) ;
      free_object(initializer) ;
      }
   else if (initializer)
      {
      if (initializer->consp())
	 {
	 array = new FrArray(((FrList*)initializer)->listlength(),
			      (FrList*)initializer,false) ;
	 }
      else
	 {
	 initializer = new FrList(initializer) ;
	 array = new FrArray(1,(FrList*)initializer,false) ;
	 }
      ((FrList*)initializer)->eraseList(false) ;
      }
   else
      array = new FrArray(0) ;
   return array ;
}

//----------------------------------------------------------------------

static FrObject *string_to_FrArray(const char *&input, const char *)
{
   FrObject *initializer = string_to_FrObject(input) ;
   if (initializer)
      {
      FrArray *array ;
      if (initializer->consp())
	 array = new FrArray(((FrList*)initializer)->listlength(),
			      (FrList*)initializer,false) ;
      else
	 {
	 initializer = new FrList(initializer) ;
	 array = new FrArray(1,(FrList*)initializer,false) ;
	 }
      ((FrList*)initializer)->eraseList(false) ;
      return array ;
      }
   else
      return new FrArray(0) ;
}

//----------------------------------------------------------------------

static bool verify_FrArray(const char *&input, const char *, bool)
{
   return valid_FrObject_string(input,true) ;
}

//----------------------------------------------------------------------

static FrObject *string_to_vector(const char *&input, const char *digits)
{
   bool widechar = read_extended_strings(false) ; // use Lisp compatibility
   FrObject *initializer = string_to_FrObject(input) ;
   read_extended_strings(widechar) ;
   FrArray *array ;
   if (digits && *digits)
      {
      size_t len = atoi(digits) ;
      array = new FrArray(len,(FrList*)initializer) ;
      free_object(initializer) ;
      }
   else if (initializer)
      {
      if (initializer->consp())
	 {
	 array = new FrArray(((FrList*)initializer)->listlength(),
			      (FrList*)initializer,false) ;
	 }
      else
	 {
	 initializer = new FrList(initializer) ;
	 array = new FrArray(1,(FrList*)initializer,false) ;
	 }
      ((FrList*)initializer)->eraseList(false) ;
      }
   else
      array = new FrArray(0) ;
   return array ;

}

//----------------------------------------------------------------------

static bool verify_vector(const char *&input, const char *, bool)
{
   bool widechar = read_extended_strings(false) ; // use Lisp compatibility
   bool valid = valid_FrObject_string(input,true) ;
   read_extended_strings(widechar) ;
   return valid ;
}

//----------------------------------------------------------------------

// end of file frarray.cpp //
