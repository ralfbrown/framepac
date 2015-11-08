/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File: frstring.cpp	 	class FrString 				*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,1998,1999,2000,2001,2009		*/
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

#if defined(__GNUC__)
#  pragma implementation "frstring.h"
#endif

#include "frbytord.h"
#include "frstring.h"

/************************************************************************/
/*	Global data for this module				      	*/
/************************************************************************/

const char str_lowercaseString[] = "lowercaseString" ;
const char str_uppercaseString[] = "uppercaseString" ;

/************************************************************************/
/************************************************************************/

//----------------------------------------------------------------------

void FrString::lowercaseString(FrCharEncoding enc)
{
   size_t len = stringLength() ;
   switch (charWidth())
      {
      case 1:
	 {
	 const unsigned char *map = FrLowercaseTable(enc) ;
	 for (size_t i = 0 ; i < len ; i++)
	    m_value[i] = (char)map[(unsigned char)m_value[i]] ;
	 }
	 break ;
      case 2:
	 {
	 for (size_t i = 0 ; i < len ; i++)
	    {
	    FrChar16 c = FrByteSwap16(((FrChar16*)m_value)[i]) ;
	    ((FrChar16*)m_value)[i] = (FrChar16)FrByteSwap16(Fr_towlower(c)) ;
	    }
	 }
	 break ;
      case 4:
	 unsupp_char_size(str_lowercaseString) ;
	 break ;
      default:
	 bad_char_width(str_lowercaseString) ;
	 break ;
      }
   return ;
}

//----------------------------------------------------------------------

void FrString::uppercaseString(FrCharEncoding enc)
{
   size_t len = stringLength() ;
   switch (charWidth())
      {
      case 1:
	 {
	 const unsigned char *map = FrUppercaseTable(enc) ;
	 for (size_t i = 0 ; i < len ; i++)
	    m_value[i] = map[(unsigned char)m_value[i]] ;
	 }
	 break ;
      case 2:
	 {
	 for (size_t i = 0 ; i < len ; i++)
	    {
	    FrChar16 c = FrByteSwap16(((FrChar16*)m_value)[i]) ;
	    ((FrChar16*)m_value)[i] = (FrChar16)FrByteSwap16(Fr_towupper(c)) ;
	    }
	 }
	 break ;
      case 4:
	 unsupp_char_size(str_uppercaseString) ;
	 break ;
      default:
	 bad_char_width(str_uppercaseString) ;
	 break ;
      }
   return ;
}

// end of file frstrng1.cpp //
