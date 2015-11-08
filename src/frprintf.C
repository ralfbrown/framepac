/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC								*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frprintf.cpp		formatted print-to-string functions	*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 2006,2009 Ralf Brown/Carnegie Mellon University	*/
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

#include "frconfig.h"
#include "frmem.h"
#include "frprintf.h"

#ifdef __WATCOMC__
#  undef NO_EXT_KEYS
#endif

#ifdef FrSTRICT_CPLUSPLUS
#  include <cstdio>
#else
#  include <stdio.h>
#endif /* FrSTRICT_CPLUSPLUS */
#include <stdarg.h>

/************************************************************************/
/************************************************************************/

size_t Fr_vsprintf(char *buf, size_t buflen, const char *fmt,
		   va_list args)
{
   // for now
#if defined(__WATCOMC__) && !defined(NO_EXT_KEYS)
   return _vbprintf(buf,buflen,fmt,args) ;
#else
   // note: Berkeley Unix returns the original buffer address, while
   // SysV, Borland, Watcom, etc. snprintf() return the number of characters
   // generated.  To work on both, ignore the return value and search
   // for the end of the generated output explicitly.
   //(void)vsnprintf(buf,buflen,fmt,args)
   //return strlen(buf);
   return vsnprintf(buf,buflen,fmt,args) ;
#endif
}

//----------------------------------------------------------------------

size_t Fr_sprintf(char *buf, size_t buflen, const char *fmt, ...)
{
   va_list args ;
   va_start(args,fmt) ;
   size_t count = Fr_vsprintf(buf,buflen,fmt,args) ;
   va_end(args) ;
   return count ;
}

//----------------------------------------------------------------------

char *Fr_vaprintf(const char *fmt, va_list args)
{
   FrSafeVAList(args) ;
   size_t count = Fr_vsprintf(0,0,fmt,FrSafeVarArgs(args)) ;
   FrSafeVAListEnd(args) ;
   char *buf = FrNewN(char,count+1) ;
   if (buf)
      Fr_vsprintf(buf,count+1,fmt,args) ;
   return buf ;
}

//----------------------------------------------------------------------

char *Fr_aprintf(const char *fmt, ...)
{
   va_list args ;
   va_start(args,fmt) ;
   size_t count = Fr_vsprintf(0,0,fmt,args) ;
   va_end(args) ;
   char *buf = FrNewN(char,count+1) ;
   if (buf)
      {
      va_start(args,fmt) ;
      Fr_vsprintf(buf,count+1,fmt,args) ;
      va_end(args) ;
      }
   return buf ;
}

// end of file frprintf.cpp //
