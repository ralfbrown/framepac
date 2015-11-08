/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frstruct.h	class FrStruct (Lisp-style structure)		*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,2001,2006,2007,2009		*/
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

#ifndef __FRSTRUCT_H_INCLUDED
#define __FRSTRUCT_H_INCLUDED

#ifndef __FROBJECT_H_INCLUDED
#include "frobject.h"
#endif

#ifndef __FRREADER_H_INCLUDED
#include "frreader.h"
#endif

#if defined(__GNUC__)
#  pragma interface
#endif

/************************************************************************/
/************************************************************************/

class FrStructField
   {
   private:
      static FrAllocator allocator ;
      FrStructField *m_next ;
      const FrSymbol *m_name ;
      const FrObject *m_value ;
   private: // methods
      FrStructField *next() const { return m_next ; }
      const FrSymbol *name() const { return m_name ; }
   public:
      void *operator new(size_t) { return allocator.allocate() ; }
      void operator delete(void *block) { allocator.release(block) ; }
      FrStructField(const FrSymbol *n = 0, const FrObject *v = 0)
	    { m_name = n ; set(v) ; m_next = 0 ; }
      ~FrStructField() ;
      FrObject *get() const { return (FrObject *)m_value ; }
      void set(const FrObject *v) { m_value = v ; }
      FrStructField *copy() const ;
      FrStructField *deepcopy() const ;
      friend class FrStruct ;
   } ;

//----------------------------------------------------------------------

class FrStruct : public FrObject
   {
   private:
      static FrAllocator allocator ;
      static FrReader reader ;
   protected:
      const FrSymbol *m_typename ;
      FrStructField *m_fields ;
      void putnew(const FrSymbol *fieldname, const FrObject *fieldvalue) ;
   public:
      void *operator new(size_t) { return allocator.allocate() ; }
      void operator delete(void *block) { allocator.release(block) ; }
      FrStruct(const FrSymbol *type = 0) ;
      virtual ~FrStruct() ;
      virtual FrObjectType objType() const ;
      virtual const char *objTypeName() const ;
      virtual FrObjectType objSuperclass() const ;
      virtual bool structp() const ;
      virtual FrReader *objReader() const { return &reader ; }
      virtual ostream &printValue(ostream &output) const ;
      virtual char *displayValue(char *buffer) const ;
      virtual size_t displayLength() const ;
      virtual void freeObject() ;
      virtual FrObject *copy() const ;
      virtual FrObject *deepcopy() const ;
      virtual bool iterateVA(FrIteratorFunc, va_list args) const ;
      void setStructType(const FrSymbol *name) { m_typename = name ; }
      FrObject *get(const FrSymbol *fieldname) const ;
      FrObject *get(const char *fieldname) const ;
      FrStructField *getField(const FrSymbol *fieldname) ;
      FrStructField *getField(const char *fieldname) ;
      void put(const FrSymbol *fieldname, const FrObject *fieldvalue) ;
      void put(const FrSymbol *fieldname, const FrObject *fieldvalue,
	       bool copy) ;
      bool remove(const FrSymbol *fieldname) ;
      // access to internal state
      FrSymbol *typeName() const { return (FrSymbol*)m_typename ; }
      FrList *fieldNames() const ;
   //friend functions
      friend FrObject *read_FrStruct(istream &, const char *) ;
      friend FrObject *string_to_FrStruct(const char *&, const char *) ;
   } ;

/************************************************************************/
/************************************************************************/

#endif /* !__FRSTRUCT_H_INCLUDED */

// end of file frstruct.h //
