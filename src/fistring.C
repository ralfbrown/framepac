/************************************************************************/
/*									*/
/*  FramepaC								*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File: fistring.cpp	 	class FrString I/O			*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,1998,1999,2000,2001,2003,2009	*/
/*		 Ralf Brown/Carnegie Mellon University			*/
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

#if defined(__GNUC__)
#  pragma implementation "frstring.h"
#endif

#include "frctype.h"
#include "frreader.h"
#include "frstring.h"
#include "frpcglbl.h"

#ifdef FrSTRICT_CPLUSPLUS
#  include <iomanip>
#  include <iostream>
#else
#  include <iomanip.h>
#  include <iostream.h>
#endif

/************************************************************************/
/*	Forward Declarations						*/
/************************************************************************/

static FrObject *read_FrString(istream &input, const char *) ;
static FrObject *read_FrString16(istream &input, const char *) ;
static FrObject *read_FrString32(istream &input, const char *) ;
static FrObject *string_to_FrString(const char *&input, const char *) ;
static FrObject *string_to_FrString16(const char *&input, const char *) ;
static FrObject *string_to_FrString32(const char *&input, const char *) ;
static bool verify_FrString(const char *&input, const char *, bool) ;
static bool verify_FrString16(const char *&input, const char *, bool) ;
static bool verify_FrString32(const char *&input, const char *, bool) ;

static FrReader FrString_reader(string_to_FrString, read_FrString,
				verify_FrString,'"') ;
static FrReader FrString16_reader(string_to_FrString16, read_FrString16,
				  verify_FrString16, '\'') ;
static FrReader FrString32_reader(string_to_FrString32, read_FrString32,
				  verify_FrString32, '`') ;

/************************************************************************/
/*	Global Data							*/
/************************************************************************/

static const char stringQUOTE[] = "QUOTE" ;
static const char stringBACKQUOTE[] = "BACKQUOTE" ;

static const char errmsg_stringtoolong[]
    = "string literal too long--truncated and remainder skipped" ;
static const char errmsg_unterminated[]
    = "end of input reached before string was terminated\n\t(%.65s)" ;
static const char errmsg_wronglength2[]
    = "string literal (%.20s...) wrong length for 16-bit characters" ;
static const char errmsg_wronglength4[]
    = "string literal (%.20s...) wrong length for 32-bit characters" ;
static const char nomem_reading[]
    = "while reading a string literal" ;

/************************************************************************/
/*	Methods for class FrString					*/
/************************************************************************/

FrReader *FrString::objReader() const
{
   return &FrString_reader ;
}

/************************************************************************/
/************************************************************************/

static FrObject *read_FrString(istream &input, const char *)
{
   char terminator ;
   char buf[1024] ;
   char *str = nullptr ;
   int totalsize = 0 ;
   bool quoted = false ;
   char c = 0 ;

   input >> terminator ;      // get first non-whitespace character
   do {
#if 1
      input.get(buf,sizeof(buf),terminator) ;
      int gcount = input.gcount() ;
      if (gcount == 0)
	 {
	 if (input.eof() ) // || input.fail())
	    {
	    if (str)
	       str[totalsize] = '\0' ;
	    FrWarningVA(errmsg_unterminated,str) ;
	    break ;
	    }
	 else
	    {    // this should get the terminating delimiter....
	    input.clear() ;
	    input.get(buf[0])  ;
	    gcount = 1 ;
	    if (quoted)			// if quoted, continue reading string
	       {
	       input.get(buf+1,sizeof(buf)-1,terminator) ;
	       gcount += input.gcount() ;
	       }
	    }
	 }
#else
//      input.read(buf,1) ;
      int gcount ;
      if (input.peek() != EOF)
	 {
	 input.get(buf[0]) ;
	 gcount = 1 ;
	 }
      else
	 gcount = 0 ;
//cerr<<"got: "<<buf[0]<<" (0x"<<hex<<(((unsigned char)buf[0])&0xFF)<<")"<<endl;
      if (input.eof() || input.fail())
	 {
	 if (str)
	    str[totalsize] = '\0' ;
	 FrWarningVA(errmsg_unterminated,str) ;
	 break ;
	 }
#endif
      char *bufptr = buf ;
      int count = 0 ;
      while (gcount-- > 0)
	 {
         c = *bufptr++ ;
	 if (quoted)
            {
	    if (c != '\n')
	       buf[count++] = (c == '0') ? '\0' : c ;
	    quoted = false ;
	    c = '\0' ;   // ensure that quoted terminator doesn't end loop
            }
	 else if (c == '\\')
	    quoted = true ;
         else if (c != terminator)
	    buf[count++] = c ;
	 else
	    break ;
	 }
      char *newstr ;
      if ((newstr = (char*)FrRealloc(str,totalsize+count+1,true)) == 0)
	 {
	 FrNoMemory(nomem_reading) ;
	 if (str)
	    FrFree(str) ;
	 return 0 ;
	 }
      else
	 str = newstr ;
      memcpy(str+totalsize,buf,count) ;
      totalsize += count ;
      count = 0 ;
      } while (c != terminator) ;
   int width ;
   int numchars ;
   switch (terminator)
      {
      case '"':
	 return new FrString(str,totalsize,1,false) ;
      case '\'':
	 width = 2 ;
	 numchars = totalsize / 2 ;
	 if ((totalsize % 2) != 0)
	    FrWarningVA(errmsg_wronglength2,str) ;
	 break ;
      case '`':
	 width = 4 ;
	 numchars = totalsize / 4 ;
	 if ((totalsize % 4) != 0)
	    FrWarningVA(errmsg_wronglength4,str) ;
	 break ;
      default:  // can't happen
	 width = 0 ;   // avoid "uninit var" warning
	 numchars = 0 ;
	 break ;
      }
   return new FrString(str,numchars,width,false) ;
}

//----------------------------------------------------------------------

static FrObject *read_FrString16(istream &input, const char *digits)
{
   if (read_widechar_strings)
      return read_FrString(input,digits) ;
   else // read as Lisp (quote X) construct
      {
      input.get() ;  // consume the quote character
      return new FrList(FrSymbolTable::add(stringQUOTE),read_FrObject(input)) ;
      }
}

//----------------------------------------------------------------------

static FrObject *read_FrString32(istream &input, const char *digits)
{
   if (read_widechar_strings)
      return read_FrString(input,digits) ;
   else // read as Lisp (backquote X) construct
      {
      input.get() ;  // consume the backquote character
      return new FrList(FrSymbolTable::add(stringBACKQUOTE),
		      read_FrObject(input)) ;
      }
}

//----------------------------------------------------------------------

static FrObject *string_to_FrString(const char *&input, const char *)
{
   char c ;
   int count = 0 ;
   char terminator = *input++ ;
   register const char *in = input ;

   while ((c = *in++) != '\0' && c != terminator && count < INT_MAX)
      {
      if (c == '\\')
	 {
	 if ((c = *in++) == '\0')
	    break ;
	 else if (c == '\n')
	    continue ;
	 }
      count++ ;
      }
   if (c == '\0')
      FrWarningVA(errmsg_unterminated,input) ;
   else if (count >= INT_MAX)
      FrWarning(errmsg_stringtoolong) ;
   char *str ;
   if ((str = FrNewN(char,count+1)) == 0)
      {
      FrNoMemory(nomem_reading) ;
      return 0 ;
      }
   in = input ;		// back to start of string
   count = 0 ;
   while ((c = *in++) != '\0' && c != terminator && count < INT_MAX)
       {
       if (c == '\\')
	  {
	  if ((c = *in++) == '\0')
	     break ;
	  else if (c == '\n')
	     continue ;
          str[count++] = (c == '0') ? '\0' : c ;
	  }
       else
          str[count++] = c ;
       }
   input = c ? in : in-1 ;
   int width ;
   int numchars ;
   switch (terminator)
      {
      case '"':
	 return new FrString(str,count,1,false) ;
      case '\'':
	 width = 2 ;
	 numchars = count / 2 ;
	 if ((count % 2) != 0)
	    FrWarningVA(errmsg_wronglength2,str) ;
	 break ;
      case '`':
	 width = 4 ;
	 numchars = count / 4 ;
	 if ((count % 4) != 0)
	    FrWarningVA(errmsg_wronglength4,str) ;
	 break ;
      default:  // can't happen
	 width = 0 ;   // avoid "uninit var" warning
	 numchars = 0 ;
	 break ;
      }
   return new FrString(str,numchars,width,false) ;
}

//----------------------------------------------------------------------

static FrObject *string_to_FrString16(const char *&input, const char *digits)
{
   if (read_widechar_strings)
      return string_to_FrString(input,digits) ;
   else // read as Lisp (quote X) construct
      {
      input++ ;	// consume the quote character
      return new FrList(FrSymbolTable::add(stringQUOTE),
			 string_to_FrObject(input)) ;
      }
}

//----------------------------------------------------------------------

static FrObject *string_to_FrString32(const char *&input, const char *digits)
{
   if (read_widechar_strings)
      return string_to_FrString(input,digits) ;
   else // read as Lisp (backquote X) construct
      {
      input++ ;	// consume the backquote character
      return new FrList(FrSymbolTable::add(stringBACKQUOTE),
			 string_to_FrObject(input)) ;
      }
}

//----------------------------------------------------------------------

static bool verify_FrString(const char *&input, const char *, bool)
{
   char c ;
   int count = 0 ;
   char terminator = *input++ ;
   register const char *in = input ;

   while ((c = *in) != '\0' && c != terminator && count < INT_MAX)
      {
      in++ ;
      if (c == '\\')
	 {
	 if ((c = *in++) == '\0')
	    break ;
	 else if (c == '\n')
	    continue ;
	 }
      count++ ;
      }
   input = in ;
   if (c == '\0')			// string properly terminated?
      return false ;
   else if (count >= INT_MAX)		// string TOO long?
      return false ;
   input++ ;				// consume terminator
   return true ;
}

//----------------------------------------------------------------------

static bool verify_FrString16(const char *&input, const char *digits,
				bool strict)
{
   if (read_widechar_strings)
      return verify_FrString(input,digits,strict) ;
   else // read as Lisp (quote X) construct
      {
      input++ ;	// consume the quote character
      return valid_FrObject_string(input,strict) ;
      }
}

//----------------------------------------------------------------------

static bool verify_FrString32(const char *&input, const char *digits,
				bool strict)
{
   if (read_widechar_strings)
      return verify_FrString(input,digits,strict) ;
   else // read as Lisp (backquote X) construct
      {
      input++ ;	// consume the backquote character
      return valid_FrObject_string(input,strict) ;
      }
}

// end of file fistring.cpp //
