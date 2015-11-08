/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC								*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frexec.cpp		subprogram invocation			*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1995,1996,1997,1998,2000,2001,2002,2005,2006,2007,	*/
/*		2009,2010,2012,2015					*/
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

#if defined(__WATCOMC__) || defined(_MSC_VER)
#include <io.h>
#include <dos.h>			// for sleep()
#endif
#if defined(__SOLARIS__)
#  include <sys/filio.h>	 // for FIONBIO
#endif /* __SOLARIS__ */
#if defined(unix) || defined(__linux__) || defined(__GNUC__)
#  include <sys/ioctl.h>
#endif

#include <errno.h>
#include "frconfig.h"

#ifdef FrSTRICT_CPLUSPLUS
#  include <fstream>
#  include <cstdio>
#  include <cstdlib>
#  include <cstring>
#  include <string>
#else
#  include <fstream.h>
#  include <stdio.h>
#  include <stdlib.h>
#  include <string.h>
#endif /* FrSTRICT_CPLUSPLUS */

#if defined(__WINDOWS__) || defined(__NT__)
#include <winsock.h>  // for gethostname()
//extern "C" FILE *popen(const char *cmd, const char *mode) ;
#elif defined(unix) || defined(__linux__) || defined(__GNUC__)
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#else /* !__WINDOWS__ && !__NT__ && !unix */
# define gethostname(buf,size) \
      (strncpy(buf,"localhost",size), buf[size-1] = '\0', 0)
#endif

#include "framerr.h"
#include "frexec.h"
#include "frfilutl.h"
#include "frmem.h"
#include "frmswin.h"
#include "frprintf.h"
#include "frsckstr.h"
#include "frutil.h"

#ifndef FrUSING_POPEN
#include <process.h>
#endif /* FrUSING_POPEN */

extern bool FramepaC_verbose ;

/************************************************************************/
/*    Manifest constants for this module				*/
/************************************************************************/

#define MAX_CMDLINE 8192

/************************************************************************/
/*	Global variables for this module				*/
/************************************************************************/

static FrMutex exec_mutex(true) ;

static int child_pid = -1 ;

/************************************************************************/
/************************************************************************/

static void set_child_pid(int pid)
{
    FrCRITSECT_ENTER(exec_mutex) ;
    child_pid = pid ;
    FrCRITSECT_LEAVE(exec_mutex) ;
    return ;
}

//----------------------------------------------------------------------

static bool connect_to_port(const char *hostname, int portnum, int &pipe_in,
			    int &pipe_out,istream *&stream_in,
			    ostream *&stream_out, bool silent = false)
{
#ifdef FrUSING_SOCKETS
   FrSocket s = FrConnectToPort(hostname,portnum) ;
   if (s < 0)
      {
      if (!silent)
	 FrWarning("Unable to connect to remote process!") ;
      return false ;
      }
   pipe_in = pipe_out = s ;
   stream_in = new FrISockStream(s) ;
   stream_out = new FrOSockStream(s) ;
   return true ;
#else /* FrUSING_SOCKETS */
   (void)hostname; (void)portnum ; (void)pipe_in ; (void)pipe_out ;
   (void)silent ; (void)stream_in ; (void)stream_out ;
   FrWarning("Sockets are not supported in this implementation") ;
   return false ;
#endif /* FrUSING_SOCKETS */
}

//----------------------------------------------------------------------

static char **adjust_arglist(size_t count, char **args, bool copy,
			     const char *remote_exec, const char *remote_host,
			     int portnum, const char *network_flag)
{
   size_t extra = 0 ;
   bool have_port = false ;
   if (portnum > 0 && network_flag && *network_flag)
      {
      have_port = true ;
      extra++ ;			// extra argument for the network flag
      }
   char cmdline[MAX_CMDLINE] ;
   cmdline[0] = '\0' ;
   char *end = cmdline ;
   if (remote_exec && remote_host && *remote_host)
      {
      Fr_sprintf(cmdline,sizeof(cmdline),remote_exec,remote_host) ;
      char *nul = strchr(cmdline,'\0') ;
      char *cmd = cmdline ;
      for ( ; cmd < nul ; cmd++)
	 {
	 if (*cmd == ' ')
	    {
	    while (*cmd == ' ' && cmd < nul)
	       *cmd++ = '\0' ;
	    extra++ ;
	    }
	 }
      extra++ ;				// one more word than blanks
      }
   if (copy)
      {
      if (count == 0)
	 {
	 for (char **a = args ; *a ; a++)
	    count++ ;
	 }
      char **new_args = FrNewN(char*,count+2) ;
      if (!new_args)
	 {
	 FrNoMemory("while building argument list for FrExec") ;
	 return 0 ;
	 }
      for (size_t i = 0 ; i <= count ; i++)
	 new_args[i] = FrDupString(args[i]) ;
      new_args[count+1] = 0 ;
      args = new_args ;
      }
   if (extra)
      {
      char **new_args = FrNewR(char*,args,count+extra+2) ;
      if (new_args)
	 {
	 for (size_t i = count ; i > 1 ; i--)
	    new_args[extra+i-1] = new_args[i-1] ;
	 if (!have_port)
	    new_args[extra] = new_args[0] ;
	 else
	    {
	    static char nflag[350] ;
	    static char local_host[300] ;
#if defined(__WINDOWS__) || defined(__NT__)
	    FramepaC_winsock_init() ;	 // ensure that WinSock is ready
#endif /* __WINDOWS__ || __NT__ */
	    int status = gethostname(local_host,sizeof(local_host)) ;
	    if (status == 0)
	       Fr_sprintf(nflag,sizeof(nflag),"%s%s:%d",
			  network_flag,local_host,portnum) ;
	    else
	       Fr_sprintf(nflag,sizeof(nflag),"%slocalhost:%d",
			  network_flag,portnum) ;
	    new_args[extra-1] = new_args[0] ;
	    new_args[extra] = FrDupString(nflag) ;
	    }
	 size_t argc = 0 ;
	 for (char *cmd = cmdline ; cmd < end ; cmd++)
	    {
	    while (*cmd == '\0' && cmd < end)
	       cmd++ ;
	    new_args[argc++] = FrDupString(cmd) ;
	    cmd = strchr(cmd,'\0') ;
	    }
	 return new_args ;
	 }
      else
	 {
	 FrNoMemory("while expanding argument list for FrExec") ;
	 return 0 ;
	 }
      }
   return args ;
}

//----------------------------------------------------------------------

static char **build_arglist(const char *remote_exec,
			    const char *remote_host, int portnum,
			    const char *network_flag, const char *program,
			    va_list args)
{
   FrSafeVAList(args) ;
   int argc = 1 ;
   const char *arg ;
   do {
      arg = va_arg(FrSafeVarArgs(args),const char *) ;
      argc++ ;
      } while (arg) ;
   FrSafeVAListEnd(args) ;
   char **arglist = FrNewC(char*,argc+2) ;
   argc = 0 ;
   arglist[argc++] = FrDupString(program) ;
   do {
      arg = va_arg(args,const char *) ;
      arglist[argc++] = FrDupString(arg) ;
      } while (arg) ;
   arglist[argc] = 0 ;
   return adjust_arglist(argc,arglist,false,remote_exec,remote_host,portnum,
			 network_flag) ;
}

//----------------------------------------------------------------------

static void delete_arglist(char **arglist)
{
   if (arglist)
      {
      for (int i = 0 ; arglist[i] ; i++)
	 FrFree(arglist[i]) ;
      FrFree(arglist) ;
      }
   return ;
}

//----------------------------------------------------------------------

static bool fork_program(char **arglist, int &pipe_in, int &pipe_out)
{
#if defined(MSDOS) || defined(__MSDOS__) || defined(__WINDOWS__) || defined(__NT__)
  FrWarning("Sorry, fork() is not available under MS-DOS or Windoze.\n"
	    "Change the program's configuration to indicate a socket number\n"
	    "other than -1 to permit the use of "
#ifdef FrUSING_POPEN
	    "popen"
#else
	    "spawn"
#endif /* FrUSING_POPEN */
	    "(), which IS supported.") ;
  (void)arglist ; (void)pipe_in ; (void)pipe_out ;
  return false ;
#else
  int pipe_desc[2] ;
  pipe_desc[0] = EOF ;
  pipe_desc[1] = EOF ;
  errno = 0 ;
  int pipe_stat = pipe( pipe_desc ) ;
  if (pipe_stat)
     {
     if (pipe_desc[0] != EOF) close(pipe_desc[0]) ;
     if (pipe_desc[1] != EOF) close(pipe_desc[1]) ;
     FrErrorVA("bad write pipe (errno=%d: %s)",errno,strerror(errno)) ;
     return false ;
     }
  int pipe_in_child = pipe_desc[0] ;
  int pipe_out_parent = pipe_desc[1] ;
  errno = 0 ;
  pipe_stat = pipe( pipe_desc ) ;
  if (pipe_stat)
     {
     close(pipe_in_child) ;
     close(pipe_out_parent) ;
     if (pipe_desc[0] != EOF && pipe_desc[0] != pipe_in_child)
	(void)close(pipe_desc[0]) ;
     if (pipe_desc[1] != EOF && pipe_desc[1] != pipe_out_parent)
	(void)close(pipe_desc[1]) ;
     FrErrorVA("bad read pipe (errno=%d: %s)",errno,strerror(errno)) ;
     return false ;
     }
  int pipe_in_parent = pipe_desc[0] ;
  int pipe_out_child = pipe_desc[1] ;

  errno = 0 ;
  int pid = fork() ;
  if (pid == -1)
     {
     close(pipe_in_parent) ;
     close(pipe_in_child) ;
     close(pipe_out_parent) ;
     close(pipe_out_child) ;
     FrErrorVA("unable to fork %s (errno=%d)",arglist[0],errno) ;
     return false ;
     }
  else if (pid == 0)
     {
     dup2( pipe_in_child, 0 ) ;	  // put the read end of the pipe on stdin
     dup2( pipe_out_child, 1 ) ;  // put the write end of the pipe on stdout
//   dup2( 1, 2 ) ;		  // also put stderr thru the pipe
     if (!FramepaC_verbose)
	{
	close(2) ;
	open(FrNULL_DEVICE,O_WRONLY) ;
	}
     close(pipe_in_child) ;
     close(pipe_out_child) ;
     close( pipe_in_parent ) ;	 // close the unused ends of the pipes
     close( pipe_out_parent ) ;
     errno = 0 ;
     execvp( arglist[0], arglist) ;
     // not reached except when error
     FrErrorVA("couldn't exec program %s (errno=%d) -- check configuration file",
	       arglist[0],errno) ;
     return false ;
     }
  else
     {
     close(pipe_in_child) ;	// close the unused ends of the pipes
     close(pipe_out_child) ;
     pipe_in = pipe_in_parent ;
     pipe_out = pipe_out_parent ;
     set_child_pid(pid) ;
     }
  return true ;
#endif /* __WINDOWS__ || __NT__ */
}

//----------------------------------------------------------------------

#ifdef FrUSING_POPEN
static bool popen_program(char **arglist, ostream &err)
{
   if (!arglist || !arglist[0] || !*arglist[0])
      return false ;
   if (!FrFileExecutable(arglist[0]))
      {
      FrWarningVA("unable to execute %s\n"
		  "Check whether the file exists and has proper attributes",
		  arglist[0]) ;
      return false ;
      }
   int cmdlen = 0 ;
   for (int i = 0 ; arglist[i] ; i++)
      {
      cmdlen += strlen(arglist[i]) ;
      if (arglist[i+1])
	 cmdlen++ ;
      }
   FrLocalAlloc(char,shellcmd,1024,cmdlen+1) ;
   char *cmd = shellcmd ;
   for (int j = 0 ; arglist[j] ; j++)
      {
      int len = strlen(arglist[j]) ;
      memcpy(cmd,arglist[j],len) ;
      cmd += len ;
      if (arglist[j+1])
	 *cmd++ = ' ' ;
      else
	 *cmd = '\0' ;
      }
   FILE *fp = popen(shellcmd,"w") ;
   FrLocalFree(shellcmd) ;
   if (!fp)
      err << "popen() of subprocess failed!" << endl ;
   // we really should remember fp in order to do a pclose(fp) before
   // program termination....
   return (fp != 0) ;
}
#endif /* FrUSING_POPEN */

//----------------------------------------------------------------------

static bool _FrExecProgram(const char *hostname, int portnum,
			   int &pipe_in, int &pipe_out,
			   istream *&stream_in, ostream *&stream_out,
			   ostream &err,
			   const char *progname, char **arglist)
{
   set_child_pid(-1) ;
   if (progname && *progname)
      {
      bool result ;
      if (portnum > 0)
	 {
#ifdef FrUSING_POPEN
	 result = popen_program(arglist,err) ;
	 if (result)
#else
	 int sp ;
         sp = spawnvp(P_NOWAIT,arglist[0],(char const * const *)arglist) == 0 ;
	 if (sp == 0)
#endif /* FrUSING_POPEN */
	    {
	    result = true ;
	    // wait for the child program to contact us
	    FrSocket sock = FrAwaitConnection(portnum, FrPOPEN_LOAD_TIMEOUT,
					      err,false) ;
	    if (sock != (FrSocket)INVALID_SOCKET)
	       {
	       pipe_in = pipe_out = sock ;
	       stream_in = new FrISockStream(sock) ;
	       stream_out = new FrOSockStream(sock) ;
	       if (!stream_in || !stream_in->good())
		  result = false ;
	       // the child program is supposed to send us a line identifying
	       // itself
	       if (FramepaC_verbose)
		  {
		  char line[MAX_CMDLINE] ;
		  stream_in->getline(line,sizeof(line)) ;
		  err << "; Connected: " << line << endl ;
		  }
	       else
		  stream_in->ignore(MAX_CMDLINE,'\n') ;
	       }
	    else
	       result = false ;
	    // close the listening socket to avoid having it hang around
	    (void)FrAwaitConnection(-1,0,err,false) ;
	    }
	 }
      else
	 {
	 result = fork_program(arglist,pipe_in,pipe_out) ;
	 if (result)
	    {
	    stream_in = Fr_ifstream(pipe_in) ;
	    stream_out = Fr_ofstream(pipe_out) ;
	    }
	 }
      return result ;
      }
   else
      return connect_to_port(hostname,portnum,pipe_in, pipe_out,
			     stream_in,stream_out) ;
}

//----------------------------------------------------------------------

bool __FrCDECL FrExecProgram(const char *remote, const char *hostname,
			     int portnum, const char *network_flag,
			     int &pipe_in, int &pipe_out,
			     istream *&stream_in, ostream *&stream_out,
			     ostream &err, const char *progname, ...)
{
   va_list progargs ;
   va_start(progargs,progname) ;
   char **arglist = build_arglist(remote,hostname,portnum,network_flag,
				  progname,progargs) ;
   va_end(progargs) ;
   if (!arglist)
      return false ;
   bool result = _FrExecProgram(hostname,portnum,pipe_in,pipe_out,
				stream_in,stream_out,err,progname,arglist) ;
   delete_arglist(arglist) ;
   return result ;
}

//----------------------------------------------------------------------

bool FrExecProgram(const char *remote, const char *hostname,
		   int portnum, const char *network_flag,
		   int &pipe_in, int &pipe_out,
		   istream *&stream_in, ostream *&stream_out,
		   ostream &err, const char *progname, va_list progargs)
{
   char **arglist = build_arglist(remote,hostname,portnum,network_flag,
				  progname,progargs) ;
   if (!arglist)
      return false ;
   bool result = _FrExecProgram(hostname,portnum,pipe_in,pipe_out,
				stream_in,stream_out,err,progname,arglist) ;
   delete_arglist(arglist) ;
   return result ;
}

//----------------------------------------------------------------------

bool FrExecProgram(const char *remote, const char *hostname,
		   int portnum, const char *network_flag,
		   int &pipe_in, int &pipe_out,
		   istream *&stream_in, ostream *&stream_out,
		   ostream &err, const char **argv)
{
   char **arglist = adjust_arglist(0,(char**)argv,true,remote,hostname,portnum,
				   network_flag) ;
   if (!arglist)
      return false ;
   const char *progname = argv[0] ;
   bool result = _FrExecProgram(hostname,portnum,pipe_in,pipe_out,
				stream_in,stream_out,err,progname,arglist) ;
   delete_arglist(arglist) ;
   return result ;
}

//----------------------------------------------------------------------

int __FrCDECL FrExecProgramAndWait(ostream &err, const char *progname, ...)
{
   va_list progargs ;
   va_start(progargs,progname) ;
   char **arglist = build_arglist((char*)0,(char*)0,-1,(char*)0,
				  progname,progargs) ;
   va_end(progargs) ;
   if (!arglist)
      return -1 ;
   int pipe_in, pipe_out ;
   istream *stream_in ;
   ostream *stream_out ;
   int status = -1 ;
   FrCRITSECT_ENTER(exec_mutex) ;
   bool started = _FrExecProgram((char*)0,-1,pipe_in,pipe_out,
				 stream_in,stream_out,err,progname,arglist) ;
   int pid = FrChildProgramID() ;
   FrCRITSECT_LEAVE(exec_mutex) ;
   if (started)
      {
      waitpid(pid,&status,0) ;
      FrShutdownPipe(stream_in,stream_out,pipe_in,pipe_out) ;
      }
   delete_arglist(arglist) ;
   return status ;
}

//----------------------------------------------------------------------

int FrChildProgramID()
{
   FrCRITSECT_ENTER(exec_mutex) ;
   int id = child_pid ;
   FrCRITSECT_LEAVE(exec_mutex) ;
   return id ;
}

//----------------------------------------------------------------------

bool FrShutdownPipe(istream *stream_in, ostream *stream_out,
		      int pipe_in, int pipe_out)
{
   if (pipe_in != pipe_out)
      delete stream_out ;
   delete stream_in ;
   if (pipe_in != EOF)
      (void)close(pipe_in) ;
   if (pipe_out != EOF)
      (void)close(pipe_out) ;
   return true ;
}

// end of file frexec.cpp //
