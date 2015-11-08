/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File: frstrut1.cpp	 	string-manipulation utility functions	*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,1998,1999,2000,2002,2003,2004,	*/
/*		2007,2009,2010 Ralf Brown/Carnegie Mellon University	*/
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
#include "frutil.h"
#include "frpcglbl.h"

/************************************************************************/
/*	Global/External Variables for this module			*/
/************************************************************************/

extern bool FramepaC_PnP_mode ;

/************************************************************************/
/************************************************************************/

//----------------------------------------------------------------------
// convert a string containing a one-line sentence into a list of strings,
// one per word or other token (i.e. punctuation) in the original sentence
// (note: identical to gloss_string_to_wordlist in ebutil.cpp except that the
//  latter uses possible_gloss_delimiters[] instead of
//  FrStdWordDelimiters(Latin1) )

FrList *FrCvtString2Wordlist(const char *sentence, const char *delim,
			     char const * const *abbrevs, FrCharEncoding enc)
{
   if (!sentence || !*sentence)
      return 0 ;
   if (!delim)
      delim = FrStdWordDelimiters(enc) ;
   const unsigned char *alphachars = FrStdIsAlphaTable(enc) ;
   while (Fr_isspace(*sentence))
      sentence++ ;			// skip leading whitespace
   const char *end = strchr(sentence,'\0') ;
   if (end != sentence)
      end-- ;
   if (*end == '\n')	// strip trailing newline if present
      end-- ;
   while (end >= sentence && Fr_isspace(*end)) // strip trailing whitespace
      end-- ;
   FrList *words = 0 ;
   FrList **w_end = &words ;
   while (*sentence && sentence <= end)
      {
      while (Fr_isspace(*sentence))
	 sentence++ ;			// skip leading whitespace
      const char *start = sentence ;	// remember start of word
      while (sentence <= end && !delim[*(unsigned char*)sentence])
	 sentence++ ;
      switch (*sentence)
	 {
	 case '\'':			// check if contraction or doubled
	    if (sentence == start)	// at start of word, must be a quote
	       {
	       sentence++ ;
	       if (*sentence == '\'')	// doubled?
		  {
		  sentence++ ;
		  words->pushlistend(new FrString("''"),w_end) ;
		  }
	       else
		  words->pushlistend(new FrString("'"),w_end) ;
	       }
	    else
	       {
	       if (sentence < end &&
		   (((unsigned char*)sentence)[1] < 0x80 ||
		    delim['\xB0']) &&	// Latin-1/2 ?
		   alphachars[(unsigned char)sentence[1]])
		  {  // contraction!
		  sentence += 2 ;
		  // include the rest of the word
		  while (alphachars[(unsigned char)*sentence])
		     sentence++ ;
		  }
	       words->pushlistend(new FrString(start,sentence-start),w_end) ;
	       }
	    break ;
	 case '`':			// check if double back-quote
	    if (sentence == start)
	       {
	       sentence++;
	       if (*sentence == '`')	// doubled?
		  {
		  sentence++ ;
		  words->pushlistend(new FrString("``"),w_end) ;
		  }
	       else
		  words->pushlistend(new FrString("`"),w_end) ;
	       }
	    else
	       words->pushlistend(new FrString(start,sentence-start),w_end) ;
	    break ;
	 case '.':		// special handling for numbers and abbrevs
	 case ',':
	    if (sentence == start)
	       sentence++ ;	// ensure a non-empty word
	    else if (Fr_isdigit(sentence[-1]) && Fr_isdigit(sentence[1]))
	       {		// add the rest of the number
	       sentence += 2 ;
	       do {
		  while (sentence <= end && Fr_isdigit(*sentence))
		     sentence++ ;
		  if ((*sentence == '.' || *sentence == ',') &&
		      Fr_isdigit(sentence[1]))
		     sentence += 2 ;
		  else
		     break ;
		  } while (*sentence) ;
	       }
	    else if (*sentence == '.')
	       {
	       if (alphachars[(unsigned char)sentence[-1]] &&
		   alphachars[(unsigned char)sentence[1]])
		  {
		  // must be an abbreviation with periods, so read remainder
		  sentence += 2 ;
		  while (sentence <= end &&
			 (!delim[*(unsigned char*)sentence] || *sentence=='.'))
		     sentence++ ;
		  }
	       else if ((Fr_isspace(sentence[1]) || sentence[1] == '\0') &&
			FrIsKnownAbbreviation(start,sentence,abbrevs))
		  sentence++ ;		// include period in word
	       }
	    words->pushlistend(new FrString(start,sentence-start),w_end) ;
	    break ;
	 case '-':			// check if m-dash or hyphen
	    if (sentence == start)	// at start of word, is dash or minus
	       {
	       sentence++ ;
	       if (*sentence == '-')	// check if doubled
		  sentence++ ;
	       else
		  {			// grab any digits from negative number
		  while (sentence <= end && Fr_isdigit(*sentence))
		     sentence++ ;
		  if ((*sentence == '.' || *sentence == ',') &&
		      Fr_isdigit(sentence[1]))
		     {
		     sentence += 2 ;
		     while (sentence <= end && Fr_isdigit(*sentence))
			sentence++ ;
		     }
		  }
	       words->pushlistend(new FrString(start,sentence-start),w_end) ;
	       }
	    else if (Fr_isdigit(sentence[-1]))
	       {
	       // OK, we have a number range (123-456) or numeric prefix
	       // (12-ton), so split it in three
	       words->pushlistend(new FrString(start,sentence-start),w_end) ;
	       words->pushlistend(new FrString("-",1),w_end) ;
	       sentence++ ;		// consume the dash
	       }
	    else if (!delim[((unsigned char*)sentence)[1]])
	       {
	       sentence++ ;
	       // grab rest of hyphenated word
	       while (sentence <= end &&
		      (!delim[*(unsigned char*)sentence] ||
		       *sentence == '-'))
		  sentence++ ;
	       words->pushlistend(new FrString(start,sentence-start),w_end) ;
	       }
	    else
	       words->pushlistend(new FrString(start,sentence-start),w_end) ;
	    break ;
	 case ':':
	 case '/':
	    if (FramepaC_PnP_mode)
	       {
	       //treat as normal character inside glossary variable or marker
	       // else make it a word
	       bool in_gloss_var = false ;
	       for (const char *tmp = start ; tmp < sentence ; tmp++)
		  if (*tmp == '<')
		     {
		     in_gloss_var = true ;
		     break ;
		     }
	       if (in_gloss_var)
		  {
		  sentence++ ;		// consume the colon or slash
		  // advance up to & including the greater-than sign
		  while (*sentence && *sentence != '>' &&
			 !Fr_isspace(*sentence))
		     sentence++ ;
		  if (*sentence && !Fr_isspace(*sentence))
		     sentence++ ;
		  }
	       else if (sentence == start)
		  sentence++ ;
	       words->pushlistend(new FrString(start,sentence-start),w_end) ;
	       break ;
	       }
	    else if (sentence > start &&
		     (sentence[-1] == '<' || sentence[1] == '>'))
	       {
	       sentence++ ;
	       break ;
	       }
	    // fall through
	 default:
	    if (sentence == start)
	       sentence++ ;		// ensure at least one char in 'word'
	    words->pushlistend(new FrString(start,sentence-start),w_end) ;
	    break ;
	 }
      }
   *w_end = 0 ;				// properly terminate the word list
   return words ;
}

//----------------------------------------------------------------------
// convert a sentence with words delimited by single blanks into a list
// of strings, one per word in the original sentence

FrList *FrCvtSentence2Wordlist(const char *sentence)
{
   if (!sentence || !*sentence)
      return 0 ;
   const char *end = strchr(sentence,'\0') ;
   if (end[-1] == '\n') // strip trailing newline if present
      end-- ;
   FrList *words = 0 ;
   FrList **w_end = &words ;
   for (const char *blank = sentence ; blank < end ; blank++)
      {
      if (*blank == ' ')
	 {
	 words->pushlistend(new FrString(sentence,blank-sentence),w_end) ;
	 sentence = blank + 1 ;
	 }
      }
   if (*sentence && sentence < end)
      words->pushlistend(new FrString(sentence,end-sentence),w_end) ;
   *w_end = 0 ;				// properly terminate the result
   return words ;
}

// end of file frstrut1.cpp //
