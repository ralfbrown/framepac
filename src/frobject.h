/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File: frobject.h	 class FrObject 				*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,1998,1999,2001,2006,2009,2015	*/
/*		 Ralf Brown/Carnegie Mellon University			*/
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

#ifndef __FROBJECT_H_INCLUDED
#define __FROBJECT_H_INCLUDED

#ifndef __FRCOMMON_H_INCLUDED
#include "frcommon.h"
#endif

#ifndef __FRCTYPE_H_INCLUDED
#include "frctype.h"
#endif

#ifndef __FRMEM_H_INCLUDED
#include "frmem.h"
#endif

#if defined(__GNUC__)
#  pragma interface
#endif

/**********************************************************************/
/**********************************************************************/

typedef bool (*FrIteratorFunc)(const FrObject *, va_list) ;
typedef bool (*FrCompareFunc)(const FrObject *,const FrObject *) ;

/**********************************************************************/
/*	declaration of class FrObject				      */
/**********************************************************************/

class FrObject
   {
   private:
      // none
   public:
      void *operator new(size_t size) { return FrMalloc(size) ; }
      void operator delete(void *block) { FrFree(block) ; }
#ifdef FrOBJECT_VIRTUAL_DTOR
      virtual ~FrObject() {}
#endif /* FrOBJECT_VIRTUAL_DTOR */
      virtual FrObjectType objType() const ;
      virtual const char *objTypeName() const ;
      virtual FrObjectType objSuperclass() const ;
      virtual FrReader *objReader() const { return 0 ; }
      virtual ostream &printValue(ostream &output) const ;
      virtual char *displayValue(char *buffer) const ;
      virtual size_t displayLength() const ;
      virtual FrObject *copy() const ;
      virtual FrObject *deepcopy() const ;
      virtual unsigned long hashValue() const ;
      bool __FrCDECL iterate(FrIteratorFunc func, ...) const ;
   //virtual member functions for type determination
      virtual bool atomp() const ;
      virtual bool consp() const ;
      virtual bool symbolp() const ;
      virtual bool numberp() const ;
      virtual bool floatp() const ;
      virtual bool stringp() const ;
      virtual bool framep() const ;
      virtual bool structp() const ;
      virtual bool hashp() const ;
      virtual bool queuep() const ;
      virtual bool arrayp() const ;
      virtual bool widgetp() const ;
      virtual bool vectorp() const ;
   //virtual member functions for increased performance/ease-of-use
      virtual void freeObject() ;
      virtual const char *printableName() const ;
      virtual FrSymbol *coerce2symbol(FrCharEncoding = FrChEnc_Latin1) const ;
      virtual size_t length() const ;
      virtual FrObject *car() const ;
      virtual FrObject *cdr() const ;
      virtual bool equal(const FrObject *obj) const ;
      virtual int compare(const FrObject *obj) const ;
      virtual bool iterateVA(FrIteratorFunc func, va_list) const ;
      virtual FrObject *subseq(size_t start, size_t stop) const ;
      virtual FrObject *reverse() ;
      virtual FrObject *getNth(size_t N) const ;
      virtual bool setNth(size_t N, const FrObject *elt) ;
      virtual size_t locate(const FrObject *item,
			    size_t start = (size_t)-1) const ;
      virtual size_t locate(const FrObject *item,
			    FrCompareFunc func,
			    size_t start = (size_t)-1) const ;
      virtual FrObject *insert(const FrObject *,size_t location,
			        bool copyitem = true) ;
      virtual FrObject *elide(size_t start, size_t end) ;
      virtual FrObject *removeDuplicates() const ;
      virtual FrObject *removeDuplicates(FrCompareFunc cmp) const ;
      virtual bool expand(size_t increment) ;
      virtual bool expandTo(size_t newsize) ;
      virtual long int intValue() const ;
      virtual double floatValue() const ;
      virtual double fraction() const ;
      virtual double real() const ;
      virtual double imag() const ;
   // non-virtual functions which dispatch to the proper virtual function
   // after checking for non-null object
      ostream &print(ostream &out) const ;
      char *print() const ;  // returns buffer which must be FrFree()d
      char *print(char *buf) const ;
      size_t printSize() const ;
   // a helper function for debugging with e.g. GDB
      void _() const ;   // does print(cerr)
   //overloaded operators
   } ;

istream &operator >> (istream &input, FrObject *&obj) ;
ostream &operator << (ostream &output, const FrObject *object) ;

//----------------------------------------------------------------------
// non-member functions related to class FrObject

inline int CONSP(const FrObject *x) { return ((x)&&(x)->consp()) ; }
inline int ATOMP(const FrObject *x) { return ((x)&&(x)->atomp()) ; }
inline int NUMBERP(const FrObject *x) { return ((x)&&(x)->numberp()) ; }
inline int STRINGP(const FrObject *x) { return ((x)&&(x)->stringp()) ; }
inline int SYMBOLP(const FrObject *x) { return ((x)&&(x)->symbolp()) ; }
inline int FRAMEP(const FrObject *x) { return ((x)&&(x)->framep()) ; }
inline int STRUCTP(const FrObject *x) { return ((x)&&(x)->structp()) ; }
inline int HASHP(const FrObject *x) { return ((x)&&(x)->hashp()) ; }
inline int QUEUEP(const FrObject *x) { return ((x)&&(x)->queuep()) ; }
inline int ARRAYP(const FrObject *x) { return ((x)&&(x)->arrayp()) ; }
inline int VECTORP(const FrObject *x) { return ((x)&&(x)->vectorp()) ; }

inline void free_object(FrObject *obj) { if (obj) obj->freeObject() ; }

size_t FrObject_string_length(const FrObject *obj) ;

//----------------------------------------------------------------------
// non-member variables related to class FrObject

extern FrObject NIL ;

#ifdef FrSYMBOL_VALUE
extern FrObject UNBOUND ;
#endif

/**********************************************************************/
/**********************************************************************/

class FrAtom : public FrObject
   {
   private:
      //nothing
   public:
      virtual FrObjectType objType() const ;
      virtual const char *objTypeName() const ;
      virtual FrObjectType objSuperclass() const ;
      virtual bool atomp() const ;
   } ;

/**********************************************************************/
/*	 overloaded operators (non-member functions)		      */
/**********************************************************************/

inline ostream &operator << (ostream &output,const FrObject &object)
{
   FramepaC_bgproc() ;
   return object.printValue(output) ;
}

/**********************************************************************/
/**********************************************************************/

size_t Fr_number_length(unsigned long number) ;
size_t FrDisplayWidth(size_t new_width) ;

#endif /* !__FROBJECT_H_INCLUDED */

// end of file frobject.h //
