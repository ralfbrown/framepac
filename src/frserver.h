/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frserver.h	    network-server code				*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1995,1996,1997,2009,2015				*/
/*	    Ralf Brown/Carnegie Mellon University			*/
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

#ifndef __FRSERVER_H_INCLUDED
#define __FRSERVER_H_INCLUDED

#ifndef __FRCOMMON_H_INCLUDED
#include "frcommon.h"
#endif

#if defined(unix) || defined(__linux__) || defined(__GNUC__)
#  include <sys/time.h>
#else
#  include <time.h>
#endif

/************************************************************************/
/*    Manifest constants						*/
/************************************************************************/

#define FrPROTOCOL_MAJOR_VERSION	 1
#define FrPROTOCOL_MINOR_VERSION	 0

#define FrSERVER_MAJOR_VERSION  1
#define FrSERVER_MINOR_VERSION  0
#define FrSERVER_PATCHLEVEL     0

/************************************************************************/
/************************************************************************/

#define FrSRV_MAX_DATABASES   64

/************************************************************************/
/************************************************************************/

class FrConnection ;
class FrPacket ;
class FrServerDB ;

class FrServerData
   {
   private:
      FrPacket *sent_notifications[128] ;
      FrServerDB *databases[FrSRV_MAX_DATABASES] ;
      time_t last_notify_time ;
      time_t last_request_time ;
      int client_handle ;
      int open_databases ;
   public:
      FrServerData() ;
      ~FrServerData() ;
      void sentNotification(int seqnum) ;
      void gotNotification(int seqnum) ;
      void gotRequest() ;
      bool dbhandleAvailable() ;
      int openDatabase(FrServerDB *db) ;
      bool closeDatabase(int handle) ;
      FrServerDB *selectDatabase(int handle,
				 FrSymbolTable **symtab = 0) ;

      // access to internal state
      int clientHandle() const { return client_handle ; }
      FrServerDB *getDatabase(int handle) ;
      time_t lastNotification() const { return last_notify_time ; }
      time_t lastRequest() const { return last_request_time ; }
   } ;

/************************************************************************/
/************************************************************************/

void initialize_peer_mode() ;
void shutdown_peer_mode() ;

FrSymbolTable *select_database(FrConnection *connection, int dbhandle) ;

#endif /* !__FRSERVER_H_INCLUDED */

// end of file frserver.h //
