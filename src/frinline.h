/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frinline.h	inline wrappers to make procedural interface	*/
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

#ifndef __FRINLINE_H_INCLUDED
#define __FRINLINE_H_INCLUDED

#if defined(_MSC_VER) && _MSC_VER >= 800
// disable the Visual C++ warning about unreferenced inline functions
#pragma warning(disable : 4514)
#endif /* _MSC_VER >= 800 */

//----------------------------------------------------------------------
// non-member functions related to class FrList

inline size_t listlength(const FrList *list) { return list->listlength() ; }
inline bool listequiv(const FrList *l1, const FrList *l2)
   { return l1->equiv(l2) ; }
inline FrList *listmember(const FrList *list,const FrObject *item)
   { return list->member(item) ; }
inline FrList *listmember(const FrList *list,const FrObject *item,FrCompareFunc cmp)
   { return list->member(item,cmp) ; }
inline FrCons *listassoc(const FrList *l,const FrObject *item)
   { return l->assoc(item) ; }
inline FrCons *listassoc(const FrList *l,const FrObject *item,FrCompareFunc cmp)
   { return l->assoc(item,cmp) ; }
inline FrList *copytree(const FrList *l)
   { return l ? (FrList *)l->deepcopy() : 0 ; }
inline FrList *copylist(const FrList *l) { return l ? (FrList *)l->copy() : 0 ; }
inline FrObject *listhead(const FrList *list)
   { return list ? list->first() : 0 ; }
inline FrList *listtail(const FrList *list)
   { return list ? list->rest() : 0 ; }
inline FrList *listdifference(const FrList *l1, const FrList *l2)
   { return l1->difference(l2) ; }
inline FrList *listdifference(const FrList *l1, const FrList *l2,FrCompareFunc cmp)
   { return l1->difference(l2,cmp) ; }
inline FrList *listunion(const FrList *l1, const FrList *l2)
   { return l1->listunion(l2) ; }
inline FrList *listunion(const FrList *l1, const FrList *l2, FrCompareFunc cmp)
   { return l1->listunion(l2,cmp) ; }
inline FrList *listintersection(const FrList* l1, const FrList *l2)
   { return l1->intersection(l2) ; }
inline FrList *listintersection(const FrList* l1,const FrList *l2,FrCompareFunc cmp)
   { return l1->intersection(l2,cmp) ; }
inline FrList *listsort(FrList *l, ListSortCmpFunc *cmpfunc)
   { return l->sort(cmpfunc) ; }
inline FrObject *listnth(FrList *l,size_t n)
   { return l->nth(n) ; }

//----------------------------------------------------------------------
// non-member functions related to class FrSymbol

inline FrSymbol *makeSymbol(const char *name)
   { return FrSymbolTable::add(name) ; }
inline const char *symbol_name(const FrSymbol *symbol)
   { return SYMBOLP(symbol) ? symbol->symbolName() : 0 ; }

//----------------------------------------------------------------------
// procedural interface to FramepaC frame manipulation capabilities

// Watcom C++ 10.x pulls in the functions referenced by inline functions
// even if the functions themselves are never called!  To reduce the
// executable bloat, we define the frame functions in a separate source
// module (frinline.cpp) for Watcom C
#ifdef __WATCOMC__
FrSymbol *frame_name(const FrFrame *frame) ;
FrFrame *find_frame(const FrSymbol *framename) ;
bool frame_is_empty(const FrFrame *frame) ;
void create_slot(FrFrame *frame,const FrSymbol *slot) ;
void create_facet(FrFrame *frame,const FrSymbol *slot,const FrSymbol *facet);
void add_filler(FrFrame *frame,const FrSymbol *slot,
		const FrSymbol *facet,FrObject *filler) ;
void add_value(FrFrame *frame,const FrSymbol *slot,FrObject *filler) ;
void add_sem(FrFrame *frame,const FrSymbol *slot,FrObject *filler) ;
void add_fillers(FrFrame *frame,const FrSymbol *slot,
		 const FrSymbol *facet,FrList *fillers) ;
void add_values(FrFrame *frame,const FrSymbol *slot, FrList *fillers) ;
void add_sems(FrFrame *frame,const FrSymbol *slot,FrList *fillers) ;
void replace_filler(FrFrame *frame,const FrSymbol *slot, const FrSymbol *facet,
		    const FrObject *old, const FrObject *newfiller) ;
void replace_filler(FrFrame *frame,const FrSymbol *slot, const FrSymbol *facet,
		    const FrObject *old, const FrObject *newfiller,
		    FrCompareFunc cmp) ;
void erase_frame(FrFrame *frame) ;
void erase_slot(FrFrame *frame,const FrSymbol *slot) ;
void erase_facet(FrFrame *frame,const FrSymbol *slot, const FrSymbol *facet) ;
void erase_filler(FrFrame *frame,const FrSymbol *slot,
		  const FrSymbol *facet,FrObject *filler) ;
void erase_filler(FrFrame *frame,const FrSymbol *slot,
		  const FrSymbol *facet,FrObject *filler,
		  FrCompareFunc cmp) ;
void erase_value(FrFrame *frame,const FrSymbol *slot,FrObject *fill) ;
void erase_value(FrFrame *frame,const FrSymbol *slot,FrObject *fill,
		 FrCompareFunc cmp) ;
void erase_sem(FrFrame *frame,const FrSymbol *slot,FrObject *fill) ;
void erase_sem(FrFrame *frame,const FrSymbol *slot,FrObject *fill,
	       FrCompareFunc cmp) ;
const FrList *get_fillers(const FrFrame *frame,const FrSymbol *slot,
			   const FrSymbol *facet,bool inherit = true) ;
FrObject *get_filler(const FrFrame *frame,const FrSymbol *slot,
		      const FrSymbol *facet,bool inherit = true) ;
const FrList *get_values(const FrFrame *frame,const FrSymbol *slot,
			  bool inherit = true) ;
FrObject *get_value(const FrFrame *frame,const FrSymbol *slot,
		     bool inherit = true) ;
const FrList *get_sem(const FrFrame *frame,const FrSymbol *slot,
		       bool inherit = true) ;
bool frame_locked(const FrFrame *frame) ;
void copy_frame(FrSymbol *oldframe, FrSymbol *newframe,
		bool temporary = false) ;
bool is_a_p(const FrFrame *frame,const FrFrame *poss_parent) ;
bool part_of_p(const FrFrame *frame,const FrFrame *poss_container) ;
FrList *inheritable_slots(const FrFrame *frame,FrInheritanceType inherit,
				  bool include_names) ;
void inherit_all_fillers(FrFrame *frame) ;
void inherit_all_fillers(FrFrame *frame,FrInheritanceType inherit) ;
FrList *slots_in_frame(const FrFrame *frame) ;
FrList *facets_in_slot(const FrFrame *frame, const FrSymbol *slot) ;
#else
inline FrSymbol *frame_name(const FrFrame *frame)
   { return frame ? frame->frameName() : 0 ; }
inline FrFrame *find_frame(const FrSymbol *framename)
   { return framename ? framename->symbolFrame() : 0 ; }
inline bool frame_is_empty(const FrFrame *frame)
   { return frame ? frame->emptyFrame() : false ; }
inline void create_slot(FrFrame *frame,const FrSymbol *slot)
   { if (frame) (void)frame->createSlot(slot) ; }
inline void create_facet(FrFrame *frame,const FrSymbol *slot,const FrSymbol *facet)
   { if (frame) (void)frame->createFacet(slot,facet) ; }
inline void add_filler(FrFrame *frame,const FrSymbol *slot,
		      const FrSymbol *facet,FrObject *filler)
   { if (frame) frame->addFiller(slot,facet,filler) ; }
inline void add_value(FrFrame *frame,const FrSymbol *slot,FrObject *filler)
   { if (frame) frame->addValue(slot,filler) ; }
inline void add_sem(FrFrame *frame,const FrSymbol *slot,FrObject *filler)
   { if (frame) frame->addSem(slot,filler) ; }
inline void add_fillers(FrFrame *frame,const FrSymbol *slot,
		        const FrSymbol *facet,FrList *fillers)
   { if (frame) frame->addFillers(slot,facet,fillers) ; }
inline void add_values(FrFrame *frame,const FrSymbol *slot, FrList *fillers)
   { if (frame) frame->addValues(slot,fillers) ; }
inline void add_sems(FrFrame *frame,const FrSymbol *slot,FrList *fillers)
   { if (frame) frame->addSems(slot,fillers) ; }
inline void replace_filler(FrFrame *frame,const FrSymbol *slot, const FrSymbol *facet,
			   const FrObject *old, const FrObject *newfiller)
   { if (frame) frame->replaceFiller(slot,facet,old,newfiller) ; }
inline void replace_filler(FrFrame *frame,const FrSymbol *slot, const FrSymbol *facet,
			   const FrObject *old, const FrObject *newfiller,
			   FrCompareFunc cmp)
   { if (frame) frame->replaceFiller(slot,facet,old,newfiller,cmp) ; }
inline void erase_frame(FrFrame *frame)
   { if (frame) frame->eraseFrame() ; }
inline void erase_slot(FrFrame *frame,const FrSymbol *slot)
   { if (frame) frame->eraseSlot(slot) ; }
inline void erase_facet(FrFrame *frame,const FrSymbol *slot, const FrSymbol *facet)
   { if (frame) frame->eraseFacet(slot,facet) ; }
inline void erase_filler(FrFrame *frame,const FrSymbol *slot,
		         const FrSymbol *facet,FrObject *filler)
   { if (frame) frame->eraseFiller(slot,facet,filler) ; }
inline void erase_filler(FrFrame *frame,const FrSymbol *slot,
		         const FrSymbol *facet,FrObject *filler,
			 FrCompareFunc cmp)
   { if (frame) frame->eraseFiller(slot,facet,filler,cmp) ; }
inline void erase_value(FrFrame *frame,const FrSymbol *slot,FrObject *fill)
   { if (frame) frame->eraseValue(slot,fill) ; }
inline void erase_value(FrFrame *frame,const FrSymbol *slot,FrObject *fill,
			FrCompareFunc cmp)
   { if (frame) frame->eraseValue(slot,fill,cmp) ; }
inline void erase_sem(FrFrame *frame,const FrSymbol *slot,FrObject *fill)
   { if (frame) frame->eraseSem(slot,fill) ; }
inline void erase_sem(FrFrame *frame,const FrSymbol *slot,FrObject *fill,
			FrCompareFunc cmp)
   { if (frame) frame->eraseSem(slot,fill,cmp) ; }
inline const FrList *get_fillers(const FrFrame *frame,const FrSymbol *slot,
				  const FrSymbol *facet,bool inherit = true)
   { return frame ? frame->getFillers(slot,facet,inherit) : 0 ; }
inline FrObject *get_filler(const FrFrame *frame,const FrSymbol *slot,
			 const FrSymbol *facet,bool inherit = true)
   { return frame ? frame->firstFiller(slot,facet,inherit) : 0 ; }
inline const FrList *get_values(const FrFrame *frame,const FrSymbol *slot,
				 bool inherit = true)
   { return frame ? frame->getValues(slot,inherit) : 0 ; }
inline FrObject *get_value(const FrFrame *frame,const FrSymbol *slot,
			 bool inherit = true)
   { return frame ? frame->getValue(slot,inherit) : 0 ; }
inline const FrList *get_sem(const FrFrame *frame,const FrSymbol *slot,
			      bool inherit = true)
   { return frame ? frame->getSem(slot,inherit) : 0 ; }
inline bool frame_locked(const FrFrame *frame)
   { return frame ? frame->isLocked() : 0 ; }
inline void copy_frame(FrSymbol *oldframe, FrSymbol *newframe,
 		       bool temporary = false)
   { FrFrame *fr = oldframe->symbolFrame() ;
     if (fr) fr->copyFrame(newframe,temporary) ; }
inline bool is_a_p(const FrFrame *frame,const FrFrame *poss_parent)
   { return frame ? frame->isA_p(poss_parent) : false ; }
inline bool part_of_p(const FrFrame *frame,const FrFrame *poss_container)
   { return frame ? frame->partOf_p(poss_container) : false ; }
inline FrList *inheritable_slots(const FrFrame *frame,FrInheritanceType inherit,
				  bool include_names)
   { return frame ? frame->collectSlots(inherit,0,include_names) : 0 ; }
inline void inherit_all_fillers(FrFrame *frame)
   { if (frame) frame->inheritAll() ; }
inline void inherit_all_fillers(FrFrame *frame,FrInheritanceType inherit)
   { if (frame) frame->inheritAll(inherit) ; }
inline FrList *slots_in_frame(const FrFrame *frame)
   { return frame ? frame->allSlots() : 0 ; }
inline FrList *facets_in_slot(const FrFrame *frame, const FrSymbol *slot)
   { return (frame && slot) ? frame->slotFacets(slot) : 0 ; }
#endif /* __WATCOMC__ */

//----------------------------------------------------------------------
// procedural interface to FramepaC virtual frame manipulation capabilities

// Watcom C++ 10.x pulls in the functions referenced by inline functions
// even if the functions themselves are never called!  To reduce the
// executable bloat, we define the frame functions in a separate source
// module (frinline.cpp) for Watcom C
#ifdef __WATCOMC__
bool is_frame(FrSymbol *frname) ;
bool is_deleted_frame(FrSymbol *frname) ;
FrFrame *find_vframe(FrSymbol *frname) ;
FrFrame *create_vframe(FrSymbol *frame) ;
bool frame_is_empty(FrSymbol *frame) ;
bool frame_is_dirty(FrSymbol *frame) ;
int start_transaction() ;
int end_transaction(int transaction) ;
int abort_transaction(int transaction) ;
FrFrame *lock_frame(FrSymbol *frame) ;
bool unlock_frame(FrSymbol *frame) ;
bool frame_locked(FrSymbol *frame) ;
int commit_frame(FrSymbol *frame) ;
int discard_frame(FrSymbol *frame) ;
int delete_frame(FrSymbol *frame) ;
FrFrame *get_old_frame(FrSymbol *frame, int generation) ;
void copy_vframe(FrSymbol *oldframe, FrSymbol *newframe) ;
void create_slot(FrSymbol *frame,const FrSymbol *slot) ;
void create_facet(FrSymbol *frame,const FrSymbol *slot,const FrSymbol *facet) ;
void add_filler(FrSymbol *frame,const FrSymbol *slot,
		const FrSymbol *facet, const FrObject *filler) ;
void add_value(FrSymbol *frame,const FrSymbol *slot, const FrObject *filler) ;
void add_sem(FrSymbol *frame,const FrSymbol *slot, const FrObject *filler);
void add_fillers(FrSymbol *frame,const FrSymbol *slot,
		 const FrSymbol *facet, const FrList *fillers) ;
void add_values(FrSymbol *frame,const FrSymbol *slot, const FrList *fillers);
void add_sems(FrSymbol *frame,const FrSymbol *slot,const FrList *fillers);
void replace_filler(FrSymbol *frame, const FrSymbol *slot,
		    const FrSymbol *facet, const FrObject *old,
		    const FrObject *newfiller) ;
void replace_filler(FrSymbol *frame, const FrSymbol *slot,
		    const FrSymbol *facet, const FrObject *old,
		    const FrObject *newfiller, FrCompareFunc cmp) ;
void erase_frame(FrSymbol *frame) ;
void erase_slot(FrSymbol *frame,const FrSymbol *slot) ;
void erase_facet(FrSymbol *frame,const FrSymbol *slot, const FrSymbol *facet) ;
void erase_filler(FrSymbol *frame,const FrSymbol *slot,
		  const FrSymbol *facet,FrObject *filler) ;
void erase_filler(FrSymbol *frame,const FrSymbol *slot,
		  const FrSymbol *facet,FrObject *filler,
		  FrCompareFunc cmp) ;
void erase_sem(FrSymbol *frame,const FrSymbol *slot,FrObject *fill) ;
void erase_sem(FrSymbol *frame,const FrSymbol *slot,FrObject *fill,
	       FrCompareFunc cmp) ;
void erase_value(FrSymbol *frame,const FrSymbol *slot,FrObject *fill) ;
void erase_value(FrSymbol *frame,const FrSymbol *slot,FrObject *fill,
		 FrCompareFunc cmp) ;
const FrList *get_fillers(FrSymbol *frame,const FrSymbol *slot,
			   const FrSymbol *facet,bool inherit = true) ;
FrObject *get_filler(FrSymbol *frame,const FrSymbol *slot,
			 const FrSymbol *facet,bool inherit = true) ;
const FrList *get_values(FrSymbol *frame,const FrSymbol *slot,
			 bool inherit = true) ;
FrObject *get_value(FrSymbol *frame,const FrSymbol *slot,
		     bool inherit = true) ;
const FrList *get_sem(FrSymbol *frame,const FrSymbol *slot,
			      bool inherit = true) ;
bool is_a_p(FrSymbol *frame, FrSymbol *poss_parent) ;
bool part_of_p(FrSymbol *frame, FrSymbol *poss_container) ;
FrList *inheritable_slots(FrSymbol *frame,FrInheritanceType inherit) ;
void inherit_all_fillers(FrSymbol *frame) ;
void inherit_all_fillers(FrSymbol *frame,FrInheritanceType inherit) ;
FrList *slots_in_frame(FrSymbol *frame) ;
FrList *facets_in_slot(FrSymbol *frame, const FrSymbol *slot) ;
#else
inline bool is_frame(FrSymbol *frname)
   { return frname ? frname->isFrame() : false ; }
inline bool is_deleted_frame(FrSymbol *frname)
   { return frname ? frname->isDeletedFrame() : false ; }
inline FrFrame *find_vframe(FrSymbol *frname)
   { return frname ? frname->findFrame() : 0 ; }
inline FrFrame *create_vframe(FrSymbol *frame)
   { return frame ? frame->createVFrame() : 0 ; }
inline bool frame_is_empty(FrSymbol *frame)
   { return frame ? frame->emptyFrame() : false ; }
inline bool frame_is_dirty(FrSymbol *frame)
   { return frame ? frame->dirtyFrame() : false ; }
inline int start_transaction()
   { return ((FrSymbol*)0)->startTransaction() ; }
inline int end_transaction(int transaction)
   { return ((FrSymbol*)0)->endTransaction(transaction) ; }
inline int abort_transaction(int transaction)
   { return ((FrSymbol*)0)->abortTransaction(transaction) ; }
inline FrFrame *lock_frame(FrSymbol *frame)
   { return frame ? frame->lockFrame() : 0 ; }
inline bool unlock_frame(FrSymbol *frame)
   { return frame ? frame->unlockFrame() : false ; }
inline bool frame_locked(FrSymbol *frame)
   { return frame ? frame->isLocked() : false ; }
inline int commit_frame(FrSymbol *frame)
   { return frame ? frame->commitFrame() : 0 ; }
inline int discard_frame(FrSymbol *frame)
   { return frame ? frame->discardFrame() : 0 ; }
inline int delete_frame(FrSymbol *frame)
   { return frame ? frame->deleteFrame() : 0 ; }
inline FrFrame *get_old_frame(FrSymbol *frame, int generation)
   { return frame ? frame->oldFrame(generation) : 0 ; }
inline void copy_vframe(FrSymbol *oldframe, FrSymbol *newframe)
   { if (oldframe) oldframe->copyFrame(newframe) ; }
inline void create_slot(FrSymbol *frame,const FrSymbol *slot)
   { if (frame) (void)frame->createSlot(slot) ; }
inline void create_facet(FrSymbol *frame,const FrSymbol *slot,const FrSymbol *facet)
   { if (frame) (void)frame->createFacet(slot,facet) ; }
inline void add_filler(FrSymbol *frame,const FrSymbol *slot,
		      const FrSymbol *facet, const FrObject *filler)
   { if (frame) frame->addFiller(slot,facet,filler) ; }
inline void add_value(FrSymbol *frame,const FrSymbol *slot, const FrObject *filler)
   { if (frame) frame->addValue(slot,filler) ; }
inline void add_sem(FrSymbol *frame,const FrSymbol *slot, const FrObject *filler)
   { if (frame) frame->addSem(slot,filler) ; }
inline void add_fillers(FrSymbol *frame,const FrSymbol *slot,
		        const FrSymbol *facet, const FrList *fillers)
   { if (frame) frame->addFillers(slot,facet,fillers) ; }
inline void add_values(FrSymbol *frame,const FrSymbol *slot, const FrList *fillers)
   { if (frame) frame->addValues(slot,fillers) ; }
inline void add_sems(FrSymbol *frame,const FrSymbol *slot,const FrList *fillers)
   { if (frame) frame->addSems(slot,fillers) ; }
inline void replace_filler(FrSymbol *frame, const FrSymbol *slot,
			   const FrSymbol *facet, const FrObject *old,
			   const FrObject *newfiller)
   { if (frame) frame->replaceFiller(slot,facet,old,newfiller) ; }
inline void replace_filler(FrSymbol *frame, const FrSymbol *slot,
			   const FrSymbol *facet, const FrObject *old,
			   const FrObject *newfiller, FrCompareFunc cmp)
   { if (frame) frame->replaceFiller(slot,facet,old,newfiller,cmp) ; }
inline void erase_frame(FrSymbol *frame)
   { if (frame) frame->eraseFrame() ; }
inline void erase_slot(FrSymbol *frame,const FrSymbol *slot)
   { if (frame) frame->eraseSlot(slot) ; }
inline void erase_facet(FrSymbol *frame,const FrSymbol *slot, const FrSymbol *facet)
   { if (frame) frame->eraseFacet(slot,facet) ; }
inline void erase_filler(FrSymbol *frame,const FrSymbol *slot,
		         const FrSymbol *facet,FrObject *filler)
   { if (frame) frame->eraseFiller(slot,facet,filler) ; }
inline void erase_filler(FrSymbol *frame,const FrSymbol *slot,
		         const FrSymbol *facet,FrObject *filler,
			 FrCompareFunc cmp)
   { if (frame) frame->eraseFiller(slot,facet,filler,cmp) ; }
inline void erase_sem(FrSymbol *frame,const FrSymbol *slot,FrObject *fill)
   { if (frame) frame->eraseSem(slot,fill) ; }
inline void erase_sem(FrSymbol *frame,const FrSymbol *slot,FrObject *fill,
			FrCompareFunc cmp)
   { if (frame) frame->eraseSem(slot,fill,cmp) ; }
inline void erase_value(FrSymbol *frame,const FrSymbol *slot,FrObject *fill)
   { if (frame) frame->eraseValue(slot,fill) ; }
inline void erase_value(FrSymbol *frame,const FrSymbol *slot,FrObject *fill,
			FrCompareFunc cmp)
   { if (frame) frame->eraseValue(slot,fill,cmp) ; }
inline const FrList *get_fillers(FrSymbol *frame,const FrSymbol *slot,
				  const FrSymbol *facet,bool inherit = true)
   { return frame ? frame->getFillers(slot,facet,inherit) : 0 ; }
inline FrObject *get_filler(FrSymbol *frame,const FrSymbol *slot,
			 const FrSymbol *facet,bool inherit = true)
   { return frame ? frame->firstFiller(slot,facet,inherit) : 0 ; }
inline const FrList *get_values(FrSymbol *frame,const FrSymbol *slot,
			 bool inherit = true)
   { return frame ? frame->getValues(slot,inherit) : 0 ; }
inline FrObject *get_value(FrSymbol *frame,const FrSymbol *slot,
			 bool inherit = true)
   { return frame ? frame->getValue(slot,inherit) : 0 ; }
inline const FrList *get_sem(FrSymbol *frame,const FrSymbol *slot,
			      bool inherit = true)
   { return frame ? frame->getSem(slot,inherit) : 0 ; }
inline bool is_a_p(FrSymbol *frame, FrSymbol *poss_parent)
   { return frame ? frame->isA_p(poss_parent) : false ; }
inline bool part_of_p(FrSymbol *frame, FrSymbol *poss_container)
   { return frame ? frame->partOf_p(poss_container) : false ; }
inline FrList *inheritable_slots(FrSymbol *frame,FrInheritanceType inherit)
   { return frame ? frame->collectSlots(inherit) : 0 ; }
inline void inherit_all_fillers(FrSymbol *frame)
   { if (frame) frame->inheritAll() ; }
inline void inherit_all_fillers(FrSymbol *frame,FrInheritanceType inherit)
   { if (frame) frame->inheritAll(inherit) ; }
inline FrList *slots_in_frame(FrSymbol *frame)
   { return frame ? frame->allSlots() : 0 ; }
inline FrList *facets_in_slot(FrSymbol *frame, const FrSymbol *slot)
   { return (frame && slot) ? frame->slotFacets(slot) : 0 ; }
#endif /* __WATCOMC__ */

//----------------------------------------------------------------------
// procedural interface to FramepaC symbol table manipulation capabilities

inline FrSymbolTable *create_symbol_table(int max_symbols)
   { return new FrSymbolTable(max_symbols) ; }
inline void name_symbol_table(FrSymbolTable *symtab,const char *name)
   { if (symtab) symtab->setName(name) ; }
inline FrSymbolTable *select_symbol_table(FrSymbolTable *newsym)
   { return newsym ? newsym->select() : FrSymbolTable::selectDefault() ; }
inline FrSymbolTable *default_symbol_table()
   { return FrSymbolTable::selectDefault() ; }
inline FrSymbolTable *current_symbol_table()
   { return FrSymbolTable::current() ; }
inline void set_notification(FrSymbolTable *symtab,
			     VFrameNotifyType type,
			     VFrameNotifyFunc *func)
   { if (symtab) symtab->setNotify(type,func) ; }
inline VFrameNotifyFunc *get_notification(FrSymbolTable *symtab,
				          VFrameNotifyType type)
   { return symtab ? symtab->getNotify(type) : 0 ; }


#endif /* __FRINLINE_H_INCLUDED */

// end of file frinline.h //
