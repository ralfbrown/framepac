/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frctype.cpp		wide-character-manipulation functions	*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1998,1999,2004,2009					*/
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

#include <ctype.h>
#include "frctype.h"
#if defined(__SUNOS__) || defined(__SOLARIS__) || defined(__linux__)
#  include <wctype.h>
#endif
#if defined(__WATCOMC__) && __WATCOMC__ >= 1200	// OpenWatcom
//  (Watcom 10.x and 11.x have the isw... and tow... funcs in ctype.h)
#  include <wctype.h>
#endif

#if defined(PURIFY) && defined(__SOLARIS__)
#  define NO_WCTYPE
#endif

/************************************************************************/
/************************************************************************/

FrChar16 _Fr_towlower(FrChar16 c)
{
#if !defined(NO_WCTYPE) && !defined(FrBROKEN_TOWLOWER)
   return (FrChar16)towlower(c) ;
#else
   return Fr_is8bit(c) ? tolower(c) : c ;
#endif /* NO_WCTYPE */
}

//----------------------------------------------------------------------

FrChar16 _Fr_towupper(FrChar16 c)
{
#if !defined(NO_WCTYPE) && !defined(FrBROKEN_TOWLOWER)
   return (FrChar16)towupper(c) ;
#else
   return Fr_is8bit(c) ? toupper(c) : c ;
#endif /* NO_WCTYPE */
}

//----------------------------------------------------------------------

int _Fr_iswalpha(FrChar16 c)
{
#ifndef NO_WCTYPE
   return iswalpha(c) ;
#else
   return Fr_is8bit(c) && isalpha(c) ;
#endif /* NO_WCTYPE */
}

//----------------------------------------------------------------------

int _Fr_iswspace(FrChar16 c)
{
#ifndef NO_WCTYPE
   return iswspace(c) ;
#else
   return Fr_is8bit(c) ? isspace(c) : false ;
#endif /* NO_WCTYPE */
}

//----------------------------------------------------------------------

int _Fr_iswupper(FrChar16 c)
{
#ifndef NO_WCTYPE
   return iswupper(c) ;
#else
   return Fr_is8bit(c) ? isupper(c) : false ;
#endif /* NO_WCTYPE */
}

//----------------------------------------------------------------------

int _Fr_iswlower(FrChar16 c)
{
#ifndef NO_WCTYPE
   return iswlower(c) ;
#else
   return Fr_is8bit(c) ? islower(c) : false ;
#endif /* NO_WCTYPE */
}

// end of file frwctype.cpp //
