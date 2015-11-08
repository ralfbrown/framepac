/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC								*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frrandom.h		random-number fns & random sampling	*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1999,2000,2004,2009					*/
/*	   Ralf Brown/Carnegie Mellon University			*/
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

#ifndef __FRRANDOM_H_INCLUDED
#define __FRRANDOM_H_INCLUDED

#ifndef __FRLIST_H_INCLUDED
#include "frlist.h"
#endif

/************************************************************************/
/************************************************************************/

void FrSeedRandom() ;			// seed RNG from current time
void FrSeedRandom(unsigned int seed) ;
inline void FrSeedRandom(int seed) { FrSeedRandom((unsigned int)seed) ; }
size_t FrRandomNumber(size_t range) ;
inline size_t FrRandomNumber(int range)
   { return FrRandomNumber((size_t)range) ; }
double FrRandomNumber(double range) ;
char *FrRandomSample(size_t total_size, size_t sample_size,
		     bool reseed = true) ;
FrList *FrRandomSample(FrList *items, size_t sample_size,
		       bool delete_discarded = true, bool reseed = true) ;

#endif /* !__FRRANDOM_H_INCLUDED */

// end of file frrandom.h //
