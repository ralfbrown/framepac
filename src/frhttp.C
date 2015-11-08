/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC								*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frhttp.h		HTTP file retrieval			*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 2000,2001,2003,2004,2009,2010,2011			*/
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

#include "framerr.h"
#include "frfilutl.h"
#include "frsckstr.h"
#include "frstring.h"
#include "frsstrm.h"
#include "frurl.h"
#include "frutil.h"

#ifdef FrSTRICT_CPLUSPLUS
#  include <cstdlib>
#  include <cstring>
#  include <string>
#else
#  include <stdlib.h>
#  include <string.h>
#endif /* FrSTRICT_CPLUSPLUS */

/************************************************************************/
/************************************************************************/

#if COMMENT
   First line of header sent to server is
	{mode} HTTP/1.0\r\n
	>>{mode}s:
            	CONNECT   (HTTPS only)
		GET
		POST
		HEAD




	GET {path} HTTP/1.0
	User-Agent:
	Pragma: no-cache
	Authorization:
	Proxy-Authorization:
	Cookie:
	\r\n

	POST {path} HTTP/1.0
	User-Agent:
	Pragma: no-cache
	Authorization:
	Proxy-Authorization:
	Cookie:
	Content-Type:
	Content-Length:
	\r\n


Reply starts with server protocol version followed by numeric status code:
	HTTP/1.0 200
Then comes the reply header, followed by a blank line, followed by the doc:
	Date: Mon, 11 Dec 2000 17:56:39 GMT
	Server: Apache/1.3.6 (Unix) mod_ssl/2.2.8 OpenSSL/0.9.2b
	Connection: close
	Content-Type: text/html
	Content-Length: 0000
	Transfer-Encoding:		HTTP/1.1
	\r\n
	

#endif /* COMMENT */

/************************************************************************/
/*	Global data for this module					*/
/************************************************************************/

static char *user_agent = 0 ;

/************************************************************************/
/************************************************************************/

static size_t hex_value(char c)
{
   if (c >= '0' && c <= '9')
      return c - '0' ;
   else if (c >= 'a' && c <= 'f')
      return c - 'a' + 10 ;
   else if (c >= 'A' && c <= 'F')
      return c - 'A' + 10 ;
   else
      return -1 ;
}

//----------------------------------------------------------------------

static void skip_CRLF(string &s, size_t &pos)
{
   if (pos < s.length() && s[pos] == '\r')
      {
      pos++ ;
      if (pos < s.length() && s[pos] == '\n')
	 pos++ ;
      }
   return ;
}

//----------------------------------------------------------------------

static bool ishex(char c)
{
   return ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f')
	   || (c >= 'A' && c <= 'F')) ;
}

//----------------------------------------------------------------------

static size_t decode_hex(string &body, size_t &pos)
{
   size_t len = 0 ;
   while (pos < body.length() && ishex(body[pos]))
      {
      len = (16 * len) + hex_value(body[pos]) ;
      pos++ ;
      }
   // skip over everything up to and including the following CRLF
   while (pos < body.length() && body[pos] != '\r')
      pos++ ;
   skip_CRLF(body,pos) ;
   return len ;
}

//----------------------------------------------------------------------

static void unchunk_http_body(string &body)
{
   string unchunked ;
   unchunked.reserve(body.length()) ;
   size_t pos = 0 ;
   while (pos < body.length())
      {
      size_t chunklen = decode_hex(body,pos) ;
      unchunked.append(body,pos,chunklen) ;
      pos += chunklen ;
      skip_CRLF(body,pos) ;
      }
   body = unchunked ;
   return ;
}

//----------------------------------------------------------------------

void FrSetHTTPUserAgent(const char *agent)
{
   FrFree(user_agent) ;
   user_agent = FrDupString(agent) ;
   return ;
}

//----------------------------------------------------------------------

const char *FrGetHTTPUserAgent()
{
   return user_agent ? user_agent : "minimal URL fetcher" ;
}

//----------------------------------------------------------------------

bool FrFetchHTTP(FrSocket s, const char *server, int port, const char *path,
		 const char *params, bool secure, char *&content,
		 size_t &content_length, const char *user,
		 const char *password, const char *post_data,
		 bool skip_header, const char *extra_headers)
{
   (void)secure; //!!!
   content = 0 ;
   content_length = 0 ;
   if (s == (FrSocket)INVALID_SOCKET)
      return false ;
   FrOSockStream out(s) ;
   FrISockStream in(s) ;
   if (in.good() && out.good())
      {
      char *encodedpath = FrURLEncode(path) ;
      char *encodedparams = params ? FrURLEncode(params) : 0 ;
      if (encodedpath && (!params || encodedparams))
	 {
	 // send the retrieval request
	 size_t postlen = 0 ;
	 if (post_data)
	    {
	    out << "POST " ;
	    postlen = strlen(post_data) ;
	    }
	 else
	    out << "GET " ;
	 out << encodedpath ;
	 if (encodedparams)
	    out << '?' << encodedparams ;
	 out << " HTTP/1.1\r\n" ;
	 // specify the host, in case of virtual hosting
	 out << "Host: " << server ;
	 if (port > 0)
	    out << ":" << port ;
	 out << endl ;
	 // identify ourself
	 out << "User-Agent: " << FrGetHTTPUserAgent() << "\r\n" ;
	 out << "Accept: */*\r\n" ;
	 out << "Connection: close\r\n" ;
	 // optionally pass authentication information
	 if (user && password && *user)
	    {
	    size_t userlen = strlen(user) ;
	    size_t passlen = strlen(password) + 1 ;
	    size_t authlen = userlen + passlen + 1 ;
	    FrLocalAlloc(char,auth,1024,authlen) ;
	    if (auth)
	       {
	       memcpy(auth,user,userlen) ;
	       auth[userlen++] = ':' ;
	       memcpy(auth+userlen,password,passlen) ;
	       char *encodedauth = FrBase64Encode(auth) ;
	       out << "WWW-Authenticate: " << encodedauth << "\r\n" ;
	       FrFree(encodedauth) ;
	       }
	    FrLocalFree(auth) ;
	    }
	 if (extra_headers)
	    out << extra_headers ;
	 if (post_data)
	    out << "Content-Type: application/x-www-form-urlencoded\r\n"
		<< "Content-Length: " << postlen << "\r\n" ;
	 out << "\r\n" << flush ;
	 if (postlen > 0)
	    out << post_data << "\r\n" << flush ;
	 Fr_usleep(50000) ; // wait a bit for the response
	 // get and parse the return code
	 char line[FrMAX_LINE] ;
	 in.getline(line,sizeof(line)) ;
	 char *statcode = strchr(line,' ') ;
	 int statuscode = 0 ;
	 if (statcode)
	    {
	    statuscode = atoi(statcode) ;
	    if (!post_data &&
		(statuscode == 301	// permanent redirection
		 || statuscode == 302 || statuscode == 303) // temporary
	       )
	       {
	       // instead of reading the returned file, just scan the header
	       //   for a Location: line
	       while (!in.eof() && !in.fail())
		  {
		  line[0] = '\0' ;
		  in.getline(line,sizeof(line)) ;
		  if (line[0] == '\0' || line[0] == '\r')
		     break ;
		  else if (Fr_strnicmp(line,"Location:",9) == 0)
		     {
		     char *loc = line+9 ;
		     FrSkipWhitespace(loc) ;
		     char *end = strchr(loc,'\0') ;
		     if (end > loc && end[-1] == '\r')
			*--end = '\0' ;
		     if (end > loc)
			{
			// OK, we got a new location, so go get that URL
			//   instead of the original one
			char *newurl = FrDupStringN(loc,end-loc) ;
			bool status = FrFetchURL(newurl,content,
						 content_length) ;
			FrFree(newurl) ;
			return status ;
			}
		     }
		  }
	       }
	    }
	 size_t content_len(0) ;
	 bool chunked = false ;
	 if (skip_header)
	    {
	    int prev = '\0' ;
	    string tag ;
	    tag.reserve(500) ;
	    bool accumulate_tag = false ;
	    while (!in.eof() && !in.fail())
	       {
	       int curr = in.get() ;
	       if (curr == '\r' && in.peek() == '\n')
		  curr = in.get() ;
	       if (curr == EOF || (curr == '\n' && prev == '\n'))
		  break ;
	       if (curr == '\n')
		  {
		  // start a new header line
		  tag.clear() ;
		  accumulate_tag = true ;
		  }
	       else if (accumulate_tag)
		  {
		  if (curr != ':')
		     tag += (char)tolower((char)curr) ;
		  else
		     {
		     char linebuf[500] ;
		     // check the tag name
		     if (tag == "content-length")
			{
			in.getline(linebuf,sizeof(linebuf),'\r') ;
			char *lineptr = linebuf ;
			FrSkipWhitespace(lineptr) ;
			char *end ;
			size_t len = strtoul(lineptr,&end,0) ;
			if (end != lineptr)
			   content_len = len ;
			}
		     else if (tag == "transfer-encoding")
			{
			in.getline(linebuf,sizeof(linebuf),'\r') ;
			Fr_strlwr(linebuf) ;
			if (strstr(linebuf,"chunked"))
			   chunked = true ;
			}
		     else
			{
			while ((curr = in.get()) != EOF && curr != '\r')
			   ;
			}
		     accumulate_tag = false ;
		     }
		  }
	       prev = curr ;
	       }
	    }
	 string doc ;
	 doc.reserve(content_len > 0 ? content_len+2 : 4000) ;
	 size_t c_len(0) ;
	 while (!in.eof() && !in.fail())
	    {
	    int c = in.get() ;
	    if (c == EOF)
	       break ;
	    doc += (char)c ;
	    c_len++ ;
	    if (content_len > 0 && c_len == content_len)
	       break ;
	    }
	 doc += '\0' ;
	 if (chunked)
	    unchunk_http_body(doc) ;
	 content = FrNewN(char,c_len+1) ;
	 if (content)
	    {
	    content[doc.copy(content,c_len)] = '\0' ;
	    content_length = c_len ;
	    }
	 else
	    FrNoMemory("while fetching HTTP page\n") ;
	 return statuscode < 400 ;
	 }
      }
   return false ;
}

//----------------------------------------------------------------------

bool FrFetchHTTP(const char *server, int port, const char *path,
		 const char *params, bool secure, char *&content,
		 size_t &content_length, const char *user,
		 const char *password, const char *post_data,
		 bool skip_header, const char *extra_headers)
{
   content = 0 ;
   content_length = 0 ;
   bool success = false ;
   if (server && *server && port > 0 && path != 0)
      {
      FrSocket s = FrConnectToPort(server,port) ;
      if (s != (FrSocket)INVALID_SOCKET)
	 {
	 success = FrFetchHTTP(s,server,port,path,params,secure,content,
			       content_length, user, password, post_data,
			       skip_header, extra_headers) ;
	 FrDisconnectPort(s) ;
	 }
      }
   return success ;
}

// end of file frhttp.cpp //
