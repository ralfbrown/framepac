/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frlocate.cpp		file-location utility functions		*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 2010,2013,2015 Ralf Brown/Carnegie Mellon University	*/
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

#include "frfilutl.h"
#include "frprintf.h"
#include "frutil.h"

#ifdef FrSTRICT_CPLUSPLUS
#  include <cstring>
#else
#  include <string.h>
#endif

#if defined(unix) || defined(__linux__) || defined(__GNUC__)
#  include <unistd.h>	// for getcwd()
#endif

/************************************************************************/
/************************************************************************/

static char *fallback_dir = 0 ;

/************************************************************************/
/************************************************************************/

static char *locate_in_default_directories(const char *filename)
{
   // check the full path given
   if (FrFileExists(filename))
      return FrDupString(filename) ;
   // check for the file in the current directory
   filename = FrFileBasename(filename) ;
   if (FrFileExists(filename))
      return FrDupString(filename) ;
   // check for the file in the user's home directory
   const char *home = getenv("HOME") ;
   if (home)
      {
      char *path = FrAddDefaultPath(filename,home) ;
      if (path && FrFileExists(path))
	 return path ;
      FrFree(path) ;
      // check for hidden version of the filename by prepending a period
      if (filename[0] != '.')
	 {
	 char *dotted = Fr_aprintf(".%s",filename) ;
	 path = FrAddDefaultPath(dotted,home) ;
	 if (path && FrFileExists(path))
	    {
	    FrFree(dotted) ;
	    return path ;
	    }
	 FrFree(dotted) ;
	 FrFree(path) ;
	 }
      }
   return 0 ;
}

//----------------------------------------------------------------------

static char *locate_in_fallback_directory(const char *filename)
{
   if (fallback_dir && *fallback_dir)
      {
      char *path = FrAddDefaultPath(FrFileBasename(filename),fallback_dir) ;
      if (path && FrFileExists(path))
	 return path ;
      FrFree(path) ;
      }
   return 0 ;
}

//----------------------------------------------------------------------

char *FrLocateFile(const char *filename, const FrList *directories)
{
   char *file = locate_in_default_directories(filename) ;
   if (file)
      return file ;
   // finally, check the given fallback directories in turn
   for ( ; directories ; directories = directories->rest())
      {
      const char *directory = FrPrintableName(directories->first()) ;
      if (directory)
	 {
	 char *path = FrAddDefaultPath(filename,directory) ;
	 if (path && FrFileExists(path))
	    return path ;
	 FrFree(path) ;
	 }
      }
   // if we get here, the file was not found in any of the directories, so
   //   make one last attempt at the fallback location
   return locate_in_fallback_directory(filename) ;
}

//----------------------------------------------------------------------

char *FrLocateFile(const char *filename, va_list args)
{
   char *file = locate_in_default_directories(filename) ;
   if (file)
      return file ;
   FrVarArg(const char *,dir) ;
   while (dir)
      {
      if (dir)
	 {
	 char *path = FrAddDefaultPath(filename,dir) ;
	 if (path && FrFileExists(path))
	    return path ;
	 FrFree(path) ;
	 }
      dir = va_arg(args,const char *) ;
      }
   // if we get here, the file was not found in any of the directories, so
   //   make one last attempt at the fallback location
   return locate_in_fallback_directory(filename) ;
}

//----------------------------------------------------------------------

char *FrLocateFile(const char *filename, ...)
{
   FrVarArgs(filename) ;
   char *file = FrLocateFile(filename,args) ;
   FrVarArgEnd() ;
   return file ;
}

//----------------------------------------------------------------------

bool FrLocateFileSetFallback(const char *directory, bool strip_tail)
{
   FrFree(fallback_dir) ;
   if (strip_tail)
      {
      fallback_dir = FrFileDirectory(directory) ;
      if (fallback_dir)
	 {
	 size_t len = strlen(fallback_dir) ;
	 if (len > 0 && fallback_dir[len-1] == '/')
	    fallback_dir[len-1] = '\0' ;
	 }
      if (!fallback_dir || !*fallback_dir ||
	  (fallback_dir[0] == '.' && fallback_dir[1] == '\0'))
	 {
	 FrFree(fallback_dir) ;
	 char cwd[5000] ;
	 if (getcwd(cwd,sizeof(cwd)))
	    fallback_dir = FrDupString(cwd) ;
	 }
      }
   else
      fallback_dir = FrDupString(directory) ;
   return fallback_dir != 0 ;
}

// end of file frlocate.cpp //
