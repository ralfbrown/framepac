/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC								*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File: frthresh.cpp	      multiple-threshold list			*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1999,2000,2003,2009					*/
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

#include "frclust.h"
#include "frfloat.h"
#include "frsymtab.h"

#ifdef FrSTRICT_CPLUSPLUS
# include <fstream>
#else
# include <fstream.h>
#endif /* FrSTRICT_CPLUSPLUS */

/************************************************************************/
/************************************************************************/

FrFloat dummy_to_force_linkage_of_FrFloat_code ;

/************************************************************************/
/************************************************************************/

static int compare_index(const FrObject *o1, const FrObject *o2)
{
   if (o1 && o2)
      {
      if (o1->consp() && o2->consp())
	 {
	 FrObject *index1 = ((FrList*)o1)->first() ;
	 FrObject *index2 = ((FrList*)o2)->first() ;
	 return index1->intValue() - index2->intValue() ;
	 }
      else
	 return o1->compare(o2) ;
      }
   else if (o1)
      return -1 ;
   else if (o2)
      return +1 ;
   return 0 ;
}

//----------------------------------------------------------------------

FrThresholdList::FrThresholdList(const char *filename, double def_thresh)
{
   if (filename && *filename)
      {
      ifstream in(filename) ;
      if (in.good())
	 {
	 min_threshold = 2.0 ;
	 FrList *threshold_list = 0 ;
	 long max_thres = -1 ;
	 while (!in.eof())
	    {
	    FrObject *obj ;
	    in >> obj ;
	    if (obj == FrSymbolTable::add("*EOF*"))
	       break ;
	    if (obj->consp())
	       {
	       FrList *spec = (FrList*)obj ;
	       FrObject *cnt = spec->first() ;
	       FrObject *thr = spec->second() ;
	       if (!cnt || !cnt->numberp() || cnt->intValue() < 0 ||
		   !thr || !thr->numberp() || thr->floatValue() < 0.0)
		  {
		  char *entry = spec->print() ;
		  FrWarningVA("malformed threshold entry:\n"
			      "\t%s\n"
			      "  want \"(count threshold)\", where count >= 1 and threshold >= 0.0",entry) ;
		  FrFree(entry) ;
		  free_object(obj) ;
		  }
	       else
		  {
		  if (cnt->intValue() > max_thres)
		     max_thres = cnt->intValue() ;
		  if (thr->floatValue() < min_threshold)
		     min_threshold = thr->floatValue() ;
		  pushlist(obj,threshold_list) ;
		  }
	       }
	    else
	       {
	       FrWarning("expected \"(count threshold)\" in thresholds file") ;
	       free_object(obj) ;
	       }
	    }
	 if (max_thres > 0)
	    {
	    max_index = max_thres ;
	    thresholds = FrNewN(double,max_thres + 1) ;
	    threshold_list = threshold_list->sort(compare_index) ;
	    double thr = 2.0 ;
	    int i = 0 ;
	    while (threshold_list)
	       {
	       FrList *item = (FrList*)poplist(threshold_list) ;
	       int idx = item->first()->intValue() ;
	       double newthr = item->second()->floatValue() ;
	       free_object(item) ;
	       for ( ; i < idx ; i++)
		  thresholds[i] = thr ;
	       thr = newthr ;
	       }
	    // handle the final index-threshold pair
	    thresholds[i] = thr ;
	    return ;
	    }
	 }
      }
   max_index = 0 ;
   min_threshold = def_thresh ;
   thresholds = FrNewN(double,1) ;
   if (thresholds)
      thresholds[0] = def_thresh ;
   else
      FrNoMemory("building threshold list") ;
   return ;
}

//----------------------------------------------------------------------

FrThresholdList::~FrThresholdList()
{
   FrFree(thresholds) ;
   thresholds = 0 ;
   return ;
}

// end of file frthresh.cpp //
