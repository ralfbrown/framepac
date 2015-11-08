/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File vfinfo.cpp    -- "virtual memory" backing-store support        */
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,2009,2015				*/
/*	    Ralf Brown/Carnegie Mellon University			*/
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

#include "frhash.h"
#include "vfinfo.h"
#include "frutil.h"

/**********************************************************************/
/**********************************************************************/

static bool prefix_match(const FrSymbol *name, FrObject * /*info*/, va_list args)
{
   //HashEntryVFrame* entry = (HashEntryVFrame*)info ;
   FrVarArg(const char *,prefix) ;
   FrVarArg(size_t,prefixlen) ;
   FrVarArg(FrList**,matches) ;
   if (name)
      {
      if (memcmp(name->symbolName(),prefix,prefixlen) == 0)
	 {
	 pushlist(name,*matches) ;
	 }
      }
   return true ;
}

//----------------------------------------------------------------------

static bool completion_for(const FrSymbol *name, FrObject * /*info*/, va_list args)
{
   //HashEntryVFrame* entry = (HashEntryVFrame*)info ;
   FrVarArg(const char *,prefix) ;
   FrVarArg(size_t,prefixlen) ;
   FrVarArg(char**,match) ;
   if (name)
      {
      const char *namestr = name->symbolName() ;
      if (prefixlen == 0 || memcmp(prefix,namestr,prefixlen) == 0)
	 {
	 if (*match)
	    {
	    size_t i ;
	    for (i = 0 ; (*match)[i] && (*match)[i] == namestr[i] ; ++i)
	       ;
	    (*match)[i] = '\0' ;
	    }
	 else
	    *match = FrDupString(namestr) ;
	 }
      }
   return true ;
}

/**********************************************************************/
/*    Member functions for class VFrameInfo			      */
/**********************************************************************/

FrList *VFrameInfo::prefixMatches(const char *prefix) const
{
   FrList *matches = 0 ;
   if (hash && prefix)
      {
      hash->iterate(prefix_match,prefix,strlen(prefix),&matches) ;
      }
   return matches ;
}

//----------------------------------------------------------------------

char *VFrameInfo::completionFor(const char *prefix) const
{
   char *match = 0 ;
   if (!prefix)
      prefix = "" ;
   if (hash)
      {
      hash->iterate(completion_for,prefix,strlen(prefix),&match) ;
      }
   return match ;
}

// end of file vfinfo.cpp //
