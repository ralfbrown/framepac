/************************************************************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File: frfilutl.cpp	 	file-access utility functions		*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1996,1997,1998,2000,2001,2002,2003,2004,2006,2007,	*/
/*		2009,2010,2011,2012,2015				*/
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

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "framerr.h"
#include "frfilutl.h"
#include "frmem.h"
#include "frprintf.h"
#include "frutil.h"

#if defined(__MSDOS__) || defined(__WATCOMC__) || defined(_MSC_VER)
#  include <fcntl.h>  // for O_RDONLY
#  include <io.h>    // for fsync()
#  include <direct.h> // for mkdir()
#  include <process.h> // for getpid()
#elif defined(unix) || defined(__GNUC__)
#  include <sys/stat.h>  // for mkdir()
#  if defined(__linux__)
#    include <sys/types.h>
#    include <fcntl.h>
#  endif /* __linux__ */
#  if defined(__CYGWIN__)
#    include <sys/fcntl.h>
#  endif /* __CYGWIN__ */
#  include <unistd.h>
#else
#  include <unistd.h>
#endif
#ifdef __WATCOMC__
#  include <dos.h>
#endif /* __WATCOMC__ */

/************************************************************************/
/*    Manifest constants for this module				*/
/************************************************************************/

#ifndef O_BINARY
#  define O_BINARY 0
#endif

/************************************************************************/
/*    Global data for this module					*/
/************************************************************************/

//----------------------------------------------------------------------

static const char fopen_read_mode[] = FrFOPEN_READ_MODE ;
static const char fopen_append_mode[] = FrFOPEN_APPEND_MODE ;

/************************************************************************/
/*	Helper functions						*/
/************************************************************************/

#if defined(FrHAVE_MKSTEMP)
#elif defined(FrNEED_MKTEMP) && !defined(FrHAVE_TEMPNAM)
// a dummy version that does approximately the same thing as Unix mktemp()
static char *mktemp(char *pattern)
{
   char *end = strchr(pattern,'\0') ;
   if (!end || strcmp(end-6,"XXXXXX") != 0)
      {
      errno = EINVAL ;
      return 0 ;
      }
   Fr_sprintf(end-5,6,"%X.%3.03X",(getpid()>>12)&0xF,(getpid()&0xFFF)) ;
   char disambig = 'a' ;
   do {
      end[-6] = disambig ;
      int handle ;
      int attrib = _A_NORMAL ;
      int status = _dos_creatnew(pattern,attrib,&handle) ;
      close(handle) ;
      unlink(pattern) ;
      if (status == 0)
	 return pattern ;
      } while (++disambig <= 'z') ;
   // if we get to this point, all of the possibilities 'a'-'z' already exist
   return 0 ;
}
#endif /* FrNEED_MKTEMP && !FrHAVE_TEMPNAM */

/************************************************************************/
/************************************************************************/

char *FrTempFile(const char *basename, const char *tempdir)
{
   if (!tempdir)
      {
      tempdir = getenv("TMPDIR") ;
      if (!tempdir)
	 tempdir = getenv("TEMP") ;
      if (!tempdir)
	 tempdir = getenv("TMP") ;
      }
   if (!tempdir || !*tempdir)
      tempdir = "." ;
   if (!basename || !*basename)
      basename = "tmp" ;
#if defined(FrHAVE_TEMPNAM) && !defined(FrHAVE_MKSTEMP)
   char *filename = tempnam(tempdir,basename) ;
   if (!filename)
      FrWarning("unable to generate temporary file's name\n"
		"\t(bad parameters or out of memory)") ;
#ifndef FrREPLACE_MALLOC
   else
      filename = FrDupString(filename) ;
#endif /* FrREPLACE_MALLOC */
#else
   size_t baselen = strlen(basename) ;
   size_t templen = strlen(tempdir) ;
   size_t len = baselen + templen + 8 ;
   char *filename = FrNewN(char,len) ;
   if (filename)
      {
      memcpy(filename,tempdir,templen) ;
      if (tempdir[templen-1] != '/'
#ifdef FrMSDOS_PATHNAMES
	  && tempdir[len-1] != '\\'
#endif /* FrMSDOS_PATHNAMES */
	 )
	 {
	 filename[templen++] = '/' ;
	 }
      memcpy(filename+templen,basename,baselen) ;
      memcpy(filename+templen+baselen,"XXXXXX",7) ;
#ifdef FrHAVE_MKSTEMP
      int fd = mkstemp(filename) ;
      if (fd == EOF)
	 *filename = '\0' ;
      else
	 close(fd) ;
#else
      mktemp(filename) ;
#endif /* FrHAVE_MKSTEMP */
      if (!*filename)
	 FrWarningVA("unable to generate a unique temporary filename from %s!",
		     basename) ;
      }
   else
      FrNoMemory("while generating a temporary filename") ;
#endif /* FrHAVE_TEMPNAM */
   return filename ;
}

//----------------------------------------------------------------------

char *FrForceFilenameExt(const char *filename, const char *ext)
{
   if (!filename)
      filename = "" ;
   size_t extlen = ext ? strlen(ext) : 0 ;
   size_t namelen = strlen(filename) ;
   char *fullname = FrNewN(char,namelen+extlen+2) ;
   memcpy(fullname,filename,namelen+1) ;
   // strip off any existing extension
   char *slash = strrchr(fullname,'/') ;
#ifdef FrMSDOS_PATHNAMES
   char *backslash = strrchr(fullname,'\\') ;
   if (backslash && (!slash || backslash > slash))
      slash = backslash ;
#endif /* FrMSDOS_PATHNAMES */
   if (!slash)
      slash = fullname ;
   char *period = strrchr(slash,'.') ;
   if (period)
      *period = '\0' ;
   // and add the new extension
   if (ext)
      {
      strcat(fullname,".") ;
      strcat(fullname,ext) ;
      }
   return fullname ;
}

//----------------------------------------------------------------------

char *FrAddDefaultPath(const char *filename, const char *default_dir)
{
   if (!filename)
      return 0 ;
   if (!default_dir)
      default_dir = "." ;
   if (*filename && *filename != '/'
#ifdef FrMSDOS_PATHNAMES
       && *filename != '\\' && filename[1] != ':'
#endif /* FrMSDOS_PATHNAMES */
      )
      {
      size_t namelen = strlen(filename) + 1 ;
      size_t dirlen = strlen(default_dir) ;
      char *fullname = FrNewN(char,namelen+dirlen+1) ;
      if (fullname)
	 {
	 memcpy(fullname,default_dir,dirlen) ;
	 const char *last = &fullname[dirlen-1] ;
	 if (*last != '/'
#ifdef FrMSDOS_PATHNAMES
	     && *last != '\\' && *last != ':'
#endif /* FrMSDOS_PATHNAMES */
	    )
	    {
	    fullname[dirlen++] = '/' ;
	    }
	 memcpy(fullname+dirlen,filename,namelen) ;
	 return fullname ;
	 }
      }
   return FrDupString(filename) ;
}

//----------------------------------------------------------------------

static const char *find_last_component(const char *filepath)
{
   if (!filepath)
      return 0 ;
   const char *end = strchr(filepath,'\0')-1 ;
   const char *start = filepath ;
#ifdef FrMSDOS_PATHNAMES
   if (*filepath && filepath[1] == ':')
      start = filepath + 2 ;
#endif /* FrMSDOS_PATHNAMES */
   while (end > start && *end != '/'
#ifdef FrMSDOS_PATHNAMES
	  && *end != '\\'
#endif /* FrMSDOS_PATHNAMES */
	 )
      end-- ;
   if (*end == '/' || *end == '\\')
      end++ ;
   return end ;
}

//----------------------------------------------------------------------

static char *strip_last_component(const char *filepath,
				  bool keep_slash = false)
{
   const char *end = find_last_component(filepath) ;
   if (!keep_slash && end > filepath && (end[-1] == '/' || end[-1] == '\\'))
      end-- ;
   int len = end-filepath ;
   if (len == 0)
      {
      len = 1 ;
      filepath = "." ;
      }
   char *path = FrNewN(char,len+1) ;
   if (path)
      {
      memcpy(path,filepath,len) ;
      path[len] = '\0' ;
      }
   return path ;
}

//----------------------------------------------------------------------

static bool create_directory(const char *pathname)
{
   if (FrIsWriteableDirectory(pathname)) // does directory already exist?
      return true ;			// if yes, we were successful
   char *parent = strip_last_component(pathname) ;
   if (parent && *parent)
      {
      // does parent directory exist?
      // if not, try to create it
      if (!FrIsWriteableDirectory(parent) &&
	  !create_directory(parent))
	 {
	 FrFree(parent) ;
	 return false ;			// bail out if unable to create parent
	 }
      }
   FrFree(parent) ;
   if (*find_last_component(pathname) == '\0') // trailing slash?
      return true ;
   return mkdir(pathname
#if !defined(__MSDOS__) && !defined(__WATCOMC__) && !defined(_MSC_VER)
		,FrMKDIR_MODE
#endif /* !__MSDOS__ && !__WATCOMC__ && !_MSC_VER */
	       ) != -1 ;
}

//----------------------------------------------------------------------

FILE *FrOpenFile(const char *filename, bool write, bool force_creation)
{
   if (force_creation && !FrFileWriteable(filename))
      {
      char *dir = strip_last_component(filename) ;
      if (dir && *dir)
	 {
	 if (!create_directory(dir))
	    {
	    FrFree(dir) ;
	    return 0 ;			// must fail if unable to create dir
	    }
	 }
      FrFree(dir) ;
      }
   return fopen(filename,write ? fopen_append_mode : fopen_read_mode) ;
}

//----------------------------------------------------------------------

bool FrCreatePath(const char *dirpath)
{
   return create_directory(dirpath) ;
}

//----------------------------------------------------------------------

char *FrFileDirectory(const char *filename)
{
   return strip_last_component(filename,true) ;
}

//----------------------------------------------------------------------

const char *FrFileBasename(const char *filepath)
{
   return find_last_component(filepath) ;
}

//----------------------------------------------------------------------

const char *FrFileExtension(const char *filepath)
{
   const char *tail = find_last_component(filepath) ;
   const char *extension ;
   if (tail && *tail)
      {
      const char *dot = strrchr(tail,'.') ;
      if (dot)
	 extension = dot + 1 ;
      else
	 extension = strchr(tail,'\0') ;
      }
   else
      extension = strchr(filepath,'\0') ;
   return extension ;
}

//----------------------------------------------------------------------

int Fr_fsync(int fd)
{
#if defined(__WINDOWS__) || defined(__NT__)
   (void)fd ;
   return flushall() ;
#elif defined(__MSDOS__) || (defined(__WATCOMC__) && __WATCOMC__ < 1100) || defined(_MSC_VER)
   union REGS regs ;

   regs.h.ah = 0x68 ;
#ifdef __WATCOMC__
   regs.w.bx = (short)fd ;
#else
   regs.x.bx = fd ;
#endif /* __WATCOMC__ */
   intdos(&regs,&regs) ;
   return regs.x.cflag ? -1 : 0 ;
#else
   return fsync(fd) ;
#endif /* __WINDOWS__ || __NT__ */
}

//----------------------------------------------------------------------

static void show_error(off_t count, off_t totalsize)
{
   if (errno == EIO)
      FrWarningVA("I/O error after %lu of %lu bytes were written",
		  (unsigned long)count,(unsigned long)totalsize) ;
#ifdef ENOSPC
   else if (errno == ENOSPC)
      FrWarningVA("disk full after %lu of %lu bytes were written",
		  (unsigned long)count,(unsigned long)totalsize) ;
#endif /* ENOSPC */
#ifdef EPIPE
   else if (errno == EPIPE)
      FrWarningVA("broken pipe after %lu of %lu bytes were written",
		  (unsigned long)count,(unsigned long)totalsize) ;
#endif /* EPIPE */
#ifdef EFBIG
   else if (errno == EFBIG)
      FrWarningVA("file exceeded maximum allowable size after %lu of %lu bytes"
		  " were written",
		  (unsigned long)count,(unsigned long)totalsize) ;
#endif /* EFBIG */
#ifdef EROFS
   else if (errno == EROFS)
      FrWarningVA("attempted to write to read-only file system") ;
#endif
#ifdef EDQUOT
   else if (errno == EDQUOT)
      FrWarningVA("disk quota exceeded after %lu of %lu bytes were written",
		  (unsigned long)count,(unsigned long)totalsize) ;
#endif /* EDQUOT */
   else
      FrWarningVA("write failed after %lu of %lu bytes were written"
		  " (errno=%d)",
		  (unsigned long)count,(unsigned long)totalsize,(int)errno) ;
   return ;
}

//----------------------------------------------------------------------

bool Fr_write(int fd, const void *addr, size_t totalsize,
	      bool report_errors)
{
   const char *address = (char*)addr ;
   size_t remainder = totalsize ;
   do {
      errno = 0 ;
      size_t written = ::write(fd,address,remainder) ;
      if (written == (size_t)~0)
	 {
	 if (errno == EINTR || errno == EAGAIN)
	    continue ;
	 if (report_errors)
	    show_error(totalsize-remainder,totalsize) ;
	 return false ;
	 }
      remainder -= written ;
      address += written ;
      } while (remainder > 0) ;
   return true ;
}

//----------------------------------------------------------------------

bool Fr_fwrite(const void *addr, off_t totalsize, FILE *fp)
{
   if (!fp)
      return false ;
   // even under 64-bit Linux, fwrite chokes if given more than 2GiB to write
   //   in one shot, so break up the buffer write into smaller pieces as needed
   off_t count = 0 ;
   const char *address = (char*)addr ;
#define BLOCK_SIZE (16*1048576)
   for (off_t i = 0 ; i < totalsize / BLOCK_SIZE ; i++)
      {
      size_t written ;
      do {
         errno = 0 ;
         written = fwrite(address,1,BLOCK_SIZE,fp) ;
         } while (errno == EINTR || errno == EAGAIN) ;
      address += written ;
      count += written ;
      if (written < BLOCK_SIZE)
	 {
	 show_error(count,totalsize) ;
	 return false ;
	 }
      }
   size_t remainder = totalsize - count ;
   size_t written ;
   do {
      errno = 0 ;
      written = fwrite(address,1,remainder,fp) ;
      } while (errno == EINTR || errno == EAGAIN) ;
   count += written ;
   if (written < remainder)
      show_error(count,totalsize) ;
   return count == totalsize ;
}

//----------------------------------------------------------------------

off_t FrFileSize(FILE *fp)
{
   if (!fp)
      return 0 ;
   // note: while fstat() could be used here, this version will reflect the
   //   correct size even if the file has been written to without the directory
   //   entry being updated (as happens, e.g. under Windoze)
   int fd = fileno(fp) ;
   off_t pos = fdtell(fd) ;
   off_t end = lseek(fd,0L,SEEK_END) ;	// find file's size
   lseek(fd,pos,SEEK_SET) ;		// return to original position
   return end ;
}

//----------------------------------------------------------------------

off_t FrFileSize(const char *filename)
{
   if (!filename || !*filename)
      return 0 ;
   int fd = open(filename,O_RDONLY|O_BINARY) ;
   if (fd == EOF)
      return 0 ;
   // note: while fstat() could be used here, this version will reflect the
   //   correct size even if the file has been written to without the directory
   //   entry being updated (as happens, e.g. under Windoze)
   off_t end = lseek(fd,0L,SEEK_END) ;	// find file's size
   close(fd) ;
   return end ;
}

//----------------------------------------------------------------------

bool FrGetDelimited(FILE *in, char *buf, size_t bufsize,
		      char delim)
{
   if (bufsize == 0)
      return false ;
   bufsize-- ;			// reserve space for NUL terminator
   int c ;
   while ((c = fgetc(in)) != EOF && c != delim && bufsize > 0)
      {
      *buf++ = (char)c ;
      bufsize-- ;
      }
   *buf = '\0' ;
   if (c != EOF)
      {
      ungetc(c,in) ;		// put back the delimiter
      return true ;
      }
   else
      return false ;
}

//----------------------------------------------------------------------

FILE *FrOpenMaybeCompressedInfile(const char *file, bool &piped)
{
   piped = false ;
   FILE *fp ;
   if (!file || !*file)
      return 0 ;
   else if (strcmp(file,"-") == 0)
      return stdin ;
#if defined(unix) || defined(__linux__) || defined(__GNUC__)
   char signature[8] ;
   FILE *sigfp = fopen(file,"rb") ;
   if (!sigfp)
      return 0 ;
   int siglen = fread(signature,1,sizeof(signature),sigfp) ;
   fclose(sigfp) ;
   size_t len = strlen(file) ;
   if ((siglen > 3 && signature[0] == '\x1F' && signature[1] == '\x8B' &&
	signature[2] == 8 && (signature[3] & 0xE0) == 0)
//       || (len > 3 && strcmp(file+len-3,".gz") == 0))
      )
      {
      char *cmd = Fr_aprintf("gunzip -c %s",file) ;
      fp = popen(cmd,"r") ;
      if (fp)
	 piped = true ;
      FrFree(cmd) ;
      }
   else if ((siglen > 3 && signature[0] == 'B' && signature[1] == 'Z' &&
	     signature[2] == 'h' && isdigit(signature[3]))
//	    || (len > 4 && strcmp(file+len-4,".bz2") == 0)
//	    || (len > 3 && strcmp(file+len-3,".bz") == 0))
      )
      {
      char *cmd = Fr_aprintf("bzip2 -dc %s",file) ;
      fp = popen(cmd,"r") ;
      if (fp)
	 piped = true ;
      FrFree(cmd) ;
      }
   else if ((siglen > 4 && signature[0] == '\xFD' && signature[1] == '7' &&
	     signature[2] == 'z' && signature[3] == 'X' &&
	     signature[4] == 'Z')
//	    || (len > 3 && strcmp(file+len-3,".xz") == 0))
      )
      {
      char *cmd = Fr_aprintf("xz -dc %s",file) ;
      fp = popen(cmd,"r") ;
      if (fp)
         piped = true ;
      FrFree(cmd) ;
      }
   else if (len > 5 && strcmp(file+len-5,".lzma") == 0)
      {
      char *cmd = Fr_aprintf("lzma -dc %s",file) ;
      fp = fopen(cmd,"r") ;
      if (fp)
         piped = true ;
      FrFree(cmd) ;
      }
   else
#endif
      fp = fopen(file,fopen_read_mode) ;
   return fp ;
}

//----------------------------------------------------------------------

bool FrCloseMaybeCompressedInfile(FILE *fp, bool piped)
{
   errno = 0 ;
   if (fp == stdin)
      return true ;
#if defined(unix) || defined(__linux__) || defined(__GNUC__)
   if (piped)
      {
      if (fp)
	 pclose(fp) ;
      }
   else
#endif
      if (fp && fp != stdin)
	 fclose(fp) ;
   return (errno == 0) ;
}

//----------------------------------------------------------------------

FILE *FrOpenMaybeCompressedOutfile(const char *file, bool &piped)
{
   piped = false ;
   FILE *fp ;
#if defined(unix) || defined(__linux__) || defined(__GNUC__)
   size_t len = strlen(file) ;
   if (len > 3 && strcmp(file+len-3,".gz") == 0)
      {
      char *cmd = Fr_aprintf("gzip -c9f >%s",file) ;
      fp = popen(cmd,"w") ;
      if (fp)
	 piped = true ;
      FrFree(cmd) ;
      }
   else if ((len > 4 && strcmp(file+len-4,".bz2") == 0) ||
	    (len > 3 && strcmp(file+len-3,".bz") == 0))
      {
      char *cmd = Fr_aprintf("bzip2 >%s",file) ;
      fp = popen(cmd,"w") ;
      if (fp)
	 piped = true ;
      FrFree(cmd) ;
      }
   else if (len > 3 && strcmp(file+len-3,".xz") == 0)
      {
      char *cmd = Fr_aprintf("xz -cz >%s",file) ;
      fp = popen(cmd,"w") ;
      if (fp)
         piped = true ;
      FrFree(cmd) ;
      }
   else if (len > 5 && strcmp(file+len-5,".lzma") == 0)
      {
      char *cmd = Fr_aprintf("lzma -cz >%s",file) ;
      fp = popen(cmd,"w") ;
      if (fp)
         piped = true ;
      FrFree(cmd) ;
      }
   else
#endif
   if (strcmp(file,"-") == 0)
      fp = stdout ;
   else
      fp = fopen(file,FrFOPEN_WRITE_MODE) ;
   return fp ;
}

//----------------------------------------------------------------------

bool FrCloseMaybeCompressedOutfile(FILE *fp, bool piped)
{
   errno = 0 ;
#if defined(unix) || defined(__linux__) || defined(__GNUC__)
   if (piped)
      {
      if (fp)
	 pclose(fp) ;
      }
   else
#endif
      if (fp && fp != stdout)
	 fclose(fp) ;
   return (errno == 0) ;
}

//----------------------------------------------------------------------

// end of file frfilutl.cpp //
