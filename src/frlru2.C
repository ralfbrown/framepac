/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frlru2.cpp	    discard of least-recently used frames       */
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,1998,2000,2006,2009		*/
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

#include <stdio.h>
#include "frsymtab.h"
#include "frframe.h"
#include "frlru.h"
#include "frpcglbl.h"

extern int (*FramepaC_discard_func)() ;

/**********************************************************************/
/*    Global variables limited to this module			      */
/**********************************************************************/

static int Initialized = 0 ;

#ifdef FrLRU_DISCARD
static bool discard_in_progress = false ;
#endif /* FrLRU_DISCARD */

/**********************************************************************/
/**********************************************************************/

#ifdef FrLRU_DISCARD

static bool discard_clean_frame(const FrSymbol *obj, FrNullObject, va_list args)
{
   FrVarArg(uint32_t,thres) ;
   FrVarArg(int *,discarded) ;
   FrFrame *frame = ((FrSymbol*)obj)->symbolFrame() ;
   // throw out the current frame if it is a clean virtual frame last used
   // before the threshold value of the clock
   if (frame && frame->isVFrame() && !frame->isLocked() &&
       !frame->dirtyFrame() && frame->getLRUclock() < thres)
      {
      frame->discard() ;	// discard the frame
      (*discarded)++ ;
      }
   return true ;
}

//----------------------------------------------------------------------

static bool discard_dirty_frame(const FrSymbol *obj, FrNullObject, va_list args)
{
   FrVarArg(uint32_t,thres) ;
   FrVarArg(int *,discarded) ;
   FrFrame *frame = ((FrSymbol*)obj)->symbolFrame() ;
   // throw out any and all unlocked virtual frames, even if dirty, that were
   // last used before the threshold value of the clock
   if (frame && frame->isVFrame() && !frame->isLocked() &&
       frame->getLRUclock() < thres)
      {
      if (frame->dirtyFrame())
	 frame->frameName()->commitFrame() ;
      frame->discard() ;	// discard the frame
      (*discarded)++ ;
      }
   return true ;
}

//----------------------------------------------------------------------

static int discard_oldest_frames(FrSymbolTable *symtab,va_list args)
{
   uint32_t oldest = symtab->oldestFrameClock() ;
   uint32_t newest = symtab->currentLRUclock() ;
   int total_frames = symtab->countFrames() ;
   int numframes = total_frames ;
   int discarded ;
   int total_discarded = 0 ;
   FrVarArg(int *,frames_discarded) ;
   do {
      uint32_t thres = oldest + (newest-oldest)/3 ;  // look at oldest third
      oldest = thres ;  // next time through, use next segment...
      discarded = 0 ;
      symtab->iterate(discard_clean_frame,thres,&discarded) ;
      if (numframes > 40 && total_discarded <= total_frames/4)
	 {
	 int trans ;
	 if (VFrame_Info)
	    trans = VFrame_Info->startTransaction() ;
	 else
	    trans = 0 ;
	 symtab->iterate(discard_dirty_frame,thres,&discarded) ;
	 if (VFrame_Info)
	    VFrame_Info->endTransaction(trans) ;
	 }
      total_discarded += discarded ;
      numframes -= discarded ;
      } while (numframes > 50 && total_discarded <= total_frames/4) ;
   *frames_discarded += total_discarded ;
   return true ;
}

#endif /* FrLRU_DISCARD */

/**********************************************************************/
/**********************************************************************/

int FramepaC_LRU_discard_frames()
{
   int discarded = 0 ;
#ifdef FrLRU_DISCARD
   if (discard_in_progress)
      return 0 ;
   discard_in_progress = true ;
   if (Initialized)
      do_all_symtabs(discard_oldest_frames, &discarded) ;
   if (discarded)
      {
      FrMessageVA("FramepaC: discarded %u virtual frames to free memory",
		  discarded) ;
      }
   discard_in_progress = false ;
#endif /* FrLRU_DISCARD */
   return discarded ;
}

//----------------------------------------------------------------------

void initialize_FramepaC_LRU()
{
   Initialized++ ;
   if (!FramepaC_discard_func)
      FramepaC_discard_func = FramepaC_LRU_discard_frames ;
}

//----------------------------------------------------------------------

void shutdown_FramepaC_LRU()
{
   --Initialized ;
}

// end of file frlru2.cpp //
