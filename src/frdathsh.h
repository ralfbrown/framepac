/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frdathsh.h	class HashEntryVFrame				*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,2001,2009,2015			*/
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

#ifndef __FRDATHSH_H_INCLUDED
#define __FRDATHSH_H_INCLUDED

#ifndef __FRHASHT_H_INCLUDED
#include "frhasht.h"
#endif

#if defined(__GNUC__)
#  pragma interface
#endif

/**********************************************************************/
/*	Definition of class HashEntryVFrame 			      */
/**********************************************************************/

class HashEntryVFrame : public FrHashEntry
   {
   private:
      static FrAllocator allocator ;
      FrSymbol *name ;	      // frame's name
      long int offset ;	      // location of frame in database file
      long int oldoffset ;    // location of previous version of frame
      long int indexpos ;     // location of entry in index file
   public:
#ifdef FrFRAME_ID
      long int frameID ;
#endif /* FrFRAME_ID */
      char deleted ;
      char undeleted ;
      char locked ;
   //public member functions
      void *operator new(size_t) { return allocator.allocate() ; }
      void operator delete(void *ent) { allocator.release(ent) ; }
      HashEntryVFrame() ;
      HashEntryVFrame(const FrSymbol *nm) ;
      HashEntryVFrame(const FrSymbol *nm, long int ofs) ;
      HashEntryVFrame(const FrSymbol *nm, long int ofs, long int idxpos) ;
      HashEntryVFrame(const FrSymbol *nm, long int ofs, long int idxpos,
		      long int frame_id) ;
      virtual ~HashEntryVFrame() {}
      virtual FrHashEntryType entryType() const ;
      virtual FrSymbol *entryName() const ;
      virtual int sizeOf() const ;
      virtual size_t hashIndex(int size) const ;
      virtual int keycmp(const FrHashEntry *entry) const ;
      virtual bool prefixMatch(const char *nameprefix,int len) const ;
      virtual FrObject *copy() const ;
      FrSymbol *frameName() const { return name ; }
      void setName(const FrSymbol *newname) { name = (FrSymbol *)newname ; }
      long int frameOffset() const { return offset ; }
      long int oldFrameOffset() const { return oldoffset ; }
      void setOffset(long int ofs) { oldoffset = offset ; offset = ofs ; }
      long int indexPosition() const { return indexpos ; }
      void setPosition(long int ofs) { indexpos = ofs ; }
   } ;

#endif /* !__FRDATHSH_H_INCLUDED */

// end of file frdathsh.h //
