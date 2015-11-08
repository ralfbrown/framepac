/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frassert.cpp		debugging assertions			*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1997,2009 Ralf Brown/Carnegie Mellon University	*/
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

#include "frcommon.h"
#include "framerr.h"
#include "frassert.h"

/************************************************************************/
/************************************************************************/

static bool failure_is_fatal = true ;

/************************************************************************/
/************************************************************************/

int FrAssertionFailureFatal(int is_fatal)
{
   int was_fatal = failure_is_fatal ;
   failure_is_fatal = (is_fatal != 0) ;
   return was_fatal ;
}

//----------------------------------------------------------------------

int FrAssertionFailed(const char *file, size_t line)
{
   static const char format[] = "Assertion Failure at %s line %ld" ;
   if (failure_is_fatal)
      FrErrorVA(format,file,(long)line) ;
   else
      FrMessageVA(format,file,(long)line) ;
   return 0 ;
}

//----------------------------------------------------------------------

int FrAssertionFailed(const char *assertion, const char *file, size_t line)
{
   static const char format[] = "Assertion Failure at %s line %ld\n\t%s" ;
   if (failure_is_fatal)
      FrErrorVA(format,file,(long)line,assertion) ;
   else
      FrMessageVA(format,file,(long)line,assertion) ;
   return 0 ;
}

// end of file frassert.cpp //
