/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frfilut3.cpp		more file-access utility functions	*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1998,2000,2004,2005,2006,2009,2015			*/
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

#include <ctype.h>
#include "frctype.h"
#include "frfilutl.h"

/************************************************************************/
/*	Manifest Constants for this module				*/
/************************************************************************/

#define BUFFER_SIZE 1024

/************************************************************************/
/************************************************************************/

static FrFILETYPE check_type(const char *buf, size_t buflen)
{
   size_t i ;
   if (buflen < 1)
      return FrFT_UNKNOWN ;		// no data, so can't check!
   // first, check for a Unicode marker; if present, this is definitely
   // a Unicode text file
   if (buflen >= 2 &&
       ((buf[0] == '\376' && buf[1] == '\377') ||
	(buf[0] == '\377' && buf[1] == '\376')))
      return FrFT_UNICODE ;
   // if the file starts with a UTF8-encoded Unicode byte-order-marker,
   //   we can declare it to be in UTF8
   if (buflen >= 3 && buf[0] == '\xEF' && buf[1] == '\xBB' && buf[2] == '\xBF')
      return FrFT_UTF8 ;
   // next, check whether there are any control characters other than
   // CR, LF, or Tab; if not, this is a plain ASCII text file
   bool nonASCII = false ;
   for (i = 0 ; i < buflen ; i++)
      {
      unsigned char c = (unsigned char)buf[i] ;
      if (/*iscntrl(c)*/ c < ' ' && c != '\t' && c != '\n' && c != '\r')
	 {
	 nonASCII = true ;
	 break ;
	 }
      }
   if (!nonASCII)
      return FrFT_ASCII ;
   // OK, we've got either a binary file or a Unicode file
   // try to distinguish by checking for spaces or newlines immediately
   // preceded or followed by a NUL
   size_t uniNL = 0 ;
   size_t uniSpace = 0 ;
   char prev = 0xFF ;
   for (i = 0 ; i < buflen ; i++)
      {
      char c = buf[i] ;
      char next = trunc2char((i+1 < buflen) ? buf[i+1] : '\xFF') ;
      if (c == ' ' || c == '\t')
	 { if (prev == '\0' || next == '\0') uniSpace++ ; }
      if (c == '\n')
	 { if (prev == '\0' || next == '\0') uniNL++ ; }
      prev = c ;
      }
   if (uniNL > 1 && uniSpace > 3)
      return FrFT_UNICODE ;
   return FrFT_BINARY ;
}

//----------------------------------------------------------------------

FrFILETYPE FrFileType(FILE *fp)
{
   char buf[BUFFER_SIZE] ;
   off_t pos = ftell(fp) ;
   fseek(fp,0,SEEK_SET) ;
   int count = fread(buf,1,sizeof(buf),fp) ;
   FrFILETYPE type = FrFT_UNKNOWN ;
   if (count > 0)
      type = check_type(buf,(size_t)count) ;
   fseek(fp,pos,SEEK_SET) ;
   return type ;
}

//----------------------------------------------------------------------

FrFILETYPE FrFileType(const char *filename)
{
   FILE *fp = fopen(filename,FrFOPEN_READ_MODE) ;
   if (fp)
      {
      FrFILETYPE type = FrFileType(fp) ;
      fclose(fp) ;
      return type ;
      }
   else
      return FrFT_UNKNOWN ;
}

//----------------------------------------------------------------------

FrFILETYPE FrFileType(istream &in)
{
   char buf[BUFFER_SIZE] ;
   long pos = in.tellg() ;
   in.seekg(0) ;
   (void)in.read(buf,sizeof(buf)) ;
   FrFILETYPE type = check_type(buf,in.gcount()) ;
   in.seekg(pos) ;
   return type ;
}

//----------------------------------------------------------------------

// end of frfilut3.cpp //
