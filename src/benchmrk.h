/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File benchmark.cpp	   Test/Demo program: benchmark functions	*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,2009,2015				*/
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

#ifndef __BENCHMRK_H_INCLUDED
#define __BENCHMRK_H_INCLUDED

#include "FramepaC.h"

int display_menu(ostream &out, istream &in, bool allow_exit,
		 int maxchoice, const char *title, const char *menu) ;
void display_object_info(ostream &out, const FrObject *obj) ;
void interpret_command(ostream &out, istream &in, bool allow_bench) ;
int get_number(ostream &out, istream &in, const char *prompt,int min,int max) ;

void benchmarks_menu(ostream &out, istream &in) ;


#endif /* !__BENCHMRK_H_INCLUDED */

// end of file benchmrk.h //
