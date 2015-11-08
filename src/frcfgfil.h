/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frcfgfil.h		configuration file processing		*/
/*  LastEdit: 08nov2015   						*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,1998,2001,2004,2005,2006,2008,	*/
/*		2009,2010 Ralf Brown/Carnegie Mellon University		*/
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

#ifndef __FRCFGFIL_H_INCLUDED
#define __FRCFGFIL_H_INCLUDED

#ifndef __FRCOMMON_H_INCLUDED
#include "frcommon.h"
#endif

#ifndef __FRLIST_H_INCLUDED
#include "frlist.h"
#endif

#if defined(__GNUC__)
#  pragma interface
#endif

/************************************************************************/
/************************************************************************/

#define FrSET_READONLY	0
#define FrSET_STARTUP 	1
#define FrSET_RUNTIME	2
#define FrSET_ANYTIME	3

struct FrCommandBit
   {
   const char *name ;
   long bit ;
   bool default_value ;
   const char *description ;
   } ;

enum FrConfigVariableType
   {
   integer, cardinal, real, basedir, filename,
   filelist, cstring, bitflags, list,
   assoclist, symbol, symlist, yesno, keyword,
   invalid, user
   } ;

struct FrConfigurationTable ;

class FrConfiguration
   {
   private:
      bool valid ;
      int current_line ;
      static bool in_startup ;
   protected: // data
      char *base_dir ;
      char *input_filename ;
      FrConfigurationTable *curr_state ;
   protected: // methods
      void warn(const char *message) const ;
      void warn(const char *message, const char *keyword, long value) const ;
      void warn(const char *message, const char *keyword, size_t value) const ;
      void warn(const char *message, const char *keyword, double value) const ;
      void warn(const char *message, const char *keyword, const char *) const ;
      void warn(const char *message, const char *text) const ;

      bool skipToSection(istream &infile, const char *section_name,
			 bool from_start = true) ;
      char *getLine(istream &input, bool check_for_section_hdr = false) ;
      bool parseLine(istream &input, bool single_section_only = false,
		       bool skip_unknown_keywords = false) ;
      FrConfigurationTable *findParameter(const char *param_name) const ;
      char *currentValue(const FrConfigurationTable *param) ;
      char *currentValue(long bits, const FrCommandBit *bitflags) const ;
      char *keywordValue(const FrConfigurationTable *param, int value) const ;

      // utility functions for parsing lines
      void skipws(const char *&line) const ;
      void skipws_comma(const char *&line) const ;
      bool skipto(char what, const char *&line, const char *errmsg) const ;
   public:
      FrConfiguration() ;
      FrConfiguration(const char *base_directory) ;
      virtual ~FrConfiguration() ;
      bool load(const char *filename, const char *section = 0,
		  bool reset = true) ;
      bool load(istream &instream, const char *section = 0,
		  bool reset = true) ;
      bool load(const char *filename, bool reset)
	 { return load(filename,0,reset) ; }
      bool load(istream &instream, bool reset)
	 { return load(instream,0,reset) ; }
      bool loadRaw(istream &input, const char *section, FrList *&params);
      bool loadRaw(const char *filename, const char *section, FrList *&params);
      void freeValues() ;

      static void startupComplete() ;
      static void beginningShutdown() ;

      // user help and runtime get/set
      FrList *listParameters() const ;
      FrList *listFlags(const char *param_name) const ;
      FrList *describeParameter(const char *param_name) ;
      bool validValues(const char *param_name,const char *&min_value,
			 const char *&max_value,
			 const char *&default_value) const ;
      char *currentValue(const char *param_name) ;
      char *currentValue(const char *param_name, const char *bit_name) ;
      			// (use FrFree on result when done)
      bool setParameter(const char *param_name,
			  const char *new_value, ostream *err = 0) ;
      bool setParameter(const char *new_value,
			  const FrConfigurationTable *param,
			  bool initializing = false, ostream *err = 0) ;

      // the different variable types' parsing functions
      bool parseInteger(const char *line, const FrConfigurationTable *param,
			  bool initializing);
      bool parseCardinal(const char *line,
			   const FrConfigurationTable *param,
			   bool initializing) ;
      bool parseReal(const char *line, const FrConfigurationTable *param,
		       bool initializing) ;

      bool parseBasedir(const char *line, void *storage_location,
			  bool initializing = false) ;
      bool parseFilename(const char *line, void *storage_location,
			   bool initializing = false) ;
      bool parseFilelist(const char *line, void *storage_location,
			   void *extra) ;
      bool parseString(const char *line, void *storage_location,
			 bool initializing = false) ;
      bool parseBitflags(const char *line, void *storage_location,
			   void *extra) ;
      bool parseList(const char *line, void *storage_location, void *extra) ;
      bool parseAssoclist(const char *line, void *storage_location,
			    void *extra) ;
      bool parseSymbol(const char *line, void *storage_location,
			 void *extra) ;
      bool parseSymlist(const char *line, void *storage_location,
			  void *extra) ;
      bool parseYesno(const char *line, void *storage_location,
			void *extra) ;
      bool parseKeyword(const char *line, void *storage_location,
			  const FrConfigurationTable *param) ;
      bool parseInvalid(const char *line, void *storage_location,
			  void *extra) ;

      // user-type parsing; needs to be overridden by derived classes that
      //   want to provide additional data types
      virtual bool parseUser(FrConfigVariableType type, const char *line,
			  void *storage_location, void *extra) ;
      virtual size_t maxUserType() const ;

      // access to internal state
      bool good() const { return valid ; }
      int lineNumber() const { return current_line ; }
      FrConfigurationTable *currentState() const { return curr_state ; }
      void setState(FrConfigurationTable *state, size_t line)
	 { curr_state = state ; current_line = line ; }
      ostream &dumpFlags(const char *heading, unsigned long flags,
			 FrCommandBit *bits,ostream &output) const ;
      ostream &dumpValues(const char *heading, ostream &output) const ;
      virtual ostream &dump(ostream &out) const = 0 ;

      // the following two methods must be provided by every derived class
      // init() -- clear all data members for config file to default values
      //	(most "standard" variables can be cleared in base class)
      // resetState() -- point curr_state at initial FrConfigurationTable
      virtual void init() ;
      virtual void initUserType(FrConfigVariableType type, void *location) ;
      virtual void resetState() = 0 ;

      // derived classes may optionally override the maximum length of a single
      // line (but lines can be continued, so total length is unlimited)
      virtual size_t maxline() const ;

      // derived classes must override the following function if they want to
      //   store any configuration values as member variables
      // body should be something like
      //    return (size_t)((char*)(&this->last_local_var) - (char*)this) ;
      virtual size_t lastLocalVar() const ;

      // derived classes may override the following function to be notified
      //   whenever a variable's value is changed
      virtual bool onChange(FrConfigVariableType, void *where) ;

      // derived classes may override the following function to be notified
      //   whenever a variable's value is about to be retrieved
      virtual bool onRead(FrConfigVariableType, void *where) ;
   } ;

// Visual C++ complains if we use a pointer-to-member before defining the class
// so we have to put FrConfigurationTable's definition AFTER FrConfiguration's
struct FrConfigurationTable
   {
   const char *keyword ;
   FrConfigVariableType vartype ;
   void *location ;
   int	settability ;
   FrConfigurationTable *next_state ;
   void *extra_args ;
   const char *default_value ;
   const char *min_value ;
   const char *max_value ;
   const char *description ;
   } ;

extern const char *FrConfigVariableTypeNames[] ;

#endif /* !__FRCFGFIL_H_INCLUDED */

// end of file frcfgfil.h //
