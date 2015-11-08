/************************************************************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frsymbol.h	       class FrSymbol				*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,1998,2000,2001,2009,2012,2015	*/
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

#ifndef __FRSYMBOL_H_INCLUDED
#define __FRSYMBOL_H_INCLUDED

#ifndef __FRLIST_H_INCLUDED
#include "frlist.h"
#endif

#include <string.h>

#if defined(__GNUC__)
#  pragma interface
#endif

/**********************************************************************/
/*	Optional Demon support					      */
/**********************************************************************/

typedef bool FrDemonFunc(const FrSymbol *frame, const FrSymbol *slot,
			   const FrSymbol *facet, const FrObject *value,
			   va_list args) ;

enum FrDemonType
   {
   DT_IfCreated, DT_IfAdded, DT_IfRetrieved, DT_IfMissing, DT_IfDeleted
   } ;

struct FrDemonList ;

struct FrDemons
   {
   FrDemonList *if_created ; // demons to call just after creating facet
   FrDemonList *if_added ;   // demons to call just before adding filler
   FrDemonList *if_missing ; // demons to call before attempting inheritance
   FrDemonList *if_retrieved ; // demons to call just before returning filler
   FrDemonList *if_deleted ; // demons to call just after deleting filler
   void *operator new(size_t size) { return FrMalloc(size) ; }
   void operator delete(void *obj) { FrFree(obj) ; }
   } ;

/**********************************************************************/
/**********************************************************************/

#ifdef FrSYMBOL_RELATION
#  define if_FrSYMBOL_RELATION(x) x
#else
#  define if_FrSYMBOL_RELATION(x)
#endif

#ifdef FrSYMBOL_VALUE
#  define if_FrSYMBOL_VALUE(x) x
#else
#  define if_FrSYMBOL_VALUE(x)
#endif

#ifdef FrDEMONS
#  define if_FrDEMONS(x) x
#else
#  define if_FrDEMONS(x)
#endif

FrSymbol *findSymbol(const char *name) ;

class FrSymbol : public FrAtom
   {
   private:
      FrFrame  *m_frame ;		// in lieu of a full property list,
      if_FrSYMBOL_RELATION(FrSymbol *m_inv_relation) ;// we have these 2 ptrs
      if_FrSYMBOL_VALUE(FrObject *m_value) ;
      if_FrDEMONS(FrDemons  *theDemons) ;
      char      m_name[1] ; //__attribute__((bnd_variable_size)) ;
   private: // methods
      void setInvRelation(FrSymbol *inv)
	 {
	 (void)inv ;  if_FrSYMBOL_RELATION(m_inv_relation = inv) ;
	 }
      void setDemons(FrDemons *d)
	 {
	 (void)d ; if_FrDEMONS(theDemons = d) ;
	 }
      void *operator new(size_t size,void *where)
	  { (void)size; return where ; }
      FrSymbol(const char *symname,int len)
	 // private use for FramepaC only //
	  { memcpy(m_name,(char *)symname,len) ;
	    if_FrSYMBOL_VALUE(m_value = &UNBOUND) ;
	    setFrame(0) ;
	    setInvRelation(0) ;
	    setDemons(0) ;
	  }
   public:
      FrSymbol() _fnattr_noreturn ;
      virtual ~FrSymbol()  _fnattr_noreturn ;
      void *operator new(size_t size) { return FrMalloc(size) ; }
      void operator delete(void *sym, size_t) { FrFree(sym) ; }
      virtual FrObjectType objType() const ;
      virtual const char *objTypeName() const ;
      virtual FrObjectType objSuperclass() const ;
      virtual ostream &printValue(ostream &output) const ;
      virtual char *displayValue(char *buffer) const ;
      virtual size_t displayLength() const ;
      virtual void freeObject() {}  // can never free a FrSymbol
      virtual long int intValue() const ;
      virtual const char *printableName() const ;
      virtual FrSymbol *coerce2symbol(FrCharEncoding) const
	  { return (FrSymbol*)this ; }
      virtual bool symbolp() const ;
      virtual size_t length() const ;
      virtual int compare(const FrObject *obj) const ;
      FrSymbol *makeSymbol(const char *nam) const ;
      FrSymbol *findSymbol(const char *nam) const ;
      const char *symbolName() const { return m_name ; }
      static bool nameNeedsQuoting(const char *name) ;
      FrFrame *symbolFrame() const { return m_frame ; }
      ostream &printBinding(ostream &output) const ;
#ifdef FrSYMBOL_VALUE
      void setValue(const FrObject *newval) { m_value = (FrObject *)newval; }
      FrObject *symbolValue() const { return m_value ; }
#else
      void setValue(const FrObject *newval) ;
      FrObject *symbolValue() const ;
#endif /* FrSYMBOL_VALUE */
#ifdef FrDEMONS
      FrDemons *demons() const { return theDemons ; }
      bool addDemon(FrDemonType type,FrDemonFunc *func, va_list args = 0) ;
      bool removeDemon(FrDemonType type, FrDemonFunc *func) ;
#else
      FrDemons *demons() const { return 0 ; }
      bool addDemon(FrDemonType,FrDemonFunc,va_list=0) { return false ; }
      bool removeDemon(FrDemonType,FrDemonFunc) { return false ; }
#endif /* FrDEMONS */
      void setFrame(const FrFrame *fr) { m_frame = (FrFrame *)fr ; }
      void defineRelation(const FrSymbol *inverse) ;
      void undefineRelation() ;
#ifdef FrSYMBOL_RELATION
      FrSymbol *inverseRelation() const { return m_inv_relation ; }
#else
      FrSymbol *inverseRelation() const { return 0 ; }
#endif /* FrSYMBOL_RELATION */
   //functions to support VFrames
      FrFrame *createFrame() ;
      FrFrame *createVFrame() ;
      FrFrame *createInstanceFrame() ;
      bool isFrame() ;
      bool isDeletedFrame() ;
      FrFrame *findFrame() ;
      int startTransaction() ;
      int endTransaction(int transaction) ;
      int abortTransaction(int transaction) ;
      FrFrame *lockFrame() ;
      static bool lockFrames(FrList *locklist) ;
      bool unlockFrame() ;
      static bool unlockFrames(FrList *locklist) ;
      bool isLocked() const ;
      bool emptyFrame() ;
      bool dirtyFrame() ;
      int commitFrame() ;
      int discardFrame() ;
      int deleteFrame() ;
      FrFrame *oldFrame(int generation) ;
      FrFrame *copyFrame(FrSymbol *newframe, bool temporary = false) ;
      bool renameFrame(FrSymbol *newname) ;
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
      void pushFiller(const FrSymbol *slotname,const FrSymbol *facet,
                      const FrObject *filler) ;
      FrObject *popFiller(const FrSymbol *slotname, const FrSymbol *facet) ;
      void replaceFiller(const FrSymbol *slotname, const FrSymbol *facetname,
			 const FrObject *old, const FrObject *newfiller) ;
      void replaceFiller(const FrSymbol *slotname, const FrSymbol *facetname,
			 const FrObject *old, const FrObject *newfiller,
			 FrCompareFunc cmp) ;
      void replaceFacet(const FrSymbol *slotname, const FrSymbol *facet,
			const FrList *newfillers) ;
      void eraseFrame() ;
      void eraseCopyFrame() ;
      void eraseSlot(const char *slotname) ;
      void eraseSlot(const FrSymbol *slotname) ;
      void eraseFacet(const FrSymbol *slotname,const FrSymbol *facetname) ;
      void eraseFiller(const FrSymbol *slotname,const FrSymbol *facetname,
		       const FrObject *filler) ;
      void eraseFiller(const FrSymbol *slotname,const FrSymbol *facetname,
		       const FrObject *filler,FrCompareFunc cmp) ;
      void eraseSem(const FrSymbol *slotname, const FrObject *filler) ;
      void eraseSem(const FrSymbol *slotname, const FrObject *filler,
			      FrCompareFunc cmp) ;
      void eraseValue(const FrSymbol *slotname, const FrObject *filler) ;
      void eraseValue(const FrSymbol *slotname, const FrObject *filler,
			      FrCompareFunc cmp) ;
//      const FrList *getImmedFillers(const FrSymbol *slotname,
//				     const FrSymbol *facet) const ;
      const FrList *getFillers(const FrSymbol *slotname,
				const FrSymbol *facet, bool inherit = true) ;
      FrObject *firstFiller(const FrSymbol *slotname,const FrSymbol *facet,
			      bool inherit = true) ;
      const FrList *getValues(const FrSymbol *slotname,bool inherit = true) ;
      FrObject *getValue(const FrSymbol *slotname,bool inherit=true) ;
      const FrList *getSem(const FrSymbol *slotname,bool inherit = true) ;
      bool isA_p(FrSymbol *poss_parent) ;
      bool partOf_p(FrSymbol *poss_container) ;
      FrList *collectSlots(FrInheritanceType inherit,FrList *allslots=0,
			    bool include_names = false) ;
      void inheritAll() ;
      void inheritAll(FrInheritanceType inherit) ;
      FrList *allSlots() const ;
      FrList *slotFacets(const FrSymbol *slotname) const ;
      bool doSlots(bool (*func)(const FrFrame *frame,
				const FrSymbol *slot, va_list args),
		   va_list args) ;
      bool doFacets(const FrSymbol *slotname,
		    bool (*func)(const FrFrame *frame,
				 const FrSymbol *slot,
				 const FrSymbol *facet, va_list args),
		    va_list args) ;
      bool doAllFacets(bool (*func)(const FrFrame *frame,
				    const FrSymbol *slot,
				    const FrSymbol *facet, va_list args),
		       va_list args) ;
   //overloaded operators
      int operator == (const char *symname) const
         { return (FrSymbol *)this == findSymbol(symname) ; }
      int operator != (const char *symname) const
         { return (FrSymbol *)this != findSymbol(symname) ; }
      operator char* () const { return (char*)symbolName() ; }
   //friends
      friend class FrSymbolTable ;
   } ;

//----------------------------------------------------------------------
// non-member functions related to class FrSymbol

FrSymbol *read_Symbol(istream &input) ;
FrSymbol *string_to_Symbol(const char *&input) ;
bool verify_Symbol(const char *&input, bool strict = false) ;
FrSymbol *makeSymbol(const FrSymbol *sym) ; // copy from another symbol table
FrSymbol *gensym(const char *basename = 0, const char *suffix = 0) ;
FrSymbol *gensym(const FrSymbol *basename) ;

void define_relation(const char *relname,const char *invname) ;
void undefine_relation(const char *relname) ;

/**********************************************************************/
/*	Optional Demon support					      */
/**********************************************************************/

#ifdef FrDEMONS
inline bool add_demon(FrSymbol *sym, FrDemonType type,
			FrDemonFunc *func,va_list args)
   { return (sym) ? sym->addDemon(type,func,args) : false ; }
inline bool remove_demon(FrSymbol *sym, FrDemonType type, FrDemonFunc *func)
   { return (sym) ? sym->removeDemon(type,func) : false ; }
#endif /* FrDEMONS */

/**********************************************************************/
/*	 overloaded operators (non-member functions)		      */
/**********************************************************************/

inline istream &operator >> (istream &input,FrSymbol *&obj)
{
   FramepaC_bgproc() ;
   obj = read_Symbol(input) ;
   return input ;
}

#endif /* !__FRSYMBOL_H_INCLUDED */

// end of file frsymbol.h //
