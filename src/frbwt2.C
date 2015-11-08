/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC								*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frbwt2.cpp	    Burrows-Wheeler Transform n-gram index	*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 2003,2004,2006,2007,2008,2009,2011,2012,2015		*/
/*		 Ralf Brown/Carnegie Mellon University			*/
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

#include "frbwt.h"
#include "frsymbol.h"
#include "frstring.h"
#include "frutil.h"
#include "frvocab.h"

#ifdef FrSTRICT_CPLUSPLUS
#  include <cstdlib>
#  include <cmath>
#else
#  include <math.h>
#endif

/************************************************************************/
/*	Manifest Constants						*/
/************************************************************************/

#define DEFAULT_FREQ_ESTIMATE	0.01

/************************************************************************/
/************************************************************************/

// default value of the Stupid Backoff penalty as per Google's original
//    implementation
double FramepaC_StupidBackoff_alpha = 0.4 ;

/************************************************************************/
/************************************************************************/

//----------------------------------------------------------------------

#define FREQ(start,end) (freqtable[(start) * keylength + (end)])

static double overlapped_prob(const double *freqtable, size_t keylength,
			      size_t split, size_t &numfactors)
{
   // compute the joint prob of the prefix times the conditional prob
   //   of the suffix
   double denom = FREQ(split,keylength-2) ;
   double prob1 = 0.0 ;
   if (denom > 0.0)
      {
      double cond = FREQ(split,keylength-1) / denom ;
      prob1 = (FREQ(0,split) * cond) ;
      if (prob1 > 0.0)
	 numfactors++ ;
      }
   // compute the reverse conditional prob of the prefix times the
   //   joint prob of the suffix
   denom = FREQ(0,split) ;
   double prob2 = 0.0 ;
   if (denom > 0.0)
      {
      double cond = FREQ(1,split) / denom ;
      prob2 = (FREQ(split,keylength-1) * cond) ;
      if (prob2 > 0.0)
	 numfactors++ ;
      }
   return prob1 + prob2 ;
}

//----------------------------------------------------------------------

double FrBWTIndex::estimateFrequency(const uint32_t *searchkey,
				     size_t keylength,
				     size_t *numbackoffs) const
{
   if (keylength == 0)
      return 0.0 ;
   else if (keylength == 1)
      {
      size_t freq = unigramFrequency(searchkey[0]) ;
      return (freq > 0) ? (double)freq : discountMass() ;
      }
   FrLocalAlloc(double,freqtable,1024,keylength * keylength) ;
   if (!freqtable)
      {
      FrNoMemory("while estimating n-gram frequency") ;
      return discountMass() ;
      }
   // fill in the table of occurrence frequencies of all subphrases of the
   //   given search key
   size_t i ;
   for (i = 0 ; i < keylength ; i++)
      {
      FrBWTLocation range(unigram(searchkey[i])) ;
      double frq = range.rangeSize() ;
      if (frq == 0.0)
	 {
	 if (discountMass() > 0.0)
	    frq = discountMass() ;	// some smoothing for OOVs
	 else
	    frq = DEFAULT_FREQ_ESTIMATE ;
	 }
      else
	 frq /= massRatio() ;
      FREQ(i,i) = frq ;
      for (size_t j = i+1 ; j < keylength ; j++)
	 {
	 range = extendMatch(range, searchkey[j]) ;
	 FREQ(i,j) = range.rangeSize() ;
	 }
      if (i == 0 && FREQ(0,keylength-1) > 0)
	 {
	 // hooray, we have at least one instance of a complete match
	 frq = FREQ(0,keylength-1) ;
	 FrLocalFree(freqtable) ;
	 return frq ;
	 }
      }
   if (numbackoffs)
      (*numbackoffs)++ ;
   if (keylength == 2)
      {
      // bigram backoff is simply to assume independence
      double freq = FREQ(0,0) / numItems() * FREQ(1,1) ;
      if (freq > 1.0)			// if expected more than one occur,
	 freq = sqrt(freq) ;		//   lack says words anti-correlated
      FrLocalFree(freqtable) ;
      return freq ;
      }
#if 0
   // emulate the computations in tgeval.cpp from LM
   if (keylength == 3)
      {
      // first level of trigram backoff is to try for either
      //    P(w1,w2) * P(w3|w2)     or   P(w2,w3) * P(w2|w1)
      double bi_1 = FREQ(0,1) ;
      double bi_2 = FREQ(1,2) ;
      if (bi_1 > 0.0)
	 {
	 bi_1 /= massRatio() ;
	 if (bi_2 > 0.0)
	    return bi_1 * (bi_2 / (double)FREQ(1,1)) ;
	 else
	    return bi_1 * (FREQ(2,2) / (double)numItems()) / 2.0 ;
	 }
      else if (bi_2 > 0.0)
	 return FREQ(0,0) / numItems() * bi_2 / massRatio() / 2.0 ;
      else
	 return (FREQ(0,0) / numItems() * FREQ(1,1) / numItems() *
		 FREQ(2,2) / 6.0) ;
      }
#endif
   // no complete match, so try to piece together the full key in as many
   //   ways as possible, and take the average of the estimates thus produced
   double estimate = 0.0 ;
   size_t numfactors = 0 ;
   // find the longest prefix match and longest suffix match
   for (i = keylength - 1 ; i > 0 ; i--)
      if (FREQ(0,i) > 0)
	 break ;
   size_t j ;
   for (j = 1 ; j < keylength ; j++)
      if (FREQ(j,keylength-1) > 0)
	 break ;
   if (i >= j && i > 0 && j < keylength )
      {
      // we have an overlap between prefix and suffix, so compute frequency
      //   estimates for all one-word overlaps
      for (size_t k = i ; k <= j ; k++)
	 {
	 estimate += overlapped_prob(freqtable,keylength,k,numfactors) ;
	 }
      }
   else
      {
      // further backoff -- estimate the prefix needed to match up against
      //    the longest suffix and suffix for longest prefix
      if (i > 0)
	 {
	 FREQ(i,keylength-1) = estimateFrequency(searchkey+i,keylength-i,
						 numbackoffs) ;
	 }
      if (j + 1 < keylength)
	 {
	 FREQ(0,j) = estimateFrequency(searchkey,j+1,numbackoffs) ;
	 }
      estimate += overlapped_prob(freqtable,keylength,i,numfactors) ;
      estimate += overlapped_prob(freqtable,keylength,j,numfactors) ;
      numfactors *= 2 ;			// extra penalty for backing off
      }
//!!!
   if (numfactors == 0)
      estimate = discountMass() ;
   else
      {
      estimate /= numfactors ;
      double expected = FREQ(0,0) / massRatio() ;
      for (i = 1 ; i < keylength ; i++)
	 expected *= (FREQ(i,i) / totalMass()) ;
      if (expected > 1.0)		// if expected >1 occur, lack says
	 estimate = sqrt(estimate) ;	//  words are anti-correlated
      }
   FrLocalFree(freqtable) ;
   return estimate ;
}

//----------------------------------------------------------------------

double FrBWTIndex::frequency(const uint32_t *searchkey,size_t keylength,
			     bool allow_backoff, size_t *numbackoffs) const
{
   if (allow_backoff)
      return estimateFrequency(searchkey,keylength,numbackoffs) ;
   else
      return frequency(searchkey,keylength) ;
}

//----------------------------------------------------------------------

double FrBWTIndex::frequency(FrNGramHistory *history,size_t histlen,
			     uint32_t next_ID, bool allow_backoff,
			     size_t *numbackoffs) const
{
#if NEW
   if (allow_backoff)
      return estimateFrequency(searchkey,keylength,numbackoffs) ;
   else
#else
   (void)allow_backoff; (void)numbackoffs ;
#endif /* NEW */
      return frequency(history,histlen,next_ID) ;
}

//----------------------------------------------------------------------

void FrBWTIndex::initClassSizes(const FrVocabulary *vocab)
{
   if (vocab)
      {
      FrFree(m_class_sizes) ;
      m_class_sizes = 0 ;
      m_num_classes = 0 ;
      size_t last_id = 0 ;
      for (size_t i = 0 ; i < vocab->numWords() ; i++)
	 {
	 const char *ent = vocab->indexEntry(i) ;
	 if (ent)
	    {
	    size_t id = vocab->getID(ent) ;
	    if (id > last_id)
	       last_id = id ;
	    }
	 }
      m_class_sizes = FrNewC(size_t,last_id+1) ;
      if (m_class_sizes)
	 {
	 m_num_classes = last_id + 1 ;
	 for (size_t i = 0 ; i < vocab->numWords() ; i++)
	    {
	    const char *ent = vocab->indexEntry(i) ;
	    if (ent)
	       {
	       size_t id = vocab->getID(ent) ;
	       m_class_sizes[id]++ ;
	       }
	    }
	 }
      else
	 FrWarning("unable to allocate memory for class-size information; language-model statistics will be degraded") ;
      }
   return ;
}

//----------------------------------------------------------------------

double FrBWTIndex::rawCondProbability(const uint32_t *searchkey,
				      size_t keylength) const
{
   if (keylength == 0)
      return 0.0 ;
   size_t class_size = classSize(searchkey[keylength-1]) ;
   if (wordsAreReversed() || keylength == 1)
      {
      // we can take advantage of efficient left-extensions since words are
      //   stored in the file right-to-left
      FrBWTLocation loc = unigram(searchkey[0]) ;
      if (keylength == 1)
	 {
	 double prob = (loc.nonEmpty()
			? loc.rangeSize() / totalMass()
			: discountMass()) ;
	 return prob / class_size ;
	 }
      for (size_t i = 1 ; i < keylength-1 && loc.nonEmpty() ; i++)
	 {
	 loc = extendMatch(loc,searchkey[i]) ;
	 }
      double count = loc.rangeSize() ;
      if (count)
	 {
	 loc = extendMatch(loc,searchkey[keylength-1]) ;
	 return loc.rangeSize() / (double)(count * class_size) ;
	 }
      }
   else
      {
      // since words are stored in the file left-to-right and there is no
      //   efficient way to extend an n-gram match to the right, we have to
      //   look up both the history and history+predictee
      size_t hist_count = revFrequency(searchkey,keylength-1) ;
      if (hist_count)
	 return (revFrequency(searchkey,keylength) /
		 (double)(hist_count * class_size)) ;
      }
   return 0.0 ;
}

//----------------------------------------------------------------------

void FrBWTIndex::initZerogram(FrNGramHistory *history) const
{
   if (wordsAreReversed())
      {
      // store start and length of matching range
      history->setIndex(0) ;
      history->setCount(numItems()) ;
      }
   else
      {
      // store wordID and history frequency count
      history->setIndex(FrVOCAB_WORD_NOT_FOUND) ;
      history->setCount(numItems()) ;
      }
//   history->setSmoothing(DEFAULT_SMOOTHING) ;
   return ;
}

//----------------------------------------------------------------------

void FrBWTIndex::advanceHistory(FrNGramHistory *history,
				size_t histlen) const
{
   // if we are storing just the wordIDs, shift them all up by one position
   for (size_t i = histlen ; i > 0 ; i--)
      history[i+1] = history[i] ;
   return ;
}

//----------------------------------------------------------------------

double FrBWTIndex::rawCondProbability(FrNGramHistory *history, size_t histlen,
				      uint32_t next_ID) const
{
   if (histlen == 0)
      return 0.0 ;
   size_t class_size = classSize(next_ID) ;
   if (histlen == 1)
      {
      FrBWTLocation loc = unigram(next_ID) ;
      double prob = (loc.nonEmpty()
		     ? loc.rangeSize() / totalMass()
		     : discountMass()) ;
      if (wordsAreReversed())
	 {
	 history[1].setLoc(loc) ;
	 }
      else
	 {
	 history[1].setID(next_ID) ;
	 history[1].setCount(loc.rangeSize()) ;
	 }
      return prob / class_size ;
      }
   else if (wordsAreReversed())
      {
      // we can take advantage of efficient left-extensions since words are
      //   stored in the file right-to-left
      FrBWTLocation loc(history[histlen-1].startLoc(),
			history[histlen-1].endLoc()) ;
      if (loc.nonEmpty())
	 {
	 double count = loc.rangeSize() ;
	 loc = extendMatch(loc,next_ID) ;
	 size_t pred_count = loc.rangeSize() ;
	 history[histlen].setLoc(loc) ;
	 return pred_count / (double)(count * class_size) ;
	 }
      else
	 {
	 history[histlen].setLoc(loc) ;
	 }
      }
   else
      {
      // since words are stored in the file left-to-right, and there is no
      //   efficient way to extend an n-gram match to the right, we have to
      //   store the word IDs and do a complete lookup of history+predictee
      //   each time
      size_t hist_count = history[histlen-1].count() ;
      if (hist_count)
	 {
	 FrBWTLocation loc(unigram(next_ID)) ;
	 for (size_t i = 1 ; i < histlen ; i++)
	    {
	    loc = extendMatch(loc,history[i].wordID()) ;
	    }
	 size_t pred_count = loc.rangeSize() ;
	 history[histlen].setID(history[histlen-1].wordID()) ;
	 history[histlen].setCount(pred_count) ;
	 return (pred_count / (double)(hist_count * class_size)) ;
	 }
      else
	 {
	 history[histlen].setID(history[histlen-1].wordID()) ;
	 history[histlen].setCount(0) ;
	 }
      }
   return 0.0 ;
}

//----------------------------------------------------------------------

double FrBWTIndex::condProbabilityKN(const uint32_t *searchkey,
				     size_t keylength, size_t *max_exist) const
{
   //FIXME: for now, we use Cubic-Weighted-Mean smoothing if KN is
   //   requested since we don't have any precomputed discounts and computing
   //   them on the fly is computationally prohibitive, especially for uni-
   //   and bigrams
   return condProbability(searchkey,keylength,FrLMSmooth_CubeMean,0,max_exist);
}

//----------------------------------------------------------------------

static double mean_smoothing_weight(FrLMSmoothing smoothing, size_t len)
{
   switch (smoothing)
      {
      case FrLMSmooth_CubeMean: return (len * len * len) ;
      case FrLMSmooth_QuadMean: return (len * len) ;
      case FrLMSmooth_WtMean:	return len ;
      case FrLMSmooth_ExpMean:  return ::pow(2.0,len) ;
      default:			return 1.0 ;
      }
}

//----------------------------------------------------------------------

double FrBWTIndex::condProbability(const uint32_t *searchkey, size_t keylength,
				   FrLMSmoothing smoothing,
				   size_t *numbackoffs,
				   size_t *max_exist) const
{
   if (keylength == 0)
      return (smoothing != FrLMSmooth_None) ? discountMass() : 0.0 ;
   switch (smoothing)
      {
      case FrLMSmooth_None:
      case FrLMSmooth_Backoff:
      case FrLMSmooth_StupidBackoff:
         {
	 double rawprob = rawCondProbability(searchkey,keylength) ;
	 if (max_exist && rawprob > 0.0 && keylength > *max_exist)
	    *max_exist = keylength ;
	 if (rawprob)
	    return rawprob / massRatio() ;
	 else if (smoothing == FrLMSmooth_StupidBackoff)
	    {
	    if (numbackoffs) (*numbackoffs)++ ;
	    return (FramepaC_StupidBackoff_alpha *
		    condProbability(searchkey+1,keylength-1,smoothing,
				    numbackoffs,max_exist)) ;
	    }
	 else if (smoothing == FrLMSmooth_Backoff)
	    {
	    if (numbackoffs) (*numbackoffs)++ ;
	    return condProbability(searchkey+1,keylength-1,smoothing,
				   numbackoffs,max_exist) ;
	    }
	 else
	    return 0.0 ;
	 }
      case FrLMSmooth_Max:
         {
	 size_t maxrank = 0 ;
	 double prob = 0.0 ;
	 for (size_t i = 1 ; i <= keylength ; i++)
	    {
	    double p = rawCondProbability(searchkey+keylength-i,i) ;
	    if (p == 0)
	       break ;
	    if (p > prob)
	       prob = p ;
	    maxrank = i ;
	    }
	 if (max_exist)
	    *max_exist = maxrank ;
	 return prob ? prob / massRatio() : discountMass() ;
	 }
      case FrLMSmooth_Mean:
      case FrLMSmooth_WtMean:
      case FrLMSmooth_QuadMean:
      case FrLMSmooth_CubeMean:
      case FrLMSmooth_ExpMean:
         {
	 size_t maxrank = 0 ;
	 double total = 0.0 ;
	 double totalwt = 0 ;
	 for (size_t i = 1 ; i <= keylength ; i++)
	    {
	    double rawprob = rawCondProbability(searchkey+keylength-i,i) ;
	    double wt = mean_smoothing_weight(smoothing,i) ;
	    totalwt += wt ;
	    if (rawprob > 0.0)
	       total += (rawprob * wt) ;
	    else
	       {
	       maxrank = i - 1 ;
	       if (numbackoffs) *numbackoffs += (keylength - i) ;
	       for (i = i+1 ; i <= keylength ; i++)
		  totalwt += mean_smoothing_weight(smoothing,i) ;
	       break ;
	       }
	    }
	 if (max_exist)
	    *max_exist = maxrank ;
	 return total / totalwt ;
	 }
      case FrLMSmooth_SimpKN:
	 return condProbabilityKN(searchkey,keylength,max_exist) ;
      default:
	 FrMissedCase("FrBWTIndex::condProbability") ;
      }
   return discountMass() ;
}

//----------------------------------------------------------------------

double FrBWTIndex::condProbabilityKN(FrNGramHistory *history, size_t histlen,
				     uint32_t next_ID, size_t *max_exist) const
{
   //FIXME: for now, we use Cubic-Weighted-Mean smoothing if KN is
   //   requested since we don't have any precomputed discounts and computing
   //   them on the fly is computationally prohibitive, especially for uni-
   //   and bigrams
   return condProbability(history,histlen,next_ID,FrLMSmooth_CubeMean,0,
			  max_exist);
}

//----------------------------------------------------------------------

double FrBWTIndex::condProbabilityBackoff(FrNGramHistory *history,
					  size_t histlen,
					  uint32_t next_ID,
					  size_t *max_exist,
					  size_t *numbackoffs,
					  FrLMSmoothing smoothing) const
{
   // get the raw, unsmoothed conditional probability
   double prob = 0.0 ;
   double backoff_factor = 1.0 ;
   double backoff = 0.0 ;
   if (smoothing == FrLMSmooth_StupidBackoff)
      backoff = FramepaC_StupidBackoff_alpha ;
   else if (smoothing == FrLMSmooth_Backoff)
      backoff = 1.0 ;
   for (size_t rank = histlen ; rank > 0 ; rank--)
      {
      double rawprob = rawCondProbability(history,rank-1,next_ID) ;
      if (rawprob > 0.0)
	 {
	 if (max_exist && rank > *max_exist)
	    *max_exist = rank ;
	 if (prob == 0.0)
	    prob = rawprob ;
	 }
      else
	 {
	 backoff_factor *= backoff ;
	 if (numbackoffs) (*numbackoffs)++ ;
	 }
      }
   return prob * backoff_factor ;
}

//----------------------------------------------------------------------

double FrBWTIndex::condProbabilityMax(FrNGramHistory *history, size_t histlen,
				      uint32_t next_ID,
				      size_t *max_exist) const
{
   double prob = 0.0 ;
   size_t maxrank = 0 ;
   for (size_t i = histlen ; i > 0 ; i--)
      {
      double p = rawCondProbability(history,i,next_ID) ;
      if (p > prob)
	 prob = p ;
      if (p > 0 && !maxrank)
	 maxrank = i ;
      }
   if (max_exist && maxrank > *max_exist)
      *max_exist = maxrank ;
   return prob ? prob / massRatio() : discountMass() ;
}

//----------------------------------------------------------------------

double FrBWTIndex::condProbabilityMean(FrNGramHistory *history, size_t histlen,
				       uint32_t next_ID,
				       FrLMSmoothing smoothing,
				       size_t *numbackoffs,
				       size_t *max_exist) const
{
   double total = 0.0 ;
   double totalwt = 0 ;
   size_t maxrank = 0 ;
   for (size_t i = histlen ; i > 0 ; i--)
      {
      double rawprob = rawCondProbability(history,i,next_ID) ;
      double wt = mean_smoothing_weight(smoothing,i) ;
      totalwt += wt ;
      if (rawprob > 0.0)
	 {
	 total += (rawprob * wt) ;
	 if (i > maxrank)
	    maxrank = i ;
	 }
      else if (numbackoffs)
	 (*numbackoffs)++ ;
      }
   if (max_exist && maxrank > *max_exist)
      *max_exist = maxrank ;
   return total / totalwt ;
}

//----------------------------------------------------------------------

double FrBWTIndex::condProbability(FrNGramHistory *history, size_t histlen,
				   uint32_t next_ID, FrLMSmoothing smoothing,
				   size_t *numbackoffs,
				   size_t *max_exist) const
{
   if (histlen == 0)
      return (smoothing != FrLMSmooth_None) ? discountMass() : 0.0 ;
   switch (smoothing)
      {
      case FrLMSmooth_None:
      case FrLMSmooth_Backoff:
      case FrLMSmooth_StupidBackoff:
	 return condProbabilityBackoff(history,histlen,next_ID,max_exist,
				       numbackoffs,smoothing) ;
      case FrLMSmooth_Max:
	 return condProbabilityMax(history,histlen,next_ID,max_exist) ;
      case FrLMSmooth_Mean:
      case FrLMSmooth_WtMean:
      case FrLMSmooth_QuadMean:
      case FrLMSmooth_CubeMean:
      case FrLMSmooth_ExpMean:
	 return condProbabilityMean(history,histlen,next_ID,smoothing,
				    numbackoffs,max_exist) ;
      case FrLMSmooth_SimpKN:
	 return condProbabilityKN(history,histlen,next_ID,max_exist) ;
      default:
	 FrMissedCase("FrBWTIndex::condProbability") ;
      }
   return discountMass() ;
}

//----------------------------------------------------------------------

size_t FrBWTIndex::longestMatch(const uint32_t *searchkey, size_t keylength)
const
{
   if (keylength == 0 || !searchkey)
      return 0 ;
   size_t match = 0 ;
   if (wordsAreReversed())
      {
      FrBWTLocation loc(unigram(searchkey[keylength-1])) ;
      while (++match < keylength && loc.nonEmpty())
	 {
	 loc = extendMatch(loc,searchkey[keylength-match-1]) ;
	 }
      }
   else
      {
      FrBWTLocation loc(unigram(searchkey[0])) ;
      while (++match < keylength && loc.nonEmpty())
	 {
	 loc = extendMatch(loc,searchkey[match]) ;
	 }
      }
   return match ;
}

/************************************************************************/
/************************************************************************/

void FrParseLMSmoothingType(FrSymbol *type, FrLMSmoothing *smoothing)
{
   if (type && smoothing)
      {
      const char *type_name = FrPrintableName(type) ;
      if (Fr_strnicmp(type_name,"NONE",3) == 0)
	 *smoothing = FrLMSmooth_None ;
      else if (Fr_strnicmp(type_name,"BACKOFF",1) == 0)
	 *smoothing = FrLMSmooth_Backoff ;
      else if (Fr_strnicmp(type_name,"MAX",3) == 0)
	 *smoothing = FrLMSmooth_Max ;
      else if (Fr_strnicmp(type_name,"MEAN",4) == 0)
	 *smoothing = FrLMSmooth_Mean ;
      else if (Fr_strnicmp(type_name,"WTMEAN",3) == 0 ||
	       Fr_strnicmp(type_name,"WEIGHTEDMEAN",8) == 0)
	 *smoothing = FrLMSmooth_WtMean ;
      else if (Fr_strnicmp(type_name,"QUADMEAN",5) == 0 ||
	       Fr_strnicmp(type_name,"QUADWTMEAN",7) == 0 ||
	       Fr_strnicmp(type_name,"QUADWEIGHTEDMEAN",12) == 0)
	 *smoothing = FrLMSmooth_QuadMean ;
      else if (Fr_strnicmp(type_name,"CUBEMEAN",5) == 0 ||
	       Fr_strnicmp(type_name,"CUBEWTMEAN",7) == 0 ||
	       Fr_strnicmp(type_name,"CUBEWEIGHTEDMEAN",12) == 0)
	 *smoothing = FrLMSmooth_CubeMean ;
      else if (Fr_strnicmp(type_name,"EXPMEAN",4) == 0 ||
	       Fr_strnicmp(type_name,"EXPWTMEAN",6) == 0 ||
	       Fr_strnicmp(type_name,"EXPWEIGHTEDMEAN",11) == 0)
	 *smoothing = FrLMSmooth_ExpMean ;
      else if (Fr_strnicmp(type_name,"SIMPLE_KN",4) == 0)
	 *smoothing = FrLMSmooth_SimpKN ;
      else if (Fr_strnicmp(type_name,"STUPID_BACKOFF",3) == 0)
	 {
	 *smoothing = FrLMSmooth_StupidBackoff ;
	 const char *dash = strchr(type_name,'-') ;
	 if (dash)
	    {
	    dash++ ;
	    char *end = nullptr ;
	    double val = strtod(dash,&end) ;
	    if (end != dash && val >= 0.0 && val <= 1.0)
	       FramepaC_StupidBackoff_alpha = val ;
	    }
	 }
      else
	 cerr << "Unknown smoothing type (expected "
	         "NONE, BACKOFF, MAX, MEAN, WTMEAN, QUADMEAN, CUBEMEAN,\n"
	         "\tEXPMEAN or SIMPLE_KN)"
	      << endl ;
      }
   return ;
}

// end of file frbwt2.cpp //
