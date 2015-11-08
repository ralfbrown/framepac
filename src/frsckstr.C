/****************************** -*- C++ -*- *****************************/
/*									*/
/*  Socket I/O								*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frsckstr.cpp		socket-based standard I/O streams	*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1996,1997,1998,1999,2000,2001,2002,2003,2004,2006,	*/
/*		2009,2010,2011,2012,2015				*/
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
#  pragma implementation "frsckstr.h"
#  define _GLIBCPP_DEPRECATED	// make GCC 3.x compatible with rest of world
#endif

#include <errno.h>
#include <signal.h>
#include <stdio.h>  // needed for EOF on some systems
#include <stdlib.h>
#if defined(__WINDOWS__) || defined(__NT__) || defined(_WIN32)
#include <winsock.h>
#endif /* __WINDOWS__ || __NT__ */
#include "frsckstr.h"
#include "frmswin.h"
#include "frsignal.h"

#if defined(unix) || defined(__linux__) || defined(__GNUC__)
#include <fcntl.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/time.h>
#if defined(__SUNOS__) || defined(__SOLARIS__)
#include <string.h>
#include <sys/ioctl.h>
#include <sys/signal.h>
#  if defined(__SOLARIS__)
#  include <sys/filio.h>	 // for FIONBIO
#  endif /* __SOLARIS__ */
#elif defined(__linux__)
#include <string.h>
#include <sys/ioctl.h>
#elif defined(__alpha__)
#include <string.h>
#include <sys/ioctl.h>
#elif defined(__CYGWIN__)
#include <sys/ioctl.h>
#else
#include <ioctl.h>
#endif /* __SUNOS__ || __SOLARIS__ */
#endif /* unix */

#ifdef __WATCOMC__
#include <io.h>
#include <time.h>
#endif /* __WATCOMC__ */

#ifdef _MSC_VER
#include <io.h>

# ifndef dup
#  define dup _dup
# endif
#endif /* _MSC_VER */

#if __GNUC__ >= 3
#  define seekoff pubseekoff
#  define base pbase
#  define out_waiting() (pptr() - pbase())
#endif /* __GNUC__ */

/************************************************************************/
/*	Manifest Constants						*/
/************************************************************************/

#define LOCALHOSTNAME_SIZE 250

#define PUTBACK_SIZE 4

/************************************************************************/
/*	Global data for this module					*/
/************************************************************************/

int FrAwaitActivity(FrSocket fd, int timeout_microsec) ;

extern void (*FrShutdown)() ;

static const char str_FrSocket_intro[] = "#<FrSocket " ;
static const char str_FrSocket_end[] = ">" ;
static const char str_colonIN[] = " :IN" ;
static const char str_colonOUT[] = " :OUT" ;
static const char str_colonIO[] = " :IO" ;

#ifdef FrUSE_SSL
static SSL_CTX *ssl_context = 0 ;
#endif /* FrUSE_SSL */

/************************************************************************/
/*    Exception Handling						*/
/************************************************************************/

#ifdef SIGPIPE
static FrSignalHandler *sigpipe ;
#endif /* SIGPIPE */
#ifdef FrBSD_SOCKETS
static int sigpipe_ignored = false ;
#endif /* FrBSD_SOCKETS */

//----------------------------------------------------------------------

#ifdef FrBSD_SOCKETS
static void ignore_sigpipe()
{
   if (!sigpipe_ignored)
      {
#ifdef SIGPIPE
      sigpipe = new FrSignalHandler(SIGPIPE,(FrSignalHandlerFunc*)SIG_IGN) ;
#endif /* SIGPIPE */
      sigpipe_ignored = true ;
      }
   return ;
}
#endif /* FrBSD_SOCKETS */

//----------------------------------------------------------------------

#ifdef FrBSD_SOCKETS
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
#endif /* FrBSD_SOCKETS */

/************************************************************************/
/*    Utility functions							*/
/************************************************************************/

#ifdef FrBSD_SOCKETS
static int safe_select(fd_set *fdset, int timeout_sec, int timeout_microsec,
		       fd_set *readset, fd_set *writeset, fd_set *errset)
{
   struct timeval t ;

   if (timeout_sec < 0)
      timeout_sec = 18*3600 ;		// wait eighteen hours
   else if (timeout_sec > SHRT_MAX)
      timeout_sec = SHRT_MAX ;
   int result ;
   do {
      if (readset)
	 *readset = *fdset ;
      if (writeset)
	 *writeset = *fdset ;
      if (errset)
	 *errset = *fdset ;
      FrMessageLoop() ;
#if defined(__WINDOWS__) || defined(__NT__)
      t.tv_sec = (timeout_sec > 0) ? 1 : 0 ;
#else
      t.tv_sec = timeout_sec ;
#endif /* __WINDOWS__ || __NT__ */
      int tv_sec_orig = t.tv_sec ;
      t.tv_usec = timeout_microsec ;
      result = select(FD_SETSIZE,readset,writeset,errset,
		      (t.tv_sec >= 0) ? &t : 0) ;
      if (result < 0 && t.tv_sec > 1) // in case we are interrupted, don't
	t.tv_sec -= 1 ;		      //    wait as long on the retry
      if (t.tv_sec == 0)
	 timeout_sec -= tv_sec_orig ;
      else if (timeout_sec >= t.tv_sec)
	 timeout_sec -= t.tv_sec ;
      else
	 timeout_sec-- ;
      } while (result <= 0 && timeout_sec > 0) ; // retry if syscall aborted
   return result ;
}
#endif /* FrBSD_SOCKETS */

//----------------------------------------------------------------------

static void check_socket(FrSocket socket, bool *can_read, bool *can_write,
			 bool *have_error, int timeout_microsec = 0)
{
#ifdef FrBSD_SOCKETS
   struct timeval t ;
   t.tv_sec = timeout_microsec / 1000000 ;
   t.tv_usec = timeout_microsec % 1000000 ;
   int result ;
   fd_set fdset ;
   FD_ZERO(&fdset) ;
   FD_SET(socket,&fdset) ;
   fd_set readset, writeset, errset ;
   do {
      readset = fdset ;
      writeset = fdset ;
      errset = fdset ;
      FrMessageLoop() ;
      result = select(FD_SETSIZE,can_read ? &readset : 0,
		      can_write ? &writeset : 0,
		      have_error ? &errset : 0, &t) ;
      } while (result < 0) ;
   if (can_read)
      *can_read = (FD_ISSET(socket,&readset) != 0) ;
   if (can_write)
      *can_write = (FD_ISSET(socket,&writeset) != 0) ;
   if (have_error)
      *have_error = (FD_ISSET(socket,&errset) != 0) ;
#else
   (void)socket; (void)timeout_microsec ;
   if (can_read)
      *can_read = false ;
   if (can_write)
      *can_write = false ;
   if (have_error)
      *have_error = true ;
#endif /* FrBSD_SOCKETS */
   return ;
}

//----------------------------------------------------------------------

static void set_lingering(int sock, bool want_to_linger, int interval = 0)
{
#ifdef FrBSD_SOCKETS
   struct linger l ;
   l.l_onoff = want_to_linger ? 1 : 0 ;
   l.l_linger = (unsigned short)interval ;
   errno = 0 ;
   if (setsockopt(sock, SOL_SOCKET, SO_LINGER, (char*)&l, sizeof(l)) == -1 &&
       want_to_linger && errno != 0 && errno != EBADF
#ifdef ENOTSOCK
       && errno != ENOTSOCK
#endif /* ENOTSOCK */
#ifdef ENOPROTOOPT
       && errno != ENOPROTOOPT
#endif /* ENOPROTOOPT */
      )
      cerr << "Error " << errno
	   << " setting 'linger' option on socket (fd = " << sock << ") to "
	   << l.l_onoff << endl ;
#else
   (void)sock ; (void)want_to_linger ;
#endif /* FrBSD_SOCKETS */
   return ;
}

//----------------------------------------------------------------------

static void force_hard_disconnect(FrSocket sock)
{
   set_lingering(sock,true,0) ;
   return ;
}

//----------------------------------------------------------------------

static int close_socket_immediately(int sock)
{
   set_lingering(sock,false) ;		// return immed even if pending data
   int status = close_socket(sock) ;
   if (status == -1 && errno == EIO)
      cerr << "Error writing out final data while closing network connection."
	   << endl ;
   return status ;
}

//----------------------------------------------------------------------

#ifdef FrBSD_SOCKETS
static int bind_socket(FrSocket s, int port, int allow_reuse = 1)
{
   int reuse = allow_reuse ;
   int len = sizeof(reuse) ;
#if defined(__WINDOWS__) || defined(__NT__) || defined(__SOLARIS__)
   setsockopt(s,SOL_SOCKET,SO_REUSEADDR,(char*)&reuse,len) ;
#else
   setsockopt(s,SOL_SOCKET,SO_REUSEADDR,(void*)&reuse,len) ;
#endif /* __WINDOWS__ || __NT__ */
   force_hard_disconnect(s) ;
   struct sockaddr_in sa;
   memset(&sa,'\0',sizeof(sa)) ;
   sa.sin_port	 = htons((unsigned short)port) ;
   sa.sin_addr.s_addr = htonl(INADDR_ANY) ; // accept conns from anybody
   sa.sin_family	 = AF_INET ;
   return bind(s,(sockaddr*)&sa, sizeof(sa)) ;
}
#endif /* FrBSD_SOCKETS */

//----------------------------------------------------------------------

static FrSocket accept_connection(FrSocket s)
{
#ifdef FrBSD_SOCKETS
   sockaddr_in sa;
   socklen_t i = (socklen_t)sizeof(sa) ;
   FrSocket sock = (FrSocket)accept(s,(sockaddr *)&sa,&i) ;
   if (sock != (FrSocket)SOCKET_ERROR)
      force_hard_disconnect(sock) ;	// force hard disconnect on close
   else
      {
      cerr << "accept() error!" << endl ;
      sock = (FrSocket)INVALID_SOCKET ;
      }
#else
   (void)s ;
   FrSocket sock = (FrSocket)INVALID_SOCKET ;
#endif /* FrBSD_SOCKETS */
   return sock ;
}

//----------------------------------------------------------------------

#ifdef FrUSE_SSL
#if defined(__SUNOS__) && !defined(__SOLARIS__)
static void release_ssl_context(size_t,caddr_t)
#else
static void release_ssl_context()
#endif /* __SUNOS__ && !__SOLARIS__ */
{
   if (ssl_context)
      SSL_CTX_free(ssl_context) ;
   return ;
}
#endif /* FrUSE_SSL */

//----------------------------------------------------------------------

#ifdef FrUSE_SSL
static SSL *new_SSL_handle()
{
   if (!ssl_context)
      {
      // initalization; assume SSLeay v0.8 or higher
      SSLeay_add_ssl_algorithms() ;	// initialize the SSLeay library
      ssl_context = SSL_CTX_new(SSLv23_client_method()) ;
      if (!ssl_context)
	 return 0 ;
      SSL_CTX_set_options(ssl_context, SSL_OP_ALL) ;
      SSL_CTX_set_default_verify_paths(ssl_context) ;
#if defined(__SUNOS__) && !defined(__SOLARIS__)
      // SunOS 4.x has on_exit instead of the standard atexit()....
      on_exit(release_ssl_context,(caddr_t)0) ;
#else
      atexit(release_ssl_context) ;
#endif /* __SUNOS__ && !__SOLARIS__ */
      }
   return SSL_new(ssl_context) ;
}
#endif /* FrUSE_SSL */

//----------------------------------------------------------------------

#ifdef FrUSE_SSL
static int read_SSL_socket(FrSocket s, SSL *handle, void *buf, int size)
{
   return handle ? SSL_read(handle,buf,size) : read_socket(s,buf,size) ;
}
#else
#define read_SSL_socket(s,handle,buf,size) read_socket(s,buf,size)
#endif /* FrUSE_SSL */

//----------------------------------------------------------------------

#ifdef FrUSE_SSL
static int write_SSL_socket(FrSocket s, SSL *handle, void *buf, int size)
{
   return handle ? SSL_write(handle,buf,size) : write_socket(s,buf,size) ;
}
#else
#define write_SSL_socket(s,handle,buf,size) write_socket(s,buf,size)
#endif /* FrUSE_SSL */

//----------------------------------------------------------------------

#ifdef FrUSE_SSL
static void close_SSL_socket(FrSocket s, SSL *&handle)
{
   (void)close_socket(s) ;
   if (handle)
      {
      SSL_free(handle) ;
      handle = 0 ;
      }
   return ;
}
#else
#define close_SSL_socket(s,handle) close_socket(s)
#endif /* FrUSE_SSL */

//----------------------------------------------------------------------

#ifdef FrUSE_SSL
static void close_SSL_socket_immediately(FrSocket s, SSL *&handle)
{
   (void)close_socket_immediately(s) ;
   if (handle)
      {
      SSL_free(handle) ;
      handle = 0 ;
      }
   return ;
}
#else
#define close_SSL_socket_immediately(s,handle) close_socket_immediately(s)
#endif /* FrUSE_SSL */

/************************************************************************/
/*    Methods for class FrSockStreamBuf					*/
/************************************************************************/

FrSockStreamBuf::FrSockStreamBuf()
{
   init() ;
   return ;
}

//----------------------------------------------------------------------

FrSockStreamBuf::FrSockStreamBuf(FrSocket s, char*& buf, size_t size,
				 bool use_SSL)
{
   init() ;
   setsocket(s,buf,size,use_SSL) ;
   return ;
}

//----------------------------------------------------------------------

void FrSockStreamBuf::init()
{
   INIT_WINSOCK() ;
   socket = INVALID_SOCKET ;
   buffer = 0 ;
   buffer_size = 0 ;
   putback_size = 1 ;
   big_putback = 0 ;
   putback_buffer = 0 ;
   SSL_handle = 0 ;
   nonblocking_writes = false ;
   return ;
}

//----------------------------------------------------------------------

FrSockStreamBuf::~FrSockStreamBuf()
{
#ifdef FrBSD_SOCKETS
   if (socket != (FrSocket)INVALID_SOCKET)
      close_SSL_socket_immediately(socket,SSL_handle) ;
#endif /* FrBSD_SOCKETS */
   socket = (FrSocket)INVALID_SOCKET ;
   FrFree(buffer) ;
   buffer = 0 ;
   FrFree(putback_buffer) ;
   putback_buffer = 0 ;
   return ;
}

//----------------------------------------------------------------------

void FrSockStreamBuf::setsocket(FrSocket s,char *&buf,size_t size,
				bool use_SSL)
{
   INIT_WINSOCK() ;
   buf = 0 ;
   SSL_handle = 0 ;
   if (buffer && buffer_size != size)
      {
      FrFree(buffer) ;
      buffer = 0 ;
      }
   putback_size = PUTBACK_SIZE ;
   if (size < PUTBACK_SIZE + 1)
      size = PUTBACK_SIZE + 1 ;
   if (!buffer)
      buffer = FrNewN(char,size) ;
   buffer_size = size ;
   socket = s ;
   setbuf(buffer,size) ;
   int pb = putback_size ;
   setp(buffer+pb,buffer+buffer_size) ;
   setg(buffer,buffer+pb,buffer+pb) ;
#ifdef FrBSD_SOCKETS
   unsigned long nb = 1 ;		// set socket to non-blocking operation
   ioctl_socket(s,FIONBIO,&nb) ;
#endif /* FrBSD_SOCKETS */
   buf = buffer ;
#ifdef FrUSE_SSL
   if (use_SSL)
      {
      SSL_handle = new_SSL_handle() ;
      SSL_set_fd(SSL_handle,(int)s) ;
      //!!! lots more to be added here....
      }
#else
   (void)use_SSL ;
#endif /* FrUSE_SSL */
   return ;
}

//----------------------------------------------------------------------

int FrSockStreamBuf::sync()
{
   int len = out_waiting() ;
   if (len)
      {
      int status = writeBuffer() ;
      if (status == EOF)
	 return EOF ;
      else if (status == 0)
	 {
	 // unable to write to network and using nonblocking writes, so expand
	 //   the stream's buffer
	 (void)expandBufferBy(FrSOCKSTREAM_BUFSIZE) ;
	 }
      return 0 ;			// indicate success
      }
   else if (in_avail())
      {
      setg(eback(), base(), base()) ;
      setp(pbase(), pbase()) ;
      return 0 ;			// indicate success
      }
   // reset buffer pointers to initial values
   int pb = putback_size ;
   setp(buffer+pb,buffer+buffer_size) ;
   setg(buffer,buffer+pb,buffer+pb) ;
   FrMessageLoop() ;
   return 0 ;				// 0 means successful
}

//----------------------------------------------------------------------

int FrSockStreamBuf::overflow(int c)
{
   if (sync() != 0)
      return EOF ;
   if (c != EOF)
      sputc(c) ;
   return 1 ;				// wrote one character
}

//----------------------------------------------------------------------

bool FrSockStreamBuf::fillBuffer()
{
   bool allow_blocking = true ;	// first time through the loop, allow
   				//   a blocking read; subsequent times,
				//   we stop if any data has been got
   size_t total_len = 0 ;
   int pb = putback_size ;
   int len ;
   do {
      // All right, read the available data from the socket.  One big prob
      //   though -- the only thing guaranteed to make select() indicate
      //   data available for reading from a socket is the arrival of new
      //   data since the last call, even if there is pending unread data
      //   on that socket!  Thus, to allow programs to avoid blocking by
      //   trying to read the FrISockStream only when data is actually
      //   available, we need to retrieve ALL pending data from the socket
      //   and buffer it.  Since this may be more than we can fit into the
      //   buffer, expand the buffer as needed (fortunately, the network
      //   layer will allow only a limited amount of data to build up before
      //   forcing the other side to pause transmission).
      if (total_len + pb >= buffer_size)
	 (void)expandBufferBy(FrSOCKSTREAM_BUFSIZE) ;
      len = read_SSL_socket(socket,SSL_handle,buffer+pb+total_len,
			    buffer_size-pb-total_len);
      // we got burned by Win95 saying that the socket has data in
      // the select() call but then reneging on the actual read....
#if defined(__WINDOWS__) || defined(__NT__)
      if (len == SOCKET_ERROR)
	 {
	 if (WSAGetLastError() == WSAEWOULDBLOCK && allow_blocking)
	    {
	    (void)FrAwaitActivity(socket,500000) ;
	    continue ;
	    }
	 else
	    break ;
	 }
#else
      if (len < 0)
	 {
#if defined(unix) || defined(__linux__) || defined(__GNUC__)
	 if (errno == EWOULDBLOCK && allow_blocking)
	    {
	    (void)FrAwaitActivity(socket,2000000) ;
	    continue ;
	    }
	 else
#endif /* unix */
	    break ;
	 }
#endif /* __WINDOWS__ || __NT__ */
      total_len += len ;
      allow_blocking = false ;
      // update buffer pointers
      setg(buffer,buffer+pb,buffer+pb+total_len) ;
      setp(buffer+pb,buffer+pb) ;		// nothing to put
      } while (len > 0) ;
   return (total_len > 0) ;
}

//----------------------------------------------------------------------

int FrSockStreamBuf::writeBuffer()
{
   int len = out_waiting() ;
   if (len == 0)
      return 1 ;			// successfully wrote everything :-)
#ifdef FrBSD_SOCKETS
   char *buf = pbase() ;
   int wrote_data = 0 ;
   do {
      bool can_write, have_error ;
      check_socket(socket,0,&can_write,&have_error) ;
      if (have_error)
	 return EOF ;
      else if (can_write)
	 {
	 ignore_sigpipe() ;
	 int count = write_SSL_socket(socket,SSL_handle,buf,len) ;
	 restore_sigpipe() ;
#if defined(__WINDOWS__) || defined(__NT__)
	 // we got burned by Win95 saying that the socket is writable in
	 // the select() call but then reneging on the actual write....
	 if (count == SOCKET_ERROR)
	    {
	    if (WSAGetLastError() == WSAEWOULDBLOCK)
	       {
	       if (nonblocking_writes)
		  break ;
	       else
		  {
		  (void)FrAwaitActivity(socket,500000) ;
		  continue ;
		  }
	       }
	    else
	       return EOF ;
	    }
#else
	 if (count < 0)
	    {
#if defined(unix) || defined(__linux__) || defined(__GNUC__)
	    if (errno == EWOULDBLOCK)
	       {
	       if (nonblocking_writes)
		  break ;
	       else
		  {
		  (void)FrAwaitActivity(socket,2000000) ;
		  continue ;
		  }
	       }
	    else
#endif /* unix */
	       return EOF ;
	    }
#endif /* __WINDOWS__ || __NT__ */
	 len -= count ;
	 buf += count ;
	 wrote_data = 1 ;
	 }
      } while (len > 0) ;
   // update buffer pointers
   if (wrote_data)
      {
      // update the buffer pointers
      setg(buffer,buffer+putback_size,buffer+putback_size) ;
      if (len > 0)
	 {
	 // move the remaining data down to the start of the buffer
	 memmove(buffer+putback_size,buf,len) ;
	 // force both pbase() and pptr() to start of main buffer
	 setp(buffer+putback_size,buffer+buffer_size) ;
	 // then update pptr() to point past end of active data
	 pbump(len) ;
	 }
      else
	 setp(buffer+putback_size,buffer+buffer_size) ;
      }
#else
   int wrote_data = 1 ;			// just send it to the bit bucket....
   setg(buffer,buffer+putback_size,buffer+putback_size) ;
   setp(buffer+putback_size,buffer+buffer_size) ;
#endif /* FrBSD_SOCKETS */
   return wrote_data ;
}

//----------------------------------------------------------------------

int FrSockStreamBuf::underflow()
{
   FrMessageLoop() ;
   if (big_putback > 0 && putback_buffer)
      {
      char c = putback_buffer[big_putback--] ;
      if (big_putback == 0)
	 {
	 FrFree(putback_buffer) ;
	 putback_buffer = 0 ;
	 }
      return c ;
      }
   else if (in_avail())
      return (unsigned char)*gptr() ;
   int timeout = 0 ;
   for (;;)
      {
      bool can_read, have_error ;
      check_socket(socket,&can_read,0,&have_error,timeout) ;
      if (have_error)
	 return EOF ;
      else if (can_read)
	 {
	 if (!fillBuffer())
	    return EOF ;
	 break ;
	 }
      timeout = 100000 ;		// wait up to 100ms next time
      }
   FrMessageLoop() ;
   return (unsigned char)*gptr() ;
}

//----------------------------------------------------------------------

int FrSockStreamBuf::uflow()
{
   int c = underflow() ;
   if (c != EOF && gptr() < egptr())
#if 1
      gbump(1) ;			// skip the character
#else
      stossc() ;
#endif
   return c ;
}

//----------------------------------------------------------------------

int FrSockStreamBuf::pbackfail(int c)
{
   if (c != EOF)
      {
      char *endget = egptr() ;
      int left = endget - gptr() ;
      // do we have room in the streambuf's normal buffer to insert the char?
      if (left > 0)
	 {
	 char *getptr = gptr() ;
	 memmove(getptr,getptr+1,endget-getptr) ;
	 *getptr = (char)c ;
	 setg(eback(),gptr(),egptr()+1) ;
	 return c ;
	 }
      size_t count = in_avail() ;
      char *buf = FrNewR(char,putback_buffer,big_putback+count+1) ;
      if (buf)
	 {
	 putback_buffer = buf ;
	 if (big_putback > 0)
	    memmove(buf+count,buf,big_putback) ;
	 char *getptr = gptr() ;
	 for (size_t i = 0 ; i < count ; i++)
	    buf[count-i-1] = getptr[i] ;
	 buf[count+big_putback++] = (char)c ;
	 size_t pb = putback_size ;
	 setg(buffer,buffer+pb,buffer+pb) ;
	 setp(buffer+pb,buffer+pb) ;	// nothing to put
	 return c ;
	 }
      }
   return EOF ;				// sorry, can't putback that char....
}

//----------------------------------------------------------------------

int FrSockStreamBuf::connectionDied() const
{
   bool have_error ;
   check_socket(socket,0,0,&have_error) ;
   return have_error ;
}

//----------------------------------------------------------------------

int FrSockStreamBuf::outputPossible()
{
   if (connectionDied())
      return -1 ;
   int left = epptr() - pptr() ;
   if (left > 0)
      return left ;
   bool can_write, have_error ;
   check_socket(socket,0,&can_write,&have_error) ;
   if (have_error)
      return -1 ;
   else if (can_write)
      return 1 ;
   return 0 ;
}

//----------------------------------------------------------------------

int FrSockStreamBuf::outputPending()
{
   if (connectionDied())
      return EOF ;
   int pending = out_waiting() ;
   if (pending && nonblocking_writes)
      {
      if (writeBuffer() == EOF)
	 return EOF ;
      pending = out_waiting() ;
      }
   return pending ;
}

//----------------------------------------------------------------------

bool FrSockStreamBuf::expandBufferBy(size_t extra_bytes)
{
   if (extra_bytes)
      {
      char *newbuf = FrNewR(char,buffer,buffer_size+extra_bytes) ;
      if (newbuf)
	 {
	 // adjust the standard stream pointers, preserving any data currently
	 //   in the buffer
	 int out_pending = out_waiting() ;
	 int in_data = gptr() - buffer ;
	 buffer = newbuf ;
	 buffer_size += extra_bytes ;
	 setbuf(buffer,buffer_size) ;
	 setg(buffer,buffer+putback_size,buffer+buffer_size) ;
	 setp(buffer+putback_size,buffer+buffer_size) ;
	 pbump(out_pending) ;
	 gbump(in_data) ;
	 }
      else
	 return false ;			// unable to expand buffer
      }
   return true ;			// sucessfully expanded
}

/************************************************************************/
/*    Methods for class FrSockStreamBase				*/
/************************************************************************/

FrSockStreamBase::FrSockStreamBase() : buf()
{
   this->ios::init(&buf) ;
   return ;
}

//----------------------------------------------------------------------

FrSockStreamBase::FrSockStreamBase(FrSocket s, char *&bufptr, size_t size,
				   bool use_SSL)
   : buf(s, bufptr, size, use_SSL)
{
   this->ios::init(&buf) ;
   return ;
}

/************************************************************************/
/*    Methods for class FrOSockStream					*/
/************************************************************************/

FrOSockStream::FrOSockStream(FrSocket s, bool use_SSL) :
   ostream(FrSockStreamBase::rdbuf()),
   FrSockStreamBase(s, buffer, FrSOCKSTREAM_BUFSIZE, use_SSL)
{
   if (s == (FrSocket)INVALID_SOCKET)
      {
      ostream::clear(ios::badbit) ;
      buffer = 0 ;
      }
   else
      {
      if (buffer)
	 *buffer = 0 ;
      FrSockStreamBase::rdbuf()->seekoff(0, ios::beg, ios::out) ;
      }
   return ;
}

//----------------------------------------------------------------------

FrOSockStream::FrOSockStream(const char *hostname, int port, bool use_SSL) :
   ostream(FrSockStreamBase::rdbuf()),
   FrSockStreamBase()
{
   FrSocket s = FrConnectToPort(hostname,port) ;
   if (s != (FrSocket)INVALID_SOCKET)
      {
      FrSockStreamBase::setsocket(s,buffer,FrSOCKSTREAM_BUFSIZE,use_SSL) ;
      if (buffer)
	 *buffer = 0 ;
      FrSockStreamBase::rdbuf()->seekoff(0, ios::beg, ios::out) ;
      }
   else
      {
      ostream::clear(ios::badbit) ;
      buffer = 0 ;
      }
   return ;
}

//----------------------------------------------------------------------

FrObjectType FrOSockStream::objType() const
{
   return OT_FrOSockStream ;
}

//----------------------------------------------------------------------

const char *FrOSockStream::objTypeName() const
{
   return "FrOSockStream" ;
}

//----------------------------------------------------------------------

FrObjectType FrOSockStream::objSuperclass() const
{
   return OT_FrObject;
}

//----------------------------------------------------------------------

ostream &FrOSockStream::printValue(ostream &output) const
{
   output << str_FrSocket_intro << socketNumber() << str_colonOUT << ""
	  << str_FrSocket_end ;
   return output ;
}

//----------------------------------------------------------------------

char *FrOSockStream::displayValue(char *bufptr) const
{
   int socknum = socketNumber() ;
   size_t len = strlen(str_FrSocket_intro) ;
   memcpy(bufptr,str_FrSocket_intro,len) ;
   bufptr += len ;
   if (socknum < 0)
      {
      *bufptr++ = '-' ;
      *bufptr++ = '1' ;
      }
   else
      {
      ultoa(socknum,bufptr,10) ;
      bufptr = strchr(bufptr,'\0') ;
      }
   len = strlen(str_colonOUT) ;
   memcpy(bufptr,str_colonOUT,len) ;
   bufptr += len ;
   len = strlen(str_FrSocket_end) ;
   memcpy(bufptr,str_FrSocket_end,len+1) ;
   return bufptr + len ;
}

//----------------------------------------------------------------------

size_t FrOSockStream::displayLength() const
{
   int socknum = socketNumber() ;
   if (socknum < 0)
      return 17 ;
   else
      return 15 + Fr_number_length(socketNumber()) ;
}

//----------------------------------------------------------------------

FrObject *FrOSockStream::copy() const
{
   return new FrOSockStream(dup(socketNumber())) ;
}

//----------------------------------------------------------------------

FrObject *FrOSockStream::deepcopy() const
{
   return new FrOSockStream(dup(socketNumber())) ;
}

//----------------------------------------------------------------------

void FrOSockStream::setsocket(FrSocket s)
{
   FrSockStreamBase::setsocket(s,buffer,FrSOCKSTREAM_BUFSIZE) ;
   return ;
}

//----------------------------------------------------------------------

int FrOSockStream::connectionDied()
{
   int died = FrSockStreamBase::connectionDied() ;
   if (died)
      setstate(ios::eofbit) ;
   return died ;
}

//----------------------------------------------------------------------

int FrOSockStream::outputPending()
{
   int pending = FrSockStreamBase::outputPending() ;
   if (pending == EOF)
      setstate(ios::eofbit) ;
   return pending ;
}

/************************************************************************/
/*    Methods for class FrISockStream					*/
/************************************************************************/

FrISockStream::FrISockStream(FrSocket s, bool use_SSL) :
   istream(&buf),
   buf(s, buffer, FrSOCKSTREAM_BUFSIZE, use_SSL),
   listen_port(-1), listening(false)
{
   init(&buf) ;
   if (s == (FrSocket)INVALID_SOCKET)
      {
      this->istream::clear(ios::badbit) ;
      FrFree(buffer) ;
      buffer = 0 ;
      }
   else
      {
      if (buffer)
	 *buffer = 0 ;
      rdbuf()->seekoff(0, ios::beg, ios::in) ;
      }
   return ;
}

//----------------------------------------------------------------------

FrISockStream::FrISockStream(FrSocket s, int port, bool use_SSL) :
   istream(&buf),
   buf(s, buffer, 2, use_SSL)
{
   init(&buf) ;
   listen_port = port ;
   listening = false ;
#ifdef FrBSD_SOCKETS
   if (s == (FrSocket)INVALID_SOCKET)
      {
      s = (FrSocket)socket(PF_INET, SOCK_STREAM, 0) ;
      if (s != (FrSocket)SOCKET_ERROR)
	 setsocket(s) ;
      else
	 s = (FrSocket)INVALID_SOCKET ;
      }
#endif /* FrBSD_SOCKETS */
   if (s == (FrSocket)INVALID_SOCKET)
      {
      istream::clear(ios::badbit) ;
      FrFree(buffer) ;
      buffer = 0 ;
      }
   else
      {
      if (buffer)
	 *buffer = 0 ;
      rdbuf()->seekoff(0, ios::beg, ios::in) ;
#ifdef FrBSD_SOCKETS
      if (bind_socket(s,listen_port) < 0)
	 s = (FrSocket)INVALID_SOCKET ;
      setsocket(s) ;
#endif /* FrBSD_SOCKETS */
      }
   return ;
}

//----------------------------------------------------------------------

FrISockStream::FrISockStream(const char *hostname, int port) :
   istream(&buf), buf(), buffer(0), listening(false)
{
   init(&buf) ;
   listen_port = -1 ;
   FrSocket s = FrConnectToPort(hostname,port) ;
   if (s != (FrSocket)INVALID_SOCKET)
      {
      buf.setsocket(s,buffer,FrSOCKSTREAM_BUFSIZE) ;
      if (buffer)
	 *buffer = 0 ;
      rdbuf()->seekoff(0, ios::beg, ios::in) ;
      }
   else
      {
      istream::clear(ios::badbit) ;
      FrFree(buffer) ;
      buffer = 0 ;
      }
   return ;
}

//----------------------------------------------------------------------

FrISockStream::~FrISockStream()
{
   if (listening)
      {
      set_lingering(socketNumber(),false) ;
      }
   return ;
}

//----------------------------------------------------------------------

FrObjectType FrISockStream::objType() const
{
   return OT_FrISockStream ;
}

//----------------------------------------------------------------------

const char *FrISockStream::objTypeName() const
{
   return "FrISockStream" ;
}

//----------------------------------------------------------------------

FrObjectType FrISockStream::objSuperclass() const
{
   return OT_FrObject;
}

//----------------------------------------------------------------------

ostream &FrISockStream::printValue(ostream &output) const
{
   output << str_FrSocket_intro << socketNumber() << str_colonIN << ""
	  << str_FrSocket_end ;
   return output ;
}

//----------------------------------------------------------------------

char *FrISockStream::displayValue(char *outbuffer) const
{
   int socknum = socketNumber() ;
   size_t len = strlen(str_FrSocket_intro) ;
   memcpy(outbuffer,str_FrSocket_intro,len) ;
   outbuffer = strchr(outbuffer,'\0') ;
   if (socknum < 0)
      {
      *outbuffer++ = '-' ;
      *outbuffer++ = '1' ;
      }
   else
      {
      ultoa(socknum,outbuffer,10) ;
      outbuffer = strchr(outbuffer,'\0') ;
      }
   len = strlen(str_colonIN) ;
   memcpy(outbuffer,str_colonIN,len) ;
   outbuffer += len ;
   len = strlen(str_FrSocket_end) ;
   memcpy(outbuffer,str_FrSocket_end,len) ;
   return outbuffer + len ;
}

//----------------------------------------------------------------------

size_t FrISockStream::displayLength() const
{
   int socknum = socketNumber() ;
   if (socknum < 0)
      return 16 ;
   else
      return 14 + Fr_number_length(socketNumber()) ;
}

//----------------------------------------------------------------------

FrObject *FrISockStream::copy() const
{
   return new FrISockStream(dup(socketNumber())) ;
}

//----------------------------------------------------------------------

FrObject *FrISockStream::deepcopy() const
{
   return new FrISockStream(dup(socketNumber())) ;
}

//----------------------------------------------------------------------

void FrISockStream::reset()
{
   istream::rdbuf()->seekoff(0, ios::end, ios::in) ;
   return ;
}

//----------------------------------------------------------------------

void FrISockStream::setsocket(FrSocket s)
{
   buf.setsocket(s,buffer,FrSOCKSTREAM_BUFSIZE) ;
   return ;
}

//----------------------------------------------------------------------

FrSocket FrISockStream::awaitConnection(int timeout, ostream &err,
					ostream *outstream)
{
   INIT_WINSOCK() ;
   FrMessageLoop() ;
   FrSocket socket = socketNumber() ;
   if (socket == (FrSocket)INVALID_SOCKET || listen_port == -1)
      return (FrSocket)INVALID_SOCKET ;
#ifdef FrBSD_SOCKETS
   if (timeout < 0)
      timeout = 18*3600 ;		// wait up to eighteen hours
   if (timeout > 0 && out)
      *outstream << "; Awaiting network connection on port "
		 << listen_port << "." << flush << endl ;
   if (!listening)
      {
      if (listen(socket, 1) == SOCKET_ERROR) // allow only one connection
	 {				     //	  attempt at any time
	 return (FrSocket)INVALID_SOCKET ;
	 }
      else
	 listening = true ;
      }
   fd_set fdset ;
   FD_ZERO(&fdset) ;
   FD_SET(socket,&fdset) ;
   fd_set readset, errset ;
   int k = safe_select(&fdset,timeout,0,&readset,NULL,&errset) ;
   FrSocket sock = (FrSocket)INVALID_SOCKET ;
   if (k > 0 && FD_ISSET(socket,&errset))
      {
      setsocket((FrSocket)INVALID_SOCKET) ;
      listen_port = -1 ;
      err << "listen socket died!!!" << endl ;
      sock = (FrSocket)INVALID_SOCKET ;
      }
   else if (FD_ISSET(socket,&readset))
      {
      sock = accept_connection(socket) ;
      }
   else
      sock = (FrSocket)INVALID_SOCKET ;
   return sock ;
#else
   (void)timeout ; (void)err ; (void)outstream ;
   return (FrSocket)INVALID_SOCKET ;
#endif /* FrBSD_SOCKETS */
}

//----------------------------------------------------------------------

int FrISockStream::connectionDied()
{
   int died = buf.connectionDied() ;
   if (died)
      setstate(ios::eofbit) ;
   return died ;
}

//----------------------------------------------------------------------

int FrISockStream::inputAvailable()
{
   int count = istream::rdbuf()->in_avail() ;
   if (count > 0)
      return count ;
   bool can_read, have_error ;
   check_socket(socketNumber(),&can_read,0,&have_error) ;
   if (have_error)
      {
      setstate(ios::eofbit) ;
      return EOF ;
      }
   else if (can_read)
      {
      // because the select() used by check_socket() might not say yes
      //   again until even more data arrives on the socket, we have to
      //   retrieve and buffer *all* pending data right now.
      if (fillBuffer())
	 return istream::rdbuf()->in_avail() ;
      else
	 {
	 // if the select claimed to have data but none was read, we've hit EOF
	 setstate(ios::eofbit) ;
	 return EOF ;
	 }
      }
   return 0 ;			// no data available
}

/************************************************************************/
/*    Methods for class FrSockStream					*/
/************************************************************************/

FrSockStream::FrSockStream(FrSocket s) : FrISockStream(s), FrOSockStream(s)
{
   return ;
}

//----------------------------------------------------------------------

FrSockStream::FrSockStream(const char *hostname, int port)
{
   FrSocket s = FrConnectToPort(hostname,port) ;
   if (s != (FrSocket)INVALID_SOCKET)
      {
      FrISockStream::setsocket(s) ;
      FrOSockStream::setsocket(s) ;
      }
   else
      {
      FrISockStream::clear(ios::badbit) ;
      FrOSockStream::clear(ios::badbit) ;
      }
   return ;
}

//----------------------------------------------------------------------

FrObjectType FrSockStream::objType() const
{
   return OT_FrSockStream ;
}

//----------------------------------------------------------------------

const char *FrSockStream::objTypeName() const
{
   return "FrSockStream" ;
}

//----------------------------------------------------------------------

FrObjectType FrSockStream::objSuperclass() const
{
   return OT_FrObject;
}

//----------------------------------------------------------------------

ostream &FrSockStream::printValue(ostream &output) const
{
   output << str_FrSocket_intro << socketNumber() << str_colonIO << ""
	  << str_FrSocket_end ;
   return output ;
}

//----------------------------------------------------------------------

char *FrSockStream::displayValue(char *buffer) const
{
   int socknum = socketNumber() ;
   size_t len = strlen(str_FrSocket_intro) ;
   memcpy(buffer,str_FrSocket_intro,len) ;
   buffer += len ;
   if (socknum < 0)
      {
      *buffer++ = '-' ;
      *buffer++ = '1' ;
      }
   else
      {
      ultoa(socknum,buffer,10) ;
      buffer = strchr(buffer,'\0') ;
      }
   len = strlen(str_colonIO) ;
   memcpy(buffer,str_colonIO,len) ;
   buffer += len ;
   len = strlen(str_FrSocket_end) ;
   memcpy(buffer,str_FrSocket_end,len) ;
   return buffer + len ;
}

//----------------------------------------------------------------------

size_t FrSockStream::displayLength() const
{
   int socknum = socketNumber() ;
   if (socknum < 0)
      return 17 ;
   else
      return 15 + Fr_number_length(socketNumber()) ;
}

//----------------------------------------------------------------------

FrObject *FrSockStream::copy() const
{
   return new FrSockStream(dup(socketNumber())) ;
}

//----------------------------------------------------------------------

FrObject *FrSockStream::deepcopy() const
{
   return new FrSockStream(dup(socketNumber())) ;
}

/************************************************************************/
/************************************************************************/

static int listen_port = -1 ;
static FrSocket listen_socket = (FrSocket)INVALID_SOCKET ;

//----------------------------------------------------------------------

FrSocket FrListeningSocket()
{
   return listen_socket ;
}

//----------------------------------------------------------------------

bool FrCloseListener()
{
   if (listen_socket != (FrSocket)INVALID_SOCKET)
      {
      bool status = (close_socket_immediately(listen_socket) == 0) ;
      listen_socket = (FrSocket)INVALID_SOCKET ;
      listen_port = -1 ;
      return status ;
      }
   return true ;
}

//----------------------------------------------------------------------

#if defined(unix) || defined(__WINDOWS__) || defined(__NT__)
FrSocket FrAwaitConnection(int port_number, int timeout, ostream &err,
			   bool verbose, bool terminate_on_error)
{
   INIT_WINSOCK() ;
   FrMessageLoop() ;
   if (timeout < 0)
      timeout = 18 * 3600 ;		// wait up to 18 hours
   if (timeout > 1 && verbose)
      cout << "; Awaiting network connection on port " << port_number << "."
	   << flush << endl ;
   FrSocket s ;
   if (listen_port != port_number)
      {
      if (listen_socket != (FrSocket)INVALID_SOCKET)
	 {
	 close_socket_immediately(listen_socket) ;
	 listen_socket = (FrSocket)INVALID_SOCKET ;
	 listen_port = -1 ;
	 }
      if (port_number == -1)
	 return (FrSocket)INVALID_SOCKET ;
      s = (FrSocket)socket(PF_INET, SOCK_STREAM, 0) ;
      if (s == (FrSocket)SOCKET_ERROR)
	 {
	 if (verbose || terminate_on_error)
	    err << "Unable to create socket.  Bailing out...." << endl ;
	 if (terminate_on_error)
	    {
	    FrShutdown() ;
	    exit(1) ;
	    }
	 else
	    return (FrSocket)INVALID_SOCKET ;
	 }
      if (bind_socket(s,port_number) < 0)
	 {
	 (void)close_socket(s) ;
	 if (verbose || terminate_on_error)
	    err << "Unable to bind socket.	Bailing out...." << endl ;
	 if (terminate_on_error)
	    {
	    FrShutdown() ;
	    exit(1) ;
	    }
	 else
	    return (FrSocket)INVALID_SOCKET ;
	 }
      FrMessageLoop() ;
      listen_socket = s ;
      listen_port = port_number ;
      if (listen(s, 1) == SOCKET_ERROR) // allow only one connection attempt
	 {				//   at any time
	 (void)close_socket_immediately(s) ;
	 listen_socket = (FrSocket)INVALID_SOCKET ;
	 listen_port = -1 ;
	 return (FrSocket)INVALID_SOCKET ;
	 }
      }
   else
      s = listen_socket ;
   FrSocket sock = (FrSocket)INVALID_SOCKET ;
   do {
      fd_set fdset, readset, errset ;
      FD_ZERO(&fdset) ;
      FD_SET(s,&fdset) ;
      int k = safe_select(&fdset,timeout,0,&readset,NULL,&errset) ;
      if (k < 0)
	 {
	 // interrupted system call....
	 if (timeout > 1)
	    timeout-- ;
//	   cerr << "select() error!" << endl ;
	 }
      else if (FD_ISSET(s,&errset))
	 {
	 listen_socket = (FrSocket)INVALID_SOCKET ;
	 listen_port = -1 ;
	 cerr << "listen socket died!!!" << endl ;
	 return (FrSocket)INVALID_SOCKET ;
	 }
      else if (FD_ISSET(s,&readset))
	 {
	 sock = accept_connection(s) ;
	 }
      else if (timeout > 0)
	 timeout-- ;
      } while (sock == INVALID_SOCKET && timeout > 0) ;
   return sock ;
}
#else
FrSocket FrAwaitConnection(int, int, ostream &,bool,bool)
{
   return (FrSocket)INVALID_SOCKET ;
}
#endif /* unix || __WINDOWS__ || __NT__ */

//----------------------------------------------------------------------

bool FrAwaitConnection(int port_number, istream *&in, ostream *&out,
			 ostream *&err)
{
#ifdef FrBSD_SOCKETS
   FrISockStream *listener = new FrISockStream((FrSocket)INVALID_SOCKET,
					       port_number) ;
   FrSocket sock ;
   do {
      sock = listener->awaitConnection(-1, *err, out) ;
      if (!listener->isListening())
	 {
	 cerr << "Unable to create socket to listen on port " << port_number
	      << endl ;
	 delete listener ;
	 return false ;
	 }
      } while (sock == INVALID_SOCKET) ;
   delete listener ;
   force_hard_disconnect(sock) ;
   in = new FrISockStream(sock) ;
   out = new FrOSockStream(sock) ;
   err = out ;
   cout << "Connected." << endl ;
#else
   (void)port_number ; (void)in ; (void)out ; (void)err ;
#endif /* FrBSD_SOCKETS */
   return true ;
}

//----------------------------------------------------------------------

bool FrInitiateConnection(const char *host, int port_number,
			    istream *&in, ostream *&out, ostream *&err)
{
#ifdef FrBSD_SOCKETS
   FrOSockStream *stream = new FrOSockStream(host,port_number) ;
   if (stream && stream->good())
      {
      out = err = stream ;
      in = new FrISockStream(stream->socketNumber()) ;
      force_hard_disconnect(stream->socketNumber()) ;
      FrMessageLoop() ;
      return true ;
      }
#else
   (void)host ; (void)port_number ; (void)in ; (void)out ; (void)err ;
#endif /* FrBSD_SOCKETS */
   return false ;
}

//----------------------------------------------------------------------

void FrCloseConnection(istream *&in, ostream *&out, ostream *&err)
{
   if (in != &cin)
      delete in ;
   if (out != &cout)
      delete out ;
   if (err != &cerr && err != out)
      delete err ;
   in = &cin ;
   out = &cout ;
   err = &cerr ;
   return ;
}

//----------------------------------------------------------------------

#ifndef FrBSD_SOCKETS
FrSocket FrConnectToPort(const char *, int)
{
   return (FrSocket)INVALID_SOCKET ;
}
#else
FrSocket FrConnectToPort(const char *hostname, int portnum)
{
#if defined(__WINDOWS__) || defined(__NT__) || defined(_WIN32)
   if (!FrWinSock_initialized && !FramepaC_winsock_init())
      return (FrSocket)INVALID_SOCKET ;
#endif /* __WINDOWS__ || __NT__ || _WIN32 */
   FrMessageLoop() ;
   int localhost ;
   char *localhostname = 0 ;
   if (!hostname || !*hostname)
      {
      localhost = true ;
      localhostname = FrNewN(char,LOCALHOSTNAME_SIZE) ;
      gethostname(localhostname,LOCALHOSTNAME_SIZE) ;
      localhostname[LOCALHOSTNAME_SIZE-1] = '\0' ;
      hostname = localhostname ;
      }
   else
      localhost = false ;
   struct hostent *host = gethostbyname(hostname) ;
   if (localhost)
      FrFree(localhostname) ;
   if (host == 0)
      return (FrSocket)INVALID_SOCKET ;
   struct sockaddr_in sa ;
   memcpy((char*)&sa.sin_addr,(char*)host->h_addr,host->h_length) ;
   sa.sin_family = host->h_addrtype ;
   sa.sin_port = htons((unsigned short)portnum) ;
   FrSocket s = (FrSocket)socket(sa.sin_family,SOCK_STREAM,0) ;
   if (s != (FrSocket)SOCKET_ERROR)
      {
      FrMessageLoop() ;
      if (connect(s,(sockaddr*)&sa,sizeof(sa)) < 0)
	 {
	 if (0
#ifdef WSAEADDRNOTAVAIL
	     || errno == WSAEADDRNOTAVAIL
#endif
#ifdef EADDRNOTAVAIL
	     || errno == EADDRNOTAVAIL
#endif
#ifdef WSAECONNREFUSED
	     || errno == WSAECONNREFUSED
#endif
#ifdef ECONNREFUSED
	     || errno == ECONNREFUSED
#endif
#ifdef WSAENETUNREACH
	     || errno == WSAENETUNREACH
#endif
#ifdef ENETUNREACH
	     || errno == ENETUNREACH
#endif
	    )
	    close_socket(s) ;
	 else
	    close_socket_immediately(s) ;
	 return (FrSocket)INVALID_SOCKET ;
	 }
      }
   FrMessageLoop() ;
   return s ;
}
#endif /* !FrBSD_SOCKETS */

//----------------------------------------------------------------------

int FrDisconnectPort(FrSocket s)
{
   FrMessageLoop() ;
#ifdef FrBSD_SOCKETS
   return close_socket_immediately(s) ;
#else
   (void)s ;
   return SOCKET_ERROR ;
#endif /* FrBSD_SOCKETS */
}

//----------------------------------------------------------------------

int FrInputAvailable(FrSocket s)
{
   INIT_WINSOCK() ;
   bool can_read, have_error ;
   check_socket(s,&can_read,0,&have_error) ;
   if (have_error)
      return -1 ;
   else if (can_read)
      return 1 ;
   else
      return 0 ;
}

//----------------------------------------------------------------------

int FrAwaitActivity(FrSocket *fds, int numfds, int timeout_sec,
		    int timeout_microsec)
{
   INIT_WINSOCK() ;
#ifdef FrBSD_SOCKETS
   fd_set fdset ;
   FD_ZERO(&fdset) ;
   int numsocks = 0 ;
   for (int i = 0 ; i < numfds ; i++)
      if (fds[i] != (FrSocket)-1)
	 {
	 FD_SET(fds[i],&fdset) ;
	 numsocks++ ;
	 }
   if (numsocks == 0)
      return false ;
   fd_set readset, errset ;
   int result = safe_select(&fdset,timeout_sec,timeout_microsec,&readset,NULL,
			    &errset) ;
   return (result > 0) ;
#else
   FrMessageLoop() ;
   (void)fds ; (void)numfds ; (void)timeout ;
   return 0 ;
#endif /* FrBSD_SOCKETS */
}

//----------------------------------------------------------------------

int FrAwaitActivity(FrSocket fd, int timeout_microsec)
{
   INIT_WINSOCK() ;
#ifdef FrBSD_SOCKETS
   int timeout_sec = timeout_microsec / 1000000 ;
   timeout_microsec %= 1000000 ;
   fd_set fdset ;
   FD_ZERO(&fdset) ;
   FD_SET(fd,&fdset) ;
   fd_set readset, writeset, errset ;
   int result = safe_select(&fdset,timeout_sec,timeout_microsec,&readset,
			    &writeset,&errset) ;
   return (result > 0) ;
#else
   FrMessageLoop() ;
   (void)fds ; (void)numfds ; (void)timeout ;
   return 0 ;
#endif /* FrBSD_SOCKETS */
}

//----------------------------------------------------------------------

bool FrSSLAvailable()
{
#ifdef FrUSE_SSL
   return true ;
#else
   return false ;
#endif /* FrUSE_SSL */
}

//----------------------------------------------------------------------

// end of frsckstr.cpp //
