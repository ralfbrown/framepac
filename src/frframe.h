/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frframe.h		class FrFrame				*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,1998,2001,2006,2009		*/
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

#ifndef __FRFRAME_H_INCLUDED
#define __FRFRAME_H_INCLUDED

// the following also includes frlist.h and frobject.h as needed
#ifndef __FRSYMBOL_H_INCLUDED
#include "frsymbol.h"
#endif

#ifndef __FRREADER_H_INCLUDED
#include "frreader.h"
#endif

#ifndef __FRLRU_H_INCLUDED
#include "frlru.h"
#endif

#if defined(__GNUC__)
#  pragma interface
#endif

/**********************************************************************/
/*    Manifest Constants					      */
/**********************************************************************/

#define FrNUM_PREDEFINED_SLOTS 4

/**********************************************************************/
/*    Other types associated with FrFrame			      */
/**********************************************************************/

typedef bool FrSlotsFuncFrame(const FrFrame *,const FrSymbol *,va_list) ;
typedef bool FrFacetsFuncFrame(const FrFrame *,const FrSymbol *,
			       const FrSymbol *, va_list) ;
typedef bool FrAllFacetsFuncFrame(const FrFrame *,const FrSymbol *,
				  const FrSymbol *, va_list) ;
typedef void FrAllFramesFunc(FrFrame *, va_list) ;

typedef FrSymbol *FrInheritanceLocFunc(const FrSymbol *frame,
				        const FrSymbol *slot,
					const FrSymbol *facet) ;

/**********************************************************************/
/*    Private class FrSlot					      */
/**********************************************************************/

class FrSlot
   {
   private:
      static FrAllocator allocator ;
   public:
      FrSlot *next ;
      FrSymbol *name ;
      FrList *value_facet ;
      FrFacet *other_facets ;
   // member functions
      void *operator new(size_t) { return allocator.allocate() ; }
      void operator delete(void *s) { allocator.release(s) ; }
      FrSlot() {}
      FrSlot(FrSymbol *slotname)
	 { next = 0 ; name = slotname ; value_facet = 0 ; other_facets = 0 ; }
   } ;

/************************************************************************/
/*	Declarations for class FrFrame					*/
/************************************************************************/

class FrFrame : public FrObject
   {
   private:
      static FrAllocator allocator ;
      static FrReader reader ;
   protected:
      FrSymbol *name ;
      FrSlot *last_slot ;
      FrSlot predef_slots[FrNUM_PREDEFINED_SLOTS] ;
#ifdef FrLRU_DISCARD
   public:
      uint32_t LRUclock ;
#endif /* FrLRU_DISCARD */
   protected:
      char visited ;
      char dirty ;
      char virtual_frame ;
      char locked ;
   private: // member functions
      void printSlot(ostream &output,const FrSlot *slot) const ;
      void deleteInverse(const FrSymbol *slotname, FrSymbol *framename) ;
      bool insertInverse(const FrSymbol *slotname, FrSymbol *framename) ;
   protected: // member functions
      void build_empty_frame(FrSymbol *frname) ;
   public: // member functions which should not be called by user code
      void markDirty(bool dirt = true) { dirty = (char)dirt ; }
      void setLock(bool lock = true) { locked = (char)lock ; }
      void addFillerNoCopy(const FrSymbol *slotname,const FrSymbol *facet,
			   FrObject *filler) ;
#ifdef FrLRU_DISCARD
      void adjust_LRUclock()
	 { if (LRUclock>LRUclock_LIMIT) LRUclock -= LRUclock_LIMIT ; else LRUclock = 0 ; }
      uint32_t getLRUclock() const { return LRUclock ; }
      void setLRUclock(uint32_t value) { LRUclock = value ; }
#endif /* FrLRU_DISCARD */
   public:
      void *operator new(size_t) { return allocator.allocate() ; }
      void operator delete(void *fr,size_t) { allocator.release(fr) ; }
      FrFrame() ;
      FrFrame(const char *framename) ;
      FrFrame(FrSymbol *framename) ;
      virtual ~FrFrame() ;
      virtual FrObjectType objType() const ;
      virtual const char *objTypeName() const ;
      virtual FrObjectType objSuperclass() const ;
      virtual FrReader *objReader() const { return &reader ; }
      virtual ostream &printValue(ostream &output) const ;
      virtual char *displayValue(char *buffer) const ;
      virtual size_t displayLength() const ;
      virtual bool framep() const ;
      virtual void freeObject() {}  // normally don't want to delete frame
      virtual bool iterateVA(FrIteratorFunc func, va_list args) const ;
      bool dirtyFrame() const { return (bool)dirty ; }
      bool isLocked() const { return (bool)locked ; }
      bool isVFrame() const { return (bool)virtual_frame ; }
      int commitFrame() ;
      int numberOfSlots() const ;
      FrSymbol *frameName() const { return name ; }
      FrFrame *copyFrame(FrSymbol *newname, bool temporary = false) const ;
      bool renameFrame(FrSymbol *newname) ;
      bool emptyFrame() const ;
      FrSlot *createSlot(const FrSymbol *slotname) ;
      void createFacet(const FrSymbol *slotname, const FrSymbol *facetname) ;
      void addFiller(const FrSymbol *slotname,const FrSymbol *facet,
		     const FrObject *filler) ;
      void addValue(const FrSymbol *slotname, const FrObject *filler) ;
      void addSem(const FrSymbol *slotname, const FrObject *filler) ;
      void addFillers(const FrSymbol *slotname,const FrSymbol *facet,
		      const FrList *fillers);
      void addValues(const FrSymbol *slotname, const FrList *fillers) ;
      void addSems(const FrSymbol *slotname, const FrList *fillers) ;
      void replaceFiller(const FrSymbol *slotname,const FrSymbol *facet,
			 const FrObject *oldfiller,const FrObject *newfiller) ;
      void replaceFiller(const FrSymbol *slotname,const FrSymbol *facet,
			 const FrObject *oldfiller,const FrObject *newfiller,
			 FrCompareFunc cmp) ;
      void replaceValue(const FrSymbol *slotname,const FrObject *oldfiller,
			const FrObject *newfiller) ;
      void replaceValue(const FrSymbol *slotname,const FrObject *oldfiller,
			const FrObject *newfiller,FrCompareFunc cmp) ;
      void replaceSem(const FrSymbol *slotname,const FrObject *oldfiller,
		      const FrObject *newfiller) ;
      void replaceSem(const FrSymbol *slotname,const FrObject *oldfiller,
		      const FrObject *newfiller,FrCompareFunc cmp) ;
      void replaceFacet(const FrSymbol *slotname, const FrSymbol *facet,
			const FrList *newfillers) ;
      void eraseFrame() ;
      void eraseCopyFrame() ;
      void discard() ;
      void eraseSlot(const char *slotname) ;
      void eraseSlot(const FrSymbol *slotname) ;
      void eraseFacet(const FrSymbol *slotname,const FrSymbol *facetname) ;
      void eraseFiller(const FrSymbol *slotname,const FrSymbol *facetname,
		       const FrObject *filler) ;
      void eraseFiller(const FrSymbol *slotname,const FrSymbol *facetname,
		       const FrObject *filler,FrCompareFunc cmp) ;
      void eraseValue(const FrSymbol *slotname, const FrObject *filler) ;
      void eraseValue(const FrSymbol *slotname, const FrObject *filler,
			      FrCompareFunc cmp) ;
      void eraseSem(const FrSymbol *slotname, const FrObject *filler) ;
      void eraseSem(const FrSymbol *slotname, const FrObject *filler,
			      FrCompareFunc cmp) ;
      const FrList *getImmedFillers(const FrSymbol *slotname,
				     const FrSymbol *facet) const ;
      const FrList *getImmedValues(const FrSymbol *slotname) const ;
      const FrList *getFillers(const FrSymbol *slotname,
				const FrSymbol *facet,
				bool inherit = true) const ;
      FrObject *firstFiller(const FrSymbol *slotname,const FrSymbol *facet,
			      bool inherit = true) const;
      const FrList *getValues(const FrSymbol *slotname,
			       bool inherit = true) const ;
      FrObject *getValue(const FrSymbol *slotname,bool inherit=true) const ;
      const FrList *getSem(const FrSymbol *slotname,
			    bool inherit = true) const ;
      FrObject *popFiller(const FrSymbol *slotname,const FrSymbol *facet) ;
      void pushFiller(const FrSymbol *slotname,const FrSymbol *facet,
		      const FrObject *filler) ;
      FrSymbol *isA() const ;
      const FrList *isA_list() const ;
      FrSymbol *instanceOf() const ;
      const FrList *instanceOf_list() const ;
      FrSymbol *partOf() const ;
      const FrList *partOf_list() const ;
      const FrList *subclassesOf() const ;
      const FrList *instances() const ;
      const FrList *hasParts() const ;
      bool isA_p(const FrFrame *poss_parent) const ;
      bool partOf_p(const FrFrame *poss_container) const ;
      void markVisited() { visited = true ; }
      void clearVisited() { visited = false ; }
      bool wasVisited() const { return (bool)visited ; }
      FrList *collectSlots(FrInheritanceType inherit,FrList *allslots=0,
			    bool include_names = false) const ;
      void inheritAll() ;
      void inheritAll(FrInheritanceType inherit) ;
      FrList *allSlots() const ;
      FrList *slotFacets(const FrSymbol *slot) const ;
      bool doSlots(FrSlotsFuncFrame *func,va_list args) const ;
      bool doFacets(const FrSymbol *slotname,FrFacetsFuncFrame *func,
		    va_list args) const ;
//      bool doAllFacets(FrAllFacetsFuncFrame *func, va_list args) const ;
//(use iterate() or iterateVA() instead of doAllFacets)
   // friend functions
      friend void __FrCDECL doAllFrames(FrAllFramesFunc *func, ...) ;
      friend void compact_Frame(bool) ;
   } ;

//----------------------------------------------------------------------
// non-member functions related to class FrFrame

FrFrame *create_frame(const char *framename) ;
FrFrame *create_frame(const FrSymbol *framename) ;
void delete_all_frames() ;

void define_relation(const char *relname, const char *invname) ;
void undefine_relation(const char *relname, const char *invname) ;
FrInheritanceType set_inheritance_type(const FrSymbol *inherit) ;
FrInheritanceType set_inheritance_type(FrInheritanceType inherit) ;
void set_user_inheritance(FrInheritanceFunc *, FrInheritanceLocFunc *) ;
void get_user_inheritance(FrInheritanceFunc **, FrInheritanceLocFunc **) ;
FrInheritanceType get_inheritance_type() ;

//----------------------------------------------------------------------
// procedural interface to FramepaC frame manipulation capabilities

bool __FrCDECL do_slots(const FrFrame *frame,
			  bool (*func)(const FrFrame *frame,
					 const FrSymbol *slot,
					 va_list args),
			  ...) ;
bool __FrCDECL do_facets(const FrFrame *frame,const FrSymbol *slotname,
			   bool (*func)(const FrFrame *frame,
					  const FrSymbol *slot,
					  const FrSymbol *facet,
					  va_list args),
			   ...) ;
#if 0
// use frame->iterate() or frame->iterateVA() instead
bool __FrCDECL do_all_facets(const FrFrame *frame,
			       bool (*func)(const FrFrame *frame,
					      const FrSymbol *slot,
					      const FrSymbol *facet,
					      va_list args),
			       ...) ;
#endif /* 0 */

//----------------------------------------------------------------------
// FrameKit compatibility

FrFrame *FrameKit_to_FramepaC(const FrList *framespec, bool warn = true) ;
FrList *FramepaC_to_FrameKit(const FrFrame *frame) ;
bool import_FrameKit_frames(istream &input, ostream &report) ;
bool export_FrameKit_frames(ostream &output, FrList *frames) ;
bool export_FramepaC_frames(ostream &output, FrList *frames) ;

#endif /* !__FRFRAME_H_INCLUDED */

// end of file frframe.h //
