/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC								*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frexec.h		subprogram invocation			*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1995,1996,1997,2001,2005,2009,2015			*/
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

#ifndef __FREXEC_H_INCLUDED
#define __FREXEC_H_INCLUDED

#if defined(unix) || defined(__linux__) || defined(__GNUC__)
#define FrUSING_SOCKETS
#define FrUSING_POPEN
#elif defined(__WINDOWS__) || defined(__NT__)
#define FrUSING_SOCKETS
//#define FrUSING_POPEN
#else
//#define FrUSING_SOCKETS
//#define FrUSING_POPEN
#endif /* unix, __WINDOWS__ || __NT__ */

#ifndef __FRCOMMON_H_INCLUDED
#include "frcommon.h"
#endif

/************************************************************************/
/*    Manifest Constants						*/
/************************************************************************/

#define FrPOPEN_LOAD_TIMEOUT  60	// max number of seconds to wait after
					// popen() for child program to
					// contact us

/************************************************************************/
/************************************************************************/

bool __FrCDECL FrExecProgram(const char *remote, const char *hostname,
			     int portnum, const char *network_flag,
			     int &pipe_in, int &pipe_out,
			     istream *&stream_in, ostream *&stream_out,
			     ostream &err, const char *progname, ...) ;
bool FrExecProgram(const char *remote, const char *hostname,
		   int portnum, const char *network_flag,
		   int &pipe_in, int &pipe_out,
		   istream *&stream_in, ostream *&stream_out,
		   ostream &err, const char *progname, va_list progargs) ;
bool FrExecProgram(const char *remote, const char *hostname,
		   int portnum, const char *network_flag,
		   int &pipe_in, int &pipe_out,
		   istream *&stream_in, ostream *&stream_out,
		   ostream &err, const char **argv) ;
int FrChildProgramID() ;
bool FrShutdownPipe(istream *stream_in, ostream *stream_out,
		    int pipe_in, int pipe_out) ;

int __FrCDECL FrExecProgramAndWait(ostream &err, const char *progname, ...) ;

/*
  FrExecProgram args:
     remote:   string to prepend for running on remote machine, or 0
     hostname: machine on which to run remotely, or to contact via socket
     portnum:  socket number or -1
     network_flag:   0 or start of arg to tell program to run on socket (i.e.
	      "-n" will add a flag "-n<portnum>" if running via sockets)
     pipe_in: returns fd/socket for reading from child program
     pipe_out: returns fd/socket for writing to child program
     stream_in: returns istream for reading from child program
     stream_out: returns ostream for writing to child program
     progname: the name of the program to execute (0 or "" to only establish
	      network connection, requires valid hostname and portnum)
     ... : NULL-terminated list of argument strings

     argv: array of pointers to strings, terminated with a NULL pointer;
	  argv[0] is the name of the program to execute, all other
	  elements before the NULL pointer are arguments to pass the program
*/

#endif /* !__FREXEC_H_INCLUDED */

// end of file frexec.h //
