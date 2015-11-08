/************************************************************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frclient.cpp	    network-client code				*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1995,1996,1997,1998,2001,2006,2009,2011,2013,2014	*/
/*		2015 Ralf Brown/Carnegie Mellon University		*/
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
#  pragma implementation "frclient.h"
#endif

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#if defined(unix) || defined(__linux__) || defined(__GNUC__)
#include <sys/time.h>
#include <sys/socket.h>
#endif /* unix */
#if defined(__MSDOS__) || defined(__WATCOMC__) || defined(_MSC_VER) || defined(__linux__)
#include <time.h>
#endif /* __MSDOS__ || __WATCOMC__ || _MSC_VER || __linux__ */
#include "frobject.h"
#include "frqueue.h"
#include "frclisrv.h"
#include "frclient.h"
#include "frconnec.h"
#include "frsignal.h"
#include "frutil.h"
#include "frpcglbl.h"

#ifdef __WATCOMC__
struct fd_set { char bits[10] ; } ;
#define FD_SETSIZE 80
#define FD_ISSET(s,fdset) (((fdset)->bits[s/8]&(1<<(s%8)))!=0)
struct timeval { long int tv_sec, tv_usec ; } ;
extern "C" int select(int size,fd_set *read, fd_set *write, fd_set *exc,
		      timeval *limit) ;
#endif /* __WATCOMC__ */

#if defined(_MSC_VER) && (defined(_WINDOWS) || _MSC_VER >= 800)
# include "winsock.h"  // for fd_set and related declarations
#endif /* _MSC_VER && _WINDOWS */

/************************************************************************/
/*    Global variables local to this module			      	*/
/************************************************************************/

static fd_set connection_fdset ;	// bitmap of open connections
static FrClient *client_list = 0 ;
static FrConnection *current_connection = 0 ;

#ifdef SIGPIPE
static FrSignalHandler *sigpipe ;
#endif

/************************************************************************/
/*    Types for this modules						*/
/************************************************************************/

typedef void *FrClient::*FrClientPostProc(FrPacket *request, FrPacket *reply) ;

/************************************************************************/
/*    Global variables which are visible from other modules	      	*/
/************************************************************************/

/************************************************************************/
/*    Manifest constants for this module			      	*/
/************************************************************************/

#define TIMEOUT 120   // two minutes

#define ONE_MILLION (1000000L)

/************************************************************************/
/*    Utility Functions						     	*/
/************************************************************************/

static void get_server_reply(FrPacket * /*request*/, FrPacket *reply,
			     void * /* result*/, void *client_data)
{
   *((FrPacket**)client_data) = reply ;
}

//----------------------------------------------------------------------

static void *await_reply(FrPacket *request, FrPacket *&reply)
{
   time_t end_time = time(0) + TIMEOUT ;
   do {
      // process any incoming messages until a reply to 'request' arrives
      // or the timeout period expires
      FrClient::process() ;
      reply = request->getConnection()->replyReceived(request) ;
      } while (!reply && time(0) < end_time) ;
   if (reply)
      return reply->packetData() ;
   else
      return 0 ;
}

//----------------------------------------------------------------------

static int server_request(FrConnection *conn, int reqcode, int datasize,
			  const char *data, FrAsyncCallback *callback,
			  void *client_data, FrPacket **replypacket = 0)
{
   current_connection = conn ;
   char *packetdata ;
   if (datasize)
      {
      packetdata = FrNewN(char,datasize) ;
      if (!packetdata)
	 {
	 FrWarning("out of memory while sending a packet") ;
	 return ENOMEM ;
	 }
      memcpy(packetdata,data,datasize) ;
      }
   else
      packetdata = 0 ;
   char header[4] ;
   int seqnumber = conn->nextSeqNum() ;
   FrStoreByte(reqcode,	   header+2) ;
   FrStoreByte(seqnumber,  header+3) ;
   FrPacket *request = new FrPacket(conn,header,false) ;
   request->setData(datasize,packetdata) ;
   conn->send(request) ;
   if (callback)
      {
      // add the callback and client data to be invoked when the server's
      // response arrives
      request->setRecvCallback(callback,client_data) ;
      }
   else
      {
      FrPacket *reply = 0 ;
      request->setRecvCallback(get_server_reply,&reply) ;
      // get reply
      void *result = await_reply(request,reply) ;
      if (replypacket)
	 *replypacket = reply ;
(void)result ; //!!!
      }
   delete request ;
   current_connection = 0 ;
   return -1 ;
}

/**********************************************************************/
/*    Exception Handling					      */
/**********************************************************************/

#if defined(unix) || defined(__linux__) || defined(__GNUC__)
static void close_connection(FrConnection *conn)
{
   (void)conn ; //!!!
   return ;
}
#endif /* unix */

//----------------------------------------------------------------------

#ifdef SIGPIPE
static void sigpipe_handler(int)
{
   if (current_connection)
      {
      current_connection->setNetError(NE_SIGPIPE) ;
      FrMessageVA("Connection %d is down",
		  current_connection->connectionSocket()) ;
      close_connection(current_connection) ;
      }
   else
      {
      FrMessage("Received SIGPIPE") ;
      }
   return ;
}
#endif /* SIGPIPE */

//----------------------------------------------------------------------

static void setup_sigpipe_handler()
{
   static bool installed = false ;

   if (!installed)
      {
#ifdef SIGPIPE
      sigpipe = new FrSignalHandler(SIGPIPE,sigpipe_handler) ;
#endif /* SIGPIPE */
      installed = true ;
      }
   return ;
}

/**********************************************************************/
/*    Member functions for class FrRemoteDB			      */
/**********************************************************************/

FrRemoteDB::FrRemoteDB(FrConnection *conn, int dbhandle)
{
   connection = conn ;
   handle = dbhandle ;
   return ;
}

//----------------------------------------------------------------------

FrRemoteDB::~FrRemoteDB()
{
   if (connection)
      {
      //!!!
      }
   return ;
}

//----------------------------------------------------------------------

bool FrRemoteDB::readFrame(FrSymbol *framename)
{
   if (!connection || !framename)
      {
      Fr_errno = EINVAL ;
      return false ;
      }
   return false ; //!!! for now
}

//----------------------------------------------------------------------

bool FrRemoteDB::writeFrame(FrSymbol *framename)
{
   if (!connection || !framename)
      {
      Fr_errno = EINVAL ;
      return false ;
      }

   return false ; //!!! for now
}

//----------------------------------------------------------------------

bool FrRemoteDB::getUserData(DBUserData *userdata) const
{
   if (!connection | !userdata)
      {
      Fr_errno = EINVAL ;
      return false ;
      }

   return false ; //!!! for now
}

//----------------------------------------------------------------------

bool FrRemoteDB::setUserData(DBUserData *userdata) const
{
   if (!connection | !userdata)
      {
      Fr_errno = EINVAL ;
      return false ;
      }

   return false ; //!!! for now
}

/**********************************************************************/
/*    Member functions for class FrClient			      */
/**********************************************************************/

FrClient::FrClient()
{
   connection = 0 ;
   m_port = 0 ;
   client_handle = -1 ;
   server_name = 0 ;
   server_info = 0 ;
   user_name = 0 ;
   user_password = 0 ;
   symtab = FrSymbolTable::current() ;
   setup_sigpipe_handler() ;
   return ;
}

//----------------------------------------------------------------------

FrClient::FrClient(const char *server, uint16_t client_id)
{
   connection = 0 ;
   client_handle = -1 ;
   server_name = 0 ;
   server_info = 0 ;
   user_name = 0 ;
   user_password = 0 ;
   symtab = FrSymbolTable::current() ;
   if (server)
      {
      setup_sigpipe_handler() ;
      server_name = FrDupString(server) ;
      if (!connect(client_id,server))
	 {
	 connection = 0 ;
	 FrFree(server_name) ;
	 server_name = 0 ;
	 server_info = 0 ;
	 }
      }
   return ;
}

//----------------------------------------------------------------------

FrClient::FrClient(const char *server, int port, uint16_t client_id,
		   const char *username, const char *password)
{
   user_name = 0 ;
   user_password = 0 ;
   connection = 0 ;
   m_port = port ;
   client_handle = -1 ;
   server_name = 0 ;
   server_info = 0 ;
   symtab = FrSymbolTable::current() ;
   if (server)
      {
      setup_sigpipe_handler() ;
      if (connect(client_id,server,m_port) && username && *username)
	 {
	 server_name = FrDupString(server) ;
	 registerClient(client_id,username,password) ;
	 return ;
	 }
      }
   return ;
}

//----------------------------------------------------------------------

FrClient::FrClient(FrConnection *conn)
{
   connection = conn ;
   symtab = FrSymbolTable::current() ;
   next = client_list ;
   prev = 0 ;
   if (client_list)
      client_list->prev = this ;
   client_list = this ;
//!!!
   return ;
}

//----------------------------------------------------------------------

FrClient::~FrClient()
{
   unregisterClient() ;
   disconnect() ;
   if (next)
      next->prev = prev ;
   if (prev)
      prev->next = next ;
   else
      client_list = next ;
   return ;
}

//----------------------------------------------------------------------

FrClient *FrClient::findClient(const char *servername, int serverport)
{
   for (FrClient *cl = client_list ; cl ; cl = cl->next)
      {
      if (cl->m_port == serverport && strcmp(cl->server_name,servername) == 0)
	 return cl ;
      }
   return 0 ;
}

//----------------------------------------------------------------------

int get_server_identification(FrConnection *connection,FrPacket **reply)
{
   if (!reply)
      return FSE_INVALIDPARAMS ;
   *reply = 0 ;
   int status = server_request(connection,FSReq_IDENTIFY,0,0,0,0,reply) ;
   return status ;
}

//----------------------------------------------------------------------

FrServerInfo *FrClient::getServerInfo() const
{
   FrPacket *reply ;
   int status = get_server_identification(connection,&reply) ;
   if (!reply || status != FSE_Success)
      return 0 ;
   char *data = reply->packetData() ;
   if (strcmp(data+FSRIdent_SIGNATURE,SIGNATURE_SERVER) != 0 &&
       strcmp(data+FSRIdent_SIGNATURE,SIGNATURE_PEER) != 0)
      {
      Fr_errno = FSE_NOTSERVER ;
      return 0 ;
      }
   FrServerInfo *info = new FrServerInfo ;
   info->protocol_version = 100*FrLoadByte(data+FSRIdent_PROTOMAJORVER)+
			    FrLoadByte(data+FSRIdent_PROTOMINORVER) ;
   info->server_version = 100*FrLoadByte(data+FSRIdent_SERVERMAJORVER) +
			  FrLoadByte(data+FSRIdent_SERVERMINORVER) ;
   info->server_patchlevel = FrLoadByte(data+FSRIdent_SERVERPATCHLEV) ;
   info->peer_mode = (bool)(strcmp(data+FSRIdent_SIGNATURE,SIGNATURE_PEER) == 0) ;
   info->max_request = FrLoadByte(data+FSRIdent_MAXREQUEST) ;
   info->max_notification = FrLoadByte(data+FSRIdent_MAXNOTIFY) ;
   info->outstanding_requests = FrLoadByte(data+FSRIdent_REQUESTLIMIT) ;
   info->max_clients = FrLoadByte(data+FSRIdent_MAXCLIENTS) ;
   info->num_clients = FrLoadByte(data+FSRIdent_CURRCLIENTS) ;
   info->password_register = (bool)FrLoadByte(data+FSRIdent_PSWD_REGISTER) ;
   info->password_accessdb = (bool)FrLoadByte(data+FSRIdent_PSWD_DBOPEN) ;
   info->password_createdb = (bool)FrLoadByte(data+FSRIdent_PSWD_DBCREATE) ;
   info->password_revertframe = (bool)FrLoadByte(data+FSRIdent_PSWD_REVERT) ;
   info->password_deleteframe = (bool)FrLoadByte(data+FSRIdent_PSWD_DELETE) ;
   info->password_systemconfig = (bool)FrLoadByte(data+FSRIdent_PSWD_SETCONFIG) ;
   return info ;
}

//----------------------------------------------------------------------

bool FrClient::connect(uint16_t client_id, const char *server, int port,
		       const char *username, const char *password)
{
   m_port = port ;
   connection = FrConnection::findConnection(server,port) ;
   if (!connection)
      {
      connection = new FrConnection ;
      if (!connection)
	 {
	 FrWarning("out of memory while opening network connection") ;
	 return false ;
	 }
      if (connection->connect(server,m_port))
	 registerClient(client_id,username,password) ;
      else
	 {
	 // error connecting to server; Fr_errno already set by connect()
	 return false ;
	 }
      }
   return true ;
}


//----------------------------------------------------------------------

bool FrClient::disconnect()
{
   bool result ;
   if (connection)
      {
      result = connection->disconnect() ;
      connection = 0 ;
      }
   else
      result = false ;
   return result ;
}

//----------------------------------------------------------------------

bool FrClient::registerClient(int client_id, const char *username,
			      const char *password)
{
   int peermode = 0 ; //!!!

   if (user_name)			// can't register twice, so unregister
      unregisterClient() ;		//   first before re-registering
   if (username && *username)
      {
      if (!password)
	 password = "" ;
      int userlen = strlen(username) + 1 ;
      int passlen = strlen(password) + 1 ;
      int datalen = 6 + userlen + passlen ;
      FrLocalAlloc(char,data,1000,datalen) ;
      FrStoreShort(0x0200,   data+0) ;	// max. supported packet size
      FrStoreShort(client_id,data+2) ;	// what type of client?
      FrStoreByte(peermode,  data+4) ;	// client-server or peer-to-peer?
      FrStoreByte(1,	     data+5) ;	// get preferences
      memcpy(data+6,username,userlen) ;
      memcpy(data+userlen+6,password,passlen) ;
      FrPacket *reply ;
      int status = server_request(connection,FSReq_REGISTER,datalen,data,
				  0,0,&reply) ;
      FrLocalFree(data) ;
      if (reply && status == FSE_Success)
         {
	 user_name = FrNewN(char,userlen) ;
	 memcpy(user_name,username,userlen) ;
	 user_password = FrNewN(char,passlen) ;
	 memcpy(user_password,password,passlen) ;
	 char *replydata = reply->packetData() ;
	 int maxpacket = FrLoadShort(replydata+0) ;
	 client_handle = FrLoadShort(replydata+2) ;
	 //!!! process the preferences we were given
	 (void)maxpacket ; (void)client_handle ;
	 return true ;
	 }
      }
   return false ;
}

//----------------------------------------------------------------------

bool FrClient::unregisterClient()
{
   if (user_name)
      {
      char data[2] ;
      FrStoreShort(client_handle,data+0) ;
      int status = server_request(connection,FSReq_UNREGISTER,
				  (int)sizeof(data),data,0,0) ;
      FrFree(user_name) ;
      user_name = 0 ;
      FrFree(user_password) ;
      user_password = 0 ;
      client_handle = -1 ;
      return (bool)(status == FSE_Success) ;
      }
   else
      return true ;			// always succeed if not registered
}

//----------------------------------------------------------------------

FrList *FrClient::registeredClients(int clientID, int handle,
				    const char *username) const
{
   FrList *clients = 0 ;
(void)clientID; (void)handle; (void)username;
   //!!!

   return clients ;
}

//----------------------------------------------------------------------

bool FrClient::getPreferences(int *broadcastmsg, int *personalmsg,
			      int *clientmsg, int *updatemsg,
			      char *clientdata, int datasize)
{
   char data[2] ;
   FrStoreShort(client_ident, data+0) ;
   FrPacket *reply ;
   int status = server_request(connection,FSReq_GETUSERPREF,
			       (int)sizeof(data),data,0,0,&reply) ;
   if (status == FSE_Success && reply)
      {
      char *replydata = reply->packetData() ;
      if (broadcastmsg)
	 *broadcastmsg = FrLoadByte(replydata+0) ;
      if (personalmsg)
	 *personalmsg = FrLoadByte(replydata+1) ;
      if (clientmsg)
	 *clientmsg = FrLoadByte(replydata+2) ;
      if (updatemsg)
	 *updatemsg = FrLoadByte(replydata+3) ;
      if (clientdata && datasize > 0)
	 memcpy(clientdata,replydata+4,datasize) ;
      return true ;
      }
   else
      return false ;
}

//----------------------------------------------------------------------

bool FrClient::setPreferences(int broadcastmsg, int personalmsg,
			      int clientmsg, int updatemsg,
			      const char *clientdata, int datasize,
			      FrAsyncCallback *callback, void *client_data)
{
   if (datasize < 0 || (datasize > 0 && !clientdata))
      {
      Fr_errno = EINVAL ;
      return false ;
      }
   FrLocalAlloc(char,data,1024,6+datasize) ;
   if (!data)
      {
      Fr_errno = ENOMEM ;
      return false ;
      }
   FrStoreShort(client_ident, data+0) ;
   FrStoreByte(broadcastmsg,  data+2) ;
   FrStoreByte(personalmsg,   data+3) ;
   FrStoreByte(clientmsg,     data+4) ;
   FrStoreByte(updatemsg,     data+5) ;
   if (datasize > 0)
      memcpy(data+6,clientdata,datasize) ;
   int status = server_request(connection,FSReq_SETUSERPREF,
			       (int)sizeof(data),data,
			       callback,client_data) ;
   FrLocalFree(data) ;
   return (bool)(status == FSE_Success) ;
}

//----------------------------------------------------------------------

FrRemoteDB *FrClient::openDatabase(const char *dbname, const char *password)
{
   if (!dbname || !*dbname)
      {
      Fr_errno = EINVAL ;
      return 0 ;
      }
   if (!password)
      {
      password = user_password ;
      if (!password)
	 password = "" ;
      }
   int dblen = strlen(dbname) + 1 ;
   int passlen = strlen(password) + 1 ;
   int datalen = 1 + dblen + passlen ;
   FrLocalAlloc(char,data,1000,datalen) ;
   if (!data)
      {
      Fr_errno = ENOMEM ;
      return 0 ;
      }
   FrStoreByte(0, data+0) ; //!!! db open flags
   memcpy(data+1,dbname,dblen) ;
   memcpy(data+1+dblen,password,passlen) ;
   FrPacket *reply ;
   int status = server_request(connection,FSReq_OPENDATABASE,
			       datalen, data, 0, 0, &reply) ;
   FrLocalFree(data) ;
   if (status == FSE_Success && reply)
      {
      char *replydata = reply->packetData() ;
      int dbhandle = FrLoadByte(replydata+0) ;
      int editor_config = FrLoadShort(replydata+1) ;
      (void)editor_config ; //!!!
      FrRemoteDB *db = new FrRemoteDB(connection,dbhandle) ;
      return db ;
      }
   return 0 ;
}

//----------------------------------------------------------------------

FrRemoteDB *FrClient::createDatabase(const char *dbname, const char *password,
				     const DBUserData *info)
{
   if (!dbname || !*dbname)
      {
      Fr_errno = EINVAL ;
      return 0 ;
      }
   if (!password)
      {
      password = user_password ;
      if (!password)
	 password = "" ;
      }
   int dblen = strlen(dbname) + 1 ;
   int passlen = strlen(password) + 1 ;
   int datalen = 92 + dblen + passlen ;
   FrLocalAlloc(char,data,1000,datalen) ;
   if (!data)
      {
      Fr_errno = ENOMEM ;
      return 0 ;
      }
   FrStoreLong(-1L,  data+0) ;		// estimated size in bytes
   FrStoreLong(-1L,  data+4) ;		// estimated size in records
   FrStoreShort(0,   data+8) ;		// ID for frame editor configuration
   memcpy(data+10,info,82) ;		// additional DB configuration info
   memcpy(data+92,dbname,dblen) ;
   memcpy(data+92+dblen,password,passlen) ;
   FrPacket *reply ;
   int status = server_request(connection,FSReq_CREATEDATABASE,
			       datalen, data, 0, 0, &reply) ;
   FrLocalFree(data) ;
   if (status == FSE_Success && reply)
      {
      char *replydata = reply->packetData() ;
      int dbhandle = FrLoadByte(replydata+0) ;
      FrRemoteDB *db = new FrRemoteDB(connection,dbhandle) ;
      return db ;
      }
   return 0 ;
}

//----------------------------------------------------------------------

FrRemoteDB *FrClient::getDBbyName(const char *dbname) const
{
(void)dbname ;
   return 0 ; //!!! for now
}

//----------------------------------------------------------------------

bool FrClient::closeDatabase(FrRemoteDB *db,
			     FrAsyncCallback *callback, void *client_data)
{
   if (!db)
      {
      Fr_errno = EINVAL ;
      return false ;
      }
   char data[1] ;
   FrStoreByte(db->dbHandle(),  data+0) ;
   int status = server_request(connection,FSReq_CLOSEDATABASE,
			       sizeof(data),data, callback,client_data) ;
   return (bool)(status == FSE_Success) ;
}

//----------------------------------------------------------------------

int FrClient::findIndex(const char *dbname, int indexnum, INDEXINFO *idxinfo)
{
   (void)dbname ; (void)indexnum ; (void)idxinfo ;
//!!!
   return -1 ;
}

//----------------------------------------------------------------------

bool FrClient::getUserData(const FrRemoteDB *db, DBUserData *userdata,
			   FrAsyncCallback *callback, void *client_data)
{
   if (!db || !userdata)
      {
      Fr_errno = EINVAL ;
      return false ;
      }
   char data[1] ;
   int dbhandle = db->dbHandle() ;
   FrStoreByte(dbhandle, data+0) ;
   FrPacket *reply ;
   int status = server_request(connection,FSReq_GETUSERDATA,
			       (int)sizeof(data),data,
			       callback,client_data,&reply) ;
   if (callback)
      return (bool)(status == FSE_Success) ;
   else if (status == FSE_Success && reply)
      {
      char *replydata = reply->packetData() ;
      memcpy(userdata,replydata+0,sizeof(DBUserData)) ;
      return true ;
      }
   else
      return false ;
}

//----------------------------------------------------------------------

bool FrClient::setUserData(const FrRemoteDB *db, DBUserData *userdata,
			   FrAsyncCallback *callback, void *client_data)
{
   if (!db || !userdata)
      {
      Fr_errno = EINVAL ;
      return false ;
      }
   char data[1+sizeof(*userdata)] ;
   int dbhandle = db->dbHandle() ;
   FrStoreByte(dbhandle, data+0) ;
   memcpy(data+1,userdata,sizeof(*userdata)) ;
   int status = server_request(connection,FSReq_SETUSERDATA,
			       (int)sizeof(data),data,
			       callback,client_data) ;
   return (bool)(status == FSE_Success) ;
}

//----------------------------------------------------------------------

FrNotifyHandler *FrClient::getNotify(int type)
{
   (void)type ;
   return 0 ; //!!! for now
}

//----------------------------------------------------------------------

void FrClient::setNotify(int type, FrNotifyHandler *handler)
{
   (void)type ; (void)handler ;
//!!!
}

//----------------------------------------------------------------------

char *FrClient::readDBIndex(const FrRemoteDB *db, int indexnum, bool keyonly,
			    const char *indexname,
			    FrAsyncCallback *callback, void *client_data)
{
   if (!db || (indexnum < 0 && indexnum != -1))
      {
      Fr_errno = EINVAL ;
      return 0 ;
      }
   if (indexnum != -1)
      indexname = "" ;
   else if (!indexname || !*indexname)
      {
      Fr_errno = EINVAL ;
      return 0 ;
      }
   int indexlen = strlen(indexname) + 1 ;
   int datalen = 3 + indexlen ;
   FrLocalAlloc(char,data,1000,datalen) ;
   if (!data)
      {
      Fr_errno = ENOMEM ;
      return 0 ;
      }
   int dbhandle = db->dbHandle() ;
   FrStoreByte(dbhandle,     data+0) ;
   FrStoreByte((int)keyonly, data+1) ;	// return key values only?
   FrStoreShort(indexnum,    data+2) ;	// which index to retrieve
   memcpy(data+3,indexname,indexlen) ;
   FrPacket *reply ;
   int status = server_request(connection,FSReq_GETDBINDEX,datalen,data,
			       callback,client_data,&reply) ;
   FrLocalFree(data) ;
   if (!callback && (status == FSE_Success))
      {
      char *userdata = FrNewN(char,sizeof(DBUserData)) ;
      memcpy(userdata,reply->packetData(),sizeof(DBUserData)) ;
      return userdata ;
      }
   else
      return 0 ;
}

//----------------------------------------------------------------------

bool FrClient::getFrame(const FrRemoteDB *db, FrSymbol *framename,
			FrAsyncCallback *callback, void *client_data)
{
   if (!db || !framename)
      {
      Fr_errno = EINVAL ;
      return false ;
      }
   int namelen = strlen(framename->symbolName()) + 1 ;
   int datalen = 3 + namelen ;
   FrLocalAlloc(char,data,1024,datalen) ;
   if (!data)
      {
      Fr_errno = ENOMEM ;
      return false ;
      }
   int dbhandle = db->dbHandle() ;
   FrStoreByte(dbhandle,  data+0) ;
   FrStoreByte(0,         data+1) ;	// no subclasses
   FrStoreByte(0,         data+2) ;	// no parts-of
   memcpy(data+3,framename->symbolName(),namelen) ;
   FrPacket *reply ;
   int status = server_request(connection,FSReq_GETFRAME,datalen,data,
			       callback,client_data,&reply) ;
   FrLocalFree(data) ;
   if (!callback && (status == FSE_Success))
      {
      const char *replydata = reply->packetData() ;
      (void)string_to_FrObject(replydata) ;
      }
   return (bool)(status == FSE_Success) ;
}

//----------------------------------------------------------------------

bool FrClient::getFrames(const FrRemoteDB *db, FrList *frames, bool subclasses,
			 bool parts_of,
			 FrAsyncCallback *callback, void *client_data)
{
   if (!db || !frames)
      {
      Fr_errno = EINVAL ;
      return false ;
      }
   int datalen = 3 ;
   for (FrList *l = frames ; l ; l = l->rest())
      datalen += FrObject_string_length(l->first()) + 1 ;
   FrLocalAlloc(char,data,1024,datalen) ;
   if (!data)
      {
      Fr_errno = ENOMEM ;
      return false ;
      }
   int dbhandle = db->dbHandle() ;
   FrStoreByte(dbhandle,        data+0) ;
   FrStoreByte((int)subclasses, data+1) ;
   FrStoreByte((int)parts_of,   data+2) ;
   char *ptr = data + 3 ;
   for (FrList *fr = frames ; fr ; fr = fr->rest())
      {
      ptr = fr->first()->print(ptr) + 1 ;
      }
   FrPacket *reply ;
   int status = server_request(connection,FSReq_GETFRAME,datalen,data,
			       callback,client_data,&reply) ;
   FrLocalFree(data) ;
   if (!callback && (status == FSE_Success))
      {
      const char *replydata = reply->packetData() ;
      int len = reply->packetLength() ;
      while (replydata && *replydata && len > 0)
	 {
	 len -= strlen(replydata) - 1 ;
	 (void)string_to_FrObject(replydata) ;
	 if (len > 0)
	    replydata++ ;		// skip NUL at end of frame rep.
	 }
      }
   return (bool)(status == FSE_Success) ;
}

//----------------------------------------------------------------------

bool FrClient::getOldFrame(const FrRemoteDB *db, FrSymbol *framename,
			   int generation, FrAsyncCallback *callback,
			   void *client_data)
{
   if (!db || !framename)
      {
      Fr_errno = EINVAL ;
      return false ;
      }
   int namelen = strlen(framename->symbolName()) + 1 ;
   int datalen = 3 + namelen ;
   FrLocalAlloc(char,data,1024,datalen) ;
   if (!data)
      {
      Fr_errno = ENOMEM ;
      return false ;
      }
   int dbhandle = db->dbHandle() ;
   FrStoreByte(dbhandle,    data+0) ;
   FrStoreShort(generation, data+1) ;
   memcpy(data+3,framename->symbolName(),namelen) ;
   int status = server_request(connection,FSReq_GETOLDFRAME,datalen,data,
			       callback,client_data) ;
   FrLocalFree(data) ;
   return (bool)(status == FSE_Success) ;
}

//----------------------------------------------------------------------

bool FrClient::createFrame(const FrRemoteDB *db, FrSymbol *framename,
			   FrAsyncCallback *callback, void *client_data)
{
   if (!db || !framename)
      {
      Fr_errno = EINVAL ;
      return false ;
      }
   int namelen = strlen(framename->symbolName()) + 1 ;
   int datalen = 2 + namelen ;
   FrLocalAlloc(char,data,1024,datalen) ;
   if (!data)
      {
      Fr_errno = ENOMEM ;
      return false ;
      }
   int dbhandle = db->dbHandle() ;
   FrStoreByte(dbhandle, data+0) ;
   FrStoreByte(0,        data+1) ;	// no gensym
   memcpy(data+2,framename->symbolName(),namelen) ;
   int status = server_request(connection,FSReq_CREATEFRAME,datalen,data,
			       callback,client_data) ;
   FrLocalFree(data) ;
   return (bool)(status == FSE_Success) ;
}

//----------------------------------------------------------------------

FrSymbol *FrClient::createNewFrame(const FrRemoteDB *db, FrSymbol *framename,
				 char **newname,
				 FrAsyncCallback *callback, void *client_data)
{
   if (!db || !framename)
      {
      Fr_errno = EINVAL ;
      return 0 ;
      }
   int namelen = strlen(framename->symbolName()) + 1 ;
   int datalen = 2 + namelen ;
   FrLocalAlloc(char,data,1024,datalen) ;
   if (!data)
      {
      Fr_errno = ENOMEM ;
      return 0 ;
      }
   int dbhandle = db->dbHandle() ;
   FrStoreByte(dbhandle, data+0) ;
   FrStoreByte(1,        data+1) ;	// do gensym if necessary
   memcpy(data+2,framename->symbolName(),namelen) ;
   FrPacket *reply ;
   int status = server_request(connection,FSReq_CREATEFRAME,datalen,data,
			       callback,client_data,&reply) ;
   FrLocalFree(data) ;
   if (callback)
      {
      Fr_errno = status ;
      return 0 ;
      }
   else if (status == FSE_Success)
      {
      const char *gensym_name = framename->symbolName() ; //!!!
      FrSymbol *new_name = symtab->add(gensym_name) ;
      int len = strlen(gensym_name)+1 ;
      *newname = FrNewN(char,len) ;
      memcpy(newname,gensym_name,len) ;
      return new_name ;
      }
   else
      return 0 ;
}

//----------------------------------------------------------------------

bool FrClient::deleteFrame(const FrRemoteDB *db, FrSymbol *framename,
			   const char *password,
			   FrAsyncCallback *callback,void *client_data)
{
   if (!db || !framename)
      {
      Fr_errno = EINVAL ;
      return false ;
      }
   if (!password)
      password = "" ;
   int namelen = strlen(framename->symbolName()) + 1 ;
   int passlen = strlen(password) + 1 ;
   int datalen = 1 + namelen + passlen ;
   FrLocalAlloc(char,data,1024,datalen) ;
   if (!data)
      {
      Fr_errno = ENOMEM ;
      return false ;
      }
   int dbhandle = db->dbHandle() ;
   FrStoreByte(dbhandle, data+0) ;
   memcpy(data+1,framename->symbolName(),namelen) ;
   memcpy(data+1+namelen,password,passlen) ;
   int status = server_request(connection,FSReq_DELETEFRAME,datalen,data,
			       callback,client_data) ;
   FrLocalFree(data) ;
   return (bool)(status == FSE_Success) ;
}

//----------------------------------------------------------------------

bool FrClient::updateFrame(const FrRemoteDB *db, FrSymbol *framename,
			   FrAsyncCallback *callback, void *client_data)
{
   if (!db || !framename)
      {
      Fr_errno = EINVAL ;
      return false ;
      }
   int namelen = strlen(framename->symbolName()) + 1 ;
   int datalen = 1 + namelen ;
   FrLocalAlloc(char,data,1024,datalen) ;
   if (!data)
      {
      Fr_errno = ENOMEM ;
      return false ;
      }
   int dbhandle = db->dbHandle() ;
   FrStoreByte(dbhandle, data+0) ;
   memcpy(data+1,framename->symbolName(),namelen) ;
   int status = server_request(connection,FSReq_UPDATEFRAME,datalen,data,
			       callback,client_data) ;
   FrLocalFree(data) ;
   return (bool)(status == FSE_Success) ;
}

//----------------------------------------------------------------------

int FrClient::proxyAdd(const FrRemoteDB *db, const FrSymbol *framename,
		       const FrSymbol *slot, const FrSymbol *facet,
		       const FrObject *filler, FrAsyncCallback *callback,
		       void *client_data)
{
   if (!db || !framename || !slot || !facet)
      {
      Fr_errno = EINVAL ;
      return -1 ;
      }
   int namelen = strlen(framename->symbolName()) + 1 ;
   int slotlen = strlen(slot->symbolName()) + 1 ;
   int facetlen = strlen(facet->symbolName()) + 1 ;
   int datalen = 4 + namelen + slotlen + facetlen +
		 FrObject_string_length(filler) ;
   FrLocalAlloc(char,data,1024,datalen) ;
   if (!data)
      {
      Fr_errno = ENOMEM ;
      return false ;
      }
   int dbhandle = db->dbHandle() ;
   FrStoreByte(dbhandle, data+0) ;
   FrStoreByte(0x00,     data+1) ;	// subfunction is 'add'
   char *ptr = data + 2 ;
   memcpy(ptr,framename->symbolName(),namelen) ;
   ptr += namelen ;
   memcpy(ptr,slot->symbolName(),slotlen) ;
   ptr += slotlen ;
   memcpy(ptr,facet->symbolName(),facetlen) ;
   ptr += facetlen ;
   filler->print(ptr) ;
   int status = server_request(connection,FSReq_PROXYUPDATE,datalen,data,
			       callback,client_data) ;
   FrLocalFree(data) ;
   return status ;
}


//----------------------------------------------------------------------

int FrClient::proxyDelete(const FrRemoteDB *db, const FrSymbol *framename,
			  const FrSymbol *slot, const FrSymbol *facet,
			  const FrObject *filler, FrAsyncCallback *callback,
			  void *client_data)
{
   if (!db || !framename || !slot || !facet)
      {
      Fr_errno = EINVAL ;
      return -1 ;
      }
   int namelen = strlen(framename->symbolName()) + 1 ;
   int slotlen = strlen(slot->symbolName()) + 1 ;
   int facetlen = strlen(facet->symbolName()) + 1 ;
   int datalen = 4 + namelen + slotlen + facetlen +
		 FrObject_string_length(filler) ;
   FrLocalAlloc(char,data,1024,datalen) ;
   if (!data)
      {
      Fr_errno = ENOMEM ;
      return false ;
      }
   int dbhandle = db->dbHandle() ;
   FrStoreByte(dbhandle, data+0) ;
   FrStoreByte(0x01,     data+1) ;	// subfunction is 'delete'
   char *ptr = data + 2 ;
   memcpy(ptr,framename->symbolName(),namelen) ;
   ptr += namelen ;
   memcpy(ptr,slot->symbolName(),slotlen) ;
   ptr += slotlen ;
   memcpy(ptr,facet->symbolName(),facetlen) ;
   ptr += facetlen ;
   filler->print(ptr) ;
   int status = server_request(connection,FSReq_PROXYUPDATE,datalen,data,
			       callback,client_data) ;
   FrLocalFree(data) ;
   return status ;
}


//----------------------------------------------------------------------

bool FrClient::lockFrame(const FrRemoteDB *db, const FrSymbol *framename,
			 FrAsyncCallback *callback, void *client_data)
{
   if (!db || !framename)
      {
      Fr_errno = EINVAL ;
      return false ;
      }
   int namelen = strlen(framename->symbolName()) + 1 ;
   int datalen = 1 + namelen ;
   FrLocalAlloc(char,data,1024,datalen) ;
   if (!data)
      {
      Fr_errno = ENOMEM ;
      return false ;
      }
   int dbhandle = db->dbHandle() ;
   FrStoreByte(dbhandle, data+0) ;
   memcpy(data+1,framename->symbolName(),namelen) ;
   int status = server_request(connection,FSReq_LOCKFRAME,datalen,
			       data, callback, client_data) ;
   FrLocalFree(data) ;
   return (bool)(status == FSE_Success) ;
}

//----------------------------------------------------------------------

bool FrClient::unlockFrame(const FrRemoteDB *db, const FrSymbol *framename,
			   FrAsyncCallback *callback, void *client_data)
{
   if (!db || !framename)
      {
      Fr_errno = EINVAL ;
      return false ;
      }
   int namelen = strlen(framename->symbolName()) + 1 ;
   int datalen = 1 + namelen ; 
   FrLocalAlloc(char,data,1024,datalen) ;
   if (!data)
      {
      Fr_errno = ENOMEM ;
      return false ;
      }
   int dbhandle = db->dbHandle() ;
   FrStoreByte(dbhandle, data+0) ;
   memcpy(data+1,framename->symbolName(),namelen) ;
   int status = server_request(connection,FSReq_UNLOCKFRAME,datalen,
			       data, callback, client_data) ;
   FrLocalFree(data) ;
   return (bool)(status == FSE_Success) ;
}

//----------------------------------------------------------------------

int FrClient::matchingFrames(const FrRemoteDB *db, int format, int len,
			     const char *criteria, FrList **matches,
			     FrAsyncCallback *callback, void *client_data) const
{
(void)db ; (void)format ; (void)len ; (void)criteria ; (void)matches ;
(void)callback ; (void)client_data ;
   return 0 ; //!!! for now -- no matches found
}
				
//----------------------------------------------------------------------

int FrClient::beginTransaction(const FrRemoteDB *db,
			       FrAsyncCallback *callback, void *client_data)
{
   if (!db)
      {
      Fr_errno = EINVAL ;
      return -1 ;
      }
   char data[1] ;
   int dbhandle = db->dbHandle() ;
   FrStoreByte(dbhandle, data+0) ;
   FrPacket *reply ;
   int status = server_request(connection,FSReq_BEGINTRANSACTION,sizeof(data),
			       data,callback,client_data,&reply) ;
   if (callback)
      return (status == FSE_Success) ? 0 : -1 ;
   else if (reply && status == FSE_Success)
      {
      char *replydata = reply->packetData() ;
      int transaction = FrLoadShort(replydata+0) ;
      return transaction ;
      }
   else
      return -1 ;
}

//----------------------------------------------------------------------

bool FrClient::endTransaction(const FrRemoteDB *db, int transaction,
			      FrAsyncCallback *callback, void *client_data)
{
   if (!db)
      {
      Fr_errno = EINVAL ;
      return false ;
      }
   char data[3] ;
   int dbhandle = db->dbHandle() ;
   FrStoreByte(dbhandle,     data+0) ;
   FrStoreShort(transaction, data+1) ;
   int status = server_request(connection,FSReq_ENDTRANSACTION,sizeof(data),
			       data, callback,client_data) ;
   return (bool)(status == FSE_Success) ;
}

//----------------------------------------------------------------------

bool FrClient::abortTransaction(const FrRemoteDB *db, int transaction,
				FrAsyncCallback *callback, void *client_data)
{
   if (!db)
      {
      Fr_errno = EINVAL ;
      return false ;
      }
   char data[3] ;
   int dbhandle = db->dbHandle() ;
   FrStoreByte(dbhandle,     data+0) ;
   FrStoreShort(transaction, data+1) ;
   int status = server_request(connection,FSReq_ABORTTRANSACTION,sizeof(data),
			       data, callback,client_data) ;
   return (bool)(status == FSE_Success) ;
}

//----------------------------------------------------------------------

bool FrClient::personalMessage(int clienthandle, int urgent, int length,
			       const char *message,FrAsyncCallback *callback,
			       void *client_data)
{
   if (!message)
      {
      Fr_errno = EINVAL ;
      return false ;
      }
   if (length > 255)
      {
      Fr_errno = E2BIG ;
      return false ;
      }
   int datalen = 4 + length ;
   FrLocalAlloc(char,data,1024,datalen) ;
   if (!data)
      {
      Fr_errno = ENOMEM ;
      return false ;
      }
   FrStoreShort(clienthandle, data+0) ;
   FrStoreByte(urgent,        data+2) ;
   FrStoreByte(length,        data+3) ;
   memcpy(data+4,message,length) ;
   int status = server_request(connection,FSReq_SENDPERSONALMSG,datalen,data,
			       callback,client_data) ;
   FrLocalFree(data) ;
   return (bool)(status == FSE_Success) ;
}

//----------------------------------------------------------------------

int FrClient::broadcastMessage(int length, const char *message,
			       FrAsyncCallback *callback, void *client_data)
{
   if (!message)
      {
      Fr_errno = EINVAL ;
      return false ;
      }
   if (length > 255)
      {
      Fr_errno = E2BIG ;
      return false ;
      }
   int datalen = 1 + length ;
   FrLocalAlloc(char,data,1024,datalen) ;
   if (!data)
      {
      Fr_errno = ENOMEM ;
      return false ;
      }
   FrStoreByte(length, data+0) ;
   memcpy(data+1,message,length) ;
   FrPacket *reply ;
   int status = server_request(connection,FSReq_SENDBROADCASTMSG,datalen,data,
			       callback,client_data,&reply) ;
   FrLocalFree(data) ;
   if (callback)
      return (status == FSE_Success) ? 0 : -1 ;
   else if (reply && status == FSE_Success)
      {
      char *replydata = reply->packetData() ;
      int missed = FrLoadShort(replydata+0) ;
      return missed ;
      }
   else
      return -1 ;
}

//----------------------------------------------------------------------

bool FrClient::clientMessage(int clienthandle, uint16_t msgtype, int &length,
			     const char *&message,FrAsyncCallback *callback,
			     void *client_data)
{
   if (!message)
      {
      Fr_errno = EINVAL ;
      return false ;
      }
   if (length > 255)
      {
      Fr_errno = E2BIG ;
      return false ;
      }
   int datalen = 5 + length ;
   FrLocalAlloc(char,data,1024,datalen) ;
   if (!data)
      {
      Fr_errno = ENOMEM ;
      return false ;
      }
   FrStoreShort(clienthandle, data+0) ;
   FrStoreShort(msgtype,      data+2) ;
   FrStoreByte(length,        data+4) ;
   memcpy(data+5,message,length) ;
   FrPacket *reply ;
   int status = server_request(connection,FSReq_SENDCLIENTMSG,datalen,data,
			       callback,client_data,&reply) ;
   FrLocalFree(data) ;
   if (callback)
      return (bool)(status == FSE_Success) ;
   else if (reply && status == FSE_Success)
      {
      char *replydata = reply->packetData() ;
      int datlen = FrLoadByte(replydata+0) ;
      if (datlen > 0)
	 {
	 length = datlen ;
	 message = FrNewN(char,datlen) ;
	 memcpy(VoidPtr(message),replydata+1,datlen) ;
	 }
      else
	 {
	 length = 0 ;
	 message = 0 ;
	 }
      return true ;
      }
   else
      return false ;
}

//----------------------------------------------------------------------

bool FrClient::serverStatistics(int stats, char *statbuffer,
				FrAsyncCallback *callback,
				void *client_data)
{
   if (!statbuffer)
      {
      Fr_errno = EINVAL ;
      return false ;
      }
   char data[1] ;
   FrStoreByte(stats, data+0) ;
   FrPacket *reply ;
   int status = server_request(connection,FSReq_SERVERSTATISTICS,
			       sizeof(data),data,callback,client_data,&reply) ;
   if (callback)
      return (bool)(status == FSE_Success) ;
   else if (status == FSE_Success)
      {
      switch (stats)
	 {
	 case 0:
      //!!!
	 default:
	    break ;
	 }
      return true ;
      }
   else
      return false ;
}

//----------------------------------------------------------------------

long int FrClient::isServerStillUp(FrAsyncCallback *callback,
				   void *client_data)
{
   FrPacket *reply ;
   int status = server_request(connection,FSReq_HOWAREYOU,0,0,callback,
			       client_data,&reply) ;
   if (callback)
      return (status == FSE_Success) ? -1L : 0L ;
   else if (reply && status == FSE_Success)
      {
      char *replydata = reply->packetData() ;
      long int pendingrequests = FrLoadLong(replydata+0) ;
      return pendingrequests ;
      }
   else
      return -1L ;
}

//----------------------------------------------------------------------

static void process_exception(FrClient *client, FrQueue *queue)
{
   (void)client ; (void)queue ; //!!!
}

//----------------------------------------------------------------------

static void process_incoming_data(FrClient *client, FrQueue *queue)
{
   (void)client ; (void)queue ; //!!!
}

//----------------------------------------------------------------------

static void process_completed_packet(FrPacket *packet)
{
   (void)packet ; //!!!
}

//----------------------------------------------------------------------

int FrClient::process(int timeout)
{
   if (timeout < 0)
      timeout = 0 ;
   struct timeval timelimit ;
   timelimit.tv_sec = timeout / ONE_MILLION ;
   timelimit.tv_usec = timeout % ONE_MILLION ;
   fd_set readfds = connection_fdset ;
   fd_set exceptfds = connection_fdset ;
   int numselected = select(FD_SETSIZE,&readfds,NULL,&exceptfds,&timelimit) ;
   if (numselected < 0)
      {
      Fr_errno = FSE_SOCKETERROR ;
      return -1 ;
      }
   if (numselected > 0)
      {
      for (FrClient *client = client_list ; client ; client = client->next)
	 {
	 current_connection = client->getConnection() ;
	 int sock = client->connectionSocket() ;
	 FrQueue *queue = current_connection->getQueue() ;
	 if (FD_ISSET(sock,&exceptfds))
	    process_exception(client,queue) ;
	 if (FD_ISSET(sock,&readfds))
	    process_incoming_data(client,queue) ;
	 current_connection = 0 ;
	 int qlen = queue->length() ;
	 while (qlen > 0)
	    {
	    FrPacket *packet = (FrPacket*)queue->pop() ;
	    if (packet)
	       {
	       if (packet->packetComplete())
		  process_completed_packet(packet) ;
	       else
		  queue->add(packet) ;	// put back on queue until complete
	       }
	    qlen-- ;			// we've processed another packet
	    }
	 }
      return 1 ;
      }
   else
      return 0 ;
}

//----------------------------------------------------------------------

FrIdleFunc *FrClient::setIdleFunc(FrIdleFunc *idle_function)
{
(void)idle_function ;

   return 0 ; // for now!!!
}

//----------------------------------------------------------------------

bool FrClient::connected() const
{
   return (bool)(connection != 0 && connection->connectionSocket() != -1) ;
}

//----------------------------------------------------------------------

bool FrClient::registered() const
{
   return (bool)(user_name != 0) ;
}

//----------------------------------------------------------------------

int FrClient::connectionSocket() const
{
   return connection ? connection->connectionSocket() : -1 ;
}

//----------------------------------------------------------------------

// end of file frclient.cpp //
