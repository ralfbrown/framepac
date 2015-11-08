/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frfcache.cpp		file-cache access functions		*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1999,2000,2001,2006,2009,2013				*/
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

#include <fcntl.h>
#if defined(__WATCOMC__) || defined(__MSDOS__) || defined(__NT__)
#  include <io.h>
#endif /* __WATCOMC__ || __MSDOS__ || __NT__ */
#include "frconfig.h"
#if defined(__linux__)
#  include <unistd.h>	// for unlink()
#endif /* __linux__ */
#include "frbytord.h"
#include "frmem.h"
#include "frnumber.h"
#include "frstring.h"
#include "frsymbol.h"
#include "frutil.h"
#include "frfilutl.h"

/************************************************************************/
/*	Manifest Constants						*/
/************************************************************************/

#define FILE_CACHE_SIGNATURE "FramepaC File Cache"

#ifndef O_BINARY
#  define O_BINARY 0
#endif /* !O_BINARY */

/************************************************************************/
/*	Types for this module						*/
/************************************************************************/

typedef unsigned char LONGbuffer[4] ;

/************************************************************************/
/*	Global Variables for this module				*/
/************************************************************************/

FrFileCache *FrFileCache::caches = 0 ;

/************************************************************************/
/*	Helper Functions						*/
/************************************************************************/

static bool check_signature(FILE *fp, const char *signature)
{
   char buf[300] ;
   size_t siglen = strlen(signature) ;
   if (fread(buf,sizeof(char),siglen,fp) < siglen)
      return false ;
   buf[siglen] = '\0' ;
   if (strcmp(buf,signature) == 0)
      {
      // skip remainder of first line
      return fgets(buf,sizeof(buf),fp) != 0 ;
      }
   else
      return false ;
}

/************************************************************************/
/*	Methods for class FrFileCache					*/
/************************************************************************/

FrFileCache::FrFileCache(const char *dir_path, const char *cachename)
{
   next_cache = caches ;
   caches = this ;
   dirpath = FrDupString(dir_path) ;
   name = FrDupString(cachename) ;
   numfiles = 0 ;
   filenames = 0 ;
   starts = 0 ;
   ends = 0 ;
   datafd = EOF ;			// assume failure (non-existent cache)
   char *cachepath = FrAddDefaultPath(cachename,dir_path) ;
   char *cache_idx = FrForceFilenameExt(cachepath,"idx") ;
   FILE *indexfp = fopen(cache_idx,FrFOPEN_READ_MODE) ;
   FrFree(cache_idx) ;
   if (indexfp && check_signature(indexfp,FILE_CACHE_SIGNATURE))
      {
      char *cache_dat = FrForceFilenameExt(cachepath,"dat") ;
      datafd = open(cache_dat,O_RDONLY | O_BINARY) ;
      FrFree(cache_dat) ;
      if (datafd != EOF)
	 {
	 // load the index
	 LONGbuffer version, num_ent ;
	 if (fread(version,1,sizeof(version),indexfp) == sizeof(version) &&
	     fread(num_ent,1,sizeof(num_ent),indexfp) == sizeof(num_ent))
	    {
	    if (FrLoadLong(version) != 1)
	       FrWarning("invalid version number in file cache's index file!");
	    numfiles = FrLoadLong(num_ent) ;
	    filenames = FrNewN(char*,numfiles) ;
	    starts = FrNewN(unsigned long,2*numfiles) ;
	    if (!filenames || !starts)
	       {
	       close(datafd) ;
	       datafd = EOF ;
	       FrNoMemory("opening file cache") ;
	       FrFree(filenames) ;
	       }
	    else
	       {
	       ends = starts + numfiles ;
	       for (size_t i = 0 ; i < numfiles ; i++)
		  {
		  LONGbuffer value ;
		  bool success ;
		  success = (fread(value,1,sizeof(value),indexfp) == sizeof(value)) ;
		  starts[i] = FrLoadLong(value) ;
		  success = (fread(value,1,sizeof(value),indexfp) == sizeof(value)) ;
		  ends[i] = FrLoadLong(value) ;
		  char filename[FrMAX_LINE] ;
		  filename[0] = '\0' ;
		  success = (fgets(filename,sizeof(filename),indexfp) != 0) ;
		  size_t len = strlen(filename) ;
		  if (len > 0 && filename[len-1] == '\n')
		     filename[len-1] = '\0' ;
		  filenames[i] = FrDupString(filename) ;
		  (void)success ; // we don't actually care about the value, but keep the compiler happy
		  }
	       }
	    }
	 else
	    {
	    // error reading index
	    close(datafd) ;
	    datafd = EOF ;
	    FrWarning("error reading file cache's index") ;
	    }
	 }
      fclose(indexfp) ;
      }
   FrFree(cachepath) ;
   return ;
}

//----------------------------------------------------------------------

FrFileCache::~FrFileCache()
{
   FrFileCache *prev_cache = 0 ;
   for (FrFileCache *c = caches ; c ; c = c->next_cache)
      {
      if (c == this)
	 {
	 if (prev_cache)
	    prev_cache->next_cache = next_cache ;
	 else
	    FrFileCache::caches = next_cache ;
	 break ;
	 }
      prev_cache = c ;
      }
   FrFree(dirpath) ;
   FrFree(name) ;
   if (datafd != EOF)
      close(datafd) ;
   FrFree(starts) ;
   starts = ends = 0 ;
   if (filenames)
      {
      for (size_t i = 0 ; i < numfiles ; i++)
	 FrFree(filenames[i]) ;
      FrFree(filenames) ;
      }
   numfiles = 0 ;
   return ;
}

//----------------------------------------------------------------------

FrFileCache *FrFileCache::findCache(const char *dirpath, const char *cachename)
{
   for (FrFileCache *c = caches ; c ; c = c->next_cache)
      if (strcmp(c->dirpath,dirpath) == 0 &&
	  strcmp(c->name,cachename) == 0)
	 return c ;
   // if we get here, there was no cache for the specified directory
   return 0 ;
}

//----------------------------------------------------------------------

FrITextFile *FrFileCache::openFile(const char *filename)
{
   if (!filename || !*filename)
      return 0 ;
   if (numfiles == 0)
      {
      char *fullpath = FrAddDefaultPath(filename,dirpath) ;
      FrITextFile *file = new FrITextFile(filename) ;
      FrFree(fullpath) ;
      return file ;
      }
   // perform binary search of (sorted) array of filenames included in cache
   size_t lo = 0 ;
   size_t hi = numfiles - 1 ;
   do {
      size_t mid = (hi + lo) / 2 ;
      const char *midfile = filenames[mid] ;
      int cmp = strcmp(filename,midfile) ;
      if (cmp > 0)			// file after midpoint?
	 lo = mid + 1 ;
      else if (cmp < 0)			// file before midpoint?
	 {
	 if (mid == 0)
	    break ;
	 else
	    hi = mid - 1 ;
	 }
      else // if (cmp == 0)		// filename matched?
	 return new FrITextFile(datafd,starts[mid],ends[mid]) ;
      } while (lo <= hi) ;
   // if we get here, the requested file is not included in the cache, so
   //   perform a normal file open
   char *fullpath = FrAddDefaultPath(filename,dirpath) ;
   FrITextFile *file = new FrITextFile(filename) ;
   FrFree(fullpath) ;
   return file ;
}

//----------------------------------------------------------------------

FrITextFile *FrFileCache::openFile(const char *filename, const char *cachename)
{
   if (!filename || !*filename)
      return 0 ;
   if (!cachename || !*cachename)
      return new FrITextFile(filename) ;
   char *dir_path = FrFileDirectory(filename) ;
   FrFileCache *cache = FrFileCache::findCache(dir_path,cachename) ;
   if (!cache)
      {
      // cache doesn't exist yet, so load it if present in directory
      cache = new FrFileCache(dir_path,cachename) ;
      }
   FrFree(dir_path) ;
   if (cache && cache->good())
      return cache->openFile(FrFileBasename(filename)) ;
   else
      return new FrITextFile(filename) ;
}

//----------------------------------------------------------------------

static const char *extract_filename(const FrObject *obj)
{
   if (obj)
      {
      if (obj->stringp())
	 return (const char*)((FrString*)obj)->stringValue() ;
      else if (obj->symbolp())
	 return ((FrSymbol*)obj)->symbolName() ;
      }
   return 0 ;
}

//----------------------------------------------------------------------

static int compare_indexrec(const FrObject *o1, const FrObject *o2)
{
   const FrList *rec1 = (FrList*)o1 ;
   const FrList *rec2 = (FrList*)o2 ;
   const FrString *name1 = (FrString*)rec1->first() ;
   const FrString *name2 = (FrString*)rec2->first() ;
   return name1->compare(name2) ;
}

//----------------------------------------------------------------------

bool FrFileCache::erase(const char *dir_path, const char *cachename)
{
   clear(dir_path,cachename) ;
   char *cachepath = FrAddDefaultPath(cachename,dir_path) ;
   char *cache_idx = FrForceFilenameExt(cachepath,"idx") ;
   unlink(cache_idx) ;
   FrFree(cache_idx) ;
   char *cache_dat = FrForceFilenameExt(cachepath,"dat") ;
   unlink(cache_dat) ;
   FrFree(cache_dat) ;
   FrFree(cachepath) ;
   return true ;
}

//----------------------------------------------------------------------

bool FrFileCache::clear(const char *dir_path, const char *cachename)
{
   FrFileCache *cache = FrFileCache::findCache(dir_path,cachename) ;
   if (cache)
      {
      delete cache ;
      return true ;
      }
   else
      return false ;
}

//----------------------------------------------------------------------

bool FrFileCache::clearAll()
{
   while (caches)
      delete caches ;
   return true ;
}

//----------------------------------------------------------------------

bool FrFileCache::create(const char *dir_path, const char *cachename,
			   const FrList *files, FrFileCacheFunc *preproc)
{
   if (!dir_path || !*dir_path || !cachename || !*cachename)
      {
      FrWarning("must specify both path and cache name\n"
		"\tfor FrFileCache::create") ;
      return false ;
      }
   char *cachepath = FrAddDefaultPath(cachename,dir_path) ;
   char *cache_idx = FrForceFilenameExt(cachepath,"idx") ;
   FILE *indexfp = fopen(cache_idx,FrFOPEN_WRITE_MODE) ;
   FrFree(cache_idx) ;
   char *cache_dat = FrForceFilenameExt(cachepath,"dat") ;
   FrFree(cachepath) ;
   FILE *datafp = fopen(cache_dat,FrFOPEN_WRITE_MODE) ;
   FrFree(cache_dat) ;
   if (!indexfp || !datafp)
      {
      FrWarning("unable to create file cache's files!") ;
      if (indexfp)
	 fclose(indexfp) ;
      return false ;
      }
   fputs(FILE_CACHE_SIGNATURE,datafp) ;
   fputs(" ; do not manually edit!",datafp) ;
   fputc('\n',datafp) ;
   fflush(datafp) ;
   fputs(FILE_CACHE_SIGNATURE,indexfp) ;
   fputs(" ; do not manually edit!",indexfp) ;
   fputc('\n',indexfp) ;
   fflush(indexfp) ;
   LONGbuffer value ;
   FrStoreLong(1,value) ;
   (void)Fr_fwrite(value,sizeof(value),1,indexfp) ;
   FrList *indexrecords = 0 ;
   for ( ; files ; files = files->rest())
      {
      const char *filename = extract_filename(files->first()) ; ;
      if (FrFileReadable(filename))
	 {
	 FILE *fp = fopen(filename,FrFOPEN_READ_MODE) ;
	 if (fp)
	    {
	    off_t start = ftell(datafp) ;
	    if (preproc)
	       preproc(fp,filename,datafp) ;
	    else
	       {
	       // copy the file's contents to the cache
	       char buffer[4*FrMAX_LINE] ;
	       int count ;
	       do {
	          count = fread(buffer,1,sizeof(buffer),fp) ;
		  if (count > 0)
		     (void)Fr_fwrite(buffer,count,1,datafp) ;
	          } while (count > 0) ;
	       }
	    fclose(fp) ;
	    fflush(datafp) ;
	    off_t end = ftell(datafp) ;
	    // add a new record to the temporary in-memory index
	    pushlist(new FrList(new FrString(filename),new FrInteger(start),
				new FrInteger(end)),
		     indexrecords) ;
	    }
	 }
      }
   // write the sorted index record to the index file
   indexrecords = indexrecords->sort(compare_indexrec) ;
   FrStoreLong(indexrecords->listlength(),value) ;
   (void)Fr_fwrite(value,sizeof(value),1,indexfp) ;
   while (indexrecords)
      {
      FrList *indexrec = (FrList*)poplist(indexrecords) ;
      // write starting offset
      FrStoreLong(indexrec->second()->intValue(),value) ;
      (void)Fr_fwrite(value,sizeof(value),1,indexfp) ;
      // write ending offset
      FrStoreLong(indexrec->third()->intValue(),value) ;
      (void)Fr_fwrite(value,sizeof(value),1,indexfp) ;
      fputs((char*)((FrString*)indexrec->first())->stringValue(),indexfp) ;
      fputc('\n',indexfp) ;
      }
   fflush(indexfp) ;
   fclose(indexfp) ;
   fclose(datafp) ;
   return true ;
}

// end of file frfcache.cpp //
