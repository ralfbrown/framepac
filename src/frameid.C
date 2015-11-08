/************************************************************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frameid.cpp     "unique" identifiers for virtual frames	*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,2009,2013,2015			*/
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

#include "vframe.h"
#include "frameid.h"

#define NO_FRAME_ID (-1L)

/**********************************************************************/
/*	Frame Identifier functions				      */
/**********************************************************************/

bool set_frameID(FrameIdentDirectory *frame_IDs, long int ID,
	 	 const FrSymbol *name)
{
   int dirnum = (int)(ID / FrIDs_PER_BLOCK) ;
   int entnum = (int)(ID % FrIDs_PER_BLOCK) ;
   if (!frame_IDs || dirnum >= FRAME_IDENT_DIR_SIZE)
      return false ;
   FrameIDArray *dir = frame_IDs->IDs[dirnum] ;
   if (!dir)
      {
      dir = frame_IDs->IDs[dirnum] = new FrameIDArray ;
      if (!dir)
	 return false ;
      dir->m_freecount = FrIDs_PER_BLOCK-1 ;
      for (unsigned int i = 0 ; i < FrIDs_PER_BLOCK ; i++)
	 {
	 dir->m_frames[i] = nullptr ;
	 }
      }
   else if (dir->m_frames[entnum] == 0)
      dir->m_freecount-- ;
   dir->m_frames[entnum] = name ;
   return true ;
}

//----------------------------------------------------------------------

#ifdef FrFRAME_ID
long int allocate_frameID(FrameIdentDirectory *frame_IDs, const FrSymbol *name)
{
   long int ID = NO_FRAME_ID ;
   int dirnum ;

   if (!frame_IDs)
      return NO_FRAME_ID ;    // no frame ID because we are not tracking them
   for (dirnum = 0 ; dirnum < FRAME_IDENT_DIR_SIZE ; dirnum++)
      {
      FrameIDArray *dir = frame_IDs->IDs[dirnum] ;
      if (dir == 0)
	 break ;
      if (dir->m_freecount)
	 {
	 unsigned int entrynum ;
	 const FrSymbol **ent = dir->m_frames ;
	 for (entrynum = 0 ; entrynum < FrIDs_PER_BLOCK ; entrynum++)
	    if (ent[entrynum] == 0)
	       break ;
	 if (entrynum < FrIDs_PER_BLOCK)
	    {
	    ent[entrynum] = name ;
	    ID = (dirnum*FrIDs_PER_BLOCK) + entrynum ;
	    dir->m_freecount-- ;
	    break ;
	    }
	 else
	    dir->m_freecount = 0 ;
	 }
      }
   if (ID == NO_FRAME_ID && dirnum < FRAME_IDENT_DIR_SIZE)
      {
      FrameIDArray *newdir = new FrameIDArray ;
      newdir->m_freecount = FrIDs_PER_BLOCK-1 ;
      for (unsigned int i = 1 ; i < FrIDs_PER_BLOCK ; i++)
	 {
	 newdir->m_frames[i] = nullptr ;
	 }
      newdir->m_frames[0] = name ;
      frame_IDs->IDs[dirnum] = newdir ;
      ID = (dirnum*FrIDs_PER_BLOCK) ;
      }
   return ID ;
}
#endif /* FrFRAME_ID */

//----------------------------------------------------------------------

#ifdef FrFRAME_ID
void deallocate_frameID(FrameIdentDirectory *frame_IDs, long int frameID)
{
   if (!frame_IDs)
      return ;
   int dirnum = (int)(frameID / FrIDs_PER_BLOCK) ;
   if (dirnum >= 0 && dirnum < FRAME_IDENT_DIR_SIZE && frame_IDs)
      {
      FrameIDArray *dir = frame_IDs->IDs[dirnum] ;
      if (dir)
	 {
	 dir->m_frames[(int)(frameID % FrIDs_PER_BLOCK)] = nullptr ;
	 dir->m_freecount++ ;
	 }
      }
}
#endif /* !FrFRAME_ID */



// end of file frameid.cpp //
