/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frfinddb.cpp		Find database files			*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,2001,2009,2011,2015		*/
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

#include <string.h>
#include "framerr.h"
#include "frfilutl.h"
#include "frfinddb.h"
#include "frlist.h"
#include "frstring.h"
#include "frutil.h"

/************************************************************************/

#if defined(__MSDOS__) || defined(__WATCOMC__) || defined(_MSC_VER)
#  include <dos.h>
#  include <io.h>
#elif defined(__SUNOS__) || defined(__SOLARIS__)
#  include <unistd.h>
#else
#  include <unistd.h>
#endif /* __MSDOS__, __SUNOS__, etc. */

#if defined(unix) || defined(__linux__) || defined(__GNUC__)
#include <dirent.h>
#endif /* unix */

/************************************************************************/
/*    Global variables for this module					*/
/************************************************************************/

static char *db_directory = 0 ;

/************************************************************************/
/************************************************************************/

static int string_compare(const FrObject *s1, const FrObject *s2)
{
   if (!s1 || !s1->stringp())
      return 1 ;   // non-strings sort after strings
   else if (!s2 || !s2->stringp())
      return -1 ;
   else if ((*(FrString*)s1) < (*(FrString*)s2))
      return -1 ;
   else if ((*(FrString*)s1) > (*(FrString*)s2))
      return 1 ;
   else
      return 0 ;
}

//----------------------------------------------------------------------

const char *FramepaC_get_db_dir()
{
   return db_directory ;
}

//----------------------------------------------------------------------

void FramepaC_set_db_dir(const char *dir)
{
   FrFree(db_directory) ;
   if (!dir)
      dir = "." ;
   db_directory = FrDupString(dir) ;
   return ;
}

//----------------------------------------------------------------------

#if defined(__MSDOS__) || defined(__WATCOMC__)
static FrList *find_files_DOS(const char *directory, const char *mask,
			       const char *extension,
			       bool strip_extension = true)
{
   if (!mask)
      mask = "*" ;
   if (!extension)
      extension = "*" ;
   struct find_t ffblk ;
   FrList *files = 0 ;
   size_t dirlen = strlen(directory) ;
   size_t masklen = strlen(mask) + 1 ;
   size_t buflen = dirlen + masklen + strlen(extension) + 2 ;
   FrLocalAlloc(char,fullname,1024,buflen) ;
   if (!fullname)
      {
      FrNoMemory("while getting disk directory") ;
      return 0 ;
      }
   memcpy(fullname,directory,dirlen) ;
   fullname[dirlen++] = '/' ;
   memcpy(fullname+dirlen,mask,masklen) ;
   if (*extension)
      {
      strcat(fullname,".") ;
      strcat(fullname,extension) ;
      }
   int found = _dos_findfirst(fullname,_A_NORMAL,&ffblk) ;
   while (found == 0)
      {
      char *period = strip_extension ? strchr(ffblk.name,'.')
	                             : strchr(ffblk.name,'\0') ;
      if (period)
	 {
	 int len = period - ffblk.name ;
	 pushlist(new FrString(ffblk.name,len),files) ;
	 }
      found = _dos_findnext(&ffblk) ;
      }
   FrLocalFree(fullname) ;
   return files->sort(string_compare) ;
}
#endif /* __MSDOS__ || __WATCOMC__ */

//----------------------------------------------------------------------

#if (defined(_MSC_VER) && _MSC_VER >= 800) || (defined(__WATCOMC__) && defined(__WINDOWS__))
static FrList *find_files_Windows(const char *directory, const char *mask,
				   const char *extension,
				   bool strip_extension = true)
{
   if (!mask)
      mask = "*" ;
   if (!extension)
      extension = "*" ;
   struct _finddata_t ffblk ;
   FrList *files = 0 ;
   size_t dirlen = strlen(directory) ;
   size_t masklen = strlen(mask) + 1 ;
   size_t buflen = dirlen + masklen + strlen(extension) + 2 ;
   FrLocalAlloc(char,fullname,1024,buflen) ;
   if (!fullname)
      {
      FrNoMemory("while getting disk directory") ;
      return 0 ;
      }
   memcpy(fullname,directory,dirlen) ;
   fullname[dirlen++] = '/' ;
   memcpy(fullname+dirlen,mask,masklen) ;
   if (*extension)
      {
      strcat(fullname,".") ;
      strcat(fullname,extension) ;
      }
   ffblk.attrib = _A_NORMAL ;
   long searchhandle = _findfirst(fullname,&ffblk) ;
//!!!
   if (searchhandle == -1)
      {
      FrLocalFree(fullname) ;
      return 0 ;
      }
   int found = 1 ;
   while (found > 0)
      {
      char *period = strip_extension ? strchr(ffblk.name,'.')
	                             : strchr(ffblk.name,'\0') ;
      if (period)
	 {
	 int len = period - ffblk.name ;
	 pushlist(new FrString(ffblk.name,len),files) ;
	 }
      found = _findnext(searchhandle,&ffblk) ;
      }
   _findclose(searchhandle) ;
   FrLocalFree(fullname) ;
   return files->sort(string_compare) ;
}
#endif /* _MSC_VER >= 800 */

//----------------------------------------------------------------------

#if defined(unix) || defined(__linux__) || defined(__GNUC__)
static FrList *find_files_Unix(const char *directory, const char *filemask,
			       const char *extension,
			       bool strip_extension = true)
{
   if (!extension)
      extension = "" ;
   if (!filemask)
      filemask = "*" ;
   size_t filelen = strlen(filemask) ;
   size_t extlen = strlen(extension) + 1 ;
   size_t masklen = filelen + extlen + 1 ;
   FrLocalAlloc(char,mask,1024,masklen) ;
   if (!mask)
      {
      FrNoMemory("while searching for files") ;
      return 0 ;
      }
   memcpy(mask,filemask,filelen+1) ;
   if (*extension)
      {
      mask[filelen++] = '.' ;
      memcpy(mask+filelen,extension,extlen) ;
      }
   FrList *files = 0 ;
   DIR *dirp = opendir(directory) ;
   if (dirp)
      {
      struct dirent *entry ;
      while ((entry = readdir(dirp)) != 0)
	 {
	 const char *name = entry->d_name ;
	 const char *m = mask ;
	 while (*name && *m && *m != '*' &&
		(*name == *m || *m == '?'))
	    {
	    name++ ;
	    m++ ;
	    }
	 bool match = false ;
	 if (*m == '*')
	    {
	    size_t tail = strlen(m+1) ;
	    size_t remaining = strlen(name) ;
	    if (tail <= remaining &&
		strcmp(name+remaining-tail,m+1) == 0)
	       match = true ;
	    }
	 else if (*m == *name)
	    match = true ;
	 if (match)
	    {
	    name = entry->d_name ;
	    size_t len = strlen(name) ;
	    if (strip_extension && *extension)
	       len -= (strlen(extension) + 1) ;
	    pushlist(new FrString(name,len), files) ;
	    }
	 }
      closedir(dirp) ;
      }
   FrLocalFree(mask) ;
   return files->sort(string_compare) ;
}
#endif /* unix */

//----------------------------------------------------------------------

FrList *find_databases(const char *directory, const char *mask)
{
   if (!directory)
      directory = FramepaC_get_db_dir() ;
   if (!mask)
      mask = "*" ;
#if defined(__MSDOS__) || defined(__WATCOMC__)
   return find_files_DOS(directory,mask,DB_EXTENSION) ;
#elif defined(_MSC_VER) && _MSC_VER >= 800
   return find_files_Windows(directory,mask,DB_EXTENSION) ;
#else
   return find_files_Unix(directory,mask,DB_EXTENSION) ;
#endif
}

//----------------------------------------------------------------------

FrList *FrFindFiles(const char *directory, const char *mask)
{
   if (!directory)
      directory = FramepaC_get_db_dir() ;
   if (!mask)
      mask = "*" ;
#if (defined(_MSC_VER) && _MSC_VER >= 800) || (defined(__WATCOMC__) && defined(__WINDOWS__))
   return find_files_Windows(directory,mask,0,false) ;
#elif defined(__MSDOS__) || defined(__WATCOMC__)
   return find_files_DOS(directory,mask,0,false) ;
#else
   return find_files_Unix(directory,mask,0,false) ;
#endif
}

// end of file frfinddb.cpp //
