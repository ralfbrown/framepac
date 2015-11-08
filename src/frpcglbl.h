/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frpcglbl.h	globals shared between various FramepaC modules */
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,1998,1999,2000,2001,2002,2009,	*/
/*		2013 Ralf Brown/Carnegie Mellon University		*/
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

#ifndef __FRPCGLBL_H_INCLUDED
#define __FRPCGLBL_H_INCLUDED

#ifndef __FRCTYPE_H_INCLUDED
#include "frctype.h"
#endif

#ifndef __VFRAME_H_INCLUDED
#include "vframe.h"
// includes frcommon.h
#endif

#ifndef __FRAMERR_H_INCLUDED
#include "framerr.h"
#endif

#ifndef __VFINFO_H_INCLUDED
#include "vfinfo.h"
#endif

/************************************************************************/
/*    Manifest constants for class FrSymbolTable			*/
/************************************************************************/

#define DEFAULT_SYMBOLTABLE_SIZE 1023
#define SYMBOLTABLE_MIN_INCREMENT 48
#define MAX_SYMBOLTABLE_SIZE (UINT_MAX / sizeof (FrSymbol*))

/************************************************************************/
/*    Identifier remapping to avoid name clashes with end-user code	*/
/************************************************************************/

#define ActiveSymbolTable	 FramepaC_ActiveSymbolTable
#define omit_inverse_links	 FramepaC_omit_inverse_links

#define inheritFillersNone	 FramepaC_inheritFillersNone
#define inheritFillersSimple	 FramepaC_inheritFillersSimple
#define inheritFillersDFS        FramepaC_inheritFillersDFS
#define inheritFillersBFS	 FramepaC_inheritFillersBFS
#define inheritFillersPartOfDFS  FramepaC_inheritFillersPartOfDFS
#define inheritFillersPartOfBFS  FramepaC_inheritFillersPartOfBFS
#define inheritFillersLocalDFS   FramepaC_inheritFillersLocalDFS
#define inheritFillersLocalBFS   FramepaC_inheritFillersLocalBFS
#define inheritFillersUser       FramepaC_inheritFillersUser

#define read_widechar_strings	 FramepaC_read_widechar_strings
#define read_extended_strings    FramepaC_read_extended_strings

#define allocate_block	         FramepaC_allocate_block

/************************************************************************/
/*    shorthands for the most-commonly used symbols			*/
/************************************************************************/

#define symbolNIL        (ActiveSymbolTable->std_symbols[0])
#define symbolT		 (ActiveSymbolTable->std_symbols[1])
#define symbolQUESMARK   (ActiveSymbolTable->std_symbols[2])
#define symbolISA        (ActiveSymbolTable->std_symbols[3])
#define symbolINSTANCEOF (ActiveSymbolTable->std_symbols[4])
#define symbolSUBCLASSES (ActiveSymbolTable->std_symbols[5])
#define symbolINSTANCES  (ActiveSymbolTable->std_symbols[6])
#define symbolHASPARTS   (ActiveSymbolTable->std_symbols[7])
#define symbolPARTOF     (ActiveSymbolTable->std_symbols[8])
#define symbolVALUE      (ActiveSymbolTable->std_symbols[9])
#define symbolSEM        (ActiveSymbolTable->std_symbols[10])
#define symbolCOMMON     (ActiveSymbolTable->std_symbols[11])
#define symbolEOF	 (ActiveSymbolTable->std_symbols[12])
#define symbolINHERITS   (ActiveSymbolTable->std_symbols[13])
#define symbolPERIOD     (ActiveSymbolTable->std_symbols[14])
#define FramepaC_NUM_STD_SYMBOLS 16

/************************************************************************/
/************************************************************************/

#define VFrame_Info (ActiveSymbolTable->bstore_info)

/************************************************************************/
/*    Private class FrFacet						*/
/************************************************************************/

class FrFacet
   {
   private:
      static FrAllocator allocator ;
   public:
      FrFacet *next ;
      FrSymbol *facet_name ;
      FrList *fillers ;
   // member functions
      FrFacet() { next = 0 ; facet_name = 0 ; fillers = 0 ; }
      FrFacet(const FrSymbol *name)
	    { next=0 ; facet_name = (FrSymbol *)name ; fillers=0 ; }
      void *operator new(size_t) { return allocator.allocate() ; }
      void operator delete(void *s) { allocator.release(s) ; }
    } ;

/**********************************************************************/
/*    Manifest constants for frames				      */
/**********************************************************************/

// the following must match the definition of predefined_slot_names[] in
// frframev.cpp!
#define ISA_slot 0
#define SUBCLASSES_slot 1
#define INSTANCEOF_slot 2
#define PARTOF_slot 3

/************************************************************************/
/*	Support for Demons						*/
/************************************************************************/

#ifdef FrDEMONS

struct FrDemonList
   {
   FrDemonList *next ;
   FrDemonFunc *demon ;
   va_list args ;
   void *operator new(size_t size) { return FrMalloc(size) ; }
   void operator delete(void *obj) { FrFree(obj) ; }
   } ;

#ifdef __GNUC__
static
#endif
void FramepaC_apply_demons(const FrDemonList *, const FrSymbol *fr,
			  const FrSymbol *slot, const FrSymbol *facet,
			  const FrObject *value) ;

#define CALL_DEMON(type,fr,slot,facet,val) \
	if ((slot)->demons() && (slot)->demons()->type) \
	   FramepaC_apply_demons((slot)->demons()->type,fr,slot,facet,val)

#else /* FrDEMONS */
#  define CALL_DEMON(type,fr,slot,facet,val)
#endif /* FrDEMONS */

/************************************************************************/

extern FrSymbolTable *ActiveSymbolTable ;
extern FrMemFooter **FramepaC_symbol_list_ptr ;

extern void (*FramepaC_shutdown_peermode_func)() ;

/************************************************************************/

extern FrInheritanceType FramepaC_inheritance_type ;
extern _FrFrameInheritanceFunc *FramepaC_inheritance_function ;

/************************************************************************/
/*	support for Lisp-style #1= and #1# structure sharing		*/
/************************************************************************/

extern FrList *FramepaC_read_associations ;

/************************************************************************/
/*    Internal functions used across modules				*/
/************************************************************************/

extern class VFrame *(*FramepaC_new_VFrame)(class FrSymbol *name) ;
extern void (*FramepaC_delete_all_frames)() ;
extern void (*FramepaC_shutdown_all_VFrames)() ;
extern void (*FramepaC_destroy_all_symbol_tables)() ;
FrObject *string_to_Frame(const char *&input,const char *infix_digits = 0) ;
FrSymbol *FramepaC_string_to_unquoted_Symbol(const char *&input,
					      const char *) ;
FrSymbol *FramepaC_read_unquoted_Symbol(istream &input, const char *) ;

struct FrMemFooter *FramepaC_allocate_block(const char *errloc) ;

char FrSkipWhitespace(istream &) ;

/************************************************************************/

extern const unsigned char FramepaC_valid_symbol_characters[] ;
#define Fr_issymbolchar(c) (FramepaC_valid_symbol_characters[(unsigned char)c])

//----------------------------------------------------------------------
// functions for updating a hash code built from other hash values

#if defined(__WATCOMC__) && defined(__386__)
extern unsigned long FramepaC_update_ulong_hash(unsigned long hash,
						unsigned long newval) ;
#pragma aux FramepaC_update_ulong_hash = \
	"add   ebx,eax" \
	"ror   ebx,5" \
	parm [ebx][eax] \
	value [ebx] \
	modify exact [ebx] ;

#elif defined(_MSC_VER) && _MSC_VER >= 800

inline unsigned long FramepaC_update_ulong_hash(unsigned long hash,
						unsigned long newval)
{
   _asm {
          mov eax,hash ;
	  add eax,newval ;
	  ror eax,5 ;
	  mov hash,eax ;
        } ;
   return hash ;
}

#elif defined(__GNUC__) && defined(__386__)
inline unsigned long FramepaC_update_ulong_hash(unsigned long hash,
						unsigned long newval)
{
   unsigned long newhash ;
   __asm__("add %2,%1\n\t"
	   "ror $5,%1"
	   : "=g" (newhash) : "0" (hash), "g" (newval) : "cc") ;
   return newhash ;
}

#else // neither Watcom C++ nor Visual C++ nor GCC on x86

inline unsigned long FramepaC_update_ulong_hash(unsigned long hash,
						unsigned long newval)
{
   hash += newval ;
   return ((hash << 26) | ((hash >> 5) & 0x07FFFFFF)) ;
}

#endif /* __WATCOMC__ && __386__ */

//----------------------------------------------------------------------

#ifdef __FRSYMTAB_H_INCLUDED
inline VFrame *find_vframe_inline(const FrSymbol *sym)
{
   register VFrame *fr = (VFrame *)sym->symbolFrame() ;

#ifdef FrLRU_DISCARD
   if (VFrame_Info &&
       (fr || (fr = VFrame_Info->retrieveFrame(sym)) != 0))
      {
      // mark frame as most recently accessed
      fr->setLRUclock(ActiveSymbolTable->LRUclock++) ;
      }
#else
   if (!fr && VFrame_Info)
      fr = VFrame_Info->retrieveFrame(sym) ;
#endif
   return fr ;
}
#endif /* __FRSYMTAB_H_INCLUDED */

//----------------------------------------------------------------------

inline bool equal_inline(const FrObject *obj1,const FrObject *obj2)
{
   if (obj1 == obj2)		// two objects are equal if their pointers
      return True ;		//  are equal
   else if (!obj1 || !obj2)	// if only one pointer is NULL, then the
      return false ;		//  objects can't be equal
   else
      return obj1->equal(obj2) ;
}

//----------------------------------------------------------------------

extern bool FramepaC_read_widechar_strings ;
inline bool FramepaC_read_extended_strings(bool read)
{
   bool old = FramepaC_read_widechar_strings ;
   FramepaC_read_widechar_strings = read ;
   return old ;
}

/************************************************************************/
/*	 Miscellaneous variables shared between modules			*/
/************************************************************************/

extern size_t FramepaC_display_width ;
extern size_t FramepaC_initial_indent ;

extern size_t FramepaC_num_memmaps ;
extern size_t FramepaC_total_memmap_size ;

#endif /* __FRPCGLBL_H_INCLUDED */

// end of file frpcglbl.h //
