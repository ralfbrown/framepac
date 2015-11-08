/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frunicod.h		Unicode character-manipulation funcs	*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1996,1997,1998,2004,2007,2009				*/
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

#ifndef __FRUNICOD_H_INCLUDED
#define __FRUNICOD_H_INCLUDED

#ifndef __FRCOMMON_H_INCLUDED
#include "frcommon.h"
#endif

#include <stdio.h>

//================================
// 16-bit Unicode support
//================================

#define Fr_highbyte(widech) ((char)((widech >> 8) & 0xFF))
#define Fr_lowbyte(widech)  ((char)(widech & 0xFF))

//----------------------------------------------------------------------

extern bool FramepaC_check_Unicode_corruption ;

//----------------------------------------------------------------------

FrChar16 Fr_ugetc(FILE *in, bool &byteswap) ;
FrChar16 Fr_ugetc(istream &in, bool &byteswap) ;

FrChar16 *Fr_ugets(FILE *in, FrChar16 *buffer, size_t maxline,
		  bool &byteswap) ;
FrChar16 *Fr_ugets(istream &in, FrChar16 *buffer, size_t maxline,
		  bool &byteswap) ;

//----------------------------------------------------------------------

int Fr_Unicode_to_UTF8(FrChar16 codepoint, char *buffer, bool &byteswap);
	// returns number of bytes of buffer used (1-3) or -1 if second
	//  codepoint needed (call next function with both)
int Fr_Unicode_to_UTF8(FrChar16 codepoint1, FrChar16 codepoint2,
		       char *buffer, bool &byteswap) ;
	// returns number of bytes of buffer used (1-4)

char *Fr_Unicode_to_UTF8(const FrChar16 *input, bool &byteswap,
			 char *buffer) ;
char *Fr_Unicode_to_UTF8(const FrChar16 *input, bool &byteswap) ;
FrChar16 *Fr_UTF8_to_Unicode(const char *input, FrChar16 *result) ;
FrChar16 *Fr_UTF8_to_Unicode(const char *input) ;
size_t Fr_UTF8len(const char *string) ;

char *Fr_utf8gets(istream &in, char *buffer, size_t maxline, bool &byteswap);
char *Fr_utf8gets(FILE *in, char *buffer, size_t maxline, bool &byteswap) ;

//----------------------------------------------------------------------

size_t Fr_wcslen(const FrChar16 *) ;
FrChar16 *Fr_wcscpy(FrChar16 *__dest, const FrChar16 *__src) ;
FrChar16 *Fr_wcscpy(FrChar16 *__dest, const char *__src) ;
FrChar16 *Fr_wcsncpy(FrChar16 *__dest, const FrChar16 *__src, size_t) ;
FrChar16 *Fr_wcsncpy(FrChar16 *__dest, const char *__src, size_t) ;
FrChar16 *Fr_wcschr(const FrChar16 *, FrChar16) ;
FrChar16 *Fr_wcsrchr(const FrChar16 *, FrChar16) ;
int Fr_wcscmp(const FrChar16 *, const FrChar16 *) ;
int Fr_wcscmp(const FrChar16 *, const char *) ;
int Fr_wcsncmp(const FrChar16 *, const FrChar16 *,size_t) ;
int Fr_wcsncmp(const FrChar16 *, const char *,size_t) ;
int Fr_wcsicmp(const FrChar16 *, const FrChar16 *) ;
int Fr_wcsicmp(const FrChar16 *, const char *) ;
int Fr_wcsnicmp(const FrChar16 *, const FrChar16 *,size_t) ;
int Fr_wcsnicmp(const FrChar16 *, const char *,size_t) ;
FrChar16 *Fr_wcsstr(const FrChar16*, const FrChar16*) ;
FrChar16 *Fr_wcsstr(const FrChar16*, const char*) ;

#define Fr_ustrchr Fr_wcschr
#define Fr_ustrlen Fr_wcslen

#endif /* !__FRUNICOD_H_INCLUDED */

// end of file frunicod.h //
