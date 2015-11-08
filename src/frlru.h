/************************************************************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frlru.h	    discard of least-recently used frames       */
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,2004				*/
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

#ifndef __FRLRU_H_INCLUDED
#define __FRLRU_H_INCLUDED

#ifndef __FRCONFIG_H_INCLUDED
// frconfig.h includes limits.h, so only include it here if not already
#include <limits.h>
#endif

/**********************************************************************/
/**********************************************************************/

#define LRUclock_LIMIT ((0xFFFFFFFF/4)*3)

void initialize_FramepaC_LRU() ;
void shutdown_FramepaC_LRU() ;
int discard_LRU_frames() ;

#endif /* !__FRLRU_H_INCLUDED */

// end of file frlru.h //
