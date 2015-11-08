/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC								*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File: frstring.h	class FrString, FrConstString, and related	*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,1998,1999,2000,2001,2002,2003	*/
/*		2004,2006,2007,2008,2009,2010,2015			*/
/*		Ralf Brown/Carnegie Mellon University			*/
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

#ifndef __FRSTRING_H_INCLUDED
#define __FRSTRING_H_INCLUDED

#ifndef __FROBJECT_H_INCLUDED
#include "frobject.h"
#endif

#include <stdio.h>

#if defined(__GNUC__)
#  pragma interface
#endif

/**********************************************************************/
/**********************************************************************/

int FrStringCmp(const void *s1, const void *s2, int length, int width1,
		int width2) ;

class FrReader ;

#define FrCHARWIDTH_SHIFT 29

class FrString : public FrAtom
   {
   private:
      static FrAllocator allocator ;
      static FrAllocator str_allocator ;
   protected: // data
      unsigned char *m_value ;
   private:
#if defined(FrSAVE_MEMORY) && (UINT_MAX >= 0xFFFFFFFFUL)
      unsigned int m_length : 30 ;
      unsigned int m_width_bits : 2 ;
#else
      uint32_t m_length ;
      unsigned int m_charwidth ;
#endif /* FrSAVE_MEMORY2 */
   protected: // methods
      static void bad_char_width(const char *func) ;
      static void unsupp_char_size(const char *where) ;
#if defined(FrSAVE_MEMORY) && (UINT_MAX >= 0xFFFFFFFFUL)
      void setLength(size_t newlen) { m_length = (unsigned)newlen ; }
      void setCharWidth(int width){ m_width_bits = (width - (width>>2) - 1) ; }
	// (above converts 1->0, 2->1, 4->2)
#else
      void setLength(size_t newlen) { m_length = (uint32_t)newlen ; }
      void setCharWidth(int width) { m_charwidth = width ; }
#endif /* FrSAVE_MEMORY2 */
      void list_to_string(const FrList *words, FrCharEncoding enc,
			  bool addblanks) ;

      static unsigned char *reserve(size_t size) ;
      static unsigned char *reserve(size_t len, int width)
	 	{ return reserve((len+1)*width) ; }
      // manipulating the actual string buffer
      bool alloc(size_t size) ;
      bool alloc(size_t len, int width)
	 	{ return alloc((len+1)*width) ; }
      bool realloc(size_t newsize) ;  //!!!
      bool realloc(size_t newlen, int width)
	 	{ return realloc((newlen+1)*width) ; }
      void free() ;
   public:
      void *operator new(size_t) { return allocator.allocate() ; }
      void operator delete(void *frstr,size_t) { allocator.release(frstr) ; }
      FrString() ;
      FrString(const void *name) ;
      FrString(FrChar_t ch, int width = 1) ;
      FrString(const void *name, int len) ; // width=1
      FrString(const void *name, int len, int width) ;
      FrString(char *name, int len, int width, bool copybuf) ;
      FrString(const FrList *words, FrCharEncoding enc = FrChEnc_Latin1,
		bool addblanks = true) ;
      FrString(const FrList *words, bool addblanks,
		bool lowercase_symbols = true) ;
      virtual ~FrString() ;
      virtual bool stringp() const ;
      virtual FrObjectType objType() const ;
      virtual const char *objTypeName() const ;
      virtual FrObjectType objSuperclass() const ;
      virtual void freeObject() ;
      virtual FrReader *objReader() const ;
      virtual long int intValue() const ;
      virtual const char *printableName() const ;
      virtual FrSymbol *coerce2symbol(FrCharEncoding enc) const ;
      virtual ostream &printValue(ostream &output) const ;
      virtual char *displayValue(char *buffer) const ;
      virtual size_t displayLength() const ;
      virtual bool equal(const FrObject *obj) const ;
      virtual int compare(const FrObject *obj) const ;
      virtual bool iterateVA(FrIteratorFunc func, va_list) const ;
      virtual FrObject *copy() const ;
      virtual FrObject *deepcopy() const ;
      virtual unsigned long hashValue() const ;
      virtual size_t length() const ;
      virtual FrObject *reverse() ;
      virtual FrObject *subseq(size_t start, size_t stop) const ;
      virtual FrObject *getNth(size_t N) const ;
      virtual bool setNth(size_t N, const FrObject *elt) ;
      virtual size_t locate(const FrObject *item,
			    size_t start = (size_t)-1) const ;
      virtual size_t locate(const FrObject *item,
			    FrCompareFunc func,
			    size_t start = (size_t)-1) const ;
      virtual FrObject *insert(const FrObject *,size_t location,
			        bool copyitem = true) ;
      virtual FrObject *elide(size_t start, size_t end) ;
      const char *stringValue() const { return (const char *)m_value ; }
#if defined(FrSAVE_MEMORY) && (UINT_MAX >= 0xFFFFFFFFUL)
      size_t stringLength() const { return m_length ; }
      int charWidth() const { return (int)(1 << m_width_bits) ; }
      size_t stringSize() const { return m_length * charWidth() ; }
#else
      size_t stringLength() const { return m_length ; }
      int charWidth() const { return (int)m_charwidth ; }
      size_t stringSize() const { return m_length * m_charwidth ; }
#endif /* FrSAVE_MEMORY2 */
      FrString *concatenate(const FrString *s) const ;
      FrString *concatenate(const FrString &s) const
		{ return concatenate(&s) ;}
      FrString *append(const FrString *s) ;
      FrString *append(const FrString &s) { return append(&s) ; }
      FrString *append(const char *s) ;
      FrString *append(const FrChar_t c) ;
      void setChar(size_t index, FrChar_t newch) ;
      void lowercaseString(FrCharEncoding enc = FrChEnc_Latin1) ;
      void uppercaseString(FrCharEncoding enc = FrChEnc_Latin1) ;
      void wordWrap(size_t width) ;
   // overloaded operators
      operator char* () const { return (char*)m_value ; }  // unsafe!  Will remove in upcoming version
      operator const char* () const { return (const char*)m_value ; }
      FrChar_t operator [] (size_t index) const ;
      FrChar_t nthChar(size_t index) const ;
      FrString *operator = (const FrString &s) ;
      FrString *operator = (const FrString *s) ;
      FrString *operator = (const char *s) ;
      FrString *operator = (const FrChar_t) ;
      FrString *operator + (const FrString &s) const { return concatenate(s) ; }
      FrString *operator + (const FrString *s) const { return concatenate(s) ; }
      FrString *operator += (const FrString &s) { return append(s) ; }
      FrString *operator += (const FrString *s) { return append(s) ; }
      FrString *operator += (const char *s)	  { return append(s) ; }
      FrString *operator += (const FrChar_t c)   { return append(c) ; }
      int operator == (const FrString &s) const
	 { return (stringLength() == s.stringLength()) &&
	          (FrStringCmp(stringValue(),s.stringValue(),
			       stringLength(),charWidth(),
			       s.charWidth())==0) ;
         }
      int operator == (const FrString *s) const
	 { return (stringLength() == s->stringLength()) &&
	          (FrStringCmp(stringValue(),s->stringValue(),
			       stringLength(),charWidth(),
			       s->charWidth()) == 0) ;
         }
      int operator == (const char *s) const
	 { return FrStringCmp(stringValue(),s,stringLength()+1,
			      charWidth(),1) == 0 ; }
      int operator != (const FrString &s) const
	 { return (stringLength() != s.stringLength()) ||
	          (FrStringCmp(stringValue(),s.stringValue(),
			       stringLength(),charWidth(),
			       s.charWidth()) != 0) ;
         }
      int operator != (const FrString *s) const
	 { return (stringLength() != s->stringLength()) ||
	          (FrStringCmp(stringValue(),s->stringValue(),
			       stringLength(),charWidth(),
			       s->charWidth()) != 0) ;
         }
      int operator != (const char *s) const
	 { return FrStringCmp(stringValue(),s,stringLength()+1,
			      charWidth(),1) != 0 ; }
      int operator < (const FrString &s) const
	 { return FrStringCmp(stringValue(),s.stringValue(),
			      stringLength()+1,charWidth(),
			      s.charWidth()) < 0 ; }
      int operator < (const FrString *s) const
	 { return FrStringCmp(stringValue(),s->stringValue(),
			      stringLength()+1,charWidth(),
			      s->charWidth()) < 0 ; }
      int operator < (const char *s) const
	 { return FrStringCmp(stringValue(),s,stringLength()+1,
			      charWidth(),1) < 0 ; }
      int operator <= (const FrString &s) const
	 { return FrStringCmp(stringValue(),s.stringValue(),
			      stringLength()+1,charWidth(),
			      s.charWidth()) <= 0 ;}
      int operator <= (const char *s) const
	 { return FrStringCmp(stringValue(),s,stringLength()+1,
			      charWidth(),1) <= 0 ; }
      int operator <= (const FrString *s) const
	 { return FrStringCmp(stringValue(),s->stringValue(),
			      stringLength()+1,charWidth(),
			      s->charWidth()) <= 0 ; }
      int operator > (const FrString &s) const
	 { return FrStringCmp(stringValue(),s.stringValue(),
			      stringLength()+1,charWidth(),
			      s.charWidth()) > 0 ; }
      int operator > (const char *s) const
	 { return FrStringCmp(stringValue(),s,stringLength()+1,
			      charWidth(),1) > 0 ; }
      int operator > (const FrString *s) const
	 { return FrStringCmp(stringValue(),s->stringValue(),
			      stringLength()+1,charWidth(),
			      s->charWidth()) > 0 ; }
      int operator >= (const FrString &s) const
	 { return FrStringCmp(stringValue(),s.stringValue(),stringLength()+1,
			      charWidth(),s.charWidth())>=0; }
      int operator >= (const char *s) const
	 { return FrStringCmp(stringValue(),s,stringLength()+1,
			      charWidth(),1) >= 0 ; }
      int operator >= (const FrString *s) const
	 { return FrStringCmp(stringValue(),s->stringValue(),
			      stringLength()+1,charWidth(),
			      s->charWidth()) >= 0 ; }

      // utility
      static unsigned long hashValue(const char *s) ;
      static unsigned long hashValue(const char *s, size_t len) ;

      // debugging
      static void dumpUnfreed(ostream &out) ;
   } ;

//----------------------------------------------------------------------

class FrConstString : public FrString
   {
   private:
      // no additional private data members
   protected:
      // no additional protected data members
   public:
      FrConstString() ;
      FrConstString(const void *name) ;
      FrConstString(const void *name, size_t len) ;
      virtual ~FrConstString() ;
      //virtual bool stringp() const ;
      virtual FrObjectType objType() const ;
      virtual const char *objTypeName() const ;
      //virtual FrObjectType objSuperclass() const ;
      virtual void freeObject() ;
      //virtual FrReader *objReader() const ;
      //virtual long int intValue() const ;
      //virtual const char *printableName() const ;
      //virtual FrSymbol *coerce2symbol(FrCharEncoding enc) const ;
      //virtual ostream &printValue(ostream &output) const ;
      //virtual char *displayValue(char *buffer) const ;
      //virtual size_t displayLength() const ;
      //virtual bool equal(const FrObject *obj) const ;
      //virtual int compare(const FrObject *obj) const ;
      //virtual bool iterateVA(FrIteratorFunc func, va_list) const ;
      virtual FrObject *copy() const ;
      virtual FrObject *deepcopy() const ;
      //virtual unsigned long hashValue() const ;
      //virtual size_t length() const ;
      //virtual FrObject *reverse() ;
      //virtual FrObject *subseq(size_t start, size_t stop) const ;
      //virtual FrObject *getNth(size_t N) const ;
      virtual bool setNth(size_t N, const FrObject *elt) ;
      //virtual size_t locate(const FrObject *item,
      // 		      size_t start = (size_t)-1) const ;
      //virtual size_t locate(const FrObject *item,
      //		      FrCompareFunc func,
      //		      size_t start = (size_t)-1) const ;
      virtual FrObject *insert(const FrObject *,size_t location,
			        bool copyitem = true) ;
      virtual FrObject *elide(size_t start, size_t end) ;

      //FrString *concatenate(const FrString *s) const ;
      //FrString *concatenate(const FrString &s) const
      //   { return concatenate(&s) ;}
      FrString *append(const FrString * /*s*/) { return 0 ; }
      FrString *append(const FrString & /*s*/) { return 0 ; }
      FrString *append(const char * /*s*/) { return 0 ; }
      FrString *append(const FrChar_t /*c*/) { return 0 ; }
      void setChar(size_t /*index*/, FrChar_t /*newch*/) { return ; }
      void lowercaseString(FrCharEncoding = FrChEnc_Latin1) { return ; }
      void uppercaseString(FrCharEncoding = FrChEnc_Latin1) { return ; }
      void wordWrap(size_t /*width*/) { return ; }
   // overloaded operators
      operator const char* () const { return (const char*)m_value ; }
      FrChar_t operator [] (size_t index) const ;
      //FrChar_t nthChar(size_t index) const ;  // inherit from FrString
      FrString *operator + (const FrString &s) const { return concatenate(s) ; }
      FrString *operator + (const FrString *s) const { return concatenate(s) ; }
      int operator == (const FrString &s) const
         { return this->FrString::operator==(s) ; }
      int operator == (const FrString *s) const
         { return this->FrString::operator==(s) ; }
      int operator == (const char *s) const
         { return this->FrString::operator==(s) ; }
      int operator != (const FrString &s) const
         { return this->FrString::operator!=(s) ; }
      int operator != (const FrString *s) const
         { return this->FrString::operator!=(s) ; }
      int operator != (const char *s) const
         { return this->FrString::operator!=(s) ; }
      int operator < (const FrString &s) const
         { return this->FrString::operator<(s) ; }
      int operator < (const FrString *s) const
         { return this->FrString::operator<(s) ; }
      int operator < (const char *s) const
         { return this->FrString::operator<(s) ; }
      int operator <= (const FrString &s) const
         { return this->FrString::operator<=(s) ; }
      int operator <= (const char *s) const
         { return this->FrString::operator<=(s) ; }
      int operator <= (const FrString *s) const
         { return this->FrString::operator<=(s) ; }
      int operator > (const FrString &s) const
         { return this->FrString::operator>(s) ; }
      int operator > (const char *s) const
         { return this->FrString::operator>(s) ; }
      int operator > (const FrString *s) const
         { return this->FrString::operator>(s) ; }
      int operator >= (const FrString &s) const
         { return this->FrString::operator>=(s) ; }
      int operator >= (const char *s) const
         { return this->FrString::operator>=(s) ; }
      int operator >= (const FrString *s) const
         { return this->FrString::operator>=(s) ; }
   } ;

//----------------------------------------------------------------------

class FrCognateData ;			// opaque except in frcognat.cpp

class FrCognateAlignment
   {
   public:
      size_t srclength ;
      size_t trglength ;
   public:
      void *operator new(size_t size) { return FrMalloc(size) ; }
      void operator delete(void *blk, size_t) { FrFree(blk) ; }
      FrCognateAlignment() { srclength = 0 ; trglength = 0 ; }
   } ;

//----------------------------------------------------------------------

// utility functions

int Fr_strnicmp(const char *s1, const char *s2, size_t n) ;
int Fr_strnicmp(const char *s1, const char *s2, size_t n, FrCharEncoding enc) ;
int Fr_strnicmp(const char *s1, const char *s2, size_t n, 
		const FrCasemapTable map) ;
char *Fr_strichr(const char *s, char c) ;
char *Fr_strichr(const char *s, char c, FrCharEncoding enc) ;
char *Fr_stristr(const char *s, const char *pattern) ;
char *Fr_stristr(const char *s, const char *pattern, FrCharEncoding enc) ;

char *Fr_strupr(char *s, FrCharEncoding encoding) ;
char *Fr_strlwr(char *s, FrCharEncoding encoding) ;
char *Fr_strmap(char *s, const char *mapping) ;

// return a pointer to the first occurrence of the marker character, or the
//   end-of-string NUL character if none
char *FrTruncationPoint(char *s, char marker) ;
const char *FrTruncationPoint(const char *s, char marker) ;

const char *FrStdWordDelimiters(FrCharEncoding encoding = FrChEnc_Latin1) ;
const unsigned char *FrStdIsAlphaTable(FrCharEncoding encoding = FrChEnc_Latin1) ;
FrCasemapTable FrUppercaseTable(FrCharEncoding enc = FrChEnc_Latin1) ;
FrCasemapTable FrLowercaseTable(FrCharEncoding enc = FrChEnc_Latin1) ;

void FrSetUserWordDelimiters(const char *delim) ;
	// 'delim' *must* point at a 256-byte table which remains valid
	//   until this function is called again with a different arg or NULL
	// if 'delim' is NULL, encoding FrChEnc_User becomes equivalent to
	//   FrChEnc_Latin1
void FrSetUserIsAlphaTable(const unsigned char *delim) ;
	// 'delim' *must* point at a 256-byte table which remains valid
	//   until this function is called again with a different arg or NULL
	// if 'delim' is NULL, encoding FrChEnc_User becomes equivalent to
	//   FrChEnc_Latin1

// convert a string containing a one-line sentence into a form with exactly
// one blank between adjacent words/punctation/etc.  Deallocate the result
// with FrFree() when done
char *FrCanonicalizeSentence(const char *string,
			     FrCharEncoding = FrChEnc_RawOctets,
			     bool force_uppercase = true,
			     const char *possible_delim /*[256]*/ = 0,
			     char const * const *abbrevs = 0) ;

char *FrCanonicalizeUSentence(const FrChar16 *string, size_t length = 0,
			      bool force_uppercase = false,
			      const char *possible_delim /*[256]*/ = 0,
			      char const * const *abbrevs = 0) ;

FrString *FrDecanonicalizeSentence(const char *string,
				   FrCharEncoding enc = FrChEnc_Latin1) ;
FrString *FrDecanonicalizeUSentence(const char *string,
				    bool force_lowercase = false) ;

// convert a string of ASCII characters into a string of Unicode characters,
// optionally canonicalizing the result.  The returned results must be
// deallocated with FrFree() when no longer needed.
char *FrASCII2Unicode(const char *string, bool canonicalize = false) ;

// convert a string containing a list of words separated by exactly one
// blank each into a list of FrString or FrSymbol
FrList *FrCvtSentence2Symbollist(char *sentence,  // DESTROYS ARG!
				  bool force_uppercase) ;
FrList *FrCvtSentence2Symbollist(const char *sentence,
				  bool force_uppercase) ;
FrList *FrCvtSentence2Symbollist(char *sentence,  // DESTROYS ARG!
				  FrCharEncoding enc = FrChEnc_Latin1) ;
FrList *FrCvtSentence2Symbollist(const char *sentence,
				  FrCharEncoding enc = FrChEnc_Latin1) ;
FrList *FrCvtSentence2Wordlist(const char *sentence) ;

// convert a string containing a one-line sentence into a list of strings,
// one per word or other token (i.e. punctuation) in the original sentence
FrList *FrCvtString2Wordlist(const char *sentence,
			      const char *possible_delim /*[256]*/ = 0,
			      char const * const *abbrevs = 0,
			      FrCharEncoding = FrChEnc_Latin1) ;
// ditto, but for Unicode characters; if non-zero, 'length' specifies the
// string's length
FrList *FrCvtUString2Wordlist(const FrChar16 *sentence, size_t length = 0,
			       const char *possible_delim /*[256]*/ = 0,
			       char const * const *abbrevs = 0) ;

FrList *FrCvtWordlist2Symbollist(const FrList *words,
				  bool force_uppercase) ;
FrList *FrCvtWordlist2Symbollist(const FrList *words,
	 			  FrCharEncoding enc = FrChEnc_Latin1) ;
FrList *FrCvtString2Symbollist(const char *sentence,
			        const char *possible_delim /*[256]*/ = 0,
				bool force_uppercase = true) ;
FrList *FrCvtString2Symbollist(const char *sentence,
			        FrCharEncoding enc,
				bool force_uppercase = true) ;
FrList *FrCvtUString2Symbollist(const FrChar16 *sentence,
				 const char *possible_delim /*[256]*/ = 0) ;
inline FrList *FrCvtString2Symbollist(const unsigned char *sentence,
				       bool force_uppercase = true)
   { return FrCvtString2Symbollist((const char*)sentence,0,force_uppercase) ; }

FrSymbol *FrCvt2Symbol(const FrObject *word, bool force_uppercase) ;
inline FrSymbol *FrCvt2Symbol(const FrObject *word,
			FrCharEncoding enc = FrChEnc_Latin1)
    { return word ? word->coerce2symbol(enc) : 0 ; }
FrSymbol *FrCvtString2Symbol(const FrString *word, bool force_uppercase) ;
FrSymbol *FrCvtString2Symbol(const char *word, bool force_uppercase) ;
FrSymbol *FrCvtString2Symbol(const FrString *word,
			      FrCharEncoding enc = FrChEnc_Latin1) ;
FrSymbol *FrCvtString2Symbol(const char *word,
			      FrCharEncoding enc = FrChEnc_Latin1) ;

FrCasemapTable FrMakeCharacterMap(const FrList *map) ;
void FrDestroyCharacterMap(FrCasemapTable mapping) ;
// destructively apply the indicated character mapping to the string
char *FrMapString(char *string, const FrCasemapTable mapping) ;

// extract words from a string containing multiple words delimited by blanks
FrString *FrFirstWord(const FrString *words) ;
FrString *FrLastWord(const FrString *words) ;
FrString *FrButLastWord(const FrString *words, size_t numwords = 1) ;

// splitting long strings into sentences
void FrSetNonbreakingAbbreviations(const FrList *abbrevs,
				   FrCharEncoding enc = FrChEnc_Latin1) ;
const char *FrSentenceBreak(const char *line,
			    FrCharEncoding enc = FrChEnc_Latin1,
			    size_t max_words = (size_t)~0) ;
const FrChar16 *FrSentenceBreak(const FrChar16 *line) ;
FrList *FrBreakIntoSentences(const char *line,
			      FrCharEncoding enc = FrChEnc_Latin1) ;

// interlanguage cognate words
void FrSetDefaultCognateLetters() ;
void FrSetCognateLetters(const FrList *cognates,
			 size_t fuzzy_match_score = 0) ; // 0 = don't change
void FrSetCognateLetters(const char *cognates,
			 size_t fuzzy_match_score = 0) ; // 0 = don't change
void FrSetCognateScoring(size_t fuzzy_match_score = 0) ; // 0 = don't change
bool FrLoadCognateLetters(const char *filename,
			  size_t fuzzy_match_score = 0) ; // 0 = don't change
bool FrSaveCognateLetters(FrCognateData *&cognates) ;
bool FrExactCognateMatchOnly(FrCognateData *&cognates) ;
bool FrRestoreCognateLetters(FrCognateData *cognates, bool free = false) ;

bool FrCognateLetters(char letter1, char letter2,
			bool casefold = true) ;
size_t FrCognateLetters(const char *str1, const char *str2,
			size_t &len1, size_t &len2, bool casefold = true) ;
		// len1 and len2 on entry are max number of chars from str1
		//   and str2 to match; on exit, actual number matched

double FrCognateScore(const char *word1, const char *word2,
		      bool casefold = true,
		      bool score_relative_to_shorter = false,
		      bool score_relative_to_average = false, //has priority
		      bool exact_letter_match_only = false,
		      FrCognateAlignment **align = 0) ;
		// if 'align' is nonzero, use delete[] on return val when done
double FrCognateScore(const FrObject *word1, const FrObject *word2,
		      bool casefold = true,
		      bool score_relative_to_shorter = false,
		      bool score_relative_to_average = false, //has priority
		      bool exact_letter_match_only = false,
		      FrCognateAlignment **align = 0) ;
		// if 'align' is nonzero, use delete[] on return val when done

bool FrWildcardMatch(const char *pattern, const char *str,
		       bool match_case = true) ;

// support for abbreviations in string canonicalization
char **FrLoadAbbreviationList(FILE *fp) ;
char **FrLoadAbbreviationList(const char *filename) ;
void FrFreeAbbreviationList(char **abbrevs) ;

// internal support functions
bool FrIsKnownAbbreviation(const char *start, const char *end,
			     char const * const *abbrevs) ;


#endif /* !__FRSTRING_H_INCLUDED */

// end of file frstring.h //
