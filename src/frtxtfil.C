/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01.							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frtxtfil.cpp		text-file reading functions		*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1999,2001,2009,2012,2015				*/
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

#include <fcntl.h>
#include <string.h>	// needed by RedHat 7.1
#if defined(unix) || defined(__linux__) || defined(__GNUC__)
#  include <unistd.h>   // for SEEK_SET
#endif /* unix */
#if defined(__WATCOMC__) || defined(__MSDOS__) || defined(__NT__)
#include <io.h>
#include <memory.h>
#endif /* __WATCOMC__ || __MSDOS__ || __NT__ */
#include "framerr.h"
#include "frfilutl.h"

/************************************************************************/
/*	Manifest Constants						*/
/************************************************************************/

#ifndef O_BINARY
#  define O_BINARY 0
#endif /* !O_BINARY */

/************************************************************************/
/*	Methods for class FrITextFile					*/
/************************************************************************/

FrITextFile::FrITextFile(int filedesc, unsigned long start, unsigned long end)
{
   init(filedesc,start,end) ;
   return ;
}

//----------------------------------------------------------------------

FrITextFile::FrITextFile(const char *filename, unsigned long start,
			 unsigned long end)
{
   init(open(filename,O_RDONLY | O_BINARY),start,end) ;
   return ;
}

//----------------------------------------------------------------------

void FrITextFile::init(int filedesc, unsigned long start, unsigned long end)
{
   fd = filedesc ;
   filestart = start ;
   bufoffset = start ;
   bufpos = 0 ;
   if (fd != EOF)
      {
      if (end != 0 && end != start)
	 fileend = end ;
      else
	 fileend = lseek(fd,0L,SEEK_END) ;
      int count = 2*BUFSIZ ;
      (void)lseek(fd,filestart,SEEK_SET) ;
      if ((unsigned long)count > fileend - filestart)
	 count = fileend - filestart ;
      bufsize = read(fd,buffer,count) ;
      }
   else
      {
      fileend = 0 ;
      bufsize = 0 ;
      }
   bufend = &buffer[bufsize] ;
   buffer[bufsize] = '\n' ;
   return ;
}

//----------------------------------------------------------------------

FrITextFile::~FrITextFile()
{
   close() ;
   return ;
}

//----------------------------------------------------------------------

void FrITextFile::close()
{
   if (fd != EOF && filestart == 0)	// don't close if part of a file cache
      ::close(fd) ;
   fd = EOF ;
   return ;
}

//----------------------------------------------------------------------

char *FrITextFile::getline()
{
   if (bufoffset + bufpos >= fileend)	// at end of file?
      return 0 ;			//   nothing more to read
   char *line = &buffer[bufpos] ;
   char *end = line ;
   while (*end != '\n')
      end++ ;
   if (end >= bufend)			// does line run past end of buffer?
      {
      // line runs past end of current buffer, so copy to start and get more
      int count = bufsize - bufpos ;
      bufoffset += bufpos ;
      memcpy(buffer,line,count) ;	// copy partial line to start of buffer
      bufpos = (size_t)count ;		// figure out how much more to read
      count = sizeof(buffer)-1 - count ;
      line = buffer ;
      if ((unsigned long)count > fileend - bufoffset - bufpos)
	 count = fileend - bufoffset - bufpos ;
      if (count)			// if data left in file, read it
	 {
	 count = read(fd, buffer+bufpos, count) ;
	 if (count < 0)
	    {
	    FrWarning("unexpected EOF (truncated file?)") ;
	    count = 0 ;
	    }
	 }
      bufsize = bufpos + count ;	// update buffer info
      end = &buffer[bufpos] ;
      buffer[bufsize] = '\n' ;
      bufend = &buffer[bufsize] ;
      while (*end != '\n')		// scan for end of line
	 end++ ;
      if (end >= bufend && count > 0)
	 {				// uh oh, line longer than buffer!
	 bufpos = bufsize - 1 ;		// arbitrarily split the line....
	 buffer[bufpos] = '\0' ;	// force proper string termination
	 return line ;
	 }
      }
   bufpos = end - buffer + 1 ;
   // zap trailing carriage returns and newlines
   while (end >= line)
      {
      if (*end == '\n' || *end == '\r')
	 *end-- = '\0' ;
      else
	 break ;
      }
   return line ;
}

//----------------------------------------------------------------------

bool FrITextFile::eof() const
{
   if (bufsize == 0)
      return true ;
   return (bufoffset + bufpos) >= fileend ;
}

//----------------------------------------------------------------------

bool FrITextFile::rewind()
{
   if (bufoffset != filestart)
      {
      if ((unsigned long)lseek(fd,filestart,SEEK_SET) != filestart)
	 return false ;
      bufoffset = filestart ;
      int count = sizeof(buffer)-1 ;
      if ((unsigned long)count > fileend - filestart)
	 count = fileend - filestart ;
      bufsize = read(fd,buffer,count) ;
      bufend = &buffer[bufsize] ;
      buffer[bufsize] = '\n' ;
      }
   bufpos = 0 ;
   return true ;
}

//----------------------------------------------------------------------

bool FrITextFile::seek(unsigned long position)
{
   unsigned long file_size = fileend - filestart ;
   if (position < file_size)
      {
      unsigned long currofs = bufoffset - filestart ;
      if (position >= currofs && bufsize >= BUFSIZ &&
	  position < currofs + bufsize - BUFSIZ)
	 {
	 bufpos = position - currofs ;
	 return true ;
	 }
      bufpos = 0 ;
      bufoffset = position + filestart ;
      if ((unsigned long)lseek(fd,bufoffset,SEEK_SET) == bufoffset)
	 {
	 int count = sizeof(buffer)-1 ;
	 if ((unsigned long)count > file_size - position)
	    count = file_size - position ;
	 bufsize = read(fd,buffer,count) ;
	 bufend = &buffer[bufsize] ;
	 buffer[bufsize] = '\n' ;
	 return true ;
	 }
      bufsize = 0 ;
      }
   return false ;
}

/************************************************************************/
/************************************************************************/


// end of file frtxtfil.cpp //
