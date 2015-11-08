/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frsymfrm.cpp	       class FrSymbol frame functions		*/
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

#include <stdlib.h>
#include <string.h>
#include "frsymtab.h"
#include "frpcglbl.h"
#include "vframe.h"

/**********************************************************************/
/*	Global variables local to this module			      */
/**********************************************************************/

/**********************************************************************/
/*    Methods for class FrSymbol			              */
/**********************************************************************/

/**********************************************************************/
/*    FrSymbol method functions for supporting VFrames		      */
/**********************************************************************/

FrFrame *FrSymbol::findFrame()
{
   return find_vframe_inline(this) ;
}

//----------------------------------------------------------------------

bool FrSymbol::isDeletedFrame()
{
   if (VFrame_Info)
      return VFrame_Info->isDeletedFrame(this) ;
   else
      return false ;
}

//----------------------------------------------------------------------

FrFrame *FrSymbol::createFrame()
{
   FrFrame *fr = find_vframe_inline(this) ;
   return fr ? fr : new FrFrame((FrSymbol *)this) ;
}

//----------------------------------------------------------------------

FrFrame *FrSymbol::createVFrame()
{
   FrFrame *fr = find_vframe_inline(this) ;
   if (fr)
      return fr ;
   if (FramepaC_new_VFrame)
      return FramepaC_new_VFrame((FrSymbol *)this) ;
   else
      return new FrFrame((FrSymbol *)this) ;
}

//----------------------------------------------------------------------

FrFrame *FrSymbol::createInstanceFrame()
{
   FrFrame *fr = find_vframe_inline(this) ;
   if (!fr)
      return 0 ;
   FrSymbol *instance = gensym(fr->frameName()) ;
   FrFrame *newfr = fr->isVFrame() ? instance->createVFrame()
				 : instance->createFrame() ;
   newfr->addValue(symbolINSTANCEOF,this) ;
   return newfr ;
}

//----------------------------------------------------------------------

int FrSymbol::startTransaction()
{
   return VFrame_Info ? VFrame_Info->startTransaction() : 0 ;
}

//----------------------------------------------------------------------

int FrSymbol::endTransaction(int transaction)
{
   return VFrame_Info ? VFrame_Info->endTransaction(transaction) : 0 ;
}

//----------------------------------------------------------------------

int FrSymbol::abortTransaction(int transaction)
{
   return VFrame_Info ? VFrame_Info->abortTransaction(transaction) : 0 ;
}

//----------------------------------------------------------------------

bool FrSymbol::unlockFrames(FrList *locklist)
{
   if (VFrame_Info)
      {
//!!!for now, just do one at a time
      while (locklist)
	 if (!((FrSymbol*)(locklist->first()))->unlockFrame())
	    return false ;	// failed
      return true ;	// successful
      }
   else
      while (locklist)
	 {
	 FrFrame *fr = ((FrSymbol*)(locklist->first()))->symbolFrame() ;
	 if (fr)
	    fr->setLock(false) ;
	 locklist = locklist->rest() ;
	 }
   return true ;	// successful
}

//----------------------------------------------------------------------

bool FrSymbol::emptyFrame()
{
   FrFrame *fr = find_vframe_inline(this) ;

   return fr ? fr->emptyFrame() : false ;
}

//----------------------------------------------------------------------

bool FrSymbol::dirtyFrame()
{
   FrFrame *fr = symbolFrame() ;
   // don't want to bring in frame if it is not yet in memory; if not in
   // memory, it is by definition not dirty

   return fr ? fr->dirtyFrame() : false ;
}

//----------------------------------------------------------------------

int FrSymbol::commitFrame()
{
   FrFrame *fr = symbolFrame() ;
   // note: we did not call find_vframe() because we don't want to read in
   // the frame if it is not yet in memory

   return fr ? fr->commitFrame() : 0 ;
}

//----------------------------------------------------------------------

int FrSymbol::discardFrame()
{
   FrFrame *fr = symbolFrame() ;
   // note: we did not call find_vframe() because we don't want to read in
   // the frame if it is not yet in memory

   if (fr)
      delete fr ;
   return 0 ;
}

//----------------------------------------------------------------------

FrFrame *FrSymbol::oldFrame(int generation)
{
   return (VFrame_Info) ? VFrame_Info->retrieveOldFrame(this,generation) : 0 ;
}

//----------------------------------------------------------------------

FrSlot *FrSymbol::createSlot(const FrSymbol *slotname)
{
   FrFrame *fr = find_vframe_inline(this) ;

   return fr ? fr->createSlot(slotname) : 0 ;
}

//----------------------------------------------------------------------

void FrSymbol::createFacet(const FrSymbol *slotname, const FrSymbol *facetname)
{
   FrFrame *fr = find_vframe_inline(this) ;

   if (fr) fr->createFacet(slotname,facetname) ;
}

//----------------------------------------------------------------------

void FrSymbol::addFiller(const FrSymbol *slotname,const FrSymbol *facet,
		       const FrObject *filler)
{
   FrFrame *fr = find_vframe_inline(this) ;

   if (fr) fr->addFiller(slotname,facet,filler);
}

//----------------------------------------------------------------------

void FrSymbol::addValue(const FrSymbol *slotname, const FrObject *filler)
{
   FrFrame *fr = find_vframe_inline(this) ;

   if (fr) fr->addValue(slotname,filler) ;
}

//----------------------------------------------------------------------

void FrSymbol::addSem(const FrSymbol *slotname, const FrObject *filler)
{
   FrFrame *fr = find_vframe_inline(this) ;

   if (fr) fr->addSem(slotname,filler) ;
}

//----------------------------------------------------------------------

void FrSymbol::addFillers(const FrSymbol *slotname, const FrSymbol *facet,
			const FrList *fillers)
{
   FrFrame *fr = find_vframe_inline(this) ;

   if (fr) fr->addFillers(slotname,facet,fillers) ;
}

//----------------------------------------------------------------------

void FrSymbol::addValues(const FrSymbol *slotname, const FrList *fillers)
{
   FrFrame *fr = find_vframe_inline(this) ;

   if (fr) fr->addValues(slotname,fillers) ;
}

//----------------------------------------------------------------------

void FrSymbol::addSems(const FrSymbol *slotname, const FrList *fillers)
{
   FrFrame *fr = find_vframe_inline(this) ;

   if (fr) fr->addSems(slotname,fillers) ;
}

//----------------------------------------------------------------------

void FrSymbol::replaceFiller(const FrSymbol *slotname,const FrSymbol *facet,
			   const FrObject *old, const FrObject *newfiller)
{
   FrFrame *fr = find_vframe_inline(this) ;

   if (fr) fr->replaceFiller(slotname,facet,old,newfiller);
}

//----------------------------------------------------------------------

void FrSymbol::replaceFiller(const FrSymbol *slotname,const FrSymbol *facet,
			   const FrObject *old, const FrObject *newfiller,
			   FrCompareFunc cmp)
{
   FrFrame *fr = find_vframe_inline(this) ;

   if (fr) fr->replaceFiller(slotname,facet,old,newfiller,cmp);
}

//----------------------------------------------------------------------

void FrSymbol::replaceFacet(const FrSymbol *slotname, const FrSymbol *facet,
			  const FrList *newfillers)
{
   FrFrame *fr = find_vframe_inline(this) ;

   if (fr) fr->replaceFacet(slotname,facet,newfillers);
}

//----------------------------------------------------------------------

void FrSymbol::pushFiller(const FrSymbol *slotname, const FrSymbol *facet,
			const FrObject *filler)
{
   FrFrame *fr = find_vframe_inline(this) ;

   if (fr) fr->pushFiller(slotname,facet,filler);
}

//----------------------------------------------------------------------

FrObject *FrSymbol::popFiller(const FrSymbol *slotname, const FrSymbol *facet)
{
   FrFrame *fr = find_vframe_inline(this) ;

   return fr ? fr->popFiller(slotname,facet) : 0 ;
}

//----------------------------------------------------------------------

void FrSymbol::eraseFrame()
{
   FrFrame *fr = find_vframe_inline(this) ;

   if (fr) fr->eraseFrame() ;
}

//----------------------------------------------------------------------

void FrSymbol::eraseCopyFrame()
{
   FrFrame *fr = find_vframe_inline(this) ;

   if (fr) fr->eraseCopyFrame() ;
}

//----------------------------------------------------------------------

void FrSymbol::eraseSlot(const FrSymbol *slotname)
{
   FrFrame *fr = find_vframe_inline(this) ;

   if (fr) fr->eraseSlot(slotname) ;
}

//----------------------------------------------------------------------

void FrSymbol::eraseFacet(const FrSymbol *slotname, const FrSymbol *facet)
{
   FrFrame *fr = find_vframe_inline(this) ;

   if (fr) fr->eraseFacet(slotname,facet) ;
}

//----------------------------------------------------------------------

void FrSymbol::eraseFiller(const FrSymbol *slotname, const FrSymbol *facet,
			 const FrObject *filler)
{
   FrFrame *fr = find_vframe_inline(this) ;

   if (fr) fr->eraseFiller(slotname,facet,filler) ;
}

//----------------------------------------------------------------------

void FrSymbol::eraseFiller(const FrSymbol *slotname, const FrSymbol *facet,
			 const FrObject *filler, FrCompareFunc cmp)
{
   FrFrame *fr = find_vframe_inline(this) ;

   if (fr) fr->eraseFiller(slotname,facet,filler,cmp) ;
}

//----------------------------------------------------------------------

void FrSymbol::eraseSem(const FrSymbol *slotname, const FrObject *filler)
{
   FrFrame *fr = find_vframe_inline(this) ;

   if (fr) fr->eraseSem(slotname,filler) ;
}

//----------------------------------------------------------------------

void FrSymbol::eraseSem(const FrSymbol *slotname, const FrObject *filler,
			FrCompareFunc cmp)
{
   FrFrame *fr = find_vframe_inline(this) ;

   if (fr) fr->eraseSem(slotname,filler,cmp) ;
}

//----------------------------------------------------------------------

void FrSymbol::eraseValue(const FrSymbol *slotname, const FrObject *filler)
{
   FrFrame *fr = find_vframe_inline(this) ;

   if (fr) fr->eraseValue(slotname,filler) ;
}

//----------------------------------------------------------------------

void FrSymbol::eraseValue(const FrSymbol *slotname, const FrObject *filler,
			FrCompareFunc cmp)
{
   FrFrame *fr = find_vframe_inline(this) ;

   if (fr) fr->eraseValue(slotname,filler,cmp) ;
}

//----------------------------------------------------------------------

const FrList *FrSymbol::getFillers(const FrSymbol *slotname,
				     const FrSymbol *facet,
				     bool inherit)
{
   FrFrame *fr = find_vframe_inline(this) ;

   return fr ? fr->getFillers(slotname,facet,inherit) : 0 ;
}

//----------------------------------------------------------------------

FrObject *FrSymbol::firstFiller(const FrSymbol *slotname, const FrSymbol *facet,
			    bool inherit)
{
   FrFrame *fr = find_vframe_inline(this) ;

   return fr ? fr->firstFiller(slotname,facet,inherit) : 0 ;
}

//----------------------------------------------------------------------

const FrList *FrSymbol::getValues(const FrSymbol *slotname, bool inherit)
{
   FrFrame *fr = find_vframe_inline(this) ;

   return fr ? fr->getValues(slotname,inherit) : 0 ;
}

//----------------------------------------------------------------------

FrObject *FrSymbol::getValue(const FrSymbol *slotname, bool inherit)
{
   FrFrame *fr = find_vframe_inline(this) ;

   return fr ? fr->getValue(slotname,inherit) : 0 ;
}

//----------------------------------------------------------------------

const FrList * FrSymbol::getSem(const FrSymbol *slotname, bool inherit)
{
   FrFrame *fr = find_vframe_inline(this) ;

   return fr ? fr->getSem(slotname,inherit) : 0 ;
}

//----------------------------------------------------------------------

bool FrSymbol::isA_p(FrSymbol *poss_parent)
{
   FrFrame *fr = find_vframe_inline(this) ;
   FrFrame *pp = poss_parent ? find_vframe_inline(poss_parent) : 0 ;

   return (fr && pp) ? fr->isA_p(pp) : false ;
}

//----------------------------------------------------------------------

bool FrSymbol::partOf_p(FrSymbol *poss_parent)
{
   FrFrame *fr = find_vframe_inline(this) ;
   FrFrame *pp = poss_parent ? find_vframe_inline(poss_parent) : 0 ;

   return (fr && pp) ? fr->partOf_p(pp) : false ;
}

//----------------------------------------------------------------------

FrFrame *FrSymbol::copyFrame(FrSymbol *newframe,bool temporary)
{
   FrFrame *fr = find_vframe_inline(this) ;

   return fr ? fr->copyFrame(newframe,temporary) : 0 ;
}

//----------------------------------------------------------------------

bool FrSymbol::renameFrame(FrSymbol *newframe)
{
   FrFrame *fr = find_vframe_inline(this) ;

   return fr ? fr->renameFrame(newframe) : false ; // can't rename if not frame
}

//----------------------------------------------------------------------

FrList *FrSymbol::allSlots() const
{
   FrFrame *fr = find_vframe_inline(this) ;

   return fr ? fr->allSlots() : 0 ;
}

//----------------------------------------------------------------------

FrList *FrSymbol::slotFacets(const FrSymbol *slotname) const
{
   FrFrame *fr = find_vframe_inline(this) ;

   return fr ? fr->slotFacets(slotname) : 0 ;
}

//----------------------------------------------------------------------

FrList *FrSymbol::collectSlots(FrInheritanceType inherit,FrList *allslots,
				 bool include_names)
{
   FrFrame *fr = find_vframe_inline(this) ;

   return fr ? fr->collectSlots(inherit,allslots,include_names) : 0 ;
}

//----------------------------------------------------------------------

void FrSymbol::inheritAll()
{
   FrFrame *fr = find_vframe_inline(this) ;

   if (fr) fr->inheritAll() ;
}

//----------------------------------------------------------------------

void FrSymbol::inheritAll(FrInheritanceType inherit)
{
   FrFrame *fr = find_vframe_inline(this) ;

   if (fr) fr->inheritAll(inherit) ;
}


// end of file frsymfrm.cpp //
