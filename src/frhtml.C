/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC								*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frhtml.cpp		low-level HTML-handling functions	*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 2001,2009,2011,2013					*/
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

#include <string.h>
#include "framerr.h"
#include "frctype.h"
#include "frlist.h"
#include "frstring.h"
#include "frsymtab.h"
#include "frurl.h"
#include "frutil.h"

/************************************************************************/
/*	Manifest Constants						*/
/************************************************************************/

#define HTML_TAG_START '<'
#define HTML_TAG_END   '>'

/************************************************************************/
/*	Types								*/
/************************************************************************/

struct HTMLSymbol
   {
   const char *name ;
   char value ;
   } ;
	
/************************************************************************/
/*	Global Data							*/
/************************************************************************/

static HTMLSymbol HTML_symbols[] =
   {
      { "nbsp",  ' ' },
      { "amp",   '&' },
      { "dquot", '"' },
      { "squot", '\'' },
      { "lt",    '<' },
      { "gt",    '>' },
      { "copy",  (char)169 },
      { "",	 '&' },
      { 0,	 0 },
   } ;

/************************************************************************/
/************************************************************************/

//----------------------------------------------------------------------

static bool is_comment_tag(const char *text)
{
   if (*text != HTML_TAG_START)
      return false ;
   text++ ;
   if (FrSkipWhitespace(text) != '!')
      return false ;
   if (text[1] != '-' || text[2] != '-')
      return false ;
   return true ;
}

//----------------------------------------------------------------------

static bool comment_tag_end(const char *text)
{
   if (text[0] != '-' || text[1] != '-')
      return false ;
   text += 2 ;
   return (FrSkipWhitespace(text) == HTML_TAG_END) ;
}

//----------------------------------------------------------------------

static const char *skip_comment(const char *text)
{
   if (is_comment_tag(text))
      {
      text += 4 ;			// skip the comment starter
      while (*text && !comment_tag_end(text))
	 text++ ;
      if (*text)
	 {
	 text = strchr(text,HTML_TAG_END) ;
	 if (text)
	    text++ ;
	 }
      }
   return text ;
}

//----------------------------------------------------------------------

char *FrStripHTMLComments(const char *text)
{
   if (!text)
      return 0 ;
   size_t len = strlen(text) ;
   char *stripped = FrNewN(char,len+1) ;
   if (stripped)
      {
      char *out = stripped ;
      while (*text)
	 {
	 const char *next = skip_comment(text) ;
	 if (next != text)
	    {
	    *out++ = ' ' ; // avoid concatenating stuff due to removed comments
	    text = next ;
	    }
	 *out++ = *text++ ;
	 }
      stripped = FrNewR(char,stripped,out-stripped+1) ;
      }
   else
      FrNoMemory("while stripping comments from HTML") ;
   return stripped ;
}

//----------------------------------------------------------------------

FrList *FrExtractHTMLTag(const char *&text)
{
   FrList *tag = 0 ;
   if (text && *text == HTML_TAG_START)
      {
      const char *next = skip_comment(text) ;
      if (next != text)
	 {
	 text = next ;
	 return 0 ;
	 }
      // extract this tag
      text++ ;			// skip initial left angle-bracket
      // first, scan to the end
      FrSkipWhitespace(text) ;
      const char *end = text ;
      while (*end && !Fr_isspace(*end) && *end != HTML_TAG_END)
	 end++ ;
      FrString tagname(text,end-text) ;
      tag = new FrList(FrCvt2Symbol(&tagname)) ;
      text = end ;
      while (*text && *text != HTML_TAG_END)
	 {
	 FrSkipWhitespace(text) ;
	 end = text ;
	 while (*end && *end != '=' && !Fr_isspace(*end) &&
		*end != HTML_TAG_END)
	    end++ ;
	 FrString *attrib = new FrString(text,end-text) ;
	 FrString *value = 0 ;
	 if (*end == '=')
	    {
	    // extract the value
	    text = end+1 ;
	    if (FrSkipWhitespace(text) == '"')
	       {
	       text++ ;
	       end = text ;
	       while (*end && *end != '"')
		  end++ ;
	       value = new FrString(text,end-text) ;
	       if (*end)
		  end++ ;
	       }
	    else
	       {
	       end = text ;
	       FrSkipToWhitespace(end) ;
	       value = new FrString(text,end-text) ;
	       if (*end)
		  end++ ;
	       }
	    }
	 pushlist(new FrList(FrCvt2Symbol(attrib),value),tag) ;
	 free_object(attrib) ;
	 text = end ;
	 }
      if (*text == HTML_TAG_END)
	 text++ ;
      }
   return listreverse(tag) ;
}

//----------------------------------------------------------------------

FrList *FrExtractHTMLTags(const char *text)
{
   FrList *tags = 0 ;
   if (text)
      {
      while (*text)
	 {
	 if (*text == HTML_TAG_START)
	    {
	    FrList *tag = FrExtractHTMLTag(text) ;
	    if (tag)
	       pushlist(tag,tags) ;
	    }
	 else
	    text++ ;
	 }
      }
   return listreverse(tags) ;
}

//----------------------------------------------------------------------

static const char *skip_protocol(const char *orig_URL)
{
   const char *URL = orig_URL ;
   while (*URL && (Fr_isalpha(*URL) || Fr_isdigit(*URL)))
      URL++ ;
   if (*URL == ':')
      {
      URL++ ;
      if (URL[0] == '/' && URL[1] == '/')
	 URL += 2 ;
      return URL ;
      }
   return orig_URL ;
}

//----------------------------------------------------------------------

static bool absolute_URL(const char *URL)
{
   if (*URL == '/')
      return true ;
   while (*URL && (Fr_isalpha(*URL) || Fr_isdigit(*URL)))
      URL++ ;
   if (*URL == ':')
      return true ;
   return false ;
}

//----------------------------------------------------------------------

static FrString *URL_directory(const char *URL)
{
   FrString *base = 0 ;
   if (URL)
      {
      // remove the last component of the URL's path and make that the base
      const char *last_slash = strrchr(URL,'/') ;
      if (last_slash)
	 last_slash++ ;
      else
	 last_slash = strchr(URL,'\0') ;
      return new FrString(URL,last_slash-URL) ;
      }
   return base ;
}

//----------------------------------------------------------------------

static FrString *build_URL(const char *thisURL, const char *base,
			   FrString *href)
{
   if (!href)
      return 0 ;
   const char *href_string = (char*)href->stringValue() ;
   if (absolute_URL(href_string))
      return (FrString*)href->deepcopy() ;
   FrString *new_URL ;
   if (*href_string == '#')
      new_URL = new FrString(thisURL) ;
   else if (base)
      new_URL = new FrString(base) ;
   else
      new_URL = URL_directory(thisURL) ;
   *new_URL += *href ;
   return new_URL ;
}

//----------------------------------------------------------------------

inline bool invalid_URL_char(char c)
{
   return !Fr_isspace(c) && c != '%' && c != '~' && c != '\\' ;
}

//----------------------------------------------------------------------

static FrString *extract_URL(const char *text)
{
   if (!text)
      return 0 ;
   const char *end = text ;
   while (*end && !invalid_URL_char(*end))
      end++ ;
   return new FrString(text,end-text) ;
}

//----------------------------------------------------------------------

FrList *FrExtractHREFs(const char *text, const char *URL, bool subrealm_only)
{
   FrList *tags = FrExtractHTMLTags(text) ;
   FrList *hrefs = 0 ;
   if (tags)
      {
      // filter the tags to get just the <A HREF=...> tags, and then extract
      //   the destination URL
      FrSymbol *symBASE = FrSymbolTable::add("BASE") ;
      FrSymbol *symA = FrSymbolTable::add("A") ;
      FrSymbol *symHREF = FrSymbolTable::add("HREF") ;
      char *base = 0 ;
      while (tags)
	 {
	 FrList *tag = (FrList*)poplist(tags) ;
	 if (tag)
	    {
	    if (tag->first() == symBASE)
	       {
	       FrList *href = (FrList*)tag->assoc(symHREF) ;
	       if (href)
		  {
		  FrFree(base) ;
		  FrString *b = (FrString*)href->second() ;
		  base = b ? FrDupString((char*)b->stringValue()) : 0 ;
		  }
	       }
	    else if (tag->first() == symA)
	       {
	       FrList *href = (FrList*)tag->assoc(symHREF) ;
	       if (href)
		  {
		  FrString *newURL = build_URL(URL,base,
					       (FrString*)href->second());
		  if (!hrefs->member(newURL,::equal))
		     pushlist(newURL,hrefs) ;
		  else
		     free_object(newURL) ;
		  }
	       }
	    tag->freeObject() ;
	    }
	 }
      FrFree(base) ;
      }
   else
      {
      // scan for full URLs
      const char *txt = text ;
      while ((txt = Fr_stristr(txt,"http:")) != 0)
	 pushlist(extract_URL(txt),hrefs) ;
      txt = text ;
      while ((txt = Fr_stristr(txt,"https:")) != 0)
	 pushlist(extract_URL(txt),hrefs) ;
      txt = text ;
      while ((txt = Fr_stristr(txt,"ftp:")) != 0)
	 pushlist(extract_URL(txt),hrefs) ;
      }
   if (subrealm_only)
      {
      FrList *candidates = hrefs ;
      hrefs = 0 ;
      FrString *base = URL_directory(URL) ;
      const char *base_string = 0 ;
      size_t base_length = 0 ;
      if (base)
	 {
	 base_string = skip_protocol((char*)base->stringValue()) ;
	 base_length = base_string ? strlen(base_string) : 0 ;
	 }
      while (candidates)
	 {
	 FrString *href = (FrString*)poplist(candidates) ;
	 if (!href)
	    continue ;
	 const char *href_string = skip_protocol((char*)href->stringValue()) ;
	 if (base_string && Fr_strnicmp(base_string,href_string,
					base_length) == 0)
	    pushlist(href,hrefs) ;
	 else
	    href->freeObject() ;
	 }
      free_object(base) ;
      }
   else
      hrefs = listreverse(hrefs) ;
   return hrefs ;
}

//----------------------------------------------------------------------

static void add_wrapped_blank(char *&out, size_t &column, size_t wrap_column)
{
   if (column >= wrap_column)
      {
      *out++ = '\n' ;
      column = 0 ;
      }
   else
      {
      *out++ = ' ' ;
      column++ ;
      }
   return ;
}

//----------------------------------------------------------------------

#define start_line(out,column) \
        { if (column > 0) { *out++ = '\n' ; column = 0 ; }}

char *FrStripHTMLMarkup(const char *html, size_t wrap_column)
{
   char *text = FrDupString(html) ;
   if (!text)
      {
      FrNoMemory("while stripping markup from HTML text") ;
      return 0 ;
      }
   char *out = text ;
   size_t column = 0 ;
   FrSymbol *symP = FrSymbolTable::add("P") ;
   FrSymbol *symSlashP = FrSymbolTable::add("/P") ;
   FrSymbol *symLI = FrSymbolTable::add("LI") ;
   FrSymbol *symBR = FrSymbolTable::add("BR") ;
   FrSymbol *symSlashTR = FrSymbolTable::add("/TR") ;
   FrSymbol *symH1 = FrSymbolTable::add("H1") ;
   FrSymbol *symSlashH1 = FrSymbolTable::add("/H1") ;
   FrSymbol *symH2 = FrSymbolTable::add("H2") ;
   FrSymbol *symSlashH2 = FrSymbolTable::add("/H2") ;
   FrSymbol *symH3 = FrSymbolTable::add("H3") ;
   FrSymbol *symSlashH3 = FrSymbolTable::add("/H3") ;
   FrSymbol *symH4 = FrSymbolTable::add("H4") ;
   FrSymbol *symSlashH4 = FrSymbolTable::add("/H4") ;
   FrSymbol *symHR = FrSymbolTable::add("HR") ;
   FrSymbol *symIMG = FrSymbolTable::add("IMG") ;
   FrSymbol *symALT = FrSymbolTable::add("ALT") ;
   while (*html)
      {
      if (Fr_isspace(*html))
	 {
	 add_wrapped_blank(out,column,wrap_column) ;
	 while (*html && Fr_isspace(*html))
	    html++ ;
	 }
      else if (*html == HTML_TAG_START)
	 {
	 FrList *tag = FrExtractHTMLTag(html) ;
	 if (tag)
	    {
	    // see whether we need to apply any formatting to output
	    FrSymbol *tagname = (FrSymbol*)tag->first() ;
	    if (tagname == symP || tagname == symSlashP || tagname == symH1 ||
		tagname == symH2 || tagname == symSlashH1)
	       {
	       start_line(out,column) ;
	       *out++ = '\n' ;
	       }
	    else if (tagname == symBR || tagname == symSlashTR ||
		     tagname == symH3 || tagname == symH4 ||
		     tagname == symSlashH2 || tagname == symSlashH3 ||
		     tagname == symSlashH4)
	       {
	       start_line(out,column) ;
	       }
	    else if (tagname == symHR)
	       {
	       start_line(out,column) ;
	       for (size_t i = 0 ; i < 5 ; i++)
		  *out++ = '-' ;
	       *out++ = '\n' ;
	       }
	    else if (tagname == symLI)
	       {
	       start_line(out,column) ;
	       *out++ = ' ' ;
	       *out++ = '*' ;
	       *out++ = ' ' ;
	       column = 3 ;
	       }
	    else if (tagname == symIMG)
	       {
	       // extract the ALT tag, if present, and insert it as the text
	       FrList *alt = (FrList*)tag->assoc(symALT) ;
	       if (alt)
		  {
		  FrString *altname = (FrString*)alt->second() ;
		  *out++ = '[' ;
		  strcpy(out,(char*)altname->stringValue()) ;
		  out = strchr(out,'\0') ;
		  *out++ = ']' ;
		  }
	       }
	    else if (out > text && !Fr_isspace(out[-1]))
               // treat the tag as though it had been a blank
	       add_wrapped_blank(out,column,wrap_column) ;
	    }
	 else // it was a comment, so insert a blank
	    add_wrapped_blank(out,column,wrap_column) ;
	 }
      else if (*html == '&')
	 {
	 html++ ;
	 // replace a named symbol by its character value
	 size_t len = 0 ;
	 while (len < 10 && html[len] != ';' && !Fr_isspace(html[len]))
	    len++ ;
	 bool matched = false ;
	 for (const HTMLSymbol *sym = HTML_symbols ; sym->name ; sym++)
	    {
	    if (Fr_strnicmp(html,sym->name,len) == 0 &&
		sym->name[len] == '\0')
	       {
	       *out++ = sym->value ;
	       html += len ;
	       column++ ;
	       if (*html == ';')
		  html++ ;
	       matched = true ;
	       break ;
	       }
	    }
	 if (!matched)
	    {
	    *out++ = '&' ;
	    column++ ;
	    }
	 }
      else
	 {
	 *out++ = *html++ ;
	 column++ ;
	 }
      }
   *out++ = '\0' ;
   return FrNewR(char,text,out-text+1) ;
}

//----------------------------------------------------------------------

// end of file frhtml.cpp //
