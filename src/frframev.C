/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01  							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frframev.cpp		class FrFrame (virtual functions)	*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,1998,2002,2004,2009,2013,2015	*/
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

#include "frframe.h"
#include "frpcglbl.h"

#ifdef FrLRU_DISCARD
#include "frlru.h"
#endif

#ifdef FrSTRICT_CPLUSPLUS
#  include <iomanip>
#else
#  include <iomanip.h>
#endif /* FrSTRICT_CPLUSPLUS */

/**********************************************************************/
/*    Global data shared with other modules		      	      */
/**********************************************************************/

FrInheritanceType (*FramepaC_set_inhtype_func)(FrInheritanceType) = 0 ;

/**********************************************************************/
/*    Global data local to this module			      	      */
/**********************************************************************/

bool omit_inverse_links = false ;
const FrFrame *FramepaC_lock_Frame_1 = 0 ;

// DON'T CHANGE THE ORDER OF THE FOLLOWING!  Other code uses the absolute
// position of these slots in the frame.  (any changes also require appropriate
// changes to frpcglbl.h, and possibly elsewhere)
FrSymbol **predefined_slot_names[] =
   { &symbolISA, &symbolSUBCLASSES, &symbolINSTANCEOF, &symbolPARTOF } ;

static const char stringVALUE[] = "VALUE" ;

/************************************************************************/
/*    Global variables for class FrFrame				*/
/************************************************************************/

FrAllocator FrFacet::allocator("FrFacet",sizeof(FrFacet)) ;

// a dummy global to force proper inheritance initialization
FrInheritanceType dummy_inherit_type = set_inheritance_type(InheritSimple) ;

/**********************************************************************/
/*	Forward declarations					      */
/**********************************************************************/

void delete_all_frames() ;

/**********************************************************************/
/*	Utility functions					      */
/**********************************************************************/

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

/**********************************************************************/
/*	Member functions for class FrFrame			      */
/**********************************************************************/

void FrFrame::build_empty_frame(FrSymbol *frname)
{
   unsigned int i ;

   name = frname ;
   if (frname)
      frname->setFrame(this) ;
   // build the predefined slots
#define last (lengthof(predef_slots)-1)
   for (i = 0 ; i < last ; i++)
      {
      predef_slots[i].name = *(predefined_slot_names[i]) ;
      predef_slots[i].value_facet = 0 ;
      predef_slots[i].other_facets = 0 ;
      predef_slots[i].next = &predef_slots[i+1] ;
      CALL_DEMON(if_created,name,predefined_slot_names[i],symbolVALUE,0) ;
      }
   predef_slots[last].name = *(predefined_slot_names[i]) ;
   predef_slots[last].value_facet = 0 ;
   predef_slots[last].other_facets = 0 ;
   predef_slots[last].next = 0 ;
   CALL_DEMON(if_created,name,predefined_slot_names[last],symbolVALUE,0) ;
   last_slot = &predef_slots[last] ;
#undef last
   virtual_frame =		     // by default, assume not a virtual frame
   locked =
   dirty = (char)false ;             // newly-created, so hasn't changed
   visited = false ;
#ifdef FrLRU_DISCARD
   LRUclock = FrSymbolTable::current()->LRUclock++ ;
   if (LRUclock > LRUclock_LIMIT)
      FrSymbolTable::current()->adjust_LRUclock() ;
#endif /* FrLRU_DISCARD */
   FramepaC_delete_all_frames = delete_all_frames ;
   return ;
}

//----------------------------------------------------------------------

FrFrame::FrFrame()
{
   build_empty_frame(0) ;
}

//----------------------------------------------------------------------

FrFrame::FrFrame(const char *framename)
{
   build_empty_frame(FrSymbolTable::add(framename)) ;
}

//----------------------------------------------------------------------

FrFrame::FrFrame(FrSymbol *framename)
{
   build_empty_frame(framename) ;
}

//----------------------------------------------------------------------

FrFrame::~FrFrame()
{
   // erase and then free the frame's slots
   eraseFrame() ;
   // unlink the frame from its name
   name->setFrame(0) ;
}

//----------------------------------------------------------------------

FrObjectType FrFrame::objType() const
{
   return OT_Frame ;
}

//----------------------------------------------------------------------

const char *FrFrame::objTypeName() const
{
   return "FrFrame" ;
}

//----------------------------------------------------------------------

FrObjectType FrFrame::objSuperclass() const
{
   return OT_FrObject ;
}

//----------------------------------------------------------------------

bool FrFrame::framep() const
{
   return true ;
}

//----------------------------------------------------------------------

// BorlandC++ 3.1 doesn't inline functions containing while loops
#ifdef __BORLANDC__
static
#else
inline
#endif
void printFacet(ostream &output,const char *name,FrList *facet)
{
   size_t orig_indent = FramepaC_initial_indent ;
   FramepaC_initial_indent += 2 ;
   output << '\n' << setw(orig_indent+1) << "[" << name ;
   while (facet)
      {
      output << ' ' << facet->first() ;
      facet = facet->rest() ;
      }
   output << ']' ;
   FramepaC_initial_indent = orig_indent ;
}

//----------------------------------------------------------------------

void FrFrame::printSlot(ostream &output,const FrSlot *slot) const
{
   size_t orig_indent = FramepaC_initial_indent ;
   FramepaC_initial_indent += 4 ;
   output << '\n' << setw(orig_indent+1) << "[" << slot->name->symbolName() ;
   if (slot->value_facet)
      printFacet(output,stringVALUE,slot->value_facet) ;
   for (FrFacet *facet = slot->other_facets ; facet ; facet = facet->next)
      printFacet(output,facet->facet_name->symbolName(),facet->fillers) ;
   output << ']' ;
   FramepaC_initial_indent = orig_indent ;
}

//----------------------------------------------------------------------

ostream &FrFrame::printValue(ostream &output) const
{
   size_t orig_indent = FramepaC_initial_indent ;
   FramepaC_initial_indent += 4 ;
   output << setw(orig_indent+1) << "[" << name ;
   for (register const FrSlot *slot = predef_slots ; slot ; slot = slot->next)
      if (slot->value_facet || slot->other_facets)
         printSlot(output,slot) ;
   output << ']' ;
   FramepaC_initial_indent = orig_indent ;
   return output ;
}

//----------------------------------------------------------------------

static size_t slot_display_length(const FrSlot *slot)
{
   int len = 2 + slot->name->displayLength() ;

   if (slot->value_facet)
      len += 6 + slot->value_facet->displayLength() ;
   for (FrFacet *facet = slot->other_facets ; facet ; facet = facet->next)
      {
      len += 1 + facet->facet_name->displayLength() ;
      if (facet->fillers)
         len += facet->fillers->displayLength() ;
      else
         len++ ;
      }
   return len ;
}

//----------------------------------------------------------------------

size_t FrFrame::displayLength() const
{
   register const FrSlot *slot ;
   const FrSlot *userslot = predef_slots[lengthof(predef_slots)-1].next ;
   int len = name->displayLength() + 2 ;

   for (slot = predef_slots ; slot != userslot ; slot = slot->next)
     if (slot->value_facet || slot->other_facets)
        len += slot_display_length(slot) ;
   for ( ; slot ; slot = slot->next)
      len += slot_display_length(slot) ;
   return len ;
}

//----------------------------------------------------------------------

static char *facet_display_value(char *buffer, const char *name,
				 FrList *fillers)
{
   *buffer++ = '[' ;
   int len = strlen(name) ;
   memcpy(buffer,name,len) ;
   buffer += len ;
   while (fillers)
      {
      *buffer++ = ' ' ;
      if (fillers->first())
	 buffer = fillers->first()->displayValue(buffer) ;
      else
	 {
	 *buffer++ = '(' ;
	 *buffer++ = ')' ;
         }
      fillers = fillers->rest() ;
      }
   *buffer++ = ']' ;
   *buffer = '\0' ;
   return buffer ;
}

//----------------------------------------------------------------------

static char *slot_display_value(char *buffer, const FrSlot *slot)
{
   *buffer++ = '[' ;
   buffer = slot->name->displayValue(buffer) ;
   if (slot->value_facet)
      buffer = facet_display_value(buffer,stringVALUE,slot->value_facet) ;
   for (FrFacet *facet = slot->other_facets ; facet ; facet = facet->next)
      buffer = facet_display_value(buffer,facet->facet_name->symbolName(),
	 		           facet->fillers) ;
   *buffer++ = ']' ;
   *buffer = '\0' ;
   return buffer ;
}

//----------------------------------------------------------------------

char *FrFrame::displayValue(char *buffer) const
{
   register const FrSlot *slot ;
   const FrSlot *userslot = predef_slots[lengthof(predef_slots)-1].next ;

   *buffer++ = '[' ;
   buffer = name->displayValue(buffer) ;
   for (slot = predef_slots ; slot != userslot ; slot = slot->next)
      if (slot->value_facet || slot->other_facets)
         buffer = slot_display_value(buffer,slot) ;
   for ( ; slot ; slot = slot->next)
     buffer = slot_display_value(buffer,slot) ;
   *buffer++ = ']' ;
   *buffer = '\0' ;
   return buffer ;
}

//----------------------------------------------------------------------
// check whether frame has any fillers

bool FrFrame::emptyFrame() const
{
   for (register const FrSlot *slot = predef_slots ; slot ; slot = slot->next)
      if (slot->value_facet || slot->other_facets)
	 return false ;
   return true ;
}

//----------------------------------------------------------------------

FrSlot *FrFrame::createSlot(const FrSymbol *sym)
{
   register FrSlot *slot ;

   for (slot = predef_slots ; slot ; slot = slot->next)
      if (slot->name == sym)
	 return slot ;  // found a slot by that name
   FramepaC_lock_Frame_1 = this ;
   if ((slot = new FrSlot((FrSymbol *)sym)) == 0)
      FrNoMemory("while creating slot") ;
   FramepaC_lock_Frame_1 = 0 ;
   // insert the new slot
   last_slot->next = slot ;
   last_slot = slot ;
   dirty = true ;             // we've changed the frame
   CALL_DEMON(if_created,name,sym,symbolVALUE,0) ;
   return slot ;
}

//----------------------------------------------------------------------

void FrFrame::createFacet(const FrSymbol *slotname, const FrSymbol *facetname)
{
   FrSlot *slot = createSlot(slotname) ;
   // find or create the requested facet
   if (facetname != symbolVALUE)
      {
      FrFacet *f ;
      FrFacet *prev = 0 ;

      for (f = slot->other_facets ; f ; prev=f, f=f->next)
	 if (f->facet_name == facetname)
	    return ;
      FramepaC_lock_Frame_1 = this ;
      if (prev)
	 prev->next = new FrFacet(facetname) ;
      else
	 slot->other_facets = new FrFacet(facetname) ;
      FramepaC_lock_Frame_1 = 0 ;
      CALL_DEMON(if_created,name,slotname,facetname,0) ;
      dirty = true ;   // we've changed the frame
      }
}

//----------------------------------------------------------------------
// remove a VALUE facet filler without checking for relations

void FrFrame::deleteInverse(const FrSymbol *slotname,
			     FrSymbol *framename)
{
   FrFrame *frame = find_vframe_inline(framename) ;
   if (frame)
      {
      FrList *curr, *prev ;
      if (frame->locked && VFrame_Info)
	 {
	 int result = VFrame_Info->proxyDel(framename,slotname,symbolVALUE,
					    name) ;
	 if (result)
	    Fr_errno = ME_LOCKED ;
	 return ;
	 }
      // if locked but no backing store, remove the inverse anyway, because
      // the calling application holds the lock
      for (register FrSlot *slot=frame->predef_slots ; slot ; slot=slot->next)
	 if (slot->name == slotname)
	    {
	    prev = 0 ;
	    for (curr=slot->value_facet ; curr ; prev=curr,curr=curr->rest())
	       {
	       if (curr->first() == name)
		  {
		  // remove the filler from the current list of fillers
		  if (prev)
		     prev->replacd(curr->rest()) ;
		  else
		     slot->value_facet = curr->rest() ;
		  CALL_DEMON(if_deleted,framename,slotname,symbolVALUE,name) ;
		  curr->replaca(0) ;
		  curr->replacd(0) ;
		  delete (FrCons *)curr ;
		  dirty = true ;
		  return ;      // found filler, so don't check any more
		  }
	       }
	    break ;  // found proper slot, so don't check any others
	    }
       }
}

//----------------------------------------------------------------------
// insert a VALUE facet filler without checking for relations

bool FrFrame::insertInverse(const FrSymbol *slotname,
			     FrSymbol *framename)
{
   FrFrame *frame = find_vframe_inline(framename) ;
   if (!frame) //!!!
      {
      frame = (virtual_frame && FramepaC_new_VFrame)
		? FramepaC_new_VFrame(framename)
	 	: new FrFrame(framename) ;
      if (!frame)
	 {
	 FrNoMemory("while creating frame for inverse relation") ;
	 return false ;
	 }
      }
   // add the current frame to the destination frame's reverse link
   if (frame->locked && VFrame_Info)
      {
      int result = VFrame_Info->proxyAdd(framename,slotname,symbolVALUE,name) ;
      if (result)
	 Fr_errno = ME_LOCKED ;
      return (bool)(result == 0) ;
      }
   FrSlot *slot = frame->createSlot(slotname) ;
   CALL_DEMON(if_added,framename,slotname,symbolVALUE,name) ;
   FrList *newval = new FrList ;
   newval->replacd(slot->value_facet) ;
   newval->replaca(name) ;
   slot->value_facet = newval ;
   frame->dirty = true ;
   return true ;  // successfully added
}

//----------------------------------------------------------------------

void FrFrame::addFillerNoCopy(const FrSymbol *slotname,
			       const FrSymbol *facetname,
			       FrObject *filler)
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
   FrList *tail = *facet ;
   FrList *next ;
   if (tail)
      {
      for (;;)
	 {
	 if (equal_inline(tail->first(),filler)) // is it what we want added?
	    return ;                             // if yes, bail out now
	 next = tail->rest() ;
	 if (!next)                     // if no more fillers, tail is last
	    break ;
	 else
	    tail = next ;               // move to next filler in list
	 }
      // the object to be added wasn't already on the filler list, so bash it
      // onto the end of the filler list
      CALL_DEMON(if_added,name,slotname,facetname,filler) ;
      tail->replacd(new FrList(filler)) ;
      }
   else
      {
      CALL_DEMON(if_added,name,slotname,facetname,filler) ;
      *facet = new FrList(filler) ;
      }
   dirty = true ;             // we've changed the frame
   // now, see whether this is a relation for which we need to maintain an
   // inverse
   if (facetname == symbolVALUE && !omit_inverse_links &&
       slotname->inverseRelation() && filler && filler->symbolp())
      insertInverse(slotname->inverseRelation(),(FrSymbol*)filler) ;
}

//----------------------------------------------------------------------

int FrFrame::commitFrame()
{
   if (virtual_frame)
      {
      if (dirty && VFrame_Info)
	 {
	 if (!VFrame_Info->storeFrame(name))
	    return -1 ;
	 }
      }
   dirty = false ;
   return 0 ;  // successful
}

//----------------------------------------------------------------------

void FrFrame::eraseFrame()
{
   register FrSlot *slot ;
   // erase all the slots in the frame
   for (slot = predef_slots ; slot ; slot = slot->next)
      {
      while (slot->other_facets)
	 {
	 FrFacet *facet = slot->other_facets ;

	 quick_erase_facet(facet->fillers,name,slot->name,facet->facet_name) ;
	 dirty = true ;
	 slot->other_facets = facet->next ;
	 delete facet ;
	 }
      //
      // finally, erase the VALUE facet, which may require deleting reverse
      // links if this is a relation slot
      if (slot->value_facet)
	 {
	 FrSymbol *inverse = slot->name->inverseRelation() ;
	 if (inverse)
	    {
	    FrList *fillers ;

	    for (fillers=slot->value_facet ; fillers ; fillers=fillers->rest())
	       {
	       FrObject *filler = fillers->first() ;
	       if (filler && filler->symbolp())
		  deleteInverse(inverse,(FrSymbol*)filler) ;
	       }
	    }
	 quick_erase_facet(slot->value_facet,name,slot->name,symbolVALUE) ;
	 dirty = true ;
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

static bool delete_one_frame(const FrSymbol *obj, FrNullObject, va_list)
{
   FrSymbol *frname = (FrSymbol*)obj ;
   FrFrame *frame = frname->symbolFrame() ;
   if (frame)
      delete frame ;
   return true ;
}

//----------------------------------------------------------------------
// to avoid warnings from different compilers due to varying implementations
//   of va_list, use a helper function

static void del_all_helper(int dummy, ...)
{
   va_list null_valist ;
   va_start(null_valist,dummy) ;
   FrSymbolTable::current()->iterateVA(delete_one_frame,null_valist) ;
   va_end(null_valist) ;
   return ;
}

void delete_all_frames()
{
   del_all_helper(0) ;
   return ;
}

/************************************************************************/
/*    Iteration functions						*/
/************************************************************************/

bool FrFrame::doSlots(FrSlotsFuncFrame *func, va_list args) const
{
   bool success = true ;
   for (const FrSlot *slot = predef_slots ; slot && success ; slot=slot->next)
      {
      FrSafeVAList(args) ;
      success = func((FrFrame *)this,slot->name,FrSafeVarArgs(args)) ;
      FrSafeVAListEnd(args) ;	
      }
   return success ;
}

//----------------------------------------------------------------------

bool FrFrame::doFacets(const FrSymbol *slotname,FrFacetsFuncFrame *func,
		     va_list args) const
{
   for (register const FrSlot *slot = predef_slots ; slot ; slot = slot->next)
      {
      if (slot->name == slotname)
	 {
	 if (slot->value_facet)
	    {
	    FrSafeVAList(args) ;
	    if (!func((FrFrame *)this,slotname,symbolVALUE,
		      FrSafeVarArgs(args)))
	       {
	       FrSafeVAListEnd(args) ;
	       return false ;
	       }
	    FrSafeVAListEnd(args) ;
	    }
	 bool success = true ;
	 for (FrFacet *facet = slot->other_facets ;
	      facet && success ;
	      facet=facet->next)
	    {
	    FrSafeVAList(args) ;
	    if (facet->fillers &&
		!func((FrFrame *)this,slotname,facet->facet_name,
		      FrSafeVarArgs(args)))
	       success = false ;
	    FrSafeVAListEnd(args) ;
	    }
	 return success ;  	// bail out now because we've done the slot
	 }
      }
   return true ;  // successful, but only because slot not found
}

//----------------------------------------------------------------------

bool FrFrame::iterateVA(FrIteratorFunc func, va_list args) const
{
   for (register const FrSlot *slot = predef_slots ; slot ; slot = slot->next)
      {
      bool result ;
      FrSymbol *slotname = slot->name ;
      if (slot->value_facet)
	 {
	 FrList *facet_desc = new FrList(name,slotname,symbolVALUE,
					   slot->value_facet) ;
	 FrSafeVAList(args) ;
	 result = func(facet_desc,FrSafeVarArgs(args)) ;
	 FrSafeVAListEnd(args) ;
	 facet_desc->eraseList(false) ;
	 if (!result)
	    return false ;
	 }
      for (FrFacet *facet = slot->other_facets ; facet ; facet=facet->next)
	 {
	 if (facet->fillers)
	    {
	    FrList *facet_desc = new FrList(name,slotname,facet->facet_name,
					      facet->fillers) ;
	    FrSafeVAList(args) ;
	    result = func(facet_desc,FrSafeVarArgs(args)) ;
	    FrSafeVAListEnd(args) ;
	    facet_desc->eraseList(false) ;
	    if (!result)
	       return false ;
	    }
	 }
      }
   return true ;			// successfully completed iteration
}

//----------------------------------------------------------------------

#if 0
bool FrFrame::doAllFacets(FrAllFacetsFuncFrame *func, va_list args) const
{
   for (register const FrSlot *slot = predef_slots ; slot ; slot = slot->next)
      {
      FrSafeVAList(args) ;
      if (!doFacets(slot->name,func,FrSafeVarArgs(args)))
	 {
	 FrSaveVAListEnd(args) ;
	 return false ;     // failed
	 }
      FrSaveVAListEnd(args) ;
      }
   return true ;            // successful
}
#endif /* 0 */

//----------------------------------------------------------------------

bool __FrCDECL do_slots(const FrFrame *frame,
			  bool (*func)(const FrFrame *frame,
					 const FrSymbol *slot,va_list args),
			  ...)
{
   va_list args ;
   bool result ;

   va_start(args,func) ;
   result = frame ? frame->doSlots(func,args) : false ;
   va_end(args) ;
   return result ;
}

//----------------------------------------------------------------------

bool __FrCDECL do_facets(const FrFrame *frame, const FrSymbol *slot,
			   bool (*func)(const FrFrame *frame,
					  const FrSymbol *slot,
					  const FrSymbol *facet,
					  va_list args),
			   ...)
{
   va_list args ;
   bool result ;

   va_start(args,func) ;
   result = frame ? frame->doFacets(slot,func,args) : false ;
   va_end(args) ;
   return result ;
}

//----------------------------------------------------------------------

#if 0
bool __FrCDECL do_all_facets(const FrFrame *frame,
			       bool (*func)(const FrFrame *frame,
					      const FrSymbol *slot,
					      const FrSymbol *facet,
					      va_list args),
			       ...)
{
   va_list args ;
   bool result ;

   va_start(args,func) ;
   result = frame ? frame->doAllFacets(func,args) : false ;
   va_end(args) ;
   return result ;
}
#endif /* 0 */

//----------------------------------------------------------------------

void __FrCDECL doAllFrames(FrAllFramesFunc *func, ...)
{
   if (!func)
      return ;
   va_list args ;
   va_start(args,func) ;
   FrSymbolTable::current()->iterateFrameVA((FrIteratorFunc)func,args) ;
   va_end(args) ;
}

//----------------------------------------------------------------------

FrInheritanceType set_inheritance_type(FrInheritanceType inherit)
{
   if (FramepaC_set_inhtype_func)
      return FramepaC_set_inhtype_func(inherit) ;
   else
      return NoInherit ;
}


// end of file frframev.cpp //
