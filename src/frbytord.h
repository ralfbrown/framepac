/************************************************************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frbytord.h	Byte-order independence functions		*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,1998,2001,2002,2004,2006,2007,	*/
/*		2008,2010,2015 Ralf Brown/Carnegie Mellon University	*/
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

#ifndef __FRBYTORD_H_INCLUDED
#define __FRBYTORD_H_INCLUDED

#ifndef __FRCOMMON_H_INCLUDED
#include "frcommon.h"
#endif

#ifndef __FRAMERR_H_INCLUDED
#include "framerr.h"
#endif

/**********************************************************************/
/**********************************************************************/

//----------------------------------------------------------------------

#if defined(__WATCOMC__) && defined(__386__)
extern short FrByteSwap16(short int value) ;
#  pragma aux FrByteSwap16 = \
	"xchg al,ah" \
        parm [ax] \
        value [ax] \
   	modify exact [ax] ;

#elif defined(__GNUC__) && defined(__886x__)
//!!! need to figure out proper inline assembler for AMD64 instruction set
inline short int FrByteSwap16(short int value)
{
   return ((value & 0xFF00) >> 8) | ((value & 0x00FF) << 8) ;
}
#elif defined(__GNUC__) && defined(__386__)
inline short FrByteSwap16(short int value)
{
   short result ;
   __asm__ ("xchg %h1,%b1" : "=a" (result) : "a" (value)) ;
   return result ;
}

#elif defined(_MSC_VER) && _MSC_VER >= 800
inline short FrByteSwap16(short int value)
{
   short result ;
   _asm {
          mov ax,value ;
	  xchg al,ah ;
	  mov result,ax ;
        } ;
   return result ;
}

#else // neither Watcom C++ nor Visual C++ nor GCC on x86

#ifdef FrLITTLEENDIAN
inline short int FrByteSwap16(short int value)
{
   return ((value & 0xFF00) >> 8) | ((value & 0x00FF) << 8) ;
}
#else
#define FrByteSwap16(x) (x)
#endif /* FrLITTLEENDIAN */

#endif /* __WATCOMC__ && __386__ */

//----------------------------------------------------------------------

inline void FrStoreByte(int value, void *location)
{
#if CHAR_BIT == 8
  ((unsigned char *)location)[0] = (unsigned char)value ;
#else
  ((unsigned char *)location)[0] = (unsigned char)(value & 0xFF) ;
#endif /* CHAR_BIT == 8 */
}

//----------------------------------------------------------------------

#if defined(__WATCOMC__) && defined(__386__)
extern void FrStoreShort(int value, void *location) ;
#  pragma aux FrStoreShort = \
	"xchg al,ah" \
	"mov word ptr [ebx],ax" \
        parm [eax][ebx] \
        modify exact [ax] ;

#elif defined(__GNUC__) && defined(__886x__)
//!!!
inline void FrStoreShort(int value, void *buffer)
{
   unsigned char *buf = (unsigned char *)buffer ;
   buf[0] = (char)(value >> 8) ;
   buf[1] = (char)value ;
}
#elif defined(__GNUC__) && defined(__386__)
#define FrStoreShort(value, location) \
   { uint16_t *_zz_loc = (uint16_t*)(location) ;  \
     __asm__ ("xchg %%ah,%%al\n\t"  \
	      "movw %%ax,%0" : "=g" (*_zz_loc) : "a" (value)) ; }

#elif defined(_MSC_VER) && _M_IX86 >= 300
inline void FrStoreShort(int value, void *buffer)
{
  _asm {
         mov ax,word ptr value ;
	 mov ebx,dword ptr buffer ;
	 xchg al,ah ;
	 mov word ptr [ebx],ax ;
       } ;
}

#else // neither Watcom C++ nor Visual C++ nor GCC on x86
inline void FrStoreShort(int value, void *buffer)
{
   unsigned char *buf = (unsigned char *)buffer ;
#if CHAR_BIT == 8
   buf[0] = (char)(value >> 8) ;
   buf[1] = (char)value ;
#else
   buf[0] = (char)((value >> 8)  & 0xFF) ;
   buf[1] = (char)(value & 0xFF) ;
#endif /* CHAR_BIT == 8 */
}
#endif /* __WATCOMC__ && __386__ */

//----------------------------------------------------------------------

inline void FrStoreAlignedShort(short int value, void *buffer)
{
   *((uint16_t*)buffer) = (uint16_t)FrByteSwap16(value)  ;
}

//----------------------------------------------------------------------

#if defined(__WATCOMC__) && defined(__386__)
extern void FrStoreThreebyte(uint32_t value, void *location) ;
#  pragma aux FrStoreThreebyte = \
	"mov byte ptr [ebx+2],al" \
	"shr eax,8" \
	"xchg al,ah" \
	"mov word ptr [ebx],ax" \
        parm [eax][ebx] \
        modify [eax] ;

#elif defined(_MSC_VER) && _M_IX86 >= 300

inline void FrStoreThreebyte(uint32_t value, void *location)
{
   _asm {
          mov ebx,location ;
	  mov eax,value ;
          mov byte ptr [ebx+2],al ;
	  shr eax,8 ;
	  xchg al,ah ;
	  mov word ptr [ebx],ax ;
        } ;
}

#else // neither Watcom C++ nor Visual C++

inline void FrStoreThreebyte(uint32_t value, void *buffer)
{
   unsigned char *buf = (unsigned char *)buffer ;
#if CHAR_BIT == 8
   buf[0] = (char)(value >> 16) ;
   buf[1] = (char)(value >> 8) ;
   buf[2] = (char)value ;
#else
   buf[0] = (char)((value >> 16) & 0xFF) ;
   buf[1] = (char)((value >> 8)  & 0xFF) ;
   buf[2] = (char)(value & 0xFF) ;
#endif /* CHAR_BIT == 8 */
}
#endif /* __WATCOMC__ && __386__ */

//----------------------------------------------------------------------

#if defined(__WATCOMC__) && defined(__386__)
extern uint32_t FrByteSwap32(uint32_t value) ;
#if defined(__SW_4) || defined(__SW_5)
#  pragma aux FrByteSwap32 = \
        "bswap eax" \
        parm [eax] \
        value [eax] \
        modify exact [eax] ;
#else
#  pragma aux FrByteSwap32 = \
	"xchg al,ah" \
	"rol  eax,16" \
	"xchg al,ah" \
        parm [eax] \
        value [eax] \
   	modify exact [eax] ;
#endif /* __SW_4 || __SW_5 */

#elif defined(__GNUC__) && defined(__586__)
inline uint32_t FrByteSwap32(uint32_t value)
{
   uint32_t result ;
   __asm__("bswap %0" : "=q" (result) : "0" (value)) ;
   return result ;
}

#elif defined(__GNUC__) && defined(__386__)
inline uint32_t FrByteSwap32(uint32_t value)
{
   uint32_t result ;
   __asm__("xchg %h0,%b0\n\t"
	   "rol $16,%0\n\t"
	   "xchg %h0,%b0" : "=q" (result) : "0" (value)) ;
   return result ;
}

#elif defined(_MSC_VER) && _MSC_VER >= 800

#if _M_IX86 >= 400
inline uint32_t FrByteSwap32(uint32_t value)
{
   uint32_t result ;
   _asm {
          mov eax,value ;
	  bswap eax ;
	  mov result,eax ;
        } ;
   return result ;
}
#else // 386 code generation
inline uint32_t FrByteSwap32(uint32_t value)
{
   uint32_t result ;
   _asm {
          mov eax,value ;
	  xchg al,ah ;
	  rol eax,16 ;
	  xchg al,ah ;
	  mov result,eax ;
        } ;
   return result ;
}
#endif /* _M_IX86 */

#else // neither Watcom C++ nor Visual C++

#ifdef FrLITTLEENDIAN
inline uint32_t FrByteSwap32(uint32_t value)
{
   return ((value & 0xFF000000) >> 24) | ((value & 0x00FF0000) >> 8) |
	  ((value & 0x0000FF00) << 8)  | ((value & 0x000000FF) << 24) ;
}
#else
#define FrByteSwap32(x) (x)
#endif /* FrLITTLEENDIAN */

#endif /* __WATCOMC__ && __386__ */

//----------------------------------------------------------------------

#if defined(__WATCOMC__) && defined(__386__)
extern void FrStoreLong(uint32_t value, void *location) ;
#if defined(__SW_4) || defined(__SW_5)
#  pragma aux FrStoreLong = \
        "bswap eax" \
	"mov dword ptr [ebx],eax" \
        parm [eax][ebx] \
        modify exact [eax] ;
#else
#  pragma aux FrStoreLong = \
	"xchg al,ah" \
	"rol eax,16" \
	"xchg al,ah" \
	"mov dword ptr [ebx],eax" \
        parm [eax][ebx] \
        modify exact [eax] ;
#endif /* __SW_4 || __SW_5 (486/Pentium/+ code) */

#elif defined(__GNUC__) && defined(__586__)
#define FrStoreLong(value, location) \
   { uint32_t *_zz_loc = (uint32_t*)(location) ;  \
     __asm__ ("bswap %0" : "=q" (*_zz_loc) : "0" (value) ) ; }
#elif defined(__GNUC__) && defined(__386__)
#define FrStoreLong(value, location) \
   { uint32_t *_zz_loc = (uint32_t*)(location) ;		\
     __asm__ ("xchg %h0,%b0\n\t"				\
	      "rol $16,%0\n\t"					\
	      "xchg %h0,%b0" : "=q" (*_zz_loc) : "0" (value) )

#elif defined(_MSC_VER)

#if _M_IX86 >= 400
inline void FrStoreLong(int value, void *buffer)
{
  _asm {
         mov eax,value ;
	 mov ebx,buffer ;
	 bswap eax ;
	 mov dword ptr [ebx],eax ;
       } ;
}
#else // 386 code generation
inline void FrStoreLong(int value, void *buffer)
{
  _asm {
         mov eax,value ;
	 mov ebx,buffer ;
	 xchg al,ah ;
	 rol eax,16 ;
	 xchg al,ah ;
	 mov dword ptr [ebx],eax ;
       } ;
}
#endif /* _M_IX86 */

#else // neither Watcom C++ nor Visual C++
inline void FrStoreLong(uint32_t value, void *buffer)
{
   unsigned char *buf = (unsigned char *)buffer ;
#if CHAR_BIT == 8
   buf[0] = (char)(value >> 24) ;
   buf[1] = (char)(value >> 16) ;
   buf[2] = (char)(value >> 8) ;
   buf[3] = (char)value ;
#else
   buf[0] = (char)((value >> 24) & 0xFF) ;
   buf[1] = (char)((value >> 16) & 0xFF) ;
   buf[2] = (char)((value >> 8)  & 0xFF) ;
   buf[3] = (char)(value & 0xFF) ;
#endif /* CHAR_BIT == 8 */
}
#endif /* __WATCOMC__ && __386__ */

//----------------------------------------------------------------------

#if defined(__WATCOMC__) || (defined(_MSC_VER) && _M_IX86 >= 0x300)
inline void FrStoreLongLE(int value, void *buffer)
{
   *((uint32_t*)buffer) = value ;
}

#else // neither Watcom C++ nor Visual C++/x86
inline void FrStoreLongLE(uint32_t value, void *buffer)
{
   unsigned char *buf = (unsigned char *)buffer ;
#if CHAR_BIT == 8
   buf[3] = (char)(value >> 24) ;
   buf[2] = (char)(value >> 16) ;
   buf[1] = (char)(value >> 8) ;
   buf[0] = (char)value ;
#else
   buf[3] = (char)((value >> 24) & 0xFF) ;
   buf[2] = (char)((value >> 16) & 0xFF) ;
   buf[1] = (char)((value >> 8)  & 0xFF) ;
   buf[0] = (char)(value & 0xFF) ;
#endif /* CHAR_BIT == 8 */
}
#endif /* __WATCOMC__ && __386__ */

//----------------------------------------------------------------------

inline void FrStoreAlignedLong(uint32_t value, void *buffer)
{
   *((uint32_t*)buffer) = FrByteSwap32(value) ;
   return ;
}

//----------------------------------------------------------------------

inline void FrStore40(uint64_t value, void *buffer)
{
   FrStoreByte((int)(value >> 32),buffer) ;
   FrStoreLong((int)(value & 0xFFFFFFFF),(char*)buffer+sizeof(FrBYTE)) ;
   return ;
}

//----------------------------------------------------------------------

inline void FrStore48(uint64_t value, void *buffer)
{
   FrStoreShort((int)(value >> 32),buffer) ;
   FrStoreLong((uint32_t)(value & 0xFFFFFFFF),(char*)buffer+sizeof(uint16_t)) ;
   return ;
}

//----------------------------------------------------------------------

#if defined(__GNUC__) && defined(__886__) && __BITS__ == 64
inline void FrStore64(uint64_t value, void *buffer)
{
   uint64_t *_zz_buf = (uint64_t*)(buffer) ;
   __asm__ ("bswapq %0" : "=q" (*_zz_buf) : "0" (value) ) ;
}
#else
inline void FrStore64(uint64_t value, void *buffer)
{
   FrStoreLong((uint32_t)(value >> 32),buffer) ;
   FrStoreLong((uint32_t)(value & 0xFFFFFFFF),
	       ((char*)buffer)+sizeof(uint32_t)) ;
   return ;
}
#endif

//----------------------------------------------------------------------

inline void FrStoreFloat(float value, void *buffer)
{
#if defined(__WATCOMC__) || defined(_MSC_VER) || defined(__SUNOS__) || defined(__SOLARIS__) || defined(__linux__) || defined(__CYGWIN__) || defined(__alpha__)
   float val = value ;
   float *valptr = &val ;
   FrStoreLong(*((uint32_t*)valptr),buffer) ;
#else
   // do any necessary byte swapping and conversions to IEEE here //
   FrInvalidFunction("FrStoreFloat") ;
#endif /* Watcom||MSC||Linux||SunOS||Solaris||Alpha, other */
}

//----------------------------------------------------------------------

inline void FrStoreDouble(double value, void *buffer)
{
#if defined(__WATCOMC__) || defined(_MSC_VER) || defined(__SUNOS__) || defined(__SOLARIS__) || defined(__linux__) || defined(__CYGWIN__) || defined(__alpha__)
   FrStore64((uint64_t)value,buffer) ;
#else
   // do any necessary byte swapping and conversions to IEEE here //
   FrInvalidFunction("FrStoreDouble") ;
#endif /* Watcom||MSC||Linux||SunOS||Solaris||Alpha, other */
}

//----------------------------------------------------------------------

inline int FrLoadByte(const void *location)
{
#if CHAR_BIT == 8
  return (((unsigned char *)location)[0]) ;
#else
  return (((unsigned char *)location)[0] & 0xFF) ;
#endif /* CHAR_BIT == 8 */
}

//----------------------------------------------------------------------

#if defined(__WATCOMC__) && defined(__386__)
extern short int FrLoadShort(const void *location) ;
#  pragma aux FrLoadShort = \
	/*"xor eax,eax"*/ \
	"mov ax,word ptr [ebx]" \
	"xchg al,ah" \
        parm [ebx] \
        value [ax] \
   	modify exact [ax] ;

#elif defined(__GNUC__) && defined(__886__)
inline uint16_t FrLoadShort(const void *location)
{
   uint16_t result ;
   //   __asm__("movzwl (%1),%%eax\n\t"
   //	   "xchg %%ah,%%al" : "=a" (result) : "r" (location)) ;
   __asm__("xchg %%ah,%%al" : "=a" (result) : "0" (*(uint16_t*)(location))) ;
   return result ;
}
#elif defined(__GNUC__) && defined(__386__)
inline uint16_t FrLoadShort(const void *location)
{
   uint16_t result ;
   __asm__("mov %1,%0\n\t"
	   "xchg %h0,%b0" : "=q" (result) : "g" (*(uint16_t*)(location))) ;
   return result ;
}

#elif defined(_MSC_VER) && _MSC_VER >= 800

inline short int FrLoadShort(const void *location)
{
  short int value ;
  _asm {
	 mov ebx,location ;
	 mov ax,word ptr [ebx] ;
	 xchg al,ah ;
         mov value,ax ;
       } ;
  return value ;
}

#else // neither Watcom C++ nor Visual C++ nor x86 GCC

inline uint16_t FrLoadShort(const void *buf)
{
#define b ((unsigned char *)buf)
#if CHAR_BIT == 8
   return (uint16_t)((b[0]<<8)|b[1]) ;
#else
   return (uint16_t)(((b[0]&0xFF)<<8)|(b[1]&0xFF)) ;
#endif /* CHAR_BIT == 8 */
#undef b
}
#endif /* __WATCOMC__ && __386__ */

//----------------------------------------------------------------------

inline int FrLoadAlignedShort(void *buffer)
{
   return FrByteSwap16(*(uint16_t*)buffer) ;
}

//----------------------------------------------------------------------

#if defined(__WATCOMC__) && defined(__386__)
extern uint32_t FrLoadThreebyte(const void *location) ;
#if defined(__SW_4) || defined(__SW_5)
#  pragma aux FrLoadThreebyte = \
	"mov eax,dword ptr [ebx]" \
        "bswap eax" \
	"shr eax,8" \
        parm [ebx] \
        value [eax] ;
#else
#  pragma aux FrLoadThreebyte = \
	"mov eax,dword ptr [ebx]" \
	"xchg al,ah" \
	"rol eax,16" \
	"xchg al,ah" \
	"shr eax,8" \
        parm [ebx] \
        value [eax] ;
#endif /* __SW_4 || __SW_5 (486/Pentium/+ code) */

#elif defined(__GNUC__) && defined(__586__)
inline uint32_t FrLoadThreebyte(const void *location)
{
   uint32_t result ;
   __asm__("movl %1,%0\n\t"
	   "bswapl %0\n\t"
	   "shrl $8,%0" : "=q" (result) : "g" (*(uint32_t*)(location))) ;
   return result ;
}

#elif defined(__GNUC__) && defined(__386__)
inline uint32_t FrLoadThreebyte(const void *location)
{
   uint32_t result ;
   __asm__("mov %1,%0\n\t"
	   "xchg %h0,%b0\n\t"
	   "rol $16,%0\n\t"
	   "xchg %h0,%b0\n\t"
	   "shr $8,%0" : "=q" (result) : "g" (*(uint32_t*)(location))) ;
   return result ;
}

#elif defined(_MSC_VER) && _M_IX86 >= 300

inline uint32_t FrLoadThreebyte(const void *location)
{
   uint32_t result ;
   _asm {
          mov ebx,location ;
	  mov eax,dword ptr [ebx] ;
	  xchg al,ah ;
	  rol eax,16 ;
	  xchg al,ah ;
	  shr eax,8 ;
	  mov result,eax ;
        } ;
   return result ;
}

#else // neither Watcom C++ nor Visual C++

inline uint32_t FrLoadThreebyte(const void *buf)
{
#define b ((unsigned char *)buf)
#if CHAR_BIT == 8
   return (((uint32_t)b[0])<<16) | (((uint32_t)b[1])<<8) | ((uint32_t)b[2]) ;
#else
   return (((uint32_t)b[0]&0xFF)<<16) | (((uint32_t)b[1]&0xFF)<<8) |
	   ((uint32_t)b[2]&0xFF) ;
#endif /* CHAR_BIT == 8 */
#undef b
}
#endif /* __WATCOMC__ && __386__ */

//----------------------------------------------------------------------

#if defined(__WATCOMC__) && defined(__386__)
extern uint32_t FrLoadLong(const void *location) ;
#if defined(__SW_4) || defined(__SW_5)
#  pragma aux FrLoadLong = \
	"mov   eax,dword ptr [ebx]" \
        "bswap eax" \
        parm [ebx] \
        value [eax] \
        modify exact [eax] ;
#else
#  pragma aux FrLoadLong = \
	"mov  eax,dword ptr [ebx]" \
	"xchg al,ah" \
	"rol  eax,16" \
	"xchg al,ah" \
        parm [ebx] \
        value [eax] \
   	modify exact [eax] ;
#endif /* __SW_4 || __SW_5 */

#elif defined(__GNUC__) && defined(__586__)
inline uint32_t FrLoadLong(const void *location)
{
   uint32_t result ;
   __asm__("bswapl %0" : "=q" (result) : "0" (*(uint32_t*)(location))) ;
   return result ;
}
#elif defined(__GNUC__) && defined(__386__)
inline uint32_t FrLoadLong(const void *location)
{
   uint32_t result ;
   __asm__("mov %1,%0\n\t"
	   "xchg %h0,%b0\n\t"
	   "rol $16,%0\n\t"
	   "xchg %h0,%b0" : "=q" (result) : "g" (*(uint32_t*)(location))) ;
   return result ;
}

#elif defined(_MSC_VER) && _MSC_VER >= 800
#if _M_IX86 >= 400
inline uint32_t FrLoadLong(const void *location)
{
  uint32_t value ;
  _asm {
	 mov ebx,location ;
	 mov eax,dword ptr [ebx] ;
	 bswap eax ;
         mov value,eax ;
       } ;
  return value ;
}
#else // 386 code generation
inline uint32_t FrLoadLong(const void *location)
{
  uint32_t value ;
  _asm {
	 mov ebx,location ;
	 mov eax,dword ptr [ebx] ;
	 xchg al,ah ;
	 rol eax,16 ;
	 xchg al,ah ;
         mov value,eax ;
       } ;
  return value ;
}
#endif /* _M_IX86 */

#else // neither Watcom C++ nor Visual C++

inline uint32_t FrLoadLong(const void *buf)
{
#define b ((unsigned char *)buf)
#if CHAR_BIT == 8
   return (((uint32_t)b[0])<<24)|(((uint32_t)b[1])<<16)|
	  (((uint32_t)b[2])<<8)|((uint32_t)b[3]) ;
#else
   return (((uint32_t)b[0]&0xFF)<<24)|(((uint32_t)b[1]&0xFF)<<16)|
	  (((uint32_t)b[2]&0xFF)<<8)|((uint32_t)b[3]&0xFF) ;
#endif /* CHAR_BIT == 8 */
#undef b
}
#endif /* __WATCOMC__ && __386__ */

//----------------------------------------------------------------------

#if defined(__WATCOMC__) || (defined(_MSC_VER) && _M_IX86 >= 300)
inline uint32_t FrLoadLongLE(const void *location)
{
   return *((uint32_t*)location) ;
}

#else // neither Watcom C++ nor Visual C++/x86

inline uint32_t FrLoadLongLE(const void *buf)
{
#define b ((unsigned char *)buf)
#if CHAR_BIT == 8
   return (((uint32_t)b[3])<<24)|(((uint32_t)b[2])<<16)|
	 (((uint32_t)b[1])<<8)|((uint32_t)b[0]) ;
#else
   return (((uint32_t)b[3]&0xFF)<<24)|(((uint32_t)b[2]&0xFF)<<16)|
	 (((uint32_t)b[1]&0xFF)<<8)|((uint32_t)b[0]&0xFF) ;
#endif /* CHAR_BIT == 8 */
#undef b
}
#endif /* __WATCOMC__ && __386__ */

//----------------------------------------------------------------------

inline uint32_t FrLoadAlignedLong(void *buffer)
{
   return FrByteSwap32(*(uint32_t*)buffer) ;
}

//----------------------------------------------------------------------

inline uint64_t FrLoad40(void *buffer)
{
   uint32_t low = FrLoadLong((char*)buffer+sizeof(FrBYTE)) ;
   return (((uint64_t)FrLoadByte(buffer)) << 32) + low ;
}

//----------------------------------------------------------------------

inline uint64_t FrLoad48(void *buffer)
{
   uint32_t low = FrLoadLong((char*)buffer+sizeof(uint16_t)) ;
   return (((uint64_t)FrLoadShort(buffer)) << 32) + low ;
}

//----------------------------------------------------------------------

inline uint64_t FrLoad64(const void *buffer)
{
#if defined(__GNUC__) && defined(__886__) && __BITS__ == 64
   uint64_t result ;
   __asm__("bswapq %0" : "=q" (result) : "0" (*(uint64_t*)(buffer))) ;
   return result ;
#else
   // 32-bit architectures
   uint32_t low = FrLoadLong((char*)buffer+sizeof(uint32_t)) ;
   return (((uint64_t)FrLoadLong(buffer)) << 32) + low ;
#endif
}

//----------------------------------------------------------------------

inline float FrLoadFloat(void *buffer)
{
#if defined(__WATCOMC__) || defined(_MSC_VER) || defined(__SUNOS__) || defined(__SOLARIS__) || defined(__linux__) || defined(__CYGWIN__) || defined(__alpha__)
   union {
      uint32_t value_i ;
      float value_f ;
      } u ;
   u.value_i = FrLoadLong(buffer) ;
   return u.value_f ;
#else
   // need to do any necessary byte swapping and conversions here //
   FrInvalidFunction("FrLoadFloat") ;
#endif /* Watcom||MSC||Linux||SunOS||Solaris||Alpha, other */
}

//----------------------------------------------------------------------

inline double FrLoadDouble(void *buffer)
{
#if defined(__WATCOMC__) || defined(_MSC_VER) || defined(__SUNOS__) || defined(__SOLARIS__) || defined(__linux__) || defined(__CYGWIN__) || defined(__alpha__)
   union {
      uint64_t result_i ;
      double result_d ;
   } u ;
   u.result_i = FrLoad64(buffer) ;
   return u.result_d ;
#else
   // do any necessary byte swapping and conversions here //
   FrInvalidFunction("FrLoadDouble") ;
#endif /* Watcom||MSC||Linux||SunOS||Solaris||Alpha, other */
}

//----------------------------------------------------------------------

#endif /* !__FRBYTORD_H_INCLUDED */

// end of file frbytord.h //
