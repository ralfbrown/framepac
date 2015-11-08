/************************************************************************/
/*									*/
/*  FramepaC								*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frlist.cpp	class FrCons and class FrList			*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,1998,1999,2000,2001,2003,2004,	*/
/*		2009 Ralf Brown/Carnegie Mellon University		*/
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
#  pragma implementation "frlist.h"
#endif

#include "framerr.h"
#include "frreader.h"
#include "frlist.h"
#include "frutil.h"
#include "frpcglbl.h"

#ifdef FrSTRICT_CPLUSPLUS
#  include <string>
#else
#  include <string.h>
#endif /* FrSTRICT_CPLUSPLUS */

/************************************************************************/
/*    Global variables for classes FrCons and FrList			*/
/************************************************************************/

static FrList *read_List(istream &input, const char *) ;
static FrList *string_to_List(const char *&input, const char *) ;
static bool verify_List(const char *&input, const char *, bool) ;

static FrReader FrList_reader((FrReadStringFunc*)string_to_List,
			      (FrReadStreamFunc*)read_List,verify_List,'(') ;

extern int FramepaC_read_nesting_level ;

/************************************************************************/
/*	Methods for class FrList					*/
/************************************************************************/

FrReader *FrList::objReader() const
{
   return &FrList_reader ;
}

/************************************************************************/
/************************************************************************/

static void expected_right_paren(const FrList *list)
{
   size_t len = list->listlength() ;
   FrObject *listhead = list->subseq(0,5) ;
   char *printed = listhead->print() ;
   free_object(listhead) ;
   if (!printed)
      printed = FrDupString("") ;
   const char *cont = "" ;
   if (len > 6)
      {
      cont = " ..." ;
      strchr(printed,'\0')[-1] = '\0' ;
      }
   FrWarningVA("malformed list (expected right parenthesis),\n\tread %s%s",
	       printed,cont) ;
   FrFree(printed) ;
   return ;
}

/************************************************************************/
/************************************************************************/

static FrList *read_List(istream &input, const char *)
{
   FrList *list, *prev ;
   FrObject *curr ;

   list = prev = nullptr ;
   input.get() ;		       // discard the initial left parenthesis
   FramepaC_read_nesting_level++ ;
   while (FrSkipWhitespace(input) != ')' && !input.eof() && !input.fail())
      {
      FrObject *obj = read_FrObject(input) ;
      if (obj == symbolPERIOD && FrSkipWhitespace(input) != ')')
	 {
	 // period is not last item, so check if it's a dotted pair
	 curr = read_FrObject(input) ;
	 // was period second-to-last in list?
	 if (FrSkipWhitespace(input) == ')')
	    {
	    if (!list)
	       prev = list = new FrList(0) ;
	    prev->replacd(curr) ;
	    break ;
	    }
	 else
	    {
            if (!list)
	       prev = list = new FrList(obj) ;
	    else
	       {
	       obj = new FrList(obj) ;
	       prev->replacd(obj) ;
	       prev = (FrList*)obj ;
	       }
	    obj = curr ;
            }
	 }
      curr = new FrList(obj) ;
      if (!list)
	 list = (FrList*)curr ;
      else
	 prev->replacd(curr) ;
      prev = (FrList*)curr ;
      }
   if (input.get() != ')')
      expected_right_paren(list) ;
   if (--FramepaC_read_nesting_level <= 0 && FramepaC_read_associations)
      {
      FramepaC_read_associations->freeObject() ;
      FramepaC_read_associations = 0 ;
      }
   return list ;
}

//----------------------------------------------------------------------

static FrList *string_to_List(const char *&input, const char *)
{
   FrList *list, *prev ;
   FrObject *curr ;

   list = prev = nullptr ;
   input++ ;			       // consume initial left parenthesis
   FramepaC_read_nesting_level++ ;
   char c ;
   while ((c = FrSkipWhitespace(input)) != ')' && c != '\0')
      {
      FrObject *obj = string_to_FrObject(input) ;
      if (obj == symbolPERIOD && FrSkipWhitespace(input) != ')')
	 {
	 // period is not last item, so check if it's a dotted pair
	 curr = string_to_FrObject(input) ;
	 // was period second-to-last in list?
	 if ((c = FrSkipWhitespace(input)) == ')')
	    {
	    if (!list)
	       prev = list = new FrList(0) ;
	    prev->replacd(curr) ;
	    break ;
	    }
	 else
	    {
            if (!list)
	       prev = list = new FrList(obj) ;
	    else
	       {
	       obj = new FrList(obj) ;
	       prev->replacd(obj) ;
	       prev = (FrList*)obj ;
	       }
	    obj = curr ;
            }
	 }
      curr = new FrList(obj) ;
      if (!list)
	 list = (FrList*)curr ;
      else
	 prev->replacd(curr) ;
      prev = (FrList*)curr ;
      }
   if (c != ')')
      expected_right_paren(list) ;
   else
      input++ ;
   if (--FramepaC_read_nesting_level <= 0 && FramepaC_read_associations)
      {
      FramepaC_read_associations->freeObject() ;
      FramepaC_read_associations = 0 ;
      }
   return list ;
}

//----------------------------------------------------------------------

static bool verify_List(const char *&input, const char *, bool)
{
   input++ ;			       // consume initial left parenthesis
   char c ;
   while ((c = FrSkipWhitespace(input)) != ')' && c != '\0')
      {
      if (!valid_FrObject_string(input,true))
	 return false ;
      }
   if (c == ')')
      {
      input++ ;				// skip terminating right paren
      return true ;			//   and indicate success
      }
   else
      return false ;
}

// end of file filist.cpp //
