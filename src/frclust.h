/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC								*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frclust.h	 FrClusterVectors and related functionality	*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1999,2000,2001,2002,2004,2005,2006,2009,2015		*/
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

#ifndef __FRCLUST_H_INCLUDED
#define __FRCLUST_H_INCLUDED

#ifndef __FRAMERR_H_INCLUDED
#include "framerr.h"
#endif

#ifndef __FRLIST_H_INCLUDED
#include "frlist.h"
#endif

#ifndef __FRTRMVEC_H_INCLUDED
#include "frtrmvec.h"
#endif

/************************************************************************/
/*	Manifest Constants						*/
/************************************************************************/

#define FrGENERATED_CLUSTER_PREFIX "<CL_"
#define FrGENERATED_CLUSTER_SUFFIX ">"

/************************************************************************/
/************************************************************************/

class FrThresholdList
   {
   private:
      size_t max_index ;
      double min_threshold ;
      double *thresholds ;
   public:
      FrThresholdList(const char *thr_file, double def_threshold) ;
      // note: if 'thr_file' is 0, def_threshold will be the only threshold
      ~FrThresholdList() ;

      double threshold(size_t index) const
	 { return (index <= max_index) ? thresholds[index]
                         	       : thresholds[max_index] ; }
      double threshold(size_t index, double scalefact) const
	 { return scalefact * ((index <= max_index) ? thresholds[index]
	      				           : thresholds[max_index]) ; }
      double threshold(size_t index1, size_t index2) const
	 { return threshold(index1 < index2 ? index1 : index2) ; }
      double threshold(size_t index1, size_t index2, double scalefact) const
	 { return threshold(index1 < index2 ? index1 : index2,scalefact) ; }
      double minThreshold() const { return min_threshold ; }
   } ;

//----------------------------------------------------------------------

enum FrClusteringMethod
   {
      FrCM_GROUPAVERAGE,
      FrCM_MULTIPASS_GAC,	// two-pass group-average clustering
      FrCM_AGGLOMERATIVE,	// bottom-up agglomerative clustering
      FrCM_KMEANS,		// k-means clustering, random init
      FrCM_KMEANS_HCLUST,	// k-means with hierarchical pre-clustering
      FrCM_TIGHT,		// Tseng's Tight Clustering
      FrCM_GROWSEED,		// add nearest neighbors to existing seeds
      FrCM_SPECTRAL,		// adaptive spectral clustering
      // the following have not yet been implemented
      FrCM_XMEANS,		// x-means: k-means with adaptive k
      FrCM_DET_ANNEAL,		// deterministic annealing
      FrCM_BUCKSHOT		// k-means w/ subsampling for initial centers
   } ;

enum FrClusteringRep
   {
      FrCR_CENTROID,		// use centroid to represent cluster
      FrCR_NEAREST,		// use nearest members
      FrCR_AVERAGE,		// use average over cluster members
      FrCR_RMS,			// use root-mean-square of members' similarity
      FrCR_FURTHEST		// use furthest-apart members
   } ;

//enum FrClusteringMeasure   is defined in frtrmvec.h

// NOTE: not all combinations of the above three items are
//    meaningful/supported:
//       1. in particular, one should generally use FrCR_CENTROID when using
//          FrCM_USER, because FrCM_USER overrides the representative *except*
//          when deciding whether to maintain a centroid for each cluster
//       2. only FrCR_CENTROID is meaningful for FrCM_KMEANS (by definition
//	    of the algorithm)
// NOTE2: if any of the above enums changes, be sure to update checks in
//    FrClusteringParameters ctor

// determine the similarity metric between a term vector and cluster, or
//   between two clusters ('cluster1'==0 in the former case, 'tv'==0 in the
//   latter).  'min_frequency' is to be set by the function to the minimal
//   occurrence frequency, which will be used to determine which threshold
//   value to apply.  Clusters are lists of FrTermVector, where the first
//   element is the centroid of the cluster and the remainder are the members.
typedef double FrClusteringSimilarityFunc(const FrTermVector *tv,
					  const FrList *cluster1,
					  const FrList *cluster2,
					  double best_sim,
					  void *sim_userdata,
					  size_t &min_frequency) ;

typedef bool FrClusterConflictFunc(const FrTermVector *tv1,
				   const FrTermVector *tv2) ;

// take the list of cluster members, and trim it down to 'desired_size'
//   members, if appropriate and necessary; update 'actualsize' and return
//   the new list of class members (should be a shallow copy of the input,
//   is allowed to be the original input if nothing had to be trimmed and
//   'actualsize' remains unchanged)
typedef FrList *FrClusterTrimFunc(const FrList *clustermembers,
				  size_t &actualsize, size_t desired_size,
				  va_list args) ;

//----------------------------------------------------------------------

class FrClusteringParameters
   {
   private:
      double default_threshold ;
      double m_alpha ;
      double m_beta ;
      FrTermVectorSimilarityFunc *tvsim_func ;
      FrClusteringSimilarityFunc *sim_func ;
      FrClusterConflictFunc *conflict_func ;
      FrThresholdList *clus_thresholds ;
      void  *tvsim_data ;
      void  *sim_data ;
      size_t desired_clusters ;
      size_t max_iterations ;		// for k-means & det. annealing
      size_t backoff_stepsize ;
      size_t cache_size ;		// number of nearest neighbors to cache
      size_t num_threads ;
      FrClusteringMethod clus_method ;
      FrClusteringRep clus_rep ;
      FrClusteringMeasure sim_measure ;
      bool sum_cluster_sizes ;
      bool hard_cluster_limit ;
      bool ignore_beyond_cluster_limit ;
   public:
      FrClusteringParameters() ;
      FrClusteringParameters(FrClusteringMethod meth,
			     FrClusteringRep rep,
			     FrClusteringMeasure meas,
			     double def_thresh,
			     FrClusteringSimilarityFunc *sim = nullptr,
			     FrClusterConflictFunc *conf = nullptr,
			     FrThresholdList *threshlist = nullptr,
			     size_t num_clusters = 10,
			     bool sum_sizes = false) ;
      ~FrClusteringParameters() {}

      // modifiers
      void numThreads(size_t thr) { num_threads = thr ; }
      void desiredClusters(size_t num) { desired_clusters = num ; }
      void maxIterations(size_t iter) { max_iterations = iter ; }
      void backoffStep(size_t step) ;
      void cacheSize(size_t c) { cache_size = c ; }
      void hardClusterLimit(bool hard) { hard_cluster_limit = hard ; }
      void ignoreBeyondClusterLimit(bool ignore) { ignore_beyond_cluster_limit = ignore ; }
      void defaultThreshold(double thr) { default_threshold = thr ; }
      void setAlphaBeta(double a, double b = 0.0) { m_alpha = a ; m_beta = b; }
      void tvUserSimFunc(FrTermVectorSimilarityFunc *tvsim, void *udata = nullptr)
	 { tvsim_func = tvsim ; tvsim_data = udata ; }
      void userSimFunc(FrClusteringSimilarityFunc *fn, void *udata = nullptr) { sim_func = fn ; sim_data = udata ; }

      bool parseParameter(const char *parmspec, const char *spec_end) ;

      // accessors
      size_t numThreads() const { return num_threads ; }
      FrClusteringMethod method() const { return clus_method ; }
      FrClusteringRep clusterRep() const { return clus_rep ; }
      FrClusteringMeasure measure() const { return sim_measure ; }
      bool hardClusterLimit() const { return hard_cluster_limit ; }
      bool ignoreBeyondClusterLimit() const { return ignore_beyond_cluster_limit ; }
      double similarity(const FrTermVector *tv1,
			const FrTermVector *tv2) const ;
      double similarity(const FrList *cluster1, const FrList *cluster2) const ;
      bool conflict(const FrTermVector *tv1, const FrTermVector *tv2) const
         { return conflict_func ? conflict_func(tv1,tv2) : false ; }
      FrTermVectorSimilarityFunc *tvSimFn() const { return tvsim_func ; }
      void *tvSimData() const { return tvsim_data ; }
      FrClusteringSimilarityFunc *simFn() const { return sim_func ; }
      void *simData() const { return sim_data ; }
      FrClusterConflictFunc *conflictFn() const { return conflict_func ; }
      double defaultThreshold() const { return default_threshold ; }
      FrThresholdList *thresholds() const { return clus_thresholds ; }
      double threshold(size_t N) const
	 { return clus_thresholds ? clus_thresholds->threshold(N) : defaultThreshold() ; }
      double alpha() const { return m_alpha ; }
      double beta() const { return m_beta ; }
      size_t desiredClusters() const { return desired_clusters ; }
      size_t maxIterations() const { return max_iterations ; }
      size_t backoffStep() const { return backoff_stepsize ; }
      size_t cacheSize() const { return cache_size ; }
      bool sumSizes() const { return sum_cluster_sizes ||
			               clus_rep == FrCR_CENTROID ; }
   } ;

/************************************************************************/
/************************************************************************/

FrClusteringMethod FrParseClusterMethod(const char *meth, ostream *out = nullptr) ;
FrClusteringRep FrParseClusterRep(const char *name, ostream *out = nullptr) ;
FrClusteringMeasure FrParseClusterMetric(const char *meas, ostream *out = nullptr) ;
bool FrParseClusteringParams(const char *paramspec,
			       FrClusteringParameters *params) ;

const char *FrClusteringMetricName(FrClusteringMeasure meas) ;

FrList *FrClusterVectors(const FrList *vectors,
			 const FrClusteringParameters *params,
			 bool exclude_singletons = true,
			 bool run_verbosely = false,
			 bool copy_vectors = false,
			 void **clustering_state = nullptr) ;
// 'vectors' is a list of FrTermVector; the clustering may be seeded by using
//  FrTermVector::setCluster to assign one or more vectors to initial clusters.
// clustering_state is used to permit incremental additions to the clustering.
//  On the first call, point it at a NULL pointer; on subsequent calls, pass
//  in the value to which it has previously been set along with a list of the
//  *new* vectors to be added to the clustering.  Once all vectors have been
//  processed, make one final call with vectors==0 to force a cleanup.
// Note: results will be unpredictable if any parameters other than 'vectors'
//  are changed between calls.

double FrClusterSimilarity(const FrList *cluster1,const FrList *cluster2,
			   const FrClusteringParameters *params,
			   double best_sim = -1.0) ;

double FrClusterSimilarity(const FrTermVector *wordvec, const FrList *cluster,
			   const FrClusteringParameters *params,
			   FrSymbol *&classname, size_t &min_freq,
			   double best_sim = -1.0) ;

bool FrIsGeneratedClusterName(const FrSymbol *name) ;
// determine whether the cluster name was generated by FramepaC or provided
//   as a seed

void FrTrimClusters(void *clustering_state, size_t desired_size,
		    FrClusterTrimFunc *trim, ...) ;
void FrMergeEquivalentClusters(void *clustering_state,
			       const FrClusteringParameters *params) ;

void FrEraseClusterList(FrList *cluster_list, bool copied_vectors = false) ;
// 'copied_vectors' should be the same as 'copy_vectors' on the clustering call

FrList *FrDiversitySampledVectors(const FrList *vectors, size_t samplesize,
				  FrClusteringMeasure measure,
				  FrTermVectorSimilarityFunc *simfn = nullptr,
				  void *simdata = nullptr,
				  bool copy_vectors = false) ;

#endif /* !__FRCLUST_H_INCLUDED */

// end of file frclust.h //
