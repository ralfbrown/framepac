/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File: frstrut3.cpp	 	string-manipulation utility functions	*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1998,2001,2009,2010					*/
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

#include <string.h>
#include "frctype.h"
#include "frstring.h"

/************************************************************************/
/*	Helper Functions						*/
/************************************************************************/

/************************************************************************/
/************************************************************************/

int Fr_strnicmp(const char *s1, const char *s2, size_t N)
{
   if (N == 0)
      return 0 ;
   int diff = 0 ;
   if (s1 && s2)
      {
      while (N-- > 0 && (diff = (Fr_toupper(*s1) - Fr_toupper(*s2))) == 0 &&
	     *s1 && *s2)
	 {
	 s1++ ;
	 s2++ ;
	 }
      }
   else if (s1)
      return +1 ;
   else if (s2)
      return -1 ;
   return diff ;
}

//----------------------------------------------------------------------

int Fr_strnicmp(const char *s1, const char *s2, size_t N, 
		const FrCasemapTable map)
{
   if (N == 0)
      return 0 ;
   int diff = 0 ;
   if (s1 && s2)
      {
      while (N-- > 0 &&
	     (diff = (map[*(unsigned char*)s1] - map[*(unsigned char*)s2]))
	       == 0 &&
	     *s1 && *s2)
	 {
	 s1++ ;
	 s2++ ;
	 }
      }
   else if (s1)
      return +1 ;
   else if (s2)
      return -1 ;
   return diff ;
}

//----------------------------------------------------------------------

char *Fr_strichr(const char *s, char c)
{
   if (!s)
      return 0 ;
   c = Fr_toupper(c) ;
   while (*s && Fr_toupper(*s) != (unsigned char)c)
      s++ ;
   return Fr_toupper(*s) == (unsigned char)c ? (char*)s : 0 ;
}

//----------------------------------------------------------------------

char *Fr_stristr(const char *s, const char *pattern)
{
   if (!s || !pattern)
      return 0 ;
   size_t patlen = strlen(pattern) ;
   while (*s)
      {
      if (Fr_toupper(*s) == Fr_toupper(*pattern))
         {
         if (patlen == 1 || Fr_strnicmp(s+1,pattern+1,patlen-1) == 0)
            return (char*)s ;
         }
      s++ ;
      }
   return 0 ;				// not found
}

//----------------------------------------------------------------------

char *FrTruncationPoint(char *s, char marker)
{
   if (!s)
      return 0 ;
   while (*s && *s != marker)
      s++ ;
   return s ;
}

//----------------------------------------------------------------------

const char *FrTruncationPoint(const char *s, char marker)
{
   if (!s)
      return 0 ;
   while (*s && *s != marker)
      s++ ;
   return s ;
}

// end of file frstrut3.cpp //
