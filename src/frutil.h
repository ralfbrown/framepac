/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC								*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frutil.h		generic utility functions		*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1997,1998,2001,2010,2013,2015				*/
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

#ifndef __FRUTIL_H_INCLUDED
#define __FRUTIL_H_INCLUDED

#include "frhash.h"

#include <stdio.h>

/************************************************************************/
/************************************************************************/

class FrObject ;

/************************************************************************/
/************************************************************************/

char *FrDupString(const char *s) ;
inline unsigned char *FrDupString(const unsigned char *s)
   { return (unsigned char*)FrDupString((const char *)s) ; }

char *FrDupStringN(const char *s, size_t length) ;
inline unsigned char *FrDupStringN(const unsigned char *s, size_t length)
   { return (unsigned char*)FrDupStringN((const char *)s,length) ; }

char *Fr_strupr(char *s) ;
char *Fr_strlwr(char *s) ;

char FrSkipWhitespace(const char *&s) ;
char FrSkipWhitespace(char *&s) ;
void FrSkipWhitespace(FILE *in) ;

char FrSkipToWhitespace(const char *&s) ;
char FrSkipToWhitespace(char *&s) ;

char *FrTrimWhitespace(char *line) ;	// remove leading/trailing whitespace

inline const char *FrPrintableName(const FrObject *obj)
   { return obj ? obj->printableName() : 0 ; }

void Fr_usleep(long microseconds) ;

#if (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4)) && (defined(__SSE4_2__) || !defined(FrFAST_MULTIPLY))
inline unsigned FrPopulationCount(uint64_t value) { return __builtin_popcountl(value) ; }
inline unsigned FrPopulationCount(uint32_t value) { return __builtin_popcount(value) ; }
#else
unsigned FrPopulationCount(uint64_t value) ;
unsigned FrPopulationCount(uint32_t value) ;
#endif

#endif /* !__FRUTIL_H_INCLUDED */

// end of file frutil.h //
