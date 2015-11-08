/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frclisrv.h	       public client-server definitions		*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1995,1997,1998,2002,2006,2009				*/
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

#ifndef __FRCLISRV_H_INCLUDED
#define __FRCLISRV_H_INCLUDED

#ifndef __FROBJECT_H_INCLUDED
#include "frobject.h"
#endif

#ifndef __VFRAME_H_INCLUDED
#include "vframe.h"
#endif

/**********************************************************************/
/*    Global constants						      */
/**********************************************************************/

/**********************************************************************/
/*	 Global Variables					      */
/**********************************************************************/

/**********************************************************************/
/*    Forward class and struct declarations			      */
/**********************************************************************/

class FrClient ;
class FrServer ;
class FrServerStatistics ;

class FrPacket ;
class FrConnection ;
class FrServerInfo ;
class FrRemoteDB ;

class INDEXINFO ;

/**********************************************************************/
/*	 useful macros						      */
/**********************************************************************/

/**********************************************************************/
/*       enumerated types     					      */
/**********************************************************************/

enum FrServerError
   {
     FSE_Success = 0x0100,
     FSE_INVALIDREQUEST = 0x0101,
     FSE_INVALIDPARAMS = 0x0102,
     FSE_NOTREGISTERED = 0x0103,
     FSE_ISREGISTERED = 0x0104,
     FSE_TOOMANYCLIENTS = 0x0105,
     FSE_UNREGISTERERROR = 0x0106,
     FSE_USERNOTREGISTERED = 0x0107,
     FSE_NEEDPASSWORD = 0x0108,
     FSE_PREFNOTAVAILABLE = 0x0109,
     FSE_CANTSETPREF = 0x010A,
     FSE_NOPRIVILEGE = 0x010B,
     FSE_NOSUCHDATABASE = 0x010C,
     FSE_READONLYDATABASE = 0x010D,
     FSE_NOSUCHFRAME = 0x010E,
     FSE_NOSUCHSLOT = 0x010F,
     FSE_NOMOREMATCHES = 0x0110,
     FSE_FRAMEEXISTS = 0x0111,
     FSE_FRAMELOCKED = 0x0112,
     FSE_ALREADYLOCKED = 0x0113,
     FSE_NOTLOCKED = 0x0114,
     FSE_ALREADYOPEN = 0x0115,
     FSE_NOTOPEN = 0x0116,
     FSE_NOPERSONALMSGS = 0x0117,
     FSE_NOCLIENTMSGS = 0x0118,
     FSE_NOUPDATENOTIFY = 0x0119,
     FSE_NOTANCESTOR = 0x011A,
     FSE_FRAMEIDLE = 0x011B,
     FSE_FRAMEACTIVE = 0x011C,
     FSE_TRANSACTPENDING = 0x011D,
     FSE_UNKNOWNINHERIT = 0x011E,
     FSE_UNKNOWNSTATISTICS = 0x011F,
     FSE_NOTIFYIGNORED = 0x0120,
     FSE_NOSUCHGENERATION = 0x0121,
     FSE_FRAMEEXISTED = 0x0122,
     FSE_UNKNOWNINDEX = 0x0123,
     FSE_DBWRITEERROR = 0x0124,
     FSE_TOOMANYREQUESTS = 0x0125,
     FSE_FILEERROR = 0x0126,
     FSE_RESOURCELIMIT = 0x0127,
     FSR_WONTACCEPT = 0x0128,
     FSE_GENERAL = 0x01FF,
     FSE_NOSUCHCONNECTION = 0x0201,
     FSE_ALREADYCONNECTED = 0x0202,
     FSE_TIMEOUT = 0x0203,
     FSE_NOSUCHSERVER = 0x0204,
     FSE_SOCKETERROR = 0x0205,
     FSE_NOTSERVER = 0x0206,
   } ;

// bits from the error code to actually transmit over network connection
#define FSE_netmask 0x00FF

enum FrServerNotification
   {
   FSNot_AREYOUTHERE = 0x80,
   FSNot_TERMINATINGCONN = 0x81,
   FSNot_SERVERGOINGDOWN = 0x82,
   FSNot_BROADCASTMSG = 0x83,
   FSNot_PERSONALMSG = 0x84,
   FSNot_CLIENTMSG = 0x85,
   FSNot_CLIENTMSGTIMEOUT = 0x86,
   FSNot_FRAMELOCKED = 0x87,
   FSNot_FRAMEUNLOCKED = 0x88,
   FSNot_FRAMECREATED = 0x89,
   FSNot_FRAMEDELETED = 0x8A,
   FSNot_FRAMEUPDATED = 0x8B,
   FSNot_CHECKACTIVITY = 0x8C,
   FSNot_DISCARDFRAME = 0x8D,
   FSNot_PROXYUPDATE = 0x8E,
   FSNot_PEERHANDOFF = 0x8F,
   FSNot_NEWCONTROLLER = 0x90,

   FSNot_First = FSNot_AREYOUTHERE,
   FSNot_Last = FSNot_NEWCONTROLLER,
   } ;

enum FrPacketType { PT_None, PT_Request, PT_Reply,
		    PT_Notification, PT_Response } ;

/**********************************************************************/
/*	 Portability Functions					      */
/**********************************************************************/

typedef unsigned char BYTE ;

/**********************************************************************/
/**********************************************************************/

typedef void FrIdleFunc() ;
typedef void FrAsyncCallback(FrPacket *request, FrPacket *reply,
			     void *result, void *client_data) ;
typedef void FrNotifyHandler(FrPacket *packet) ;
typedef void FrServerMsgHandler(const char *message) ;

/**********************************************************************/
/**********************************************************************/

class FrPacket : public FrObject
   {
   private:
      FrConnection *connection ;
      char *fragments ;
      char *data ;
      FrAsyncCallback *callback_func ;
      void *callback_data ;
      int   length ;
      int   nfragments ;
      int   missing ;
      FrPacketType type ;
      BYTE  reqcode ;
      BYTE  seqnumber ;
      BYTE  status ;
      BYTE  complete ;
      BYTE  neterror ;
   public: // methods
      void *operator new(size_t size) { return FrMalloc(size) ; }
      void operator delete(void *blk) { FrFree(blk) ; }
      FrPacket() {}
      FrPacket(FrConnection *connect, const char *header, bool am_server) ;
      virtual ~FrPacket() ;
      virtual const char *objTypeName() const ;
      virtual ostream &printValue(ostream &output) const ;
      virtual char *displayValue(char *buffer) const ;
      virtual size_t displayLength() const ;
      FrPacket *copyPacket() const ;

      // packet transfer
      int send() ;
      int reply(int status) ;
      int reply(int status,const char *data,int datalength) ;
      int replySuccess() ;
      int replyGeneralError() ;
      int replyResourceLimit() ;
      int respond(int status) ;
      int respond(int status, const char *data,int datalength) ;

      // packet management
      bool allocateBuffer(int datalength) ;
      void setData(int datalength, char *packetdata)
	    { length = datalength ; data = packetdata ; }
      bool setFragment(int fragnum, const char *frag) ;
      void setType(FrPacketType ptype) { type = ptype ; }
      void setStatus(int stat) { status = (BYTE)stat ; }
      void setRequestCode(BYTE request) { reqcode = request ; }
      void setComplete(int comp) { complete = (BYTE)comp ; }

      // callbacks
      void setRecvCallback(FrAsyncCallback *cb_func, void *cb_data) ;

      // access to internal state
      char *packetData() const { return data ; }
      char *packetEnd() const { return data ? data + length : 0 ; }
      int packetLength() const { return length ; }
      FrPacketType packetType() const { return type ; }
      BYTE requestCode() const { return reqcode ; }
      BYTE sequenceNumber() const { return seqnumber ; }
      BYTE packetStatus() const { return status ; }
      BYTE packetComplete() const { return complete ; }
      BYTE packetError() const { return neterror ; }
      FrConnection *getConnection() const { return connection ; }
      int numFragments() const { return nfragments ; }
      int missingFragments() const { return missing ; }
      bool isMissing(int fragnum) const
	{ return (bool)(fragnum >= 0 && fragnum < nfragments &&
			  !fragments[fragnum]) ; }
   } ;

/**********************************************************************/
/**********************************************************************/

class FrServerInfo
   {
   public:
      int protocol_version ;		// version * 100, i.e. v1.23 = 123
      int server_version ;		// version * 100
      int server_patchlevel ;
      int max_request ;			// highest request number supported
      int max_notification ;		// highest notification number issued
      int outstanding_requests ;	// maximum outstanding requests
      int max_clients ;			// maximum simultaneous clients supp.
      int num_clients ;			// current number of clients
      bool peer_mode ;
      bool password_register ;		// password required to register?
      bool password_accessdb ;		// password required to access DB?
      bool password_createdb ;		// password required to create DB?
      bool password_revertframe ;	// password required to revert frame?
      bool password_deleteframe ;	// password required to delete frame?
      bool password_systemconfig ;	// passwd req. to update system config?
   public:
      FrServerInfo() {}
   } ;

/**********************************************************************/
/**********************************************************************/

class FrClient
   {
   private:
      FrClient *next, *prev ;
      FrConnection *connection ;
      FrServerInfo *server_info ;
      FrSymbolTable *symtab ;
      char *server_name ;
      char *user_name ;
      char *user_password ;
      int m_port ;
      int client_handle ;
      int client_ident ;

   private: // methods

   public: // methods
      FrClient() ;
      // connect with server, but don't register
      FrClient(const char *server, uint16_t client_id) ;
      // connect with server and register client
      FrClient(const char *server, int port, uint16_t client_id,
	       const char *username, const char *password) ;
      // clone a connection
      FrClient(FrConnection *connection) ;
      ~FrClient() ;
      FrServerInfo *getServerInfo() const ;
      bool connect(uint16_t client_id, const char *server, int port = 0,
		   const char *username = 0, const char *password = 0) ;
      bool disconnect() ;
      bool registerClient(int clientID, const char *username,
			  const char *password) ;
      bool unregisterClient() ;
      FrList *registeredClients(int ID = 0xFFFF, int handle = 0xFFFF,
				const char *username = 0) const ;
      bool getPreferences(int *broadcastmsg, int *personalmsg, int *clientmsg,
			  int *updatemsg,
			  char *clientdata = 0, int datasize = 0) ;
      bool setPreferences(int broadcastmsg, int personalmsg,
			  int clientmsg, int updatemsg,
			  const char *clientdata = 0, int datasize = 0,
			  FrAsyncCallback *callback = 0,void *client_data = 0);
      FrRemoteDB *openDatabase(const char *dbname, const char *password = 0) ;
      FrRemoteDB *createDatabase(const char *dbname, const char *password,
				 const DBUserData *info) ;
      FrRemoteDB *getDBbyName(const char *dbname) const ;
      bool closeDatabase(FrRemoteDB *db,
			 FrAsyncCallback *callback = 0,void *client_data = 0);
      int findIndex(const char *dbname, int indexnum, INDEXINFO *idxinfo) ;
      bool getUserData(const FrRemoteDB *db, DBUserData *userdata,
		       FrAsyncCallback *callback = 0, void *client_data = 0) ;
      bool setUserData(const FrRemoteDB *db, DBUserData *userdata,
		       FrAsyncCallback *callback = 0, void *client_data = 0) ;
      char *readDBIndex(const FrRemoteDB *db, int indexnum = 0,
			bool keyonly = false, const char *indexname = 0,
			FrAsyncCallback *callback = 0, void *client_data = 0) ;
      bool getFrame(const FrRemoteDB *db,FrSymbol *framename,
		    FrAsyncCallback *callback = 0, void *client_data = 0) ;
      bool getFrames(const FrRemoteDB *db,FrList *frames,
		     bool subclasses = false, bool parts_of = false,
		     FrAsyncCallback *callback = 0, void *client_data = 0) ;
      bool getOldFrame(const FrRemoteDB *db,FrSymbol *framename,int generation,
		       FrAsyncCallback *callback = 0,
		       void *client_data = 0) ;
      bool createFrame(const FrRemoteDB *db,FrSymbol *framename,
		       FrAsyncCallback *callback = 0, void *client_data = 0) ;
      FrSymbol *createNewFrame(const FrRemoteDB *db,FrSymbol *basename,
			     char **newname = 0,
			     FrAsyncCallback *callback = 0,
			     void *client_data = 0) ;
      bool updateFrame(const FrRemoteDB *db,FrSymbol *framename,
		       FrAsyncCallback *callback = 0, void *client_data = 0) ;
      bool deleteFrame(const FrRemoteDB *db,FrSymbol *framename,
		       const char *password = 0,
		       FrAsyncCallback *callback = 0, void *client_data = 0) ;
      bool lockFrame(const FrRemoteDB *db,const FrSymbol *framename,
		     FrAsyncCallback *callback = 0, void *client_data = 0) ;
      bool unlockFrame(const FrRemoteDB *db,const FrSymbol *framename,
		       FrAsyncCallback *callback = 0, void *client_data = 0) ;
      int matchingFrames(const FrRemoteDB *db, int format, int len,
			 const char *criteria, FrList **matches,
			 FrAsyncCallback *callback = 0,
			 void *client_data = 0) const ;
      int proxyAdd(const FrRemoteDB *db, const FrSymbol *framename,
		   const FrSymbol *slot, const FrSymbol *facet,
		   const FrObject *filler, FrAsyncCallback *callback = 0,
		   void *client_data = 0) ;
      int proxyDelete(const FrRemoteDB *db, const FrSymbol *framename,
		      const FrSymbol *slot, const FrSymbol *facet,
		      const FrObject *filler, FrAsyncCallback *callback = 0,
		      void *client_data = 0) ;
      int beginTransaction(const FrRemoteDB *db, FrAsyncCallback *callback = 0,
			   void *client_data = 0) ;
      bool endTransaction(const FrRemoteDB *db,int transaction,
			  FrAsyncCallback *callback = 0,void *client_data = 0) ;
      bool abortTransaction(const FrRemoteDB *db,int transaction,
			    FrAsyncCallback *callback = 0,
			    void *client_data = 0) ;
      bool personalMessage(int clienthandle, int urgent, int length,
			   const char *message,
			   FrAsyncCallback *callback = 0,
			   void *client_data = 0) ;
      int broadcastMessage(int length, const char *message,
			   FrAsyncCallback *callback = 0,
			   void *client_data = 0) ;
      bool clientMessage(int clienthandle, uint16_t msgtype, int &length,
			 const char *&message, FrAsyncCallback *callback = 0,
			 void *client_data = 0) ;
      bool serverStatistics(int stats, char *statbuffer,
			    FrAsyncCallback *callback = 0,
			    void *client_data = 0) ;
      long int isServerStillUp(FrAsyncCallback *callback = 0,
			       void *client_data = 0) ;
      FrIdleFunc *setIdleFunc(FrIdleFunc *idle_function) ;
      void setNotify(int type,FrNotifyHandler *notify_func) ;

      // access to internal state
      const char *serverName() const { return server_name ; }
      bool connected() const ; //!!!
      bool registered() const ; //!!!
      FrConnection *getConnection() const { return connection ; }
      int connectionSocket() const ; //{ return connection->connectionSocket() ; }
      FrNotifyHandler *getNotify(int type) ;

      // static member functions
      static FrClient *findClient(const char *servername, int port) ;
      static int process(int timeout = 0) ;
   //friends
      friend class FrRemoteDB ;
   } ;

/**********************************************************************/
/**********************************************************************/

class FrServer
   {
   protected:
      FrServerMsgHandler *msg_handler ;
      FrServerStatistics *statistics ;
      FrList *connections ;
      int socket ;
      int server_port ;
      int num_connections ;
      bool peer_mode ;
   public:
      FrServer() ;
      FrServer(bool peer) ;
      ~FrServer() ;
      int startup() ;
      int shutdown(int time) ;
      int process(int timeout = 0) ;
      void setMessageHandler(FrServerMsgHandler *handler)
	    { msg_handler = handler ; }
      FrServerMsgHandler *messageHandler() const { return msg_handler ; }
      void addConnection(FrConnection *) ;
      void removeConnection(FrConnection *) ;

      // access to internal state
      int numConnections() const { return num_connections ; }
   } ;

/**********************************************************************/
/**********************************************************************/

int get_server_identification(FrConnection *connection,FrPacket **reply) ;

#endif /* !__FRCLISRV_H_INCLUDED */

// end of file frclisrv.h //
