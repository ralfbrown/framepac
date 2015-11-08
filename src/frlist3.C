/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frlist3.cpp	utility functions for class FrList		*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1997,1998,2001,2004,2009				*/
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

#include "frlist.h"

/**********************************************************************/
/*    Non-Member functions related to class FrList		      */
/**********************************************************************/

FrList *FrFlattenList(const FrList *list)
{
   if (!list)
      return 0 ;
   else if (list->consp())
      {
      FrList *result = 0 ;
      for ( ; list ; list = list->rest())
	 {
	 FrObject *head = list->first() ;
	 if (head)
	    {
	    if (head->consp())
	       {
	       FrList *flattened = listreverse(FrFlattenList((FrList*)head)) ;
	       result = flattened->nconc(result) ;
	       }
	    else
	       pushlist(head->deepcopy(),result) ;
	    }
	 else
	    pushlist(0,result) ;
	 }
      return listreverse(result) ;
      }
   else
      return new FrList(list->deepcopy()) ;
}

//----------------------------------------------------------------------

static void flatten(FrList *list, FrList **end, bool remove_NILs)
{
   FrList *next ;
   for ( ; list ; list = next)
      {
      next = list->rest() ;
      FrObject *head = list->first() ;
      list->replacd(0) ;
      if (head && head->consp())
	 {
	 list->replaca(0) ;
	 list->freeObject() ;
	 flatten((FrList*)head,end,remove_NILs) ;
	 }
      else if (head || !remove_NILs)
	 {
	 (*end)->replacd(list) ;
	 *end = list ;
	 }
      else
	 list->freeObject() ;
      }
   return ;
}

//----------------------------------------------------------------------

FrList *FrFlattenListInPlace(FrList *list, bool remove_NILs)
{
   if (!list || !list->consp())
      return list ;
   else // if (list->consp())
      {
      FrList *result = new FrList(0) ;
      FrList *end = result ;
      flatten(list,&end,remove_NILs) ;
      (void)poplist(result) ;
      return result ;
      }
}

// end of file frlist3.cpp //
