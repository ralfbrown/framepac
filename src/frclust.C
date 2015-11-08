/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC								*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File: frclust.cpp	      term-vector clustering			*/
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

#include "frassert.h"
#include "frbpriq.h"
#include "frclust.h"
#include "frclustp.h"
#include "frcmove.h"
#include "frhash.h"
#include "frsymtab.h"
#include "frtimer.h"
#include <math.h>

/************************************************************************/
/*	Macros and Manifest Constants					*/
/************************************************************************/

#define gen_cluster_sym() \
	gensym(FrGENERATED_CLUSTER_PREFIX,FrGENERATED_CLUSTER_SUFFIX)

#define DOT_INTERVAL		100
#define MERGE_DOT_INTERVAL	 50
#define INCR_DOT_INTERVAL	500

/************************************************************************/
/*	Global variables						*/
/************************************************************************/

#ifndef NDEBUG
#  undef _FrCURRENT_FILE
   static const char _FrCURRENT_FILE[] = __FILE__ ;
#endif /* !NDEBUG */

static const char *clustering_methods[] =
    { "INCR", "2-pass INCR", "Agglomerative",
      "k-Means-Rand", "k-Means-HClust",
      "Tight", "Grow Seeds", "Spectral",
      "x-Means", "Determ.Annealing",
      "Buckshot" } ;
static const char *clustering_reps[] =
    { "centroid", "nearest", "average", "RMS", "furthest" } ;

static size_t merge_count = 0 ;

static bool caching_neighbors = false ;

/************************************************************************/
/*	External Functions						*/
/************************************************************************/

/************************************************************************/
/*	Types								*/
/************************************************************************/

class Fr_GroupAverageInfo
   { 
   public:
      double              measure ;
      FrSymbol           *classname ;
      const FrTermVector *vector ;
      FrList             *cluster ;
      const FrClusteringParameters *params ;
      bool                no_new_clusters ;
   public:
      Fr_GroupAverageInfo() { measure = -9.99 ; classname = 0 ; }
   } ;

/************************************************************************/
/*	Helper Functions						*/
/************************************************************************/

inline void free_termvec(FrTermVector *tv)
{
   delete tv ;
   return ;
}

//----------------------------------------------------------------------

static void INITCACHE(FrTermVector *tv,size_t cache_size)
{
   if (caching_neighbors)
      tv->setCache(cache_size) ;
   return ;
}

//----------------------------------------------------------------------

static void CACHENEAREST(FrTermVector *vec, FrTermVector *n, double sim)
{
   if (n)
      {
      if (sim > vec->nearestMeasure())
	 vec->setNearest(n,n->key(),sim) ;
      else
	 vec->cacheNeighbor(n,sim) ;
      }
   return ;
}

//----------------------------------------------------------------------

static void SETNEAREST(FrTermVector *vec, FrTermVector *n,
		       FrSymbol *key, double sim, FrSymbol *newkey = 0)
{
   vec->setNearest(n,key,sim) ;
   if (newkey && key == newkey)
      CACHENEAREST(n,vec,sim) ;
   return ;
}

//----------------------------------------------------------------------

#if 0
static void REMOVECACHED(FrTermVector *vec, FrTermVector *n)
{
   vec->removeCachedNeighbor(n) ;
   return ;
}
#endif /* 0 */

//----------------------------------------------------------------------

void Fr__set_cluster_caching(size_t cache_size)
{
   if (cache_size > 1)
      {
      caching_neighbors = true ;
      FrBoundedPriQueue::setAllocators(cache_size) ;
      }
   else
      {
      caching_neighbors = false ;
      FrBoundedPriQueue::setAllocators(0) ;
      }
   return ;
}

/************************************************************************/
/*	methods for class FrClusteringParameters			*/
/************************************************************************/

FrClusteringParameters::FrClusteringParameters()
{
   clus_method = FrCM_GROUPAVERAGE ;
   clus_rep = FrCR_CENTROID ;
   sim_measure = FrCM_COSINE ;
   defaultThreshold(1.0) ;
   clus_thresholds = new FrThresholdList(0,defaultThreshold()) ;
   conflict_func = 0 ;
   sim_func = 0 ;
   tvsim_func = 0 ;
   sum_cluster_sizes = false ;
   desired_clusters = 10 ;
   max_iterations = 10 ;
   backoff_stepsize = 10 ;
   cache_size = 0 ;
   num_threads = 0 ;
   hard_cluster_limit = true ;
   m_alpha = 0.5 ;
   m_beta = 0.5 ;
   return ;
}

//----------------------------------------------------------------------

FrClusteringParameters::FrClusteringParameters(FrClusteringMethod meth,
					       FrClusteringRep rep,
					       FrClusteringMeasure meas,
					       double def_thresh,
					       FrClusteringSimilarityFunc *sim,
					       FrClusterConflictFunc *conf,
					       FrThresholdList *threshlist,
					       size_t num_clusters,
					       bool sum_sizes)
{
   if (meth > FrCM_BUCKSHOT)
      FrProgError("invalid clustering method given to FrClusteringParameters");
   if (rep > FrCR_FURTHEST)
      FrProgError("invalid cluster representative given to FrClusteringParameters");
   if (meas > FrCM_BIN_GAMMA)
      FrProgError("invalid similarity metric given to FrClusteringParameters");
   clus_method = meth ;
   clus_rep = rep ;
   sim_measure = meas ;
   clus_thresholds = threshlist ;
   if (!threshlist)
      clus_thresholds = new FrThresholdList(0,def_thresh) ;
   conflict_func = conf ;
   sim_func = sim ;
   tvsim_func = 0 ;
   defaultThreshold(def_thresh) ;
   desired_clusters = num_clusters ;
   sum_cluster_sizes = sum_sizes ;
   max_iterations = 10 ;
   backoff_stepsize = 10 ;
   cache_size = 0 ;
   num_threads = 0 ;
   hard_cluster_limit = true ;
   ignore_beyond_cluster_limit = false ;
   m_alpha = 0.5 ;
   m_beta = 0.5 ;
   return ;
}

//----------------------------------------------------------------------

void FrClusteringParameters::backoffStep(size_t step)
{
//   if (step == 0)
//      step = 1 ;
   backoff_stepsize = step ;
   return ;
}

/************************************************************************/
/************************************************************************/

static FrList *merge_clusters(FrList *cluster1, FrList *cluster2,
			      bool sum_sizes)
{
   assertq(cluster1 && cluster2 && cluster1 != cluster2) ;
   FrList *newcluster = 0 ;
   // combine the vectors and centroids for the two clusters, and put the
   //   result back into the first cluster
   FrTermVector *centroid1 = (FrTermVector*)poplist(cluster1) ;
   FrTermVector *centroid2 = (FrTermVector*)poplist(cluster2) ;
   FrObject *siz = poplist(cluster1) ;
   size_t size1 = (size_t)siz->intValue() ;
   free_object(siz) ;
   siz = poplist(cluster2) ;
   size_t size2 = (size_t)siz->intValue() ;
   free_object(siz) ;
   if (!centroid1->cluster())
      centroid1->setCluster(centroid2->cluster()) ;
   size_t wc1 = centroid1->vectorFreq() ;
   size_t wc2 = centroid2->vectorFreq() ;
   centroid1->mergeIn(centroid2) ;
   free_termvec(centroid2) ;
   newcluster = cluster1->nconc(cluster2) ;
   centroid1->clearNearest() ;
   // find maximum of the frequencies for the two clusters, and put the
   //   result back into the first cluster
   if (sum_sizes)
      centroid1->setFreq(wc1+wc2) ;
   else
      centroid1->setFreq(FrMax(wc1,wc2)) ;
   // add the updated size back to the list
   pushlist(new FrInteger(size1+size2),newcluster) ;
   // add the centroid back to the list of cluster members
   pushlist(centroid1,newcluster) ;
   return newcluster ;
}

//----------------------------------------------------------------------

static FrList *merge_clusters(FrSymHashTable *clusters,
			      FrSymbol *classname1, FrSymbol *classname2,
			      bool sum_sizes, bool run_verbosely)
{
   FrList *newcluster = 0 ;
   FrObject **clus1 = clusters->lookupValuePtr(classname1) ;
   FrObject **clus2 = clusters->lookupValuePtr(classname2) ;
   if (clus1 && clus2)
      {
      // combine the vectors and centroids for the two clusters, and put the
      //   result back into the first cluster
      FrList *cluster1 = (FrList*)(*clus1) ;
      FrList *cluster2 = (FrList*)(*clus2) ;
      if (run_verbosely)
	 {
	 if (merge_count++ == 0)
	    cout << ";   merging" << flush ;
	 else if (merge_count % MERGE_DOT_INTERVAL == 0)
	    {
	    if (merge_count % (75*MERGE_DOT_INTERVAL) == 0)
	       cout << "\n; " << flush ;
	    cout << '.' << flush ;
	    }
	 }
      newcluster = merge_clusters(cluster1,cluster2,sum_sizes) ;
      (*clus2) = 0 ;
      (*clus1) = newcluster ;
      (void)clusters->remove(classname2) ;
      }
   else if (clus1)
      {
      FrWarningVA("couldn't locate cluster for %s",
		  classname2?classname2->symbolName():"()") ;
      newcluster = (FrList*)(*clus1) ;
      }
   else if (clus2)
      {
      FrWarningVA("couldn't locate cluster for %s",
		  classname1?classname1->symbolName():"()") ;
      newcluster = (FrList*)(*clus2) ;
      clusters->remove(classname2) ;
      clusters->add(classname1,newcluster) ;
      }
   return newcluster ;
}

//----------------------------------------------------------------------

static bool combine_equiv(const FrSymbol *clustername, FrObject *entry, va_list args)
{
   FrList *cluster = (FrList*)entry ;
   if (cluster)
      {
      FrVarArg(FrSymHashTable *,clusters) ;
      FrVarArg(const FrClusteringParameters *,params) ;
      FrTermVector *centroid = FrCLUSTERCENTROID(cluster) ;
      FrSymbol *clus = centroid->cluster() ;
      if (clus && clus != clustername)
	 {
	 FrObject **ent = clusters->lookupValuePtr(clus) ;
	 if (ent && *ent)
	    {
	    (*ent) = merge_clusters(cluster,(FrList*)(*ent),
				    params->sumSizes()) ;
	    }
	 else
	    {
	    clusters->add(clus,cluster) ;
	    centroid->setKey(clus) ;
	    }
	 clusters->remove(clus) ;
	 }
      }
   return true ;			// continue iteration
}

/************************************************************************/
/************************************************************************/

void Fr__new_cluster(FrSymHashTable *clusters, FrSymbol *classname,
		     FrSymbol *clustername, const FrTermVector *tv,
		     FrClusteringRep rep, size_t cache_size)
{
   assertq(tv != 0) ;
   size_t freq = tv->vectorFreq() ;
   FrTermVector *tv2 ;
   if (rep == FrCR_CENTROID)
      {
      tv2 = new FrTermVector(*tv) ;
      if (freq == 0)
	 tv2->setFreq(1) ;
      }
   else
      {
      if (freq == 0) freq = 1 ;
      tv2 = new FrTermVector ;
      tv2->setFreq(freq) ;
      }
   tv2->setKey(clustername) ;
   tv2->setCluster(classname) ;
   INITCACHE(tv2,cache_size) ;
   FrList *clus = new FrList(tv2,new FrInteger(1),tv) ;
   clusters->add(clustername,clus) ;
   return ;
}

//----------------------------------------------------------------------

void Fr__add_to_cluster(FrSymHashTable *clusters,
			FrSymbol *classname, FrTermVector *tv,
			FrClusteringRep rep, size_t cache_size)
{
   assertq(tv != 0) ;
   if (!classname)
      classname = FrSymbolTable::add("") ;
   FrObject **entry = clusters->lookupValuePtr(classname) ;
   if (entry)
      {
      FrList *clus = (FrList*)(*entry) ;
      if (clus)
	 {
	 tv->setCluster(classname) ;
	 // format of data for a cluster is: (centroid size tv1 tv2 ... tvN)
	 // add the new term vector to the front of the term vector list
	 FrList *c2 = FrCLUSTERMEMBERS(clus) ;	// term vector list
	 pushlistnew(tv,c2) ;			// add new one to front
	 clus->rest()->replacd(c2) ;		// paste centroid back on front
	 INCRCLUSTERSIZE(clus,1) ;
	 if (rep == FrCR_CENTROID)
	    {
	    FrObject *first = clus->first() ;
	    assertq (first != 0) ;
	    if (first->consp())
	       ((FrTermVector*)((FrCons*)first)->first())->mergeIn(tv) ;
	    else
	       ((FrTermVector*)first)->mergeIn(tv) ;
	    }
	 else
	    {
	    FrTermVector *centroid = (FrTermVector*)clus->first() ;
	    centroid->incrFreq(tv->vectorFreq()) ;
	    }
	 }
      else
	 {
	 FrTermVector *tv2 ;
	 if (rep == FrCR_CENTROID)
	    tv2 = new FrTermVector(*tv) ;
	 else
	    {
	    tv2 = new FrTermVector ;
	    size_t freq = tv->vectorFreq() ;
	    if (freq == 0) freq = 1 ;
	    tv2->setFreq(freq) ;
	    }
	 tv2->setCluster(classname) ;
	 (*entry) = new FrList(tv2,new FrInteger(1),tv) ;
	 }
      }
   else
      new_cluster(clusters,classname,classname,tv,rep,cache_size) ;
   return ;
}

//----------------------------------------------------------------------

void Fr__insert_in_cluster(FrSymHashTable *clusters, FrSymbol *newcluster,
			   FrTermVector *tv,
			   const FrClusteringParameters *params)
{
   FrObject *clus_obj ;
   if (clusters->lookup(newcluster,&clus_obj))
      {
      FrList *clus = (FrList*)clus_obj ;
      tv->setCluster(newcluster) ;
      ADDCLUSTERMEMBER(clus,tv) ;
      INCRCLUSTERSIZE(clus,1) ;
      }
   else
      {
      // that cluster doesn't exist yet, so make a new one-element cluster
      new_cluster(clusters,newcluster,newcluster,tv,params->clusterRep(),
		  params->cacheSize()) ;
      }
   return ;
}

//----------------------------------------------------------------------

static void make_initial_cluster(FrSymHashTable *clusters,
				 FrTermVector *wordvec,
				 FrClusteringRep rep,
				 FrSymHashTable *map_ht,
				 size_t cache_size,
				 bool run_verbosely)
{
   static size_t count = 0 ;
   FrSymbol *clustername = gen_cluster_sym() ;
   new_cluster(clusters,wordvec->cluster(),clustername,wordvec,rep,cache_size);
   map_ht->add(wordvec->key(),clustername) ;
   if (run_verbosely && ++count % DOT_INTERVAL == 0)
      cout << '.' << flush ;
   return ;
}

//----------------------------------------------------------------------

static void check_cluster_parallel(const void *input, void * /*output*/ )
{
   const Fr_GroupAverageInfo *order= static_cast<const Fr_GroupAverageInfo*>(input) ;
   const FrTermVector *wordvec = order->vector ;
   FrList *cluster = order->cluster ;
   const FrClusteringParameters *params = order->params ;

   FrSymbol *classname = 0 ;
   size_t min_freq ;
   double sim_thresh = -999.0 ;
   const FrThresholdList *thresholds = params->thresholds() ;
   if (!thresholds)
      {
      sim_thresh = params->defaultThreshold() ;
      }
   double sim = FrClusterSimilarity(wordvec,cluster,params,classname,
				    min_freq,sim_thresh);
   Fr_GroupAverageInfo *result = const_cast<Fr_GroupAverageInfo*>(order) ;
   result->measure = -9.99 ;
   result->classname = 0 ;
   if (sim >= 0.0)
      {
      FrSymbol *seedclass = wordvec->cluster() ;
      if (seedclass && seedclass == classname)
	 {
	 result->classname = seedclass ;
	 result->measure = sim ;
	 return ;
	 }
      if (classname)
	 {
	 size_t wf = wordvec->vectorFreq() ;
	 if (order->no_new_clusters ||
	     (thresholds && sim >= thresholds->threshold(wf,min_freq)) ||
	     (!thresholds && sim >= params->defaultThreshold()))
	    {
	    result->measure = sim ;
	    result->classname = classname ;
	    }
	 }
      }
   return ;
}

//----------------------------------------------------------------------

static bool check_cluster(const FrSymbol *, FrObject *cl, va_list args)
{
   FrList *cluster = (FrList*)cl ;
   FrVarArg(FrThreadPool *,tpool) ;
   FrVarArg(Fr_GroupAverageInfo *,workorders) ;
   FrVarArg(size_t *,workorder_number) ;
   FrVarArg(const FrTermVector *,wordvec) ;
   FrVarArg(const FrClusteringParameters *,params) ;
   FrVarArg2(bool,int,no_new_clusters) ;
   if (cluster)
      {
      size_t worknum = (*workorder_number)++ ;
      workorders[worknum].vector = wordvec ;
      workorders[worknum].cluster = cluster ;
      workorders[worknum].params = params ;
      workorders[worknum].no_new_clusters = no_new_clusters ;
      tpool->dispatch(check_cluster_parallel,&workorders[worknum],0) ;
      }
   return true ;
}

//----------------------------------------------------------------------

static bool clear_cluster(const FrSymbol *name, FrObject *cl, va_list)
{
   // remove the elements of the cluster, but retain the centroid
   FrList *cluster = (FrList*)cl ;
   FrList *c2 = cluster->rest() ;	// skip the centroid
   free_object(c2->first()) ;		// release the current size variable
   c2->replaca(new FrInteger(0)) ;	// and reset size to zero
   FrList *members = c2->rest() ;
   if (FrIsGeneratedClusterName(name))
      {
      while (members)
	 {
	 FrTermVector *tv = (FrTermVector*)poplist(members) ;
	 if (tv)
	    tv->setCluster(0) ;
	 }
      }
   else
      members->eraseList(false) ;
   c2->replacd(0) ;
   return true ;			// continue iterating
}

//----------------------------------------------------------------------

bool Fr__copy_cluster(FrSymHashTable *clusters, FrSymbol *classname,
		      const FrList *clus)
{
   if (!clusters || !clus || !classname)
      return false ;
   FrList *members = FrCLUSTERMEMBERS(clus) ;
   FrTermVector *centroid = FrCLUSTERCENTROID(clus) ;
   FrObject *count = clus->second() ;
   if (members)
      members = (FrList*)members->copy() ;
   pushlist(count ? count->deepcopy() : 0,members) ;
   pushlist(centroid ? centroid->deepcopy() : 0,members) ;
   clusters->add(classname, members);
   return true ;
}

//----------------------------------------------------------------------

bool Fr__free_cluster(const FrSymbol *, FrObject *cl)
{
   // erase the cluster contents
   FrList *cluster = (FrList*)cl ;
   FrTermVector *centroid = (FrTermVector*)poplist(cluster) ;
   free_termvec(centroid) ;
   if (cluster)
      free_object(cluster->first()) ;	// free the size variable
   if (cluster && cluster->consp())
      cluster->eraseList(false) ;
   return true ;			// continue iterating
}

//----------------------------------------------------------------------

static bool set_nearest_neighbor(FrTermVector *tv, const FrList *vectors,
				 const FrClusteringParameters *params,
				 bool use_all = false)
{
   if (!tv)
      return false ;
   double max_sim = -1.0 ;
   FrTermVector *neighbor = 0 ;
   FrClusteringRep rep = params->clusterRep() ;
   FrThresholdList *threshs = params->thresholds() ;
   bool keep_all = use_all || (rep != FrCR_NEAREST && rep != FrCR_FURTHEST) ;
   FrClusteringMeasure measure = params->measure() ;
   for ( ; vectors ; vectors = vectors->rest())
      {
      FrTermVector *vector = (FrTermVector*)vectors->first() ;
      if (!vector || vector == tv)
	 continue ;
      double sim = FrTermVecSimilarity(vector,tv,measure,params->tvSimFn(),params->tvSimData()) ;
      if (sim > max_sim && !conflicting_seed_cluster(vector,tv,params) &&
	  (keep_all || sim >= threshs->minThreshold()))
	 {
	 max_sim = sim ;
	 neighbor = vector ;
	 }
      }
   if (neighbor)
      {
      INITCACHE(tv,params->cacheSize()) ;
      SETNEAREST(tv,neighbor,neighbor->key(),max_sim) ;
      return true ;
      }
   else
      return false ;
}

//----------------------------------------------------------------------

static bool map_neighbor(const FrSymbol *term, FrObject *cl, va_list args)
{
   FrList *cluster = (FrList*)cl ;
   FrVarArg(FrSymHashTable *,ht) ;
   FrTermVector *tv = FrCLUSTERCENTROID(cluster) ;
   assertq(ht && tv) ;
   FrObject *near_obj ;
   if (ht->lookup(tv->nearestKey(),&near_obj))
      tv->setNearest((FrSymbol*)near_obj) ;
   else if (tv->nearestKey() != 0)
      {
      FrVarArg2(bool,int,run_verbosely) ;
      if (run_verbosely)
	 cout << "warning: " << tv->nearestKey() << " not found for "
	      << term << "!" << endl ;
      }
   return true ;			// continue iterating
}

//----------------------------------------------------------------------

static bool find_nearest(const FrSymbol *term, FrObject *cl, va_list args)
{
   FrList *cluster = (FrList*)cl ;
   FrVarArg(const FrClusteringParameters *,params) ;
   FrVarArg(double,threshscale) ;
   FrTermVector *centroid = FrCLUSTERCENTROID(cluster) ;
   FrVarArg(double *,max) ;
   assertq(centroid != 0) ;
   FrVarArg(FrSymbol **,best_key1) ;
   FrTermVector *neighbor = centroid->nearestNeighbor() ;
   FrVarArg(FrSymbol **,best_key2) ;
   if (neighbor == centroid
#if 0
       || centroid->nearestKey() == term
#endif
      )
      {
      // Oops!  Recover semi-gracefully by clearing the neighbor pointer
      FrVarArg2(bool,int,run_verbosely) ;
      if (run_verbosely)
	 cout << ";    " << term
	      << "'s nearest-neighbor set to itself!  Clearing...."
	      << endl ;
      centroid->clearNearest() ;
      return true ;
      }
   FrThresholdList *threshs = params->thresholds() ;
   double nearest ;
   if (neighbor && !conflicting_seed_cluster(centroid,neighbor,params) &&
       centroid->nearestKey() &&
       (nearest = centroid->nearestMeasure()) > *max &&
	nearest >= threshs->threshold(centroid->vectorFreq(),
				      neighbor->vectorFreq(),threshscale))
      {
      *max = nearest ;
      *best_key1 = (FrSymbol*)term ;
      *best_key2 = centroid->nearestKey() ;
      }
   return true ;			// continue iterating
}

//----------------------------------------------------------------------

static double find_nearest(FrSymHashTable *clusters,
			   const FrClusteringParameters *params,
			   double threshscale,
			   FrSymbol *&cluster1, FrSymbol *&cluster2,
			   bool run_verbosely)
{
   assertq(clusters != 0) ;
   double best_measure = -1.0 ;
   FrSymbol *best_key1 = 0 ;
   FrSymbol *best_key2 = 0 ;
   clusters->iterate(find_nearest,params,threshscale,&best_measure,
		     &best_key1,&best_key2,run_verbosely) ;
   if (best_measure >= 0.0)
      {
      cluster1 = best_key1 ;
      cluster2 = best_key2 ;
      }
   return best_measure ;
}

//----------------------------------------------------------------------

static bool find_nearest_to(const FrSymbol *term, FrObject *cl, va_list args)
{
   FrList *cluster = (FrList*)cl ;
   FrVarArg(FrList *,refcluster) ;
   FrVarArg(FrSymbol *,key) ;
   FrVarArg(const FrClusteringParameters *,params) ;
   FrVarArg(double,threshscale) ;
   FrVarArg(double *,max) ;
   FrVarArg(FrTermVector **,best_tv) ;
   FrTermVector *centroid = FrCLUSTERCENTROID(cluster) ;
   FrTermVector *reference = FrCLUSTERCENTROID(refcluster) ;
   if (reference && centroid != reference && term != key &&
       !conflicting_seed_cluster(centroid,reference,params))
      {
      FrThresholdList *thresholds = params->thresholds() ;
      double sim = Fr__cluster_similarity(cluster,refcluster,params,*max) ;
      if (sim >= thresholds->threshold(centroid->vectorFreq(),
				       reference->vectorFreq(),
				       threshscale))
	 {
	 CACHENEAREST(centroid,reference,sim) ;
	 CACHENEAREST(reference,centroid,sim) ;
	 if (sim > *max)
	    {
	    *max = sim ;
	    *best_tv = centroid ;
	    }
	 }
      }
   return true ;			// continue iterating
}

//----------------------------------------------------------------------

static bool update_nn(const FrSymbol *term, FrObject *cl, va_list args)
{
   const FrList *cluster = (FrList*)cl ;
   FrVarArg(const FrList *,newclus) ;
   if (cluster)
      {
      FrVarArg(const FrTermVector *,oldcent) ;
      FrVarArg(FrSymbol *,newkey) ;
      const FrSymbol *key = term ;
      if (key == newkey)
	 {
	 // this is the cluster that had another merged in, which means we
	 //   must recompute the distances to ALL other clusters
	 // HOWEVER, since the similarity metric is symmetric, the new nearest
	 //    neighbor must be a cluster for which the changed cluster is its
	 //    nearest neighbor, and we're computing those anyway (so we'll
	 //   piggy-back on that computation below)
	 return true ;			// continue iterating
	 }
      FrVarArg(FrSymbol *,removed) ;
      FrVarArg(FrSymHashTable *,clusters) ;
      FrVarArg(const FrClusteringParameters *,params) ;
      FrVarArg(double,threshscale) ;
      FrTermVector *centroid = FrCLUSTERCENTROID(cluster) ;
      FrTermVector *newcent = FrCLUSTERCENTROID(newclus) ;
      FrClusteringRep rep = params->clusterRep() ;
      bool nearest_dist = (rep == FrCR_NEAREST) ;
      if (centroid == newcent ||
	  (nearest_dist && key != newkey && !centroid->nearestNeighbor()))
	 return true ;
      double sim = -1.0 ;
      if (!nearest_dist && !conflicting_seed_cluster(centroid,newcent,params))
	 sim = Fr__cluster_similarity(cluster,newclus,params,centroid->nearestMeasure()) ;
      if (sim > centroid->nearestMeasure())
	 {
	 SETNEAREST(centroid,newcent,newkey,sim,newkey) ;
	 }
      else if (centroid->nearestKey() == removed ||
	       centroid->nearestKey() == newkey)
	 {
	 // the former nearest neighbor was one of the two clusters we
	 //   just merged
	 if (nearest_dist)
	    {
	    // when using the nearest pair as the similarity measurement
	    //   point, we
	    //   can usually forego a complete recomputation and just use the
	    //   nearer of the two clusters that got merged; since one of
	    //   the two was already the best neighbor, the merged cluster
	    //   must still be the best.
	    FrThresholdList *thresholds = params->thresholds() ;
	    double csim = centroid->nearestMeasure() ;
	    if (csim >= thresholds->threshold(centroid->vectorFreq(),
					      newcent->vectorFreq(),
					      threshscale))
	       {
	       SETNEAREST(centroid,newcent,newkey,csim,newkey) ;
	       return true ;
	       }
	    else if (centroid->nearestKey() == newkey)
	       return true ;
	    }
	 if (caching_neighbors)
	    {
	    FrBoundedPriQueue *q = centroid->neighborCache() ;
	    if (q)
	       {
	       // delete the cached entries for the two clusters that got
	       //   merged
	       q->remove(oldcent) ;
	       q->remove(newcent) ;
	       // compute the similarity with the composite cluster
	       sim = Fr__cluster_similarity(cluster,newclus,params,q->lastPriority()) ;
	       // if the similarity is still within the range of cached values,
	       //   add the composite back to the cache
	       if (sim >= q->lastPriority())
		  q->push(newcent,sim) ;
	       // grab the best-valued cluster out of the cache and make it the
	       //   new neighbor
	       if (q->queueLength() > 0)
		  {
		  double csim ;
		  FrTermVector *cent = (FrTermVector*)q->pop(csim) ;
		  FrSymbol *nearkey = cent ? cent->key() : 0 ;
		  SETNEAREST(centroid,cent,nearkey,csim,newkey) ;
		  return true ;
		  }
	       // if the cache is empty, we must recompute similarity with ALL
	       //    other clusters, refilling the cache along the way
	       }
	    }
	 // recompute the distances to ALL clusters if the nearest neighbor
	 //   is the one that got merged away or the one merged into
	 double max = -1.0 ;
	 FrTermVector *nearest = 0 ;
	 clusters->iterate(find_nearest_to,cluster,term,
			   params,threshscale,&max,&nearest) ;
//	 if (verbose)
//	    cout << "nearest to " << term << " is "
//		 << nearest ? nearest->key() : 0 << " at " << max << endl ;
	 FrSymbol *nearkey = nearest ? nearest->key() : 0 ;
	 SETNEAREST(centroid,nearest,nearkey,max,newkey) ;
	 }
      else if (caching_neighbors &&
	       sim >= (centroid->neighborCache())->lastPriority())
	 {
	 SETNEAREST(centroid,newcent,newkey,sim,newkey) ;
	 }
      }
   return true ;			// continue iterating
}

//----------------------------------------------------------------------

static void cluster_bottom_up(const FrList *vectors, FrSymHashTable *clusters,
			      const FrClusteringParameters *params,
			      bool exclude_singletons, bool run_verbosely)
{
   assertq(params != 0) ;
   FrTimer timer ;
   Fr__set_cluster_caching(params->cacheSize()) ;
   // start by making each vector a cluster by itself
   if (run_verbosely)
      cout << ";   creating initial clusters" << flush ;
   size_t num_vectors = vectors->listlength() ;
   FrSymHashTable *map_ht = new FrSymHashTable(num_vectors+1) ;
   if (!map_ht)
      {
      FrNoMemory("allocating neighbor-map hash table") ;
      return ;
      }
   // expand the hash tables now to avoid fragmenting memory from repeated
   //   incremental expansions
   clusters->expandTo(num_vectors+1) ;
   // first pass: find nearest neighbor for each vector, and create a
   //   singleton cluster if the neighbor is above the minimum acceptable
   //   similarity
   bool use_all = (!exclude_singletons ||
		     params->desiredClusters() < num_vectors) ;
   for (const FrList *veclist = vectors ; veclist ; veclist = veclist->rest())
      {
      FrTermVector *wordvec = (FrTermVector*)veclist->first() ;
      if (!wordvec)
	 continue ;
      if (set_nearest_neighbor(wordvec,vectors,params,use_all))
	 make_initial_cluster(clusters,wordvec,FrCR_CENTROID,map_ht,
			      params->cacheSize(),run_verbosely) ;
      }
   // second pass:  for any vector which hasn't already been turned into a
   //   singleton cluster, create a cluster if it's somebody's neighbor
   if (run_verbosely)
      cout << '!' << flush ;
   for ( ; vectors ; vectors = vectors->rest())
      {
      FrTermVector *wordvec = (FrTermVector*)vectors->first() ;
      if (!wordvec || wordvec->nearestNeighbor())
	 continue ;
      if (wordvec->cluster() || wordvec->isNeighbor())
	 make_initial_cluster(clusters,wordvec,FrCR_CENTROID,map_ht,
			      params->cacheSize(),run_verbosely) ;
      }
   clusters->iterate(map_neighbor,map_ht,run_verbosely) ;
   delete map_ht ;
   if (run_verbosely)
      cout << endl
	   << ";   starting with " << clusters->currentSize()
	   << " initial singleton clusters (took " << timer.readsec()
	   << " seconds)" << endl ;
   timer.start() ;
   // now, as long as the closest clusters aren't too far apart, find the
   // two closest (by similarity measure) clusters and merge them into one
   double measure = 0 ;
   FrSymbol *clust1(0), *clust2(0) ;
   size_t desired_clusters = params->desiredClusters() ;
   if (desired_clusters < 2)
      desired_clusters = 2 ;
   if (run_verbosely)
      cout << ";   merging clusters by similarity (" << desired_clusters
	   << " desired)" << endl ;
   double threshscale = 1.0 ;
   size_t backed_off = 0 ;
   while (clusters->currentSize() > desired_clusters && threshscale >= 0.0)
      {
      if (run_verbosely && backed_off)
	 {
	 cout << "\n;     " << clusters->currentSize()
	      << " remain, backing off -- scaling thresholds by "
	      << threshscale << endl ;
	 }
      merge_count = 0 ;
      while (clusters->currentSize() > desired_clusters &&
	     (measure = find_nearest(clusters,params,threshscale,clust1,clust2,
				     run_verbosely)) >= 0.0)
	 {
	 FrObject *oldclus_obj ;
	 (void)clusters->lookup(clust2,&oldclus_obj) ;
	 FrList *oldclus = (FrList*)oldclus_obj ;
	 FrTermVector *removed = oldclus ? FrCLUSTERCENTROID(oldclus) : 0 ;
	 FrList *newclus = merge_clusters(clusters,clust1,clust2,
					  params->sumSizes(),run_verbosely) ;
	 FrTermVector *newcent = newclus ? FrCLUSTERCENTROID(newclus) : 0 ;
	 if (newcent)
	    newcent->clearNearest() ;
	 (void)clusters->iterate(update_nn,newclus,removed,clust1,clust2,
				 clusters,params,threshscale) ;
	 if (caching_neighbors)
	    {
	    FrBoundedPriQueue *q = newcent->neighborCache() ;
	    if (q)
	       {
	       double sim ;
	       FrTermVector *cent = (FrTermVector*)q->pop(sim) ;
	       SETNEAREST(newcent,cent,cent ? cent->key() : 0,sim) ;
	       }
	    }
	 }
      double backoff = params->backoffStep() / 100.0 ;
      if (backoff == 0)
	 break ;
      if (threshscale < backoff)
	 backoff /= 3.0 ;
      else if (threshscale < 3.0*backoff)
	 backoff /= 2.0 ;
      threshscale -= backoff ;
      backed_off++ ;
      }
   if (run_verbosely)			// we've been printing periods, so
      cout << endl ;			//   terminate the output line
   // merge seeded vectors that wound up in different clusters
   FrMergeEquivalentClusters(clusters,params) ;
   if (run_verbosely)
      cout << ";   (merging took " << timer.readsec() << " seconds)" << endl ;
   return ;
}

//----------------------------------------------------------------------

static void cluster_incr(const FrList *vectors, FrSymHashTable *clusters,
			 const FrClusteringParameters *params,
			 bool run_verbosely = false)
{
   assertq(params != 0) ;
   Fr__set_cluster_caching(0) ;
   FrClusteringRep rep = params->clusterRep() ;
   // build initial clusters from any vectors which have already been tagged
   //   with a cluster ID
   for (const FrList *veclist = vectors ; veclist ; veclist = veclist->rest())
      {
      FrTermVector *tv = (FrTermVector*)veclist->first() ;
      if (tv && tv->cluster())
	 Fr__add_to_cluster(clusters,tv->cluster(),tv,rep,params->cacheSize());
      }
   size_t num_clusters = clusters->currentSize() ;
   size_t max_clusters = params->desiredClusters() ;
   // adjust cluster limit if we have more seed clusters than the
   //   requested number of clusters
   if (max_clusters <= num_clusters)
      max_clusters = num_clusters + 1 ;
   if (max_clusters < 2)
      max_clusters = UINT_MAX ;
   FrThreadPool tpool(params->numThreads()) ;
   bool must_wait = (params->numThreads() != 0) ;
   // process the remaining vectors which have not yet been assigned to a
   //   cluster
   static size_t count = 0 ;
   if (run_verbosely)
      cout << "; " << flush ;
   bool reported_limit = false ;
   Fr_GroupAverageInfo *workorders = new Fr_GroupAverageInfo[max_clusters] ;
   for ( ; vectors ; vectors = vectors->rest())
      {
      FrTermVector *wordvec = (FrTermVector*)vectors->first() ;
      if (!wordvec || wordvec->cluster() != 0)
	 continue ;
      size_t workorder_number = 0 ;
      bool hit_cluster_limit = params->hardClusterLimit() && num_clusters >= max_clusters ;
      (void)clusters->iterate(check_cluster,&tpool,workorders,&workorder_number,wordvec,
			      params,hit_cluster_limit) ;
      if (must_wait)
	 {
	 tpool.waitUntilIdle() ;
	 }
      double bestmeasure = -1.0 ;
      FrSymbol *bestclass = 0 ;
      for (size_t i = 0 ; i < num_clusters ; i++)
	 {
	 if (workorders[i].measure > bestmeasure && workorders[i].classname)
	    {
	    bestmeasure = workorders[i].measure ;
	    bestclass = workorders[i].classname ;
	    }
	 }
      if (run_verbosely && ++count % INCR_DOT_INTERVAL == 0)
	 {
	 if (count % (75*INCR_DOT_INTERVAL) == 0)
	    cout << "\n; " << flush ;
	 cout << '.' << flush ;
	 }
      if (bestclass)
	 {
	 // add to the existing cluster and adjust cluster's centroid
	 Fr__add_to_cluster(clusters,bestclass,wordvec,rep,
			    params->cacheSize()) ;
	 }
      else if (!params->ignoreBeyondClusterLimit() || num_clusters < max_clusters)
	 {
	 // none of the existing clusters is close enough to the new word,
	 //   so create a new cluster
	 FrSymbol *clustername = wordvec->cluster() ;
	 if (!clustername)		// seeded class?
	    {
	    clustername = gen_cluster_sym() ;
	    wordvec->setCluster(clustername) ;
	    }
	 new_cluster(clusters,clustername,clustername,wordvec,rep,
		     params->cacheSize()) ;
	 num_clusters++ ;
	 if (run_verbosely && num_clusters >= max_clusters && !reported_limit)
	    {
	    cout << "(cluster limit)" << flush ;
	    reported_limit = true ;
	    }
	 if (num_clusters > max_clusters)
	    {
	    delete [] workorders ;
	    workorders = new Fr_GroupAverageInfo[num_clusters+1] ;
	    }
	 }
      }
   delete [] workorders ;
   if (run_verbosely)
      cout << endl ;
   return ;
}

//----------------------------------------------------------------------

static bool trim_cluster(const FrSymbol *, FrObject **entry, va_list args)
{
   FrVarArg(FrClusterTrimFunc*,trimfunc) ;
   FrList *cluster = (FrList*)(*entry) ;
   if (cluster && trimfunc)
      {
      FrVarArg(size_t,desired_size) ;
      // all these machinations with the arglist are necessitated by
      //  implementations (e.g. Watcom) where va_list is an array
      FrVarArg(va_list*,t_args) ;
      va_list trim_args ;
      FrCopyVAList(trim_args,t_args[0]) ;
      FrTermVector *centroid = (FrTermVector*)poplist(cluster) ;
      FrObject *size = poplist(cluster) ;
      size_t siz = size->intValue() ;
      free_object(size) ;
      FrSafeVAList(trim_args) ;
      FrList *newcluster = trimfunc(cluster,siz,desired_size,
				    FrSafeVarArgs(trim_args)) ;
      if (newcluster != cluster)
	 cluster->eraseList(false) ;
      if (centroid)
	 {
	 free_object(centroid) ;
	 centroid = Fr__make_centroid(newcluster) ;
	 }
      pushlist(new FrInteger(siz),newcluster) ;
      pushlist(centroid,newcluster) ;
      (*entry) = newcluster ;
      }
   return true ;			// continue iterating
}

//----------------------------------------------------------------------

static bool collect_cluster(const FrSymbol *name, FrObject *entry, va_list args)
{
   FrVarArg(FrList**,cluster_list) ;
   FrVarArg2(bool,int,copy_vectors) ;
   FrVarArg2(bool,int,exclude_singletons) ;
   FrVarArg2(bool,int,cleanup_hashtable) ;
   FrVarArg(FrSymHashTable*,ht) ;
   FrList *cluster = (FrList*)entry ;
   if (cleanup_hashtable)
      {
      FrTermVector *centroid = (FrTermVector*)poplist(cluster) ;
      ht->add(name,(FrObject*)0,true) ;	// erase the cluster contents
      FrObject *size = poplist(cluster) ;
      free_termvec(centroid) ;
      free_object(size) ;
      }
   else
      cluster = FrCLUSTERMEMBERS(cluster) ;
   if (cluster && (!exclude_singletons || cluster->rest() ||
		   !FrIsGeneratedClusterName(name)))
      {
      FrList *members ;
      if (copy_vectors)
	 {
	 members = (FrList*)cluster->deepcopy() ;
	 if (cleanup_hashtable)
	    cluster->eraseList(false) ;
	 }
      else if (cleanup_hashtable)
	 members = cluster ;
      else
	 members = (FrList*)cluster->copy() ;
      for (const FrList *cl = members ; cl ; cl = cl->rest())
	 {
	 FrTermVector *tv = (FrTermVector*)cl->first() ;
	 tv->setCluster(name) ;
	 }
      pushlist(name,members) ;
      pushlist(members,*cluster_list) ;
      }
   else if (cleanup_hashtable)
      cluster->eraseList(false) ;
   return true ;			// continue iterating
}

/************************************************************************/
/*	the public clustering functions					*/
/************************************************************************/

FrList *FrClusterVectors(const FrList *vectors,
			 const FrClusteringParameters *params,
			 bool exclude_singletons, bool run_verbosely,
			 bool copy_vectors, void **clustering_state)
{
   // 'vectors' is a list of FrTermVector; the vectors may be assigned to
   //   initial clusters prior to calling this function.  Each vector must
   //   have a unique non-0 key.
   // return value is a list of lists whose first element is the cluster ID
   //   and whose remaining elements are the vectors belonging to the cluster;
   //   unless 'copy_vectors' is true, the result list will point at the
   //   original FrTermVector objects.  If 'exclude_singletons' is true,
   //   single-element clusters will be omitted from the result.  Use
   //   FrEraseClusterList() to free the result.
   //
   if (!params)
      return 0 ;
   if (clustering_state && *clustering_state && !vectors)
      {
      // this was a cleanup request, so make a list out of the final clusters
      //   and delete the hash table of clusters
      FrSymHashTable *clusters = (FrSymHashTable*)*clustering_state ;
      FrList *cluster_list = 0 ;
      if (clusters)
	 {
	 clusters->iterate(collect_cluster,&cluster_list,copy_vectors,
			   exclude_singletons,true,clusters) ;
	 delete clusters ;
	 }
      if (run_verbosely)
	 cout << "; final clustering cleanup -- returning "
	      << cluster_list->listlength() << " clusters" << endl ;
      return cluster_list ;
      }
   FrThresholdList *thresholds = params->thresholds() ;
   FrClusteringMethod method = params->method() ;
   FrClusteringRep rep = params->clusterRep() ;
   FrClusteringMeasure measure = params->measure() ;
   FrSymHashTable *clusters ;
   if (clustering_state && *clustering_state)
      clusters = (FrSymHashTable*)*clustering_state ;
   else
      {
      clusters = new FrSymHashTable(2*params->desiredClusters()) ;
      if (clusters) clusters->onRemove(Fr__free_cluster) ;
      }
   if (run_verbosely)
      {
      cout << ";   starting clustering, method="
	   << clustering_methods[method] << ", rep="
	   << clustering_reps[rep] << ", measure="
	   << FrClusteringMetricName(measure) ;
      if (clustering_state && *clustering_state)
	 cout << " (" << clusters->currentSize() << " prev clusters)" ;
      cout << endl ;
      }
   switch (method)
      {
      case FrCM_MULTIPASS_GAC:
	 // run a first clustering pass to generate initial centroids for GAC
	 {
	 double newthresh = thresholds->threshold(1)*0.9 ;
	 FrThresholdList *newthresholds = new FrThresholdList(0,newthresh) ;
	 FrClusteringParameters newparams(method,rep,measure,newthresh,
					  params->simFn(),params->conflictFn(),
					  newthresholds,
					  params->desiredClusters(),
					  params->sumSizes()) ;
	 cluster_incr(vectors,clusters,&newparams,run_verbosely) ;
	 delete newthresholds ;
	 clusters->iterate(clear_cluster) ;
	 if (run_verbosely)
	    cout << "; reclustering based on initial "
		 << clusters->currentSize() << " centroids" << endl ;
	 }
	 // fall through to FrCM_GROUPAVERAGE
      case FrCM_GROUPAVERAGE:
	 cluster_incr(vectors,clusters,params,run_verbosely) ;
	 break ;
      case FrCM_AGGLOMERATIVE:
	 cluster_bottom_up(vectors,clusters,params,exclude_singletons,
			   run_verbosely) ;
	 break ;
      case FrCM_KMEANS:
	 Fr__cluster_kmeans(vectors,clusters,params,run_verbosely) ;
	 break ;
      case FrCM_KMEANS_HCLUST:
	 Fr__cluster_kmeans_hclust(vectors,clusters,params,run_verbosely) ;
	 break ;
      case FrCM_GROWSEED:
	 Fr__cluster_growseed(vectors,clusters,params,run_verbosely) ;
	 break ;
#ifdef FrGPL
      case FrCM_TIGHT:
	 Fr__cluster_tight(vectors,clusters,params,run_verbosely) ;
	 break ;
      case FrCM_SPECTRAL:
 	 Fr__cluster_spectral(vectors,clusters,params,run_verbosely) ;
	 break ;
#else
      case FrCM_TIGHT:
      case FrCM_SPECTRAL:
#endif /* FrGPL */
      case FrCM_XMEANS:
      case FrCM_BUCKSHOT:
      case FrCM_DET_ANNEAL:
	 cout << "Unimplemented clustering method specified!" << endl ;
	 break ;
      default:
	 cout << "Unknown clustering method specified!" << endl ;
	 break ;
      }
   FrList *cluster_list = 0 ;
   if (clusters)
      {
      clusters->onRemove(0) ;
      clusters->iterate(collect_cluster,&cluster_list,copy_vectors,
			exclude_singletons,clustering_state == 0,clusters) ;
      }
   if (clustering_state)
      *clustering_state = clusters ;
   else
      delete clusters ;
   if (run_verbosely)
      cout << ";   clustering complete: " << vectors->listlength()
	   << " vectors ==> " << cluster_list->listlength() << " clusters"
	   << endl ;
   return cluster_list ;
}

//----------------------------------------------------------------------

bool FrIsGeneratedClusterName(const FrSymbol *name)
{
   assertq(name != 0) ;
   const char *clusname = name->symbolName() ;
   return (memcmp(clusname,FrGENERATED_CLUSTER_PREFIX,
		  sizeof(FrGENERATED_CLUSTER_PREFIX)-1) == 0 &&
	   Fr_isdigit(clusname[sizeof(FrGENERATED_CLUSTER_PREFIX)-1])) ;
}

//----------------------------------------------------------------------

void FrTrimClusters(void *clustering_state, size_t desired_size,
		    FrClusterTrimFunc *trim, ...)
{
   if (clustering_state && trim)
      {
      FrVarArgs(trim) ;
      FrSymHashTable *clusters = (FrSymHashTable*)clustering_state ;
      // all these machinations with the arglist are necessitated by
      //  implementations (e.g. Watcom) where va_list is an array
      va_list arglist[1] ;
      FrCopyVAList(arglist[0],args) ;
      clusters->iterateAndModify(trim_cluster,desired_size,trim,arglist) ;
      FrVarArgEnd() ;
      }
   return ;
}

//----------------------------------------------------------------------

void FrMergeEquivalentClusters(void *clustering_state,
			       const FrClusteringParameters *params)
{
   if (clustering_state)
      {
      FrSymHashTable *clusters = (FrSymHashTable*)clustering_state ;
      if (clusters)
	 (void)clusters->iterate(combine_equiv,clusters,params) ;
      }
   return ;
}

//----------------------------------------------------------------------

void FrEraseClusterList(FrList *cluster_list, bool copied_vectors)
{
   while (cluster_list)
      {
      FrList *cluster = (FrList*)poplist(cluster_list) ;
      if (cluster && cluster->consp())
	 {
	 (void)poplist(cluster) ;	// discard cluster ID
	 if (copied_vectors)
	    {
	    while (cluster)
	       {
	       FrTermVector *tv = (FrTermVector*)poplist(cluster) ;
	       free_termvec(tv) ;
	       }
	    }
	 else
	    cluster->eraseList(false) ;
	 }
      else
	 free_object(cluster) ;
      }
   return ;
}

// end of file frclust.cpp //
