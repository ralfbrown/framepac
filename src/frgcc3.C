/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frgcc3.cpp		workarounds for GCC 3.x breakage	*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 2002,2003,2009,2013,2015				*/
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

#include "frfilutl.h"

#if defined(unix) || defined(__linux__) || defined(__GNUC__)
#  include <unistd.h>			// for read()
#elif defined(__MSDOS__) || defined(__WINDOWS__) || defined(__WATCOMC__)
#  include <io.h>			// for read()
#endif
#ifndef __WATCOMC__
#  ifdef FrSTRICT_CPLUSPLUS
#    include <streambuf>
#  else
#    include <streambuf.h>
#  endif /* FrSTRICT_CPLUSPLUS */
#endif /* !__WATCOMC__ */

#ifdef FrSTRICT_CPLUSPLUS
#  include <fstream>
#else
#  include <fstream.h>
#endif /* FrSTRICT_CPLUSPLUS */

/************************************************************************/
/************************************************************************/

#if __GNUC__ >= 3

class filedescbuf : public streambuf
   {
   private:
      int _fd ;
      char buffer[1] ;
   public:
      filedescbuf(int fd) { _fd = fd ; setg(buffer, buffer+1, buffer+1) ; }
      virtual int underflow() ;
      virtual int sync() ;
      virtual int overflow(int ch) ;
   } ;

//----------------------------------------------------------------------

int filedescbuf::underflow()
{
   if (gptr() < egptr())
      return *gptr() ;
//   if (::eof(_fd))
//      return EOF ;
   if (::read(_fd,buffer,sizeof(char)) <= 0)
      {
//      cerr << "fdbuf::underflow() --> EOF" << endl ;
      return EOF ;
      }
   setg(buffer, buffer, buffer+1) ;
//cerr<<"fdbuf::underflow() --> " << *gptr() << endl ;
   return *gptr() ;
}

//----------------------------------------------------------------------

int filedescbuf::overflow(int ch)
{
   if (pbase())
      sync() ;
   else
      setp(buffer, buffer + sizeof(buffer)) ;
   if (ch != EOF)
      {
      *pptr() = (char)ch ;
      pbump(1) ;
      }
   return ch ;
}

//----------------------------------------------------------------------

int filedescbuf::sync()
{
   if (pbase() && pptr() > pbase())
      {
      (void)::write(_fd, pbase(), pptr() - pbase()) ;
      setp(buffer, buffer + sizeof(buffer)) ;
      }
   return 0 ;
}

#endif /* __GNUC__ >= 3 */

/************************************************************************/
/************************************************************************/

istream *Fr_ifstream(int fd)
{
#if __GNUC__ >= 3
   // v3.0 breaks compatibility with lots of existing code in the
   //  name of compliance with a standard that probably doesn't
   //  prohibit the existing features....
   filedescbuf *fb_in = new filedescbuf(fd) ;
   return new istream(fb_in) ;
#else
   return new ifstream(fd) ;
#endif /* __GNUC__ */
}

//----------------------------------------------------------------------

ostream *Fr_ofstream(int fd)
{
#if __GNUC__ >= 3
   // v3.0 breaks compatibility with lots of existing code in the
   //  name of compliance with a standard that probably doesn't
   //  prohibit the existing features....
   filedescbuf *fb_out = new filedescbuf(fd) ;
   return new ostream(fb_out) ;
#else
   return new ofstream(fd) ;
#endif /* __GNUC__ */
}

// end of file frgcc3.cpp //

