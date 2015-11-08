/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frfilutl.h		file-access utility functions		*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1996,1997,1998,1999,2000,2001,2002,2003,2004,2008,	*/
/*		2009,2010 Ralf Brown/Carnegie Mellon University		*/
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

#ifndef __FRFILUTL_H_INCLUDED
#define __FRFILUTL_H_INCLUDED

// Magick to get files bigger than 2GB, if supported by platform
#ifndef _LARGEFILE_SOURCE
#define _LARGEFILE_SOURCE 1		// allow use of 64-bit file functions
#endif
#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE 1		// extended 64-bit file functions
#endif
#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64		// off_t is 64 bits even on 32-bit arch
#endif

#ifndef __FRCOMMON_H_INCLUDED
#include "frcommon.h"
#endif

#ifndef __FRLIST_H_INCLUDED
#include "frlist.h"
#endif

#ifndef __FRSTRING_H_INCLUDED
#include "frctype.h"		// for FrCharEncoding
#endif

#ifdef FrSTRICT_CPLUSPLUS
# include <cstdio>
#else
# include <stdio.h>
#endif

/************************************************************************/
/*	Manifest Constants						*/
/************************************************************************/

#define FrMAX_LINE 12288

/************************************************************************/
/*	Types								*/
/************************************************************************/

enum FrFILETYPE { FrFT_UNKNOWN, FrFT_ASCII, FrFT_UNICODE, FrFT_UTF8,
		  FrFT_BINARY } ;

//----------------------------------------------------------------------

class FrITextFile
   {
   private:
      char buffer[120*BUFSIZ+1] ;
      int fd ;
      unsigned long bufoffset ;
      unsigned long filestart ;
      unsigned long fileend ;
      size_t bufsize ;
      size_t bufpos ;
      char *bufend ;
   private:
      void init(int filedesc, unsigned long start, unsigned long end) ;
   public:
      void *operator new(size_t,void *where) { return where ; }
      void *operator new(size_t size) { return FrMalloc(size) ; }
      void operator delete(void *blk) { FrFree(blk) ; }
      FrITextFile(int fd, unsigned long start = 0, unsigned long end = 0) ;
      FrITextFile(const char *filename, unsigned long start = 0,
		  unsigned long end = 0) ;
      ~FrITextFile() ;

      char *getline() ;
      bool seek(unsigned long position) ;
      bool rewind() ;
      void close() ;

      // accessors
      bool good() const { return fd != EOF ; }
      bool eof() const ;
      unsigned long filesize() const { return fileend - filestart ; }
      unsigned long tell() const { return (bufoffset + bufpos) - filestart ; }
   } ;

//----------------------------------------------------------------------

typedef bool FrFileCacheFunc(FILE *inputfp, const char *infile,
			       FILE *cachefp) ;

class FrFileCache
   {
   private:
      static FrFileCache *caches ;
      FrFileCache *next_cache ;
      char *dirpath ;
      char *name ;
      char **filenames ;
      unsigned long *starts ;
      unsigned long *ends ;
      size_t numfiles ;
      int datafd ;
   public:
      FrFileCache(const char *dir_path, const char *cachename) ;
      ~FrFileCache() ;

      static FrFileCache *findCache(const char *dir_path,
				    const char *cachename) ;
      FrITextFile *openFile(const char *filename) ;
      static FrITextFile *openFile(const char *filename,const char *cachename);

      static bool create(const char *dir_path, const char *cachename,
			   const FrList *files, FrFileCacheFunc *preproc = 0) ;
      static bool erase(const char *dir_path, const char *cachename) ;
      static bool clear(const char *dir_path, const char *cachename) ;
      static bool clearAll() ;
      // accessors
      bool good() const { return datafd != EOF ; }
   } ;

/************************************************************************/
/************************************************************************/

int Fr_fsync(int fd) ;

FrFILETYPE FrFileType(const char *filename) ;
FrFILETYPE FrFileType(FILE *) ;
FrFILETYPE FrFileType(istream &) ;

char *FrTempFile(const char *basename, const char *tempdir = 0) ;
char *FrFileDirectory(const char *filename) ;
const char *FrFileBasename(const char *filepath) ; // result -> arg, don't free
const char *FrFileExtension(const char *filepath) ;// result -> arg, don't free
char *FrForceFilenameExt(const char *filename, const char *ext) ;
char *FrAddDefaultPath(const char *filename, const char *default_dir) ;
FILE *FrOpenFile(const char *filename, bool write = true,
		 bool force_creation = true) ;
bool FrCreatePath(const char *dirpath) ;
off_t FrFileSize(FILE *fp) ;
off_t FrFileSize(const char *filename) ;

FILE *FrOpenMaybeCompressedInfile(const char *file, bool &piped) ;
bool FrCloseMaybeCompressedInfile(FILE *fp, bool piped) ;

FILE *FrOpenMaybeCompressedOutfile(const char *file, bool &piped) ;
bool FrCloseMaybeCompressedOutfile(FILE *fp, bool piped) ;

// the following are needed because GCC 3.x dropped the [io]fstream
//   constructors that take a file descriptor....
ostream *Fr_ofstream(int fd) ;
istream *Fr_ifstream(int fd) ;

bool FrCopyFile(const char *srcname, const char *destname) ;
bool FrCopyFile(const char *srcname, FILE *destfp) ;

char *FrReadCanonLine(FILE *fp, bool skip_blank, FrCharEncoding,
		      bool *Unicode_bswap = 0,bool *leading_whitespace = 0,
		      bool force_upper = false,
		      const FrCasemapTable charmap = 0,
		      const char *delimiters = 0,
		      char const * const *abbrevlist = 0) ;
char *FrReadCanonLine(FILE *fp, bool skip_blank = true,
		      bool *Unicode_bswap = 0,bool *leading_whitespace = 0,
		      bool force_upper = false,
		      const FrCasemapTable charmap = 0,
		      const char *delimiters = 0,
		      char const * const *abbrevlist = 0) ;

bool FrGetDelimited(FILE *in, char *buf, size_t bufsize,
		    char delim = '\n') ;

//----------------------------------------------------------------------

#define Fr_fread(var,numbytes,fp) (fread(var,1,numbytes,fp) == numbytes)
#define Fr_fread_v(var,fp) (fread(&var,1,sizeof(var),fp) == sizeof(var))
#define Fr_fread_var(var,fp) (fread(var,1,sizeof(var),fp) == sizeof(var))
#define Fr_fread_array(var,count,fp) (fread(var,sizeof(var[0]),count,fp) == count)

//----------------------------------------------------------------------

// error-checking version of write()
bool Fr_write(int fd, const void *var, size_t totalsize,
	      bool report_errors = true) ;

// error-checking version of fwrite()
bool Fr_fwrite(const void *var, off_t totalsize, FILE *) ;
inline bool Fr_fwrite(const void *var, size_t sz, size_t n, FILE *fp)
   { return Fr_fwrite(var,sz*n,fp) ; }

#define Fr_fwrite_v(var,fp) Fr_fwrite(&(var),sizeof(var),fp)
#define Fr_fwrite_var(var,fp) Fr_fwrite((var),sizeof(var),fp)
#define Fr_fwrite_array(var,count,fp) Fr_fwrite((var),sizeof((var)[0])*(count),fp)

//----------------------------------------------------------------------

bool FrFileExists(const char *filename) ;
bool FrFileReadable(const char *filename) ;
bool FrFileWriteable(const char *filename) ;
bool FrFileReadWrite(const char *filename) ;
bool FrFileExecutable(const char *filename) ;
bool FrIsDirectory(const char *pathname) ;
bool FrIsReadableDirectory(const char *pathname) ;
bool FrIsWriteableDirectory(const char *pathname) ;

int Fr_unlink(const char *filename) ;

bool FrSameFile(const char *name1, const char *name2) ;

FrList *FrFindFiles(const char *directory, const char *mask) ;

bool FrLocateFileSetFallback(const char *directory, bool strip_tail = false) ;
char *FrLocateFile(const char *filename, const FrList *directories) ;
char *FrLocateFile(const char *filename, va_list dirs) ;
char *FrLocateFile(const char *filename, ...) ;

// safely write a new version of a file by writing to a temporary file and
//   then replacing the old version by the temporary file if and only if it
//   was successfully written.
typedef bool FrRewriteFileFunc(FILE *, void *user_data) ;
bool FrSafelyRewriteFile(const char *file_name, FrRewriteFileFunc *fn,
			 void *user_data) ;

// safely replace an old version of a file with a newer one by renaming the
//   old one to a backup name first; if description is non-NULL, error messages
//   will be written to cerr on error
bool FrSafelyReplaceFile(const char *newfile, const char *prevfile,
			 const char *description = 0) ;

#endif /* !__FRFILUTL_H_INCLUDED */

// end of frfilutl.h //
