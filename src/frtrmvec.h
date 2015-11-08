/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC								*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File: frtrmvec.h	      term vectors				*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1997,1998,1999,2000,2001,2002,2003,2004,2005,2006,	*/
/*		2009,2012,2015 Ralf Brown/Carnegie Mellon University	*/
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

#ifndef __FRTRMVEC_H_INCLUDED
#define __FRTRMVEC_H_INCLUDED

#include <stdio.h>

#ifndef __FRSYMBOL_H_INCLUDED
#include "frsymbol.h"
#endif

#ifndef __FRHASH_H_INCLUDED
#include "frhash.h"
#endif

#ifndef __FRARRAY_H_INCLUDED
#include "frarray.h"
#endif

#if defined(__GNUC__)
#  pragma interface
#endif

/************************************************************************/
/*	Manifest Constants						*/
/************************************************************************/

/************************************************************************/
/*	Types								*/
/************************************************************************/

class FrTermVector ;

enum FrClusteringMeasure
   {
      FrCM_NONE,
      FrCM_USER,		// call user function to determine similarity
      FrCM_COSINE,		// cosine-similarity between representatives
      FrCM_MANHATTAN,		// sum(|w1_i-w2_i|)
      FrCM_EUCLIDEAN,		// Euclidean distance between representatives
      FrCM_DICE,		// Dice's coeff: 2 * prod-prob / sum-prob
      FrCM_ANTIDICE,		// |intersect|/(|intersect|+2*|mismatch|)
      FrCM_JACCARD,		// Jaccard's coeff: |intersection| div |union|
      FrCM_TANIMOTO,		// (aka Extended Jaccard)
      FrCM_SIMPSON,		// |intersection|/min(|vec1|,|vec2|)
      FrCM_EXTSIMPSON,		// weighted |intersection|/min(|vec1|,|vec2|)
      FrCM_KULCZYNSKI1,		// |match|/|mismatch|
      FrCM_KULCZYNSKI2,		// 0.5*(|intersct|/|vec1| + |intersct|/|vec2|)
      FrCM_OCHIAI,		// 0.5*|intersection|/sqrt(|vec1|*|vec2|)
      FrCM_SOKALSNEATH,		// |inters|/(|inters|+|mismatch1|+|mismatch2|)
      FrCM_MCCONNAUGHEY,	// (|match|^2-|mismtch1|*|mismtch2|)/(|v1|*|v2|)
      FrCM_LANCEWILLIAMS,	// (|mismatch1|+|mismatch2|)/(|vec1|+|vec2|)
      FrCM_BRAYCURTIS,
      FrCM_CANBERRA,
      FrCM_CIRCLEPROD,
      FrCM_CZEKANOWSKI,
      FrCM_ROBINSON,
      FrCM_DRENNAN,
      FrCM_SIMILARITYRATIO,
      FrCM_JENSENSHANNON,	// symmetrically averaged KL-divergence
      FrCM_MOUNTFORD,		// 2a/(2bc+ab+ac)
      FrCM_FAGER_MCGOWAN,	// a/sqrt((a+b)*(a+c)) - 0.5*max(b,c)
      FrCM_BRAUN_BLANQUET,	// |intersection|/max(|vec1|,|vec2|)
      FrCM_TRIPARTITE,		// Tulloss' Tripartite Similarity Index
      FrCM_BIN_DICE,		// binary version of Dice
      FrCM_BIN_ANTIDICE,	// binary version of AntiDice
      FrCM_BIN_GAMMA		// binary version of Goodman-Kruskal Gamma
   } ;

typedef double FrTermVectorWeightingFunc(FrSymbol *term, double orig_weight,
					 void *user_data) ;

typedef double FrTermVectorSimilarityFunc(const FrTermVector *,
					  const FrTermVector *, void *user_data = nullptr) ;

//----------------------------------------------------------------------

class FrTFIDFrecord
   {
   private:
      static FrAllocator allocator ;
      size_t term_freq ;
      size_t doc_freq ;
   public:
      void *operator new(size_t) { return allocator.allocate() ; }
      void operator delete(void *blk) { allocator.release(blk) ; }
      FrTFIDFrecord(size_t tf, size_t df) { term_freq = tf ; doc_freq = df ; }
      FrTFIDFrecord(const FrTFIDFrecord *rec)
          { term_freq = rec->term_freq ; doc_freq = rec->doc_freq ; }
      ~FrTFIDFrecord() {}

      // manipulators
      void incr(size_t tf) { if (tf) { term_freq += tf ; doc_freq++ ; } }
      void update(size_t tf, size_t df) { term_freq += tf ; doc_freq += df ; }

      // accessors
      size_t termFrequency() const { return term_freq ; }
      size_t docFrequency() const { return doc_freq ; }
      double invDocFrequency(size_t total_docs) const ;

      double TF_IDF(size_t total_docs) const ;
   } ;

//----------------------------------------------------------------------

typedef FrHashTable<const FrSymbol*,FrTFIDFrecord*> FrTFIDFHashTable ;

class FrTFIDF
   {
   private:
      FrTFIDFHashTable *ht ;
      size_t total_docs ;
      bool modified ;
   protected:
      FrTFIDFHashTable *hashTable() const { return ht ; }
      FrTFIDFrecord **tfidfRecordPtr(FrSymbol *term) const
	 { return ht->lookupValuePtr(term) ; }
      bool tfidfRecord(FrSymbol *term, FrTFIDFrecord **udata) const
	 { return ht->lookup(term,udata) ; }
      void setTotalDocs(size_t total) { total_docs = total ; }
      void markClean() { modified = false ; }
      void markDirty() { modified = true ; }
   public:
      FrTFIDF() ;
      FrTFIDF(const char *filename) ;
      FrTFIDF(const FrTFIDF *orig) ;
      ~FrTFIDF() ;

      // manipulators
      void newDoc() { total_docs++ ; }
      void incr(FrSymbol *term, size_t tf) ;

      // accessors
      size_t totalDocuments() const { return total_docs ; }
      size_t knownWords() const { return ht ? ht->currentSize() : 0 ; }
      bool knownWord(FrSymbol *term) const { return ht->contains(term) ; }
      bool dirty() const { return modified ; }
      double tf_idf(FrSymbol *term) const ;
      double invDocFreq(FrSymbol *term) const ;
      double defaultIDF() const ;

      // I/O
      bool load(const char *filename) ;
      bool save(FILE *fp, bool verbosely) ;
      bool save(const char *filename, bool verbosely) ;
   } ;

//----------------------------------------------------------------------

class FrBoundedPriQueue ;

class FrTermVector : public FrObject
   {
   private:
      static FrAllocator allocator ;
   protected:
      double vector_length ;
      double nearest_dist ;		// measure for closest neighbor
      FrTermVector *nearest_neighbor ;	// the closest neighbor to this termvec
      FrSymbol *nearest_key ;		// key term for closest neighbor
      FrSymbol **terms ;		// the actual terms in the vector
      double *weights ;			// and their corresponding weights
      const FrSymbol *keysym ;		// the identifier for this vector
      const FrSymbol *clustername ;	// the cluster to which it belongs
      void *user_data ;			// storage for random data
      void *clustering_data ;		// storage for clustering data
      size_t num_terms ;		// number of distinct terms in vector
      size_t vector_freq ;		// number of times vector occurs
      bool is_neighbor ;		// nearest neighbor to someone else?
      bool caching ;
      bool cluster_flag ;		// for use by clustering code
   protected:
      void init() ;
      void init(FrList *wordlist, size_t num_words,
		bool is_sorted = false);
      void termMatches(const FrTermVector *other, double &match1_to_2,
		       double &match2_to_1, double &total_1, double &total_2)
	 const ;
      void termMatches(const FrTermVector *other, size_t &matches,
		       size_t &total_1, size_t &total_2) const ;
   public:
      void *operator new(size_t) { return allocator.allocate() ; }
      void operator delete(void *blk) { allocator.release(blk) ; }
      FrTermVector() { init() ; }
      FrTermVector(const FrSparseArray *counts) ;
      FrTermVector(const FrSymCountHashTable *counts) ;
      FrTermVector(const FrList *words) ;  // FrSymbol or (FrSymbol FrNumber)
      FrTermVector(FrList *words_and_weights, size_t num_words,
		    bool already_sorted_by_FrSymbol = false)
         // note: frees the FrList!
	 { init(words_and_weights,num_words,already_sorted_by_FrSymbol) ; }
      FrTermVector(const FrTermVector &oldvect) ;
      virtual ~FrTermVector() ;

      // methods needed to support inheritance from FrObject
      virtual void freeObject() ;
      virtual const char *objTypeName() const ;
      virtual ostream &printValue(ostream &output) const ;
      virtual char *displayValue(char *buffer) const ;
      virtual size_t displayLength() const ;
      virtual FrObject *deepcopy() const ;

      // binary similarity and difference metrics
      double jaccardCoefficient(const FrTermVector *othervect) const ;
      double simpsonCoefficient(const FrTermVector *othervect) const ;
      double binaryDiceCoefficient(const FrTermVector *othervect) const ;
      double binaryAntiDiceCoefficient(const FrTermVector *othervect) const ;
      double binaryGammaCoefficient(const FrTermVector *othervect) const ;

      // continuous similarity and difference metrics
      double cosine(const FrTermVector *othervect) const ;
      double manhattanDistance(const FrTermVector *othervect) const ;
      double euclideanDistance(const FrTermVector *othervect) const ;
      double extSimpsonCoefficient(const FrTermVector *othervect) const ;
      double tanimotoCoefficient(const FrTermVector *othervect) const ;
            // -> aka Extended Jaccard Coefficient
      double diceCoefficient(const FrTermVector *othervect) const ;
      double antiDiceCoefficient(const FrTermVector *othervect) const ;
      double kulczynskiMeasure1(const FrTermVector *othervect) const ;
      double kulczynskiMeasure2(const FrTermVector *othervect) const ;
      double ochiaiMeasure(const FrTermVector *othervect) const ;
      double sokalSneathMeasure(const FrTermVector *othervect) const ;
      double mcConnaugheyMeasure(const FrTermVector *othervect) const ;
      double lanceWilliamsDist(const FrTermVector *othervect) const ;
      double brayCurtisMeasure(const FrTermVector *othervect) const ;
      double canberraMeasure(const FrTermVector *othervect) const ;
      double circleProduct(const FrTermVector *othervect,
			   size_t *total_terms = 0) const ;
	      // (assumes that all weights are >= 0.0)
      double czekanowskiMeasure(const FrTermVector *othervect) const ;
      double robinsonCoefficient(const FrTermVector *othervect) const ;
      double drennanDissimilarity(const FrTermVector *othervect) const ;
      double similarityRatio(const FrTermVector *othervect) const ;
      double jensenShannonDivergence(const FrTermVector *othervect) const ;
      double mountfordCoefficient(const FrTermVector *othervect) const ;
      double braunBlanquetCoefficient(const FrTermVector *othervect) const ;
      double fagerMcGowanCoefficient(const FrTermVector *othervect) const ;
      double tripartiteSimilarityIndex(const FrTermVector *othervect) const ;

      // manipulators
      void setKey(const FrSymbol *k) { keysym = k ; }
      void setCluster(const FrSymbol *cl) { clustername = cl ; }
      void setFreq(size_t freq) { vector_freq = freq ; }
      void setCache(size_t cache_size) ;
      void incrFreq(size_t incr = 1) { vector_freq += incr ; }
      void weightTerms(FrTermVectorWeightingFunc *fn, void *user_data = 0) ;
      void weightTerms(bool linear, double null_weight, size_t range) ;//!!!
      void isNeighbor(bool neighbor) { is_neighbor = neighbor ; }
      void setCFlag(bool flag = true) { cluster_flag = flag ; }
      void normalize() ;
      void mergeIn(const FrTermVector *other, bool use_max_weight = false) ;
      void clearVector() ;
      void setUserData(void *udata) { user_data = udata ; }

      // utility
      static FrTermVector *centroid(const FrList *vectors) ;

      // accessors
      size_t numTerms() const { return num_terms ; }
      double vectorLength() const { return vector_length ; }
      FrSymbol *key() const { return (FrSymbol*)keysym ; }
      FrSymbol *cluster() const { return (FrSymbol*)clustername ; }
      size_t vectorFreq() const { return vector_freq ; }
      FrSymbol *getTerm(size_t termnum) const { return terms[termnum] ; }
      double termWeight(size_t termnum) const { return weights[termnum] ; }
      double maxTermWeight() const ;
      double totalTermWeights() const ;
      void *getUserData() const { return user_data ; }

      // support for clustering code
      void setClusteringData(void *cludata) { clustering_data = cludata ; }
      void *getClusteringData() const { return clustering_data ; }
      void setId(int id) { clustering_data = (void*)((uintptr_t)id) ; }
      int getId() const { return (int)((uintptr_t)clustering_data) ; }
      FrTermVector *nearestNeighborCached() const ;
      FrTermVector *nearestNeighbor() const
	 { if (caching) return nearestNeighborCached() ;
	   else return nearest_neighbor ; }
      double nearestMeasure() const { return nearest_dist ; }
      FrSymbol *nearestKey() const { return nearest_key ; }
      bool isNeighbor() const { return is_neighbor ; }
      bool cachingNeighbors() const { return caching ; }
      bool cFlag() const { return cluster_flag ; } // used by clustering code
      FrBoundedPriQueue *neighborCache() const ;
      void clearNearest()
	 { nearest_dist = -1.0 ; nearest_key = 0 ; is_neighbor = false ;
	   if (!caching) nearest_neighbor = 0 ; }
      void cacheNeighbor(FrTermVector *neighbor, double measure) ;
      void setNearest(FrTermVector *neighbor, FrSymbol *nkey, double measure)
	 { nearest_dist = measure ; nearest_key = nkey ;
	   if (caching) cacheNeighbor(neighbor,measure) ;
	   else nearest_neighbor = neighbor ;
	   if (neighbor) neighbor->isNeighbor(true) ; }
      void setNearest(FrSymbol *nkey) { nearest_key = nkey ; }
      void removeCachedNeighbor(FrTermVector *vec) ;
   } ;

//----------------------------------------------------------------------

void FrTermVecTotalPop(size_t total) ;	// for metrics that use "neither"

double FrTermVecSimilarity(const FrTermVector *tv1,
			   const FrTermVector *tv2,
			   FrClusteringMeasure sim_measure,
			   FrTermVectorSimilarityFunc * = nullptr,
			   void * = nullptr) ;
double FrTermVecSimilarity(const FrTermVector *tv1,
			   const FrTermVector *tv2,
			   FrClusteringMeasure sim_measure,
			   bool normalize_vectors,
			   FrTermVectorSimilarityFunc * = 0,
			   void * = nullptr) ;

double FrVectorSimilarity(FrClusteringMeasure sim_measure,
			  const double *vec1, const double *vec2,
			  size_t veclen, bool normalize = true) ;

#endif /* !__FRTRMVEC_H_INCLUDED */

// end of file frtrmvec.h //
