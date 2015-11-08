/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frnethsh.cpp	 "virtual memory" frames on net (hash entries)	*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,1998,2000,2001,2002,2003,2004,	*/
/*		2006,2009,2015 Ralf Brown/Carnegie Mellon University	*/
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
#  pragma implementation "frnethsh.h"
#endif

#include "frcmove.h"
#include "frpcglbl.h"
#include "frhash.h"
#include "frhasht.h"
#include "frnethsh.h"

#ifdef FrFRAME_ID
#include "frameid.h"
#endif

#ifdef FrSTRICT_CPLUSPLUS
#  include <iomanip>
#else
#  include <iomanip.h>
#endif /* FrSTRICT_CPLUSPLUS */


/**********************************************************************/
/*    Global variables for class HashEntryServer		      */
/**********************************************************************/

FrAllocator HashEntryServer::allocator("Hash (srvr)",sizeof(HashEntryServer)) ;

/**********************************************************************/
/*    Methods for class HashEntryServer				      */
/**********************************************************************/

static bool print_hash_entry(const FrSymbol *frame, FrObject *ent, va_list args)
{
   FrVarArg(ostream *,out) ;
   *out << frame << "  "  << hex << (uintptr_t)ent << dec << endl ;
   return true ;
}

void print_hash_table(FrSymHashTable *hash, ostream &out)
{
   out << "Hash Table contents:" << endl ;
   hash->iterate(print_hash_entry,&out) ;
   out << "--------------" << endl ;
   return ;
}

//----------------------------------------------------------------------

HashEntryServer::HashEntryServer()
{
   name = 0 ;
   deleted = delete_stored = false ;
#ifdef FrFRAME_ID
   frameID = NO_FRAME_ID ;
#endif /* FrFRAME_ID */
   return ;
}

//----------------------------------------------------------------------

HashEntryServer::HashEntryServer(const FrSymbol *nm)
{
   name = (FrSymbol *)nm ;
   deleted = delete_stored = false ;
   return ;
}

//----------------------------------------------------------------------

FrHashEntryType HashEntryServer::entryType() const
{
   return HE_Server ;
}

//----------------------------------------------------------------------

FrSymbol *HashEntryServer::entryName() const
{
   return name ;
}

//----------------------------------------------------------------------

int HashEntryServer::sizeOf() const
{
   return sizeof(HashEntryServer) ;
}

//----------------------------------------------------------------------

size_t HashEntryServer::hashIndex(int size) const
{
   unsigned long int val = (unsigned long)name ;

   return (size_t)(val % size) ;
}

//----------------------------------------------------------------------

int HashEntryServer::keycmp(const FrHashEntry *entry) const
{
   FrSymbol *name2 ;

   if (entry == 0 || entry->entryType() != HE_Server)
      return -1 ;
   name2 = ((HashEntryServer*)entry)->name ;
   return FrCompare(name,name2) ;
}

//----------------------------------------------------------------------

bool HashEntryServer::prefixMatch(const char *nameprefix,int len) const
{
   return name ? (memcmp(name->symbolName(),nameprefix,len) == 0) : false ;
}

//----------------------------------------------------------------------

FrObject *HashEntryServer::copy() const
{
   HashEntryServer *result = new HashEntryServer ;

   if (result)
      {
      result->name = name ;
      result->deleted = deleted ;
      result->delete_stored = delete_stored ;
#ifdef FrFRAME_ID
      result->frameID = frameID ;
#endif /* FrFRAME_ID */
      }
   return result ;
}

// end of file frnethsh.cpp //
