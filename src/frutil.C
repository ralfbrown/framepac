/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC								*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frutil.cpp		generic utility functions		*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1997,1998,2001,2009,2010,2011,2013			*/
/*	    Ralf Brown/Carnegie Mellon University			*/
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

#include <string.h>
#include "framerr.h"
#include "frctype.h"
#include "frmem.h"
#include "frstring.h"
#include "frsymbol.h"
#include "frutil.h"

/************************************************************************/
/*	Globals for this module						*/
/************************************************************************/

static const char dupstr_msg[] = "while duplicating a string" ;

/************************************************************************/
/************************************************************************/

char *FrDupString(const char *s)
{
   if (!s)
      return 0 ;
   size_t len = strlen(s) + 1 ;
   char *newstr = FrNewN(char,len) ;
   if (newstr)
      memcpy(newstr,s,len) ;
   else
      FrNoMemory(dupstr_msg) ;
   return newstr ;
}

//----------------------------------------------------------------------

char *FrDupStringN(const char *s, size_t length)
{
   if (!s)
      return 0 ;
   char *newstr = FrNewN(char,length + 1) ;
   if (newstr)
      {
      memcpy(newstr,s,length) ;
      newstr[length] = '\0' ;
      }
   else
      FrNoMemory(dupstr_msg) ;
   return newstr ;
}

//----------------------------------------------------------------------

char *Fr_strlwr(char *s)
{
   char *str = s ;
   if (s)
      while (*s)
	 {
	 *s = (char)Fr_tolower(*s) ;
	 s++ ;
	 }
   return str ;
}

//----------------------------------------------------------------------

char *Fr_strupr(char *s)
{
   char *str = s ;
   if (s)
      {
      char c ;
      while ((c = *s) != '\0')
	 {
	 *s++ = (char)Fr_toupper(c) ;
	 }
      }
   return str ;
}

//----------------------------------------------------------------------

char FrSkipWhitespace(const char *&s)
{
   if (s)
      {
      while (Fr_isspace(*s))
	 s++ ;
      return (char)*s ;
      }
   else
      return '\0' ;
}

//----------------------------------------------------------------------

char FrSkipWhitespace(char *&s)
{
   if (s)
      {
      while (Fr_isspace(*s))
	 s++ ;
      return *s ;
      }
   else
      return '\0' ;
}

//----------------------------------------------------------------------

void FrSkipWhitespace(FILE *in)
{
   int c ;
   while ((c = fgetc(in)) != EOF && Fr_isspace(c))
      ;
   if (c != EOF)			// put back the non-whitespace char
      ungetc(c,in) ;
   return ;
}

//----------------------------------------------------------------------

char FrSkipToWhitespace(const char *&s)
{
   if (s)
      {
      while (*s && !Fr_isspace(*s))
	 s++ ;
      return (char)*s ;
      }
   else
      return '\0' ;
}

//----------------------------------------------------------------------

char FrSkipToWhitespace(char *&s)
{
   if (s)
      {
      while (*s && !Fr_isspace(*s))
	 s++ ;
      return *s ;
      }
   else
      return '\0' ;
}

//----------------------------------------------------------------------

char *FrTrimWhitespace(char *line)
{
   if (FrSkipWhitespace(line) != '\0')
      {
      char *end = strchr(line,'\0') ;
      while (end > line && Fr_isspace(end[-1]))
	 end-- ;
      *end = '\0' ;
      }
   return line ;
}

//----------------------------------------------------------------------

#if (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4)) && (defined(__SSE4_2__) || !defined(FrFAST_MULTIPLY))
#else
unsigned FrPopulationCount(uint64_t val)
{
#if defined(FrFAST_MULTIPLY)
   // from graphics.stanford.edu/~seander/bithacks.html
   val = val - ((val >> 1) & 0x5555555555555555) ;
   val = (val & 0x3333333333333333) + ((val >> 2) & 0x3333333333333333) ;
   val = (((val + (val >> 4)) & 0x0F0F0F0F0F0F0F0F) * 0x0101010101010101) >> 56 ;
   return val ;
#elif 0 // version of the shift/mask/add version for superscalar 32-bit CPUs
   uint32_t val1 = (uint32_t)val ;
   uint32_t val2 = (uint32_t)(val >> 32) ;
   val1 = val1 - ((val1 >> 1) & 0x55555555) ;
   val2 = val2 - ((val2 >> 1) & 0x55555555) ;
   val1 = ((val1 >> 2) & 0x33333333) + (val1 & 0x33333333) ;
   val2 = ((val2 >> 2) & 0x33333333) + (val2 & 0x33333333) ;
   val1 = (val1 + (val1 >> 4)) & 0x0F0F0F0F ;
   val2 = (val2 + (val2 >> 4)) & 0x0F0F0F0F ;
   val1 += (val1 >> 8) ;
   val2 += (val2 >> 8) ;
   val1 += (val1 >> 16) ;
   val2 += (val2 >> 16) ;
   return val1 + val2 ;
#else
//naive implementation:
//   val = ((val & 0xAAAAAAAAAAAAAAAA) >> 1) + (val & 0x5555555555555555) ;
//   val = ((val & 0xCCCCCCCCCCCCCCCC) >> 2) + (val & 0x3333333333333333) ;
//   val = ((val & 0xF0F0F0F0F0F0F0F0) >> 4) + (val & 0x0F0F0F0F0F0F0F0F) ;
//   val = ((val & 0xFF00FF00FF00FF00) >> 8) + (val & 0x00FF00FF00FF00FF) ;
//   val = ((val & 0xFFFF0000FFFF0000) >> 16) + (val & 0x0000FFFF0000FFFF) ;
//   val = ((val & 0xFFFFFFFF00000000) >> 32) + (val & 0x00000000FFFFFFFF) ;
//optimized version as shown at http://en.wikipedia.org/wiki/Hamming_weight
   val = ((val >> 1) & 0x5555555555555555) + (val & 0x5555555555555555) ;
   val = ((val >> 2) & 0x3333333333333333) + (val & 0x3333333333333333) ;
   val = (val + (val >> 4)) & 0x0F0F0F0F0F0F0F0F ;
   val += (val >> 8) ;
   val += (val >> 16) ;
   val += (val >> 32) ;
   return val & 0x7F ;
#endif
}
#endif /* GCC v3.4 or higher */

//----------------------------------------------------------------------

#if (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4)) && (defined(__SSE4_2__) || !defined(FrFAST_MULTIPLY))
#else
unsigned FrPopulationCount(uint32_t val)
{
#if defined(FrFAST_MULTIPLY)
   // from graphics.stanford.edu/~seander/bithacks.html
   val = val - ((val >> 1) & 0x55555555) ;
   val = (val & 0x33333333) + ((val >> 2) & 0x33333333) ;
   val = (((val + (val >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24 ;
   return val ;
#else
   val = ((val & 0xAAAAAAAA) >> 1) + (val & 0x55555555) ;
   val = ((val & 0xCCCCCCCC) >> 2) + (val & 0x33333333) ;
   val = ((val + (val >> 4)) & 0x0F0F0F0F) ;
   val = val + (val >> 8) ;
   val = val + (val >> 16) ;
   return val & 0x3F ;
#endif
}
#endif /* GCC v3.4 or higher */

//----------------------------------------------------------------------

unsigned FrPopulationCount(uint16_t val)
{
#if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4)
   return __builtin_popcount(val) ;
#else
   val = ((val & 0xAAAA) >> 1) + (val & 0x5555) ;
   val = ((val & 0xCCCC) >> 2) + (val & 0x3333) ;
   val = ((val + (val >> 4)) & 0x0F0F) ;
   val = val + (val >> 8) ;
   return val & 0x1F ;
#endif
}

//----------------------------------------------------------------------

// end of file frutil.cpp //



