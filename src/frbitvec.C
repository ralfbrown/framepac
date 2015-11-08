/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frbitvec.cpp	class FrBitVector				*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1997,1998,2001,2004,2009,2015				*/
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

#if defined(__GNUC__)
#  pragma implementation "frbitvec.h"
#endif

#include <stdlib.h>
#include "frcmove.h"
#include "frctype.h"
#include "frnumber.h"
#include "frreader.h"
#include "frstring.h"
#include "frpcglbl.h"
#include "frbitvec.h"

/************************************************************************/
/*	global variables for this module				*/
/************************************************************************/

static FrInteger _bitvector_one(1) ;
static FrInteger _bitvector_zero(0) ;

static const char errmsg_malformed_vector[] =
    "malformed bit-vector encountered on input" ;

//----------------------------------------------------------------------

static FrBitVector *read_bitvector(istream &input, const char *) ;
static FrBitVector *string_to_bitvector(const char *&input, const char *) ;
static bool verify_bitvector(const char *&input, const char *, bool) ;

static FrReader FrBitVector_reader((FrReadStringFunc*)string_to_bitvector,
				   (FrReadStreamFunc*)read_bitvector,
				   verify_bitvector,
				   FrREADER_LEADIN_LISPFORM,"*") ;

//----------------------------------------------------------------------

static const char num_set_bits[] =
   {
     0, 1, 1, 2, 1, 2, 2, 3,   1, 2, 2, 3, 2, 3, 3, 4,	 // 0x00 - 0x0F
     1, 2, 2, 3, 2, 3, 3, 4,   2, 3, 3, 4, 3, 4, 4, 5,   // 0x10 - 0x1F
     1, 2, 2, 3, 2, 3, 3, 4,   2, 3, 3, 4, 3, 4, 4, 5,   // 0x20 - 0x2F
     2, 3, 3, 4, 3, 4, 4, 5,   3, 4, 4, 5, 4, 5, 5, 6,   // 0x30 - 0x3F
     1, 2, 2, 3, 2, 3, 3, 4,   2, 3, 3, 4, 3, 4, 4, 5,   // 0x40 - 0x4F
     2, 3, 3, 4, 3, 4, 4, 5,   3, 4, 4, 5, 4, 5, 5, 6,   // 0x50 - 0x5F
     2, 3, 3, 4, 3, 4, 4, 5,   3, 4, 4, 5, 4, 5, 5, 6,   // 0x60 - 0x6F
     3, 4, 4, 5, 4, 5, 5, 6,   4, 5, 5, 6, 5, 6, 6, 7,   // 0x70 - 0x7F
     1, 2, 2, 3, 2, 3, 3, 4,   2, 3, 3, 4, 3, 4, 4, 5,   // 0x80 - 0x8F
     2, 3, 3, 4, 3, 4, 4, 5,   3, 4, 4, 5, 4, 5, 5, 6,   // 0x90 - 0x9F
     2, 3, 3, 4, 3, 4, 4, 5,   3, 4, 4, 5, 4, 5, 5, 6,   // 0xA0 - 0xAF
     3, 4, 4, 5, 4, 5, 5, 6,   4, 5, 5, 6, 5, 6, 6, 7,   // 0xB0 - 0xBF
     2, 3, 3, 4, 3, 4, 4, 5,   3, 4, 4, 5, 4, 5, 5, 6,   // 0xC0 - 0xCF
     3, 4, 4, 5, 4, 5, 5, 6,   4, 5, 5, 6, 5, 6, 6, 7,   // 0xD0 - 0xDF
     3, 4, 4, 5, 4, 5, 5, 6,   4, 5, 5, 6, 5, 6, 6, 7,   // 0xE0 - 0xEF
     4, 5, 5, 6, 5, 6, 6, 7,   5, 6, 6, 7, 6, 7, 7, 8,   // 0xF0 - 0xFF
   } ;

/************************************************************************/
/*	Helper Functions						*/
/************************************************************************/

#define update_vecthash FramepaC_update_ulong_hash

//----------------------------------------------------------------------

bool booleanValue(const FrObject *obj)
{
   if (!obj || obj == symbolNIL)
      return false ;
   else if (obj == symbolT)
      return true ;
   else if (obj->symbolp())
      {
      const char *name = ((FrSymbol*)obj)->symbolName() ;
      char firstch = Fr_toupper(name[0]) ;
      if (firstch == 'Y' || firstch == 'T')
	 return true ;
      else
	 return false ;
      }
   else if (obj->stringp())
      {
      FrString *str = (FrString*)obj ;
      FrChar_t firstch = str->nthChar(0) ;
      if (firstch == (FrChar_t)-1)
	 return false ;
      if (Fr_is8bit(firstch))
	  firstch = Fr_toupper(firstch) ;
      if (firstch == 'Y' || firstch == 'T')
	 return true ;
      else
	 return false ;
      }
   else
      return obj->intValue() != 0 ;
}

/************************************************************************/
/*	methods for class FrBitVector					*/
/************************************************************************/

FrBitVector::FrBitVector(size_t len)
{
   if (len == 0)
      len = FrDEFAULT_BITVECTOR_LEN ;
   size = (len + FrBITS_PER_UINT-1) / FrBITS_PER_UINT ;
   vector = FrNewC(unsigned int,size) ;
   _length = 0 ;
   if (vector)
      _length = len ;
   else
      size = 0 ;
   return ;
}

//----------------------------------------------------------------------

FrBitVector::FrBitVector(size_t len, unsigned int *vect)
{
   _length = len ;
   size = (len + FrBITS_PER_UINT-1) / FrBITS_PER_UINT ;
   vector = vect ;
   return ;
}

//----------------------------------------------------------------------

FrBitVector::FrBitVector(size_t len, const FrList *init)
{
   _length = init->listlength() ;
   if (len > _length)
      _length = len ;
   vector = nullptr ;
   if (_length)
      {
      size = (_length + FrBITS_PER_UINT-1) / FrBITS_PER_UINT ;
      vector = FrNewC(unsigned int,size) ;
      if (vector)
	 {
	 for (size_t loc = 0 ; init ; init = init->rest(), loc++)
	    {
	    setBit(loc,booleanValue(init->first())) ;
	    }
	 }
      else
	 size = _length = 0 ;
      }
   else
      size = 0 ;
   return ;
}

//----------------------------------------------------------------------

FrBitVector::FrBitVector(const FrBitVector &vect) : FrObject()
{
   _length = vect._length ;
   size = vect.size ;
   vector = FrNewN(unsigned int,size) ;
   if (vector)
      {
      memcpy(vector,vect.vector,_length/CHAR_BIT) ;
      }
   else
      size = _length = 0 ;
   return ;
}

//----------------------------------------------------------------------

FrBitVector::~FrBitVector()
{
   if (vector)
      FrFree(vector) ;
   return ;
}

//----------------------------------------------------------------------

void FrBitVector::freeObject()
{
   delete this ;
   return ;
}

//----------------------------------------------------------------------

const char *FrBitVector::objTypeName() const
{
   return "FrBitVector" ;
}

//----------------------------------------------------------------------

FrObjectType FrBitVector::objType() const
{
   return OT_FrBitVector ;
}

//----------------------------------------------------------------------

FrObjectType FrBitVector::objSuperclass() const
{
   return OT_FrObject ;
}

//----------------------------------------------------------------------

bool FrBitVector::vectorp() const
{
   return true ;
}

//----------------------------------------------------------------------

ostream &FrBitVector::printValue(ostream &output) const
{
   output << "#*" ;
   for (size_t i = 0 ; i < _length ; i++)
      output << (char)('0'+getBit(i)) ;
   return output ;
}

//----------------------------------------------------------------------

char *FrBitVector::displayValue(char *buffer) const
{
   *buffer++ = '#' ;
   *buffer++ = '*' ;
   for (size_t i = 0 ; i < _length ; i++)
      *buffer++ = (char)('0'+getBit(i)) ;
   *buffer = '\0' ;
   return buffer ;
}

//----------------------------------------------------------------------

size_t FrBitVector::displayLength() const
{
   return _length + 2 ;
}

//----------------------------------------------------------------------

FrObject *FrBitVector::copy() const
{
   return new FrBitVector(*this) ;
}

//----------------------------------------------------------------------

FrObject *FrBitVector::deepcopy() const
{
   return new FrBitVector(*this) ;
}

//----------------------------------------------------------------------

unsigned long FrBitVector::hashValue() const
{
   unsigned long hash = _length ;
   for (size_t i = 0 ; i < size ; i++)
      hash = update_vecthash(hash,vector[i]) ;
   return hash ;
}

//----------------------------------------------------------------------

size_t FrBitVector::length() const
{
   return _length ;
}

//----------------------------------------------------------------------

bool FrBitVector::equal(const FrObject *obj) const
{
   if (!obj)
      return false ;
   if (obj->vectorp())
      {
      FrBitVector *v = (FrBitVector*)obj ;
      if (v->_length != _length)
	 return false ;
      for (size_t i = 0 ; i < size ; i++)
	 {
	 if (vector[i] != v->vector[i])
	    return false ;
	 }
      return true ;
      }
   else
      return false ;
}

//----------------------------------------------------------------------

int FrBitVector::compare(const FrObject *obj) const
{
   if (!obj)
      return 1 ;			// anything is greater than NIL
   else if (!obj->vectorp())
      return -1 ;			// non-vectors after vectors
   FrBitVector *v = (FrBitVector*)obj ;
   size_t common = FrMin(_length,v->_length) ;
   common = (common + FrBITS_PER_UINT-1) / FrBITS_PER_UINT ;
   for (size_t i = 0 ; i < common ; i++)
      {
      if (vector[i] != v->vector[i])
	 {
	 // the vectors differ in the current int, so see which is later
	 // (we can't just compare the ints, because the low bit is most
	 // significant in the vector comparison)
	 for (size_t j = FrBITS_PER_UINT*i ; j < _length ; j++)
	    {
	    int b1 = getBit(j) ;
	    int b2 = v->getBit(j) ;
	    if (b1 < b2)
	       return 1 ;
	    else if (b1 > b2)
	       return -1 ;
	    }
	 }
      }
   // if we get to this point, the common prefixes of the vectors are equal,
   // so the longer one should come later
   if (_length < v->_length)
      return 1 ;
   else if (_length > v->_length)
      return -1 ;
   return 0 ;				// vectors are equal!
}

//----------------------------------------------------------------------

bool FrBitVector::iterateVA(FrIteratorFunc func, va_list args) const
{
   bool success = true ;
   for (size_t i = 0 ; i < _length && success ; i++)
      {
      FrSafeVAList(args) ;
      if (!func(getBit(i) ? &_bitvector_one : &_bitvector_zero,
		FrSafeVarArgs(args)))
	 success = false ;
      FrSafeVAListEnd(args) ;
      }
   return success ;
}

//----------------------------------------------------------------------

FrObject *FrBitVector::subseq(size_t start, size_t stop) const
{
   if (stop >= _length)
      stop = _length-1 ;
   if (start > stop)
      return 0 ;
   FrBitVector *newvect = new FrBitVector(stop-start+1) ;
   if (newvect)
      {
      for (size_t i = start ; i <= stop ; i++)
	 {
	 // !!! this can be sped up considerably....
	 newvect->setBit(i-start,getBit(i)) ;
	 }
      }
   return newvect ;
}

//----------------------------------------------------------------------

FrObject *FrBitVector::reverse()
{
   for (size_t i = 0 ; i < _length/2 ; i++)
      {
      // !!! this can be sped up considerably....
      int j = _length - i - 1 ;
      int b1 = getBit(i) ;
      int b2 = getBit(j) ;
      setBit(i,b2) ;
      setBit(j,b1) ;
      }
   return this ;
}

//----------------------------------------------------------------------

FrObject *FrBitVector::getNth(size_t N) const
{
   if (getBit(N))
      return &_bitvector_one ;
   else
      return &_bitvector_zero ;
}

//----------------------------------------------------------------------

bool FrBitVector::setNth(size_t N, const FrObject *elt)
{
   return setBit(N,booleanValue(elt)) ;
}

//----------------------------------------------------------------------

bool FrBitVector::setBit(size_t N, bool newvalue)
{
   if (N >= _length)
      return false ;
   size_t loc = N / FrBITS_PER_UINT ;
   size_t mask = 1U << (N % FrBITS_PER_UINT) ;
   if (newvalue)
      vector[loc] |= mask ;
   else
      vector[loc] &= ~mask ;
   return true ;
}

//----------------------------------------------------------------------

bool FrBitVector::setRange(size_t firstN, size_t lastN, bool newvalue)
{
   if (lastN >= _length)
      lastN = _length - 1 ;
   if (lastN < firstN)
      lastN = firstN ;
   if (firstN >= _length)
      return false ;
   // not the most efficient implementation, but I just need something Q&D
   //   right now  !!!
   for ( ; firstN <= lastN ; firstN++)
      {
      size_t loc = firstN / FrBITS_PER_UINT ;
      size_t mask = 1U << (firstN % FrBITS_PER_UINT) ;
      if (newvalue)
	 vector[loc] |= mask ;
      else
	 vector[loc] &= ~mask ;
      }
   return true ;
}

//----------------------------------------------------------------------

bool FrBitVector::getBit(size_t N) const
{
   if (N >= _length)
      return false ;
   size_t loc = N / FrBITS_PER_UINT ;
   size_t mask = 1U << (N % FrBITS_PER_UINT) ;
   return (vector[loc] & mask) != 0 ;
}

//----------------------------------------------------------------------

size_t FrBitVector::locate(const FrObject *item, size_t start) const
{
   if (_length == 0)
      return (size_t)-1 ;
   if (start == (size_t)-1)
      start = 0 ;
   else
      start++ ;
   if (item && item->consp())
      {
//!!!
      bool bit = booleanValue(item) ;
      for (size_t pos = start ; pos < _length ; pos++)
	 if (getBit(pos) == bit)
	    return pos ;
      }
   else
      {
      bool bit = booleanValue(item) ;
      for (size_t pos = start ; pos < _length ; pos++)
	 if (getBit(pos) == bit)
	    return pos ;
      }
   return (size_t)-1 ;
}

//----------------------------------------------------------------------

size_t FrBitVector::locate(const FrObject *item, FrCompareFunc /*func*/,
			    size_t start) const
{
   if (_length == 0)
      return (size_t)-1 ;
   if (start == (size_t)-1)
      start = 0 ;
   else
      start++ ;
   if (item && item->consp())
      {
//!!!
      bool bit = booleanValue(item) ;
      for (size_t pos = start ; pos < _length ; pos++)
	 if (getBit(pos) == bit)
	    return pos ;
      }
   else
      {
      bool bit = booleanValue(item) ;
      for (size_t pos = start ; pos < _length ; pos++)
	 if (getBit(pos) == bit)
	    return pos ;
      }
   return (size_t)-1 ;
}

//----------------------------------------------------------------------

FrObject *FrBitVector::insert(const FrObject *newelt, size_t location,
				bool /*copyitem*/)
{
   if (newelt && newelt->consp())
      {
      const FrList *newelts = (FrList*)newelt ;
      size_t numbits = newelts->listlength() ;
      expand(numbits) ;
      // !!! this can sped up considerably....
      for (size_t i = _length-1 ; i >= location+numbits ; i--)
	 setBit(i,getBit(i-numbits)) ;
      for (size_t j = location ;
	   j < location+numbits && newelts ;
	   j++, newelts = newelts->rest())
	 {
	 setBit(j,booleanValue(newelts->first())) ;
	 }
      }
   else
      {
      expand(1) ;
      // !!! this can sped up considerably....
      for (size_t i = _length-1 ; i > location ; i--)
	 setBit(i,getBit(i-1)) ;
      setBit(location,booleanValue(newelt)) ;
      }
   return this ;
}

//----------------------------------------------------------------------

FrObject *FrBitVector::elide(size_t start, size_t end)
{
   if (start >= _length || start > end)
      return this ;
   size_t offset = end-start+1 ;
   for (size_t i = end+1 ; i < _length ; i++)
      {
      // !!! this can be sped up considerably....
      setBit(i-offset,getBit(i)) ;
      }
   _length -= offset ;
   size = (_length + FrBITS_PER_UINT-1) / FrBITS_PER_UINT ;
   vector = FrNewR(unsigned int,vector,size) ;
   return this ;
}

//----------------------------------------------------------------------

bool FrBitVector::expand(size_t increment)
{
   size_t newlen = _length + increment ;
   size_t newsize = (newlen + FrBITS_PER_UINT-1) / FrBITS_PER_UINT ;
   if (newlen < _length)		// did we wrap around size_t?
      {
      newlen = (size_t)((unsigned long)FrBITS_PER_UINT *
			(UINT_MAX/FrBITS_PER_UINT)) ;
      newsize = (size_t)(((unsigned long)UINT_MAX) / FrBITS_PER_UINT) ;
      }
   if (newsize == size)
      {
      _length = newlen ;
      return true ;
      }
   unsigned int *newvect = FrNewR(unsigned int,vector,newsize);
   if (newvect)
      {
      vector = newvect ;
      for (size_t i = size ; i < newsize ; i++)
	 {
	 vector[i] = 0 ;		// fill new part with zero bits
	 }
      _length = newlen ;
      size = newsize ;
      return true ;			// successfully expanded
      }
   else
      return false ;			// unable to expand
}

//----------------------------------------------------------------------

bool FrBitVector::expandTo(size_t newlen)
{
   if (newlen > _length)
      return expand(newlen-_length) ;
   else
      return true ;			// trivially successful
}

//----------------------------------------------------------------------

FrBitVector *FrBitVector::intersection(const FrBitVector *othervect) const
{
   size_t newlen = FrMin(_length,othervect->_length) ;
   FrBitVector *newvect = new FrBitVector(newlen) ;
   if (newvect)
      {
      size_t newsize = newvect->size ;
      for (size_t i = 0 ; i < newsize ; i++)
	 {
	 newvect->vector[i] = vector[i] & othervect->vector[i] ;
	 }
      }
   return newvect ;
}

//----------------------------------------------------------------------

size_t FrBitVector::intersectionBits(const FrBitVector *othervect) const
{
   size_t newlen = FrMin(_length,othervect->_length) ;
   size_t count = 0 ;
   size_t newsize = ((newlen+FrBITS_PER_UINT-1)/FrBITS_PER_UINT) *
		    sizeof(vector[0]) ;
   const unsigned char *v1 = (unsigned char*)vector ;
   const unsigned char *v2 = (unsigned char*)othervect->vector ;
   for (size_t i = 0 ; i < newsize ; i++)
      count += num_set_bits[(v1[i] & v2[i])] ;
   return count ;
}

//----------------------------------------------------------------------

bool FrBitVector::intersects(const FrBitVector *othervect) const
{
   size_t newlen = FrMin(_length,othervect->_length) ;
   size_t newsize = ((newlen+FrBITS_PER_UINT-1)/FrBITS_PER_UINT) *
		    sizeof(vector[0]) ;
   const unsigned char *v1 = (unsigned char*)vector ;
   const unsigned char *v2 = (unsigned char*)othervect->vector ;
   for (size_t i = 0 ; i < newsize ; i++)
      {
      if ((v1[i] & v2[i]) != 0)
	 return true ;
      }
   return false ;
}

//----------------------------------------------------------------------

FrBitVector *FrBitVector::vectorunion(const FrBitVector *othervect) const
{
   size_t newlen = FrMax(_length,othervect->_length) ;
   size_t minlen = FrMin(_length,othervect->_length) ;
   FrBitVector *newvect = new FrBitVector(newlen) ;
   minlen = ((newlen+FrBITS_PER_UINT-1)/FrBITS_PER_UINT) *
                   sizeof(vector[0]) ;
   if (newvect)
      {
      size_t newsize = newvect->size ;
      for (size_t i = 0 ; i < newsize ; i++)
	 {
	 if (i < minlen)
	    newvect->vector[i] = vector[i] | othervect->vector[i] ;
	 else if (i < _length)
	    newvect->vector[i] = vector[i] ;
	 else
	    newvect->vector[i] = othervect->vector[i] ;
	 }
      }
   return newvect ;
}

//----------------------------------------------------------------------

size_t FrBitVector::vectorunionBits(const FrBitVector *othervect) const
{
   size_t newlen = FrMax(_length,othervect->_length) ;
   size_t minlen = FrMin(_length,othervect->_length) ;
   size_t count = 0 ;
   size_t newsize = ((newlen+FrBITS_PER_UINT-1)/FrBITS_PER_UINT) *
		    sizeof(vector[0]) ;
   minlen = ((minlen+FrBITS_PER_UINT-1)/FrBITS_PER_UINT) *
		    sizeof(vector[0]) ;
   const unsigned char *v1 = (unsigned char*)vector ;
   const unsigned char *v2 = (unsigned char*)othervect->vector ;
   for (size_t i = 0 ; i < newsize ; i++)
      {
      if (i < minlen)
	 count += num_set_bits[(v1[i] | v2[i])] ;
      else if (i < _length)
	 count += num_set_bits[v1[i]] ;
      else
	 count += num_set_bits[v2[i]] ;
      }
   return count ;
}

//----------------------------------------------------------------------

FrBitVector *FrBitVector::difference(const FrBitVector *othervect) const
{
   size_t newlen = _length ;
   FrBitVector *newvect = new FrBitVector(newlen) ;
   if (newvect)
      {
      size_t newsize = newvect->size ;
      size_t othersize = othervect->size ;
      for (size_t i = 0 ; i < newsize ; i++)
	 {
	 if (i < othersize)
	    newvect->vector[i] = vector[i] & ~othervect->vector[i] ;
	 else
	    newvect->vector[i] = vector[i] ;
	 }
      }
   return newvect ;
}

//----------------------------------------------------------------------

size_t FrBitVector::differenceBits(const FrBitVector *othervect) const
{
   size_t othersize ;
   if (othervect)
      othersize = othervect->_length ;
   else
      othersize = 0 ;
   size_t count = 0 ;
   othersize *= sizeof(vector[0]) ;
   size_t newsize = size * sizeof(vector[0]) ;
   const unsigned char *v1 = (unsigned char*)vector ;
   const unsigned char *v2 = (unsigned char*)othervect->vector ;
   for (size_t i = 0 ; i < newsize ; i++)
      {
      if (i < othersize)
	 count += num_set_bits[(v1[i] & ~v2[i])] ;
      else
	 count += num_set_bits[v1[i]] ;
      }
   return count ;
}

//----------------------------------------------------------------------

void FrBitVector::clear()
{
   if (vector)
      memset(vector,'\0',size*sizeof(vector[0])) ;
}

//----------------------------------------------------------------------

void FrBitVector::negate()
{
   if (size > 1)
      {
      for (size_t i = 0 ; i < size-1 ; i++)
	 vector[i] = ~vector[i] ;
      }
   else if (size == 0)
      return ;
   unsigned int mask = 0 ;
   for (size_t i = (size-1)*FrBITS_PER_UINT ; i < _length ; i++)
      {
      mask |= 1U << (i % FrBITS_PER_UINT) ;
      }
   vector[size-1] ^= mask ;
}

//----------------------------------------------------------------------

size_t FrBitVector::countBits() const
{
   size_t bitcount = 0 ;
   const unsigned char *bits = (unsigned char *)vector ;
   for (size_t i = 0 ; i < size*sizeof(vector[0]) ; i++)
      bitcount += num_set_bits[bits[i]] ;
   return bitcount ;
}

/************************************************************************/
/************************************************************************/

static FrBitVector *read_bitvector(istream &input, const char *digits)
{
   FrBitVector *vector ;
   if (digits && *digits)
      {
      size_t len = atoi(digits) ;
      vector = new FrBitVector(len) ;
      if (!vector)
	 return 0 ;
      for (size_t i = 0 ; i < len ; i++)
	 {
	 int c = input.get() ;
	 if (c == '1')
	    vector->setBit(i) ;
	 else if (c != '0')
	    {
	    FrWarning(errmsg_malformed_vector) ;
	    if (c != EOF)
	       input.putback(trunc2char(c)) ;
	    break ;
	    }
	 }
      }
   else
      {
      vector = new FrBitVector(1) ;
      if (!vector)
	 return 0 ;
      size_t i = 0 ;
      int c ;
      while ((c = input.get()) == '0' || c == '1')
	 {
	 if (i >= vector->vectorlength())
	    vector->expand(1) ;
	 vector->setBit(i++,c-'0') ;
	 }
      if (c != EOF)
	 input.putback(trunc2char(c)) ;
      }
   return vector ;
}

//----------------------------------------------------------------------

static FrBitVector *string_to_bitvector(const char *&input, const char *digits)
{
   FrBitVector *vector ;
   const char *in = input ;
   if (digits && *digits)
      {
      size_t len = atoi(digits) ;
      vector = new FrBitVector(len) ;
      for (size_t i = 0 ; i < len ; i++, in++)
	 {
	 char c = *in ;
	 if (c == '1')
	    vector->setBit(i) ;
	 else if (c != '0')
	    {
	    FrWarning(errmsg_malformed_vector) ;
	    break ;
	    }
	 }
      }
   else
      {
      size_t len = 0 ;
      char c ;
      while ((c = *in) == '0' || c == '1')
	 {
	 len++ ;
	 in++ ;
	 }
      vector = new FrBitVector(len) ;
      size_t i ;
      for (in = input, i = 0 ; (c = *in) == '0' || c == '1' ; in++, i++)
	 vector->setBit(i,*in-'0') ;
      }
   input = in ;			// update caller's buffer pointer
   return vector ;
}

//----------------------------------------------------------------------

static bool verify_bitvector(const char *&input, const char *digits,
			       bool strict)
{
   const char *in = input ;
   if (digits && *digits)
      {
      size_t len = atoi(digits) ;
      for (size_t i = 0 ; i < len ; i++, in++)
	 {
	 char c = *in ;
	 if (c != '0' && c != '1')
	    {
	    input = in ;		// update caller's buffer pointer
	    return false ;
	    }
	 }
      input = in ;			// update caller's buffer pointer
      return true ;
      }
   else
      {
      size_t len = 0 ;
      char c ;
      while ((c = *in) == '0' || c == '1')
	 {
	 len++ ;
	 in++ ;
	 }
      input = in ;			// update caller's buffer pointer
      return (!strict || c != '\0') ;
      }
}

// end of file frbitvec.cpp //
