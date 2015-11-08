/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File: frsignal.cpp	class FrSignalHandler				*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1997,1998,2001,2002,2006,2009,2015			*/
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
#  pragma implementation "frsignal.h"
#endif

#include <signal.h>
#include "framerr.h"
#include "frsignal.h"

/************************************************************************/
/************************************************************************/

#if defined(_MSC_VER)
typedef void __FrCDECL SigHandler(int arg) ;
#elif defined(__linux__) || defined(__SOLARIS__) || defined(__WATCOMC__) || defined(__TURBOC__) || defined(__CYGWIN__)
typedef void SigHandler(int arg) ;
#else
typedef void SigHandler() ;
#endif

static SigHandler handler0 ;
static SigHandler handler1 ;
static SigHandler handler2 ;
static SigHandler handler3 ;
static SigHandler handler4 ;
static SigHandler handler5 ;
static SigHandler handler6 ;
static SigHandler handler7 ;
static SigHandler handler8 ;
static SigHandler handler9 ;
static SigHandler handler10 ;
static SigHandler handler11 ;
static SigHandler handler12 ;
static SigHandler handler13 ;
static SigHandler handler14 ;
static SigHandler handler15 ;
static SigHandler handler16 ;
static SigHandler handler17 ;
static SigHandler handler18 ;
static SigHandler handler19 ;
static SigHandler handler20 ;
static SigHandler handler21 ;
static SigHandler handler22 ;
static SigHandler handler23 ;
static SigHandler handler24 ;
static SigHandler handler25 ;
static SigHandler handler26 ;
static SigHandler handler27 ;
static SigHandler handler28 ;
static SigHandler handler29 ;
static SigHandler handler30 ;
static SigHandler handler31 ;

SigHandler *handlers[] =
   { handler0, handler1, handler2, handler3,
     handler4, handler5, handler6, handler7,
     handler8, handler9, handler10, handler11,
     handler12, handler13, handler14, handler15,
     handler16, handler17, handler18, handler19,
     handler20, handler21, handler22, handler23,
     handler24, handler25, handler26, handler27,
     handler28, handler29, handler30, handler31 } ;

FrSignalHandler *signal_handlers[lengthof(handlers)] ;

static int active_signal = 99 ;

//----------------------------------------------------------------------

#if defined(__WATCOMC__) || defined(__linux__) || defined(__SOLARIS__) || defined(__CYGWIN__) && !defined(__TURBOC__)
#define defhandler(i) static void handler##i(int arg) { invoke(i,arg) ; }
#elif defined(_MSC_VER)
#define defhandler(i) static void __FrCDECL handler##i(int arg) { invoke(i,arg) ; }
#else
#define defhandler(i) static void handler##i() { invoke(i,0) ; }
#endif /* __WATCOMC__ || _MSC_VER */

//----------------------------------------------------------------------

static void error_function(int arg) _fnattr_noreturn ;
static void error_function(int arg)
{
   char errmsg[] = "signal number 00(00) received" ;
#define errmsg_ofs 14
#define arg_ofs 17
   errmsg[errmsg_ofs] = (char)((active_signal/10) + '0') ;
   errmsg[errmsg_ofs+1] = (char)((active_signal%10) + '0') ;
   errmsg[arg_ofs] = (char)((arg/10)%10 + '0') ;
   errmsg[arg_ofs+1] = (char)((arg%10) + '0') ;
   FrError(errmsg) ;
}

//----------------------------------------------------------------------

static void invoke(int signum, int arg)
{
   if (signal_handlers[signum] && signal_handlers[signum]->currentHandler())
      {
      active_signal = signal_handlers[signum]->signalNumber() ;
      FrSignalHandlerFunc *func = signal_handlers[signum]->currentHandler() ;
      func(arg) ;
      }
}

defhandler(0) ;
defhandler(1) ;
defhandler(2) ;
defhandler(3) ;
defhandler(4) ;
defhandler(5) ;
defhandler(6) ;
defhandler(7) ;
defhandler(8) ;
defhandler(9) ;
defhandler(10) ;
defhandler(11) ;
defhandler(12) ;
defhandler(13) ;
defhandler(14) ;
defhandler(15) ;
defhandler(16) ;
defhandler(17) ;
defhandler(18) ;
defhandler(19) ;
defhandler(20) ;
defhandler(21) ;
defhandler(22) ;
defhandler(23) ;
defhandler(24) ;
defhandler(25) ;
defhandler(26) ;
defhandler(27) ;
defhandler(28) ;
defhandler(29) ;
defhandler(30) ;
defhandler(31) ;

/************************************************************************/
/************************************************************************/

FrSignalHandler::FrSignalHandler(int signal_num, FrSignalHandlerFunc *handler)
{
   func = 0 ;
   number = signal_num ;
   set(handler) ;
   // insert ourself into the list of handlers
   for (size_t i = 0 ; i < lengthof(signal_handlers) ; i++)
      {
      if (signal_handlers[i] == 0)
	 {
	 signal_handlers[i] = this ;
#if !defined(__WATCOMC__) && !defined(_MSC_VER) && !defined(__linux__) && !defined(__SOLARIS__) && !defined(__TURBOC__) && !defined(__CYGWIN__)
	 // BSD syntax (note: SunOS-Posix has a sigaction which is same as
	   // sigvec except structure field names are sa_...)
	 struct sigvec vec ;
	 struct sigvec oldvec ;
	 vec.sv_mask = 0 ;
	 vec.sv_flags = 0 ;
	 vec.sv_handler = (void(*)(...))handlers[i] ;
	 ::sigvec(signal_num,&vec,&oldvec) ;
	 old_handler = oldvec.sv_handler ;
	 old_mask = oldvec.sv_mask ;
	 old_flags = oldvec.sv_flags ;
#else
	 // System V/Posix syntax
	 old_handler = (void*)signal(signal_num,handlers[i]) ;
#endif /* !__WATCOMC__ && !_MSC_VER */
	 return ;
	 }
      }
}

//----------------------------------------------------------------------

FrSignalHandler::~FrSignalHandler()
{
#if !defined(__WATCOMC__) && !defined(_MSC_VER) && !defined(__linux__) && !defined(__SOLARIS__) && !defined(__TURBOC__) && !defined(__CYGWIN__)
   // BSD syntax
   struct sigvec vec ;
   vec.sv_mask = old_mask ;
   vec.sv_flags = old_flags ;
   vec.sv_handler = (void(*)(...))&old_handler ;
   ::sigvec(number,&vec,NULL) ;
#else /* handle SystemV/Posix syntax */
   signal(number,(SigHandler*)old_handler) ;
#endif /* !__WATCOMC__ && !_MSC_VER */
   // remove ourself from the list of active signal handlers
   for (size_t i = 0 ; i < lengthof(signal_handlers) ; i++)
      if (signal_handlers[i] == this)
	 signal_handlers[i] = 0 ;
   return ;
}

//----------------------------------------------------------------------

FrSignalHandlerFunc *FrSignalHandler::set(FrSignalHandlerFunc *new_handler)
{
   if (new_handler == (FrSignalHandlerFunc*)SIG_IGN)
      new_handler = 0 ;
   else if (new_handler == (FrSignalHandlerFunc*)SIG_ERR)
      new_handler = error_function ;
   FrSignalHandlerFunc *oldfunc = func ;
   func = new_handler ;
   return oldfunc ;
}

//----------------------------------------------------------------------

void FrSignalHandler::raise(int arg) const
{
   if (func)
      func(arg) ;
   return ;
}

// end of file frsignal.cpp //
