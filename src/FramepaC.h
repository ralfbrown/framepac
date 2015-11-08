/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File FramepaC.h							*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,1998,1999,2000,2001,2002,2003,	*/
/*		2005,2006,2007,2009,2010,2015				*/
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

#ifndef __FRAMEPAC_H_INCLUDED
#define __FRAMEPAC_H_INCLUDED

// frcommon.h and frobject.h get included by most other FramepaC header files
// frcommon.h includes frassert.h
//   frobject.h also includes frctype.h

#ifndef __FRAMERR_H_INCLUDED
#include "framerr.h"
#endif

#ifndef __FRHSORT_H_INCLUDED
#include "frhsort.h"
#endif

#ifndef __FRQSORT_H_INCLUDED
#include "frqsort.h"
#endif

#ifndef __FRFEATVEC_H_INCLUDED
#include "frfeatvec.h"
#endif

#ifndef __FRLLIST_H_INCLUDED
#include "frllist.h"
#endif

#ifndef __FRUNICOD_H_INCLUDED
#include "frunicod.h"
#endif

#ifndef __FRSTACK_H_INCLUDED
#include "frstack.h"
// also includes frqueue.h, frlist.h
#endif

#ifndef __FRHASH_H_INCLUDED
#include "frhash.h"
// also includes frmem.h
#endif

#ifndef __FRHASHT_H_INCLUDED
#include "frhasht.h"
#endif

#ifndef __FRARRAY_H_INCLUDED
#include "frarray.h"
#endif

#ifndef __FRBITVEC_H_INCLUDED
#include "frbitvec.h"
#endif

#ifndef __FRSTRING_H_INCLUDED
#include "frstring.h"
#endif

#ifndef __FRFLOAT_H_INCLUDED
#include "frfloat.h"
// also includes frnumber.h
#endif

#ifndef __FRSYMTAB_H_INCLUDED
#include "frsymtab.h"
// also includes frsymbol.h
#endif

#ifndef __VFRAME_H_INCLUDED
#include "vframe.h"
// also includes frframe.h
#endif

#ifndef __FRSTRUCT_H_INCLUDED
#include "frstruct.h"
#endif

#ifndef __FRMOTIF_H_INCLUDED
#include "frmotif.h"
#endif

#ifndef __FRHELP_H_INCLUDED
#include "frhelp.h"
#endif

#ifndef __FRLRU_H_INCLUDED
#include "frlru.h"
#endif

#ifndef __FRPASSWD_H_INCLUDED
#include "frpasswd.h"
#endif

#ifndef __FRREADER_H_INCLUDED
#include "frreader.h"
#endif

#ifndef __FRINLINE_H_INCLUDED
#include "frinline.h"
#endif

#ifndef __FRSCKSTR_H_INCLUDED
#include "frsckstr.h"
#endif

#ifndef __FRMSWIN_H_INCLUDED
#include "frmswin.h"
#endif

#ifndef __FRCLISRV_H_INCLUDED
#include "frclisrv.h"
#endif

#ifndef __FRCFGFIL_H_INCLUDED
#include "frcfgfil.h"
#endif

#ifndef __FRTIMER_H_INCLUDED
#include "frtimer.h"
#endif

#ifndef __FRBYTORD_H_INCLUDED
#include "frbytord.h"
#endif

#ifndef __FRFILUTL_H_INCLUDED
#include "frfilutl.h"
#endif

#ifndef __FREXEC_H_INCLUDED
#include "frexec.h"
#endif

#ifndef __FRSIGNAL_H_INCLUDED
#include "frsignal.h"
#endif

#ifndef __FRTHREAD_H_INCLUDED
#include "frthread.h"
#endif

#ifndef __FRMMAP_H_INCLUDED
#include "frmmap.h"
#endif

#ifndef __FRUTIL_H_INCLUDED
#include "frutil.h"
#endif

#ifndef __FRREGEXP_H_INCLUDED
#include "frregexp.h"
#endif

#ifndef __FRMD5_H_INCLUDED
#include "frmd5.h"
#endif

#ifndef __FRNETSRV_H_INCLUDED
#include "frnetsrv.h"
#endif

//#ifndef __FROVRRID_H_INCLUDED
//#include "frovrrid.h"
// (unfortunately, "template <typename T>" not supported by Watcom C 11.0)
//#endif

#ifndef __FRRANDOM_H_INCLUDED
#include "frrandom.h"
#endif

#ifndef __FREVENT_H_INCLUDED
#include "frevent.h"
#endif

#ifndef __FRBPRIQ_H_INCLUDED
#include "frbpriq.h"
#endif

#ifndef __FRCLUST_H_INCLUDED
#include "frclust.h"
#endif

#ifndef FRBWT_H_INCLUDED
#include "frbwt.h"
#endif

#ifndef __FRPRINTF_H_INCLUDED
#include "frprintf.h"
#endif

#ifndef FRTXTSPN_H_INCLUDED
#include "frtxtspn.h"
#endif

#ifndef __FRMORPHP_H_INCLUDED
#include "frmorphp.h"
#endif

#ifndef FRVOCAB_H_INCLUDED
#include "frvocab.h"
#endif

#ifndef _FRURL_H_INCLUDED
#include "frurl.h"
#endif

#ifndef __FRSLFREG_H_INCLUDED
#include "frslfreg.h"
#endif

#ifndef __FRSPELL_H_INCLUDED
#include "frspell.h"
#endif

#ifndef __FRMATH_H_INCLUDED
#include "frmath.h"
#endif

/************************************************************************/
/*	 other non-member functions					*/
/************************************************************************/

void initialize_FramepaC(int max_symbols = 0) ;
extern void (*FrShutdown)() ;

bool NIL_symbol(const FrObject *object) ;

//----------------------------------------------------------------------

#endif /* !__FRAMEPAC_H_INCLUDED */

// end of file FramepaC.h //
