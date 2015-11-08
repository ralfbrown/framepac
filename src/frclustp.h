/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC								*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File: frclustp.h	      term-vector clustering privates		*/
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

#include "frnumber.h"

/************************************************************************/
/*	Macros and Manifest Constants					*/
/************************************************************************/

#define gen_cluster_sym() \
	gensym(FrGENERATED_CLUSTER_PREFIX,FrGENERATED_CLUSTER_SUFFIX)

/************************************************************************/
/*	Private API, subject to change!					*/
/************************************************************************/

inline FrTermVector *FrCLUSTERCENTROID(const FrList *cluster)
{
   return (FrTermVector*)cluster->first() ;
}

//----------------------------------------------------------------------

#define FrCLUSTERMEMBERS(cluster) ((cluster)->rest()->rest())

//----------------------------------------------------------------------

inline void ADDCLUSTERMEMBER(FrList *cluster, FrTermVector *mem)
{
   FrList *c2 = FrCLUSTERMEMBERS(cluster) ;
   pushlist(mem,c2) ;
   cluster->rest()->replacd(c2) ;
   return ;
}

//----------------------------------------------------------------------

inline void INCRCLUSTERSIZE(FrList *cluster, size_t increment = 1)
{
   FrList *c2 = cluster->rest() ;
   FrObject *clus_size = c2->first() ;
   size_t size = (size_t)clus_size->intValue() ;
   size += increment ;
   free_object(clus_size) ;
   c2->replaca(new FrInteger(size)) ;
   return ;
}

//----------------------------------------------------------------------

inline bool conflicting_seed_cluster(const FrTermVector *tv1,
				       const FrTermVector *tv2,
				       const FrClusteringParameters *params)
{
   FrSymbol *cluster1 = tv1->cluster() ;
   FrSymbol *cluster2 = tv2->cluster() ;
   if (cluster1 && cluster2 && cluster1 != cluster2)
      return True ;
   return params->conflict(tv1,tv2) ;
}

/************************************************************************/
/************************************************************************/

FrSymbol *Fr__nearest_centroid(const FrList *centroids,
			       FrTermVector *tv, FrClusteringMeasure meas,
			       const FrClusteringParameters *params) ;
bool Fr__extract_centroid(const FrSymbol *, FrObject *cl, va_list args) ;

void Fr__set_cluster_caching(size_t cache_size) ;

void Fr__new_cluster(FrSymHashTable *clusters, FrSymbol *classname,
		     FrSymbol *clustername, const FrTermVector *tv,
		     FrClusteringRep rep, size_t cache_size) ;
#define new_cluster Fr__new_cluster

void Fr__add_to_cluster(FrSymHashTable *clusters,
			FrSymbol *classname, FrTermVector *tv,
			FrClusteringRep rep, size_t cache_size) ;
void Fr__insert_in_cluster(FrSymHashTable *clusters, FrSymbol *newcluster,
			   FrTermVector *tv,
			   const FrClusteringParameters *params) ;

bool Fr__copy_cluster(FrSymHashTable *clusters, FrSymbol *classname,
			const FrList *clus) ;

bool Fr__free_cluster(const FrSymbol *term, FrObject *cl) ;

FrTermVector *Fr__make_centroid(const FrList *tvlist) ;

double Fr__cluster_similarity(const FrList *cluster1,
			      const FrList *cluster2,
			      const FrClusteringParameters *params,
			      // allow a user-similarity function to short-circuit its eval if the score
			      //   will be less than the best so far
			      double best_sim = -1.0) ;

void Fr__cluster_kmeans(const FrList *vectors, FrSymHashTable *clusters,
			const FrClusteringParameters *params,
			bool run_verbosely = false) ;

void Fr__cluster_kmeans_hclust(const FrList *vectors, FrSymHashTable *clusters,
			       const FrClusteringParameters *params,
			       bool run_verbosely) ;

void Fr__cluster_tight(const FrList *vectors, FrSymHashTable *clusters,
		       const FrClusteringParameters *params,
		       bool run_verbosely) ;

void Fr__cluster_growseed(const FrList *vectors, FrSymHashTable *clusters,
			  const FrClusteringParameters *params,
			  bool run_verbosely) ;

void Fr__cluster_spectral(const FrList *vectors, FrSymHashTable *clusters,
			  const FrClusteringParameters *params,
			  bool run_verbosely = false) ;

// end of file frclustp.h //
