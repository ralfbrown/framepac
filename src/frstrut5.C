/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File: frstrut5.cpp	 	string-manipulation utility functions	*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 2001,2009 Ralf Brown/Carnegie Mellon University	*/
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
/************************************************************************/

int Fr_strnicmp(const char *s1, const char *s2, size_t N, FrCharEncoding enc)
{
   if (N == 0)
      return 0 ;
   const unsigned char *map = FrUppercaseTable(enc) ;
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

char *Fr_strichr(const char *s, char c, FrCharEncoding enc)
{
   if (!s)
      return 0 ;
   const unsigned char *map = FrUppercaseTable(enc) ;
   unsigned char ch = map[(unsigned char)c] ;
   while (*s && map[*(unsigned char*)s] != ch)
      s++ ;
   return (map[*(unsigned char*)s] == ch) ? (char*)s : 0 ;
}

//----------------------------------------------------------------------

char *Fr_stristr(const char *s, const char *pattern, FrCharEncoding enc)
{
   if (!s || !pattern)
      return 0 ;
   size_t patlen = strlen(pattern) ;
   const unsigned char *map = FrUppercaseTable(enc) ;
   while (*s)
      {
      if (map[*(unsigned char*)s] == map[*(unsigned char*)pattern])
         {
         if (patlen == 1 || Fr_strnicmp(s+1,pattern+1,patlen-1,enc) == 0)
            return (char*)s ;
         }
      s++ ;
      }
   return 0 ;				// not found
}

// end of file frstrut5.cpp //
