/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC								*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File: frclust1.cpp	      k-means term-vector clustering		*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1999,2000,2001,2002,2003,2004,2005,2006,2009,2015	*/
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

#include "frassert.h"
#include "frclust.h"
#include "frclustp.h"
#include "frqsort.h"
#include "frrandom.h"
#include "frtimer.h"

/************************************************************************/
/*	Manifest Constants						*/
/************************************************************************/

#define KMEANS_DOT_INTERVAL     250	// how often to show progress update

/************************************************************************/
/*	Global variables						*/
/************************************************************************/

#ifndef NDEBUG
#  undef _FrCURRENT_FILE
   static const char _FrCURRENT_FILE[] = __FILE__ ;
#endif /* !NDEBUG */

/************************************************************************/
/*	Types for this module						*/
/************************************************************************/

struct VecSim
   {
   public:
      FrTermVector *tv ;
      double score ;
   public: // methods
      // sort in ascending order by score
      static int compare(const VecSim &v1, const VecSim &v2)
	 { if (v1.score < v2.score) return -1 ;
   	    if (v1.score > v2.score) return +1 ;
	    return 0 ;
	 }
      static void swap(VecSim &v1, VecSim &v2)
	 { FrTermVector *tv_tmp = v1.tv ;
   	   double sc_tmp = v1.score ;
	   v1.tv = v2.tv ; v1.score = v2.score ;
	   v2.tv = tv_tmp ; v2.score = sc_tmp ;
	 }
} ;

/************************************************************************/
/*	Diversity-Sampled Vectors					*/
/************************************************************************/

static FrList *sort_by_similarity(FrList *vectors,
				  FrClusteringMeasure measure,
				  FrTermVectorSimilarityFunc *simfn = nullptr,
				  void *simdata = nullptr)
{
   size_t total_vecs = vectors->simplelistlength() ;
   FrLocalAllocC(VecSim,sim,2048,total_vecs+1) ;
   if (!sim)
      {
      FrNoMemory("while sorting vectors by similarity") ;
      vectors->eraseList(false) ;
      return 0 ;
      }
   size_t count = 0 ;
   for (FrList *vlist = vectors ; vlist ; vlist = vlist->rest())
      {
      FrTermVector *tv1 = (FrTermVector*)vlist->first() ;
      // compute the vector's overall similarity to the entire set of vectors
      double score = 0.0 ;
      for (FrList *vlist2 = vectors ; vlist2 ; vlist2 = vlist2->rest())
	 {
	 if (vlist2 != vlist)
	    {
	    FrTermVector *tv2 = (FrTermVector*)vlist2->first() ;
	    double simscore = FrTermVecSimilarity(tv1,tv2,measure,simfn,simdata) ;
	    score += simscore * simscore ;
	    }
	 }
      sim[count].tv = tv1 ;
      sim[count].score = score ;
      count++ ;
      }
   FrQuickSort(sim,count) ;
   vectors->eraseList(false) ;
   FrList *result = nullptr ;
   FrList **end = &result ;
   for (size_t i = 0 ; i < total_vecs ; i++)
      result->pushlistend(sim[i].tv,end) ;
   *end = nullptr ;				// terminate the list
   FrLocalFree(sim) ;
   return result ;
}

//----------------------------------------------------------------------

FrList *FrDiversitySampledVectors(const FrList *vectors, size_t samplesize,
				  FrClusteringMeasure measure,
				  FrTermVectorSimilarityFunc *simfn,
				  void *simdata,
				  bool copy_vectors)
{
   FrList *vecs = vectors ? (FrList*)vectors->copy() : 0 ;
   FrList *sample = FrRandomSample(vecs,samplesize,false,true) ;
   sample = sort_by_similarity(sample,measure,simfn,simdata) ;
   if (copy_vectors && sample)
      {
      FrList *copy = (FrList*)sample->deepcopy() ;
      sample->eraseList(false) ;
      sample = copy ;
      }
   return sample ;
}

/************************************************************************/
/*	Helper functions						*/
/************************************************************************/

bool Fr__extract_centroid(const FrSymbol *, FrObject *cl, va_list args)
{
   FrList *cluster = (FrList*)cl ;
   FrTermVector *centroid = FrCLUSTERCENTROID(cluster) ;
   FrVarArg(FrList **,centroids) ;
   if (centroids)
      pushlist(centroid,*centroids) ;
   return true ;			// continue iterating
}

//----------------------------------------------------------------------

FrSymbol *Fr__nearest_centroid(const FrList *centroids,
			       FrTermVector *tv, FrClusteringMeasure meas,
			       const FrClusteringParameters *params)
{
   FrSymbol *best_cluster = nullptr ;
   double best_sim = -1.0 ;
   for (const FrList *cent = centroids ; cent ; cent = cent->rest())
      {
      FrTermVector *centroid = (FrTermVector*)cent->first() ;
      double sim = FrTermVecSimilarity(centroid,tv,meas,params->tvSimFn(),params->tvSimData()) ;
      FrSymbol *key = centroid->key() ;
      if (sim > best_sim && !conflicting_seed_cluster(centroid,tv,params) &&
	  (!tv->cluster() || tv->cluster() == key))
	 {
	 best_sim = sim ;
	 best_cluster = key ;
	 }
      }
   if (!best_cluster)
      {
      for (const FrList *cent = centroids ; cent ; cent = cent->rest())
	 {
	 FrTermVector *centroid = (FrTermVector*)cent->first() ;
	 double sim = FrTermVecSimilarity(centroid,tv,meas,
					  params->tvSimFn(),params->tvSimData()) ;
	 if (sim > best_sim)
	    {
	    best_sim = sim ;
	    best_cluster = centroid->key() ;
	    }
	 }
      }
   return best_cluster ;
}

//----------------------------------------------------------------------

static bool update_centroid_clear_cluster(const FrSymbol *, FrObject *cl,va_list args)
{
   // remove the elements of the cluster, but retain the centroid
   FrList *cluster = (FrList*)cl ;
   if (!cluster)			// does the cluster actually exist?
      return true ;			// continue iterating
   FrList *members = FrCLUSTERMEMBERS(cluster) ;
   FrTermVector *centroid ;
   if (members)
      {
      FrTermVector *old_centroid = FrCLUSTERCENTROID(cluster) ;
      centroid = Fr__make_centroid(members) ;
      centroid->setCluster(old_centroid->cluster()) ;
      centroid->setKey(old_centroid->key()) ;
      free_object(old_centroid) ;
      cluster->replaca(centroid) ;
      // update count to zero and zap the list of members
      FrList *c2 = cluster->rest() ;
      free_object(c2->first()) ;
      c2->replaca(new FrInteger(0)) ;
      c2->replacd(0) ;
      }
   else
      centroid = FrCLUSTERCENTROID(cluster) ;
   FrVarArg(FrList **,centroids) ;
   FrVarArg(FrList **,old_members) ;
   pushlist(centroid,*centroids) ;
   if (old_members)
      {
#if 0
      for (FrList *m = members ; m ; m = m->rest())
	 {
	 FrTermVector *member = (FrTermVector*)m->first() ;
	 if (member && FrIsGeneratedClusterName(member->cluster()))
	    member->setCluster(0) ;
	 }
#endif /* 0 */
      *old_members = members->nconc(*old_members) ;
      }
   else
      {
      while (members)
	 {
	 FrTermVector *member = (FrTermVector*)poplist(members) ;
	 (void)member;
//	 if (member && FrIsGeneratedClusterName(member->cluster()))
//	    member->setCluster(0) ;
	 }
      }
   return true ;			// continue iterating
}

//----------------------------------------------------------------------

static FrList *get_centroids(const FrList *vectors, FrSymHashTable *clusters,
			     const FrClusteringParameters *params)
{
   // build initial clusters from any vectors which have already been tagged
   //   with a cluster ID
   FrList *centroids = nullptr ;
   for (const FrList *veclist = vectors ; veclist ; veclist = veclist->rest())
      {
      FrTermVector *tv = (FrTermVector*)veclist->first() ;
      if (tv && tv->cluster())
	 Fr__add_to_cluster(clusters,tv->cluster(),tv,
			    params->clusterRep(),params->cacheSize());
      }
   if (clusters->currentSize() > 0)
      {
      // extract the centroids from the existing clusters, and clear the
      //   associated lists of cluster members
      clusters->iterate(update_centroid_clear_cluster,&centroids,0) ;
      }
   return centroids ;
}

//----------------------------------------------------------------------

static void cluster_kmeans(const FrList *vectors, FrSymHashTable *clusters,
			   const FrClusteringParameters *params,
			   FrList *centroids,
			   bool run_verbosely, bool have_prior_clusters)
{
   assertq(clusters != nullptr) ;
   FrTimer timer ;
   size_t wanted = params->desiredClusters() ;
   if (clusters->currentSize() < wanted)
      {
      cout << ";   found only " << clusters->currentSize()
	   << " centroids, wanted " << wanted << endl ;
      wanted = clusters->currentSize() ;
      }
   // now that we have the initial cluster centroids, repeat 'maxIterations'
   //   times or until convergence:
   //      1. for each vector, assign it to the nearest centroid
   //      2. recompute the centroids based on the vectors assigned to each
   size_t max_iter = params->maxIterations() ;
   if (max_iter < 2)
      {
      // it doesn't make sense to run <2 iterations unless this is second call
      max_iter = have_prior_clusters ? 1 : 2 ;
      }
   FrClusteringMeasure measure = params->measure() ;
   if (run_verbosely)
      cout << ";  " << flush ;
   bool changed = true ;
   size_t iter ;
   size_t count = 0 ;
   FrList *all_vectors = nullptr ;
   for (iter = 0 ; iter < max_iter && changed ; iter++)
      {
      FrList *old_vecs = nullptr ;
      if (iter != 0 || have_prior_clusters)
	 {
	 // clear out the cluster-assignment results of the previous iteration
	 centroids->eraseList(false) ;
	 centroids = nullptr ;
	 clusters->iterate(update_centroid_clear_cluster,&centroids,
			   have_prior_clusters ? &old_vecs : 0) ;
	 have_prior_clusters = false ;
	 if (clusters->currentSize() < wanted)
	    cout << "\n;   now have " << clusters->currentSize()
		 << " centroids\n;   " << flush ;
	 if (clusters->currentSize() != centroids->simplelistlength())
	    cout << "OOPS: lost centroids somewhere!\n;   " << flush ;
	 }
      if (old_vecs)
	 {
	 all_vectors = vectors ? old_vecs->nconc((FrList*)vectors) : old_vecs ;
	 if (run_verbosely)
	    cout << "\n;   total of " << all_vectors->simplelistlength()
		 << " vectors to reassign to clusters" << endl << ";   "
		 << flush ;
	 }
      else if (!all_vectors)
	 all_vectors = (FrList*)vectors ;
      old_vecs = nullptr ;
      changed = false ;
      for (const FrList *vecs = all_vectors ; vecs ; vecs = vecs->rest())
	 {
	 FrTermVector *tv = (FrTermVector*)vecs->first() ;
	 // assign the term vector to the cluster with the nearest centroid
	 FrSymbol *newcluster = Fr__nearest_centroid(centroids,tv,measure,
						     params) ;
	 if (tv->cluster() != newcluster)
	    changed = true ;
	 Fr__insert_in_cluster(clusters,newcluster,tv,params) ;
	 if (run_verbosely && (++count % KMEANS_DOT_INTERVAL) == 0)
	    {
	    if (count % (75 * KMEANS_DOT_INTERVAL) == 0)
	       cout << "\n;   " << flush ;
	    cout << '.' << flush ;
	    }
	 }
      if (run_verbosely)
	 {
	 if (count % (75 * KMEANS_DOT_INTERVAL) >
	     (count+KMEANS_DOT_INTERVAL) % (75 * KMEANS_DOT_INTERVAL))
	    cout << "\n;   " << flush ;
	 cout << ':' << flush ;
	 count += KMEANS_DOT_INTERVAL ;
	 }
      }
   if (run_verbosely)
      {
      cout << "\n;   " ;
      if (changed)
	 cout << "terminated" ;
      else
	 cout << "converged" ;
      cout << " after " << iter << " iterations" << endl ;
      }
   // merge seeded vectors that wound up in different clusters
//   FrMergeEquivalentClusters(clusters,params) ;
   if (run_verbosely)
      cout << ";     (clustering took " << timer.readsec() << " seconds)"
	   << endl ;
   centroids->eraseList(false) ;
   while (all_vectors && all_vectors != vectors)
      {
      (void)poplist(all_vectors) ;
      }
   return ;
}

//----------------------------------------------------------------------

static int compare_cluster_size(const FrObject *o1, const FrObject *o2)
{
   const FrList *clus1 = (const FrList*)o1 ;
   const FrList *clus2 = (const FrList*)o2 ;
   size_t size1 = clus1->simplelistlength() ;
   size_t size2 = clus2->simplelistlength() ;
   if (size1 > size2)
      return -1 ;
   else if (size1 < size2)
      return +1 ;
   return 0 ;
}

/************************************************************************/
/************************************************************************/

void Fr__cluster_kmeans(const FrList *vectors, FrSymHashTable *clusters,
			const FrClusteringParameters *params,
			bool run_verbosely)
{
   FrTimer timer ;
   Fr__set_cluster_caching(0) ;
   bool have_prior_clusters = (clusters->currentSize() > 0) ;
   FrList *centroids = get_centroids(vectors,clusters,params) ;
   size_t wanted = params->desiredClusters() ;
   if (clusters->currentSize() < wanted)
      {
      if (run_verbosely)
	 cout << ";   " << clusters->currentSize() << " clusters given, "
	      << wanted << " desired" << endl ;
      size_t numvecs = vectors->simplelistlength() ;
      size_t needed = wanted - clusters->currentSize() ;
      // build initial centroids by randomly selecting approx 2*wanted vectors
      //   and then choosing the ones which are least similar to each other
      size_t samplesize = needed ;
      if (needed < numvecs/16)
	 samplesize *= 3 ;
      else if (needed <= numvecs/8)
	 samplesize *= 2 ;
      else if (needed <= numvecs/4)
	 samplesize += (samplesize / 2) ;
      else if (needed + 100 < numvecs)
	 samplesize += 50 ;
      if (run_verbosely && needed > 0)
	 cout << ";   selecting "<< needed << " initial centroids by sampling "
	      << samplesize << " vectors from " << numvecs << endl ;
      FrList *sample = FrDiversitySampledVectors(vectors,samplesize,
						 params->measure(),
						 params->tvSimFn()) ;
      while (sample && clusters->currentSize() < wanted)
	 {
	 FrTermVector *tv = (FrTermVector*)poplist(sample) ;
	 if (!tv)
	    {
	    cout << ";   oops, got a null term vector!" << endl ;
	    continue ;
	    }
	 FrTermVector *centroid = new FrTermVector(*tv) ;
	 FrSymbol *clustername = gen_cluster_sym() ;
	 centroid->setKey(tv->key()) ;
	 centroid->setCluster(clustername) ;
	 if (centroid->vectorFreq() == 0) centroid->setFreq(1) ;
	 clusters->add(clustername,new FrList(centroid,new FrInteger(0)));
	 pushlist(centroid,centroids) ;
	 }
      sample->eraseList(false) ;
      if (run_verbosely)
	 cout << ";   (" << timer.readsec() << " sec to select "
	      << clusters->currentSize() << " initial clusters)"
	      << endl ;
      }
   cluster_kmeans(vectors,clusters,params,centroids,run_verbosely,
		  have_prior_clusters) ;
   return ;
}

//----------------------------------------------------------------------

void Fr__cluster_kmeans_hclust(const FrList *vectors, FrSymHashTable *clusters,
			       const FrClusteringParameters *params,
			       bool run_verbosely)
{
   // select initial centroids by performing a bottom-up agglomerative
   //   clustering to 3 * 'desiredClusters' clusters and picking the
   //   'desiredClusters' largest clusters that result
   FrTimer timer ;
   Fr__set_cluster_caching(0) ;
   bool have_prior_clusters = (clusters->currentSize() > 0) ;
   FrList *centroids = get_centroids(vectors,clusters,params) ;
   size_t wanted = params->desiredClusters() ;
   if (clusters->currentSize() < wanted)
      {
      if (run_verbosely)
	 cout << ";   " << clusters->currentSize() << " clusters given, "
	      << wanted << " desired" << endl ;
      FrSymHashTable seedclusters ;
      FrClusteringParameters seedparams(FrCM_AGGLOMERATIVE,FrCR_NEAREST,
					params->measure(),1E-9) ;
      seedparams.desiredClusters(3*wanted) ;
      seedparams.tvUserSimFunc(params->tvSimFn(),params->tvSimData()) ;
      seedparams.userSimFunc(params->simFn()) ;
      FrList *clus = FrClusterVectors(vectors,&seedparams,true,run_verbosely) ;
      // sort clusters by size and pick off the 'wanted' largest
      clus = clus->sort(compare_cluster_size) ;
      FrList *c = clus ;
      for (size_t i = 0 ; clusters->currentSize() < wanted && c ; i++)
	 {
	 const FrList *chead = (FrList*)c->first() ;
	 const FrList *cluster = chead->rest() ;
	 FrSymbol *clusname = (FrSymbol*)(chead->first()) ;
	 FrTermVector *centroid = Fr__make_centroid(cluster) ;
	 FrObject *cl ;
	 if (clusters->lookup(clusname,&cl,true))
	    {
	    Fr__free_cluster(clusname,cl) ;
	    }
	 FrList *newcluster = new FrList(centroid, new FrInteger(0)) ;
	 clusters->add(clusname,newcluster) ;
	 centroid->setKey(clusname) ;
	 if (!centroid->cluster())
	    centroid->setCluster(clusname) ;
	 if (centroid->vectorFreq() == 0) centroid->setFreq(1) ;
	 pushlist(centroid,centroids) ;
	 c = c->rest() ;
	 }
      FrEraseClusterList(clus) ;
      // clear out the cluster assignments that got added to the term vectors
      for (const FrList *vec = vectors ; vec ; vec = vec->rest())
	 {
	 FrTermVector *vector = (FrTermVector*)vec->first() ;
	 if (vector)
	    vector->setCluster(0) ;
	 }
      }
   cluster_kmeans(vectors,clusters,params,centroids,run_verbosely,
		  have_prior_clusters) ;
   return ;
}

// end of file frclust1.cpp //
