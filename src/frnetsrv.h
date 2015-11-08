/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frnetsrv.h		class FrNetworkServer			*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1996,1997,1998,1999,2000,2001,2006,2009		*/
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

#ifndef __FRNETSRV_H_INCLUDED
#define __FRNETSRV_H_INCLUDED

#ifndef __FRSCKSTR_H_INCLUDED
#include "frsckstr.h"
#endif

/************************************************************************/
/************************************************************************/

class FrEvent ;
class FrEventList ;
class FrNetworkServer ;
class FrNetworkConnection ;

// function to be called in order to continue processing a network server
//   request that was interrupted in order to allow work for other clients;
//   return true if the function needs to be called again for the same
//   request or false if the request has been completed (the server will
//   do the equivalent of setContinuation(0,0)).
typedef bool FrNetworkContinueFunc(FrNetworkServer *server,
				   void *user_data) ;


class FrNetworkServer
{
   protected:
      int port_number ;			// port on which to listen
      size_t num_connections ;		// currently-active connections
      size_t max_connections ;		// maximum simultaneous connections
      size_t curr_connection ;		// which connection are we processing?
      FrSocket *sockets ;
      FrNetworkConnection **connections ;
      FrEventList *events ;
      bool shutdown_requested ;
      bool unicode ;			// use Unicode (16-bit) characters?
      bool unicode_bswap ;		// do Unicode chars need byte-swapping?
      bool EUC ;			// use EUC multi-byte characters?
      bool want_canonical ;		// should lines be canonicalized?
      bool reading_objects ;		// read by object instead of by line?
   private:
      bool processInput() ;
      bool haveContinuations() ;	// any conns have continued calls?
   public:
      FrNetworkServer(int portnum, size_t maxconn = 0) ;
      virtual ~FrNetworkServer() ;

      bool run(clock_t timeout = 0) ;
      void requestShutdown() ;		// may be called from onLineReceived()

      FrEvent *addEvent(time_t first_time, bool from_now = true,
			void *user_data = 0, size_t interval = 0) ;
      bool setRepeatInterval(FrEvent *event, size_t interval) const ;

      bool disconnect(size_t conn) ;
      bool shutdown() ;		// disconnect all

      // network activity callbacks; override as appropriate
      virtual void onConnect(size_t conn) ;
      virtual void onDisconnect(size_t conn) ;
      virtual bool onLineReceived(size_t conn, const char *line) ;
      virtual bool onObjectReceived(size_t conn, FrObject *obj) ;
      virtual bool onBackgroundWork(bool idle) ;
           // return true if more work to do; param says if srv has stuff to do
	   // (server may block awaiting connection if return is false)
      virtual bool onTimedEvent(void *user_data) ; // return false to cancel
      virtual void onEventDone(void *user_data) ;  // must free user_data

      // modifiers
      void setUnicode(bool uni, bool uni_bswap = false) ;
      void setEUC(bool euc = true) ;
      void setContinuation(FrNetworkContinueFunc *func, void *data = 0) ;
      void wantCanonical(bool canon) { want_canonical = canon ; }
      void readObjects(bool readobj) { reading_objects = readobj ; }

      // accessors
      size_t activeConnections() const { return num_connections ; }
      size_t maximumConnections() const { return max_connections ; }
      size_t currentConnection() const { return curr_connection ; }
      FrISockStream *inputStream(size_t conn_num) const ;
      FrISockStream *inputStream() const
	 { return inputStream(currentConnection()) ; }
      FrOSockStream *outputStream(size_t conn_num) const ;
      FrOSockStream *outputStream() const
	 { return outputStream(currentConnection()) ; }
      FrSocket connectionSocket(size_t conn_num) const
	    { return sockets[conn_num] ; }
      FrSocket connectionSocket() const
	    { return sockets[currentConnection()] ; }
      bool good() const ;
      bool wantCanonical() const { return want_canonical ; }
      bool readingObjects() const { return reading_objects ; }
} ;

#endif /* !__FRNETSRV_H_INCLUDED */

// end of file frnetsrv.h //
