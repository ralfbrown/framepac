/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frconnec.cpp		 network-connection functions		*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1995,1996,1997,1998,2000,2001,2009,2015		*/
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
#  pragma implementation "frconnec.h"
#endif

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <sys/types.h>
#if defined(unix) || defined(__linux__) || defined(__GNUC__)
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "frconfig.h"
#ifdef __SOLARIS__
#  include </usr/include/netdb.h>
    // work around for problems with /usr/local/include/netdb.h being included
#else
#  include <netdb.h>
#endif /* __SOLARIS__ */
#else
#include <time.h>
#endif
#include "framerr.h"
#include "frclisrv.h"
#include "frctype.h"
#include "frserver.h"
#include "frmswin.h"
#include "frsckstr.h"
#include "frconnec.h"
#include "frsignal.h"

#if (defined(__MSDOS__) || defined(MSDOS)) && !defined(__NT__)
#undef FrSERVER
#endif

#if defined(FrSERVER)

#if defined(__MSDOS__) || defined(MSDOS)
#  include <io.h>
#elif defined(__SUNOS__)
#  include <sys/unistd.h>
#elif defined(unix) || defined(__linux__) || defined(__GNUC__)
#  include <unistd.h>
#  include <sys/socket.h>
#endif

#define LOCALHOSTNAME_SIZE 255

#if defined(unix)
#if defined(__SUNOS__) || defined(__SOLARIS__)
#include <unistd.h>
#include <sys/signal.h>
#endif /* __SUNOS__ || __SOLARIS__ */

#elif defined(__WINDOWS__) || defined(__NT__)
#undef INVALID_SOCKET
#include <winsock.h>
#endif /* unix / __WINDOWS__||__NT__ */

#ifdef __WATCOMC__
#include <time.h>
#endif /* __WATCOMC__ */

/************************************************************************/
/*    Global variables local to this module			      	*/
/************************************************************************/

static FrConnection *current_connection ;

FrConnection *FrConnection::connection_list = 0 ;

/**********************************************************************/
/*    Utility Functions						      */
/**********************************************************************/

//----------------------------------------------------------------------

static void print_header(const char *header, int hdrsize,
			 const char *prefix = "==>")
{
   fprintf(stderr,prefix);
   for (int j=0;j<hdrsize;j++)
      fprintf(stderr," %2.02X",(unsigned char)header[j]) ;
   fprintf(stderr,"\n") ;
}

/**********************************************************************/
/*    Exception Handling					      */
/**********************************************************************/

#ifdef SIGPIPE
static FrSignalHandler *sigpipe ;
#endif /* SIGPIPE */
static bool sigpipe_ignored = false ;

//----------------------------------------------------------------------

static void ignore_sigpipe()
{
   if (!sigpipe_ignored)
      {
#ifdef SIGPIPE
      sigpipe = new FrSignalHandler(SIGPIPE,SIG_IGN) ;
#endif /* SIGPIPE */
      sigpipe_ignored = true ;
      }
   return ;
}

//----------------------------------------------------------------------

static void restore_sigpipe()
{
   if (sigpipe_ignored)
      {
#ifdef SIGPIPE
      delete sigpipe ;
      sigpipe = 0 ;
#endif /* SIGPIPE */
      sigpipe_ignored = false ;
      }
   return ;
}

//----------------------------------------------------------------------

static int connection_died(FrSocket socket)
{
#ifdef FrBSD_SOCKETS
   struct timeval timeout ;
   timeout.tv_sec = timeout.tv_usec = 0 ;
   fd_set errset ;
   FD_ZERO(&errset) ;
   FD_SET(socket,&errset) ;
   if (select(FD_SETSIZE,NULL,NULL,&errset,&timeout) > 0)
      return true ;
#else
   (void)socket ;
#endif /* FrBSD_SOCKETS */
   return false ;
}

/************************************************************************/
/*    notification handlers	   					*/
/************************************************************************/

static void default_notification_nop(FrPacket *packet)
{
   delete packet ;
}

//----------------------------------------------------------------------

static void default_notification_accepted(FrPacket *packet)
{
   packet->respond(FSE_Success) ;
   delete packet ;
}

//----------------------------------------------------------------------

static void default_notification_unknown(FrPacket *packet)
{
   packet->respond(FSE_INVALIDREQUEST) ;
   delete packet ;
}

//----------------------------------------------------------------------

static void default_peer_handoff(FrPacket *packet)
{
   packet->respond(FSE_INVALIDREQUEST) ;
   delete packet ;
}

//----------------------------------------------------------------------

static void default_peer_handed_off(FrPacket *packet)
{
   packet->respond(FSE_INVALIDREQUEST) ;
   delete packet ;
}

/************************************************************************/
/*    methods for class FrConnection					*/
/************************************************************************/

FrConnection::FrConnection()
{
   m_socket = -1 ;
   num_users = 0 ;
   prev = 0 ;
   next = connection_list ;
   connection_list = this ;
   max_datasize = 512 ;
   setDefaultNotifications() ;
   return ;
}

//----------------------------------------------------------------------

FrConnection::FrConnection(int sock)
{
   m_socket = sock ;
   num_users = 1 ;
   prev = 0 ;
   next = connection_list ;
   connection_list = this ;
   max_datasize = 512 ;
   setDefaultNotifications() ;
   return ;
}

//----------------------------------------------------------------------

FrConnection::~FrConnection()
{
   if (m_socket != -1)
      {
      close_socket(m_socket) ;
      }
   // unlink from connection list
   if (prev)
      prev->next = next ;
   else
      connection_list = next ;
   if (next)
      next->prev = prev ;
   return ;
}

//----------------------------------------------------------------------

FrConnection::FrConnection(const char *server, int port)
{
   m_socket = -1 ;
   setDefaultNotifications() ;
   connect(server,port) ;
   return ;
}

//----------------------------------------------------------------------

bool FrConnection::connect(const char *server, int port)
{
   if (!server || !*server)
      {
      Fr_errno = FSE_NOSUCHSERVER ;
      return false ;
      }
   if (!server_name)
      {
      server_name = FrDupString(server) ;
      }
   if (port == 0)
      port = FrSERVER_PORT ;
   if (Fr_stricmp(server,server_name) == 0) // is this the correct server?
      {
      num_users++ ;
      if (m_socket == (FrSocket)-1)	// did something cause a disconnect?
	 {
	 // attempt to (re)establish connection with server
	 m_socket = FrConnectToPort(server_name,port) ;
	 if (m_socket == (FrSocket)-1)
	    {
	    Fr_errno = FSE_NOSUCHSERVER ;
	    return false ;
	    }
	 // verify that we are indeed talking to an actual server
	 FrPacket *reply ;
	 int status = get_server_identification(this,&reply) ;
	 if (!reply || status != FSE_Success)
	    {
	    // error, so abort the connection
	    disconnect() ;
	    Fr_errno = FSE_NOSUCHSERVER ;
	    return false ;
	    }
	 char *data = reply->packetData() ;
	 int proto_version = 100*FrLoadByte(data+FSRIdent_PROTOMAJORVER) +
			     FrLoadByte(data+FSRIdent_PROTOMINORVER) ;
	 if ((strcmp(data+FSRIdent_SIGNATURE,SIGNATURE_SERVER) == 0 ||
	      strcmp(data+FSRIdent_SIGNATURE,SIGNATURE_PEER) == 0) &&
	     proto_version >= 100*FrPROTOCOL_MAJOR_VERSION+FrPROTOCOL_MINOR_VERSION)
	    {
	    // good server....
	    }
	 else
	    {
	    // bad server, so abort the connection
	    disconnect() ;
	    Fr_errno = FSE_NOTSERVER ;
	    return false ;
	    }
	 }
      }
   else
      {
      Fr_errno = FSE_ALREADYCONNECTED ;
      return false ;
      }
   return true ;
}

//----------------------------------------------------------------------

FrConnection *FrConnection::findConnection(const char *server, int port)
{
   for (FrConnection *conn = connection_list ; conn ; conn = conn->next)
      {
      if (conn->port == port && conn->server_name &&
	  strcmp(server,conn->server_name) == 0)
	 return conn ;
      }
   return 0 ;
}

//----------------------------------------------------------------------

bool FrConnection::disconnect()
{
   if (num_users > 0)
      {
      num_users-- ;
      if (num_users <= 0)
	 {
	 if (close_socket(m_socket) == -1)
	    return false ;
	 m_socket = -1 ;
	 }
      return true ;
      }
   return true ;
}

//----------------------------------------------------------------------

bool FrConnection::abort()
{
   if (m_socket != (FrSocket)-1)
      {
      bool success = (close_socket(m_socket) != -1) ;
      m_socket = (FrSocket)-1 ;
      return success ;
      }
   return true ;			// trivially successful
}

//----------------------------------------------------------------------

static bool CliServ_convert_errno(int errnum)
{
   switch (errnum)
      {
#ifdef EADDRINUSE
      case EADDRINUSE:
	 Fr_errno = FSE_ALREADYCONNECTED ;
	 break;
#endif
#ifdef ENOTSOCK
      case ENOTSOCK:
#endif
      case EBADF:
	 Fr_errno = FSE_NOSUCHCONNECTION ;
	 break ;
      default:
	 Fr_errno = errnum ;
	 break ;
      }
   return false ;
}

//----------------------------------------------------------------------

int FrConnection::nextSeqNum()
{
   int seqnum = curr_seqnum ;
   do {
      seqnum++ ;
      if (seqnum >= (int)lengthof(pending))
	 seqnum = 0 ;
      if (!pending[seqnum])
	 {
	 curr_seqnum = seqnum ;
	 return seqnum ;
	 }
      } while (seqnum != curr_seqnum) ;
   return -1 ;				// all sequence numbers in use!
}

//----------------------------------------------------------------------

FrPacket *FrConnection::replyReceived(const FrPacket *request) const
{
(void)request ;
   return 0 ; //!!!
}

//----------------------------------------------------------------------

bool FrConnection::send(int hdrsize, char *header, int datasize,
			const char *data)
{
   INIT_WINSOCK() ;
   FrMessageLoop() ;
   if (m_socket < 0)
      {
      Fr_errno = FSE_NOSUCHCONNECTION ;
      return false ;
      }
   current_connection = this ;
   int written = 0 ;
   const char *packet = data ;
// check for exception on the socket
   if (connection_died(m_socket))
      {
      // connection died....
      disconnect() ;
      return false ;
      }
   ignore_sigpipe() ;			// just in case the conn. dies in write
   if (datasize <= max_datasize)
      {
      FrStoreShort(datasize,header) ;
//!!!
//print_header(header,hdrsize) ;
      if ((written = write_socket(m_socket,header,hdrsize)) < hdrsize)
	 {
	 if (written == -1 && errno == EPIPE)
	    abort() ;
	 restore_sigpipe() ;
	 return CliServ_convert_errno(errno) ;
	 }
      while (datasize &&
	     (written = write_socket(m_socket,(char*)packet,datasize)) > 0)
	 {
         datasize -= written ;
         packet += written ;
         }
      if (datasize != 0 && written == -1 && errno == EPIPE)
	 abort() ;
      }
   else					// need to split up packet
      {
      FrStoreShort(FRAGMENT_HEADER, header+0) ;
//!!!
//print_header(header,hdrsize) ;
      if ((written = write_socket(m_socket,header,hdrsize)) < hdrsize)
	 {
	 if (written == -1 && errno == EPIPE)
	    abort() ;
	 restore_sigpipe() ;
	 return CliServ_convert_errno(errno) ;
	 }
      char fragheader[4] ;
      FrStoreLong(datasize,fragheader) ;
      if (write_socket(m_socket,fragheader,sizeof(fragheader)) <
	  (int)sizeof(fragheader))
	 {
	 restore_sigpipe() ;
	 return CliServ_convert_errno(errno) ;
	 }
      int overflow;
      int fragnum = 0 ;
      do {
	 int msg_size ;
	 FrStoreShort(fragnum++ | FRAGMENT_FLAG, header) ;
	 if (datasize > max_datasize)
	    {
	    msg_size = max_datasize ;
	    overflow = 1 ;
	    }
	 else
	    {
	    msg_size = datasize ;
	    overflow = 0 ;
	    }
//!!!
//print_header(header,4) ;
         if (write_socket(m_socket,header,4) < 4)
	    {
	    restore_sigpipe() ;
	    return CliServ_convert_errno(errno) ;
	    }
	 while (msg_size &&
		(written = write_socket(m_socket,(char*)packet,msg_size)) > 0)
	    {
	    packet += written ;
	    msg_size -= written ;
	    }
	 if (written < 0)
	    {
	    restore_sigpipe() ;
	    return CliServ_convert_errno(errno) ;
	    }
	 } while (overflow) ;
      }
   restore_sigpipe() ;
   return true ; // successful
}

//----------------------------------------------------------------------

bool FrConnection::send(FrPacket *packet)
{
   INIT_WINSOCK() ;
   FrMessageLoop() ;
   if (m_socket < 0)
      {
      Fr_errno = FSE_NOSUCHCONNECTION ;
      return false ;
      }
   char header[5] ;
   int hdrsize = 4 ;
   FrPacketType ptype = packet->packetType() ;
   if (ptype == PT_Reply || ptype == PT_Response)
      hdrsize = 5 ;
   FrStoreByte(packet->requestCode(),    header+2) ;
   FrStoreByte(packet->sequenceNumber(), header+3) ;
   if (hdrsize > 4)
      FrStoreByte(packet->packetStatus(),header+4) ;
   return send(hdrsize,header,packet->packetLength(),packet->packetData()) ;
}

//----------------------------------------------------------------------

bool FrConnection::sendNotification(FrPacket *request, int type, int datasize,
				    const char *data)
{
   if (m_socket < 0)
      {
      Fr_errno = FSE_NOSUCHCONNECTION ;
      return false ;
      }
   char header[4] ;
   FrStoreByte(type,         header+2) ;
   FrStoreByte(nextSeqNum(), header+3) ;
(void)request; //!!!
   return send(sizeof(header),header,datasize,data) ;
}

//----------------------------------------------------------------------

static FrPacket *allocate_big_packet(FrConnection *conn,char *head,
				     bool am_server)
{
   int connection = conn->connectionSocket() ;
   FrPacket *packet = new FrPacket(conn,head,am_server) ;
   // if a five-byte header, get the status field
   if (packet->packetType() == PT_Reply || packet->packetType() == PT_Response)
     {
     if (read_socket(connection,head+4,1) < 1)
        {
	CliServ_convert_errno(errno) ;
        return 0 ;
	}
     else
        packet->setStatus(FrLoadByte(head+4)) ;
     }
   // get the full-packet size field (a LONG)
   if (read_socket(connection,head,4) < 4)
     {
     delete packet ;
     CliServ_convert_errno(errno) ;
     return 0 ;
     }
//!!!
//print_header(head,4,"+++") ;
   int plength = FrLoadLong(head) ;
   if (!packet->allocateBuffer(plength))
      {
      FrWarning("unable to allocate packet") ;
      return 0 ;
      }
   return packet ;
}

//----------------------------------------------------------------------

int compare_packets(const FrObject *packet1, const FrObject *packet2)
{
   FrPacket *p1 = (FrPacket*)packet1 ;
   FrPacket *p2 = (FrPacket*)packet2 ;

   if (p1->getConnection() == p2->getConnection() &&
       p1->sequenceNumber() == p2->sequenceNumber() &&
       p1->requestCode() == p2->requestCode())
     return 1 ;  // they are equivalent!
   return 0 ;  // the packets differ
}

//----------------------------------------------------------------------

int FrConnection::recv(FrQueue *queue)
{
   INIT_WINSOCK() ;
   FrMessageLoop() ;
   current_connection = this ;
   char head[5] ;
   head[4] = '\0' ; // set status to "successful" for packets without field

   FrStoreByte(0x7F, head+2) ;  // invalid code to detect crashed connection
   FrStoreByte(0xFF, head+3) ;
   // check for exception/data-available on the socket
   if (connection_died(m_socket))
      {
      // the connection died....
      disconnect() ;
      current_connection = 0 ;
      return -1 ;
      }
   // OK, we can safely read from the socket
   if (read_socket(m_socket,head,4) < 4)
      {
      current_connection = 0 ;
      return CliServ_convert_errno(errno) ;
      }
   if (FrLoadByte(head+2) == 0x7F &&
       FrLoadByte(head+3) == 0xFF)	// crashed connection?
      {
cerr<<"crashed connection!"<<endl;
//      handle_crashed_connection(conn) ;
      current_connection = 0 ;
      return -1 ;
      }
//!!!
//print_header(head,4,"<==") ;
   FrPacket *packet ;
   int fragnum = 0 ;
   if (FrLoadLong(head) == FRAGMENT_HEADER)
      {
      packet = allocate_big_packet(this,head,am_server) ;
      if (!packet)
	 return -1 ;
      if (queue)
	 queue->add(packet,false) ;
      current_connection = 0 ;
      return 0 ;   // successful, but packet not complete
      }
   else if ((FrLoadLong(head) & FRAGMENT_FLAG) != 0)
      {
      FrPacket *newpack = new FrPacket(this,head,am_server) ;
      newpack->setComplete(true) ;
      packet = queue ? (FrPacket*)queue->find(newpack,compare_packets) : 0 ;
      if (packet)
	 {
	 delete newpack ;
	 queue->remove(packet) ;
	 }
      else
	 {
	 packet = newpack ;
	 //!!!
	 }
      fragnum = (FrLoadLong(head) & ~FRAGMENT_FLAG) ;
      if (fragnum >= packet->numFragments())
	 {

	 current_connection = 0 ;
	 return -1 ;   // Error!
	 }
      if (packet->isMissing(fragnum))
	 {
	 FrLocalAlloc(char,fragdata,1024,maxDataSize()) ;
	 //!!!
	 packet->setFragment(fragnum,fragdata) ;
	 FrLocalFree(fragdata) ;
	 }
      }
   else
      {
      packet = new FrPacket(this,head,am_server) ;
      if (!packet->allocateBuffer(packet->packetLength()))
	 {
	 FrNoMemory("allocating buffer for packet from server") ;
	 current_connection = 0 ;
	 return -1 ;  // Error!
	 }
      }
   if (packet->numFragments() == 0 &&
       (packet->packetType() == PT_Reply || packet->packetType() == PT_Response))
      {
      if (read_socket(m_socket,head+4,1) < 0)
	 return CliServ_convert_errno(errno) ;
      }
   packet->setStatus(FrLoadByte(head+4)) ;
   int datasize = packet->numFragments() ? max_datasize
					 : packet->packetLength() ;
   if (packet->numFragments() && fragnum == packet->numFragments()-1)
      datasize = packet->packetLength() % max_datasize ;
   char *data = packet->packetData() + (fragnum * max_datasize) ;
   if (datasize > 0 && read_socket(m_socket,data,datasize) < 0 )
      {
      delete packet ;
      current_connection = 0 ;
      return CliServ_convert_errno(errno) ;
      }
   if (packet->missingFragments() == 0)
      packet->setComplete(true) ;
   if (queue)
      queue->add(packet,false) ;
   current_connection = 0 ;
   return packet->packetComplete() ;
}

//----------------------------------------------------------------------

void FrConnection::setDefaultNotifications()
{
   notification_table[FSNot_TERMINATINGCONN-FSNot_First] =
      notification_table[FSNot_SERVERGOINGDOWN-FSNot_First] =
      notification_table[FSNot_BROADCASTMSG-FSNot_First] =
      notification_table[FSNot_CLIENTMSGTIMEOUT-FSNot_First] =
      notification_table[FSNot_DISCARDFRAME-FSNot_First] =
	 default_notification_nop ;
   notification_table[FSNot_FRAMELOCKED-FSNot_First] =
      notification_table[FSNot_FRAMEUNLOCKED-FSNot_First] =
      notification_table[FSNot_FRAMECREATED-FSNot_First] =
      notification_table[FSNot_FRAMEDELETED-FSNot_First] =
      notification_table[FSNot_FRAMEUPDATED-FSNot_First] =
      notification_table[FSNot_AREYOUTHERE-FSNot_First] =
      notification_table[FSNot_PERSONALMSG-FSNot_First] =
	 default_notification_accepted ;
   notification_table[FSNot_PEERHANDOFF-FSNot_First] =
	 default_peer_handoff ;
   notification_table[FSNot_NEWCONTROLLER-FSNot_First] =
	 default_peer_handed_off ;
   notification_table[FSNot_PROXYUPDATE-FSNot_First] =
	 default_notification_unknown ;
}

//----------------------------------------------------------------------

FrNotifyHandler *FrConnection::getNotification(FrServerNotification type) const
{
   if (type >= FSNot_First && type <= FSNot_Last)
      return notification_table[type-FSNot_First] ;
   else
      return 0 ;
}

//----------------------------------------------------------------------

void FrConnection::setNotification(FrServerNotification type,
				   FrNotifyHandler *handler)
{
   if (type >= FSNot_First && type <= FSNot_Last)
      notification_table[type-FSNot_First] = handler ;
}

//----------------------------------------------------------------------

#endif /* FrSERVER */

// end of file frconnec.cpp //
