/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frhelp.h	    help system -- interface to Mosaic		*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,2009				*/
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

#ifndef __FRHELP_H_INCLUDED
#define __FRHELP_H_INCLUDED

#ifndef __FRMOTIF_H_INCLUDED
#include "frmotif.h"
#endif

/************************************************************************/
/*	Configuration options						*/
/************************************************************************/

// name of the executable for Mosaic (may be anywhere on path or full pathname)
//#define FrMOSAIC_PROGNAME "Mosaic"
#define FrMOSAIC_PROGNAME "mosaic"

// name of subdirectory to use for help files (if it exists, otherwise use
//  main directory)
#define FrHELP_SUBDIRECTORY "help"

#define FrMOSAIC_HOMEPAGE "help.html"

/************************************************************************/
/************************************************************************/

#define FrHelptext_completion 0   // help for frame name completion
#define FrHelptext_verify     1   // default help for verify dialog
#define FrHelptext_numtexts (FrHelptext_completion+1)

/************************************************************************/
/************************************************************************/

bool FrInitializeHelp(const char *base_directory, const char *browser = 0) ;
void FrShutdownHelp() ;

void FrSetHelpText(int helptype, const char **helptext) ;
void FrHelp(Widget, XtPointer helptexts, XtPointer) ;
void FrHelp1(Widget, XtPointer document, XtPointer) ;

int popup_help(char *document) ;

#endif /* !__FRHELP_H_INCLUDED */

// end of file frhelp.h //
