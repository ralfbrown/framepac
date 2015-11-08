/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frfeatvec.h		feature vector routines			*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 2010,2012,2015 Ralf Brown/Carnegie Mellon University	*/
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

#ifndef __FRFEATVEC_H_INCLUDED
#define __FRFEATVEC_H_INCLUDED

#include <cmath>
#include <values.h>
#include "frbitvec.h"
#include "frlist.h"
#include "frstring.h"
#include "frsymbol.h"
#include "frutil.h"

/************************************************************************/
/************************************************************************/

class FrFeatureVectorMap
   {
   private:
      static FrAllocator allocator ;
      FrAllocator *m_allocator ;	// allocate FrFeatureVector::m_values
      size_t       m_featurecount ;	// number of features defined
      size_t       m_alloccount ;	// # of features for which space alloc
      char       **m_names ;
      unsigned    *m_sort_order ;
      FrBitVector *m_protected ;
      FrFeatureVectorMap *m_self ;	// avoid double init in global ctors
      bool         m_sorted ;
   public:
      enum { unknown = (size_t)~0 } ;
   protected:
      void init() ;
      void init(const FrFeatureVectorMap *oldmap) ;
      void sortNames() ;
      int compare(const char *name1, const char *name2) const ;
   public:
      void *operator new(size_t) { return allocator.allocate() ; }
      void operator delete(void *blk) { allocator.release(blk) ; }
      FrFeatureVectorMap() ;
      FrFeatureVectorMap(const FrFeatureVectorMap &oldmap) ; // extend an existing map
      FrFeatureVectorMap(const FrFeatureVectorMap *oldmap) ; // extend an existing map
      ~FrFeatureVectorMap() ;

      int compare(unsigned index1, unsigned index2) const ;

      // modifiers
      size_t newFeature(const char *feature,
			bool protect = false) ; // no check if exists
      size_t addFeature(const char *feature,
			bool protect = false) ; // return existing if possible
      void protectFeature(size_t ID) { m_protected->setBit(ID) ; }
      void finalize(size_t feature_size) ;

      // accessors
      bool finalized() const { return m_allocator != 0 ; }
      size_t numFeatures() const { return m_featurecount ; }
      size_t featureIndex(const char *feature, bool full_scan = false) const ;
      size_t featureIndex(const FrSymbol *feature) const ;
      const char *featureName(size_t N) const ;
      FrList *featureNames() const ;
      bool isProtected(size_t N) const { return m_protected->getBit(N) ; }
      FrList *protectedFeatures() const ;

      // memory management
      void *allocFeatureValues() const { return m_allocator->allocate() ; }
      void freeFeatureValues(void *val) { m_allocator->release(val) ; }
   } ;

//----------------------------------------------------------------------

template <typename T>
class FrFeatureVectorTemplate
   {
   private:
      static FrAllocator allocator ;
      FrFeatureVectorMap *m_map ;
      T			 *m_values ;
      mutable T		  m_score ;
      unsigned		  m_numfeatures ;
   public:
//      static constexpr T unknown = (T)-FLT_MAX ;
      static const T unknown = (T)-FLT_MAX ;
   public:
      void *operator new(size_t) { return allocator.allocate() ; }
      void operator delete(void *blk) { allocator.release(blk) ; }
      FrFeatureVectorTemplate()
	 {
	 init() ;
	 }
      FrFeatureVectorTemplate(FrFeatureVectorMap *map)
	 {
	 init(map) ;
	 }
      FrFeatureVectorTemplate(const FrFeatureVectorTemplate<T> &orig)
	 {
	 init(orig) ;
	 }
      FrFeatureVectorTemplate(const FrFeatureVectorTemplate<T> *orig)
	 {
	 if (orig)
	    init(*orig) ;
	 else
	    init() ;
	 }
      ~FrFeatureVectorTemplate()
	 {
	 if (m_values) m_map->freeFeatureValues(m_values) ;
	 m_numfeatures = 0 ;
	 m_score = unknown ;
	 }
      void init()
	 {
	 m_map = 0 ;
	 m_values = 0 ;
	 m_numfeatures = 0 ;
	 m_score = unknown ;
	 }
      void init(const FrFeatureVectorTemplate<T> &orig)
	 {
	 m_map = const_cast<FrFeatureVectorMap*>(orig.m_map) ;
	 m_values = (T*)m_map->allocFeatureValues() ;
	 if (m_values)
	    {
	    m_numfeatures = orig.numFeatures() ;
	    for (unsigned i = 0 ; i < m_numfeatures ; i++)
	       m_values[i] = orig.m_values[i] ;
	    m_score = orig.m_score ;
	    }
	 else
	    {
	    m_numfeatures = 0 ;
	    m_score = unknown ;
	    }
	 }
      void init(FrFeatureVectorMap *map, const T* values = 0)
	 {
	 m_map = map ;
	 m_values = (T*)map->allocFeatureValues() ;
	 if (m_values)
	    {
	    m_numfeatures = map->numFeatures() ;
	    if (values)
	       {
	       for (unsigned i = 0 ; i < m_numfeatures ; i++)
		  m_values[i] = values[i] ;
	       }
	    else
	       {
	       for (unsigned i = 0 ; i < m_numfeatures ; i++)
		  m_values[i] = 0.0 ;
	       }
	    }
	 else
	    m_numfeatures = 0 ;
	 m_score = unknown ;
	 }
      FrFeatureVectorTemplate<T> *clone() const
	 {
	 return new FrFeatureVectorTemplate<T>(this) ;
	 }
      void copyValues(const FrFeatureVectorTemplate<T> *source)
	 {
	 for (unsigned i = 0 ; i < m_numfeatures ; i++)
	    m_values[i] = source->m_values[i] ;
	 }

      // accessors
      const FrFeatureVectorMap *featureMap() const { return m_map ; }
      FrFeatureVectorMap *featureMap() { return m_map ; }
      static size_t featureSize() { return sizeof(T) ; }
      T cachedScore() const { return m_score ; }
      T score() const
	 {
	    if (m_score <= unknown)
	       {
	       m_score = 0.0 ;
	       for (size_t i = 0 ; i < numFeatures() ; i++)
		  m_score += m_values[i] ;
	       }
	    return m_score ;
	 } ;
      T score(const FrFeatureVectorTemplate<T> *weights) const
	 {
	    m_score = 0.0 ;
	    for (size_t i = 0 ; i < numFeatures() ; i++)
	       m_score += m_values[i] * weights->m_values[i] ;
	    return m_score ;
	 }
      T value(size_t N) const
	 { return (N < numFeatures()) ? m_values[N] : unknown ; }
      T value(const char *feature) const
	 { return value(m_map->featureIndex(feature)) ; }
      T value(const FrSymbol *feature) const
	 { return value(m_map->featureIndex(feature)) ; }
      T absoluteValue(size_t N) const
	 { T val = value(N) ; return val >= 0 ? val : -val ; }
      unsigned numFeatures() const { return m_numfeatures ; }
      const char *featureName(size_t N) const
         { return m_map->featureName(N) ; }

      // modifiers
      void clear()
	 {
	    for (size_t i = 0 ; i < numFeatures() ; i++)
	       m_values[i] = 0 ;
	    m_score = unknown ;
	 }
      void setValues(T val)
	 {
	    for (size_t i = 0 ; i < numFeatures() ; i++)
	       m_values[i] = val ;
	    m_score = unknown ;
	 }
      bool setValue(size_t N, T val)
	 {
	    if (N < numFeatures())
	       {
	       m_values[N] = val ;
	       m_score = unknown ;
	       return true ;
	       }
	    return false ;
	 }
      void setValue(const char *feature, T val)
         { setValue(m_map->featureIndex(feature),val) ; }
      void setValue(const FrSymbol *feature, T val)
         { setValue(m_map->featureIndex(feature),val) ; }
      bool incrValue(size_t N, T increment = 1.0)
         {
	    if (N < numFeatures())
	       {
	       m_values[N] += increment ;
	       m_score = unknown ;
	       return true ;
	       }
	    return false ;
	 }
      void incrValue(const char *feature, T increment = 1.0)
         { incrValue(m_map->featureIndex(feature),increment) ; }
      void incrValue(const FrSymbol *feature, T increment = 1.0)
         { incrValue(m_map->featureIndex(feature),increment) ; }
      bool scaleValue(size_t N, T factor)
         {
	    if (N < numFeatures())
	       {
	       m_values[N] *= factor ;
	       m_score = unknown ;
	       return true ;
	       }
	    return false ;
	 }
      void scaleValue(const char *feature, T factor)
         { scaleValue(m_map->featureIndex(feature),factor) ; }
      void scaleValue(const FrSymbol *feature, T factor)
         { scaleValue(m_map->featureIndex(feature),factor) ; }

      bool addByName(FrFeatureVectorTemplate<T> *other)
         {
	    bool success = false ;
	    for (size_t i = 0 ; i < other->numFeatures() ; i++)
	       {
	       if (incrValue(other->featureName(i),other->value(i)))
		  success = true ;
	       }
	    return success ;
	 }
      bool add(const FrFeatureVectorTemplate<T> *other)
      // assumes equal indices
         {
	    if (!other || other->numFeatures() != numFeatures())
	       return false ;
	    for (size_t i = 0 ; i < numFeatures() ; i++)
	       incrValue(i,other->value(i)) ;
	    return true ;
	 }
      bool add(const FrFeatureVectorTemplate<T> *other, T otherweight)
      // assumes equal indices
         {
	    if (!other || other->numFeatures() != numFeatures())
	       return false ;
	    for (size_t i = 0 ; i < numFeatures() ; i++)
	       {
	       incrValue(i,otherweight * other->value(i)) ;
	       }
	    return true ;
	 }
      bool add(const FrFeatureVectorTemplate<T> *other,
	       T myweight, T otherweight)
      // assumes equal indices
         {
	    if (!other || other->numFeatures() != numFeatures())
	       return false ;
	    T totalwt = (myweight + otherweight) ;
	    if (totalwt == 0.0) return false ;
	    for (size_t i = 0 ; i < numFeatures() ; i++)
	       {
	       T val = myweight * value(i) + otherweight * other->value(i) ;
	       setValue(i,val / totalwt) ;
	       }
	    return true ;
	 }
      bool subtract(const FrFeatureVectorTemplate<T> *other)
      // assumes equal indices
         {
	    if (!other || other->numFeatures() != numFeatures())
	       return false ;
	    for (size_t i = 0 ; i < numFeatures() ; i++)
	       incrValue(i,-other->value(i)) ;
	    return true ;
	 }
      bool multiply(FrFeatureVectorTemplate<T> *other) // assumes equal indices
         {
	    if (!other || other->numFeatures() != numFeatures())
	       return false ;
	    for (size_t i = 0 ; i < numFeatures() ; i++)
	       scaleValue(i,other->value(i)) ;
	    return true ;
	 }
      void scaleValues(T scale)
         {
	    for (size_t i = 0 ; i < numFeatures() ; i++)
	       scaleValue(i,scale) ;
	 }
      bool multiplyByName(FrFeatureVectorTemplate<T> *other)
         {
	    bool success = false ;
	    for (size_t i = 0 ; i < other->numFeatures() ; i++)
	       {
	       if (scaleValue(other->featureName(i),other->value(i)))
		  success = true ;
	       }
	    return success ;
	 }
      T maxValue() const
	 {
	    T max = value(0) ;
	    for (size_t i = 1 ; i < numFeatures() ; i++)
	       {
	       if (value(i) > max)
		  max = value(i) ;
	       }
	    return max ;
	 }
      T maxMagnitude() const
	 {
	    T max = absoluteValue(0) ;
	    for (size_t i = 1 ; i < numFeatures() ; i++)
	       {
	       T val = absoluteValue(i) ;
	       if (val > max)
		  max = val ;
	       }
	    return max ;
	 }
      void maxMagnitude(const FrFeatureVectorTemplate<T> *other)
	 {
	    for (size_t i = 1 ; i < numFeatures() ; i++)
	       {
	       T val1 = absoluteValue(i) ;
	       T val2 = other->absoluteValue(i) ;
	       if (val2 > val1)
		  val1 = val2 ;
	       setValue(i,val1) ;
	       }
	 }
      T minValue() const
	 {
	    T min = value(0) ;
	    for (size_t i = 1 ; i < numFeatures() ; i++)
	       {
	       if (value(i) < min)
		  min = value(i) ;
	       }
	    return min ;
	 }
      double normSquared() const
	 {
	    double n = 0.0 ;
	    for (size_t i = 0 ; i < numFeatures() ; i++)
	       n += (value(i) * value(i)) ;
	    return n ;
	 }
      double norm() const
	 {
	    return ::sqrt(normSquared()) ;
	 }

      // I/O
      bool loadValues(const FrList *values)
         {
	    bool set_value = false ;
	    size_t idx = 0 ;
	    for ( ; values ; values = values->rest(), idx++)
	       {
	       FrObject *val = values->first() ;
	       if (!val) continue ;
	       if (val->consp())
		  {
		  FrObject *n = ((FrList*)val)->first() ;
		  FrObject *v = ((FrList*)val)->second() ;
		  if (n && v && v->numberp())
		     set_value |= setValue(FrPrintableName(n),v->floatValue());
		  }
	       else if (val->numberp())
		  set_value |= setValue(idx,val->floatValue()) ;
	       }
	    return set_value ;
	 }
      FrList *printable(bool include_names = false) const
         {
	    FrList *result ;
	    FrList **end = &result ;
	    for (size_t i = 0 ; i < numFeatures() ; i++)
	       {
	       FrObject *feat = new FrFloat(value(i)) ;
	       if (include_names)
		  feat = new FrList(new FrString(featureName(i)),feat) ;
	       result->pushlistend(feat,end) ;
	       }
	    *end = 0 ;  // terminate the list
	    return result ;
	 }
   } ;

template <typename T>
FrAllocator FrFeatureVectorTemplate<T>::allocator("FrFeatureVector",sizeof(FrFeatureVectorTemplate<T>)) ;

//----------------------------------------------------------------------

typedef FrFeatureVectorTemplate<float>  FrFeatureVectorFloat ;
typedef FrFeatureVectorTemplate<double>  FrFeatureVectorDouble ;

#endif /* !__FRFEATVEC_H_INCLUDED */

/* end of file frfeatvec.h */

