/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frsymfr2.cpp	       class FrSymbol methods for VFrames	*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,1998,1999,2000,2001,2009		*/
/*		 Ralf Brown/Carnegie Mellon University			*/
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

#include <stdlib.h>
#include <string.h>
#include "frsymbol.h"
#include "frpcglbl.h"

/**********************************************************************/
/*    FrSymbol method functions for supporting VFrames		      */
/**********************************************************************/
// these are included here rather than in frsymbol.cpp or frsymfrm.cpp
// because they are called internally, and would thus cause frsymfrm to
// be included in every executable

//----------------------------------------------------------------------

bool FrSymbol::isFrame()
{
   if (symbolFrame())
      return true ;
   else if (VFrame_Info)
      return VFrame_Info->isFrame(this) ;
   else
      return false ;
}

//----------------------------------------------------------------------

bool FrSymbol::isLocked() const
{
   FrFrame *fr = find_vframe_inline(this) ;

   if (fr)
      return fr->isLocked() ;
   else if (VFrame_Info)
      return VFrame_Info->isLocked(this) ;
   else
      return false ;
}

//----------------------------------------------------------------------

FrFrame *FrSymbol::lockFrame()
{
   FrFrame *fr = find_vframe_inline(this) ;

   if (fr && !fr->isLocked() && VFrame_Info)
      {
      if (VFrame_Info->lockFrame(this))
         fr->setLock(true) ;
      else
         {
	 FrWarningVA("unable to lock frame %s",symbolName()) ;
         return 0 ;
         }
      }
   return fr ;
}

//----------------------------------------------------------------------

bool FrSymbol::lockFrames(FrList *locklist)
{
   if (VFrame_Info)
      {
//!!!for now, just do one at a time
      while (locklist)
	 {
	 if (!((FrSymbol*)(locklist->first()))->lockFrame())
	    return false ;	// failed
	 locklist = locklist->rest() ;
	 }
      return true ;	// successful
      }
   else
      {
      while (locklist)
	 {
	 FrFrame *fr = ((FrSymbol*)(locklist->first()))->symbolFrame() ;
	 if (fr)
	    fr->setLock(true) ;
	 locklist = locklist->rest() ;
	 }
      return true ;   // successful
      }
}

//----------------------------------------------------------------------

bool FrSymbol::unlockFrame()
{
   FrFrame *fr = symbolFrame() ;
   // don't use find_vframe above, to avoid pulling in frame if not already
   // in memory

   if (fr && fr->isLocked() && VFrame_Info)
      {
      if (VFrame_Info->unlockFrame(this))
         fr->setLock(false) ;
      else
         return false ;
      }
   return true ;
}

//----------------------------------------------------------------------

int FrSymbol::deleteFrame()
{
   FrFrame *fr = symbolFrame() ;
   // don't use find_vframe to avoid pulling in frame if not already in memory

   if (fr)
      {
      fr->markDirty() ;		 // force the frame to be flushed
      if (VFrame_Info)
	 VFrame_Info->deleteFrame(this,true) ;
      delete fr ;
      }
   else if (VFrame_Info && VFrame_Info->isFrame(this))
      VFrame_Info->deleteFrame(this,true) ;
   return 0 ;
}

// end of file frsymfr2.cpp //
