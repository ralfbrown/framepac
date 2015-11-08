/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC								*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frurl.cpp		URL-handling functions			*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 2000,2001,2009 Ralf Brown/Carnegie Mellon University	*/
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
#include "frurl.h"

/************************************************************************/
/*	Global data for this module					*/
/************************************************************************/

static char hexdigits[] = "0123456789ABCDEF" ;

static const char base64chars[] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/" ;

/************************************************************************/
/************************************************************************/

inline bool char_needs_quoting(char c)
{
   return (bool)Fr_isspace(c) ||
          c == '%' || c == '~' || c == '\\' ;
}

//----------------------------------------------------------------------

char *FrURLEncode(const char *string, size_t stringlen)
{
   size_t len = 0 ;
   size_t i ;
   for (i = 0 ; i < stringlen ; i++)
      len += (char_needs_quoting(string[i]) ? 3 : 1) ;
   char *result = FrNewN(char,len+1) ;
   size_t pos = 0 ;
   if (!result)
      {
      FrNoMemory("while encoding URL for transmission") ;
      return 0 ;
      }
   for (i = 0 ; i < stringlen ; i++)
      {
      char c = string[i] ;
      if (char_needs_quoting(c))
	 {
	 result[pos++] = '%' ;
	 result[pos++] = hexdigits[(c>>4)&0x0F] ;
	 result[pos++] = hexdigits[c&0x0F] ;
	 }
      else
	 result[pos++] = c ;
      }
   result[len] = '\0' ;
   return result ;
}

//----------------------------------------------------------------------

char *FrURLEncode(const char *string)
{
   return string ? FrURLEncode(string,strlen(string)) : 0 ;
}

//----------------------------------------------------------------------

char *FrBase64Encode(const char *input, size_t len)
{
   if (!input || len == 0)
      return 0 ;
   size_t encodedlen = 4 * ((len + 2) / 3) ;
   char *result = FrNewN(char,encodedlen+1) ;
   if (result)
      {
      // split each group of three bytes into four sets of six bits, and output
      //   each of those as a printing character
      char *out = result ;
      size_t i ;
      for (i = 0 ; i < len ; i += 3)
	 {
	 char c1 = input[i] ;
	 char c2 = input[i+1] ;
	 char c3 = input[i+2] ;
	 *out++ = base64chars[(c1 >> 2) & 0x3F] ;
	 *out++ = base64chars[((c1 & 0x03) << 4) | ((c2 >> 4) & 0x0F)] ;
	 *out++ = base64chars[((c2 & 0x0F) << 2) | ((c3 >> 6) & 0x03)] ;
	 *out++ = base64chars[(c3 & 0x3F)] ;
	 }
      if (i > len)			// less than 3 bytes in last group?
	 out[-1] = '=' ;		//  -- insert a null marker
      if (i > len+1)			// only one byte in last group?
	 out[-2] = '=' ;		//  -- insert another null marker
      }
   else
      FrNoMemory("while Base64-encoding string") ;
   return result ;
}

//----------------------------------------------------------------------

char *FrBase64Encode(const char *string)
{
   return string ? FrBase64Encode(string,strlen(string)) : 0 ;
}

//----------------------------------------------------------------------

char *FrURLDecode(const char *string, size_t stringlen)
{
   char *result = FrNewN(char,stringlen+1) ;
   size_t len = 0 ;
   if (!result)
      {
      FrNoMemory("while decoding URL") ;
      return 0 ;
      }
   for (size_t i = 0 ; i < stringlen ; i++)
      {
      char c = string[i] ;
      if (c == '%')
	 {
	 c = string[++i] ;
	 int hi = Fr_isdigit(c) ? (c - '0') : (Fr_toupper(c) - 'A' + 10) ;
	 c = string[++i] ;
	 int lo = Fr_isdigit(c) ? (c - '0') : (Fr_toupper(c) - 'A' + 10) ;
	 result[len++] = (char)(16 * hi + lo) ;
	 }
      else
	 result[len++] = c ;
      }
   result[len] = '\0' ;
   return result ;
}

//----------------------------------------------------------------------

char *FrURLDecode(const char *string)
{
   return string ? FrURLDecode(string,strlen(string)) : 0 ;
}

// end of file frurl.cpp //
