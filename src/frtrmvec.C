/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC								*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File: frtrmvec.cpp	      term vectors				*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1999,2000,2001,2002,2003,2004,2005,2006,2009,2012,	*/
/*		2015 Ralf Brown/Carnegie Mellon University		*/
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

#if defined(__GNUC__)
#  pragma implementation "frtrmvec.h"
#endif

#include "framerr.h"
#include "frassert.h"
#include "frbpriq.h"
#include "frlist.h"
#include "frnumber.h"
#include "frhsort.h"
#include "frtrmvec.h"

#ifdef FrSTRICT_CPLUSPLUS
# include <cstdlib>
#else
# include <stdlib.h>
#endif /* FrSTRICT_CPLUSPLUS */
#include <float.h>		// WatcomC++ needs this for DBL_MAX
#include <math.h>

/************************************************************************/
/*	Manifest Constants						*/
/************************************************************************/

/************************************************************************/
/*	Global variables						*/
/************************************************************************/

FrAllocator FrTermVector::allocator("FrTermVector",sizeof(FrTermVector)) ;

#ifndef NDEBUG
#  undef _FrCURRENT_FILE
   static const char _FrCURRENT_FILE[] = __FILE__ ;
#endif /* !NDEBUG */

static size_t total_population = 0 ;

/************************************************************************/
/* 	Helper Functions						*/
/************************************************************************/

inline double maximum(double a, double b) { return a > b ? a : b ; }
inline double minimum(double a, double b) { return a < b ? a : b ; }

//----------------------------------------------------------------------

static int compare_word_stats(const FrObject *o1, const FrObject *o2)
{
   assertq(o1 && o2 && o1->consp() && o2->consp()) ;
   FrSymbol *word1 = (FrSymbol*)((FrCons*)o1)->first() ;
   FrSymbol *word2 = (FrSymbol*)((FrCons*)o2)->first() ;
   if (word1 && word2)
      {
      if (word1 < word2)
	 return -1 ;
      else if (word1 > word2)
	 return +1 ;
      return 0 ;
      }
   else if (word1)
      return -1 ;
   else if (word2)
      return +1 ;
   return 0 ;			// say that items are equal
}

/************************************************************************/
/*	Methods for FrTermVector					*/
/************************************************************************/

void FrTermVector::init()
{
   num_terms = 0 ;
   vector_length = 0.0 ;
   terms = 0 ;
   weights = 0 ;
   keysym = 0 ;
   clustername = 0 ;
   vector_freq = 0 ;
   user_data = 0 ;
   caching = false ;
   clearNearest() ;
   is_neighbor = false ;
   cluster_flag = false ;
   clustering_data = 0 ;
   return ;
}

//----------------------------------------------------------------------

void FrTermVector::init(FrList *wordlist, size_t num_words,
			bool is_sorted)
{
   init() ;
   if (!is_sorted)
      wordlist = wordlist->sort(compare_word_stats) ;
   num_terms = num_words ;
   weights = (double*)FrMalloc((num_terms+1)*
			       (sizeof(double)+sizeof(FrSymbol*))) ;
   if (!weights)
      {
      FrNoMemory("building term vector") ;
      while (wordlist)
	 {
	 FrCons *terminfo = (FrCons*)poplist(wordlist) ;
	 free_object(terminfo->consCdr()) ;
	 free_object(terminfo) ;
	 }
      num_terms = 0 ;
      FrFree(weights) ;
      weights = 0 ;
      }
   else
      {
      terms = (FrSymbol**)&weights[num_terms] ;
      for (size_t i = 0 ; i < num_terms ; i++)
	 {
	 FrCons *terminfo = (FrCons*)poplist(wordlist) ;
	 if (!terminfo)
	    continue ;
	 FrNumber *termweight = (FrNumber*)terminfo->consCdr() ;
	 FrSymbol *word = (FrSymbol*)terminfo->first() ;
	 terminfo->replacd(0) ;
	 double weight = termweight->floatValue() ;
	 free_object(termweight) ;
	 free_object(terminfo) ;
	 terms[i] = word ;
	 weights[i] = weight ;
	 vector_length += weight * weight ;
	 }
      vector_length = sqrt(vector_length) ;
      assertq(wordlist == 0) ;
      }
   return ;
}

//----------------------------------------------------------------------

FrTermVector::FrTermVector(const FrSparseArray *counts)
{
   init() ;
   if (counts)
      {
      num_terms = counts->arrayLength() ;
      weights = (double*)FrMalloc(num_terms*
				  (sizeof(double)+sizeof(FrSymbol*))) ;
      terms = (FrSymbol**)&weights[num_terms] ;
      size_t highestpos = counts->highestPosition() ;
      size_t dest = 0 ;
      for (size_t i = 0 ; i < highestpos ; i = counts->nextItem(i))
	 {
	 FrArrayItem *item = &counts->item(i) ;
	 FrSymbol *word = (FrSymbol*)item->getIndex() ;
	 size_t count = (size_t)item->getValue() ;
	 double weight = count ? (double)count : 1.0 ;
	 terms[dest] = word ;
	 weights[dest] = weight ;
	 vector_length += weight * weight ;
	 ++dest ;
	 }
      vector_length = sqrt(vector_length) ;
      }
   return ;
}

//----------------------------------------------------------------------

// internal class to sort hash table elements
class HashItem
   {
   public:
      FrSymbol *word ;
      double    weight ;

   public:
      HashItem(FrSymbol *w, double wt) { word = w ; weight = wt ; }
      HashItem(const HashItem &orig) { word = orig.word ; weight = orig.weight ; }

      static int compare(const HashItem &o1, const HashItem &o2)
	 {
	    const FrSymbol *p1 = o1.word ;
	    const FrSymbol *p2 = o2.word ;
	    return (p1 < p2) ? -1 : ((p1 > p2) ? +1 : 0) ;
	 }
      static void swap(HashItem &o1, HashItem &o2)
	 {
	    FrSymbol *t_word = o1.word ;
	    double t_weight = o1.weight ;
	    o1.word = o2.word ;
	    o1.weight = o2.weight ;
	    o2.word = t_word ;
	    o2.weight = t_weight ;
	 }
      HashItem &operator = (const HashItem &orig)
	 {
	    word = orig.word ;
	    weight = orig.weight ;
	    return *this ;
	 }
   } ;

static bool add_term(const FrSymbol *term, size_t weight, va_list args)
{
   FrVarArg(HashItem*,items) ;
   FrVarArg(size_t*,index) ;
   FrVarArg(double*,vector_length) ;
   items[*index].word = (FrSymbol*)term ;
   double wt = weight ? weight : 1.0 ;
   items[*index].weight = wt ;
   (*vector_length) += (wt * wt) ;
   ++(*index) ;
   return true ;
}

//----------------------------------------------------------------------

FrTermVector::FrTermVector(const FrSymCountHashTable *counts)
{
   init() ;
   if (counts)
      {
      num_terms = counts->currentSize() ;
      HashItem *items = FrNewN(HashItem,num_terms) ;
      size_t index = 0 ;
      counts->iterate(add_term,items,&index,&vector_length) ;
      FrSmoothSort(items, num_terms) ;
      weights = (double*)FrMalloc(num_terms*
				  (sizeof(double)+sizeof(FrSymbol*))) ;
      terms = (FrSymbol**)&weights[num_terms] ;
      for (size_t i = 0 ; i < num_terms ; ++i)
	 {
	 terms[i] = items[i].word ;
	 weights[i] = items[i].weight ;
	 }
      FrFree(items) ;
      vector_length = sqrt(vector_length) ;
      }
   return ;
}

//----------------------------------------------------------------------

FrTermVector::FrTermVector(const FrList *words)
{
   FrList *wordlist = 0 ;
   size_t num_words = 0 ;
   for (const FrList *w = words ; w ; w = w->rest())
      {
      FrObject *word = w->first() ;
      FrSymbol *term ;
      size_t count ;
      if (!word)
	 continue ;
      else if (word->consp())
	 {
	 term = (FrSymbol*)((FrCons*)word)->first() ;
	 count = ((FrCons*)word)->consCdr()->intValue() ;
	 }
      else if (word->symbolp())
	 {
	 term = (FrSymbol*)word ;
	 count = 1 ;
	 }
      else
	 continue ;
      pushlist(new FrCons(term,new FrInteger(count)),wordlist) ;
      num_words++ ;
      }
   init(wordlist,num_words,false) ;
   return ;
}

//----------------------------------------------------------------------

FrTermVector::FrTermVector(const FrTermVector &oldvect) : FrObject()
{
   num_terms = oldvect.num_terms ;
   vector_length = oldvect.vector_length ;
   weights = (double*)FrMalloc((num_terms+1)*
			       (sizeof(double)+sizeof(FrSymbol*))) ;
   terms = (FrSymbol**)&weights[num_terms] ;
   keysym = oldvect.keysym ;
   clustername = oldvect.clustername ;
   nearest_dist = oldvect.nearest_dist ;
   nearest_neighbor = oldvect.nearest_neighbor ;
   nearest_key = oldvect.nearest_key ;
   vector_freq = oldvect.vector_freq ;
   user_data = oldvect.user_data ;
   is_neighbor = oldvect.is_neighbor ;
   caching = oldvect.caching ;
   cluster_flag = oldvect.cluster_flag ;
   clustering_data = oldvect.clustering_data ; // need to be careful!!!
   if (caching)
      {
      FrBoundedPriQueue *n = (FrBoundedPriQueue*)nearest_neighbor ;
      nearest_neighbor = (FrTermVector*)new FrBoundedPriQueue(n) ;
      }
   if (terms && weights)
      {
      for (size_t i = 0 ; i < num_terms ; i++)
	 {
	 terms[i] = oldvect.terms[i] ;
	 weights[i] = oldvect.weights[i] ;
	 }
      }
   else
      {
      terms = 0 ;
      FrFree(weights) ;
      weights = 0 ;
      num_terms = 0 ;
      vector_length = 0.0 ;
      vector_freq = 0 ;
      }
   return ;
}

//----------------------------------------------------------------------

void FrTermVector::clearVector()
{
   num_terms = 0 ;
   terms = 0 ;
   FrFree(weights) ;
   weights = 0 ;
   vector_length = 0.0 ;
   return ;
}

//----------------------------------------------------------------------

FrTermVector::~FrTermVector()
{
   clearVector() ;
   user_data = 0 ;
   if (caching)
      {
      FrBoundedPriQueue *n = (FrBoundedPriQueue*)nearest_neighbor ;
      delete n ;
      nearest_neighbor = 0 ;
      caching = false ;
      }
   clearNearest() ;
   return ;
}

//----------------------------------------------------------------------

void FrTermVector::freeObject()
{
   delete this ;
   return ;
}

//----------------------------------------------------------------------

const char *FrTermVector::objTypeName() const
{
   return "FrTermVector" ;
}

//----------------------------------------------------------------------

ostream &FrTermVector::printValue(ostream &output) const
{
   output << "#" << objTypeName() << "<" << num_terms << "," << vector_length
	  << "=" ;
   if (terms && weights)
      {
      for (size_t i = 0 ; i < num_terms ; i++)
	 {
	 output << terms[i] << '/' << weights[i] ;
	 if (i < num_terms-1)
	    output << ',' ;
	 }
      }
   output << '>' ;
   return output ;
}

//----------------------------------------------------------------------

char *FrTermVector::displayValue(char *buffer) const
{
   memcpy(buffer,"#FrTermVector<",14) ;
   buffer += 14 ;
   ultoa(num_terms,buffer,10) ;
   buffer = strchr(buffer,'\0') ;
   *buffer++ = ',' ;
   ultoa((unsigned long)vector_length,buffer,10) ; //!!!(s.b. float!)
   buffer = strchr(buffer,'\0') ;
   *buffer++ = '>' ;
   *buffer = '\0' ;
   return buffer ;
}

//----------------------------------------------------------------------

size_t FrTermVector::displayLength() const
{
   return 16 + Fr_number_length(num_terms) +
      Fr_number_length((int)vector_length) ; //!!!(need length of float!)
}

//----------------------------------------------------------------------

FrObject *FrTermVector::deepcopy() const
{
   return new FrTermVector(*this) ;
}

//----------------------------------------------------------------------

void FrTermVector::setCache(size_t cache_size)
{
   if (!caching)
      {
      nearest_neighbor = (FrTermVector*)new FrBoundedPriQueue(cache_size) ;
      caching = true ;
      }
   return ;
}

//----------------------------------------------------------------------

void FrTermVector::weightTerms(FrTermVectorWeightingFunc *weightfn,
			       void *userdata)
{
   if (!weightfn)
      return ;
   vector_length = 0.0 ;
   for (size_t term = 0 ; term < num_terms ; term++)
      {
      double new_weight = weightfn(terms[term],weights[term],userdata) ;
      weights[term] = new_weight ;
      vector_length += (new_weight * new_weight) ;
      }
   vector_length = sqrt(vector_length) ;
   return ;
}

//----------------------------------------------------------------------

void FrTermVector::normalize()
{
   if (vector_length != 1.0)
      {
      for (size_t term = 0 ; term < num_terms ; term++)
	 weights[term] /= vector_length ;
      vector_length = 1.0 ;
      }
   return ;
}

//----------------------------------------------------------------------

void FrTermVector::mergeIn(const FrTermVector *other, bool use_max)
{
   if (!other || other->num_terms == 0 || !other->terms)
      return ;
   size_t term1(0) ;
   size_t term2(0) ;
   size_t total_terms = 0 ;
   FrSymbol *word2 = other->terms[0] ;
   // count the number of terms in the union of the two vectors
   while (term1 < num_terms && term2 < other->num_terms)
      {
      FrSymbol *word1 = terms[term1] ;
      total_terms++ ;
      if (word1 <= word2)
	 term1++ ;			// advance first term list
      if (word1 >= word2)
	 {
	 term2++ ;			// advance second term list
	 if (term2 < other->num_terms)
	    word2 = other->terms[term2] ;
	 }
      }
   total_terms += (num_terms - term1) + (other->num_terms - term2) ;
   if (total_terms == num_terms)
      {
      // no new terms introduced by the other vector, so do an in-place update
      term1 = 0 ;
      term2 = 0 ;
      vector_length = 0.0 ;
      word2 = other->terms[0] ;
      while (term1 < num_terms && term2 < other->num_terms)
	 {
	 FrSymbol *word1 = terms[term1] ;
	 if (word1 == word2)
	    {
	    weights[term1] += other->weights[term2++] ;
	    if (term2 < other->num_terms)
	       word2 = other->terms[term2] ; // advance second term list
	    }
	 else if (word1 > word2)
	    {
	    // this CAN'T happen if no new terms are introduced!
	    FrProgError("in-place FrTermVector::mergeIn found term not yet\n"
			"present in the vector!") ;
	    }
	 double wt = weights[term1++] ;
	 vector_length += wt * wt ;
	 }
      while (term1 < num_terms)
	 {
	 double wt = weights[term1++] ;
	 vector_length += wt * wt ;
	 }
      vector_length = sqrt(vector_length) ;
      if (use_max)
	 {
	 if (other->vector_freq > vector_freq)
	    vector_freq = other->vector_freq ;
	 }
      else
	 vector_freq += other->vector_freq ;
      return ;
      }
   // for the larger term vectors, reduce memory fragmentation by limiting the
   //   number of different allocation sizes
   if (total_terms > 1024)
      total_terms = (total_terms + 127) & 0xFFFFFF80 ;
   double *newweights =
      (double*)FrMalloc((total_terms+1)*(sizeof(double)+sizeof(FrSymbol*))) ;
   FrSymbol **newterms = (FrSymbol**)&newweights[total_terms] ;
   if (newterms && newweights)
      {
      // merge the two vectors, computing new length
      term1 = 0 ;
      term2 = 0 ;
      vector_length = 0.0 ;
      size_t new_numtrm = 0 ;
      word2 = other->terms[0] ;
      while (term1 < num_terms && term2 < other->num_terms)
	 {
	 FrSymbol *word1 = terms[term1] ;
	 double wt ;
	 if (word1 < word2)
	    {
	    newterms[new_numtrm] = word1 ;
	    wt = weights[term1] ;
	    term1++ ;			// advance first term list
	    }
	 else if (word1 > word2)
	    {
	    newterms[new_numtrm] = word2 ;
	    wt = other->weights[term2] ;
	    term2++ ;			// advance second term list
	    if (term2 < other->num_terms)
	       word2 = other->terms[term2] ;
	    }
	 else // if (word1 == word2)
	    {
	    newterms[new_numtrm] = word1 ;
	    if (use_max)
	       wt = maximum(weights[term1],other->weights[term2]) ;
	    else
	       wt = (weights[term1] + other->weights[term2]) ;
	    term1++ ;			// advance first term list
	    term2++ ;			// advance second term list
	    if (term2 < other->num_terms)
	       word2 = other->terms[term2] ;
	    }
	 vector_length += wt * wt ;
	 newweights[new_numtrm++] = wt ;
	 }
      while (term1 < num_terms)
	 {
	 newterms[new_numtrm] = terms[term1] ;
	 newweights[new_numtrm] = weights[term1] ;
	 vector_length += weights[term1] * weights[term1] ;
	 term1++ ;
	 new_numtrm++ ;
	 }
      while (term2 < other->num_terms)
	 {
	 newterms[new_numtrm] = other->terms[term2] ;
	 newweights[new_numtrm] = other->weights[term2] ;
	 vector_length += newweights[new_numtrm] * newweights[new_numtrm] ;
	 term2++ ;
	 new_numtrm++ ;
	 }
      num_terms = new_numtrm ;
      vector_length = sqrt(vector_length) ;
      FrFree(weights) ;
      terms = newterms ;
      weights = newweights ;
      if (use_max)
	 {
	 if (other->vector_freq > vector_freq)
	    vector_freq = other->vector_freq ;
	 }
      else
	 vector_freq += other->vector_freq ;
      }
   else
      {
      // out of memory!  Leave the term vector unchanged and report error
      FrFree(newweights) ;
      FrNoMemory("merging term vectors") ;
      }
   return ;
}

//----------------------------------------------------------------------

void FrTermVector::termMatches(const FrTermVector *othervect,
				double &match1_to_2, double &match2_to_1,
				double &total_1, double &total_2) const
{
   match1_to_2 = 0.0 ;
   match2_to_1 = 0.0 ;
   total_1 = 0.0 ;
   total_2 = 0.0 ;

   size_t term1(0) ;
   size_t term2(0) ;
   FrSymbol *word1(terms[0]) ;
   FrSymbol *word2(othervect->terms[0]) ;
   size_t other_terms(othervect->num_terms) ;
   for ( ; ; )
      {
      if (word1 <= word2)
	 {
	 double wt1 = weights[term1++] ;
	 total_1 += wt1 ;
	 if (word1 == word2)
	    {
	    // we have a word in common!
	    double wt2 = othervect->weights[term2++] ;
	    match1_to_2 += wt1 ;
	    match2_to_1 += wt2 ;
	    total_2 += wt2 ;
	    if (term2 >= other_terms)	// advance second term list
	       break ;
	    word2 = othervect->terms[term2] ;
	    }
	 if (term1 >= num_terms)	// advance first term list
	    break ;
	 word1 = terms[term1] ;
	 }
      else
	 {
	 total_2 += othervect->weights[term2++] ;
	 if (term2 >= other_terms)	// advance second term list
	    break ;
	 word2 = othervect->terms[term2] ;
	 }
      }
   while (term1 < num_terms)
      total_1 += weights[term1++] ;
   while (term2 < other_terms)
      total_2 += othervect->weights[term2++] ;
   return ;
}

//----------------------------------------------------------------------

void FrTermVector::termMatches(const FrTermVector *othervect,
				size_t &matches,
				size_t &total_1, size_t &total_2) const
{
   matches = 0 ;
   total_1 = 0 ;
   total_2 = 0 ;

   size_t term1(0) ;
   size_t term2(0) ;
   FrSymbol *word1(terms[0]) ;
   FrSymbol *word2(othervect->terms[0]) ;
   size_t other_terms(othervect->num_terms) ;
   for ( ; ; )
      {
      if (word1 <= word2)
	 {
	 size_t present = (weights[term1++] != 0.0) ;
	 total_1 += present ;
	 if (word1 == word2)
	    {
	    // we have a word in common!
	    size_t present2 = (othervect->weights[term2++] != 0.0) ;
	    if (present && present2)
	       matches++ ;
	    total_2 += present2 ;
	    if (term2 >= other_terms)	// advance second term list
	       {
	       while (term1 < num_terms)
		  {
		  if (weights[term1++] != 0)
		     total_1++ ;
		  }
	       break ;
	       }
	    word2 = othervect->terms[term2] ;
	    }
	 if (term1 >= num_terms)	// advance first term list
	    {
	    while (term2 < other_terms)
	       {
	       if (othervect->weights[term2++])
		  total_2++ ;
	       }
	    break ;
	    }
	 word1 = terms[term1] ;
	 }
      else
	 {
	 if (othervect->weights[term2++] != 0.0)
	    total_2++ ;
	 if (term2 >= other_terms)	// advance second term list
	    {
	    while (term1 < num_terms)
	       {
	       if (weights[term1++] != 0)
		  total_1++ ;
	       }
	    break ;
	    }
	 word2 = othervect->terms[term2] ;
	 }
      }
   return ;
}

//----------------------------------------------------------------------

double FrTermVector::binaryDiceCoefficient(const FrTermVector *othervect)
const
{
   if (!othervect || othervect->num_terms == 0)
      return num_terms == 0 ? 1.0 : 0.0 ; // no overlap, but same if also null
   size_t total_1, total_2, intersection ;
   termMatches(othervect,intersection,total_1,total_2) ;
   return (2.0 * intersection) / (total_1 + total_2) ;
}

//----------------------------------------------------------------------

double FrTermVector::binaryAntiDiceCoefficient(const FrTermVector *othervect)
const
{
   if (!othervect || othervect->num_terms == 0)
      return num_terms == 0 ? 1.0 : 0.0 ; // no overlap, but same if also null
   size_t total_1, total_2, intersection ;
   termMatches(othervect,intersection,total_1,total_2) ;
   size_t diff = (total_1 - intersection) + (total_2 - intersection) ;
   return intersection / (double)(intersection + 2 * diff) ;
}

//----------------------------------------------------------------------

double FrTermVector::binaryGammaCoefficient(const FrTermVector *othervect)
const
{
   if (!othervect || othervect->num_terms == 0)
      return num_terms == 0 ? 1.0 : -1.0 ; // no overlap, but same if also null
   size_t term1(0) ;
   size_t term2(0) ;
   FrSymbol *word1(terms[0]) ;
   FrSymbol *word2(othervect->terms[0]) ;
   size_t other_terms(othervect->num_terms) ;
   size_t both(0) ;
   size_t only_1(0) ;
   size_t only_2(0) ;
   size_t neither(0) ;
   for ( ; term1 < num_terms && term2 < other_terms ; )
      {
      if (word1 <= word2)
	 {
	 size_t present = (weights[term1++] != 0.0) ;
	 if (word1 == word2)
	    {
	    // we have a word in common
	    if (othervect->weights[term2++] != 0.0)
	       {
	       if (present)
		  both++ ;
	       else
		  only_2++ ;
	       }
	    else if (present)
	       only_1++ ;
	    }
	 else if (present)
	    only_1++ ;
	 }
      else if (othervect->weights[term2++] != 0.0)
	 only_2++ ;
      else
	 neither++ ;
      }
   while (term1 < num_terms)
      {
      if (weights[term1++] != 0.0)
	 only_1++ ;
      else
	 neither++ ;
      }
   while (term2 < other_terms)
      {
      if (othervect->weights[term2++] != 0.0)
	 only_2++ ;
      else
	 neither++ ;
      }
   double N(both + only_1 + only_2 + neither) ;
   if (total_population > N)
      {
      neither += (size_t)(total_population - N) ;
      N = total_population ;
      }
   double concordance(both / N * neither / N) ;
   double discordance(only_1 / N * only_2 / N) ;
   if (concordance + discordance > 0)
      return (concordance - discordance) / (concordance + discordance) ;
   else
      return 1.0 ;
}

//----------------------------------------------------------------------

double FrTermVector::jaccardCoefficient(const FrTermVector *othervect) const
{
   if (!othervect || othervect->num_terms == 0)
      return num_terms == 0 ? 1.0 : 0.0 ; // no overlap, but same if also null
   size_t total_1, total_2, intersection ;
   termMatches(othervect,intersection,total_1,total_2) ;
   // the size of the union is the sum of the terms in each vector, less the
   //   number of common terms (which would otherwise be double-counted)
   size_t union_size = total_1 + total_2 - intersection ;
   return intersection / (double)union_size ;
}

//----------------------------------------------------------------------

double FrTermVector::simpsonCoefficient(const FrTermVector *othervect) const
{
   if (!othervect || othervect->num_terms == 0)
      return num_terms == 0 ? 1.0 : 0.0 ; // no overlap, but match if also null
   size_t total_1, total_2, intersection ;
   termMatches(othervect,intersection,total_1,total_2) ;
   size_t smaller = total_1 < total_2 ? total_1 : total_2 ;
   return intersection / (double)smaller ;
}

//----------------------------------------------------------------------

#ifdef __686__DONT_USE
// because this function is such a hot spot in clustering, it's worth
//  having multiple versions optimized to varying architectures
double FrTermVector::cosine(const FrTermVector *othervect) const
{
   if (vector_length == 0.0 || othervect->vector_length == 0.0)
      return 0.0 ;
//   register const FrTermVector *other = othervect ;
#define other othervect
#define terms2 (other->terms)
   double cosin(0.0) ;
   register size_t term1(num_terms) ;
   register size_t term2(other->num_terms-1) ;
   while (--term1 > 0 && term2 > 0)
      {
      register FrSymbol *word1 = terms[term1] ;
      while (word1 < terms2[term2])
	 {
	 if (term2 == 0)
	    break ;
	 term2-- ;
	 }
      if (word1 == terms2[term2])
	 cosin += (weights[term1] * other->weights[term2]) ;
      }
   while (term1-- > 0)
      {
      if (terms[term1] == terms2[0])
	 {
	 cosin += weights[term1] * other->weights[0] ;
	 break ;
	 }
      }
   while (term2-- > 0)
      {
      if (terms[0] == terms2[term2])
	 {
	 cosin += weights[0] * other->weights[term2] ;
	 break ;
	 }
      }
   return cosin / (vector_length * other->vector_length) ;
}
#undef other
#undef terms2
#undef word2
#endif /* __686__ */

//----------------------------------------------------------------------

double FrTermVector::cosine(const FrTermVector *othervect) const
{
   if (vector_length == 0.0 || othervect->vector_length == 0.0)
      return 0.0 ;
   size_t term1(0) ;
   size_t term2(0) ;
   FrSymbol *word1(terms[0]) ;
   FrSymbol *word2(othervect->terms[0]) ;
   size_t other_terms(othervect->num_terms) ;
   double cosin(0.0) ;
   for ( ; ; )
      {
      if (word1 <= word2)
	 {
	 if (word1 == word2)
	    {
	    cosin += (weights[term1] * othervect->weights[term2++]) ;
	    if (term2 >= other_terms)
	       break ;
	    word2 = othervect->terms[term2] ;
	    }
	 if (++term1 >= num_terms)	// advance first term list
	    break ;
	 word1 = terms[term1] ;
	 }
      else
	 {
	 if (++term2 >= other_terms)	// advance second term list
	    break ;			//   and quit if at and
	 word2 = othervect->terms[term2] ;
	 }
      }
   return cosin / (vector_length * othervect->vector_length) ;
}

//----------------------------------------------------------------------

double FrTermVector::euclideanDistance(const FrTermVector *othervect) const
{
   if (!othervect || othervect->vector_length == 0.0 || num_terms == 0)
      return vector_length ;		// assume other vector is at origin
   size_t term1(0) ;
   size_t term2(0) ;
   FrSymbol *word1(terms[0]) ;
   FrSymbol *word2(othervect->terms[0]) ;
   size_t other_terms(othervect->num_terms) ;
   double sum(0.0) ;
   for ( ; ; )
      {
      if (word1 <= word2)
	 {
	 if (word1 == word2)
	    {
	    double dist = weights[term1++] - othervect->weights[term2++] ;
	    sum += dist * dist ;
	    if (term2 >= other_terms)
	       {
	       // add in all remaining terms from this vector
	       while (term1 < num_terms)
		  {
		  dist = weights[term1++] ;
		  sum += dist * dist ;
		  }
	       break ;
	       }
	    word2 = othervect->terms[term2] ;
	    }
	 else
	    {
	    sum += weights[term1] * weights[term1] ;
	    term1++ ;
	    }
	 if (term1 >= num_terms)		// advance first term list
	    {
	    // add in all remaining terms from other vector
	    while (term2 < other_terms)
	       {
	       double dist = othervect->weights[term2++] ;
	       sum += dist * dist ;
	       }
	    break ;
	    }
	 word1 = terms[term1] ;
	 }
      else
	 {
	 double dist = othervect->weights[term2++] ;
	 sum += dist * dist ;
	 if (term2 >= other_terms)	// advance second term list
	    {
	    // add in all remaining terms from this vector
	    while (term1 < num_terms)
	       {
	       dist = weights[term1++] ;
	       sum += dist * dist ;
	       }
	    break ;
	    }
	 word2 = othervect->terms[term2] ;
	 }
      }
   return sqrt(sum) ;
}

//----------------------------------------------------------------------

double FrTermVector::manhattanDistance(const FrTermVector *othervect) const
{
   if (!othervect || othervect->vector_length == 0.0)
      {
      double dist(0.0) ;
      for (size_t i = 0 ; i < num_terms ; i++)
	 dist += fabs(weights[i]) ;
      return dist ;
      }
   else if (num_terms == 0)
      {
      double dist(0.0) ;
      for (size_t i = 0 ; i < othervect->num_terms ; i++)
	 dist += fabs(othervect->weights[i]) ;
      return dist ;
      }
   size_t term1(0) ;
   size_t term2(0) ;
   FrSymbol *word1(terms[0]) ;
   FrSymbol *word2(othervect->terms[0]) ;
   size_t other_terms(othervect->num_terms) ;
   double sum(0.0) ;
   for ( ; ; )
      {
      if (word1 <= word2)
	 {
	 double wt1 = weights[term1++] ;
	 if (word1 == word2)
	    {
	    double dist = wt1 - othervect->weights[term2++] ;
	    sum += fabs(dist) ;
	    if (term2 >= other_terms)
	       {
	       // add in all remaining terms from this vector
	       while (term1 < num_terms)
		  sum += fabs(weights[term1++]) ;
	       break ;
	       }
	    word2 = othervect->terms[term2] ;
	    }
	 else
	    sum += fabs(wt1) ;
	 if (term1 >= num_terms)	// advance first term list
	    {
	    // add in all remaining terms from other vector
	    while (term2 < other_terms)
	       sum += fabs(othervect->weights[term2++]) ;
	    break ;
	    }
	 word1 = terms[term1] ;
	 }
      else
	 {
	 sum += fabs(othervect->weights[term2++]) ;
	 if (term2 >= other_terms)	// advance second term list
	    {
	    // add in all remaining terms from this vector
	    while (term1 < num_terms)
	       sum += fabs(weights[term1++]) ;
	    break ;
	    }
	 word2 = othervect->terms[term2] ;
	 }
      }
   return sqrt(sum) ;
}

//----------------------------------------------------------------------

double FrTermVector::tanimotoCoefficient(const FrTermVector *othervect) const
{
   if (!othervect || othervect->num_terms == 0)
      return num_terms == 0 ? 1.0 : 0.0 ; // no overlap, but same if also null
   size_t term1(0) ;
   size_t term2(0) ;
   FrSymbol *word1(terms[0]) ;
   FrSymbol *word2(othervect->terms[0]) ;
   size_t other_terms(othervect->num_terms) ;
   double union_size(0.0) ;
   double intersection(0.0) ;
   for ( ; ; )
      {
      if (word1 <= word2)
	 {
	 double wt1 = weights[term1++] ;
	 union_size += wt1 ;
	 if (word1 == word2)
	    {
	    double wt2 = othervect->weights[term2++] ;
	    intersection += wt1 * wt2 ;
	    if (wt2 > wt1)
	       union_size += wt2-wt1 ;
	    if (term2 >= other_terms)	// advance second term list
	       break ;
	    word2 = othervect->terms[term2] ;
	    }
	 if (term1 >= num_terms)	// advance first term list
	    {
	    while (term2 < other_terms)
	       union_size += othervect->weights[term2++] ;
	    break ;
	    }
	 word1 = terms[term1] ;
	 }
      else
	 {
	 union_size += othervect->weights[term2++] ;
	 if (term2 >= other_terms)	// advance second term list
	    break ;
	 word2 = othervect->terms[term2] ;
	 }
      }
   while (term1 < num_terms)
      union_size += weights[term1++] ;
   if (union_size > intersection)
      return intersection / (union_size - intersection) ;
   else
      return 1.0 ;
}

//----------------------------------------------------------------------

double FrTermVector::extSimpsonCoefficient(const FrTermVector *othervect)
const
{
   if (!othervect || othervect->num_terms == 0)
      return num_terms == 0 ? 1.0 : 0.0 ; // no overlap, but match if also null
   size_t term1(0) ;
   size_t term2(0) ;
   FrSymbol *word1(terms[0]) ;
   FrSymbol *word2(othervect->terms[0]) ;
   size_t other_terms(othervect->num_terms) ;
   double intersection(0.0) ;
   double total_1(0.0) ;
   double total_2(0.0) ;
   for ( ; ; )
      {
      if (word1 <= word2)
	 {
	 double wt1 = weights[term1++] ;
	 total_1 += wt1 ;
	 if (word1 == word2)
	    {
	    double wt2 = othervect->weights[term2++] ;
	    intersection += minimum(wt1,wt2) ;	// we have a term in common!
	    total_2 += wt2 ;
	    if (term2 >= other_terms)	// advance second term list
	       break ;
	    word2 = othervect->terms[term2] ;
	    }
	 if (term1 >= num_terms)	// advance first term list
	    break ;
	 word1 = terms[term1] ;
	 }
      else
	 {
	 total_2 += othervect->weights[term2++] ;
	 if (term2 >= other_terms)	// advance second term list
	    break ;
	 word2 = othervect->terms[term2] ;
	 }
      }
   while (term1 < num_terms)
      total_1 += weights[term1++] ;
   while (term2 < othervect->num_terms)
      total_2 += othervect->weights[term2++] ;
   double smaller = minimum(total_1,total_2) ;
   return intersection / smaller ;
}

//----------------------------------------------------------------------

double FrTermVector::braunBlanquetCoefficient(const FrTermVector *othervect)
const
{
   if (!othervect || othervect->num_terms == 0)
      return num_terms == 0 ? 1.0 : 0.0 ; // no overlap, but match if also null
   size_t term1(0) ;
   size_t term2(0) ;
   FrSymbol *word1(terms[0]) ;
   FrSymbol *word2(othervect->terms[0]) ;
   size_t other_terms(othervect->num_terms) ;
   double intersection(0.0) ;
   double total_1(0.0) ;
   double total_2(0.0) ;
   for ( ; ; )
      {
      if (word1 <= word2)
	 {
	 double wt1 = weights[term1++] ;
	 total_1 += wt1 ;
	 if (word1 == word2)
	    {
	    double wt2 = othervect->weights[term2++] ;
	    intersection += minimum(wt1,wt2) ;	// we have a term in common!
	    total_2 += wt2 ;
	    if (term2 >= other_terms)	// advance second term list
	       break ;
	    word2 = othervect->terms[term2] ;
	    }
	 if (term1 >= num_terms)	// advance first term list
	    break ;
	 word1 = terms[term1] ;
	 }
      else
	 {
	 total_2 += othervect->weights[term2++] ;
	 if (term2 >= other_terms)	// advance second term list
	    break ;
	 word2 = othervect->terms[term2] ;
	 }
      }
   while (term1 < num_terms)
      total_1 += weights[term1++] ;
   while (term2 < othervect->num_terms)
      total_2 += othervect->weights[term2++] ;
   double larger = maximum(total_1,total_2) ;
   return intersection / larger ;
}

//----------------------------------------------------------------------

double FrTermVector::diceCoefficient(const FrTermVector *othervect) const
{
   if (!othervect || othervect->num_terms == 0)
      return num_terms == 0 ? 1.0 : 0.0 ; // no overlap, but match if also null
   double maxwt1(maximum(1.0,maxTermWeight())) ;
   double maxwt2(maximum(1.0,othervect->maxTermWeight())) ;
   size_t term1(0) ;
   size_t term2(0) ;
   FrSymbol *word1(terms[0]) ;
   FrSymbol *word2(othervect->terms[0]) ;
   size_t other_terms(othervect->num_terms) ;
   double prod(0.0), sum(0.0) ;
   for ( ; ; )
      {
      if (word1 <= word2)
	 {
	 double wt1 = weights[term1++] / maxwt1 ;
	 sum += wt1 ;
	 if (word1 == word2)
	    {
	    // we have a word in common!
	    double wt2 = othervect->weights[term2++] / maxwt2 ;
	    sum += wt2 ;
	    prod += wt1 * wt2 ;
	    if (term2 >= other_terms)	// advance second term list
	       break ;
	    word2 = othervect->terms[term2] ;
	    }
	 if (term1 >= num_terms)	// advance first term list
	    {
	    while (term2 < other_terms)
	       sum += othervect->weights[term2++] / maxwt2 ;
	    break ;
	    }
	 word1 = terms[term1] ;
	 }
      else
	 {
	 double wt2 = othervect->weights[term2++] / maxwt2 ;
	 sum += wt2 ;
	 if (term2 >= other_terms)	// advance second term list
	    break ;
	 word2 = othervect->terms[term2] ;
	 }
      }
   while (term1 < num_terms)
      sum += weights[term1++] / maxwt1 ;
   return (sum > 0.0) ? (2.0 * prod / sum) : 0.0 ;
}

//----------------------------------------------------------------------

double FrTermVector::antiDiceCoefficient(const FrTermVector *othervect) const
{
   if (!othervect || othervect->num_terms == 0)
      return num_terms == 0 ? 1.0 : 0.0 ; // no overlap, but match if also null
   double maxwt1(maximum(1.0,maxTermWeight())) ;
   double maxwt2(maximum(1.0,othervect->maxTermWeight())) ;
   size_t term1(0) ;
   size_t term2(0) ;
   FrSymbol *word1(terms[0]) ;
   FrSymbol *word2(othervect->terms[0]) ;
   size_t other_terms(othervect->num_terms) ;
   double prod(0.0), same(0.0), diff(0.0) ;
   for ( ; ; )
      {
      if (word1 <= word2)
	 {
	 double wt1 = weights[term1++] / maxwt1 ;
	 if (word1 == word2)
	    {
	    // we have a word in common!
	    double wt2 = othervect->weights[term2++] / maxwt2 ;
	    if (wt1 != 0 && wt2 != 0)
	       {
	       same += minimum(wt1,wt2) ;
	       prod += wt1 * wt2 ;
	       }
	    else
	       diff += (wt1 + wt2) ;
	    if (term2 >= other_terms)	// advance second term list
	       break ;
	    word2 = othervect->terms[term2] ;
	    }
	 else
	    diff += wt1 ;
	 if (term1 >= num_terms)	// advance first term list
	    {
	    while (term2 < other_terms)
	       diff += othervect->weights[term2++] / maxwt2 ;
	    break ;
	    }
	 word1 = terms[term1] ;
	 }
      else
	 {
	 diff += othervect->weights[term2++] / maxwt2 ;
	 if (term2 >= other_terms)	// advance second term list
	    break ;
	 word2 = othervect->terms[term2] ;
	 }
      }
   while (term1 < num_terms)
      diff += weights[term1++] / maxwt1 ;
   return (same+diff > 0.0) ? (prod / (same + 2.0*diff)) : 0.0 ;
}

//----------------------------------------------------------------------

double FrTermVector::kulczynskiMeasure1(const FrTermVector *othervect) const
{
   if (!othervect || othervect->num_terms == 0)
      return num_terms == 0 ? 1.0 : 0.0 ; // no overlap, but match if also null
   size_t term1(0) ;
   size_t term2(0) ;
   FrSymbol *word1(terms[0]) ;
   FrSymbol *word2(othervect->terms[0]) ;
   size_t other_terms(othervect->num_terms) ;
   double match(0.0), mismatch(0.0) ;
   for ( ; ; )
      {
      if (word1 <= word2)
	 {
	 double wt1 = weights[term1++] ;
	 if (word1 == word2)
	    {
	    // we have a word in common!
	    double wt2 = othervect->weights[term2++] ;
	    match += wt1 + wt2 ;
	    if (term2 >= other_terms)	// advance second term list
	       break ;
	    word2 = othervect->terms[term2] ;
	    }
	 else
	    mismatch += wt1 ;
	 if (term1 >= num_terms)	// advance first term list
	    {
	    while (term2 < other_terms)
	       mismatch += othervect->weights[term2++] ;
	    break ;
	    }
	 word1 = terms[term1] ;
	 }
      else
	 {
	 mismatch += othervect->weights[term2++] ;
	 if (term2 >= other_terms)	// advance second term list
	    break ;
	 word2 = othervect->terms[term2] ;
	 }
      }
   while (term1 < num_terms)
      mismatch += weights[term1++] ;
   if (mismatch)
      return match / mismatch ;
   else
//      return DBL_MAX ;
      return 2*match ;
}

//----------------------------------------------------------------------

double FrTermVector::kulczynskiMeasure2(const FrTermVector *othervect) const
{
   if (!othervect || othervect->num_terms == 0)
      return num_terms == 0 ? 1.0 : 0.0 ; // no overlap, but match if also null
   double sum1, sum2, match1, match2 ;
   termMatches(othervect,match1,match2,sum1,sum2) ;
   if (sum1 == 0.0 || sum2 == 0.0)
      return 0.0 ;
   else
      return (match1 / sum1 + match2 / sum2) / 2.0 ;
}

//----------------------------------------------------------------------

double FrTermVector::ochiaiMeasure(const FrTermVector *othervect) const
{
   if (!othervect || othervect->num_terms == 0)
      return num_terms == 0 ? 1.0 : 0.0 ; // no overlap, but match if also null
   double sum1, sum2, match1, match2 ;
   termMatches(othervect,match1,match2,sum1,sum2) ;
   if (sum1 == 0.0 || sum2 == 0.0)
      return 0.0 ;
   else
      return (0.5 * (match1 + match2)) / sqrt(sum1 * sum2) ;
}

//----------------------------------------------------------------------

double FrTermVector::sokalSneathMeasure(const FrTermVector *othervect) const
{
   if (!othervect || othervect->num_terms == 0)
      return num_terms == 0 ? 1.0 : 0.0 ; // no overlap, but match if also null
   double sum1, sum2, match1, match2 ;
   termMatches(othervect,match1,match2,sum1,sum2) ;
   double mismatch1 = sum1 - match1 ;
   double mismatch2 = sum2 - match2 ;
   double match = (0.5 * (match1 + match2)) ;
   return match / (match + 2.0*mismatch1 + 2.0*mismatch2) ;
}

//----------------------------------------------------------------------

double FrTermVector::mcConnaugheyMeasure(const FrTermVector *othervect) const
{
   if (!othervect || othervect->num_terms == 0)
      return num_terms == 0 ? 1.0 : 0.0 ; // no overlap, but match if also null
   double sum1, sum2, match1, match2 ;
   termMatches(othervect,match1,match2,sum1,sum2) ;
   if (sum1 == 0.0 || sum2 == 0.0)
      return 0.0 ;
   else
      {
      double mismatch1 = sum1 - match1 ;
      double mismatch2 = sum2 - match2 ;
      double match_sq = (match1 * match2) ;
      return (match_sq - mismatch1 * mismatch2) / (sum1 * sum2) ;
      }
}

//----------------------------------------------------------------------

double FrTermVector::lanceWilliamsDist(const FrTermVector *othervect) const
{
   if (!othervect || othervect->num_terms == 0 || num_terms == 0)
      return 1.0 ;			// maximal difference
   double sum1, sum2, match1, match2 ;
   termMatches(othervect,match1,match2,sum1,sum2) ;
   if (sum1 + sum2 == 0)
      return 1.0 ;
   else
      {
      double mismatch1 = sum1 - match1 ;
      double mismatch2 = sum2 - match2 ;
      return (mismatch1 + mismatch2) / (sum1 + sum2) ;
      }
}

//----------------------------------------------------------------------

double FrTermVector::brayCurtisMeasure(const FrTermVector *othervect) const
{
   if (vector_length == 0.0 || othervect->vector_length == 0.0)
      return 0.0 ;
   size_t term1(0) ;
   size_t term2(0) ;
   FrSymbol *word1(terms[0]) ;
   FrSymbol *word2(othervect->terms[0]) ;
   size_t other_terms(othervect->num_terms) ;
   double absdiff(0.0), sum(0.0) ;
   for ( ; ; )
      {
      if (word1 <= word2)
	 {
	 double wt1 = weights[term1++] ;
	 if (word1 == word2)
	    {
	    double wt2 = othervect->weights[term2++] ;
	    absdiff += fabs(wt1 - wt2) ;
	    sum += (wt1 + wt2) ;
	    if (term2 >= other_terms)
	       break ;
	    word2 = othervect->terms[term2] ;
	    }
	 else
	    {
	    absdiff += fabs(wt1) ;
	    sum += wt1 ;
	    }
	 if (term1 >= num_terms)	// advance first term list
	    break ;
	 word1 = terms[term1] ;
	 }
      else
	 {
	 double wt2 = othervect->weights[term2++] ;
	 absdiff += fabs(wt2) ;
	 sum += wt2 ;
	 if (term2 >= other_terms) // advance second term list
	    break ;			      //   and quit if at and
	 word2 = othervect->terms[term2] ;
	 }
      }
   while (term1 < num_terms)
      {
      absdiff += fabs(weights[term1]) ;
      sum += weights[term1++] ;
      }
   while (term2 < other_terms)
      {
      absdiff += fabs(othervect->weights[term2]) ;
      sum += othervect->weights[term2++] ;
      }
   if (sum)
      return 1.0 - (absdiff / sum) ;
   else
      return 0.0 ;
}

//----------------------------------------------------------------------

double FrTermVector::canberraMeasure(const FrTermVector *othervect) const
{
   if (vector_length == 0.0 || othervect->vector_length == 0.0)
      return 0.0 ;
   size_t term1(0) ;
   size_t term2(0) ;
   FrSymbol *word1(terms[0]) ;
   FrSymbol *word2(othervect->terms[0]) ;
   double total(0.0) ;
   size_t count(0) ;
   for ( ; ; )
      {
      if (word1 <= word2)
	 {
	 if (word1 == word2)
	    {
	    double wt1 = weights[term1++] ;
	    double wt2 = othervect->weights[term2++] ;
	    double diff = wt1 - wt2 ;
	    double sum = wt1 + wt2 ;
	    if (sum)
	       total += fabs(diff / sum) ;
	    if (term2 >= othervect->num_terms)
	       break ;
	    word2 = othervect->terms[term2] ;
	    }
	 else
	    {
	    total += 1.0 ;		// fabs(diff/sum) == 1.0
	    term1++ ;
	    }
	 count++ ;
	 if (term1 >= num_terms)		// advance first term list
	    break ;
	 word1 = terms[term1] ;
	 }
      else
	 {
	 total += 1.0 ;			// fabs(diff/sum) == 1.0
	 count++ ;
	 if (++term2 >= othervect->num_terms) // advance second term list
	    break ;			      //   and quit if at and
	 word2 = othervect->terms[term2] ;
	 }
      }
   if (term1 < num_terms)
      {
      total += num_terms - term1 ;   // fabs(diff/sum) == 1.0, for each term
      count += num_terms - term1 ;
      }
   if (term2 < othervect->num_terms)
      {
      total += othervect->num_terms-term2 ; //fabs(diff/sum)==1.0,for each term
      count += othervect->num_terms - term2 ;
      }
   return total / count ;
}

//----------------------------------------------------------------------

double FrTermVector::czekanowskiMeasure(const FrTermVector *othervect) const
{
   if (vector_length == 0.0 || othervect->vector_length == 0.0)
      return 0.0 ;
   size_t term1(0) ;
   size_t term2(0) ;
   FrSymbol *word1(terms[0]) ;
   FrSymbol *word2(othervect->terms[0]) ;
   size_t other_terms(othervect->num_terms) ;
   double sum_of_min(0.0), sum(0.0) ;
   for ( ; ; )
      {
      if (word1 <= word2)
	 {
	 double wt1 = weights[term1++] ;
	 sum += wt1 ;
	 if (word1 == word2)
	    {
	    double wt2 = othervect->weights[term2++] ;
	    sum += wt2 ;
	    sum_of_min += minimum(wt1,wt2) ;
	    if (term2 >= other_terms)
	       break ;
	    word2 = othervect->terms[term2] ;
	    }
	 if (term1 >= num_terms)		// advance first term list
	    break ;
	 word1 = terms[term1] ;
	 }
      else
	 {
	 sum += othervect->weights[term2++] ;
	 if (term2 >= other_terms)	// advance second term list
	    break ;			//   and quit if at and
	 word2 = othervect->terms[term2] ;
	 }
      }
   while (term1 < num_terms)
      sum += weights[term1++] ;
   while (term2 < other_terms)
      sum += othervect->weights[term2++] ;
   if (sum)
      return 2.0 * sum_of_min / sum ;
   else
      return 0.0 ;
}

//----------------------------------------------------------------------

double FrTermVector::robinsonCoefficient(const FrTermVector *othervect) const
{
   if (vector_length == 0.0 || othervect->vector_length == 0.0)
      return 0.0 ;
   double totalwt1(totalTermWeights()) ;
   double totalwt2(othervect->totalTermWeights()) ;
   if (totalwt1 == 0 || totalwt2 == 0)
      return 0.0 ;
   size_t term1(0) ;
   size_t term2(0) ;
   FrSymbol *word1(terms[0]) ;
   FrSymbol *word2(othervect->terms[0]) ;
   size_t other_terms(othervect->num_terms) ;
   double sum(0.0) ;
   for ( ; ; )
      {
      if (word1 <= word2)
	 {
	 double wt1 = weights[term1++] / totalwt1 ;
	 if (word1 == word2)
	    {
	    double wt2 = othervect->weights[term2++] / totalwt2 ;
	    sum += fabs(wt1 - wt2) ;
	    if (term2 >= other_terms)
	       break ;
	    word2 = othervect->terms[term2] ;
	    }
	 else
	    sum += fabs(wt1) ;
	 if (term1 >= num_terms)		// advance first term list
	    break ;
	 word1 = terms[term1] ;
	 }
      else
	 {
	 sum += fabs(othervect->weights[term2++] / totalwt2) ;
	 if (term2 >= other_terms)	// advance second term list
	    break ;			//   and quit if at and
	 word2 = othervect->terms[term2] ;
	 }
      }
   while (term1 < num_terms)
      sum += fabs(weights[term1++]) / totalwt1 ;
   while (term2 < other_terms)
      sum += fabs(othervect->weights[term2++]) / totalwt2 ;
   return 2.0 - sum ;
}

//----------------------------------------------------------------------

double FrTermVector::drennanDissimilarity(const FrTermVector *othervect)
   const
{
   if (vector_length == 0.0 || othervect->vector_length == 0.0)
      return 1.0 ;
   double totalwt1(totalTermWeights()) ;
   double totalwt2(othervect->totalTermWeights()) ;
   if (totalwt1 == 0 || totalwt2 == 0)
      return 0.0 ;
   size_t term1(0) ;
   size_t term2(0) ;
   FrSymbol *word1(terms[0]) ;
   FrSymbol *word2(othervect->terms[0]) ;
   size_t other_terms(othervect->num_terms) ;
   double sum(0.0) ;
   for ( ; ; )
      {
      if (word1 <= word2)
	 {
	 double wt1 = weights[term1++] / totalwt1 ;
	 if (word1 == word2)
	    {
	    double wt2 = othervect->weights[term2++] / totalwt2 ;
	    sum += fabs(wt1 - wt2) ;
	    if (term2 >= other_terms)
	       break ;
	    word2 = othervect->terms[term2] ;
	    }
	 else
	    sum += wt1 ;
	 if (term1 >= num_terms)		// advance first term list
	    break ;
	 word1 = terms[term1] ;
	 }
      else
	 {
	 double wt2 = othervect->weights[term2++] / totalwt2 ;
	 sum += (-wt2) ;
	 if (term2 >= other_terms)	// advance second term list
	    break ;			//   and quit if at and
	 word2 = othervect->terms[term2] ;
	 }
      }
#if 0
   while (term1 < num_terms)
      sum += (weights[term1++] / totalwt1) ;
   while (term2 < other_terms)
      sum += (-othervect->weights[term2++] / totalwt2) ;
#endif /* 0 */
   return sum / 2.0 ;
}

//----------------------------------------------------------------------

double FrTermVector::circleProduct(const FrTermVector *othervect,
				    size_t *total_terms) const
{
   if (vector_length == 0.0 || othervect->vector_length == 0.0)
      {
      if (total_terms)
	 *total_terms = 0 ;
      return 0.0 ;
      }
   size_t term1(0) ;
   size_t term2(0) ;
   FrSymbol *word1(terms[0]) ;
   FrSymbol *word2(othervect->terms[0]) ;
   size_t other_terms(othervect->num_terms) ;
   double sum(0.0) ;
   size_t count(0) ;
   for ( ; ; )
      {
      if (word1 <= word2)
	 {
	 count++ ;
	 if (word1 == word2)
	    {
	    double wt1 = weights[term1] ;
	    double wt2 = othervect->weights[term2++] ;
	    sum += minimum(wt1,wt2) ;
	    if (term2 >= other_terms)
	       {
	       term1++ ;		// we've also consumed first term
	       break ;
	       }
	    word2 = othervect->terms[term2] ;
	    }
	 if (++term1 >= num_terms)	// advance first term list
	    {
	    count += other_terms - term2 ;
	    break ;
	    }
	 word1 = terms[term1] ;
	 }
      else
	 {
	 count++ ;
	 if (++term2 >= other_terms)	// advance second term list
	    break ;			//   and quit if at and
	 word2 = othervect->terms[term2] ;
	 }
      }
   count += num_terms - term1 ;
   if (total_terms)
      *total_terms = count ;
   return sum ;
}

//----------------------------------------------------------------------

double FrTermVector::similarityRatio(const FrTermVector *othervect) const
{
   if (!othervect || othervect->num_terms == 0)
      return num_terms == 0 ? 1.0 : 0.0 ; // no overlap, but match if also null
   size_t term1(0) ;
   size_t term2(0) ;
   FrSymbol *word1(terms[0]) ;
   FrSymbol *word2(othervect->terms[0]) ;
   size_t other_terms(othervect->num_terms) ;
   double prod(0.0), sum(0.0) ;
   for ( ; ; )
      {
      if (word1 <= word2)
	 {
	 double wt1 = weights[term1++] ;
	 sum += (wt1*wt1) ;
	 if (word1 == word2)
	    {
	    // we have a word in common!
	    double wt2 = othervect->weights[term2++] ;
	    sum += (wt2 * wt2) ;
	    prod += (wt1 * wt2) ;
	    if (term2 >= other_terms)	// advance second term list
	       break ;
	    word2 = othervect->terms[term2] ;
	    }
	 if (term1 >= num_terms)	// advance first term list
	    {
	    while (term2 < other_terms)
	       {
	       double wt = othervect->weights[term2++] ;
	       sum += (wt*wt) ;
	       }
	    break ;
	    }
	 word1 = terms[term1] ;
	 }
      else
	 {
	 double wt2 = othervect->weights[term2++] ;
	 sum += (wt2 * wt2) ;
	 if (term2 >= other_terms)	// advance second term list
	    break ;
	 word2 = othervect->terms[term2] ;
	 }
      }
   while (term1 < num_terms)
      {
      double wt = weights[term1++] ;
      sum += (wt*wt) ;
      }
   if (sum != prod)
      return prod / (sum - prod) ;
   else
      return DBL_MAX ;
}

//----------------------------------------------------------------------
//  the Jensen-Shannon divergence is an averaged Kullback-Liebler divergence:
//      JS(a,b) == (KL(a|avg(a,b)) + KL(b|avg(a,b))) / 2
//      KL(a|b) == sum_y( a(y)(log a(y) - log b(y)) )

//static double log_2 = 0.0 ;
static double log_2 = ::log(2.0) ;

static double KL_term(double a, double b)
{
   if (a <= 0.0 || b <= -0.0)
      return 0.0 ;
//   if (log_2 == 0.0)
//      log_2 = ::log(2.0) ;
   return a * (::log(a)/log_2 - ::log(b)/log_2) ;
}

static double JS_term(double a, double b)
{
   double avg = (a + b) / 2.0 ;
   return KL_term(a,avg) + KL_term(b,avg) ;
}

double FrTermVector::jensenShannonDivergence(const FrTermVector *othervect)
const
{
   if (!othervect || othervect->num_terms == 0 || num_terms == 0)
      return 0.0 ;			// no overlap
   double totalwt1(totalTermWeights()) ;
   double totalwt2(othervect->totalTermWeights()) ;
   size_t term1(0) ;
   size_t term2(0) ;
   FrSymbol *word1(terms[0]) ;
   FrSymbol *word2(othervect->terms[0]) ;
   size_t other_terms(othervect->num_terms) ;
   double sum(0.0) ;
   for ( ; ; )
      {
      if (word1 <= word2)
	 {
	 double wt1 = weights[term1++] / totalwt1 ;
	 if (word1 == word2)
	    {
	    // we have a word in common!
	    double wt2 = othervect->weights[term2++] / totalwt2 ;
	    sum += JS_term(wt1,wt2) ;
	    if (term2 >= other_terms)	// advance second term list
	       break ;
	    word2 = othervect->terms[term2] ;
	    }
	 else
	    sum += JS_term(wt1,0) ;
	 if (term1 >= num_terms)	// advance first term list
	    {
	    while (term2 < other_terms)
	       sum += JS_term(0,othervect->weights[term2++]/totalwt2) ;
	    break ;
	    }
	 word1 = terms[term1] ;
	 }
      else
	 {
	 double wt2 = othervect->weights[term2++] / totalwt2 ;
	 sum += JS_term(0,wt2) ;
	 if (term2 >= other_terms)	// advance second term list
	    break ;
	 word2 = othervect->terms[term2] ;
	 }
      }
   while (term1 < num_terms)
      sum += JS_term(weights[term1++]/totalwt1,0) ;
   return sum / 2.0 ;
}

//----------------------------------------------------------------------

static void contingency_table(const FrTermVector *tv1,
			      const FrTermVector *tv2,
			      double &A, double &B, double &C)
{
   double a(0.0) ;
   double b(0.0) ;
   double c(0.0) ;
   double maxwt1(maximum(1.0,tv1->maxTermWeight())) ;
   double maxwt2(maximum(1.0,tv2->maxTermWeight())) ;
   size_t term1(0) ;
   size_t term2(0) ;
   FrSymbol *word1(tv1->getTerm(0)) ;
   FrSymbol *word2(tv2->getTerm(0)) ;
   size_t num_terms(tv1->numTerms()) ;
   size_t other_terms(tv2->numTerms()) ;
   for ( ; ; )
      {
      if (word1 <= word2)
	 {
	 double wt1 = tv1->termWeight(term1++) / maxwt1 ;
	 if (word1 == word2)
	    {
	    double wt2 = tv2->termWeight(term2++) / maxwt2 ;
	    a += minimum(wt1,wt2) ;	// we have a term in common!
	    if (term2 >= other_terms)	// advance second term list
	       break ;
	    word2 = tv2->getTerm(term2) ;
	    }
	 else
	    b += wt1 ;
	 if (term1 >= num_terms)	// advance first term list
	    break ;
	 word1 = tv1->getTerm(term1) ;
	 }
      else
	 {
	 c += (tv2->termWeight(term2++) / maxwt2) ;
	 if (term2 >= other_terms)	// advance second term list
	    break ;
	 word2 = tv2->getTerm(term2) ;
	 }
      }
   while (term1 < num_terms)
      b += (tv1->termWeight(term1++) / maxwt1) ;
   while (term2 < other_terms)
      c += (tv2->termWeight(term2++) / maxwt2) ;
   A = a ;
   B = b ;
   C = c ;
   return ;
}

//----------------------------------------------------------------------

double FrTermVector::mountfordCoefficient(const FrTermVector *othervect)
const
{
   if (!othervect || othervect->num_terms == 0)
      return num_terms == 0 ? 1.0 : 0.0 ; // no overlap, but match if also null
   double a ;
   double b ;
   double c ;
   contingency_table(this,othervect,a,b,c) ;
   double divisor = (2.0 * b * c) + (a * b) + (a * c) ;
   if (divisor <= 0.0)
      return (a > 0.0) ? 1.0 : 0.0 ;
   return (2.0 * a / divisor) ;
}

//----------------------------------------------------------------------

double FrTermVector::fagerMcGowanCoefficient(const FrTermVector *othervect)
const
{
   if (!othervect || othervect->num_terms == 0)
      return num_terms == 0 ? 1.0 : 0.0 ; // no overlap, but match if also null
   double a ;
   double b ;
   double c ;
   contingency_table(this,othervect,a,b,c) ;
   double divisor = sqrt((a+b) * (a+c)) ;
   double main_term ;
   if (divisor == 0.0)
      main_term = (a > 0.0) ? 1.0 : 0.0 ;
   else
      main_term = a / divisor ;
   return main_term - (maximum(b,c) / 2.0) ;
}

//----------------------------------------------------------------------

double FrTermVector::tripartiteSimilarityIndex(const FrTermVector *other)
const
{
   if (!other || other->num_terms == 0)
      return num_terms == 0 ? 1.0 : 0.0 ; // no overlap, but match if also null
   double a ;
   double b ;
   double c ;
   contingency_table(this,other,a,b,c) ;
   // avoid division by zero by tweaking b and c if necessary
   if (a == 0)
      {
      if (b == 0.0) b = 0.001 ;
      if (c == 0.0) c = 0.001 ;
      }
   double U = ::log(1.0 + (a+minimum(b,c)) / (a+maximum(b,c))) / log_2 ;
   double S = 1.0 / sqrt(log(2.0 + minimum(b,c)/(a+1.0)) / log_2) ;
   double R_1 = log(1.0 + a / (a+b)) / log_2 ;
   double R_2 = log(1.0 + a / (a+c)) / log_2 ;
   double R = R_1 * R_2 ;
   return sqrt(U * S * R) ;
}

//----------------------------------------------------------------------

double FrTermVector::maxTermWeight() const
{
   double maxweight = 0.0 ;
   for (size_t i = 0 ; i < num_terms ; i++)
      {
      if (weights[i] > maxweight)
	 maxweight = weights[i] ;	// not a probabil., so need to adjust
      }
   return maxweight ;
}

//----------------------------------------------------------------------

double FrTermVector::totalTermWeights() const
{
   double totalweight = 0.0 ;
   for (size_t i = 0 ; i < num_terms ; i++)
      totalweight += weights[i] ;
   return totalweight ;
}


//----------------------------------------------------------------------

void FrTermVector::cacheNeighbor(FrTermVector *n, double sim)
{
   if (caching)
      {
      FrBoundedPriQueue *q = (FrBoundedPriQueue*)nearest_neighbor ;
      q->push(n,sim) ;
      }
   return ;
}

//----------------------------------------------------------------------

void FrTermVector::removeCachedNeighbor(FrTermVector *n)
{
   if (caching)
      {
      FrBoundedPriQueue *q = (FrBoundedPriQueue*)nearest_neighbor ;
      q->remove(n) ;
      }
   return ;
}

//----------------------------------------------------------------------

FrTermVector *FrTermVector::centroid(const FrList *vectors)
{
   FrTermVector *centroid = new FrTermVector ;
   for ( ; vectors ; vectors = vectors->rest())
      {
      FrTermVector *vector = (FrTermVector*)vectors->first() ;
      if (!vector /* || !vector->termvectorp()*/ )
	 continue ;
      centroid->mergeIn(vector) ;
      }
   return centroid ;
}

//----------------------------------------------------------------------

FrTermVector *FrTermVector::nearestNeighborCached() const
{
   assertq(caching != false) ;
   const FrBoundedPriQueue *n = (FrBoundedPriQueue*)nearest_neighbor ;
   return n ? (FrTermVector*)n->first() : 0 ;
}

//----------------------------------------------------------------------

FrBoundedPriQueue *FrTermVector::neighborCache() const
{
   return caching ? (FrBoundedPriQueue*)nearest_neighbor : 0 ;
}

/************************************************************************/
/*	Procedural Interface						*/
/************************************************************************/

void FrTermVecTotalPop(size_t total)
{
   total_population = total ;
   return ;
}

//----------------------------------------------------------------------

double FrTermVecSimilarity(const FrTermVector *tv1,
			   const FrTermVector *tv2,
			   FrClusteringMeasure sim_measure,
			   FrTermVectorSimilarityFunc *sim,
			   void *sim_data)
{
   switch (sim_measure)
      {
      case FrCM_COSINE:
	 return tv1->cosine(tv2) ;
      case FrCM_EUCLIDEAN:
         {
	 double total_len = (tv1->vectorLength() + tv2->vectorLength()) ;
	 if (total_len > 0.0)
	    return 1.0 - (tv1->euclideanDistance(tv2) / total_len) ;
	 else
	    return 1.0 ;
	 }
      case FrCM_MANHATTAN:
	 {
	 double total_dist = (tv1->manhattanDistance(0) +
			      tv2->manhattanDistance(0)) ;
	 if (total_dist > 0.0)
	    return 1.0 - (tv1->manhattanDistance(tv2) / total_dist) ;
	 else
	    return 1.0 ;
	 }
      case FrCM_JACCARD:
	 return tv1->jaccardCoefficient(tv2) ;
      case FrCM_DICE:
	 return tv1->diceCoefficient(tv2) ;
      case FrCM_ANTIDICE:
	 return tv1->antiDiceCoefficient(tv2) ;
      case FrCM_TANIMOTO:
	 return tv1->tanimotoCoefficient(tv2) ;
      case FrCM_SIMPSON:
	 return tv1->simpsonCoefficient(tv2) ;
      case FrCM_EXTSIMPSON:
	 return tv1->extSimpsonCoefficient(tv2) ;
      case FrCM_KULCZYNSKI1:
	 return tv1->kulczynskiMeasure1(tv2) ;
      case FrCM_KULCZYNSKI2:
	 return tv1->kulczynskiMeasure2(tv2) ;
      case FrCM_OCHIAI:
	 return tv1->ochiaiMeasure(tv2) ;
      case FrCM_SOKALSNEATH:
	 return tv1->sokalSneathMeasure(tv2) ;
      case FrCM_MCCONNAUGHEY:
	 // mcConnaugheyMeasure() return is in range -1.0...+1.0
	 return (tv1->mcConnaugheyMeasure(tv2) + 1.0) / 2.0 ;
      case FrCM_LANCEWILLIAMS:
	 return 1.0 - tv1->lanceWilliamsDist(tv2) ;
      case FrCM_BRAYCURTIS:
	 return tv1->brayCurtisMeasure(tv2) ;
      case FrCM_CANBERRA:
	 return 1.0 - tv1->canberraMeasure(tv2) ;
      case FrCM_CIRCLEPROD:
	 {
	 size_t total_terms ;
	 double cp = tv1->circleProduct(tv2,&total_terms) ;
	 return cp / total_terms ;
	 }
      case FrCM_CZEKANOWSKI:
	 return tv1->czekanowskiMeasure(tv2) ;
      case FrCM_ROBINSON:
	 return tv1->robinsonCoefficient(tv2) / 2.0 ;
      case FrCM_DRENNAN:
	 return 1.0 - tv1->drennanDissimilarity(tv2) ;
      case FrCM_SIMILARITYRATIO:
	 return tv1->similarityRatio(tv2) ;
      case FrCM_JENSENSHANNON:
	 return 1.0 - tv1->jensenShannonDivergence(tv2) ;
      case FrCM_MOUNTFORD:
	 return tv1->mountfordCoefficient(tv2) ;
      case FrCM_FAGER_MCGOWAN:
	 return tv1->fagerMcGowanCoefficient(tv2) ;
      case FrCM_BRAUN_BLANQUET:
	 return tv1->braunBlanquetCoefficient(tv2) ;
      case FrCM_TRIPARTITE:
	 return tv1->tripartiteSimilarityIndex(tv2) ;
      case FrCM_BIN_DICE:
	 return tv1->binaryDiceCoefficient(tv2) ;
      case FrCM_BIN_ANTIDICE:
	 return tv1->binaryAntiDiceCoefficient(tv2) ;
      case FrCM_BIN_GAMMA:
	 return tv1->binaryGammaCoefficient(tv2) ;
      case FrCM_NONE:
	 return 0.0 ;
      case FrCM_USER:
	 if (sim)
	    return sim(tv1,tv2,sim_data) ;
	 else
	    FrProgError("called FrTermVecSimilarity(FrCM_USER)") ;
#ifndef NDEBUG
      // as long as we cover all the measures, we don't need to use a default
      //  case, because the FrClusteringParameters ctor ensures that only valid
      //  enum values can get here
      default:
	 FrMissedCase("FrTermVecSimilarity()") ;
#endif /* !NDEBUG */
      }
   return 0.0 ;
}

//----------------------------------------------------------------------

double FrTermVecSimilarity(const FrTermVector *tv1,
			   const FrTermVector *tv2,
			   FrClusteringMeasure sim_measure, bool normalize,
			   FrTermVectorSimilarityFunc *sim_fn,
			   void *sim_data)
{
   // if we are asked to normalize vectors first and it's not a metric that
   //   self-normalizes, make copies of the vectors, normalize them to unit
   //   length, and then compute the similarity on the normalized vectors
   if (normalize && sim_measure != FrCM_COSINE &&
       sim_measure != FrCM_ROBINSON && sim_measure != FrCM_DRENNAN &&
       sim_measure != FrCM_SIMPSON && sim_measure != FrCM_MOUNTFORD &&
       sim_measure != FrCM_FAGER_MCGOWAN && sim_measure != FrCM_TRIPARTITE)
      {
      if (tv1 && tv2)
	 {
	 FrTermVector *vec1 = new FrTermVector(*tv1) ;
	 FrTermVector *vec2 = new FrTermVector(*tv2) ;
	 vec1->normalize() ;
	 vec2->normalize() ;
	 double sim = FrTermVecSimilarity(vec1,vec2,sim_measure,sim_fn,sim_data) ;
	 free_object(vec1) ;
	 free_object(vec2) ;
	 return sim ;
	 }
      else
	 return 0.0 ;
      }
   else
      return FrTermVecSimilarity(tv1,tv2,sim_measure,sim_fn,sim_data) ;
}

// end of file frtrmvec.cpp //
