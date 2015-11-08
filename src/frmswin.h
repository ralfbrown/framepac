/************************************************************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frmswin.h		 Microsoft Windows-specific functions	*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1996,1997,2001 Ralf Brown/Carnegie Mellon University	*/
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

#ifndef __FRMSWIN_H_INCLUDED
#define __FRMSWIN_H_INCLUDED

#ifndef __FRCONFIG_H_INCLUDED
#include "frconfig.h"
#endif

#if defined(__WINDOWS__) || defined(__NT__) || defined(_WIN32)
extern int FrWinSock_initialized ;

int FramepaC_winsock_init() ;
void FrMinimizeWindow() ;
void FrHideWindow() ;
void FrDestroyWindow() ;
void FrMessageLoop() ;
void FrExitProcess(int exitcode) ;
unsigned long FrGetCPUTime() ;		// CPU time in FrTICKS_PER_SEC units
#define INIT_WINSOCK() { if (!FrWinSock_initialized) FramepaC_winsock_init(); }

#else /* !__WINDOWS__, !__NT__ */

#define FrMinimizeWindow()
#define FrHideWindow()
#define FrDestroyWindow()
#define FrMessageLoop()
#define INIT_WINSOCK()
#define FrExitProcess(exitcode) exit(exitcode)
#endif /* __WINDOWS__ || __NT__ */

void FrSetAppTitle(const char *title) ;

void FrSleep(int seconds) ;
void Fr_usleep(long microseconds) ;

#endif /* !__FRMSWIN_H_INCLUDED */

// end of file frmswin.h //
