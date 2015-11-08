/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC								*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frspell.h		spelling-correction functions		*/
/*  LastEdit: 09nov2015							*/
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

#ifndef __FRSPELL_H_INCLUDED
#define __FRSPELL_H_INCLUDED

#ifndef __FRHASH_H_INCLUDED
#include "frhash.h"
#endif

/************************************************************************/
/************************************************************************/

class FrLetterConfusionMatrix
   {
   private:

   public:
      FrLetterConfusionMatrix() ;
      ~FrLetterConfusionMatrix() ;

      static FrLetterConfusionMatrix *load(const char *filename) ;
      bool save(const char *filename) const ;

      double score(char letter1, char letter2) const ;
      double score(const char *seq1, const char *seq2) const ;
   } ;

//----------------------------------------------------------------------

class FrSpellCorrectionData
   {
   public:
      FrObjHashTable *m_good_words ;
      const FrSymCountHashTable *m_wordcounts ;
      FrLetterConfusionMatrix *m_confmatrix ;
   public:
      FrSpellCorrectionData(FrObjHashTable *gw = nullptr,
			    const FrSymCountHashTable *wc = nullptr,
			    FrLetterConfusionMatrix *cm = nullptr)
	 { m_good_words = gw ; m_wordcounts = wc ; m_confmatrix = cm ; }
      ~FrSpellCorrectionData()
	 {
	 m_good_words = nullptr ;
	 m_wordcounts = nullptr ;
	 m_confmatrix = nullptr ; 
	 }
   } ;

//----------------------------------------------------------------------

bool FrSpellingKnownPhrase(const char *term, const FrObjHashTable *good_words,
			   bool allow_normalization = true, char split_char = ' ') ;
bool FrSpellingSuggestion(const char *term, char *suggestion,
			  const FrSpellCorrectionData *corr_data,
			  // the letters to be considered as possible replacements
			  const char *typo_letters = 0,
			  bool allow_normalization = true) ;
// note: 'suggestion' must point at a buffer at least twice as long as 'term'!
FrList *FrSpellingSuggestions(const char *term,
			      const FrSpellCorrectionData *corr_data,
			      // the letters to be considered as possible replacements
			      const char *typo_letters = 0,
			      bool allow_normalization = true,
			      bool allow_self = true) ;
// return value: list of lists, each containing a suggestion and the corresponding frequency

#endif /* !__FRSPELL_H_INCLUDED */

// end of file frspell.h //
