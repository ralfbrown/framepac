/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frcommon.h	       common definitions			*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,1998,1999,2000,2001,2002,2003,	*/
/*		2004,2005,2006,2007,2008,2009,2013,2015			*/
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

#ifndef __FRCOMMON_H_INCLUDED
#define __FRCOMMON_H_INCLUDED

#ifndef __FRCONFIG_H_INCLUDED
#include "frconfig.h"
#endif

#ifndef __FRASSERT_H_INCLUDED
#include "frassert.h"
#endif

#ifdef FrSTRICT_CPLUSPLUS
#  include <cstddef>
#  include <cstdarg>
#else
#  include <stdarg.h>
#  include <stddef.h>
#endif /* FrSTRICT_CPLUSPLUS */

/**********************************************************************/
/*    Global constants						      */
/**********************************************************************/

#define FramepaC_Version 201
#define FramepaC_Version_string "2.01"

/**********************************************************************/
/*    Forward class and struct declarations			      */
/**********************************************************************/

// standard classes (to avoid #include'ing files)
#if __GNUC__ >= 3
#  include <iostream>
using namespace std ;
#else
#include <iostream.h>
class istream ;
class ostream ;
#endif /* __GNUC__ */

// public classes
class FrObject ;
class FrAtom ;
class FrFrame ;
class FrSymbol ;
class _FrChar ;
class FrString ;
class FrStruct ;
class FrCons ;
class FrList ;
class FrNumber ;
class FrInteger ;
class FrFloat ;
class VFrame ;
class FrBitVector ;
class FrHashEntry ;
class FrReader ;
class FrSymbolTable ;
class FrWidget ;
class FrISockStream ;
class FrOSockStream ;
class FrSockStream ;

// semi-private
class FrSlot ;
class FrFacet ;
struct FrMemFooter ;
class VFrameInfo ;

/**********************************************************************/
/**********************************************************************/

typedef unsigned char FrBYTE ;

enum FrInheritanceType
   {
   NoInherit,	  // no inheritance
   InheritSimple, // only look at first filler of IS-A (or INSTANCE-OF) slots
   InheritDFS,	  // depth-first search on IS-A
   InheritBFS,	  // breadth-first search on IS-A
   InheritPartDFS,// depth-first search on PART-OF
   InheritPartBFS,// breadth-first search on PART-OF
   InheritLocalDFS,  // follow slot's INHERITS facet before doing DFS
   InheritLocalBFS,  // follow slot's INHERITS facet before doing BFS
   InheritUser	  // user-provided function
   } ;

typedef FrList *FrInheritanceFunc(const FrSymbol *frame,
				   const FrSymbol *slot,
				   const FrSymbol *facet) ;

// internal type needed in FrSymbolTable
typedef const FrList *_FrFrameInheritanceFunc(FrFrame *frame,
					       const FrSymbol *slot,
					       const FrSymbol *facet) ;

#ifdef __SOLARIS__
 typedef uint16_t FrChar16 ;
#elif __886__
 typedef uint16_t FrChar16 ;
#else
 typedef wchar_t FrChar16 ;
#endif /* __SOLARIS__ */

/**********************************************************************/
/*	 useful macros						      */
/**********************************************************************/

#ifndef lengthof
#  define lengthof(x) (sizeof(x)/sizeof((x)[0]))
#endif

#ifndef Fr_offsetof
# define Fr_offsetof(var,fld) (unsigned)(((char*)&(var).fld) - ((char*)&(var)))
#endif

/**********************************************************************/
/*       enumerated types     					      */
/**********************************************************************/

#ifndef FrANSI_BOOL
enum _bool { false, true } ;
typedef enum _bool bool ;
#endif /* !FrANSI_BOOL */

// declarations for backwards compatibility with versions prior to 1.90
#define FrBool bool
#define True   true
#define False  false

enum FrObjectType
   {
   OT_NIL,
   OT_FrObject,
   OT_Frame,
   OT_VFrame,
   OT_VFrameInfo,
   OT_FrAtom,
   OT_FrCons,
   OT_FrNumber,
   OT_FrString,
   OT_FrSymbol,
   OT_FrList,
   OT_FrStruct,
   OT_FrInteger,
   OT_FrInteger64,
   OT_FrFloat,
   OT_FrQueue,
   OT_FrStack,
   OT_FrArray,
   OT_FrSparseArray,
   OT_FrHashTable,
   OT_FrHashEntry,
   OT_FrStream,
   OT_FrISockStream,
   OT_FrOSockStream,
   OT_FrSockStream,
   OT_FrBitVector,
   OT_FrWidget,
   OT_FrWSeparator,
   OT_FrWFrame,
   OT_FrWArrow, OT_FrWArrowG,
   OT_FrWLabel,
   OT_FrWRowColumn,
   OT_FrWList,
   OT_FrWOptionMenu,
   OT_FrWPopupMenu,
   OT_FrWPulldownMenu,
   OT_FrWMessagePopup,
   OT_FrWVerifyPopup,
   OT_FrWPromptPopup,
   OT_FrWDialogPopup,
   OT_FrWPushButton,
   OT_FrWPushButtonG,
   OT_FrWToggleButton,
   OT_FrWCascadeButton,
   OT_FrWForm,
   OT_FrWSlider,
   OT_FrWButtonBar,
   OT_FrWText,
   OT_FrWShadowText,
   OT_FrWFramePrompt,
   OT_FrWFrameCompleter,
   OT_FrWTextWindow,
   OT_FrWMainWindow,
   OT_FrWScrollWindow,
   OT_FrWScrollBar,
   OT_FrWRadioBox,
   OT_FrWSelectionBox,		
   OT_FrWProgressIndicator,
   OT_FrWProgressPopup,
   OT_FrBoundedPriQueue,

   OT_List = OT_FrList,
   OT_Cons = OT_FrCons,
   OT_Symbol = OT_FrSymbol,
   OT_Number = OT_FrNumber,
   OT_Float = OT_FrFloat,
   OT_Integer = OT_FrInteger
   } ;

/**********************************************************************/
/*	 Global Variables					      */
/**********************************************************************/

extern int Fr_errno ;

// should FramepaC complain when using a virtual function on an object type
// for which the method is meaningless?  (default=True)
extern bool FramepaC_typesafe_virtual ;

/**********************************************************************/
/*	 Portability Functions					      */
/**********************************************************************/

#ifdef FrNEED_ULTOA
char *ultoa(unsigned long value, char *buffer, int radix) ;
#endif /* FrNEED_ULTOA */

/**********************************************************************/
/**********************************************************************/

typedef void (*FramepaC_bgproc_funcptr)() ;
extern FramepaC_bgproc_funcptr FramepaC_bgproc_func ;
#ifdef FrSERVER
inline void FramepaC_bgproc()
   { if (FramepaC_bgproc_func) FramepaC_bgproc_func() ; }
inline void set_FramepaC_bgproc_func(FramepaC_bgproc_funcptr func)
   { FramepaC_bgproc_func = func ; }
inline FramepaC_bgproc_funcptr get_FramepaC_bgproc_func()
   { return FramepaC_bgproc_func ; }
#else
#  define FramepaC_bgproc()
#  define set_FramepaC_bgproc_func(x)
inline FramepaC_bgproc_funcptr get_FramepaC_bgproc_func() { return 0 ; }
#endif

typedef void FrShutdownHookFunc(int before_after) ;

void set_FramepaC_shutdown_hook(FrShutdownHookFunc *func) ;

/**********************************************************************/
/*	C++11 type-safety templates -- use LHS specialization	      */
/**********************************************************************/

// inspired by C Bloom: range-checked cast from a larger int type to a smaller one
template <typename to_t, typename from_t>
to_t resize_int(const from_t &from)
{
   to_t to = static_cast<to_t>(from) ;
   assert( to == from ) ;
   return to ;
}

// from C Bloom: cast from void* to other* that won't cause silent
//   breakage if the data type of the source object changes to non-pointer
template <typename T>
T *CastVoid(void *ptr)
{
   return (T*)ptr ;
}

/**********************************************************************/
/*	type-safety templates					      */
/**********************************************************************/

// inspired by C Bloom: cast pointer (and pointer only) to void*
template <typename T>
void *VoidPtr(T *ptr)
{
   return (void*)ptr ;
}

#define trunc2char(i) resize_int<char>(i)

/**********************************************************************/
/**********************************************************************/

bool eql(const FrObject *,const FrObject *) ;
bool equal(const FrObject *,const FrObject *) ;
bool equal_casefold(const FrObject *,const FrObject *,
		      const unsigned char *charmap) ;

/**********************************************************************/
/**********************************************************************/

// if the extra indexes are enabled, we need frame IDs
#ifdef FrEXTRA_INDEXES
#  define FrFRAME_ID
#endif

#endif /* !__FRCOMMON_H_INCLUDED */

// end of file frcommon.h //
