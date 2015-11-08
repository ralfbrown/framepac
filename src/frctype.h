/************************************************************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frctype.h		character-manipulation functions	*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,1998,1999,2001,2003,2006,2008,	*/
/*		2010 Ralf Brown/Carnegie Mellon University		*/
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

#ifndef __FRCTYPE_H_INCLUDED
#define __FRCTYPE_H_INCLUDED

#include "frcommon.h"  // for FrChar16

//----------------------------------------------------------------------

extern const unsigned char FramepaC_toupper_table[] ;
#define Fr_toupper(c) (FramepaC_toupper_table[(unsigned char)c])
extern const unsigned char FramepaC_tolower_table[] ;
#define Fr_tolower(c) (FramepaC_tolower_table[(unsigned char)c])
extern const unsigned char FramepaC_ispunct_table[] ;
#define Fr_ispunct(c) (FramepaC_ispunct_table[(unsigned char)c])
extern const unsigned char FramepaC_isdigit_table[] ;
#define Fr_isdigit(c) (FramepaC_isdigit_table[(unsigned char)c])
extern const unsigned char FramepaC_isspace_table[] ;
#define Fr_isspace(c) (FramepaC_isspace_table[(unsigned char)c])
extern const unsigned char FramepaC_isalpha_table[] ;
extern const unsigned char FramepaC_isalpha_table_EUC[] ;
extern const unsigned char FramepaC_isalpha_table_raw[] ;
#define Fr_isalpha(c) (FramepaC_isalpha_table[(unsigned char)c])
extern const unsigned char FramepaC_islower_table[] ;
#define Fr_islower(c) (FramepaC_islower_table[(unsigned char)c])
extern const unsigned char FramepaC_isupper_table[] ;
#define Fr_isupper(c) (FramepaC_isupper_table[(unsigned char)c])

extern const unsigned char FramepaC_unaccent_table_Latin1[] ;
#define Fr_unaccent_Latin1(c)(FramepaC_unaccent_table_Latin1[(unsigned char)c])
extern const unsigned char FramepaC_unaccent_table_Latin2[] ;
#define Fr_unaccent_Latin2(c)(FramepaC_unaccent_table_Latin2[(unsigned char)c])

//----------------------------------------------------------------------

#if defined(FrDEFAULT_CHARSET_Latin1)
#define Fr_unaccent(c) Fr_unaccent_Latin1(c)
#else
#define Fr_unaccent(c) Fr_unaccent_Latin2(c)
#endif

//----------------------------------------------------------------------
// Support for 16-bit characters
//----------------------------------------------------------------------

int _Fr_iswalpha(FrChar16 c) ;
int _Fr_iswdigit(FrChar16 c) ;
int _Fr_iswspace(FrChar16 c) ;
int _Fr_iswlower(FrChar16 c) ;
int _Fr_iswupper(FrChar16 c) ;
FrChar16 _Fr_towlower(FrChar16 c) ;
FrChar16 _Fr_towupper(FrChar16 c) ;

#define Fr_is8bit(c) ((unsigned)(c) < 0x0100)

inline int Fr_iswdigit(FrChar16 c) { return c >= '0' && c <= '9' ; }
inline int Fr_iswspace(FrChar16 c)
   { return Fr_is8bit(c) ? Fr_isspace(c) : _Fr_iswspace(c) ; }
inline int Fr_iswalpha(FrChar16 c)
   { return Fr_is8bit(c) ? Fr_isalpha(c) : _Fr_iswalpha(c) ; }
inline int Fr_iswlower(FrChar16 c)
   { return Fr_is8bit(c) ? Fr_islower(c) : _Fr_iswlower(c) ; }
inline int Fr_iswupper(FrChar16 c)
   { return Fr_is8bit(c) ? Fr_isupper(c) : _Fr_iswupper(c) ; }

inline FrChar16 Fr_towlower(FrChar16 c)
   { return (FrChar16)(Fr_is8bit(c) ? Fr_tolower(c) : _Fr_towlower(c)) ; }
inline FrChar16 Fr_towupper(FrChar16 c)
   { return (FrChar16)(Fr_is8bit(c) ? Fr_toupper(c) : _Fr_towupper(c)) ; }

//----------------------------------------------------------------------

enum FrCharEncoding
   {
     FrChEnc_Latin1,
     FrChEnc_Latin2,
     FrChEnc_Unicode,
     FrChEnc_EUC,
     FrChEnc_RawOctets,
     FrChEnc_User,
     FrChEnc_UTF8	// UTF-8 encoded Unicode; acts essentially like EUC
   } ;

typedef uint32_t FrChar_t ;
typedef unsigned char const *FrCasemapTable ;

//----------------------------------------------------------------------

int Fr_stricmp(const char *s1, const char *s2) ;
int Fr_stricmp(const char *s1, const char *s2, const unsigned char *charmap) ;

FrCharEncoding FrParseCharEncoding(const char *enc_name) ;
const char *FrCharEncodingName(FrCharEncoding enc) ;

#endif /* !__FRCTYPE_H_INCLUDED */

// end of frctype.h //
