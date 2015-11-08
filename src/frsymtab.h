/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frsymtab.h	       class FrSymbolTable			*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,1998,2000,2001,2002,2006,2009,	*/
/*		2013,2015 Ralf Brown/Carnegie Mellon University		*/
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

#ifndef __FRSYMTAB_H_INCLUDED
#define __FRSYMTAB_H_INCLUDED

#ifndef __FRHASH_H_INCLUDED
#include "frhash.h"
#endif

#if defined(__GNUC__)
#  pragma interface
#endif

/************************************************************************/
/*	Manifest Constants						*/
/************************************************************************/

// number of differently-size allocation lists for FrSymbol objects
#define FrSYMTAB_SIZE_BINS	6
// difference in name lengths between the different bins
#define FrSYMTAB_SIZE_GRAN	8
// anything higher than the last bin can store goes onto a general list
 
#define FramepaC_NUM_STD_SYMBOLS 16

/************************************************************************/
/*	Types							      	*/
/************************************************************************/

enum VFrameNotifyType
   {
   VFNot_CREATE,
   VFNot_DELETE,
   VFNot_UPDATE,
   VFNot_LOCK,
   VFNot_UNLOCK,
   VFNot_PROXYADD,
   VFNot_PROXYDEL
   } ;

typedef void FrSymTabDeleteFunc(FrSymbolTable *) ;

typedef int DoAllSymtabsFunc(FrSymbolTable *, va_list) ;

typedef void VFrameNotifyFunc(VFrameNotifyType type,const FrSymbol *frame) ;
typedef VFrameNotifyFunc *VFrameNotifyPtr ;

typedef bool VFrameProxyFunc(VFrameNotifyType type, const FrSymbol *frame,
			     const FrSymbol *slot, const FrSymbol *facet,
			     const FrObject *filler) ;
typedef VFrameProxyFunc *VFrameProxyPtr ;

typedef void VFrameShutdownFunc(const char *server, int seconds) ;
typedef VFrameShutdownFunc *VFrameShutdownPtr ;

/**********************************************************************/
/*    Declarations for class FrSymbolTable		      	      */
/**********************************************************************/

class FrSymbolTable : public FrSymbolTableX
   {
   private:
      FrSymbolTable *m_next, *m_prev ;
      char          *m_name ;		// user-supplied name to identify table
      FrAllocator   *m_allocators[FrSYMTAB_SIZE_BINS] ;
      FrMemFooter   *palloc_list ; 	// buffers for symbol name strings
      FrMemFooter   *palloc_pool ;	// pre-alloc bufs for symbol names
      FrSymTabDeleteFunc *delete_hook ; // func to call on symtab deletion
      FrCriticalSection m_palloc_lock ;
      FrMutex        m_palloc_mutex ;
   public:
      FrSymbol      *std_symbols[FramepaC_NUM_STD_SYMBOLS] ;
      VFrameInfo    *bstore_info ;
      _FrFrameInheritanceFunc *inheritance_function ;
   private:
   public:
#ifdef FrLRU_DISCARD
      uint32_t LRUclock ;
#endif /* FrLRU_DISCARD */
      FrInheritanceType inheritance_type ;
   private: // member functions
      void init_palloc() ;
      void *palloc(unsigned int numbytes) ;
      void free_palloc() ;
      const FrSymbol *allocate_symbol(const char *name, size_t namelen) ;
   public: // member functions which should not be called by applications
      int countFrames() const ;
#ifdef FrLRU_DISCARD
      void adjust_LRUclock() ;
      uint32_t currentLRUclock() const { return LRUclock ; }
      uint32_t oldestFrameClock() const ;
#endif /* FrLRU_DISCARD */
   public: // member functions
      void *operator new(size_t size) { return FrMalloc(size) ; }
      void operator delete(void *obj) { FrFree(obj) ; }
      FrSymbolTable(unsigned int max_symbols = 0) ;
      ~FrSymbolTable() ;
      void setName(const char *nam) ;
      FrSymbolTable *select() ;   // make this the current symbol table
      static FrSymbol *add(const char *name) ;  // always adds to current s.t.
      static FrSymbol *add(const char *name,FrSymbolTable *symtab) ;
#ifdef FrSYMBOL_VALUE
      static FrSymbol *add(const char *name,const FrObject *value) ;
      static FrSymbol *add(const char *name,FrSymbolTable *symtab,
			    const FrObject *value) ;
#endif
      FrSymbol *gensym(const char *basename = 0, const char *suffix = 0) ;
      FrSymbol *lookup(const char *name) const ;
      void setNotify(VFrameNotifyType,VFrameNotifyFunc *) ;
      VFrameNotifyPtr getNotify(VFrameNotifyType) const ;
      void setProxy(VFrameNotifyType,VFrameProxyFunc *) ;
      VFrameProxyPtr getProxy(VFrameNotifyType) const ;
      void setShutdown(VFrameShutdownFunc *) ;
      VFrameShutdownPtr getShutdown() const ;
      void setDeleteHook(FrSymTabDeleteFunc *delhook) ;
      FrSymTabDeleteFunc *getDeleteHook() const { return delete_hook ; }
      bool isReadOnly() const ;
      bool __FrCDECL iterateFrame(FrIteratorFunc func, ...) const ;
      bool iterateFrameVA(FrIteratorFunc func, va_list) const ;
      FrList *listRelations() const ;

      // debugging support
      bool checkSymbols() const ;	// verify that all symbols are valid
   //static member functions
      static FrSymbolTable *current() ;  // get currently active symbol table
      static FrSymbolTable *selectDefault() ;
   //friend functions
      friend const FrSymbol *Fr_allocate_symbol(FrSymbolTable *, const char *name, size_t namelen) ;
      friend void __FrCDECL do_all_symtabs(DoAllSymtabsFunc *func,...) ;
      friend FrSymbolTable *select_symbol_table(const char *table_name) ;
      friend void _FramepaC_destroy_all_symbol_tables() ;
      friend FrSymbol *new_FrSymbol(const char *name, size_t len) ;
   } ;

/**********************************************************************/

void name_symbol_table(const char *name) ;
void destroy_symbol_table(FrSymbolTable *symtab) ;
FrSymbolTable *select_symbol_table(const char *table_name) ;

#endif /* !__FRSYMTAB_H_INCLUDED */

// end of frsymtab.h //
