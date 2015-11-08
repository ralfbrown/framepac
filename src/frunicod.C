/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frunicod.cpp		Unicode character-manipulation funcs	*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1996,1997,2001,2003,2004,2007,2009			*/
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

#include <stdlib.h>
#include <string.h>	// needed by RedHat 7.1
#include "frbytord.h"
#include "frmem.h"
#include "frunicod.h"

/************************************************************************/
/*	Global variables						*/
/************************************************************************/

bool FramepaC_check_Unicode_corruption = false ;

/************************************************************************/
/************************************************************************/

//----------------------------------------------------------------------

FrChar16 Fr_uputc(ostream &out,FrChar16 ch)
{
   out.put(Fr_highbyte(ch)) ;
   out.put(Fr_lowbyte(ch)) ;
   if (!out.bad())
      return ch ;
   else
      return (FrChar16)EOF ;
}

//----------------------------------------------------------------------
//  output a 16-bit character in network (big-endian) byte order

FrChar16 Fr_uputc(FILE *out, FrChar16 ch)
{
   bool success = (putc(Fr_highbyte(ch),out) != EOF) &&
		    (putc(Fr_lowbyte(ch),out) != EOF) ;
   if (success)
      return ch ;
   else
      return (FrChar16)EOF ;
}

//----------------------------------------------------------------------
//  read a 16-bit character, performing byte-swapping as needed

FrChar16 Fr_ugetc(istream &in, bool &byteswap)
{
   for (;;)
      {
      int c1 = in.get() ;
      int c2 = in.get() ;
      if (c1 == EOF || c2 == EOF)
	 return (FrChar16)EOF ;
      int ch ;
      if (byteswap)
	 ch = ((c2 & 0xFF) << 8) | (c1 & 0xFF) ;
      else
	 ch = ((c1 & 0xFF) << 8) | (c2 & 0xFF) ;
      // swallow the byte-order markers, toggling the byte-swapping as needed
      if (ch == 0xFFFE)
	 {
	 byteswap = !byteswap ;
	 continue ;
	 }
      else if (ch == 0xFEFF)
	 continue ;
      else
	 return (FrChar16)ch ;
      }
}

//----------------------------------------------------------------------

FrChar16 Fr_ugetc(FILE *in, bool &byteswap)
{
   for (;;)
      {
      int c1 = getc(in) ;
      int c2 = getc(in) ;
      if (c1 == EOF || c2 == EOF)
	 return (FrChar16)EOF ;
      int ch ;
      if (byteswap)
	 ch = ((c2 & 0xFF) << 8) | (c1 & 0xFF) ;
      else
	 ch = ((c1 & 0xFF) << 8) | (c2 & 0xFF) ;
      // swallow the byte-order markers, toggling the byte-swapping as needed
      if (ch == 0xFFFE)
	 {
	 byteswap = !byteswap ;
	 continue ;
	 }
      else if (ch == 0xFEFF)
	 continue ;
      else
	 return (FrChar16)ch ;
      }
}

//----------------------------------------------------------------------

FrChar16 *Fr_ugets(istream &in, FrChar16 *buffer, size_t maxline,
		  bool &byteswap)
{
   FrChar16 *buf = buffer ;
   if (maxline == 0)
      return 0 ;
   else
      maxline-- ;			// reserve space for the terminator
   while (maxline > 0)
      {
      int c1 = in.get() ;
      int c2 = in.get() ;
      if (c1 == EOF || c2 == EOF)
	 break ;
      int ch ;
      if (byteswap)
	 ch = ((c2 & 0xFF) << 8) | (c1 & 0xFF) ;
      else
	 ch = ((c1 & 0xFF) << 8) | (c2 & 0xFF) ;
      if (FramepaC_check_Unicode_corruption &&
	  (ch == 0x0a00 || ch == 0x2000))
	 {
	 cerr << "The Unicode character stream appears to have been corrupted "
	         "near\n" ;
	 cerr.write((char*)buffer,buf-buffer) ;
	 cerr << endl ;
	 in.get() ;			// try to resynchronize
	 }
      if (ch == '\n' || ch == '\0')
	 break ;
      // swallow the byte-order markers, toggling the byte-swapping as needed
      if (ch == 0xFFFE)
	 {
	 byteswap = !byteswap ;
	 continue ;
	 }
      else if (ch == 0xFEFF)
	 continue ;
      FrStoreShort(ch,buf) ;
      buf++ ;
      maxline-- ;
      }
   *buf = 0 ;
   return buffer ;
}

//----------------------------------------------------------------------

FrChar16 *Fr_ugets(FILE *in, FrChar16 *buffer, size_t maxline,
		  bool &byteswap)
{
   FrChar16 *buf = buffer ;
   if (maxline == 0)
      return 0 ;
   else
      maxline-- ;			// reserve space for the terminator
   while (maxline > 0)
      {
      int c1 = getc(in) ;
      int c2 = getc(in) ;
      if (c1 == EOF || c2 == EOF)
	 {
	 if (buf == buffer)
	    {
	    *buf = 0 ;
	    return 0 ;
	    }
	 break ;
         }
      int ch ;
      if (byteswap)
	 ch = ((c2 & 0xFF) << 8) | (c1 & 0xFF) ;
      else
	 ch = ((c1 & 0xFF) << 8) | (c2 & 0xFF) ;
      if (FramepaC_check_Unicode_corruption &&
	  (ch == 0x0a00 || ch == 0x2000))
	 {
	 cerr << "The Unicode character stream appears to have been corrupted "
	         "near\n" ;
	 cerr.write((char*)buffer,buf-buffer) ;
	 cerr << endl ;
	 (void)getc(in) ;		// try to resynchronize
	 }
      if (ch == '\n' || ch == '\0')
	 break ;
      // swallow the byte-order markers, toggling the byte-swapping as needed
      if (ch == 0xFFFE)
	 {
	 byteswap = !byteswap ;
	 continue ;
	 }
      else if (ch == 0xFEFF)
	 continue ;
      FrStoreShort(ch,buf++) ;
      maxline-- ;
      }
   *buf = 0 ;
   return buffer ;
}

//----------------------------------------------------------------------

size_t Fr_wcslen(const FrChar16 *string)
{
   const FrChar16 *s = string ;
   while (*s)
      s++ ;
   return s-string ;
}

//----------------------------------------------------------------------

int Fr_Unicode_to_UTF8(FrChar16 codepoint, char *buffer, bool &byteswap)
{
   if (codepoint < 0x80)
      {
	 // encode as single byte
      *buffer = (unsigned char)codepoint ;
      return (codepoint ? 1 : 0) ;
      }
   else if (codepoint < 0x800)
      {
	 // encode in two bytes
      buffer[0] = (unsigned char)(0xC0 | ((codepoint & 0x07C0) >> 6)) ;
      buffer[1] = (unsigned char)(0x80 | (codepoint & 0x003F)) ;
      return 2 ;
      }
   else if (codepoint >= 0xD800 && codepoint < 0xE000)
      {
      // high or low surrogate, need to get second 16-bit value and call
      //  two-arg version of Fr_Unicode_to_UTF
      return -1 ;
      }
   else
      {
      if (codepoint == 0xFFFE)
	 {
	 byteswap = !byteswap ;
	 codepoint = 0xFEFF ;
	 }
	 // encode as three bytes
      buffer[0] = (unsigned char)(0xE0 | ((codepoint & 0xF000) >> 12)) ;
      buffer[1] = (unsigned char)(0x80 | ((codepoint & 0x0FC0) >> 6)) ;
      buffer[2] = (unsigned char)(0x80 | (codepoint & 0x003F)) ;
      return 3 ;
      }
}

//----------------------------------------------------------------------

int Fr_Unicode_to_UTF8(FrChar16 codepoint, FrChar16 codepoint2,
		       char *buffer, bool &byteswap)
{
   if (codepoint < 0x80)
      {
	 // encode as single byte
      *buffer = (unsigned char)codepoint ;
      return (codepoint ? 1 : 0) ;
      }
   else if (codepoint < 0x800)
      {
	 // encode in two bytes
      buffer[0] = (unsigned char)(0xC0 | ((codepoint & 0x07C0) >> 6)) ;
      buffer[1] = (unsigned char)(0x80 | (codepoint & 0x003F)) ;
      return 2 ;
      }
   else if (codepoint < 0xD800 || codepoint >= 0xE000)
      {
      if (codepoint == 0xFFFE)
	 {
	 byteswap = !byteswap ;
	 codepoint = 0xFEFF ;
	 }
	 // encode as three bytes
      buffer[0] = (unsigned char)(0xE0 | ((codepoint & 0xF000) >> 12)) ;
      buffer[1] = (unsigned char)(0x80 | ((codepoint & 0x0FC0) >> 6)) ;
      buffer[2] = (unsigned char)(0x80 | (codepoint & 0x003F)) ;
      return 3 ;
      }
   else
      {
      FrChar16 codepoint1 = (FrChar16)(codepoint & 0x3FF) ;
      codepoint2 &= 0x3FF ;
      if (codepoint >= 0xDC00)
	 // low surrogate
	 codepoint = (FrChar16)(((codepoint2 << 12) | codepoint1) + 0x10000) ;
      else
	 codepoint = (FrChar16)(((codepoint1 << 12) | codepoint2) + 0x10000) ;
      buffer[0] = (unsigned char)(0xF0 | (codepoint >> 18)) ;
      buffer[1] = (unsigned char)(0x80 | ((codepoint & 0x03F000) >> 12)) ;
      buffer[2] = (unsigned char)(0x80 | ((codepoint & 0x000FC0) >> 6)) ;
      buffer[3] = (unsigned char)(0x80 | (codepoint & 0x00003F)) ;
      return 4 ;
      }
}

//----------------------------------------------------------------------

char *Fr_Unicode_to_UTF8(const FrChar16 *input, bool &byteswap,
			 char *buffer)
{
   FrChar16 u ;
   do {
      u = byteswap ? (FrChar16)FrByteSwap16(*input) : *input ;
      if (u)
	 input++ ;
      size_t bytes = Fr_Unicode_to_UTF8(u,buffer,byteswap) ;
      if (bytes == 0)
	 {
	 FrChar16 u2 = byteswap ? (FrChar16)FrByteSwap16(*input) : *input ;
	 if (u2)
	    input++ ;
	 buffer += Fr_Unicode_to_UTF8(u,u2,buffer,byteswap) ;
	 }
      else
	 buffer += bytes ;
      } while (u) ;
   return buffer ;
}

//----------------------------------------------------------------------

char *Fr_Unicode_to_UTF8(const FrChar16 *input, bool &byteswap)
{
   // worst-case size of result is three bytes for each Unicode char
   size_t len = Fr_wcslen(input) ;
   char *buffer = FrNewN(char,(3*len+1)) ;
   if (buffer)
      {
      char *end = Fr_Unicode_to_UTF8(input,byteswap,buffer) ;
      return (char*)FrRealloc(buffer,end-buffer+1) ;
      }
   else
      {
      FrNoMemory("while converting UTF-16 string to UTF-8") ;
      return 0 ;
      }
}

//----------------------------------------------------------------------

FrChar16 *Fr_UTF8_to_Unicode(const char *input, FrChar16 *result)
{
   while (*input)
      {
      unsigned char u = (unsigned char)*input++ ;
      if (u < 0x80)
	 {
	 FrStoreShort(u,result) ;
	 if (u)
	    result++ ;
	 }
      else if ((u & 0xE0) == 0xC0)
	 {
	 unsigned char low = (unsigned char)*input++ ;
	 if (!low)
	    break ;
	 u &= 0x1F ;
	 low &= 0x3F ;
	 FrStoreShort((u<<6)|low,result++) ;
	 }
      else if ((u & 0xF0) == 0xE0)
	 {
	 unsigned char mid = (unsigned char)*input++ ;
	 if (!mid)
	    break ;
	 unsigned char low = (unsigned char)*input++ ;
	 if (!low)
	    break ;
	 u &= 0x0F ;
	 mid &= 0x3F ;
	 low &= 0x3F ;
	 FrStoreShort((u<<12)|(mid<<6)|low,result++) ;
	 }
      else if ((u & 0xF8) == 0xF0)
	 {
	 // code value bigger than 16 bits; split into two surrogate chars
	 unsigned hi = (unsigned char)*input++ ;
	 u &= 0x07 ;
	 if (!hi)
	    break ;
	 unsigned char mid = (unsigned char)*input++ ;
	 hi &= 0x3F ;
	 if (!mid)
	    break ;
	 unsigned char low = (unsigned char)*input++ ;
	 mid &= 0x3F ;
	 if (!low)
	    break ;
	 low &= 0x3F ;
	 uint32_t codepoint = (u << 18) | (hi << 12) | (mid << 6) | low ;
	 FrStoreShort(0xD800+(codepoint >> 10),result++) ;
	 FrStoreShort(0xDC00+(codepoint & 0x03FF),result++) ;
	 }
      else if ((u & 0xFC) == 0xF8)
	 {
	 // five-byte encoding
	 FrWarning("five-byte UTF-8 encoding not yet supported; char skipped") ;
	 // skip next four bytes, unless we hit EOS
	 for (size_t i = 0 ; i < 4 ; i++)
	    {
	    if (*input)
	       input++ ;
	    }
	 }
      else if ((u & 0xFE) == 0xFC)
	 {
	 // six-byte encoding
	 FrWarning("six-byte UTF-8 encoding not yet supported; char skipped") ;
	 // skip next five bytes, unless we hit EOS
	 for (size_t i = 0 ; i < 5 ; i++)
	    {
	    if (*input)
	       input++ ;
	    }
	 }
      else
	 FrWarningVA("invalid UTF-8 byte %2.02X found in Fr_UTF8_to_Unicode",
		     u) ;
      }
   return result ;
}

//----------------------------------------------------------------------

FrChar16 *Fr_UTF8_to_Unicode(const char *input)
{
   FrChar16 *buffer = FrNewN(FrChar16,strlen(input)+1) ;
   if (buffer)
      {
      FrChar16 *end = Fr_UTF8_to_Unicode(input,buffer) ;
      *end = 0 ;			// ensure proper termination
      return FrNewR(FrChar16,buffer,end-buffer+1) ;
      }
   else
      {
      FrNoMemory("while converting UTF-8 string to UTF-16") ;
      return 0 ;
      }
}

//----------------------------------------------------------------------

size_t Fr_UTF8len(const char *string)
{
   size_t count = 0 ;
   if (string)
      {
      while (*string)
	 {
	 unsigned char u = (unsigned char)*string++ ;
	 if (u < 0x80)
	    count++ ;
	 else if ((u & 0xE0) == 0xC0)
	    {
	    count += 2 ;
	    string++ ;
	    }
	 else if ((u & 0xF0) == 0xE0)
	    {
	    count += 3 ;
	    string += 2 ;
	    }
	 else if ((u & 0xF8) == 0xF8)
	    {
	    count += 4 ;
	    string += 3 ;
	    }
	 else
	    {
	    // oops, invalid code in string!
	    FrWarningVA("invalid UTF-8 byte %2.02X found in Fr_UTF8len()",u) ;
	    }
	 }
      }
   return count ;
}

//----------------------------------------------------------------------

static int read_UCS2_codepoint(istream &in, bool byteswap,
			      const char *bufstart, const char *bufptr)
{
   int c1 = in.get() ;
   if (c1 == EOF)
      return EOF ;
   int c2 = in.get() ;
   if (c2 == EOF)
      return EOF ;
   int ch ;
   if (byteswap)
      ch = ((c2 & 0xFF) << 8) | (c1 & 0xFF) ;
   else
      ch = ((c1 & 0xFF) << 8) | (c2 & 0xFF) ;
   if (FramepaC_check_Unicode_corruption &&
       (ch == 0x0a00 || ch == 0x2000))
      {
      cerr << "The Unicode character stream appears to have been corrupted "
	    "near\n" ;
      cerr.write((char*)bufstart,bufptr-bufstart) ;
      cerr << endl ;
      in.get() ;			// try to resynchronize
      }
   return ch ;
}

//----------------------------------------------------------------------

char *Fr_utf8gets(istream &in, char *buffer, size_t maxline, bool &byteswap)
{
   char *buf = buffer ;
   if (maxline == 0)
      return 0 ;
   else
      maxline-- ;			// reserve space for the terminator
   while (maxline > 3)
      {
      int ch = read_UCS2_codepoint(in,byteswap,buffer,buf) ;
      if (ch == EOF)
	 {
	 if (buf == buffer)
	    buffer = 0 ;		// indicate error condition
	 break ;
	 }
      if (ch == '\n' || ch == '\0')
	 break ;
      size_t bytes = Fr_Unicode_to_UTF8((FrChar16)ch,buffer,byteswap) ;
      if (bytes == 0)
	 {
	 FrChar16 ch2 = (FrChar16)read_UCS2_codepoint(in,byteswap,buffer,buf) ;
	 bytes = Fr_Unicode_to_UTF8((FrChar16)ch,ch2,buffer,byteswap) ;
	 }
      buf += bytes ;
      maxline -= bytes ;
      }
   *buf = 0 ;
   return buffer ;
}

//----------------------------------------------------------------------

static int read_UCS2_codepoint(FILE *in, bool byteswap, const char *bufstart,
			      const char *bufptr)
{
   int c1 = getc(in) ;
   if (c1 == EOF)
      return EOF ;
   int c2 = getc(in) ;
   if (c2 == EOF)
      return EOF ;
   int ch ;
   if (byteswap)
      ch = ((c2 & 0xFF) << 8) | (c1 & 0xFF) ;
   else
      ch = ((c1 & 0xFF) << 8) | (c2 & 0xFF) ;
   if (FramepaC_check_Unicode_corruption &&
       (ch == 0x0a00 || ch == 0x2000))
      {
      cerr << "The Unicode character stream appears to have been corrupted "
	    "near\n" ;
      cerr.write((char*)bufstart,bufptr-bufstart) ;
      cerr << endl ;
      (void)getc(in) ;			// try to resynchronize
      }
   return ch ;
}

//----------------------------------------------------------------------

char *Fr_utf8gets(FILE *in, char *buffer, size_t maxline, bool &byteswap)
{
   char *buf = buffer ;
   if (maxline == 0)
      return 0 ;
   else
      maxline-- ;			// reserve space for the terminator
   while (maxline > 3)
      {
      int ch = read_UCS2_codepoint(in,byteswap,buffer,buf) ;
      if (ch == EOF)
	 {
	 if (buf == buffer)
	    buffer = 0 ;		// indicate error condition
	 break ;
	 }
      if (ch == '\n' || ch == '\0')
	 break ;
      size_t bytes = Fr_Unicode_to_UTF8((FrChar16)ch,buffer,byteswap) ;
      if (bytes == 0)
	 {
	 FrChar16 ch2 = (FrChar16)read_UCS2_codepoint(in,byteswap,buffer,buf) ;
	 bytes = Fr_Unicode_to_UTF8((FrChar16)ch,ch2,buffer,byteswap) ;
	 }
      buf += bytes ;
      maxline -= bytes ;
      }
   *buf = 0 ;
   return buffer ;
}

// end of file frunicod.cpp //
