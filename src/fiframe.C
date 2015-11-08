/************************************************************************/
/*									*/
/*  FramepaC -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File fiframe.cpp		class FrFrame Input/Output functions	*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,1998,2001,2002,2009		*/
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

#include "frframe.h"
#include "framerr.h"
#include "frutil.h"
#include "frpcglbl.h"

static FrObject *read_Frame(istream &input,const char *) ;
static bool verify_Frame(const char *&input, const char *, bool strict) ;

FrReader FrFrame::reader(string_to_Frame,(FrReadStreamFunc*)read_Frame,
			  verify_Frame,'[') ;

/************************************************************************/
/*    Global data local to this module				        */
/************************************************************************/

static const char errmsg_facet_symbol[]
      = "malformed facet--name must be a symbol." ;
static const char errmsg_facet_malformed[]
      = "malformed facet or hit end of input/file before terminating ']'." ;
static const char errmsg_slot_symbol[]
      = "malformed slot--slot name must be a symbol." ;
static const char errmsg_slot_malformed[]
      = "malformed slot or hit end of input/file before terminating ']'." ;
static const char errmsg_frame_name[]
      = "malformed frame--name must be a symbol." ;
static const char errmsg_frame_malformed[]
      = "malformed frame or end of input/file reached." ;

bool read_as_VFrame = false ;

/************************************************************************/
/************************************************************************/

bool read_virtual_frames(bool frames_read_as_virtual)
{
   bool old = read_as_VFrame ;

   read_as_VFrame = frames_read_as_virtual ;
   return old ;
}

//----------------------------------------------------------------------

static void read_Facet(istream &input,FrFrame *frame,FrSymbol *slot)
{
   input.get() ;		    // consume the left bracket
   FrSymbol *facet ;
   facet = read_Symbol(input) ;	    // get facet name
   if (!facet || !facet->symbolp()) // the name must be a symbol
      {
      FrWarning(errmsg_facet_symbol) ;
      free_object(facet) ;
      return ;
      }
   frame->createFacet(slot,facet) ;
   char ch ;
   while ((ch = FrSkipWhitespace(input)) != 0 && ch != ']')
      {
      frame->addFillerNoCopy(slot,facet,read_FrObject(input)) ;
      }
   if (input.get() != ']')
      FrWarning(errmsg_facet_malformed) ;
}

//----------------------------------------------------------------------

static void read_Slot(istream &input,FrFrame *frame)
{
   input.get() ;		    // consume the left bracket
   FrSymbol *slot ;
   slot = read_Symbol(input) ;      // get slot name
   if (!slot || !slot->symbolp())   // the name must be a symbol
      {
      FrWarning(errmsg_slot_symbol) ;
      free_object(slot) ;
      return ;
      }
   frame->createSlot(slot) ;
   while (FrSkipWhitespace(input) == '[')
      read_Facet(input,frame,slot) ;
   int ch ;
   ch = input.get() ;		    // get first non-whitespace character
   if (ch != ']')		    // ch may be EOF
      FrWarning(errmsg_slot_malformed) ;
}

//----------------------------------------------------------------------

static FrObject *read_Frame(istream &input, const char *)
{
   input.get() ;		    // consume the initial left bracket
   FrSymbol *name ;
   name = read_Symbol(input) ;	    // read frame name
   if (!name || !name->symbolp())   // the name must be a symbol
      {
      FrWarning(errmsg_frame_name) ;
      free_object(name) ;
      return 0 ;
      }
   FrFrame *frame = find_vframe_inline(name) ;
   if (!frame)
      frame = (read_as_VFrame && FramepaC_new_VFrame)
	 	? FramepaC_new_VFrame(name) : new FrFrame(name) ;
   while (FrSkipWhitespace(input) == '[')
      read_Slot(input,frame) ;
   if (input.get() != ']')	    // next non-whitespace char may be EOF
      FrWarning(errmsg_frame_malformed) ;
   return frame ;
}

//----------------------------------------------------------------------

static void string_to_Facet(const char *&input,FrFrame *frame,FrSymbol *slot)
{
   FrSymbol *facet ;

   input++ ;			       // consume the left bracket
   facet = string_to_Symbol(input) ;   // get facet name
   if (!facet || !facet->symbolp())    // the name must be a symbol
      {
      FrWarning(errmsg_facet_symbol) ;
      return ;
      }
   frame->createFacet(slot,facet) ;
   char c ;
   while ((c = FrSkipWhitespace(input)) != ']' && c != '\0')
      {
      frame->addFillerNoCopy(slot,facet,string_to_FrObject(input)) ;
      }
   if (c != ']')
      FrWarning(errmsg_facet_malformed) ;
   else
      input++ ;
}

//----------------------------------------------------------------------

static void string_to_Slot(const char *&input,FrFrame *frame)
{
   FrSymbol *slot ;

   input++ ;				  // consume the left bracket
   slot = string_to_Symbol(input) ;	  // get slot name
   if (!slot || !slot->symbolp())	  // the name must be a symbol
      {
      FrWarning(errmsg_slot_symbol) ;
      return ;
      }
   frame->createSlot(slot) ;
   while (FrSkipWhitespace(input) == '[')
      string_to_Facet(input,frame,slot) ;
   if (*input != ']')
      FrWarning(errmsg_slot_malformed) ;
   else
      input++ ;
}

//----------------------------------------------------------------------

FrObject *string_to_Frame(const char *&input, const char *)
{
   FrFrame *frame ;
   FrSymbol *name ;

   input++ ;				// consume the initial left bracket
   name = string_to_Symbol(input) ;	// read frame name
   if (name && name->symbolp())        	// the name must be a symbol
      {
      frame = find_vframe_inline(name) ;
      if (!frame)
	 frame = (read_as_VFrame && FramepaC_new_VFrame)
	    	? FramepaC_new_VFrame(name) : new FrFrame(name) ;
      while (FrSkipWhitespace(input) == '[')
	 string_to_Slot(input,frame) ;
      if (*input == ']')
	 input++ ;
      else
	 FrWarning(errmsg_frame_malformed) ;
      return frame ;
      }
   else
      {
      FrWarning(errmsg_frame_name) ;
      return 0 ;
      }
}

//----------------------------------------------------------------------

static bool verify_Facet(const char *&input)
{
   input++ ;				// consume the left bracket
   if (!verify_Symbol(input,true))	// check facet name
      return false ;
   char c ;
   while ((c = FrSkipWhitespace(input)) != ']' && c != '\0')
      {
      if (!valid_FrObject_string(input,true))
	 return false ;
      }
   if (c == ']')
      {
      input++ ;				// skip over terminator
      return true ;			//   and indicate success
      }
   else
      return false ;
}

//----------------------------------------------------------------------

static bool verify_Slot(const char *&input)
{
   input++ ;				// consume the left bracket
   if (!verify_Symbol(input,true))	// check slot name
      return false ;
   while (FrSkipWhitespace(input) == '[')
      {
      if (!verify_Facet(input))		// check for well-formed facet repres.
	 return false ;
      }
   if (*input == ']')
      {
      input++ ;				// skip over terminator
      return true ;			//   and indicate success
      }
   else
      return false ;
}

//----------------------------------------------------------------------

static bool verify_Frame(const char *&input, const char *, bool)
{
   input++ ;				// consume the initial left bracket
   if (!verify_Symbol(input,true))	// check frame name
      return false ;
   while (FrSkipWhitespace(input) == '[')
      {
      if (!verify_Slot(input))		// check for well-formed slot repres.
	 return false ;
      }
   if (*input == ']')
      {
      input++ ;				// skip over terminator
      return true ;			//   and indicate success
      }
   else
      return false ;
}

// end of fiframe.cpp //
