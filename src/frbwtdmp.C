/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC								*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frbwtdmp.cpp	    extract text from BWTransform n-gram index	*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 2004,2006,2009 Ralf Brown/Carnegie Mellon University	*/
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

#include "frbwt.h"
#include "frbitvec.h"

/************************************************************************/
/*	Methods for class FrBWTIndex					*/
/************************************************************************/

FrBitVector *FrBWTIndex::findRecordStarts(uint32_t &limit) const
{
   limit = numItems() ;
   FrBitVector *starts = new FrBitVector(limit) ;
   if (!starts)
      return 0 ;
   starts->setRange(0,limit,true) ;
   size_t i ;
   uint32_t succ = 0 ;
   for (i = 0 ; i < numItems() ; i++)
      {
      succ = getSuccessor(i,succ) ;
      if (succ < limit)
	 starts->setBit(succ,false) ;
      }
   return starts ;
}

//----------------------------------------------------------------------

bool FrBWTIndex::deconstruct(uint32_t *IDs, size_t bufsize) const
{
   if (!IDs || bufsize < totalItems())
      return false ;
   uint32_t limit ;
   FrBitVector *starts = findRecordStarts(limit) ;
   if (!starts)
      return false ;
   size_t count = 0 ;
   size_t total = totalItems() ;
   for (size_t i = 0 ; i < limit ; i++)
      {
      if (starts->getBit(i))
	 {
	 // nobody pointing at this entry, so it's the start of a record
	 size_t loc = i ;
	 while (loc < numItems() && count < total)
	    {
	    uint32_t id = getID(loc) ;
//if(id==0xFFFFFFFF)cout<<" IDs["<<count<<"] == -1, loc="<<loc<<", numItems="<<numItems()<<", C[#-1]="<<C(numIDs()-1)<<' '<<C(numIDs())<<endl;
	    IDs[count++] = id ;
	    loc = getSuccessor(loc) ;
	    }
	 if (count >= total)
	    {
	    if (i < numItems())
	       FrWarning("BWT index is corrupt!") ;
	    break ;
	    }
	 if (loc == i && i >= numItems())
	    IDs[count++] = (i - numItems()) + EORvalue() ;
//	 else if (loc >= EORvalue())
//	    IDs[count++] = loc ;
	 else
	    IDs[count++] = loc ;
	 }
      }
   delete starts ;
   return true ;
}

//----------------------------------------------------------------------

uint32_t *FrBWTIndex::deconstruct(size_t reserve_extra) const
{
   uint32_t *IDs = FrNewN(uint32_t,totalItems()+reserve_extra) ;
   if (IDs && !deconstruct(IDs,totalItems()))
      {
      FrFree(IDs) ;
      return 0 ;
      }
   return IDs ;
}

// end of file frbwtdmp.cpp //
