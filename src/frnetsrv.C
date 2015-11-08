/************************************************************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frnetsrv.cpp		class FrNetworkServer			*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1996,1997,1998,1999,2000,2001,2003,2006,2012,2015	*/
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

#include "framerr.h"
#include "frctype.h"
#if defined(__WINDOWS__) || defined(__NT__) || defined(_WIN32)
#include <winsock.h>
#endif /* __WINDOWS__ || __NT__ || _WIN32 */
#if defined(unix) || defined(__linux__) || defined(__GNUC__)
#  include <unistd.h>		// needed by RedHat 7.1 for close()
#endif
#include "frevent.h"
#include "frexec.h"
#include "frfilutl.h"
#include "frnetsrv.h"
#include "frreader.h"
#include "frstring.h"
#include "frutil.h"

/************************************************************************/
/*    Manifest constants						*/
/************************************************************************/

// maximum number of simultaneous clients in network-server mode
#define MAX_CONNECTIONS 32

#define EXPAND_INCREMENT 128

/************************************************************************/
/*    Types for this module						*/
/************************************************************************/

class FrNetworkConnection
   {
#ifdef FrUSING_SOCKETS
   private:
      char *canonical_line ;		// canonicalized input line
      char *partial_object ;		// pending incomplete object
      FrObject *object ;		// current complete object, converted
      char line[FrMAX_LINE+1] ;		// current line of text
      FrISockStream *in ;
      FrOSockStream *out ;
      bool unicode ;			// I/O in Unicode?
      bool unicode_bswap ;
      bool EUC ;			// I/O in EUC/Big5/GB?
      bool want_canonical ;
      bool valid_object ;		// do we have a complete object yet?
      size_t linelen ;
      size_t partial_buflen ;		// chars allocated for partial_object
      size_t partial_count ;		// chars actually used by partial_obj
      FrNetworkContinueFunc *continue_func ;
      void *continue_data ;
   private: // methods
      void init() ;
      bool getChar(int &c) ;
      int peekChar() ;
      void putbackChar(int c) ;
   public: // methods
      FrNetworkConnection() ;
      FrNetworkConnection(FrSocket s) ;
      ~FrNetworkConnection() ;

      bool getPartialLine() ;
      bool getPartialObject() ;
      bool continuePriorCall(FrNetworkServer *server) ;
      const char *canonicalLine() const
	 { return want_canonical ? canonical_line : line ; }
      FrObject *receivedObject() const
         { return valid_object ? object : 0 ; }

      // modifiers
      void setUnicode() { unicode = true ; EUC = false ; }
      void clearUnicode() { unicode = false ; }
      void setEUC() { EUC = true ; unicode = false ; }
      void clearEUC() { EUC = false ; }
      void wantCanonical() { want_canonical = true ; }
      void setByteSwap(bool swap) { unicode_bswap = swap ; }
      void setContinuation(FrNetworkContinueFunc *func, void *data = 0)
         { continue_func = func ; continue_data = data ; }

      // access to internal state
      FrISockStream *inSocket() const { return in ; }
      FrOSockStream *outSocket() const { return out ; }
      bool inSocketGood() const { return in && in->good() ; }
      bool continued() const { return continue_func != 0 ; }
#endif /* FrUSING_SOCKETS */
   } ;

//----------------------------------------------------------------------

class FrNetworkServerEvent
   {
   private:
      time_t scheduled_time ;
      size_t repeat_interval ;
      FrEvent *invoking_event ;
      FrNetworkServer *invoking_server ;
      void *user_data ;
   public:
      FrNetworkServerEvent(time_t sched, size_t repeat, FrNetworkServer *srv,
			   void *user_data = 0) ;
      ~FrNetworkServerEvent() ;

      // accessors
      size_t repeatInterval() const { return repeat_interval ; }
      time_t scheduledTime() const { return scheduled_time ; }
      FrNetworkServer *server() const { return invoking_server ; }
      void *userData() const { return user_data ; }

      // modifiers
      void scheduledTime(time_t new_time) { scheduled_time = new_time ; }
      void repeatInterval(size_t interval) { repeat_interval = interval ; }
      void setEvent(FrEvent *ev) { invoking_event = ev ; }
   } ;

/************************************************************************/
/************************************************************************/

#ifdef FrUSING_SOCKETS
int current_connection = 0 ;
#endif /* FrUSING_SOCKETS */

/************************************************************************/
/*	Methods for class FrNetworkServerEvent				*/
/************************************************************************/

FrNetworkServerEvent::FrNetworkServerEvent(time_t sched, size_t repeat,
					   FrNetworkServer *srv,
					   void *udata)
{
   scheduled_time = sched ;
   repeat_interval = repeat ;
   invoking_event = 0 ;
   invoking_server = srv ;
   user_data = udata ;
   return ;
}

//----------------------------------------------------------------------

FrNetworkServerEvent::~FrNetworkServerEvent()
{
   scheduled_time = 0 ;
   if (invoking_server)
      invoking_server->onEventDone(user_data) ;
   user_data = 0 ;
   return ;
}

/************************************************************************/
/*	Methods for class FrNetworkConnection				*/
/************************************************************************/

FrNetworkConnection::FrNetworkConnection()
{
   init() ;
   return ;
}

//----------------------------------------------------------------------

FrNetworkConnection::FrNetworkConnection(FrSocket s)
{
   init() ;
   in = new FrISockStream(s) ;
   out = new FrOSockStream(s) ;
   if (out)
      out->useNonBlockingWrites() ;
   return ;
}

//----------------------------------------------------------------------

FrNetworkConnection::~FrNetworkConnection()
{
   delete in ;
   delete out ;
   FrFree(canonical_line) ;
   canonical_line = 0 ;
   FrFree(partial_object) ;
   partial_object = 0 ;
   partial_count = 0 ;
   partial_buflen = 0 ;
   free_object(object);
   return ;
}

//----------------------------------------------------------------------

void FrNetworkConnection::init()
{
   in = 0 ;
   out = 0 ;
   linelen = 0 ;
   partial_buflen = 0 ;
   partial_count = 0 ;
   unicode = false ;
   unicode_bswap = false ;
   EUC = false ;
   canonical_line = 0 ;
   object = 0 ;
   partial_object = 0 ;
   want_canonical = false ;
   valid_object = false ;
   continue_func = 0 ;
   return ;
}

//----------------------------------------------------------------------

bool FrNetworkConnection::getChar(int &ch)
{
   if (in->inputAvailable() > 0)
      {
      ch = in->get() ;
      return true ;
      }
   else
      {
      ch = EOF ;
      return false ;
      }
}

//----------------------------------------------------------------------

int FrNetworkConnection::peekChar()
{
   if (in->inputAvailable() > 0)
      return in->peek() ;
   else
      return EOF ;
}

//----------------------------------------------------------------------

void FrNetworkConnection::putbackChar(int c)
{
   in->putback((char)c) ;
   return ;
}

//----------------------------------------------------------------------

#ifdef FrUSING_SOCKETS
bool FrNetworkConnection::getPartialLine()
{
   if (canonical_line)
      {
      // get rid of the last vestiges of the previous line
      FrFree(canonical_line) ;
      canonical_line = 0 ;
      }
   if (unicode)
      {
      int c1 = 0 ;
      while (linelen < (int)(sizeof(line)-1) && getChar(c1))
	 {
	 // Windog likes to break up even very small packets into fragments
	 // with an odd number of bytes....
	 if (in->inputAvailable() <= 0)	// second char available?
	    {				//   if not, unget the first and
	    putbackChar(c1) ;		//   report that line is not yet
	    return false ;		//   complete
	    }
	 int c2 ;
	 getChar(c2) ;
	 int c ;
	 if (unicode_bswap)
	    c = ((c2 & 0xFF) << 8) | (c1 & 0xFF) ;
	 else
	    c = ((c1 & 0xFF) << 8) | (c2 & 0xFF) ;
	 if (c1 == EOF || c2 == EOF || c == '\n' ||
	     c == '\r')			// end of file or end of line?
	    {
	    line[linelen++] = '\0' ;	// (0x0000 is Unicode NUL)
	    line[linelen] = '\0' ;	// terminate the line,
	    if (want_canonical)
	       {
	       if (line[0] || line[1])
		  canonical_line = FrCanonicalizeUSentence((FrChar16*)line,0,
							   true) ;
	       else
		  canonical_line = FrNewC(char,sizeof(FrChar16)) ;
	       }
	    linelen = 0 ;		//  reset pointer for next line, and
	    return true ;		//  say that it is complete
	    }
	 // swallow the byte-order markers, toggling byte-swapping as needed
	 if (c == 0xFFFE)
	    {
	    unicode_bswap = !unicode_bswap ;
	    continue ;
	    }
	 else if (c == 0xFEFF)
	    continue ;
	 if (unicode_bswap)
	    {
	    line[linelen++] = (char)c2 ;
	    line[linelen++] = (char)c1 ;
	    }
	 else
	    {
	    line[linelen++] = (char)c1 ;
	    line[linelen++] = (char)c2 ;
	    }
	 }
      }
   else // !unicode
      {
      int c = 0 ;
      while (linelen < (int)(sizeof(line)-1) && getChar(c))
	 {
	 if (c == EOF || c == '\n' ||
	     c == '\r')			// is line finished?
	    {
	    line[linelen] = '\0' ;	// terminate the line,
	    linelen = 0 ;		//  reset pointer for next line, and
	    FrCharEncoding encoding = EUC ? FrChEnc_EUC : FrChEnc_Latin1 ;
	    if (*line)
	       canonical_line = FrCanonicalizeSentence(line,encoding) ;
	    else
	       canonical_line = FrDupString(line) ;
	    // handle MS-DOS/Windows end-of-line
	    if (c == '\r' && peekChar() == '\n')
	       (void)getChar(c) ;
	    return true ;		//  say that it is complete
	    }
	 line[linelen++] = (char)c ;
	 }
      }
   if (linelen >= (int)(sizeof(line)-1))
      {
      line[sizeof(line)-1] = '\0' ;	// terminate the line,
      linelen = 0 ;			// reset pointer for next line,
      return true ;			// and indicate that it is complete
      }
   return false ;			// the line is not yet complete
}
#endif /* FrUSING_SOCKETS */

//----------------------------------------------------------------------

#ifdef FrUSING_SOCKETS
bool FrNetworkConnection::getPartialObject()
{
   // get rid of the last vestiges of the previous object
   free_object(object) ;
   object = 0 ;
   valid_object = false ;
   // read as much input as is available, or until we get a complete object
   while (!in->eof() && !in->fail() && in->inputAvailable() > 0)
      {
      int c ;
      if (!getChar(c) || c == EOF)
	 break ;
      if (partial_count == 0 && Fr_isspace(c)) // skip leading whitespace
	 continue ;
      if (partial_count+1 >= partial_buflen)
	 {
	 char *new_partial = FrNewR(char,partial_object,
				    partial_buflen+EXPAND_INCREMENT) ;
	 if (new_partial)
	    {
	    partial_object = new_partial ;
	    partial_buflen += EXPAND_INCREMENT ;
	    }
	 else
	    return false ;
	 }
      const char *obj = partial_object ;
      partial_object[partial_count++] = (char)c ;
      partial_object[partial_count] = '\0' ;
      if (!Fr_isalpha(c) && FramepaC_readtable->validString(obj,true))
	 {
	 obj = partial_object ;		// validString() modifies obj
	 object = FramepaC_readtable->readString(obj) ;
	 if (obj < partial_object+partial_count)
	    putbackChar(c) ;		// 'c' is part of the next object
	 valid_object = true ;
	 partial_count = 0 ;
	 return true ;			// we have a complete object now
	 }
      }
   if (in->eof() || in->fail())
      {
      // special case for end of file -- if the partial object we have so
      //   far is valid, we'll assume it's complete
      const char *obj = partial_object ;
      partial_object[partial_count] = '\0' ;
      if (obj && FramepaC_readtable->validString(obj,false))
	 {
	 obj = partial_object ;	// validString() modifies obj
	 object = FramepaC_readtable->readString(obj) ;
	 valid_object = true ;
	 partial_count = 0 ;
	 return true ;
	 }
      }
   return false ;			// the object is not yet complete
}
#endif /* FrUSING_SOCKETS */

//----------------------------------------------------------------------

bool FrNetworkConnection::continuePriorCall(FrNetworkServer *server)
{
   if (continue_func)
      {
      bool again = continue_func(server,continue_data) ;
      if (again)
	 return true ;
      else
	 {
	 continue_func = 0 ;
	 continue_data = 0 ;
	 }
      }
   return false ;
}

/************************************************************************/
/*	Methods for class FrNetworkServer				*/
/************************************************************************/

FrNetworkServer::FrNetworkServer(int portnum, size_t maxconn)
{
   if (maxconn == 0)
      maxconn = MAX_CONNECTIONS ;
   port_number = portnum ;
   num_connections = 0 ;
   max_connections = 0 ;
   curr_connection = 0 ;
   events = new FrEventList ;
   shutdown_requested = false ;
   unicode = false ;
   unicode_bswap = false ;
   EUC = false ;
   want_canonical = false ;
   reading_objects = false ;
   sockets = FrNewN(FrSocket,maxconn+1) ;
   connections = FrNewC(FrNetworkConnection*,maxconn+1) ;
   if (sockets && connections)
      {
      max_connections = maxconn ;
      for (size_t s = 0 ; s <= maxconn ; s++)
	 sockets[s] = -1 ;
      }
   return ;
}

//----------------------------------------------------------------------

FrNetworkServer::~FrNetworkServer()
{
   shutdown() ;
   FrFree(sockets) ;		sockets = 0 ;
   FrFree(connections) ;	connections = 0 ;
   delete events ;		events = 0 ;
   num_connections = 0 ;
   max_connections = 0 ;
   return ;
}

//----------------------------------------------------------------------

bool FrNetworkServer::haveContinuations()
{
   for (size_t i = 0 ; i < max_connections ; i++)
      if (connections[i] && connections[i]->continued())
	 return true ;
   return false ;
}

//----------------------------------------------------------------------

bool FrNetworkServer::processInput()
{
   bool data_available = false ;
   size_t prev_connection = curr_connection ;
   do {
      // scan all active connections for incoming data
      if (connections[curr_connection])
	 {
	 FrNetworkConnection *conn = connections[curr_connection] ;
	 FrISockStream *in = conn->inSocket() ;
	 FrOSockStream *out = conn->outSocket() ;
	 if (in->good() && in->inputAvailable() > 0)
	    data_available = true ;
	 int pending = 0 ;
	 if (!in->good() || !out->good() || in->connectionDied() ||
	     out->connectionDied())
	    {
	    // the connection is finished, so close it and free up resources
	    disconnect(curr_connection) ;
	    }
	 else if ((pending = out->outputPending()) != 0)
	    {
	    // we're still waiting to complete the write of the last command's
	    //  reply, so don't process further commands from this client yet,
	    //  or we've lost the connection, in which was we need to disconn.
	    if (pending == EOF)
	       disconnect(curr_connection) ;
	    }
	 else if (conn->continuePriorCall(this))
	    data_available = true ;
	 else
	    {
	    bool disconnected = false ;
	    if (reading_objects)
	       {
	       while (connections[curr_connection] && conn->getPartialObject())
		  {
		  if (!onObjectReceived(curr_connection,
					conn->receivedObject()) ||
		      in->eof() || in->fail())
		     {
		     // server terminated conn, so close it
		     disconnect(curr_connection) ;
		     disconnected = true ;
		     break ;
		     }
		  else if (conn->continued())
		     break ;
		  }
	       }
	    else // (!reading_objects)
	       {
	       while (!in->eof() && !in->fail() &&
		      connections[curr_connection] &&
		      conn->getPartialLine())
		  {
		  if (!onLineReceived(curr_connection,conn->canonicalLine()))
		     {
		     // server terminated conn, so close it and free up rsrces
		     disconnect(curr_connection) ;
		     disconnected = true ;
		     break ;
		     }
		  else if (conn->continued())
		     break ;
		  }
	       }
	    conn = connections[curr_connection] ;
	    in = conn ? conn->inSocket() : 0 ;
	    if (!disconnected && in && (in->eof() || in->fail()))
	       {
	       // the connection is finished, so close it and free up resources
	       disconnect(curr_connection) ;
	       }
	    }
	 }
      // advance to the next connection
      curr_connection++ ;
      if (curr_connection >= max_connections)
	 curr_connection = 0 ;
   } while (curr_connection != prev_connection) ;
   return data_available ;
}

//----------------------------------------------------------------------

bool FrNetworkServer::run(clock_t timeout)
{
   if (!good())
      return true ;			// can't run, so indicate shutdown...
#ifdef FrUSING_SOCKETS
   bool timed_out = false ;
   if (timeout == 0)
      timeout = INT_MAX ;
   bool working ;
   bool data_available = true ;
   do {
      // run any background tasks
      working = onBackgroundWork(!data_available) ;
      // if we are not yet at capacity, see whether anyone else wants to talk
      if (num_connections < max_connections)
	 {
	 int conn_timeout = (num_connections > 0 || working) ? 0 : timeout ;
	 if (conn_timeout > 1 && events->haveEvents())
	    conn_timeout = 1 ;
	 FrSocket sock = FrAwaitConnection(port_number,conn_timeout,cerr) ;
	 if (sock > 0)
	    {
	    sockets[max_connections] = FrListeningSocket() ;
	    FrNetworkConnection *conn = new FrNetworkConnection(sock) ;
	    if (conn && conn->inSocketGood())
	       {
	       if (unicode)
		  conn->setUnicode() ;
	       if (EUC)
		  conn->setEUC() ;
	       if (want_canonical)
		  conn->wantCanonical() ;
	       num_connections++ ;
	       for (size_t i = 0 ; i < max_connections ; i++)
		  {
		  if (connections[i] == 0)
		     {
		     connections[i] = conn ;
		     sockets[i] = sock ;
		     onConnect(i) ;
		     break ;
		     }
		  }
	       }
	    else
	       {
	       delete conn ;
	       FrDisconnectPort(sock) ;
	       }
	    }
	 else if (num_connections == 0 && timeout != INT_MAX)
	    timed_out = true ;
	 }
      // now that we've checked for new connections, see whether there is any
      // data available on the existing connections
      bool cont = haveContinuations() ;
      int data_timeout_microsec = 0 ;
      int data_timeout = -1 ;
      if (working)
	 data_timeout = 0 ;
      else if (cont)
	 {
	 // force the OS scheduler to at least briefly switch to another task
	 //   if any is runnable
	 data_timeout = 0 ;
	 data_timeout_microsec = 1000 ;
	 }
      if (data_timeout == -1 && events->haveEvents())
	 data_timeout = 1 ;
      int active = 0 ;
      if (num_connections > 0)
	 active = FrAwaitActivity(sockets,max_connections+1,data_timeout,
				  data_timeout_microsec) ;
      if (active > 0 || cont)
	 data_available = processInput() ;
      else
	 data_available = false ;
      if (events->haveEvents())
	 events->executeEvents() ;
      } while ((working || data_available) &&
	       !shutdown_requested && !timed_out) ;
   return timed_out||shutdown_requested ;// indicate whether we went too long
					// without getting any connections
#else
   (void)timeout ;
   return false ;
#endif /* FrUSING_SOCKETS */
}

//----------------------------------------------------------------------

bool FrNetworkServer::disconnect(size_t conn)
{
   (void)conn ;
#ifdef FrUSING_SOCKETS
   if (conn < max_connections && connections[conn] != 0)
      {
      FrNetworkConnection *connection = connections[conn] ;
      FrISockStream *in = connection->inSocket() ;
      onDisconnect(conn) ;
      connections[conn] = 0 ;
      sockets[conn] = -1 ;
      num_connections-- ;
      FrDisconnectPort(in->socketNumber()) ;
      delete connection ;
      return true ;
      }
   else
#endif /* FrUSING_SOCKETS */
      return false ;			// can't disconnect that one!
}

//----------------------------------------------------------------------

bool FrNetworkServer::shutdown()
{
   bool success = true ;
   for (size_t i = 0 ; i < num_connections ; i++)
      {
      if (connections[i])
	 if (!disconnect(i))
	    success = false ;
      }
   if (sockets[max_connections] != (FrSocket)INVALID_SOCKET)
      (void)close_socket(sockets[max_connections]) ;
   FrCloseListener() ;
   return success ;
}

//----------------------------------------------------------------------

void FrNetworkServer::requestShutdown()
{
   shutdown_requested = true ;
   return ;
}

//----------------------------------------------------------------------

void FrNetworkServer::setUnicode(bool uni, bool uni_bswap)
{
   unicode = uni ;
   unicode_bswap = uni_bswap ;
   if (uni)
      EUC = false ;
   return ;
}

//----------------------------------------------------------------------

void FrNetworkServer::setEUC(bool euc)
{
   EUC = euc ;
   if (euc)
      {
      unicode = false ;
      unicode_bswap = false ;
      }
   return ;
}

//----------------------------------------------------------------------

void FrNetworkServer::setContinuation(FrNetworkContinueFunc *func,
				      void *data)
{
   if (connections && connections[curr_connection])
      connections[curr_connection]->setContinuation(func,data) ;
   return ;
}

//----------------------------------------------------------------------

static time_t event_callback(void *user_data)
{
   FrNetworkServerEvent *event = (FrNetworkServerEvent*)user_data ;
   time_t next_time = 0 ;
   if (event)
      {
      FrNetworkServer *server = event->server() ;
      size_t interval = event->repeatInterval() ;
      if (server && !server->onTimedEvent(event->userData()))
	 interval = 0 ;			// further repetition was cancelled
      if (interval > 0)
	 {
	 next_time = event->scheduledTime() + interval ;
	 time_t curr_time = time(0) ;
	 if (next_time <= curr_time)
	    next_time = curr_time + interval ;
	 event->scheduledTime(next_time) ;
	 }
      }
   return next_time ;
}

//----------------------------------------------------------------------

static void event_cleanup(void *user_data)
{
   FrNetworkServerEvent *event = (FrNetworkServerEvent*)user_data ;
   if (event)
      delete event ;
   return ;
}

//----------------------------------------------------------------------

FrEvent *FrNetworkServer::addEvent(time_t first_time, bool relative_time,
			       void *user_data, size_t interval)
{
   if (relative_time)
      first_time += time(0) ;
   FrNetworkServerEvent *event_data ;
   event_data = new FrNetworkServerEvent(first_time,interval,this,user_data) ;
   FrEvent *ev = events->addEvent(first_time,event_callback,event_data,false,
				  event_cleanup) ;
   if (ev)
      event_data->setEvent(ev) ;
   return ev ;
}

//----------------------------------------------------------------------

bool FrNetworkServer::setRepeatInterval(FrEvent *event, size_t interval)const
{
   if (events && event)
      {
      FrNetworkServerEvent *ev ;
      ev = (FrNetworkServerEvent*)events->getUserData(event) ;
      if (ev)
	 {
	 ev->repeatInterval(interval) ;
	 return true ;
	 }
      }
   return false ;
}

//----------------------------------------------------------------------

FrISockStream *FrNetworkServer::inputStream(size_t conn_num) const
{
   (void)conn_num ;
#ifdef FrUSING_SOCKETS
   if (conn_num < max_connections && connections && connections[conn_num])
      return connections[conn_num]->inSocket() ;
   else
#endif /* FrUSING_SOCKETS */
      return 0 ;
}

//----------------------------------------------------------------------

FrOSockStream *FrNetworkServer::outputStream(size_t conn_num) const
{
   (void)conn_num ;
#ifdef FrUSING_SOCKETS
   if (conn_num < max_connections && connections && connections[conn_num])
      return connections[conn_num]->outSocket() ;
   else
#endif /* FrUSING_SOCKETS */
      return 0 ;
}

//----------------------------------------------------------------------

bool FrNetworkServer::good() const
{
#ifdef FrUSING_SOCKETS
   return connections && sockets ;
#else
   return false ;
#endif /* FrUSING_SOCKETS */
}

//----------------------------------------------------------------------

void FrNetworkServer::onConnect(size_t)
{
   cerr << "Got a new connection (" << num_connections << " active)" << endl ;
   return ;
}

//----------------------------------------------------------------------

void FrNetworkServer::onDisconnect(size_t conn)
{
   size_t num_conn = num_connections ? num_connections-1 : 0 ;
   cerr << "Connection " << conn << " terminated (" << num_conn
	 << " still active)" << endl ;
   return ;
}

//----------------------------------------------------------------------

// some gymnastics to keep the compiler from complaining about adding the
//  "noreturn" attribute to onLineReceived and onObjectReceived methods,
//  because we can't actually add the attribute since the properly-overridden
//  versions WON'T be "noreturn"
static void did_not_override(const char *what) _fnattr_noreturn ;

static void did_not_override(const char *what)
{
   FrProgErrorVA("used FrNetworkServer subclass in line mode without\n"
		 "overriding %s",what) ;
}

//----------------------------------------------------------------------

bool FrNetworkServer::onLineReceived(size_t, const char *)
{
   if (this) did_not_override("onLineReceived") ;
   return false ;
}

//----------------------------------------------------------------------

bool FrNetworkServer::onObjectReceived(size_t, FrObject *)
{
   if (this) did_not_override("onObjectReceived") ;
   return false ;
}

//----------------------------------------------------------------------

bool FrNetworkServer::onBackgroundWork(bool /*idle*/)
{
   return false ;
}

//----------------------------------------------------------------------

bool FrNetworkServer::onTimedEvent(void *)
{
   return false ;
}

//----------------------------------------------------------------------

void FrNetworkServer::onEventDone(void *)
{
   return ;
}

// end of file frnetsrv.cpp //

