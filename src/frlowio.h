/************************************************************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frlowio.h		low-level I/O function declarations	*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 2001,2015 Ralf Brown/Carnegie Mellon University	*/
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

#ifndef __FRLOWIO_H_INCLUDED
#define __FRLOWIO_H_INCLUDED

// encapsulate some common conditional compilations in this header....


#if defined(unix) || defined(__linux__) || defined(__GNUC__)
#  include <unistd.h>
#elif defined(__WATCOMC__) || defined(_MSC_VER)
#  include <io.h>
#elif defined(__MSDOS__) || defined(__WINDOWS__) || defined(__NT__)
#  include <io.h>
#endif

#endif /* !__FRLOWIO_H_INCLUDED */

// end of file frlowio.h //
