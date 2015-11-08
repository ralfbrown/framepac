/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File: frcmap.cpp	 	character-mapping utility functions	*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,1998,2009,2010,2015		*/
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

#include "framerr.h"
#include "frlist.h"
#include "frnumber.h"
#include "frstring.h"

/************************************************************************/
/************************************************************************/

static const char invalid_char_map_msg[] =
   "invalid character mapping (ignored)" ;

/************************************************************************/
/************************************************************************/

char *FrMapString(char *string, const FrCasemapTable mapping)
{
   if (string)
      {
      for (char *s = string ; *s ; s++)
	 {
	 *s = (char)mapping[*(unsigned char*)s] ;
	 }
      }
   return string ;
}

//----------------------------------------------------------------------

FrCasemapTable FrMakeCharacterMap(const FrList *map)
{
   unsigned char *mapping = FrNewN(unsigned char,UCHAR_MAX+1) ;
   if (!mapping)
      {
      FrNoMemory("while creating character mapping") ;
      return 0 ;
      }
   for (int i = 0 ; i <= (int)UCHAR_MAX ; i++)
      mapping[i] = (unsigned char)i ;
   while (map)
      {
      FrList *m = (FrList*)map->first() ;
      if (m && m->consp())
	 {
	 FrNumber *n1 = (FrNumber*)m->first() ;
	 FrNumber *n2 = (FrNumber*)m->rest() ;
	 if (n2 && n2->consp())
	    n2 = (FrNumber*)((FrList*)n2)->first() ;
	 if (n1 && n2 && n1->numberp() && n2->numberp())
	    {
	    int mapfrom = (int)*n1 ;
	    int mapto = (int)*n2 ;
	    if (mapfrom >= 0 && mapfrom <= (int)UCHAR_MAX && mapto >= 0 &&
		mapto <= (int)UCHAR_MAX)
	       mapping[mapfrom] = (unsigned char)mapto ;
	    else
	       FrWarning("character mapping out of range (ignored)") ;
	    }
	 else
	    FrWarning(invalid_char_map_msg) ;
	 }
      else
	 FrWarning(invalid_char_map_msg) ;
      map = map->rest() ;
      }
   return mapping ;
}

//----------------------------------------------------------------------

void FrDestroyCharacterMap(FrCasemapTable mapping)
{
   FrFree(VoidPtr(mapping)) ;
   return ;
}

// end of file frcmap.cpp //
