/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File vframe.cpp	 "virtual memory" frames for FramepaC           */
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,2000,2001,2002,2009,2011,2013	*/
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "frpcglbl.h"
#include "vfinfo.h"
#include "mikro_db.h"
#include "frfinddb.h"
#include "frutil.h"

/************************************************************************/
/*    Global variables imported from other modules			*/
/************************************************************************/

extern bool omit_inverse_links ;

/************************************************************************/
/*    Procedural interface to VFrame class				*/
/************************************************************************/

//----------------------------------------------------------------------

#if 0
bool __FrCDECL do_all_facets(FrSymbol *frame,
			       bool (*func)(const FrFrame *frame,
					      const FrSymbol *slot,
					      const FrSymbol *facet,
					      va_list args),
			       ...)
{
   va_list args ;
   bool result ;
   FrFrame *fr = find_vframe_inline(frame) ;

   va_start(args,func) ;
   result = fr ? fr->doAllFacets(func,args) : false ;
   va_end(args) ;
   return result ;
}
#endif /* 0 */

//----------------------------------------------------------------------

VFrame *load_frame(const char *framerep,bool locked)
{
   VFrame *fr ;
   bool old_omit = omit_inverse_links ;
   const char *rep ;
   FrSymbol *name ;

   if (framerep[0] != '[')
      return 0 ;		       // invalid framerep....
   rep = framerep+1 ;
   name = string_to_Symbol(rep) ;
   // create the frame explicitly before attempting to convert, or
   // we'll wind up recursively calling ourselves to retrieve the
   // frame being converted as a result of retrieving the frame....
   fr = new VFrame(name) ;
   omit_inverse_links = true ;
   string_to_Frame(framerep) ;	       //   and fill it in from string
   omit_inverse_links = old_omit ;
   fr->markDirty(false) ;	       // no changes since being read in
   fr->setLock(locked) ;
   return fr ;
}

//----------------------------------------------------------------------
//    Helper function for commit_all_frames()

static void commit_a_frame(FrFrame *frame, va_list args)
{
   FrVarArg(int *,committed) ;
   if (frame->commitFrame() != 0)
      {
      Fr_errno = FE_COMMITFAILED ;
      *committed = -1 ;
      }
   return ;
}

//----------------------------------------------------------------------

int commit_all_frames()
{
   int all_frames_committed = 0 ;

   FrSymbolTable::current()->iterateFrame((FrIteratorFunc)commit_a_frame,
				   &all_frames_committed) ;
   return all_frames_committed ;
}

/************************************************************************/
/*    Non-member functions for class VFrameInfo			      	*/
/************************************************************************/

//----------------------------------------------------------------------

bool get_DB_user_data(DBUserData *user_data)
{
   if (VFrame_Info)
      return VFrame_Info->getDBUserData(user_data) ;
   else
      return false ;
}

//----------------------------------------------------------------------

bool set_DB_user_data(DBUserData *user_data)
{
   if (VFrame_Info)
      return VFrame_Info->setDBUserData(user_data) ;
   else
      return false ;
}

//----------------------------------------------------------------------

int prefetch_frames(FrList *frames)
{
   if (VFrame_Info)
      return VFrame_Info->prefetchFrames(frames) ;
   else
      return 0 ;
}

/************************************************************************/
/* 	Helper functions						*/
/************************************************************************/

//----------------------------------------------------------------------

long int frameID_for_symbol(const FrSymbol *sym)
{
#ifdef FrFRAME_ID
   if (VFrame_Info)
      return VFrame_Info->lookupID(sym) ;
#else
   (void)sym ;
#endif /* !FrFRAME_ID */
   return NO_FRAME_ID ;
}

//----------------------------------------------------------------------

FrSymbol *symbol_for_frameID(long int ID)
{
#ifdef FrFRAME_ID
   if (VFrame_Info)
      return VFrame_Info->lookupSym(ID) ;
#else
   (void)ID ;
#endif /* !FrFRAME_ID */
   return 0 ;
}

//----------------------------------------------------------------------

FrList *available_databases(FrSymbolTable *symtab)
{
   FrSymbolTable *orig_symtab ;
   if (symtab)
      orig_symtab = symtab->select() ;
   else
      orig_symtab = 0 ;
   FrList *dbs ;
   if (VFrame_Info)
      dbs = VFrame_Info->availableDatabases() ;
   else
      dbs = find_databases(0,0) ;
   if (symtab)
      orig_symtab->select() ;
   return dbs ;
}

//----------------------------------------------------------------------

FrList *available_databases(const char *servername)
{
   (void)servername ;
   return 0 ; // for now
}

//----------------------------------------------------------------------

static void collect_prefix_match_aux(FrFrame *frame, va_list args)
{
   FrVarArg(char *,prefix) ;
   FrVarArg(int,prefix_len) ;
   FrVarArg(FrList **,matches) ;
   FrSymbol *name = frame->frameName() ;
   if (name &&
       (prefix_len == 0 || memcmp(name->symbolName(),prefix,prefix_len) == 0))
      pushlist(name,*matches) ;
   return ;
}

//----------------------------------------------------------------------
// return the list of all frames with names starting with the indicated
// string, and optionally return the longest prefix which is the same
// for all matching frames

FrList *collect_prefix_matching_frames(const char *prefix,
					char *longest_prefix,
					int longprefix_buflen)
{
   FrList *matches ;

   if (VFrame_Info)
      matches = VFrame_Info->prefixMatches(prefix) ;
   else
      {
      matches = 0 ;
      if (!prefix)
	 prefix = "" ;
      int prefix_len = strlen(prefix) ;
      FrSymbolTable::current()->iterateFrame((FrIteratorFunc)collect_prefix_match_aux,
				      prefix, prefix_len,&matches) ;
      }
   // now check for the longest common prefix to all found frame names
   if (longest_prefix)
      {
      FrList *m = matches ;

      if (m)
	 {
	 strncpy(longest_prefix,((FrSymbol*)m->first())->symbolName(),
		 longprefix_buflen);
	 longest_prefix[longprefix_buflen-1] = '\0' ;
	 m = m->rest() ;
	 while (m)
	    {
	    const char *name = ((FrSymbol*)m->first())->symbolName() ;
	    int len ;
	    for (len = 0 ;
	         len < longprefix_buflen-1 && name[len]==longest_prefix[len] ;
		 len++)
	       ;
	    longest_prefix[len] = '\0' ; // truncate to new longest length
	    m = m->rest() ;
	    }
	 }
      else
	 *longest_prefix = '\0' ;
      }
   return matches ;
}

//----------------------------------------------------------------------
//

static void complete_name_aux(FrFrame *frame, va_list args)
{
   FrVarArg(char *,prefix) ;
   FrVarArg(int,prefix_len) ;
   FrVarArg(char **,matchptr) ;
   FrSymbol *name = frame->frameName() ;
   const char *strname = name ? name->symbolName() : "" ;

   if (prefix_len == 0 || memcmp(strname,prefix,prefix_len) == 0)
      {   // we have a matching frame, so adjust maximal prefix
      if (*matchptr)
         {
	 int len ;
         for (len = 0 ;
              *matchptr[len] && strname[len] == *matchptr[len] ; len++)
            ;
         *matchptr[len] = '\0' ;
         }
      else
         {
         *matchptr = FrDupString(strname) ;
	 }
      }
   return ;
}

//----------------------------------------------------------------------
// return a string indicating the maximal completion possible for the
// given frame name prefix

char *complete_frame_name(const char *prefix)
{
   if (VFrame_Info)
      return VFrame_Info->completionFor(prefix) ;
   else
      {
      char *match = 0 ;
      FrSymbolTable::current()->iterateFrame((FrIteratorFunc)complete_name_aux,
					     prefix,&match) ;
      return match ;
      }
}

/**********************************************************************/
/*    Initialization functions					      */
/**********************************************************************/

FrSymbolTable *initialize_VFrames_memory(int symtabsize)
{
   FrSymbolTable *symtab = new FrSymbolTable(symtabsize) ;

   if (symtab)
      {
      symtab->select() ;
      VFrame_Info = 0 ;
      // force all changes to the symbol table to be updated in both the active
      // copy and the copy addressed via 'symtab'
      FrSymbolTable::selectDefault() ;
      symtab->select() ;
//!!!
      }
   return symtab ;
}


// end of file vframe.cpp //
