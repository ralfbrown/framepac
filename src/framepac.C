/************************************************************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File FramepaC.cpp							*/
/*  LastEdit: 08nov2015	   						*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,1998,2002,2006,2009,2013,2015	*/
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

#include <stdlib.h>
#ifdef _MSC_VER
#include <new.h>
#endif /* _MSC_VER */
#include "frpasswd.h"
#include "frpcglbl.h"
#include "frserver.h"

/**********************************************************************/
/**********************************************************************/

extern bool read_as_VFrame ;
extern void (*FrShutdown)() ;

void (*FramepaC_shutdown_peermode_func)() = nullptr ;

/**********************************************************************/
/*    Global variables local to this module			      */
/**********************************************************************/

static bool Initialized = false ;

static FrShutdownHookFunc *FramepaC_shutdown_hook ;

// work around compiler problem in OpenWatcom 1.4
#if defined(__WATCOMC__) && __WATCOMC__ > 1100
extern "C" _WPRTLINK int _compiled_under_generic ;
_WPRTLINK int _compiled_under_generic = 1 ;
#endif

/**********************************************************************/
/*	Generic Object Comparison Functions			      */
/**********************************************************************/

bool eql(const FrObject *obj1,const FrObject *obj2)
{
   if (obj1 == obj2)		// two objects are eql if their pointers
      return true ;		//  are equal
   if (obj1 && obj1->numberp())
      return obj1->equal(obj2) ;
   else
      return false ;		// eql only checks numbers for equality

}

//----------------------------------------------------------------------

bool equal(const FrObject *obj1,const FrObject *obj2)
{
   return equal_inline(obj1,obj2) ;
}

//----------------------------------------------------------------------
// perform a case-insensitive equality comparison between two FrStrings,
//   two FrSymbols, or between FrString and FrSymbol (for other type
//   combinations, fall back to default equality comparison)

bool equal_casefold(const FrObject *obj1,const FrObject *obj2,
		      const unsigned char *charmap)
{
   if (obj1 == obj2)
      return true ;
   else if (!obj1 || !obj2)	 // can't be equal if only one is NULL
      return false ;
   else
      {
      const char *str1 = obj1->printableName() ;
      const char *str2 = obj2->printableName() ;
      if (str1 && str2)
	 return Fr_stricmp(str1,str2,charmap) == 0 ;
      else if (str1 || str2)
	 return false ;
      else if (obj1->consp() && obj2->consp())
	 {
	 const FrList *l1 = (FrList*)obj1 ;
	 const FrList *l2 = (FrList*)obj2 ;
	 while (l1 && l2)
	    {
	    if (!equal_casefold(l1->first(),l2->first(),charmap))
	       return false ;
	    l1 = l1->rest() ;
	    l2 = l2->rest() ;
	    }
	 return (l1 == 0 && l2 == 0) ;
	 }
      }
   return equal_inline(obj1,obj2) ;
}

/**********************************************************************/
/*	Miscellaneous utility functions				      */
/**********************************************************************/

bool NIL_symbol(const FrObject *object)
{
   if (object && object->symbolp() &&
       strcmp(dynamic_cast<const FrSymbol*>(object)->symbolName(),"NIL") == 0)
      return true ;
   else
      return false ;
}

/**********************************************************************/
/*	 initialization functions				      */
/**********************************************************************/

static void shutdown_FramepaC()
{
   if (Initialized)
      {
      Initialized = false ;
      if (FramepaC_shutdown_hook)
	 FramepaC_shutdown_hook(1) ;
      if (FramepaC_shutdown_all_VFrames)
	 {
	 FramepaC_shutdown_all_VFrames() ;
	 FramepaC_shutdown_all_VFrames = nullptr ;
	 }
      if (FramepaC_destroy_all_symbol_tables)
	 {
	 FramepaC_destroy_all_symbol_tables() ;
	 FramepaC_destroy_all_symbol_tables = nullptr ;
	 }
      if (FramepaC_shutdown_peermode_func)
	 {
	 FramepaC_shutdown_peermode_func() ;
	 FramepaC_shutdown_peermode_func = nullptr ;
	 }
      if (FramepaC_shutdown_hook)
	 {
	 FramepaC_shutdown_hook(2) ;
	 FramepaC_shutdown_hook = nullptr ;
	 }
      delete FramepaC_readtable ;
      FramepaC_readtable = nullptr ;
      if (FramepaC_clear_userinfo_dir)
	 FramepaC_clear_userinfo_dir() ;
      void FramepaC_shutdown_symboltables() ;
      FramepaC_shutdown_symboltables() ;
#ifndef NVALGRIND
      for (FrReader *r = FrReader::firstReader() ; r ; r = r->nextReader())
	 {
	 r->FramepaC_shutdown() ;
	 }
      void FramepaC_gc() ;
      FramepaC_gc() ;
#endif /* !NVALGRIND */
      }
   return ;
}

//----------------------------------------------------------------------

#ifdef _MSC_VER
void __cdecl shutdown_FramepaC_VC()
{
   shutdown_FramepaC() ;
}
#endif /* _MSC_VER */

//----------------------------------------------------------------------

#if defined(__SUNOS__) && !defined(__SOLARIS__)
// SunOS 4.x has on_exit() which passes different args to the shutdown
// function from the standard atexit(); Solaris has atexit() but no on_exit().
static void shutdown_FramepaC_sun(int status, caddr_t arg)
{
   (void)status ; (void)arg ;
   FrShutdown() ;
}
#endif /* __SUNOS__ && !__SOLARIS__ */

//----------------------------------------------------------------------

void initialize_FramepaC(int max_symbols)
{
   if (!Initialized)
      {
      FrShutdown = shutdown_FramepaC ;
      FrSymbolTable::current()->expandTo(max_symbols) ;
      void FramepaC_init_inheritance_setting_func() ;
      FramepaC_init_inheritance_setting_func() ;
      initialize_FrReadTable() ;
#if defined(__SUNOS__) && !defined(__SOLARIS__)
      // SunOS 4.x has on_exit() instead of the standard atexit(); Solaris
      // has atexit() but no on_exit()....
      on_exit(shutdown_FramepaC_sun,(caddr_t) 0) ;
#elif defined(_MSC_VER)
      atexit(shutdown_FramepaC_VC) ;
      _set_new_mode(1) ;		// do GC if malloc() fails
#else
      atexit(shutdown_FramepaC) ;
#endif /* __SUNOS__ && !__SOLARIS__ */
      Initialized = true ;
      }
   return ;
}

//----------------------------------------------------------------------

void set_FramepaC_shutdown_hook(FrShutdownHookFunc *hook)
{
   FramepaC_shutdown_hook = hook ;
}

// end of file FramepaC.cpp //
