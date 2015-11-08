/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC								*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frmath.cpp	general math/probability functions		*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 2010 Ralf Brown/Carnegie Mellon University		*/
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

#include <cmath>
#include "float.h"
#include "limits.h"
#include "frmath.h"

/************************************************************************/
/************************************************************************/

//----------------------------------------------------------------------

double FrMath::log(double x)
{
   // avoid infinities and other problems by restricting the input to be
   //  a number large enough to produce a valid logarithm
   if (x < DBL_MIN)
      x = DBL_MIN ;
   return ::log(x) ;
}

//----------------------------------------------------------------------

double FrMath::smoothedLog(double x, double threshold)
{
   double lg = FrMath::log(x) ;
   if (lg < threshold)
      return threshold * ::sqrt(lg / threshold) ;
   return lg ;
}

//----------------------------------------------------------------------

double FrMath::gaussianProbability(double x, double mean, double stddev)
{
   double variance = stddev * stddev ;
   if (variance > 0)
      {
      double deviation = x - mean ;
      double power = (deviation * deviation) / (2.0 * variance) ;
      double normalizer = ::sqrt(2.0 * M_PI * variance) ;
      return ::exp(-power) / normalizer ;
      }
   else
      return (x == mean) ? 1.0 : 0.0 ;
}

//----------------------------------------------------------------------

double FrMath::F_measure(double beta, double precision, double recall)
{
   if ((precision > 0.0 && beta != 0.0) || recall > 0.0)
      {
      double beta2 = beta * beta ;
      return (1 + beta2) * precision * recall / (beta2 * precision + recall) ;
      }
   else
      return 0.0 ;
}


/* end of file frmath.cpp */
