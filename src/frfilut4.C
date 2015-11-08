/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frfilut4.cpp		FrSafelyRewriteFile			*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 2000,2001,2003,2005,2008,2009				*/
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

#if defined(__WATCOMC__) || defined(__MSDOS__) || defined(_MSC_VER)
#  include <io.h> 	// for unlink()
#endif /* __WATCOMC__ || __MSDOS__ || _MSC_VER */
#ifdef __linux__
#  include <unistd.h>	// for unlink()
#endif /* __linux__ */
#include "framerr.h"
#include "frctype.h"
#include "frfilutl.h"

#ifdef FrSTRICT_CPLUSPLUS
# include <cstdlib>
# include <cstring>	
# include <string>	// needed by RedHat 7.1
#else
# include <stdlib.h>
# include <string.h>	// needed by RedHat 7.1
#endif /* FrSTRICT_CPLUSPLUS */

#include <errno.h>

/************************************************************************/
/************************************************************************/

int Fr_unlink(const char *filename)
{
   return unlink(filename) ;
}

//----------------------------------------------------------------------

bool FrSafelyReplaceFile(const char *tempname, const char *filename,
			   const char *description)
{
   if (!filename || !tempname)
      {
      errno = EINVAL ;
      return false ;
      }
   char *bakname = FrForceFilenameExt(filename,"bak") ;
   bool success = false ;
   if (bakname)
      {
      (void)Fr_unlink(bakname) ;
      errno = 0 ;
      if (FrFileExists(filename) && rename(filename,bakname) != 0)
	 {
	 if (description && *description)
	    {
	    cerr << "Error renaming " << description << " to " << bakname
		 << endl ;
	    if (errno == EACCES)
	       cerr << "  (access denied)" << endl ;
	    }
	 }
      else if  (rename(tempname,filename) != 0)
	 {
	 if (description && *description)
	    {
	    cerr << "Error renaming temporary file to " << filename << endl ;
	    if (errno == EACCES)
	       cerr << "  (access denied)" << endl ;
	    }
	 }
      else
	 success = (!FrFileExists(bakname) || Fr_unlink(bakname) == 0) ;
      FrFree(bakname) ;
      }
   else
      {
      FrNoMemory("replacing file with a new version") ;
      errno = ENOMEM ;
      }
   return success ;
}

//----------------------------------------------------------------------

bool FrSafelyRewriteFile(const char *file_name, FrRewriteFileFunc *fn,
			   void *user_data)
{
   if (!fn)
      {
      errno = EINVAL ;
      return false ;
      }
   char *tmpname = FrForceFilenameExt(file_name,"tmp") ;
   if (!tmpname)
      {
      errno = ENOMEM ;
      return false ;
      }
   FILE *fp = fopen(tmpname,FrFOPEN_WRITE_MODE) ;
   if (!fp)
      {
      FrFree(tmpname) ;
      return false ;
      }
   bool success = fn(fp,user_data) ;
   errno = 0 ;
   while (fflush(fp) == EOF && errno == EINTR)
      ;
   (void)fdatasync(fileno(fp)) ;	// flush kernel buffers for file
   fclose(fp) ;
   if (success)
      success = FrSafelyReplaceFile(tmpname,file_name) ;
   else
      Fr_unlink(tmpname) ;
   FrFree(tmpname) ;
   return success ;
}

//----------------------------------------------------------------------

bool FrSameFile(const char *name1, const char *name2)
{
   if (name1 == name2)
      return true ;
   else if (!name1 || !name2)
      return false ;
#if defined(__MSDOS__) || defined(__WINDOWS__) || defined(__NT__)
   for ( ; *name1 && *name2 ; name1++, name2++)
      {
      char c1 = *name1 ;
      char c2 = *name2 ;
      if ((c1 == '/' && c2 == '\\') || (c1 == '\\' && c2 == '/'))
	 continue ;
      else if (Fr_toupper(c1) != Fr_toupper(c2))
	 break ;
      }
   return *name1 == *name2 ;
#else
   return strcmp(name1,name2) == 0 ;
#endif /* __MSDOS__ || __WINDOWS__ || __NT__ */
}

//----------------------------------------------------------------------

// end of frfilut4.cpp //
