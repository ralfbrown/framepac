/****************************** -*- C++ -*- *****************************/
/*									*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frslfreg.cpp	    class for self-registering obj instances	*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 2009 Ralf Brown/Carnegie Mellon University		*/
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
#include "frslfreg.h"

/************************************************************************/
/*	Helper functions						*/
/************************************************************************/

bool FrComparePriority(int prio1, int prio2, FrComparisonType sense,
		       bool *done)
{
   // 'done' assumes we are iterating down a list sorted by decreasing
   //   priority value and passing the list's priority as 'prio1', and is
   //   used to short-circuit the iteration
   switch (sense)
      {
      case FrComp_DontCare:
	 return true ;
      case FrComp_Equal:
	 if (done && prio1 < prio2) *done = true ;
	 return prio1 == prio2 ;
      case FrComp_Less:
	 return prio1 < prio2 ;
      case FrComp_LessEqual:
	 return prio1 <= prio2 ;
      case FrComp_Greater:
	 if (done && prio1 <= prio2) *done = true ;
	 return prio1 > prio2 ;
      case FrComp_GreaterEqual:
	 if (done && prio1 < prio2) *done = true ;
	 return prio1 >= prio2 ;
      case FrComp_NotEqual:
	 return prio1 != prio2 ;
      default:
	 FrMissedCase("FrComparePriority") ;
	 return false ;
      }
}

// end of file frselfreg.C //

