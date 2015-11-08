/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frinline.cpp	"inline" wrappers to make procedural interface	*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,2009				*/
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

#include "frsymbol.h"
#include "frframe.h"
#include "vframe.h"
#include "frinline.h"

// Under Watcom C++ 10.x, the inline functions in frinline.h cause references
// to the functions they call even if they themselves are never called!
// Thus, to avoid bloating the executable, we move those functions out
// into this separate module

#ifdef __WATCOMC__
FrSymbol *frame_name(const FrFrame *frame)
   { return frame ? frame->frameName() : 0 ; }
FrFrame *find_frame(const FrSymbol *framename)
   { return framename ? framename->symbolFrame() : 0 ; }
bool frame_is_empty(const FrFrame *frame)
   { return frame ? frame->emptyFrame() : false ; }
void create_slot(FrFrame *frame,const FrSymbol *slot)
   { if (frame) (void)frame->createSlot(slot) ; }
void create_facet(FrFrame *frame,const FrSymbol *slot,const FrSymbol *facet)
   { if (frame) (void)frame->createFacet(slot,facet) ; }
void add_filler(FrFrame *frame,const FrSymbol *slot,
		      const FrSymbol *facet,FrObject *filler)
   { if (frame) frame->addFiller(slot,facet,filler) ; }
void add_value(FrFrame *frame,const FrSymbol *slot,FrObject *filler)
   { if (frame) frame->addValue(slot,filler) ; }
void add_sem(FrFrame *frame,const FrSymbol *slot,FrObject *filler)
   { if (frame) frame->addSem(slot,filler) ; }
void add_fillers(FrFrame *frame,const FrSymbol *slot,
		        const FrSymbol *facet,FrList *fillers)
   { if (frame) frame->addFillers(slot,facet,fillers) ; }
void add_values(FrFrame *frame,const FrSymbol *slot, FrList *fillers)
   { if (frame) frame->addValues(slot,fillers) ; }
void add_sems(FrFrame *frame,const FrSymbol *slot,FrList *fillers)
   { if (frame) frame->addSems(slot,fillers) ; }
void replace_filler(FrFrame *frame,const FrSymbol *slot, const FrSymbol *facet,
			   const FrObject *old, const FrObject *newfiller)
   { if (frame) frame->replaceFiller(slot,facet,old,newfiller) ; }
void replace_filler(FrFrame *frame,const FrSymbol *slot, const FrSymbol *facet,
			   const FrObject *old, const FrObject *newfiller,
			   FrCompareFunc cmp)
   { if (frame) frame->replaceFiller(slot,facet,old,newfiller,cmp) ; }
void erase_frame(FrFrame *frame)
   { if (frame) frame->eraseFrame() ; }
void erase_slot(FrFrame *frame,const FrSymbol *slot)
   { if (frame) frame->eraseSlot(slot) ; }
void erase_facet(FrFrame *frame,const FrSymbol *slot, const FrSymbol *facet)
   { if (frame) frame->eraseFacet(slot,facet) ; }
void erase_filler(FrFrame *frame,const FrSymbol *slot,
		         const FrSymbol *facet,FrObject *filler)
   { if (frame) frame->eraseFiller(slot,facet,filler) ; }
void erase_filler(FrFrame *frame,const FrSymbol *slot,
		         const FrSymbol *facet,FrObject *filler,
			 FrCompareFunc cmp)
   { if (frame) frame->eraseFiller(slot,facet,filler,cmp) ; }
void erase_value(FrFrame *frame,const FrSymbol *slot,FrObject *fill)
   { if (frame) frame->eraseValue(slot,fill) ; }
void erase_value(FrFrame *frame,const FrSymbol *slot,FrObject *fill,
			FrCompareFunc cmp)
   { if (frame) frame->eraseValue(slot,fill,cmp) ; }
void erase_sem(FrFrame *frame,const FrSymbol *slot,FrObject *fill)
   { if (frame) frame->eraseSem(slot,fill) ; }
void erase_sem(FrFrame *frame,const FrSymbol *slot,FrObject *fill,
			FrCompareFunc cmp)
   { if (frame) frame->eraseSem(slot,fill,cmp) ; }
const FrList *get_fillers(const FrFrame *frame,const FrSymbol *slot,
				  const FrSymbol *facet,bool inherit)
   { return frame ? frame->getFillers(slot,facet,inherit) : 0 ; }
FrObject *get_filler(const FrFrame *frame,const FrSymbol *slot,
			 const FrSymbol *facet,bool inherit)
   { return frame ? frame->firstFiller(slot,facet,inherit) : 0 ; }
const FrList *get_values(const FrFrame *frame,const FrSymbol *slot,
				 bool inherit)
   { return frame ? frame->getValues(slot,inherit) : 0 ; }
FrObject *get_value(const FrFrame *frame,const FrSymbol *slot,
			 bool inherit)
   { return frame ? frame->getValue(slot,inherit) : 0 ; }
const FrList *get_sem(const FrFrame *frame,const FrSymbol *slot,
			      bool inherit)
   { return frame ? frame->getSem(slot,inherit) : 0 ; }
bool frame_locked(const FrFrame *frame)
   { return frame ? (frame->isLocked() != false) : false ; }
void copy_frame(FrSymbol *oldframe, FrSymbol *newframe, bool temporary)
   { FrFrame *fr = oldframe->symbolFrame() ;
     if (fr) fr->copyFrame(newframe,temporary) ; }
bool is_a_p(const FrFrame *frame,const FrFrame *poss_parent)
   { return frame ? frame->isA_p(poss_parent) : false ; }
bool part_of_p(const FrFrame *frame,const FrFrame *poss_container)
   { return frame ? frame->partOf_p(poss_container) : false ; }
FrList *inheritable_slots(const FrFrame *frame,FrInheritanceType inherit,
				  bool include_names)
   { return frame ? frame->collectSlots(inherit,0,include_names) : 0 ; }
void inherit_all_fillers(FrFrame *frame)
   { if (frame) frame->inheritAll() ; }
void inherit_all_fillers(FrFrame *frame,FrInheritanceType inherit)
   { if (frame) frame->inheritAll(inherit) ; }
FrList *slots_in_frame(const FrFrame *frame)
   { return frame ? frame->allSlots() : 0 ; }
FrList *facets_in_slot(const FrFrame *frame, const FrSymbol *slot)
   { return (frame && slot) ? frame->slotFacets(slot) : 0 ; }


   bool is_frame(FrSymbol *frname)
   { return frname ? frname->isFrame() : false ; }
bool is_deleted_frame(FrSymbol *frname)
   { return frname ? frname->isDeletedFrame() : false ; }
FrFrame *find_vframe(FrSymbol *frname)
   { return frname ? frname->findFrame() : 0 ; }
FrFrame *create_vframe(FrSymbol *frame)
   { return frame ? frame->createVFrame() : 0 ; }
bool frame_is_empty(FrSymbol *frame)
   { return frame ? frame->emptyFrame() : false ; }
bool frame_is_dirty(FrSymbol *frame)
   { return frame ? frame->dirtyFrame() : false ; }
int start_transaction()
   { return ((FrSymbol*)0)->startTransaction() ; }
int end_transaction(int transaction)
   { return ((FrSymbol*)0)->endTransaction(transaction) ; }
int abort_transaction(int transaction)
   { return ((FrSymbol*)0)->abortTransaction(transaction) ; }
FrFrame *lock_frame(FrSymbol *frame)
   { return frame ? frame->lockFrame() : 0 ; }
bool unlock_frame(FrSymbol *frame)
   { return frame ? frame->unlockFrame() : false ; }
bool frame_locked(FrSymbol *frame)
   { return frame ? frame->isLocked() : false ; }
int commit_frame(FrSymbol *frame)
   { return frame ? frame->commitFrame() : 0 ; }
int discard_frame(FrSymbol *frame)
   { return frame ? frame->discardFrame() : 0 ; }
int delete_frame(FrSymbol *frame)
   { return frame ? frame->deleteFrame() : 0 ; }
FrFrame *get_old_frame(FrSymbol *frame, int generation)
   { return frame ? frame->oldFrame(generation) : 0 ; }
void copy_vframe(FrSymbol *oldframe, FrSymbol *newframe)
   { if (oldframe) oldframe->copyFrame(newframe) ; }
void create_slot(FrSymbol *frame,const FrSymbol *slot)
   { if (frame) (void)frame->createSlot(slot) ; }
void create_facet(FrSymbol *frame,const FrSymbol *slot,const FrSymbol *facet)
   { if (frame) (void)frame->createFacet(slot,facet) ; }
void add_filler(FrSymbol *frame,const FrSymbol *slot,
		      const FrSymbol *facet, const FrObject *filler)
   { if (frame) frame->addFiller(slot,facet,filler) ; }
void add_value(FrSymbol *frame,const FrSymbol *slot, const FrObject *filler)
   { if (frame) frame->addValue(slot,filler) ; }
void add_sem(FrSymbol *frame,const FrSymbol *slot, const FrObject *filler)
   { if (frame) frame->addSem(slot,filler) ; }
void add_fillers(FrSymbol *frame,const FrSymbol *slot,
		        const FrSymbol *facet, const FrList *fillers)
   { if (frame) frame->addFillers(slot,facet,fillers) ; }
void add_values(FrSymbol *frame,const FrSymbol *slot, const FrList *fillers)
   { if (frame) frame->addValues(slot,fillers) ; }
void add_sems(FrSymbol *frame,const FrSymbol *slot,const FrList *fillers)
   { if (frame) frame->addSems(slot,fillers) ; }
void replace_filler(FrSymbol *frame, const FrSymbol *slot,
			   const FrSymbol *facet, const FrObject *old,
			   const FrObject *newfiller)
   { if (frame) frame->replaceFiller(slot,facet,old,newfiller) ; }
void replace_filler(FrSymbol *frame, const FrSymbol *slot,
			   const FrSymbol *facet, const FrObject *old,
			   const FrObject *newfiller, FrCompareFunc cmp)
   { if (frame) frame->replaceFiller(slot,facet,old,newfiller,cmp) ; }
void erase_frame(FrSymbol *frame)
   { if (frame) frame->eraseFrame() ; }
void erase_slot(FrSymbol *frame,const FrSymbol *slot)
   { if (frame) frame->eraseSlot(slot) ; }
void erase_facet(FrSymbol *frame,const FrSymbol *slot, const FrSymbol *facet)
   { if (frame) frame->eraseFacet(slot,facet) ; }
void erase_filler(FrSymbol *frame,const FrSymbol *slot,
		         const FrSymbol *facet,FrObject *filler)
   { if (frame) frame->eraseFiller(slot,facet,filler) ; }
void erase_filler(FrSymbol *frame,const FrSymbol *slot,
		         const FrSymbol *facet,FrObject *filler,
			 FrCompareFunc cmp)
   { if (frame) frame->eraseFiller(slot,facet,filler,cmp) ; }
void erase_sem(FrSymbol *frame,const FrSymbol *slot,FrObject *fill)
   { if (frame) frame->eraseSem(slot,fill) ; }
void erase_sem(FrSymbol *frame,const FrSymbol *slot,FrObject *fill,
			FrCompareFunc cmp)
   { if (frame) frame->eraseSem(slot,fill,cmp) ; }
void erase_value(FrSymbol *frame,const FrSymbol *slot,FrObject *fill)
   { if (frame) frame->eraseValue(slot,fill) ; }
void erase_value(FrSymbol *frame,const FrSymbol *slot,FrObject *fill,
			FrCompareFunc cmp)
   { if (frame) frame->eraseValue(slot,fill,cmp) ; }
const FrList *get_fillers(FrSymbol *frame,const FrSymbol *slot,
				  const FrSymbol *facet,bool inherit)
   { return frame ? frame->getFillers(slot,facet,inherit) : 0 ; }
FrObject *get_filler(FrSymbol *frame,const FrSymbol *slot,
			 const FrSymbol *facet,bool inherit)
   { return frame ? frame->firstFiller(slot,facet,inherit) : 0 ; }
const FrList *get_values(FrSymbol *frame,const FrSymbol *slot,
			 bool inherit)
   { return frame ? frame->getValues(slot,inherit) : 0 ; }
FrObject *get_value(FrSymbol *frame,const FrSymbol *slot,
			 bool inherit)
   { return frame ? frame->getValue(slot,inherit) : 0 ; }
const FrList *get_sem(FrSymbol *frame,const FrSymbol *slot,
			      bool inherit)
   { return frame ? frame->getSem(slot,inherit) : 0 ; }
bool is_a_p(FrSymbol *frame, FrSymbol *poss_parent)
   { return frame ? frame->isA_p(poss_parent) : false ; }
bool part_of_p(FrSymbol *frame, FrSymbol *poss_container)
   { return frame ? frame->partOf_p(poss_container) : false ; }
FrList *inheritable_slots(FrSymbol *frame,FrInheritanceType inherit)
   { return frame ? frame->collectSlots(inherit) : 0 ; }
void inherit_all_fillers(FrSymbol *frame)
   { if (frame) frame->inheritAll() ; }
void inherit_all_fillers(FrSymbol *frame,FrInheritanceType inherit)
   { if (frame) frame->inheritAll(inherit) ; }
FrList *slots_in_frame(FrSymbol *frame)
   { return frame ? frame->allSlots() : 0 ; }
FrList *facets_in_slot(FrSymbol *frame, const FrSymbol *slot)
   { return (frame && slot) ? frame->slotFacets(slot) : 0 ; }
#endif /* __WATCOMC__ */

// end of file frinline.cpp //
