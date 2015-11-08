/************************************************************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File: frabbrev.cpp	 	abbreviations support for canonicaliz	*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 2004,2009,2010,2015					*/
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

#include <errno.h>
#include <string.h>
#include "frstring.h"
#include "frfilutl.h"
#include "frutil.h"

/************************************************************************/
/************************************************************************/

bool FrIsKnownAbbreviation(const char *start, const char *end,
			   char const * const *abbrevs)
{
   if (!abbrevs)
      return false ;
   for ( ; *abbrevs ; abbrevs++)
      {
      size_t len = strlen(*abbrevs) ;
      if (len == (size_t)(end - start) && memcmp(start,*abbrevs,len) == 0)
	 return true ;
      }
   return false ;
}

//----------------------------------------------------------------------

static char *next_abbreviation(FILE *fp)
{
   static char line[FrMAX_LINE] ;
   if (feof(fp) || !fgets(line,sizeof(line),fp))
      return 0 ;
   char *lineptr = line ;
   FrSkipWhitespace(lineptr) ;
   // strip off comments
   char *cmt = strchr(lineptr,';') ;
   if (cmt)
      *cmt = '\0' ;
   // use only the first token on the line
   for (char *lp = lineptr ; *lp ; lp++)
      {
      if (Fr_isspace(*lp))
	 {
	 *lp = '\0' ;
	 break ;
	 }
      }
   return lineptr ;
}

//----------------------------------------------------------------------

char **FrLoadAbbreviationList(FILE *fp)
{
   size_t num_abbrevs = 0 ;
   char *abbr ;
   while ((abbr = next_abbreviation(fp)) != 0)
      {
      if (*abbr)
	 num_abbrevs++ ;
      }
   char **abbrevs = FrNewN(char*,num_abbrevs+1) ;
   if (abbrevs)
      {
      fseek(fp,0L,SEEK_SET) ;
      size_t count = 0 ;
      while ((abbr = next_abbreviation(fp)) != 0)
	 {
	 if (*abbr)
	    {
	    size_t len = strlen(abbr) ;
	    char *a = FrNewN(char,len+1) ;
	    if (a)
	       {
	       memcpy(a,abbr,len+1) ;
	       abbrevs[count++] = a ;
	       }
	    }
	 }
      abbrevs[count] = nullptr ;
      }
   else
      errno = ENOMEM ;
   return abbrevs ;
}

//----------------------------------------------------------------------

char **FrLoadAbbreviationList(const char *filename)
{
   if (!filename || !*filename)
      {
      errno = EINVAL ;
      return 0 ;
      }
   FILE *fp = fopen(filename,"r") ;
   if (!fp)
      return 0 ;
   char **abbr = FrLoadAbbreviationList(fp) ;
   fclose(fp) ;
   return abbr ;
}

//----------------------------------------------------------------------

void FrFreeAbbreviationList(char **abbrevs)
{
   if (abbrevs)
      {
      for (char **abbr = abbrevs ; *abbr ; abbr++)
	 FrFree(*abbr) ;
      FrFree(abbrevs) ;
      }
   return ;
}

// end of file frabbrev.cpp //
