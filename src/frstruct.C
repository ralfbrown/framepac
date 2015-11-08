/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frstruct.cpp	class FrStruct (Lisp-style structure)		*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,1998,2001,2006,2007,2008,2009	*/
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
#  pragma implementation "frstruct.h"
#endif

#include "frstruct.h"
#include "frpcglbl.h"

#ifdef FrSTRICT_CPLUSPLUS
#  include <iomanip>
#else
#  include <iomanip.h>
#endif

/************************************************************************/
/*    Global data for this module					*/
/************************************************************************/

static const char stringHASH_S[] = "#S(" ;
static const char str_FrStruct[] = "FrStruct" ;

/************************************************************************/
/*    Global variables for class FrStructField				*/
/************************************************************************/

FrAllocator FrStructField::allocator("StructField",sizeof(FrStructField)) ;

/************************************************************************/
/*    Global variables for class FrStruct				*/
/************************************************************************/

FrAllocator FrStruct::allocator(str_FrStruct,sizeof(FrStruct)) ;

/************************************************************************/
/*    Methods for class FrStructField					*/
/************************************************************************/

FrStructField::~FrStructField()
{
   free_object((FrObject*)m_value) ;
   return ;
}

//----------------------------------------------------------------------

FrStructField *FrStructField::copy() const
{
   return new FrStructField(name(),m_value) ;
}

//----------------------------------------------------------------------

FrStructField *FrStructField::deepcopy() const
{
   return new FrStructField(name(),m_value ? m_value->deepcopy() : 0) ;
}

/************************************************************************/
/*    Methods for class FrStruct					*/
/************************************************************************/

FrStruct::FrStruct(const FrSymbol *type)
{
   m_typename = type ;
   m_fields = 0 ;
   return ;
}

FrStruct::~FrStruct()
{
   while (m_fields)
      {
      FrStructField *f = m_fields ;
      m_fields = m_fields->next() ;
      delete f ;
      }
   return ;
}

//----------------------------------------------------------------------

FrObjectType FrStruct::objType() const
{
   return OT_FrStruct ;
}

//----------------------------------------------------------------------

const char *FrStruct::objTypeName() const
{
   return str_FrStruct ;
}

//----------------------------------------------------------------------

FrObjectType FrStruct::objSuperclass() const
{
   return OT_FrStruct ;
}

//----------------------------------------------------------------------

bool FrStruct::structp() const
{
   return true ;
}

//----------------------------------------------------------------------

void FrStruct::freeObject()
{
   delete this ;
   return ;
}

//----------------------------------------------------------------------

FrObject *FrStruct::copy() const
{
   return deepcopy() ;
}

//----------------------------------------------------------------------

FrObject *FrStruct::deepcopy() const
{
   FrStruct *newstruc = new FrStruct(m_typename) ;
   FrStructField *f = m_fields ;
   FrStructField **n = &newstruc->m_fields ;
   while (f)
      {
      *n = f->deepcopy() ;
      n = &(*n)->m_next ;
      f = f->next() ;
      }
   *n = 0 ;				// properly terminate the list
   return newstruc ;
}

//----------------------------------------------------------------------

bool FrStruct::iterateVA(FrIteratorFunc func, va_list args) const
{
   for (FrStructField *f = m_fields ; f ; f = f->next())
      {
      FrCons *element = new FrCons(f->name(),f->m_value) ;
      FrSafeVAList(args) ;
      bool result = func(element,FrSafeVarArgs(args)) ;
      FrSafeVAListEnd(args) ;
      element->freeObject() ;
      if (!result)
	 return false ;
      }
   return true ;
}

//----------------------------------------------------------------------

FrObject *FrStruct::get(const FrSymbol *fieldname) const
{
   for (FrStructField *f = m_fields ; f ; f = f->next())
      {
      if (f->name() == fieldname)
	 return (FrObject *)f->m_value ;
      }
   return 0 ;  // not found
}

//----------------------------------------------------------------------

FrObject *FrStruct::get(const char *fieldname) const
{
   for (FrStructField *f = m_fields ; f ; f = f->next())
      {
      if (f->name() && strcmp(f->name()->symbolName(),fieldname) == 0)
	 return (FrObject *)f->m_value ;
      }
   return 0 ;  // not found
}

//----------------------------------------------------------------------

FrStructField *FrStruct::getField(const FrSymbol *fieldname)
{
   for (FrStructField *f = m_fields ; f ; f = f->next())
      {
      if (f->name() == fieldname)
	 return f ;
      }
   return 0 ;  // not found
}

//----------------------------------------------------------------------

FrStructField *FrStruct::getField(const char *fieldname)
{
   for (FrStructField *f = m_fields ; f ; f = f->next())
      {
      if (f->name() && strcmp(f->name()->symbolName(),fieldname) == 0)
	 return f ;
      }
   return 0 ;  // not found
}

//----------------------------------------------------------------------

void FrStruct::putnew(const FrSymbol *fieldname, const FrObject *fieldvalue)
{
   FrStructField *f = new FrStructField(fieldname,fieldvalue) ;

   if (m_fields)
      {
      FrStructField *prev = m_fields ;
      while (prev->next())
	 prev = prev->next() ;
      prev->m_next = f ;
      }
   else
      m_fields = f ;
   return ;
}

//----------------------------------------------------------------------

void FrStruct::put(const FrSymbol *fieldname, const FrObject *fieldvalue)
{
   if (fieldvalue)
      fieldvalue = fieldvalue->deepcopy() ;
   FrStructField *f = m_fields ;
   FrStructField *prev = 0 ;
   for ( ; f ; f = f->next())
      {
      if (f->name() == fieldname)
	 {
	 free_object((FrObject*)f->m_value) ;
	 f->m_value = fieldvalue ;
	 return ;
	 }
      prev = f ;
      }
   f = new FrStructField(fieldname,fieldvalue) ;
   if (prev)
      prev->m_next = f ;
   else
      m_fields = f ;
   return ;
}

//----------------------------------------------------------------------

void FrStruct::put(const FrSymbol *fieldname, const FrObject *fieldvalue,
		    bool copy_data)
{
   FrStructField *f = m_fields ;
   FrStructField *prev = 0 ;

   if (fieldvalue && copy_data)
      fieldvalue = fieldvalue->deepcopy() ;
   for ( ; f ; f = f->next())
      {
      if (f->name() == fieldname)
	 {
	 free_object((FrObject*)f->m_value) ;
	 f->m_value = fieldvalue ;
	 return ;
	 }
      prev = f ;
      }
   f = new FrStructField(fieldname,fieldvalue) ;
   if (prev)
      prev->m_next = f ;
   else
      m_fields = f ;
   return ;
}

//----------------------------------------------------------------------

bool FrStruct::remove(const FrSymbol *fieldname)
{
   FrStructField **prev = &m_fields ;
   for (FrStructField *f = m_fields ; f ; f = f->next())
      {
      if (f->name() == fieldname)
	 {
	 *prev = f->next() ;
	 delete f ;
	 return true ;
	 }
      prev = &f->m_next ;
      }
   return false ;
}

//----------------------------------------------------------------------

FrList *FrStruct::fieldNames() const
{
   FrList *names = 0 ;
   for (const FrStructField *f = m_fields ; f ; f = f->next())
      {
      pushlist(f->name(),names) ;
      }
   return names ;
}

//----------------------------------------------------------------------

ostream &FrStruct::printValue(ostream &output) const
{
   output << stringHASH_S << m_typename ;
   size_t column = FramepaC_initial_indent + sizeof(stringHASH_S)-1 ;
   size_t orig_indent = FramepaC_initial_indent ;
   FramepaC_initial_indent = column + 2 ;
   bool first = true ;
   for (const FrStructField *f = m_fields ; f ; f = f->next())
      {
      size_t len = FrObject_string_length(f->name()) +
		   FrObject_string_length(f->m_value) + 3 ;
      column += len ;
      if (column > FramepaC_display_width && !first)
	 {
	 output << '\n' << setw(orig_indent+sizeof(stringHASH_S)-1) << " " ;
	 column = orig_indent + sizeof(stringHASH_S)-1 + len ;
	 }
      output << " :" << f->name() << ' ' << f->m_value ;
      first = false ;
      }
   output << ')' ;
   FramepaC_initial_indent = orig_indent ;
   return output ;
}

//----------------------------------------------------------------------

size_t FrStruct::displayLength() const
{
   int len = 4 + FrObject_string_length(m_typename) ;  // #S(TYPE and )

   for (const FrStructField *f = m_fields ; f ; f = f->next())
      {
      len += FrObject_string_length(f->name())
	     + FrObject_string_length(f->m_value) + 3 ;
      }
   return len ;
}

//----------------------------------------------------------------------

char *FrStruct::displayValue(char *buffer) const
{
   memcpy(buffer,stringHASH_S,3) ;
   buffer += 3 ;
   buffer = m_typename->print(buffer) ;
   for (const FrStructField *f = m_fields ; f ; f = f->next())
      {
      *buffer++ = ' ' ;
      *buffer++ = ':' ;
      buffer = f->name()->print(buffer) ;
      *buffer++ = ' ' ;
      buffer = f->m_value->print(buffer) ;
      }
   *buffer++ = ')' ;
   *buffer = '\0' ;
   return buffer ;
}

// end of file frstruct.cpp //
