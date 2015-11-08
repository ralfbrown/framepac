/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC								*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frgeturl.cpp		retrieve file by URL			*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 2000,2001,2003,2009,2015				*/
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

#include <fcntl.h>
#if defined(__WATCOMC__) || defined(__MSDOS__) || defined(__NT__)
#  include <io.h>
#elif defined(unix) || defined(__linux__) || defined(__GNUC__)
#  include <unistd.h>
#endif /* __WATCOMC__ || __MSDOS__ || __NT__, unix */
#include <stdlib.h>
#include <string.h>
#include "framerr.h"
#include "frcommon.h"
#include "frctype.h"
#include "frlist.h"
#include "frmem.h"
#include "frnumber.h"
#include "frstring.h"
#include "frurl.h"
#include "frutil.h"

/************************************************************************/
/*	Manifest Constants						*/
/************************************************************************/

#ifndef O_BINARY
#  define O_BINARY 0
#endif /* !O_BINARY */

/************************************************************************/
/*	Global Variables						*/
/************************************************************************/

static FrList *proxy_servers = 0 ;

/************************************************************************/
/************************************************************************/

static bool get_local_file(const char *host, const char *path,
			     char *&content, size_t &content_length)
{
   content = 0 ;
   content_length = 0 ;
   if (!host || Fr_stricmp(host,"localhost") == 0)
      {
      int fd = open(path,O_RDONLY|O_BINARY) ;
      if (fd >= 0)
	 {
	 off_t filelen = lseek(fd,0L,SEEK_END) ;
	 (void)lseek(fd,0L,SEEK_SET) ;
	 content = FrNewN(char,filelen+1) ;
	 if (!content)
	    {
	    FrNoMemory("while loading local file into memory") ;
	    close(fd) ;
	    return false ;
	    }
	 int count = read(fd,content,filelen) ;
	 close(fd) ;
	 if (count < 0)
	    count = 0 ;
	 content[count] = '\0' ;
	 content_length = count ;
	 return count == filelen ;
	 }
      }
   return false ;
}

/************************************************************************/
/************************************************************************/

// parse URL into protocol, host, port, and path; optionally, redirect through
//   a proxy server

//----------------------------------------------------------------------

static bool known_toplevel_domain(const char *start, const char *end)
{
   static const char *domains[] =
      { ".com", ".org", ".edu", ".net", ".us", ".ca", ".de", ".uk", ".fr",
	".jp", ".it", ".dk", ".no", ".se", ".es", ".pt", ".cn" } ;
   if (!end)
      end = strchr(start,'\0') ;
   size_t len = end - start ;
   for (size_t i = 0 ; i < lengthof(domains) ; i++)
      {
      size_t domlen = strlen(domains[i]) ;
      if (domlen < len && Fr_stricmp(end-domlen,domains[i]) == 0)
	 return true ;
      }
   return false ;
}

//----------------------------------------------------------------------

static char *guess_protocol_from_host(const char *start, const char *end,
				      bool &is_file)
{
   is_file = false ;
   size_t len = end - start ;
   if (len == 3 && Fr_strnicmp(start,"www",3) == 0)
      return FrDupString("http") ;
   else if (len == 6 && Fr_strnicmp(start,"secure",6) == 0)
      return FrDupString("https") ;
   else if (len == 3 && Fr_strnicmp(start,"ftp",3) == 0)
      return FrDupString("ftp") ;
   else if (len == 4 && Fr_strnicmp(start,"wais",4) == 0)
      return FrDupString("wais") ;
   else if (len == 6 && Fr_strnicmp(start,"gopher",6) == 0)
      return FrDupString("gopher") ;
   const char *slash = strchr(end,'/') ;
   if (known_toplevel_domain(end,slash))
      return FrDupString("http") ;
   else if (!slash)
      {
      is_file = true ;
      return FrDupString("file") ;
      }
   return 0 ;
}

//----------------------------------------------------------------------

static void split_off_parameters(const char *url, char *&path, char *&params)
{
   const char *quesmark = strchr(url,'?') ;
   if (quesmark)
      {
      path = FrDupStringN(url,quesmark - url) ;
      params = FrDupString(quesmark+1) ;
      }
   else
      {
      path = FrDupString(url) ;
      params = 0 ;
      }
   return ;
}

//----------------------------------------------------------------------

bool FrSetURLProxyServer(const char *protocol, const char *host, int port)
{
   FrString proto(protocol) ;
   proto.lowercaseString() ;
   if (!host || !*host)
      {
      // remove proxy from list, allowing direct access on that protocol
      FrList *proxy = (FrList*)proxy_servers->assoc(&proto,::equal) ;
      if (proxy)
	 {
	 proxy_servers = listremove(proxy_servers,proxy) ;
	 free_object(proxy) ;
	 return true ;
	 }
      }
   else if (port > 0)
      {
      FrList *proxy = new FrList(proto.deepcopy(),new FrString(host),
				 new FrInteger(port)) ;
      pushlist(proxy,proxy_servers) ;
      return true ;
      }
   return false ;
}

//----------------------------------------------------------------------

bool FrGetURLProxyServer(const char *protocol, char *&host, int &port)
{
   // search the proxy list for a proxy for the given protocol
   host = 0 ;
   port = 0 ;
   if (protocol && *protocol)
      {
      FrString proto(protocol) ;
      proto.lowercaseString() ;
      FrList *proxy = (FrList*)proxy_servers->assoc(&proto,::equal) ;
      if (proxy)
	 {
	 FrString *hostname = (FrString*)proxy->second() ;
	 FrInteger *portnum = (FrInteger*)proxy->third() ;
	 host = FrDupString((char*)hostname->stringValue()) ;
	 port = (int)portnum->intValue() ;
	 return true ;
	 }
      }
   return false ;			// no proxy for specified protocol
}

//----------------------------------------------------------------------

bool FrParseURL(const char *url, char *&protocol, char *&host, int &port,
		  char *&user, char *&password, char *&path, char *&params,
		  bool bypass_proxies)
{
   protocol = 0 ;
   host = 0 ;
   port = 0 ;
   user = 0 ;
   password = 0 ;
   path = 0 ;
   params = 0 ;
   if (url)
      {
      // split the URL into its components:
      //          protocol://user:password@host:port/path
      const char *ptr = url ;
      while (*ptr && (Fr_isalpha(*ptr) || Fr_isdigit(*ptr)))
	 ptr++ ;
      bool got_host = false ;
      if (*ptr == ':')
	 {
	 // we have a valid protocol name
	 protocol = FrDupStringN(url, ptr - url) ;
	 if (!bypass_proxies && FrGetURLProxyServer(protocol,host,port))
	    {
	    path = FrDupString(ptr+1) ;
	    return true ;
	    }
	 if (!protocol)
	    FrNoMemory("while parsing URL") ;
	 url = ptr + 1 ;
	 }
      else if (*ptr == '.' && (ptr != url || ptr[1] != '/'))
	 {
	 // hmm, looks like caller omitted the protocol and skipped right to
	 //   the hostname, so check if we can guess the protocol from the
	 //   first component of the hostname
	 bool is_file ;
	 protocol = guess_protocol_from_host(url,ptr,is_file) ;
	 if (is_file)
	    {
	    host = FrDupString("localhost") ;
	    got_host = true ;
	    }
	 else if (!bypass_proxies && FrGetURLProxyServer(protocol,host,port))
	    {
	    path = FrDupString(url) ;
	    return true ;
	    }
	 }
      else if (*ptr == '/' || *ptr == '\\' || *ptr == '.')
	 {
	 // hmm, looks like caller skipped right to path, so assume that it's
	 //   a local file
	 protocol = FrDupString("file") ;
	 host = FrDupString("localhost") ;
	 got_host = true ;
	 }
      else
	 {
	 // invalid URL!
	 return false ;
	 }
      bool got_path = false ;
      if (!got_host)
	 {
	 if (url[0] == '/' && url[1] == '/')
	    url += 2 ;
	 // scan for the start of the path
	 ptr = url ;
	 while (*ptr && (*ptr != '/' && *ptr != '\\'))
	    ptr++ ;
	 if (*ptr)
	    {
	    split_off_parameters(ptr,path,params) ;
	    got_path = true ;
	    }
	 // split the part between protocol and path into its constituents
	 const char *colon1 = 0 ;
	 const char *atsign = 0 ;
	 const char *colon2 = 0 ;
	 for (const char *p = url ; p < ptr ; p++)
	    {
	    if (*p == '@' && !atsign)
	       atsign = p ;
	    else if (*p == ':')
	       {
	       if (atsign)
		  {
		  if (!colon2)
		     colon2 = p ;
		  }
	       else if (!colon1)
		  colon1 = p ;
	       }
	    }
	 const char *hoststart = atsign ? atsign + 1 : url ;
	 const char *hostend = colon2 ? colon2 : ptr ;
	 host = FrDupStringN(hoststart, hostend - hoststart) ;
	 if (atsign)
	    {
	    if (colon1)
	       {
	       user = FrDupStringN(url,colon1 - url) ;
	       password = FrDupStringN(colon1 + 1, atsign - colon1 - 1) ;
	       }
	    else
	       user = FrDupStringN(url,atsign - url) ;
	    }
	 if (colon2)
	    port = atoi(colon2+1) ;
	 }
      if (!got_path)
	 split_off_parameters(url,path,params) ;
      if (port == 0)
	 {
	 if (!protocol)			// assume using WWW by default
	    protocol = FrDupString("http") ;
	 // assign the default port for the protocol, if known
	 if (Fr_stricmp(protocol,"http") == 0)
	    port = 80 ;
	 else if (Fr_stricmp(protocol,"https") == 0)
	    port = 443 ; // HTTP over SSL
	 else if (Fr_stricmp(protocol,"ftp") == 0)
	    port = 21 ;
	 else if (Fr_stricmp(protocol,"gopher") == 0)
	    port = 70 ;
	 else if (Fr_stricmp(protocol,"irc") == 0)
	    port = 194 ;
	 else if (Fr_stricmp(protocol,"mailto") == 0)
	    port = 25 ; // SMTP
	 else if (Fr_stricmp(protocol,"news") == 0 ||
		  Fr_stricmp(protocol,"nntp") == 0)
	    port = 119 ; // NNTP
	 else if (Fr_stricmp(protocol,"snews") == 0 ||
		  Fr_stricmp(protocol,"snntp") == 0)
	    port = 563 ; // NNTP over SSL
	 else if (Fr_stricmp(protocol,"telnet") == 0)
	    port = 23 ;
	 else if (Fr_stricmp(protocol,"wais") == 0)
	    port = 210 ; // Z3950
	 }
      return true ;
      }
   return false ;
}

//----------------------------------------------------------------------

bool FrFetchURL(const char *url, char *&content, size_t &content_length,
		  bool text_mode, bool bypass_proxies,
		  const char *post_data, const char *extra_HTTP_headers)
{
   char *protocol ;
   char *host ;
   int port ;
   char *path ;
   char *user ;
   char *password ;
   char *params ;
   bool success = FrParseURL(url,protocol,host,port,user,password,path,
			       params,bypass_proxies) ;
   if (success)
      {
      if (Fr_stricmp(protocol,"http") == 0 ||
	  Fr_stricmp(protocol,"https") == 0)
	 {
	 bool secure = (Fr_toupper(protocol[4]) == 'S') ;
	 success = FrFetchHTTP(host,port,path,params,secure,
			       content,content_length,user,password,
			       post_data,true,extra_HTTP_headers) ;
	 }
      else if (Fr_stricmp(protocol,"ftp") == 0)
	 {
	 success = FrFetchFTP(host,port,user,password,path,
			      content,content_length,text_mode) ;
	 }
      //else if (Fr_stricmp(protocol,"wais") == 0)
      //else if (Fr_stricmp(protocol,"gopher") == 0)
      //else if (Fr_stricmp(protocol,"news") == 0)
      //else if (Fr_stricmp(protocol,"snews") == 0)
      //else if (Fr_stricmp(protocol,"shttp") == 0)
      else if (Fr_stricmp(protocol,"file") == 0)
	 success = get_local_file(host,path,content,content_length) ;
      else // unrecognized protocol
	 success = false ;
      }
   FrFree(protocol) ;
   FrFree(host) ;
   FrFree(user) ;
   FrFree(password) ;
   FrFree(path) ;
   FrFree(params) ;
   return success ;
}

// end of file frgeturl.cpp //
