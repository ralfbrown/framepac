/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frameid.h    -- "unique" identifiers for virtual frames	*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,2009,2013				*/
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

#ifndef __FRAMEID_H_INCLUDED
#define __FRAMEID_H_INCLUDED

#ifndef __FRMEM_H_INCLUDED
#include "frmem.h"
#endif

/**********************************************************************/
/**********************************************************************/

// frameIDs are stored in a two-level array, with the first level
// providing pointers into smaller subarrays (each subarray fitting
// within one suballocation block [see FrALLOC_GRANULARITY and
// FrMemFooter in frmem.h] and containing the actual pointers to the
// symbol names).  The values shown here, combined with the current
// values of FrALLOC_GRANULARITY, yield a maximum supported database
// size of about 20,000 frames for MS-DOS, 800,000 for Watcom and
// Visual C++, and 4 million frames for other systems.

#ifdef __MSDOS__
#  define FRAME_IDENT_DIR_SIZE 20
#elif defined(__WATCOMC__) || (defined(_MSC_VER) && _MSC_VER >= 800)
#  define FRAME_IDENT_DIR_SIZE 200
#else
#  define FRAME_IDENT_DIR_SIZE 1000
#endif /* __MSDOS__ */

/**********************************************************************/
/**********************************************************************/

// maximum items we can store in a block without having to resort to large
// memory allocations
#define FrIDs_PER_BLOCK ((FrFOOTER_OFS / sizeof(FrSymbol*)) - 20)

typedef struct
   {
   const FrSymbol *m_frames[FrIDs_PER_BLOCK] ;
   int             m_freecount ;
   } FrameIDArray ;

struct FrameIdentDirectory
   {
   FrameIDArray *IDs[FRAME_IDENT_DIR_SIZE] ;
   } ;

/**********************************************************************/
/**********************************************************************/

long int allocate_frameID(FrameIdentDirectory *frIDs, const FrSymbol *name) ;
void deallocate_frameID(FrameIdentDirectory *frIDs, long int frameID) ;
bool set_frameID(FrameIdentDirectory *frame_IDs, long int ID,
	 	 const FrSymbol *name) ;

#endif /* __FRAMEID_H_INCLUDED */

// end of file frameid.h //
