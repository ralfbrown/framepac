/************************************************************************/
/*									*/
/*  FramepaC								*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File finumber.cpp	   FrNumber input				*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,1998,2000,2001,2003,2005,2009,	*/
/*		2010,2013,2015 Ralf Brown/Carnegie Mellon University	*/
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
#  pragma implementation "frnumber.h"
#endif

#include <stdlib.h>
#include <stdio.h>  // needed for EOF on some systems
#include "framerr.h"
#include "frnumber.h"
#include "frreader.h"
#include "frpcglbl.h"

/************************************************************************/
/*	Global variables from other modules		      		*/
/************************************************************************/

extern FrNumber *(*_FramepaC_make_Float)(const char *str, char **stopper) ;
extern FrNumber *(*_FramepaC_make_Rational)(long num, long denom) ;
extern FrInteger64 *(*_FramepaC_make_Int64)(const char *str, char **stopper,
					     long radix) ;
extern double (*_FramepaC_long_to_double)(long value) ;

/************************************************************************/
/*	Global data limited to this module			      	*/
/************************************************************************/

static const char errmsg_malformed_rational[] =
	"malformed rational number (zero or missing denominator)" ;

static FrNumber *string_to_Number(const char *&input,const char *) ;
static FrNumber *read_Number(istream &input, const char *) ;
static FrObject *read_Number_or_Symbol(istream &input, const char *) ;
static FrObject *string_to_Number_or_Symbol(const char *&input, const char*) ;
static FrObject *binary_to_Number(const char *&input,const char *) ;
static FrObject *read_binary(istream &input, const char *) ;
static FrObject *octal_to_Number(const char *&input, const char *) ;
static FrObject *read_octal(istream &input, const char *) ;
static FrObject *hex_to_Number(const char *&input, const char *) ;
static FrObject *read_hex(istream &input, const char *) ;
static FrObject *radix_to_Number(const char *&input, const char *digits) ;
static FrObject *read_radix(istream &input, const char *digits) ;

static bool verify_Number(const char *&input,const char *, bool strict) ;
static bool verify_Number_or_Symbol(const char *&input,const char *, bool);
static bool verify_binary(const char *&input,const char *, bool strict) ;
static bool verify_octal(const char *&input,const char *, bool strict) ;
static bool verify_hex(const char *&input,const char *, bool strict) ;
static bool verify_radix(const char *&input,const char *, bool strict) ;

static FrReader FrNumber_reader((FrReadStringFunc*)string_to_Number,
				(FrReadStreamFunc*)read_Number, verify_Number,
				FrREADER_LEADIN_CHARSET,"0123456789") ;
static FrReader SignedNum_reader(string_to_Number_or_Symbol,
				 read_Number_or_Symbol,
				 verify_Number_or_Symbol,
				 FrREADER_LEADIN_CHARSET,"+-") ;

static FrReader Binary_reader(binary_to_Number, read_binary, verify_binary,
			      FrREADER_LEADIN_LISPFORM,"B") ;
static FrReader Hex_reader(hex_to_Number, read_hex, verify_hex,
			   FrREADER_LEADIN_LISPFORM,"X") ;
static FrReader Octal_reader(octal_to_Number, read_octal, verify_octal,
			     FrREADER_LEADIN_LISPFORM,"O") ;
static FrReader Radix_reader(radix_to_Number, read_radix, verify_radix,
			     FrREADER_LEADIN_LISPFORM,"R") ;

/************************************************************************/
/*	Methods for class FrNumber					*/
/************************************************************************/

FrReader *FrNumber::objReader() const
{
   return &FrNumber_reader ;
}

/**********************************************************************/
/*    FrNumber reading functions				      */
/**********************************************************************/

static long atol(const char *buf, int radix)
{
   long value = 0 ;
   bool negative ;

   if (*buf == '-')
      {
      negative = true ;
      buf++ ;
      }
   else
      {
      negative = false ;
      if (*buf == '+')
	 buf++ ;
      }
   for ( ; *buf ; buf++)
      {
      int digit = *buf - '0' ;
      if (digit > 9)
	 {
	 int c = Fr_toupper(*buf) ;
	 digit = c - 'A' + 10 ;
	 }
      value = radix*value + digit ;
      }
   if (negative)
      return -value ;
   else
      return value ;
}

//----------------------------------------------------------------------

static bool is_radix_digit(int c, int radix)
{
   if (radix <= 10)
      return (bool)(c >= '0' && c <= radix + '0' - 1) ;
   else
      {
      if (c >= '0' && c <= '9')
	 return true ;
      c = Fr_toupper(c) - 'A' + 10 ;
      if (c < 10 || c >= radix)
	 return false ;
      else
	 return true ;
      }
}

//----------------------------------------------------------------------

static FrNumber *read_Number(istream &input, int radix,
			      const char *prefix = 0)
{
   char tempbuf[FrMAX_NUMSTRING_LEN+1] ;
   int count = 0 ;
   int ch ;

   if (prefix)
      {
      strncpy(tempbuf,prefix,sizeof(tempbuf)) ;
      tempbuf[sizeof(tempbuf)-1] = '\0' ;
      count = strlen(tempbuf) ;
      }
   ch = input.get() ;
   do {
      tempbuf[count++] = trunc2char(ch) ;
      } while (count < FrMAX_NUMSTRING_LEN && (ch = input.get()) != EOF &&
	       Fr_isdigit(ch)) ;
   if (radix == 10 && (ch == '.' || Fr_toupper(ch) == 'E') &&
      _FramepaC_make_Float)
      {
      if (ch == '.')
	 {
	 tempbuf[count++] = trunc2char(ch) ;
	 while (count < FrMAX_NUMSTRING_LEN && (ch = input.get()) != EOF &&
		Fr_isdigit(ch))
	    tempbuf[count++] = trunc2char(ch) ;
	 }
      // processing optional exponent
      if (Fr_toupper(ch) == 'E')
	 {
	 tempbuf[count++] = trunc2char(ch) ;
	 ch = input.peek() ;
	 if (ch == '+' || ch == '-')
	    input >> tempbuf[count++] ;
	 while (count < FrMAX_NUMSTRING_LEN && (ch = input.get()) != EOF &&
		Fr_isdigit(ch))
	    tempbuf[count++] = trunc2char(ch) ;
	 }
      if (ch != EOF)
	 input.putback(trunc2char(ch)) ; // read one char too many, so put it back...
      tempbuf[count] = '\0' ;	   // ensure proper string termination
      return _FramepaC_make_Float(tempbuf,0) ;
      }
   else if (ch == '/')		// Lisp-style rational?
      {
      tempbuf[count] = '\0' ;	// ensure proper string termination
      ch = input.peek() ;
      int denominator = 0 ;
      while (is_radix_digit(ch,radix))
	 {
	 input.get() ;
	 denominator = radix*denominator + (ch-'0') ;
	 ch = input.peek() ;
	 }
      if (denominator)
	 {
	 if (_FramepaC_make_Rational)
	    return _FramepaC_make_Rational(atol(tempbuf,radix),denominator) ;
	 }
      else
	 FrWarning(errmsg_malformed_rational) ;
      return new FrInteger(atol(tempbuf,radix)) ;
      }
   else
      {
      if (ch != EOF)
	 input.putback(trunc2char(ch)) ; // read one char too many, so put it back...
      tempbuf[count] = '\0' ;	   // ensure proper string termination
      // check whether the number can be represented without int64_t
      unsigned long val = 0 ;
      char *bufptr = tempbuf ;
      bool neg = false ;
      if (*bufptr == '+')
	 bufptr++ ;
      else if (*bufptr == '-')
	 {
	 neg = true ;
	 bufptr++ ;
	 }
      unsigned long limit_u = ULONG_MAX/radix ;
      unsigned long limit_s = LONG_MAX/radix ;
      while (*bufptr)
	 {
	 if ((neg && val > limit_s) || (!neg && val > limit_u))
	    {
	    // oops, number is too big for a 'long', so see if we have int64_t
	    if (_FramepaC_make_Int64)
	       return _FramepaC_make_Int64(tempbuf,0,radix) ;
	    else
	       return new FrInteger(neg ? LONG_MIN : ULONG_MAX) ;
	    }
	 val = (val * radix) + (*bufptr - '0') ;
	 bufptr++ ;
	 }
      return new FrInteger(atol(tempbuf,radix)) ;
      }
}

//----------------------------------------------------------------------

static FrNumber *read_Number(istream &input, const char *prefix)
{
   return read_Number(input,10,prefix) ;
}

//----------------------------------------------------------------------

FrObject *read_binary(istream &input, const char *)
{
   return read_Number(input,2) ;
}

//----------------------------------------------------------------------

FrObject *read_octal(istream &input, const char *)
{
   return read_Number(input,8) ;
}

//----------------------------------------------------------------------

FrObject *read_hex(istream &input, const char *)
{
   return read_Number(input,16) ;
}

//----------------------------------------------------------------------

FrObject *read_radix(istream &input, const char *digits)
{
   int radix ;
   if (digits)
      {
      radix = atol(digits,10) ;
      if (radix == 0)
	 radix = 10 ;
      else if (radix < 2)
	 radix = 2 ;
      else if (radix > 36)
	 radix = 36 ;
      }
   else
      radix = 10 ;
   return read_Number(input,radix) ;
}

//----------------------------------------------------------------------

static FrObject *read_Number_or_Symbol(istream &input, const char *digits)
{
   char c1, c2 ;

   // problems with putback under GCC 3, so we try to work around by taking
   //   advantage of the fact that we can pass additional characters to the
   //   function we'll be invoking
   c1 = trunc2char(input.get()) ;	    // read the first two characters
   c2 = trunc2char(input.peek()) ;
   if (digits && *digits)
      {
      input.putback(c1) ;		    // and restore the stream
      if (Fr_isdigit(c2))		    // was this actually a number?
	 return read_Number(input,digits) ;
      else
	 return FramepaC_read_unquoted_Symbol(input,digits) ;
      }
   else
      {
      const char *first = (c1 == '+' ? "+" : "-") ;
      if (Fr_isdigit(c2))		    // was this actually a number?
	 return read_Number(input,first) ;
      else
	 return FramepaC_read_unquoted_Symbol(input,first) ;
      }
}

//----------------------------------------------------------------------

static FrNumber *string_to_Number(const char *&input, int radix)
{
   char tempbuf[FrMAX_NUMSTRING_LEN+1] ;
   int count = 0 ;
   char ch ;
   const char *ptr = input ;

   do {
      tempbuf[count++] = *ptr++ ;
      } while (count < FrMAX_NUMSTRING_LEN && *ptr &&
	       is_radix_digit(*ptr,radix)) ;
   ch = *ptr ;
   if (radix == 10 && (ch == '.' || Fr_toupper(ch) == 'E') &&
       _FramepaC_make_Float)
      {
      // OK, we have a floating-point number
      char *stopper ;
      FrNumber *value = _FramepaC_make_Float(input,&stopper) ;
      if (stopper)
	 input = stopper ;
      return value ;
      }
   else if (ch == '/')		// Lisp-style rational?
      {
      tempbuf[count] = '\0' ;	// ensure proper string termination
      ptr++ ;			// consume the slash
      long denominator = 0 ;
      while (is_radix_digit(*ptr,radix))
	 {
	 denominator = radix*denominator + (*ptr-'0') ;
	 ptr++ ;
	 }
      input = ptr ;
      if (denominator)
	 {
	 if (_FramepaC_make_Rational)
	    return _FramepaC_make_Rational(atol(tempbuf,radix),denominator) ;
	 }
      else
	 FrWarning(errmsg_malformed_rational) ;
      return new FrInteger(atol(tempbuf,radix)) ;
      }
   else
      {
      tempbuf[count] = '\0' ;	 // ensure proper string termination
      input = ptr ;
      // check whether the number can be represented without int64_t
      unsigned long val = 0 ;
      char *bufptr = tempbuf ;
      bool neg = false ;
      if (*bufptr == '+')
	 bufptr++ ;
      else if (*bufptr == '-')
	 {
	 neg = true ;
	 bufptr++ ;
	 }
      unsigned long limit_u = ULONG_MAX/radix ;
      unsigned long limit_s = LONG_MAX/radix ;
      while (*bufptr)
	 {
	 if ((neg && val > limit_s) || (!neg && val > limit_u))
	    {
	    // oops, number is too big for a 'long', so see if we have int64_t
	    if (_FramepaC_make_Int64)
	       return _FramepaC_make_Int64(tempbuf,0,radix) ;
	    else
	       return new FrInteger(neg ? LONG_MIN : ULONG_MAX) ;
	    }
	 val = (val * radix) + (*bufptr - '0') ;
	 bufptr++ ;
	 }
      return new FrInteger(atol(tempbuf,radix)) ;
      }
}


//----------------------------------------------------------------------

static FrNumber *string_to_Number(const char *&input, const char *)
{
   return string_to_Number(input,10) ;
}

//----------------------------------------------------------------------

static FrObject *binary_to_Number(const char *&input, const char *)
{
   return string_to_Number(input,2) ;
}

//----------------------------------------------------------------------

static FrObject *octal_to_Number(const char *&input, const char *)
{
   return string_to_Number(input,8) ;
}

//----------------------------------------------------------------------

static FrObject *hex_to_Number(const char *&input, const char *)
{
//   input++ ;				// discard the leading 'X'
   return string_to_Number(input,16) ;
}

//----------------------------------------------------------------------

static FrObject *radix_to_Number(const char *&input, const char *digits)
{
//   input++ ;				// discard the leading 'R'
   int radix ;
   if (digits)
      {
      radix = atol(digits) ;
      if (radix == 0)
	 radix = 10 ;
      else if (radix < 2)
	 radix = 2 ;
      else if (radix > 36)
	 radix = 36 ;
      }
   else
      radix = 10 ;
   return string_to_Number(input,radix) ;
}

//----------------------------------------------------------------------

static FrObject *string_to_Number_or_Symbol(const char *&input,
					     const char *digits)
{
   if (Fr_isdigit(input[1]))
      return string_to_Number(input,digits) ;
   else
      return FramepaC_string_to_unquoted_Symbol(input,digits) ;
}

//----------------------------------------------------------------------

static bool verify_Number(const char *&input, int radix, bool strict)
{
   int count = 0 ;
   const char *ptr = input ;

   do {
      ptr++ ;
      } while (count++ < FrMAX_NUMSTRING_LEN && *ptr &&
	       is_radix_digit(*ptr,radix)) ;
   char ch = *ptr ;
   if (radix == 10 && (ch == '.' || Fr_toupper(ch) == 'E'))
      {
      // OK, we have a floating-point number
      char *stopper ;
      double value = strtod(input,&stopper) ; // convert the number
      (void)value ; // we don't actually need the value, so keep the compiler happy
      if (!stopper || stopper == input)
	 return false ;
      else
	 ptr = stopper ;
      }
   else if (ch == '/')		// Lisp-style rational?
      {
      ptr++ ;			// consume the slash
      while (is_radix_digit(*ptr,radix))
	 ptr++ ;
      }
   input = ptr ;
   return (!strict || *input) ;
}


//----------------------------------------------------------------------

static bool verify_Number(const char *&input, const char *, bool strict)
{
   return verify_Number(input,10,strict) ;
}

//----------------------------------------------------------------------

static bool verify_binary(const char *&input, const char *, bool strict)
{
   return verify_Number(input,2,strict) ;
}

//----------------------------------------------------------------------

static bool verify_octal(const char *&input, const char *, bool strict)
{
   return verify_Number(input,8,strict) ;
}

//----------------------------------------------------------------------

static bool verify_hex(const char *&input, const char *, bool strict)
{
   input++ ;				// discard the leading 'X'
   return verify_Number(input,16,strict) ;
}

//----------------------------------------------------------------------

static bool verify_radix(const char *&input, const char *digits,
			   bool strict)
{
   input++ ;				// discard the leading 'R'
   int radix ;
   if (digits)
      {
      radix = atol(digits) ;
      if (radix == 0)
	 radix = 10 ;
      else if (radix < 2)
	 radix = 2 ;
      else if (radix > 36)
	 radix = 36 ;
      }
   else
      radix = 10 ;
   return verify_Number(input,radix,strict) ;
}

//----------------------------------------------------------------------

static bool verify_Number_or_Symbol(const char *&input, const char *digits,
				      bool strict)
{
   if (Fr_isdigit(input[1]))
      return verify_Number(input,digits,strict) ;
   else
      return verify_Symbol(input,strict) ;
}

// end of file finumber.cpp //

