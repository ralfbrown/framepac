/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC								*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frmath.h	general math/probability functions		*/
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

#ifndef __FRMATH_H_INCLUDED
#define __FRMATH_H_INCLUDED

/************************************************************************/
/************************************************************************/

class FrMath 
   {
   public:
      static double log(double x) ;
      static double smoothedLog(double x, double threshold = -10.0) ;
      static double gaussianProbability(double x, double mean, double stddev) ;
      static double F_measure(double beta, double precision, double recall) ;
   } ;

#endif /* !__FRMATH_H_INCLUDED */

/* end of file frmath.h */
