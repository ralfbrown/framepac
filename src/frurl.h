/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC								*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frurl.h		URL-handling declarations		*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 2000,2001,2003,2009,2010				*/
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

#ifndef _FRURL_H_INCLUDED
#define _FRURL_H_INCLUDED

#ifndef __FRCOMMON_H_INCLUDED
#include "frcommon.h"
#endif

#ifndef __FRLIST_H_INCLUDED
#include "frlist.h"
#endif

#ifndef __FRSCKSTR_H_INCLUDED
#include "frsckstr.h"
#endif

/************************************************************************/
/************************************************************************/

// low-level URL processing
char *FrURLEncode(const char *string) ; 		  // use FrFree(retval)
char *FrURLEncode(const char *string, size_t stringlen) ; // use FrFree(retval)
char *FrURLDecode(const char *string) ; 		  // use FrFree(retval)
char *FrURLDecode(const char *string, size_t stringlen) ; // use FrFree(retval)

char *FrBase64Encode(const char *string) ; 		  // use FrFree(retval)
char *FrBase64Encode(const char *string, size_t strlen) ; // use FrFree(retval)

bool FrGetURLProxyServer(const char *protocol, char *&host, int &port) ;
bool FrParseURL(const char *url, char *&protocol, char *&host, int &port,
		char *&user, char *&password, char *&path, char *&params,
		bool bypass_proxies = false) ;
   // [free all returned strings with FrFree() when done]

// low-level retrieval functions
void FrSetHTTPUserAgent(const char *agent) ;
const char *FrGetHTTPUserAgent() ;
bool FrFetchHTTP(FrSocket s, const char *server, int port, const char *path,
		 const char *params, bool secure,
		 char *&content, size_t &content_length,
		 const char *username = 0, const char *password = 0,
		 const char *post_data = 0, bool skip_header = true,
		 const char *extra_headers = 0) ;
bool FrFetchHTTP(const char *server, int port, const char *path,
		 const char *params, bool secure,
		 char *&content, size_t &content_length,
		 const char *username = 0, const char *password = 0,
		 const char *post_data = 0, bool skip_header = true,
		 const char *extra_headers = 0) ;
bool FrFetchFTP(const char *server, int port, const char *user,
		const char *password, const char *path,
		char *&content, size_t &content_length,
		bool text_mode = false) ;

// the top-level retrieval function
bool FrFetchURL(const char *url, char *&content, size_t &content_length,
		bool text_mode = false, bool bypass_proxies = false,
		const char *post_data = 0,
		const char *extra_HTTP_headers = 0) ;
       // (note: 'post_data' is only relevant for HTTP/HTTPS)

// some simple HTML-handling functions
char *FrStripHTMLComments(const char *text) ;
FrList *FrExtractHTMLTags(const char *text) ;
FrList *FrExtractHREFs(const char *text, const char *URL,
		       bool same_subrealm_only = false) ;
char *FrStripHTMLMarkup(const char *html, size_t wrap_column) ;

#endif /* !_FRURL_H_INCLUDED */

// end of file frurl.h //
