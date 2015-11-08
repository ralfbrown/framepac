/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frvars.cpp	       global variables				*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,1998,2001,2009,2013		*/
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

#include "frpcglbl.h"
#include "FramepaC.h"

/************************************************************************/
/*    forward declarations						*/
/************************************************************************/

static void dummy_FrShutdown() ;

/************************************************************************/
/*	global variables						*/
/************************************************************************/

bool FramepaC_verbose = false ;

size_t FramepaC_num_memmaps = 0 ;
size_t FramepaC_total_memmap_size = 0 ;

/************************************************************************/
/************************************************************************/

class VFrame *(*FramepaC_new_VFrame)(class FrSymbol *name) = 0 ;
void (*FramepaC_shutdown_all_VFrames)() = 0 ;
void (*FramepaC_delete_all_frames)() = 0 ;
void (*FrShutdown)() = dummy_FrShutdown ;

void (*FramepaC_clear_userinfo_dir)() = 0 ;

/************************************************************************/
/*    dummy functions to reduce the amount of code pulled in 		*/
/************************************************************************/

static void dummy_FrShutdown() {}

// end of file frvars.cpp //
