/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC								*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File: frstrng3.cpp		class FrConstString			*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 2004,2005,2009 Ralf Brown/Carnegie Mellon University	*/
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

#if defined(__GNUC__)
#  pragma implementation "frstring.h"
#endif

#include "frstring.h"

#ifdef FrSTRICT_CPLUSPLUS
#  include <cstdlib>
#  include <cstring>
#else
#  include <stdlib.h>
#  include <string.h>	// for GCC 2.x
#endif /* FrSTRICT_CPLUSPLUS */

/************************************************************************/
/************************************************************************/

int FrStringCmp(const void *s1, const void *s2, int length, int width1,
		int width2) ;
void *FrStrCpy(void *dest, const void *src, int length, int w1,int w2) ;

/************************************************************************/

/**********************************************************************/
/*    Member functions for class FrConstString			      */
/**********************************************************************/

FrConstString::FrConstString()
{
   setLength(0) ;
   setCharWidth(1) ;
   m_value = 0 ;
   return ;
}

//----------------------------------------------------------------------

FrConstString::FrConstString(const void *name)
{
   if (!name)
      name = "" ;
   setCharWidth(1) ;
   setLength(strlen((const char *)name)) ;
   m_value = (unsigned char*)name ;
   return ;
}

//----------------------------------------------------------------------

FrConstString::FrConstString(const void *name, size_t len)
{
   if (!name)
      name = "" ;
   setCharWidth(1) ;
   setLength(len) ;
   m_value = (unsigned char*)name ;
   return ;
}

//----------------------------------------------------------------------

FrConstString::~FrConstString()
{
   m_value = 0 ;
   setLength(0) ;
   return ;
}

//----------------------------------------------------------------------

FrObjectType FrConstString::objType() const
{
   return OT_FrString ;
}

//----------------------------------------------------------------------

const char *FrConstString::objTypeName() const
{
   return "FrConstString" ;
}

//----------------------------------------------------------------------

void FrConstString::freeObject()
{
   delete this ;
   return ;
}

//----------------------------------------------------------------------

FrObject *FrConstString::copy() const
{
   return new FrConstString(stringValue()) ;
}

//----------------------------------------------------------------------

FrObject *FrConstString::deepcopy() const
{
   return new FrConstString(stringValue(),stringLength()) ;
}

//----------------------------------------------------------------------

bool FrConstString::setNth(size_t /*N*/, const FrObject * /*newchar*/)
{
   return false ;
}

//----------------------------------------------------------------------

FrObject *FrConstString::insert(const FrObject * /*newelts*/,
				  size_t /*pos*/, bool /*cp*/)
{
   return 0 ;
}

//----------------------------------------------------------------------

FrObject *FrConstString::elide(size_t /*start*/, size_t /*end*/)
{
   return 0 ;
}


// end of file frcstrng.cpp //

