/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC								*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File: frvecsim.cpp	      vector similarity functions		*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 2006,2009 Ralf Brown/Carnegie Mellon University	*/
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

#include "frclust.h"

#ifdef FrSTRICT_CPLUSPLUS
#  include <cmath>
#else
#  include <math.h>
#endif

/************************************************************************/
/*	Helper functions						*/
/************************************************************************/

inline double maximum(double a, double b) { return a > b ? a : b ; }
inline double minimum(double a, double b) { return a < b ? a : b ; }

//----------------------------------------------------------------------

static double vector_length(const double *vec, size_t veclen)
{
   double length = 0.0 ;
   for (size_t i = 0 ; i < veclen ; i++)
      {
      length += (vec[i] * vec[i]) ;
      }
   return sqrt(length) ;
}

//----------------------------------------------------------------------

static double total_weights(const double *vec, size_t veclen)
{
   double total = 0.0 ;
   for (size_t i = 0 ; i < veclen ; i++)
      {
      total += vec[i] ;
      }
   return total ;
}

//----------------------------------------------------------------------

static double maximum_weight(const double *vec, size_t veclen)
{
   double max = 0.0 ;
   for (size_t i = 0 ; i < veclen ; i++)
      {
      if (vec[i] > max)
	 max = vec[i] ;
      }
   return max ;
}

//----------------------------------------------------------------------

static void term_matches(const double *vec1, const double *vec2,
			 size_t veclen, size_t &both, size_t &neither,
			 size_t &only_1, size_t &only_2)
{
   // count the number of non-zero elements in each vector as well as how
   //   many elements are non-zero in both vectors
   both = 0 ;
   neither = 0 ;
   only_1 = 0 ;
   only_2 = 0 ;
   for (size_t i = 0 ; i < veclen ; i++)
      {
      if (vec1[i])
	 {
	 if (vec2[i])
	    both++ ;
	 else
	    only_1++ ;
	 }
      else if (vec2[i])
	 only_2++ ;
      else
	 neither++ ;
      }
   return ;
}

//----------------------------------------------------------------------

static void term_matches(const double *vec1, const double *vec2,
			 size_t veclen, bool normalize,
			 double &match1_to_2, double &match2_to_1,
			 double &total_1, double &total_2)
{
   match1_to_2 = 0.0 ;
   match2_to_1 = 0.0 ;
   total_1 = 0.0 ;
   total_2 = 0.0 ;
   for (size_t i = 0 ; i < veclen ; i++)
      {
      double v1 = vec1[i] ;
      double v2 = vec2[i] ;
      total_1 += v1 ;
      total_2 += v2 ;
      if (v1 && v2)
	 {
	 match1_to_2 += v1 ;
	 match2_to_1 += v2 ;
	 }
      }
   if (normalize)
      {
      double length1 = vector_length(vec1,veclen) ;
      double length2 = vector_length(vec2,veclen) ;
      if (length1 > 0.0)
	 {
	 total_1 /= length1 ;
	 match1_to_2 /= length1 ;
	 }
      if (length2 > 0.0)
	 {
	 total_2 /= length2 ;
	 match2_to_1 /= length2 ;
	 }
      }
   return ;
}

//----------------------------------------------------------------------

static void contingency_table(const double *vec1, const double *vec2,
			      size_t veclen, double &A, double &B, double &C)
{
   double a(0.0) ;
   double b(0.0) ;
   double c(0.0) ;
   double maxwt1 = maximum_weight(vec1,veclen) ;
   double maxwt2 = maximum_weight(vec2,veclen) ;
   // prevent division by zero -- if maximum weight is zero, all elements are
   //   zero, so it doesn't matter what we divide by; pick 1.0
   if (maxwt1 == 0) maxwt1 = 1.0 ;
   if (maxwt2 == 0) maxwt2 = 1.0 ;
   for (size_t i = 0 ; i < veclen ; i++)
      {
      double v1 = vec1[i] / maxwt1 ;
      double v2 = vec2[i] / maxwt2 ;
      if (v1 && v2)
	 a += minimum(v1,v2) ;
      else if (v1)
	 b += v1 ;
      else if (v2)
	 c += v2 ;
      }
   A = a ;
   B = b ;
   C = c ;
   return ;
}

/************************************************************************/
/*	Similarity metrics						*/
/************************************************************************/

static double binary_antidice_coefficient(const double *vec1,
					  const double *vec2,
					  size_t veclen)
{
   if (!vec1 && !vec2)
      return 1.0 ;
   else if (!vec1 || !vec2)
      return (veclen == 0) ? 1.0 : 0.0 ; // no overlap, but same if zero-length
   size_t only_1, only_2, intersection, neither ;
   term_matches(vec1,vec2,veclen,intersection,neither,only_1,only_2) ;
   size_t diff = only_1 + only_2 ;
   return intersection / (double)(intersection + 2 * diff) ;
}

//----------------------------------------------------------------------

static double binary_dice_coefficient(const double *vec1, const double *vec2,
				      size_t veclen)
{
   if (!vec1 && !vec2)
      return 1.0 ;
   else if (!vec1 || !vec2)
      return (veclen == 0) ? 1.0 : 0.0 ; // no overlap, but same if zero-length
   size_t only_1, only_2, intersection, neither ;
   term_matches(vec1,vec2,veclen,intersection,neither,only_1,only_2) ;
   size_t total_1(only_1 + intersection) ;
   size_t total_2(only_2 + intersection) ;
   return (2.0 * intersection) / (double)(total_1 + total_2) ;
}

//----------------------------------------------------------------------

static double binary_gamma_coefficient(const double *vec1,
				       const double *vec2,
				       size_t veclen)
{
   if (!vec1 && !vec2)
      return 1.0 ;
   else if (!vec1 || !vec2)
      return (veclen == 0) ? 1.0 : -1.0 ;// no overlap, but same if zero-length
   size_t only_1, only_2, both, neither ;
   term_matches(vec1,vec2,veclen,both,neither,only_1,only_2) ;
   double N(both + only_1 + only_2 + neither) ;
   double concordance(both / N * neither / N) ;
   double discordance(only_1 / N * only_2 / N) ;
   if (concordance + discordance > 0)
      return (concordance - discordance) / (concordance + discordance) ;
   else
      return 1.0 ;
}

//----------------------------------------------------------------------

static double jaccard_coefficient(const double *vec1, const double *vec2,
				  size_t veclen)
{
   if (!vec1 && !vec2)
      return 1.0 ;
   else if (!vec1 || !vec2)
      return (veclen == 0) ? 1.0 : 0.0 ;// no overlap, but same if zero-length
   size_t only_1, only_2, both, neither ;
   term_matches(vec1,vec2,veclen,both,neither,only_1,only_2) ;
   size_t union_size(only_1 + only_2 + both) ;
   return both / (double)union_size ;
}

//----------------------------------------------------------------------

static double simpson_coefficient(const double *vec1, const double *vec2,
				  size_t veclen)
{
   if (!vec1 && !vec2)
      return 1.0 ;
   else if (!vec1 || !vec2)
      return (veclen == 0) ? 1.0 : 0.0 ;// no overlap, but same if zero-length
   size_t only_1, only_2, both, neither ;
   term_matches(vec1,vec2,veclen,both,neither,only_1,only_2) ;
   size_t smaller = (only_1 < only_2) ? only_1 + both : only_2 + both ;
   return both / (double)smaller ;
}

//----------------------------------------------------------------------

static double cosine_similarity(const double *vec1, const double *vec2,
				size_t veclen, bool normalize)
{
   double cosine = 0.0 ;
   if (normalize)
      {
      double length1 = 0.0 ;
      double length2 = 0.0 ;
      for (size_t i = 0 ; i < veclen ; i++)
	 {
	 double value1 = vec1[i] ;
	 double value2 = vec2[i] ;
	 cosine += (value1 * value2) ;
	 length1 += (value1 * value1) ;
	 length2 += (value2 * value2) ;
	 }
      length1 = sqrt(length1) ;
      length2 = sqrt(length2) ;
      cosine = cosine / length1 / length2 ;
      }
   else
      {
      for (size_t i = 0 ; i < veclen ; i++)
	 cosine += (vec1[i] * vec2[i]) ;
      }
   return cosine ;
}

//----------------------------------------------------------------------

static double euclidean_distance(const double *vec1, const double *vec2,
				 size_t veclen, bool normalize)
{
   double distance = 0.0 ;
   if (normalize)
      {
      double length1 = vector_length(vec1,veclen) ;
      double length2 = vector_length(vec2,veclen) ;
      // prevent division by zero -- if ||vector|| is zero, all elements are
      //   zero, so it doesn't matter what we divide by; pick 1.0
      if (length1 == 0) length1 = 1.0 ;
      if (length2 == 0) length2 = 1.0 ;
      for (size_t i = 0 ; i < veclen ; i++)
	 {
	 double dist = (vec1[i] / length1) - (vec2[i] / length2) ;
	 distance += (dist * dist) ;
	 }
      }
   else
      {
      for (size_t i = 0 ; i < veclen ; i++)
	 {
	 double dist = (vec1[i] - vec2[i]) ;
	 distance += (dist * dist) ;
	 }
      }
   return sqrt(distance) ;
}

//----------------------------------------------------------------------

static double manhattan_distance(const double *vec1, const double *vec2,
				 size_t veclen, bool normalize)
{
   double distance = 0.0 ;
   if (normalize)
      {
      double length1 = vector_length(vec1,veclen) ;
      double length2 = vector_length(vec2,veclen) ;
      // prevent division by zero -- if ||vector|| is zero, all elements are
      //   zero, so it doesn't matter what we divide by; pick 1.0
      if (length1 == 0) length1 = 1.0 ;
      if (length2 == 0) length2 = 1.0 ;
      for (size_t i = 0 ; i < veclen ; i++)
	 {
	 distance += fabs((vec1[i] / length1) - (vec2[i] / length2)) ;
	 }
      }
   else
      {
      for (size_t i = 0 ; i < veclen ; i++)
	 {
	 distance += fabs(vec1[i] - vec2[i]) ;
	 }
      }
   return distance ;
}

//----------------------------------------------------------------------

#if NEW
static double chebyshev_distance(const double *vec1, const double *vec2,
				 size_t veclen, bool normalize)
{
   double max_dist = 0.0 ;
   for (size_t i = 0 ; i < veclen ; i++)
      {
      double dist = fabs(vec1[i] - vec2[i]) ;
      if (dist > max_dist)
	 max_dist = dist ;
      }
   if (normalize)
      {
      double length1 = vector_length(vec1,veclen) ;
      double length2 = vector_length(vec2,veclen) ;
      double max_length = maximum(length1,length2) ;
      // scaled result is in [0,2]
      return max_dist / max_length ;
      }
   else
      return max_dist ;
}
#endif

//----------------------------------------------------------------------

static double tanimoto_coefficient(const double *vec1, const double *vec2,
				   size_t veclen, bool /*normalize*/)
{
   if (!vec1 && !vec2)
      return 1.0 ;
   else if (!vec1 || !vec2)
      return (veclen == 0) ? 1.0 : 0.0 ;// no overlap, but same if zero-length
   double union_size(0.0) ;
   double intersection(0.0) ;
   for (size_t i = 0 ; i < veclen ; i++)
      {
      union_size += maximum(vec1[i],vec2[i]) ;
      intersection += minimum(vec1[i],vec2[i]) ;
      }
   if (union_size > intersection)
      return intersection / (union_size - intersection) ;
   else
      return 1.0 ;
}

//----------------------------------------------------------------------

static double extended_simpson_coefficient(const double *vec1,
					   const double *vec2,
					   size_t veclen, bool normalize)
{
   if (!vec1 && !vec2)
      return 1.0 ;
   else if (!vec1 || !vec2)
      return (veclen == 0) ? 1.0 : 0.0 ;// no overlap, but same if zero-length
   double intersection(0.0) ;
   double total_1(0.0) ;
   double total_2(0.0) ;
   if (normalize)
      {
      double length1 = vector_length(vec1,veclen) ;
      double length2 = vector_length(vec2,veclen) ;
      // prevent division by zero -- if ||vector|| is zero, all elements are
      //   zero, so it doesn't matter what we divide by; pick 1.0
      if (length1 == 0) length1 = 1.0 ;
      if (length2 == 0) length2 = 1.0 ;
      for (size_t i = 0 ; i < veclen ; i++)
	 {
	 double v1 = vec1[i] / length1 ;
	 double v2 = vec2[i] / length2 ;
	 total_1 += v1 ;
	 total_2 += v2 ;
	 intersection += minimum(v1,v2) ;
	 }
      }
   else
      {
      for (size_t i = 0 ; i < veclen ; i++)
	 {
	 total_1 += vec1[i] ;
	 total_2 += vec2[i] ;
	 intersection += minimum(vec1[i],vec2[i]) ;
	 }
      }
   double smaller = minimum(total_1,total_2) ;
   if (smaller)
      return intersection / smaller ;
   else
      return 0.0 ;
}

//----------------------------------------------------------------------

static double braun_blanquet_coefficient(const double *vec1,
					 const double *vec2,
					 size_t veclen, bool normalize)
{
   if (!vec1 && !vec2)
      return 1.0 ;
   else if (!vec1 || !vec2)
      return (veclen == 0) ? 1.0 : 0.0 ;// no overlap, but same if zero-length
   double intersection(0.0) ;
   double total_1(0.0) ;
   double total_2(0.0) ;
   if (normalize)
      {
      double length1 = vector_length(vec1,veclen) ;
      double length2 = vector_length(vec2,veclen) ;
      // prevent division by zero -- if ||vector|| is zero, all elements are
      //   zero, so it doesn't matter what we divide by; pick 1.0
      if (length1 == 0) length1 = 1.0 ;
      if (length2 == 0) length2 = 1.0 ;
      for (size_t i = 0 ; i < veclen ; i++)
	 {
	 double v1 = vec1[i] / length1 ;
	 double v2 = vec2[i] / length2 ;
	 total_1 += v1 ;
	 total_2 += v2 ;
	 intersection += minimum(v1,v2) ;
	 }
      }
   else
      {
      for (size_t i = 0 ; i < veclen ; i++)
	 {
	 total_1 += vec1[i] ;
	 total_2 += vec2[i] ;
	 intersection += minimum(vec1[i],vec2[i]) ;
	 }
      }
   double larger = maximum(total_1,total_2) ;
   if (larger)
      return intersection / larger ;
   else
      return 0.0 ;
}

//----------------------------------------------------------------------

static double dice_coefficient(const double *vec1, const double *vec2,
			       size_t veclen, bool normalize)
{
   if (!vec1 && !vec2)
      return 1.0 ;
   else if (!vec1 || !vec2)
      return (veclen == 0) ? 1.0 : 0.0 ;// no overlap, but same if zero-length
   double prod(0.0) ;
   double sum(0.0) ;
   if (normalize)
      {
      double length1 = vector_length(vec1,veclen) ;
      double length2 = vector_length(vec2,veclen) ;
      // prevent division by zero -- if ||vector|| is zero, all elements are
      //   zero, so it doesn't matter what we divide by; pick 1.0
      if (length1 == 0) length1 = 1.0 ;
      if (length2 == 0) length2 = 1.0 ;
      for (size_t i = 0 ; i < veclen ; i++)
	 {
	 double v1 = vec1[i] / length1 ;
	 double v2 = vec2[i] / length2 ;
	 prod += (v1 * v2) ;
	 sum += (v1 + v2) ;
	 }
      }
   else
      {
      for (size_t i = 0 ; i < veclen ; i++)
	 {
	 prod += (vec1[i] * vec2[i]) ;
	 sum += (vec1[i] + vec2[i]) ;
	 }
      }
   return (sum > 0.0) ? (2.0 * prod / sum) : 0.0 ;
}

//----------------------------------------------------------------------

static double antidice_coefficient(const double *vec1, const double *vec2,
				   size_t veclen, bool normalize)
{
   if (!vec1 && !vec2)
      return 1.0 ;
   else if (!vec1 || !vec2)
      return (veclen == 0) ? 1.0 : 0.0 ;// no overlap, but same if zero-length
   double prod(0.0) ;
   double same(0.0) ;
   double diff(0.0) ;
   if (normalize)
      {
      double length1 = vector_length(vec1,veclen) ;
      double length2 = vector_length(vec2,veclen) ;
      // prevent division by zero -- if ||vector|| is zero, all elements are
      //   zero, so it doesn't matter what we divide by; pick 1.0
      if (length1 == 0) length1 = 1.0 ;
      if (length2 == 0) length2 = 1.0 ;
      for (size_t i = 0 ; i < veclen ; i++)
	 {
	 double v1 = vec1[i] / length1 ;
	 double v2 = vec2[i] / length2 ;
	 if (v1 && v2)
	    {
	    same += minimum(v1,v2) ;
	    prod += (v1 * v2) ;
	    }
	 else
	    diff += (v1 + v2) ;
	 }
      }
   else
      {
      for (size_t i = 0 ; i < veclen ; i++)
	 {
	 double v1 = vec1[i] ;
	 double v2 = vec2[i] ;
	 if (v1 && v2)
	    {
	    same += minimum(v1,v2) ;
	    prod += (v1 * v2) ;
	    }
	 else
	    diff += (v1 + v2) ;
	 }
      }
   return (same+diff > 0.0) ? (prod / (same + 2.0*diff)) : 0.0 ;
}

//----------------------------------------------------------------------

static double kulczynski_measure1(const double *vec1, const double *vec2,
				  size_t veclen, bool normalize)
{
   if (!vec1 && !vec2)
      return 1.0 ;
   else if (!vec1 || !vec2)
      return (veclen == 0) ? 1.0 : 0.0 ;// no overlap, but same if zero-length
   double match(0.0) ;
   double mismatch(0.0) ;
   if (normalize)
      {
      double length1 = vector_length(vec1,veclen) ;
      double length2 = vector_length(vec2,veclen) ;
      // prevent division by zero -- if ||vector|| is zero, all elements are
      //   zero, so it doesn't matter what we divide by; pick 1.0
      if (length1 == 0) length1 = 1.0 ;
      if (length2 == 0) length2 = 1.0 ;
      for (size_t i = 0 ; i < veclen ; i++)
	 {
	 double value = (vec1[i] / length1 + vec2[i] / length2) ;
	 if (vec1[i] && vec2[i])
	    match += value ;
	 else
	    mismatch += value ;
	 }
      }
   else
      {
      for (size_t i = 0 ; i < veclen ; i++)
	 {
	 double value = (vec1[i] + vec2[i]) ;
	 if (vec1[i] && vec2[i])
	    match += value ;
	 else
	    mismatch += value ;
	 }
      }
   if (mismatch)
      return match / mismatch ;
   else
      return 2.0 * match ;
}

//----------------------------------------------------------------------

static double kulczynski_measure2(const double *vec1, const double *vec2,
				  size_t veclen, bool normalize)
{
   if (!vec1 && !vec2)
      return 1.0 ;
   else if (!vec1 || !vec2)
      return (veclen == 0) ? 1.0 : 0.0 ;// no overlap, but same if zero-length
   double sum1, sum2, match1, match2 ;
   term_matches(vec1,vec2,veclen,normalize,match1,match2,sum1,sum2) ;
   if (sum1 == 0.0 || sum2 == 0.0)
      return 0.0 ;
   else
      return (match1 / sum1 + match2 / sum2) / 2.0 ;
}

//----------------------------------------------------------------------

static double ochiai_measure(const double *vec1, const double *vec2,
			     size_t veclen, bool normalize)
{
   if (!vec1 && !vec2)
      return 1.0 ;
   else if (!vec1 || !vec2)
      return (veclen == 0) ? 1.0 : 0.0 ;// no overlap, but same if zero-length
   double sum1, sum2, match1, match2 ;
   term_matches(vec1,vec2,veclen,normalize,match1,match2,sum1,sum2) ;
   if (sum1 == 0.0 || sum2 == 0.0)
      return 0.0 ;
   else
      return (0.5 * (match1 + match2)) / sqrt(sum1 * sum2) ;
}

//----------------------------------------------------------------------

static double sokal_sneath_measure(const double *vec1, const double *vec2,
				   size_t veclen, bool normalize)
{
   if (!vec1 && !vec2)
      return 1.0 ;
   else if (!vec1 || !vec2)
      return (veclen == 0) ? 1.0 : 0.0 ;// no overlap, but same if zero-length
   double sum1, sum2, match1, match2 ;
   term_matches(vec1,vec2,veclen,normalize,match1,match2,sum1,sum2) ;
   double mismatch1 = sum1 - match1 ;
   double mismatch2 = sum2 - match2 ;
   double match = (0.5 * (match1 + match2)) ;
   return match / (match + 2.0*mismatch1 + 2.0*mismatch2) ;
}

//----------------------------------------------------------------------

static double mcConnaughey_measure(const double *vec1, const double *vec2,
				   size_t veclen, bool normalize)
{
   if (!vec1 && !vec2)
      return 1.0 ;
   else if (!vec1 || !vec2)
      return (veclen == 0) ? 1.0 : 0.0 ;// no overlap, but same if zero-length
   double sum1, sum2, match1, match2 ;
   term_matches(vec1,vec2,veclen,normalize,match1,match2,sum1,sum2) ;
   if (sum1 == 0.0 || sum2 == 0.0)
      return 0.0 ;
   else
      {
      double mismatch1 = sum1 - match1 ;
      double mismatch2 = sum2 - match2 ;
      double match_sq = (match1 * match2) ;
      return (match_sq - mismatch1 * mismatch2) / (sum1 * sum2) ;
      }
}

//----------------------------------------------------------------------

static double lance_williams_distance(const double *vec1, const double *vec2,
				      size_t veclen, bool normalize)
{
   if (!vec1 && !vec2)
      return 0.0 ;
   else if (!vec1 || !vec2)
      return (veclen == 0) ? 0.0 : 1.0 ; // max distance unless both zero
   double sum1, sum2, match1, match2 ;
   term_matches(vec1,vec2,veclen,normalize,match1,match2,sum1,sum2) ;
   if (sum1 + sum2 == 0)
      return 1.0 ;
   else
      {
      double mismatch1 = sum1 - match1 ;
      double mismatch2 = sum2 - match2 ;
      return (mismatch1 + mismatch2) / (sum1 + sum2) ;
      }
}

//----------------------------------------------------------------------

static double bray_curtis_measure(const double *vec1, const double *vec2,
				  size_t veclen, bool normalize)
{
   if (!vec1 && !vec2)
      return 1.0 ;
   else if (!vec1 || !vec2)
      return (veclen == 0) ? 1.0 : 0.0 ;// no overlap, but same if zero-length
   double absdiff(0.0), sum(0.0) ;
   if (normalize)
      {
      double length1 = vector_length(vec1,veclen) ;
      double length2 = vector_length(vec2,veclen) ;
      // prevent division by zero -- if ||vector|| is zero, all elements are
      //   zero, so it doesn't matter what we divide by; pick 1.0
      if (length1 == 0) length1 = 1.0 ;
      if (length2 == 0) length2 = 1.0 ;
      for (size_t i = 0 ; i < veclen ; i++)
	 {
	 double v1 = vec1[i] / length1 ;
	 double v2 = vec2[i] / length2 ;
	 absdiff += fabs(v1 - v2) ;
	 sum += (v1 + v2) ;
	 }
      }
   else
      {
      for (size_t i = 0 ; i < veclen ; i++)
	 {
	 double v1 = vec1[i] ;
	 double v2 = vec2[i] ;
	 absdiff += fabs(v1 - v2) ;
	 sum += (v1 + v2) ;
	 }
      }
   if (sum)
      return 1.0 - (absdiff / sum) ;
   else
      return 0.0 ;
}

//----------------------------------------------------------------------

static double canberra_measure(const double *vec1, const double *vec2,
			       size_t veclen, bool normalize)
{
   if (!vec1 && !vec2)
      return 1.0 ;
   else if (!vec1 || !vec2)
      return (veclen == 0) ? 1.0 : 0.0 ;// no overlap, but same if zero-length
   double total(0.0) ;
   if (normalize)
      {
      double length1 = vector_length(vec1,veclen) ;
      double length2 = vector_length(vec2,veclen) ;
      // prevent division by zero -- if ||vector|| is zero, all elements are
      //   zero, so it doesn't matter what we divide by; pick 1.0
      if (length1 == 0) length1 = 1.0 ;
      if (length2 == 0) length2 = 1.0 ;
      for (size_t i = 0 ; i < veclen ; i++)
	 {
	 double v1 = vec1[i] / length1 ;
	 double v2 = vec2[i] / length2 ;
	 double diff = (v1 - v2) ;
	 double sum = (v1 + v2) ;
	 if (sum > 0)
	    total += fabs(diff / sum) ;
	 }
      }
   else
      {
      for (size_t i = 0 ; i < veclen ; i++)
	 {
	 double v1 = vec1[i] ;
	 double v2 = vec2[i] ;
	 double diff = (v1 - v2) ;
	 double sum = (v1 + v2) ;
	 if (sum > 0)
	    total += fabs(diff / sum) ;
	 }
      }
   return total / veclen ;
}

//----------------------------------------------------------------------

static double czekanowski_measure(const double *vec1, const double *vec2,
				  size_t veclen, bool normalize)
{
   if (!vec1 && !vec2)
      return 1.0 ;
   else if (!vec1 || !vec2)
      return (veclen == 0) ? 1.0 : 0.0 ;// no overlap, but same if zero-length
   double sum_of_min(0.0), sum(0.0) ;
   if (normalize)
      {
      double length1 = vector_length(vec1,veclen) ;
      double length2 = vector_length(vec2,veclen) ;
      // prevent division by zero -- if ||vector|| is zero, all elements are
      //   zero, so it doesn't matter what we divide by; pick 1.0
      if (length1 == 0) length1 = 1.0 ;
      if (length2 == 0) length2 = 1.0 ;
      for (size_t i = 0 ; i < veclen ; i++)
	 {
	 double v1 = vec1[i] / length1 ;
	 double v2 = vec2[i] / length2 ;
	 sum_of_min += minimum(v1,v2) ;
	 sum += (v1 + v2) ;
	 }
      }
   else
      {
      for (size_t i = 0 ; i < veclen ; i++)
	 {
	 double v1 = vec1[i] ;
	 double v2 = vec2[i] ;
	 sum_of_min += minimum(v1,v2) ;
	 sum += (v1 + v2) ;
	 }
      }
   if (sum)
      return 2.0 * sum_of_min / sum ;
   else
      return 0.0 ;
}

//----------------------------------------------------------------------

static double robinson_coefficient(const double *vec1, const double *vec2,
				   size_t veclen, bool /*normalize*/)
{
   if (!vec1 && !vec2)
      return 1.0 ;
   else if (!vec1 || !vec2)
      return (veclen == 0) ? 1.0 : 0.0 ;// no overlap, but same if zero-length
   double sum(0.0) ;
   double totalwt1 = total_weights(vec1,veclen) ;
   double totalwt2 = total_weights(vec2,veclen) ;
   // prevent division by zero -- if sum of weights is zero, all elements are
   //   zero, so it doesn't matter what we divide by; pick 1.0
   if (totalwt1 == 0) totalwt1 = 1.0 ;
   if (totalwt2 == 0) totalwt2 = 1.0 ;
   for (size_t i = 0 ; i < veclen ; i++)
      {
      double v1 = vec1[i] / totalwt1 ;
      double v2 = vec2[i] / totalwt2 ;
      sum += fabs(v1 - v2) ;
      }
   return 2.0 - sum ;
}

//----------------------------------------------------------------------

static double drennan_dissimilarity(const double *vec1, const double *vec2,
				    size_t veclen, bool /*normalize*/)
{
   if (!vec1 && !vec2)
      return 1.0 ;
   else if (!vec1 || !vec2)
      return (veclen == 0) ? 1.0 : 0.0 ;// no overlap, but same if zero-length
   double sum(0.0) ;
   double totalwt1 = total_weights(vec1,veclen) ;
   double totalwt2 = total_weights(vec2,veclen) ;
   // prevent division by zero -- if sum of weights is zero, all elements are
   //   zero, so it doesn't matter what we divide by; pick 1.0
   if (totalwt1 == 0) totalwt1 = 1.0 ;
   if (totalwt2 == 0) totalwt2 = 1.0 ;
   for (size_t i = 0 ; i < veclen ; i++)
      {
      double v1 = vec1[i] / totalwt1 ;
      double v2 = vec2[i] / totalwt2 ;
      sum += fabs(v1 - v2) ;
      }
   return sum / 2.0 ;
}

//----------------------------------------------------------------------

static double circle_product(const double *vec1, const double *vec2,
			     size_t veclen, bool normalize)
{
   if (!vec1 || !vec2)
      return 0.0 ;
   double sum(0.0) ;
   if (normalize)
      {
      double length1 = vector_length(vec1,veclen) ;
      double length2 = vector_length(vec2,veclen) ;
      // prevent division by zero -- if ||vector|| is zero, all elements are
      //   zero, so it doesn't matter what we divide by; pick 1.0
      if (length1 == 0) length1 = 1.0 ;
      if (length2 == 0) length2 = 1.0 ;
      for (size_t i = 0 ; i < veclen ; i++)
	 {
	 double v1 = vec1[i] / length1 ;
	 double v2 = vec2[i] / length2 ;
	 sum += minimum(v1,v2) ;
	 }
      }
   else
      {
      for (size_t i = 0 ; i < veclen ; i++)
	 {
	 double v1 = vec1[i] ;
	 double v2 = vec2[i] ;
	 sum += minimum(v1,v2) ;
	 }
      }
   return sum ;
}

//----------------------------------------------------------------------

static double similarity_ratio(const double *vec1, const double *vec2,
			       size_t veclen, bool normalize)
{
   if (!vec1 && !vec2)
      return 1.0 ;
   else if (!vec1 || !vec2)
      return (veclen == 0) ? 1.0 : 0.0 ;// no overlap, but same if zero-length
   double prod(0.0) ;
   double sum(0.0) ;
   if (normalize)
      {
      double length1 = vector_length(vec1,veclen) ;
      double length2 = vector_length(vec2,veclen) ;
      // prevent division by zero -- if ||vector|| is zero, all elements are
      //   zero, so it doesn't matter what we divide by; pick 1.0
      if (length1 == 0) length1 = 1.0 ;
      if (length2 == 0) length2 = 1.0 ;
      for (size_t i = 0 ; i < veclen ; i++)
	 {
	 double v1 = vec1[i] / length1 ;
	 double v2 = vec2[i] / length2 ;
	 sum += (v1 *v1) + (v2 * v2) ;
	 prod += (v1 * v2) ;
	 }
      }
   else
      {
      for (size_t i = 0 ; i < veclen ; i++)
	 {
	 double v1 = vec1[i] ;
	 double v2 = vec2[i] ;
	 sum += (v1 * v1) + (v2 * v2) ;
	 prod += (v1 * v2) ;
	 }
      }
   if (sum != prod)
      return prod / (sum - prod) ;
   else
      return DBL_MAX ;
}

//----------------------------------------------------------------------
//  the Jensen-Shannon divergence is an averaged Kullback-Liebler divergence:
//      JS(a,b) == (KL(a|avg(a,b)) + KL(b|avg(a,b))) / 2
//      KL(a|b) == sum_y( a(y)(log a(y) - log b(y)) )

//static double log_2 = 0.0 ;
static double log_2 = ::log(2.0) ;

static double KL_term(double a, double b)
{
   if (a <= 0.0 || b <= -0.0)
      return 0.0 ;
//   if (log_2 == 0.0)
//      log_2 = ::log(2.0) ;
   return a * (::log(a)/log_2 - ::log(b)/log_2) ;
}

static double JS_term(double a, double b)
{
   double avg = (a + b) / 2.0 ;
   return KL_term(a,avg) + KL_term(b,avg) ;
}

//----------------------------------------------------------------------

static double jensen_shannon_divergence(const double *vec1,
					const double *vec2,
					size_t veclen, bool /*normalize*/)
{
   if (!vec1 || !vec2 || veclen == 0)
      return 0.0 ;			// no overlap
   double totalwt1 = total_weights(vec1,veclen) ;
   double totalwt2 = total_weights(vec2,veclen) ;
   double sum(0.0) ;
   for (size_t i = 0 ; i < veclen ; i++)
      {
      double v1 = vec1[i] / totalwt1 ;
      double v2 = vec2[i] / totalwt2 ;
      sum += JS_term(v1,v2) ;
      }
   return sum / 2.0 ;
}

//----------------------------------------------------------------------

static double mountford_coefficient(const double *vec1,	const double *vec2,
				    size_t veclen)
{
   if (!vec1 && !vec2)
      return 1.0 ;
   else if (!vec1 || !vec2)
      return (veclen == 0) ? 1.0 : 0.0 ;// no overlap, but same if zero-length
   double a ;
   double b ;
   double c ;
   contingency_table(vec1,vec2,veclen,a,b,c) ;
   double divisor = (2.0 * b * c) + (a * b) + (a * c) ;
   if (divisor <= 0.0)
      return (a > 0.0) ? 1.0 : 0.0 ;
   return (2.0 * a / divisor) ;
}

//----------------------------------------------------------------------

static double fager_mcgowan_coefficient(const double *vec1, const double *vec2,
					size_t veclen)
{
   if (!vec1 && !vec2)
      return 1.0 ;
   else if (!vec1 || !vec2)
      return (veclen == 0) ? 1.0 : 0.0 ;// no overlap, but same if zero-length
   double a ;
   double b ;
   double c ;
   contingency_table(vec1,vec2,veclen,a,b,c) ;
   double divisor = sqrt((a+b) * (a+c)) ;
   double main_term ;
   if (divisor == 0.0)
      main_term = (a > 0.0) ? 1.0 : 0.0 ;
   else
      main_term = a / divisor ;
   return main_term - (maximum(b,c) / 2.0) ;
}

//----------------------------------------------------------------------

static double tripartite_similarity_index(const double *vec1,
					  const double *vec2,
					  size_t veclen)
{
   if (!vec1 && !vec2)
      return 1.0 ;
   else if (!vec1 || !vec2)
      return (veclen == 0) ? 1.0 : 0.0 ;// no overlap, but same if zero-length
   double a ;
   double b ;
   double c ;
   contingency_table(vec1,vec2,veclen,a,b,c) ;
   // avoid division by zero by tweaking b and c if necessary
   if (a == 0)
      {
      if (b == 0.0) b = 0.001 ;
      if (c == 0.0) c = 0.001 ;
      }
   double U = ::log(1.0 + (a+minimum(b,c)) / (a+maximum(b,c))) / log_2 ;
   double S = 1.0 / sqrt(log(2.0 + minimum(b,c)/(a+1.0)) / log_2) ;
   double R_1 = log(1.0 + a / (a+b)) / log_2 ;
   double R_2 = log(1.0 + a / (a+c)) / log_2 ;
   double R = R_1 * R_2 ;
   return sqrt(U * S * R) ;
}

//----------------------------------------------------------------------

#ifdef NEW
static double mahalanobis_distance(const double *vec1, const double *vec2,
				   size_t veclen, const double *covariances)
{
   // d = srqt( (x-y) \times covariances^-1 \times (x-y) )
}
#endif /* NEW */

//----------------------------------------------------------------------

#ifdef NEW
static double jaro_distance(const double *vec1, const double *vec2,
			    size_t veclen)
{
   // string similarity measure between strings s1 and s2
   // for m=#matching_chars
   //   d = 0 if m = 0
   //   d = (m/|s1| + m/|s2| + (m-(#transpositions/2))/m) / 3
   //
   // chars are considered matches if the same and not more than floor(max(|s1|,|s2|)/2)-1
   //   positions apart

   return 0.0 ;
}
#endif /* NEW */

//----------------------------------------------------------------------

#ifdef NEW
static double jaro_winkler_distance(const double *vec1, const double *vec2,
				    size_t veclen)
{
   // for strings s1 and s2:
   // d = jaro_distance(s1,s2) + (prefixlen*scale_factor*(1 - jaro_distance(s1,s2))) ;
   // where prefixlen=min(|common_prefix(s1,s2)|,4) ;
   // scale_factor <= 0.25; standard value 0.1
   // see: http://web.archive.org/web/20100227020019/http://www.census.gov/geo/msb/stand/strcmp.c

   return 0.0 ;
}
#endif /* NEW */

//----------------------------------------------------------------------

#if 0
// other string similarity measures:
//   Hamming distance: min # substitutions to change one string into other
//   Needleman-Wunsch / Sellers' algorithm
//   Smith-Waterman: DP local sequence alignment
//   Ratcliff/Obershelp: #match_chars / (|s1|+|s2|), where match_chars are longest common subsequence plus recursively match_chars in the unmatched regions before/after LCS (DDJ, July 1988 p46)
//   Gotoh
//   Soundex
//   Overlap coefficient
//   Hellinger/Bhattacharyya
//   tau metric (KL approximant)
//   Lee distance: sum_i(min(|x_i-y_i|,q-|x_i-y_i|)) for q-ary alphabet; same as binary Hamming distance if q=2
#endif

/************************************************************************/
/************************************************************************/

double FrVectorSimilarity(FrClusteringMeasure sim,
			  const double *vec1, const double *vec2,
			  size_t veclen, bool normalize)
{
   if (veclen == 0)
      return 1.0 ;			// empty vectors are always identical
   if (!vec1 || !vec2)
      return -1.0 ;			// maximum diff if vector missing
   switch (sim)
      {
      case FrCM_COSINE:
	 return cosine_similarity(vec1,vec2,veclen,normalize) ;
      case FrCM_EUCLIDEAN:
	 return 1.0 - euclidean_distance(vec1,vec2,veclen,normalize) ;
      case FrCM_MANHATTAN:
	 return 1.0 - manhattan_distance(vec1,vec2,veclen,normalize) ;
      case FrCM_JACCARD:
	 return jaccard_coefficient(vec1,vec2,veclen) ;
      case FrCM_SIMPSON:
	 return simpson_coefficient(vec1,vec2,veclen) ;
      case FrCM_EXTSIMPSON:
	 return extended_simpson_coefficient(vec1,vec2,veclen,normalize) ;
      case FrCM_DICE:
	 return dice_coefficient(vec1,vec2,veclen,normalize) ;
      case FrCM_ANTIDICE:
	 return antidice_coefficient(vec1,vec2,veclen,normalize) ;
      case FrCM_TANIMOTO:
	 return tanimoto_coefficient(vec1,vec2,veclen,normalize) ;
      case FrCM_BRAUN_BLANQUET:
	 return braun_blanquet_coefficient(vec1,vec2,veclen,normalize) ;
      case FrCM_KULCZYNSKI1:
	 return kulczynski_measure1(vec1,vec2,veclen,normalize) ;
      case FrCM_KULCZYNSKI2:
	 return kulczynski_measure2(vec1,vec2,veclen,normalize) ;
      case FrCM_OCHIAI:
	 return ochiai_measure(vec1,vec2,veclen,normalize) ;
      case FrCM_SOKALSNEATH:
	 return sokal_sneath_measure(vec1,vec2,veclen,normalize) ;
      case FrCM_MCCONNAUGHEY:
	 // mcConnaughey_measure() return is in range -1.0...+1.0
	 return (mcConnaughey_measure(vec1,vec2,veclen,normalize)+1.0) / 2.0 ;
      case FrCM_LANCEWILLIAMS:
	 return 1.0 - lance_williams_distance(vec1,vec2,veclen,normalize) ;
      case FrCM_BRAYCURTIS:
	 return bray_curtis_measure(vec1,vec2,veclen,normalize) ;
      case FrCM_CANBERRA:
	 return 1.0 - canberra_measure(vec1,vec2,veclen,normalize) ;
      case FrCM_CIRCLEPROD:
	 return circle_product(vec1,vec2,veclen,normalize) / veclen ;
      case FrCM_CZEKANOWSKI:
	 return czekanowski_measure(vec1,vec2,veclen,normalize) ;
      case FrCM_ROBINSON:
	 return robinson_coefficient(vec1,vec2,veclen,normalize) / 2.0 ;
      case FrCM_DRENNAN:
	 return 1.0 - drennan_dissimilarity(vec1,vec2,veclen,normalize) ;
      case FrCM_SIMILARITYRATIO:
	 return similarity_ratio(vec1,vec2,veclen,normalize) ;
      case FrCM_JENSENSHANNON:
	 return 1.0 - jensen_shannon_divergence(vec1,vec2,veclen,normalize) ;
      case FrCM_MOUNTFORD:
	 return mountford_coefficient(vec1,vec2,veclen) ;
      case FrCM_FAGER_MCGOWAN:
	 return fager_mcgowan_coefficient(vec1,vec2,veclen) ;
      case FrCM_TRIPARTITE:
	 return tripartite_similarity_index(vec1,vec2,veclen) ;
      case FrCM_BIN_DICE:
	 return binary_dice_coefficient(vec1,vec2,veclen) ;
      case FrCM_BIN_ANTIDICE:
	 return binary_antidice_coefficient(vec1,vec2,veclen) ;
      case FrCM_BIN_GAMMA:
	 return binary_gamma_coefficient(vec1,vec2,veclen) ;
      case FrCM_NONE:
	 return 0.0 ;
      default:
	 FrMissedCase("FrVectorSimilarity()") ;
	 return 0.0 ;
      }
}

// end of file frvecsim.cpp //
