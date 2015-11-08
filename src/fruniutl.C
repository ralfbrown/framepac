/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File: fruniutl.cpp	 	Unicode string-manip utility functions	*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1996,1997,1998,1999,2003,2004,2009			*/
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
#include "frbytord.h"
#include "frctype.h"
#include "frlist.h"
#include "frstring.h"
#include "frunicod.h"

/************************************************************************/
/*    Manifest constants for this module				*/
/************************************************************************/

/************************************************************************/
/*    Global data for this module					*/
/************************************************************************/

extern bool FramepaC_PnP_mode ;

static bool dummy_byteswap = false ;

/************************************************************************/
/************************************************************************/

#define store_upcase_quoted(result,c,force_uppercase) \
	 if (force_uppercase) c = Fr_towupper(c) ; \
         result += Fr_Unicode_to_UTF8(c,result,dummy_byteswap) ;

//----------------------------------------------------------------------
// convert a Unicode string containing a one-line sentence into a list of
// strings, one per word or other token (i.e. punctuation) in the original
// sentence

FrList *FrCvtUString2Wordlist(const FrChar16 *sentence, size_t length,
			      const char *delim, char const * const *abbrevs)
{
   (void)abbrevs;//!!!need to implement this functionality
   if (!sentence || !*sentence)
      return 0 ;
   if (!delim)
      delim = FrStdWordDelimiters() ;
   FrChar16 c ;
   while (Fr_iswspace(FrByteSwap16(*sentence)))
      sentence++ ;			// skip leading whitespace
   const FrChar16 *end = sentence + length ;
   if (!length)
      {
      while (*end)
	 end++ ;
      }
   if (end > sentence)
      end-- ;
   if (FrByteSwap16(*end) == '\n')	// strip trailing newline if present
      end-- ;
   while (end >= sentence && Fr_iswspace(FrByteSwap16(*end)))
      end-- ;				// strip trailing whitespace
   FrList *words = 0 ;
   while (*sentence && sentence <= end)
      {
      while (Fr_iswspace(FrByteSwap16(*sentence)))
	 sentence++ ;			// skip leading whitespace
      const FrChar16 *start = sentence ;	// remember start of word
      c = FrByteSwap16(*sentence) ;
      while (sentence <= end && (!Fr_is8bit(c) || !delim[c]))
	 {
	 FrChar16 tmp_c = c ;
	 c = FrByteSwap16(*(++sentence)) ;
	 // stop the loop if the current char is a digit and the next is a
	 // non-ASCII character
	 if (sentence > start+1 && Fr_iswdigit(tmp_c) && !Fr_iswdigit(c))
	    break ;
	 }
      switch (c)
	 {
	 case '\'':			// check if contraction or doubled
	    if (sentence == start)	// at start of word, must be a quote
	       {
	       sentence++ ;
	       if (FrByteSwap16(*sentence) == '\'')	// doubled?
		  {
		  sentence++ ;
		  pushlist(new FrString("\""),words) ;
		  }
	       else
		  pushlist(new FrString("'"),words) ;
	       }
	    else
	       {
	       if (sentence[1] && Fr_iswalpha(c = FrByteSwap16(sentence[1])))
		  {  // contraction!
		  sentence += 2 ;
		  // include the rest of the word
		  while (*sentence && Fr_iswalpha(c = FrByteSwap16(*sentence)))
		     sentence++ ;
		  }
	       pushlist(new FrString(start,sentence-start,sizeof(FrChar16)),
			words) ;
	       }
	    break ;
	 case '`':			// check if double back-quote
	    if (sentence == start)
	       {
	       sentence++;
	       if (FrByteSwap16(*sentence) == '`') // doubled?
		  {
		  sentence++ ;
		  pushlist(new FrString(start,sentence-start,sizeof(FrChar16)),
			   words) ;
		  }
	       else
		  pushlist(new FrString("`"),words) ;
	       }
	    else
	       pushlist(new FrString(start,sentence-start,sizeof(FrChar16)),
			words) ;
	    break ;
	 case '.':		// special handling for numbers
	 case ',':
	    if (sentence == start)
	       sentence++ ;	// ensure a non-empty word
	    else if (Fr_iswdigit(FrByteSwap16(sentence[-1])) &&
		     Fr_iswdigit(FrByteSwap16(sentence[1])))
	       {		// add the rest of the number
	       sentence += 2 ;
	       do {
		  while (sentence <= end &&
			 Fr_iswdigit(FrByteSwap16(*sentence)))
		     sentence++ ;
		  if (((c = FrByteSwap16(*sentence)) == '.' || c == ',') &&
		      Fr_iswdigit(FrByteSwap16(sentence[1])))
		     sentence += 2 ;
		  else
		     break ;
		  } while (*sentence) ;
	       }
	    else if (FrByteSwap16(*sentence) == '.')
	       {
	       if (Fr_iswalpha(c = FrByteSwap16(sentence[-1])) &&
		   Fr_iswalpha(c = FrByteSwap16(sentence[1])))
		  {
		  // must be an abbreviation with periods, so read remainder
		  sentence += 2 ;
		  while (sentence <= end &&
			 (!Fr_is8bit(c = FrByteSwap16(*sentence)) ||
			  !delim[c] || c == '.'))
		     sentence++ ;
		  }
	       }
	    pushlist(new FrString(start,sentence-start,sizeof(FrChar16)),
		     words) ;
	    break ;
	 case '-':			// check if m-dash or hyphen
	    if (sentence == start)	// at start of word, is dash or minus
	       {
	       sentence++ ;
	       if (FrByteSwap16(*sentence) == '-') // check if doubled
		  sentence++ ;
	       else if (!Fr_iswdigit(FrByteSwap16(sentence[-1])))
		  //not range or num-prefix?
		  {			// grab any digits from negative number
		  while (sentence <= end &&
			 Fr_iswdigit(FrByteSwap16(*sentence)))
		     sentence++ ;
		  c = FrByteSwap16(*sentence) ;
		  if ((c == '.' || c == ',') &&
		      Fr_iswdigit(FrByteSwap16(sentence[1])))
		     {
		     sentence += 2 ;
		     while (sentence <= end &&
			    Fr_iswdigit(FrByteSwap16(*sentence)))
			sentence++ ;
		     }
		  }
	       pushlist(new FrString(start,sentence-start,sizeof(FrChar16)),
			words) ;
	       }
	    else if (!Fr_is8bit(c = FrByteSwap16(sentence[1])) ||
		     (!delim[(unsigned char)c] &&
		      !Fr_isdigit(c)))
	       {
	       sentence++ ;
	       // grab rest of hyphenated word
	       while (sentence <= end &&
		      (!Fr_is8bit(c = FrByteSwap16(*sentence)) ||
		       !delim[(unsigned char)c] || c == '-'))
		  sentence++ ;
	       pushlist(new FrString(start,sentence-start,sizeof(FrChar16)),
			words) ;
	       }
	    else
	       pushlist(new FrString(start,sentence-start,sizeof(FrChar16)),
			words) ;
	    break ;
	 default:
	    if (sentence == start)
	       sentence++ ;		// ensure at least one char in 'word'
	    pushlist(new FrString(start,sentence-start,sizeof(FrChar16)),words);
	    break ;
	 }
      }
   return listreverse(words) ;
}

//----------------------------------------------------------------------
// convert a string containing a one-line sentence into a string containing
// a list of words separated by single blanks

char *FrCanonicalizeUSentence(const FrChar16 *sentence, size_t length,
			      bool force_uppercase, const char *delim,
			      char const * const *abbrevs)
{
   (void)abbrevs;//!!!need to implement this functionality
   if (!sentence || !*sentence)
      return 0 ;
   if (!delim)
      delim = FrStdWordDelimiters() ;
   const FrChar16 *end = sentence + length ;
   if (!length)
      {
      while (*end != '\0')
	 {
	 end++ ;
	 length++ ;
	 }
      }
   // skip leading whitespace
   FrChar16 c ;
   while (Fr_iswspace(FrByteSwap16(*sentence)) && length>0)
      {
      sentence++ ;
      length-- ;
      }
   char *resultsent = FrNewN(char,(2*length+1)*sizeof(FrChar16)) ;
   char *result = resultsent ;
   if (end != sentence)
      end-- ;
   if (FrByteSwap16(*end) == '\n')	// strip trailing newline if present
      end-- ;
   while (end >= sentence && Fr_iswspace(FrByteSwap16(*end)))
      end-- ;				// strip trailing whitespace
   while (*sentence && sentence <= end)
      {
      while (sentence <= end && Fr_iswspace(FrByteSwap16(*sentence)))
	 sentence++ ;			// skip leading whitespace
      const FrChar16 *start = sentence ;	// remember start of word
      c = FrByteSwap16(*sentence) ;
      while (sentence <= end && (!Fr_is8bit(c) || !delim[c]))
	 {
	 store_upcase_quoted(result,c,force_uppercase) ;
	 FrChar16 tmp_c = c ;
	 c = FrByteSwap16(*(++sentence)) ;
	 // stop the loop if the current char is a digit and the next is a
	 // non-ASCII character
	 if (sentence > start+1 && Fr_iswdigit(tmp_c) && !Fr_iswdigit(c))
	    break ;
	 }
      switch (c)
	 {
	 case '.':			// special handling for numbers
	 case ',':
	    if (sentence == start)
	       {
	       sentence++ ;
	       result += Fr_Unicode_to_UTF8(c,result,dummy_byteswap) ;
	       }
	    else if (sentence < end && Fr_iswdigit(sentence[-1]) &&
		     Fr_iswdigit(sentence[1]))
	       {			// add the rest of the number
	       sentence++ ;
	       // copy the period/comma
	       result += Fr_Unicode_to_UTF8(c,result,dummy_byteswap) ;
	       c = FrByteSwap16(*sentence++) ;
	       // copy first digit after "."/","
	       result += Fr_Unicode_to_UTF8(c,result,dummy_byteswap) ;
	       do {
		  while (sentence <= end &&
			 Fr_iswdigit(c = FrByteSwap16(*sentence)))
		     {
		     result += Fr_Unicode_to_UTF8(c,result,dummy_byteswap) ;
		     sentence++ ;
		     }
		  c = FrByteSwap16(*sentence) ;
		  FrChar16 c2 = FrByteSwap16(sentence[1]) ;
		  if ((c == '.' || c == ',') && Fr_iswdigit(c2))
		     {
		     // copy the period/comma
		     result += Fr_Unicode_to_UTF8(c,result,dummy_byteswap) ;
		     // copy first digit after
		     result += Fr_Unicode_to_UTF8(c2,result,dummy_byteswap) ;
		     sentence += 2 ;
		     }
		  else
		     break ;
		  } while (sentence <= end && *sentence) ;
	       }
	    else if (sentence < end &&
		     (c = FrByteSwap16(*sentence)) == '.')
	       {
	       FrChar16 c_1 = FrByteSwap16(sentence[-1]) ;
	       FrChar16 c2 = FrByteSwap16(sentence[1]) ;
	       if (Fr_iswalpha(c_1) && Fr_iswalpha(c2))
		  {
		  // must be an abbreviation with periods, so copy remainder
		  sentence++ ;
		  result += Fr_Unicode_to_UTF8(c,result,dummy_byteswap) ;
		  c = FrLoadShort(sentence++) ;
		  store_upcase_quoted(result,c,force_uppercase) ;
		  while (Fr_iswalpha(c = FrLoadShort(sentence)) || c == '.')
		     {
		     sentence++ ;
		     store_upcase_quoted(result,c,force_uppercase) ;
		     if (sentence > end)
		        break ;
		     }
		  }
	       }
	    *result++ = ' ' ;
	    break ;
	 case '-':			// check if m-dash or hyphen
	    if (sentence == start)	// at start of word, is dash or minus
	       {
	       sentence++ ;
	       result += Fr_Unicode_to_UTF8(c,result,dummy_byteswap) ;
	       c = FrLoadShort(sentence) ;
	       if (c == '-')	// check if doubled
		  {
		  result += Fr_Unicode_to_UTF8(c,result,dummy_byteswap) ;
		  sentence++ ;
		  *result++ = ' ' ;
		  }
	       else
		  {			// grab any digits from negative number
		  while (sentence <= end &&
			 Fr_iswdigit(c = FrLoadShort(sentence)))
		     {
		     result += Fr_Unicode_to_UTF8(c,result,dummy_byteswap) ;
		     sentence++ ;
		     }
		  if (((c = FrLoadShort(sentence)) == '.' || c == ',') &&
		      Fr_isdigit(FrLoadShort(sentence+1)))
		     {
		     result += Fr_Unicode_to_UTF8(c,result,dummy_byteswap) ;
		     sentence++ ;
		     do {
		        // copy following digits
		        c = FrLoadShort(sentence++) ;
			result += Fr_Unicode_to_UTF8(c,result,dummy_byteswap) ;
		        } while (sentence <= end &&
				 Fr_iswdigit(c = FrLoadShort(sentence))) ;
		     }
		  }
	       *result++ = ' ' ;
	       }
	    else if (Fr_iswdigit(c = FrLoadShort(sentence-1)))
	       {
	       // OK, we have a number range (123-456) or numeric prefix
	       // (12-ton), so split it in three
	       *result++ = ' ' ;
	       FrChar16 c2 = FrLoadShort(sentence++) ;
	       result += Fr_Unicode_to_UTF8(c2,result,dummy_byteswap) ;
	       *result++ = ' ' ;
	       }
	    else if (!Fr_is8bit(c = FrByteSwap16(sentence[1])) ||
		     !delim[c])
	       {
	       FrChar16 c2 = FrByteSwap16(*sentence++) ;
	       store_upcase_quoted(result,c2,force_uppercase) ;
	       // grab rest of hyphenated word on next iteration
	       }
	    else
	       *result++ = ' ' ;
	    break ;
	 case '\'':			// check if contraction or doubled
	    if (sentence == start)	// at start of word, must be a quote
	       {
	       FrChar16 c2 = FrLoadShort(sentence++) ;
	       result += Fr_Unicode_to_UTF8(c2,result,dummy_byteswap) ;
//	       if (FrLoadShort(sentence) == '\'')	// doubled?
//!!!		  result[-1] = '"' ;	// convert to double quote
	       }
	    else
	       {
	       FrChar16 c2 = FrLoadShort(sentence+1) ;
	       if (Fr_iswalpha(c2))
		  {  // contraction!
		  sentence++ ;
		  // copy the quote
		  result += Fr_Unicode_to_UTF8(c,result,dummy_byteswap) ;
		  // include the rest of the word
		  c = FrLoadShort(sentence) ;
		  do {
		     sentence++ ;
		     store_upcase_quoted(result,c,force_uppercase) ;
		     c = FrLoadShort(sentence) ;
		     } while (Fr_iswalpha(c) && sentence <= end) ;
		  }
	       }
	    *result++ = ' ' ;
	    break ;
	 case '`':			// check if double back-quote
	    if (sentence == start)
	       {
	       FrChar16 c2 = FrLoadShort(sentence++) ;
	       result += Fr_Unicode_to_UTF8(c2,result,dummy_byteswap) ;
//	       if (FrLoadShort(sentence) == '`')	// doubled?
//		  result[-1] = '"' ;	// convert to double quote
	       }
	    *result++ = ' ' ;
	    break ;
	 case ':':
	    if (FramepaC_PnP_mode)
	       {
	       //treat as normal character inside glossary variable or marker
	       // else make it a word
	       bool in_gloss_var = false ;
	       for (const FrChar16 *tmp = start ; tmp < sentence ; tmp++)
		  if (*tmp == '<')
		     {
		     in_gloss_var = true ;
		     break ;
		     }
	       if (in_gloss_var)
		  {
		  FrChar16 c2 = FrLoadShort(sentence++) ; // consume the colon
		  result += Fr_Unicode_to_UTF8(c2,result,dummy_byteswap) ;
		  // advance up to & incl the gt sign
		  while (*sentence && (c2 = FrLoadShort(sentence)) != '>')
		     {
		     sentence++ ;
		     store_upcase_quoted(result,c2,force_uppercase) ;
		     }
		  c2 = FrLoadShort(sentence) ;
		  if (c2 == '<')
		     {
		     sentence++ ;	// consume the less-than sign
		     result += Fr_Unicode_to_UTF8(c2,result,dummy_byteswap) ;
		     // advance up to & including the greater-than sign
		     while ((c2 = FrByteSwap16(*sentence)) != 0 &&
			    c2 != '>' && c2 != ' ')
			{
			sentence++ ;
			store_upcase_quoted(result,c2,force_uppercase) ;
			}
		     c2 = FrByteSwap16(*sentence) ;
		     if (c2 == '>')
			{
			result += Fr_Unicode_to_UTF8(c2,result,dummy_byteswap);
			sentence++ ;
			}
		     }
		  }
	       else
		  {
		  // ensure at least one char in 'word'
		  if (sentence == start)
		     {
		     FrChar16 c2 = FrByteSwap16(*sentence++) ;
		     result += Fr_Unicode_to_UTF8(c2,result,dummy_byteswap) ;
		     }
		  }
	       *result++ = ' ' ;	// insert word separator
	       break ;
	       }
	    // fall through to default case
	 default:
	    if (sentence == start)
	       {
	       sentence++ ;
	       // ensure at least 1 char in 'word'
	       store_upcase_quoted(result,c,force_uppercase) ;
	       }
//	    if (*sentence)		 // if more words, insert a blank as
	       *result++ = ' ' ;	 //   separator
	    break ;
	 }
      }
   while (result > resultsent && result[-1] == ' ')
      result-- ;			// trim trailing blanks
   FrStoreShort(0,result) ;		// terminate the string
   // reduce size to amt needed
   char *res = (char*)FrRealloc(resultsent,result-resultsent+sizeof(FrChar16),
				true) ;
   if (res)				// was realloc successful?
      return res ;			// return trimmed expanded string
   else
      return resultsent ;		// return the expanded string
}

//----------------------------------------------------------------------

FrString *FrDecanonicalizeUSentence(const char *sentence, bool /*force_lower*/)
{
   FrChar16 *result = Fr_UTF8_to_Unicode(sentence) ;
   if (result)
      return new FrString((char*)result,Fr_wcslen(result),sizeof(FrChar16),
			  false) ;
   else
      {
      FrNoMemory("in FrDecanonicalizeUSentence") ;
      return 0 ;
      }
}

//----------------------------------------------------------------------

FrList *FrCvtUString2Symbollist(const FrChar16 *sentence,
			        const char *possible_delim)
{
   char *sent = FrCanonicalizeUSentence(sentence,0,false,possible_delim) ;
   if (!sent)
      return 0 ;
   FrList *syms = FrCvtSentence2Symbollist(sent) ;
   FrFree(sent) ;
   return syms ;
}

//----------------------------------------------------------------------

char *FrASCII2Unicode(const char *string, bool canonicalize)
{
   if (!string)
      return 0 ;
   size_t len = strlen(string)+1 ;
   FrChar16 *ustring = FrNewN(FrChar16,len) ;
   if (ustring)
      {
      for (size_t i = 0 ; i < len ; i++)
	 FrStoreShort((FrChar16)string[i],&ustring[i]) ;
      if (canonicalize)
	 {
	 FrChar16 *tmp = ustring ;
	 ustring = (FrChar16*)FrCanonicalizeUSentence(ustring,len-1,false) ;
	 FrFree(tmp) ;
	 }
      }
   return (char*)ustring ;
}

//----------------------------------------------------------------------

// end of file fruniutl.cpp //
