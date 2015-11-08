/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frnethsh.h    -- "virtual memory" frames over net (hash entry) */
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,1999,2001,2009			*/
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

#ifndef __FRNETHSH_H_INCLUDED
#define __FRNETHSH_H_INCLUDED

#if defined(__GNUC__)
#  pragma interface
#endif

/**********************************************************************/
/*	Definition of class HashEntryServer 			      */
/**********************************************************************/

class HashEntryServer : public FrHashEntry
   {
   private:
      static FrAllocator allocator ;
      FrSymbol *name ;	      // frame's name
   public:
#ifdef FrFRAME_ID
      long int frameID ;
#endif /* FrFRAME_ID */
      char deleted ;
      char delete_stored ;
      char locked ;
   public:
      void *operator new(size_t) { return allocator.allocate() ; }
      void operator delete(void *ent) { allocator.release(ent) ; }
      HashEntryServer() ;
      HashEntryServer(const FrSymbol *nm) ;
      virtual ~HashEntryServer() {}
      virtual FrHashEntryType entryType() const ;
      virtual FrSymbol *entryName() const ;
      virtual int sizeOf() const ;
      virtual size_t hashIndex(int size) const ;
      virtual int keycmp(const FrHashEntry *entry) const ;
      virtual bool prefixMatch(const char *nameprefix,int len) const ;
      virtual FrObject *copy() const ;
      FrSymbol *frameName() const { return name ; }
      void setName(const FrSymbol *newname) { name = (FrSymbol *)newname ; }
   } ;


#endif /* __FRNETHSH_H_INCLUDED */

// end of file frnethsh.h //
