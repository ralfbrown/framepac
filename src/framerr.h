/************************************************************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File framerr.h	   error handlers				*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,1998,2002,2015 			*/
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

#ifndef __FRAMERR_H_INCLUDED
#define __FRAMERR_H_INCLUDED

#ifndef __FRCONFIG_H_INCLUDED
#  include "frconfig.h"
#endif

/**********************************************************************/
/*    Type definitions						      */
/**********************************************************************/

typedef void (*FramepaC_error_handler)(const char *message) ;

#ifndef __GNUC__
// make __attribute__ definition disapear where not supported
#  define __attribute__(x)
#endif
/**********************************************************************/
/*	 Manifest constants for error codes			      */
/**********************************************************************/

#define FE_SUCCESSFUL		0

#define FE_COMMITFAILED		1000	// error flushing file to disk
#define FE_FILEOPEN	        1001    // unable to open file
#define FE_FILECLOSE	  	1002	// error closing file
#define FE_SEEK	        	1003	// error setting file position
#define FE_WRITEFAULT   	1004	// error writing to file
#define FE_READFAULT    	1005	// error reading from file
#define FE_CORRUPTED    	1006	// database file corrupted
#define FE_INVALIDPARM	        1007    // invalid parameter to function

#define ME_NOTRANSACTIONS     	2000	// transactions disabled
#define ME_NOINDEX            	2001
#define ME_NOTFOUND           	2002
#define ME_BADINDEX           	2003
#define ME_CANTCREATE         	2004
#define ME_NOSUCHTRANSACTION	2005	// bad transaction handle
#define ME_ROLLBACK		2006	// error while undoing transaction
#define ME_TOOMANYDBS	        2007	// too many databases open at once
#define ME_LOCKED		2008	// db already in use by another proc.
#define ME_PASSWORD		2009    // incorrrect password given
#define ME_PRIVILEGED	        2010    // insufficient privileges for oper.

/**********************************************************************/
/*	 Global Variables					      */
/**********************************************************************/

extern int Fr_errno ;

/**********************************************************************/
/*    Function prototypes					      */
/**********************************************************************/

void FrMessage(const char *message) ;
void FrMessageVA(const char *message,...)
     __attribute__((format(printf,1,2)));
void FrWarning(const char *message) ;
void FrWarningVA(const char *message,...)
     __attribute__((format(printf,1,2)));
_fnattr_noreturn void FrError(const char *message) ;
_fnattr_noreturn void FrErrorVA(const char *message,...)
     __attribute__((format(printf,1,2)));
_fnattr_noreturn void FrProgError(const char *message) ;
_fnattr_noreturn void FrProgErrorVA(const char *message,...)
     __attribute__((format(printf,1,2)));
void FrNoMemory(const char *where) ;
void FrNoMemoryVA(const char *where, ...)
     __attribute__((format(printf,1,2)));

void FrUndefined(const char *funcname) ;
void FrInvalidVirtualFunction(const char *class_and_method) ;
void FrMissedCase(const char *funcname) ;

FramepaC_error_handler set_message_handler(FramepaC_error_handler) ;
FramepaC_error_handler set_warning_handler(FramepaC_error_handler) ;
FramepaC_error_handler set_fatal_error_handler(FramepaC_error_handler) ;
FramepaC_error_handler set_out_of_memory_handler(FramepaC_error_handler) ;
FramepaC_error_handler set_prog_error_handler(FramepaC_error_handler) ;
FramepaC_error_handler set_undef_function_handler(FramepaC_error_handler) ;
FramepaC_error_handler set_invalid_function_handler(FramepaC_error_handler) ;
FramepaC_error_handler set_missed_case_handler(FramepaC_error_handler) ;

#endif /* __FRAMERR_H_INCLUDED */

// end of file framerr.h //
