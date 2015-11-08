/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File: frsntbrk.cpp		sentence-splitting functions		*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 2000,2001,2002,2009					*/
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

#include <string.h>
#include "frctype.h"
#include "frlist.h"
#include "frstring.h"
#include "frutil.h"

/************************************************************************/
/*	Global Variables						*/
/************************************************************************/

static FrList *nonbreaking_abbrevs = 0 ;

/************************************************************************/
/*	Helper Functions						*/
/************************************************************************/

static bool nonbreaking_abbreviation(FrString *word, FrCharEncoding encoding)
{
   if (!word || !nonbreaking_abbrevs)
      return false ;
   word->lowercaseString(encoding) ;
   for (const FrList *nba = nonbreaking_abbrevs ; nba ; nba = nba->rest())
      {
      FrString *abbr = (FrString*)nba->first() ;
      if (*word == *abbr)
	 return true ;
      }
   return false ;			// no match
}

/************************************************************************/
/************************************************************************/

const char *FrSentenceBreak(const char *line, FrCharEncoding encoding,
			    size_t max_words)
{
   size_t numwords = 0 ;
   const char *brkpt ;
   do {
      // find first period, exclamation mark, or question mark in line
      brkpt = line ;
      char prevc = ' ' ;
      for ( ; *brkpt ; brkpt++)
	 {
	 char c = *brkpt ;
	 // paragraph breaks are automatically sentence breaks as well
	 if ((c == '\n' && brkpt[1] == '\n') ||
	     c == '.' || c == '!' || c == '?' || c == ':')
	    break ;
	 if (!Fr_isspace(prevc) && Fr_isspace(c))
	    {
	    numwords++ ;
	    if (numwords >= max_words)
	       {
	       FrSkipWhitespace(brkpt) ;
	       return brkpt ;
	       }
	    }
	 prevc = c ;
	 }
      if (!*brkpt)
	 break ;
      // if we found one of the above, and it is followed by space (i.e.
      // not the very last thing on the line, not embedded in a number,
      // etc.)
      if (brkpt[1] && Fr_isspace(brkpt[1]))
	 {
	 const char *endwhite = brkpt+2 ;
	 while (*endwhite &&
		(Fr_isspace(*endwhite) || *endwhite == '"' ||
		 *endwhite == '\''))
	    endwhite++ ;
	 // can only be a sentence break if following nonwhite char is not
	 //   lowercase and not another possible sentence break, or it was
	 //   a question mark or exclamation point at the end of the line
	 if ((!*endwhite && *brkpt != '.') ||
	     (*endwhite && !Fr_islower(*endwhite) && !Fr_isdigit(*endwhite) &&
	      !strchr(".,;:!?-",*endwhite)))
	    {
	    // OK, we've passed the tests so far, now the final one: it must
	    // not be a period following one of the abbreviations we've
	    // defined as NOT breaking a sentence
	    const char *end = brkpt-1 ;
	    while (end > line && Fr_isspace(*end))
	       end-- ;
	    const char *start = end ;
	    while (start > line &&
		   (Fr_isupper(start[-1]) || Fr_islower(start[-1])))
	       start-- ;
	    if (start == end && Fr_isupper(*start) && *brkpt == '.')
	       {}			// don't break on an initial
	    else
	       {
	       FrString *word = new FrString(start,end-start+1) ;
	       bool dobreak = !nonbreaking_abbreviation(word,encoding) ;
	       free_object(word) ;
	       if (dobreak)
		  return endwhite ;
	       }
	    }
	 }
      line = brkpt + 1 ;
      } while (*brkpt) ;
   return 0 ;				// no sentence break found
}

//----------------------------------------------------------------------

const FrChar16 *FrSentenceBreak(const FrChar16 *line)
{
   const FrChar16 *brkpt ;
   do {
      // find first period, exclamation mark, or question mark in line
      brkpt = line ;
      while (*brkpt && (*brkpt != '.' && *brkpt != '!' &&
			*brkpt != '?'))
	 brkpt++ ;
      if (!*brkpt)
	 break ;
      // if we found one of the above, and it is followed by space (i.e.
      // not the very last thing on the line, not embedded in a number,
      // etc.)
      if (brkpt[1] && Fr_iswspace(brkpt[1]))
	 {
	 const FrChar16 *endwhite = brkpt+2 ;
	 while (*endwhite &&
		(Fr_iswspace(*endwhite) || *endwhite == '"' ||
		 *endwhite == '\''))
	    endwhite++ ;
	 // can only be a sentence break if following nonwhite char is not
	 //   lowercase and not another possible sentence break, or it was
	 //   a question mark or exclamation point at the end of the line
	 if ((!*endwhite && *brkpt != '.') ||
	     (*endwhite && !Fr_iswlower(*endwhite) &&
	      !strchr(".,;:!?",*endwhite)))
	    {
	    // OK, we've passed the tests so far, now the final one: it must
	    // not be a period following one of the abbreviations we've
	    // defined as NOT breaking a sentence
	    const FrChar16 *end = brkpt-1 ;
	    while (end > line && Fr_iswspace(*end))
	       end-- ;
	    const FrChar16 *start = end ;
	    while (start > line &&
		   (Fr_iswupper(start[-1]) || Fr_iswlower(start[-1])))
	       start-- ;
	    if (start == end && Fr_isupper(*start) && *brkpt == '.')
	       {}			// don't break on an initial
	    else
	       {
	       FrString *word = new FrString(start,end-start+1) ;
	       bool dobreak = !nonbreaking_abbreviation(word,
							  FrChEnc_Unicode) ;
	       free_object(word) ;
	       if (dobreak)
		  return endwhite ;
	       }
	    }
	 }
      line = brkpt + 1 ;
      } while (*brkpt) ;
   return 0 ;				// no break found
}

//----------------------------------------------------------------------

FrList *FrBreakIntoSentences(const char *line, FrCharEncoding encoding)
{
   FrList *sentences = 0 ;
   FrList **end = &sentences ;
   while (line && *line)
      {
      FrSkipWhitespace(line) ;
      const char *brk ;
      if (encoding == FrChEnc_Unicode)
	 brk = (char*)FrSentenceBreak((FrChar16*)line) ;
      else
	 brk = FrSentenceBreak(line,encoding) ;
      if (brk)
	 sentences->pushlistend(new FrString(line,brk-line),end) ;
      else
	 sentences->pushlistend(new FrString(line),end) ;
      line = brk ;
      }
   *end = 0 ;				// properly terminate the result list
   return sentences ;
}

//----------------------------------------------------------------------

void FrSetNonbreakingAbbreviations(const FrList *abbrevs, FrCharEncoding enc)
{
   free_object(nonbreaking_abbrevs) ;
   nonbreaking_abbrevs = 0 ;
   for ( ; abbrevs ; abbrevs = abbrevs->rest())
      {
      FrObject *abbr = abbrevs->first() ;
      if (abbr && abbr->stringp())
	 {
	 FrString *copy = (FrString*)abbr->deepcopy() ;
	 copy->lowercaseString(enc) ;
	 pushlist(copy,nonbreaking_abbrevs) ;
	 }
      }
   nonbreaking_abbrevs = listreverse(nonbreaking_abbrevs) ;
   return ;
}

// end of file frsntbrk.cpp //
