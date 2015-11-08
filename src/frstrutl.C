/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File: frstrutl.cpp	 	string-manipulation utility functions	*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,1998,1999,2000,2002,2003,2004,	*/
/*		2006,2007,2009 Ralf Brown/Carnegie Mellon University	*/
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
#include "frmem.h"
#include "frstring.h"
#include "frpcglbl.h"

/************************************************************************/
/*    Global data for this module					*/
/************************************************************************/


// list the characters at which we might want to break a word when splitting
// a sentence into a word list
// (Note: this table assumes that data files use ASCII or the ANSI/Latin-1
//  superset thereof!)
// (Note2: should be identical to table in fruniutl.cpp)
static const char FramepaC_possible_delimiter[] =
   {
      1, 1, 1, 1, 1, 1, 1, 1,	1, 1, 1, 1, 1, 1, 1, 1,	   // ^@ to ^O
      1, 1, 1, 1, 1, 1, 1, 1,	1, 1, 1, 1, 1, 1, 1, 1,	   // ^P to ^_
      1, 1, 1, 1, 1, 1, 1, 1,	1, 1, 1, 1, 1, 1, 1, 1,	   // SP to /
      0, 0, 0, 0, 0, 0, 0, 0,	0, 0, 1, 1, 0, 1, 0, 1,	   //  0 to ?
      1, 0, 0, 0, 0, 0, 0, 0,	0, 0, 0, 0, 0, 0, 0, 0,	   //  @ to O
      0, 0, 0, 0, 0, 0, 0, 0,	0, 0, 0, 1, 1, 1, 1, 0,	   //  P to _
      1, 0, 0, 0, 0, 0, 0, 0,	0, 0, 0, 0, 0, 0, 0, 0,	   //  ` to o
      0, 0, 0, 0, 0, 0, 0, 0,	0, 0, 0, 1, 1, 1, 1, 0,	   //  p to DEL
      0, 0, 1, 0, 1, 1, 0, 0,	0, 1, 0, 0, 0, 0, 0, 0,	   // 0x80 to 0x8F
      0, 1, 1, 1, 1, 0, 0, 0,	0, 0, 0, 0, 0, 0, 0, 0,	   // 0x90 to 0x9F
      1, 1, 0, 1, 0, 1, 0, 1,	0, 0, 0, 1, 0, 0, 0, 0,	   // 0xA0 to 0xAF
      1, 0, 0, 0, 0, 1, 1, 1,	1, 0, 0, 1, 0, 0, 0, 1,	   // 0xB0 to 0xBF
      0, 0, 0, 0, 0, 0, 0, 0,	0, 0, 0, 0, 0, 0, 0, 0,	   // 0xC0 to 0xCF
      0, 0, 0, 0, 0, 0, 0, 0,	0, 0, 0, 0, 0, 0, 0, 0,	   // 0xD0 to 0xDF
      0, 0, 0, 0, 0, 0, 0, 0,	0, 0, 0, 0, 0, 0, 0, 0,	   // 0xE0 to 0xEF
      0, 0, 0, 0, 0, 0, 0, 0,	0, 0, 0, 0, 0, 0, 0, 0,	   // 0xF0 to 0xFF
   } ;

// 0xA3 = pound sterling

// list the characters at which we might want to break a word when splitting
// a sentence into a word list
// (Note: this table assumes that data files use ASCII or the ANSI/Latin-1
//  superset thereof!)
// (Note2: should be identical to table in fruniutl.cpp)
static const char FramepaC_possible_delimiter_EUC[] =
   {
      1, 1, 1, 1, 1, 1, 1, 1,	1, 1, 1, 1, 1, 1, 1, 1,	   // ^@ to ^O
      1, 1, 1, 1, 1, 1, 1, 1,	1, 1, 1, 1, 1, 1, 1, 1,	   // ^P to ^_
      1, 1, 1, 1, 1, 1, 1, 1,	1, 1, 1, 1, 1, 1, 1, 1,	   // SP to /
      0, 0, 0, 0, 0, 0, 0, 0,	0, 0, 1, 1, 0, 1, 0, 1,	   //  0 to ?
      1, 0, 0, 0, 0, 0, 0, 0,	0, 0, 0, 0, 0, 0, 0, 0,	   //  @ to O
      0, 0, 0, 0, 0, 0, 0, 0,	0, 0, 0, 1, 1, 1, 1, 0,	   //  P to _
      1, 0, 0, 0, 0, 0, 0, 0,	0, 0, 0, 0, 0, 0, 0, 0,	   //  ` to o
      0, 0, 0, 0, 0, 0, 0, 0,	0, 0, 0, 1, 1, 1, 1, 0,	   //  p to DEL
      0, 0, 0, 0, 0, 0, 0, 0,	0, 0, 0, 0, 0, 0, 0, 0,	   // 0x80 to 0x8F
      0, 0, 0, 0, 0, 0, 0, 0,	0, 0, 0, 0, 0, 0, 0, 0,	   // 0x90 to 0x9F
      0, 0, 0, 0, 0, 0, 0, 0,	0, 0, 0, 0, 0, 0, 0, 0,	   // 0xA0 to 0xAF
      0, 0, 0, 0, 0, 0, 0, 0,	0, 0, 0, 0, 0, 0, 0, 0,	   // 0xB0 to 0xBF
      0, 0, 0, 0, 0, 0, 0, 0,	0, 0, 0, 0, 0, 0, 0, 0,	   // 0xC0 to 0xCF
      0, 0, 0, 0, 0, 0, 0, 0,	0, 0, 0, 0, 0, 0, 0, 0,	   // 0xD0 to 0xDF
      0, 0, 0, 0, 0, 0, 0, 0,	0, 0, 0, 0, 0, 0, 0, 0,	   // 0xE0 to 0xEF
      0, 0, 0, 0, 0, 0, 0, 0,	0, 0, 0, 0, 0, 0, 0, 0,	   // 0xF0 to 0xFF
   } ;

static const char FramepaC_possible_delimiter_raw[] =
   {
      1, 1, 1, 1, 1, 1, 1, 1,	1, 1, 1, 1, 1, 1, 1, 1,	   // ^@ to ^O
      1, 1, 1, 1, 1, 1, 1, 1,	1, 1, 1, 1, 1, 1, 1, 1,	   // ^P to ^_
      1, 0, 0, 0, 0, 0, 0, 0,	0, 0, 0, 0, 0, 0, 0, 0,	   // SP to /
      0, 0, 0, 0, 0, 0, 0, 0,	0, 0, 0, 0, 0, 0, 0, 0,	   //  0 to ?
      0, 0, 0, 0, 0, 0, 0, 0,	0, 0, 0, 0, 0, 0, 0, 0,	   //  @ to O
      0, 0, 0, 0, 0, 0, 0, 0,	0, 0, 0, 0, 0, 0, 0, 0,	   //  P to _
      0, 0, 0, 0, 0, 0, 0, 0,	0, 0, 0, 0, 0, 0, 0, 0,	   //  ` to o
      0, 0, 0, 0, 0, 0, 0, 0,	0, 0, 0, 0, 0, 0, 0, 0,	   //  p to DEL
      0, 0, 0, 0, 0, 0, 0, 0,	0, 0, 0, 0, 0, 0, 0, 0,	   // 0x80 to 0x8F
      0, 0, 0, 0, 0, 0, 0, 0,	0, 0, 0, 0, 0, 0, 0, 0,	   // 0x90 to 0x9F
      0, 0, 0, 0, 0, 0, 0, 0,	0, 0, 0, 0, 0, 0, 0, 0,	   // 0xA0 to 0xAF
      0, 0, 0, 0, 0, 0, 0, 0,	0, 0, 0, 0, 0, 0, 0, 0,	   // 0xB0 to 0xBF
      0, 0, 0, 0, 0, 0, 0, 0,	0, 0, 0, 0, 0, 0, 0, 0,	   // 0xC0 to 0xCF
      0, 0, 0, 0, 0, 0, 0, 0,	0, 0, 0, 0, 0, 0, 0, 0,	   // 0xD0 to 0xDF
      0, 0, 0, 0, 0, 0, 0, 0,	0, 0, 0, 0, 0, 0, 0, 0,	   // 0xE0 to 0xEF
      0, 0, 0, 0, 0, 0, 0, 0,	0, 0, 0, 0, 0, 0, 0, 0,	   // 0xF0 to 0xFF
   } ;

//----------------------------------------------------------------------

bool FramepaC_PnP_mode = false ;
const char *user_delimiter = 0 ;
const unsigned char *user_isalpha = 0 ;

/************************************************************************/
/************************************************************************/

const char *FrStdWordDelimiters(FrCharEncoding encoding)
{
   switch (encoding)
      {
      case FrChEnc_Latin1:
      case FrChEnc_Latin2:
      case FrChEnc_Unicode:
	 return FramepaC_possible_delimiter ;
      case FrChEnc_EUC:
      case FrChEnc_UTF8:
	 return FramepaC_possible_delimiter_EUC ;
      case FrChEnc_RawOctets:
	 return FramepaC_possible_delimiter_raw ;
      case FrChEnc_User:
	 return user_delimiter ? user_delimiter
			       : FramepaC_possible_delimiter ;
      default:
	 FrWarning("unrecognized character encoding, using Latin-1") ;
	 return FramepaC_possible_delimiter ;
      }
}

//----------------------------------------------------------------------

void FrSetUserWordDelimiters(const char *delim)
{
   user_delimiter = delim ;
   return ;
}

//----------------------------------------------------------------------

const unsigned char *FrStdIsAlphaTable(FrCharEncoding encoding)
{
   switch (encoding)
      {
      case FrChEnc_Latin1:
      case FrChEnc_Latin2:
	 return FramepaC_isalpha_table ;
      case FrChEnc_Unicode:
      case FrChEnc_EUC:
      case FrChEnc_UTF8:
	 return FramepaC_isalpha_table_EUC ;
      case FrChEnc_RawOctets:
	 return FramepaC_isalpha_table_raw ;
      case FrChEnc_User:
	 return user_isalpha ? user_isalpha
			       : FramepaC_isalpha_table ;
      default:
	 FrWarning("unrecognized character encoding, using Latin-1") ;
	 return FramepaC_isalpha_table ;
      }
}

//----------------------------------------------------------------------

void FrSetUserIsAlphaTable(const unsigned char *alpha)
{
   user_isalpha = alpha ;
   return ;
}

/************************************************************************/
/************************************************************************/

//----------------------------------------------------------------------
// convert a string containing a one-line sentence into a string containing
// a list of words separated by single blanks

char *FrCanonicalizeSentence(const char *sentence, FrCharEncoding enc,
			     bool force_uppercase, const char *delim,
			     char const * const *abbrevs)
{
   if (!sentence || !*sentence)
      return 0 ;
   if (!delim)
      delim = FrStdWordDelimiters(enc) ;
   char *resultsent = FrNewN(char,2*strlen(sentence)+1) ;
   unsigned char const *alphachars = FrStdIsAlphaTable(enc) ;
   char *result = resultsent ;
   while (Fr_isspace(*sentence))
      sentence++ ;			// skip leading whitespace
   const char *end = sentence ;
   while (*end != '\0')
      end++ ;
   if (end != sentence)
      end-- ;
   if (*end == '\n')			// strip trailing newline if present
      end-- ;
   while (end >= sentence && Fr_isspace(*end)) // strip trailing whitespace
      end-- ;
   const unsigned char *map
	 = FrUppercaseTable(force_uppercase ? enc : FrChEnc_RawOctets) ;
   while (*sentence && sentence <= end)
      {
      while (Fr_isspace(*sentence))
	 sentence++ ;			// skip leading whitespace
      const char *start = sentence ;	// remember start of word
      char c ;
      while (sentence <= end &&
	     (c = (char)map[*(unsigned char*)sentence],
	      !delim[(unsigned char)c]))
	 {
	 *result++ = c ;
	 sentence++ ;
	 }
      switch (*sentence)
	 {
	 case '.':			// special handling for numbers
	 case ',':
	    if (sentence == start)
	       *result++ = *sentence++ ; // ensure a non-empty word
	    else if (sentence > start && Fr_isdigit(sentence[-1]) &&
		     Fr_isdigit(sentence[1]))
	       {			// add the rest of the number
	       *result++ = *sentence++ ; // copy the period/comma
	       *result++ = *sentence++ ; // copy first digit after period/comma
	       do {
		  while (sentence <= end && Fr_isdigit(*sentence))
		     *result++ = *sentence++ ;
		  if ((*sentence == '.' || *sentence == ',') &&
		      Fr_isdigit(sentence[1]))
		     {
		     *result++ = *sentence++ ; // copy the period/comma
		     *result++ = *sentence++ ; // copy following digit
		     }
		  else if (Fr_toupper(*sentence) == 'E' &&
			   (Fr_isdigit(sentence[1]) || sentence[1] == '+' ||
			    sentence[1] == '-'))
		     {
		     // copy the exponent and then quit
		     *result++ = *sentence++ ;
		     *result++ = *sentence++ ;
		     while (sentence < end && Fr_isdigit(*sentence))
			*result++ = *sentence++ ;
		     break ;
		     }
		  else
		     break ;
		  } while (*sentence) ;
	       }
	    else if (*sentence == '.')
	       {
	       if (alphachars[(unsigned char)sentence[-1]] &&
		   alphachars[(unsigned char)sentence[1]])
		  {
		  // must be an abbreviation with periods, so copy remainder
		  *result++ = *sentence++ ;
		  *result++ = (char)map[*((unsigned char*)sentence)] ;
		  sentence++ ;
		  while (alphachars[(unsigned char)*sentence] ||
			 Fr_isdigit(*sentence) || *sentence == '.')
		     {
		     *result++ = (char)map[*((unsigned char*)sentence)] ;
		     sentence++ ;
		     }
		  }
	       else if ((Fr_isspace(sentence[1]) || sentence[1] == '\0') &&
			FrIsKnownAbbreviation(start,sentence,abbrevs))
		  *result++ = *sentence++ ;	// keep period attached
	       }
	    *result++ = ' ' ;
	    break ;
	 case '-':			// check if m-dash or hyphen
	    if (sentence == start)	// at start of word, is dash or minus
	       {
	       *result++ = *sentence++ ;
	       if (*sentence == '-')	// check if doubled
		  *result++ = *sentence++ ;
	       else
		  {			// grab any digits from negative number
		  while (sentence <= end && Fr_isdigit(*sentence))
		     *result++ = *sentence++ ;
		  if ((*sentence == '.' || *sentence == ',') &&
		      Fr_isdigit(sentence[1]))
		     {
		     *result++ = *sentence++ ; // copy the period
		     *result++ = *sentence++ ; // copy following digit
		     while (sentence <= end &&
			    (Fr_isdigit(*sentence) ||
			     ((*sentence == '.' || *sentence == ',') &&
			      Fr_isdigit(sentence[1]))))
			*result++ = *sentence++ ;
		     }
		  }
	       *result++ = ' ' ;
	       }
	    else if (Fr_isdigit(sentence[-1]))
	       {
	       // OK, we have a number range (123-456) or numeric prefix
	       // (12-ton), so split it in three
	       *result++ = ' ' ;
	       *result++ = *sentence++ ;
	       *result++ = ' ' ;
	       }
	    else if (!delim[((unsigned char*)sentence)[1]])
	       {
	       *result++ = (char)map[*((unsigned char*)sentence)] ;
	       sentence++ ;
	       // will grab rest of hyphenated word on next iteration
	       }
	    else
	       *result++ = ' ' ;
	    break ;
	 case '\'':			// check if contraction or doubled
	    if (sentence == start)	// at start of word, must be a quote
	       {
	       *result++ = *sentence++ ;
	       if (*sentence == '\'')	// doubled?
		  *result++ = *sentence++ ;
	       }
	    else
	       {
	       if ((enc == FrChEnc_Latin1 || enc == FrChEnc_Latin2 ||
		    ((unsigned char*)sentence)[1] < 0x80) &&
		   alphachars[(unsigned char)sentence[1]])
		  {  // contraction!
		  *result++ = *sentence++ ; // copy the quote
		  // copy first char after quote
		  *result++ = (char)map[*((unsigned char*)sentence)] ;
		  sentence++ ;
		  // include the rest of the word
		  while (alphachars[(unsigned char)*sentence])
		     {
		     *result++ = (char)map[*((unsigned char*)sentence)] ;
		     sentence++ ;
		     }
		  }
	       }
	    *result++ = ' ' ;
	    break ;
	 case '`':			// check if double back-quote
	    if (sentence == start)
	       {
	       *result++ = *sentence++ ;
	       if (*sentence == '`')	// doubled?
		  *result++ = *sentence++ ;
	       }
	    *result++ = ' ' ;
	    break ;
	 case ':':
	 case '/':
	    if (FramepaC_PnP_mode)
	       {
	       //treat as normal character inside glossary variable or marker
	       // else make it a word
	       bool in_gloss_var = false ;
	       for (const char *tmp = start ; tmp < sentence ; tmp++)
		  {
		  if (*tmp == '<')
		     {
		     in_gloss_var = true ;
		     break ;
		     }
		  }
	       if (in_gloss_var)
		  {
		  *result++ = *sentence++ ; // consume the colon
		  // advance up to & incl the gt sign
		  do {
		     *result++ = (char)map[*((unsigned char*)sentence)] ;
		     } while (*sentence && *sentence++ != '>') ;
		  if (*sentence == '<')
		     {
		     *result = *sentence++ ; // consume the gt sign
		     // advance up to & including the greater-than sign
		     while (*sentence && *sentence!= '>' && *sentence!= ' ')
			{
			*result++ = (char)map[*((unsigned char*)sentence)] ;
			sentence++ ;
			}
		     if (*sentence == '>')
			*result++ = *sentence++ ;
		     }
		  }
	       else
		  {
		  // ensure at least one char in 'word'
		  if (sentence == start)
		     *result++ = *sentence++ ;
		  }
	       *result++ = ' ' ;	// insert word separator
	       break ;
	       }
	    else if (sentence > start &&
		     (sentence[-1] == '<' || sentence[1] == '>'))
	       {
	       *result++ = *sentence++ ;
	       break ;
	       }
	    // fall through to default case
	 default:
	    if (sentence == start)
	       {
	       // ensure at least one char in 'word'
	       *result++ = (char)map[*((unsigned char*)sentence)] ;
	       sentence++ ;
	       }
//	    if (*sentence)		 // if more words,
	       *result++ = ' ' ;	 //   insert a blank as separator
	    break ;
	 }
      }
   while (result > resultsent && result[-1] == ' ')
      result-- ;			// trim trailing blanks
   *result = '\0' ;			// terminate the string
   // reduce buffer size to amount actually needed
   char *res = (char*)FrRealloc(resultsent,result-resultsent+1,true) ;
   if (res)				// was realloc successful?
      return res ;			// return trimmed expanded string
   else
      return resultsent ;		// return the expanded string
}

// end of file frstrutl.cpp //
