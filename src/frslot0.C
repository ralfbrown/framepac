/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01  							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frframev.cpp		class FrFrame (virtual functions)	*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,1998,2009				*/
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

#include "frframe.h"

/************************************************************************/
/*    Global variables for class FrFrame				*/
/************************************************************************/

FrAllocator FrSlot::allocator("FrSlot",sizeof(FrSlot)) ;
FrAllocator FrFrame::allocator("FrFrame",sizeof(FrFrame)) ;

// end of file frslot0.cpp //
