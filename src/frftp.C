/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC								*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frftp.cpp		FTP file retrieval			*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 2001,2003,2006,2009					*/
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>	// needed by RedHat 7.1
#include "frctype.h"
#include "frfilutl.h"
#include "frprintf.h"
#include "frsckstr.h"
#include "frstring.h"
#include "frurl.h"

/************************************************************************/
/************************************************************************/

static bool get_FTP_response(FrSockStream &str, char *&line)
{
   // read up until we get a line starting with a three-digit number followed
   //   by whitespace, or we hit EOF
   size_t alloc = 0 ;
   size_t numread = 0 ;
   line = 0 ;
   while (!str.eof() && !str.fail())
      {
      int c = str.get() ;
      if (c == EOF)
	 break ;
      if (numread >= alloc)
	 {
	 alloc += 128 ;
	 char *newline = FrNewR(char,line,alloc) ;
	 if (!newline)
	    break ;
	 line = newline ;
	 }
      line[numread] = (char)c ;
      if (c == '\n')
	 {
	 line[numread] = '\0' ;
	 if (numread > 0 && line[numread-1] == '\r')
	    line[--numread] = '\0' ;
	 if (numread > 3 && Fr_isdigit(line[0]) && Fr_isdigit(line[1]) &&
	     Fr_isdigit(line[2]) && Fr_isspace(line[3]))
	    return true ;
	 numread = 0 ;
	 }
      else
	 numread++ ;
      }
   return false ;
}

//----------------------------------------------------------------------

static bool login_to_server(FrSockStream &str, const char *user,
			      const char *password)
{
   // get the welcome message and see whether it's a proper login prompt
   char *response = 0 ;
   if (!get_FTP_response(str,response) || response[0] != '2')
      {
      FrFree(response) ;
      return false ;
      }
   FrFree(response) ;
   str << "USER " << user << "\r\n" << flush ;
   if (!get_FTP_response(str,response))
      {
      FrFree(response) ;
      return false ;
      }
   if (response[0] == '2')
      {
      // no password required, so we're good to go!
      FrFree(response) ;
      return true ;
      }
   else if (response[0] != '3')
      {
      // login refused!
      FrFree(response) ;
      return false ;
      }
   size_t SKEY_challenge_offset = 0 ;
   if (Fr_strnicmp(response,"331 s/key ",10) == 0)
      SKEY_challenge_offset = 10 ;
   else if (Fr_strnicmp(response,"331 opiekey ",12) == 0)
      SKEY_challenge_offset = 12 ;
   if (SKEY_challenge_offset)
      {
      //!!! support for S/KEY or OPIE goes here
      }
   FrFree(response) ;
   str << "PASS " << password << "\r\n" << flush ;
   if (!get_FTP_response(str,response))
      {
      FrFree(response) ;
      return false ;
      }
   else if (response[0] != '2')
      {
      FrFree(response) ;
      return false ;
      }
   FrFree(response) ;
   return true ;
}

//----------------------------------------------------------------------

static bool open_passive_connection(FrSockStream &str, FrISockStream *&data)
{
   str << "PASV\r\n" << flush ;
   char *response = 0 ;
   data = 0 ;
   bool success = false ;
   if (get_FTP_response(str,response) && response[0] == '2')
      {
      // parse the line returned by the FTP server to get the IP address and
      //   port number, which are returned as a list of six comma-separated
      //   decimal numbers
      char *info = response + 4 ;
      // skip to first number
      while (*info && !Fr_isdigit(*info))
	 info++ ;
      unsigned char addr[4] ;
      size_t i ;
      char *end ;
      success = true ;
      for (i = 0 ; i < sizeof(addr) && Fr_isdigit(*info) ; i++)
	 {
	 addr[i] = (unsigned char)strtol(info,&end,0) ;
	 if (end != info)
	    {
	    if (*end == ',')
	       end++ ;
	    info = end ;
	    }
	 else
	    {
	    success = false ;
	    break ;
	    }
	 }
      if (success)
	 {
	 char hostname[40] ;
	 Fr_sprintf(hostname,sizeof(hostname),"%d.%d.%d.%d%c",
		    addr[0],addr[1],addr[2],addr[3],'\0') ;
	 addr[0] = (unsigned char)strtol(info,&end,0) ;
	 int portnum = 0 ;
	 success = false ;
	 if (end != info)
	    {
	    if (*end == ',')
	       end++ ;
	    info = end ;
	    addr[1] = (unsigned char)strtol(info,&end,0) ;
	    if (end != info)
	       {
	       portnum = (addr[0] << 8) + addr[1] ;
	       success = true ;
	       }
	    }
	 if (success && portnum >= 0)
	    {
            // OK, now that we have the host and port, open a connection
	    FrSocket sock = FrConnectToPort(hostname,portnum) ;
	    success = (sock != (FrSocket)INVALID_SOCKET) ;
	    if (success)
	       {
	       data = new FrISockStream(sock) ;
	       success = (data != 0 && data->good()) ;
	       }
	    }
	 }
      }
   FrFree(response) ;
   return success ;
}

//----------------------------------------------------------------------

static bool open_port(FrSockStream &str, FrISockStream *&data)
{
   data = 0 ;
(void)str ;
//!!!
   return false ;
}

/************************************************************************/
/************************************************************************/

bool FrFetchFTP(const char *server, int port, const char *user,
		  const char *password, const char *path,
		  char *&content, size_t &content_length, bool text_mode)
{
   content = 0 ;
   content_length = 0 ;
   if (user == 0)
      user = "anonymous" ;
   if (password == 0)
      password = "guest@" ;
   bool success = false ;
   if (server && port > 0 && path != 0)
      {
      FrSockStream str(server,port) ;
      if (str.good() && login_to_server(str,user,password))
	 {
	 char *directory = FrFileDirectory(path) ;
	 char *response = 0 ;
	 if (directory)
	    {
	    size_t len = strlen(directory) ;
	    if (len > 1 && directory[len-1] == '/')
	       directory[len-1] = '\0' ;
	    str << "cwd " << directory << "\r\n" << flush ;
	    FrFree(directory) ;
	    if (!get_FTP_response(str,response) || response[0] != '2')
	       {
	       FrFree(response) ;
	       return false ;
	       }
	    }
	 // first, try a passive-mode retrieval
	 FrISockStream *data = 0 ;
	 if (!open_passive_connection(str,data))
	    {
	    // if passive fails, we need to use a PORT command and accept an
	    //   incoming connection
	    if (!open_port(str,data))
	       {
//!!!
	       }
	    }
	 if (data && data->good() && !data->eof() && !data->fail())
	    {
	    // retrieve the file
	    const char *filename = FrFileBasename(path) ;
	    str << "RETR " << filename << endl ;
	    if (get_FTP_response(str,response) && response[0] == '1')
	       {
	       // read the file's data off the data socket
	       size_t alloc = 0 ;
	       content = 0 ;
	       while (!data->eof() && !data->fail())
		  {
		  int c = data->get() ;
		  if (c == EOF)
		     break ;
		  if (text_mode && c == '\n' && content_length > 0 &&
		      content[content_length-1] == '\r')
		     content_length-- ;	// convert CRLF to plain LF
		  else if (content_length >= alloc)
		     {
		     size_t newalloc = alloc + 512 ;
		     char *newcont = FrNewR(char,content,newalloc+1) ;
		     if (newcont)
			{
			alloc = newalloc ;
			content = newcont ;
			}
		     else
			break ;
		     }
		  content[content_length++] = (char)c ;
		  }
	       if (content)
		  content[content_length] = '\0' ;
	       success = true ;
	       }
	    else
	       success = false ;
	    }
	 delete data ;
	 FrFree(response) ;
	 }
      // let's be polite and logout
      str << "QUIT" << "\r\n" << flush ;
      }
   return success ;
}

// end of file frftp.cpp //
