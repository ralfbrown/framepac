/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File: frstrut4.cpp	 	string-manipulation utility functions	*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 2000,2001,2003,2004,2009,2010				*/
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
#include "framerr.h"
#include "frbytord.h"
#include "frctype.h"
#include "frstring.h"
#include "frsymtab.h"
#include "frunicod.h"

/************************************************************************/
/*	Global Data							*/
/************************************************************************/

const unsigned char FramepaC_EUC_toupper_table[] =
{
       0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, // ^@-^O
      16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, // ^P-^_
      32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, // SP- /
      48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, // 0 - ?
      64,'A','B','C','D','E', 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, // @ - O
      80, 81, 82, 83, 84, 85, 86, 87, 88, 89,'Z', 91, 92, 93, 94, 95, // P - _
      96,'A','B','C','D','E', 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, // ` - o
      80, 81, 82, 83, 84, 85, 86, 87, 88, 89,'Z',123,124,125,126,127, // p -DEL
     128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,
     144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,
     160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,
     176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,
     192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,
     208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,
     224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,
     240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255
} ;

//----------------------------------------------------------------------

const unsigned char FramepaC_EUC_tolower_table[] =
{
       0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, // ^@-^O
      16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, // ^P-^_
      32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, // SP- /
      48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, // 0 - ?
      64, 97, 98, 99,100,101,102,103,104,105,106,107,108,109,110,111, // @ - O
     112,113,114,115,116,117,118,119,120,121,122, 91, 92, 93, 94, 95, // P - _
      64, 97, 98, 99,100,101,102,103,104,105,106,107,108,109,110,111, // ` - o
     112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127, // p -DEL
     128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,
     144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,
     160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,
     176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,
     192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,
     208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,
     224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,
     240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255
} ;

//----------------------------------------------------------------------

const unsigned char FramepaC_no_casechange_table[] =
   {
       0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
      16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
      32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
      48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
      64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
      80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
      96, 97, 98, 99,100,101,102,103,104,105,106,107,108,109,110,111,
     112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,
     128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,
     144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,
     160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,
     176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,
     192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,
     208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,
     224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,
     240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255
   } ;

/************************************************************************/
/*	Helper Functions						*/
/************************************************************************/

FrCasemapTable FrUppercaseTable(FrCharEncoding enc)
{
   switch (enc)
      {
      case FrChEnc_Latin1:
      case FrChEnc_Latin2:
	 return FramepaC_toupper_table ;
      case FrChEnc_EUC:
      case FrChEnc_Unicode:
      case FrChEnc_UTF8:
	 return FramepaC_EUC_toupper_table ;
      case FrChEnc_RawOctets:
	 return FramepaC_no_casechange_table ;
      default:
	 return FramepaC_no_casechange_table ;
      }
}

//----------------------------------------------------------------------

FrCasemapTable FrLowercaseTable(FrCharEncoding enc)
{
   switch (enc)
      {
      case FrChEnc_Latin1:
      case FrChEnc_Latin2:
	 return FramepaC_tolower_table ;
      case FrChEnc_EUC:
      case FrChEnc_Unicode:
      case FrChEnc_UTF8:
	 return FramepaC_EUC_tolower_table ;
      case FrChEnc_RawOctets:
	 return FramepaC_no_casechange_table ;
      default:
	 return FramepaC_no_casechange_table ;
      }
}

/************************************************************************/
/************************************************************************/

FrSymbol *FrCvt2Symbol(const FrObject *word, bool force_uppercase)
{
   if (word)
      return FrCvt2Symbol(word,force_uppercase ? FrChEnc_Latin1
			  		       : FrChEnc_RawOctets) ;
   return 0 ;
}

//----------------------------------------------------------------------

char *Fr_strlwr(char *s, FrCharEncoding encoding)
{
   char *str = s ;
   if (s)
      {
      const unsigned char * downcase = FrLowercaseTable(encoding) ;
      char c ;
      while ((c = *s) != '\0')
	 *s++ = (char)downcase[(unsigned char)c] ;
      }
   return str ;
}

//----------------------------------------------------------------------

char *Fr_strupr(char *s, FrCharEncoding encoding)
{
   char *str = s ;
   if (s)
      {
      const unsigned char *upcase = FrUppercaseTable(encoding) ;
      char c ;
      while ((c = *s) != '\0')
	 *s++ = (char)upcase[(unsigned char)c] ;
      }
   return str ;
}

//----------------------------------------------------------------------

char *Fr_strmap(char *s, const char *mapping)
{
   char *str = s ;
   if (s)
      {
      char c ;
      while ((c = *s) != '\0')
	 *s++ = (char)mapping[(unsigned char)c] ;
      }
   return str ;
}

// end of file frstrut4.cpp //

