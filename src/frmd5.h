/************************************************************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File: frmd5.h		MD5 hash function			*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1998,2003,2006 Ralf Brown/Carnegie Mellon University	*/
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

#ifndef __FRMD5_H_INCLUDED
#define __FRMD5_H_INCLUDED

#include "frconfig.h"

#ifdef FrSTRICT_CPLUSPLUS
#  include <cstdio>
#  include <iostream>
#else
#  include <iostream.h>
#  include <stdio.h>
#endif /* FrSTRICT_CPLUSPLUS */

struct FrMD5Signature
   {
   public: // data members
      unsigned char signature[16] ;
   public: // methods
      FrMD5Signature(uint32_t sig[4]) ;
      void print(FILE *fp) const ;
      ostream &print(ostream &out) const ;
   } ;

//----------------------------------------------------------------------

// compute signature of NUL-terminated string
// (returned signature should be delete'd when no longer needed)
FrMD5Signature *FrMD5(char *string) ;

// compute signature of an arbitrary block of data
// (returned signature should be delete'd when no longer needed)
FrMD5Signature *FrMD5(char *data, size_t length) ;

#endif /* !__FRMD5_H_INCLUDED */

// end of file frmd5.h //
