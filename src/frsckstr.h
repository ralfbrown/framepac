/****************************** -*- C++ -*- *****************************/
/*									*/
/*  Socket I/O								*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frsckstr.h		socket-based standard I/O stream	*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1996,1997,1998,1999,2000,2001,2003,2006,2009,2010,	*/
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

#ifndef __FRSCKSTR_H_INCLUDED
#define __FRSCKSTR_H_INCLUDED

#ifndef __FROBJECT_H_INCLUDED
#include "frobject.h"
#endif

#if !defined(__WATCOMC__) && !defined(_MSC_VER)
#if __GNUC__ >= 3
#  include <streambuf>
using namespace std ;
#else
#  include <streambuf.h>
#endif /* __GNUC__ > 3 */
#endif /* !__WATCOMC__ && !_MSC_VER */

#if defined(__GNUC__)
#  pragma interface
#endif

/************************************************************************/
/************************************************************************/

#if defined(__WINDOWS__) || defined(__NT__)

// use a smaller buffer under Windows so that we can run the message dispatch
// loop more frequently
#define FrSOCKSTREAM_BUFSIZE 1024

#define close_socket closesocket
#define read_socket(s,buf,len) ::recv(s,buf,len,0)
#define write_socket(s,buf,len) ::send(s,buf,len,0)
#define ioctl_socket ioctlsocket

typedef int socklen_t ;

#else /* !__WINDOWS__ && !__NT__ */

// use a larger buffer since we don't need to run a message loop
#define FrSOCKSTREAM_BUFSIZE 8192

typedef int FrSocket ;
#define SOCKET_ERROR (-1)

#define read_socket  ::read
#define write_socket ::write
#define close_socket ::close
#define ioctl_socket ::ioctl

#if !defined(__linux__) && !defined(__CYGWIN__)
typedef int socklen_t ;
#endif /* !__linux__ */

#endif /* __WINDOWS__ || __NT__ */

#if defined(unix) || defined(__linux__) || defined(__GNUC__) || defined(__WINDOWS__) || defined(__NT__) || defined(_WIN32)
#  define FrBSD_SOCKETS
#endif

#ifndef INVALID_SOCKET
# define INVALID_SOCKET (-1)
#endif

/************************************************************************/
/************************************************************************/

typedef int FrSocket ;

//struct SSL ;
#include "openssl/ossl_typ.h"

#if !defined(__GNUC__) || __GNUC__ < 3
#  define int_type int
#endif /* __GNUC__ >= 3 */

/************************************************************************/
/************************************************************************/

/* the socket classes were patterned after code posted to Usenet
    by David Morse <morse@pobox.com> */

//----------------------------------------------------------------------

class FrSockStreamBuf : public streambuf
   {
   private:
      char  *buffer ;
      char  *putback_buffer ;
      SSL   *SSL_handle ;
      size_t buffer_size ;
      size_t putback_size ;
      size_t big_putback ;
      FrSocket socket ;
      bool nonblocking_writes ;
   private:
      void init() ;
      bool expandBufferBy(size_t extra_bytes) ;
   protected:
      virtual int sync() ;		// force buffered output to socket
      virtual int_type overflow(int_type c) ; //do this when output buffer full
      virtual int_type underflow() ;	// what to do when more input needed
      virtual int_type uflow() ;	// ditto, return next char of input
      virtual int_type pbackfail(int_type c) ; //do this when putback area full
   public:
      FrSockStreamBuf() ;
      FrSockStreamBuf(FrSocket s, char *&buf, size_t size,
			bool use_SSL = false) ;
      void setsocket(FrSocket s, char *&buf, size_t size,
		     bool use_SSL = false) ;
      virtual ~FrSockStreamBuf() ;

      // manipulators
      bool fillBuffer() ;
      int writeBuffer() ;
      void useNonBlockingWrites(bool nbwrites = true)
         { nonblocking_writes = nbwrites ; }

      // accessors
      FrSocket socketNumber() const { return socket ; }
      bool usingNonBlockingWrites() const { return nonblocking_writes ; }
      int connectionDied() const ;	// check if socket still alive
      int outputPossible() ;		// bytes writeable without blocking
      int outputPending() ;		// bytes still buffered
   } ;

//----------------------------------------------------------------------

class FrSockStreamBase : public virtual ios
   {
   private:
      FrSockStreamBuf buf ;
   protected:
      void setsocket(FrSocket s, char *&buff, size_t size,
		     bool use_SSL = false)
	 { buf.setsocket(s,buff,size,use_SSL) ; clear() ; }
      FrSockStreamBase() ;
   public:
      FrSockStreamBase(FrSocket s, char *&bufptr, size_t size,
		       bool use_SSL = false) ;
      streambuf *rdbuf() { return &buf ; }

      // manipulators
      bool fillBuffer() { return buf.fillBuffer() ; }
      void useNonBlockingWrites(bool nbw = true)
         { buf.useNonBlockingWrites(nbw) ; }

      // accessors
      FrSocket socketNumber() const { return buf.socketNumber() ; }
      int connectionDied() const { return buf.connectionDied() ; }
      bool usingNonBlockingWrites() const
         { return buf.usingNonBlockingWrites() ; }
      int outputPending() { return buf.outputPending() ; }
   } ;

//----------------------------------------------------------------------

class FrOSockStream : public ostream, public /*virtual*/ FrSockStreamBase,
		      public virtual FrObject
   {
   protected:
      char *buffer ;
      void setsocket(FrSocket s) ;
   public:
      FrOSockStream(FrSocket s = -1, bool use_SSL = false) ;
      FrOSockStream(const char *hostname, int port, bool use_SSL = false) ;
      virtual FrObjectType objType() const ;
      virtual const char *objTypeName() const ;
      virtual FrObjectType objSuperclass() const ;
      virtual ostream &printValue(ostream &output) const ;
      virtual char *displayValue(char *buffer) const ;
      virtual size_t displayLength() const ;
      virtual FrObject *copy() const ;
      virtual FrObject *deepcopy() const ;

      // manipulators
      void useNonBlockingWrites(bool nbw = true)
            { FrSockStreamBase::useNonBlockingWrites(nbw) ; }

      // accessors
      FrSocket socketNumber() const
	    { return FrSockStreamBase::socketNumber(); }
      bool usingNonBlockingWrites() const
            { return FrSockStreamBase::usingNonBlockingWrites() ; }
      int connectionDied() ;
      int outputPending() ;
   } ;

//----------------------------------------------------------------------

class FrISockStream : public istream, public virtual FrObject
   {
   protected: // data
      FrSockStreamBuf buf ;
      char *buffer ;
      int listen_port ;
      int listening ;
   protected: // methods
      void setsocket(FrSocket s) ;
      bool fillBuffer() { return buf.fillBuffer() ; }
   public:
      FrISockStream(FrSocket s = (FrSocket)INVALID_SOCKET,
		    bool use_SSL = false) ;
      FrISockStream(FrSocket s, int port, bool use_SSL = false) ;
      FrISockStream(const char *hostname, int port) ;
      ~FrISockStream() ;
      virtual FrObjectType objType() const ;
      virtual const char *objTypeName() const ;
      virtual FrObjectType objSuperclass() const ;
      virtual ostream &printValue(ostream &output) const ;
      virtual char *displayValue(char *_buffer) const ;
      virtual size_t displayLength() const ;
      virtual FrObject *copy() const ;
      virtual FrObject *deepcopy() const ;

      // accessors
      FrSocket socketNumber() const { return buf.socketNumber(); }
      FrSocket awaitConnection(int timeout, ostream &err, ostream *out = 0) ;
      int connectionDied() ;
      int inputAvailable() ;
      bool isListening() const { return (bool)listening ; }

      // modifiers
      void reset() ;
   } ;

//----------------------------------------------------------------------

class FrSockStream : public FrISockStream, public FrOSockStream
   {
   public:
      FrSockStream(FrSocket s = 0) ;
      FrSockStream(const char *hostname, int port) ;
      virtual FrObjectType objType() const ;
      virtual const char *objTypeName() const ;
      virtual FrObjectType objSuperclass() const ;
      virtual ostream &printValue(ostream &output) const ;
      virtual char *displayValue(char *buffer) const ;
      virtual size_t displayLength() const ;
      virtual FrObject *copy() const ;
      virtual FrObject *deepcopy() const ;

      // manipulators
      void useNonBlockingWrites(bool nbw = true)
         { FrOSockStream::useNonBlockingWrites(nbw) ; }

      // accessors
      FrSocket socketNumber() const { return FrISockStream::socketNumber() ; }
      int connectionDied() { return FrISockStream::connectionDied() ; }
      int inputAvailable() { return FrISockStream::inputAvailable() ; }
      int outputPending() { return FrOSockStream::outputPending() ; }
      bool usingNonBlockingWrites() const
         { return FrOSockStream::usingNonBlockingWrites() ; }
   } ;

//----------------------------------------------------------------------

bool FrSSLAvailable() ;

bool FrAwaitConnection(int port_number, istream *&in, ostream *&out,
			 ostream *&err) ;
FrSocket FrAwaitConnection(int port_number, int timeout, ostream &err,
			   bool verbose = true,
			   bool terminate_on_error = true) ;
bool FrInitiateConnection(const char *host, int port_number,
			    istream *&in, ostream *&out, ostream *&err) ;
FrSocket FrListeningSocket() ;
bool FrCloseListener() ;

void FrCloseConnection(istream *&in, ostream *&out, ostream *&err) ;

FrSocket FrConnectToPort(const char *hostname, int port_number) ;
int FrDisconnectPort(FrSocket s) ;

int FrInputAvailable(FrSocket s) ;
int FrAwaitActivity(FrSocket *fds, int numfds, int timeout_sec,
                    int timeout_microsec = 0) ;

const char *FrGetPeerName(FrSocket s) ;

#endif /* !__FRSCKSTR_H_INCLUDED */

// end of file frsckstr.h //
