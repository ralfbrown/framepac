/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frpacket.cpp	    network-packet manipulation			*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1995,1996,1997,2009,2011				*/
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

#include "frconfig.h"

#ifdef FrSERVER
#include <iostream.h>
#include <stdlib.h>
#include "framerr.h"
#include "frconnec.h"

/************************************************************************/
/************************************************************************/

FrPacket::FrPacket(FrConnection *connect, const char *header, bool am_server)
{
   connection = connect ;
   length = FrLoadShort(header+0) ;
   reqcode = (BYTE)FrLoadByte(header+2) ;
   seqnumber = (BYTE)FrLoadByte(header+3) ;
   callback_func = 0 ;
   callback_data = 0 ;
   if (am_server)
      {
//FIXME
      }
   else
      {
      }
}

//----------------------------------------------------------------------

FrPacket::~FrPacket()
{
   if (data)
      FrFree(data) ;
   if (fragments)
      FrFree(fragments) ;
}

//----------------------------------------------------------------------

FrPacket *FrPacket::copyPacket() const
{
   FrPacket *copy = new FrPacket ;
   if (copy)
      {

      }
   else
      FrNoMemory("while copying a network packet") ;
   return copy ;
}

//----------------------------------------------------------------------

int FrPacket::send()
{
   if (connection)
      {
      connection->send(this) ;
      return FSE_Success ;
      }
   else
      return FSE_INVALIDPARAMS ;
}

//----------------------------------------------------------------------

int FrPacket::reply(int status)
{
   if (!connection)
      return FSE_NOSUCHCONNECTION ;
   char header[5] ;
   FrStoreByte(reqcode,   header+2) ;
   FrStoreByte(seqnumber, header+3) ;
   FrStoreByte(status,    header+4) ;
   return connection->send(sizeof(header),header,0,0) ;
}

//----------------------------------------------------------------------

int FrPacket::reply(int status, const char *replydata, int datalength)
{
   if (!connection)
      return FSE_NOSUCHCONNECTION ;
   char header[5] ;
   FrStoreByte(reqcode,   header+2) ;
   FrStoreByte(seqnumber, header+3) ;
   FrStoreByte(status,    header+4) ;
   return connection->send((int)sizeof(header),header,datalength,replydata) ;
}

//----------------------------------------------------------------------

int FrPacket::replySuccess()
{
   if (!connection)
      return FSE_NOSUCHCONNECTION ;
   char header[5] ;
   FrStoreByte(reqcode,    header+2) ;
   FrStoreByte(seqnumber,  header+3) ;
   FrStoreByte(FSE_Success,header+4) ;
   return connection->send(sizeof(header),header,0,0) ;
}

//----------------------------------------------------------------------

int FrPacket::replyGeneralError()
{
   if (!connection)
      return FSE_NOSUCHCONNECTION ;
   char header[5] ;
   FrStoreByte(reqcode,    header+2) ;
   FrStoreByte(seqnumber,  header+3) ;
   FrStoreByte(FSE_GENERAL,header+4) ;
   return connection->send(sizeof(header),header,0,0) ;
}

//----------------------------------------------------------------------

int FrPacket::replyResourceLimit()
{
   if (!connection)
      return FSE_NOSUCHCONNECTION ;
   char header[5] ;
   FrStoreByte(reqcode,           header+2) ;
   FrStoreByte(seqnumber,         header+3) ;
   FrStoreByte(FSE_RESOURCELIMIT, header+4) ;
   return connection->send(sizeof(header),header,0,0) ;
}

//----------------------------------------------------------------------

int FrPacket::respond(int status)
{
   if (!connection)
      return FSE_NOSUCHCONNECTION ;
   char header[5] ;
   FrStoreByte(reqcode,   header+2) ;
   FrStoreByte(seqnumber, header+3) ;
   FrStoreByte(status,    header+4) ;
   return connection->send(sizeof(header),header,0,0) ;
}

//----------------------------------------------------------------------

int FrPacket::respond(int status, const char *replydata, int datalength)
{
   if (!connection)
      return FSE_NOSUCHCONNECTION ;
   char header[5] ;
   FrStoreByte(reqcode,   header+2) ;
   FrStoreByte(seqnumber, header+3) ;
   FrStoreByte(status,    header+4) ;
   return connection->send(sizeof(header),header,datalength,replydata) ;
}

//----------------------------------------------------------------------

int FrPacket::allocateBuffer(int datalength)
{
   if (!connection)
      return false ;
   if (data)
      FrFree(data) ;
   if (fragments)
      {
      FrFree(fragments) ;
      fragments = 0 ;
      }
   data = FrNewN(char,datalength) ;
   length = datalength ;
   int max_datasize = connection->maxDataSize() ;
   if (datalength > max_datasize)
      {
      nfragments = (datalength+max_datasize-1) / max_datasize ;
      missing = nfragments ;
      fragments = FrNewC(char,nfragments) ;
      if (!fragments)
	 {
	 FrFree(data) ;
	 data = 0 ;
	 }
      }
   else
      missing = nfragments = 0 ;
   complete = false ;
   return data != 0 ;
}

//----------------------------------------------------------------------

bool FrPacket::setFragment(int fragnum, const char *frag)
{
   if  (fragnum >= 0 && fragnum < nfragments && fragments &&
	fragments[fragnum] == 0 && connection && data)
      {
      int max_size = connection->maxDataSize() ;
      int len = max_size ;
      if (fragnum == nfragments-1)
	 len = length % max_size ;	// left-over bytes in last fragment
      memcpy(data+fragnum*max_size,frag,len) ;
      if (--missing <= 0 && callback_func)
	 callback_func(this,0,0,callback_data) ;
      return true ;
      }
   else
      return false ;
}

//----------------------------------------------------------------------

//----------------------------------------------------------------------

//----------------------------------------------------------------------

const char *FrPacket::objTypeName() const
{
   return "FrPacket" ;
}

//----------------------------------------------------------------------

ostream &FrPacket::printValue(ostream &output) const
{
   output << "#<FrPacket :Seq=" << (int)seqnumber << " :Req=" << (int)reqcode
	  << " :Len=" << length << ">" ;
   return output ;
}

//----------------------------------------------------------------------

char *FrPacket::displayValue(char *buffer) const
{
   memcpy(buffer,"#<FrPacket :Seq=",16) ;
   buffer += 16 ;
   ultoa(seqnumber,buffer,10) ;
   buffer = strchr(buffer,'\0') ;
   memcpy(buffer," :Req=",6) ;
   buffer += 6 ;
   ultoa(reqcode,buffer,10) ;
   buffer = strchr(buffer,'\0') ;
   memcpy(buffer," :Len=",6) ;
   buffer += 6 ;
   ultoa(length,buffer,10) ;
   buffer = strchr(buffer,'\0') ;
   *buffer++ = '>' ;
   *buffer = '\0' ;
   return buffer ;
}

//----------------------------------------------------------------------

size_t FrPacket::displayLength() const
{
   return 29 + Fr_number_length(seqnumber) + Fr_number_length(reqcode) +
	 Fr_number_length(length) ;
}

//----------------------------------------------------------------------

void FrPacket::setRecvCallback(FrAsyncCallback *cb_func, void *cb_data)
{
   callback_func = cb_func ;
   callback_data = cb_data ;
}

#endif /* FrSERVER */

// end of file frpacket.cpp //
