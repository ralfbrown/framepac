/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File vfinfo.h    -- "virtual memory" backing-store support          */
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,2009,2015				*/
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

#ifndef __VFINFO_H_INCLUDED
#define __VFINFO_H_INCLUDED

#ifndef __VFRAME_H_INCLUDED
#include "vframe.h"
#endif

#ifndef __FRSYMTAB_H_INCLUDED
#include "frsymtab.h"
#endif

#ifndef __FRHASH_H_INCLUDED
#include "frhash.h"
#endif

/**********************************************************************/
/*      Declaration of class VFrameInfo                               */
/**********************************************************************/

// an abstract base class from which we'll specialize for the different
// types of backing store
class VFrameInfo : public FrObject
   {
   protected:
      FrSymHashTable *hash ;
   public:
      VFrameNotifyFunc *create_handler, *delete_handler, *update_handler ;
      VFrameNotifyFunc *lock_handler, *unlock_handler ;
      VFrameShutdownFunc *shutdown_handler ;
      VFrameProxyFunc *proxyadd_handler, *proxydel_handler ;
   protected: //data
      int active_transactions ;
      bool use_transactions ;
   private: // data
      bool readonly ;
   protected: // methods
      void setReadOnly(bool ro) { readonly = ro ; }
   public:
      VFrameInfo() { active_transactions = 0 ; }
      virtual ~VFrameInfo() {}
      void *operator new(size_t size) { return FrMalloc(size) ; }
      void operator delete(void *obj) { FrFree(obj) ; }
      virtual FrObjectType objType() const ;
      virtual const char *objTypeName() const ;
      virtual FrObjectType objSuperclass() const ;
//    virtual ostream &printValue(ostream &output) const ;
//    virtual char *displayValue(char *buffer) const ;
//    virtual size_t displayLength() const ;
      virtual bool isFrame(const FrSymbol *) const ;
      virtual bool isDeletedFrame(const FrSymbol *) const ;
      virtual BackingStore backingStoreType() const = 0 ;
      virtual int backstorePresent() const = 0 ;
      virtual int lockFrame(const FrSymbol *frame) = 0 ;
      virtual int unlockFrame(const FrSymbol *frame) = 0 ;
      virtual bool isLocked(const FrSymbol *frame) const = 0 ;
      virtual VFrame *retrieveFrame(const FrSymbol *name) const = 0 ;
      virtual VFrame *retrieveFrameAsync(const FrSymbol *name, int &done) const = 0;
      virtual VFrame *retrieveOldFrame(FrSymbol *name,int generation)
 		const = 0 ;
      virtual bool storeFrame(const FrSymbol *name) const = 0 ;
      virtual int syncFrames(frame_update_hookfunc *hook = 0) = 0 ;
      virtual int createFrame(const FrSymbol *name,
			      bool upd_backstore = true) = 0 ;
      virtual int deleteFrame(const FrSymbol *name,
			      bool upd_backstore = true) = 0 ;
      virtual bool renameFrame(const FrSymbol *oldname,
			       const FrSymbol *newname) = 0 ;
      virtual int proxyAdd(FrSymbol *frame, const FrSymbol *slot,
		           const FrSymbol *facet,
 			   const FrObject *filler) const = 0 ;
      virtual int proxyDel(FrSymbol *frame, const FrSymbol *slot,
		           const FrSymbol *facet,
			   const FrObject *filler) const = 0 ;
      virtual int startTransaction() = 0 ;
      virtual int endTransaction(int transaction) = 0 ;
      virtual int abortTransaction(int transaction) = 0 ;
      virtual void setNotify(VFrameNotifyType,VFrameNotifyFunc *) = 0 ;
      virtual VFrameNotifyPtr getNotify(VFrameNotifyType) const = 0 ;
      virtual void setProxy(VFrameNotifyType,VFrameProxyFunc *) = 0 ;
      virtual VFrameProxyPtr getProxy(VFrameNotifyType) const = 0 ;
      virtual void setShutdown(VFrameShutdownFunc *) = 0 ;
      virtual VFrameShutdownPtr getShutdown() const = 0 ;
      virtual long int lookupID(const FrSymbol *name) const = 0 ;
      virtual FrSymbol *lookupSym(long int frameID) const = 0 ;
      virtual bool getDBUserData(DBUserData *user_data) const = 0 ;
      virtual bool setDBUserData(DBUserData *user_data) = 0 ;
      virtual FrList *availableDatabases() const = 0 ;
      bool inTransaction() const
	    { return (bool)(active_transactions != 0) ; }
      bool isReadOnly() const { return readonly ; }
      FrList *prefixMatches(const char *prefix) const ;
      char *completionFor(const char *prefix) const ;
      virtual int prefetchFrames(FrList *frames) ;
   } ;


#endif /* !__VFINFO_H_INCLUDED */

// end of file vfinfo.h //
