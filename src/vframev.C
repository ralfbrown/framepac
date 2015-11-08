/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File vframev.cpp	 "virtual memory" frames (virtual functions)    */
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,2001,2002,2009			*/
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
#  pragma implementation "vframe.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "frpcglbl.h"
#include "vfinfo.h"
#include "mikro_db.h"
#include "frfinddb.h"

/************************************************************************/
/*	Types for this module						*/
/************************************************************************/

static class VFrame *new_VFrame(class FrSymbol *name) ;

class VFrameInit
   {
   public:
      VFrameInit() { FramepaC_new_VFrame = new_VFrame ; }
      ~VFrameInit() { FramepaC_new_VFrame = 0 ; }
   } ;

VFrameInit FramepaC_VFrameInit ;

/************************************************************************/
/*    Global variables imported from other modules		      	*/
/************************************************************************/

extern bool omit_inverse_links ;

/************************************************************************/
/*    VFrame member and associated functions			      	*/
/************************************************************************/

static void _shutdown_all_VFrames() ; // forward declaration

VFrame::VFrame(FrSymbol *framename)
{
   name = framename ;
   framename->setFrame(this) ;
   virtual_frame =
   dirty = true ;		// frame needs to be written to backing store
   if (VFrame_Info)
      VFrame_Info->createFrame(name) ;
   if (!FramepaC_shutdown_all_VFrames)
      FramepaC_shutdown_all_VFrames = _shutdown_all_VFrames ;
   return ;
}

//----------------------------------------------------------------------

VFrame::~VFrame()
{
   if (dirty && !emptyFrame())
      commitFrame() ;
   return ;
}

//----------------------------------------------------------------------

FrObjectType VFrame::objType() const
{
   return OT_VFrame ;
}

//----------------------------------------------------------------------

const char *VFrame::objTypeName() const
{
   return "VFrame" ;
}

//----------------------------------------------------------------------

FrObjectType VFrame::objSuperclass() const
{
   return OT_Frame ;
}

/**********************************************************************/
/*    Procedural interface to VFrame class			      */
/**********************************************************************/

bool __FrCDECL do_slots(FrSymbol *frame,
			  bool (*func)(const FrFrame *frame,
					 const FrSymbol *slot,
					 va_list args),
			  ...)
{
   va_list args ;
   bool result ;
   FrFrame *fr = find_vframe_inline(frame) ;

   va_start(args,func) ;
   result = fr ? fr->doSlots(func,args) : false ;
   va_end(args) ;
   return result ;
}

//----------------------------------------------------------------------

bool __FrCDECL do_facets(FrSymbol *frame,const FrSymbol *slotname,
			   bool (*func)(const FrFrame *frame,
					  const FrSymbol *slot,
					  const FrSymbol *facet,
					  va_list args),
			   ...)
{
   va_list args ;
   bool result ;
   FrFrame *fr = find_vframe_inline(frame) ;

   va_start(args,func) ;
   result = fr ? fr->doFacets(slotname,func,args) : false ;
   va_end(args) ;
   return result ;
}

//----------------------------------------------------------------------

static VFrame *new_VFrame(FrSymbol *name)
{
   return new VFrame(name) ;
}

/**********************************************************************/
/* 	Helper functions					      */
/**********************************************************************/

/**********************************************************************/
/*    Cleanup functions						      */
/**********************************************************************/

//----------------------------------------------------------------------

int shutdown_VFrames(FrSymbolTable *symtab)
{
   if (synchronize_VFrames() == -1)
      {
      Fr_errno = FE_COMMITFAILED ;
      return -1 ;
      }
   if (!symtab)
      {
      // switch to default, then destroy current symbol table
      symtab = FrSymbolTable::selectDefault() ;
      }
   FrSymbolTable *oldsymtab = symtab->select() ;
   VFrameInfo *info = VFrame_Info ;
   VFrame_Info = 0 ;
   // force the change to be made in the table itself as well as the
   // current working copy
   oldsymtab->select()->select() ;
   destroy_symbol_table(symtab) ;
   if (oldsymtab != symtab)
      oldsymtab->select() ;
   delete info ;
   return 0 ;
}

//----------------------------------------------------------------------

static int shutdown_VFrames_aux(FrSymbolTable *symtab, va_list args)
{
   (void)args ;   // avoid compiler warning
   shutdown_VFrames(symtab) ;
   return true ;
}

//----------------------------------------------------------------------

static void _shutdown_all_VFrames()
{
   do_all_symtabs(shutdown_VFrames_aux) ;
}

// end of file vframev.cpp //
