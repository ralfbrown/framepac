/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC -- frame manipulation in C++				*/
/*  Version 2.01 							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frhasht.h	 class FrHashTableOld				*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,1999,2001,2008,2009,2015		*/
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

#ifndef __FRHASHT_H_INCLUDED
#define __FRHASHT_H_INCLUDED

#ifndef __FRAMERR_H_INCLUDED
#include "framerr.h"
#endif

#ifndef __FROBJECT_H_INCLUDED
#include "frobject.h"
#endif

#ifndef __FRMEM_H_INCLUDED
#include "frmem.h"
#endif

#ifndef __FRREADER_H_INCLUDED
#include "frreader.h"
#endif

#if defined(__GNUC__)
#  pragma interface
#endif

/************************************************************************/
/*    Declarations for class FrHashTableOld			     	*/
/************************************************************************/

#define DEFAULT_HASHTABLE_SIZE 1023
#define HASHTABLE_FILL_FACTOR  2    /* avg 2 items per bucket */
#define HASHTABLE_MIN_INCREMENT 16
#define MAX_HASHTABLE_SIZE (UINT_MAX / HASHTABLE_FILL_FACTOR / sizeof (FrHashEntry *))

enum FrHashEntryType
   {
   HE_none,
   HE_base,
   HE_FrObject,
   HE_VFrame,
   HE_Server
   } ;

class FrHashEntry : public FrObject
   {
   private:
      //
   public:
      FrHashEntry *next ;
      bool in_use ;
   public:
      void *operator new(size_t size) { return FrMalloc(size) ; }
      void operator delete(void *obj) { FrFree(obj) ; }
      FrHashEntry() { next = 0 ; in_use = false ; }
      virtual ~FrHashEntry() ;
      virtual bool hashp() const ;
      virtual FrObjectType objType() const ;
      virtual const char *objTypeName() const ;
      virtual FrObjectType objSuperclass() const ;
      virtual FrHashEntryType entryType() const ;
      virtual FrSymbol *entryName() const ;
      virtual int sizeOf() const ;
      virtual size_t hashIndex(int size) const ;
      virtual int keycmp(const FrHashEntry *entry) const ;
      virtual FrObject *copy() const ;
      virtual FrObject *deepcopy() const ;
   } ;

/************************************************************************/
/************************************************************************/

//class FrHashEntryObject is now obsolete -- use FrObjHashTable and FrObjHashEntry
//   instead of FrHashTableOld and FrHashEntryObject

/************************************************************************/
/************************************************************************/

typedef FrHashEntry *FrHashEntryPtr ;

#endif /* !__FRHASHT_H_INCLUDED */

// end of file frhasht.h //
