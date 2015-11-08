/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frhasht.cpp		class FrHashTableOld 			*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,1998,2000,2001,2003,2004,2009,    */
/*		2011,2015 Ralf Brown/Carnegie Mellon University		*/
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
#  pragma implementation "frhasht.h"
#endif

#include "frcmove.h"
#include "frhasht.h"

#ifdef FrSTRICT_CPLUSPLUS
#  include <cstdlib>
#  include <iomanip>
#else
#  include <iomanip.h>
#  include <stdlib.h>
#endif /* FrSTRICT_CPLUSPLUS */

/************************************************************************/
/*    Global data local to this module					*/
/************************************************************************/

#if 0
static const char str_NIL[] = "NIL" ;
static const char str_hash_intro[] = "#H(" ;
#endif

/************************************************************************/
/*    Methods for class FrHashEntry					*/
/************************************************************************/

FrHashEntry::~FrHashEntry()
{
   if (in_use)
      FrProgError("deleted FrHashEntry which was still in use") ;
   return ;
}

//----------------------------------------------------------------------

bool FrHashEntry::hashp() const
{
   return true ;
}

//----------------------------------------------------------------------

const char *FrHashEntry::objTypeName() const
{
   return "FrHashEntry" ;
}

//----------------------------------------------------------------------

FrObjectType FrHashEntry::objType() const
{
   return OT_FrHashEntry ;
}

//----------------------------------------------------------------------

FrObjectType FrHashEntry::objSuperclass() const
{
   return OT_FrObject ;
}

//----------------------------------------------------------------------

FrHashEntryType FrHashEntry::entryType() const
{
   return HE_base ;
}

//----------------------------------------------------------------------

FrSymbol *FrHashEntry::entryName() const
{
   return 0 ;
}

//----------------------------------------------------------------------

int FrHashEntry::sizeOf() const
{
   return sizeof(FrHashEntry) ;
}

//----------------------------------------------------------------------

size_t FrHashEntry::hashIndex(int size) const
{
   return (size_t)((unsigned long)this % size) ;
}

//----------------------------------------------------------------------

int FrHashEntry::keycmp(const FrHashEntry *entry) const
{
   return FrCompare(this,entry) ;
}

//----------------------------------------------------------------------

FrObject *FrHashEntry::copy() const
{
   FrHashEntry *result = new FrHashEntry ;

   if (result)
      {
      }
   return result ;
}

//----------------------------------------------------------------------

FrObject *FrHashEntry::deepcopy() const
{
   FrHashEntry *result = new FrHashEntry ;

   if (result)
      {
      }
   return result ;
}

/************************************************************************/
/*    Methods for class FrHashTableOld					*/
/************************************************************************/

/************************************************************************/
/*    I/O functions for class FrHashTableOld				*/
/************************************************************************/

#if 0
static bool print_hash_entry(const FrObject *obj, va_list args)
{
   FrVarArg(ostream *,output) ;
   FrVarArg(size_t *,loc) ;
   FrVarArg(bool *,first) ;
   size_t len = obj ? obj->displayLength()+1 : 3 ;
   *loc += len ;
   if (*loc > FramepaC_display_width && !*first)
      {
      (*output) << '\n' << setw(FramepaC_initial_indent) << " " ;
      *loc = FramepaC_initial_indent + len ;
      }
   (*output) << obj << ' ' ;
   *first = false ;
   return true ;
}

//----------------------------------------------------------------------

ostream &FrHashTableOld::printValue(ostream &output) const
{
   if ((FrObject*)this == &NIL)
      return output << str_NIL ;
   else
      {
      output << str_hash_intro ;
      size_t orig_indent = FramepaC_initial_indent ;
      FramepaC_initial_indent += sizeof(str_hash_intro)-1 ;
      size_t loc = FramepaC_initial_indent ;
      bool first = true ;
      iterate(print_hash_entry,&output,&loc,&first) ;
      FramepaC_initial_indent = orig_indent ;
      return output << ')' ;
      }
}

//----------------------------------------------------------------------

static bool display_hash_entry(const FrObject *ent, va_list args)
{
   FrHashEntry *entry = (FrHashEntry*)ent ;
   FrVarArg(char **,buffer) ;
   if (entry)
      {
      *buffer = entry->displayValue(*buffer) ;
      strcat(*buffer," ") ;
      (*buffer)++ ;
      }
   return true ;
}

//----------------------------------------------------------------------

char *FrHashTableOld::displayValue(char *buffer) const
{
   if ((FrObject*)this == &NIL)
      {
      memcpy(buffer,str_NIL,3) ;
      return buffer+3 ;
      }
   else
      {
      strcpy(buffer,str_hash_intro) ;
      iterate(display_hash_entry,&buffer) ;
      *buffer++ = ')' ;
      *buffer = '\0' ;
      return buffer ;
      }
}

//----------------------------------------------------------------------

static bool dlength_hash_entry(const FrObject *ent, va_list args)
{
   FrHashEntry *entry = (FrHashEntry*)ent ;
   FrVarArg(int *,length) ;
   if (entry)
      *length += entry->displayLength() ;
   return true ;
}

//----------------------------------------------------------------------

size_t FrHashTableOld::displayLength() const
{
   if ((FrObject*)this == &NIL)
      return 3 ;
   else
      {
      int len = 4 ;
      iterate(dlength_hash_entry,&len) ;
      return len ;
      }
}
#endif /* 0 */

// end of file frhasht.cpp //
