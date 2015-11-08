/************************************************************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File fistruct.cpp	class FrStruct reader				*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,1998,2001,2009,2015		*/
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

#if defined(__GNUC__)
#  pragma implementation "frstruct.h"
#endif

#include "frstruct.h"
#include "frutil.h"
#include "frpcglbl.h"

/************************************************************************/
/*    Global data for this module					*/
/************************************************************************/

static const char errmsg_malformed_form[]
      = "malformed Lisp-style #S() form" ;

/************************************************************************/
/*    Global variables for class FrStruct				*/
/************************************************************************/

FrObject *read_FrStruct(istream &input, const char *) ;
FrObject *string_to_FrStruct(const char *&input, const char *) ;
static bool verify_FrStruct(const char *&input, const char *, bool) ;

FrReader FrStruct::reader(string_to_FrStruct, read_FrStruct, verify_FrStruct,
			  FrREADER_LEADIN_LISPFORM,"S") ;

/**********************************************************************/
/*    Input functions						      */
/**********************************************************************/

FrObject *read_FrStruct(istream &input, const char *)
{
   char ch ;
   FrStruct *result ;

   bool widechar = read_extended_strings(false) ; // use Lisp compatibility
   if (FrSkipWhitespace(input) == '#')
      {
      input >> ch ;	// consume the '#'
      ch = trunc2char(input.peek()) ;
      if (Fr_toupper(ch) == 'S')
	 input >> ch ;			// consume the 'S'
      }
   input >> ch ;
   if (ch == '(')
      {
      FrObject *obj = read_FrObject(input) ;
      if (!obj || !obj->symbolp())
	 {
	 FrWarning(errmsg_malformed_form) ;
	 read_extended_strings(widechar) ;
	 return 0 ;
	 }
      result = new FrStruct((FrSymbol*)obj) ;
      while ((ch = FrSkipWhitespace(input)) != ')')
	 {
	 if (ch == ':')
	    input >> ch ;		// consume the colon
	 FrObject *name = read_FrObject(input) ;
	 if (!name || !name->symbolp())
	    {
	    FrWarning(errmsg_malformed_form) ;
	    read_extended_strings(widechar) ;
	    return 0 ;
	    }
	 obj = read_FrObject(input) ;
	 result->putnew((FrSymbol*)name,obj) ;
	 }
      if (FrSkipWhitespace(input) == ')')
	 input >> ch ;			// consume the paren
      else
	 FrWarning(errmsg_malformed_form) ;
      }
   else
      {
      FrWarning(errmsg_malformed_form) ;
      result = 0 ;
      }
   read_extended_strings(widechar) ;
   return result ;
}

//----------------------------------------------------------------------

FrObject *string_to_FrStruct(const char *&input, const char *)
{
   FrStruct *result ;

   bool widechar = read_extended_strings(false) ; // use Lisp compatibility
   if (FrSkipWhitespace(input) == '#')
      {
      input++ ;				// consume the '#'
      if (Fr_toupper(*input) == 'S')
	 input++ ;			// consume the 'S'
      }
   if (*input++ == '(')
      {
      FrObject *obj = string_to_FrObject(input) ;
      if (!obj || !obj->symbolp())
	 {
	 FrWarning(errmsg_malformed_form) ;
	 read_extended_strings(widechar) ;
	 return 0 ;
	 }
      result = new FrStruct((FrSymbol*)obj) ;
      char ch ;
      while ((ch = FrSkipWhitespace(input)) != ')')
	 {
	 if (ch == ':')
	    input++ ;			// consume the colon
	 FrObject *name = string_to_FrObject(input) ;
	 if (!obj || !obj->symbolp())
	    {
	    FrWarning(errmsg_malformed_form) ;
	    read_extended_strings(widechar) ;
	    return 0 ;
	    }
	 obj = string_to_FrObject(input) ;
	 result->putnew((FrSymbol*)name,obj) ;
	 }
      if (FrSkipWhitespace(input) == ')')
	 input++ ;			// consume the paren
      else
	 FrWarning(errmsg_malformed_form) ;
      }
   else
      {
      FrWarning(errmsg_malformed_form) ;
      result = 0 ;
      }
   read_extended_strings(widechar) ;
   return result ;
}

//----------------------------------------------------------------------

static bool verify_FrStruct(const char *&input, const char *, bool)
{
   bool widechar = read_extended_strings(false) ; // use Lisp compatibility
   if (FrSkipWhitespace(input) == '#')
      {
      input++ ;				// consume the '#'
      if (Fr_toupper(*input) == 'S')
	 input++ ;			// consume the 'S'
      }
   bool valid = valid_FrObject_string(input,true) ;
   read_extended_strings(widechar) ;
   return valid ;
}

// end of file fistruct.cpp //

