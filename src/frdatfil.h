/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frdatfil.h    -- "virtual memory" frames using disk storage	*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,2001,2009				*/
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

#ifndef __FRDATFIL_H_INCLUDED
#define __FRDATFIL_H_INCLUDED

#ifndef __VFINFO_H_INCLUDED
#include "vfinfo.h"
#endif

#ifdef FrFRAME_ID
#include "frameid.h"
#endif

#if defined(__GNUC__)
#  pragma interface
#endif

/**********************************************************************/
/**********************************************************************/

/**********************************************************************/
/*      Declaration of class VFrameInfoFile                           */
/**********************************************************************/

class VFrameInfoFile : public VFrameInfo
   {
   private:
      DBFILE *db ;
#ifdef FrFRAME_ID
      FrameIdentDirectory *frame_IDs ;
#endif /* FrFRAME_ID */
   public:
      VFrameInfoFile(const char *file, bool transactions = false,
		     bool force_create = true, const char *password = 0) ;
      virtual ~VFrameInfoFile() ;
      virtual BackingStore backingStoreType() const ;
      virtual int backstorePresent() const ;
      virtual bool isFrame(const FrSymbol *frame) const ;
      virtual bool isDeletedFrame(const FrSymbol *frame) const ;
      virtual int lockFrame(const FrSymbol *frame) ;
      virtual int unlockFrame(const FrSymbol *frame) ;
      virtual bool isLocked(const FrSymbol *frame) const ;
      virtual VFrame *retrieveFrame(const FrSymbol *name) const ;
      virtual VFrame *retrieveFrameAsync(const FrSymbol *name, int &done) const ;
      virtual VFrame *retrieveOldFrame(FrSymbol *name, int generation)
		const ;
      virtual bool storeFrame(const FrSymbol *name) const ;
      virtual int syncFrames(frame_update_hookfunc *hook = 0) ;
      virtual int createFrame(const FrSymbol *name, bool upd_backstore = true) ;
      virtual int deleteFrame(const FrSymbol *name, bool upd_backstore = true) ;
      virtual bool renameFrame(const FrSymbol *oldname,
				 const FrSymbol *newname) ;
      virtual int proxyAdd(FrSymbol *frame, const FrSymbol *slot,
		           const FrSymbol *facet,
 			   const FrObject *filler) const ;
      virtual int proxyDel(FrSymbol *frame, const FrSymbol *slot,
		           const FrSymbol *facet,
			   const FrObject *filler) const ;
      virtual int startTransaction() ;
      virtual int endTransaction(int transaction) ;
      virtual int abortTransaction(int transaction) ;
      virtual void setNotify(VFrameNotifyType,VFrameNotifyFunc *)  ;
      virtual VFrameNotifyPtr getNotify(VFrameNotifyType) const ;
      virtual void setProxy(VFrameNotifyType,VFrameProxyFunc *)  ;
      virtual VFrameProxyPtr getProxy(VFrameNotifyType) const ;
      virtual void setShutdown(VFrameShutdownFunc *) ;
      virtual VFrameShutdownPtr getShutdown() const ;
      char *mainIndexFilename() const ;
      int extra_index_stream(int idxtype) const ;
      virtual long int lookupID(const FrSymbol *name) const ;
      virtual FrSymbol *lookupSym(long int frameID) const ;
      virtual bool getDBUserData(DBUserData *user_data) const ;
      virtual bool setDBUserData(DBUserData *user_data) ;
      virtual FrList *availableDatabases() const ;
      virtual int prefetchFrames(FrList *frames) ;
   } ;

#endif /* !__FRDATFIL_H_INCLUDED */

// end of file frdatfil.h //
