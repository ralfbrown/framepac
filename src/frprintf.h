/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC								*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File: frprintf.h	formatted-output functions			*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 2006,2013,2014 Ralf Brown/Carnegie Mellon University	*/
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

#ifndef __FRPRINTF_H_INCLUDED
#define __FRPRINTF_H_INCLUDED

#ifndef __GNUC__
#  define __attribute__(x)
#elif __GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 4)
#  define gnu_printf printf
#endif

// formatted-printing functions
size_t Fr_sprintf(char *buf, size_t buflen, const char *fmt, ...)
     __attribute__((format(printf,3,4)));
size_t Fr_vsprintf(char *buf, size_t buflen, const char *fmt, va_list)
     __attribute__((format(printf,3,0)));

char *Fr_aprintf(const char *fmt, ...)  // free result with FrFree()
     __attribute__((format(printf,1,2)));
char *Fr_vaprintf(const char *fmt, va_list args)
     __attribute__((format(gnu_printf,1,0))) ;

#endif /* !__FRPRINTF_H_INCLUDED */

// end of file frprintf.h //
