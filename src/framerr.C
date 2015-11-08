/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File framerr.cpp	    error handlers				*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,1998,2001,2003,2005,2006,2009,	*/
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

#include "frconfig.h"
#include "framerr.h"
#include "frprintf.h"

#ifdef FrSTRICT_CPLUSPLUS
#  include <cstdarg>
#  include <cstdio>   // needed for RedHat 7.1
#  include <cstdlib>
#  include <iomanip>
#  include <iostream>
using namespace std ;
#else
#  include <stdarg.h>
#  include <stdio.h>   // needed for RedHat 7.1
#  include <stdlib.h>
#  include <iostream.h>
#  include <iomanip.h>
#endif /* FrSTRICT_CPLUSPLUS */

extern void (*FrShutdown)() ;

/************************************************************************/
/*    Global variables shared with other modules in FramepaC          	*/
/************************************************************************/

int Fr_errno ;

/************************************************************************/
/*    Global variables limited to this module			      	*/
/************************************************************************/

static FramepaC_error_handler message_handler = nullptr ;
static FramepaC_error_handler warning_handler = nullptr ;
static FramepaC_error_handler fatal_error_handler = nullptr ;
static FramepaC_error_handler out_of_memory_handler = nullptr ;
static FramepaC_error_handler prog_error_handler = nullptr ;
static FramepaC_error_handler undef_function_handler = nullptr ;
static FramepaC_error_handler invalid_function_handler = nullptr ;
static FramepaC_error_handler missedcase_handler = nullptr ;

static const char no_message_string[] = "(no message supplied)" ;

static const char fatal_error_str[] = "\n\aFATAL ERROR: " ;
static const char prog_error_str[] = "\n\aProgramming Error: " ;
static const char contact_devlopers_msg[] =
   "\nPlease contact the developers to report"
   "\nthis error so that it may be corrected." ;

/************************************************************************/
/*	Helpers								*/
/************************************************************************/

/************************************************************************/
/*    Error reporting functions					      	*/
/************************************************************************/

static void message(const char *msg)
{
   cout << "FramepaC: " << msg << endl ;
}

//----------------------------------------------------------------------

static void warning(const char *message)
{
   cerr << "\nWARNING: " << message << endl ;
}

//----------------------------------------------------------------------

_fnattr_noreturn static void fatal_error(const char *message) ;
static void fatal_error(const char *message)
{
   cerr << fatal_error_str << message << endl ;
   FrShutdown() ;
   exit(127) ;
}

#ifdef __WATCOMC__
#pragma aux fatal_error aborts ;
#endif /* __WATCOMC__ */

//----------------------------------------------------------------------

_fnattr_noreturn static void prog_error(const char *message) ;
static void prog_error(const char *message)
{
   cerr << prog_error_str << message << contact_devlopers_msg << endl ;
   FrShutdown() ;
   exit(125) ;
}

#ifdef __WATCOMC__
#pragma aux prog_error aborts ;
#endif /* __WATCOMC__ */

//----------------------------------------------------------------------

_fnattr_noreturn static void out_of_memory(const char *message) ;
static void out_of_memory(const char *message)
{
   cerr << "\n\aOUT OF MEMORY " << message << endl ;
   FrShutdown() ;
   exit(126) ;
}

#ifdef __WATCOMC__
#pragma aux out_of_memory aborts ;
#endif /* __WATCOMC__ */

//----------------------------------------------------------------------

static void undef_function(const char *name)
{
   cerr << "\nError: Function " << name << " has not been implemented."
	<< endl ;
   return ;
}

//----------------------------------------------------------------------

static void invalid_function(const char *class_and_method)
{
   cerr << "\nInternal Error: attempted to invoke the invalid virtual\n"
	   "function " << class_and_method << endl
	<< "  This is typically the result of failing to check the\n"
	   "  object's type before performing the operation" << endl ;
   return ;
}

//----------------------------------------------------------------------

static void missed_case(const char *funcname)
{
   cerr << "\nProgramming Error: missed case in switch statement in "
	<< funcname << endl ;
   return ;
}

//----------------------------------------------------------------------

void FrMessage(const char *msg)
{
   if (!msg || !*msg)
      msg = no_message_string ;
   if (message_handler)
      message_handler(msg) ;
   else
      message(msg) ;
   return ;
}

//----------------------------------------------------------------------

void FrMessageVA(const char *msg,...)
{
   if (!msg || !*msg)
      msg = no_message_string ;
   va_list args ;
   va_start(args,msg) ;
   char buf[500] ;
   Fr_vsprintf(buf,sizeof(buf),msg,args) ;
   va_end(args) ;
   FrMessage(buf) ;
   return ;
}

//----------------------------------------------------------------------

void FrWarning(const char *message)
{
   if (!message || !*message)
      message = no_message_string ;
   if (warning_handler)
      warning_handler(message) ;
   else
      warning(message) ;
   return ;
}

//----------------------------------------------------------------------

void FrWarningVA(const char *message, ...)
{
   if (!message || !*message)
      message = no_message_string ;
   va_list args ;
   va_start(args,message) ;
   char buf[500] ;
   Fr_vsprintf(buf,sizeof(buf),message,args) ;
   va_end(args) ;
   FrWarning(buf) ;
   return ;
}

//----------------------------------------------------------------------

void FrError(const char *message)
{
   if (!message || !*message)
      message = no_message_string ;
   if (fatal_error_handler)
      fatal_error_handler(message) ;
   else
      fatal_error(message) ;
   exit(125) ;
}

//----------------------------------------------------------------------

void FrErrorVA(const char *message, ...)
{
   if (!message || !*message)
      message = no_message_string ;
   va_list args ;
   va_start(args,message) ;
   char buf[500] ;
   Fr_vsprintf(buf,sizeof(buf),message,args) ;
   va_end(args) ;
   FrError(buf) ;
   exit(125) ;
}

//----------------------------------------------------------------------

void FrProgError(const char *message)
{
   if (!message || !*message)
      message = no_message_string ;
   if (prog_error_handler)
      prog_error_handler(message) ;
   else
      prog_error(message) ;
   exit(125) ;
}

//----------------------------------------------------------------------

void FrProgErrorVA(const char *message, ...)
{
   if (!message || !*message)
      message = no_message_string ;
   va_list args ;
   va_start(args,message) ;
   char buf[500] ;
   Fr_vsprintf(buf,sizeof(buf),message,args) ;
   va_end(args) ;
   FrProgError(buf) ;
   exit(125) ;
}

//----------------------------------------------------------------------

void FrNoMemory(const char *message)
{
   if (!message || !*message)
      message = no_message_string ;
   if (out_of_memory_handler)
      out_of_memory_handler(message) ;
   else
      out_of_memory(message) ;
   return ;
}

//----------------------------------------------------------------------

void FrNoMemoryVA(const char *message, ...)
{
   if (!message || !*message)
      message = no_message_string ;
   va_list args ;
   va_start(args,message) ;
   char buf[500] ;
   Fr_vsprintf(buf,sizeof(buf),message,args) ;
   va_end(args) ;
   FrNoMemory(buf) ;
   return ;
}

//----------------------------------------------------------------------

void FrUndefined(const char *name)
{
   if (!name || !*name)
      name = no_message_string ;
   if (undef_function_handler)
      undef_function_handler(name) ;
   else
      undef_function(name) ;
   return ;
}

//----------------------------------------------------------------------

void FrInvalidVirtualFunction(const char *class_and_method)
{
   if (!class_and_method || !*class_and_method)
      class_and_method = "[not specified]" ;
   if (undef_function_handler)
      invalid_function_handler(class_and_method) ;
   else
      invalid_function(class_and_method) ;
   return ;
}

//----------------------------------------------------------------------

void FrMissedCase(const char *funcname)
{
   if (!funcname || !*funcname)
      funcname = "?" ;
   if (missedcase_handler)
      missedcase_handler(funcname) ;
   else
      missed_case(funcname) ;
   return ;
}

//----------------------------------------------------------------------

FramepaC_error_handler set_warning_handler(FramepaC_error_handler err)
{
   FramepaC_error_handler old = warning_handler ;

   warning_handler = err ;
   return old ;
}

//----------------------------------------------------------------------

FramepaC_error_handler set_message_handler(FramepaC_error_handler msg)
{
   FramepaC_error_handler old = message_handler ;

   message_handler = msg ;
   return old ;
}

//----------------------------------------------------------------------

FramepaC_error_handler set_fatal_error_handler(FramepaC_error_handler err)
{
   FramepaC_error_handler old = fatal_error_handler ;

   fatal_error_handler = err ;
   return old ;
}

//----------------------------------------------------------------------

FramepaC_error_handler set_out_of_memory_handler(FramepaC_error_handler err)
{
   FramepaC_error_handler old = out_of_memory_handler ;

   out_of_memory_handler = err ;
   return old ;
}

//----------------------------------------------------------------------

FramepaC_error_handler set_prog_error_handler(FramepaC_error_handler err)
{
   FramepaC_error_handler old = prog_error_handler ;

   prog_error_handler = err ;
   return old ;
}

//----------------------------------------------------------------------

FramepaC_error_handler set_undef_function_handler(FramepaC_error_handler err)
{
   FramepaC_error_handler old = undef_function_handler ;

   undef_function_handler = err ;
   return old ;
}

//----------------------------------------------------------------------

FramepaC_error_handler set_invalid_function_handler(FramepaC_error_handler err)
{
   FramepaC_error_handler old = invalid_function_handler ;

   invalid_function_handler = err ;
   return old ;
}

//----------------------------------------------------------------------

FramepaC_error_handler set_missed_case_handler(FramepaC_error_handler err)
{
   FramepaC_error_handler old = missedcase_handler ;

   missedcase_handler = err ;
   return old ;
}

//----------------------------------------------------------------------

// end of file framerr.cpp //
