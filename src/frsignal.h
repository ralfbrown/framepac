/************************************************************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File: frsignal.h	class FrSignalHandler				*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1997,2001 Ralf Brown/Carnegie Mellon University	*/
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

#ifndef __FRSIGNAL_H_INCLUDED
#define __FRSIGNAL_H_INCLUDED

#ifndef __FRCOMMON_H_INCLUDED
#include "frcommon.h"
#endif

#if defined(__GNUC__)
#  pragma interface
#endif

/************************************************************************/
/************************************************************************/

typedef void FrSignalHandlerFunc(int) ;

class FrSignalHandler
   {
   private:
      void *old_handler ;
      FrSignalHandlerFunc *func ;
      long old_mask ;
      long old_flags ;
      int  number ;			// signal number
   public:
      FrSignalHandler(int signal, FrSignalHandlerFunc *handler) ;
      ~FrSignalHandler() ;
      FrSignalHandlerFunc *set(FrSignalHandlerFunc *new_handler) ;
      void raise(int arg) const ;

      // access to internal state
      int signalNumber() const { return number ; }
      FrSignalHandlerFunc *currentHandler() const { return func ; }
   } ;

#endif /* !__FRSIGNAL_H_INCLUDED */

// end of file frsignal.h //
