/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File: frcognat.cpp		inter-language cognate scoring		*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 2001,2002,2006,2008,2009,2015				*/
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

#include "framerr.h"
#include "frcmove.h"
#include "frctype.h"
#include "frreader.h"
#include "frsymbol.h"
#include "frstring.h"
#include "frutil.h"

/************************************************************************/
/*	Manifest Constants						*/
/************************************************************************/

#define EXACT_MATCH_SCORE 100
#define FUZZY_MATCH_SCORE 90

#define MAX_CHARS 256

/************************************************************************/
/*	Types for this module						*/
/************************************************************************/

class FrCognateData
   {
   private:
      char *cog_letters[MAX_CHARS] ;
      char *cog_buffer ;
      char *rev_letters[MAX_CHARS] ;
      const char *rev_buffer ;
   public:
      FrCognateData() ;
      FrCognateData(const FrCognateData *orig) ;
      ~FrCognateData()
   	 { FrFree(cog_buffer) ; cog_buffer = 0 ; rev_buffer = 0 ; }

      // modifiers
      bool set(const FrCognateData *cognates) ;
      bool set(const char *cognates) ;

      // accessors
      bool OK() const { return cog_buffer != 0 ; }
      const char *buffer() const { return cog_buffer ; }
      const char * const *letters() const { return cog_letters ; }
      const char *revbuffer() const { return rev_buffer ; }
      const char * const *revletters() const { return rev_letters ; }
   } ;

/************************************************************************/
/*	Global Variables						*/
/************************************************************************/

static FrCognateData cognate_data ;
static size_t fuzzy_match_score = FUZZY_MATCH_SCORE ;

/************************************************************************/
/*	Helper Functions						*/
/************************************************************************/

size_t cognate_score(unsigned char letter1, unsigned char letter2,
		     bool casefold = true,
		     bool exact_letter_match_only = false)
{
   if (casefold)
      {
      letter1 = Fr_toupper(letter1) ;
      letter2 = Fr_toupper(letter2) ;
      }
   if (letter1 == letter2)
      return EXACT_MATCH_SCORE ;
   else if (exact_letter_match_only)
      return 0 ;
   else if (!letter2 || !cognate_data.letters()[letter1])
      return 0 ;
   const char *match = 0 ;
   const char *cog = cognate_data.letters()[letter1] ;
   for ( ; *cog ; cog++)
      {
      char cogletter = casefold ? Fr_toupper(*cog) : *cog ;
      if (cogletter == '(')		// multi-letter string?
	 {
	 cog++ ;
	 if (*cog && *cog != ')' && cog[1] == ')')
	    {
	    // single letter in parens
	    cogletter = casefold ? Fr_toupper(*cog) : *cog ;
	    cog++ ;
	    }
	 else
	    {
	    while (*cog && *cog != ')')	// skip the entire string, since
	       cog++ ;			//   we're only doing single-letter
	    if (Fr_isdigit(cog[1]))
	       cog++ ;			// skip the weight digit
	    continue ;
	    }
	 }
      if (cogletter == letter2)
	 {
	 match = cog ;
	 break ;
	 }
      else if (Fr_isdigit(cog[1]))
	 cog++ ;			// skip the weight digit
      }
   if (match)
      {
      if (Fr_isdigit(match[1]))
	 return (fuzzy_match_score * (match[1] - '0')) / 10 ;
      else
	 return fuzzy_match_score ;
      }
   return 0 ;
}

//----------------------------------------------------------------------

inline size_t cognate_score(char letter1, char letter2,
			    bool casefold = true,
			    bool exact_letter_match_only = false)
{
   return cognate_score((unsigned char)letter1,(unsigned char)letter2,
			casefold,exact_letter_match_only) ;
}

/************************************************************************/
/*	Methods for class FrCognateData					*/
/************************************************************************/

FrCognateData::FrCognateData()
{
   cog_buffer = 0 ;
   rev_buffer = 0 ;
   for (size_t i = 0 ; i < MAX_CHARS ; i++)
      {
      cog_letters[i] = 0 ;
      rev_letters[i] = 0 ;
      }
   return ;
}

//----------------------------------------------------------------------

FrCognateData::FrCognateData(const FrCognateData *orig)
{
   cog_buffer = 0 ;
   (void)this->set(orig) ;
   return ;
}

//----------------------------------------------------------------------

static const char *find_separator(const char *cogbuffer)
{
   if (cogbuffer)
      {
      for (const char *sep = cogbuffer ; *sep ; sep++)
	 {
	 if (sep > cogbuffer && Fr_isspace(sep[-1]) && sep[0] == '*' &&
	     sep[1] == '*')
	    return sep ;
	 }
      }
   // if we get here, the separator was not found
   return 0 ;
}

//----------------------------------------------------------------------

bool FrCognateData::set(const char *cognates)
{
   FrFree(cog_buffer) ;
   cog_buffer = FrDupString(cognates) ;
   rev_buffer = find_separator(cog_buffer) ;
   for (size_t i = 0 ; i < lengthof(cog_letters) ; i++)
      {
      cog_letters[i] = 0 ;
      rev_letters[i] = 0 ;
      }
   if (cognates)
      {
      if (!cog_buffer)
	 return false ;			// out of memory!
      char *cog = cog_buffer ;
      FrSkipWhitespace(cog) ;
      while (*cog && (cog < rev_buffer || !rev_buffer))
	 {
	 cog_letters[(unsigned char)*cog] = cog+1 ;
	 FrSkipToWhitespace(cog) ;
	 if (*cog)
	    *cog++ = '\0' ;
	 FrSkipWhitespace(cog) ;
	 }
      if (rev_buffer)
	 {
	 cog = (char*)(rev_buffer + 2) ; // skip the separator
	 FrSkipToWhitespace(cog) ;
	 FrSkipWhitespace(cog) ;
	 while (*cog)
	    {
	    rev_letters[(unsigned char)*cog] = cog+1 ;
	    FrSkipToWhitespace(cog) ;
	    if (*cog)
	       *cog++ = '\0' ;
	    FrSkipWhitespace(cog) ;
	    }
	 }
      }
   return true ;
}

//----------------------------------------------------------------------

bool FrCognateData::set(const FrCognateData *cognates)
{
   FrFree(cog_buffer) ;
   cog_buffer = FrDupString(cognates->buffer()) ;
   if (cog_buffer)
      {
      rev_buffer = cognates->revbuffer() - cognates->buffer() + cog_buffer ;
      for (size_t i = 0 ; i < MAX_CHARS ; i++)
	 {
	 if (cognates->cog_letters[i])
	    cog_letters[i] = ((cognates->letters()[i] - cognates->buffer()) +
			      cog_buffer) ;
	 else
	    cog_letters[i] = 0 ;
	 if (cognates->rev_letters[i])
	    rev_letters[i] = ((cognates->revletters()[i] -
			       cognates->buffer()) + cog_buffer) ;
	 else
	    rev_letters[i] = 0 ;
	 }
      }
   return true ;
}

/************************************************************************/
/************************************************************************/

void FrSetDefaultCognateLetters()
{
   static char default_cognates[] =
      "cskz CSKZ scz SCZ kc KC zsc ZSC dt DT td TD pb PB bp BP fv FV "
      "vf VF mn MN nm NM aáàä AÁÀÄ eéè EÉÈ iíì IÍÌ oóòö OÓÒÖ uúùü UÚÙÜ "
      "äa ÄA öo ÖO üu ÜU áa ÁA àa ÀA ée ÉE èe ÈE óo ÓO òo ÒO úu ÚU ùu ÙU "
      "., ,. '\" \"' " ;

   FrSetCognateLetters(default_cognates) ;
   return ;
}

//----------------------------------------------------------------------

void FrSetCognateScoring(size_t fuzzy_score)
{
   if (fuzzy_score > 0 && fuzzy_score < EXACT_MATCH_SCORE)
      fuzzy_match_score = fuzzy_score ;
   return ;
}

//----------------------------------------------------------------------

void FrSetCognateLetters(const char *cognates, size_t fuzzy_score)
{
   cognate_data.set(cognates) ;
   FrSetCognateScoring(fuzzy_score) ;
   return ;
}

//----------------------------------------------------------------------

bool FrSaveCognateLetters(FrCognateData *&cognates)
{
   cognates = new FrCognateData(cognate_data) ;
   return cognates != 0 ;
}

//----------------------------------------------------------------------

bool FrExactCognateMatchOnly(FrCognateData *&cognates)
{
   cognates = new FrCognateData(cognate_data) ;
   if (cognates)
      {
      cognate_data.set((char*)0) ;
      return true ;
      }
   else
      return false ;
}

//----------------------------------------------------------------------

bool FrRestoreCognateLetters(FrCognateData *cognates, bool free_data)
{
   if (!cognates)
      return false ;
   cognate_data.set(cognates) ;
   if (free_data)
      delete cognates ;
   return true ;
}

//----------------------------------------------------------------------

bool FrCognateLetters(char letter1, char letter2, bool casefold)
{
   return (cognate_score((unsigned char)letter1,(unsigned char)letter2,
			 casefold) > 0) ;
}

//----------------------------------------------------------------------

static void check_cognate(const char *str2, char letter2,
			  const char *cog, bool casefold, size_t len2,
			  size_t &best_match1, size_t &best_match2,
			  size_t &best_score)
{
   if (!cog)
      return ;
   size_t score = best_score ;
   size_t match1 = best_match1 ;
   size_t match2 = best_match2 ;
   for ( ; *cog ; cog++)
      {
      char cogletter = casefold ? Fr_toupper(*cog) : *cog ;
      if (cogletter == '(')
	 {
	 cog++ ;
	 const char *end = strchr(cog,')') ;
	 if (!end)
	    break ;
	 size_t len = end - cog ;
	 size_t weight = 10 ;
	 if (end != cog && Fr_isdigit(end[1]))
	    {
	    end++ ;
	    weight = (*end - '0') ;
	    }
	 if (len <= len2 && (match1 + match2 <= len+1))
	    {
	    if ((casefold ? Fr_strnicmp(cog,str2,len) : strncmp(cog,str2,len)) == 0)
	       {
	       size_t sc = (fuzzy_match_score * weight) / 10 ;
	       if (sc > score)
		  {
		  match1 = 1 ;
		  match2 = len ;
		  score = sc ;
		  }
	       break ;
	       }
	    }
	 cog = end ;
	 }
      else if (cogletter == letter2)
	 {
	 if (match1 < 1 || match2 < 1)
	    {
	    match1 = match2 = 1 ;
	    if (Fr_isdigit(cog[1]))
	       {
	       size_t sc = (fuzzy_match_score * (cog[1] - '0')) / 10 ;
	       cog++ ;		// consume the weight digit
	       if (sc > score)
		  {
		  score = sc ;
		  }
	       }
	    else if (score < fuzzy_match_score)
	       {
	       score = fuzzy_match_score ;
	       }
	    }
	 break ;
	 }
      else if (Fr_isdigit(cog[1]))
	 cog++ ;			// skip the weight digit
      }
   if (score > best_score)
      {
      best_score = score ;
      best_match1 = match1 ;
      best_match2 = match2 ;
      }
   return ;
}

//----------------------------------------------------------------------
// on input, 'len1' and 'len2' contain the maximum number of characters
//   to match in 'str1' and 'str2'; on return, they contain the number
//   of characters actually matched

size_t FrCognateLetters(const char *str1, const char *str2,
			size_t &len1, size_t &len2, bool casefold)
{
   if (len1 < 1 || len2 < 1 || !str1 || !*str1 || !str2 || !*str2)
      return 0 ;
   size_t best_match1 = 0 ;
   size_t best_match2 = 0 ;
   size_t score = 0 ;
   // check for the identity mapping
   if (*str1 == *str2 || (casefold && Fr_toupper(*str1) == Fr_toupper(*str2)))
      {
      best_match1 = best_match2 = 1 ;
      score = EXACT_MATCH_SCORE ;
      if (len1 == 1 && len2 == 1)
	 return score ;
      }
   // now check one-to-one and one-to-many mappings
   unsigned char letter1 = casefold ? Fr_toupper(*str1) : *str1 ;
   unsigned char letter2 = casefold ? Fr_toupper(*str2) : *str2 ;
   const char *cog = cognate_data.letters()[letter1] ;
   check_cognate(str2,letter2,cog,casefold,len2,best_match1,best_match2,
		 score) ;
   // finally, check many-to-one mappings
   if (len1 > 1)
      {
      cog = cognate_data.revletters()[letter2] ;
      check_cognate(str1,'\0',cog,casefold,len1,best_match2,best_match1,
		    score) ;
      }
   len1 = best_match1 ;
   len2 = best_match2 ;
   return score ;
}

//----------------------------------------------------------------------

#define VALUE(array,i,j) (array[(i)*rowsize+(j)])

static size_t best_score(const char *string1, const char *string2,
			 size_t ofs1, size_t len1,
			 size_t ofs2, size_t len2, size_t base_score,
			 size_t *cogscores, size_t &match1, size_t &match2,
			 bool casefold, bool exact_letter_match_only)
{
   string1 += ofs1 ;
   string2 += ofs2 ;
   size_t rowsize = len2+1 ;
   if (exact_letter_match_only)
      {
      match1 = 1 ;
      match2 = 1 ;
      return (cognate_score(*string1,*string2,casefold,exact_letter_match_only)
	      + VALUE(cogscores,(ofs1+1),(ofs2+1))) ;
      }
   bool ul_quadrant = (ofs1 <= len1/2 && ofs2 <= len2/2) ;
   size_t score = base_score ;
   size_t max1 = len1-ofs1 ;
   size_t max2 ;
   size_t sc ;
   do {
      max2 = 1 ;
      sc = FrCognateLetters(string1,string2,max1,max2,casefold) ;
      sc += VALUE(cogscores,ofs1+max1,ofs2+max2) ;
      if (sc > score || (sc == score && ul_quadrant && max1 && max2))
	 {
	 score = sc ;
	 match1 = max1 ;
	 match2 = max2 ;
	 }
      if (max1 > 0)
	 max1-- ;
      } while (max1 > 0) ;
   max2 = len2-ofs2 ;
   while (max2 > 1)
      {
      max1 = 1 ;
      sc = FrCognateLetters(string1,string2,max1,max2,casefold) ;
      sc += VALUE(cogscores,ofs1+max1,ofs2+max2) ;
      if (sc > score || (sc == score && ul_quadrant && max1 && max2))
	 {
	 score = sc ;
	 match1 = max1 ;
	 match2 = max2 ;
	 }
      if (max2 > 0)
	 max2-- ;
      }
   return score ;
}

//----------------------------------------------------------------------

double FrCognateScore(const char *string1, const char *string2,
		      bool casefold, bool score_by_shorter,
		      bool score_by_average,
		      bool exact_letter_match_only,
		      FrCognateAlignment **align)
{
   if (string1 && string2 && *string1 && *string2)
      {
      int len1 = strlen(string1) ;
      int len2 = strlen(string2) ;
      // start by computing all possible pairs of cognate letters
      size_t rowsize = len2+1 ;
      FrLocalAllocC(size_t,cogscore,4096,(len1+1)*rowsize) ;
      FrLocalAllocC(FrCognateAlignment,cogptr,4096,len1*len2) ;
      if (unlikely(!cogscore || !cogptr))
	 {
	 FrLocalFree(cogptr) ;
	 FrLocalFree(cogscore) ;
	 FrNoMemory("while computing cognate score") ;
	 return -1.0 ;
	 }
      // find the mapping which maximizes the cognates, subject to the
      //   constraint that there be no crossing correspondences, i.e. the
      //   target letters must be in the same sequence as the source letters
      //   from which they map
      // we can do this using Dynamic Programming, since the maximal score
      //   for any prefix is the maximum of the scores for skipping an SL
      //   letter, skipping a TL letter, or matching the current SL and TL
      //   letters/ngrams
      // fill in from the edges toward the origin
      for (int i = len1 - 1 ; i >= 0 ; i--)
	 {
	 for (int j = len2 - 1 ; j >= 0 ; j--)
	    {
	    size_t *curr = &VALUE(cogscore,i,j) ;
	    size_t score = curr[rowsize] ; 	// cogscore[i+1][j]
	    size_t match1(0), match2(0) ;
	    if (curr[1] > score)		// cogscore[i][j+1]
	       {
	       score = curr[1] ;
	       match2 = 1 ;
	       }
	    else
	       match1 = 1 ;
	    // only bother computing cognate score for the current letter
	    //   pair if it is possible to exceed the overall score for
	    //   skipping a letter
//	    if (score < curr[rowsize+1] + EXACT_MATCH_SCORE)
	       {
	       size_t sc = best_score(string1,string2,i,len1,j,len2,
				      score,cogscore,match1,match2,
				      casefold,exact_letter_match_only) ;
	       // we used to have a check for doubled letters here, but that
	       //   has been subsumed by one-many and many-one matches in
	       //   the cognate-letter list
	       if (sc > score)
		  score = sc ;
	       }
	    *curr = score ;
	    cogptr[i*len2 + j].srclength = match1 ;
	    cogptr[i*len2 + j].trglength = match2 ;
	    }
	 }
      if (align)
	 {
	 FrCognateAlignment *al = new FrCognateAlignment[len1+len2+1] ;
	 if (al)
	    {
	    *align = al ;
	    size_t count(0) ;
	    int srcpos(0), trgpos(0) ;
	    // read out the forced alignment corresponding to the best-scoring
	    //   path through the DP scoring table
	    while (srcpos < len1 && trgpos < len2)
	       {
	       FrCognateAlignment *cogalign = &cogptr[srcpos*len2 + trgpos] ;
	       al[count].srclength = cogalign->srclength ;
	       srcpos += cogalign->srclength ;
	       al[count].trglength = cogalign->trglength ;
	       trgpos += cogalign->trglength ;
	       count++ ;
	       }
	    }
	 }
      FrLocalFree(cogptr) ;
      double len = score_by_shorter ? FrMin(len1,len2) : FrMax(len1,len2) ;
      if (score_by_average)
	 {
	 if (len2 > len1)
	    len = (len1 + len2) / 2.0 ;
	 else
	    len = len1 ;
	 }
      double score = cogscore[0] / (len * EXACT_MATCH_SCORE) ;
//for(i=0;i<len1;i++){printf("%c  ",string1[i]);for(j=0;j<len2;j++)printf("%5d",VALUE(cogscore,i,j));printf("\n");}
      FrLocalFree(cogscore) ;
      return score ;
      }
   else
      {
      if (align)
	 *align = 0 ;
      return -1.0 ;
      }
}

//----------------------------------------------------------------------

double FrCognateScore(const FrObject *word1, const FrObject *word2,
		      bool casefold, bool score_by_shorter,
		      bool score_by_average,
		      bool exact_letter_match_only,
		      FrCognateAlignment **align)
{
   const char *string1 = FrPrintableName(word1) ;
   const char *string2 = FrPrintableName(word2) ;
   return FrCognateScore(string1,string2,casefold,score_by_shorter,
			 score_by_average,exact_letter_match_only,align) ;
}

// end of file frcognat.cpp //
