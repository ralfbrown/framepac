/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC								*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File: frstrut7.cpp	 	string-manipulation utility functions	*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 2006,2009,2011 Ralf Brown/Carnegie Mellon University	*/
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

#include "frstring.h"

#ifdef FrSTRICT_CPLUSPLUS
#  include <cstring>
#endif

/************************************************************************/
/*	Manifest Constants						*/
/************************************************************************/

#define MATCH_SINGLE	'?'
#define MATCH_ANY	'*'

/************************************************************************/
/************************************************************************/

static bool match(const char *pattern, size_t patlen, const char *s,
		    bool match_case, bool complete_match)
{
   if (match_case)
      {
      for ( ; *pattern && patlen > 0 ; pattern++, s++, patlen--)
	 {
	 if (!*s)
	    return false ;
	 if (*pattern != MATCH_SINGLE && *pattern != *s)
	    return false ;
	 }
      }
   else
      {
      for ( ; *pattern && patlen > 0 ; pattern++, s++, patlen--)
	 {
	 if (!*s)
	    return false ;
	 if (*pattern != MATCH_SINGLE &&
	     Fr_toupper(*pattern) != Fr_toupper(*s))
	    return false ;
	 }
      }
   return (complete_match ? (!*s) : true) ;
}

//----------------------------------------------------------------------

bool FrWildcardMatch(const char *pattern, const char *s,
		       bool match_case)
{
   if (!pattern || !s)
      return false ;
   const char *wild = strchr(pattern,MATCH_ANY) ;
   if (!wild)
      return match(pattern,strlen(pattern),s,match_case,true) ;
   else
      {
      const char *end = strchr(s,'\0') ;
      size_t wildlen = strlen(wild+1) ;
      return (match(pattern,wild-pattern,s,match_case,false) &&
	      match(wild+1,wildlen,end-wildlen,match_case,true)) ;
      }
}

// end of file frstrut7.cpp //
