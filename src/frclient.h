/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frclient.h	    network-client code				*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1995,1996,1997,2001,2009				*/
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

#ifndef __FRCLIENT_H_INCLUDED
#define __FRCLIENT_H_INCLUDED

#if defined(__GNUC__)
#  pragma interface
#endif

/************************************************************************/
/************************************************************************/

class FrRemoteDB
   {
   private:
      FrConnection *connection ;
      int   handle ;
   public:
      FrRemoteDB(FrConnection *conn, int handle) ;
      ~FrRemoteDB() ;
      bool readFrame(FrSymbol *framename) ;
      bool writeFrame(FrSymbol *framename) ;
      bool getUserData(DBUserData *userdata) const ;
      bool setUserData(DBUserData *userdata) const ;

      // access to internal state
      int dbHandle() const { return handle ; }
   } ;



#endif /* !__FRCLIENT_H_INCLUDED */

// end of file frclient.h //
