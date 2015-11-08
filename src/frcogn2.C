/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File: frcogn2.cpp		inter-language cognate scoring #2	*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 2001,2002,2009,2015					*/
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

#include <stdio.h>
#include "frlist.h"
#include "frstring.h"

//----------------------------------------------------------------------

void FrSetCognateLetters(const FrList *cognates, size_t fuzzy_score)
{
   FrString *letters = new FrString(cognates) ;
   if (letters)
      FrSetCognateLetters((char*)letters->stringValue(),fuzzy_score) ;
   free_object(letters) ;
   return ;
}

//----------------------------------------------------------------------

bool FrLoadCognateLetters(const char *filename, size_t fuzzy_score)
{
   if (filename && *filename)
      {
      FILE *fp = fopen(filename,"r") ;
      if (fp)
	 {
	 FrList *cognates = 0 ;
	 FrList **end = &cognates ;
	 while (!feof(fp))
	    {
	    char line[1024] ;
	    if (fgets(line,sizeof(line),fp))
	       cognates->pushlistend(new FrString(line),end) ;
	    }
	 *end = 0 ;			// terminate the list
	 FrSetCognateLetters(cognates,fuzzy_score) ;
	 free_object(cognates) ;
	 fclose(fp) ;
	 return true ;
	 }
      }
   return false ;
}

//----------------------------------------------------------------------

// end of file frcogn2.cpp //
