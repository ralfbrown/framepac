/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File: frstrng2.cpp	 	class FrString, concat/append		*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,1998,1999,2000,2001,2002,2006,   	*/
/*		2009,2010 Ralf Brown/Carnegie Mellon University		*/
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
#  pragma implementation "frstring.h"
#endif

#include <string.h>
#include "framerr.h"
#include "frbytord.h"
#include "frcmove.h"
#include "frstring.h"

/************************************************************************/
/************************************************************************/

void *FrStrCpy(void *dest, const void *src, int length, int w1, int w2) ;

static const char nomem_appending[]
    = "while appending two strings" ;
static const char nomem_concat[]
    = "while concatenating two strings" ;

/************************************************************************/
/************************************************************************/

FrString *FrString::concatenate(const FrString *s) const
{
   FrString *newstring = new FrString ;
   int new_width = FrMax(charWidth(),s->charWidth()) ;
   int new_len ;

   if (!newstring)
      {
      FrNoMemory(nomem_concat) ;
      return 0 ;
      }
   new_len = stringLength() + s->stringLength() ;
   newstring->setCharWidth(new_width) ;
   unsigned char *newval = reserve(new_len,new_width) ;
   newstring->setLength(new_len) ;
   if (!newval)
      {
      newstring->freeObject() ;
      FrNoMemory(nomem_concat) ;
      return 0 ;
      }
   newstring->m_value = newval ;
   void *add = FrStrCpy(newval,stringValue(),stringLength(),
			new_width,charWidth()) ;
   add = FrStrCpy(add,s->stringValue(),s->stringLength(),
		  new_width,s->charWidth()) ;
   if (new_width > 1)
      memset(add,'\0',new_width) ;
   else
      *((char*)add) = '\0' ;		// proper termination
   return newstring ;
}

//----------------------------------------------------------------------

FrString *FrString::append(const FrString *s)
{
   int width = charWidth() ;
   int newlen = stringLength() + s->stringLength() ;
   int new_width = FrMax(width,s->charWidth()) ;
   unsigned char *newvalue ;
   if (new_width == width)
      {
      if (!realloc(newlen,new_width))
	 {
	 FrNoMemory(nomem_appending) ;
	 return 0 ;
	 }
      }
   else
      {
      newvalue = reserve(newlen,new_width) ;
      if (!newvalue)
	 {
	 FrNoMemory(nomem_appending) ;
	 return 0 ;
	 }
      FrStrCpy(newvalue,stringValue(),stringLength(),new_width,width) ;
      free() ;
      setCharWidth(new_width) ;
      m_value = newvalue ;
      }
   FrStrCpy(m_value+stringLength()*new_width,s->stringValue(),
	    s->stringLength(),new_width,s->charWidth()) ;
   if (new_width > 1)
      memset(m_value+newlen*new_width,'\0',new_width) ;
   else
      m_value[newlen] = '\0' ; // ensure proper NUL-termination
   setLength(newlen) ;
   return this ;
}

//----------------------------------------------------------------------

FrString *FrString::append(const FrChar_t c)
{
   int newlen = stringLength() + 1 ;
   int newwidth ;
   if (c <= 0x000000FF)
      newwidth = 1 ;
   else if (c <= 0x0000FFFF)
      newwidth = 2 ;
   else
      newwidth = 4 ;
   int width = charWidth() ;
   newwidth = FrMax(width,newwidth) ;
   if (newwidth == width)
      {
      if (!realloc(newlen,width))
	 {
	 FrNoMemory(nomem_appending) ;
	 return 0 ;
	 }
      }
   else
      {
      unsigned char *newvalue = reserve(newlen,width) ;
      if (!newvalue)
	 {
	 FrNoMemory(nomem_appending) ;
	 return 0 ;
	 }
      FrStrCpy(newvalue,stringValue(),stringLength(),newwidth,width) ;
      free() ;
      m_value = newvalue ;
      }
   size_t len = stringLength() ;
   if (newwidth == 1)
      m_value[len] = (unsigned char)c ;
   else if (newwidth == 2)
      {
      FrStoreShort(c,((uint16_t*)m_value)+len) ;
      }
   else
      {
      FrStoreLong(c,((uint32_t*)m_value)+len) ;
      }
   if (width > 1)
      memset(m_value+newlen*width,'\0',width) ;
   else
      m_value[newlen*width] = '\0' ; // ensure proper NUL-termination
   setLength(newlen) ;
   setCharWidth(newwidth) ;
   return this ;
}

//----------------------------------------------------------------------

FrString *FrString::append(const char *s)
{
   int len = strlen(s) ;
   unsigned int width = charWidth() ;
   int newlen = stringLength() + len ;
   if (!realloc(newlen,width))
      {
      FrNoMemory(nomem_appending) ;
      return 0 ;
      }
   if (width > 1)
      {
      char *end = (char*)FrStrCpy(m_value+stringLength()*width,s,len,width,1) ;
      memset(end,'\0',width) ;
      }
   else
      {
      memcpy(m_value+stringLength(),s,len) ;
      m_value[newlen] = '\0' ;
      }
   setLength(newlen) ;
   return this ;
}

// end of file frstrng2.cpp //
