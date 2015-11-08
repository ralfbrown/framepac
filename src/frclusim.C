/************************************************************************/
/*									*/
/*  FramepaC								*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File: frclusim.cpp	      term-vector clustering -- similarity fns	*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1999,2000,2001,2002,2003,2004,2005,2009,2015		*/
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
#include "frnumber.h"
#include <math.h>

/************************************************************************/
/*	Macros and Manifest Constants					*/
/************************************************************************/

/************************************************************************/
/*	Global variables						*/
/************************************************************************/

#ifndef NDEBUG
#  undef _FrCURRENT_FILE
   static const char _FrCURRENT_FILE[] = __FILE__ ;
#endif /* !NDEBUG */

/************************************************************************/
/*	Helper Functions						*/
/************************************************************************/

FrTermVector *Fr__make_centroid(const FrList *tvlist)
{
   FrTermVector *centroid = new FrTermVector ;
   for ( ; tvlist ; tvlist = tvlist->rest())
      centroid->mergeIn((FrTermVector*)tvlist->first()) ;
   return centroid ;
}

/************************************************************************/
/*	Similarity Metrics						*/
/************************************************************************/

static double nearest_similarity(const FrTermVector *tv, const FrList *cluster,
				 FrSymbol *&classname, size_t &min_freq,
				 FrClusteringMeasure sim_measure,
				 FrTermVectorSimilarityFunc *sim_fn = nullptr,
				 void *sim_data = nullptr)
{
   // compute maximum of the similarity scores between the current vector
   //   and all members of the cluster
   double sim = 0.0 ;
   min_freq = ~0 ;
   // iterate over the members of the cluster
   for (const FrList *cl = cluster ; cl ; cl = cl->rest())
      {
      FrTermVector *vec = (FrTermVector*)cl->first() ;
      if (!vec)
	 continue ;
      // skip empty vectors
      if (vec->numTerms() != 0)
	 {
	 double s = FrTermVecSimilarity(vec,tv,sim_measure,sim_fn,sim_data) ;
	 size_t freq = vec->vectorFreq() ;
	 if (s > sim)
	    sim = s ;
	 if (freq < min_freq)
	    min_freq = freq ;
	 }
      if (!classname)
	 classname = vec->cluster() ;
      }
   return sim ;
}

//----------------------------------------------------------------------

static double nearest_similarity(const FrTermVector *tv, const FrList *cluster,
				 FrClusteringMeasure sim_measure,
				 FrTermVectorSimilarityFunc *sim_fn,
				 void *sim_data = nullptr)
{
   // compute maximum of the similarity scores between the current vector
   //   and all members of the cluster
   double sim = 0.0 ;
   // iterate over the members of the cluster
   for (const FrList *cl = cluster ; cl ; cl = cl->rest())
      {
      FrTermVector *vec = (FrTermVector*)cl->first() ;
      // skip empty vectors
      if (vec && vec->numTerms() != 0)
	 {
	 double s = FrTermVecSimilarity(vec,tv,sim_measure,sim_fn,sim_data) ;
	 if (s > sim)
	    sim = s ;
	 }
      }
   return sim ;
}

//----------------------------------------------------------------------

static double furthest_similarity(const FrTermVector *tv,const FrList *cluster,
				  FrSymbol *&classname, size_t &min_freq,
				  FrClusteringMeasure sim_measure,
				  FrTermVectorSimilarityFunc *sim_fn,
				  void *sim_data = nullptr)
{
   // compute maximum of the similarity scores between the current vector
   //   and all members of the cluster
   double sim = 1.0 ;
   min_freq = ~0 ;
   // iterate over the members of the cluster
   for (const FrList *cl = cluster ; cl ; cl = cl->rest())
      {
      FrTermVector *vec = (FrTermVector*)cl->first() ;
      if (!vec)
	 continue ;
      // skip empty vectors
      if (vec->numTerms() != 0)
	 {
	 double s = FrTermVecSimilarity(vec,tv,sim_measure,sim_fn,sim_data) ;
	 size_t freq = vec->vectorFreq() ;
	 if (s < sim)
	    sim = s ;
	 if (freq < min_freq)
	    min_freq = freq ;
	 }
      if (!classname)
	 classname = vec->cluster() ;
      }
   return sim ;
}

//----------------------------------------------------------------------

static double furthest_similarity(const FrTermVector *tv,const FrList *cluster,
				  FrClusteringMeasure sim_measure,
				  FrTermVectorSimilarityFunc *sim_fn,
				  void *sim_data = nullptr)
{
   // compute minimum of the similarity scores between the current vector
   //   and all members of the cluster
   double sim = 1.0 ;
   // iterate over the members of the cluster
   for (const FrList *cl = cluster ; cl ; cl = cl->rest())
      {
      FrTermVector *vec = (FrTermVector*)cl->first() ;
      // skip empty vectors
      if (vec && vec->numTerms() != 0)
	 {
	 double s = FrTermVecSimilarity(vec,tv,sim_measure,sim_fn,sim_data) ;
	 if (s < sim)
	    sim = s ;
	 }
      }
   return sim ;
}

//----------------------------------------------------------------------

static double average_similarity(const FrTermVector *tv, const FrList *cluster,
				 FrSymbol *&classname, size_t &min_freq,
				 FrClusteringMeasure measure,
				 FrTermVectorSimilarityFunc *sim_fn,
				 void *sim_data,
				 bool use_RMS)
{
   // compute average of the similarity scores between the current vector
   //   and all members of the cluster
   double sim = 0.0 ;
   size_t count = 0 ;
   min_freq = ~0 ;
   // iterate over the members of the cluster
   for (const FrList *cl = cluster ; cl ; cl = cl->rest())
      {
      FrTermVector *vec = (FrTermVector*)cl->first() ;
      // skip empty vectors
      if (vec && vec->numTerms() != 0)
	 {
	 double s = FrTermVecSimilarity(vec,tv,measure,sim_fn,sim_data) ;
	 count++ ;
	 if (use_RMS)
	    s *= s ;
	 if (vec->vectorFreq() < min_freq)
	    min_freq = vec->vectorFreq() ;
	 sim += s ;
	 }
      if (vec && !classname)
	 classname = vec->cluster() ;
      }
   if (count)
      {
      sim /= count ;
      if (use_RMS)
	 sim = sqrt(sim) ;
      }
   return sim ;
}

//----------------------------------------------------------------------

static double average_similarity(const FrTermVector *tv, const FrList *cluster,
				 FrClusteringMeasure measure,
				 FrTermVectorSimilarityFunc *sim_fn,
				 void *sim_data,
				 bool use_RMS)
{
   // compute average of the similarity scores between the current vector
   //   and all members of the cluster
   double sim = 0.0 ;
   size_t count = 0 ;
   // iterate over the members of the cluster
   for (const FrList *cl = cluster ; cl ; cl = cl->rest())
      {
      FrTermVector *vec = (FrTermVector*)cl->first() ;
      // skip empty vectors
      if (vec && vec->numTerms() != 0)
	 {
	 double s = FrTermVecSimilarity(vec,tv,measure,sim_fn,sim_data) ;
	 count++ ;
	 if (use_RMS)
	    s *= s ;
	 sim += s ;
	 }
      }
   if (count)
      {
      sim /= count ;
      if (use_RMS)
	 sim = sqrt(sim) ;
      }
   return sim ;
}

//----------------------------------------------------------------------

static double by_member_similarity(const FrList *cluster1,
				   const FrList *cluster2,
				   FrClusteringRep rep,
				   FrClusteringMeasure sim_measure,
				   FrTermVectorSimilarityFunc *sim_fn,
				   void *sim_data)
{
   double sim = -1.0 ;
   if (cluster1 && cluster2)
      {
      size_t count = 0 ;
      sim = 0.0 ;
      size_t clust1size = cluster1->simplelistlength() ;
      size_t clust2size = cluster2->simplelistlength() ;
      if (rep == FrCR_NEAREST)
	 {
	 for ( ; cluster2 ; cluster2=cluster2->rest())
	    {
	    FrTermVector *tv = (FrTermVector*)cluster2->first() ;
	    double c = nearest_similarity(tv,cluster1,sim_measure,sim_fn,sim_data) ;
	    if (c > sim)
	       sim = c ;
	    }
	 }
      else if (rep == FrCR_RMS)
	 {
	 for ( ; cluster2 ; cluster2=cluster2->rest())
	    {
	    FrTermVector *tv = (FrTermVector*)cluster2->first() ;
	    sim += average_similarity(tv,cluster1,sim_measure,sim_fn,sim_data,true) ;
	    count++ ;
	    }
	 }
      else if (rep == FrCR_AVERAGE)
	 {
	 for ( ; cluster2 ; cluster2=cluster2->rest())
	    {
	    FrTermVector *tv = (FrTermVector*)cluster2->first() ;
	    sim += average_similarity(tv,cluster1,sim_measure,sim_fn,sim_data,false) ;
	    count++ ;
	    }
	 }
      else if (rep == FrCR_FURTHEST)
	 {
	 for ( ; cluster2 ; cluster2=cluster2->rest())
	    {
	    FrTermVector *tv = (FrTermVector*)cluster2->first() ;
	    double c = furthest_similarity(tv,cluster1,sim_measure,sim_fn,sim_data) ;
	    if (c > sim)
	       sim = c ;
	    }
	 }
      if (count)
	 sim /= count ;
      if (rep == FrCR_NEAREST || rep == FrCR_FURTHEST)
	 {
	 if (clust2size > clust1size)
	    sim /= ::sqrt(clust2size) ;
	 else
	    sim /= ::sqrt(clust1size) ;
	 }
      }
   return sim ;
}

//----------------------------------------------------------------------

double Fr__cluster_similarity(const FrList *cluster1,
			      const FrList *cluster2,
			      const FrClusteringParameters *params,
			      double best_sim)
{
   FrClusteringMeasure sim_measure = params->measure() ;
   FrClusteringRep rep = params->clusterRep() ;
   FrTermVector *centroid1 = FrCLUSTERCENTROID(cluster1) ;
   FrTermVector *centroid2 = FrCLUSTERCENTROID(cluster2) ;
   assertq(centroid1 && centroid2) ;
   if (sim_measure == FrCM_USER && !params->tvSimFn())
      {
      double sim = -1.0 ;
      // call user function
      FrClusteringSimilarityFunc *sim_func = params->simFn() ;
      size_t min_freq ;
      if (sim_func)
	 sim = sim_func(0,cluster1,cluster2,best_sim,params->simData(),min_freq) ;
      else
	 FrProgError("clustering metric FrCM_USER was specified, but no user\n"
		     "function was provided!") ;
      return sim ;
      }
   else if (rep == FrCR_CENTROID)
      return FrTermVecSimilarity(centroid1,centroid2,sim_measure,
				 params->tvSimFn(),params->tvSimData()) ;
   else
      return by_member_similarity(FrCLUSTERMEMBERS(cluster1),
				  FrCLUSTERMEMBERS(cluster2),
				  rep,sim_measure,params->tvSimFn(),
				  params->tvSimData()) ;
}

//----------------------------------------------------------------------

double FrClusterSimilarity(const FrTermVector *wordvec,
			   const FrList *cluster,
			   const FrClusteringParameters *params,
			   FrSymbol *&classname, size_t &min_freq,
			   double best_sim)
{
   FrClusteringMeasure measure = params->measure() ;
   FrClusteringRep rep = params->clusterRep() ;
   FrTermVector *centroid = FrCLUSTERCENTROID(cluster) ;
   FrList *members = FrCLUSTERMEMBERS(cluster) ;
   double sim = -1.0 ;
   min_freq = ~0 ;
   assertq(centroid != 0) ;
   if (rep == FrCR_CENTROID &&
       (measure != FrCM_USER || params->tvSimFn()))
      {
      sim = FrTermVecSimilarity(centroid,wordvec,measure,params->tvSimFn(),params->tvSimData()) ;
      classname = centroid->cluster() ;
      min_freq = centroid->vectorFreq() ;
      }
   else // if (members)
      {
      if (measure == FrCM_USER && !params->tvSimFn())
	 {
	 // call user function
	 FrClusteringSimilarityFunc *sim_func = params->simFn() ;
	 if (sim_func)
	    {
	    sim = sim_func(wordvec,0,cluster,best_sim,params->simData(),min_freq) ;
	    classname = centroid->cluster() ;
	    min_freq = centroid->vectorFreq() ;
	    }
	 else
	    FrProgError("specified clustering metric FrCM_USER without a\n"
			"similarity function!") ;
	 }
      else if (rep == FrCR_NEAREST)
	 sim = nearest_similarity(wordvec,members,classname,min_freq,measure,
				  params->tvSimFn(),params->tvSimData()) ;
      else if (rep == FrCR_RMS)
	 sim = average_similarity(wordvec,members,classname,min_freq,measure,
				  params->tvSimFn(),params->tvSimData(),true) ;
      else if (rep == FrCR_AVERAGE)
	 sim = average_similarity(wordvec,members,classname,min_freq,measure,
				  params->tvSimFn(),params->tvSimData(),false) ;
      else if (rep == FrCR_FURTHEST)
	 sim = furthest_similarity(wordvec,members,classname,min_freq,measure,
				   params->tvSimFn(),params->tvSimData());
      else
	 FrMissedCase("cluster_similarity") ;
      }
   return sim ;
}

//----------------------------------------------------------------------
// the user-accessible version of the similarity-scoring function.
// This function takes cluster lists as returned by FrClusterVector,
//   i.e. a cluster ID followed by the term vectors for the members of
//   the cluster

double FrClusterSimilarity(const FrList *cluster1,const FrList *cluster2,
			   const FrClusteringParameters *params,
			   double best_sim)
{
   cluster1 = cluster1->rest() ;	// skip cluster ID
   cluster2 = cluster2->rest() ;	// skip cluster ID
   double sim = -1.0 ;
   FrClusteringMeasure sim_measure = params->measure() ;
   FrClusteringRep rep = params->clusterRep() ;
   if (sim_measure == FrCM_USER && !params->tvSimFn())
      {
      FrTermVector *centroid1 = Fr__make_centroid(cluster1) ;
      FrTermVector *centroid2 = Fr__make_centroid(cluster2) ;
      assertq(centroid1 && centroid2) ;
      FrList *cl1 = (FrList*)cluster1 ;
      pushlist(new FrInteger(cluster1->simplelistlength()),cl1) ;
      pushlist(centroid1,cl1) ;
      FrList *cl2 = (FrList*)cluster2 ;
      pushlist(new FrInteger(cluster2->simplelistlength()),cl2) ;
      pushlist(centroid2,cl2) ;
      // call user function
      FrClusteringSimilarityFunc *sim_func = params->simFn() ;
      size_t min_freq ;
      if (sim_func)
	 {
	 sim = sim_func(0,cl1,cl2,best_sim,params->simData(),min_freq) ;
	 }
      else
	 FrProgError("clustering metric FrCM_USER was specified, but no user\n"
		     "function was provided!") ;
      free_object(centroid1) ;
      free_object(centroid2) ;
      (void)poplist(cl1) ;
      (void)poplist(cl1) ;
      (void)poplist(cl2) ;
      (void)poplist(cl2) ;
      }
   else if (rep == FrCR_CENTROID)
      {
      FrTermVector *centroid1 = Fr__make_centroid(cluster1) ;
      FrTermVector *centroid2 = Fr__make_centroid(cluster2) ;
      assertq(centroid1 && centroid2) ;
      sim = FrTermVecSimilarity(centroid1,centroid2,sim_measure,
				params->tvSimFn(),params->tvSimData()) ;
      free_object(centroid1) ;
      free_object(centroid2) ;
      }
   else
      {
      return by_member_similarity(cluster1,cluster2,rep,sim_measure,
				  params->tvSimFn(),params->tvSimData()) ;
      }
   return sim ;
}

// end of file frclusim.cpp //
