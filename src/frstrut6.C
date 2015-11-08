/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File: frstrut4.cpp	 	string-manipulation utility functions	*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 2000,2001,2003,2004,2007,2009				*/
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
#include "frbytord.h"
#include "frstring.h"
#include "frsymtab.h"
#include "frunicod.h"
#include "frutil.h"

/************************************************************************/
/************************************************************************/

//----------------------------------------------------------------------

FrSymbol *FrCvtString2Symbol(const char *word, FrCharEncoding enc)
{
   char buf[FrMAX_SYMBOLNAME_LEN+2] ;
   char *s1 = buf ;
   char *end = buf+FrMAX_SYMBOLNAME_LEN ;
   const unsigned char *map = FrUppercaseTable(enc) ;
   do {
      *s1++ = (char)map[(unsigned char)(*word++)] ;
      } while (s1[-1] && s1 <= end) ;
   *s1 = '\0' ;
   return FrSymbolTable::add(buf) ;
}

//----------------------------------------------------------------------

FrSymbol *FrCvtString2Symbol(const FrString *word, FrCharEncoding enc)
{
   if (!word)
      return 0 ;
   char buf[FrMAX_SYMBOLNAME_LEN+2] ;
   switch (word->charWidth())
      {
      case 1:
	 {
	 const unsigned char *map = FrUppercaseTable(enc) ;
	 char *s1 ;
	 s1 = buf ;
	 const char *s2 = (const char*)word->stringValue() ;
	 char *end = buf+FrMAX_SYMBOLNAME_LEN ;
	 do {
	    *s1++ = (char)map[(unsigned char)(*s2++)] ;
	    } while (s1[-1] && s1 <= end) ;
	 *s1 = '\0' ;
	 return FrSymbolTable::add(buf) ;
	 }
      case 2:
	 {
	 const FrChar16 *s2 = (const FrChar16*)word->stringValue() ;
	 size_t i ;
	 for (i = 0 ; i < FrMAX_SYMBOLNAME_LEN ; )
	    {
	    FrChar16 codepoint = FrByteSwap16(*s2++) ;
	    bool byteswap = false ;
	    i += Fr_Unicode_to_UTF8(codepoint,buf+i,byteswap) ;
	    }
	 buf[i] = '\0' ;
	 return FrSymbolTable::add(buf) ;
	 }
      case 4:
	 FrWarning("32-bit characters not yet supported by FrCvtString2Symbol!") ;
	 return FrSymbolTable::add("") ;
      default:
	 FrProgErrorVA("bad character width in FrCvtString2Symbol") ;
	 return 0 ;
      }
}

//----------------------------------------------------------------------

FrSymbol *FrCvtString2Symbol(const char *word, bool force_uppercase)
{
   return FrCvtString2Symbol(word,force_uppercase ? FrChEnc_Latin1
			     			  : FrChEnc_RawOctets) ;
}


//----------------------------------------------------------------------

FrSymbol *FrCvtString2Symbol(const FrString *word, bool force_uppercase)
{
   return FrCvtString2Symbol(word,force_uppercase ? FrChEnc_Latin1
			     			  : FrChEnc_RawOctets) ;
}

//----------------------------------------------------------------------

FrList *FrCvtWordlist2Symbollist(const FrList *words, bool force_uppercase)
{
   FrList *syms = 0 ;
   FrList **end = &syms ;
   for ( ; words ; words = words->rest())
      {
      FrString *word = (FrString*)words->first() ;
      if (!word)
	 continue ;
      else if (word->stringp())
	 syms->pushlistend(FrCvtString2Symbol((FrString*)word,
					      force_uppercase),end) ;
      else if (word->symbolp())
	 syms->pushlistend(word,end) ;
      }
   *end = 0 ;				// properly terminate the result
   return syms ;
}

//----------------------------------------------------------------------

FrList *FrCvtWordlist2Symbollist(const FrList *words, FrCharEncoding enc)
{
   FrList *syms = 0 ;
   FrList **end = &syms ;
   for ( ; words ; words = words->rest())
      {
      FrString *word = (FrString*)words->first() ;
      if (!word)
	 continue ;
      else if (word->stringp())
	 syms->pushlistend(FrCvtString2Symbol((FrString*)word,enc),end) ;
      else if (word->symbolp())
	 syms->pushlistend(word,end) ;
      }
   *end = 0 ;				// properly terminate the result
   return syms ;
}

//----------------------------------------------------------------------
// convert a sentence with words delimited by single blanks into a list
// of symbols, one per word in the original sentence
// MODIFIES BUT LATER RESTORES ITS ARGUMENT (except that a trailing newline
// is permanently removed)

FrList *FrCvtSentence2Symbollist(char *sentence, bool force_uppercase)
{
   if (!sentence || !*sentence)
      return 0 ;
   char *end = sentence ;
   while (*end != '\0')
      end++ ;
   if (end[-1] == '\n') // strip trailing newline if present
      end-- ;
   *end = '\0' ;
   FrList *words = 0 ;
   FrList **w_end = &words ;
   char *blank ;
   for (blank = sentence ; blank < end ; blank++)
      {
      if (*blank == ' ')
	 {
	 *blank = '\0' ;
	 if (force_uppercase)
	    words->pushlistend(FrCvtString2Symbol(sentence,true),w_end) ;
	 else
	    words->pushlistend(FrSymbolTable::add(sentence),w_end) ;
	 *blank++ = ' ' ;	// restore the character we clobbered above
	 sentence = blank ;
	 }
      }
   if (sentence < end)
      {
      if (force_uppercase)
	 words->pushlistend(FrCvtString2Symbol(sentence,true),w_end) ;
      else
	 words->pushlistend(FrSymbolTable::add(sentence),w_end) ;
      }
   *w_end = 0 ;				// properly terminate the result
   return words ;
}

//----------------------------------------------------------------------

FrList *FrCvtSentence2Symbollist(const char *sentence, bool force_uppercase)
{
   char *sent = FrDupString(sentence) ;
   FrList *symlist = ::FrCvtSentence2Symbollist(sent,force_uppercase) ;
   FrFree(sent) ;
   return symlist ;
}

//----------------------------------------------------------------------
// convert a sentence with words delimited by single blanks into a list
// of symbols, one per word in the original sentence
// MODIFIES BUT LATER RESTORES ITS ARGUMENT (except that a trailing newline
// is permanently removed)

FrList *FrCvtSentence2Symbollist(char *sentence, FrCharEncoding enc)
{
   if (!sentence || !*sentence)
      return 0 ;
   char *end = sentence ;
   while (*end != '\0')
      end++ ;
   if (end[-1] == '\n') // strip trailing newline if present
      end-- ;
   *end = '\0' ;
   FrList *words = 0 ;
   FrList **w_end = &words ;
   char *blank ;
   for (blank = sentence ; blank < end ; blank++)
      {
      if (*blank == ' ')
	 {
	 *blank = '\0' ;
	 words->pushlistend(FrCvtString2Symbol(sentence,enc),w_end) ;
	 *blank++ = ' ' ;	// restore the character we clobbered above
	 sentence = blank ;
	 }
      }
   if (sentence < end)
      words->pushlistend(FrCvtString2Symbol(sentence,enc),w_end) ;
   *w_end = 0 ;				// properly terminate the result list
   return words ;
}

//----------------------------------------------------------------------


FrList *FrCvtSentence2Symbollist(const char *sentence, FrCharEncoding enc)
{
   char *sent = FrDupString(sentence) ;
   FrList *symlist = ::FrCvtSentence2Symbollist(sent,enc) ;
   FrFree(sent) ;
   return symlist ;
}

//----------------------------------------------------------------------

FrList *FrCvtString2Symbollist(const char *sentence,
			       const char *possible_delim,
			       bool force_uppercase)
{
   char *sent = FrCanonicalizeSentence(sentence,FrChEnc_RawOctets,
				       force_uppercase,possible_delim) ;
   if (!sent)
      return 0 ;
   FrList *syms = FrCvtSentence2Symbollist(sent) ;
   FrFree(sent) ;
   return syms ;
}

//----------------------------------------------------------------------

FrList *FrCvtString2Symbollist(const char *sentence,
			       FrCharEncoding enc,
			       bool force_uppercase)
{
   char *sent = FrCanonicalizeSentence(sentence,enc,force_uppercase,0) ;
   if (!sent)
      return 0 ;
   FrList *syms = FrCvtSentence2Symbollist(sent) ;
   FrFree(sent) ;
   return syms ;
}

// end of file frstrut6.cpp //
