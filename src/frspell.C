/************************************************************************/
/*									*/
/*  FramepaC								*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frspell.C		word-list based spelling correction	*/
/*  LastEdit: 27sep2015							*/
/*									*/
/*  (c) Copyright 2015 Ralf Brown/Carnegie Mellon University		*/
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

#include "frfloat.h"
#include "frspell.h"
#include "frstring.h"
#include "frsymtab.h"

/************************************************************************/
/*	Manifest Constants						*/
/************************************************************************/

// factor by which to reduce frequency if a term is in the training
//   data but not the good-words list; this lets us use a spelling
//   that we hadn't known if it is very frequent
#define UNLISTED_WORD_DISCOUNT  100

// factor by which to boost reported frequency of a suggestion if the
//   term being checked is already known in the dictionary; this
//   biases selection of a best alternative based on multiple class
//   members towards the known term(s) in the class.
#define KNOWN_BONUS 10

#define WORDPAIR_DISCOUNT 3

/************************************************************************/
/*	Types local to this module					*/
/************************************************************************/

class TypoSequence
   {
   public:
      // since we only handle 1- and 2-letter sequences at this point, we can just
      //   use hard-coded storage within the object itself
      char m_intended[4] ;		// what the person intended to write
      char m_actual[4] ;		// what the person actually typed
   public:
      TypoSequence() { m_intended[0] = m_actual[0] = '\0' ; } // clear strings

      // accessors
      bool noChange() const { return m_intended[0] == '\0' && m_actual[0] == '\0' ; }
      const char *intended() const { return m_intended ; }
      const char *actual() const { return m_actual ; }

      // modifiers
      void setChange(const char *from, const char *to) { setIntended(from) ; setActual(to) ; }
      void setIntended(char c) { m_intended[0] = c ; m_intended[1] = '\0' ; }
      void setActual(char c) { m_actual[0] = c ; m_actual[1] = '\0' ; }
      void setIntended(const char *s)
	 { strncpy(m_intended,s,sizeof(m_intended)) ; m_intended[sizeof(m_intended)-1] = '\0' ; }
      void setActual(const char *s)
	 { strncpy(m_actual,s,sizeof(m_actual)) ; m_actual[sizeof(m_actual)-1] = '\0' ; }
      void setSwapped(char c1, char c2)
	 {
	    m_intended[0] = c1 ; m_intended[1] = c2 ; m_intended[2] = '\0' ;
	    m_actual[0] = c2 ; m_actual[1] = c1 ; m_actual[2] = '\0' ;
	 }
 
   } ;

/************************************************************************/
/*	Globals								*/
/************************************************************************/

static const char default_typo_letters[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ " ;

/************************************************************************/
/*	Helper functions						*/
/************************************************************************/

static size_t count_words(const char *term, char splitchar)
{
   size_t count = 1 ;
   if (term)
      {
      for ( ; *term ; term++)
	 {
	 if (*term == splitchar)
	    count++ ;
	 }
      }
   return count ;
}

//----------------------------------------------------------------------

static size_t word_frequency(const char *term, const FrSpellCorrectionData *sc_data,
			     char splitchar = ' ', size_t base_freq = 1, bool recursing = false)
{
   if (!sc_data)
      return base_freq ;
   const FrSymCountHashTable *word_counts = sc_data->m_wordcounts ;
   if (!word_counts)
      return base_freq ;
   size_t freq = word_counts->lookup(FrSymbolTable::add(term)) ;
   const char *space ;
   size_t wordcount = recursing ? 1 : count_words(term,splitchar) ;
   if (freq == 0 && (space = strchr(term,splitchar)) != nullptr)
      {
      FrString word1(term,space-term) ;
      size_t freq1 = word_counts->lookup(FrSymbolTable::add(word1.stringValue())) ;
      if (freq1 > 0)
	 {
	 freq = word_frequency(space+1,sc_data,splitchar,base_freq,true) ;
	 size_t len = strlen(space+1) ;
	 len *= len ;
	 size_t len1 = word1.stringLength() ;
	 len1 *= len1 ;
	 freq = ((freq * len) + (freq1 * len1) + 1) / (len + len1) ;
	 }
      }
   if (wordcount > 1)
      {
      wordcount *= wordcount ;
      freq = (freq + (wordcount * WORDPAIR_DISCOUNT) - 1) / (wordcount * WORDPAIR_DISCOUNT) ;
      }
   return freq ? freq : base_freq ;
}

//----------------------------------------------------------------------

static int compare_freq(const FrObject *o1, const FrObject *o2)
{
   const FrList *l1 = static_cast<const FrList*>(o1) ;
   const FrList *l2 = static_cast<const FrList*>(o2) ;
   const FrInteger *freq1 = static_cast<FrInteger*>(l1->second()) ;
   const FrInteger *freq2 = static_cast<FrInteger*>(l2->second()) ;
   size_t f1 = freq1->intValue() ;
   size_t f2 = freq2->intValue() ;
   return (f1 > f2) ? -1 : ((f1 < f2) ? +1 : 0) ;
}

//----------------------------------------------------------------------

static bool better_suggestion(const char *suggest, char *best_suggest, size_t &max_freq,
			      const FrSpellCorrectionData *sc_data,
			      const TypoSequence &typo_seq, bool allow_normalization)
{
   const FrObjHashTable *good_words = sc_data->m_good_words ;
   const FrLetterConfusionMatrix *conf_matrix = sc_data->m_confmatrix ;
   double freq = 0.0 ;
   if (FrSpellingKnownPhrase(suggest,good_words,allow_normalization))
      {
      freq = word_frequency(suggest,sc_data,' ') ;
      }
   else if (strchr(suggest,' ') == nullptr &&
	    FrSpellingKnownPhrase(suggest,good_words,allow_normalization,'/'))
      {
      freq = word_frequency(suggest,sc_data,'/') ;
      }
   if (conf_matrix)
      {
      double discount = conf_matrix->score(typo_seq.intended(),typo_seq.actual()) ;
      freq *= discount ;
      }
   if (freq > max_freq)
      {
      strcpy(best_suggest,suggest) ;
      max_freq = freq ;
      return true ;
      }
   return false ;
}

//----------------------------------------------------------------------

static void add_if_known(const char *suggest, FrList *&suggestions,
			 const FrSpellCorrectionData *sc_data,
			 const TypoSequence &typo_seq, bool allow_normalization)
{
   const FrObjHashTable *good_words = sc_data->m_good_words ;
   const FrLetterConfusionMatrix *conf_matrix = sc_data->m_confmatrix ;
   if (FrSpellingKnownPhrase(suggest,good_words,allow_normalization))
      {
      double bonus = typo_seq.noChange() ? KNOWN_BONUS : 1.0 ;
      double freq = word_frequency(suggest,sc_data) * bonus ;
      if (conf_matrix)
	 {
	 freq *= conf_matrix->score(typo_seq.intended(),typo_seq.actual()) ;
	 }
      pushlist(new FrList(new FrString(suggest),new FrFloat(freq)),suggestions) ;
      }
   else if (strchr(suggest,' ') == nullptr &&
	    FrSpellingKnownPhrase(suggest,good_words,allow_normalization,'/'))
      {
      double freq = word_frequency(suggest,sc_data,'/') ;
      if (conf_matrix)
	 {
	 freq *= conf_matrix->score(typo_seq.intended(),typo_seq.actual()) ;
	 }
      pushlist(new FrList(new FrString(suggest),new FrFloat(freq)),suggestions) ;
      }      
   return ;
}

/************************************************************************/
/*	Methods for class FrLetterConfusionMatrix			*/
/************************************************************************/

FrLetterConfusionMatrix::FrLetterConfusionMatrix()
{
   //FIXME
   return ;
}

//----------------------------------------------------------------------

FrLetterConfusionMatrix::~FrLetterConfusionMatrix()
{
   //FIXME
   return ;
}

//----------------------------------------------------------------------

FrLetterConfusionMatrix *FrLetterConfusionMatrix::load(const char *filename)
{
   if (!filename || !*filename)
      return nullptr ;

   //FIXME
   return nullptr ;
}

//----------------------------------------------------------------------

bool FrLetterConfusionMatrix::save(const char *filename) const
{
   if (!filename || !*filename)
      return false ;

   //FIXME
   return false ;
}

//----------------------------------------------------------------------

double FrLetterConfusionMatrix::score(char letter1, char letter2) const
{
   if (letter1 == letter2)
      return 1.0 ;

   //FIXME
   return 1.0 ;
}

//----------------------------------------------------------------------

double FrLetterConfusionMatrix::score(const char *seq1, const char *seq2) const
{
   if (!seq1 || !seq2)
      return 0.0 ;
   if (strcmp(seq1,seq2) == 0)
      return 1.0 ;

   //FIXME
   return 1.0 ;
}

/************************************************************************/
/************************************************************************/

bool FrSpellingKnownPhrase(const char *term, const FrObjHashTable *good_words,
			   bool allow_normalization, char split_char)
{
   if (!good_words)
      return false ;
   FrString termstr(term) ;
   const FrSymbol *norm = (FrSymbol*)good_words->lookup(&termstr) ;
   if (norm)
      return allow_normalization || strcmp(term,norm->symbolName()) == 0 ;
   const char *space = strchr(term,split_char) ;
   if (space)
      {
      // the term is a phrase, so check the individual words
      FrString firstword(term,space-term) ;
      norm = (FrSymbol*)good_words->lookup(&firstword) ;
      if (!norm || (!allow_normalization && strcmp(firstword.stringValue(),norm->symbolName()) != 0))
	 return false ;
      return FrSpellingKnownPhrase(space+1,good_words,allow_normalization,split_char) ;
      }
   return false ;
}

//----------------------------------------------------------------------

bool FrSpellingSuggestion(const char *term, char *best_suggest,
			  const FrSpellCorrectionData *sc_data,
			  const char *typo_letters,
			  bool allow_normalization)
{
   if (!term || !*term || !sc_data || !sc_data->m_good_words)
      return false ;
   if (!typo_letters)
      typo_letters = default_typo_letters ;
   bool elide_blanks_only = false ;
   size_t termlen = strlen(term) ;
   char suggest[termlen+2] ;
   best_suggest[0] = '\0' ; 		// no suggestion yet
   size_t max_freq = 0 ;
   const FrObjHashTable *good_words = sc_data->m_good_words ;
   const FrSymCountHashTable *word_counts = sc_data->m_wordcounts ;
   TypoSequence typo_seq ;
   if (better_suggestion(suggest,best_suggest,max_freq,sc_data,typo_seq,allow_normalization))
      {
      if (strchr(term,' ') == nullptr)
	 {
	 return true ;
	 }
      elide_blanks_only = true ;
      }
   size_t max_freq2 = word_counts ? word_counts->lookup(FrSymbolTable::add(term)) / UNLISTED_WORD_DISCOUNT : 0 ;
   if (max_freq2 > max_freq)
      {
      strcpy(best_suggest,term) ;
      max_freq = max_freq2 ;
      }
   if (strchr(term,' ') != nullptr)
      {
      // the term has multiple words, so check whether there are
      //   extraneous blanks
      size_t len = 0 ;
      // mash the multiple 'words' into one
      for (size_t i = 0 ; i < termlen ; ++i)
	 {
	 if (!Fr_isspace(term[i]))
	    {
	    suggest[len++] = term[i] ;
	    }
	 }
      suggest[len] = '\0' ;
      // and check whether the concatenation is a known word
      if (FrSpellingKnownPhrase(suggest,good_words,allow_normalization))
	 {
	 strcpy(best_suggest,suggest) ;
	 return true ;
	 }
      }
   if (elide_blanks_only)
      return best_suggest[0] != '\0' ;
   strcpy(suggest,term) ;
   // check for swapped letters
   for (size_t i = 1 ; i < termlen ; ++i)
      {
      // swap the letter pair at the current position
      char tmp = suggest[i-1] ;
      suggest[i-1] = suggest[i] ;
      suggest[i] = tmp ;
      better_suggestion(suggest,best_suggest,max_freq,sc_data,typo_seq,allow_normalization) ;
      // unswap the letters for the next iteration
      tmp = suggest[i-1] ;
      suggest[i-1] = suggest[i] ;
      suggest[i] = tmp ;
      }
   // check for changed letter
   size_t typo_count = strlen(typo_letters) ;
   for (size_t i = 0 ; i < termlen ; ++i)
      {
      char c = suggest[i] ;
      for (size_t j = 0 ; j < typo_count ; ++j)
	 {
	 // don't check for the original letter, or a change from a
	 //   letter to a blank/slash (punctuation to blank/slash is OK)
	 if (typo_letters[j] == c || ((typo_letters[j] == ' ' || typo_letters[j] == '/') && !ispunct(c)))
	    continue ;
	 suggest[i] = typo_letters[j] ;
	 better_suggestion(suggest,best_suggest,max_freq,sc_data,typo_seq,allow_normalization) ;
	 }
      // restore the letter at the current position
      suggest[i] = c ;
      }
   // check for omitted letter
   strcpy(suggest+1,term) ;
   suggest[termlen+1] = '\0' ;
   for (size_t i = 0 ; i <= termlen ; ++i)
      {
      // don't insert before initial punctuation, as that would
      //   hallucinate a one-letter word
      if (i == 0 && ispunct(term[0]))
	 continue ;
      // ditto for trailing punctuation
      if (i == termlen && ispunct(term[termlen-1]))
	 continue ;
      // iterate through the possible replacement characters,
      //   inserting each at the current position
      for (size_t j = 0 ; j < typo_count ; ++j)
	 {
	 if (typo_letters[j] == ' ' && (i <= 1 || i + 1 >= termlen))
	    continue ;		// don't split off too-short fragment
	 suggest[i] = typo_letters[j] ;
	 better_suggestion(suggest,best_suggest,max_freq,sc_data,typo_seq,allow_normalization) ;
	 }
      // shift the gap for the inserted letter in prep for the next iteration
      suggest[i] = suggest[i+1] ;
      }
   // check for inserted letter
   for (size_t i = 0 ; i < termlen ; ++i)
      {
      memcpy(suggest,term,i) ;
      strcpy(suggest+i,term+i+1) ;
      better_suggestion(suggest,best_suggest,max_freq,sc_data,typo_seq,allow_normalization) ;
      }
   return best_suggest[0] != '\0' ;
}

//----------------------------------------------------------------------

FrList *FrSpellingSuggestions(const char *term, const FrSpellCorrectionData *sc_data,
			      const char *typo_letters, bool allow_normalization,
			      bool allow_self)
{
   if (!term || !*term || !sc_data || !sc_data->m_good_words)
      return nullptr ;
   if (!typo_letters)
      typo_letters = default_typo_letters ;
   FrList *suggestions = 0 ;
   TypoSequence typo_seq ;
   add_if_known(term,suggestions,sc_data,typo_seq,allow_normalization) ;
   bool elide_blanks_only = false ;
   if (suggestions)
      {
      if (strchr(term,' ') == nullptr)
	 return suggestions ;
      elide_blanks_only = true ;
      }
   size_t termlen = strlen(term) ;
   char suggest[termlen+2] ;
   // if the given term exists in the training data, use it as a
   //   suggestion even if it isn't in the good-words list
   const FrSymCountHashTable *word_counts = sc_data->m_wordcounts ;
   size_t freq = ((word_counts && allow_self)
		  ? word_counts->lookup(FrSymbolTable::add(term)) / UNLISTED_WORD_DISCOUNT
		  : 0) ;
   if (freq > 0)
      {
      pushlist(new FrList(new FrString(term),new FrInteger(freq)),suggestions) ;
      }
   if (strchr(term,' ') != nullptr)
      {
      // the term has multiple words, so check whether there are
      //   extraneous blanks
      size_t len = 0 ;
      // mash the multiple 'words' into one
      for (size_t i = 0 ; i < termlen ; ++i)
	 {
	 if (!Fr_isspace(term[i]))
	    {
	    suggest[len++] = term[i] ;
	    }
	 }
      suggest[len] = '\0' ;
      // and check whether the concatenation is a known word
      typo_seq.setChange(""," ") ;
      add_if_known(suggest,suggestions,sc_data,typo_seq,allow_normalization) ;
      }
   if (elide_blanks_only)
      return suggestions ;
   strcpy(suggest,term) ;
   // check for swapped letters
   for (size_t i = 1 ; i < termlen ; ++i)
      {
      // swap the letter pair at the current position
      char tmp = suggest[i-1] ;
      suggest[i-1] = suggest[i] ;
      suggest[i] = tmp ;
      typo_seq.setSwapped(suggest[i-1],suggest[i]) ;
      add_if_known(suggest,suggestions,sc_data,typo_seq,allow_normalization) ;
      // unswap the letters for the next iteration
      tmp = suggest[i-1] ;
      suggest[i-1] = suggest[i] ;
      suggest[i] = tmp ;
      }
   // check for changed letter
   strcpy(suggest,term) ;
   size_t typo_count = strlen(typo_letters) ;
   for (size_t i = 0 ; i < termlen ; ++i)
      {
      char c = suggest[i] ;
      typo_seq.setActual(c) ;
      for (size_t j = 0 ; j < typo_count ; ++j)
	 {
	 // don't check for the original letter, or a change from a
	 //   letter to a blank/slash (punctuation to blank/slash is OK)
	 if (typo_letters[j] == c || ((typo_letters[j] == ' ' || typo_letters[j] == '/') && !ispunct(c)))
	    continue ;
	 suggest[i] = typo_letters[j] ;
	 typo_seq.setIntended(typo_letters[j]) ;
	 add_if_known(suggest,suggestions,sc_data,typo_seq,allow_normalization) ;
	 }
      // restore the letter at the current position
      suggest[i] = c ;
      }
   // check for omitted letter
   strcpy(suggest+1,term) ;
   suggest[termlen+1] = '\0' ;
   typo_seq.setActual("") ;
   for (size_t i = 0 ; i <= termlen ; ++i)
      {
      // don't insert before initial punctuation, as that would
      //   hallucinate a one-letter word
      if (i == 0 && ispunct(term[0]))
	 continue ;
      // ditto for trailing punctuation
      if (i == termlen && ispunct(term[termlen-1]))
	 continue ;
      // iterate through the possible replacement characters,
      //   inserting each at the current position
      for (size_t j = 0 ; j < typo_count ; ++j)
	 {
	 if (typo_letters[j] == ' ' && (i <= 1 || i + 1 >= termlen))
	    continue ;		// don't split off too-short fragment
	 suggest[i] = typo_letters[j] ;
	 typo_seq.setIntended(typo_letters[j]) ;
	 add_if_known(suggest,suggestions,sc_data,typo_seq,allow_normalization) ;
	 }
      // shift the gap for the inserted letter in prep for the next iteration
      suggest[i] = suggest[i+1] ;
      }
   // check for inserted letter
   typo_seq.setIntended("") ;
   for (size_t i = 0 ; i < termlen ; ++i)
      {
      // drop the letter at position 'i' and check whether that produces a known term
      memcpy(suggest,term,i) ;
      strcpy(suggest+i,term+i+1) ;
      typo_seq.setActual(term[i]) ;
      add_if_known(suggest,suggestions,sc_data,typo_seq,allow_normalization) ;
      }
   return suggestions ? suggestions->sort(compare_freq) : 0 ;
}

// end of file frspell.C //
