/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frreader.h		FramepaC object reader (input funcs)	*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,1998,2001,2009,2013		*/
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

#ifndef __FRREADER_H_INCLUDED
#define __FRREADER_H_INCLUDED

#ifndef __FRMEM_H_INCLUDED
#include "frmem.h"
#endif

#if defined(__GNUC__)
#  pragma interface
#endif

//----------------------------------------------------------------------

typedef FrObject *FrReadStringFunc(const char *&input, const char *digits) ;
typedef FrObject *FrReadStreamFunc(istream &input, const char *digits) ;
typedef bool FrReadVerifyFunc(const char *&input, const char *digits,
				bool strict) ;

#define FrREADER_LEADIN_LISPFORM (-1)
#define FrREADER_LEADIN_CHARSET  (-2)

//----------------------------------------------------------------------

class FrReadTable
   {
   private:
      FrReadStringFunc *string[256] ;
      FrReadStreamFunc *stream[256] ;
      FrReadVerifyFunc *verify[256] ;
   public:
      FrReadTable() ;
      ~FrReadTable() ;
      void setReader(char c,FrReadStringFunc *) ;
      void setReader(char c,FrReadStreamFunc *) ;
      void setReader(char c,FrReadVerifyFunc *) ;
      void setReader(char c,FrReadStringFunc *,FrReadStreamFunc *,
		     FrReadVerifyFunc *) ;
      FrReadStringFunc *getStringReader(char c) const
#if CHAR_BIT == 8
	    { return string[(unsigned char)c] ; }
#else
	    { return ((unsigned char)c < lengthof(string))
	        ? string[(unsigned char)c] : 0 ; }
#endif /* CHAR_BIT == 8 */
      FrReadStreamFunc *getStreamReader(char c) const
#if CHAR_BIT == 8
	    { return stream[(unsigned char)c] ; }
#else
	    { return ((unsigned char)c < lengthof(stream))
	        ? stream[(unsigned char)c] : 0 ; }
#endif /* CHAR_BIT == 8 */
      FrReadVerifyFunc *getVerifier(char c) const
#if CHAR_BIT == 8
	    { return verify[(unsigned char)c] ; }
#else
	    { return ((unsigned char)c < lengthof(stream))
	        ? verify[(unsigned char)c] : 0 ; }
#endif /* CHAR_BIT == 8 */
      bool validString(const char *&input, bool strict = false) const ;
      FrObject *readString(const char *&input) const ;
      FrObject *readStream(istream &input) const ;
   } ;

//----------------------------------------------------------------------

class FrReader
   {
   private:
      static FrReader *reader_list ;
      FrReader *next ;
      char *lead_string ;
      FrReadStringFunc *string ;
      FrReadStreamFunc *stream ;
      FrReadVerifyFunc *verify ;
      int lead_char ;
   public:
      void *operator new(size_t size) { return FrMalloc(size) ; }
      void operator delete(void *blk) { FrFree(blk) ; }
      FrReader(FrReadStringFunc*,FrReadStreamFunc*,FrReadVerifyFunc*,
	       int leadin_char, const char *leadin_chars = 0) ;
      ~FrReader() ;
      const char *lispFormName() const
	 { return (lead_char == FrREADER_LEADIN_LISPFORM) ? lead_string : 0 ; }
      FrReader *nextReader() const { return next ; }
      static FrReader *firstReader() { return reader_list ; }
      FrObject *readString(const char *&input, const char *digits = 0) const
	    { return string ? string(input,digits) : 0 ; }
      bool validString(const char *&input, bool strict = false,
			 const char *digits = 0) const
	    { return verify ? verify(input,digits,strict) : false ; }
      FrObject *readStream(istream &input, const char *digits = 0) const
	    { return stream ? stream(input,digits) : 0 ; }

      void FramepaC_shutdown() ;
   } ;

//----------------------------------------------------------------------


extern FrReadTable *FramepaC_readtable ;

bool read_virtual_frames(bool frames_read_as_virtual) ;
bool read_extended_strings(bool read) ;

inline FrObject *read_FrObject(istream &input)
   { return FramepaC_readtable->readStream(input) ; }
inline FrObject *string_to_FrObject(const char *&input)
   { return FramepaC_readtable->readString(input) ; }
inline FrObject *string_to_FrObject(char *&input)
   { const char *in = input ;
     FrObject *obj = FramepaC_readtable->readString(in) ;
     input = (char*)in ;
     return obj ; }

inline bool valid_FrObject_string(const char *&input,bool strict = false)
   { return FramepaC_readtable->validString(input,strict) ; }
inline bool valid_FrObject_string(char *&input,bool strict = false)
   { const char *in = input ;
     bool valid = FramepaC_readtable->validString(in,strict) ;
     input = (char*)in ;
     return valid ;
   }

void initialize_FrReadTable() ;

#endif /* !__FRREADER_H_INCLUDED */

// end of file frreader.h //
