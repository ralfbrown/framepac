/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frserver.cpp	    network-server code				*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1995,1996,1997,1998,2001,2006,2009,2011,2015		*/
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
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(unix) || defined(__linux__) || defined(__GNUC__)
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <netinet/in.h>
#endif /* unix */
#if defined(__MSDOS__) || defined(MSDOS)
#  include <io.h>
#  include <time.h>
#elif defined(__WINDOWS__) || defined(__NT__) || defined(_WIN32)
#  include <io.h>
#  include <sys/stat.h>
#  include "frconfig.h"
#  include <winsock.h>
#endif

#include "frhash.h"
#include "frstruct.h"
#include "frfinddb.h"
#include "frpcglbl.h"
#include "frclisrv.h"
#include "frserver.h"
#include "frconnec.h"
#include "frevent.h"

#define NOTIFICATION_TIMEOUT 30

#ifdef __WATCOMC__

#if !defined(__WINDOWS__) && !defined(__NT__)
struct sockaddr ;
struct msghdr ;
struct timeval { long int tv_sec, tv_usec ; } ;
struct fd_set { char bits[10] ; } ;
#define FD_SETSIZE 80
#define FD_ISSET(s,fdset) (((fdset)->bits[s/8]&(1<<(s%8)))!=0)
#define FD_SET(s,fdset) (((fdset)->bits[s/8]|=(char)(1<<(s%8))))
#define FD_CLR(s,fdset) (((fdset)->bits[s/8]&=(char)(~(1<<(s%8)))))
extern "C" int select(int size,fd_set *read, fd_set *write, fd_set *exc,
		      timeval *limit) ;
#define AF_INET 0
#define PF_INET 0
#define SOCK_STREAM 0
#define INADDR_ANY (-1)
struct sockaddr_in { int sin_family ;
		     int sin_port ;
		     struct { int s_addr ;} sin_addr ; } ;
struct sockaddr { int sin_family ;
		  int sin_port ; } ;
extern "C" int close(int) ;
extern "C" int listen(int,int) ;
#endif /* !__WINDOWS__ && !__NT__ */
#endif /* __WATCOMC__ */

class FrEventList ;
class FrEvent ;

#ifdef FrSERVER

/************************************************************************/
/*    Types local to this module					*/
/************************************************************************/

class FrServerDB : public FrObject
   {
   private:
      FrSymbolTable *symtab ;
      FrList *users ;
      char *dbname ;
      FrSymHashTable *locks ;
   public:
      FrServerDB(const char *name) ;
      virtual ~FrServerDB() ;
      bool addUser(FrConnection *conn) ;
      bool removeUser(FrConnection *conn) ;
      FrSymbolTable *select() { return symtab->select() ; }

      // access to internal state
      bool good() const { return dbname && symtab ; }
      const char *name() const { return dbname ; }
      const FrList *getUsers() const { return users ; }
      const bool inUse() const { return users != 0 ; }
   } ;

//----------------------------------------------------------------------

class FrServerClient : public FrObject
   {
   private:
      int handle ;
      int identifier ;
      FrConnection *connection ;
      FrStruct *userinfo ;
      char *username ;
   public:
      FrServerClient(FrConnection *connect) ;
      virtual ~FrServerClient() ;
      void setName(const char *name) ;
      void setID(int ident) ;

      // access to internal state
      int clientHandle() const { return handle ; }
      int clientID() const { return identifier ; }
      const char *userName() const { return username ; }
      FrStruct *userInfo() const { return userinfo ; }
      FrConnection *netConnection() const { return connection ; }
   } ;

//----------------------------------------------------------------------

class FrServerPrivate ;
typedef int FrServerPrivate::*FrServerProc(FrServerPrivate *server,
					   FrPacket *request) ;

struct FrServerProcPtr
   {
     int reqnum ;
     FrServerProc *func ;
   } ;

class FrServerPrivate : public FrServer
   {
   private:
      static bool procs_initialized ;
      static FrServerProcPtr server_proc_ptrs[] ;
      static FrServerProc *server_procs[256] ;
   public:
      void server_message(const char *msg) ;
      void __FrCDECL trace(const char *fmt, ...) ;
      void abort_bad_connection(FrConnection *connection) ;
      int process_request(FrPacket *request) ;
      static int identify_self(FrServerPrivate *server, FrPacket *request) ;
      static int register_client(FrServerPrivate *server, FrPacket *request) ;
      static int unregister_client(FrServerPrivate *server, FrPacket *request) ;
      static int get_database_list(FrServerPrivate *server, FrPacket *request) ;
      static int get_index_info(FrServerPrivate *server, FrPacket *request) ;
      static int find_clients(FrServerPrivate *server, FrPacket *request) ;
      static int get_user_preferences(FrServerPrivate *server, FrPacket *request) ;
      static int set_user_preferences(FrServerPrivate *server, FrPacket *request) ;
      static int open_database(FrServerPrivate *server, FrPacket *request) ;
      static int create_database(FrServerPrivate *server, FrPacket *request) ;
      static int close_database(FrServerPrivate *server, FrPacket *request) ;
      static int create_index(FrServerPrivate *server, FrPacket *request) ;
      static int get_database_index(FrServerPrivate *server, FrPacket *request) ;
      static int get_frame(FrServerPrivate *server, FrPacket *request) ;
      static int get_old_frame(FrServerPrivate *server, FrPacket *request) ;
      static int get_fillers(FrServerPrivate *server, FrPacket *request) ;
      static int lock_frame(FrServerPrivate *server, FrPacket *request) ;
      static int unlock_frame(FrServerPrivate *server, FrPacket *request) ;
      static int create_frame(FrServerPrivate *server, FrPacket *request) ;
      static int delete_frame(FrServerPrivate *server, FrPacket *request) ;
      static int revert_frame(FrServerPrivate *server, FrPacket *request) ;
      static int update_frame(FrServerPrivate *server, FrPacket *request) ;
      static int begin_transaction(FrServerPrivate *server, FrPacket *request) ;
      static int end_transaction(FrServerPrivate *server, FrPacket *request) ;
      static int abort_transaction(FrServerPrivate *server, FrPacket *request) ;
      static int send_personal_message(FrServerPrivate *server, FrPacket *request) ;
      static int send_broadcast_message(FrServerPrivate *server, FrPacket *request) ;
      static int send_client_message(FrServerPrivate *server, FrPacket *request) ;
      static int test_isa_p(FrServerPrivate *server, FrPacket *request) ;
      static int check_restrictions(FrServerPrivate *server, FrPacket *request) ;
      static int indexed_retrieval(FrServerPrivate *server, FrPacket *request) ;
      static int server_statistics(FrServerPrivate *server, FrPacket *request) ;
      static int how_are_you(FrServerPrivate *server, FrPacket *request) ;
      static int get_system_config(FrServerPrivate *server, FrPacket *request) ;
      static int set_system_config(FrServerPrivate *server, FrPacket *request) ;
      static int get_user_data(FrServerPrivate *server, FrPacket *request) ;
      static int set_user_data(FrServerPrivate *server, FrPacket *request) ;
      static int proxy_update(FrServerPrivate *server, FrPacket *request) ;
      static int are_you_there(FrServerPrivate *server, FrPacket *request) ;
      static int terminating_connection(FrServerPrivate *server, FrPacket *request) ;
      static int server_going_down(FrServerPrivate *server, FrPacket *request) ;
      static int broadcast_message(FrServerPrivate *server, FrPacket *request) ;
      static int personal_message(FrServerPrivate *server, FrPacket *request) ;
      static int client_message(FrServerPrivate *server, FrPacket *request) ;
      static int client_message_timeout(FrServerPrivate *server, FrPacket *request) ;
      static int frame_locked(FrServerPrivate *server, FrPacket *request) ;
      static int frame_unlocked(FrServerPrivate *server, FrPacket *request) ;
      static int frame_created(FrServerPrivate *server, FrPacket *request) ;
      static int frame_deleted(FrServerPrivate *server, FrPacket *request) ;
      static int frame_updated(FrServerPrivate *server, FrPacket *request) ;
      static int check_activity(FrServerPrivate *server, FrPacket *request) ;
      static int discard_frame(FrServerPrivate *server, FrPacket *request) ;
      static int proxy_update_notification(FrServerPrivate *server, FrPacket *request) ;
      static int inheritable_facets(FrServerPrivate *server, FrPacket *request) ;
      static int peer_handoff(FrServerPrivate *server, FrPacket *request) ;
      static int inherit_all_fillers(FrServerPrivate *server, FrPacket *request) ;
      static int peer_handed_off(FrServerPrivate *server, FrPacket *request) ;
   } ;

//----------------------------------------------------------------------

class FrServerStatistics
   {
   protected:
      size_t requests[FSReq_Last+1] ;
      size_t notifications[FSNot_Last-FSNot_First+1] ;
      size_t databases_opened ;
      size_t client_registrations ;
      size_t crashed_clients ;
      size_t transactions_successful ;
      size_t transactions_aborted ;
      size_t transactions_open ;
      size_t pending_requests ;
      size_t outstanding_notifications ;
      unsigned curr_clients ;
      unsigned max_clients ;
   public:
      FrServerStatistics()
	    { memset((char*)this,'\0',sizeof(FrServerStatistics)) ; }
      void addClient()
	    { curr_clients++ ; client_registrations++ ;
	      if (curr_clients > max_clients) max_clients = curr_clients ; }
      void removeClient() { if (curr_clients > 0) --curr_clients ; }
      void addCrash() { crashed_clients++ ; }
      void addOpen() { databases_opened++ ; }
      void startTransaction() { transactions_open++ ; }
      void abortTransaction()
	    { if (transactions_open > 0)
		{ transactions_open-- ; transactions_aborted++ ; }}
      void endTransaction()
	    { if (transactions_open > 0)
		{ transactions_open-- ; transactions_successful++ ; }}
      void addRequest(int N)
	    { if (N >= 0 && N <= FSReq_Last)
		 { requests[N]++ ; pending_requests++ ; }}
      void handledRequest() { if (pending_requests > 0) pending_requests-- ; }
      void addNotify(int N)
	    { if (N >= FSNot_First && N <= FSNot_Last)
		 { notifications[N-FSNot_First]++ ;
		   outstanding_notifications++ ; }}
      void repliedNotify() { if (outstanding_notifications > 0)
				outstanding_notifications-- ; }

      // access to statistics variables
      unsigned numClients() const { return curr_clients ; }
      unsigned maxClients() const { return max_clients ; }
      size_t numRegistrations() const { return client_registrations ; }
      size_t numCrashes() const { return crashed_clients ; }
      size_t numOpens() const { return databases_opened ; }
      size_t numTransSuccessful() const { return transactions_successful ; }
      size_t numTransAborted() const { return transactions_aborted ; }
      size_t numTransPending() const { return transactions_open ; }
      size_t numPendingRequests() const { return pending_requests ; }
      size_t numNotifications() const { return outstanding_notifications ; }
   } ;

/************************************************************************/
/*    Global variables local to this module				*/
/************************************************************************/

static struct fd_set connect_fdset ;	// bitmap of open connections
static FrList *connection_list = 0 ;
static FrConnection *current_connection ;
static int current_client_handle = 0 ;	// counter for client handles

static FrServerDB *open_databases[FrSRV_MAX_DATABASES] ;
static FrList *active_clients = 0 ;

static bool server_tracing_enabled = true ;

static FrServer *peer_mode_server = 0 ;

FrServerProc *FrServerPrivate::server_procs[256] ;
bool FrServerPrivate::procs_initialized = false ;

static FrEventList *server_events = 0 ;

/************************************************************************/
/*    Global data local to this module					*/
/************************************************************************/

FrServerProcPtr FrServerPrivate::server_proc_ptrs[] =
   {
     { FSReq_IDENTIFY, (FrServerProc*)&FrServerPrivate::identify_self },
     { FSReq_REGISTER, (FrServerProc*)&FrServerPrivate::register_client },
     { FSReq_UNREGISTER, (FrServerProc*)&FrServerPrivate::unregister_client },
     { FSReq_GETDATABASELIST, (FrServerProc*)&FrServerPrivate::get_database_list },
     { FSReq_GETINDEXINFO, (FrServerProc*)&FrServerPrivate::get_index_info },
     { FSReq_FINDCLIENTS, (FrServerProc*)&FrServerPrivate::find_clients },
     { FSReq_GETUSERPREF, (FrServerProc*)&FrServerPrivate::get_user_preferences },
     { FSReq_SETUSERPREF, (FrServerProc*)&FrServerPrivate::set_user_preferences },
     { FSReq_OPENDATABASE, (FrServerProc*)&FrServerPrivate::open_database },
     { FSReq_CREATEDATABASE, (FrServerProc*)&FrServerPrivate::create_database },
     { FSReq_CLOSEDATABASE, (FrServerProc*)&FrServerPrivate::close_database },
     { FSReq_CREATEINDEX, (FrServerProc*)&FrServerPrivate::create_index },
     { FSReq_GETDBINDEX, (FrServerProc*)&FrServerPrivate::get_database_index },
     { FSReq_GETFRAME, (FrServerProc*)&FrServerPrivate::get_frame },
     { FSReq_GETOLDFRAME, (FrServerProc*)&FrServerPrivate::get_old_frame },
     { FSReq_GETFILLERS, (FrServerProc*)&FrServerPrivate::get_fillers },
     { FSReq_LOCKFRAME, (FrServerProc*)&FrServerPrivate::lock_frame },
     { FSReq_UNLOCKFRAME, (FrServerProc*)&FrServerPrivate::unlock_frame },
     { FSReq_CREATEFRAME, (FrServerProc*)&FrServerPrivate::create_frame },
     { FSReq_DELETEFRAME, (FrServerProc*)&FrServerPrivate::delete_frame },
     { FSReq_REVERTFRAME, (FrServerProc*)&FrServerPrivate::revert_frame },
     { FSReq_UPDATEFRAME, (FrServerProc*)&FrServerPrivate::update_frame },
     { FSReq_BEGINTRANSACTION, (FrServerProc*)&FrServerPrivate::begin_transaction },
     { FSReq_ENDTRANSACTION, (FrServerProc*)&FrServerPrivate::end_transaction },
     { FSReq_ABORTTRANSACTION, (FrServerProc*)&FrServerPrivate::abort_transaction },
     { FSReq_SENDPERSONALMSG, (FrServerProc*)&FrServerPrivate::send_personal_message },
     { FSReq_SENDBROADCASTMSG, (FrServerProc*)&FrServerPrivate::send_broadcast_message },
     { FSReq_SENDCLIENTMSG, (FrServerProc*)&FrServerPrivate::send_client_message },
     { FSReq_TEST_ISA_P, (FrServerProc*)&FrServerPrivate::test_isa_p },
     { FSReq_CHECKRESTRICTIONS, (FrServerProc*)&FrServerPrivate::check_restrictions },
     { FSReq_INDEXEDRETRIEVAL, (FrServerProc*)&FrServerPrivate::indexed_retrieval },
     { FSReq_SERVERSTATISTICS, (FrServerProc*)&FrServerPrivate::server_statistics },
     { FSReq_HOWAREYOU, (FrServerProc*)&FrServerPrivate::how_are_you },
     { FSReq_GETSYSTEMCONFIG, (FrServerProc*)&FrServerPrivate::get_system_config },
     { FSReq_SETSYSTEMCONFIG, (FrServerProc*)&FrServerPrivate::set_system_config },
     { FSReq_GETUSERDATA, (FrServerProc*)&FrServerPrivate::get_user_data },
     { FSReq_SETUSERDATA, (FrServerProc*)&FrServerPrivate::set_user_data },
     { FSReq_PROXYUPDATE, (FrServerProc*)&FrServerPrivate::proxy_update },
     { FSReq_INHERITABLEFACETS, (FrServerProc*)&FrServerPrivate::inheritable_facets },
     { FSReq_INHERITFILLERS, (FrServerProc*)&FrServerPrivate::inherit_all_fillers },
     { FSNot_AREYOUTHERE, (FrServerProc*)&FrServerPrivate::are_you_there },
     { FSNot_TERMINATINGCONN, (FrServerProc*)&FrServerPrivate::terminating_connection },
     { FSNot_SERVERGOINGDOWN, (FrServerProc*)&FrServerPrivate::server_going_down },
     { FSNot_BROADCASTMSG, (FrServerProc*)&FrServerPrivate::broadcast_message },
     { FSNot_PERSONALMSG, (FrServerProc*)&FrServerPrivate::personal_message },
     { FSNot_CLIENTMSG, (FrServerProc*)&FrServerPrivate::client_message },
     { FSNot_CLIENTMSGTIMEOUT, (FrServerProc*)&FrServerPrivate::client_message_timeout },
     { FSNot_FRAMELOCKED, (FrServerProc*)&FrServerPrivate::frame_locked },
     { FSNot_FRAMEUNLOCKED, (FrServerProc*)&FrServerPrivate::frame_unlocked },
     { FSNot_FRAMECREATED, (FrServerProc*)&FrServerPrivate::frame_created },
     { FSNot_FRAMEDELETED, (FrServerProc*)&FrServerPrivate::frame_deleted },
     { FSNot_FRAMEUPDATED, (FrServerProc*)&FrServerPrivate::frame_updated },
     { FSNot_CHECKACTIVITY, (FrServerProc*)&FrServerPrivate::check_activity },
     { FSNot_DISCARDFRAME, (FrServerProc*)&FrServerPrivate::discard_frame },
     { FSNot_PROXYUPDATE, (FrServerProc*)&FrServerPrivate::proxy_update_notification },
     { FSNot_PEERHANDOFF, (FrServerProc*)&FrServerPrivate::peer_handoff },
     { FSNot_NEWCONTROLLER, (FrServerProc*)&FrServerPrivate::peer_handed_off },
   } ;

/************************************************************************/
/*    Forward declarations						*/
/************************************************************************/

static bool close_connection(FrConnection *connection) ;

/************************************************************************/
/*    Global variables which are visible from other modules		*/
/************************************************************************/

/************************************************************************/
/*    Manifest constants for this module				*/
/************************************************************************/

#define ONE_MILLION (1000000L)

#define MAX_BACKLOG 5

/************************************************************************/
/*    Utility Functions							*/
/************************************************************************/

static int list_to_netdata(FrList *list, char *&buffer, bool incl_count = true)
{
   FrList *l ;
   int count = 0 ;
   int len = 0 ;
   for (l = list ; l ; l = l->rest())
      {
      len += FrObject_string_length(l->first()) + 1 ;
      count++ ;
      }
   if (incl_count)
      len += 2 ;
   buffer = FrNewN(char,len) ;
   if (!buffer)
      return -1 ;
   char *buf = buffer ;
   if (incl_count)
      {
      FrStoreShort(count,buf) ;
      buf += 2 ;
      }
   for (l = list ; l ; l = l->rest())
      buf = l->first()->print(buf) + 1 ;
   return len ;
}

//----------------------------------------------------------------------

static FrList *netdata_to_list(const char *start, const char *end)
{
   FrList *l = 0 ;
   while (start < end && *start)
      {
      FrObject *obj = FrSymbolTable::add(start) ;
      start = strchr(start,'\0') + 1 ;
      pushlist(obj,l) ;
      }
   return listreverse(l) ;
}

//----------------------------------------------------------------------

static FrInheritanceType netinherit_to_fpinherit(int type)
{
   switch (type)
      {
      case 0:
	 return NoInherit ;
      case 1:
	 return InheritDFS ;
      case 2:
	 return InheritLocalDFS ;
      case 3:
	 return InheritPartDFS ;
      case 4:
	 return InheritBFS ;
      case 5:
	 return InheritLocalBFS ;
      case 6:
	 return InheritPartBFS ;
      default:
	 return (FrInheritanceType)-1 ;
      }
}

//----------------------------------------------------------------------

FrSymbolTable *select_database(FrConnection *connection, int dbhandle)
{
   if (!connection || dbhandle < 0 || dbhandle >= FrSRV_MAX_DATABASES)
      return 0 ;
   FrServerData *server_data = connection->serverData() ;
   if (!server_data)
      return 0 ;
   FrSymbolTable *symtab ;
   FrServerDB *db = server_data->selectDatabase(dbhandle,&symtab) ;
   if (!db)
      {
      Fr_errno = FSE_NOSUCHDATABASE ;
      //!!!
      return 0 ;
      }
   else
      return symtab ;
}

//----------------------------------------------------------------------

static FrServerClient *select_client(int client_handle)
{
   for (FrList *cl = active_clients ; cl ; cl = cl->rest())
      {
      FrServerClient *client = (FrServerClient*)cl->first() ;
      if (client->clientHandle() == client_handle)
	 return client ;
      }
   return 0 ;
}

//----------------------------------------------------------------------

static FrServerClient *select_client(FrPacket *request)
{
   FrConnection *conn = request->getConnection() ;
   for (FrList *cl = active_clients ; cl ; cl = cl->rest())
      {
      FrServerClient *client = (FrServerClient*)cl->first() ;
      if (client->netConnection() == conn)
	 return client ;
      }
   return 0 ;
}

//----------------------------------------------------------------------

static time_t notification_timed_out(void *user_data)
{
   FrPacket *packet = (FrPacket*)user_data ;
   if (packet)
      {
      //server->repliedNotify() ;
   //!!!
      }
   return 0 ;				// don't reschedule event
}

//----------------------------------------------------------------------

static FrServerDB *always_zero() { return 0 ; }

//----------------------------------------------------------------------

static void notify_database_users(FrPacket *request, FrSymbol *frame)
{
   if (!request || !frame)
      return ;
   int reqcode = request->requestCode() ;
   int notcode ;
   switch (reqcode)
      {
      case FSReq_LOCKFRAME:
	 notcode = FSNot_FRAMELOCKED ;
	 break ;
      case FSReq_UNLOCKFRAME:
	 notcode = FSNot_FRAMEUNLOCKED ;
	 break ;
      case FSReq_CREATEFRAME:
	 notcode = FSNot_FRAMECREATED ;
	 break ;
      case FSReq_DELETEFRAME:
	 notcode = FSNot_FRAMEDELETED ;
	 break ;
      case FSReq_UPDATEFRAME:
	 notcode = FSNot_FRAMEUPDATED ;
	 break ;
      default:
	 return ;
      }
   FrEvent *timeout = server_events->addEvent(NOTIFICATION_TIMEOUT,
					      notification_timed_out,
					      request,true) ;
   FrServerDB *db = always_zero() ; //!!! for now
   if (db)
      {
      FrConnection *originator = request->getConnection() ;
      const FrList *users = db->getUsers() ;
      int namelen = strlen(frame->symbolName()) + 1 ;
      int len = namelen + 1 ;
      FrLocalAlloc(char,buf,512,len) ;
      if (!buf)
	 {
	 FrWarning("out of memory while notifying database users") ;
	 return ;
	 }
      memcpy(buf+1,frame->symbolName(),namelen) ;
      for ( ; users ; users = users->rest())
	 {
	 FrConnection *user = (FrConnection*)users->first() ;
	 if (user != originator)
	    {
	    int dbhandle = 0 ; //!!!
	    FrStoreByte(dbhandle,buf+0) ;
	    user->sendNotification(request,notcode,len,buf) ;
	    }
	 }
      FrLocalFree(buf) ;
      }
   (void)timeout ; //!!!for now
}

/************************************************************************/
/*    Network Functions							*/
/************************************************************************/

//----------------------------------------------------------------------

static int start_listening(int base_port, int limit_port, int *server_port)
{
   int server_socket = socket(PF_INET, SOCK_STREAM, 0) ;
   if (server_socket < 0)
      return -1 ;
   struct sockaddr_in address ;
   address.sin_addr.s_addr = INADDR_ANY ;
   address.sin_family = AF_INET ;
   int successful ;
   // prepare to listen on one of the ports in the specified range
   do {
      address.sin_port = (short)base_port++ ;
      successful = bind(server_socket, (sockaddr*)&address, sizeof(address)) ;
      } while (base_port <= limit_port && successful == -1) ;
   if (successful == -1)
      {
      close(server_socket) ;
      server_socket = -1 ;
      }
   else
      listen(server_socket,MAX_BACKLOG) ;
   if (server_port)
      *server_port = address.sin_port ;
   return server_socket ;
}

//----------------------------------------------------------------------

static FrConnection *accept_connection(int server_socket, int port)
{
   struct sockaddr_in address ;
   address.sin_port = (short)port ;
   address.sin_addr.s_addr = INADDR_ANY ;  // accept any address
   address.sin_family = AF_INET ;	// user Internet protocols
   int addrsize = sizeof(address) ;
   int newsock = accept(server_socket,(sockaddr*)&address,&addrsize) ;
   if (newsock < 0)
      return 0 ;
   FrConnection *conn = new FrConnection(newsock) ;
   FD_SET(newsock,&connect_fdset) ;
   pushlist(conn,connection_list) ;
   return conn ;
}

//----------------------------------------------------------------------

static bool close_connection(FrConnection *connection)
{
   connection_list = listremove(connection_list,connection,::equal) ;
   bool success = connection->disconnect() ;
   return success ;
}

//----------------------------------------------------------------------

static bool connection_went_down(FrServer *server, FrConnection *connection)
{
   if (connection)
      {
      // notify user of connection shutdown
      //!!!

      // abort the connection
      connection->abort() ;
      connection_list = listremove(connection_list,connection,::equal) ;
      if (server)
	 server->removeConnection(connection) ;
      return true ;
      }
   return false ;
}

//----------------------------------------------------------------------

static bool handle_exception(FrServer *server, FrConnection *connection)
{
   if (connection)
      {
      // if there was an exception on the socket, there's really not much
      // more we can do than to kill the connection on our side
      connection_went_down(server,connection) ;
      return true ;
      }
   return false ;
}

//----------------------------------------------------------------------

static bool receive_packets(FrConnection *connection, FrQueue *queue, bool)
{
   if (connection)
      {
      int status = connection->recv(queue) ;
      return status != -1 ;
      }
   return false ; //!!!for now
}

/************************************************************************/
/*    Member functions for class FrServerDB				*/
/************************************************************************/

FrServerDB::FrServerDB(const char *name)
{
   users = 0 ;
   locks = new FrSymHashTable ;
   dbname = FrDupString(name) ;
   if (dbname)
      {
      symtab = initialize_VFrames_disk(name,0) ;
      }
   return ;
}

//----------------------------------------------------------------------

FrServerDB::~FrServerDB()
{
   while (users)
      {
      FrConnection *user = (FrConnection*)poplist(users) ;
      removeUser(user) ;
      }
   FrFree(dbname) ;
   delete locks ;
   if (symtab)
      {
      symtab->select() ;
      shutdown_VFrames() ;
      }
   return ;
}

//----------------------------------------------------------------------

bool FrServerDB::addUser(FrConnection *conn)
{
   if (!users->member(conn))
      {

      pushlist(conn,users) ;
      return true ;
      }
   else
      return false ;
}

//----------------------------------------------------------------------

bool FrServerDB::removeUser(FrConnection *conn)
{
   if (users->member(conn))
      {

      users = listremove(users,conn) ;
      return true ;
      }
   else
      return false ;
}

/************************************************************************/
/*    Member functions for class FrServerData				*/
/************************************************************************/

FrServerData::FrServerData()
{
   client_handle = 0 ;
   for (int i = 0 ; i < (int)lengthof(sent_notifications) ; i++)
      sent_notifications[i] = 0 ;
   for (int j = 0 ; j < (int)lengthof(databases) ; j++)
      databases[j] = 0 ;
   open_databases = 0 ;
   last_notify_time = last_request_time = ::time(0) ;
}

//----------------------------------------------------------------------

FrServerData::~FrServerData()
{
   for (int i = 0 ; i < (int)lengthof(databases) ; i++)
      if (databases[i])
	 closeDatabase(i) ;
}

//----------------------------------------------------------------------

void FrServerData::gotNotification(int seqnum)
{
   if (seqnum >= FSNot_First && seqnum <= FSNot_Last)
      {

      }
}

//----------------------------------------------------------------------

void FrServerData::sentNotification(int seqnum)
{
   if (seqnum >= FSNot_First && seqnum <= FSNot_Last)
      {

      last_notify_time = time(0) ;
      }
}

//----------------------------------------------------------------------

void FrServerData::gotRequest()
{
   last_request_time = time(0) ;
}

//----------------------------------------------------------------------

bool FrServerData::dbhandleAvailable()
{
   return (open_databases < (int)lengthof(databases)) ;
}

//----------------------------------------------------------------------

int FrServerData::openDatabase(FrServerDB *db)
{
   int handle = -1 ;
   if (db)
      {
      for (int i = 0 ; i < (int)lengthof(databases) ; i++)
	 {
	 if (databases[i] == 0 && handle < 0)
	    handle = i ;
	 else if (databases[i] == db)
	    return i ;
	 }
      if (handle != -1)
	 {
	 databases[handle] = db ;
	 open_databases++ ;
	 for (unsigned int j = 0 ; j < sizeof(::open_databases) ; j++)
	    if (::open_databases[j] == 0)
	       ::open_databases[j] = db ;
	 }
      }
   return handle ;
}

//----------------------------------------------------------------------

bool FrServerData::closeDatabase(int handle)
{
FrConnection *connection = 0 ; //!!!
   if (handle >= 0 && handle < (int)lengthof(databases) && databases[handle])
      {
      FrServerDB *db = databases[handle] ;
      db->removeUser(connection) ;
      if (!db->inUse())
	 delete db ;
      open_databases-- ;
      databases[handle] = 0 ;
      return true ;
      }
   else
      return false ;
}

//----------------------------------------------------------------------

FrServerDB *FrServerData::getDatabase(int handle)
{
   if (handle >= 0 && handle < (int)lengthof(databases))
      return databases[handle] ;
   else
      return 0 ;
}

//----------------------------------------------------------------------

FrServerDB *FrServerData::selectDatabase(int handle,
					 FrSymbolTable **symtab)
{
   if (handle >= 0 && handle < (int)lengthof(databases))
      {
      FrServerDB *db = databases[handle] ;
      if (db)
	 {
	 FrSymbolTable *old_symtab = db->select() ;
	 if (symtab)
	    *symtab = old_symtab ;
	 return db ;
	 }
      else
	 {
	 //!!! send reply with status FSE_NOTOPEN
	 }
      return db ;
      }
   else
      {
      //!!! send reply with status FSE_NOSUCHDATABASE
      return 0 ;
      }
}

/************************************************************************/
/*    Member functions for class FrServerClient				*/
/************************************************************************/

FrServerClient::FrServerClient(FrConnection *connect)
{
   connection = connect ;
   username = 0 ;
   userinfo = 0 ;
   pushlist(this,active_clients) ;
   return ;
}

//----------------------------------------------------------------------

FrServerClient::~FrServerClient()
{
   active_clients = listremove(active_clients,this) ;
   FrFree(username) ;
   if (userinfo)
      userinfo->freeObject() ;
   return ;
}

//----------------------------------------------------------------------

void FrServerClient::setName(const char *name)
{
   FrFree(username) ;
   if (userinfo)
      userinfo->freeObject() ;
   username = FrDupString(name) ;
   //!!! get userinfo
   return ;
}

/************************************************************************/
/*    Member functions for class FrServerPrivate			*/
/************************************************************************/

void FrServerPrivate::server_message(const char *msg)
{
   if (msg_handler)
      msg_handler(msg) ;
   return ;
}

//----------------------------------------------------------------------

void __FrCDECL FrServerPrivate::trace(const char *fmt, ...)
{
   if (server_tracing_enabled && msg_handler)
      {
      char msg[1000] ;
      va_list args ;
      va_start(args,fmt) ;
      Fr_vsprintf(msg,sizeof(msg),fmt,args) ;
      va_end(args) ;
      server_message(msg) ;
      }
   return ;
}

//----------------------------------------------------------------------

void FrServerPrivate::abort_bad_connection(FrConnection *connection)
{
   char msg[300] ;
   Fr_sprintf(msg,sizeof(msg),
	      "WARNING: bad packet received on #%d, closing connection",
	      connection->connectionSocket()) ;
   server_message(msg) ;
   close_connection(connection) ;
   return ;
}

//----------------------------------------------------------------------

int FrServerPrivate::identify_self(FrServerPrivate *server, FrPacket *request)
{
   if (request->packetLength() != 0)
      return request->reply(FSE_INVALIDPARAMS) ;
   char identification[FSRIdent_COMMENT+1] ;

   server->trace("Identifying Myself") ;
   memset(identification,'\0',sizeof(identification)) ;
   FrStoreByte(FrPROTOCOL_MAJOR_VERSION,identification+FSRIdent_PROTOMAJORVER) ;
   FrStoreByte(FrPROTOCOL_MINOR_VERSION,identification+FSRIdent_PROTOMINORVER) ;
   FrStoreByte(FrSERVER_MAJOR_VERSION,	identification+FSRIdent_SERVERMAJORVER) ;
   FrStoreByte(FrSERVER_MINOR_VERSION,	identification+FSRIdent_SERVERMINORVER) ;
   FrStoreByte(FrSERVER_PATCHLEVEL,	identification+FSRIdent_SERVERPATCHLEV) ;
   strcpy(identification+FSRIdent_SIGNATURE,
	  server->peer_mode ? SIGNATURE_PEER : SIGNATURE_SERVER) ;
   FrStoreByte(FSReq_Last,		identification+FSRIdent_MAXREQUEST) ;
   FrStoreByte(FSNot_Last,		identification+FSRIdent_MAXNOTIFY) ;
   FrStoreByte(0x7F,			identification+FSRIdent_REQUESTLIMIT) ;
   FrStoreByte(1,			identification+FSRIdent_PSWD_REGISTER) ;
   FrStoreByte(1,			identification+FSRIdent_PSWD_DBOPEN) ;
   FrStoreByte(0,			identification+FSRIdent_PSWD_DBCREATE) ;
   FrStoreByte(0,			identification+FSRIdent_PSWD_REVERT) ;
   FrStoreByte(0,			identification+FSRIdent_PSWD_DELETE) ;
   FrStoreByte(0,			identification+FSRIdent_PSWD_SETCONFIG) ;
   FrStoreShort(0x0400,			identification+FSRIdent_MAXCLIENTS) ;
   FrStoreShort(server->statistics->numClients(),
					identification+FSRIdent_CURRCLIENTS) ;
   identification[FSRIdent_COMMENT] = '\0' ; // no comment string
   return request->reply(FSE_Success,identification,sizeof(identification)) ;
}

//----------------------------------------------------------------------

int FrServerPrivate::register_client(FrServerPrivate *server, FrPacket *request)
{
   if (request->packetLength() < FSRReg_USERNAME+3)
      return request->reply(FSE_INVALIDPARAMS) ;
   char *data = request->packetData() ;
   uint16_t clientID = FrLoadShort(data+FSRReg_CLIENTID) ;
   int regmode = FrLoadByte(data+FSRReg_REGMODE) ;
   bool getpref = FrLoadByte(data+FSRReg_GETPREF) ;
   char *username = data+FSRReg_USERNAME ;
   char *password = strchr(username,'\0') + 1 ;
   if (!*password || password >= request->packetEnd())
      password = 0 ;
   server->trace("Register: %s (mode %d, ID %d, %s)",
			username,regmode,clientID,
			getpref ? "get preferences" : "no preferences") ;
   // advance client handle number until we find a free handle
   while (select_client(current_client_handle))
      current_client_handle++ ;
   //!!!
//   FrServerClient *client = new FrServerClient(server->getConnection()) ;
//   client->setName(username) ;

   char registration[50] ;
   int regsize = sizeof(registration) ;
   server->statistics->addClient() ;
   current_client_handle++ ;		// next time, start with next handle
   return request->reply(FSE_Success,registration,regsize) ;
}

//----------------------------------------------------------------------

int FrServerPrivate::unregister_client(FrServerPrivate *server, FrPacket *request)
{
   if (request->packetLength() != FSRunreg_REQLENGTH)
      return request->reply(FSE_INVALIDPARAMS) ;
   char *data = request->packetData() ;
   uint16_t clhandle = FrLoadShort(data+FSRunreg_CLIENTHANDLE) ;
   server->trace("Unregister: %4.04X",clhandle) ;
   //!!!

   server->statistics->removeClient() ;
   return request->replySuccess() ;
}

//----------------------------------------------------------------------

int FrServerPrivate::get_database_list(FrServerPrivate *server, FrPacket *request)
{
   if (request->packetLength() < 2)
      return request->reply(FSE_INVALIDPARAMS) ;
   char *data = request->packetData() ;
   server->trace("Get Database FrList: %s",data) ;
   FrList *databases = find_databases(0,data) ;
   char *dblist ;
   int dblistlen = list_to_netdata(databases,dblist,true) ;
   int status = request->reply(FSE_Success,dblist,dblistlen) ;
   FrFree(dblist) ;
   return status ;
}

//----------------------------------------------------------------------

int FrServerPrivate::get_index_info(FrServerPrivate *server, FrPacket *request)
{
   if (request->packetLength() < FSRIInfo_DBNAME+2)
      return request->reply(FSE_INVALIDPARAMS) ;
   char *data = request->packetData() ;
   int indexnum = FrLoadShort(data+FSRIInfo_INDEXNUM) ;
   char *dbname = data+FSRIInfo_DBNAME ;
   server->trace("Get Index Info: %s index %d",dbname,indexnum) ;
   char *indexname = database_index_name(dbname,indexnum) ;
   if (!indexname)
      return request->replyResourceLimit() ;
   int exitcode = FSE_Success ;
   int indexlen = strlen(indexname) + 1 ;
   int replysize =  indexlen + 32 ;
   FrLocalAllocC(char,replydata,1024,replysize) ;
   if (!replydata)
      return request->replyResourceLimit() ;
   int nextindex ;
   if (!FrFileExists(indexname))
      {
      exitcode = FSE_UNKNOWNINDEX ;
      nextindex = 0xFFFF ;
      }
   else
      {
      nextindex = indexnum+1 ;
      struct stat st ;
      if (stat(indexname,&st) == -1)	// get file size
	 exitcode = FSE_FILEERROR ;
      uint32_t indexsize = (uint32_t)st.st_size ;
      uint32_t lastmod = (uint32_t)st.st_mtime ;
      uint16_t indextype = (uint16_t)indexnum ;
      uint32_t indexentries = 0xFFFFFFFF ; //!!! not implemented yet
      FrStoreShort(nextindex,  replydata+FSRIInfo_NEXTINDEX) ;
      FrStoreLong(indexsize,   replydata+FSRIInfo_INDEXSIZE) ;
      FrStoreLong(lastmod,     replydata+FSRIInfo_LASTMOD) ;
      FrStoreShort(indextype,  replydata+FSRIInfo_INDEXTYPE) ;
      FrStoreLong(indexentries,replydata+FSRIInfo_NUMENTRIES) ;
      memcpy(replydata+FSRIInfo_INDEXNAME,indexname,indexlen) ;
      }
   FrFree(indexname) ;
   int status = request->reply(exitcode,replydata,replysize) ;
   FrLocalFree(replydata) ;
   return status ;
}

//----------------------------------------------------------------------

int FrServerPrivate::find_clients(FrServerPrivate *server, FrPacket *request)
{
   if (request->packetLength() < FSRfcli_CLIENTNAME+1)
      return request->reply(FSE_INVALIDPARAMS) ;
   char *data = request->packetData() ;
   int maxlisting = FrLoadShort(data+FSRfcli_MAXLISTING) ;
   uint16_t clientID = FrLoadShort(data+FSRfcli_CLIENTID) ;
   uint16_t clienthandle = FrLoadShort(data+FSRfcli_CLIENTHANDLE) ;
   char *clientname = data+FSRfcli_CLIENTNAME ;
   server->trace("Find Clients: ID=%4.04X handle=%4.04X name=%s",
		clientID,clienthandle,clientname) ;
   int replysize ;
   char *replydata ;
   FrServerClient *client ;
   if (clienthandle != 0xFFFF)		// wildcard?
      {
      int matches ;
      client = select_client(clienthandle) ;
      int clientlen = 0 ;
      if (client)
	 {
	 matches = 1 ;
	 clientlen = strlen(client->userName()) + 1 ;
	 replysize = 6 + clientlen) ;
	 }
      else
	 {
	 matches = 0 ;
	 replysize = 2 ;
	 }
      replydata = FrNewN(char,replysize) ;
      if (!replydata)
	 {
	 FrNoMemory("while listing clients") ;
	 return request->replyResourceLimit() ;
	 }
      FrStoreShort(matches,replydata+FSRfcli_NUMLISTED) ;
      if (matches)
	 {
	 FrStoreShort(client->clientID(),     replydata+FSRfcli_CLIENTID) ;
	 FrStoreShort(client->clientHandle(), replydata+FSRfcli_CLIENTHANDLE) ;
	 memcpy(replydata+FSRfcli_USERNAME,client->userName(),clientlen) ;
	 }
      }
   else
      {
      int matches = 0 ;
      replysize = 2 ;
      FrList *cl ;
      for (cl = active_clients ; cl ; cl = cl->rest())
	 {
	 client = (FrServerClient*)cl->first() ;
	 if ((!*clientname || strcmp(clientname,client->userName()) == 0) &&
	     (clientID == 0xFFFF || clientID == client->clientID()))
	    {
	    replysize += 5+strlen(client->userName()) ;
	    if (++matches >= maxlisting)
	       break ;
	    }
	 }
      replydata = FrNewN(char,replysize) ;
      if (!replydata)
	 {
	 FrNoMemory("while listing clients") ;
	 return request->replyResourceLimit() ;
	 }
      char *repdata = replydata ;
      FrStoreShort(matches,replydata+FSRfcli_NUMLISTED) ;
      for (cl = active_clients ; cl ; cl = cl->rest())
	 {
	 FrServerClient *client = (FrServerClient*)cl->first() ;
	 if ((!*clientname || strcmp(clientname,client->userName()) == 0) &&
	     (clientID == 0xFFFF || clientID == client->clientID()))
	    {
	    FrStoreShort(client->clientID(),	repdata+FSRfcli_CLIENTID) ;
	    FrStoreShort(client->clientHandle(),repdata+FSRfcli_CLIENTHANDLE) ;
	    strcpy(repdata+FSRfcli_USERNAME,client->userName()) ;
	    repdata += 5+strlen(client->userName()) ;
	    if (--matches <= 0)
	       break ;
	    }
	 }
      }
   int status = request->reply(FSE_Success,replydata,replysize) ;
   FrFree(replydata) ;
   return status ;
}

//----------------------------------------------------------------------

int FrServerPrivate::get_user_preferences(FrServerPrivate *server,
					  FrPacket *request)
{
   if (request->packetLength() != FSRgpref_REQLENGTH)
      return request->reply(FSE_INVALIDPARAMS) ;
   char *data = request->packetData() ;
   uint16_t clientID = FrLoadShort(data+FSRgpref_CLIENTID) ;
   server->trace("Get Preferences: client %4.04X",clientID) ;

   int replysize = 0 ;
   FrLocalAlloc(char,replydata,1024,replysize) ;
   if (!replydata)
      return request->replyResourceLimit() ;
   //!!!
   int status = request->reply(FSE_Success,replydata,replysize) ;
   FrLocalFree(replydata) ;
   return status ;
}

//----------------------------------------------------------------------

int FrServerPrivate::set_user_preferences(FrServerPrivate *server, FrPacket *request)
{
   if (request->packetLength() < FSRspref_CLIENTDATA)
      return request->reply(FSE_INVALIDPARAMS) ;
   char *data = request->packetData() ;
   uint16_t clientID = FrLoadShort(data+FSRspref_CLIENTID) ;
   bool broadcastmsg = FrLoadByte(data+FSRspref_BCASTMSG) ;
   bool personalmsg = FrLoadByte(data+FSRspref_PERSONMSG) ;
   bool clientmsg = FrLoadByte(data+FSRspref_CLIENTMSG) ;
   bool updatenotify = FrLoadByte(data+FSRspref_UPDATENOTIFY) ;
   char *clientdata = data+FSRspref_CLIENTDATA ;
   server->trace("Set Preferences: client %4.04X %s %s %s %s",clientID,
		broadcastmsg ? "bcast" : "nobcast",
		personalmsg ? "pers" : "nopers",
		clientmsg ? "cmsg" : "nocmsg",
		updatenotify ? "updates" : "noupdates") ;
(void)clientdata ;
   //!!!
   return request->replySuccess() ;
}

//----------------------------------------------------------------------

int FrServerPrivate::open_database(FrServerPrivate *server, FrPacket *request)
{
   if (request->packetLength() < FSRopendb_DBNAME+3)
      return request->reply(FSE_INVALIDPARAMS) ;
   char *data = request->packetData() ;
   int flags = FrLoadByte(data+FSRopendb_FLAGS) ;
   char *dbname = data+FSRopendb_DBNAME ;
   char *password = strchr(dbname,'\0') + 1 ;
   if (!*password)
      password = 0 ;
   server->trace("Open Database: %s (flags %2.02X)",dbname,flags) ;
   FrServerClient *client = select_client(request) ;
   if (!client)
      return request->reply(FSE_USERNOTREGISTERED) ;
   //!!!

   FrSymbolTable *symtab = initialize_VFrames_disk(dbname,0,true,false,
						   password) ;
   if (!symtab)
      {
      int exitcode = FSE_NOSUCHDATABASE ;
      if (Fr_errno == ME_PRIVILEGED)
	 exitcode = FSE_NEEDPASSWORD ;
      else if (Fr_errno == ENOMEM)
	 exitcode = FSE_RESOURCELIMIT ;
      return request->reply(exitcode) ;
      }
//!!!
   int replysize = 3 ;
   FrLocalAlloc(char,replydata,1024,replysize) ;
   if (!replydata)
      return request->replyResourceLimit() ;
   int exitcode = FSE_Success ;
   int dbhandle = 0 ; //!!!
   uint16_t cfgnumber = 0 ; //!!!
   server->trace("   handle = %d",dbhandle) ;
   FrStoreByte(dbhandle,  replydata+FSRopendb_DBHANDLE) ;
   FrStoreShort(cfgnumber,replydata+FSRopendb_CFGNUM) ;
   //!!!
   if (exitcode == FSE_Success)
      server->statistics->addOpen() ;
   int status = request->reply(exitcode,replydata,replysize) ;
   FrLocalFree(replydata) ;
   return status ;
}

//----------------------------------------------------------------------

int FrServerPrivate::create_database(FrServerPrivate *server, FrPacket *request)
{
   if (request->packetLength() < FSRcrdb_DBNAME+2)
      return request->reply(FSE_INVALIDPARAMS) ;
   char *data = request->packetData() ;
   uint32_t estsize = FrLoadLong(data+FSRcrdb_ESTSIZE) ;
   uint32_t estrecords = FrLoadLong(data+FSRcrdb_ESTRECS) ;
   uint16_t cfgnumber = FrLoadShort(data+FSRcrdb_CFGNUM) ;
   char *dbname = data+FSRcrdb_DBNAME ;
   char *password = strchr(dbname,'\0') + 1 ;
   if (!*password)
      password = 0 ;
   server->trace("Create Database: %s",dbname) ;
(void)estsize ; (void)estrecords ; (void)cfgnumber ;
   //!!!
   FrSymbolTable *symtab = initialize_VFrames_disk(dbname,0,true,true,
						   password) ;
   if (!symtab)
      return request->reply((Fr_errno == ME_PRIVILEGED)
			      ? FSE_NEEDPASSWORD
			      : FSE_NOSUCHDATABASE) ;
   DBUserData *user_data = 0 ; //!!!
   set_DB_user_data(user_data) ;
   int dbhandle = 0 ;
   //!!!
   char replydata[1] ;
   FrStoreByte(dbhandle,replydata+FSRcrdb_DBHANDLE) ;
   return request->reply(FSE_Success,replydata,sizeof(replydata)) ;
}

//----------------------------------------------------------------------

int FrServerPrivate::close_database(FrServerPrivate *server, FrPacket *request)
{
   if (request->packetLength() != FSRclose_REQLENGTH)
      return request->reply(FSE_INVALIDPARAMS) ;
   char *data = request->packetData() ;
   BYTE dbhandle = (BYTE)FrLoadByte(data+FSRclose_DBHANDLE) ;
   server->trace("Close Database: %d",dbhandle) ;
   FrSymbolTable *symtab = select_database(request->getConnection(),dbhandle) ;
   if (!symtab)
      return request->reply(FSE_NOSUCHDATABASE) ;
   if (shutdown_VFrames() < 0)
      {
      symtab->select() ;
      return request->replyGeneralError() ;
      }
   //!!!
   symtab->select() ;
   return request->replySuccess() ;
}

//----------------------------------------------------------------------

int FrServerPrivate::create_index(FrServerPrivate *server, FrPacket *request)
{
   if (request->packetLength() != FSRcrindex_REQLENGTH)
      return request->reply(FSE_INVALIDPARAMS) ;
   char *data = request->packetData() ;
   int dbhandle = FrLoadByte(data+FSRcrindex_DBHANDLE) ;
   int indextype = FrLoadShort(data+FSRcrindex_INDEXTYPE) ;
   server->trace("Create Index: db=%d, type=%d",dbhandle,indextype) ;
   FrSymbolTable *symtab = select_database(request->getConnection(),dbhandle) ;
   if (!symtab)
      return request->reply(FSE_NOSUCHDATABASE) ;
//!!!	
   symtab->select() ;
   return request->replySuccess() ;
}

//----------------------------------------------------------------------

int FrServerPrivate::get_database_index(FrServerPrivate *server, FrPacket *request)
{
   if (request->packetLength() < 5)
      return request->reply(FSE_INVALIDPARAMS) ;
   char *data = request->packetData() ;
   int dbhandle = FrLoadByte(data+FSRgetidx_DBHANDLE) ;
   bool get_all = FrLoadByte(data+FSRgetidx_FULLINDEX) ;
   int indexnum = FrLoadShort(data+FSRgetidx_INDEXNUM) ;
   char *indexname = data+FSRgetidx_INDEXNAME ;
   FrSymbolTable *symtab = select_database(request->getConnection(),dbhandle) ;
   if (!symtab)
      {
      server->trace("Get index:	 invalid handle") ;
      return request->reply(FSE_NOSUCHDATABASE) ;
      }
   if (indexnum != 0xFFFF)
      {
      char *dbname = "" ; //!!!
      indexname = database_index_name(dbname,indexnum) ;
      }
   else
      {
      indexnum = 0 ; //!!!
      }
   server->trace("Get Index: %s",indexname) ;
(void)get_all ;
//!!!
   if (indexnum != 0xFFFF)
      FrFree(indexname) ;
   int replysize = 16 + 0 ;
   FrLocalAllocC(char,replydata,1024,replysize) ;
   if (!replydata)
      return request->replyResourceLimit() ;
   FrStoreByte(0,	 replydata+FSRgetidx_INDEXTYPE) ;
   FrStoreShort(indexnum,replydata+FSRgetidx_FOUNDNUM) ;
   //insert index file at replydata+FSRgetidx_INDEX //
   int status = request->reply(FSE_Success,replydata,replysize) ;
   FrLocalFree(replydata) ;
   symtab->select() ;
   return status ;
}

//----------------------------------------------------------------------

int FrServerPrivate::get_frame(FrServerPrivate *server, FrPacket *request)
{
   if (request->packetLength() < 5)
      return request->reply(FSE_INVALIDPARAMS) ;
   char *data = request->packetData() ;
   int dbhandle = FrLoadByte(data+FSRgetframe_DBHANDLE) ;
   bool subclasses = FrLoadByte(data+FSRgetframe_SUBCLASSES) ;
   bool partsof = FrLoadByte(data+FSRgetframe_PARTSOF) ;
   server->trace("Get Frames: %s",data+FSRgetframe_FRAME) ;
   FrSymbolTable *symtab = select_database(request->getConnection(),dbhandle) ;
   if (!symtab)
      return request->reply(FSE_NOSUCHDATABASE) ;
   FrList *frames = netdata_to_list(data+FSRgetframe_FRAME,
				    data+request->packetLength()) ;
   int replysize = 1 ;
   bool missing = false ;
   FrList *fr ;
   for (fr = frames ; fr ; fr = fr->rest())
      {
      FrSymbol *name = (FrSymbol*)fr->first() ;
      if (name->isFrame())
	 {
	 replysize += FrObject_string_length(name->findFrame()) + 1 ;
	 if (subclasses)
	    {
	    const FrList *subcl = name->symbolFrame()->subclassesOf() ;
	    for ( ; subcl ; subcl = subcl->rest())
	       {
	       name = (FrSymbol*)subcl->first() ;
	       if (name && name->symbolp() && name->isFrame())
		  replysize += FrObject_string_length(name->findFrame()) + 1 ;
	       }
	    }
	 if (partsof)
	    {
	    const FrList *parts = name->symbolFrame()->partOf_list() ;
	    for ( ; parts ; parts = parts->rest())
	       {
	       name = (FrSymbol*)parts->first() ;
	       if (name && name->symbolp() && name->isFrame())
		  replysize += FrObject_string_length(name->findFrame()) + 1 ;
	       }
	    }
	 }
      else
	 missing = true ;
      }
   FrLocalAlloc(char,replydata,1024,replysize) ;
   if (!replydata)
      {
      FrWarning("out of memory while getting frame(s)") ;
      return request->replyResourceLimit() ;
      }
   char *result = replydata ;
   for (fr = frames ; fr ; fr = fr->rest())
      {
      FrSymbol *name = (FrSymbol*)fr->first() ;
      if (name->isFrame())
	 {
	 result = name->findFrame()->print(result) + 1 ;
	 // (we skipped the terminating NUL to preserve it)
	 if (subclasses)
	    {
	    const FrList *subcl = name->symbolFrame()->subclassesOf() ;
	    for ( ; subcl ; subcl = subcl->rest())
	       {
	       name = (FrSymbol*)subcl->first() ;
	       if (name && name->symbolp() && name->isFrame())
		  {
		  result = name->findFrame()->print(result) + 1 ;
		  // (we skipped the terminating NUL to preserve it)
		  }
	       }
	    }
	 if (partsof)
	    {
	    const FrList *parts = name->symbolFrame()->partOf_list() ;
	    for ( ; parts ; parts = parts->rest())
	       {
	       name = (FrSymbol*)parts->first() ;
	       if (name && name->symbolp() && name->isFrame())
		  {
		  result = name->findFrame()->print(result) + 1 ;
		  // (we skipped the terminating NUL to preserve it)
		  }
	       }
	    }
	 }
      }
   *result = '\0' ;			// null string to mark end of list
   int status = request->reply(missing ? FSE_NOSUCHFRAME : FSE_Success,
			       replydata,replysize) ;
   FrLocalFree(replydata) ;
   symtab->select() ;
   return status ;
}

//----------------------------------------------------------------------

int FrServerPrivate::get_old_frame(FrServerPrivate *server, FrPacket *request)
{
   if (request->packetLength() < 5)
      return request->reply(FSE_INVALIDPARAMS) ;
   char *data = request->packetData() ;
   int dbhandle = FrLoadByte(data+FSRgetold_DBHANDLE) ;
   int generation = FrLoadShort(data+FSRgetold_GENERATION) ;
   char *framename = data+FSRgetold_FRAME ;
   server->trace("Get Old FrFrame: %s",framename) ;
   FrSymbolTable *symtab = select_database(request->getConnection(),dbhandle) ;
   if (!symtab)
      return request->reply(FSE_NOSUCHDATABASE) ;
   FrSymbol *name = FrSymbolTable::add(framename) ;
   if (!name->isFrame())
      {
      symtab->select() ;
      return request->reply(FSE_NOSUCHFRAME) ;
      }
   FrFrame *frame = name->oldFrame(generation) ;
   if (!frame)
      {
      symtab->select() ;
      return request->reply(FSE_NOSUCHGENERATION) ;
      }
   int replysize = FrObject_string_length(frame) + 1 ;
   FrLocalAlloc(char,replydata,1024,replysize) ;
   if (!replydata)
      {
      FrWarning("out of memory while retrieving old frame") ;
      return request->replyResourceLimit() ;
      }
   frame->print(replydata+FSRgetold_FRAMEREP) ;
   int status = request->reply(FSE_Success,replydata,replysize) ;
   FrLocalFree(replydata) ;
   symtab->select() ;
   return status ;
}

//----------------------------------------------------------------------

int FrServerPrivate::get_fillers(FrServerPrivate *server, FrPacket *request)
{
   if (request->packetLength() < 5)
      return request->reply(FSE_INVALIDPARAMS) ;
   char *data = request->packetData() ;
   int dbhandle = FrLoadByte(data+FSRgetfill_DBHANDLE) ;
   int inherittype = FrLoadByte(data+FSRgetfill_INHERITANCE) ;
   char *framename = data+FSRgetfill_FRAME ;
   char *slotname = strchr(framename,'\0') + 1 ;
   char *facetname = strchr(slotname,'\0') + 1 ;
   server->trace("Get Inherited Fillers: %s %s %s",framename,slotname,
		facetname) ;
   FrSymbolTable *symtab = select_database(request->getConnection(),dbhandle) ;
   if (!symtab)
      return request->reply(FSE_NOSUCHDATABASE) ;
   FrInheritanceType itype = netinherit_to_fpinherit(inherittype) ;
   if ((int)itype == -1)
      return request->reply(FSE_UNKNOWNINHERIT) ;
   FrSymbol *frame = FrSymbolTable::add(framename) ;
   FrSymbol *slot = FrSymbolTable::add(slotname) ;
   FrSymbol *facet = FrSymbolTable::add(facetname) ;
   FrInheritanceType old_itype = get_inheritance_type() ;
   set_inheritance_type(itype) ;
   const FrList *fillers = frame->getFillers(slot,facet,true) ;
   set_inheritance_type(old_itype) ;
   if (!fillers)
      return request->replyGeneralError() ;
   int replysize = 1 ;			// count the terminating empty string
   const FrList *l ;
   for (l = fillers ; l ; l = l->rest())
      replysize += FrObject_string_length(l->first()) + 1 ;
   FrLocalAlloc(char,replydata,1024,replysize) ;
   char *buf = replydata + FSRgetfill_FILLERS ;
   for (l = fillers ; l ; l = l->rest())
      buf = l->first()->print(buf) + 1 ;
   *buf = '\0' ;			// terminate list with null string
   int status = request->reply(FSE_Success,replydata,replysize) ;
   FrLocalFree(replydata) ;
   symtab->select() ;
   return status ;
}

//----------------------------------------------------------------------

int FrServerPrivate::lock_frame(FrServerPrivate *server, FrPacket *request)
{
   if (request->packetLength() < 4)
      return request->reply(FSE_INVALIDPARAMS) ;
   char *data = request->packetData() ;
   int dbhandle = FrLoadByte(data+FSRlock_DBHANDLE) ;
   int wait = FrLoadShort(data+FSRlock_WAIT) ;
   char *framename = data+FSRlock_FRAME ;
   server->trace("Lock FrFrame: %s (wait %d)",framename,wait) ;
   FrSymbolTable *symtab = select_database(request->getConnection(),dbhandle) ;
   if (!symtab)
      return request->reply(FSE_NOSUCHDATABASE) ;
   FrSymbol *frame = FrSymbolTable::add(framename) ;
   if (frame->isLocked())
      {
      symtab->select() ;
      return request->reply(FSE_ALREADYLOCKED) ;
      }
   else if (!frame->lockFrame())
      {
      symtab->select() ;
      return request->replyGeneralError() ;
      }
   else
      {
      //!!! remember who locked the frame
      }
   notify_database_users(request,frame) ;
   symtab->select() ;
   return request->replySuccess() ;
}

//----------------------------------------------------------------------

int FrServerPrivate::unlock_frame(FrServerPrivate *server, FrPacket *request)
{
   if (request->packetLength() < 2)
      return request->reply(FSE_INVALIDPARAMS) ;
   char *data = request->packetData() ;
   int dbhandle = FrLoadByte(data+FSRunlock_DBHANDLE) ;
   char *framename = data+FSRunlock_FRAME ;
   server->trace("Unlock FrFrame: %s",framename) ;
   FrSymbolTable *symtab = select_database(request->getConnection(),dbhandle) ;
   if (!symtab)
      return request->reply(FSE_NOSUCHDATABASE) ;
   FrSymbol *frame = FrSymbolTable::add(framename) ;
   if (!frame->isLocked())
      {
      symtab->select() ;
      return request->reply(FSE_NOTLOCKED) ;
      }
   else if (!frame->unlockFrame())
      {
      symtab->select() ;
      return request->replyGeneralError() ;
      }
   else
      {
      //!!! remove from lock list
      }
   notify_database_users(request,frame) ;
   symtab->select() ;
   return request->replySuccess() ;
}

//----------------------------------------------------------------------

int FrServerPrivate::create_frame(FrServerPrivate *server, FrPacket *request)
{
   if (request->packetLength() < FSRcreate_MINLENGTH)
      return request->reply(FSE_INVALIDPARAMS) ;
   char *data = request->packetData() ;
   int dbhandle = FrLoadByte(data+FSRcreate_DBHANDLE) ;
   bool gsym = FrLoadByte(data+FSRcreate_GENSYM) ;
   char *framename = data+FSRcreate_FRAME ;
   server->trace("Create FrFrame: %s%s",framename,gsym ? " gensym" : "") ;
   FrSymbolTable *symtab = select_database(request->getConnection(),dbhandle) ;
   if (!symtab)
      return request->reply(FSE_NOSUCHDATABASE) ;
   FrSymbol *frame = FrSymbolTable::add(framename) ;
   if (frame->isFrame())
      {
      //!!! gensym a new name, because the specified frame already exists
      }
   frame->createVFrame() ;
   if (store_VFrame(frame) < 0)
      {
      symtab->select() ;
      return request->reply(FSE_DBWRITEERROR) ;
      }
   int replysize = 0 ;
   FrLocalAlloc(char,replydata,1024,replysize) ;
//!!!
   int status = request->reply(FSE_Success,replydata,replysize) ;
   notify_database_users(request,frame) ;
   FrLocalFree(replydata) ;
   symtab->select() ;
   return status ;
}

//----------------------------------------------------------------------

int FrServerPrivate::delete_frame(FrServerPrivate *server, FrPacket *request)
{
   if (request->packetLength() < FSRdelete_MINLENGTH)
      return request->reply(FSE_INVALIDPARAMS) ;
   char *data = request->packetData() ;
   int dbhandle = FrLoadByte(data+FSRdelete_DBHANDLE) ;
   char *framename = data+FSRdelete_FRAME ;
   char *password = strchr(framename,'\0') + 1 ;
   if (!*password || password >= request->packetEnd())
      password = 0 ;
   server->trace("Delete FrFrame: %s",framename) ;
   FrSymbolTable *symtab = select_database(request->getConnection(),dbhandle) ;
   if (!symtab)
      return request->reply(FSE_NOSUCHDATABASE) ;
   FrSymbol *frame = FrSymbolTable::add(framename) ;
   if (frame->isLocked())
      {
      symtab->select() ;
      return request->reply(FSE_FRAMELOCKED) ;
      }
   frame->deleteFrame() ;
   int exitcode ;
   if (store_VFrame(frame) < 0)
      exitcode = FSE_DBWRITEERROR ;
   else
      {
      exitcode = FSE_Success ;
      notify_database_users(request,frame) ;
      }
   symtab->select() ;
   return request->reply(exitcode) ;
}

//----------------------------------------------------------------------

int FrServerPrivate::revert_frame(FrServerPrivate *server, FrPacket *request)
{
   if (request->packetLength() < FSRrevert_MINLENGTH)
      return request->reply(FSE_INVALIDPARAMS) ;
   char *data = request->packetData() ;
   int dbhandle = FrLoadByte(data+FSRrevert_DBHANDLE) ;
   int generations = FrLoadShort(data+FSRrevert_GENERATIONS) ;
   char *framename = data+FSRrevert_FRAME ;
   char *password = strchr(framename,'\0') + 1 ;
   if (!*password || password >= request->packetEnd())
      password = 0 ;
   server->trace("Reverting FrFrame: %s (%d generations)",framename,generations) ;
   FrSymbolTable *symtab = select_database(request->getConnection(),dbhandle) ;
   if (!symtab)
      return request->reply(FSE_NOSUCHDATABASE) ;
//!!!	
   symtab->select() ;
   return request->replySuccess() ;
}

//----------------------------------------------------------------------

int FrServerPrivate::update_frame(FrServerPrivate *server, FrPacket *request)
{
   if (request->packetLength() < FSRupdate_MINLENGTH)
      return request->reply(FSE_INVALIDPARAMS) ;
   char *data = request->packetData() ;
   int dbhandle = FrLoadByte(data+FSRupdate_DBHANDLE) ;
   char *framerep = data+FSRupdate_FRAMEREP ;
   char *tmp = strchr(framerep,'[') + 1 ;
   server->trace("Updating FrFrame: %s",tmp) ;
   FrSymbolTable *symtab = select_database(request->getConnection(),dbhandle) ;
   if (!symtab)
      return request->reply(FSE_NOSUCHDATABASE) ;
   FrSymbol *frame = string_to_Symbol(tmp) ;
   if (frame->isLocked())
      {
      symtab->select() ;
      return request->reply(FSE_FRAMELOCKED) ;
      }
   frame->eraseFrame() ;		// ensure a fresh start
   string_to_FrObject(framerep) ;	// side effect: create the frame
   int exitcode ;
   if (store_VFrame(frame) < 0)
      exitcode = FSE_DBWRITEERROR ;
   else
      {
      exitcode = FSE_Success ;
      notify_database_users(request,frame) ;
      }
   symtab->select() ;
   return request->reply(exitcode) ;
}

//----------------------------------------------------------------------

int FrServerPrivate::begin_transaction(FrServerPrivate *server, FrPacket *request)
{
   if (request->packetLength() != FSRbtrans_PACKETLENGTH)
      return request->reply(FSE_INVALIDPARAMS) ;
   char *data = request->packetData() ;
   int dbhandle = FrLoadByte(data+FSRbtrans_DBHANDLE) ;
   server->trace("Begin Transaction: db=%d",dbhandle) ;
   FrSymbolTable *symtab = select_database(request->getConnection(),dbhandle) ;
   if (!symtab)
      return request->reply(FSE_NOSUCHDATABASE) ;
   int transaction = ((FrSymbol*)0)->startTransaction() ;
   symtab->select() ;
   if (transaction < 0)
      return request->replyResourceLimit() ;
   server->statistics->startTransaction() ;
   char replydata[2] ;
   FrStoreShort(transaction, replydata+FSRbtrans_TRANSACTION) ;
   return request->reply(FSE_Success,replydata,sizeof(replydata)) ;
}

//----------------------------------------------------------------------

int FrServerPrivate::end_transaction(FrServerPrivate *server, FrPacket *request)
{
   if (request->packetLength() != FSRetrans_PACKETLENGTH)
      return request->reply(FSE_INVALIDPARAMS) ;
   char *data = request->packetData() ;
   int dbhandle = FrLoadByte(data+FSRetrans_DBHANDLE) ;
   int transaction = FrLoadShort(data+FSRetrans_TRANSACTION) ;
   server->trace("End Transaction: db=%d, transaction=%d",dbhandle,
		transaction) ;
   FrSymbolTable *symtab = select_database(request->getConnection(),dbhandle) ;
   if (!symtab)
      return request->reply(FSE_NOSUCHDATABASE) ;
   bool success = ((FrSymbol*)0)->endTransaction(transaction) != -1 ;
   server->statistics->endTransaction() ;
   symtab->select() ;
   return request->reply(success ? FSE_Success : FSE_GENERAL) ;
}

//----------------------------------------------------------------------

int FrServerPrivate::abort_transaction(FrServerPrivate *server, FrPacket *request)
{
   if (request->packetLength() != FSRatrans_PACKETLENGTH)
      return request->reply(FSE_INVALIDPARAMS) ;
   char *data = request->packetData() ;
   int dbhandle = FrLoadByte(data+FSRatrans_DBHANDLE) ;
   int transaction = FrLoadShort(data+FSRatrans_TRANSACTION) ;
   server->trace("Abort Transaction: db=%d, transaction=%d",dbhandle,
		transaction) ;
   FrSymbolTable *symtab = select_database(request->getConnection(),dbhandle) ;
   if (!symtab)
      return request->reply(FSE_NOSUCHDATABASE) ;
   bool success = ((FrSymbol*)0)->abortTransaction(transaction) != -1 ;
   server->statistics->abortTransaction() ;
   symtab->select() ;
   return request->reply(success ? FSE_Success : FSE_GENERAL) ;
}

//----------------------------------------------------------------------

int FrServerPrivate::send_personal_message(FrServerPrivate *server, FrPacket *request)
{
   if (request->packetLength() < FSRpmsg_MINLENGTH)
      return request->reply(FSE_INVALIDPARAMS) ;
   char *data = request->packetData() ;
   uint16_t recipient = FrLoadShort(data+FSRpmsg_RECIPIENT) ;
   bool urgent = FrLoadByte(data+FSRpmsg_URGENT) ;
   int length = FrLoadByte(data+FSRpmsg_MSGLEN) ;
   if (request->packetLength() != FSRpmsg_MESSAGE+length)
      return request->reply(FSE_INVALIDPARAMS) ;
   void *msg = data+FSRpmsg_MESSAGE ;
   server->trace("Personal Message: to %4.04X, length %d",recipient,length) ;
(void)msg ;
//!!!	
   if (urgent)
      {
      }
   return request->replySuccess() ;
}

//----------------------------------------------------------------------

int FrServerPrivate::send_broadcast_message(FrServerPrivate *server, FrPacket *request)
{
   if (request->packetLength() < FSRbmsg_MINLENGTH)
      return request->reply(FSE_INVALIDPARAMS) ;
   char *data = request->packetData() ;
   int length = FrLoadByte(data+FSRbmsg_MSGLEN) ;
   void *msg = data+FSRbmsg_MESSAGE ;
   if (request->packetLength() != FSRbmsg_MESSAGE + length)
      return request->reply(FSE_INVALIDPARAMS) ;
   server->trace("Broadcast Message: length %d",length) ;
   //!!!
(void)msg ;

   return request->replySuccess() ;
}

//----------------------------------------------------------------------

int FrServerPrivate::send_client_message(FrServerPrivate *server, FrPacket *request)
{
   if (request->packetLength() < FSRcmsg_MINLENGTH)
      return request->reply(FSE_INVALIDPARAMS) ;
   char *data = request->packetData() ;
   uint16_t recipient = FrLoadShort(data+FSRcmsg_RECIPIENT) ;
   uint16_t msgtype = FrLoadShort(data+FSRcmsg_MSGTYPE) ;
   int length = FrLoadByte(data+FSRcmsg_MSGLEN) ;
   char *msg = data+FSRcmsg_MESSAGE ;
   if (request->packetLength() < FSRcmsg_MESSAGE+length)
      return request->reply(FSE_INVALIDPARAMS) ;
   server->trace("Client Message: to %4.04X, type %4.04X",recipient,msgtype) ;
   //!!!
(void)msg;

   return request->replySuccess() ;
}

//----------------------------------------------------------------------

int FrServerPrivate::test_isa_p(FrServerPrivate *server, FrPacket *request)
{
   if (request->packetLength() < FSRisa_MINLENGTH)
      return request->reply(FSE_INVALIDPARAMS) ;
   char *data = request->packetData() ;
   int dbhandle = FrLoadByte(data+FSRisa_DBHANDLE) ;
   int hierarchy = FrLoadByte(data+FSRisa_HIERARCHY) ;
   char *framename = data+FSRisa_FRAME ;
   char *ancestor = strchr(framename,'\0')+1 ;
   server->trace("Test Ancestry: %s %s %s",framename,
		hierarchy ? "PART-OF" : "IS-A", ancestor) ;
   FrSymbolTable *symtab = select_database(request->getConnection(),dbhandle) ;
   if (!symtab)
      return request->reply(FSE_NOSUCHDATABASE) ;
   int exitcode = FSE_Success ;
   FrSymbol *fr = FrSymbolTable::add(framename) ;
   FrSymbol *ancest = FrSymbolTable::add(ancestor) ;
   if (hierarchy)
      {
      if (!fr->partOf_p(ancest))
	 exitcode = FSE_NOTANCESTOR ;
      }
   else
      {
      if (!fr->isA_p(ancest))
	 exitcode = FSE_NOTANCESTOR ;
      }
   symtab->select() ;
   return request->reply(exitcode) ;
}

//----------------------------------------------------------------------

int FrServerPrivate::check_restrictions(FrServerPrivate *server, FrPacket *request)
{
   if (request->packetLength() < FSRrestrict_MINLENGTH)
      return request->reply(FSE_INVALIDPARAMS) ;
   char *data = request->packetData() ;
   int dbhandle = FrLoadByte(data+FSRrestrict_DBHANDLE) ;
   char *framename = data+FSRrestrict_FRAME ;
   char *framerep = strchr(framename,'\0') + 1 ;
   server->trace("Check Restrictions: %s",framename) ;
   FrSymbolTable *symtab = select_database(request->getConnection(),dbhandle) ;
   if (!symtab)
      return request->reply(FSE_NOSUCHDATABASE) ;
(void)framerep ;
//!!!
   int replysize = 1 ;
   FrLocalAlloc(char,replydata,1024,replysize) ;
   if (!replydata)
      {
      symtab->select() ;
      return request->replyResourceLimit() ;
      }
   char *violations = replydata + FSRrestrict_VIOLATIONS ;
   *violations = '\0' ;
   int status = request->reply(FSE_Success,replydata,replysize) ;
   FrLocalFree(replydata) ;
   symtab->select() ;
   return status ;
}

//----------------------------------------------------------------------

int FrServerPrivate::indexed_retrieval(FrServerPrivate *server, FrPacket *request)
{
   if (request->packetLength() < FSRiget_MINLENGTH)
      return request->reply(FSE_INVALIDPARAMS) ;
   char *data = request->packetData() ;
   int dbhandle = FrLoadByte(data+FSRiget_DBHANDLE) ;
   int format = FrLoadShort(data+FSRiget_FORMAT) ;
   if (format != 0x01)
      {
      server->trace("Indexed Retrieval: unknown query format") ;
      return request->reply(FSE_INVALIDPARAMS) ;
      }
   if (request->packetLength() < 6)
      return request->reply(FSE_INVALIDPARAMS) ;
   int flags = FrLoadByte(data+FSRiget_FLAGS) ;
   char *query = data+FSRiget_QUERY ;
   char *rootframe = strchr(query,'\0')+1 ;
   server->trace("Indexed Retrieval: %s",query) ;
   FrSymbolTable *symtab = select_database(request->getConnection(),dbhandle) ;
   if (!symtab)
      return request->reply(FSE_NOSUCHDATABASE) ;
   FrSymbol *root = FrSymbolTable::add(rootframe) ;
(void)flags ; (void)root ;
   int replysize = 1 ; //FIXME
   FrLocalAlloc(char,replydata,1024,replysize) ;
   if (!replydata)
      {
      symtab->select() ;
      return request->replyResourceLimit() ;
      }
//!!!

   int status = request->reply(FSE_Success,replydata,replysize) ;
   FrLocalFree(replydata) ;
   symtab->select() ;
   return status ;
}

//----------------------------------------------------------------------

int FrServerPrivate::server_statistics(FrServerPrivate *server, FrPacket *request)
{
   if (request->packetLength() != FSRstats_PACKETLENGTH)
      return request->reply(FSE_INVALIDPARAMS) ;
   char *data = request->packetData() ;
   int stattype = FrLoadByte(data+FSRstats_TYPE) ;
   if (stattype > 2)
      {
      server->trace("Get Server Statistics:  invalid type") ;
      return request->reply(FSE_INVALIDPARAMS) ;
      }
   server->trace("Get Server Statistics: type %d",stattype) ;
   int replysize ;
   char *replydata ;
   int exitcode = FSE_Success ;
   switch (stattype)
      {
      case 0x00:			// general statistics
	 replysize = FSRstats_REPSIZE_GENERAL ;
	 replydata = FrNewN(char,replysize) ;
	 if (!replydata)
	    {
	    exitcode = FSE_RESOURCELIMIT ;
	    replysize = 0 ;
	    }
	 else
	    {
	    FrServerStatistics *stats = server->statistics ;
	    FrStoreShort(stats->maxClients(),	     replydata+0) ;
	    FrStoreLong(stats->numRegistrations(),   replydata+2) ;
	    FrStoreLong(stats->numCrashes(),	     replydata+6) ;
	    FrStoreLong(stats->numOpens(),	     replydata+10) ;
	    FrStoreLong(stats->numTransSuccessful(), replydata+14) ;
	    FrStoreLong(stats->numTransAborted(),    replydata+18) ;
	    FrStoreLong(stats->numTransPending(),    replydata+22) ;
	    FrStoreLong(stats->numPendingRequests(), replydata+26) ;
	    FrStoreLong(stats->numNotifications(),   replydata+30) ;
	    }
	 break ;
      case 0x01:			// request counts
	 replysize = FSRstats_REPSIZE_COUNTS ;
	 replydata = FrNewN(char,replysize) ;
	 if (!replydata)
	    {
	    exitcode = FSE_RESOURCELIMIT ;
	    replysize = 0 ;
	    }
	 else
	    {

	    }
	 break ;
      case 0x02:			// notification counts
	 replysize = FSRstats_REPSIZE_NOTIFY ;
	 replydata = FrNewN(char,replysize) ;
	 if (!replydata)
	    {
	    exitcode = FSE_RESOURCELIMIT ;
	    replysize = 0 ;
	    }
	 else
	    {

	    }
	 break ;
      default:
	 server->trace("CAN'T HAPPEN: unhandled statistics type") ;
	 replysize = 0 ;
	 replydata = 0 ;
	 exitcode = FSE_INVALIDPARAMS ;
	 break ;
      }
   int status = request->reply(exitcode,replydata,replysize) ;
   FrFree(replydata) ;
   return status ;
}

//----------------------------------------------------------------------

int FrServerPrivate::how_are_you(FrServerPrivate *server, FrPacket *request)
{
   if (request->packetLength() != FSRhay_PACKETLENGTH)
      return request->reply(FSE_INVALIDPARAMS) ;
   char *data = request->packetData() ;
   server->trace("How Are You?:	 I am fine.") ;
   char replydata[2] ;
   int pending_requests = -1 ; //!!!
(void)data ;
   FrStoreShort(pending_requests, replydata+FSRhay_PENDINGREQUESTS) ;
   return request->reply(FSE_Success,replydata,sizeof(replydata)) ;
}

//----------------------------------------------------------------------

int FrServerPrivate::get_system_config(FrServerPrivate *server, FrPacket *request)
{
   if (request->packetLength() != FSRgetsys_PACKETLENGTH)
      return request->reply(FSE_INVALIDPARAMS) ;
   char *data = request->packetData() ;
   int configID = FrLoadShort(data+FSRgetsys_CONFIGNUM) ;
   int checkonly = FrLoadByte(data+FSRgetsys_CHECKONLY) ;
   server->trace("Get System Config: %d%s",configID,checkonly ? " check" : "") ;
   int config_size = 0 ; //!!!
   char *config = 0 ; //!!!

   int replysize = 2 ;
   if (!checkonly)
      replysize += config_size ;
   FrLocalAlloc(char,replydata,1024,replysize) ;
   if (!replydata)
      return request->replyResourceLimit() ;
   FrStoreShort(config_size, replydata+0) ;
   if (!checkonly)
      memcpy(replydata+2,config,config_size) ;
   int status = request->reply(FSE_Success,replydata,replysize) ;
   FrLocalFree(replydata) ;
   return status ;
}

//----------------------------------------------------------------------

int FrServerPrivate::set_system_config(FrServerPrivate *server, FrPacket *request)
{
   if (request->packetLength() < FSRsetsys_MINLENGTH)
      return request->reply(FSE_INVALIDPARAMS) ;
   char *data = request->packetData() ;
   int configID = FrLoadShort(data+FSRsetsys_CONFIGNUM) ;
   int config_size = FrLoadShort(data+FSRsetsys_CONFIGSIZE) ;
   char *config_data = data + FSRsetsys_CONFIGDATA ;
   char *password = config_data + config_size ;
   if (!*password || password >= request->packetEnd())
      password = 0 ;
   if (request->packetLength() < FSRsetsys_MINLENGTH+config_size)
      return request->reply(FSE_INVALIDPARAMS) ;
   server->trace("Set System Config: %d, %d bytes",configID,config_size) ;
//!!!

   return request->replySuccess() ;
}

//----------------------------------------------------------------------

int FrServerPrivate::get_user_data(FrServerPrivate *server, FrPacket *request)
{
   if (request->packetLength() != FSRgetuser_PACKETLENGTH)
      return request->reply(FSE_INVALIDPARAMS) ;
   char *data = request->packetData() ;
   int dbhandle = FrLoadByte(data+FSRgetuser_DBHANDLE) ;
   server->trace("Get User Data: db=%d",dbhandle) ;
   FrSymbolTable *symtab = select_database(request->getConnection(),dbhandle) ;
   if (!symtab)
      return request->reply(FSE_NOSUCHDATABASE) ;
   int replysize = 0 ;///!!!
   FrLocalAlloc(char,replydata,1024,replysize) ;
   if (!replydata)
      {
      symtab->select() ;
      return request->replyResourceLimit() ;
      }
   //!!!
   int status = request->reply(FSE_Success,replydata,replysize) ;
   FrLocalFree(replydata) ;
   symtab->select() ;
   return status ;
}

//----------------------------------------------------------------------

int FrServerPrivate::set_user_data(FrServerPrivate *server, FrPacket *request)
{
   if (request->packetLength() != 1+sizeof(DBUserData))
      return request->reply(FSE_INVALIDPARAMS) ;
   char *data = request->packetData() ;
   int dbhandle = FrLoadByte(data+0) ;
   char *udata = data+1 ;
   server->trace("Set User Data: db=%d",dbhandle) ;
   FrSymbolTable *symtab = select_database(request->getConnection(),dbhandle) ;
   if (!symtab)
      return request->reply(FSE_NOSUCHDATABASE) ;
(void)udata ;
//!!!	
   symtab->select() ;
   return request->replySuccess() ;
}

//----------------------------------------------------------------------

int FrServerPrivate::proxy_update(FrServerPrivate *server, FrPacket *request)
{
   if (request->packetLength() < 6)
      return request->reply(FSE_INVALIDPARAMS) ;
   char *data = request->packetData() ;
   int dbhandle = FrLoadByte(data+0) ;
   int func = FrLoadByte(data+1) ;
   char *framename = data+2 ;
   char *slotname = strchr(framename,'\0') + 1 ;
   char *facetname = strchr(slotname,'\0') + 1 ;
   char *filler = strchr(facetname,'\0') + 1 ;
   server->trace("Proxy Update: db=%d, %s %s %s",dbhandle,framename,
		slotname,facetname) ;
   FrSymbolTable *symtab = select_database(request->getConnection(),dbhandle) ;
   if (!symtab)
      return request->reply(FSE_NOSUCHDATABASE) ;
(void)func ; (void)filler ;
//!!!
   symtab->select() ;
   return request->replySuccess() ;
}

//----------------------------------------------------------------------

int FrServerPrivate::inheritable_facets(FrServerPrivate *server, FrPacket *request)
{
   if (request->packetLength() < 3)
      return request->reply(FSE_INVALIDPARAMS) ;
   char *data = request->packetData() ;
   int dbhandle = FrLoadByte(data+0) ;
   FrInheritanceType inhtype = netinherit_to_fpinherit(FrLoadByte(data+1)) ;
   if ((int)inhtype == -1)
      return request->reply(FSE_UNKNOWNINHERIT) ;
   char *framename = data+2 ;
   server->trace("Get Inheritable Facets: %s",framename) ;
   FrSymbolTable *symtab = select_database(request->getConnection(),dbhandle) ;
   if (!symtab)
      return request->reply(FSE_NOSUCHDATABASE) ;

   int replysize = 0 ; //!!!
   FrLocalAlloc(char,replydata,1024,replysize) ;
   if (!replydata)
      {
      symtab->select() ;
      return request->replyResourceLimit() ;
      }
   //!!!
   int status = request->reply(FSE_Success,replydata,replysize) ;
   FrLocalFree(replydata) ;
   symtab->select() ;
   return status ;
}

//----------------------------------------------------------------------

int FrServerPrivate::inherit_all_fillers(FrServerPrivate *server, FrPacket *request)
{
   if (request->packetLength() < 3)
      return request->reply(FSE_INVALIDPARAMS) ;
   char *data = request->packetData() ;
   int dbhandle = FrLoadByte(data+0) ;
   FrInheritanceType inhtype = netinherit_to_fpinherit(FrLoadByte(data+1)) ;
   if ((int)inhtype == -1)
      request->reply(FSE_UNKNOWNINHERIT) ;
   char *framename = data+2 ;
   server->trace("Inherit All Fillers: %s",framename) ;
   FrSymbolTable *symtab = select_database(request->getConnection(),dbhandle) ;
   if (!symtab)
      return request->reply(FSE_NOSUCHDATABASE) ;

   int replysize = 0 ; //!!!
   FrLocalAlloc(char,replydata,1024,replysize) ;
   if (!replydata)
      {
      symtab->select() ;
      return request->replyResourceLimit() ;
      }
   //!!!
   int status = request->reply(FSE_Success,replydata,replysize) ;
   FrLocalFree(replydata) ;
   symtab->select() ;
   return status ;
}

//----------------------------------------------------------------------

int FrServerPrivate::are_you_there(FrServerPrivate *server, FrPacket *request)
{
   if (request->packetLength() != 0)
      return request->reply(FSE_INVALIDPARAMS) ;
   char *data = request->packetData() ;
(void)server ; (void)data ;

   return 0 ;
}

//----------------------------------------------------------------------

int FrServerPrivate::terminating_connection(FrServerPrivate *server, FrPacket *request)
{
   char *data = request->packetData() ;
(void)server ; (void)data ;

   return 0 ;
}

//----------------------------------------------------------------------

int FrServerPrivate::server_going_down(FrServerPrivate *server, FrPacket *request)
{
   char *data = request->packetData() ;
(void)server ; (void)data ;

   return 0 ;
}

//----------------------------------------------------------------------

int FrServerPrivate::broadcast_message(FrServerPrivate *server, FrPacket *request)
{
   char *data = request->packetData() ;
(void)server ; (void)data ;

   return 0 ;
}

//----------------------------------------------------------------------

int FrServerPrivate::personal_message(FrServerPrivate *server, FrPacket *request)
{
   char *data = request->packetData() ;
(void)server ; (void)data ;

   return 0 ;
}

//----------------------------------------------------------------------

int FrServerPrivate::client_message(FrServerPrivate *server, FrPacket *request)
{
   char *data = request->packetData() ;
(void)server ; (void)data ;

   return 0 ;
}

//----------------------------------------------------------------------

int FrServerPrivate::client_message_timeout(FrServerPrivate *server, FrPacket *request)
{
   char *data = request->packetData() ;
(void)server ; (void)data ;

   return 0 ;
}

//----------------------------------------------------------------------

int FrServerPrivate::frame_locked(FrServerPrivate *server, FrPacket *request)
{
   char *data = request->packetData() ;
(void)server ; (void)data ;

   return 0 ;
}

//----------------------------------------------------------------------

int FrServerPrivate::frame_unlocked(FrServerPrivate *server, FrPacket *request)
{
   char *data = request->packetData() ;
(void)server ; (void)data ;

   return 0 ;
}

//----------------------------------------------------------------------

int FrServerPrivate::frame_created(FrServerPrivate *server, FrPacket *request)
{
   char *data = request->packetData() ;
(void)server ; (void)data ;

   return 0 ;
}

//----------------------------------------------------------------------

int FrServerPrivate::frame_deleted(FrServerPrivate *server, FrPacket *request)
{
   char *data = request->packetData() ;
(void)server ; (void)data ;

   return 0 ;
}

//----------------------------------------------------------------------

int FrServerPrivate::frame_updated(FrServerPrivate *server, FrPacket *request)
{
   char *data = request->packetData() ;
(void)server ; (void)data ;

   return 0 ;
}

//----------------------------------------------------------------------

int FrServerPrivate::check_activity(FrServerPrivate *server, FrPacket *request)
{
   char *data = request->packetData() ;
(void)server ; (void)data ;

   return 0 ;
}

//----------------------------------------------------------------------

int FrServerPrivate::discard_frame(FrServerPrivate *server, FrPacket *request)
{
   char *data = request->packetData() ;
(void)server ; (void)data ;

   return 0 ;
}

//----------------------------------------------------------------------

int FrServerPrivate::proxy_update_notification(FrServerPrivate *server, FrPacket *request)
{
   char *data = request->packetData() ;
(void)server ; (void)data ;

   return 0 ;
}

//----------------------------------------------------------------------

int FrServerPrivate::peer_handoff(FrServerPrivate *server, FrPacket *request)
{
   char *data = request->packetData() ;
(void)server ; (void)data ;

   return 0 ;
}

//----------------------------------------------------------------------

int FrServerPrivate::peer_handed_off(FrServerPrivate *server, FrPacket *request)
{
   char *data = request->packetData() ;
(void)server ; (void)data ;

   return 0 ;
}

//----------------------------------------------------------------------

int FrServerPrivate::process_request(FrPacket *request)
{
   int request_code = request->requestCode() ;
   if (request_code >= 0 && request_code <= FSReq_Last)
      statistics->addRequest(request_code) ;
   else if (request_code >= FSNot_First && request_code <= FSNot_Last)
      statistics->repliedNotify() ;
   current_connection = request->getConnection() ;
   if (!procs_initialized)
      {
      for (int i = 0 ; i < (int)lengthof(server_procs) ; i++)
	 server_procs[i] = 0 ;
      for (int j = 0 ; j < (int)lengthof(server_proc_ptrs) ; j++)
	 server_procs[server_proc_ptrs[j].reqnum] = server_proc_ptrs[j].func ;
      procs_initialized = true ;
      }
   FrServerProc *func = server_procs[request_code] ;
   if (func)
      func(this,request) ;
   if (request_code >= 0 && request_code <= FSReq_Last)
      statistics->handledRequest() ;
   current_connection = 0 ;
   return 0 ; //!!! for now
}

/************************************************************************/
/*    Member functions for class FrServer				*/
/************************************************************************/

FrServer::FrServer()
{
   num_connections = 0 ;
   socket = start_listening(FrSERVER_PORT,FrSERVER_PORT,&server_port) ;
   if (socket != -1)
      {
      FD_SET(socket,&connect_fdset) ;
      statistics = new FrServerStatistics ;
      msg_handler = 0 ;
      if (!server_events)
	 server_events = new FrEventList ;
      }
   else
      {
      statistics = 0 ;
      msg_handler = 0 ;
      server_events = 0 ;
      }
   connections = 0 ;
   //!!!
}

//----------------------------------------------------------------------

FrServer::FrServer(bool peermode)
{
   if (peermode)
      socket = start_listening(FrPEER_PORT,FrPEER_PORT+500,&server_port) ;
   else
      socket = start_listening(FrSERVER_PORT,FrSERVER_PORT,&server_port) ;
   peer_mode = peermode ;
   if (socket != -1)
      {
      FD_SET(socket,&connect_fdset) ;
      statistics = new FrServerStatistics ;
      msg_handler = 0 ;
      if (!server_events)
	 server_events = new FrEventList ;
      }
   else
      {
      statistics = 0 ;
      msg_handler = 0 ;
      server_events = 0 ;
      }
   connections = 0 ;
   //!!!
   return ;
}

//----------------------------------------------------------------------

FrServer::~FrServer()
{
   if (socket != -1)
      {
      shutdown(0) ;			// terminate NOW
#ifdef _MSC_VER
      FD_CLR((SOCKET)socket,&connect_fdset) ;
#else
      FD_CLR(socket,&connect_fdset) ;
#endif
      }
   //!!!
   return ;
}

//----------------------------------------------------------------------

int FrServer::startup()
{

   return 0 ; // for now!!!
}

//----------------------------------------------------------------------

int FrServer::shutdown(int time)
{
   if (time < 0)
      time = 0 ;
   // send a termination notification to all the open connections
   for (FrList *clist = connections ; clist ; clist = clist->rest())
      {
      FrConnection *conn = (FrConnection*)clist->first() ;

      (void)conn ; //!!!
      }
   if (time == 0)
      {
      // close all of the connections
      while (connections)
	 {
	 FrConnection *conn = (FrConnection*)poplist(connections) ;
	 conn->disconnect() ;
	 delete conn ;
	 }
      }
   return 0 ; // for now!!!
}

//----------------------------------------------------------------------

static bool connection_alive(FrConnection *connection)
{
   if (connection && connection->connected())
      return true ;
   else
      return false ;
}

//----------------------------------------------------------------------

static void process_received_packets(FrServer *server, FrQueue *queue)
{
   if (queue)
      {
      int len = queue->length() ;
      while (len-- > 0)
	 {
	 FrPacket *packet = (FrPacket*)queue->pop() ;
	 if (packet && packet->packetComplete())
	    {
	    int status = ((FrServerPrivate*)server)->process_request(packet) ;
//!!!
	    (void)status ; //!!!
	    }
	 else
	    queue->add(packet) ;
	 }
      }
}

//----------------------------------------------------------------------

int FrServer::process(int timeout)
{
   static FrQueue queue ;

   if (timeout < 0)
      timeout = 0 ;
   struct timeval timelimit ;
   timelimit.tv_sec = timeout / ONE_MILLION ;
   timelimit.tv_usec = timeout % ONE_MILLION ;
   fd_set read_fdset ;
   fd_set excep_fdset ;

   read_fdset = connect_fdset ;
   excep_fdset = connect_fdset ;
   int numselected = select(FD_SETSIZE,&read_fdset,0,&excep_fdset,&timelimit) ;
   if (numselected > 0 && FD_ISSET(socket,&read_fdset))
      {
      FrConnection *conn = accept_connection(socket,server_port) ;
#ifdef _MSC_VER
      FD_CLR((SOCKET)socket,&read_fdset) ;
#else
      FD_CLR(socket,&read_fdset) ;
#endif /* _MSC_VER */
      numselected-- ;
      if (conn)
	 addConnection(conn) ;
      }
   if (numselected > 0)
      {
      for (FrList *l = connection_list ; l ; l = l->rest())
	 {
	 FrConnection *cinfo = (FrConnection*)l->first() ;
	 if (!cinfo)
	    continue ;
	 int socket = cinfo->connectionSocket() ;
	 if (FD_ISSET(socket,&excep_fdset))
	    handle_exception(this,cinfo) ;
	 if (FD_ISSET(socket,&read_fdset))
	    receive_packets(cinfo,&queue,true) ;
	 else if (!connection_alive(cinfo))
	    connection_went_down(this,cinfo) ;
	 }
      }
   if (queue.length())
      {
      process_received_packets(this,&queue) ;
      if (server_events)
	 server_events->executeEvents() ; // handle anything prev. scheduled
      return 1 ;			// we processed something
      }
   else
      {
      if (server_events)
	 server_events->executeEvents() ; // handle anything prev. scheduled
      return 0 ;			// nothing processed this time
      }
}

//----------------------------------------------------------------------

void FrServer::addConnection(FrConnection *conn)
{
   num_connections++ ;
   pushlist(conn,connections) ;
}

//----------------------------------------------------------------------

void FrServer::removeConnection(FrConnection *conn)
{
   if (connections->member(conn))
      {
      num_connections-- ;
      connections = listremove(connections,conn) ;
      }
}

//----------------------------------------------------------------------

#endif /* FrSERVER */

/************************************************************************/
/************************************************************************/

void shutdown_peer_mode()
{
#ifdef FrSERVER
   if (peer_mode_server)
      {
      delete peer_mode_server ;
      peer_mode_server = 0 ;
      }
#endif /* FrSERVER */
}

//----------------------------------------------------------------------

void initialize_peer_mode()
{
#ifdef FrSERVER
   if (!server_events)
      server_events = new FrEventList ;
   if (!peer_mode_server)
      {
      peer_mode_server = new FrServer(true) ;
      //!!!
      }
   FramepaC_shutdown_peermode_func = shutdown_peer_mode ;
#endif /* FrSERVER */
}

// end of file frserver.cpp //

