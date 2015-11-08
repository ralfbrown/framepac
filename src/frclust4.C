/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC								*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File: frclust4.cpp	      Seed-Growing main code			*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 2005,2009,2015 Ralf Brown/Carnegie Mellon University	*/
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

/************************************************************************/
/*	Types for this class						*/
/************************************************************************/

class Fr_GrowSeedsInfo
   {
   public:
      FrTermVector *vector ;
      FrSymHashTable *clusters ;
      const FrList *seeds ;
      const FrClusteringParameters *params ;
      size_t id ;
   } ;

/************************************************************************/
/*	Global variables						*/
/************************************************************************/

static FrMutex grow_mutex ;

/************************************************************************/
/************************************************************************/

static FrTermVector *nearest_seed(FrTermVector *vector, const FrList *seeds,
				  const FrClusteringParameters *params)
{
   FrTermVector *cluster = nullptr ;
   double best_sim = -999.9 ;
   double threshold = params->threshold(0) ;
   FrClusteringMeasure measure = params->measure() ;
   for ( ; seeds ; seeds = seeds->rest())
      {
      FrTermVector *seed = (FrTermVector*)seeds->first() ;
      double sim = FrTermVecSimilarity(vector,seed,measure,params->tvSimFn(),params->tvSimData()) ;
      if (sim >= threshold && sim > best_sim)
	 {
	 best_sim = sim ;
	 cluster = seed ;
	 }
      }
#if 0
   if (cluster)
      {
      if (strchr(vector->key()->symbolName(),' '))
	 cerr<<"nearest neighbor of "<<vector->key()<<" is "<<cluster->key()<<" @ "<<best_sim<<endl;
      }
#endif
   return cluster ;
}

//----------------------------------------------------------------------

static void grow_clusters_parallel(const void *input, void * /*output*/ )
{
   const Fr_GrowSeedsInfo *order = static_cast<const Fr_GrowSeedsInfo*>(input) ;
   if (!order)
      return ;
   FrTermVector *vector = order->vector ;
   if (vector->cluster())
      return ;				// already classified,so we're done
   FrSymHashTable *clusters = order->clusters ;
   const FrList *seeds = order->seeds ;
   const FrClusteringParameters *params = order->params ;
   FrTermVector *cluster = nearest_seed(vector,seeds,params) ;
   FrSymbol *clustername = cluster ? cluster->cluster() : 0 ;
   vector->setCluster(clustername) ;
   // Fr_add_to_cluster isn't thread safe, so serialize access
   grow_mutex.lock() ;
   if (clustername)
      {
      // request centroid for a new cluster, nearest for existing cluster
      //   so that the "centroid" is in fact the first (seed) vector
      FrClusteringRep rep = (clusters->contains(clustername)
			     ? FrCR_NEAREST
			     : FrCR_CENTROID) ;
      Fr__add_to_cluster(clusters,clustername,vector,rep,
			 params->cacheSize()) ;
      }
   else if (0)
      {
      Fr__add_to_cluster(clusters,clustername,vector,FrCR_NEAREST,
			 params->cacheSize()) ;
      }
   grow_mutex.unlock() ;
   return ;
}

//----------------------------------------------------------------------

/* Grow-Seed clustering main function */
void Fr__cluster_growseed(const FrList *vectors, FrSymHashTable *clusters,
			  const FrClusteringParameters *params,
			  bool run_verbosely)
{
   FrList *seeds = nullptr ;
   size_t num_vectors = 0 ;
   for (const FrList *v = vectors ; v ; v = v->rest())
      {
      ++num_vectors ;
      FrTermVector *vector = (FrTermVector*)v->first() ;
      FrSymbol *clustername = vector->cluster() ;
      if (clustername)
	 {
	 pushlist(vector,seeds) ;
	 // request centroid for a new cluster, nearest for existing cluster
	 //   so that the "centroid" is in fact the first (seed) vector
	 FrClusteringRep rep = (clusters->contains(clustername)
				? FrCR_NEAREST
				: FrCR_CENTROID) ;
	 Fr__add_to_cluster(clusters,clustername,vector,rep,
			    params->cacheSize()) ;
	 }
      }
   if (run_verbosely)
      {
      cout << ";    " << seeds->listlength() << " seed vectors" << endl ;
      cout << ";    " << flush ;
      }
   FrThreadPool tpool(params->numThreads()) ;
   bool must_wait = (params->numThreads() != 0) ;
   Fr_GrowSeedsInfo *workorders = new Fr_GrowSeedsInfo[num_vectors] ;
   size_t count = 0 ;
   for ( ; vectors ; vectors = vectors->rest())
      {
      FrTermVector *vector = static_cast<FrTermVector*>(vectors->first()) ;
      workorders[count].vector = vector ;
      workorders[count].clusters = clusters ;
      workorders[count].seeds = seeds ;
      workorders[count].params = params ;
      workorders[count].id = count ;
      tpool.dispatch(grow_clusters_parallel,&workorders[count],0) ;
      count++ ;
      if (run_verbosely && count % 1000 == 0)
	 {
	 if (must_wait)
	    {
	    tpool.waitUntilIdle() ;
	    }
	 cout << "." << flush ;
	 if (count % 50000 == 0)
	    {
	    cout << "\n;   " << ((count/50000)%10) << flush ;
	    }
	 }
      }
   if (must_wait)
      {
      tpool.waitUntilIdle() ;
      }
   delete [] workorders ;
   if (run_verbosely)
      {
      cout << endl ;
      }
   seeds->eraseList(false) ;		// shallow erase, don't kill vectors
   return ;
}

// end of file frclust4.cpp //
