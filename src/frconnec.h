/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frconnec.h		 network-connection functions		*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1995,1996,1997,2001,2009				*/
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

#ifndef __FRCONNEC_H_INCLUDED
#define __FRCONNEC_H_INCLUDED

#ifndef __FRQUEUE_H_INCLUDED
#include "frqueue.h"
#endif

#ifndef __FRCLISRV_H_INCLUDED
#include "frclisrv.h"
#endif

#ifndef __FRBYTORD_H_INCLUDED
#include "frbytord.h"
#endif

#if defined(__GNUC__)
#  pragma interface
#endif

/************************************************************************/
/*    forward class and structure declarations				*/
/************************************************************************/

class FrPacket ;
class FrServerData ;

/************************************************************************/
/*    manifest constants						*/
/************************************************************************/

#define FrSERVER_PORT	5924
#define FrPEER_PORT	5925

#define FRAGMENT_FLAG	0x8000	 // bit 15
#define FRAGMENT_HEADER 0xFFFF

#define NE_SUCCESS	0
#define NE_READERR	1
#define NE_EXCEPTION	2
#define NE_SIGPIPE	0x5350

#define MAX_DATABASES	64

/************************************************************************/
/*    Enumerated Types							*/
/************************************************************************/

enum FrServerRequest
   {
    FSReq_IDENTIFY = 0x00,
    FSReq_REGISTER = 0x01,
    FSReq_UNREGISTER = 0x02,
    FSReq_GETDATABASELIST = 0x03,
    FSReq_GETINDEXINFO = 0x04,
    FSReq_FINDCLIENTS = 0x05,
    FSReq_GETUSERPREF = 0x06,
    FSReq_SETUSERPREF = 0x07,
    FSReq_OPENDATABASE = 0x08,
    FSReq_CREATEDATABASE = 0x09,
    FSReq_CLOSEDATABASE = 0x0A,
    FSReq_CREATEINDEX = 0x0B,
    FSReq_GETDBINDEX = 0x0C,
    FSReq_GETFRAME = 0x0D,
    FSReq_GETOLDFRAME = 0x0E,
    FSReq_GETFILLERS = 0x0F,
    FSReq_LOCKFRAME = 0x10,
    FSReq_UNLOCKFRAME = 0x11,
    FSReq_CREATEFRAME = 0x12,
    FSReq_DELETEFRAME = 0x13,
    FSReq_REVERTFRAME = 0x14,
    FSReq_UPDATEFRAME = 0x15,
    FSReq_BEGINTRANSACTION = 0x16,
    FSReq_ENDTRANSACTION = 0x17,
    FSReq_ABORTTRANSACTION = 0x18,
    FSReq_SENDPERSONALMSG = 0x19,
    FSReq_SENDBROADCASTMSG = 0x1A,
    FSReq_SENDCLIENTMSG = 0x1B,
    FSReq_TEST_ISA_P = 0x1C,
    FSReq_CHECKRESTRICTIONS = 0x1D,
    FSReq_INDEXEDRETRIEVAL = 0x1E,
    FSReq_SERVERSTATISTICS = 0x1F,
    FSReq_HOWAREYOU = 0x20,
    FSReq_GETSYSTEMCONFIG = 0x21,
    FSReq_SETSYSTEMCONFIG = 0x22,
    FSReq_GETUSERDATA = 0x23,
    FSReq_SETUSERDATA = 0x24,
    FSReq_PROXYUPDATE = 0x25,
    FSReq_INHERITABLEFACETS = 0x26,
    FSReq_INHERITFILLERS = 0x27,

    FSReq_Last = FSReq_INHERITFILLERS
   } ;

enum FSR_Identify
   {
    FSRIdent_PROTOMAJORVER = 0x00,
    FSRIdent_PROTOMINORVER = 0x01,
    FSRIdent_SERVERMAJORVER = 0x02,
    FSRIdent_SERVERMINORVER = 0x03,
    FSRIdent_SERVERPATCHLEV = 0x04,
    FSRIdent_SIGNATURE = 0x05,
    FSRIdent_MAXREQUEST = 0x18,
    FSRIdent_MAXNOTIFY = 0x19,
    FSRIdent_REQUESTLIMIT = 0x1A,
    FSRIdent_PSWD_REGISTER = 0x1C,
    FSRIdent_PSWD_DBOPEN = 0x1D,
    FSRIdent_PSWD_DBCREATE = 0x1E,
    FSRIdent_PSWD_REVERT = 0x1F,
    FSRIdent_PSWD_DELETE = 0x20,
    FSRIdent_PSWD_SETCONFIG = 0x21,
    FSRIdent_MAXCLIENTS = 0x4B,
    FSRIdent_CURRCLIENTS = 0x4D,
    FSRIdent_COMMENT = 0x4F
   } ;

#define SIGNATURE_SERVER "FramepaC DB-Server"
#define SIGNATURE_PEER	 "FramepaC Peer2Peer"

enum FSR_Register
   {
    FSRReg_MAXPACKET = 0x00,
    FSRReg_CLIENTID = 0x02,
    FSRReg_REGMODE = 0x04,
    FSRReg_GETPREF = 0x05,
    FSRReg_USERNAME = 0x06,

    FSRReg_CLIENTHANDLE = 0x02,
    FSRReg_BROADCASTMSG = 0x04,
    FSRReg_PERSONALMSG = 0x05,
    FSRReg_NOTIFICATIONS = 0x06,
    FSRReg_CLIENTDATA = 0x07
   } ;

enum FSR_Unregister
   {
    FSRunreg_CLIENTHANDLE = 0x00,
    //
      FSRunreg_REQLENGTH = 2,
   } ;

enum FSR_DBList
   {
    FSRdblist_DBNAME = 0x00,

    FSRdblist_MATCHES = 0x00,
    FSRdblist_FIRSTNAME = 0x02,
   } ;

enum FSR_IndexInfo
   {
    FSRIInfo_INDEXNUM = 0x00,
    FSRIInfo_DBNAME = 0x02,

    FSRIInfo_NEXTINDEX = 0x00,
    FSRIInfo_INDEXSIZE = 0x02,
    FSRIInfo_LASTMOD = 0x06,
    FSRIInfo_INDEXTYPE = 0x0A,
    FSRIInfo_NUMENTRIES = 0x0C,
    FSRIInfo_INDEXNAME = 0x20
   } ;

//FindClient request
#define FSRfcli_MAXLISTING	0x00
#define FSRfcli_CLIENTID	0x02
#define FSRfcli_CLIENTHANDLE	0x04
#define FSRfcli_CLIENTNAME	0x06
//FindClient reply
#define FSRfcli_NUMLISTED	0x00
#define FSRfcli_CLIENTID	0x02
#define FSRfcli_CLIENTHANDLE	0x04
#define FSRfcli_USERNAME	0x06

enum FSR_GetPrefs
   {
   FSRgpref_CLIENTID = 0x00,
   //
     FSRgpref_REQLENGTH = 2,
   } ;

enum FSR_SetPrefs
   {
   FSRspref_CLIENTID = 0x00,
   FSRspref_BCASTMSG = 0x02,
   FSRspref_PERSONMSG = 0x03,
   FSRspref_CLIENTMSG = 0x04,
   FSRspref_UPDATENOTIFY = 0x05,
   FSRspref_CLIENTDATA = 0x06,
   } ;

enum FSR_OpenDB
   {
   FSRopendb_FLAGS = 0x00,
   FSRopendb_DBNAME = 0x01,
   //FSRopendb_PASSWORD = var

   FSRopendb_DBHANDLE = 0x00,
   FSRopendb_CFGNUM = 0x01,
   FSRopendb_CFGDATA = 0x02,
   } ;

enum FSR_CreateDB
   {
   FSRcrdb_ESTSIZE = 0x00,
   FSRcrdb_ESTRECS = 0x04,
   FSRcrdb_CFGNUM = 0x08,
   FSRcrdb_DBNAME = 0x5C,
   //FSRcrdb_PASSWORD = var

   FSRcrdb_DBHANDLE = 0x00
   } ;

enum FSR_CloseDB
   {
   FSRclose_DBHANDLE = 0x00,
   //
     FSRclose_REQLENGTH = 1,
   } ;

enum FSR_CreateIndex
   {
   FSRcrindex_DBHANDLE = 0x00,
   FSRcrindex_INDEXTYPE = 0x01,
   //additional data?
      FSRcrindex_REQLENGTH = 3,
   } ;

enum FSR_GetIndex
   {
   FSRgetidx_DBHANDLE = 0x00,
   FSRgetidx_FULLINDEX = 0x01,
   FSRgetidx_INDEXNUM = 0x02,
   FSRgetidx_INDEXNAME = 0x04,

   FSRgetidx_INDEXTYPE = 0x00,
   FSRgetidx_FOUNDNUM = 0x01,
   FSRgetidx_INDEX = 0x10
   } ;

enum FSR_GetFrame
   {
   FSRgetframe_DBHANDLE = 0x00,
   FSRgetframe_SUBCLASSES = 0x01,
   FSRgetframe_PARTSOF = 0x02,
   FSRgetframe_FRAME = 0x03,

   FSRgetframe_FRAMEREP = 0x00
   } ;

enum FSR_GetOldFrame
   {
   FSRgetold_DBHANDLE = 0x00,
   FSRgetold_GENERATION = 0x01,
   FSRgetold_FRAME = 0x03,

   FSRgetold_FRAMEREP = 0x00
   } ;

enum FSR_GetFillers
   {
   FSRgetfill_DBHANDLE = 0x00,
   FSRgetfill_INHERITANCE = 0x01,
   FSRgetfill_FRAME = 0x02,
   //FSRgetfill_SLOT = var
   //FSRgetfill_FACET = var

   FSRgetfill_FILLERS = 0x00
   } ;

enum FSR_LockFrame
   {
   FSRlock_DBHANDLE = 0x00,
   FSRlock_WAIT = 0x01,
   FSRlock_FRAME = 0x03
   } ;

enum FSR_UnlockFrame
   {
   FSRunlock_DBHANDLE = 0x00,
   FSRunlock_FRAME = 0x01
   } ;

enum FSR_CreateFrame
   {
   FSRcreate_DBHANDLE = 0x00,
   FSRcreate_GENSYM = 0x01,
   FSRcreate_FRAME = 0x02,
      FSRcreate_MINLENGTH = 0x03,

   FSRcreate_NEWNAME = 0x00
   } ;

enum FSR_DeleteFrame
   {
   FSRdelete_DBHANDLE = 0x00,
   FSRdelete_FRAME = 0x01,
   //FSRdelete_PASSWORD = var
      FSRdelete_MINLENGTH = 0x03
   } ;

enum FSR_RevertFrame
   {
   FSRrevert_DBHANDLE = 0x00,
   FSRrevert_GENERATIONS = 0x01,
   FSRrevert_FRAME = 0x03,
   //FSRrevert_PASSWORD = var
      FSRrevert_MINLENGTH = 0x05
   } ;

enum FSR_UpdateFrame
   {
   FSRupdate_DBHANDLE = 0x00,
   FSRupdate_FRAMEREP = 0x01,
      FSRupdate_MINLENGTH = 0x05
   } ;

enum FSR_BeginTransaction
   {
   FSRbtrans_DBHANDLE = 0x00,
      FSRbtrans_PACKETLENGTH = 0x01,

   FSRbtrans_TRANSACTION = 0x00
   } ;

enum FSR_EndTransaction
   {
   FSRetrans_DBHANDLE = 0x00,
   FSRetrans_TRANSACTION = 0x01,
      FSRetrans_PACKETLENGTH = 0x03
   } ;

enum FSR_AbortTransaction
   {
   FSRatrans_DBHANDLE = 0x00,
   FSRatrans_TRANSACTION = 0x01,
   FSRatrans_PACKETLENGTH = 0x03
   } ;

enum FSR_PersonalMessage
   {
   FSRpmsg_RECIPIENT = 0x00,
   FSRpmsg_URGENT = 0x02,
   FSRpmsg_MSGLEN = 0x03,
   FSRpmsg_MESSAGE = 0x04,
      FSRpmsg_MINLENGTH = 0x04
   } ;

enum FSR_BroadcastMessage
   {
   FSRbmsg_MSGLEN = 0x00,
   FSRbmsg_MESSAGE = 0x01,
      FSRbmsg_MINLENGTH = 0x01,

   FSRbmsg_UNDELIVERED = 0x00
   } ;

enum FSR_ClientMessage
   {
   FSRcmsg_RECIPIENT = 0x00,
   FSRcmsg_MSGTYPE = 0x02,
   FSRcmsg_MSGLEN = 0x04,
   FSRcmsg_MESSAGE = 0x05,
      FSRcmsg_MINLENGTH = 0x05,

   FSRcmsg_REPLYLEN = 0x00,
   FSRcmsg_REPLYMSG = 0x01
   } ;

enum FSR_TestISA
   {
   FSRisa_DBHANDLE = 0x00,
   FSRisa_HIERARCHY = 0x01,
   FSRisa_FRAME = 0x02,
   //FSRisa_ANCESTOR = var
      FSRisa_MINLENGTH = 0x04
   } ;

enum FSR_CheckRestrictions
   {
   FSRrestrict_DBHANDLE = 0x00,
   FSRrestrict_FRAME = 0x0C,
   //FSRrestrict_FRAMEREP = var
      FSRrestrict_MINLENGTH = 0x0D,

   FSRrestrict_VIOLATIONS = 0x00
   } ;

enum FSR_IndexedRetrieval
   {
   FSRiget_DBHANDLE = 0x00,
   FSRiget_FORMAT = 0x01,
   FSRiget_FLAGS = 0x03,
   FSRiget_QUERY = 0x04,
   //FSRiget_ROOTFRAME = var
      FSRiget_MINLENGTH = 0x03,

   FSRiget_FOUNDFRAMES = 0x00
   } ;

enum FSR_ServerStatistics
   {
   FSRstats_TYPE = 0x00,
      FSRstats_PACKETLENGTH = 0x01,
   //!!!
      FSRstats_REPSIZE_GENERAL = 34,
      FSRstats_REPSIZE_COUNTS = 34,
      FSRstats_REPSIZE_NOTIFY = 34,
   } ;

enum FSR_HowAreYou
   {
   //no data on request
      FSRhay_PACKETLENGTH = 0x00,
   //reply data
   FSRhay_PENDINGREQUESTS = 0x00,
   } ;

enum FSR_GetSystemConfig
   {
   FSRgetsys_CONFIGNUM = 0x00,
   FSRgetsys_CHECKONLY = 0x02,
      FSRgetsys_PACKETLENGTH = 0x03,

   FSRgetsys_CONFIGSIZE = 0x00,
   FSRgetsys_CONFIGDATA = 0x02
   } ;

enum FSR_SetSystemConfig
   {
   FSRsetsys_CONFIGNUM = 0x00,
   FSRsetsys_CONFIGSIZE = 0x02,
   FSRsetsys_CONFIGDATA = 0x04,
   //FSRsetsys_PASSWORD = var
      FSRsetsys_MINLENGTH = 0x05
   } ;

enum FSR_GetUserData
   {
   FSRgetuser_DBHANDLE = 0x00,
      FSRgetuser_PACKETLENGTH = 0x01,
   //!!!
   } ;

enum FSR_SetUserData
   {
   FSRsetuser_DBHANDLE = 0x00,
   //!!!
   } ;

enum FSR_ProxyUpdate
   {
   FSRproxy_DBHANDLE = 0x00,
   FSRproxy_FUNCTION = 0x01,
   FSRproxy_FRAME = 0x02,
   //FSRproxy_SLOT = var
   //FSRproxy_FACET = var
   //FSRproxy_FILLER = var
   } ;

enum FSR_FindFacets
   {
   FSRfacets_DBHANDLE = 0x00,
   FSRfacets_INHERITANCE = 0x01,
   FSRfacets_FRAME = 0x02,

   FSRfacets_FACETS = 0x00
   } ;

enum FSR_InheritAllFillers
   {
   FSRinherit_DBHANDLE = 0x00,
   FSRinherit_INHERITANCE = 0x01,
   FSRinherit_FRAME = 0x02,

   FSRinherit_FRAMEREP = 0x00
   } ;

//----------------------------------------------------------------------

enum FSN_GoingDown
   {
   FSNgoingdown_SECONDS = 0x00
   } ;

enum FSN_BroadcastMsg
   {
   FSNbcast_MESSAGE = 0x00
   } ;

enum FSN_PersonalMsg
   {
   FSNpmsg_MESSAGE = 0x00
   } ;

enum FSN_ClientMsg
   {
   FSNcmsg_CLIENTID = 0x00,
   FSNcmsg_CLIENTHANDLE = 0x02,
   FSNcmsg_MSGTYPE = 0x04,
   FSNcmsg_MSGLEN = 0x06,
   FSNcmsg_MESSAGE = 0x07,

   FSNcmsg_REPLYLEN = 0x00,
   FSNcmsg_REPLYMSG = 0x01
   } ;

enum FSN_FrameLocked
   {
   FSNlock_DBHANDLE = 0x00,
   FSNlock_FRAME = 0x01
   } ;

enum FSN_FrameUnlocked
   {
   FSNunlock_DBHANDLE = 0x00,
   FSNunlock_FRAME = 0x01
   } ;

enum FSN_FrameCreated
   {
   FSNcreate_DBHANDLE = 0x00,
   FSNcreate_FRAME = 0x01,

   FSNcreate_NEWLOC = 0x00
   } ;

enum FSN_FrameDeleted
   {
   FSNdelete_DBHANDLE = 0x00,
   FSNdelete_FRAME = 0x01
   } ;

enum FSN_FrameUpdated
   {
   FSNupdate_DBHANDLE = 0x00,
   FSNupdate_FRAME = 0x01,

   FSNupdate_NEWLOC = 0x00
   } ;

enum FSN_CheckActivity
   {
   FSNactive_DBHANDLE = 0x00,
   FSNactive_FRAME   = 0x01,
   //FSNactive_USER = var

   FSNactive_TIMEOUT = 0x00
   } ;

enum FSN_ForcedDiscard
   {
   FSNdiscard_DBHANDLE = 0x00,
   FSNdiscard_FRAME = 0x01,
   } ;

enum FSN_ProxyUpdate
   {
   FSNproxy_DBHANDLE = 0x00,
   FSNproxy_OPERATION = 0x01,
   FSNproxy_FRAME = 0x02,
   //FSNproxy_SLOT = var
   //FSNproxy_FACET = var
   //FSNproxy_FILLER = var
   } ;

enum FSN_HandoffRequest
   {
   FSNhandoff_DBHANDLE = 0x00,
   FSNhandoff_URGENCY = 0x01
   } ;

enum FSN_ControlHandedOff
   {
   FSNhandover_DBHANDLE = 0x00,
   FSNhandover_PORT = 0x01,
   FSNhandover_SERVER = 0x02
   } ;

/************************************************************************/
/************************************************************************/

class FrPendingPacket
   {
   private:
      FrPacket *packet ;
      FrAsyncCallback *callback ;
      void *client_data ;
   public:
      FrPendingPacket() ;
      ~FrPendingPacket() ;
   } ;

/************************************************************************/
/************************************************************************/

class FrConnection : public FrObject
   {
   private:
      static FrConnection *connection_list ;
      FrConnection *prev, *next ;
      FrQueue queue ;
      char *server_name ;
      FrNotifyHandler *notification_table[FSNot_Last-FSNot_First+1] ;
      FrPendingPacket *pending[128] ;
      FrServerData *server_data ;
      int m_socket ;
      int max_datasize ;
      int curr_seqnum ;
      int neterror ;
      int num_users ;
      bool am_server ;
   public:
      FrConnection() ;
      FrConnection(int socket) ;
      FrConnection(const char *server, int port = 0) ;
      virtual ~FrConnection() ;
      bool connect(const char *server, int port = 0) ;
      bool disconnect() ;
      bool abort() ;
      bool send(FrPacket *packet) ;
      bool send(int hdrsize, char *header, int datasize, const char *data) ;
      bool sendNotification(FrPacket *request, int type, int datasize,
			    const char *data) ;
      int recv(FrQueue *queue) ;
      void setDefaultNotifications() ;
      FrNotifyHandler *getNotification(FrServerNotification type) const ;
      void setNotification(FrServerNotification type,
			   FrNotifyHandler *handler) ;
      void setNetError(int err) { neterror = err ; }
      FrPacket *replyReceived(const FrPacket *request) const ;
      //!!!

      // access to internal state
      bool connected() const { return (bool)(m_socket != -1) ; }
      int connectionSocket() const { return m_socket ; }
      int netError() const { return neterror ; }
      int maxDataSize() const { return max_datasize ; }
      FrServerData *serverData() const { return server_data ; }
      int nextSeqNum() ;
      FrQueue *getQueue() { return &queue ; }

      // static member functions
      static FrConnection *findConnection(const char *server, int port = 0) ;
   } ;


#endif /* !__FRCONNEC_H_INCLUDED */

// end of file frconnec.h //
