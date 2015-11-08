/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File: frstring.cpp	 	class FrString 				*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,1998,1999,2000,2001,2003,2004,	*/
/*		2006,2007,2008,2009,2011,2013,2014			*/
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
#  pragma implementation "frstring.h"
#endif

#include "frassert.h"
#include "frbytord.h"
#include "frcmove.h"
#include "frctype.h"
#include "frstring.h"
#include "frpcglbl.h"
#include "fr_mem.h"			// for MIN_ALLOC

#ifdef FrSTRICT_CPLUSPLUS
#  include <cstdlib>
#else
#  include <stdlib.h>
#endif /* FrSTRICT_CPLUSPLUS */

/************************************************************************/

int FrStringCmp(const void *s1, const void *s2, int length, int width1,
		int width2) ;
void *FrStrCpy(void *dest, const void *src, int length, int w1,int w2) ;

#define SHORT_STRING_LIMIT ((MIN_ALLOC + FrALIGN_SIZE -1) / FrALIGN_SIZE)

/************************************************************************/
/*    Global variables for class FrString				*/
/************************************************************************/

FrAllocator FrString::str_allocator("short strings",SHORT_STRING_LIMIT) ;
FrAllocator FrString::allocator("FrString",sizeof(FrString)) ;

/************************************************************************/
/*	Global data for this module				      	*/
/************************************************************************/

const char str_wordWrap[] = "wordWrap" ;

static const char errmsg_wrongsize[]
    = "invalid character size--empty string stored" ;
static const char nomem_ctor[]
    = "while constructing a FrString" ;
static const char nomem_inserting[]
    = "while inserting into a FrString" ;
static const char nomem_op_eq[]
    = "in FrString::operator=" ;

#if defined(__WATCOMC__) && defined(__386__)
extern unsigned long hashvalue1(const char *string, size_t length) ;
#pragma aux hashvalue1 = \
	"xor  eax,eax" \
	"xor  edx,edx" \
        "test ecx,ecx" \
	"jz   l2" \
    "l1: ror  eax,5" \
	"mov  dl,[ebx]" \
	"inc  ebx" \
	"add  eax,edx" \
	"dec  ecx" \
	"jne  l1" \
    "l2:" \
	parm [ebx][ecx] \
	value [eax] \
	modify exact [eax ebx ecx edx] ;

#elif defined(_MSC_VER) && _MSC_VER >= 800
inline unsigned long hashvalue1(const char *string, size_t strlength)
{
   unsigned long hash ;
   _asm {
          mov ebx,string ;
          xor eax,eax ;
	  mov ecx,strlength ;
	  xor edx,edx ;
          jcxz l2 ;
	} ;
     l1:
   _asm {
          ror eax,5 ;
	  mov dl,[ebx] ;
	  inc ebx ;
	  add eax,edx ;
	  dec ecx ;
	  jne l1 ;
	  mov hash,eax ;
        } ;
     l2:
   return hash ;
}

#elif defined(__GNUC__) && defined(__386__)
inline unsigned long hashvalue1(const char *name, int symlength)
{
   if (symlength)
      {
      unsigned long hashvalue ;
      unsigned long t1 ;
      __asm__("xor %0,%0\n\t"
	      "xor %1,%1\n"
	".LBL%=:\n\t"
	      "ror $5,%0\n\t"
	      "movb (%2),%b1\n\t"
	      "inc %2\n\t"
	      "add %1,%0\n\t"
	      "dec %3\n\t"
	      "jne .LBL%=\n\t"
	      : "=&r" (hashvalue), "=&q" (t1)
	      : "r" (name), "r" (symlength)
	      : "cc") ;
      return hashvalue ;
      }
   return 0 ;
}

#else
// shift counts for one-byte character string hashing
static int shifts1[16] = {  0, 23, 18, 13,  8,  3, 22, 17,
			   12,  7,  2, 21, 16, 11,  6,  1 } ;
inline unsigned long hashvalue1(const char *string, size_t length)
{
   unsigned long hash = 0 ;
   for (int i = length ; i > 0 ; i--)
      hash += (*((unsigned char*)string)++) << shifts1[i & 15] ;
   return hash ;
}
#endif /* __WATCOMC__ && __386__ */

// shift counts for two-byte character string hashing
static int shifts2[16] = {  0, 15, 10,  5, 14,  9,  4, 13,
			    8,  3, 12,  7,  2, 11,  6,  1 } ;


/*
// possible alternate to the above hash function: FNV-1a
//  URL: http://www.isthe.com/chongo/tech/comp/fnv/index.html

   hash = init
   foreach byte
      hash = (hash ^ byte) * FNV_prime
   return hash

for 32-bit hashes:
    init = 2166136261
    FNV_prime = 0x01000193 (16777619)

for 64-bit hashes:
    init = 14695981039346656037
    FNV_prime = 0x0100000001B3 (1099511628211)

for 128-bit hashes:
    init = 144066263297769815596495629667062367629
    FNV_prime = 2**88 + 0x13B (309485009821345068724781371)

x86 assembler by M.S.Schulte
fnv1a_32:
   mov esi, buffer
   mov ecx, length
   mov eax, basis
   mov edi, 01000193h ;fnv_32_prime
   xor ebx, ebx
 nexta:
   mov bl, [esi]
   xor eax, ebx
   mul edi
   inc esi
   dec ecx
   jnz nexta

*/

/************************************************************************/
/*    Global variables local to this module				*/
/************************************************************************/

/************************************************************************/
/*	Buffer Allocation						*/
/************************************************************************/

unsigned char *FrString::reserve(size_t siz)
{
   if (siz <= str_allocator.objectSize())
      return (unsigned char*)str_allocator.allocate() ;
   else
      return FrNewN(unsigned char,siz) ;
}

//----------------------------------------------------------------------

bool FrString::alloc(size_t siz)
{
   m_value = reserve(siz) ;
   return (m_value != 0) ;
}

//----------------------------------------------------------------------

bool FrString::realloc(size_t newsize)
{
   if (!stringValue())
      return alloc(newsize) ;
   size_t currsize = stringSize() + charWidth() ;
   if (newsize <= str_allocator.objectSize())
      {
      if (currsize > str_allocator.objectSize())
	 {
	 // need to copy from FrMalloc block into smaller FrAllocator block
	 void *newvalue = str_allocator.allocate() ;
	 if (newvalue)
	    {
	    memcpy(newvalue,m_value,newsize) ;
	    FrFree(m_value) ;		         // free the old block
	    m_value = (unsigned char*)newvalue ; //   and point at the new one
	    return true ;
	    }
	 }
      else
	 return true ;
      }
   else // newsize > str_allocator.objectSize()
      {
      if (currsize <= str_allocator.objectSize())
	 {
	 // need to copy from FrAllocator block into larger FrMalloc block
	 unsigned char *newvalue = FrNewN(unsigned char,newsize) ;
	 if (newvalue)
	    {
	    memcpy(newvalue,m_value,currsize) ;
	    str_allocator.release(m_value) ; 	// free the old block
	    m_value = (unsigned char*)newvalue ; //  and point at the new one
	    return true ;
	    }
	 }
      else
	 {
	 void *newvalue = FrNewR(unsigned char,m_value,newsize) ;
	 if (newvalue)
	    {
	    m_value = (unsigned char*)newvalue ;
	    return true ;
	    }
	 }
      }
   return false ;
}

//----------------------------------------------------------------------

void FrString::free()
{
   if (stringValue())
      {
      if (stringSize()+charWidth() <= str_allocator.objectSize())
	 str_allocator.release(m_value) ;
      else
	 FrFree(m_value) ;
      m_value = 0 ;
      setLength(0) ;
      setCharWidth(1) ;
      }
   return ;
}

/**********************************************************************/
/*    Member functions for class FrString			      */
/**********************************************************************/

FrString::FrString()
{
   setLength(0) ;
   setCharWidth(1) ;
   m_value = 0 ;
   return ;
}

//----------------------------------------------------------------------

FrString::FrString(const void *name)
{
   if (!name)
      name = "" ;
   size_t len = strlen((const char *)name) ;
   setCharWidth(1) ;
   setLength(len) ;
   if (alloc(len+1))
      memcpy(m_value,name,len+1) ;
   else
      {
      FrNoMemory(nomem_ctor) ;
      setLength(0) ;
      }
   return ;
}

//----------------------------------------------------------------------

FrString::FrString(const void *name, int len)
{
   setCharWidth(1) ;
   if (alloc(len+1))
      {
      setLength(len) ;
      memcpy(m_value,name,len) ;
      m_value[len] = '\0' ;
      return ;
      }
   else
      {
      FrNoMemory(nomem_ctor) ;
      setLength(0) ;
      }
   return ;
}

//----------------------------------------------------------------------

FrString::FrString(FrChar_t c, int width)
{
   setCharWidth(width) ;
   setLength(1) ;
   if (!alloc(1,width))
      {
      FrNoMemory(nomem_ctor) ;
      setLength(0) ;
      m_value[0] = '\0' ;
      }
   else if (width == 1)
      {
      m_value[0] = (unsigned char)c ;
      m_value[1] = '\0' ;
      }
   else if (width == 2)
      {
      FrStoreShort(c,m_value) ;
      ((uint16_t*)m_value)[1] = 0 ;
      }
   else if (width == 4)
      {
      FrStoreLong(c,m_value) ;
      ((uint32_t*)m_value)[1] = 0 ;
      }
   else
      {
      FrWarning(errmsg_wrongsize) ;
      setLength(0) ;
      m_value[0] = '\0' ;
      }
   return ;
}

//----------------------------------------------------------------------

FrString::FrString(const void *name, int len, int width)
{
   size_t n ;
   switch (width)
      {
      case 1:
	 if (alloc(len+1))
	    {
	    setLength(len) ;
	    setCharWidth(1) ;
	    memcpy(m_value,name,len) ;
	    m_value[len] = '\0' ;
	    return ;
	    }
	 break ;
      case 2:
	 if (alloc(len,2))
	    {
	    n = 2 * len ;
	    setLength(len) ;
	    setCharWidth(2) ;
	    memcpy(m_value,name,n) ;
	    m_value[n] = '\0' ;
	    m_value[n+1] = '\0' ;
	    return ;
	    }
	 break ;
      case 4:
	 if (alloc(len,4))
	    {
	    n = 4 * len ;
	    setLength(len) ;
	    setCharWidth(4) ;
	    memcpy(m_value,name,n) ;
	    memset(m_value+n,'\0',4) ;
	    return ;
	    }
	 break ;
      default:
	 FrWarning(errmsg_wrongsize) ;
	 setLength(0) ;
	 setCharWidth(1) ;
	 m_value = 0 ;
	 return ;
      }
   if (!stringValue())
      {
      FrNoMemory(nomem_ctor) ;
      setLength(0) ;
      setCharWidth(1) ;
      }
   return ;
}

//----------------------------------------------------------------------

FrString::FrString(char *name, int len, int width,bool copybuf)
{
   setCharWidth(width) ;
   if (copybuf)
     {
     if (alloc(len,width))
        memcpy(m_value,name,len*width) ;
     }
   else if ((size_t)(len+1)*width <= str_allocator.objectSize())
      {
      if (alloc(len,width))
	 memcpy(m_value,name,len*width) ;
      FrFree(name) ;
      }
   else
      m_value = (unsigned char *)name ;
   if (!stringValue())
      {
      setLength(0) ;
      return ;
      }
   setLength(len) ;
   if (width > 1)
      memset(m_value+len*width,'\0',width) ; // ensure proper NUL-termination
   else
      m_value[len*width] = '\0' ;
   return ;
}

//----------------------------------------------------------------------

void FrString::list_to_string(const FrList *words,
			       FrCharEncoding enc, bool addblanks)
{
   size_t len = 0 ;
   setCharWidth(1) ;
   if (!words)
      {
      m_value = 0 ;
      setLength(len) ;
      return ;
      }
   const FrList *w ;
   const FrList *rest ;
   for (w = words ; w ; w = rest)
      {
      rest = w->rest() ;
      FrString *word = (FrString*)w->first() ;
      if (word)
	 {
	 FrObjectType objtype = word->objType() ;
	 if (objtype == OT_FrString)
	    {
	    len += word->stringLength() ;
	    if (word->charWidth() > charWidth())
	       setCharWidth(word->charWidth()) ;
	    }
	 else if (objtype == OT_Symbol)
	    len += strlen(((FrSymbol*)word)->symbolName()) ;
	 else
	    len += word->displayLength() ;
	 }
      if (addblanks && rest)
	 len++ ;
      }
   if (!alloc(len,charWidth()))
      {
      FrNoMemory("while building string from word list") ;
      setLength(0) ;
      return ;
      }
   unsigned char *buf = m_value ;
   for (w = words ; w ; w = rest)
      {
      FrString *word = (FrString*)w->first() ;
      rest = w->rest() ;
      if (word)
	 {
	 const char *wordstr ;
	 FrObjectType objtype = word->objType() ;
	 if (objtype == OT_FrString)
	    {
	    wordstr = (const char *)word->stringValue() ;
	    buf = (unsigned char*)FrStrCpy(buf,wordstr,
					   word->stringLength(),charWidth(),
					   word->charWidth()) ;
	    }
	 else if (objtype == OT_Symbol)
	    {
	    char wstr[FrMAX_SYMBOLNAME_LEN+2] ;
	    const unsigned char *map = FrLowercaseTable(enc) ;
	    const char *n = ((FrSymbol*)word)->symbolName() ;
	    char *lc = wstr ;
	    while (*n)
	       *lc++ = (char)map[(unsigned char)*n++] ;
	    *lc = '\0' ;
	    size_t slen = strlen(wstr) ;
	    if (charWidth() == 1)
	       {
	       memcpy(buf,wstr,slen) ;
	       buf += slen ;
	       }
	    else
	       buf = (unsigned char*)FrStrCpy(buf,wstr,slen,charWidth(),1) ;
	    }
	 else
	    {
	    char *wstr = word->print() ;
	    if (wstr)
	       {
	       buf = (unsigned char*)FrStrCpy(buf,wstr,strlen(wstr),
					      charWidth(),1) ;
	       FrFree(wstr) ;
	       }
	    }
	 if (addblanks && rest)		// unless on last word, add a space
	    {				//   as separator
	    if (charWidth() == 1)
	       *buf++ = ' ' ;
	    else
	       buf = (unsigned char*)FrStrCpy(buf," ",1,charWidth(),1) ;
	    }
	 }
      }
   if (charWidth() > 1)
      memset(buf,'\0',charWidth()) ;
   else
      *buf = '\0' ;			// ensure proper string termination
   setLength(len) ;
   return ;
}

//----------------------------------------------------------------------

FrString::FrString(const FrList *words,FrCharEncoding enc,bool addblanks)
{
   list_to_string(words,enc,addblanks) ;
   return ;
}

//----------------------------------------------------------------------

FrString::FrString(const FrList *words, bool addblanks, bool lcsymbols)
{
   list_to_string(words,lcsymbols ? FrChEnc_Latin1 : FrChEnc_RawOctets,
		  addblanks) ;
   return ;
}

//----------------------------------------------------------------------

FrString::~FrString()
{
   free() ;
   setLength(0) ;
//   setCharWidth(0) ; //test
   return ;
}

//----------------------------------------------------------------------

FrObjectType FrString::objType() const
{
   return OT_FrString ;
}

//----------------------------------------------------------------------

const char *FrString::objTypeName() const
{
   return "FrString" ;
}

//----------------------------------------------------------------------

FrObjectType FrString::objSuperclass() const
{
   return OT_FrAtom ;
}

//----------------------------------------------------------------------

void FrString::freeObject()
{
   delete this ;
   return ;
}

//----------------------------------------------------------------------

bool FrString::stringp() const
{
   return true ;
}

//----------------------------------------------------------------------

long int FrString::intValue() const
{
   long intval = 0 ;
   if (stringValue())
      {
      char *end = 0 ;
      intval = strtol((char*)stringValue(),&end,0) ;
      if (!end || end == (char*)stringValue())
	 {
	 //FIXME: output error message?
	 }
      }
   return intval ;
}

//----------------------------------------------------------------------

const char *FrString::printableName() const
{
   return (char*)stringValue() ;
}

//----------------------------------------------------------------------

FrSymbol *FrString::coerce2symbol(FrCharEncoding enc) const
{
   return FrCvtString2Symbol(this,enc) ;
}

//----------------------------------------------------------------------

unsigned long FrString::hashValue(const char *s)
{
   if (s)
      {
      size_t len = strlen(s) ;
      return hashvalue1(s,len) ;
      }
   return 0 ;
}

//----------------------------------------------------------------------

unsigned long FrString::hashValue(const char *s, size_t len)
{
   return s ? hashvalue1(s,len) : 0 ;
}

//----------------------------------------------------------------------

unsigned long FrString::hashValue() const
{
   switch (charWidth())
      {
      case 1:
	 return hashvalue1(stringValue(),stringLength()) ;
      case 2:
	 {
	 const uint16_t *string = (uint16_t*)stringValue() ;
	 unsigned long hash = 0 ;
	 for (int i = stringLength() ; i > 0 ; i--)
	    hash += FrByteSwap16(*string++) << shifts2[i & 15] ;
	 return hash ;
	 }
      case 4:
	 {
	 const uint32_t *string = (uint32_t*)stringValue() ;
	 unsigned long hash = 0 ;
	 for (int i = stringLength() ; i > 0 ; i--)
	    hash += FrByteSwap32(*string++) ;
	 return hash ;
	 }
      default:
	 // can't happen!
	 bad_char_width("hashValue") ;
	 return (unsigned long)this ;
      }
}

//----------------------------------------------------------------------

size_t FrString::length() const
{
   return stringLength() ;
}

//----------------------------------------------------------------------

FrObject *FrString::subseq(size_t start, size_t stop) const
{
   size_t len = stringLength() ;
   if (stop >= len)
      stop = len-1 ;
   if (start >= len || start > stop)
      return new FrString("") ;
   else
      return new FrString(stringValue()+charWidth()*start,
			   (stop-start+1),charWidth()) ;
}

//----------------------------------------------------------------------

FrObject *FrString::reverse()
{
   size_t len = stringLength() ;
   if (len > 1)
      {
      switch (charWidth())
	 {
	 case 1:
	    {
	    unsigned char *val = (unsigned char*)stringValue() ;
	    unsigned char *last = val + len - 1 ;
	    while (val < last)
	       {
	       unsigned char c = *val ;
	       *val++ = *last ;
	       *last-- = c ;
	       }
	    }
	    break ;
	 case 2:
	    {
	    uint16_t *val = (uint16_t*)stringValue() ;
	    uint16_t *last = val + len - 1 ;
	    while (val < last)
	       {
	       uint16_t c = *val ;
	       *val++ = *last ;
	       *last-- = c ;
	       }
	    }
	    break ;
	 case 4:
	    {
	    uint32_t *val = (uint32_t*)stringValue() ;
	    uint32_t *last = val + len - 1 ;
	    while (val < last)
	       {
	       uint32_t c = *val ;
	       *val++ = *last ;
	       *last-- = c ;
	       }
	    }
	    break ;
	 default:
	    // can't happen!
	    bad_char_width("reverse") ;
	    break ;
	 }
      }
   return this ;
}

//----------------------------------------------------------------------

FrObject *FrString::getNth(size_t N) const
{
   if (N >= stringLength())
      return 0 ;
   else
      {
      FrChar_t c ;
      switch (charWidth())
	 {
	 case 1:
	    c = (FrChar_t) (unsigned char)m_value[N] ;
	    break ;
	 case 2:
	    c = (FrChar_t) FrByteSwap16(((uint16_t*)m_value)[N]) ;
	    break ;
	 case 4:
	    c = (FrChar_t) FrByteSwap32(((uint32_t*)m_value)[N]) ;
	    break ;
	 default:
	    bad_char_width("getNth") ;
	    return 0 ;
	 }
      return new FrString(c) ;
      }
}

//----------------------------------------------------------------------

bool FrString::setNth(size_t N, const FrObject *newchar)
{
   if (N >= stringLength() || !newchar || !newchar->stringp())
      return false ;
   else
      {
      FrChar_t c ;
      switch (((FrString*)newchar)->charWidth())
	 {
	 case 1:
	    c = ((unsigned char*)((FrString*)newchar)->m_value)[0] ;
	    break ;
	 case 2:
	    c = ((uint16_t*)((FrString*)newchar)->m_value)[0] ;
	    break ;
	 case 4:
	    c = ((uint32_t*)((FrString*)newchar)->m_value)[0] ;
	    break ;
	 default:
	    c = 0 ; // avoid uninitialized-variable warning
	    bad_char_width("setNth") ;
	    break ;
	 }
      switch (charWidth())
	 {
	 case 1:
	    m_value[N] = (unsigned char)c ;
	    break ;
	 case 2:
	    ((uint16_t*)m_value)[N] = (uint16_t)c ;
	    break ;
	 case 4:
	    ((uint32_t*)m_value)[N] = (uint32_t)c ;
	    break ;
	 default:
	    bad_char_width("setNth") ;
	    break ;
	 }
      return true ;
      }
}

//----------------------------------------------------------------------

void FrString::bad_char_width(const char *func)
{
   char buf[500] ;
   strncpy(buf,"bad character width in FrString::",sizeof(buf)) ;
   size_t len = strlen(buf) ;
   strncpy(buf+len,func,sizeof(buf)-len) ;
   FrWarning(buf) ;
   return ;
}

//----------------------------------------------------------------------

void FrString::unsupp_char_size(const char *where)
{
   char buf[500] ;
   size_t len = strlen(where) ;
   if (len > sizeof(buf) - 100)
      len = sizeof(buf) - 100 ;
   memcpy(buf,where,len) ;
   strncpy(buf+len," not yet supported for character sizes other than 8 bits",
	   sizeof(buf) - len) ;
   FrWarning(buf) ;
   return ;
}

//----------------------------------------------------------------------

inline FrString *combine_into_string(const FrList *items)
{
   FrString *string = new FrString(items,true) ;
   if (string && string->stringLength() > 0)
      return string ;
   else
      {
      free_object(string) ;
      return 0 ;
      }
}

//----------------------------------------------------------------------

size_t FrString::locate(const FrObject *item, size_t startpos) const
{
   if (item)
      {
      if (item->consp())
	 {
	 FrString *substr = combine_into_string((FrList*)item) ;
	 if (substr)
	    {
	    size_t sublen = substr->stringLength() ;
	    if (sublen > stringLength())
	       {
	       substr->freeObject() ;
	       return (size_t)-1 ;		// can't possibly match
	       }
	    size_t pos = (startpos != (size_t)-1) ? startpos+1 : 0 ;
	    size_t len = stringLength() - sublen ;
	    for ( ; pos <= len ; pos++)
	       {
	       if (FrStringCmp(m_value+pos*charWidth(),substr->stringValue(),
			       sublen,charWidth(),substr->charWidth()) == 0)
		  {
		  substr->freeObject() ;
		  return pos ;
		  }
	       }
	    substr->freeObject() ;
	    }
	 else
	    return (size_t)-1 ;		// can't do the matching
	 }
      else if (item->stringp())
	 {
	 FrString *substr = (FrString*)item ;
	 size_t sublen = substr->stringLength() ;
	 if (sublen > stringLength())
	    return (size_t)-1 ;		// can't possibly match
	 size_t pos = (startpos != (size_t)-1) ? startpos+1 : 0 ;
	 size_t len = stringLength() - sublen ;
	 for ( ; pos <= len ; pos++)
	    {
	    if (FrStringCmp(m_value+pos*charWidth(),substr->stringValue(),
			    sublen,charWidth(),substr->charWidth()) == 0)
	       return pos ;
	    }
	 }
      }
   return (size_t)-1 ;
}

//----------------------------------------------------------------------

size_t FrString::locate(const FrObject *item, FrCompareFunc cmp,
			 size_t startpos) const
{
   if (item)
      {
      if (item->consp())
	 {
	 FrString *substr = combine_into_string((FrList*)item) ;
	 if (substr)
	    {
	    size_t sublen = substr->stringLength() ;
	    if (sublen > stringLength())
	       {
	       substr->freeObject() ;
	       return (size_t)-1 ;		// can't possibly match
	       }
	    size_t pos = (startpos != (size_t)-1) ? startpos+1 : 0 ;
	    size_t len = stringLength() - sublen ;
	    for ( ; pos <= len ; pos++)
	       {
	       FrString candidate(m_value+pos*charWidth(),sublen,charWidth()) ;
	       if (cmp(&candidate,substr) == 0)
		  {
		  substr->freeObject() ;
		  return pos ;
		  }
	       }
	    substr->freeObject() ;
	    }
	 else
	    return (size_t)-1 ;		// can't do the matching
	 }
      else if (item->stringp())
	 {
	 FrString *substr = (FrString*)item ;
	 size_t sublen = substr->stringLength() ;
	 if (sublen > stringLength())
	    return (size_t)-1 ;		// can't possibly match
	 size_t pos = (startpos != (size_t)-1) ? startpos+1 : 0 ;
	 size_t len = stringLength() - sublen ;
	 for ( ; pos <= len ; pos++)
	    {
	    FrString candidate(m_value+pos*charWidth(),sublen,charWidth()) ;
	    if (cmp(&candidate,substr) == 0)
	       return pos ;
	    }
	 }
      }
   return (size_t)-1 ;
}


//----------------------------------------------------------------------

FrObject *FrString::insert(const FrObject *newelts, size_t pos, bool cp)
{
   (void)cp;				// (we always have to copy the chars)
   if (!newelts)
      return this ;			// no change if nothing to insert
   if (pos > stringLength())
      return this ;			// can't insert outside of string!
   // first, figure out how many characters we are inserting, and how big
   // they will be
   bool copied = false ;
   int newwidth ;
   size_t inslength ;
   if (newelts->stringp())
      {
      newwidth = ((FrString*)newelts)->charWidth() ;
      inslength = ((FrString*)newelts)->stringLength() ;
      }
   else if (newelts->symbolp())
      {
      newwidth = 1 ;
      inslength = strlen(((FrSymbol*)newelts)->symbolName()) ;
      }
   else if (newelts->consp())
      {
      newelts = combine_into_string((FrList*)newelts) ;
      if (newelts)
	 {
	 copied = true ;
	 newwidth = ((FrString*)newelts)->charWidth() ;
	 inslength = ((FrString*)newelts)->stringLength() ;
	 }
      else
	 return this ;			// can't handle it!
      }
   else
      return this ;			// can't handle that type!
   newwidth = FrMax(newwidth,charWidth()) ;
   // now expand the string to make room for the new characters
   size_t newsize = newwidth * (stringLength() + inslength + 1) ;
   if (newwidth == charWidth())
      {
      if (!realloc(newsize))
	 {
	 FrNoMemory(nomem_inserting) ;
	 if (copied)
	    ((FrString*)newelts)->freeObject() ;
	 return this ;
	 }
#if defined(__SUNOS__) || defined(__SOLARIS__)
      unsigned char *d = m_value + (pos+inslength)*newwidth ;
      unsigned char *s = m_value + pos*newwidth ;
      for (int len = inslength*newwidth ; len > 0 ; len--)
	 *(--d) = *(--s) ;
#else
      memmove(m_value+(pos+inslength)*newwidth,m_value+pos*newwidth,
	      inslength*newwidth) ;
#endif /* __SUNOS__ || __SOLARIS__ */
      }
   else
      {
      unsigned char *newvalue = reserve(newsize) ;
      if (!newvalue)
	 {
	 FrNoMemory(nomem_inserting) ;
	 if (copied)
	    ((FrString*)newelts)->freeObject() ;
	 return this ;
	 }
      int width = charWidth() ;
      FrStrCpy(newvalue,stringValue(),pos,newwidth,width) ;
      FrStrCpy(newvalue+(pos+inslength)*newwidth,stringValue()+(pos*width),
	       stringLength()-pos,newwidth,width) ;
      free() ;
      m_value = newvalue ;
      }
   setLength(stringLength() + inslength) ;
   setCharWidth(newwidth) ;
   // and finally, copy the new characters into the string
   if (newelts->stringp())
      FrStrCpy(m_value+pos*charWidth(),((FrString*)newelts)->stringValue(),
	       inslength,charWidth(),((FrString*)newelts)->charWidth()) ;
   else
      {
      // must be symbol, since we've thrown out or converted everything else
      FrStrCpy(m_value+pos*charWidth(),((FrSymbol*)newelts)->symbolName(),
	       inslength,charWidth(),1) ;
      }
   if (copied)
      ((FrString*)newelts)->freeObject() ;
   return this ;
}

//----------------------------------------------------------------------

FrObject *FrString::elide(size_t start, size_t end)
{
   size_t lngth = stringLength() ;
   if (end < start)
      end = start ;
   if (start >= lngth)
      return this ;			// nothing to be elided!
   int width = charWidth() ;
   if (end < lngth)
      {
      size_t len = (lngth-end)*width ;
      memcpy(m_value+start*width,m_value+(end+1)*width,len) ;
      size_t newlen = lngth - (end-start+1) ;
      if (realloc(newlen,width))
	 setLength(newlen) ;
      }
   else
      {
      if (realloc(start,width))
	 setLength(start) ;
      }
   return this ;
}

//----------------------------------------------------------------------

bool FrString::equal(const FrObject *obj) const
{
   if (this == obj)
      return true ;	   // equal if comparing to ourselves
   else if (obj && !obj->stringp())
      return false ;	   // not equal if other object is not a FrString
   else
      {
      FrString *s2 = (FrString*)obj ;
      if (stringLength() != s2->stringLength())
	 return false ;
      return FrStringCmp(m_value,s2->stringValue(),stringLength(),charWidth(),
			 s2->charWidth()) == 0 ;
      }
}

//----------------------------------------------------------------------

int FrString::compare(const FrObject *obj) const
{
   if (!obj)
      return 1 ;     // anything is greater than NIL / empty-list
   if (!obj->stringp())
      return -1 ;    // sort all non-strings after strings
   else
      return FrStringCmp(m_value,((FrString*)obj)->stringValue(),
			 stringLength()+1,charWidth(),
			 ((FrString*)obj)->charWidth()) ;
}

//----------------------------------------------------------------------

bool FrString::iterateVA(FrIteratorFunc func, va_list args) const
{
   size_t len = stringLength() ;
   int width = charWidth() ;
   for (size_t pos = 0 ; pos < len ; pos++)
      {
      FrString character(m_value+width*pos,1,width) ;
      FrSafeVAList(args) ;
      bool success = func(&character,FrSafeVarArgs(args)) ;
      FrSafeVAListEnd(args) ;
      if (!success)
	 return false ;
      }
   return true ;
}

//----------------------------------------------------------------------

FrObject *FrString::copy() const
{
   return new FrString(stringValue(),stringLength(),charWidth()) ;
}

//----------------------------------------------------------------------

FrObject *FrString::deepcopy() const
{
   return new FrString(stringValue(),stringLength(),charWidth()) ;
}

//----------------------------------------------------------------------

ostream &FrString::printValue(ostream &output) const
{
   register int c ;
   char terminator ;
   int width = charWidth() ;
   int size = stringLength() * width ;

   switch (width)
      {
      case 1: terminator = '"' ; break ;
      case 2: terminator = '\'' ; break ;
      case 4: terminator = '`' ; break ;
      default:
	  bad_char_width("printValue") ;
	  terminator = '"' ;
	  break ;
      }
   output << terminator ;
   for (int i = 0 ; i < size ; i++)
      {
      c = m_value[i] ;
      if (c == terminator || c == '\\')
	 output << '\\' << (unsigned char)c ;
      else if (c == '\0')
         output << "\\0" ;
      else
         output << (unsigned char) c ;
      }
   output << terminator ;
   return output ;
}

//----------------------------------------------------------------------

size_t FrString::displayLength() const
{
   int width = charWidth() ;
   int size = stringLength() * width ;
   int len = size + 2 ;  // chars in string plus surrounding quotes
   int terminator ;

   switch (width)
      {
      case 1: terminator = '"' ; break ;
      case 2: terminator = '\'' ; break ;
      case 4: terminator = '`' ; break ;
      default:
	  bad_char_width("displayLength") ;
	  terminator = '"' ;
	  break ;
      }
   for (int i = 0 ; i < size ; i++)
      {
      if (m_value[i] == terminator || m_value[i] == '\\' || m_value[i] == '\0')
         len++ ;	// character needs to be quoted
      }
   return len ;
}

//----------------------------------------------------------------------

char *FrString::displayValue(char *buffer) const
{
   register int c ;
   char terminator ;
   int width = charWidth() ;
   int size = stringLength() * width ;

   switch (width)
      {
      case 1: terminator = '"' ; break ;
      case 2: terminator = '\'' ; break ;
      case 4: terminator = '`' ; break ;
      default:
	  bad_char_width("displayValue") ;
	  terminator = '"' ;
	  break ;
      }
   *buffer++ = terminator ;
   for (int i = 0 ; i < size ; i++)
      {
      c = m_value[i] ;
      if (c == terminator || c == '\\')
	 {
         *buffer++ = '\\' ;
	 *buffer++ = (char)c ;
         }
      else if (c == '\0')
	 {
         *buffer++ = '\\' ;
	 *buffer++ = '0' ;
	 }
      else
         *buffer++ = (char)c ;
      }
   *buffer++ = terminator ;
   *buffer = '\0' ;
   return buffer ;
}

//----------------------------------------------------------------------

static size_t next_space(const unsigned char *value, size_t column)
{
   while (value[1] && !Fr_isspace(value[1]))
      {
      value++ ;
      column++ ;
      }
   return column ;
}

//----------------------------------------------------------------------

void FrString::wordWrap(size_t width)
{
   if (width > 0)
      {
      if (charWidth() != 1)
	 {
	 unsupp_char_size(str_wordWrap) ;
	 return ;
	 }
      size_t column = 0 ;
      size_t len = stringLength() ;
      for (size_t i = 0 ; i < len ; i++)
	 {
	 if (m_value[i] == '\n' ||
	     (Fr_isspace(m_value[i]) && column > 0 &&
	      next_space(m_value+i,column) >= width)
	    )
	    {
	    m_value[i] = '\n' ;
	    column = 0 ;
	    }
	 else if (m_value[i] == '\t')
	    column = (column + 7) & ~7 ;
	 else
	    column++ ;
	 }
      }
   return ;
}

//----------------------------------------------------------------------

FrString *FrString::operator = (const FrString &s)
{
   size_t len = s.stringLength() ;
   int width = s.charWidth() ;
   if (realloc(len,width))
      {
      memcpy(m_value,s.stringValue(),(len+1)*width) ;
      setLength(len) ;
      setCharWidth(width) ;
      }
   else
      FrNoMemory(nomem_op_eq) ;
   return this ;
}

//----------------------------------------------------------------------

FrString *FrString::operator = (const FrString *s)
{
   size_t len = s->stringLength() ;
   size_t width = s->charWidth() ;
   if (realloc(len,width))
      {
      memcpy(m_value,s->stringValue(),(len+1)*width) ;
      setLength(len) ;
      setCharWidth(width) ;
      }
   else
      FrNoMemory(nomem_op_eq) ;
   return this ;
}

//----------------------------------------------------------------------

FrString *FrString::operator = (const FrChar_t c)
{
   int width ;
   if (c <= 0x000000FF)
      width = 1 ;
   else if (c <= 0x0000FFFF)
      width = 2 ;
   else
      width = 4 ;
   if (realloc(1,width))
      {
      setCharWidth(width) ;
      setLength(1) ;
      if (width == 1)
	 {
	 m_value[0] = (unsigned char)c ;
	 m_value[1] = '\0' ;
	 }
      else if (width == 2)
	 {
	 *((uint16_t*)m_value) = FrByteSwap16((uint16_t)c) ;
	 ((uint16_t*)m_value)[1] = 0 ;
	 }
      else
	 {
	 *((uint32_t*)m_value) = FrByteSwap32((uint32_t)c) ;
	 ((uint32_t*)m_value)[1] = 0 ;
	 }
      }
   else
      {
      FrNoMemory(nomem_op_eq) ;
      setLength(0) ;
      }
   return this ;
}

//----------------------------------------------------------------------

FrString *FrString::operator = (const char *s)
{
   size_t len = strlen(s) ;
   if (realloc(len,1))
      {
      memcpy(m_value,s,len+1) ;
      setLength(len) ;
      setCharWidth(1) ;
      }
   else
      FrNoMemory(nomem_op_eq) ;
   return (FrString*)this ;
}

//----------------------------------------------------------------------

void FrString::setChar(size_t index,FrChar_t newch)
{
   if (index < stringLength())
      {
      switch (charWidth())
	 {
	 case 1:
	    ((unsigned char*)m_value)[index] = (unsigned char)newch ;
	    break ;
	 case 2:
	    ((uint16_t*)m_value)[index] = FrByteSwap16((uint16_t)newch) ;
	    break ;
	 case 4:
	    ((uint32_t*)m_value)[index] = FrByteSwap32(newch) ;
	    break ;
	 default:
	    bad_char_width("setChar") ;
	 }
      }
   else
      FrProgError("index out of range in FrString::setChar") ;
   return ;
}

//----------------------------------------------------------------------

FrChar_t FrString::operator [] (size_t index) const
{
   if (index < stringLength())
      {
      switch (charWidth())
	 {
	 case 1:
	    return (FrChar_t) (unsigned char)m_value[index] ;
	 case 2:
	    return (FrChar_t) FrByteSwap16(((uint16_t*)m_value)[index]) ;
	 case 4:
	    return (FrChar_t) FrByteSwap32(((uint32_t*)m_value)[index]) ;
	 default:
	    bad_char_width("[]") ;
	    return (FrChar_t)-1 ;
	 }
      }
   else
      return (FrChar_t)-1 ;
}

//----------------------------------------------------------------------

FrChar_t FrString::nthChar(size_t index) const
{
   if (index < stringLength())
      {
      switch (charWidth())
	 {
	 case 1:
	    return (FrChar_t) (unsigned char)m_value[index] ;
	 case 2:
	    return (FrChar_t) FrByteSwap16(((uint16_t*)m_value)[index]) ;
	 case 4:
	    return (FrChar_t) FrByteSwap32(((uint32_t*)m_value)[index]) ;
	 default:
	    bad_char_width("nthChar") ;
	    return (FrChar_t)-1 ;
	 }
      }
   else
      return (FrChar_t)-1 ;
}

//----------------------------------------------------------------------

static bool dump_unfreed(void *obj, va_list args)
{
   FrString *string = (FrString*)obj ;
   FrVarArg(ostream *,out) ;
   if (string && out)
      (*out) << string << endl ;
   return true ;			// continue iterating
}

void FrString::dumpUnfreed(ostream &out)
{
   allocator.iterate(dump_unfreed,&out) ;
   return ;
}

/**********************************************************************/
/*    Non-member functions related to class FrString		      */
/**********************************************************************/

int FrStringCmp(const void *s1, const void *s2, int length, int width1,
		int width2)
{
   if (s1 == 0)
      return s2 ? +1 : 0 ;
   else if (s2 == 0)
      return -1 ;
#ifndef PURIFY
#if defined(FrBIGENDIAN)
   if (width1 == width2)
      return memcmp(s1,s2,width1*length) ;
#else
   if (width1 == 1 && width2 == 1)
      return memcmp(s1,s2,length) ;
#endif /* FrBIGENDIAN */
   else
#endif /* !PURIFY */
      for (int i = 0 ; i < length ; i++)
         {
	 int32_t c1, c2 ;
         switch (width1)
            {
	    case 1:   c1 = ((unsigned char *)s1)[i] ; break ;
	    case 2:   c1 = FrByteSwap16(((uint16_t *)s1)[i]) ; break ;
            case 4:   c1 = FrByteSwap32(((uint32_t *)s1)[i]) ; break ;
            default:  c1 = 0 ; break ;
            }
         switch (width2)
            {
            case 1:   c2 = ((unsigned char *)s2)[i] ; break ;
            case 2:   c2 = FrByteSwap16(((uint16_t *)s2)[i]) ; break ;
            case 4:   c2 = FrByteSwap32(((uint32_t *)s2)[i]) ; break ;
            default:  c2 = 0 ; break ;
            }
         if (c1 != c2)
            return (c1 > c2) ? 1 : -1 ;
         }
   return 0 ;	// the two strings are equal
}

//----------------------------------------------------------------------

void *FrStrCpy(void *dest, const void *src, int length, int w1, int w2)
{
   register uint32_t c ;

   if (w1 == w2)
      {
      size_t len = length * w1 ;
      memcpy(dest,src,len) ;
      return (char*)dest + len ;
      }
   unsigned char *destchar = (unsigned char*)dest ;
   for (int i = 0 ; i < length ; i++)
      {
      switch (w2)
         {
	 case 1: c = ((unsigned char *)src)[i] ; break ;
	 case 2: c = FrLoadShort(((uint16_t *)src)+i) ; break ;
	 case 4: c = FrLoadLong(((uint32_t *)src)+i) ; break ;
	 default: c = 0 ; break ;  // assign value to avoid "uninit var" warn
	 }
      switch (w1)
	 {
	 case 1: *destchar = (unsigned char)c ; break ;
	 case 2: FrStoreShort(c,destchar) ; break ;
	 case 4: FrStoreLong(c,destchar) ; break ;
	 default: break ;
	 }
      destchar += w1 ;
      }
   return destchar ;
}

// end of file frstring.cpp //
