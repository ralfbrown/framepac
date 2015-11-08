/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frcritsec.C		short-duration critical section mutex	*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 2010,2013,2014,2015					*/
/*	    Ralf Brown/Carnegie Mellon University			*/
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

#include "frcritsec.h"
#include "frthread.h"

/************************************************************************/
/************************************************************************/

#ifdef FrMULTITHREAD
#if __cplusplus >= 201103L || (__GNUC__ >= 4 && !defined(HELGRIND))
void FrCriticalSection::backoff_acquire()
{
   size_t loops = 0 ;
   while (true)
      {
      incrCollisions() ;
      if (!tryacquire())
	 return ;
      FrThreadBackoff(loops) ;
      }
   return ;
}
#endif
#endif /* FrMULTITHREAD */

/************************************************************************/
/*	Platform-specific implementations 				*/
/************************************************************************/

#ifdef FrMULTITHREAD
# if defined(__linux__)
#  include "frcritsec-linux.C"
# elif defined (__WINDOWS__)
#  include "frcritsec-windows.C"
# else
#  include "frcritsec-pthread.C"
# endif /* __linux__ / __WINDOWS__ */
#endif /* FrMULTITHREAD */

// end of file frcritsec.cpp //
