/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frfilut5.cpp		FrFileExists, FrFileReadable, etc.	*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 2001,2005,2009,2015					*/
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

#include <sys/stat.h>
#include "frfilutl.h"
#include "frlowio.h"

using namespace std ;

#ifndef F_OK
#  define F_OK 0
#endif

#ifndef X_OK
#define X_OK 1
#endif

#ifndef W_OK
#define W_OK 2
#endif

#ifndef R_OK
#  define R_OK 4
#endif

/************************************************************************/
/************************************************************************/

bool FrFileExists(const char *filename)
{
   return (filename != 0 && *filename && access(filename,F_OK) == 0) ;
}

//----------------------------------------------------------------------

bool FrFileReadable(const char *filename)
{
   return (filename != 0 && *filename && access(filename,R_OK) == 0) ;
}

//----------------------------------------------------------------------

bool FrFileWriteable(const char *filename)
{
   return (filename != 0 && *filename && access(filename,W_OK) == 0) ;
}

//----------------------------------------------------------------------

bool FrFileReadWrite(const char *filename)
{
   return (filename != 0 && *filename && access(filename,R_OK | W_OK) == 0) ;
}

//----------------------------------------------------------------------

bool FrFileExecutable(const char *filename)
{
   return (filename != 0 && *filename && access(filename,X_OK) == 0) ;
}

//----------------------------------------------------------------------

bool FrIsDirectory(const char *filename)
{
   if (filename && *filename)
      {
#if !defined(S_IFDIR) && defined(_S_IFDIR)
#  define S_IFDIR _S_IFDIR
#endif
#ifdef S_IFDIR
      struct stat statbuf ;
      if (stat(filename,&statbuf) == 0 && S_ISDIR(statbuf.st_mode))
	 return true ;
#endif
      return access(filename,X_OK) == 0 ;
      }
   return false ;
}

//----------------------------------------------------------------------

bool FrIsReadableDirectory(const char *filename)
{
   return FrIsDirectory(filename) && (access(filename,R_OK) == 0) ;
}

//----------------------------------------------------------------------

bool FrIsWriteableDirectory(const char *filename)
{
   return (filename != 0 && access(filename,X_OK | W_OK) == 0) ;
}

//----------------------------------------------------------------------

// end of frfilut5.cpp //
