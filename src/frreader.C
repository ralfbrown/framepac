/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frreader.cpp		FramepaC object reader (input funcs)	*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,1998,1999,2001,2002,2003,2004,	*/
/*		2009 Ralf Brown/Carnegie Mellon University		*/
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
#  pragma implementation "frreader.h"
#endif

#include <stdlib.h>
#include "framerr.h"
#include "frctype.h"
#include "frlist.h"
#include "frsymbol.h"
#include "frnumber.h"
#include "frreader.h"
#include "frpcglbl.h"
#include "frutil.h"

//----------------------------------------------------------------------

// force the global constructors for this module to run before those of
// the application program
#ifdef __WATCOMC__
#pragma initialize library ;
#endif /* __WATCOMC__ */

#if defined(_MSC_VER) && _MSC_VER >= 800
#pragma warning(disable : 4073)  // don't warn about the following #pragma ...
#pragma init_seg(lib)
#endif /* _MSC_VER >= 800 */

/************************************************************************/
/*    Global variables shared with other modules			*/
/************************************************************************/

//NOTE: this table assumes the ASCII character set (or some superset)
// allowable characters for symbols:
//       a-zA-Z0-9 and -_*$+#<=>?:/%&@. plus high-bit chars

const unsigned char FramepaC_valid_symbol_characters[] =
   {
      0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0, // ^@-^O
      0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0, // ^P-^_
      0, 33,  0, 35, 36, 37, 38,  0,   0,  0, 42, 43,  0, 45, 46, 47, // SP- /
     48, 49, 50, 51, 52, 53, 54, 55,  56, 57, 58,  0, 60, 61, 62, 63, // 0 - ?
     64, 65, 66, 67, 68, 69, 70, 71,  72, 73, 74, 75, 76, 77, 78, 79, // @ - O
     80, 81, 82, 83, 84, 85, 86, 87,  88, 89, 90,  0,  0,  0,  0, 95, // P - _
      0, 65, 66, 67, 68, 69, 70, 71,  72, 73, 74, 75, 76, 77, 78, 79, // ` - o
     80, 81, 82, 83, 84, 85, 86, 87,  88, 89, 90,  0,  0,  0,  0,  0, // p -DEL
    128,129,130,131,132,133,134,135, 136,137,138,139,140,141,142,143,
    144,145,146,147,148,149,150,151, 152,153,138,155,140,157,158,159,
    160,161,162,163,164,165,166,167, 168,169,170,171,172,173,174,175,
    176,177,178,179,180,181,182,183, 184,169,186,187,188,189,174,191,
    192,193,194,195,196,197,198,199, 200,201,202,203,204,205,206,207, // uc acc
    208,209,210,211,212,213,214,215, 216,217,218,219,220,221,222,223,
    192,193,194,195,196,197,198,199, 200,201,202,203,204,205,206,207, // lc acc
    208,209,210,211,212,213,214,215, 216,217,218,219,220,221,222,159,
   } ;

#define Fr_isprint(c) ((unsigned char)c > ' ' && (unsigned)c < 0x100)

FrReadTable *FramepaC_readtable = 0 ;

FrList *FramepaC_read_associations = 0 ;
int FramepaC_read_nesting_level = 0 ;

bool FramepaC_read_widechar_strings = true ;

/************************************************************************/
/*      forward declarations						*/
/************************************************************************/

/************************************************************************/
/*    Global data local to this module				        */
/************************************************************************/

static const char stringFUNCTION[] = "FUNCTION" ;

static const char errmsg_symname_too_long[]
     = "symbol name too long (remainder ignored)." ;
static const char errmsg_read_incomplete[]
   = "attempted to read object with incomplete printed representation" ;

//----------------------------------------------------------------------

static FrObject *read_Lisp_form(istream &input, const char *) ;
static FrObject *string_to_Lisp_form(const char *&input, const char *) ;
static bool verify_Lisp_form(const char *&input, const char *,
			       bool strict) ;

static FrObject *string_to_dummy_Symbol(const char *&input, const char *) ;
static FrObject *read_dummy_Symbol(istream &input, const char *) ;
static bool verify_dummy_Symbol(const char *&input, const char *,
				  bool strict) ;
static bool verify_unquoted_Symbol(const char *&input, const char *,
				     bool strict) ;

/************************************************************************/
/*    Global variables for class FrReader				*/
/************************************************************************/

FrReader *FrReader::reader_list = 0 ;

static FrReader LispForm_reader(string_to_Lisp_form, read_Lisp_form,
				verify_Lisp_form, '#') ;

/************************************************************************/
/*    Methods for class FrReadTable					*/
/************************************************************************/

//----------------------------------------------------------------------
// ignore Lisp-style comments

static FrObject *skip_comment_string(const char *&input, const char *)
{
   char c ;
   input++ ;			       // consume the initial semicolon
   do {
      while ((c = *input) != '\0' && c != '\n')
	 input++ ;			// skip to end of line
      while (Fr_isspace(*input))	// then skip any whitespace
	 input++ ;
      c = *input ;			// get next non-whitespace char
      } while (c == ';') ;		// loop until not a comment
   if (!c)				// return if at end of string
      return symbolEOF ;
   return FramepaC_readtable->readString(input) ;
}

//----------------------------------------------------------------------
// ignore Lisp-style comments

static bool skip_comment_verify(const char *&input, const char *,
				  bool strict)
{
   char c ;
   input++ ;			       // consume the initial semicolon
   do {
      while ((c = *input) != '\0' && c != '\n')
	 input++ ;			// skip to end of line
      while (Fr_isspace(*input))	// then skip any whitespace
	 input++ ;
      c = *input ;			// get next non-whitespace char
      } while (c == ';') ;		// loop until not a comment
   if (!c)				// return if at end of string
      return false ;			//  -- no object to be read
   return FramepaC_readtable->validString(input,strict) ;
}

//----------------------------------------------------------------------
// ignore Lisp-style comments

static FrObject *skip_comment_stream(istream &input, const char *)
{
   int c ;
   do {
      do {
      c = EOF ;
      } while (!input.eof() && !input.fail() &&
	       (c = input.get()) != '\n' && c != EOF) ;
      while (!input.eof() && !input.fail() && (c = input.peek()) != EOF)
 	 {
	 if (!Fr_isspace(c))
	    break ;
	 input.get() ;
	 c = EOF ;
	 }
      } while (c == ';') ;
   if (c == EOF)
      return symbolEOF ;
   else
      return FramepaC_readtable->readStream(input) ;
}

//----------------------------------------------------------------------

FrSymbol *FramepaC_read_unquoted_Symbol(istream &input, const char *prefix)
{
   register int count = 0 ;
   char name[FrMAX_SYMBOLNAME_LEN+1] ;
   int c = EOF ;

   if (prefix)
      {
      strncpy(name,prefix,sizeof(name)) ;
      name[sizeof(name)-1] = '\0' ;
      count = strlen(name) ;
      }
   while (!input.eof() && !input.fail() &&
	  (c = input.get()) != EOF && count < FrMAX_SYMBOLNAME_LEN &&
	  (name[count] = Fr_issymbolchar(c)) != '\0')
      {
      count++ ;
      c = EOF ;
      }
   if (count >= FrMAX_SYMBOLNAME_LEN)
      {
      // consume the rest of the symbol
      do {
         c = EOF ;
	 } while (!input.eof() && !input.fail() &&
		  (c = input.get()) != EOF && Fr_issymbolchar(c)) ;
      FrWarning(errmsg_symname_too_long) ;
      }
   if (c != EOF)
      input.putback((char)c) ;   // read one char too many, so put it back
   name[FrMAX_SYMBOLNAME_LEN] = '\0' ;	// ensure proper string termination
   return FrSymbolTable::add(name) ;
}

//----------------------------------------------------------------------

static FrObject *read_dummy_Symbol(istream &input, const char *)
{
   char name[2] ;

   name[0] = (char)input.get() ;
   name[1] = '\0' ;
   return FrSymbolTable::add(name) ;
}

//----------------------------------------------------------------------

static FrSymbol *read_Symbol(istream &input,char *prefix)
{
   char name[FrMAX_SYMBOLNAME_LEN+1] ;
   int c = EOF ;
   bool quoted ;

   register int count = strlen(prefix) ;
   if (count >= FrMAX_SYMBOLNAME_LEN)
      {
      // consume the rest of the symbol
      while (!input.eof() && !input.fail() &&
	     (c = input.get()) != EOF && Fr_issymbolchar(c))
	 ;
      FrWarning(errmsg_symname_too_long) ;
      name[sizeof(name)-1] = '\0' ;
      return FrSymbolTable::add(name) ;
      }
   else
      c = '\0' ;
   memcpy(name,prefix,count) ;
   if (name[0] == '|')
      {
      input.getline(name+count,FrMAX_SYMBOLNAME_LEN-count,'|') ;
      count += input.gcount() ;
      quoted = true ;
      }
   else
      {
      while (count < FrMAX_SYMBOLNAME_LEN && !input.fail() &&
	     (c = input.get()) != EOF &&
	     (name[count] = Fr_issymbolchar(c)) != '\0')
	 count++ ;
      quoted = false ;
      }
   if (count >= FrMAX_SYMBOLNAME_LEN)
      {
      // consume the rest of the symbol
      while (!input.fail() && (c = input.get()) != EOF && Fr_issymbolchar(c))
	 c = EOF ;
      FrWarning(errmsg_symname_too_long) ;
      }
   if (c != EOF)
      input.putback((char)c) ;   // read one char too many, so put it back
   name[FrMAX_SYMBOLNAME_LEN] = '\0' ;	// ensure proper string termination
   return (count || quoted)
	     ? FrSymbolTable::add(name)
	     : FrSymbolTable::add("?") ;
}

//----------------------------------------------------------------------

FrReadTable::FrReadTable()
{
   unsigned int i ;
   for (i = 0 ; i < lengthof(string) ; i++)
      {
      if (i < lengthof(FramepaC_valid_symbol_characters) &&
	  FramepaC_valid_symbol_characters[i])
	 string[i] = (FrReadStringFunc*)FramepaC_string_to_unquoted_Symbol ;
      else if (!Fr_isspace(i))
	 string[i] = string_to_dummy_Symbol ;
      else
	 string[i] = 0 ;  // treat as whitespace
      }
   string[(size_t)';'] = skip_comment_string ;
   for (i = 0 ; i < lengthof(stream) ; i++)
      {
      if (i < lengthof(FramepaC_valid_symbol_characters) &&
	  FramepaC_valid_symbol_characters[i])
	 stream[i] = (FrReadStreamFunc*)FramepaC_read_unquoted_Symbol ;
      else if (!Fr_isspace(i))
	 stream[i] = read_dummy_Symbol ;
      else
	 stream[i] = 0 ;  // treat as whitespace
      }
   stream[(size_t)';'] = skip_comment_stream ;
   for (i = 0 ; i < lengthof(verify) ; i++)
      {
      if (i < lengthof(FramepaC_valid_symbol_characters) &&
	  FramepaC_valid_symbol_characters[i])
	 verify[i] = (FrReadVerifyFunc*)verify_unquoted_Symbol ;
      else if (!Fr_isspace(i))
	 verify[i] = verify_dummy_Symbol ;
      else
	 verify[i] = 0 ;  // treat as whitespace
      }
   verify[(size_t)';'] = skip_comment_verify ;
   return ;
}

//----------------------------------------------------------------------

FrReadTable::~FrReadTable()
{
   if (FramepaC_readtable == this)
      FramepaC_readtable = 0 ;
}

//----------------------------------------------------------------------

void FrReadTable::setReader(char c, FrReadStringFunc *stringfunc)
{
#if CHAR_BITS > 8
   if ((unsigned char)c < lengthof(string))
      string[(unsigned char)c] = stringfunc ;
#else
   string[(unsigned char)c] = stringfunc ;
#endif /* CHAR_BITS > 8 */
}

//----------------------------------------------------------------------

void FrReadTable::setReader(char c, FrReadStreamFunc *streamfunc)
{
#if CHAR_BITS > 8
   if ((unsigned char)c < lengthof(stream))
      stream[(unsigned char)c] = streamfunc ;
#else
   stream[(unsigned char)c] = streamfunc ;
#endif /* CHAR_BITS > 8 */
}

//----------------------------------------------------------------------

void FrReadTable::setReader(char c, FrReadVerifyFunc *streamfunc)
{
#if CHAR_BITS > 8
   if ((unsigned char)c < lengthof(stream))
      verify[(unsigned char)c] = streamfunc ;
#else
   verify[(unsigned char)c] = streamfunc ;
#endif /* CHAR_BITS > 8 */
}

//----------------------------------------------------------------------

void FrReadTable::setReader(char c, FrReadStringFunc *stringfunc,
			    FrReadStreamFunc *streamfunc,
			    FrReadVerifyFunc *verifyfunc)
{
#if CHAR_BITS > 8
   if ((unsigned char)c < lengthof(string))
      string[(unsigned char)c] = stringfunc ;
   if ((unsigned char)c < lengthof(stream))
      stream[(unsigned char)c] = streamfunc ;
   if ((unsigned char)c < lengthof(verify))
      verify[(unsigned char)c] = verifyfunc ;
#else
   string[(unsigned char)c] = stringfunc ;
   stream[(unsigned char)c] = streamfunc ;
   verify[(unsigned char)c] = verifyfunc ;
#endif /* CHAR_BITS > 8 */
}

//----------------------------------------------------------------------

bool FrReadTable::validString(const char *&input, bool strict) const
{
   char c ;
   FrReadVerifyFunc *f ;

   do {
      if ((c = *input) == '\0')
	 return false ;
      input++ ;
      f = getVerifier(c) ;
      } while (!f) ;
   input-- ;  // unget the character
   return f(input,0,strict) ;
}

//----------------------------------------------------------------------

FrObject *FrReadTable::readString(const char *&input) const
{
   char c ;
   FrReadStringFunc *f ;

   do {
      if ((c = *input) == '\0')
	 return symbolEOF ;
      input++ ;
      f = getStringReader(c) ;
      } while (!f) ;
   input-- ;  // unget the character
   return f(input,0) ;
}

//----------------------------------------------------------------------

FrObject *FrReadTable::readStream(istream &input) const
{
   int c ;
   FrReadStreamFunc *f ;

#if NEW || 0
   // GCC 3.0.1's runtime library is massively broken with respect to EOF
   //   on files.  While this code avoids hanging at EOF, it still doesn't
   //   get things right on small input files
   while (!input.eof() && !input.fail() && (c = input.peek()) != EOF)
      {
      if ((f = getStreamReader((char)c)) != 0)
	 return f(input,0) ;
      (void)input.get() ;		// skip whitespace
      }
   return symbolEOF ;
#else
   for (;;)
      {
      if (input.fail() || input.eof() || (c = input.peek()) == EOF)
         return symbolEOF ;
      if ((f = getStreamReader((char)c)) != 0)
         return f(input,0) ;
      input.get() ;			// skip whitespace
      }
#endif
}

/************************************************************************/
/*    Methods for class FrReader					*/
/************************************************************************/

FrReader::FrReader(FrReadStringFunc *stringfunc, FrReadStreamFunc *streamfunc,
		   FrReadVerifyFunc *verifyfunc,
		   int leadin_char, const char *leadin_string)
{
   lead_char = leadin_char ;
   lead_string = 0 ;
   next = 0 ;
   string = 0 ;
   stream = 0 ;
   verify = 0 ;
   if (!FramepaC_readtable)
      FramepaC_readtable = new FrReadTable ;
   if (leadin_char >= 0)
      FramepaC_readtable->setReader((unsigned char)leadin_char,
				    stringfunc,streamfunc,verifyfunc) ;
   else if (leadin_string)
      {
      if (lead_char == FrREADER_LEADIN_LISPFORM)
	 {
	 lead_string = FrDupString(leadin_string) ;
	 next = reader_list ;
	 reader_list = this ;
	 string = stringfunc ;
	 stream = streamfunc ;
	 verify = verifyfunc ;
	 }
      else if (lead_char == FrREADER_LEADIN_CHARSET)
	 {
	 while (leadin_string && *leadin_string)
	    {
	    FramepaC_readtable->setReader(*(unsigned char*)leadin_string,
					  stringfunc, streamfunc, verifyfunc) ;
	    leadin_string++ ;
	    }
	 }
      }
   return ;
}

//----------------------------------------------------------------------

FrReader::~FrReader()
{
   FrReader *prev = 0 ;

   // unlink from list of readers
   for (FrReader *r = reader_list ; r ; r = r->next)
      {
      if (r == this)
	 {
	 if (prev)
	    prev->next = next ;
	 else
	    reader_list = next ;
	 break ;
	 }
      prev = r ;
      }
   FrFree(lead_string) ;
   lead_string = 0 ;
   return ;
}

//----------------------------------------------------------------------

void FrReader::FramepaC_shutdown()
{
   FrFree(lead_string) ;
   lead_string = 0 ;
   return ;
}

/************************************************************************/
/*    Input functions (stream)						*/
/************************************************************************/

char FrSkipWhitespace(istream &input)
{
   int c ;

   input >> ws ;			// skip whitespace
   while ((c = input.peek()) == ';')	// ignore any Lisp-style comments
      {
      input.ignore(INT_MAX,'\n') ;	// skip to end of line
      input >> ws ;			// then skip any whitespace
      }
   return (c == EOF) ? '\0' : (char)c ;
}

//----------------------------------------------------------------------

static FrList *adjust_vector_length(FrList *vector, const char *digits)
{
   if (*digits)
      {
      int len = atoi(digits) ;
      if (len > 0 && vector && vector->consp())
	 {
	 int listlen = ((FrList*)vector)->listlength() ;
	 if (listlen < len)
	    {
	    // extend the 'vector' until the specified length
	    FrList *end = ((FrList*)vector)->last() ;
	    FrObject *fill = end->first() ;
	    while (listlen < len)
	       {
	       end->nconc(new FrList(fill ? fill->deepcopy() : 0)) ;
	       end = end->rest() ;
	       listlen++ ;
	       }
	    }
	 else if (listlen > len)
	    {
	    // truncate 'vector' at specified length
	    FrList *tail = ((FrList*)vector)->nthcdr(len-1) ;
	    FrList *discard = tail->rest() ;
	    tail->replacd(0) ;	// chop off unwanted part
	    free_object(discard) ; // and throw it away
	    }
	 }
      }
   return vector ;
}

//----------------------------------------------------------------------

static FrObject *convert_Lisp_form(istream &input, const char *digits)
{
   int count = 1 ;
   char buf[FrMAX_SYMBOLNAME_LEN+2] ;

   buf[0] = '#' ;
   while (count <= FrMAX_SYMBOLNAME_LEN)
      {
      int ch = input.get() ;
      if (ch == EOF)
	 break ;
      if (!Fr_isalpha(ch) && ch != '*')
	 {
         input.putback((char)ch) ;
	 break ;
	 }
      buf[count++] = (char)ch ;
      }
   buf[count] = '\0' ;
   FrReader *reader = FrReader::firstReader() ;
   while (reader)
      {
      if (Fr_stricmp(reader->lispFormName(),buf+1) == 0)
	 return reader->readStream(input,digits) ;
      reader = reader->nextReader() ;
      }
   // if we got here, the name didn't match, so treat it as a symbol starting
   // with #
   return read_Symbol(input,buf) ;
}

//----------------------------------------------------------------------

static FrObject *read_Lisp_form(istream &input, const char *)
{
   FrObject *result ;
   bool widechar ;
   char buf[FrMAX_SYMBOLNAME_LEN+1] ;
   char digits[100] ;
   int numdigits = 1 ;
   FrNumber *num ;

   input.get() ;			// consume the '#'
   int c = input.peek() ;
   while (Fr_isdigit(c))
      {
      input.get() ;			// we're gonna consume the digit
      if (numdigits < (int)sizeof(digits)-1)
	 digits[numdigits++] = (char)c ;
      c = input.peek() ;
      }
   digits[numdigits] = '\0' ;
   switch (c)
      {
      case '(':
	 {
	 FrReader *reader = FrReader::firstReader() ;
	 while (reader)
	    {
	    const char *name = reader->lispFormName() ;
	    if (name[0] == '\0')	//  find reader for ""
	       return reader->readStream(input,digits+1) ;
	    reader = reader->nextReader() ;
	    }
	 // no vector reader linked in, so read it as a list
	 widechar = read_extended_strings(false) ; // use Lisp compatibility
	 result = read_FrObject(input) ;
	 read_extended_strings(widechar) ;
	 result = adjust_vector_length((FrList*)result,digits+1) ;
	 break ;
	 }
      case '<':
	 FrError(errmsg_read_incomplete) ;
	 input.ignore(INT_MAX,'>') ;
	 result = 0 ;
	 break ;
      case '\'':
	 // expand #'foo to (FUNCTION foo)
	 input.get() ;			// consume the quote
	 result = new FrList(FrSymbolTable::add(stringFUNCTION),
			   read_FrObject(input)) ;
	 break ;
      case '\\':
	 // treat #\a as the symbol |a|
	 input.get() ;			// consume the backslash
	 buf[0] = (char)input.get() ;
	 buf[1] = '\0' ;
	 result = FrSymbolTable::add(buf) ;
	 break ;
      case ':':				// uninterned symbol
	 // since FramepaC has no uninterned symbols, just ignore the #: and
	 // read a normal symbol
	 input.get() ;			// consume the colon
	 result = FramepaC_read_unquoted_Symbol(input,0) ;
	 break ;
      case '|':
	 // balanced comment (unfortunately, nesting not yet supported)
	 int peekch ;
	 peekch = 0 ;
	 do {
	    input.get() ;		  // throw out the bar
	    input.ignore(INT_MAX,'|') ;	  // skip to matching vertical bar
	    } while ((peekch = input.peek()) != EOF && peekch != '#') ;
	 if (peekch == '#')
	    {
	    input.get() ;		// throw out the final hash
	    input.ignore() ;
	    peekch = input.peek() ;
	    }
	 if (peekch == ')' && FramepaC_read_nesting_level > 0)
	    result = 0 ;
	 else
	    result = read_FrObject(input) ;
	 break ;
      case '#':				// shared object reference
	 input.get() ;			// consume the hash mark
	 num = new FrInteger(atol(digits+1)) ;
	 result = FramepaC_read_associations->assoc(num,eql) ;
	 if (result && result->consp())
	    result = ((FrList*)result)->second() ;
	 if (result)
	    result = result->deepcopy() ;
	 free_object(num) ;
         break ;
      case '=':				// shared object definition
	 input.get() ;			// consume the equal sign
	 result = read_FrObject(input) ; // and get next object
	 num = new FrInteger(atol(digits+1)) ;
	 FrList *assoc ;
	 assoc = new FrList(num, result ? (FrList*)result->deepcopy() : 0) ;
	 pushlist(assoc,FramepaC_read_associations) ;
         break ;
      case ')':
	 // this wasn't actually a Lisp form, it was a standalone '#' at the
	 //   end of a list!
	 result = FrSymbolTable::add("#") ;
	 break ;
      default:
	 if (Fr_isprint(c))
	    result = convert_Lisp_form(input,digits+1) ;
	 else
	    {
	    // not a recognized form, so read it as a symbol beginning with '#'
	    digits[0] = '#' ;
	    result = read_Symbol(input,digits) ;
	    }
	 break ;
      }
   return result ;
}

/**********************************************************************/
/*    Input functions (string)					      */
/**********************************************************************/

char FrSkipWhitespace(const char *&s) ;

static FrObject *convert_Lisp_form(const char *&input, const char *digits)
{
   int count = 0 ;
   char buf[FrMAX_SYMBOLNAME_LEN+1] ;
   const char *orig_input = input ;

   while (count < FrMAX_SYMBOLNAME_LEN &&
	  (Fr_isalpha(*input) || *input == '*'))
      buf[count++] = *input++ ;
   buf[count] = '\0' ;
   FrReader *reader = FrReader::firstReader() ;
   while (reader)
      {
      if (Fr_stricmp(reader->lispFormName(),buf) == 0)
	 return reader->readString(input,digits) ;
      reader = reader->nextReader() ;
      }
   // if we got here, the name didn't match, so treat it as a symbol starting
   // with #
   input = orig_input ;
   return FramepaC_string_to_unquoted_Symbol(input,0) ;
}

//----------------------------------------------------------------------

static FrObject *string_to_Lisp_form(const char *&input, const char *)
{
   FrObject *result ;
   bool widechar ;
   FrNumber *num ;
   char digits[100] ;
   int numdigits = 1 ;
   const char *orig_input = input ;

   input++ ;   // consume the '#'
   while (Fr_isdigit(*input) && numdigits < (int)sizeof(digits)-1)
      digits[numdigits++] = *input++ ;
   digits[numdigits] = '\0' ; 			// ensure proper termination
   switch (*input)
      {
      case '(':
	 {
	 FrReader *reader = FrReader::firstReader() ;
	 while (reader)
	    {
	    const char *name = reader->lispFormName() ;
	    if (name[0] == '\0')	//  find reader for ""
	       return reader->readString(input,digits+1) ;
	    reader = reader->nextReader() ;
	    }
	 // no vector reader linked in, so read it as a list
	 widechar = read_extended_strings(false) ; // use Lisp compatibility
	 result = string_to_FrObject(input) ;
	 read_extended_strings(widechar) ;
	 result = adjust_vector_length((FrList*)result,digits+1) ;
	 break ;
	 }
      case '<':
	 FrError(errmsg_read_incomplete) ;
	 while (*input && *input != '>')
	    input++ ;
	 result = 0 ;
	 break ;
      case '\'':
	 // expand #'foo to (FUNCTION foo)
	 input++ ;			// consume the quote
	 result = new FrList(FrSymbolTable::add(stringFUNCTION),
			      string_to_FrObject(input)) ;
	 break ;
      case '\\':
	 // treat #\a as the symbol |a|
	 char buf[2] ;
	 input++ ;			// consume the backslash
	 buf[0] = *input++ ;
	 buf[1] = '\0' ;
	 result = FrSymbolTable::add(buf) ;
	 break ;
      case ':':				// uninterned symbol
	 // since FramepaC has no uninterned symbols, just ignore the #: and
	 // read a normal symbol
	 input++ ;			// consume the colon
	 result = FramepaC_string_to_unquoted_Symbol(input,0) ;
	 break ;
      case '|':
	 // balanced comment
	 input++ ;			  // throw out the bar
	 do {
	    while (*input && *input != '|')
	       input++ ;	 	  // skip to matching vertical bar
	    if (*input)
	       input++ ;
	    } while (*input && *input != '#') ;
	 if (*input == '#')
	    {
	    input++ ;	   		  // throw out the final hash
	    FrSkipWhitespace(input) ;
	    }
	 if (*input == ')' && FramepaC_read_nesting_level > 0)
	    result = 0 ;
	 else
	    result = string_to_FrObject(input) ;
	 break ;
      case '#':				// shared object reference
	 input++ ;			// consume the hash mark
	 num = new FrInteger(atol(digits+1)) ;
	 result = FramepaC_read_associations->assoc(num,eql) ;
	 if (result && result->consp())
	    result = ((FrList*)result)->second() ;
	 if (result)
	    result = result->deepcopy() ;
	 free_object(num) ;
         break ;
      case '=':				// shared object definition
	 input++ ;  				// consume the equal sign
	 result = string_to_FrObject(input) ;	// and get next object
	 num = new FrInteger(atol(digits+1)) ;
	 if (result)
	    result = result->deepcopy() ;
	 result = new FrList(num,result) ;
         pushlist(result, FramepaC_read_associations) ;
	 result = ((FrList*)result)->second() ;  // don't return the 'num'
         break ;
      case ')':
	 // this wasn't actually a Lisp form, it was a standalone '#' at the
	 //   end of a list!
	 result = FrSymbolTable::add("#") ;
	 break ;
      case 'r':
      case 'R':
      case 'x':
      case 'X':
	 {
	 FrReader *reader = FrReader::firstReader() ;
	 while (reader)
	    {
	    const char *name = reader->lispFormName() ;
	    if (name[0] == Fr_toupper(*input) && name[1] == '\0')
	       return reader->readString(input,digits+1) ;
	    reader = reader->nextReader() ;
	    }
	 }
	 // if we got here, there was no matching single-character form name,
	 // so fall through to the default, and check for longer matches
      default:
	 if (Fr_isprint(*input))
	    result = convert_Lisp_form(input,digits+1) ;
	 else
	    {
	    // not a recognized form, so read it as a symbol beginning with '#'
	    input = orig_input ; // unget everything
	    result = FramepaC_string_to_unquoted_Symbol(input,0) ;
	    }
	 break ;
      }
   return result ;
}

//----------------------------------------------------------------------

static bool verify_long_Lisp_form(const char *&input, const char *,
				    bool strict)
{
   int count = 0 ;
   char buf[FrMAX_SYMBOLNAME_LEN+1] ;
   const char *orig_input = input ;

   while (count < FrMAX_SYMBOLNAME_LEN && (Fr_isalpha(*input) || *input == '*'))
      buf[count++] = *input++ ;
   buf[count] = '\0' ;
   FrReader *reader = FrReader::firstReader() ;
   while (reader)
      {
      if (Fr_stricmp(reader->lispFormName(),buf) == 0)
	 return reader->validString(input,strict) ;
      reader = reader->nextReader() ;
      }
   // if we got here, the name didn't match, so treat it as a symbol starting
   // with #
   input = orig_input ;
   return verify_unquoted_Symbol(input,0,strict) ;
}

//----------------------------------------------------------------------

static bool verify_Lisp_form(const char *&input, const char *, bool strict)
{
   bool result = false ;
   bool widechar ;
   char digits[100] ;
   int numdigits = 1 ;
   const char *orig_input = input ;

   input++ ;   // consume the '#'
   while (Fr_isdigit(*input) && numdigits < (int)sizeof(digits)-1)
      digits[numdigits++] = *input++ ;
   digits[numdigits] = '\0' ; 			// ensure proper termination
   switch (*input)
      {
      case '(':
	 {
	 FrReader *reader = FrReader::firstReader() ;
	 while (reader)
	    {
	    const char *name = reader->lispFormName() ;
	    if (name[0] == '\0')	//  find reader for ""
	       return reader->validString(input,strict) ;
	    reader = reader->nextReader() ;
	    }
	 // no vector reader linked in, so read it as a list
	 widechar = read_extended_strings(false) ; // use Lisp compatibility
	 result = FramepaC_readtable->validString(input,strict) ;
	 read_extended_strings(widechar) ;
	 break ;
	 }
      case '<':
	 while (*input && *input != '>')
	    input++ ;
	 if (*input == '>')
	    {
	    input++ ;
	    result = true ;
	    }
	 else
	    result = false ;
	 break ;
      case '\'':
	 // expand #'foo to (FUNCTION foo)
	 input++ ;			// consume the quote
	 result = FramepaC_readtable->validString(input,strict) ;
	 break ;
      case '\\':
	 // treat #\a as the symbol |a|
	 input++ ;			// consume the backslash
	 input++ ;			// consume the character
	 result = true ;
	 break ;
      case ':':				// uninterned symbol
	 // since FramepaC has no uninterned symbols, just ignore the #: and
	 // read a normal symbol
	 input++ ;			// consume the colon
	 result = verify_unquoted_Symbol(input,0,strict) ;
	 break ;
      case '|':
	 // balanced comment
	 input++ ;			// throw out the bar
	 do {
	    while (*input && *input != '|')
	       input++ ;		// skip to matching vertical bar
	    if (*input)
	       input++ ;		// consume the vertical bar
	    } while (*input && *input != '#') ;
	 if (*input == '#')
	    {
	    input++ ;			// throw out the final hash
	    FrSkipWhitespace(input) ;
	    }
	 if (*input == ')' && FramepaC_read_nesting_level > 0)
	    result = true ;
	 else
	    result = FramepaC_readtable->validString(input,strict) ;
	 break ;
      case '#':				// shared object reference
	 input++ ;			// consume the hash mark
	 result = true ;
         break ;
      case '=':				// shared object definition
	 input++ ;			// consume the equal sign
	 result = FramepaC_readtable->validString(input,strict) ;
         break ;
      case ')':
	 // this wasn't actually a Lisp form, it was a standalone '#' at the
	 //   end of a list!
	 result = true ;
	 break ;
      case 'r':
      case 'R':
      case 'x':
      case 'X':
	 {
	 FrReader *reader = FrReader::firstReader() ;
	 while (reader)
	    {
	    const char *name = reader->lispFormName() ;
	    if (name[0] == Fr_toupper(*input) && name[1] == '\0')
	       return reader->validString(input,strict,digits+1) ;
	    reader = reader->nextReader() ;
	    }
	 }
	 // if we got here, there was no matching single-character form name,
	 // so fall through to the default, and check for longer matches
      default:
	 if (Fr_isprint(*input))
	    result = verify_long_Lisp_form(input,digits+1,strict) ;
	 else
	    {
	    // not a recognized form, so read it as a symbol beginning with '#'
	    input = orig_input ; // unget everything
	    result = verify_unquoted_Symbol(input,0,strict) ;
	    }
	 break ;
      }
   return result ;
}

//----------------------------------------------------------------------

FrSymbol *FramepaC_string_to_unquoted_Symbol(const char *&input, const char *)
{
   register int count = 0 ;
   register const char *in = input ;
   char name[FrMAX_SYMBOLNAME_LEN+1] ;

   while ((name[count] = Fr_issymbolchar(*in)) != '\0' &&
	  count < FrMAX_SYMBOLNAME_LEN)
      {
      count++ ;
      in++ ;
      }
   if (count >= FrMAX_SYMBOLNAME_LEN)
      {
      // consume the rest of the symbol
      while (Fr_issymbolchar(*in))
	 in++ ;
      FrWarning(errmsg_symname_too_long) ;
      name[FrMAX_SYMBOLNAME_LEN] = '\0' ; // ensure proper string termination
      }
   input = in ;
   return FrSymbolTable::add(name) ;
}

//----------------------------------------------------------------------

static bool verify_unquoted_Symbol(const char *&input, const char *,
				     bool strict)
{
   register int count = 0 ;
   register const char *in = input ;

   while (count < FrMAX_SYMBOLNAME_LEN)
      {
      int c = *((unsigned char*)in) ;
      if (!Fr_issymbolchar(c))
	 break ;
      in++ ;
      }
   if (count >= FrMAX_SYMBOLNAME_LEN)
      {
      // consume the rest of the symbol
      while (Fr_issymbolchar(*in))
	 in++ ;
      }
   input = in ;
   // if 'strict' test, we can't have hit the end of the string yet, since
   // that means we might have only part of the symbol's name so far
   return (!strict || *in != '\0') ;
}

//----------------------------------------------------------------------

static FrObject *string_to_dummy_Symbol(const char *&input, const char *)
{
   char name[2] ;

   name[0] = *input++ ;
   name[1] = '\0' ;
   return FrSymbolTable::add(name) ;
}

//----------------------------------------------------------------------

static bool verify_dummy_Symbol(const char *&input, const char *,
				  bool /*strict*/)
{
   // since dummy symbols are always exactly one character in length, if we
   // get here, we (by definition) have a valid dummy symbol
   input++ ;				// consume the character
   return true ;
}

//----------------------------------------------------------------------

void initialize_FrReadTable()
{
   if (!FramepaC_readtable)
      FramepaC_readtable = new FrReadTable ;
}


// end of file frreader.cpp //
