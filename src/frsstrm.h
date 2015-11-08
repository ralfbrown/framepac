/************************************************************************/
/*									*/
/*  FramepaC								*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File: frsstrm.h	 compiler-independent strstream declaration	*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 2004,2006 Ralf Brown/Carnegie Mellon University	*/
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

#ifndef __FRSSTRM_H_INCLUDED
#define __FRSSTRM_H_INCLUDED

#ifndef __FRCOMMON_H_INCLUDED
#include "frcommon.h"
#endif

#ifdef FrSTRICT_CPLUSPLUS
# include <sstream>
# if __GNUC__ == 3 || __GNUC__ == 4
#   pragma GCC diagnostic ignored "-Wdeprecated"
#   include <backward/strstream>
# elif defined(__GNUC__) && __GNUC__ < 3
#   include <strstream.h>
# endif
using namespace std ;
#else
# if defined(_MSC_VER)
#  include <strstrea.h>
# else
#  include <strstream.h>
# endif /* _MSC_VER */
#endif /* FrSTRICT_CPLUSPLUS */

#endif /* !__FRSSTRM_H_INCLUDED */

// end of file frsstrm.h //
