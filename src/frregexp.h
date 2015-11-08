/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File: frregexp.cpp	 	class FrRegExp for regular expressions	*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1997,1999,2000,2001,2009				*/
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

#ifndef __FRREGEXP_H_INCLUDED
#define __FRREGEXP_H_INCLUDED

#ifndef __FRSYMBOL_H_INCLUDED
#include "frsymbol.h"
#endif

#if defined(__GNUC__)
#  pragma interface
#endif

/************************************************************************/
/************************************************************************/

// special characters in regular expressions
#define FrRE_QUOTE     '%'
#define FrRE_OPTIONAL  '?'
#define FrRE_KLEENE    '*'
#define FrRE_MULTIPLE  '+'
#define FrRE_ALT_BEG   '('
#define FrRE_ALT_SEP   ','   	// alternation separator
#define FrRE_ALT_END   ')'
#define FrRE_CLASS_BEG '<'	// named set of strings that may match
#define FrRE_CLASS_END '>'
#define FrRE_COUNT_BEG '{'   	// start of explicit count specifier
#define FrRE_COUNT_END '}'   	// end of explicit count specifier
#define FrRE_CHARSET_BEG '['
#define FrRE_CHARSET_NEG '^'
#define FrRE_CHARSET_END ']'

/************************************************************************/
/************************************************************************/

class FrRegExElt ;

class FrRegExp
{
   protected:
      FrRegExp *_next ;			// next regex in list
      FrRegExElt *regex ; 		// head node of the current regex
      char *replacement ;
      FrSymbol *_token ;
      FrList *_classes ;		// equivalence classes
   public:
      FrRegExp()
	    { _next = 0 ; regex = 0 ; replacement = 0 ; _token = 0 ; }
      FrRegExp(const char *re, const char *repl, FrSymbol *token,
	       FrRegExp *next = 0) ;
      ~FrRegExp() ;

      void relink(FrRegExp *new_next) { _next = new_next ; }

      FrRegExElt *compile(const char *re) ;
      bool addToClass(FrSymbol *classname, const char *element,
		      const char *repl = 0) ;
      FrObject *match(const char *word) const ;
      FrObject *match(const FrSymbol *word) const
	    { return word ? match(word->symbolName()) : 0 ; }
      char *replace(char *string) const ;

      // access to internal state
      FrRegExp *next() const { return _next ; }
      FrSymbol *token() const { return _token ; }
} ;

#endif /* !__FRREGEXP_H_INCLUDED */

// end of file frregexp.h //
