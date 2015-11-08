/************************************************************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File framekit.cpp		FrameKit conversion functions		*/
/*  LastEdit: 08nov2015   						*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,2009,2015				*/
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

#include "frpcglbl.h"
#include "frprintf.h"

/**********************************************************************/
/*    Global data local to this module				      */
/**********************************************************************/

static const char stringREAD[] = "Read " ;
static const char stringMAKEFRAME[] = "MAKE-FRAME" ;
static const char stringMAKEFR_OLD[] = "MAKE-FRAME-OLD" ;
static const char stringEXPORTED[] = ";; FramepaC frames, exported in " ;
static const char stringDELFRAME[] = ";; deleted frame: " ;
static const char stringNONFRAME[] = ";; non-frame: " ;

static const char errmsg_slot_symbol[]
      = "malformed slot--slot name must be a symbol" ;
static const char errmsg_facet_symbol[]
      = "facet name must be a symbol" ;
static const char errmsg_frame_name[]
      = "malformed frame--name must be a symbol" ;

/**********************************************************************/
/*    Global variables for this module				      */
/**********************************************************************/

extern bool read_as_VFrame ;

/**********************************************************************/
/*	FrameKit compatibility functions			      */
/**********************************************************************/

static void bad_format(const char *message)
{
   char *msg = Fr_aprintf("bad format--%s!", message) ;
   FrWarning(msg) ;
   FrFree(msg) ;
   return ;
}

//----------------------------------------------------------------------

static void convert_FrameKit_facet(FrFrame *frame,FrSymbol *slot,
				   FrList *facetspec,bool views)
{
   FrSymbol *facet = (FrSymbol *)facetspec->first() ;
   if (facet && facet->symbolp())
      {
      facetspec = facetspec->rest() ;
      if (views)
	 {
	 FrObject *first = facetspec->first() ;
	 if (first && first->consp())
	    facetspec = (FrList *)first ;
	 first = facetspec->first() ;
	 if (first && first->symbolp() &&
	     (FrSymbol *)first == symbolCOMMON)
	    facetspec = facetspec->rest() ;
	 else
	    {
	    bad_format("view other than COMMON encountered") ;
	    facetspec = nullptr ;
	    }
	 }
      if (facetspec)
	 frame->addFillers(slot,facet,facetspec) ;
      else
	 frame->createSlot(slot) ;
      }
   else
      bad_format(errmsg_facet_symbol) ;
}

//----------------------------------------------------------------------

static void convert_FrameKit_slot(FrFrame *frame,const FrList *slotspec,
				  bool views)
{
   if (slotspec && slotspec->consp())
      {
      FrSymbol *slot = (FrSymbol *)slotspec->first() ;
      if (slot && slot->symbolp())
	 {
	 slotspec = slotspec->rest() ;
	 while (slotspec)
	    {
	    convert_FrameKit_facet(frame,slot,(FrList *)slotspec->first(),
				   views) ;
	    slotspec = slotspec->rest() ;
	    }
	 }
      else
	 bad_format(errmsg_slot_symbol) ;
      }
   else
      bad_format("slotspec must be a list") ;
}

//----------------------------------------------------------------------

static FrFrame *convert_FrameKit_frame(const FrList *framespec,bool views)
{
   FrFrame *frame ;
   FrSymbol *framename = (FrSymbol*)framespec->first() ;

   if (framename && framename->symbolp())
      {
      frame = read_as_VFrame
	         ? framename->createVFrame()
		 : framename->createFrame() ;
      framespec = framespec->rest() ;
      while (framespec)
	 {
	 convert_FrameKit_slot(frame,(const FrList *)framespec->first(),views) ;
	 framespec = framespec->rest() ;
	 }
      }
   else
      {
      bad_format(errmsg_frame_name) ;
      frame = nullptr ;
      }
   return frame ;
}

//----------------------------------------------------------------------

FrFrame *FrameKit_to_FramepaC(const FrList *framespec, bool warn)
{
   FrSymbol *makefr = (FrSymbol*)framespec->first() ;
   if (makefr && makefr->symbolp() && *makefr == stringMAKEFR_OLD)
      return convert_FrameKit_frame(framespec->rest(),false) ;
   else if (makefr && makefr->symbolp() && *makefr == stringMAKEFRAME)
      return convert_FrameKit_frame(framespec->rest(),true) ;
   else
      {
      if (warn)
         bad_format("FrameKit frame not converted") ;
      return 0 ;
      }
}

//----------------------------------------------------------------------

static bool FramepaC_to_FrameKit_facet(const FrFrame *frame,
				       const FrSymbol *slot,
				       const FrSymbol *facet, va_list args)
{
   FrList **facets ;
   const FrList *fillers = frame->getImmedFillers(slot,facet) ;

   if (fillers)
      {
      FrList *flist = (FrList*)fillers->deepcopy() ;

      pushlist(symbolCOMMON,flist) ;
      flist = new FrList(facet,flist) ;
      facets = va_arg(args,FrList **) ;
      *facets = (*facets)->nconc(new FrList(flist)) ;
      }
   return true ;  // successful
}

//----------------------------------------------------------------------

static bool FramepaC_to_FrameKit_slot(const FrFrame *frame,
				      const FrSymbol *slotname,
				      va_list args)
{
   FrList **slots ;
   FrList *temp_FrameKit_facets = nullptr ;

   do_facets(frame,slotname,FramepaC_to_FrameKit_facet,&temp_FrameKit_facets) ;
   if (temp_FrameKit_facets)
      {
      pushlist(slotname,temp_FrameKit_facets) ;
      slots = va_arg(args,FrList **) ;
      *slots = (*slots)->nconc(new FrList(temp_FrameKit_facets)) ;
      }
   return true ;  // successful
}

//----------------------------------------------------------------------

FrList *FramepaC_to_FrameKit(const FrFrame *frame)
{
   if (!frame)
      return 0 ;
   FrList *temp_FrameKit_slots = nullptr ;
   FrList *list = new FrList(FrSymbolTable::add(stringMAKEFRAME),
			 FrSymbolTable::add(frame->frameName()->symbolName()));

   if (do_slots(frame,FramepaC_to_FrameKit_slot,&temp_FrameKit_slots))
      return list->nconc(temp_FrameKit_slots) ;
   else
      {
      FrWarning("error encountered while converting frame to FrameKit format.") ;
      return 0 ;
      }
}

//----------------------------------------------------------------------

bool import_FrameKit_frames(istream &input,ostream &output)
{
   FrObject *obj ;
   FrFrame *fr ;
   FrSymbol *symbolMAKEFRAME = FrSymbolTable::add(stringMAKEFRAME) ;
   FrSymbol *symbolMAKEFROLD = FrSymbolTable::add(stringMAKEFR_OLD) ;

   if (output)
      output << "Importing frames" << endl ;
   bool oldvirt = read_virtual_frames(true) ;
   if (!VFrame_Info)
      read_virtual_frames(oldvirt) ;
   while (!input.eof())
      {
      input >> obj ;
      if (obj)
	 {
	 if (obj->consp() &&
	     (((FrCons*)obj)->first() == symbolMAKEFRAME ||
	      ((FrCons*)obj)->first() == symbolMAKEFROLD))
	    {
	    fr = FrameKit_to_FramepaC((FrList*)obj) ;
	    if (!fr)
	       {
	       read_virtual_frames(oldvirt) ;
	       return false ;	// error reading stream
	       }
	    if (output)
	       output << stringREAD << fr->frameName() << endl ;
	    }
	 else if (output)
	    {
	    output << stringREAD << obj->objTypeName() << ' ' ;
	    if (obj->framep())
	       output << ((FrFrame*)obj)->frameName() ;
	    else if (obj->consp())
	       output << "(" << obj->car() << " ... )" ;
	    else
	       output << obj ;
	    output << endl ;
	    }
	 obj->freeObject() ;
	 }
      }
   read_virtual_frames(oldvirt) ;
   return true ;
}

//----------------------------------------------------------------------

bool export_FrameKit_frames(ostream &output, FrList *frames)
{
   if (!output)
      return false ;
   output << stringEXPORTED << "FrameKit format" << endl ;
   while (frames)
      {
      FrSymbol *frame = (FrSymbol *)(frames->first()) ;
      if (frame && frame->symbolp())
         {
         if (frame->isFrame())
	    {
   	    FrList *fkit = FramepaC_to_FrameKit(frame->findFrame()) ;
	    output << fkit << endl ;
	    free_object(fkit) ;
	    }
         else if (frame->isDeletedFrame())
	    output << stringDELFRAME << frame << endl ;
	 else
	    output << stringNONFRAME << frame << endl ;
	 }
      else
         return false ;
      frames = frames->rest() ;
      }
   return true ;
}

//----------------------------------------------------------------------

bool export_FramepaC_frames(ostream &output, FrList *frames)
{
   if (!output)
      return false ;
   output << stringEXPORTED << "native format" << endl ;
   while (frames)
      {
      FrSymbol *frame = (FrSymbol *)(frames->first()) ;
      if (frame && frame->symbolp())
         {
         if (frame->isFrame())
	    output << frame->findFrame() << endl ;
         else if (frame->isDeletedFrame())
	    output << stringDELFRAME << frame << endl ;
	 else
	    output << stringNONFRAME << frame << endl ;
	 }
      else
         return false ;
      frames = frames->rest() ;
      }
   return true ;
}


// end of file framekit.cpp //
