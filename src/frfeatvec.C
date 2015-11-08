/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frfeatvec.cpp		feature vector routines			*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 2010,2015 Ralf Brown/Carnegie Mellon University	*/
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

#include "framerr.h"
#include "frfeatvec.h"
#include "frqsort.h"

/************************************************************************/
/*	Manifest Constants						*/
/************************************************************************/

#define ALLOC_GRANULARITY 256

/************************************************************************/
/*	Global Variables			       			*/
/************************************************************************/

FrAllocator FrFeatureVectorMap::allocator("FrFeatureVecMap",
					  sizeof(FrFeatureVectorMap)) ;

/************************************************************************/
/*	Methods for class FrFeatureVectorMap				*/
/************************************************************************/

FrFeatureVectorMap::FrFeatureVectorMap()
{
   init() ;
   return ;
}

//----------------------------------------------------------------------

FrFeatureVectorMap::FrFeatureVectorMap(const FrFeatureVectorMap &oldmap)
{
   init(&oldmap) ;
   return ;
}

//----------------------------------------------------------------------

FrFeatureVectorMap::FrFeatureVectorMap(const FrFeatureVectorMap *oldmap)
{
   init(oldmap) ;
   return ;
}

//----------------------------------------------------------------------

FrFeatureVectorMap::~FrFeatureVectorMap()
{
   delete m_allocator ;	m_allocator = 0 ;
   for (size_t i = 0 ; i < numFeatures() ; i++)
      FrFree(m_names[i]) ;
   FrFree(m_names) ;
   FrFree(m_sort_order) ;
   delete m_protected ; m_protected = 0 ;
   m_self = 0 ;
   return ;
}

//----------------------------------------------------------------------

void FrFeatureVectorMap::init()
{
   if (m_self != this)
      {
      // we haven't yet been initialized, so it's OK to initialize
      m_allocator = 0 ;
      m_featurecount = 0 ;
      m_alloccount = 0 ;
      m_names = 0 ;
      m_protected = new FrBitVector ;
      m_sort_order = 0 ;
      m_sorted = true ;
      m_self = this ;
      }
   return ;
}

//----------------------------------------------------------------------

void FrFeatureVectorMap::init(const FrFeatureVectorMap *oldmap)
{
   m_self = 0 ;
   init() ;
   if (oldmap && oldmap->numFeatures() > 0)
      {
      m_featurecount = oldmap->numFeatures() ;
      m_alloccount = oldmap->m_alloccount ;
      m_names = FrNewN(char*,m_alloccount) ;
      m_sort_order = FrNewN(unsigned,m_alloccount) ;
      if (m_names && m_sort_order)
	 {
	 // copy the feature names from the old map
	 for (size_t i = 0 ; i < numFeatures() ; i++)
	    {
	    m_names[i] = FrDupString(oldmap->m_names[i]) ;
	    m_sort_order[i] = oldmap->m_sort_order[i] ;
	    }
	 }
      else
	 {
	 FrFree(m_names) ; 	m_names = 0 ;
	 FrFree(m_sort_order) ;	m_sort_order = 0 ;
	 FrNoMemory("while copying feature map") ;
	 m_featurecount = m_alloccount = 0 ;
	 }
      }
   return ;
}

//----------------------------------------------------------------------

size_t FrFeatureVectorMap::newFeature(const char *feature, bool protect)
{
   if (feature && *feature)
      {
      init() ;				// in case called before ctor
      if (m_featurecount >= m_alloccount)
	 {
	 // expand our buffer
	 size_t new_count = m_alloccount + ALLOC_GRANULARITY ;
	 char **new_names = FrNewR(char*,m_names,new_count) ;
	 unsigned *new_order = FrNewR(unsigned,m_sort_order,new_count) ;
	 if (new_names && new_order)
	    {
	    m_names = new_names ;
	    m_sort_order = new_order ;
	    m_alloccount = new_count ;
	    m_protected->expandTo(m_alloccount) ;
	    }
	 else
	    {
	    if (new_names)
	       m_names = new_names ;
	    if (new_order)
	       m_sort_order = new_order ;
	    FrNoMemory("while adding a new feature to a feature vector map") ;
	    return unknown ;
	    }
	 }
      m_names[m_featurecount] = FrDupString(feature) ;
      if (protect)
	 m_protected->setBit(m_featurecount) ;
      m_sorted = false ;
      return m_featurecount++ ;
      }
   return unknown ;
}

//----------------------------------------------------------------------

size_t FrFeatureVectorMap::addFeature(const char *feature, bool protect)
{
   if (feature && *feature)
      {
      init() ;				// in case called before ctor
      size_t index = featureIndex(feature,true) ;
      if (index == unknown)
	 return newFeature(feature,protect) ;
      else
	 return index ;
      }
   return unknown ;
}

//----------------------------------------------------------------------

void FrFeatureVectorMap::finalize(size_t feature_size)
{
   sortNames() ;
   delete m_allocator ;
   m_allocator = new FrAllocator("FrFeatureValues",
				 numFeatures() * feature_size) ;
   return ;
}

//----------------------------------------------------------------------

static FrFeatureVectorMap *current_map = 0 ;

static int compare_names(const void *index1, const void *index2)
{
   return current_map->compare(*((unsigned*)index1), *((unsigned*)index2)) ;
}

//----------------------------------------------------------------------

void FrFeatureVectorMap::sortNames()
{
   if (!m_sorted)
      {
      for (size_t i = 0 ; i < numFeatures() ; i++)
	 m_sort_order[i] = i ;
      current_map = this ;
      qsort(m_sort_order,numFeatures(),sizeof(m_sort_order[0]),compare_names) ;
      current_map = 0 ;
      m_sorted = true ;
      }
   return ;
}

//----------------------------------------------------------------------

int FrFeatureVectorMap::compare(const char *name1, const char *name2) const
{
   return Fr_stricmp(name1,name2) ;
}

//----------------------------------------------------------------------

int FrFeatureVectorMap::compare(unsigned index1, unsigned index2) const
{
   return Fr_stricmp(m_names[index1], m_names[index2]) ;
}

//----------------------------------------------------------------------

size_t FrFeatureVectorMap::featureIndex(const char *feature,
					bool full_scan) const
{
   if (!m_sorted)
      {
      if (full_scan)
	 {
	 for (size_t i = 0 ; i < numFeatures() ; i++)
	    {
	    if (Fr_stricmp(m_names[i],feature) == 0)
	       return i ;
	    }
	 }
      return unknown ;
      }
   if (feature && *feature)
      {
      // perform a binary search on the array of feature names
      size_t lo = 0 ;
      size_t hi = numFeatures() ;
      while (lo < hi)
	 {
	 size_t mid = (hi + lo) / 2 ;
	 int cmp = compare(m_names[m_sort_order[mid]],feature) ;
	 if (cmp < 0)
	    lo = mid + 1 ;
	 else if (cmp > 0)
	    hi = mid ;
	 else
	    return m_sort_order[mid] ;
	 }
      }
   return unknown ;
}

//----------------------------------------------------------------------

size_t FrFeatureVectorMap::featureIndex(const FrSymbol *feature) const
{
   return feature ? featureIndex(feature->symbolName()) : unknown ;
}

//----------------------------------------------------------------------

const char *FrFeatureVectorMap::featureName(size_t N) const
{
   return (N < numFeatures()) ? m_names[N] : 0 ;
}

//----------------------------------------------------------------------

FrList *FrFeatureVectorMap::featureNames() const
{
   FrList *feats ;
   FrList **end = &feats ;

   for (size_t i = 0 ; i < numFeatures() ; i++)
      {
      const char *name = featureName(i) ;
      if (name && *name)
	 feats->pushlistend(new FrString(name),end) ;
      }
   *end = 0 ;		// terminate list
   return feats ;
}

//----------------------------------------------------------------------

FrList *FrFeatureVectorMap::protectedFeatures() const
{
   FrList *feats ;
   FrList **end = &feats ;

   for (size_t i = 0 ; i < numFeatures() ; i++)
      {
      if (!isProtected(i))
	 continue ;
      const char *name = featureName(i) ;
      if (name && *name)
	 feats->pushlistend(new FrString(name),end) ;
      }
   *end = 0 ;		// terminate list
   return feats ;
}

// end of file frfeatvec.cpp //
