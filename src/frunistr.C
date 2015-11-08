/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frunistr.cpp		Unicode character-string funcs		*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1998,2001,2003,2006,2009				*/
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

#include "frbytord.h"
#include "frctype.h"
#include "frunicod.h"

#ifdef FrSTRICT_CPLUSPLUS
# include <cstdlib>
# include <cstring>
# include <string>	// needed by RedHat 7.1
#else
# include <iostream.h>
# include <stdlib.h>
# include <string.h>	// needed by RedHat 7.1
#endif /* FrSTRICT_CPLUSPLUS */

/************************************************************************/
/*	Global variables						*/
/************************************************************************/

/************************************************************************/
/************************************************************************/

//----------------------------------------------------------------------

FrChar16 *Fr_wcschr(const FrChar16 *s, FrChar16 c)
{
   if (s)
      {
      while (*s)
	 {
	 if ((FrChar16)FrByteSwap16(*s) == c)
	    return (FrChar16*)s ;
	 s++ ;
	 }
      return (c == 0) ? (FrChar16*)s : 0 ;
      }
   return 0 ;
}

//----------------------------------------------------------------------

FrChar16 *Fr_wcsrchr(const FrChar16 *s, FrChar16 c)
{
   FrChar16 *found = 0 ;
   if (s)
      {
      while (*s)
	 {
	 if ((FrChar16)FrByteSwap16(*s) == c)
	    found = (FrChar16*)s ;
	 s++ ;
	 }
      if (c == 0)
	 found = (FrChar16*)s ;
      }
   return found ;
}

//----------------------------------------------------------------------

int Fr_wcscmp(const FrChar16 *s1, const FrChar16 *s2)
{
   while (*s1 && *s1 == *s2)
      {
      s1++ ;
      s2++ ;
      }
   return FrByteSwap16(*s1) - FrByteSwap16(*s2) ;
}

//----------------------------------------------------------------------

int Fr_wcscmp(const FrChar16 *s1, const char *s2)
{
   while (*s1 && *s1 == (FrChar16)*s2)
      {
      s1++ ;
      s2++ ;
      }
   return FrByteSwap16(*s1) - (FrChar16)*s2 ;
}

//----------------------------------------------------------------------

int Fr_wcsicmp(const FrChar16 *s1, const FrChar16 *s2)
{
   int diff = 0 ;
   while ((diff = (Fr_towupper(FrByteSwap16(*s1)) -
		   Fr_towupper(FrByteSwap16(*s2)))) == 0 &&
	  *s1)
      {
      s1++ ;
      s2++ ;
      }
   return diff ;
}

//----------------------------------------------------------------------

int Fr_wcsicmp(const FrChar16 *s1, const char *s2)
{
   int diff = 0 ;
   while ((diff = (Fr_towupper(FrByteSwap16(*s1)) -
		   (FrChar16)Fr_toupper(*s2))) == 0 &&
	  *s1)
      {
      s1++ ;
      s2++ ;
      }
   return diff ;
}

//----------------------------------------------------------------------

int Fr_wcsncmp(const FrChar16 *s1, const FrChar16 *s2, size_t N)
{
   if (N == 0)
      return 0 ;
   while (--N > 0 && *s1 && *s1 == *s2)
      {
      s1++ ;
      s2++ ;
      }
   return FrByteSwap16(*s1) - FrByteSwap16(*s2) ;
}

//----------------------------------------------------------------------

int Fr_wcsncmp(const FrChar16 *s1, const char *s2, size_t N)
{
   if (N == 0)
      return 0 ;
   while (--N > 0 && *s1 && *s1 == (FrChar16)*s2)
      {
      s1++ ;
      s2++ ;
      }
   return FrByteSwap16(*s1) - (FrChar16)*s2 ;
}

//----------------------------------------------------------------------

int Fr_wcsnicmp(const FrChar16 *s1, const FrChar16 *s2, size_t N)
{
   if (N == 0)
      return 0 ;
   int diff = 0 ;
   while (--N > 0 && (diff = (Fr_towupper(FrByteSwap16(*s1)) -
			      Fr_towupper(FrByteSwap16(*s2)))) == 0 &&
	  *s1)
      {
      s1++ ;
      s2++ ;
      }
   return diff ;
}

//----------------------------------------------------------------------

int Fr_wcsnicmp(const FrChar16 *s1, const char *s2, size_t N)
{
   if (N == 0)
      return 0 ;
   int diff = 0 ;
   while (--N > 0 && (diff = (Fr_towupper(FrByteSwap16(*s1)) -
			      (FrChar16)Fr_toupper(*s2))) == 0 &&
	  *s1)
      {
      s1++ ;
      s2++ ;
      }
   return diff ;
}

//----------------------------------------------------------------------

FrChar16 *Fr_wcsstr(const FrChar16 *s, const FrChar16 *pattern)
{
   if (!s || !pattern)
      return 0 ;
   size_t patlen = Fr_wcslen(pattern) ;
   while (*s)
      {
      if (*s == *pattern)
	 {
	 if (patlen == 1 || Fr_wcsncmp(s+1,pattern+1,patlen-1) == 0)
	    return (FrChar16*)s ;
	 }
      s++ ;
      }
   return 0 ;
}

//----------------------------------------------------------------------

FrChar16 *Fr_wcsstr(const FrChar16 *s, const char *pattern)
{
   if (!s || !pattern)
      return 0 ;
   size_t patlen = strlen(pattern) ;
   while (*s)
      {
      if (*s == (FrChar16)*pattern)
	 {
	 if (patlen == 1 || Fr_wcsncmp(s+1,pattern+1,patlen-1) == 0)
	    return (FrChar16*)s ;
	 }
      s++ ;
      }
   return 0 ;
}

//----------------------------------------------------------------------

FrChar16 *Fr_wcscpy(FrChar16 *dest, const FrChar16 *src)
{
   FrChar16 *dst = dest ;
   if (src && dst)
      {
      do {
         *dst++ = *src++ ;
         } while (*src) ;
      }
   return dest ;
}

//----------------------------------------------------------------------

FrChar16 *Fr_wcscpy(FrChar16 *dest, const char *src)
{
   FrChar16 *dst = dest ;
   if (src && dst)
      {
      do {
         FrStoreShort((FrChar16)*src++,dst++) ;
         } while (*src) ;
      }
   return dest ;
}

//----------------------------------------------------------------------

FrChar16 *Fr_wcsncpy(FrChar16 *dest, const FrChar16 *src, size_t N)
{
   FrChar16 *dst = dest ;
   if (src && dst && N > 0)
      {
      do {
         *dst++ = *src++ ;
         } while (*src && --N > 0) ;
      }
   return dest ;
}

//----------------------------------------------------------------------

FrChar16 *Fr_wcsncpy(FrChar16 *dest, const char *src, size_t N)
{
   FrChar16 *dst = dest ;
   if (src && dst && N > 0)
      {
      do {
         FrStoreShort((FrChar16)*src++,dst++) ;
         } while (*src && --N > 0) ;
      }
   return dest ;
}

//----------------------------------------------------------------------


// end of file frunistr.cpp //

