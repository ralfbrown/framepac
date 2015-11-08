/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frframe.cpp		class FrFrame				*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,2000,2001,2002,2009,2013		*/
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
#  pragma implementation "frframe.h"
#endif

#include <stdlib.h>
#include <string.h>
#include "frframe.h"
#include "frlist.h"
#include "frsymtab.h"
#include "frpcglbl.h"

/************************************************************************/
/*    Types local to this module					*/
/************************************************************************/

/************************************************************************/
/*    Global variables shared with other modules in FramepaC		*/
/************************************************************************/

extern bool omit_inverse_links ;
extern const FrFrame *FramepaC_lock_Frame_1 ;
const FrFrame *FramepaC_lock_Frame_2 = 0 ;

extern FrInheritanceType (*FramepaC_set_inhtype_func)(FrInheritanceType) ;

/************************************************************************/
/*    forward declarations						*/
/************************************************************************/

static _FrFrameInheritanceFunc inheritFillersNone ;
static _FrFrameInheritanceFunc inheritFillersSimple ;
static _FrFrameInheritanceFunc inheritFillersDFS ;
static _FrFrameInheritanceFunc inheritFillersBFS ;
static _FrFrameInheritanceFunc inheritFillersPartOfDFS ;
static _FrFrameInheritanceFunc inheritFillersPartOfBFS ;
static _FrFrameInheritanceFunc inheritFillersLocalDFS ;
static _FrFrameInheritanceFunc inheritFillersLocalBFS ;
static _FrFrameInheritanceFunc inheritFillersUser ;
static _FrFrameInheritanceFunc inheritFillersDFS2 ;
static _FrFrameInheritanceFunc inheritFillersLocalDFS2 ;
static const FrList *inheritFillersBFS(FrList *queue, FrFrame *frame,
					const FrSymbol *slot,
					const FrSymbol *facet) ;

/************************************************************************/
/*    private class IFrame used for improving inheritance speed		*/
/************************************************************************/

static bool isa_p_helper(const FrFrame *frame, const FrFrame *poss_parent) ;
static bool partOf_p_helper(const FrFrame *frame, const FrFrame *poss_cont) ;

class IFrame : public FrFrame
   {
   public:
   //friend functions for maximum speed
//      friend _FrFrameInheritanceFunc inheritFillersNone ;
      friend _FrFrameInheritanceFunc inheritFillersSimple ;
      friend _FrFrameInheritanceFunc inheritFillersDFS ;
      friend _FrFrameInheritanceFunc inheritFillersDFS2 ;
      friend _FrFrameInheritanceFunc inheritFillersBFS ;
      friend _FrFrameInheritanceFunc inheritFillersPartOfDFS ;
      friend _FrFrameInheritanceFunc inheritFillersPartOfBFS ;
      friend _FrFrameInheritanceFunc inheritFillersLocalDFS ;
      friend _FrFrameInheritanceFunc inheritFillersLocalDFS2 ;
      friend _FrFrameInheritanceFunc inheritFillersLocalBFS ;
      friend const FrList *inheritFillersBFS(FrList *queue, FrFrame *frame,
					const FrSymbol *slot,
					const FrSymbol *facet) ;
      friend bool isa_p_helper(const FrFrame *frame, const FrFrame *poss_parent) ;
      friend bool partOf_p_helper(const FrFrame *frame, const FrFrame *poss_cont) ;
      friend inline bool is_predefined_slot(const IFrame *fr,
					    const FrSlot *slot) ;
   } ;

/************************************************************************/
/*    Global variables local to this module				*/
/************************************************************************/

static FrList *catchall_facet = 0 ;   // list to use for any unknown facet name

static FrInheritanceFunc *user_inherit_func = 0 ;
static FrInheritanceLocFunc *user_inherit_loc_func = 0 ;

/************************************************************************/
/*    Global data for to this module					*/
/************************************************************************/

static const char str_nomem_copyslot[] = "while copying slot" ;
static const char str_nomem_copyframe[] = "while copying frame" ;

/************************************************************************/
/*	Utility functions						*/
/************************************************************************/

#ifdef FrDEMONS
// BorlandC++ won't inline functions containing 'while'
#if !defined(__BORLANDC__)
inline
#endif
void quick_erase_facet(FrList *&facet, const FrSymbol *frname,
		       const FrSymbol *slotname, const FrSymbol *facetname)
{
   while (facet)
      {
      FrObject *filler = poplist(facet) ;
      CALL_DEMON(if_deleted,frname,slotname,facetname,filler) ;
      free_object(filler) ;
      }
}
#else
inline void quick_erase_facet(FrList *&facet, const FrSymbol *frname,
			      const FrSymbol *slotname,
			      const FrSymbol *facetname)
{
   (void)frname ; (void)slotname ; (void)facetname ;
   facet->eraseList(true) ;
   facet = 0 ;
}
#endif /* FrDEMONS */

//----------------------------------------------------------------------

#ifdef FrDEMONS
static void FramepaC_apply_demons(const FrDemonList *demons,
				  const FrSymbol *frame,
				  const FrSymbol *slot, const FrSymbol *facet,
				  const FrObject *value)
{
   while (demons)
      {
      if (demons->demon(frame,slot,facet,value,demons->args))
	 break ;
      demons = demons->next ;
      }
}
#endif /* FrDEMONS */

//----------------------------------------------------------------------

inline bool is_predefined_slot(const IFrame *fr, const FrSlot *slot)
{
#ifdef __MSDOS__
   return ((FrSlot _huge *)slot >= (FrSlot _huge *)fr->predef_slots &&
	   (FrSlot _huge *)slot < (FrSlot _huge *)&fr->predef_slots[FrNUM_PREDEFINED_SLOTS]) ;
#else
   return (bool)(slot >= fr->predef_slots &&
		   slot < &fr->predef_slots[FrNUM_PREDEFINED_SLOTS]) ;
#endif
}

/**********************************************************************/
/*	Member functions for class FrFrame			      */
/**********************************************************************/

//----------------------------------------------------------------------

int FrFrame::numberOfSlots() const
{
   int i = lengthof(predef_slots) ;

   for (register FrSlot *slot = predef_slots[lengthof(predef_slots)-1].next ;
	slot ;
	slot = slot->next)
      i++ ;
   return i ;
}

//----------------------------------------------------------------------

static void copy_slot(FrSlot *dest, const FrSlot *src)
{
   FrFacet *newfacet ;

   dest->name = src->name ;
   dest->value_facet = src->value_facet
			  ? (FrList *)src->value_facet->deepcopy()
			  : 0 ;
   newfacet = 0 ;
   for (FrFacet *facet = src->other_facets ; facet ; facet = facet->next)
      {
      if (newfacet)
	 {
	 if ((newfacet->next = new FrFacet(facet->facet_name)) == 0)
	    {
	    FrNoMemory(str_nomem_copyslot) ;
	    return ;
	    }
	 newfacet = newfacet->next ;
	 }
      else
	 {
	 newfacet = new FrFacet(facet->facet_name) ;
	 if (!newfacet)
	    {
	    FrNoMemory(str_nomem_copyslot) ;
	    return ;
	    }
	 dest->other_facets = newfacet ;
	 }
      newfacet->fillers = facet->fillers
				? (FrList *)facet->fillers->deepcopy()
				: 0 ;
      }
}

//----------------------------------------------------------------------

FrFrame *FrFrame::copyFrame(FrSymbol *newname, bool temporary) const
{
   int i ;
   FrSlot *oldslot, *newslot, *currslot ;
   FrFrame *newframe ;

   if ((newframe = find_vframe_inline(newname)) != 0)
      newframe->eraseFrame() ;
   else
      {
      newframe = (virtual_frame && !temporary && FramepaC_new_VFrame)
	 	? FramepaC_new_VFrame(newname)
	 	: new FrFrame(newname) ;
      if (!newframe)
	 {
	 FrNoMemory(str_nomem_copyframe) ;
	 return 0 ;
	 }
      }
   FramepaC_lock_Frame_1 = this ;	// don't move these frames if an
   FramepaC_lock_Frame_2 = newframe ;	// alloc causes memory compaction
   for (i = 0 ; i < (int)lengthof(predef_slots) ; i++)
      copy_slot(&newframe->predef_slots[i],&predef_slots[i]) ;
   for (i = 0 ; i < (int)lengthof(predef_slots)-1 ; i++)
      newframe->predef_slots[i].next = &newframe->predef_slots[i+1] ;
   currslot = &newframe->predef_slots[lengthof(predef_slots)-1] ;
   for (oldslot = predef_slots[i].next ; oldslot ; oldslot = oldslot->next)
      {
//!!!
      newslot = new FrSlot() ;
      if (!newslot)
	 {
	 FrNoMemory(str_nomem_copyslot) ;
	 break ;
	 }
      copy_slot(newslot,oldslot) ;
      currslot->next = newslot ;
      currslot = newslot ;
      }
   currslot->next = 0 ;
   newframe->dirty = true ;
   FramepaC_lock_Frame_1 = FramepaC_lock_Frame_2 = 0 ;
   return newframe ;
}

//----------------------------------------------------------------------

static bool rename_inverse_link(const FrFrame *frame, const FrSymbol *slot,
				va_list args)
{
   FrSymbol *invslot = slot->inverseRelation() ;
   if (!invslot)
      return true ;
   FrVarArg(const FrSymbol *,oldname) ;
   FrVarArg(const FrSymbol *,newname) ;
   const FrList *facet = frame->getValues(slot,false) ; // no inheritance
   while (facet)
      {
      FrSymbol *other = (FrSymbol*)facet->first() ;
      if (other && other->symbolp() && other->isFrame())
	 {
	 // get inverse, without using inheritance
	 FrFrame *othfr = find_vframe_inline(other) ;
	 const FrList *invfacet ;
	 invfacet = !othfr ? 0 : othfr->getValues(invslot,false) ;
	 FrList *f = invfacet->member(oldname) ;
	 if (f)
	    {
	    f->replaca(newname) ;
	    other->symbolFrame()->markDirty() ;
	    }
	 else
//	    return false ;	      // frames corrupted?
	    return true ;
	 }
      facet = facet->rest() ;
      }
   return true ;
}

//----------------------------------------------------------------------

static bool __FrCDECL rename_inverse_links(FrFrame *frame, ...)
{
   va_list args ;
   va_start(args,frame) ;
   bool result = frame->doSlots(rename_inverse_link,args) ;
   va_end(args) ;
   return result ;
}

//----------------------------------------------------------------------

static bool lock_linked_frame(const FrFrame *frame, const FrSymbol *slot,
			      va_list args)
{
   FrSymbol *invslot = slot->inverseRelation() ;
   if (invslot)
      {
      FrVarArg(FrList **,locklist) ;
      const FrList *facet = frame->getValues(slot,false) ; // no inheritance
      while (facet)
	 {
	 FrSymbol *other = (FrSymbol*)facet->first() ;
	 if (other && other->symbolp() && other->isFrame() &&
	     !other->isLocked())
	    pushlist(other,*locklist) ;
	 facet = facet->rest() ;
	 }
      }
   return true ;     // successful
}

//----------------------------------------------------------------------

static bool __FrCDECL lock_linked_frames(FrFrame *frame, FrList **locklist,
					   ...)
{
   va_list args ;
   va_start(args,locklist) ;
   bool result = frame->doSlots(lock_linked_frame,args) ;
   va_end(args) ;
   if (result)
      result = FrSymbol::lockFrames(*locklist) ;
   return result ;
}

//----------------------------------------------------------------------

static bool unlock_linked_frames(FrList **locklist)
{
   while (*locklist)
      {
      FrSymbol *frame = (FrSymbol*)poplist(*locklist) ;
      if (!frame->unlockFrame())
	 {
	 pushlist(frame,*locklist) ;	  // add the failed frame back to list
	 return false ;
	 }
      }
   return true ;     // successful
}

//----------------------------------------------------------------------

bool FrFrame::renameFrame(FrSymbol *newname)
{
   if (newname->isFrame())
      {
      FrFrame *newfr = find_vframe_inline(newname) ;
      if (newfr && !newfr->emptyFrame())
	 return false ; // a non-empty frame with the new name exists!
      else
	 newname->deleteFrame() ;
      }
   FrList *locklist = 0 ;
   bool success ;
   if (!lock_linked_frames(this,&locklist,&locklist) ||
       !rename_inverse_links(this,name,newname))
      success = false ;
   else
      {
      FrSymbol *oldname = name ;
      oldname->setFrame(0) ;	 // old name no longer has associated frame
      newname->setFrame(this) ;	 // point new name at ourself
      name = newname ;		 // change our name
      dirty = true ;
      if (VFrame_Info && !VFrame_Info->renameFrame(oldname,newname))
	 success = false ;
      else
	 success = true ;
      newname->setFrame(this) ;	 // point new name at ourself
      }
   int retries = 0 ;
   while (!unlock_linked_frames(&locklist) && ++retries < 10)
      ;	
   return success ;
}

//----------------------------------------------------------------------

// Borland C++ won't inline functions with for or while loops; besides,
// an alternate version which inlined the initial if/else sequence by
// making the final condition a separate, out-lined helper function
// was actually *slower*
#ifdef __BORLANDC__
static
#else
inline
#endif
FrList **find_facet(const FrSlot *slot, const FrSymbol *facetname)
{
   if (facetname == symbolVALUE)
      return &((FrSlot*)slot)->value_facet ;
   else
      {
      for (FrFacet *facet = slot->other_facets ; facet ; facet = facet->next)
	 if (facet->facet_name == facetname)
	    return &facet->fillers ;
      return &catchall_facet ;	    // unknown name, return dummy pointer
      }
}

//----------------------------------------------------------------------

// Borland C++ won't inline functions with for or while loops; besides,
// an alternate version which inlined the initial if/else sequence by
// making the final condition a separate, out-lined helper function
// was actually *slower*
#ifdef __BORLANDC__
static
#else
inline
#endif
FrList **find_sem_facet(const FrSlot *slot)
{
   for (FrFacet *facet = slot->other_facets ; facet ; facet = facet->next)
      if (facet->facet_name == symbolSEM)
	    return &facet->fillers ;
   return &catchall_facet ;	    // unknown name, return dummy pointer
}

//----------------------------------------------------------------------

void FrFrame::eraseFiller(const FrSymbol *slotname, const FrSymbol *facetname,
			const FrObject *filler)
{
   for (FrSlot *slot = predef_slots ; slot ; slot = slot->next)
      if (slot->name == slotname)
	 {
	 FrList **facet ;
	 FrList *curr, *prev ;

	 facet = find_facet(slot,facetname) ;
	 for (curr = *facet, prev = 0 ; curr ; prev=curr, curr=curr->rest())
	    {
	    if (curr->first() == filler)
	       {
	       // undo reverse link if a relation and filler is a frame name
	       if (facetname == symbolVALUE && slotname->inverseRelation())
		  deleteInverse(slotname->inverseRelation(),
				(FrSymbol*)filler) ;
	       // and remove the filler from the current list of fillers
	       if (prev)
		  prev->replacd(curr->rest()) ;
	       else
		  (*facet) = curr->rest() ;
	       CALL_DEMON(if_deleted,name,slotname,facetname,filler) ;
	       free_object(curr->first()) ;
//	       curr->replacd(0) ;
	       delete (FrCons*)curr ;
	       dirty = true ;
	       return ;	 // don't check any more fillers, we're done
	       }
	    }
	 break ;	// this was the right slot, so bail out
	 }
}

//----------------------------------------------------------------------

void FrFrame::eraseFiller(const FrSymbol *slotname, const FrSymbol *facetname,
			const FrObject *filler, FrCompareFunc cmp)
{
   for (register FrSlot *slot = predef_slots ; slot ; slot = slot->next)
      if (slot->name == slotname)
	 {
	 FrList **facet ;
	 FrList *curr, *prev ;

	 facet = find_facet(slot,facetname) ;
	 for (curr = *facet, prev = 0 ; curr ; prev=curr, curr=curr->rest())
	    {
	    if (cmp(curr->first(),filler))
	       {
	       // undo reverse link if a relation and filler is a frame name
	       if (facetname == symbolVALUE && slotname->inverseRelation())
		  deleteInverse(slotname->inverseRelation(),
				(FrSymbol*)filler) ;
	       // and remove the filler from the current list of fillers
	       if (prev)
		  prev->replacd(curr->rest()) ;
	       else
		  (*facet) = curr->rest() ;
	       CALL_DEMON(if_deleted,name,slotname,facetname,filler) ;
	       free_object(curr->first()) ;
//	       curr->replacd(0) ;
	       delete (FrCons*)curr ;
	       dirty = true ;
	       return ;	 // don't check any more fillers, we're done
	       }
	    }
	 break ;  // bail out now, this was the requested slot
	 }
}

//----------------------------------------------------------------------

void FrFrame::eraseValue(const FrSymbol *slotname, const FrObject *filler)
{
   for (register FrSlot *slot = predef_slots ; slot ; slot = slot->next)
      if (slot->name == slotname)
	 {
	 FrList *curr, *prev ;

	 for (curr=slot->value_facet,prev=0 ; curr ; prev=curr, curr=curr->rest())
	    {
	    if (curr->first() == filler)
	       {
	       // undo reverse link if a relation and filler is a frame name
	       if (slotname->inverseRelation())
		  deleteInverse(slotname->inverseRelation(),
				(FrSymbol*)filler) ;
	       // and remove the filler from the current list of fillers
	       if (prev)
		  prev->replacd(curr->rest()) ;
	       else
		  slot->value_facet = curr->rest() ;
	       CALL_DEMON(if_deleted,name,slotname,symbolVALUE,filler) ;
	       free_object(curr->first()) ;
//	       curr->replacd(0) ;
	       delete (FrCons*)curr ;
	       dirty = true ;
	       return ;	 // don't check any more fillers, we're done
	       }
	    }
	 break ;  // bail out now, this was the requested slot
	 }
}

//----------------------------------------------------------------------

void FrFrame::eraseValue(const FrSymbol *slotname, const FrObject *filler,
			FrCompareFunc cmp)
{
   for (register FrSlot *slot = predef_slots ; slot ; slot = slot->next)
      if (slot->name == slotname)
	 {
	 FrList *curr, *prev ;

	 for (curr=slot->value_facet,prev=0 ; curr ; prev=curr, curr=curr->rest())
	    {
	    if (cmp(curr->first(),filler))
	       {
	       // undo reverse link if a relation and filler is a frame name
	       if (slotname->inverseRelation())
		  deleteInverse(slotname->inverseRelation(),
				(FrSymbol*)filler) ;
	       // and remove the filler from the current list of fillers
	       if (prev)
		  prev->replacd(curr->rest()) ;
	       else
		  slot->value_facet = curr->rest() ;
	       CALL_DEMON(if_deleted,name,slotname,symbolVALUE,filler) ;
	       free_object(curr->first()) ;
//	       curr->replacd(0) ;
	       delete (FrCons*)curr ;
	       dirty = true ;
	       return ;	 // don't check any more fillers, we're done
	       }
	    }
	 break ;	// bail out now, this was the requested slot
	 }
}

//----------------------------------------------------------------------

void FrFrame::eraseSem(const FrSymbol *slotname, const FrObject *filler)
{
   for (FrSlot *slot = predef_slots ; slot ; slot = slot->next)
      if (slot->name == slotname)
	 {
	 FrList **facet ;
	 FrList *curr, *prev ;

	 facet = find_sem_facet(slot) ;
	 for (curr = *facet, prev = 0 ; curr ; prev=curr, curr=curr->rest())
	    {
	    if (curr->first() == filler)
	       {
	       // remove the filler from the current list of fillers
	       if (prev)
		  prev->replacd(curr->rest()) ;
	       else
		  (*facet) = curr->rest() ;
	       CALL_DEMON(if_deleted,name,slotname,symbolSEM,filler) ;
	       free_object(curr->first()) ;
//	       curr->replacd(0) ;
	       delete (FrCons*)curr ;
	       dirty = true ;
	       return ;	 // don't check any more fillers, we're done
	       }
	    }
	 break ;	// this was the right slot, so bail out
	 }
}

//----------------------------------------------------------------------

void FrFrame::eraseSem(const FrSymbol *slotname, const FrObject *filler,
			FrCompareFunc cmp)
{
   for (FrSlot *slot = predef_slots ; slot ; slot = slot->next)
      if (slot->name == slotname)
	 {
	 FrList **facet ;
	 FrList *curr, *prev ;

	 facet = find_sem_facet(slot) ;
	 for (curr = *facet, prev = 0 ; curr ; prev=curr, curr=curr->rest())
	    {
	    if (cmp(curr->first(),filler))
	       {
	       // remove the filler from the current list of fillers
	       if (prev)
		  prev->replacd(curr->rest()) ;
	       else
		  (*facet) = curr->rest() ;
	       CALL_DEMON(if_deleted,name,slotname,symbolSEM,filler) ;
	       free_object(curr->first()) ;
//	       curr->replacd(0) ;
	       delete (FrCons*)curr ;
	       dirty = true ;
	       return ;	 // don't check any more fillers, we're done
	       }
	    }
	 break ;	// this was the right slot, so bail out
	 }
}

//----------------------------------------------------------------------

void FrFrame::eraseFacet(const FrSymbol *slotname, const FrSymbol *facetname)
{
   for (register FrSlot *slot = predef_slots ; slot ; slot = slot->next)
      if (slot->name == slotname)
	 {
	 FrList **facet ;

	 if (facetname == symbolVALUE)
	    {
	    FrSymbol *inverse ;

	    facet = find_facet(slot,facetname) ;
	    // undo any reverse links if this is a relation slot
	    if ((inverse = slotname->inverseRelation()) != 0)
	       {
	       FrList *fillers ;
	       for (fillers = *facet ; fillers ; fillers = fillers->rest())
		  {
		  FrObject *filler = fillers->first() ;
		  if (filler && filler->symbolp())
		     deleteInverse(inverse,(FrSymbol*)filler) ;
		  }
	       }
	    quick_erase_facet(*facet,name,slotname,facetname) ;
	    }
	 else
	    {
	    FrFacet *prev = 0 ;
	    FrFacet *facetrec = slot->other_facets ;

	    while (facetrec && facetrec->facet_name != facetname)
	       {
	       prev = facetrec ;
	       facetrec = facetrec->next ;
	       }
	    if (facetrec)
	       {
	       quick_erase_facet(facetrec->fillers,name,slotname,facetname) ;
	       if (prev)
		  prev->next = facetrec->next ;
	       else
		  slot->other_facets = facetrec->next ;
	       delete facetrec ;
	       }
	    }
	 dirty = true ;
	 return ;    // we've erased the requested facet, so quit now
	 }
}

//----------------------------------------------------------------------

void FrFrame::eraseSlot(const FrSymbol *slotname)
{
   FrSlot *prev = 0 ;

   for (register FrSlot *slot=predef_slots ; slot ; prev=slot, slot=slot->next)
      if (slot->name == slotname)
	 {
	 while (slot->other_facets)
	    {
	    FrFacet *facet = slot->other_facets ;
	
	    quick_erase_facet(facet->fillers,name,slotname,facet->facet_name) ;
	    dirty = true ;
	    slot->other_facets = facet->next ;
	    delete facet ;
	    }
	 //
	 // finally, erase the VALUE facet, which may require deleting reverse
	 // links if this is a relation slot
	 if (slot->value_facet)
	    {
	    FrSymbol *inverse ;
	
	    if ((inverse = slotname->inverseRelation()) != 0)
	       {
	       FrList *fillers ;
	
	       for (fillers=slot->value_facet ; fillers ; fillers=fillers->rest())
		  {
		  FrObject *filler = fillers->first() ;
		  if (filler && filler->symbolp())
		     deleteInverse(inverse,(FrSymbol*)filler) ;
		  }
	       }
	    quick_erase_facet(slot->value_facet,name,slotname,symbolVALUE) ;
	    dirty = true ;
	    }
	 if (!is_predefined_slot((IFrame*)this,slot))
	    {
	    if (slot == last_slot)
	       last_slot = prev ;
	    prev->next = slot->next ;
	    delete slot ;
	    }
	 return ;     // we've erased the requested slot, so bail out
	 }
}

//----------------------------------------------------------------------

void FrFrame::eraseSlot(const char *slotname)
{
   FrSymbol *sym = FrSymbolTable::current()->lookup(slotname) ;

   if (sym)
      eraseSlot(sym) ;
}

//----------------------------------------------------------------------

void FrFrame::discard()
{
   // erase and then free the frame's slots (without erasing inverse links)
   eraseCopyFrame() ;
   // and delete the frame object
   delete this ;
}

//----------------------------------------------------------------------

void FrFrame::eraseCopyFrame()
{
   register FrSlot *slot ;
   // erase all the slots in the frame, without removing inverse links
   for (slot = predef_slots ; slot ; slot = slot->next)
      {
      if (slot->value_facet)
	 {
	 quick_erase_facet(slot->value_facet,name,slot->name,symbolVALUE) ;
	 dirty = true ;
	 }
      while (slot->other_facets)
	 {
	 FrFacet *facet = slot->other_facets ;

	 quick_erase_facet(facet->fillers,name,slot->name,facet->facet_name) ;
	 dirty = true ;
	 slot->other_facets = facet->next ;
	 delete facet ;
	 }
      }
   // now free up all the slots except the predefined ones
   last_slot = slot = &predef_slots[lengthof(predef_slots)-1] ;
   while (slot->next)
      {
      FrSlot *curr = slot->next ;

      slot->next = curr->next ;
      delete curr ;
      }
}

//----------------------------------------------------------------------

const FrList *FrFrame::isA_list() const
{
   return predef_slots[ISA_slot].value_facet ;
}

//----------------------------------------------------------------------

FrSymbol *FrFrame::isA() const
{
   FrList *val = predef_slots[ISA_slot].value_facet ;

   return val ? (FrSymbol *)val->first() : 0 ;
}

//----------------------------------------------------------------------

FrSymbol *FrFrame::instanceOf() const
{
   FrList *val = predef_slots[INSTANCEOF_slot].value_facet ;

   return val ? (FrSymbol *)val->first() : 0 ;
}

//----------------------------------------------------------------------

const FrList *FrFrame::instanceOf_list() const
{
   return predef_slots[INSTANCEOF_slot].value_facet ;
}

//----------------------------------------------------------------------

FrSymbol *FrFrame::partOf() const
{
   FrList *val = predef_slots[PARTOF_slot].value_facet ;

   return val ? (FrSymbol *)val->first() : 0 ;
}

//----------------------------------------------------------------------

const FrList *FrFrame::partOf_list() const
{
   return predef_slots[PARTOF_slot].value_facet ;
}

//----------------------------------------------------------------------

const FrList *FrFrame::subclassesOf() const
{
   return predef_slots[SUBCLASSES_slot].value_facet ;
}

//----------------------------------------------------------------------

const FrList *FrFrame::instances() const
{
   return getImmedValues(symbolINSTANCES) ;
}

//----------------------------------------------------------------------

const FrList *FrFrame::hasParts() const
{
   return getImmedValues(symbolHASPARTS) ;
}

//----------------------------------------------------------------------

static bool isa_p_helper(const FrFrame *frame, const FrFrame *poss_parent)
{
   for (FrList *fr = ((IFrame *)frame)->predef_slots[ISA_slot].value_facet ;
	fr ;
	fr = fr->rest())
      {
      FrObject *head = fr->first() ;
      if (head && head->symbolp())
	 {
	 head = find_vframe_inline((FrSymbol *)head) ;
	 if (!head || (FrFrame *)head == frame) // top of hierarchy if own parent
	    return false ;
	 else if ((FrFrame *)head == poss_parent ||
		  isa_p_helper((FrFrame *)head,poss_parent))
	    return true ;
	 }
      }
   return false ;
}

//----------------------------------------------------------------------

bool FrFrame::isA_p(const FrFrame *poss_parent) const
{
   if (this == poss_parent)
      return true ;
   if (predef_slots[INSTANCEOF_slot].value_facet)
      {
      FrFrame *inst = find_vframe_inline(instanceOf()) ;
      if (inst && (inst == poss_parent || isa_p_helper(inst,poss_parent)))
	 return true ;
      }
   for (FrList *fr = predef_slots[ISA_slot].value_facet ; fr ; fr=fr->rest())
      {
      FrObject *head = fr->first() ;
      if (head && head->symbolp())
	 {
	 head = find_vframe_inline((FrSymbol *)head) ;
	 if (!head || (FrFrame *)head == this)
	    return false ;
	 else if ((FrFrame *)head == poss_parent ||
		  isa_p_helper((FrFrame *)head,poss_parent))
	   return true ;
	 }
      }
   return false ;
}

//----------------------------------------------------------------------

static bool partOf_p_helper(const FrFrame *frame, const FrFrame *poss_cont)
{
   for (FrList *fr = ((IFrame *)frame)->predef_slots[PARTOF_slot].value_facet ;
	fr ;
	fr = fr->rest())
      {
      FrObject *head = fr->first() ;

      if (head && head->symbolp())
	 {
	 head = find_vframe_inline((FrSymbol *)head) ;
	 if ((FrFrame *)head == frame)	 // top of hierarchy if own parent
	    return false ;
	 else if ((FrFrame *)head == poss_cont ||
		  partOf_p_helper((FrFrame *)head,poss_cont))
	    return true ;
	 }
      }
   return false ;
}

//----------------------------------------------------------------------

bool FrFrame::partOf_p(const FrFrame *poss_container) const
{
   FrFrame *inst ;

   if (this == poss_container)
      return true ;
   else if ((inst = find_vframe_inline(instanceOf())) != 0 &&
	    (inst == poss_container || partOf_p_helper(inst,poss_container)))
      return true ;
   else
      {
      for (FrList *fr=predef_slots[PARTOF_slot].value_facet ; fr ; fr=fr->rest())
	 {
	 FrObject *head = fr->first() ;

	 if (head && head->symbolp())
	    {
	    head = find_vframe_inline((FrSymbol *)head) ;
	    if ((FrFrame *)head == this)
	       return false ;
	    else if ((FrFrame *)head == poss_container ||
		     partOf_p_helper((FrFrame *)head,poss_container))
	       return true ;
	    }
	 }
      }
   return false ;
}

//----------------------------------------------------------------------

void FrFrame::addFiller(const FrSymbol *slotname, const FrSymbol *facetname,
		      const FrObject *filler)
{
   FrList **facet ;
   FrList *tail, *next ;
   FrSlot *slot = createSlot(slotname) ;

   // find or create the requested facet
   if (facetname == symbolVALUE)
      facet = &slot->value_facet ;
   else
      {
      facet = 0 ;
      FrFacet *f = slot->other_facets ;
      FrFacet *prev = 0 ;

      for (; f ; prev=f, f=f->next)
	 if (f->facet_name == facetname)
	    {
	    facet = &f->fillers ;
	    break ;
	    }
      if (!f)
	 {
	 FramepaC_lock_Frame_1 = this ;
	 if (prev)
	    {
	    prev->next = new FrFacet(facetname) ;
	    facet = &prev->next->fillers ;
	    }
	 else
	    {
	    slot->other_facets = new FrFacet(facetname) ;
	    facet = &slot->other_facets->fillers ;
	    }
	 FramepaC_lock_Frame_1 = 0 ;
	 CALL_DEMON(if_created,name,slotname,facetname,0) ;
	 }
      }
   tail = *facet ;
   if (tail)
      {
      for (;;)
	 {
	 if (equal_inline(tail->first(),filler)) // is it what we want added?
	    return ;				 // if yes, bail out now
	 next = tail->rest() ;
	 if (!next)			// if no more fillers, tail is last
	    break ;
	 else
	    tail = next ;		// move to next filler in list
	 }
      // the object to be added wasn't already on the filler list, so bash it
      // onto the end of the filler list
      CALL_DEMON(if_added,name,slotname,facetname,filler) ;
      tail->replacd(new FrList(filler?filler->deepcopy():0)) ;
      }
   else
      {
      CALL_DEMON(if_added,name,slotname,facetname,filler) ;
      *facet = new FrList(filler?filler->deepcopy():0) ;
      }
   dirty = true ;	      // we've changed the frame
   // now, see whether this is a relation for which we need to maintain an
   // inverse
   if (facetname == symbolVALUE && !omit_inverse_links &&
       slotname->inverseRelation() && filler && filler->symbolp())
      insertInverse(slotname->inverseRelation(),(FrSymbol*)filler) ;
}

//----------------------------------------------------------------------

void FrFrame::pushFiller(const FrSymbol *slotname, const FrSymbol *facetname,
		      const FrObject *filler)
{
   FrList **facet ;
   FrSlot *slot = createSlot(slotname) ;

   // find or create the requested facet
   if (facetname == symbolVALUE)
      facet = &slot->value_facet ;
   else
      {
      facet = 0 ;
      FrFacet *f = slot->other_facets ;
      FrFacet *prev = 0 ;

      for (; f ; prev=f, f=f->next)
	 if (f->facet_name == facetname)
	    {
	    facet = &f->fillers ;
	    break ;
	    }
      if (!f)
	 {
	 FramepaC_lock_Frame_1 = this ;
	 if (prev)
	    {
	    prev->next = new FrFacet(facetname) ;
	    facet = &prev->next->fillers ;
	    }
	 else
	    {
	    slot->other_facets = new FrFacet(facetname) ;
	    facet = &slot->other_facets->fillers ;
	    }
	 FramepaC_lock_Frame_1 = 0 ;
	 CALL_DEMON(if_created,name,slotname,facetname,0) ;
	 }
      }
   if ((*facet)->member(filler))
      return ;
   CALL_DEMON(if_added,name,slotname,facetname,filler) ;
   pushlist(filler ? filler->deepcopy() : 0,*facet) ;
   dirty = true ;	      // we've changed the frame
   // now, see whether this is a relation for which we need to maintain an
   // inverse
   if (facetname == symbolVALUE && !omit_inverse_links &&
       slotname->inverseRelation() && filler && filler->symbolp())
      insertInverse(slotname->inverseRelation(),(FrSymbol*)filler) ;
}

//----------------------------------------------------------------------

void FrFrame::addFillers(const FrSymbol *slotname,
			  const FrSymbol *facetname,
			  const FrList *fillers)
{
   for (register FrList *f = (FrList *)fillers ;
	f && f->consp() ;
	f = f->rest())
      addFiller(slotname,facetname,f->first()) ;
}

//----------------------------------------------------------------------

void FrFrame::addValue(const FrSymbol *slotname, const FrObject *filler)
{
   FrSlot *slot = createSlot(slotname) ;
   FrList *tail = slot->value_facet ;
   FrList *next ;

   if (tail)
      {
      for (;;)
	 {
	 if (equal_inline(tail->first(),filler)) // is it what we want added?
	    return ;				 // if yes, bail out now
	 next = tail->rest() ;
	 if (!next)			// if no more fillers, tail is last
	    break ;
	 else
	    tail = next ;		// move to next filler in list
	 }
      // the object to be added wasn't already on the filler list, so bash it
      // onto the end of the filler list
      CALL_DEMON(if_added,name,slotname,symbolVALUE,filler) ;
      tail->replacd(new FrList(filler?filler->deepcopy():0)) ;
      }
   else
      {
      CALL_DEMON(if_added,name,slotname,symbolVALUE,filler) ;
      slot->value_facet = new FrList(filler?filler->deepcopy():0) ;
      }
   dirty = true ;		       // we've changed the frame
   // now, see whether this is a relation for which we need to maintain an
   // inverse
   if (slotname->inverseRelation() && filler && filler->symbolp())
      insertInverse(slotname->inverseRelation(),(FrSymbol*)filler) ;
}

#ifdef _MSC_VER
// re-enable Visual C++ 'constant conditional expression' warnings
#pragma warning(default : 4127)
#endif /* _MSC_VER */

//----------------------------------------------------------------------

void FrFrame::addValues(const FrSymbol *slotname, const FrList *fillers)
{
   for (register FrList *f = (FrList *)fillers ;
	f && f->consp() ;
	f = f->rest())
      addValue(slotname,f->first()) ;
}

//----------------------------------------------------------------------

void FrFrame::addSem(const FrSymbol *slotname, const FrObject *filler)
{
   FrList **facet ;
   FrList *tail, *next ;
   FrSlot *slot = createSlot(slotname) ;

   // find or create the requested facet
   facet = 0 ;
   FrFacet *f = slot->other_facets ;
   FrFacet *prev = 0 ;

   for (; f ; prev=f, f=f->next)
      if (f->facet_name == symbolSEM)
	 {
	 facet = &f->fillers ;
	 break ;
	 }
   if (!f)
      {
      FramepaC_lock_Frame_1 = this ;
      if (prev)
	 {
	 prev->next = new FrFacet(symbolSEM) ;
	 facet = &prev->next->fillers ;
	 }
      else
	 {
	 slot->other_facets = new FrFacet(symbolSEM) ;
	 facet = &slot->other_facets->fillers ;
	 }
      FramepaC_lock_Frame_1 = 0 ;
      CALL_DEMON(if_created,name,slotname,symbolSEM,0) ;
      }
   tail = *facet ;
   if (tail)
      {
      for (;;)
	 {
	 if (equal_inline(tail->first(),filler)) // is it what we want added?
	    return ;				 // if yes, bail out now
	 next = tail->rest() ;
	 if (!next)			// if no more fillers, tail is last
	    break ;
	 else
	    tail = next ;		// move to next filler in list
	 }
      // the object to be added wasn't already on the filler list, so bash it
      // onto the end of the filler list
      CALL_DEMON(if_added,name,slotname,symbolSEM,filler) ;
      tail->replacd(new FrList(filler?filler->deepcopy():0)) ;
      }
   else
      {
      CALL_DEMON(if_added,name,slotname,symbolSEM,filler) ;
      *facet = new FrList(filler?filler->deepcopy():0) ;
      }
   dirty = true ;	      // we've changed the frame
}

//----------------------------------------------------------------------

void FrFrame::addSems(const FrSymbol *slotname, const FrList *fillers)
{
   for (register FrList *f = (FrList *)fillers ;
	f && f->consp() ;
	f = f->rest())
      addSem(slotname,f->first()) ;
}

//----------------------------------------------------------------------

void FrFrame::replaceFiller(const FrSymbol *slotname,
			     const FrSymbol *facetname,
			     const FrObject *oldfiller,
			     const FrObject *newfiller)
{
   if (oldfiller == newfiller)
      return ;				  // do nothing if no effect anyway
   for (register FrSlot *slot = predef_slots ; slot ; slot = slot->next)
      if (slot->name == slotname)
	 {
	 FrList **facet ;
	 FrList *prev, *tail ;

	 facet = find_facet(slot,facetname) ;
	 tail = *facet ;
	 prev = 0 ;
	 while (tail && tail->first() != oldfiller)
	    {
	    prev = tail ;
	    tail = tail->rest() ;
	    }
	 if (!tail)	      // did we find the old filler?
	    return ;
	 else
	    {
	    if (facetname == symbolVALUE &&
		slotname->inverseRelation() &&
		oldfiller && oldfiller->symbolp())
	       deleteInverse(slotname->inverseRelation(),
			     (FrSymbol*)oldfiller) ;
	    if (prev)
	       prev->replacd(tail->rest()) ;
	    else
	       *facet = tail->rest() ;
	    CALL_DEMON(if_deleted,name,slotname,facetname,oldfiller) ;
	    free_object(tail->first()) ;
//	    tail->replacd(0) ;
	    delete (FrCons*)tail ;
	    }
	 dirty = true ;	      // we've changed the frame
	 tail = *facet ;
	 prev = 0 ;
	 while (tail && tail->first() != newfiller)
	    {
	    prev = tail ;
	    tail = tail->rest() ;
	    }
	 CALL_DEMON(if_added,name,slotname,facetname,newfiller) ;
	 if (!tail)
	    {
	    // the new filler wasn't already on the filler list, so bash it
	    // onto the end of the filler list
	    if (prev)
	       prev->replacd(new FrList(newfiller?newfiller->deepcopy():0)) ;
	    else
	       *facet = new FrList(newfiller?newfiller->deepcopy():0) ;
	    // now, see whether this is a relation for which we need to
	    // maintain an inverse
	    if (facetname == symbolVALUE &&
		slotname->inverseRelation() &&
		newfiller && newfiller->symbolp())
	       insertInverse(slotname->inverseRelation(),
			     (FrSymbol*)newfiller) ;
	    }
	 }
}

//----------------------------------------------------------------------

void FrFrame::replaceFiller(const FrSymbol *slotname, const FrSymbol *facetname,
			  const FrObject *oldfiller, const FrObject *newfiller,
			  FrCompareFunc cmp)
{
   if (oldfiller == newfiller)
      return ;				  // do nothing if no effect anyway
   for (register FrSlot *slot = predef_slots ; slot ; slot = slot->next)
      if (slot->name == slotname)
	 {
	 FrList **facet ;
	 FrList *prev, *tail ;
	
	 facet = find_facet(slot,facetname) ;
	 tail = *facet ;
	 prev = 0 ;
	 while (tail && !cmp(tail->first(),oldfiller))
	    {
	    prev = tail ;
	    tail = tail->rest() ;
	    }
	 if (!tail)	      // did we find the old filler?
	    return ;
	 else
	    {
	    if (facetname == symbolVALUE &&
		slotname->inverseRelation() &&
		oldfiller && oldfiller->symbolp())
	       deleteInverse(slotname->inverseRelation(),
			     (FrSymbol*)oldfiller) ;
	    if (prev)
	       prev->replacd(tail->rest()) ;
	    else
	       *facet = tail->rest() ;
	    CALL_DEMON(if_deleted,name,slotname,facetname,oldfiller) ;
	    free_object(tail->first()) ;
//	    tail->replacd(0) ;
	    delete (FrCons*)tail ;
	    }
	 dirty = true ;	      // we've changed the frame
	 tail = *facet ;
	 prev = 0 ;
	 while (tail && !cmp(tail->first(),newfiller))
	    {
	    prev = tail ;
	    tail = tail->rest() ;
	    }
	 CALL_DEMON(if_added,name,slotname,facetname,newfiller) ;
	 if (!tail)
	    {
	    // the new filler wasn't already on the filler list, so bash it
	    // onto the end of the filler list
	    if (prev)
	       prev->replacd(new FrList(newfiller?newfiller->deepcopy():0)) ;
	    else
	       *facet = new FrList(newfiller?newfiller->deepcopy():0) ;
	    // now, see whether this is a relation for which we need to
	    // maintain an inverse
	    if (facetname == symbolVALUE && slotname->inverseRelation() &&
		newfiller && newfiller->symbolp())
	       insertInverse(slotname->inverseRelation(),
			     (FrSymbol*)newfiller) ;
	    }
	 }
}

//----------------------------------------------------------------------

void FrFrame::replaceValue(const FrSymbol *slotname, const FrObject *oldfiller,
			 const FrObject *newfiller)
{
   if (oldfiller == newfiller)
      return ;				  // do nothing if no effect anyway
   for (register FrSlot *slot = predef_slots ; slot ; slot = slot->next)
      if (slot->name == slotname)
	 {
	 FrList *prev, *tail ;

	 tail = slot->value_facet ;
	 prev = 0 ;
	 while (tail && tail->first() != oldfiller)
	    {
	    prev = tail ;
	    tail = tail->rest() ;
	    }
	 if (!tail)	      // did we find the old filler?
	    return ;
	 else
	    {
	    if (slotname->inverseRelation() && oldfiller &&
		oldfiller->symbolp())
	       deleteInverse(slotname->inverseRelation(),
			     (FrSymbol*)oldfiller) ;
	    if (prev)
	       prev->replacd(tail->rest()) ;
	    else
	       slot->value_facet = tail->rest() ;
	    CALL_DEMON(if_deleted,name,slotname,symbolVALUE,oldfiller) ;
	    free_object(tail->first()) ;
//	    tail->replacd(0) ;
	    delete (FrCons*)tail ;
	    }
	 dirty = true ;	      // we've changed the frame
	 tail = slot->value_facet ;
	 prev = 0 ;
	 while (tail && tail->first() != newfiller)
	    {
	    prev = tail ;
	    tail = tail->rest() ;
	    }
	 CALL_DEMON(if_added,name,slotname,symbolVALUE,newfiller) ;
	 if (!tail)
	    {
	    // the new filler wasn't already on the filler list, so bash it
	    // onto the end of the filler list
	    tail->replacd(new FrList(newfiller?newfiller->deepcopy():0)) ;
	    // now, see whether this is a relation for which we need to
	    // maintain an inverse
	    if (slotname->inverseRelation() &&
		newfiller && newfiller->symbolp())
	       insertInverse(slotname->inverseRelation(),
			     (FrSymbol*)newfiller) ;
	    }
	 }
}

//----------------------------------------------------------------------

void FrFrame::replaceValue(const FrSymbol *slotname, const FrObject *oldfiller,
			 const FrObject *newfiller, FrCompareFunc cmp)
{
   if (oldfiller == newfiller)
      return ;				  // do nothing if no effect anyway
   for (register FrSlot *slot = predef_slots ; slot ; slot = slot->next)
      if (slot->name == slotname)
	 {
	 FrList *prev, *tail ;

	 tail = slot->value_facet ;
	 prev = 0 ;
	 while (tail && !cmp(tail->first(),oldfiller))
	    {
	    prev = tail ;
	    tail = tail->rest() ;
	    }
	 if (!tail)	      // did we find the old filler?
	    return ;
	 else
	    {
	    if (slotname->inverseRelation() &&
		oldfiller && oldfiller->symbolp())
	       deleteInverse(slotname->inverseRelation(),
			     (FrSymbol*)oldfiller) ;
	    if (prev)
	       prev->replacd(tail->rest()) ;
	    else
	       slot->value_facet = tail->rest() ;
	    CALL_DEMON(if_deleted,name,slotname,symbolVALUE,oldfiller) ;
	    free_object(tail->first()) ;
//	    tail->replacd(0) ;
	    delete (FrCons*)tail ;
	    }
	 dirty = true ;	      // we've changed the frame
	 tail = slot->value_facet ;
	 prev = 0 ;
	 while (tail && !cmp(tail->first(),newfiller))
	    {
	    prev = tail ;
	    tail = tail->rest() ;
	    }
	 CALL_DEMON(if_added,name,slotname,symbolVALUE,newfiller) ;
	 if (!tail)
	    {
	    // the new filler wasn't already on the filler list, so bash it
	    // onto the end of the filler list
	    tail->replacd(new FrList(newfiller?newfiller->deepcopy():0)) ;
	    // now, see whether this is a relation for which we need to
	    // maintain an inverse
	    if (slotname->inverseRelation() &&
		newfiller && newfiller->symbolp())
	       insertInverse(slotname->inverseRelation(),
			     (FrSymbol*)newfiller) ;
	    }
	 }
}

//----------------------------------------------------------------------

void FrFrame::replaceSem(const FrSymbol *slotname, const FrObject *oldfiller,
		       const FrObject *newfiller)
{
   if (oldfiller == newfiller)
      return ;				  // do nothing if no effect anyway
   for (register FrSlot *slot = predef_slots ; slot ; slot = slot->next)
      if (slot->name == slotname)
	 {
	 FrList **facet ;
	 FrList *prev, *tail ;

	 facet = find_sem_facet(slot) ;
	 tail = *facet ;
	 prev = 0 ;
	 while (tail && tail->first() != oldfiller)
	    {
	    prev = tail ;
	    tail = tail->rest() ;
	    }
	 if (!tail)	      // did we find the old filler?
	    return ;
	 else
	    {
	    if (prev)
	       prev->replacd(tail->rest()) ;
	    else
	       *facet = tail->rest() ;
	    CALL_DEMON(if_deleted,name,slotname,symbolSEM,oldfiller) ;
	    free_object(tail->first()) ;
//	    tail->replacd(0) ;
	    delete (FrCons*)tail ;
	    }
	 dirty = true ;	      // we've changed the frame
	 tail = *facet ;
	 prev = 0 ;
	 while (tail && tail->first() != newfiller)
	    {
	    prev = tail ;
	    tail = tail->rest() ;
	    }
	 if (!tail)
	    {
	    CALL_DEMON(if_added,name,slotname,symbolSEM,newfiller) ;
	    // the new filler wasn't already on the filler list, so bash it
	    // onto the end of the filler list
	    if (prev)
	       prev->replacd(new FrList(newfiller?newfiller->deepcopy():0)) ;
	    else
	       *facet = new FrList(newfiller?newfiller->deepcopy():0) ;
	    }
	 }
}

//----------------------------------------------------------------------

void FrFrame::replaceSem(const FrSymbol *slotname, const FrObject *oldfiller,
		       const FrObject *newfiller, FrCompareFunc cmp)
{
   if (cmp(oldfiller,newfiller))
      return ;				  // do nothing if no effect anyway
   for (register FrSlot *slot = predef_slots ; slot ; slot = slot->next)
      if (slot->name == slotname)
	 {
	 FrList **facet ;
	 FrList *prev, *tail ;

	 facet = find_sem_facet(slot) ;
	 tail = *facet ;
	 prev = 0 ;
	 while (tail && !cmp(tail->first(),oldfiller))
	    {
	    prev = tail ;
	    tail = tail->rest() ;
	    }
	 if (!tail)	      // did we find the old filler?
	    return ;
	 else
	    {
	    if (prev)
	       prev->replacd(tail->rest()) ;
	    else
	       *facet = tail->rest() ;
	    CALL_DEMON(if_deleted,name,slotname,symbolSEM,oldfiller) ;
	    free_object(tail->first()) ;
//	    tail->replacd(0) ;
	    delete (FrCons*)tail ;
	    }
	 dirty = true ;	      // we've changed the frame
	 tail = *facet ;
	 prev = 0 ;
	 while (tail && !cmp(tail->first(),newfiller))
	    {
	    prev = tail ;
	    tail = tail->rest() ;
	    }
	 if (!tail)
	    {
	    CALL_DEMON(if_added,name,slotname,symbolSEM,newfiller) ;
	    // the new filler wasn't already on the filler list, so bash it
	    // onto the end of the filler list
	    if (prev)
	       prev->replacd(new FrList(newfiller?newfiller->deepcopy():0)) ;
	    else
	       *facet = new FrList(newfiller?newfiller->deepcopy():0) ;
	    }
	 }
}

//----------------------------------------------------------------------

void FrFrame::replaceFacet(const FrSymbol *slotname, const FrSymbol *facetname,
			 const FrList *newfillers)
{
   for (register FrSlot *slot = predef_slots ; slot ; slot = slot->next)
      if (slot->name == slotname)
	 {
	 FrList **facet = find_facet(slot,facetname) ;
	 FrObject *oldfiller ;
	 if (facetname == symbolVALUE &&
	     slotname->inverseRelation())
	    {
	    while (*facet)
	       {
	       oldfiller = poplist(*facet) ;
	       CALL_DEMON(if_deleted,name,slotname,facetname,oldfiller) ;
	       if (oldfiller && oldfiller->symbolp())
		  deleteInverse(slotname->inverseRelation(),
				(FrSymbol*)oldfiller) ;
	       free_object(oldfiller) ;
	       }
	    }
	 else
#ifdef FrDEMONS
	    while (*facet)
	       {
	       oldfiller = poplist(*facet) ;
	       CALL_DEMON(if_deleted,name,slotname,facetname,oldfiller) ;
	       free_object(oldfiller) ;
	       }
#else
	    if (*facet)
	       {
	       (*facet)->eraseList(true) ;
	       *facet = 0 ;
	       }
#endif /* FrDEMONS */
	 dirty = true ;	      // we've changed the frame
	 FrList *fillers = *facet = (FrList*)(newfillers ? newfillers->deepcopy()
						     : 0) ;
	 // now, see whether this is a relation for which we need to
	 // maintain an inverse
	 if (facetname == symbolVALUE &&
	     slotname->inverseRelation())
	    {
	    while (fillers)
	       {
	       FrObject *newfiller = fillers->first() ;
	       CALL_DEMON(if_added,name,slotname,facetname,newfiller) ;
	       if (newfiller && newfiller->symbolp())
		  insertInverse(slotname->inverseRelation(),
				(FrSymbol*)newfiller) ;
	       }
	    fillers = fillers->rest() ;
	    }
#ifdef FrDEMONS
	 else
	    while (fillers)
	       {
	       CALL_DEMON(if_added,name,slotname,facetname,fillers->first()) ;
	       fillers = fillers->rest() ;
	       }
#endif /* FrDEMONS */
	
	 }
}

//----------------------------------------------------------------------
// Don't do any inheritance at all

/* ARGSUSED */
const FrList *inheritFillersNone(FrFrame *frame,
				  const FrSymbol *slot,
				  const FrSymbol *facet)
{
   // avoid compiler warnings by trivially using each argument
   (void)frame ; (void)slot ; (void)facet ;
   return 0 ;
}

//----------------------------------------------------------------------
//  Follow up the first filler of the INSTANCE-OF/IS-A links

const FrList *inheritFillersSimple(FrFrame *frame,
				    const FrSymbol *slot,
				    const FrSymbol *facet)
{
   FrSymbol *parentname ;
   FrSymbol *firstparent ;
   FrList *pfillers ;
   pfillers = ((IFrame*)frame)->predef_slots[INSTANCEOF_slot].value_facet ;
   firstparent =
   parentname = pfillers
		  ? (FrSymbol*)pfillers->first()
		  : (((IFrame*)frame)->predef_slots[ISA_slot].value_facet
		      ? (FrSymbol *)(((IFrame*)frame)->predef_slots[ISA_slot].value_facet->first())
		      : 0) ;
   const FrList *fillers = 0 ;
   while (parentname)
      {
      frame = find_vframe_inline(parentname) ;
      if (!frame || frame->wasVisited())
	 break ;
      fillers = frame->getImmedFillers(slot,facet) ;
      if (fillers)
	 break ;
      pfillers = ((IFrame*)frame)->predef_slots[ISA_slot].value_facet ;
      frame->markVisited() ;
      parentname = !pfillers ? 0 : (FrSymbol*)pfillers->first() ;
      }
   // chase back up the IS-A links, clearing the 'visited' flag
   parentname = firstparent ;
   while (parentname)
      {
      frame = find_vframe_inline(parentname) ;
      if (!frame || !frame->wasVisited())
	 break ;
      pfillers = ((IFrame*)frame)->predef_slots[ISA_slot].value_facet ;
      frame->clearVisited() ;
      parentname = !pfillers ? 0 : (FrSymbol*)pfillers->first() ;
      }
   return fillers ;
}

//----------------------------------------------------------------------
//  Full depth-first search of the INSTANCE-OF/IS-A links

static const FrList *inheritFillersDFS2(FrFrame *frame,
					 const FrSymbol *slot,
					 const FrSymbol *facet)
{
   frame->markVisited() ;
   FrList *parents = ((IFrame*)frame)->predef_slots[ISA_slot].value_facet ;
   for ( ; parents ; parents = parents->rest())
      {
      FrSymbol *psym = (FrSymbol*)parents->first() ;
      if (!psym || !psym->symbolp())
	 continue ;  // no can do... so try next parent
      FrFrame *parent = find_vframe_inline(psym) ;
      if (parent == 0 || parent->wasVisited())
	 continue ;  // no can do... or we looped; so try next parent
      const FrList *fillers = parent->getImmedFillers(slot,facet) ;
      if (fillers)
	 {
	 frame->clearVisited() ;
	 return fillers ;
	 }
      // unroll the recursion by one level
      FrList *parents2 = ((IFrame *)parent)->predef_slots[ISA_slot].value_facet ;
      for ( ; parents2 ; parents2 = parents2->rest())
	 {
	 psym = (FrSymbol *)parents2->first() ;
	 if (!psym || !psym->symbolp() ||
	     (parent = find_vframe_inline(psym)) == 0 || parent->wasVisited())
	    continue ;	// no can do... or we looped; so try next parent
	 fillers = parent->getImmedFillers(slot,facet) ;
	 if (fillers)
	    {
	    frame->clearVisited() ;
	    return fillers ;
	    }
	 // unroll the recursion by a second level
	 FrList *parents3=((IFrame*)parent)->predef_slots[ISA_slot].value_facet ;
	 for ( ; parents3 ; parents3 = parents3->rest())
	    {
	    psym = (FrSymbol*)parents3->first() ;
	    if (psym && psym->symbolp() &&
		(parent = find_vframe_inline(psym)) != 0 &&
		!parent->wasVisited())
	       {
	       fillers = parent->getImmedFillers(slot,facet) ;
	       if (fillers ||
		   (fillers = inheritFillersDFS2((IFrame *)parent,slot,facet))
		      != 0)
		  {
		  frame->clearVisited() ;
		  return fillers ;
		  }
	       }
	    }
	 }
      }
   frame->clearVisited() ;
   return 0 ;	   // unable to inherit anything down to this frame
}

//----------------------------------------------------------------------

const FrList *inheritFillersDFS(FrFrame *frame,
				 const FrSymbol *slot,
				 const FrSymbol *facet)
{
   FrList *parents ;

   frame->markVisited() ;
   parents = ((IFrame*)frame)->predef_slots[INSTANCEOF_slot].value_facet ;
   if (!parents)
      parents = ((IFrame*)frame)->predef_slots[ISA_slot].value_facet ;
   for ( ; parents ; parents = parents->rest())
      {
      FrSymbol *psym = (FrSymbol*)parents->first() ;
      if (!psym || !psym->symbolp())
	 continue ;  // no can do... so try next parent
      FrFrame *parent = find_vframe_inline(psym) ;
      if (!parent || parent->wasVisited())
	 continue ;  // no can do... or we looped; so try next parent
      const FrList *fillers = parent->getImmedFillers(slot,facet) ;
      if (fillers)
	 {
	 frame->clearVisited() ;
	 return fillers ;
	 }
      // unroll the recursion by one level
      FrList *parents2 = ((IFrame *)parent)->predef_slots[ISA_slot].value_facet ;
      for ( ; parents2 ; parents2 = parents2->rest())
	 {
	 psym = (FrSymbol *)parents2->first() ;
	 if (!psym || !psym->symbolp())
	    continue ;	// no can do... so try next parent
	 parent = find_vframe_inline(psym) ;
	 if (!parent || parent->wasVisited())
	    continue ;	// no can do... or we looped; so try next parent
	 fillers = parent->getImmedFillers(slot,facet) ;
	 if (fillers)
	    {
	    frame->clearVisited() ;
	    return fillers ;
	    }
	 // unroll the recursion by a second level
	 FrList *parents3=((IFrame*)parent)->predef_slots[ISA_slot].value_facet ;
	 for ( ; parents3 ; parents3 = parents3->rest())
	    {
	    psym = (FrSymbol*)parents3->first() ;
#ifdef __GNUC__
	    parent = 0 ;
#endif /* __GNUC__ */
	    if (psym && psym->symbolp() &&
		(parent = find_vframe_inline(psym)) != 0 &&
		!parent->wasVisited())
	       {
	       fillers = parent->getImmedFillers(slot,facet) ;
	       if (fillers ||
		   (fillers = inheritFillersDFS2(parent,slot,facet)) != 0)
		  {
		  frame->clearVisited() ;
		  return fillers ;
		  }
	       }
	    }
	 }
      }
   frame->clearVisited() ;
   return 0 ;	   // unable to inherit anything down to this frame
}

//----------------------------------------------------------------------

const FrList *inheritFillersPartOfDFS(FrFrame *frame,
				       const FrSymbol *slot,
				       const FrSymbol *facet)
{
   FrList *parents ;

   frame->markVisited() ;
   parents = ((IFrame*)frame)->predef_slots[PARTOF_slot].value_facet ;
   for ( ; parents ; parents = parents->rest())
      {
      FrFrame *parent = find_vframe_inline((FrSymbol *)parents->first()) ;
      if (!parent || parent->wasVisited())
	 continue ;    // no can do... or we looped; so try next parent
      const FrList *fillers = parent->getImmedFillers(slot,facet) ;
      if (fillers)
	 {
	 frame->clearVisited() ;
	 return fillers ;
	 }
      // unroll the recursion by one level
      FrList *parents2=((IFrame*)parent)->predef_slots[PARTOF_slot].value_facet ;
      for ( ; parents2 ; parents2 = parents2->rest())
	 {
	 parent = find_vframe_inline((FrSymbol *)parents2->first()) ;
	 if (parent && !parent->wasVisited())
	    {
	    fillers = parent->getImmedFillers(slot,facet) ;
	    if (fillers ||
		(fillers = inheritFillersPartOfDFS((IFrame *)parent,slot,facet)) != 0)
	       {
	       parent->clearVisited() ;
	       return fillers ;
	       }
	    }
	 }
      }
   frame->clearVisited() ;
   return 0 ;	   // unable to inherit anything down to this frame
}

//----------------------------------------------------------------------

static void insert_in_queue(const FrList *parents, FrList *&queue,
			    FrList *&qtail)
{
   for ( ; parents ; parents = parents->rest())
      {
      FrSymbol *par = (FrSymbol*)parents->first() ;
      FrFrame *parframe = par->symbolFrame() ;
      if (!parframe || !parframe->wasVisited())
	 {
	 if (queue)
	    {
	    FrList *newtail = new FrList ;
	    qtail->replaca(par) ;
	    qtail->replacd(newtail) ;
	    qtail = newtail ;
	    }
	 else
	    qtail = queue = new FrList(par) ;
	 }
      }
   return ;
}

//----------------------------------------------------------------------

static void clear_visitlist(FrList *visitlist)
{
   while (visitlist)
      {
      FrFrame *fr = (FrFrame*)visitlist->first() ;
      FrCons *tmp = visitlist ;
      visitlist = visitlist->rest() ;
      delete tmp ;
      fr->clearVisited() ;
      }
   return ;
}

//----------------------------------------------------------------------

static const FrList *inheritFillersBFS(FrList *queue, FrFrame *frame,
					const FrSymbol *slot,
					const FrSymbol *facet)
{
   FrList *qtail = queue->last() ;
   FrList *visitlist = 0 ;
   const FrList *fillers = 0 ;
   while (queue)			   // until we run out of ancestors,...
      {
      FrSymbol *psym = (FrSymbol*)queue->first() ;
      FrCons *tmpq = (FrCons*)queue ;	   // perform an inline version of
      queue = queue->rest() ;		   // poplist() for improved speed
      delete tmpq ;			   // of execution
      if (!psym || !psym->symbolp() ||
	  (frame = find_vframe_inline(psym)) == 0 ||
	  frame->wasVisited())
	 continue ;  // this wasn't a proper link!  or we were there already...
      // check whether parent frame has fillers in the desired slot
      fillers = frame->getImmedFillers(slot,facet) ;
      if (fillers)			// does the current frame have fillers?
	 {				// if yes,
	 if (queue)
	    queue->freeObject() ;	// deallocate remainder of queue
	 break ;			// and return the fillers we found
	 }
      else				// if not, continue BFS
	 {
	 frame->markVisited() ;
	 visitlist = (FrList*)new FrCons(frame,visitlist) ;
	 // insert any of the parents of the current frame which are not
	 // already in the queue
	 insert_in_queue(((IFrame*)frame)->predef_slots[ISA_slot].value_facet,
			 queue,qtail) ;
	 }
      }
   // clean up before returning
   clear_visitlist(visitlist) ;
   return fillers ;
}


//----------------------------------------------------------------------
//   Full breadth-first search of the INSTANCE-OF/IS-A links

const FrList *inheritFillersBFS(FrFrame *frame,
				 const FrSymbol *slot,
				 const FrSymbol *facet)
{
   FrList *parents, *queue ;
   parents = ((IFrame*)frame)->predef_slots[INSTANCEOF_slot].value_facet ; ;
   if (!parents)
      parents = ((IFrame*)frame)->predef_slots[ISA_slot].value_facet ;
   // start by looking at parents
   queue = parents ? (FrList*)parents->copy() : 0 ;
   return inheritFillersBFS(queue, frame, slot, facet) ;
}

//----------------------------------------------------------------------
//   Full breadth-first search of the PART-OF link

const FrList *inheritFillersPartOfBFS(FrFrame *frame,
				       const FrSymbol *slot,
				       const FrSymbol *facet)
{
   FrList *parents, *queue, *qtail ;
   parents = ((IFrame*)frame)->predef_slots[PARTOF_slot].value_facet ;
   // start by looking at parents
   queue = parents ? (FrList*)parents->copy() : 0 ;
   qtail = queue->last() ;
   FrList *visitlist = 0 ;
   const FrList *fillers = 0 ;
   while (queue)			   // until we run out of ancestors,...
      {
      FrSymbol *psym = (FrSymbol*)queue->first() ;
      FrCons *tmpq = (FrCons*)queue ;	   // perform an inline version of
      queue = queue->rest() ;		   // poplist() for improved speed
      delete tmpq ;			   // of execution
      if (!psym || !psym->symbolp() ||
	  (frame = find_vframe_inline(psym)) == 0 ||
	  frame->wasVisited())
	 continue ;  // this wasn't a proper link!  or we were there already...
      // check whether parent frame has fillers in the desired slot
      fillers = frame->getImmedFillers(slot,facet) ;
      if (fillers)			// does the current frame have fillers?
	 {				// if yes,
	 if (queue)
	    queue->freeObject() ;	// deallocate remainder of queue
	 break ;			// and return the fillers we found
	 }
      else				// if not, continue BFS
	 {
	 frame->markVisited() ;
	 visitlist = (FrList*)new FrCons(frame,visitlist) ;
	 // insert any of the parents of the current frame which are not
	 // already in the queue
	 parents = ((IFrame*)frame)->predef_slots[PARTOF_slot].value_facet ;
	 insert_in_queue(parents,queue,qtail) ;
	 }
      }
   // clean up before returning
   clear_visitlist(visitlist) ;
   return fillers ;
}

//----------------------------------------------------------------------
//   Depth-first search of the local INHERITS link, falling back to
//   regular depth-first search if no filler found that way

static const FrList *inheritFillersLocalDFS2(FrFrame *frame,
					      const FrSymbol *slot,
					      const FrSymbol *facet)
{
   frame->markVisited() ;
   const FrList *siblings = frame->getImmedFillers(slot,symbolINHERITS) ;
   for ( ; siblings ; siblings = siblings->rest())
      {
      FrFrame *sib = find_vframe_inline((FrSymbol *)siblings->first()) ;
      if (sib && !sib->wasVisited())
	 {
	 const FrList *fillers = sib->getImmedFillers(slot,facet) ;
	 if (fillers ||
	     (fillers = inheritFillersLocalDFS2((IFrame *)sib,slot,facet)) != 0)
	    {
	    frame->clearVisited() ;
	    return fillers ;
	    }
	 }
      }
   // no filler found from siblings
   frame->clearVisited() ;
   return 0 ;
}

//----------------------------------------------------------------------

const FrList *inheritFillersLocalDFS(FrFrame *frame,
				      const FrSymbol *slot,
				      const FrSymbol *facet)
{
   const FrList *siblings = frame->getImmedFillers(slot,symbolINHERITS) ;
   while (siblings)
      {
      FrFrame *sib = find_vframe_inline((FrSymbol *)siblings->first()) ;
      if (sib)
	 {
	 const FrList *fillers = sib->getImmedFillers(slot,facet) ;
	 if (fillers ||
	     (fillers = inheritFillersLocalDFS2((IFrame *)sib,slot,facet)) != 0)
	    return fillers ;
	 }
      siblings = siblings->rest() ;
      }
   // no filler found from siblings, so switch to full DFS
   return inheritFillersDFS(frame,slot,facet) ;
}

//----------------------------------------------------------------------

const FrList *inheritFillersLocalBFS(FrFrame *frame,
				      const FrSymbol *slot,
				      const FrSymbol *facet)
{
   FrList *queue = (FrList*)frame->getImmedFillers(slot,symbolINHERITS) ;
   if (queue) queue = (FrList*)queue->copy() ;
   queue = queue->nconc(new FrList(frame->frameName())) ;
   return inheritFillersBFS(queue, frame, slot, facet) ;
}

//----------------------------------------------------------------------

const FrList *inheritFillersUser(FrFrame *frame,
				  const FrSymbol *slot,
				  const FrSymbol *facet)
{
   if (user_inherit_func)
      return user_inherit_func(frame->frameName(),slot,facet) ;
   else
      return 0 ;
}

//----------------------------------------------------------------------

const FrList *FrFrame::getImmedFillers(const FrSymbol *slotname,
					 const FrSymbol *facet) const
{
   // scan the frame looking for the desired slot
   for (register const FrSlot *slot = predef_slots ; slot ; slot = slot->next)
      if (slot->name == slotname)
	 {
	 CALL_DEMON(if_retrieved,name,slotname,facet,0) ;
	 return *find_facet(slot,facet) ;
	 }
   // if we get here, the slot is not present
   return 0 ;
}

//----------------------------------------------------------------------

const FrList *FrFrame::getImmedValues(const FrSymbol *slotname) const
{
   // scan the frame looking for the desired slot
   for (register const FrSlot *slot = predef_slots ; slot ; slot = slot->next)
      if (slot->name == slotname)
	 {
	 CALL_DEMON(if_retrieved,name,slotname,facet,0) ;
	 return slot->value_facet ;
	 }
   // if we get here, the slot is not present
   return 0 ;
}

//----------------------------------------------------------------------

const FrList * FrFrame::getFillers(const FrSymbol *slotname,
				     const FrSymbol *facet,
				     bool inherit) const
{
   register const FrSlot *slot ;

   // scan the frame looking for the desired slot
   for (slot = predef_slots ; slot ; slot = slot->next)
      if (slot->name == slotname)
	 {
	 FrList *fillers = *find_facet(slot,facet) ;
	 if (fillers)
	    {
	    CALL_DEMON(if_retrieved,name,slotname,facet,0) ;
	    return fillers ;
	    }
	 break ;  // "no fillers" is treated the same as "no slot by that name"
	 }
#ifdef FrDEMONS
   if (slotname->demons() && slotname->demons()->if_missing)
      {
      FramepaC_apply_demons(slotname->demons()->if_missing,name,slotname,
			    facet,0) ;
      // scan the frame looking for the desired slot
      for (slot = predef_slots ; slot ; slot = slot->next)
	 if (slot->name == slotname)
	    {
	    FrList *fillers = *find_facet(slot,facet) ;
	    if (fillers)
	       {
	       CALL_DEMON(if_retrieved,name,slotname,facet,0) ;
	       return fillers ;
	       }
	    break ;  // "no fillers" is treated the same as "no slot by that name"
	    }
      }
#endif /* FrDEMONS */
   // if we get here, the slot is not present, so try to inherit the fillers
   return inherit
	    ? FrSymbolTable::current()->inheritance_function((FrFrame*)this,
						      slotname,facet)
	    : 0 ;
}

//----------------------------------------------------------------------

FrObject *FrFrame::popFiller(const FrSymbol *slotname, const FrSymbol *facet)
{
   register const FrSlot *slot ;

   // scan the frame looking for the desired slot
   for (slot = predef_slots ; slot ; slot = slot->next)
      if (slot->name == slotname)
	 {
	 FrList **fillers = find_facet(slot,facet) ;
	 if (*fillers)
	    {
	    CALL_DEMON(if_retrieved,name,slotname,facet,(*fillers)->first()) ;
	    return poplist(*fillers) ;
	    }
	 break ;  // "no fillers" is treated the same as "no slot by that name"
	 }
#ifdef FrDEMONS
   if (slotname->demons() && slotname->demons()->if_missing)
      {
      FramepaC_apply_demons(slotname->demons()->if_missing,name,slotname,
			    facet,0) ;
      // scan the frame looking for the desired slot
      for (slot = predef_slots ; slot ; slot = slot->next)
	 if (slot->name == slotname)
	    {
	    FrList **fillers = find_facet(slot,facet) ;
	    if (*fillers)
	       {
	       CALL_DEMON(if_retrieved,name,slotname,facet,
			  (*fillers)->first()) ;
	       return poplist(*fillers) ;
	       }
	    break ;  // "no fillers" is treated the same as "no slot by that name"
	    }
      }
#endif /* FrDEMONS */
   // if we get here, the slot is not present in the frame proper
   return 0 ;
}

//----------------------------------------------------------------------

FrObject *FrFrame::firstFiller(const FrSymbol *slotname,
				 const FrSymbol *facet, bool inherit) const
{
   register const FrSlot *slot ;
   const FrList *fillers ;

   // scan the frame looking for the desired slot
   for (slot = predef_slots ; slot ; slot = slot->next)
      if (slot->name == slotname)
	 {
	 fillers = *find_facet(slot,facet) ;
	 if (fillers)
	    {
	    CALL_DEMON(if_retrieved,name,slotname,facet,0) ;
	    return fillers->first() ;
	    }
	 break ;  // "no fillers" is treated the same as "no slot by that name"
	 }
#ifdef FrDEMONS
   if (slotname->demons() && slotname->demons()->if_missing)
      {
      FramepaC_apply_demons(slotname->demons()->if_missing,name,slotname,
			    facet,0) ;
      // scan the frame looking for the desired slot
      for (slot = predef_slots ; slot ; slot = slot->next)
	 if (slot->name == slotname)
	    {
	    fillers = *find_facet(slot,facet) ;
	    if (fillers)
	       {
	       CALL_DEMON(if_retrieved,name,slotname,facet,0) ;
	       return fillers->first() ;
	       }
	    break ;  // "no fillers" is treated the same as "no slot by that name"
	    }
      }
#endif /* FrDEMONS */
   // if we get here, the slot is not present, so try to inherit the fillers
   if (inherit)
      {
      fillers = FrSymbolTable::current()->inheritance_function((FrFrame*)this,
							slotname,facet) ;
      return fillers ? fillers->first() : 0 ;
      }
   else
      return 0 ;
}

//----------------------------------------------------------------------

const FrList *FrFrame::getValues(const FrSymbol *slotname,
				   bool inherit) const
{
   register const FrSlot *slot ;

   // scan the frame looking for the desired slot
   for (slot = predef_slots ; slot ; slot = slot->next)
      if (slot->name == slotname)
	 {
	 FrList *fillers = slot->value_facet ;
	 if (fillers)
	    {
	    CALL_DEMON(if_retrieved,name,slotname,symbolVALUE,0) ;
	    return fillers ;
	    }
	 break ;  // "no fillers" is treated the same as "no slot by that name"
	 }
#ifdef FrDEMONS
   if (slotname->demons() && slotname->demons()->if_missing)
      {
      FramepaC_apply_demons(slotname->demons()->if_missing,name,slotname,
			    symbolVALUE,0) ;
      // scan the frame looking for the desired slot
      for (slot = predef_slots ; slot ; slot = slot->next)
	 if (slot->name == slotname)
	    {
	    FrList *fillers = slot->value_facet ;
	    if (fillers)
	       {
	       CALL_DEMON(if_retrieved,name,slotname,symbolVALUE,0) ;
	       return fillers ;
	       }
	    break ;  // "no fillers" is treated the same as "no slot by that name"
	    }
      }
#endif /* FrDEMONS */
   // if we get here, the slot is not present, so try to inherit the fillers
   return inherit
	     ? FrSymbolTable::current()->inheritance_function((FrFrame*)this,
						       slotname,symbolVALUE)
	     : 0 ;
}

//----------------------------------------------------------------------

const FrList *FrFrame::getSem(const FrSymbol *slotname,bool inherit) const
{
   register const FrSlot *slot ;

   // scan the frame looking for the desired slot
   for (slot = predef_slots ; slot ; slot = slot->next)
      if (slot->name == slotname)
	 {
	 FrList *fillers = *find_sem_facet(slot) ;
	 if (fillers)
	    {
	    CALL_DEMON(if_retrieved,name,slotname,symbolSEM,0) ;
	    return fillers ;
	    }
	 break ;  // "no fillers" is treated the same as "no slot by that name"
	 }
#ifdef FrDEMONS
   if (slotname->demons() && slotname->demons()->if_missing)
      {
      FramepaC_apply_demons(slotname->demons()->if_missing,name,slotname,
			    symbolSEM,0) ;
      // scan the frame looking for the desired slot
      for (slot = predef_slots ; slot ; slot = slot->next)
	 if (slot->name == slotname)
	    {
	    FrList *fillers = *find_sem_facet(slot) ;
	    if (fillers)
	       {
	       CALL_DEMON(if_retrieved,name,slotname,symbolSEM,0) ;
	       return fillers ;
	       }
	    break ;  // "no fillers" is treated the same as "no slot by that name"
	    }
      }
#endif /* FrDEMONS */
   // if we get here, the slot is not present, so try to inherit the fillers
   return inherit
	    ? FrSymbolTable::current()->inheritance_function((FrFrame*)this,
						      slotname,symbolSEM)
	    : 0 ;
}

//----------------------------------------------------------------------

FrObject *FrFrame::getValue(const FrSymbol *slotname, bool inherit) const
{
   register const FrSlot *slot ;
   const FrList *fillers ;

   // scan the frame looking for the desired slot
   for (slot = predef_slots ; slot ; slot = slot->next)
      if (slot->name == slotname)
	 {
	 fillers = slot->value_facet ;
	 if (fillers)
	    {
	    CALL_DEMON(if_retrieved,name,slotname,symbolVALUE,0) ;
	    return fillers->first() ;
	    }
	 break ;  // "no fillers" is treated the same as "no slot by that name"
	 }
#ifdef FrDEMONS
   if (slotname->demons() && slotname->demons()->if_missing)
      {
      FramepaC_apply_demons(slotname->demons()->if_missing,name,slotname,
			    symbolVALUE,0) ;
      // scan the frame looking for the desired slot
      for (slot = predef_slots ; slot ; slot = slot->next)
	 if (slot->name == slotname)
	    {
	    fillers = slot->value_facet ;
	    if (fillers)
	       {
	       CALL_DEMON(if_retrieved,name,slotname,symbolVALUE,0) ;
	       return fillers->first() ;
	       }
	    break ;  // "no fillers" is treated the same as "no slot by that name"
	    }
      }
#endif /* FrDEMONS */
   // if we get here, the slot is not present, so try to inherit the fillers
   if (inherit)
      {
      fillers = FrSymbolTable::current()->inheritance_function((FrFrame*)this,
							slotname,symbolVALUE) ;
      return fillers ? fillers->first() : 0 ;
      }
   else
      return 0 ;
}

//----------------------------------------------------------------------

static FrList *collect_slots_BFS(const FrFrame *frame, FrList *allslots,
				  bool include_name)
{
   FrList *visitlist = 0 ;
   const FrList *parents = frame->instanceOf_list() ;
   if (!parents)
      parents = frame->isA_list() ;
   // start by looking at parents
   FrList *queue = parents ? (FrList*)parents->copy() : 0 ;
   FrList *qtail = queue->last() ;
   while (queue)			   // until we run out of ancestors,...
      {
      FrSymbol *psym = (FrSymbol*)queue->first() ;
      FrCons *tmpq = (FrCons*)queue ;	   // perform an inline version of
      queue = queue->rest() ;		   // poplist() for improved speed
      delete tmpq ;			   // of execution
      if (!psym || !psym->symbolp() ||
	  (frame = find_vframe_inline(psym)) == 0 || frame->wasVisited())
	 continue ;  // this wasn't a proper link!  or we were there already...
      allslots = frame->collectSlots(NoInherit,allslots,include_name) ;
      // check whether parent frame has fillers in the desired slot
      ((FrFrame*)frame)->markVisited() ;
      visitlist = (FrList*)new FrCons(frame,visitlist) ;
      // insert any of the parents of the current frame which are not
      // already in the queue
      parents = frame->isA_list() ;
      insert_in_queue(parents,queue,qtail) ;
      }
   // clean up before returning
   clear_visitlist(visitlist) ;
   return allslots ;
}

//----------------------------------------------------------------------

static FrList *collect_slots_partBFS(const FrFrame *frame,FrList *allslots,
				      bool include_name)
{
   FrList *visitlist = 0 ;
   const FrList *parents = frame->partOf_list() ;
   // start by looking at parents
   FrList *queue = parents ? (FrList*)parents->copy() : 0 ;
   FrList *qtail = queue->last() ;
   while (queue)			   // until we run out of ancestors,...
      {
      FrSymbol *psym = (FrSymbol*)queue->first() ;
      FrCons *tmpq = (FrCons*)queue ;	   // perform an inline version of
      queue = queue->rest() ;		   // poplist() for improved speed
      delete tmpq ;			   // of execution
      if (!psym || !psym->symbolp() ||
	  (frame = find_vframe_inline(psym)) == 0 || frame->wasVisited())
	 continue ;  // this wasn't a proper link!  or we were there already...
      allslots = frame->collectSlots(NoInherit,allslots,include_name) ;
      // check whether parent frame has fillers in the desired slot
      ((FrFrame*)frame)->markVisited() ;
      visitlist = (FrList*)new FrCons(frame,visitlist) ;
      // insert any of the parents of the current frame which are not
      // already in the queue
      parents = frame->partOf_list() ;
      insert_in_queue(parents,queue,qtail) ;
      }
   // clean up before returning
   clear_visitlist(visitlist) ;
   return allslots ;
}

//----------------------------------------------------------------------

static FrList *collect_slots_localBFS(const FrFrame *frame,FrList *allslots,
				       bool include_name)
{
   const FrList *parents ;
   FrList *queue = 0, *qtail ;
   FrList *visitlist = 0 ;
   FrList **q_end = &queue ;
   for (FrList *all = allslots ; all ; all = all->rest())
      {
      FrSymbol *slot = (FrSymbol*)((FrList*)all->first())->first() ;
      const FrList *q = frame->getImmedFillers(slot,symbolINHERITS) ;
      for ( ; q ; q = q->rest())
	 {
	 queue->pushlistend(q->first()->copy(), q_end) ;
	 }
      }
   *q_end = new FrList(frame->frameName()) ; // properly terminate the queue
   qtail = queue->last() ;		// start by looking at frame's siblings
   while (queue)			// then their and its own ancestors
      {
      FrSymbol *psym = (FrSymbol*)queue->first() ;
      FrCons *tmpq = (FrCons*)queue ;	   // perform an inline version of
      queue = queue->rest() ;		   // poplist() for improved speed
      delete tmpq ;			   // of execution
      if (!psym || !psym->symbolp() ||
	  (frame = find_vframe_inline(psym)) == 0 || frame->wasVisited())
	 continue ;  // this wasn't a proper link!  or we were there already...
      allslots = frame->collectSlots(NoInherit,allslots,include_name) ;
      // check whether parent frame has fillers in the desired slot
      ((FrFrame*)frame)->markVisited() ;
      visitlist = (FrList*)new FrCons(frame,visitlist) ;
      // insert any of the parents of the current frame which are not
      // already in the queue
      parents = frame->isA_list() ;
      insert_in_queue(parents,queue,qtail) ;
      }
   // clean up before returning
   clear_visitlist(visitlist) ;
   return allslots ;
}

//----------------------------------------------------------------------

static FrList *collect_slots_user(FrSymbol *frame, FrList *allslots,
				   bool include_name)
{
   (void)frame ; (void)include_name ;
   FrUndefined("FrFrame::collectSlots(InheritUser)") ;
   return allslots ;
}

//----------------------------------------------------------------------
// work up the inheritance hierarchy for the frame, collecting a list of
// all slots (and their facets) that may be inherited into the frame.
// Return an assoc list of slot names and associated facets.

FrList *FrFrame::collectSlots(FrInheritanceType inherit,FrList *allslots,
				bool include_name) const
{
   register const FrSlot *slot ;
   FrSymbol *parent ;
   FrFrame *parentfr ;
   FrList *parents ;

#ifdef __GNUC__
   parentfr = 0 ; // keep compiler happy.....
#endif /* __GNUC__ */
   for (slot = predef_slots ; slot ; slot = slot->next)
      {
      FrList *a = (FrList *)allslots->assoc(slot->name) ;
      FrList *facets, *tail ;

      if (!a)
	 {
	 if (slot->value_facet)
	    pushlist((include_name ? (FrObject*)new FrList(symbolVALUE,name)
				   : (FrObject*)symbolVALUE),
		     a) ;
	 pushlist(pushlist(slot->name,a),allslots) ;
	 }
      else if (include_name)
	 {
	 if (slot->value_facet && !(FrList*)a->assoc(symbolVALUE))
	    a->nconc(new FrList(new FrList(symbolVALUE,name))) ;
	 }
      else
	 {
	 if (slot->value_facet && !a->member(symbolVALUE))
	    a->nconc(new FrList(symbolVALUE)) ;
	 }
      tail = a->last() ;
      facets = a->rest() ;
      for (FrFacet *facet = slot->other_facets ; facet ; facet = facet->next)
	 {
	 if (!facet->fillers)
	    continue ;
	 if (include_name)
	    {
	    if (!facets->assoc(facet->facet_name))
	       {
	       tail->replacd(new FrList(new FrList(facet->facet_name,name)));
	       tail = tail->rest() ;
	       }
	    }
	 else
	    {
	    if (!facets->member(facet->facet_name))
	       {
	       tail->replacd(new FrList(facet->facet_name)) ;
	       tail = tail->rest() ;
	       }
	    }
	 }
      }
   switch (inherit)
      {
      case NoInherit:
	 // do nothing
	 break ;
      case InheritSimple:
	 if ((parent = isA()) != 0)
	    {
	    parentfr = find_vframe_inline(parent) ;
	    if (parentfr && !parentfr->wasVisited())
	       {
	       parentfr->markVisited() ;
	       FrList *result = parentfr->collectSlots(inherit,allslots,
							include_name) ;
	       parentfr->clearVisited() ;
	       return result ;
	       }
	    }
	 break ;
      case InheritLocalDFS:
	 for (slot = predef_slots ; slot ; slot = slot->next)
	    {
	    const FrList *siblings = getImmedFillers(slot->name,
						      symbolINHERITS) ;
	    while (siblings)
	       {
	       FrFrame *sib = find_vframe_inline((FrSymbol *)siblings->first()) ;
	       if (sib && !sib->wasVisited())
		  {
		  sib->markVisited() ;
		  allslots = sib->collectSlots(InheritDFS,allslots,
					       include_name) ;
		  sib->clearVisited() ;
		  break ;
		  }
	       siblings = siblings->rest() ;
	       }
	    }
	 // in addition to inheriting from siblings, we also get everything
	 // available through full DFS, so fall through to that case
      case InheritDFS:
	 parents = predef_slots[ISA_slot].value_facet ;
	 for ( ; parents ; parents = parents->rest())
	    {
	    if ((parent = (FrSymbol *)parents->first()) != 0 &&
		(parentfr = find_vframe_inline(parent)) != 0 &&
		!parentfr->wasVisited())
	       {
	       parentfr->markVisited() ;
	       allslots = parentfr->collectSlots(InheritDFS,allslots,
						 include_name) ;
	       parentfr->clearVisited() ;
	       }
	    }
	 break ;
      case InheritPartDFS:
	 parents = predef_slots[PARTOF_slot].value_facet ;
	 while (parents)
	    {
	    if ((parent = (FrSymbol *)parents->first()) != 0)
	       {
	       parentfr = find_vframe_inline(parent) ;
	       if (parentfr && !parentfr->wasVisited())
		  {
		  parentfr->markVisited() ;
		  allslots = parentfr->collectSlots(inherit,allslots,
						    include_name) ;
		  parentfr->clearVisited() ;
		  }
	       }
	    parents = parents->rest() ;
	    }
	 break ;
      case InheritBFS:
	 allslots = collect_slots_BFS(this,allslots,include_name) ;
	 break ;
      case InheritPartBFS:
	 allslots = collect_slots_partBFS(this,allslots,include_name) ;
	 break ;
      case InheritLocalBFS:
	 allslots = collect_slots_localBFS(this,allslots,include_name) ;
	 break ;
      case InheritUser:
	 allslots = collect_slots_user(name,allslots,include_name) ;
	 break ;
      default:
	 FrProgError("bad inheritance type in FrFrame::collectSlots") ;
	 break ;
      }
   return allslots ;
}

//----------------------------------------------------------------------

void FrFrame::inheritAll()
{
   FrList *allslots = collectSlots(get_inheritance_type()) ;

   for (FrList *slot = allslots ; slot ; slot = slot->rest())
      {
      FrList *sl = (FrList *)slot->first() ;
      FrSymbol *slotname = (FrSymbol *)sl->first() ;

      for (FrList *facets = (FrList *)sl->rest() ; facets ;
	   facets = facets->rest())
	 {
	 FrSymbol *facetname = (FrSymbol *)facets->first() ;
	 if (!getImmedFillers(slotname,facetname))
	    addFillers(slotname,facetname,getFillers(slotname,facetname)) ;
	 }
      }
}

//----------------------------------------------------------------------

void FrFrame::inheritAll(FrInheritanceType inherit)
{
   FrInheritanceType oldinherit = get_inheritance_type() ;

   set_inheritance_type(inherit) ;
   inheritAll() ;
   set_inheritance_type(oldinherit) ;
}

//----------------------------------------------------------------------

FrList *FrFrame::allSlots() const
{
   FrList *slotlist = 0 ;
   FrList **end = &slotlist ;

   for (const FrSlot *slot = predef_slots ; slot ; slot = slot->next)
      slotlist->pushlistend(slot->name,end) ;	
   *end = 0 ;				// properly terminate the result list
   return slotlist ;
}

//----------------------------------------------------------------------

FrList *FrFrame::slotFacets(const FrSymbol *slotname) const
{
   for (const FrSlot *slot = predef_slots ; slot ; slot = slot->next)
      {
      if (slot->name == slotname)
	 {
	 FrList *facetlist = 0	 ;
	 FrList **end = &facetlist ;
	 for (const FrFacet *facets = slot->other_facets ; facets ;
	      facets = facets->next)
	    facetlist->pushlistend(facets->facet_name,end) ;
	 if (slot->value_facet)
	    facetlist->pushlistend(symbolVALUE,end) ;
	 *end = 0 ;			// properly terminate the result
	 return facetlist ;
	 }
      }
   return 0 ;
}

/**********************************************************************/
/*    procedural interface to class FrFrame			      */
/**********************************************************************/

FrFrame *create_frame(const char *frname)
{
   FrSymbol *sym = FrSymbolTable::add(frname) ;
   FrFrame *fr = find_vframe_inline(sym) ;

   return fr ? fr : new FrFrame(frname) ;
}

//----------------------------------------------------------------------

FrFrame *create_frame(const FrSymbol *frname)
{
   FrFrame *fr = find_vframe_inline(frname) ;

   return fr ? fr : new FrFrame((FrSymbol *)frname) ;
}

//----------------------------------------------------------------------

FrInheritanceType get_inheritance_type()
{
   return FrSymbolTable::current()->inheritance_type ;
}

//----------------------------------------------------------------------

void set_user_inheritance(FrInheritanceFunc *ifunc,
			  FrInheritanceLocFunc *lfunc)
{
   user_inherit_func = ifunc ;
   user_inherit_loc_func = lfunc ;
}

//----------------------------------------------------------------------

void get_user_inheritance(FrInheritanceFunc **ifunc,
			  FrInheritanceLocFunc **lfunc)
{
   if (ifunc)
      *ifunc = user_inherit_func ;
   if (lfunc)
      *lfunc = user_inherit_loc_func ;
}

//----------------------------------------------------------------------

FrInheritanceType FramepaC_set_inheritance_type(FrInheritanceType inherit)
{
   FrInheritanceType old_inherit = FramepaC_inheritance_type ;
   FrSymbolTable *active = FrSymbolTable::current() ;
   switch (inherit)
      {
      case NoInherit:
	 FramepaC_inheritance_function =
	 active->inheritance_function = inheritFillersNone;
	 break ;
      case InheritSimple:
	 FramepaC_inheritance_function =
	 active->inheritance_function = inheritFillersSimple ;
	 break ;
      case InheritDFS:
	 FramepaC_inheritance_function =
	 active->inheritance_function = inheritFillersDFS ;
	 break ;
      case InheritBFS:
	 FramepaC_inheritance_function =
	 active->inheritance_function = inheritFillersBFS ;
	 break ;
      case InheritPartDFS:
	 FramepaC_inheritance_function =
	 active->inheritance_function = inheritFillersPartOfDFS ;
	 break ;
      case InheritPartBFS:
	 FramepaC_inheritance_function =
	 active->inheritance_function = inheritFillersPartOfBFS ;
	 break ;
      case InheritLocalDFS:
	 FramepaC_inheritance_function =
	 active->inheritance_function = inheritFillersLocalDFS ;
	 break ;
      case InheritLocalBFS:
	 FramepaC_inheritance_function =
	 active->inheritance_function = inheritFillersLocalBFS ;
	 break ;
      case InheritUser:
	 FramepaC_inheritance_function =
	 active->inheritance_function = inheritFillersUser ;
	 break ;
      default:
	 FrProgError("bad value specified for inheritance type.") ;
      }
   FramepaC_inheritance_type = active->inheritance_type = inherit ;
   return old_inherit ;
}

//----------------------------------------------------------------------

FrInheritanceType set_inheritance_type(const FrSymbol *inherit)
{
   FrInheritanceType type ;
   if (!inherit)
      return FramepaC_inheritance_type ;
   if (inherit == findSymbol("NONE"))
      type = NoInherit ;
   else if (inherit == findSymbol("SIMPLE"))
      type = InheritSimple ;
   else if (inherit == findSymbol("DFS") || inherit == findSymbol("ISA-DFS"))
      type = InheritDFS ;
   else if (inherit == findSymbol("BFS") || inherit == findSymbol("ISA-BFS"))
      type = InheritBFS ;
   else if (inherit == findSymbol("PART-DFS"))
      type = InheritPartDFS ;
   else if (inherit == findSymbol("PART-BFS"))
      type = InheritPartBFS ;
   else if (inherit == findSymbol("LOCAL-DFS"))
      type = InheritLocalDFS ;
   else if (inherit == findSymbol("LOCAL-BFS"))
      type = InheritLocalBFS ;
   else
      type = InheritDFS ;
   return FramepaC_set_inheritance_type(type) ;
}

//----------------------------------------------------------------------

void FramepaC_init_inheritance_setting_func()
{
   FramepaC_set_inhtype_func = FramepaC_set_inheritance_type ;
   FramepaC_set_inheritance_type(InheritSimple) ;
   return ;
}

// end of file frframe.cpp //


