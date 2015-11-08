/************************************************************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frdathsh.cpp	Hash entries for disk-based virtual frames	*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,1998,2001,2009			*/
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

#if defined(__GNUC__)
#  pragma implementation "frdathsh.h"
#endif

#include <stdlib.h>
#include "frpcglbl.h"
#include "frcmove.h"
#include "frdathsh.h"

/************************************************************************/
/*    Global variables for class HashEntryVFrame			*/
/************************************************************************/

FrAllocator HashEntryVFrame::allocator("Hash (disk)",sizeof(HashEntryVFrame)) ;

/**********************************************************************/
/*    Methods for class HashEntryVFrame				      */
/**********************************************************************/

HashEntryVFrame::HashEntryVFrame()
{
   name = 0 ;
#ifdef FrFRAME_ID
   frameID =
#endif /* FrFRAME_ID */
   oldoffset = offset = indexpos = 0 ;
   locked = deleted = undeleted = false ;
}

//----------------------------------------------------------------------

HashEntryVFrame::HashEntryVFrame(const FrSymbol *nm)
{
   name = (FrSymbol *)nm ;
#ifdef FrFRAME_ID
   frameID =
#endif /* FrFRAME_ID */
   oldoffset = offset = indexpos = 0 ;
   locked = deleted = undeleted = false ;
}

//----------------------------------------------------------------------

HashEntryVFrame::HashEntryVFrame(const FrSymbol *nm, long int ofs)
{
   name = (FrSymbol *)nm ;
   offset = ofs ;
#ifdef FrFRAME_ID
   frameID =
#endif /* FrFRAME_ID */
   oldoffset = indexpos = 0 ;
   locked = deleted = undeleted = false ;
}

//----------------------------------------------------------------------

HashEntryVFrame::HashEntryVFrame(const FrSymbol *nm, long int ofs,
				 long int idx)
{
   name = (FrSymbol *)nm ;
#ifdef FrFRAME_ID
   frameID =
#endif /* FrFRAME_ID */
   oldoffset = 0 ;
   offset = ofs ;
   indexpos = idx ;
   locked = deleted = undeleted = false ;
}

//----------------------------------------------------------------------

HashEntryVFrame::HashEntryVFrame(const FrSymbol *nm, long int ofs,
				 long int idx, long int frame_id)
{
   name = (FrSymbol *)nm ;
   oldoffset = 0 ;
   offset = ofs ;
   indexpos = idx ;
#ifdef FrFRAME_ID
   frameID = frame_id ;
#else
   (void)frame_id ;
#endif /* FrFRAME_ID */
   locked = deleted = undeleted = false ;
}

//----------------------------------------------------------------------

FrHashEntryType HashEntryVFrame::entryType() const
{
   return HE_VFrame ;
}

//----------------------------------------------------------------------

FrSymbol *HashEntryVFrame::entryName() const
{
   return name ;
}

//----------------------------------------------------------------------

int HashEntryVFrame::sizeOf() const
{
   return sizeof(HashEntryVFrame) ;
}

//----------------------------------------------------------------------

size_t HashEntryVFrame::hashIndex(int size) const
{
   unsigned long int val = (unsigned long)name ;

   return (size_t)(val % size) ;
}

//----------------------------------------------------------------------

int HashEntryVFrame::keycmp(const FrHashEntry *entry) const
{
   if (entry == 0 || entry->entryType() != HE_VFrame)
      return -1 ;
   FrSymbol *name2 = ((HashEntryVFrame*)entry)->name ;
   return FrCompare(name,name2) ;
}

//----------------------------------------------------------------------

bool prefix_match(const FrSymbol *symname, const char *nameprefix, int len)
{
   return symname ? memcmp(symname->symbolName(),nameprefix,len) == 0 : false ;
}

bool HashEntryVFrame::prefixMatch(const char *nameprefix,int len) const
{
   if (deleted)
      return false ;
   else if (name)
      return (memcmp(name->symbolName(),nameprefix,len) == 0) ;
   else
      return false ;
}

//----------------------------------------------------------------------

FrObject *HashEntryVFrame::copy() const
{
   HashEntryVFrame *result = new HashEntryVFrame ;

   if (result)
      {
      result->name = name ;
      result->offset = offset ;
      result->oldoffset = oldoffset ;
      result->indexpos = indexpos ;
#ifdef FrFRAME_ID
      result->frameID = frameID ;
#endif /* FrFRAME_ID */
      result->deleted = deleted ;
      result->undeleted = undeleted ;
      result->locked = locked ;
      }
   return result ;
}

// end of file frdathsh.cpp //
