/************************************************************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frdatfil.cpp	 "virtual memory" frames using disk storage     */
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,2000,2001,2009,2015		*/
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
#  pragma implementation "frdatfil.h"
#endif

#include <stdlib.h>
#include <string.h>
#include "frdatfil.h"
#include "frstring.h"
#include "frlru.h"
#include "frpcglbl.h"
#include "frdathsh.h"
#include "frfinddb.h"
#include "mikro_db.h"

/************************************************************************/
/*    Global data for this module					*/
/************************************************************************/

static const char errmsg_retrieve_frame[]
     = "error retrieving frame from database" ;

/************************************************************************/
/*    VFrame member and associated functions				*/
/************************************************************************/

char *VFrames_indexfile()
{
#ifdef FrDATABASE
   if (VFrame_Info && VFrame_Info->backingStoreType() == BS_diskfile)
      return ((VFrameInfoFile *)VFrame_Info)->mainIndexFilename() ;
#endif /* FrDATABASE */
   return 0 ;
}

//----------------------------------------------------------------------

int VFrames_indexstream(int idxtype)
{
   (void)idxtype ;
#ifdef FrDATABASE
#  ifdef FrEXTRA_INDEXES
   if (VFrame_Info && VFrame_Info->backingStoreType() == BS_diskfile)
      return ((VFrameInfoFile *)VFrame_Info)->extra_index_stream(idxtype) ;
#  endif /* FrEXTRA_INDEXES */
#endif /* FrDATABASE */
   return 0 ;
}

//----------------------------------------------------------------------

void VFrames_set_indexes(bool byslot, bool byfiller, bool byword)
{
#ifdef FrDATABASE
   FramepaC_select_extra_indexes(byslot,byfiller,byword) ;
#else
   (void)byslot ; (void)byfiller ; (void)byword ;
#endif /* FrDATABASE */
}

//----------------------------------------------------------------------

void VFrames_get_indexes(bool *byslot, bool *byfiller, bool *byword)
{
#ifdef FrDATABASE
   FramepaC_determine_extra_indexes(byslot,byfiller,byword) ;
#else
   if (byslot) *byslot = false ;
   if (byfiller) *byfiller = false ;
   if (byword) *byword = false ;
#endif /* FrDATABASE */
}

/**********************************************************************/
/*    Member functions for class VFrameInfoFile			      */
/**********************************************************************/

#ifdef FrDATABASE

VFrameInfoFile::VFrameInfoFile(const char *file, bool transactions,
			       bool force_creat, const char *password)
{
   FrString *filename ;
   DBFILE *dbfile ;

   if (FramepaC_get_db_dir() && !strchr(file,'/')
#ifdef FrMSDOS_PATHNAMES
       && !strchr(file,'\\')
#endif /* FrMSDOS_PATHNAMES */
      )
      {
      filename = new FrString(FramepaC_get_db_dir()) ;
      filename->append("/") ;
      filename->append(file) ;
      }
   else
      filename = new FrString(file) ;
#ifdef FrFRAME_ID
   frame_IDs = FrNew(FrameIdentDirectory) ;
   for (int dirnum = 0 ; dirnum < FRAME_IDENT_DIR_SIZE ; dirnum++)
      frame_IDs->IDs[dirnum] = 0 ;
#endif /* FrFRAME_ID */
   VFrame_Info = this ;
   dbfile = open_database((char*)filename->stringValue(),force_creat,
			  transactions, password
#ifdef FrFRAME_ID
			  ,frame_IDs
#endif /* FrFRAME_ID */
			  ) ;
   db = dbfile ;
   use_transactions = transactions ;
   if (!dbfile)
      {
      hash = 0 ;
      if (force_creat)
         FrWarning("Unable to open database file; no virtual frames will be available.") ;
      }
   else
      {
      setReadOnly(dbfile->readonly) ;
      hash = dbfile->index ;
      load_db_configuration(db) ;
      }
}

//----------------------------------------------------------------------

VFrameInfoFile::~VFrameInfoFile()
{
   if (db)
      {
      if (close_database(db) == -1)
         FrWarning("Error while closing database file") ;
      db = 0 ;
#ifdef FrLRU_DISCARD
      shutdown_FramepaC_LRU() ;
#endif /* FrLRU_DISCARD */
      }
#ifdef FrFRAME_ID
   if (frame_IDs)
      {
      for (int dirnum = 0 ; dirnum < FRAME_IDENT_DIR_SIZE ; dirnum++)
	 if (frame_IDs->IDs[dirnum])
	    delete frame_IDs->IDs[dirnum] ;
      FrFree(frame_IDs) ;
      frame_IDs = 0 ;
      }
#endif /* FrFRAME_ID */
}

//----------------------------------------------------------------------

BackingStore VFrameInfoFile::backingStoreType() const
{
   return BS_diskfile ;
}

//----------------------------------------------------------------------

int VFrameInfoFile::backstorePresent() const
{
   return (db != 0) ;
}

//----------------------------------------------------------------------

bool VFrameInfoFile::isFrame(const FrSymbol *name) const
{
   if (hash)
      {
      FrObject *info ;
      (void)hash->lookup(name,&info) ;
      return (info && !((HashEntryVFrame*)info)->deleted) ;
      }
   else
      return false ;
}

//----------------------------------------------------------------------

bool VFrameInfoFile::isDeletedFrame(const FrSymbol *name) const
{
   if (hash)
      {
      FrObject *info ;
      (void)hash->lookup(name,&info) ;
      return (info && ((HashEntryVFrame*)info)->deleted) ;
      }
   else
      return false ;
}

//----------------------------------------------------------------------

bool VFrameInfoFile::isLocked(const FrSymbol *frame) const
{
   if (hash)
      {
      FrObject *info ;
      hash->lookup(frame,&info) ;
      return info ? (bool)((HashEntryVFrame*)info)->locked : false ;
      }
   else
      return false ;
}

//----------------------------------------------------------------------

int VFrameInfoFile::lockFrame(const FrSymbol *frame)
{
   if (isReadOnly())
      return true ;	// silently ignore if read-only database
   if (hash)
      {
      FrObject *info ;
      (void)hash->lookup(frame,&info) ;
      if (!info)
	 return false ;
      ((HashEntryVFrame*)info)->locked = true ;
      }
   return true ;
}

//----------------------------------------------------------------------

int VFrameInfoFile::unlockFrame(const FrSymbol *frame)
{
   if (isReadOnly())
      return true ;	// silently ignore if read-only database
   if (hash)
      {
      FrObject *info ;
      (void)hash->lookup(frame,&info) ;
      if (!info)
         return false ;
      else
         ((HashEntryVFrame*)info)->locked = false ;
      }
   return true ;
}

//----------------------------------------------------------------------

VFrame *VFrameInfoFile::retrieveFrame(const FrSymbol *name) const
{
   VFrame *fr ;
   bool del ;

   if (db)
      {
      char *rep ;
      FrObject *info ;
      db->index->lookup(name,&info) ;
      HashEntryVFrame *entry = (HashEntryVFrame *)info ;
      if (!entry || entry->deleted)
         return 0 ;
      rep = read_database(db,entry,&del) ;
      if (del)
         return 0 ;			  // frame no longer exists
      if (rep)
	 {
	 fr = load_frame(rep,entry->locked) ;
         FrFree(rep) ;
	 return fr ;
	 }
      else
	 FrWarning(errmsg_retrieve_frame) ;
      }
   return 0 ;	// frame does not exist in backing store
}

//----------------------------------------------------------------------

VFrame *VFrameInfoFile::retrieveFrameAsync(const FrSymbol *name, int &done) const
{
   VFrame *fr ;

   done = 0 ;
   fr = retrieveFrame(name) ;
   done = (fr ? 1 : -1) ;
   return fr ;
}

//----------------------------------------------------------------------

VFrame *VFrameInfoFile::retrieveOldFrame(FrSymbol *name, int generation)
const
{
   FrFrame *fr ;

   if (db && generation >= 0)
      {
      char *rep ;
      FrObject *info ;
      (void)db->index->lookup(name,&info) ;
      HashEntryVFrame *entry = (HashEntryVFrame *)info ;

      if (!entry)
         return 0 ;
      fr = find_vframe_inline(name) ;
      Fr_errno = 0 ;
      rep = read_old_entry(db,entry,generation) ;
      if (rep && rep[0] == '[')
	 {
	 if (fr)
	    delete fr ;  // make sure the old links are removed first
	 // create the frame explicitly before attempting to convert, or
	 // we'll wind up recursively calling ourselves to retrieve the
	 // frame being converted as a result of retrieving the frame....
	 fr = FramepaC_new_VFrame ? FramepaC_new_VFrame(name)
				  : new FrFrame(name) ;
	 const char *framerep = rep ;
	 string_to_Frame(framerep) ;
	 fr->setLock(entry->locked) ;
	 FrFree(rep) ;
	 return (VFrame *)fr ;
	 }
      else if (Fr_errno != 0)
	 FrWarning(errmsg_retrieve_frame) ;
      }
   return 0 ;	// requested version of frame does not exist in backing store
}

//----------------------------------------------------------------------

bool VFrameInfoFile::renameFrame(const FrSymbol *oldname,
				   const FrSymbol *newname)
{
   if (isReadOnly() || createFrame(newname,true) == -1)
      return false ;
   FrFrame *fr = find_vframe_inline(newname) ;
   if (fr)
      fr->markDirty() ;
   if (!storeFrame(newname) || deleteFrame(oldname) == -1)
      return false ;
   return true ; // successful
}

//----------------------------------------------------------------------

bool VFrameInfoFile::storeFrame(const FrSymbol *name) const
{
   if (isReadOnly() || !name->symbolFrame())
      return true ;	// silently ignore if read-only database
   bool result = write_database_entry(db,name) ;
   if (result)
      name->symbolFrame()->markDirty(false) ;
   return result ;
}

//----------------------------------------------------------------------

// actually int update_database_record(FrHashEntry *ent, DBFILE *db, bool force,
//				       bool sync, frame_update_hookfunc *),
// but need varargs list due to the way it is called
static bool sync_frame(const FrSymbol *name, FrObject *value, va_list args)
{
   FrHashEntry *ent = (FrHashEntry*)value ;
   FrVarArg(DBFILE *,db) ;
   FrVarArg2(bool,int,force) ;
   FrVarArg2(bool,int,synch) ;
   FrVarArg(frame_update_hookfunc *,hook) ;
   if (force ||
       (name->symbolFrame() && name->symbolFrame()->dirtyFrame())
      )
      {
      if (hook)
         hook((FrSymbol*)name) ;
      int result = update_database_record(ent,db,force,synch) ;
      if (result)
         name->symbolFrame()->markDirty(false);
      return result != false ;	
      }
   return true ;
}

//----------------------------------------------------------------------

int VFrameInfoFile::syncFrames(frame_update_hookfunc *hook)
{
   if (isReadOnly())
      return 0 ;	// silently ignore if read-only database
   int trans = startTransaction() ;
   if (hash->iterate(sync_frame,db,false,false,hook) &&
       flush_database_file(db))
      {
      return endTransaction(trans) ; // successful
      }
   else
      {
      abortTransaction(trans) ;
      return -1 ;	// failed
      }
}

//----------------------------------------------------------------------

int VFrameInfoFile::createFrame(const FrSymbol *name, bool upd_backstore)
{
   if (db)
      {
      HashEntryVFrame *entry = new HashEntryVFrame ;
      if (entry)
	 {
	 entry->deleted = false ;
	 entry->undeleted = true ;
#ifdef FrFRAME_ID
	 entry->frameID = allocate_frameID(frame_IDs, name) ;
#endif /* FrFRAME_ID */
	 if (db->index->add(name,entry))
	    delete entry ;
	 }
      if (upd_backstore && !isReadOnly())
	 {
	 if (log_frame_create(db,name) == -1)
	    return -1 ;
	 }
      }
   return 0 ; // successful
}

//----------------------------------------------------------------------

int VFrameInfoFile::deleteFrame(const FrSymbol *name, bool upd_backstore)
{
   if (db)
      {
      FrObject *info ;
      (void)db->index->lookup(name,&info) ;
      HashEntryVFrame *entry = (HashEntryVFrame *)info ;
      if (entry)
	 {
         entry->deleted = true ;
	 entry->undeleted = false ;
#ifdef FrFRAME_ID
	 deallocate_frameID(frame_IDs,entry->frameID) ;
#endif /* FrFRAME_ID */
	 if (upd_backstore && !isReadOnly())
	    {
	    if (log_frame_delete(db,name) == -1)
	       return -1 ;
//!!!
	    if (!update_database_record(entry,db,false,true))
	       return -1 ;
	    }
	 }
      }
   return 0 ; // successful
}

//----------------------------------------------------------------------

int VFrameInfoFile::proxyAdd(FrSymbol *frame, const FrSymbol *slot,
		    	     const FrSymbol *facet,
			     const FrObject *filler) const
{
   // Since only a single process can update a local database file, we know
   // that any lock which might be held is held by the current program.
   // Therefore, just go ahead and add the filler.
#ifdef FrSYMBOL_RELATION
   FrSymbol *inv = slot->inverseRelation() ;
   ((FrSymbol*)slot)->undefineRelation() ;
#endif /* FrSYMBOL_RELATION */
   frame->addFiller(slot,facet,filler) ;
#ifdef FrSYMBOL_RELATION
   ((FrSymbol*)slot)->defineRelation(inv) ;
#endif /* FrSYMBOL_RELATION */
   return 0 ;  // successful
}

//----------------------------------------------------------------------

int VFrameInfoFile::proxyDel(FrSymbol *frame, const FrSymbol *slot,
			     const FrSymbol *facet,
			     const FrObject *filler) const
{
   // Since only a single process can update a local database file, we know
   // that any lock which might be held is held by the current program.
   // Therefore, just go ahead and remove the filler.
#ifdef FrSYMBOL_RELATION
   FrSymbol *inv = slot->inverseRelation() ;
   ((FrSymbol*)slot)->undefineRelation() ;
#endif /* FrSYMBOL_RELATION */
   frame->eraseFiller(slot,facet,filler) ;
#ifdef FrSYMBOL_RELATION
   ((FrSymbol*)slot)->defineRelation(inv) ;
#endif /* FrSYMBOL_RELATION */
   return 0 ;  // successful
}

//----------------------------------------------------------------------

int VFrameInfoFile::startTransaction()
{
   if (isReadOnly())
      return 0 ;	// silently ignore if read-only database
   return db->logfile>0 ? start_database_transaction(db) : 0 ;
}

//----------------------------------------------------------------------

int VFrameInfoFile::endTransaction(int transaction)
{
   if (isReadOnly())
      return 0 ;	// silently ignore if read-only database
   return db->logfile>0 ? end_database_transaction(db,transaction) : 0 ;
}

//----------------------------------------------------------------------

int VFrameInfoFile::abortTransaction(int transaction)
{
   if (isReadOnly())
      return 0 ;	// silently ignore if read-only database
   return db->logfile>0 ? abort_database_transaction(db,transaction) : 0 ;
}

//----------------------------------------------------------------------

void VFrameInfoFile::setNotify(VFrameNotifyType type,VFrameNotifyFunc *)
{
   if (type == VFNot_CREATE || type == VFNot_DELETE || type == VFNot_UPDATE ||
       type == VFNot_LOCK || type == VFNot_UNLOCK)
      return ;
   else
      FrProgError("invalid notification type given to setNotify") ;
}

//----------------------------------------------------------------------

VFrameNotifyPtr VFrameInfoFile::getNotify(VFrameNotifyType type) const
{
   if (type == VFNot_CREATE || type == VFNot_DELETE || type == VFNot_UPDATE ||
       type == VFNot_LOCK || type == VFNot_UNLOCK)
      return 0 ;
   else
      {
      FrProgError("invalid notification type given to getNotify") ;
      return 0 ;
      }
}

//----------------------------------------------------------------------

void VFrameInfoFile::setProxy(VFrameNotifyType type,VFrameProxyFunc *)
{
   if (type == VFNot_PROXYADD || type == VFNot_PROXYDEL)
      return ;
   else
      FrProgError("invalid proxy handler type given to setProxy") ;
}

//----------------------------------------------------------------------

VFrameProxyPtr VFrameInfoFile::getProxy(VFrameNotifyType type) const
{
   if (type == VFNot_PROXYADD || type == VFNot_PROXYDEL)
      return 0 ;
   else
      {
      FrProgError("invalid proxy handler type given to getProxy") ;
      return 0 ;
      }
}

//----------------------------------------------------------------------

void VFrameInfoFile::setShutdown(VFrameShutdownFunc *)
{
   return ;
}

//----------------------------------------------------------------------

VFrameShutdownPtr VFrameInfoFile::getShutdown() const
{
   return 0 ;
}

//----------------------------------------------------------------------

char *VFrameInfoFile::mainIndexFilename() const
{
   return db->indexfile_name ;
}

//----------------------------------------------------------------------

int VFrameInfoFile::extra_index_stream(int idxtype) const
{
#ifdef FrEXTRA_INDEXES
   if (db)
      return db->extra_index_stream(idxtype);
#else
   (void)idxtype ;
#endif /* FrEXTRA_INDEXES */
   return 0 ;
}

//----------------------------------------------------------------------

long int VFrameInfoFile::lookupID(const FrSymbol *sym) const
{
#ifdef FrFRAME_ID
   if (db)
      {
      FrObject *info ;
      (void)db->index->lookup(sym,&info) ;
      HashEntryVFrame *entry = (HashEntryVFrame *)info ;
      if (entry)
	 return entry->frameID ;
      else
	 return NO_FRAME_ID ;
      }
#endif /* FrFRAME_ID */
   return NO_FRAME_ID ;
}

//----------------------------------------------------------------------

FrSymbol *VFrameInfoFile::lookupSym(long int frameID) const
{
#ifdef FrFRAME_ID
   int dirnum = (int)(frameID / FrIDs_PER_BLOCK) ;
   if (dirnum >= 0 && dirnum < FRAME_IDENT_DIR_SIZE &&
       frame_IDs && frame_IDs->IDs[dirnum])
      {
      return (FrSymbol*)frame_IDs->
	    		IDs[dirnum]->m_frames[(int)(frameID % FrIDs_PER_BLOCK)] ;
      }
#endif /* FrFRAME_ID */
   return 0 ;   // no such frame ID
}

//----------------------------------------------------------------------

bool VFrameInfoFile::getDBUserData(DBUserData *user_data) const
{
   if (db)
      {
      *user_data = db->user_data ;
      return true ;
      }
   else
      return false ;
}

//----------------------------------------------------------------------

bool VFrameInfoFile::setDBUserData(DBUserData *user_data)
{
   if (db)
      {
      db->user_data = *user_data ;
      return (update_database_header(db) != -1) ;
      }
   else
      return false ;
}

//----------------------------------------------------------------------

int VFrameInfoFile::prefetchFrames(FrList*)
{
   // no need to prefetch frames when using a local file as backing store,
   // since there is not a significant performance improvement
   return 0 ;
}

//----------------------------------------------------------------------

FrList *VFrameInfoFile::availableDatabases() const
{
   return find_databases(0,0) ;
}

#endif /* FrDATABASE */

/**********************************************************************/
/*    Initialization functions					      */
/**********************************************************************/

FrSymbolTable *initialize_VFrames_disk(const char *filename,int symtabsize,
				       bool transactions, bool force_create,
				       const char *password)
{
#ifdef FrDATABASE
   FrSymbolTable *symtab = new FrSymbolTable(symtabsize) ;
   VFrameInfoFile *info ;

   if (symtab)
      {
      symtab->select() ;
      info = new VFrameInfoFile(filename,transactions,force_create,password) ;
      if (info->backstorePresent())
	 {
	 symtab->bstore_info = info ;
	 // force all changes to the symbol table to be updated in both the
	 // active copy and the copy addressed via 'symtab'
	 FrSymbolTable::selectDefault() ;
	 symtab->select() ;
#ifdef FrLRU_DISCARD
	 initialize_FramepaC_LRU() ;
#endif
	 }
      else
         {
         destroy_symbol_table(symtab) ;
	 symtab = 0 ;
         }
      }
   return symtab ;
#else
   (void)filename ; (void)symtabsize ; (void)transactions ;
   (void)force_create ; (void)password ;
   return 0 ;
#endif /* FrDATABASE */
}

//----------------------------------------------------------------------

// end of file frdatfil.cpp //
