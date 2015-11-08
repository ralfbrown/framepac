/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frfilut2.cpp		more file-access utility functions	*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1996,1997,1998,1999,2000,2001,2003,2004,2009,2010,	*/
/*		2011 Ralf Brown/Carnegie Mellon University		*/
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
#include "frfilutl.h"
#include "frmem.h"
#include "frstring.h"
#include "frunicod.h"
#include "frutil.h" // for FrTrimWhitespace

/************************************************************************/
/************************************************************************/

char *FrReadCanonLine(FILE *fp, bool skip_blank, FrCharEncoding enc,
		      bool *Unicode_bswap, bool *leading_whitespace,
		      bool force_upper, const FrCasemapTable charmap,
		      const char *delim, char const * const *abbrevs)
{
   if (!fp)
      return 0 ;
   char line[FrMAX_LINE] ;
   char *result = 0 ;
   memset(line,'\0',sizeof(line)) ;	// keep Purify and ValGrind happy
   do {
      if (result)
	 FrFree(result) ;
      line[0] = '\0' ;
      // read lines until we get a non-blank one.  This allows us to
      // read files with extraneous blank lines
      if (enc == FrChEnc_Unicode)
	 {
	 bool bswap = false ;
	 if (!Unicode_bswap)
	    Unicode_bswap = &bswap ;
	 bool oldcheck = FramepaC_check_Unicode_corruption ;
	 FramepaC_check_Unicode_corruption = true ;
	 bool success = Fr_utf8gets(fp,line,sizeof(line),*Unicode_bswap)!=0 ;
	 FramepaC_check_Unicode_corruption = oldcheck ;
	 if (!success)
	    return 0 ;
	 }
      else if (!fgets(line,sizeof(line),fp))
	 return 0 ;
      // check whether the input line was longer than our buffer; if so,
      //   issue a warning and dump the rest of the line
      size_t len = strlen(line) ;
      if (len == sizeof(line)-1 && line[len-1] != '\n')
	 {
	 FrWarningVA("input line longer than %lu chars; remainder skipped",
		     (unsigned long)(sizeof(line)-1)) ;
	 if (enc == FrChEnc_Unicode)
	    {
	    FrChar16 *line16 = (FrChar16*)line ;
	    while (!feof(fp) && Fr_ugets(fp,(FrChar16*)line,2,
					 *Unicode_bswap) &&
		   *line16 != '\n')
	       ;
	    }
	 else
	    {
	    while (!feof(fp) && fgetc(fp) != '\n')
	       ;
	    }
	 }
      if (leading_whitespace)
	 *leading_whitespace = (bool)Fr_isspace(line[0]) ;
      // strip trailing whitespace, giving us an empty line if the line
      // consists entirely of whitespace
      char *lineptr = FrTrimWhitespace(line) ;
      if (charmap)
	 FrMapString(line,charmap) ;
      if (lineptr[0] != '\0')
	 result = FrCanonicalizeSentence(line,enc,force_upper,delim,abbrevs) ;
      else if (!skip_blank)
	 result = FrNewC(char,1) ;
      } while (skip_blank && (!result || !*result)) ;
   return result ;			// successfully read a non-empty line
}

//----------------------------------------------------------------------

char *FrReadCanonLine(FILE *fp, bool skip_blank, bool *Unicode_bswap,
		      bool *leading_whitespace, bool force_upper,
		      const FrCasemapTable charmap, const char *delim,
		      char const * const *abbrevs)
{
   return FrReadCanonLine(fp,skip_blank,FrChEnc_Latin1,Unicode_bswap,
			  leading_whitespace,force_upper,charmap,delim,
			  abbrevs) ;
}

// end of frfilut2.cpp //
