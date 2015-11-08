/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frassert.h		debugging assertions			*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1997,1999,2013 Ralf Brown/Carnegie Mellon University	*/
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

#ifndef __FRASSERT_H_INCLUDED
#define __FRASSERT_H_INCLUDED

#include <stddef.h>

int FrAssertionFailed(const char *file, size_t line) ;
int FrAssertionFailed(const char *assertion, const char *file, size_t line) ;
int FrAssertionFailureFatal(int is_fatal) ;

inline void FrTestAssertion(int test, const char *assertion,
		     const char *file, size_t line)
{
   if (!test)
      (void)FrAssertionFailed(assertion,file,line) ;
}

inline void FrTestAssertion0(int test, const char *file, size_t line)
{
   if (!test)
      (void)FrAssertionFailed(file,line) ;
}

// define a variant of assert() that is a NOP unless we are compiling
//   specifically for debugging
#ifndef debug_assert
# ifdef DEBUG
#   define debug_assert assert
# else
#   define debug_assert(x)
# endif /* DEBUG */
#endif /* !debug_assert */

#ifdef NDEBUG
# ifndef assert
#   define assert(test)
# endif /* assert */
# define assertq(test)
# define ASSERT(test,msg)
#else
# define _FrCURRENT_FILE __FILE__
# ifndef assert
#   define assert(test)   FrTestAssertion(test,#test,_FrCURRENT_FILE,__LINE__)
# endif
# define assertq(test)	  FrTestAssertion0(test,_FrCURRENT_FILE,__LINE__)
# define ASSERT(test,msg) FrTestAssertion(test,msg,_FrCURRENT_FILE,__LINE__)
#endif /* NDEBUG */

// source files which use assert() multiple times can save data space by
//   adding the following lines after #include'ing this file and before
//   invoking assert():
//
//       #ifndef NDEBUG
//       # undef _FrCURRENT_FILE
//       static const char _FrCURRENT_FILE[] = __FILE__ ;
//       #endif /* NDEBUG */

#define FrCURRENT_FILE \
#ifndef NDEBUG \
# undef _FrCURRENT_FILE \
static const char _FrCURRENT_FILE[] = __FILE__ ; \
#endif /* NDEBUG */

#endif /* !__FRASSERT_H_INCLUDED */

// end of frassert.h //
