/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frnetwork.cpp	 "virtual memory" frames using networked server */
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,1998,2000,2001,2009,2015		*/
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

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "clientno.h"
#include "frsymtab.h"
#include "frframe.h"
#include "frpcglbl.h"
#include "frnetwrk.h"
#include "frconnec.h"
#include "frserver.h"
#include "mikro_db.h"

#ifdef FrLRU_DISCARD
#include "frlru.h"
#endif

#ifdef FrFRAME_ID
#include "frameid.h"
#endif

#if defined(_MSC_VER) && _MSC_VER >= 800
#include "winsock.h"  // for gethostname()
#endif /* _MSC_VER >= 800 */

#ifdef FrSERVER

/************************************************************************/
/*      Configuration values for this module				*/
/************************************************************************/

// wait up to 30 seconds for frame to be locked
#define LOCK_TIMEOUT 30

// maximum number of distinct servers supported at one time
#define MAX_SERVERS 32

// maximum number of databases open at one time
#define MAX_DATABASES 64

// maximum length of a hostname
#define MAX_HOSTNAME 127

/************************************************************************/
/*	Manifest constants for this module				*/
/************************************************************************/

/************************************************************************/
/*	Types local to this module					*/
/************************************************************************/

struct DBCREATE
   {
   DBUserData user_data ;
   } ;

class INDEXINFO
{
   public:
      int  next_index ;
      int  size ;
      int  type ;
      char *name ;
} ;

struct DBCONNECT
   {
   bool in_use ;
   FrConnection *connection ;
   int clienthandle ;
   int dbhandle ;
   const char *server_name ;
   FrSymbolTable *symboltable ;
   } ;

/************************************************************************/
/*    Global variables local to this module				*/
/************************************************************************/

static DBUserData creation_info ;
static DBCONNECT db_connections[MAX_DATABASES] ;

static int active_connections = 0 ;

static FramepaC_bgproc_funcptr old_background_func ;

/************************************************************************/
/************************************************************************/

static void background_processing()
{
   if (active_connections)
      FrClient::process() ;
   if (old_background_func)
      old_background_func() ;
}

/************************************************************************/
/*	Connection management functions					*/
/************************************************************************/

static bool store_db_connection(FrConnection *connection, int clhandle,
				int dbhandle, const char *name,
				FrSymbolTable *symtab)
{
   for (unsigned int i = 0 ; i < lengthof(db_connections) ; i++)
      if (!db_connections[i].in_use)
	 {
	 db_connections[i].connection = connection ;
	 db_connections[i].clienthandle = clhandle ;
	 db_connections[i].dbhandle = dbhandle ;
	 db_connections[i].server_name = name ;
	 db_connections[i].symboltable = symtab ;
	 db_connections[i].in_use = true ;
	 if (active_connections++ == 0 &&
	     get_FramepaC_bgproc_func() != background_processing)
            {
	    old_background_func = get_FramepaC_bgproc_func() ;
	    set_FramepaC_bgproc_func(background_processing) ;
	    }
	 return true ;
	 }
   return false ;
}

//----------------------------------------------------------------------

static DBCONNECT *find_db_connection(const char *server)
{
   for (unsigned int i = 0 ; i < lengthof(db_connections) ; i++)
      if (db_connections[i].in_use &&
	  strcmp(db_connections[i].server_name,server) == 0)
	 return &db_connections[i] ;
   return 0 ;
}

/************************************************************************/
/*	Notification Handlers						*/
/************************************************************************/

static void update_not(FrPacket *req)
{
   FrSymbol *frname ;

   if (!req)
      return ;
   int dbhandle = FrLoadByte(req->packetData()+0) ;
   FrSymbolTable *symtab = select_database(req->getConnection(), dbhandle) ;
   if (!symtab)
      {
      if (VFrame_Info &&
	  ((VFrameInfoServer *)VFrame_Info)->old_update_notify_func)
         ((VFrameInfoServer *)VFrame_Info)->old_update_notify_func(req) ;
      return ;
      }
   frname = FrSymbolTable::add(req->packetData()+1) ;
   frname->discardFrame() ;
   if (VFrame_Info)
      {
      if (VFrame_Info->update_handler)
         VFrame_Info->update_handler(VFNot_UPDATE,frname) ;
      if (((VFrameInfoServer*)VFrame_Info)->old_update_notify_func)
         ((VFrameInfoServer *)VFrame_Info)->old_update_notify_func(req) ;
      }
   symtab->select() ;
}

//----------------------------------------------------------------------

static void lock_not(FrPacket *req)
{
   FrSymbol *frname ;

   if (!req)
      return ;
   char *data = req->packetData() ;
   int dbhandle = FrLoadByte(data+0) ;
   FrSymbolTable *symtab = select_database(req->getConnection(),dbhandle) ;
   if (!symtab)
      {
      if (VFrame_Info &&
          ((VFrameInfoServer *)VFrame_Info)->old_lock_notify_func)
         ((VFrameInfoServer *)VFrame_Info)->old_lock_notify_func(req) ;
      return ;
      }
   frname = FrSymbolTable::add(data+1) ;
   frname->lockFrame() ;
   if (VFrame_Info)
      {
      if (VFrame_Info->lock_handler)
         VFrame_Info->lock_handler(VFNot_LOCK,frname) ;
      if (((VFrameInfoServer *)VFrame_Info)->old_lock_notify_func)
         ((VFrameInfoServer *)VFrame_Info)->old_lock_notify_func(req) ;
      }
   symtab->select() ;
}

//----------------------------------------------------------------------

static void unlock_not(FrPacket *req)
{
   FrSymbol *frname ;

   if (!req)
      return ;
   char *data = req->packetData() ;
   int dbhandle = FrLoadByte(data+0) ;
   FrSymbolTable *symtab = select_database(req->getConnection(),dbhandle) ;
   if (!symtab)
      {
      if (VFrame_Info &&
          ((VFrameInfoServer *)VFrame_Info)->old_unlock_notify_func)
         ((VFrameInfoServer *)VFrame_Info)->old_unlock_notify_func(req) ;
      return ;
      }
   frname = FrSymbolTable::add(data+1) ;
   frname->unlockFrame() ;
   if (VFrame_Info)
      {
      if (VFrame_Info->unlock_handler)
         VFrame_Info->unlock_handler(VFNot_UNLOCK,frname) ;
      if (((VFrameInfoServer*)VFrame_Info)->old_unlock_notify_func)
         ((VFrameInfoServer *)VFrame_Info)->old_unlock_notify_func(req) ;
      }
   symtab->select() ;
}

//----------------------------------------------------------------------

static void create_not(FrPacket *req)
{
   FrSymbol *frname ;

   if (!req)
      return ;
   char *data = req->packetData() ;
   int dbhandle = FrLoadByte(data+0) ;
   FrSymbolTable *symtab = select_database(req->getConnection(),dbhandle) ;
   if (!symtab)
      {
      if (VFrame_Info &&
          ((VFrameInfoServer *)VFrame_Info)->old_create_notify_func)
         ((VFrameInfoServer *)VFrame_Info)->old_create_notify_func(req) ;
      return ;
      }
   frname = FrSymbolTable::add(data+1) ;
   if (VFrame_Info)
      {
      VFrame_Info->createFrame(frname,false) ;
      if (VFrame_Info->create_handler)
         VFrame_Info->create_handler(VFNot_CREATE,frname) ;
      if (((VFrameInfoServer*)VFrame_Info)->old_create_notify_func)
         ((VFrameInfoServer *)VFrame_Info)->old_create_notify_func(req) ;
      }
   symtab->select() ;
}

//----------------------------------------------------------------------

static void delete_not(FrPacket *req)
{
   FrSymbol *frname ;
   FrFrame *fr ;

   if (!req)
      return ;
   char *data = req->packetData() ;
   int dbhandle = FrLoadByte(data+0) ;
   FrSymbolTable *symtab = select_database(req->getConnection(),dbhandle) ;
   if (!symtab)
      {
      if (VFrame_Info &&
          ((VFrameInfoServer *)VFrame_Info)->old_delete_notify_func)
         ((VFrameInfoServer *)VFrame_Info)->old_delete_notify_func(req) ;
      return ;
      }
   frname = FrSymbolTable::add(data+1) ;
   if ((fr = frname->symbolFrame()) != 0)
      delete fr ;
   if (VFrame_Info)
      {
      VFrame_Info->deleteFrame(frname,false) ;
      if (VFrame_Info->delete_handler)
	 VFrame_Info->delete_handler(VFNot_DELETE,frname) ;
      if (((VFrameInfoServer*)VFrame_Info)->old_delete_notify_func)
         ((VFrameInfoServer *)VFrame_Info)->old_delete_notify_func(req) ;
      }
   symtab->select() ;
}

//----------------------------------------------------------------------

static void proxy_not(FrPacket *req)
{
   if (!req)
      return ;
   char *data = req->packetData() ;
   int dbhandle = FrLoadByte(data+0) ;
   FrSymbolTable *symtab = select_database(req->getConnection(),dbhandle) ;
   if (!symtab)
      {
      if (VFrame_Info &&
          ((VFrameInfoServer *)VFrame_Info)->old_proxy_notify_func)
         ((VFrameInfoServer *)VFrame_Info)->old_proxy_notify_func(req) ;
      return ;
      }
   int type = FrLoadByte(data+1) ;
   data += 2 ;				// skip dbhandle and type
   FrSymbol *frname = FrSymbolTable::add(data) ;
   data = strchr(data,'\0')+1 ;  // skip the NUL
   FrSymbol *slotname = FrSymbolTable::add(data) ;
   data = strchr(data,'\0')+1 ;  // skip the NUL
   FrSymbol *facetname = FrSymbolTable::add(data) ;
   data = strchr(data,'\0')+1 ;  // skip the NUL
   FrSymbol *filler = (FrSymbol*)string_to_FrObject(data) ;
   if (data >= req->packetEnd())
      {
      // bad FrPacket!

      }
   else
      {
      switch (type)
	 {
	 case 0x00:  // add
	    if (VFrame_Info->proxyadd_handler)
	       VFrame_Info->proxyadd_handler(VFNot_PROXYADD,frname,slotname,
					     facetname,filler) ;
	    break ;
	 case 0x01:  // delete
	    if (VFrame_Info->proxydel_handler)
	       VFrame_Info->proxydel_handler(VFNot_PROXYDEL,frname,slotname,
					     facetname,filler) ;
	    break ;
	 default:
	    break ;
	 }
      }
   symtab->select() ;
}

//----------------------------------------------------------------------

static int notify_remote_shutdown(FrSymbolTable *, va_list args)
{
   FrVarArg(int,seconds) ;
   FrVarArg(FrConnection *,connection) ;
   if (VFrame_Info && VFrame_Info->backingStoreType() == BS_server &&
       ((VFrameInfoServer*)VFrame_Info)->getConnection() == connection)
      {
      if (VFrame_Info->shutdown_handler)
	 VFrame_Info->shutdown_handler(
		          ((VFrameInfoServer*)VFrame_Info)->getServerName(),
			  seconds) ;
      }
   return true ;
}

//----------------------------------------------------------------------

static int close_remote_database(FrSymbolTable *symtab, va_list args)
{
   FrVarArg(FrConnection *,conn) ;
   if (VFrame_Info && VFrame_Info->backingStoreType() == BS_server &&
       ((VFrameInfoServer*)VFrame_Info)->getConnection() == conn)
      shutdown_VFrames(symtab) ;
   return true ;
}

//----------------------------------------------------------------------

static void shutdown_not(FrPacket *req)
{
   if (!req)
      return ;
   int seconds = FrLoadShort(req->packetData()) ;
   do_all_symtabs(notify_remote_shutdown,seconds,req->getConnection()) ;
   if (seconds <= 5)  // server going down (almost) immediately?
      {
      do_all_symtabs(close_remote_database,req->getConnection()) ;
      }
}

/************************************************************************/
/*	Methods for class VFrameInfoServer				*/
/************************************************************************/

static int load_byname_index(FrSymHashTable *hash,FrClient *client,
			     FrRemoteDB *db,
#ifdef FrFRAME_ID
			     FrameIdentDirectory *frame_IDs,
#endif
			     INDEXINFO &)
{
   Fr_errno = FSE_Success ;
   char *buf = client->readDBIndex(db,0) ; // get main index
   char *p ;

   if (!buf)
      {
      if (Fr_errno)
	 FrWarning("server error while loading database index") ;
      else
	 FrNoMemory("while loading database index") ;
      return -1 ;
      }
   int size = 0 ;
   p = buf ;
   while (*p)
      {
      p = strchr(p,'\0')+1 ;
      size++ ;
      }
   hash->expandTo(size+100) ;
   FrSymbolTable::current()->expandTo(size+300) ;
   p = buf ;
   while (*p)
      {
      char name[FrMAX_SYMBOLNAME_LEN+1] ;

      strncpy(name,p,FrMAX_SYMBOLNAME_LEN) ;
      name[FrMAX_SYMBOLNAME_LEN] = '\0' ;
      FrSymbol *symname = FrSymbolTable::add(name) ;
      HashEntryServer ent(symname) ;
#ifdef FrFRAME_ID
      ent.frameID = allocate_frameID(frame_IDs,symname) ;
#endif /* FrFRAME_ID */
      hash->add(&ent) ;

      p = strchr(p,'\0')+1 ;
      }
   FrFree(buf) ;
   return 0 ;
}

//----------------------------------------------------------------------

VFrameInfoServer::VFrameInfoServer(const char *servername,int port,
				   const char *database,
				   const char *username, const char *passwd,
				   bool transactions, bool force_create)
{
//   FrServerInfo s_info ;
   INDEXINFO idx_info ;
   if (port == 0)
      port = FrSERVER_PORT ;
   client = FrClient::findClient(servername,port) ;
   if (!client)
      {
      // establish client connection to server
      client = new FrClient(servername,0,MIKCLIENT_VFRAME,username,passwd) ;
      }
   dbhandle = -1 ;		 // assume failure
   use_transactions = transactions ;
   FrSymbolTable *symtab = new FrSymbolTable(0) ;
   if (!symtab)
      return ;			 // failed
   FrSymbolTable *oldsymtab = symtab->select() ;
   if (servername)
      {
      server_name = FrDupString(servername) ;
      }
   else
      {
      servername = server_name = FrNewN(char,MAX_HOSTNAME+1) ;
      gethostname((char*)servername,MAX_HOSTNAME) ;
      }
   DBCONNECT *conn = find_db_connection(servername) ;
   if (conn)
      {
      connection = conn->connection ;
      clhandle = conn->clienthandle ;
      }
   else
      {
      clhandle = client->registerClient(MIKCLIENT_VFRAME, username, passwd) ;
      if (clhandle < 0)
	 {
	 client->disconnect() ;
	 destroy_symbol_table(symtab) ;
	 oldsymtab->select() ;
	 return ;		 // failed
	 }
      }
   database_name = FrDupString(database) ;
   if (!database_name)
      {
      destroy_symbol_table(symtab) ;
      oldsymtab->select() ;
      return ;			 // failed
      }
   password = FrDupString(passwd) ;
   if (passwd && !password)
      {
      FrFree(database_name) ;
      destroy_symbol_table(symtab) ;
      oldsymtab->select() ;
      return ;		 	// failed
      }
   int idxcode = client->findIndex(database_name,0,&idx_info) ;
   Fr_errno = 0 ;
   setReadOnly(false) ;
   if (idxcode == 0)
      db = client->openDatabase(database_name, password) ;
   if (Fr_errno == FSE_READONLYDATABASE)
      setReadOnly(true) ;
   if (!db && force_create)
      db = client->createDatabase(database_name, password, &creation_info);
   if (!db)
      {
      FrFree(password) ;
      password = 0 ;
      FrFree(database_name) ;
      database_name = 0 ;
      destroy_symbol_table(symtab) ;
      oldsymtab->select() ;
      return ;
      }
   if (!store_db_connection(connection,clhandle,dbhandle,servername,symtab))
      {
      // too many connections!!
      FrWarning("Too many open connections, system may become unstable.");
      Fr_errno = ME_TOOMANYDBS ;
      }
   hash = new FrSymHashTable ;
#ifdef FrFRAME_ID
   frame_IDs = FrNew(FrameIdentDirectory) ;
   for (int dirnum = 0 ; dirnum < FRAME_IDENT_DIR_SIZE ; dirnum++)
      frame_IDs->IDs[dirnum] = 0 ;
#endif /* FrFRAME_ID */
   load_byname_index(hash,client,db,
#ifdef FrFRAME_ID
		     frame_IDs,
#endif /* FrFRAME_ID */
		     idx_info) ;
   client->getUserData(db,&db_user_data) ;
   old_update_notify_func = client->getNotify(FSNot_FRAMEUPDATED) ;
   old_create_notify_func = client->getNotify(FSNot_FRAMECREATED) ;
   old_delete_notify_func = client->getNotify(FSNot_FRAMEDELETED) ;
   old_lock_notify_func = client->getNotify(FSNot_FRAMELOCKED) ;
   old_unlock_notify_func = client->getNotify(FSNot_FRAMEUNLOCKED) ;
   old_proxy_notify_func = client->getNotify(FSNot_PROXYUPDATE) ;
   old_shutdown_notify_func = client->getNotify(FSNot_SERVERGOINGDOWN) ;
   if (old_update_notify_func != update_not)
      client->setNotify(FSNot_FRAMEUPDATED,update_not) ;
   if (old_create_notify_func != create_not)
      client->setNotify(FSNot_FRAMECREATED,create_not) ;
   if (old_delete_notify_func != delete_not)
      client->setNotify(FSNot_FRAMEDELETED,delete_not) ;
   if (old_lock_notify_func != lock_not)
      client->setNotify(FSNot_FRAMELOCKED,lock_not) ;
   if (old_unlock_notify_func != unlock_not)
      client->setNotify(FSNot_FRAMEUNLOCKED,unlock_not) ;
   if (old_proxy_notify_func != proxy_not)
      client->setNotify(FSNot_PROXYUPDATE,proxy_not) ;
   if (old_shutdown_notify_func != shutdown_not)
      client->setNotify(FSNot_SERVERGOINGDOWN,shutdown_not) ;
   // all successful, so point the symbol table at ourselves
   VFrame_Info = this ;
   // and force all changes to the symbol table to be updated in both the
   // active copy and the copy addressed via 'symtab'
   FrSymbolTable::selectDefault() ;
   symtab->select() ;
#ifdef FrLRU_DISCARD
   initialize_FramepaC_LRU() ;
#endif
}

//----------------------------------------------------------------------

VFrameInfoServer::~VFrameInfoServer()
{
   if (backstorePresent())
      {
      if (client->closeDatabase(db) == -1)
	 {
	 FrWarning("error closing database on server") ;
	 }
      }
   client->disconnect() ;
   if (database_name)
      {
      FrFree(database_name) ;
      database_name = 0 ;
      }
   if (server_name)
      {
      FrFree(server_name) ;
      server_name = 0 ;
      }
   if (client->getNotify(FSNot_FRAMEUPDATED) == update_not)
      client->setNotify(FSNot_FRAMEUPDATED, old_update_notify_func) ;
   if (client->getNotify(FSNot_FRAMECREATED) == create_not)
      client->setNotify(FSNot_FRAMECREATED, old_create_notify_func) ;
   if (client->getNotify(FSNot_FRAMEDELETED) == delete_not)
      client->setNotify(FSNot_FRAMEDELETED,old_delete_notify_func) ;
   if (client->getNotify(FSNot_FRAMELOCKED) == lock_not)
      client->setNotify(FSNot_FRAMELOCKED, old_lock_notify_func) ;
   if (client->getNotify(FSNot_FRAMEUNLOCKED) == unlock_not)
      client->setNotify(FSNot_FRAMEUNLOCKED, old_unlock_notify_func) ;
   if (client->getNotify(FSNot_PROXYUPDATE) == proxy_not)
      client->setNotify(FSNot_PROXYUPDATE,old_proxy_notify_func) ;
   if (client->getNotify(FSNot_SERVERGOINGDOWN) == shutdown_not)
      client->setNotify(FSNot_SERVERGOINGDOWN,old_shutdown_notify_func) ;
#ifdef FrLRU_DISCARD
   shutdown_FramepaC_LRU() ;
#endif
}

//----------------------------------------------------------------------

BackingStore VFrameInfoServer::backingStoreType() const
{
   return BS_server ;
}

//----------------------------------------------------------------------

int VFrameInfoServer::backstorePresent() const
{
   return (db != 0) ;
}

//----------------------------------------------------------------------

bool VFrameInfoServer::isFrame(const FrSymbol *name) const
{
   if (hash)
      {
      FramepaC_bgproc() ;		// handle any pending notifications
      HashEntryServer key(name) ;
      HashEntryServer *entry = (HashEntryServer*)hash->lookup(&key) ;
      return (entry && !entry->deleted) ;
      }
   else
      return false ;
}

//----------------------------------------------------------------------

bool VFrameInfoServer::isDeletedFrame(const FrSymbol *name) const
{
   if (hash)
      {
      FramepaC_bgproc() ;		// handle any pending notifications
      HashEntryServer key(name) ;
      HashEntryServer *entry = (HashEntryServer*)hash->lookup(&key) ;
      return (entry && entry->deleted) ;
      }
   else
      return false ;
}

//----------------------------------------------------------------------

bool VFrameInfoServer::isLocked(const FrSymbol *frame) const
{
   if (hash)
      {
      HashEntryServer key(frame) ;
      FramepaC_bgproc() ;		// handle any pending notifications
      HashEntryServer *entry = (HashEntryServer *)hash->lookup(&key) ;
      return entry ? entry->locked : false ;
      }
   else
      return false ;
}

//----------------------------------------------------------------------

int VFrameInfoServer::lockFrame(const FrSymbol *frame)
{
   Fr_errno = FSE_Success ;
   if (!hash)
      return false ;
   if (isReadOnly())
      return true ;
   if (client->lockFrame(db,frame))
      {
      HashEntryServer key(frame) ;
      HashEntryServer *entry = (HashEntryServer *)hash->lookup(&key) ;

      if (entry)
	 entry->locked = true ;
      else
	 return false ;
      return true ;
      }
   else if (Fr_errno == FSE_FRAMELOCKED)
      {
      // frame was already locked on server, so mark it locked locally
      HashEntryServer key(frame) ;
      HashEntryServer *entry = (HashEntryServer *)hash->lookup(&key) ;

      if (entry)
	 entry->locked = true ;
      if (frame->symbolFrame())
	 frame->symbolFrame()->setLock(true) ;
      return false ;
      }
   return false ;
}

//----------------------------------------------------------------------

int VFrameInfoServer::unlockFrame(const FrSymbol *frame)
{
   if (isReadOnly())
      return true ;
   if (hash && client->unlockFrame(db,frame))
      {
      HashEntryServer key(frame) ;
      HashEntryServer *entry = (HashEntryServer *)hash->lookup(&key) ;

      if (entry)
	 entry->locked = false ;
      else
	 return false ;
      return true ;
      }
   else
      return false ;
}

//----------------------------------------------------------------------

static void retrieve_callback(FrPacket *request, FrPacket *reply,
			   void *result, void *client_data)
{
   (void)request ; (void)result ;
   VFrameInfo *vinfo = VFrame_Info ;
   bool oldvirt = read_virtual_frames(true) ;
   VFrame_Info = 0 ;	// temporarily disable virtual access
			// to avoid recursive call while converting string
   char *replydata = reply->packetData() ;
   char *end = replydata + reply->packetLength() ;
   while (replydata < end)
      {
      (void)string_to_FrObject(replydata) ;
      if (replydata < end)
	 replydata++ ;			// skip terminating NUL for frame rep
      }
   read_virtual_frames(oldvirt) ;
   VFrame_Info = vinfo ;
   int *done = (int*)client_data ;
   if (done)
      *done = true ;
}

//----------------------------------------------------------------------

VFrame *VFrameInfoServer::retrieveFrameAsync(const FrSymbol *name,
					     int &done) const
{
   if (!name || !client )
      {
      done = true ;			// can't load a frame without a name
      return 0 ;			//   or a network connection
      }
   done = false ;
   VFrame *fr = (VFrame *)name->symbolFrame() ;
   // don't use find_vframe, to avoid loading the frame synchronously
   if (fr)
      return fr ;			// indicate frame already in memory
   bool success = client->getFrame(db,(FrSymbol*)name,
				   &retrieve_callback,(void*)&done) ;
   if (!success)
      done = true ;
   return false ;			// not yet in memory; will arrive
				        //   shortly if done == false
}

//----------------------------------------------------------------------

VFrame *VFrameInfoServer::retrieveFrame(const FrSymbol *name) const
{
   int result ;
   HashEntryServer *fr ;
   VFrameInfo *vinfo = VFrame_Info ;
   VFrame *loadedframe ;

   FramepaC_bgproc() ;		// handle any pending notifications
   HashEntryServer ent(name) ;
   if (!hash)
      return 0 ;
   // does the frame exist at all?  Or has it been deleted on the server?
   if ((fr=(HashEntryServer*)hash->lookup(&ent)) == 0 || (fr->deleted))
      return 0 ;	// not on server; indicate so without asking server
   bool oldvirt = read_virtual_frames(true) ;
   VFrame_Info = 0 ;	// temporarily disable virtual access
			// to avoid recursive call while converting string
   result = client->getFrame(db,(FrSymbol*)name) ;
   read_virtual_frames(oldvirt) ;
   VFrame_Info = vinfo ;
   if (result == -1)
      return 0 ;
   else
      {
      loadedframe = name ? (VFrame *)name->symbolFrame() : 0 ;
      if (loadedframe)
	 {
	 loadedframe->setLock(fr->locked) ;
         loadedframe->markDirty(false) ;  // no changes since being loaded
	 }
      return loadedframe ;
      }
}

//----------------------------------------------------------------------

VFrame *VFrameInfoServer::retrieveOldFrame(FrSymbol *name,int generation)
const
{
   int result ;
   HashEntryServer *fr ;
   VFrameInfo *vinfo = VFrame_Info ;
   VFrame *loadedframe ;

   FramepaC_bgproc() ;		// handle any pending notifications
   HashEntryServer ent(name) ;
   if (!hash)
      return 0 ;
   // does the frame exist at all?
   if ((fr=(HashEntryServer*)hash->lookup(&ent)) == 0)
      return 0 ;	// not on server; indicate so without asking server
   bool oldvirt = read_virtual_frames(true) ;
   VFrame_Info = 0 ;	// temporarily disable virtual access
			// to avoid recursive call while converting string
   result = client->getOldFrame(db,name,generation) ;
   read_virtual_frames(oldvirt) ;
   VFrame_Info = vinfo ;
   if (result == -1)
      return 0 ;
   else
      {
      loadedframe = name ? (VFrame *)name->symbolFrame() : 0 ;
      if (loadedframe)
	 loadedframe->setLock(fr->locked) ;
      return loadedframe ;
      }
}

//----------------------------------------------------------------------

bool VFrameInfoServer::renameFrame(const FrSymbol *oldname,
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

bool VFrameInfoServer::storeFrame(const FrSymbol *name) const
{
   if (isReadOnly())
      return 0 ;     // silently ignore the call if read-only database
   Fr_errno = FSE_Success ;
   if (name && name->symbolFrame())
      {
      int result = client->updateFrame(db,(FrSymbol*)name) ;
      if (result == 0)
	 name->symbolFrame()->markDirty(false) ;
      return (result != -1) ;
      }
   else
      {
      Fr_errno = FSE_GENERAL ;
      return false ;
      }
}

//----------------------------------------------------------------------

bool syncOneFrame(FrHashEntry *ent, va_list args)
{
   FrVarArg(VFrameInfoServer *,vinfo) ;
   FrVarArg(frame_update_hookfunc *,hook) ;
   HashEntryServer *entry = (HashEntryServer *)ent ;
   FrFrame *frame = entry->frameName()->symbolFrame() ;

   (void)args ; // avoid compiler warning
   if (frame)
      {
      if (frame->isVFrame() && frame->dirtyFrame())
         {
	 if (hook)
	    hook(frame->frameName()) ;
         if (vinfo->storeFrame(entry->frameName()) == -1)
            {
	    return false ;
	    }
         }
      else if (entry->deleted && !entry->delete_stored)
         {
         if (vinfo->deleteFrame(entry->frameName(),true) == -1)
	    {
            return false ;
            }
         else
            entry->delete_stored = true ;
         }
      frame->markDirty(false) ;
      }
   return true ;
}

//----------------------------------------------------------------------

int VFrameInfoServer::syncFrames(frame_update_hookfunc *hook)
{
   if (isReadOnly())	// silently ignore the call if read-only database
      return 0 ;
   if (hash && hash->doHashEntries(syncOneFrame,this,hook))
      return 0 ; // successful
   else
      return -1 ;
}

//----------------------------------------------------------------------

int VFrameInfoServer::createFrame(const FrSymbol *name, bool upd_backstore)
{
   int result ;
   char *newname = 0 ;

   if (upd_backstore && !isReadOnly())
      result = client->createFrame(db,(FrSymbol *)name) ;
   else
      result = 0 ;
   if (result == -1)
      {
//!!!
      delete newname ;
      FrWarning("unable to create frame in server's database") ;
      }
   else
      {
      HashEntryServer ent(name) ;
#ifdef FrFRAME_ID
      ent.frameID = allocate_frameID(frame_IDs, name) ;
#endif /* FrFRAME_ID */
      if (hash)
         hash->add(&ent) ;
      }
   return result ;
}

//----------------------------------------------------------------------

int VFrameInfoServer::deleteFrame(const FrSymbol *name, bool upd_backstore)
{
   int result ;

   if (upd_backstore && !isReadOnly())
      result = client->deleteFrame(db,(FrSymbol *)name,password) ;
   else
      result = 0 ;
   if (!hash)
      return result ;
   HashEntryServer key(name) ;
   HashEntryServer *ent = (HashEntryServer*)hash->lookup(&key) ;
   if (result == -1)
      {
      if (ent)
	 {
	 ent->deleted = true ;			// mark frame as deleted and
	 ent->delete_stored = false ;		// indicate it must be stored
#ifdef FrFRAME_ID
	 deallocate_frameID(frame_IDs,ent->frameID) ;
#endif /* FrFRAME_ID */
         }
      FrWarning("unable to delete frame from server's database") ;
      }
   else
      {
#ifdef FrFRAME_ID
      if (ent)
	 deallocate_frameID(frame_IDs,ent->frameID) ;
#endif /* FrFRAME_ID */
      hash->remove(&key) ;
      }
   return result ;
}

//----------------------------------------------------------------------

int VFrameInfoServer::proxyAdd(FrSymbol *frame, const FrSymbol *slot,
			       const FrSymbol *facet,
			       const FrObject *filler) const
{
   return client->proxyAdd(db,frame,slot,facet,filler) ;
}

//----------------------------------------------------------------------

int VFrameInfoServer::proxyDel(FrSymbol *frame, const FrSymbol *slot,
			       const FrSymbol *facet,
			       const FrObject *filler) const
{
   return client->proxyDelete(db,frame,slot,facet,filler) ;
}

//----------------------------------------------------------------------

int VFrameInfoServer::startTransaction()
{
   if (isReadOnly())
      return 0 ;	// silently ignore if read-only database
   return client->beginTransaction(db) ;
}

//----------------------------------------------------------------------

int VFrameInfoServer::endTransaction(int transaction)
{
   if (isReadOnly())
      return 0 ;	// silently ignore if read-only database
   return client->endTransaction(db,transaction) == true ? -1 : 0 ;
}

//----------------------------------------------------------------------

int VFrameInfoServer::abortTransaction(int transaction)
{
   int result  ;

   if (isReadOnly())
      return 0 ;	// silently ignore if read-only database
   result = client->abortTransaction(db,transaction) ;
// the following measure is rather drastic, but there is no other simple way
// to flush out the frames modified during the transaction to ensure that
// they are reloaded from the server on the next access.  Hopefully, aborted
// transactions will be very rare in practice.
   VFrameInfo *oldinfo = VFrame_Info ;
   VFrame_Info = 0 ;			// temporarily disable backing store
   delete_all_frames() ;		// remove all frames from memory
   VFrame_Info = oldinfo ;		// re-enable backing store
   return result ;
}

//----------------------------------------------------------------------

void VFrameInfoServer::setNotify(VFrameNotifyType type, VFrameNotifyFunc *fn)
{
   switch (type)
      {
      case VFNot_CREATE:
	 create_handler = fn ;
	 break ;
      case VFNot_DELETE:
	 delete_handler = fn ;
	 break ;
      case VFNot_UPDATE:
	 update_handler = fn ;
	 break ;
      case VFNot_LOCK:
	 lock_handler = fn ;
	 break ;
      case VFNot_UNLOCK:
	 unlock_handler = fn ;
	 break ;
      default:
         FrProgError("invalid notification type given to setNotify") ;
      }
}

//----------------------------------------------------------------------

VFrameNotifyPtr VFrameInfoServer::getNotify(VFrameNotifyType type) const
{
   switch (type)
      {
      case VFNot_CREATE:
         return create_handler ;
      case VFNot_DELETE:
         return delete_handler ;
      case VFNot_UPDATE:
	 return update_handler ;
      case VFNot_LOCK:
	 return lock_handler ;
      case VFNot_UNLOCK:
	 return unlock_handler ;
      default:
         FrProgError("invalid notification type given to getNotify") ;
      }
   return 0 ;
}

//----------------------------------------------------------------------

void VFrameInfoServer::setProxy(VFrameNotifyType type, VFrameProxyFunc *fn)
{
   switch (type)
      {
      case VFNot_PROXYADD:
         proxyadd_handler = fn ;
      case VFNot_PROXYDEL:
         proxydel_handler = fn ;
      default:
         FrProgError("invalid proxy handler type given to setProxy") ;
      }
}

//----------------------------------------------------------------------

void VFrameInfoServer::setShutdown(VFrameShutdownFunc *fn)
{
   shutdown_handler = fn ;
}

//----------------------------------------------------------------------

VFrameShutdownPtr VFrameInfoServer::getShutdown() const
{
   return shutdown_handler ;
}

//----------------------------------------------------------------------

VFrameProxyPtr VFrameInfoServer::getProxy(VFrameNotifyType type) const
{
   switch (type)
      {
      case VFNot_PROXYADD:
         return proxyadd_handler ;
      case VFNot_PROXYDEL:
         return proxydel_handler ;
      default:
         FrProgError("invalid proxy handler type given to getProxy") ;
      }
   return 0 ;
}

//----------------------------------------------------------------------

long int VFrameInfoServer::lookupID(const FrSymbol *sym) const
{
#ifdef FrFRAME_ID
   if (hash)
      {
      HashEntryServer key(sym) ;
      HashEntryServer *entry = (HashEntryServer *)hash->lookup(&key) ;
      if (entry)
	 return entry->frameID ;
      else
	 return NO_FRAME_ID ;
      }
#endif /* FrFRAME_ID */
   return NO_FRAME_ID ;
}

//----------------------------------------------------------------------

FrSymbol *VFrameInfoServer::lookupSym(long int frameID) const
{
#ifdef FrFRAME_ID
   int dirnum = (int)(frameID / FrIDs_PER_BLOCK) ;
   if (dirnum >= 0 && dirnum < FRAME_IDENT_DIR_SIZE &&
       frame_IDs && frame_IDs->IDs[dirnum])
      {
      return (FrSymbol*)frame_IDs->IDs[dirnum]->
			             frames[(int)(frameID % FrIDs_PER_BLOCK)];
      }
#endif /* FrFRAME_ID */
   return 0 ;   // no such frame ID
}

//----------------------------------------------------------------------

bool VFrameInfoServer::getDBUserData(DBUserData *user_data) const
{
   if (user_data)
      {
      *user_data = db_user_data ;
      return true ;
      }
   else
      return false ;
}

//----------------------------------------------------------------------

bool VFrameInfoServer::setDBUserData(DBUserData *user_data)
{
   if (!user_data)
      return false ;
   db_user_data = *user_data ;
   int result = client->setUserData(db,user_data) ;
   if (result == -1)
      return false ;
   else
      return true ;
}

//----------------------------------------------------------------------

#if 0
static void prefetch_callback(FrPacket *request, FrPacket *reply,
			   void *result, void *client_data)
{
   VFrameInfo *vinfo = VFrame_Info ;
   bool oldvirt = read_virtual_frames(true) ;
   VFrame_Info = 0 ;	// temporarily disable virtual access
			// to avoid recursive call while converting string
   char *replydata = reply->packetData() ;
   char *end = replydata + reply->packetLength() ;
   while (replydata < end)
      {
      (void)string_to_FrObject(replydata) ;
      if (replydata < end)
	 replydata++ ;			// skip terminating NUL for frame rep
      }
   read_virtual_frames(oldvirt) ;
   VFrame_Info = vinfo ;
   int *done = (int*)client_data ;
   if (done)
      *done = true ;
}
#endif /* 0 */

//----------------------------------------------------------------------

int VFrameInfoServer::prefetchFrames(FrList *frames)
{
   FrList *needed = 0 ;
   while (frames)
      {
      FrSymbol *frame = (FrSymbol*)frames->first() ;
      if (frame && frame->symbolp() && !frame->isFrame())
	 pushlist(frame,needed) ;
      frames = frames->rest() ;
      }
//!!! turn this into async version later
   int result = client->getFrames(db,needed) ;
   needed->eraseList(false) ;
   return result ;
}

//----------------------------------------------------------------------

FrList *VFrameInfoServer::availableDatabases() const
{
   return 0 ;   //!!! for now
}

#endif /* FrSERVER */

/************************************************************************/
/*    Initialization functions						*/
/************************************************************************/

FrSymbolTable *initialize_VFrames_server(const char *servername, int port,
					 const char *username,
					 const char *password,
					 const char *database,
					 int /*symtabsize*/,
					 bool transactions, bool force_create)
{
#ifdef FrSERVER
   VFrameInfoServer *info ;
   info = new VFrameInfoServer(servername, port, database, username, password,
			       transactions, force_create) ;
   if (info && info->backstorePresent())
      return FrSymbolTable::current() ;
   else
      {
      if (info)
	 delete info ;
      return 0 ;
      }
#else
   (void)servername ; (void)port ; (void)database ;
   (void)transactions ; (void)username ; (void)password ;
   (void)force_create ;
   return 0 ;
#endif /* FrSERVER */
}

//----------------------------------------------------------------------

// end of file frnetwrk.cpp //
