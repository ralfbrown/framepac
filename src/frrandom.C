/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC								*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frrandom.cpp		random-number fns & random sampling	*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1999,2000,2002,2009,2015				*/
/*	    Ralf Brown/Carnegie Mellon University			*/
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

#include <memory.h>
#include <stdlib.h>
#include <time.h>
#include "framerr.h"
#include "frrandom.h"

/************************************************************************/
/************************************************************************/

void FrSeedRandom(unsigned int seed)
{
#ifdef FrHAVE_SRAND48
   srand48(seed) ;
#else
   srand(seed) ;
#endif /* FrHAVE_SRAND48 */
   return ;
}

//----------------------------------------------------------------------

void FrSeedRandom()
{
#ifdef FrHAVE_SRAND48
   srand48(time(0)) ;
#else
   srand(time(0)) ;
#endif /* FrHAVE_SRAND48 */
   return ;
}

//----------------------------------------------------------------------

size_t FrRandomNumber(size_t range)
{
#ifdef FrHAVE_SRAND48
   long rn = lrand48() ;
#else
   if (range > RAND_MAX)
      {
      static bool warned = false ;
      if (!warned)
	 FrWarning("random number generator does not have a\n"
		   "\tsufficiently large range.");
      warned = true ;
      }
   long rn = (long)rand() ;
#endif /* FrHAVE_SRAND48 */
   return rn % range ;
}

//----------------------------------------------------------------------

double FrRandomNumber(double range)
{
#ifdef FrHAVE_SRAND48
   double rn = drand48() ;
#else
   if (range > RAND_MAX)
      {
      static bool warned = false ;
      if (!warned)
	 FrWarning("random number generator does not have a\n"
		   "\tsufficiently large range.");
      warned = true ;
      }
   double rn = ((double)rand()) / (RAND_MAX+1) ;
#endif /* FrHAVE_SRAND48 */
   return rn * range ;
}

//----------------------------------------------------------------------

char *FrRandomSample(size_t total_size, size_t sample_size, bool reseed)
{
   char *selected = FrNewC(char,total_size+1) ;
   if (!selected)
      {
      FrNoMemory("generating random sampling") ;
      return 0 ;
      }
   if (reseed)
      FrSeedRandom() ;			// seed random number gen from time
   if (sample_size > total_size / 2)
      {
      // to avoid looping nearly forever on big samples, turn it into the
      //   equivalent problem of *deselecting* a small portion of the complete
      //   set of documents
      memset(selected,1,total_size) ;
      sample_size = total_size - sample_size ;
      for ( ; sample_size > 0 ; sample_size--)
	 {
	 size_t select ;
	 do {
	    select = FrRandomNumber(total_size) ;
	    } while (!selected[select]) ;
	 selected[select] = '\0' ;
	 }
      }
   else
      {
      for ( ; sample_size > 0 ; sample_size--)
	 {
	 size_t select ;
	 do {
	    select = FrRandomNumber(total_size) ;
	    } while (selected[select]) ;
	 selected[select] = '\1' ;
	 }
      }
   return selected ;
}

//----------------------------------------------------------------------

FrList *FrRandomSample(FrList *items, size_t sample_size,
		       bool delete_discarded, bool reseed)
{
   size_t numitems = items->listlength() ;
   if (sample_size >= numitems)
      return items ;
   char *selected = FrRandomSample(numitems,sample_size,reseed) ;
   FrList *sample = 0 ;
   FrList **end = &sample ;
   for (size_t i = 0 ; i < numitems ; i++)
      {
      FrObject *item = poplist(items) ;
      if (selected[i])
	 sample->pushlistend(item,end) ;
      else if (delete_discarded)
	 free_object(item) ;
      }
   FrFree(selected) ;
   *end = 0 ;				// properly terminate the result list
   return sample ;
}

// end of file frrandom.cpp //
