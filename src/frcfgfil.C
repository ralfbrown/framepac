/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frcfgfil.cpp		configuration file processing		*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,1998,2000,2001,2002,2003,2004	*/
/*		2005,2006,2008,2009,2010,2011,2012,2015			*/
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

#if defined(__GNUC__)
#  pragma implementation "frcfgfil.h"
#endif

#include "framerr.h"
#include "frctype.h"
#include "frcfgfil.h"
#include "frfilutl.h"   // for FrMAX_LINE
#include "frlist.h"
#include "frnumber.h"
#include "frprintf.h"
#include "frreader.h"
#include "frstring.h"
#include "frsymtab.h"
#include "frutil.h"	// for FrSkipWhitespace

#ifdef FrSTRICT_CPLUSPLUS
# include <cstdio>
# include <cstdlib>
# include <fstream>
# include <iomanip>
#else
# include <iomanip.h>
# include <fstream.h>
# include <stdio.h>
# include <stdlib.h>
#endif /* FrSTRICT_CPLUSPLUS */

// don't do lots of inlining here -- this is not speed-critical code
#if defined(__WATCOMC__)
#pragma inline_depth 1 ;
#elif defined(_MSC_VER)
#pragma inline_depth(1)
#endif /* __WATCOMC__, _MSC_VER */

/************************************************************************/
/*    Manifest Constants						*/
/************************************************************************/


/************************************************************************/
/*	Global Data 							*/
/************************************************************************/

// order of names must match enum FrConfigVariableType
const char *FrConfigVariableTypeNames[] =
   {
      "integer",
      "cardinal",
      "real",
      "base directory",
      "filename",
      "list of filenames",
      "string",
      "option flags",
      "generic list",
      "association list",
      "symbol",
      "list of symbols",
      "Yes/No",
      "enumerated keyword",
      "invalid entry",
      "programmer-defined type"
   } ;

bool FrConfiguration::in_startup = true ;

/************************************************************************/
/*	Global Data for this module					*/
/************************************************************************/

static const char alloc_string_msg[] = "allocating string in FrConfiguration" ;
static const char unterm_string_msg[] = "unterminated string" ;
static const char expected_number_msg[] = "expected a number" ;
static const char expected_string_msg[] = "expected a string" ;
static const char value_out_of_range_msg[]
	= "value out of range -- adjusted to" ;

static const char default_base_dir[] = "" ;

/************************************************************************/
/*	Helper functions						*/
/************************************************************************/

static char *end_of_keyword(const char *start)
{
   char *kwend = (char*)strchr(start,' ') ;
   if (!kwend)
      kwend = (char*)strchr(start,'\0') ;
   return kwend ;
}

/************************************************************************/
/************************************************************************/

FrConfigurationTable *FrConfiguration::findParameter(const char *param) const
{
   FrConfigurationTable *ct = curr_state ;
   if (!param)
      {
      while (ct->keyword)
	 ct++ ;
      return ct ;
      }
   while (ct->keyword)
      {
      if (Fr_stricmp(ct->keyword,param) == 0)
	 return ct ;
      ct++ ;
      }
   return 0 ;
}

//-----------------------------------------------------------------------
//	Let user know about serious but non-fatal condition

void FrConfiguration::warn(const char *message) const
{
   FrWarningVA("Configuration error (%s line %d): %s",
	       input_filename ? input_filename : "",
	       current_line,message) ;
   return ;
}

//-----------------------------------------------------------------------
//	Let user know about serious but non-fatal condition

void FrConfiguration::warn(const char *message,const char *where,
			   long value) const
{
   FrWarningVA("Configuration error (%s line %d, %s): %s %ld",
	       input_filename ? input_filename : "",
	       current_line,where,message,value) ;
   return ;
}

//-----------------------------------------------------------------------
//	Let user know about serious but non-fatal condition

void FrConfiguration::warn(const char *message,const char *where,
			   size_t value) const
{
   FrWarningVA("Configuration error (%s line %d, %s): %s %lu",
	       input_filename ? input_filename : "",
	       current_line,where,message,(unsigned long)value) ;
   return ;
}

//-----------------------------------------------------------------------
//	Let user know about serious but non-fatal condition

void FrConfiguration::warn(const char *message,const char *where,
			   double value) const
{
   FrWarningVA("Configuration error (%s line %d, %s): %s %g",
	       input_filename ? input_filename : "",
	       current_line,where,message,value) ;
   return ;
}

//-----------------------------------------------------------------------

void FrConfiguration::warn(const char *message, const char *text) const
{
   FrWarningVA("Configuration error (%s line %d): %s -- %s",
	       input_filename ? input_filename : "",
	       current_line,message,text) ;
   return ;
}

//-----------------------------------------------------------------------

void FrConfiguration::warn(const char *message, const char *keyword,
			   const char *values) const
{
   FrWarningVA("Configuration error (%s line %d): %s -- %s %s",
	       input_filename ? input_filename : "",
	       current_line,message,keyword,values) ;
   return ;
}

//-----------------------------------------------------------------------
//	Define the start of a comment line

inline bool comment_start(char ch)
{
   return (ch == ';' || ch == '#') ;
}

//-----------------------------------------------------------------------

static bool comment_line(char *line)
{
   char *l = line ;

   while (l && *l && *l != '\n' && Fr_isspace(*l))
      l++ ;
   return comment_start(*l) ;
}

//-----------------------------------------------------------------------

static int non_comment_size(char *line, int size)
{
   bool in_string = false ;
   bool nonwhite = false ;
   char terminator = '"' ;
   register char c ;

   for (int i = 0 ; i < size ; i++)
      {
      c = line[i] ;
      if (in_string)
	 {
	 if (c == terminator)
	    in_string = false ;
	 }
      else if (c == '"' || c == '\'' || c == '`')
	 {
	 in_string = true ;
	 nonwhite = true ;
	 terminator = c ;
	 }
      else if (c == ';' || c == '#' || c == '\n' || c == '\0')
	 {
	 size = i ;
	 break ;
	 }
      else if (!Fr_isspace(c))
	 nonwhite = true ;
      }
   if (nonwhite)
      {
      while (size > 0 && Fr_isspace(line[size-1]))
	 size-- ;
      return size ;
      }
   return 0 ;
}

//-----------------------------------------------------------------------

void *FrCfgLoc(const FrConfiguration *cfg,
	       const FrConfigurationTable *entry)
{
   if ((size_t)entry->location > cfg->lastLocalVar())
      return entry->location ;
   else if (entry->location)
      {
      char *location = ((char*)cfg) + (size_t)entry->location ;
      return VoidPtr(location) ;
      }
   else
      return 0 ;
}

//-----------------------------------------------------------------------

FrConfiguration::FrConfiguration()
{
   valid = false ;
   curr_state = 0 ;
   current_line = 0 ;
   base_dir = FrNewN(char,sizeof(default_base_dir)) ;
   memcpy(base_dir,default_base_dir,sizeof(default_base_dir)) ;
   input_filename = 0 ;
   return ;
}

//-----------------------------------------------------------------------

FrConfiguration::FrConfiguration(const char *base_directory)
{
   valid = false ;
   curr_state = 0 ;
   current_line = 0 ;
   if (!base_directory)
      base_directory = default_base_dir ;
   size_t len = strlen(base_directory)+1 ;
   base_dir = FrNewN(char,len) ;
   memcpy(base_dir,base_directory,len) ;
   input_filename = 0 ;
   return ;
}

//-----------------------------------------------------------------------

FrConfiguration::~FrConfiguration()
{
   valid = false ;
   FrFree(base_dir) ;
   base_dir = 0 ;
   FrFree(input_filename) ;
   input_filename = 0 ;
   return ;
}

//-----------------------------------------------------------------------

static unsigned long defaultFlags(const FrCommandBit *bits)
{
   unsigned long value = 0 ;
   while (bits && bits->name)
      {
      if (bits->default_value)
	 value |= bits->bit ;
      bits++ ;
      }
   return value ;
}

//-----------------------------------------------------------------------

void FrConfiguration::init()
{
   resetState() ;
   if (curr_state != 0)
      {
      for (size_t i = 0 ; curr_state[i].keyword ; i++)
	 {
	 void *loc = FrCfgLoc(this,&curr_state[i]) ;
	 if (loc)
	    {
	    bool empty_default
	       = (curr_state[i].default_value && !*curr_state[i].default_value);
	    FrConfigVariableType vartype = curr_state[i].vartype ;
	    switch (vartype)
	       {
	       case integer:
		  if (!empty_default) *((long int *)loc) = 0 ;
		  break ;
	       case cardinal:
		  if (!empty_default) *((size_t *)loc) = 0 ;
		  break ;
	       case real:
		  if (!empty_default) *((double*)loc) = 0 ;
		  break ;
	       case basedir:
	       case filename:
	       case cstring:
		  *((char**)loc) = 0 ;
		  break ;
	       case list:
	       case assoclist:
	       case filelist:
	       case symlist:
		  (*(FrObject**)loc) = 0 ;
		  break ;
	       case symbol:
		  (*(FrSymbol**)loc) = 0 ;
		  break ;
	       case bitflags:
		  (*(unsigned long*)loc)
		     = defaultFlags((FrCommandBit*)curr_state[i].extra_args) ;
		  break ;
	       default:
		  if ((size_t)vartype <= maxUserType())
		     initUserType(vartype,loc) ;
		  break ;
	       }
	    if (curr_state[i].default_value &&
		setParameter(curr_state[i].default_value,&curr_state[i],true,
			     &cerr))
	       onChange(vartype,FrCfgLoc(this,&curr_state[i])) ;
	    else if (vartype == bitflags)
	       onChange(vartype,FrCfgLoc(this,&curr_state[i])) ;
	    }
	 }
      }
   return ;
}

//-----------------------------------------------------------------------

void FrConfiguration::startupComplete()
{
   in_startup = false ;
   return ;
}

//-----------------------------------------------------------------------

void FrConfiguration::beginningShutdown()
{
   in_startup = true ;
   return ;
}

//-----------------------------------------------------------------------

void FrConfiguration::initUserType(FrConfigVariableType, void * /* location */)
{
   return ;				// override in derived classes
}

//-----------------------------------------------------------------------

void FrConfiguration::freeValues()
{
   if (curr_state != 0)
      {
      for (size_t i = 0 ; curr_state[i].keyword ; i++)
	 {
	 void *loc = FrCfgLoc(this,&curr_state[i]) ;
	 if (loc)
	    {
	    FrConfigVariableType vartype = curr_state[i].vartype ;
	    switch (vartype)
	       {
	       case integer:
	       case cardinal:
	       case real:
	       case bitflags:
	       case symbol:
	       case yesno:
	       case keyword:
	       case invalid:
		  // do nothing
		  break ;
	       case basedir:
	       case filename:
	       case cstring:
		  FrFree(*(char**)loc) ;
		  *((char**)loc) = 0 ;
		  break ;
	       case list:
	       case assoclist:
	       case symlist:
	       case filelist:
		  free_object(*(FrObject**)loc) ;
		  (*(FrObject**)loc) = 0 ;
		  break ;
	       default:
		  if ((size_t)vartype <= maxUserType())
		     {
//!!!
		     }
		  break ;
	       }
	    }
	 }
      }
   return ;
}

//-----------------------------------------------------------------------

char *FrConfiguration::getLine(istream &input, bool check_for_section_header)
{
   if (!input.good() || input.eof())	// quit immediately if already at EOF
      return 0 ;
   size_t max_line = maxline() ;
   char *line ;
   if ((line = FrNewN(char,max_line+1)) == 0)
      {
      FrNoMemory("in configuration parser") ;
      return 0 ;
      }
   int count = 0 ;
   // go read the line into the buffer we've just allocated, expanding it as
   // needed
   bool first = true ;
   char nextch ;
   do {
      line[count] = '\0' ;
      input.getline(line+count,max_line,'\n') ;
      current_line++ ;
      if (count == 0)
	 {
	 char *lineptr = line ;
	 if (FrSkipWhitespace(lineptr) == '[' && strchr(lineptr,']') != 0)
	    {
	    if (check_for_section_header)
	       {
	       FrFree(line) ;
	       return 0 ;
	       }
	    else
	       {
	       nextch = '!' ;
	       continue ;
	       }
	    }
	 }
      line[count+max_line] = '\0' ;
      if (line[count] && !comment_line(line+count))
         {
	 int noncom = non_comment_size(line+count,input.gcount()) ;
	 if (noncom > 0)
	    {
	    count += noncom ;
	    first = false ;
	    char *newbuf = FrNewR(char,line,count+max_line+1) ;
	    if (newbuf)
	       line = newbuf ;
	    else
	       {
	       FrNoMemory("in configuration parser") ;
	       line[count] = '\0' ;
	       return line ;
	       }
	    }
         }
      if (input.eof())
         {
	 first = false ;
	 nextch = '!' ;
         }
      else
         nextch = trunc2char(input.peek()) ;
      } while (first || Fr_isspace(nextch) || comment_start(nextch)) ;
   input.clear() ;			// reset possible error on EOF
   line[count] = '\0' ;
   if (count)				// the realloc here always succeeds
      return (char*)FrRealloc(line,count+1,true) ; // since it reduces alloc
   else
      {
      FrFree(line) ;
      return 0 ;
      }
}

//-----------------------------------------------------------------------

void FrConfiguration::skipws(const char *&str) const
{
   while (*str && (Fr_isspace(*str) || *str == '\n'))
      str++ ;
   return ;
}

//-----------------------------------------------------------------------

void FrConfiguration::skipws_comma(const char *&input_line) const
{
   skipws(input_line) ;
   if (*input_line == ',')
      {
      input_line++ ;
      skipws(input_line) ;
      }
   return ;
}

//-----------------------------------------------------------------------

bool FrConfiguration::skipto(char what, const char *&line, const char *errmsg)
const
{
   skipws(line) ;
   if (*line == '\0' || *line++ != what)
      {
      if (errmsg)
	 warn(errmsg) ;
      return false ;
      }
   skipws(line) ;
   return true ;
}

//-----------------------------------------------------------------------

static const char *extract_keyword(char *&str)
{
   char *start, *end ;

   if (str)
      {
      // find first occurrence of either an equal sign or a colon, which
      // can be used interchangeably to delimit the keyword
      char *end2 = strchr(str,'=') ;
      end = strchr(str,':') ;
      if (!end || (end2 && end2 < end))
	 end = end2 ;
      if (end)
	 {
	 *end = '\0' ;
	 start = str ;
	 str = end+1 ;
	 // strip trailing whitespace from keyword
	 while (end > start && Fr_isspace(end[-1]))
	    *--end = '\0' ;
	 return start ;
	 }
      else
	 return "" ;
      }
   else
      return 0 ;
}

//-----------------------------------------------------------------------

static bool include_file(const char *line, FrConfiguration *config,
			   bool &good_include, char *&file_name)
{
   file_name = 0 ;
   if (!line)
      return false ;
   while (*line && Fr_isspace(*line))
      line++ ;
   good_include = true ;
   if (strncmp(line,"!include",8) == 0)
      {
      line += 8 ;
      char *filename = 0 ;
      config->parseFilename(line,&filename,false) ;
      if (filename && *filename)
	 {
	 FrConfigurationTable *state = config->currentState() ;
	 size_t current_line = config->lineNumber() ;
	 good_include = config->load(filename,0,false) ;
	 config->setState(state,current_line) ;
	 }
      file_name = filename ;
      return true ;
      }
   else
      return false ;
}

//-----------------------------------------------------------------------

bool FrConfiguration::skipToSection(istream &infile, const char *section,
				    bool from_start)
{
   if (from_start)
      {
      current_line = 0 ;
      infile.seekg(0) ;
      }
   if (section && *section)
      {
      size_t seclen = strlen(section) ;
      while (!infile.eof() && !infile.fail())
	 {
	 char line[FrMAX_LINE] ;
	 infile.getline(line,sizeof(line),'\n') ;
	 current_line++ ;
	 char *lineptr = line ;
	 if (FrSkipWhitespace(lineptr) == '[')
	    {
	    lineptr++ ;
	    char *end = strrchr(lineptr,']') ;
	    if (end)
	       {
	       size_t len = end - lineptr ;
	       if (len == seclen && Fr_strnicmp(section,lineptr,len) == 0)
		  return true ;
	       }
	    }
	 }
      }
   return false ;
}

//-----------------------------------------------------------------------

bool FrConfiguration::setParameter(const char *param_name,
				     const char *new_value,
				     ostream *err)
{
   FrConfigurationTable *param = findParameter(param_name) ;
   if (param)
      {
      bool set = setParameter(new_value,param,true,err) ;
      if (set)
	 onChange(param->vartype,FrCfgLoc(this,param)) ;
      return set ;
      }
   else
      {
      if (err)
	 (*err) << "Unknown configuration parameter " << param_name << endl ;
      return false ;
      }
}

//-----------------------------------------------------------------------

bool FrConfiguration::setParameter(const char *new_value,
				     const FrConfigurationTable *param,
				     bool initializing, ostream *err)
{
   if (!param)
      return false ;
   if (in_startup &&
       (param->settability & FrSET_STARTUP) == 0 &&
       (param->settability != FrSET_READONLY || !initializing))
      {
      if (!param->keyword || !*param->keyword)
	 return true ;
      if (err)
	 (*err) << "Configuration parameter " << param->keyword
		<< " may not be set at program startup" << endl ;
      return false ;
      }
   if (!in_startup && (param->settability & FrSET_RUNTIME) == 0)
      {
      if (!param->keyword || !*param->keyword)
	 return true ;
      if (err)
	 (*err) << "Configuration parameter " << param->keyword
		<< " may not be changed during the program's run" << endl ;
      return false ;
      }
   FrConfigVariableType vartype = param->vartype ;
   void *loc = FrCfgLoc(this,param) ;
   switch (vartype)
      {
      case integer:
	 return parseInteger(new_value,param,initializing) ;
      case cardinal:
	 return parseCardinal(new_value,param,initializing) ;
      case real:
	 return parseReal(new_value,param,initializing) ;
      case basedir:
	 return parseBasedir(new_value,loc,initializing) ;
      case filename:
	 return parseFilename(new_value,loc,initializing) ;
      case filelist:
	 return parseFilelist(new_value,loc,param->extra_args) ;
      case cstring:
	 return parseString(new_value,loc,initializing) ;
      case bitflags:
	 return parseBitflags(new_value,loc,param->extra_args) ;
      case list:
	 return parseList(new_value,loc,param->extra_args) ;
      case assoclist:
	 return parseAssoclist(new_value,loc,param->extra_args) ;
      case symbol:
	 return parseSymbol(new_value,loc,param->extra_args) ;
      case symlist:
	 return parseSymlist(new_value,loc,param->extra_args) ;
      case yesno:
	 return parseYesno(new_value,loc,param->extra_args) ;
      case keyword:
	 return parseKeyword(new_value,loc,param) ;
      case invalid:
	 return parseInvalid(new_value,loc,param->extra_args) ;
      default:
	 if ((size_t)vartype <= maxUserType())
	    return parseUser(vartype,new_value,loc,param->extra_args) ;
	 else
	    {
	    if (err)
	       (*err) << "; unsupported user type encountered in "
		  	 "configuration file near\n"
		      << ";\t" << param->keyword << ": " << new_value << endl ;
	    return false ;
	    }
      }
}

//----------------------------------------------------------------------

bool FrConfiguration::parseLine(istream &infile, bool single_section_only,
				bool skip_unknowns)
{
   char *input_line, *line ;
   const char *keyword ;

   line = input_line = getLine(infile,single_section_only) ;
   if (!line)
      {
      FrFree(input_line) ;
      return false ;
      }
   bool good_include ;
   char *include_filename ;
   if (include_file(line,this,good_include,include_filename))
      {
      if (!good_include)
	 FrWarningVA("error encountered while including file\n"
		     "\t'%s' into configuration",include_filename) ;
      FrFree(input_line) ;
      FrFree(include_filename) ;
      return true ;
      }
   keyword = extract_keyword(line) ;
   FrConfigurationTable *ct = findParameter(keyword) ;
   if (ct && ct->keyword)		// did we find a keyword match?
      {
      if (!setParameter(line,ct,false,&cerr))
	 {				// parse function signalled fatal error
	 FrFree(input_line) ;
	 return false ;
	 }
      onChange(ct->vartype,FrCfgLoc(this,ct)) ;
      }
   else if (!keyword)			// did we hit the end of the stream?
      {
      if (ct && setParameter(line,ct,false,&cerr))
	 onChange(ct->vartype,FrCfgLoc(this,ct)) ;
      FrFree(input_line) ;
      return false ;			// tell caller we're done with stream
      }
   else if (!skip_unknowns)		// unknown keyword
      {
      warn("unknown keyword",keyword) ;
      FrFree(input_line) ;
      return true ;
      }
   FrFree(input_line) ;
   if (ct && ct->next_state)
      curr_state = ct->next_state ;
   return true ;
}

//-----------------------------------------------------------------------

bool FrConfiguration::load(istream &input, const char *section_name,
			   bool reset)
{
   valid = false ;
   if (reset)

      init() ;			// reset data to default values
   resetState() ;		// ensure starting with the correct parse table
   current_line = 0 ;
   if (reset || !section_name)
      {
      // read common data before first marker, but only if we're setting the
      //   config from scratch or are not using sections -- otherwise, we may
      //   clobber an override of a common item that was previously loaded
      //   inside a different section
      while (parseLine(input,true,true))
	 valid = true ;
      }
   if (section_name)
      {
      if (skipToSection(input,section_name) && !reset)
	 valid = true ;			// allow an empty section if add-on
      }
   while (parseLine(input,section_name != 0,false))
      valid = true ;			// we have valid data in config file
   resetState() ;
   return good() ;
}

//-----------------------------------------------------------------------

bool FrConfiguration::load(const char *file_name, const char *section_name,
			   bool reset)
{
   char *orig_input_filename ;
   if (reset)
      {
      FrFree(input_filename) ;
      orig_input_filename = 0 ;
      }
   else
      orig_input_filename = input_filename ;
   input_filename = FrDupString(file_name) ;
   ifstream *in = new ifstream(file_name) ;
   if (in && in->good())
      {
      load(*in,section_name,reset) ;
      in->close() ;
      }
   else
      valid = false ;
   delete in ;
   if (!reset)
      {
      FrFree(input_filename) ;
      input_filename = orig_input_filename ;
      }
   return good() ;
}

//-----------------------------------------------------------------------

bool FrConfiguration::loadRaw(istream &input, const char *section,
			      FrList *&params)
{
   bool have_section = false ;
   if (input.good())
      {
      if (skipToSection(input,section))
	 {
	 while (!input.eof() && !input.fail())
	    {
	    have_section = true ;
	    char *line = getLine(input,true) ;
	    if (line)
	       {
	       char *input_line = line ;
	       const char *keyword = extract_keyword(line) ;
	       FrObject *value = string_to_FrObject(line) ;
	       FrList *param = new FrList(new FrString(keyword),value) ;
	       pushlist(param,params) ;
	       FrFree(input_line) ;
	       }
	    else if (!skipToSection(input,section,false))
	       break ;
	    }
	 }
      input.clear() ;
      }
   return have_section ;
}

//-----------------------------------------------------------------------

bool FrConfiguration::loadRaw(const char *filename, const char *section,
			      FrList *&params)
{
   params = 0 ;
   if (filename && *filename)
      {
      ifstream *input = new ifstream(filename) ;
      if (input && input->good())
	 {
	 bool have_section = loadRaw(*input,section,params) ;
	 input->close() ;
	 delete input ;
	 return have_section ;
	 }
      }
   return false ;
}

//-----------------------------------------------------------------------

bool FrConfiguration::parseInvalid(const char *input_line,
				   void * /*location*/,
				   void * /*extra*/)
{
   (void)input_line ;
   warn("unknown keyword; line skipped") ;
   return true ;
}

//-----------------------------------------------------------------------

bool FrConfiguration::parseInteger(const char *input_line,
				     const FrConfigurationTable *param,
				     bool initializing)
{
   if (initializing && input_line && !*input_line)
      {
      // keep whatever value was in that location, it's been statically
      //   initialized
      return true ;
      }
   long value ;
   char *rest ;
   long *location = (long*)FrCfgLoc(this,param) ;
   if (input_line == 0 || location == 0)
      return true ;			// quit immed if no input or output
   skipws(input_line) ;
   value = strtol(input_line,&rest,10) ;
   if (rest && rest != input_line)
      input_line = rest ;
   else
      {
      warn(expected_number_msg) ;
      return false ;
      }
   // if a default value is specified, we're allowed to use it even if
   //   it falls outside the min-max range
   if (param->default_value && *param->default_value &&
       strtol(param->default_value,&rest,10) == value)
      {
      *location = value ;
      return true ;
      }
   // if min and/or max are given, force the value to be inside the specified
   //    range, issuing a warning if not
   if (param->min_value && value < strtol(param->min_value,&rest,10) &&
       rest != param->min_value)
      {
      value = strtol(param->min_value,&rest,10) ;
      warn(value_out_of_range_msg,param->keyword,value) ;
      }
   if (param->max_value && value > strtol(param->max_value,&rest,10) &&
       rest != param->max_value)
      {
      value = strtol(param->max_value,&rest,10) ;
      warn(value_out_of_range_msg,param->keyword,value) ;
      }
   *location = value ;
   return true ;
}

//-----------------------------------------------------------------------

bool FrConfiguration::parseCardinal(const char *input_line,
				      const FrConfigurationTable *param,
				      bool initializing)
{
   if (initializing && input_line && !*input_line)
      {
      // keep whatever value was in that location, it's been statically
      //   initialized
      return true ;
      }
   size_t value ;
   char *rest ;
   size_t *location = (size_t*)FrCfgLoc(this,param) ;
   if (input_line == 0 || location == 0)
      return true ;			// quit immed if no input or output
   skipws(input_line) ;
   value = (size_t)strtol(input_line,&rest,10) ;
   if (rest && rest != input_line)
      input_line = rest ;
   else
      {
      warn(expected_number_msg) ;
      return false ;
      }
   // if a default value is specified, we're allowed to use it even if
   //   it falls outside the min-max range
   if (param->default_value && *param->default_value &&
       (size_t)strtol(param->default_value,&rest,10) == value)
      {
      *location = value ;
      return true ;
      }
   // if min and/or max are given, force the value to be inside the specified
   //    range, issuing a warning if not
   if (param->min_value && value < (size_t)strtol(param->min_value,&rest,10) &&
       rest != param->min_value)
      {
      value = (size_t)strtol(param->min_value,&rest,10) ;
      warn(value_out_of_range_msg,param->keyword,value) ;
      }
   if (param->max_value && value > (size_t)strtol(param->max_value,&rest,10) &&
       rest != param->max_value)
      {
      value = (size_t)strtol(param->max_value,&rest,10) ;
      warn(value_out_of_range_msg,param->keyword,value) ;
      }
   *location = value ;
   return true ;
}

//-----------------------------------------------------------------------

bool FrConfiguration::parseReal(const char *input_line,
				  const FrConfigurationTable *param,
				  bool initializing)
{
   if (initializing && input_line && !*input_line)
      {
      // keep whatever value was in that location, it's been statically
      //   initialized
      return true ;
      }
   char *rest ;
   double *location = (double*)FrCfgLoc(this,param) ;
   if (input_line == 0 || location == 0)
      return true ;			// quit immed if no input or output
   skipws(input_line) ;
   double value = strtod(input_line,&rest) ;
   if (rest && rest != input_line)
      input_line = rest ;
   else
      {
      warn(expected_number_msg) ;
      return false ;
      }
   // if a default value is specified, we're allowed to use it even if
   //   it falls outside the min-max range
   if (param->default_value && *param->default_value &&
       strtod(param->default_value,&rest) == value)
      {
      *location = value ;
      return true ;
      }
   // if min and/or max are given, force the value to be inside the
   //    specified range, issuing a warning if not
   if (param->min_value && value < strtod(param->min_value,&rest) &&
       rest != param->min_value)
      {
      value = strtod(param->min_value,&rest) ;
      warn(value_out_of_range_msg,param->keyword,value) ;
      }
   if (param->max_value && value > strtod(param->max_value,&rest) &&
       rest != param->max_value)
      {
      value = strtod(param->max_value,&rest) ;
      warn(value_out_of_range_msg,param->keyword,value) ;
      }
   *location = value ;
   return true ;
}

//-----------------------------------------------------------------------

bool FrConfiguration::parseString(const char *input_line,
				    void *location, bool initializing)
{
   const char *line = input_line ;
   char terminator ;
   const char *end ;
   int len ;

   if (location == 0)		// just ignore string if nowhere to put it
      return true ;
   FrFree(*((char**)location)) ;
   *((char**)location) = 0 ;
   skipws(line) ;
   terminator = initializing ? '\0' : *line ;
   if (initializing ||
       terminator == '"' || terminator == '\'' || terminator == '`')
      {
      if (!initializing)
	 line++ ;
      if ((end = strchr(line,terminator)) != 0)
	 {
	 len = (int)(end-line) ;
	 if ((*((char **)location) = FrNewN(char,len+1)) == 0)
	    FrNoMemory(alloc_string_msg) ;
	 memcpy(*((char **)location),line,len) ;
	 (*((char **)location))[len] = '\0' ;
	 }
      else
	 {
	 warn(unterm_string_msg) ;
	 return false ;
	 }
      }
   else
      warn(expected_string_msg) ;
   return true ;
}

//-----------------------------------------------------------------------

bool FrConfiguration::parseBasedir(const char *input_line, void *location,
				     bool initializing)
{
   bool result = parseFilename(input_line,&base_dir,initializing) ;
   if (result && location && location != this)
      {
      if (!base_dir)
	 {
	 base_dir = FrNewN(char,sizeof(default_base_dir)) ;
	 memcpy(base_dir,default_base_dir,sizeof(default_base_dir)) ;
	 }
      int len = strlen(base_dir)+1 ;
      FrFree(*((char**)location)) ;
      char *newdir = FrNewN(char,len) ;
      *((char**)location) = newdir ;
      memcpy(newdir,base_dir,len) ;
      }
   return result ;
}

//-----------------------------------------------------------------------

bool FrConfiguration::parseFilename(const char *line, void *location,
				      bool initializing)
{
   const char *end ;
   char terminator ;
   int len ;

   if (location == 0)		// just ignore string if nowhere to put it
      return true ;
   skipws(line) ;
   terminator = initializing ? '\0' : *line ;
   if (initializing ||
       terminator == '"' || terminator == '\'' || terminator == '`')
      {
      if (!initializing)
	 line++ ;				// skip opening quote
      if ((end = strchr(line,terminator)) != 0)
	 {
	 char *newstr ;
	 if (*line == '+')
	    {
	    line++ ;			// skip the plus sign
	    int baselen = strlen(base_dir) ;
	    len = (int)(end-line) + baselen ;
	    if ((newstr = FrNewN(char,len+1)) == 0)
	       FrNoMemory(alloc_string_msg) ;
	    else
	       {
	       memcpy(newstr,base_dir,baselen) ;
	       memcpy(newstr+baselen,line,(end-line)) ;
	       }
	    }
	 else if (*line == '=' && end == line+1)
	    {
	    newstr = FrDupString(input_filename ? input_filename : "") ;
	    len = strlen(input_filename) ;
	    }
	 else
	    {
	    len = (int)(end-line) ;
	    if ((newstr = FrNewN(char,len+1)) == 0)
	       FrNoMemory(alloc_string_msg) ;
	    else
	       memcpy(newstr,line,len+1) ;
	    }
	 if (newstr)
	    {
	    newstr[len] = '\0' ;
	    FrFree(*(char**)location) ;	// remove any prior value
	    *((char**)location) = newstr ;
	    }
	 }
      else
	 {
	 warn(unterm_string_msg) ;
	 return false ;
	 }
      }
   else
      warn(expected_string_msg) ;
   return true ;
}

//-----------------------------------------------------------------------

bool FrConfiguration::parseFilelist(const char *line, void *location,
				      void * /*extra*/)
{
   const char *end ;
   char terminator ;
   int len ;

   if (location == 0)		// just ignore string if nowhere to put it
      return true ;
   FrList *newlist = 0 ;
   skipws(line) ;
   while (*line)			// until end of line
      {
      terminator = *line ;
      if (terminator == '"' || terminator == '\'' || terminator == '`')
	 {
	 line++ ;			// skip opening quote
	 if ((end = strchr(line,terminator)) != 0)
	    {
	    char *newstr ;
	    if (*line == '+')
	       {
	       line++ ;			// skip the plus sign
	       int baselen = strlen(base_dir) ;
	       len = (int)(end-line) + baselen ;
	       if ((newstr = FrNewN(char,len+1)) == 0)
		  FrNoMemory(alloc_string_msg) ;
	       else
		  {
		  memcpy(newstr,base_dir,baselen) ;
		  memcpy(newstr+baselen,line,(end-line)) ;
		  }
	       }
	    else if (*line == '=' && end == line+1)
	       {
	       newstr = FrDupString(input_filename ? input_filename : "") ;
	       len = strlen(input_filename) ;
	       }
	    else
	       {
	       len = (int)(end-line) ;
	       if ((newstr = FrNewN(char,len+1)) == 0)
		  FrNoMemory(alloc_string_msg) ;
	       else
		  memcpy(newstr,line,len+1) ;
	       }
	    if (newstr)
	       {
	       newstr[len] = '\0' ;
	       pushlist(new FrString(newstr,len,1),newlist) ;
	       FrFree(newstr) ;
	       }
	    line = end+1 ;
	    }
	 else
	    {
	    warn(unterm_string_msg) ;
	    return false ;
	    }
	 }
      else
	 {
	 warn(expected_string_msg) ;
	 break ;			// avoid infinite loop
	 }
      // skip optional comma delimiters
      skipws_comma(line) ;
      }
   if (newlist)
      {
      free_object(*(FrList**)location) ; // remove any prior value
      *((FrList**)location) = listreverse(newlist) ;
      }
   return true ;
}

//-----------------------------------------------------------------------

bool FrConfiguration::parseBitflags(const char *input_line,
				      void *location, void *extra)
{
   if (input_line == 0 || location == 0)
      return true ;			// quit immed if no input or output

   FrCommandBit *commandbits = (FrCommandBit*)extra ;
   FrCommandBit *commands ;
   unsigned long flags = *((unsigned long*)location) ;
   skipws(input_line) ;
   while (*input_line)
      {
      FrSymbol *sym = string_to_Symbol(input_line) ;
      bool negated = false ;
      const char *symname = sym ? sym->symbolName() : "" ;
      if (*symname == '!' || *symname == '-')
	 {
	 negated = true ;
	 symname++ ;
	 }
      else if (Fr_strnicmp(symname,"NO_",3) == 0)
	 {
	 negated = true ;
	 symname += 3 ;
	 }
      else if (*symname == '+')
	 {
	 negated = false ;
	 symname++ ;
	 }
      commands = commandbits ;
      while (commands && commands->name)
	 {
	 if (Fr_stricmp(symname,commands->name) == 0)
	    {
	    if (negated)
	       flags &= ~commands->bit ;
	    else
	       flags |= commands->bit ;
	    break ;
	    }
	 else
	    commands++ ;
	 }
      if (commands && !commands->name)
	 {
	 warn("unknown name in options list",symname) ;
	 }
      skipws_comma(input_line) ;
      }
   *((unsigned long *)location) = flags ;
   return true ;
}

//-----------------------------------------------------------------------

bool FrConfiguration::parseSymbol(const char *input_line, void *location,
				    void * /*extra*/)
{
   if (input_line == 0 || location == 0) // if no args or no place to put them,
      return true ;			//   quit immediately
   FrSymbol *sym ;
   sym = string_to_Symbol(input_line) ;	// get the first symbol on the line
   if (sym)				// if we got one, store it
      *((FrSymbol **)location) = sym ;
   else					// otherwise, raise an alarm
      {
      warn("expected a Symbol") ;
      }
   return true ;
}

//-----------------------------------------------------------------------

bool FrConfiguration::parseSymlist(const char *input_line, void *location,
				     void * /*extra*/)
{
   FrList *symlist = 0 ;
   FrList **end = &symlist ;

   if (input_line == 0 || location == 0) // if no args or no place to put them,
      return true ;			//   quit immediately
   skipws(input_line) ;
   while (*input_line)			// until we hit the end of the line,
      {
      FrSymbol *sym ;
      sym = string_to_Symbol(input_line) ; // get the next symbol on the line
      symlist->pushlistend(sym,end) ;	// and store it
      skipws_comma(input_line) ;
      }
   free_object(*(FrList**)location) ;	// remove any prior value
   *end = 0 ;				// terminate the new list
   *((FrList **)location) = symlist ;
   return true ;
}

//-----------------------------------------------------------------------

bool FrConfiguration::parseList(const char *input_line, void *location,
				  void * /*extra*/)
{
   FrList *objlist = 0 ;
   FrList **end = &objlist ;

   if (input_line == 0 || location == 0) // if no args or no place to put them,
      return true ;			//   quit immediately
   skipws(input_line) ;
   while (*input_line)			// until we hit the end of the line,
      {
      // get the next obj on the line and store it on the list
      FrObject *obj = string_to_FrObject(input_line) ;
      objlist->pushlistend(obj,end) ;
      // skip optional comma delimiters
      skipws_comma(input_line) ;
      }
   free_object(*(FrList**)location) ;	// remove any prior value
   *end = 0 ;				// terminate the list
   *((FrList **)location) = objlist ;
   return true ;
}

//-----------------------------------------------------------------------

bool FrConfiguration::parseAssoclist(const char *input_line, void *location,
				       void * /*extra*/)
{
   FrSymbol *sym ;
   const char *rest ;
   FrObject *fillers ;

   if (input_line == 0 || location == 0)
      return true ;			// quit immed if no input or output
   *((FrList **)location) = 0 ;
   skipws(input_line) ;
   while (*input_line)
      {
      sym = string_to_Symbol(input_line) ;
      skipws(input_line) ;
      if (*input_line == 0 || *input_line++ != '=')
         {
	 warn("bad association list (expected \" = \")") ;
         return false ;
         }
      skipws(input_line) ;
      rest = input_line ;
      fillers = string_to_FrObject(rest) ;
      if (fillers && rest != input_line)
	 {
	 input_line = rest ;
	 pushlist(new FrCons(sym,fillers), *((FrList **)location)) ;
	 }
      else
	 {
	 warn("expected association for VALUE= in list") ;
	 return false ;
	 }
      skipws_comma(input_line) ;
      }
   return true ;
}

//-----------------------------------------------------------------------

bool FrConfiguration::parseYesno(const char *input_line, void *location,
				   void * /*extra*/)
{
   bool value ;

   skipws(input_line) ;
   char c = *input_line ;
   if (c == 'y' || c == 'Y' || c == '1' || c == '+' || c == 't' || c == 'T')
      value = true ;
   else if (c == 'n' || c == 'N' || c == '0' || c == '-' || c == 'f' || c=='F')
      value = false ;
   else
      {
      warn("expected a Yes/No value",input_line) ;
      value = false ;
      }
   *((char *)location) = trunc2char(value) ;
   return true ;
}

//-----------------------------------------------------------------------

bool FrConfiguration::parseKeyword(const char *input_line, void *location,
				     const FrConfigurationTable *param)
{
   // we have to have a list of valid values in the "min_value" field in order
   //   to be able to process the keyword parameter
   if (!param || !param->min_value)
      {
      warn("Programmer Error: No list of keywords for keyword parameter",
	   param->keyword,"will remain unchanged") ;
      return false ;
      }
   FrObject *obj = string_to_FrObject(input_line) ;
   const char *word = FrPrintableName(obj) ;
   if (word)
      {
      size_t wordlen = strlen(word) ;
      const char *start = param->min_value ;
      skipws(start) ;
      const char *end = end_of_keyword(start) ;
      int value = 0 ;
      while (*start && end > start)
	 {
	 if ((size_t)(end-start) == wordlen &&
	     Fr_strnicmp(word,start,wordlen) == 0)
	    {
	    *((int *)location) = value ;
	    break ;
	    }
	 value++ ;
	 start = end ;
	 skipws(start) ;
	 end = end_of_keyword(start) ;
	 }
      }
   else
      warn(param->keyword,"valid values are",param->min_value) ;
   free_object(obj) ;
   return true ;
}

//-----------------------------------------------------------------------

bool FrConfiguration::parseUser(FrConfigVariableType /*vartype*/,
				  const char * /*input_line*/,
				  void * /*location*/, void * /*extra*/)
{
   warn("configuration table specifies user type, but no handler defined") ;
   return true ;
}

//-----------------------------------------------------------------------

size_t FrConfiguration::maxline() const
{
   return FrMAX_LINE ;
}

//-----------------------------------------------------------------------

size_t FrConfiguration::maxUserType() const
{
   return 0 ;				// override w/ max FrConfigVariableType
}

//-----------------------------------------------------------------------

size_t FrConfiguration::lastLocalVar() const
{
   return 0 ;				// override w/ max FrConfigVariableType
}

//-----------------------------------------------------------------------

bool FrConfiguration::onChange(FrConfigVariableType, void * /*where*/)
{
   return true ;			// nothing to do, so trivially OK
}

//-----------------------------------------------------------------------

bool FrConfiguration::onRead(FrConfigVariableType, void * /*where*/)
{
   return true ;			// nothing to do, so trivially OK
}

//-----------------------------------------------------------------------

ostream &FrConfiguration::dumpFlags(const char *heading, unsigned long options,
				    FrCommandBit *bits, ostream &output) const
{
   if (heading)
      output << heading << flush ;
   else
      output << "  Options:	  " << flush ;
   if (options)
      {
      while (bits && bits->bit)
	 {
	 if (options & bits->bit)
	    output << bits->name << " " ;
	 bits++ ;
	 }
      }
   else
      output << "none" ;
   output << endl ;
   return output ;
}

//----------------------------------------------------------------------

ostream &FrConfiguration::dumpValues(const char *heading, ostream &output) const
{
   if (curr_state != 0)
      {
      output << heading << endl ;
      size_t i ;
      for (i = strlen(heading) ; i > 0 ; i--)
	 output << '=' ;
      output << endl ;
      for (i = 0 ; curr_state[i].keyword ; i++)
	 {
	 if (!*curr_state[i].keyword)
	    continue ;
	 size_t len = strlen(curr_state[i].keyword) ;
	 output << "  " << curr_state[i].keyword << ':' ;
	 if (len < 25)
	    output << setw(25-len) << " " ;
	 FrConfigVariableType vartype = curr_state[i].vartype ;
	 void *loc = FrCfgLoc(this,&curr_state[i]) ;
	 switch (vartype)
	    {
	    case integer:
	       output << (*(long int*)loc) ;
	       break ;
	    case cardinal:
	       output << (*(size_t*)loc) ;
	       break ;
	    case real:
	       output << (*(double*)loc) ;
	       break ;
	    case basedir:
	       {
	       const char *dir
		  = (loc && loc != VoidPtr(this)) ? *((char**)loc) : base_dir ;
	       output << '"' << (dir?dir:"") << '"' ;
	       }
	       break ;
	    case filename:
	    case cstring:
	       if (loc && *((char**)loc))
		  output << '"' << *((char**)loc) << '"' ;
	       else
		  output << "\"\"" ;
	       break ;
	    case bitflags:
	       {
	       FrCommandBit *bits = (FrCommandBit*)curr_state[i].extra_args ;
	       dumpFlags("",*(unsigned long*)loc,bits,output) ;
	       }
	       break ;
	    case list:
	    case assoclist:
	    case symlist:
	       if (loc)
		  output << *((FrList**)loc) ;
	       else
		  output << "()" ;
	       break ;
	    case symbol:
	       if (loc)
		  output << *((FrSymbol**)loc) ;
	       else
		  output << "NIL" ;
	       break ;
	    case yesno:
	       {
	       char value = loc ? *((char*)loc) : '\0' ;
	       output << (value ? "Yes" : "No") ;
	       }
	       break ;
	    case keyword:
	       {
	       char *value = keywordValue(&curr_state[i],*((int*)loc)) ;
	       output << value ;
	       FrFree(value) ;
	       }
	       break ;
	    default:
	       {
	       output << "{unknown type}" << endl ;
	       break ;
	       }
	    }
	 output << endl ;
	 }
      }
   else
      output << " [no keyword table defined for configuration]" << endl ;
   return output ;
}

//----------------------------------------------------------------------

FrList *FrConfiguration::listParameters() const
{
   FrList *params = 0 ;
   FrConfigurationTable *ct = curr_state ;
   while (ct && ct->keyword)
      {
      if (ct->keyword && *ct->keyword)
	 pushlist(new FrString(ct->keyword),params) ;
      ct++ ;
      }
   return listreverse(params) ;
}

//----------------------------------------------------------------------

FrList *FrConfiguration::listFlags(const char *param_name) const
{
   FrList *flags = 0 ;
   FrConfigurationTable *ct = findParameter(param_name) ;
   if (ct && ct->vartype == bitflags)
      {
      FrCommandBit *bits = (FrCommandBit*)ct->extra_args ;
      while (bits && bits->name)
	 {
	 FrList *flag ;
	 if (bits->description)
	    flag = new FrList(new FrString(bits->name),
			      new FrString(bits->description)) ;
	 else
	    flag = new FrList(new FrString(bits->name)) ;
	 pushlist(flag,flags) ;
	 }
      }
   return listreverse(flags) ;
}

//----------------------------------------------------------------------

FrList *FrConfiguration::describeParameter(const char *param_name)
{
   FrList *descrip = 0 ;
   // quicky for now: just insert the description strings
   FrList *item ;
   FrConfigurationTable *param = findParameter(param_name) ;
   if (!param)
      return descrip ;
   if (param->keyword)
      {
      item = new FrList(FrSymbolTable::add("NAME:"),
			new FrString(param->keyword)) ;
      pushlist(item,descrip) ;
      }
   const char *type = "<unknown>" ;
   if (param->vartype < user)
      type = FrConfigVariableTypeNames[param->vartype] ;
   item = new FrList(FrSymbolTable::add("TYPE:"), new FrString(type)) ;
   pushlist(item,descrip) ;
   const char *settable = "<unknown>" ;
   switch (param->settability)
      {
      case FrSET_READONLY:
	 settable = "Never" ;
	 break ;
      case FrSET_STARTUP:
	 settable = "Startup" ;
	 break ;
      case FrSET_RUNTIME:
	 settable = "Runtime" ;
	 break ;
      case FrSET_ANYTIME:
	 settable = "Anytime" ;
	 break ;
      }
   item = new FrList(FrSymbolTable::add("SET:"), new FrString(settable)) ;
   pushlist(item,descrip) ;
   char *curr_value = currentValue(param) ;
   if (curr_value)
      {
      item = new FrList(FrSymbolTable::add("CURRENT:"),
			new FrString(curr_value)) ;
      FrFree(curr_value) ;
      pushlist(item,descrip) ;
      }
   if (param->default_value)
      {
      item = new FrList(FrSymbolTable::add("DEFAULT:"),
			new FrString(param->default_value)) ;
      pushlist(item,descrip) ;
      }
   if (param->min_value)
      {
      if (param->vartype == keyword)
	 item = new FrList(FrSymbolTable::add("ENUM:"),
			   new FrString(param->min_value)) ;
      else
	 item = new FrList(FrSymbolTable::add("MIN:"),
			   new FrString(param->min_value)) ;
      pushlist(item,descrip) ;
      }
   if (param->max_value)
      {
      item = new FrList(FrSymbolTable::add("MAX:"),
			new FrString(param->max_value)) ;
      pushlist(item,descrip) ;
      }
   if (param->description && *param->description)
      {
      item = new FrList(FrSymbolTable::add("DESCRIPTION:"),
			new FrString(param->description)) ;
      pushlist(item,descrip) ;
      }
   return listreverse(descrip) ;
}

//----------------------------------------------------------------------

bool FrConfiguration::validValues(const char *param_name,
				    const char *&min_value,
				    const char *&max_value,
				    const char *&def_value) const
{
   FrConfigurationTable *ct = findParameter(param_name) ;
   if (ct && ct->keyword)
      {
      min_value = ct->min_value ;
      max_value = ct->max_value ;
      def_value = ct->default_value ;
      }
   else
      min_value = max_value = def_value = 0 ;
   return false ;
}

//----------------------------------------------------------------------

char *FrConfiguration::keywordValue(const FrConfigurationTable *param,
				    int value) const
{
   if (!param)
      return 0 ;
   const char *start = param->min_value ;
   skipws(start) ;
   for (int i = 0 ; i < value && *start ; i++)
      {
      start = end_of_keyword(start) ;
      skipws(start) ;
      }
   if (*start)
      {
      char *end = end_of_keyword(start) ;
      char *valuestring = FrNewN(char,end-start+1) ;
      if (valuestring)
	 {
	 memcpy(valuestring,start,end-start) ;
	 valuestring[end-start] = '\0' ;
	 }
      return valuestring ;
      }
   else
      return FrDupString("") ;
}

//----------------------------------------------------------------------

char *FrConfiguration::currentValue(long bits,
				    const FrCommandBit *bitflags) const
{
   size_t len = 0 ;
   // count the number of bytes needed to store the current flags as text
   for (const FrCommandBit *bf = bitflags ; bf->name ; bf++)
      {
      if (bf->bit)
	 len += strlen(bf->name) + 2 ;	// reserve space for +/- and blank
      }
   char *flags = FrNewN(char,len) ;
   if (flags)
      {
      char *currflag = flags ;
      for ( ; bitflags->name ; bitflags++)
	 {
	 if (bitflags->bit)
	    {
	    *currflag++ = ((bits & bitflags->bit) != 0) ? '+' : '-' ;
	    len = strlen(bitflags->name) ;
	    memcpy(currflag,bitflags->name,len) ;
	    currflag += len ;
	    if ((size_t)(currflag - flags) < len-1)
	       *currflag++ = ' ' ;
	    }
	 }
      *currflag = '\0' ;		// properly terminate the string
      }
   else
      FrNoMemory("while writing bit flags") ;
   return flags ;
}

//----------------------------------------------------------------------

char *FrConfiguration::currentValue(const FrConfigurationTable *param)
{
   if (!param)
      return 0 ;
   void *where = FrCfgLoc(this,param) ;
   if (!where)
      return 0 ;
   onRead(param->vartype,where) ;
   switch (param->vartype)
      {
      case integer:
	 return Fr_aprintf("%ld%c", *((long*)where),'\0');
      case cardinal:
	 return Fr_aprintf("%lu%c", *((unsigned long*)where),'\0') ;
      case real:
	 return Fr_aprintf("%g%c", *((double*)where),'\0') ;
      case basedir:
      case filename:
      case cstring:
	 if (*(char**)where)
	    return FrDupString(*(char**)where) ;
	 else
	    return FrDupString("\"\"") ;
      case filelist:
      case list:
      case assoclist:
      case symlist:
	 return (*((FrList**)where)) ? (*((FrList**)where))->print()
				     : FrDupString("()")  ;
      case bitflags:
	 return currentValue(*((long*)where),(FrCommandBit*)param->extra_args);
      case symbol:
	 if (*((FrSymbol**)where))
	    return FrDupString((*((FrSymbol**)where))->symbolName()) ;
	 else
	    return FrDupString("<NONE>") ;
      case yesno:
	 return FrDupString(*((char*)where) ? "Yes" : "No") ;
      case keyword:
	 return keywordValue(param,*((int*)where)) ;
      default:
	 break ;
      }
   // if we get here, we weren't able to print the current value
   return 0 ;
}

//----------------------------------------------------------------------

char *FrConfiguration::currentValue(const char *param_name)
{
   FrConfigurationTable *ct = findParameter(param_name) ;
   if (ct && ct->keyword)
      return currentValue(ct) ;
   return 0 ;
}

//----------------------------------------------------------------------

char *FrConfiguration::currentValue(const char *param_name,
				    const char *bit_name)
{
   FrConfigurationTable *ct = findParameter(param_name) ;
   if (ct && ct->keyword)
      {
      FrCommandBit *commandbits = (FrCommandBit*)ct->extra_args ;
      FrCommandBit *commands ;
      commands = commandbits ;
      while (commands && commands->name)
	 {
	 if (Fr_stricmp(bit_name,commands->name) == 0)
	    {
	    unsigned long flags = *((unsigned long*)FrCfgLoc(this,ct)) ;
	    size_t len = strlen(commands->name) ;
	    char *value = FrNewN(char,len+2) ;
	    if (value)
	       {
	       value[0] = (commands->bit & flags) != 0 ? '+' : '-' ;
	       memcpy(value+1,commands->name,len+1) ;
	       }
	    break ;
	    }
	 else
	    commands++ ;
	 }
      }
   return 0 ;
}

//----------------------------------------------------------------------

// end of file frcfgfil.cpp //
