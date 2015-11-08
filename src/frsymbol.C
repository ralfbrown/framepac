/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frsymbol.cpp	       class FrSymbol				*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,1998,1999,2000,2001,2002,2003,	*/
/*		2005,2009,2013,2015					*/
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
#  pragma implementation "frsymbol.h"
#endif

#include "frsymbol.h"
#include "frsymtab.h"
#include "frutil.h"
#include "frpcglbl.h"

#ifdef FrSTRICT_CPLUSPLUS
#  include <cstdlib>
#  include <string>
#else
#  include <stdlib.h>
#  include <string.h>
#endif /* FrSTRICT_CPLUSPLUS */

/************************************************************************/
/************************************************************************/

// force the global constructors for this module to run before those of
// the application program
#ifdef __WATCOMC__
#pragma initialize library ;
#endif /* __WATCOMC__ */

#if defined(_MSC_VER) && _MSC_VER >= 800
#pragma warning(disable : 4073)    // don't warn about following #pragma ....
#pragma init_seg(lib)
#endif /* _MSC_VER >= 800 */

/**********************************************************************/
/*	Global variables local to this module			      */
/**********************************************************************/

static const char errmsg_symname_too_long[]
     = "symbol name '%.40s...' too long (remainder ignored)." ;

static FrObject *read_quoted_Symbol(istream &input, const char *) ;
static FrObject *string_to_quoted_Symbol(const char *&input, const char *) ;
static bool verify_quoted_Symbol(const char *&input, const char *, bool) ;

static FrReader QuotedSym_reader(string_to_quoted_Symbol, read_quoted_Symbol,
				 verify_quoted_Symbol,'|') ;

/**********************************************************************/
/*    	Helper Functions				              */
/**********************************************************************/

static void no_bindings(const char *func)
{
   FrWarningVA("used FrSymbol::%s, but bindings are not compiled in",func) ;
   return ;
}

/**********************************************************************/
/*    Methods for class FrSymbol			              */
/**********************************************************************/

FrObjectType FrSymbol::objType() const
{
   return OT_Symbol ;
}

//----------------------------------------------------------------------

const char *FrSymbol::objTypeName() const
{
   return "FrSymbol" ;
}

//----------------------------------------------------------------------

FrObjectType FrSymbol::objSuperclass() const
{
   return OT_FrAtom ;
}

//----------------------------------------------------------------------

bool FrSymbol::symbolp() const
{
   return true ;
}

//----------------------------------------------------------------------

long int FrSymbol::intValue() const
{
   char *end = 0 ;
   long intval = strtol(symbolName(),&end,0) ;
   if (!end || end == symbolName())
      {
//!!!
      }
   return intval ;
}

//----------------------------------------------------------------------

const char *FrSymbol::printableName() const
{
   return symbolName() ;
}

//----------------------------------------------------------------------

FrSymbol::FrSymbol()
{
   FrProgError("user code called FrSymbol constructor.") ;
}

//----------------------------------------------------------------------

FrSymbol::~FrSymbol()
{
   FrProgError("destructor for FrSymbol called.") ;
}

//----------------------------------------------------------------------

FrSymbol *FrSymbol::makeSymbol(const char *nm) const
{
   return FrSymbolTable::add(nm) ;
}

//----------------------------------------------------------------------

FrSymbol *FrSymbol::findSymbol(const char *nm) const
{
   return const_cast<FrSymbol*>(FrSymbolTable::current()->lookupKey(nm)) ;
}

//----------------------------------------------------------------------

bool FrSymbol::nameNeedsQuoting(const char *name)
{
   if (!name || !*name)
      return false ;
   else if (name[0] == '#')
      return true ;
   else if (Fr_isdigit(name[0]) || !name[0] ||
       ((name[0] == '-' || name[0] == '+') && Fr_isdigit(name[1])))
      return true ;
   else if (name[0] == '~' && name[1] == '\0')
      return false ;
   for (const char *s = name ; *s ; s++)
      {
      unsigned char c = (unsigned char)*s ;
      if (!Fr_issymbolchar(c) || Fr_islower(c))
	 return true ;
      }
   return false ;
}

//----------------------------------------------------------------------

ostream &FrSymbol::printValue(ostream &output) const
{
   bool needs_quoting = nameNeedsQuoting(symbolName()) ;

   if (needs_quoting)
      output << '|' ;
   for (const char *s = symbolName() ; *s ; s++)
      {
      output << *s ;
      if (*s == '|')
	 output << '|' ;
      }
   if (needs_quoting)
      output << '|' ;
   return output ;
}

//----------------------------------------------------------------------

size_t FrSymbol::displayLength() const
{
   bool must_quote = nameNeedsQuoting(symbolName()) ;
   size_t count = 0 ;
   for (const char *s = symbolName() ; *s ; s++)
      {
      unsigned char c = (unsigned char)*s ;
      count++ ;
      if (c == '|')			// vertical bars must be doubled
	 count++ ;
      }
   return must_quote ? count + 2 : count ;
}

//----------------------------------------------------------------------

char *FrSymbol::displayValue(char *buffer) const
{
   bool needs_quoting = nameNeedsQuoting(symbolName()) ;

   if (needs_quoting)
      *buffer++ = '|' ;
   for (const char *s = symbolName() ; *s ; s++)
      {
      *buffer++ = *s ;
      if (*s == '|')
	 *buffer++ = '|' ;
      }
   if (needs_quoting)
      *buffer++ = '|' ;
   *buffer = '\0' ;
   return buffer ;
}

//----------------------------------------------------------------------

ostream &FrSymbol::printBinding(ostream &output) const
{
#ifdef FrSYMBOL_VALUE
   if (value == &UNBOUND)
      output << "*UNBOUND*" ;
   else if (value == 0)
      output << "NIL" ;
   else
      value->printValue(output) ;
#else
   static bool warned = false ;
   if (!warned)
      {
      warned = true ;
      no_bindings("printBinding") ;
      }
   output << "*UNBOUND*" ;
#endif /* FrSYMBOL_VALUE */
   return output ;
}

//----------------------------------------------------------------------

#ifndef FrSYMBOL_VALUE
void FrSymbol::setValue(const FrObject *)
{
   static bool warned = false ;
   if (!warned)
      {
      warned = true ;
      no_bindings("setValue") ;
      }
   return ;
}
#endif /* !_FrSYMBOL_VALUE */

//----------------------------------------------------------------------

#ifndef FrSYMBOL_VALUE
FrObject *FrSymbol::symbolValue() const
{
   static bool warned = false ;
   if (!warned)
      {
      warned = true ;
      no_bindings("symbolValue") ;
      }
   return 0 ;
}
#endif /* !FrSYMBOL_VALUE */

//----------------------------------------------------------------------

#ifdef FrDEMONS
bool FrSymbol::addDemon(FrDemonType type, FrDemonFunc *func, va_list args)
{
   FrDemonList **head, *d, *newdemon ;

   if (!theDemons)
      {
      theDemons = new FrDemons ;
      theDemons->if_created = theDemons->if_added = theDemons->if_retrieved =
	 theDemons->if_missing = theDemons->if_deleted = 0 ;
      }
   switch (type)
      {
      case DT_IfCreated:
	 head = &theDemons->if_created ;
	 break ;
      case DT_IfAdded:
	 head = &theDemons->if_added ;
	 break ;
      case DT_IfRetrieved:
	 head = &theDemons->if_retrieved ;
	 break ;
      case DT_IfMissing:
	 head = &theDemons->if_missing ;
	 break ;
      case DT_IfDeleted:
	 head = &theDemons->if_deleted ;
	 break ;
      default:
	 FrProgError("unknown demon type in FrSymbol::addDemon") ;
	 return false ;
      }
   d = *head ;
   while (d)
      {
      if (d->demon == func)
	 return false ;	   // this demon already active for this symbol
      d = d->next ;
      }
   newdemon = new FrDemonList ;
   newdemon->next = *head ;
   newdemon->demon = func ;
   newdemon->args = args ;
   *head = newdemon ;
   return true ;  // successfully added
}
#endif /* FrDEMONS */

//----------------------------------------------------------------------

#ifdef FrDEMONS
bool FrSymbol::removeDemon(FrDemonType type, FrDemonFunc *func)
{
   FrDemonList **head, *d, *prev ;
   bool removed ;

   if (!theDemons)
      return false ; // can't remove it because there are no demons defined
   switch (type)
      {
      case DT_IfCreated:
	 head = &theDemons->if_created ;
	 break ;
      case DT_IfAdded:
	 head = &theDemons->if_added ;
	 break ;
      case DT_IfRetrieved:
	 head = &theDemons->if_retrieved ;
	 break ;
      case DT_IfMissing:
	 head = &theDemons->if_missing ;
	 break ;
      case DT_IfDeleted:
	 head = &theDemons->if_deleted ;
	 break ;
      default:
	 FrProgError("unknown demon type in FrSymbol::removeDemon") ;
	 return false ;
      }
   d = *head ;
   prev = 0 ;
   removed = false ;
   while (d)
      {
      if (d->demon == func)
	 {
	 if (prev)
	    prev->next = d->next ;
	 else
	    *head = d->next ;
	 delete d ;
	 removed = true ;
	 break ;
	 }
      prev = d ;
      d = d->next ;
      }
   if (removed)
      if (!theDemons->if_created && !theDemons->if_added &&
	  !theDemons->if_retrieved && !theDemons->if_missing &&
	  !theDemons->if_deleted)
	 {
	 delete theDemons ;
	 theDemons = 0 ;
	 }
   return removed ;
}
#endif /* FrDEMONS */

//----------------------------------------------------------------------

void FrSymbol::defineRelation(const FrSymbol *inverse)
{
#ifdef FrSYMBOL_RELATION
   if (inverse->m_inv_relation)
      inverse->m_inv_relation->m_inv_relation = 0 ;
   if (m_inv_relation)
      m_inv_relation->m_inv_relation = 0 ;
   m_inv_relation = (FrSymbol *)inverse ;
   ((FrSymbol *)inverse)->m_inv_relation = (FrSymbol *)this ;
#else
   FrProgError("attempted to use FrSymbol::defineRelation, but\n"
	       "\tsupport for inverse relations is not compiled in") ;
#endif
   return ;
}

//----------------------------------------------------------------------

void FrSymbol::undefineRelation()
{
#ifdef FrSYMBOL_RELATION
   if (m_inv_relation)
      {
      m_inv_relation->m_inv_relation = 0 ;
      m_inv_relation = 0 ;
      }
#else
   FrProgError("attempted to use FrSymbol::undefineRelation, but\n"
	       "\tsupport for inverse relations is not compiled in") ;
#endif
   return ;
}

//----------------------------------------------------------------------

size_t FrSymbol::length() const
{
   return (size_t)strlen(symbolName()) ;
}

//----------------------------------------------------------------------

int FrSymbol::compare(const FrObject *obj) const
{
   if (!obj)
      return 1 ;     // anything is greater than NIL / empty-list
   if (!obj->symbolp())
      return -1 ;    // sort all non-symbols after symbols
   else
      return strcmp(symbolName(),((FrSymbol*)obj)->symbolName()) ;
}

/**********************************************************************/
/*    non-member functions related to class FrSymbol		      */
/**********************************************************************/

//----------------------------------------------------------------------
// copy a symbol from one symbol table into another

FrSymbol *makeSymbol(const FrSymbol *sym)
{
   return sym ? FrSymbolTable::add(sym->symbolName()) : 0 ;
}

//----------------------------------------------------------------------

FrSymbol *findSymbol(const char *name)
{
   return const_cast<FrSymbol*>(FrSymbolTable::current()->lookupKey(name)) ;
}

//----------------------------------------------------------------------

FrSymbol *gensym(const char *basename, const char *suffix)
{
   return FrSymbolTable::current()->gensym(basename,suffix) ;
}

//----------------------------------------------------------------------

FrSymbol *gensym(const FrSymbol *basename)
{
   return FrSymbolTable::current()->gensym(basename ? basename->symbolName() : 0) ;
}

/**********************************************************************/
/*    Frame relation functions					      */
/**********************************************************************/

void define_relation(const char *relname, const char *invname)
{
   FrSymbol *relation = (relname ? FrSymbolTable::add(relname) : 0) ;
   FrSymbol *inverse = (invname ? FrSymbolTable::add(invname) : 0) ;

   if (relation && inverse)
      relation->defineRelation(inverse) ;
   return ;
}

//----------------------------------------------------------------------

void undefine_relation(const char *relname)
{
   FrSymbol *relation = (relname ? FrSymbolTable::add(relname) : 0) ;

   if (relation)
      relation->undefineRelation() ;
   return ;
}

/**********************************************************************/
/*    Input functions						      */
/**********************************************************************/

static FrObject *read_quoted_Symbol(istream &input, const char *)
{
   register int count = 0 ;
   char name[FrMAX_SYMBOLNAME_LEN+2] ;

   input.get() ;  // throw away vertical bar
   while (count < FrMAX_SYMBOLNAME_LEN)
      {
      int c = input.get() ;
      if (c == EOF)
	 {
	 name[count] = '\0' ;
	 FrWarningVA("end of input reached before closing vertical bar:\n"
		     "\t|%s",name) ;
	 break ;
	 }
      else if (c == '|')
	 {
	 if (input.peek() == '|')
	    (void)input.get() ;		// doubled inside a quoted symbol
	 else
	    break ;			// single is terminator of sym name
	 }
      name[count++] = (char)c ;
      }
   if (count >= FrMAX_SYMBOLNAME_LEN)
      {
      count = FrMAX_SYMBOLNAME_LEN ;
      // consume the rest of the symbol
      int c ;
      while ((c = input.get()) != EOF && c != '|')
	 ;
      name[count] = '\0' ;	      // ensure proper string termination
      FrWarningVA(errmsg_symname_too_long,name) ;
      }
   name[count] = '\0' ;	      // ensure proper string termination
   return FrSymbolTable::add(name) ;
}

//----------------------------------------------------------------------

FrSymbol *read_Symbol(istream &input)
{
   char name[FrMAX_SYMBOLNAME_LEN+2] ;

   if (!input.good())
      return symbolEOF ;
   int c = FrSkipWhitespace(input) ;
   register int count = 0 ;
   if (c == '|')
      return (FrSymbol*)read_quoted_Symbol(input,0) ;
   else
      {
      while (count < FrMAX_SYMBOLNAME_LEN && (c = input.peek()) != EOF &&
	     (name[count] = Fr_issymbolchar(c)) != '\0')
	 {
	 (void)input.get() ;
	 count++ ;
	 }
      }
   if (count >= FrMAX_SYMBOLNAME_LEN)
      {
      // consume the rest of the symbol
      while ((c = input.peek()) != EOF && Fr_issymbolchar(c))
	 (void)input.get() ;
      name[count] = '\0' ;		 // ensure proper string termination
      FrWarningVA(errmsg_symname_too_long,name) ;
      }
#if 0  // don't need now that using peek() above
   if (c != EOF && c != '|')
      input.putback((char)c) ;   // read one char too many, so put it back
#endif
   name[count] = '\0' ;		 // ensure proper string termination
   return FrSymbolTable::add(name) ;
}

//----------------------------------------------------------------------

static FrObject *string_to_quoted_Symbol(const char *&input, const char *)
{
   register int count = 0 ;
   register const char *in = input ;
   char name[FrMAX_SYMBOLNAME_LEN+1] ;
   char c = '\0' ;

   while (count < FrMAX_SYMBOLNAME_LEN && (c = *++in) != '\0')
      {
      if (c == '|')
	 {
	 if (in[1] != '|')
	    break ;			// single pipe is terminator
	 in++ ;				// doubled inside a quoted symbol
	 }
      name[count++] = c ;
      }
   if (c == '|')
      in++ ;  // throw away vertical bar
   else if (count >= FrMAX_SYMBOLNAME_LEN)
      {
      // consume the rest of the symbol
      while (*in && *in != '|')
	 in++ ;
      name[count] = '\0' ;   // ensure proper string termination
      FrWarningVA(errmsg_symname_too_long,name) ;
      }
   input = in ;
   name[count] = '\0' ;   // ensure proper string termination
   return FrSymbolTable::add(name) ;
}

//----------------------------------------------------------------------

FrSymbol *string_to_Symbol(const char *&input)
{
   register int count = 0 ;
   char name[FrMAX_SYMBOLNAME_LEN+1] ;

   if (FrSkipWhitespace(input) == '|')
      return (FrSymbol*)string_to_quoted_Symbol(input,0) ;
   while (count < FrMAX_SYMBOLNAME_LEN &&
	  (name[count] = Fr_issymbolchar(*input)) != '\0')
      {
      input++ ;
      count++ ;
      }
   if (count >= FrMAX_SYMBOLNAME_LEN)
      {
      // consume the rest of the symbol
      while (Fr_issymbolchar(*input))
	 input++ ;
      name[count] = '\0' ;   // ensure proper string termination
      FrWarningVA(errmsg_symname_too_long,name) ;
      }
   name[count] = '\0' ;   // ensure proper string termination
   return FrSymbolTable::add(name) ;
}

//----------------------------------------------------------------------

static bool verify_quoted_Symbol(const char *&input, const char *,
				   bool /*strict*/)
{
   char c ;
   size_t count = 0 ;
   while ((c = *++input) != '\0' && count < FrMAX_SYMBOLNAME_LEN)
      {
      if (c == '|')
	 {
	 if (input[1] == '|')
	    input++ ;
	 else
	    break ;
	 }
      count++ ;
      }
   if (c != '|')
      {
      // consume the rest of the symbol name
      while (*input && (c = *input) != '|')
	 input++ ;
      }
   if (c == '|')
      {
      input++ ;				// throw away terminating vertical bar
      return true ;			// yep, this is a valid quoted symbol
      }
   else
      return false ;
}

//----------------------------------------------------------------------

bool verify_Symbol(const char *&input, bool strict)
{
   register int count = 0 ;
   if (FrSkipWhitespace(input) == '|')
      return verify_quoted_Symbol(input,0,strict) ;
   else
      {
      while (count < FrMAX_SYMBOLNAME_LEN && Fr_issymbolchar(*input))
	 {
	 count++ ;
	 input++ ;
	 }
      if (count >= FrMAX_SYMBOLNAME_LEN)
	 {
	 // consume the rest of the symbol
	 while (Fr_issymbolchar(*input))
	    input++ ;
	 }
      // if 'strict' test, we can't have hit the end of the string yet, since
      // that means we might have only part of the symbol's name so far
      return (!strict || *input != '\0') ;
      }
}

// end of file frsymbol.cpp //
