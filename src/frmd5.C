/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File: frmd5.cpp		MD5 hash function			*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1998,2001,2003,2006,2009,2013				*/
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

#include "frcommon.h"
#include "framerr.h"
#include "frbytord.h"
#include "frmd5.h"

#ifdef FrSTRICT_CPLUSPLUS
#  include <iomanip>
#  include <cstring>		// needed for GCC 4.3
#  include <string>		// needed for RedHat 7.1
#else
#  include <iomanip.h>
#  include <string.h>		// needed for RedHat 7.1
#endif /* FrSTRICT_CPLUSPLUS */

/*     From draft RFC:
** **************************************************************************
** md5.h -- Header file for implementation of MD5 Message Digest Algorithm **
** Updated: 2/13/90 by Ronald L. Rivest                                    **
** (C) 1990 RSA Data Security, Inc.                                        **
** **************************************************************************
*/

/* MDstruct is the data structure for a message digest computation.
*/
typedef struct
   {
   uint32_t buffer[4];			/* 4-word result of computation */
   unsigned char count[8];		/* Number of bits processed so far */
   } MDstruct, *MDptr;

/*** End of md5.h ***/
/*
** **************************************************************************
** md5.c -- Implementation of MD5 Message Digest Algorithm                 **
** Updated: 2/16/90 by Ronald L. Rivest                                    **
** (C) 1990 RSA Data Security, Inc.                                        **
** **************************************************************************
*/

/* Compile-time declarations of MD5 ``magic constants''.
*/
#define I0  0x67452301U      /* Initial values for MD buffer */
#define I1  0xefcdab89U
#define I2  0x98badcfeU
#define I3  0x10325476U
#define fs1  7               /* round 1 shift amounts */
#define fs2 12
#define fs3 17
#define fs4 22
#define gs1  5		     /* round 2 shift amounts */
#define gs2  9
#define gs3 14
#define gs4 20
#define hs1  4		     /* round 3 shift amounts */
#define hs2 11
#define hs3 16
#define hs4 23
#define is1  6		     /* round 4 shift amounts */
#define is2 10
#define is3 15
#define is4 21

#define rot0(X) X
#if defined(__WATCOMC__) && defined(__386__)
extern uint32_t rotfs1(uint32_t X) ;
# pragma aux rotfs1 = \
      "rol eax,7" \
      parm [eax] \
      value [eax] \
      modify exact [eax] ;
extern uint32_t rotfs2(uint32_t X) ;
# pragma aux rotfs2 = \
      "rol eax,12" \
      parm [eax] \
      value [eax] \
      modify exact [eax] ;
extern uint32_t rotfs3(uint32_t X) ;
# pragma aux rotfs3 = \
      "rol eax,17" \
      parm [eax] \
      value [eax] \
      modify exact [eax] ;
extern uint32_t rotfs4(uint32_t X) ;
# pragma aux rotfs4 = \
      "rol eax,22" \
      parm [eax] \
      value [eax] \
      modify exact [eax] ;
extern uint32_t rotgs1(uint32_t X) ;
# pragma aux rotgs1 = \
      "rol eax,5" \
      parm [eax] \
      value [eax] \
      modify exact [eax] ;
extern uint32_t rotgs2(uint32_t X) ;
# pragma aux rotgs2 = \
      "rol eax,9" \
      parm [eax] \
      value [eax] \
      modify exact [eax] ;
extern uint32_t rotgs3(uint32_t X) ;
# pragma aux rotgs3 = \
      "rol eax,14" \
      parm [eax] \
      value [eax] \
      modify exact [eax] ;
extern uint32_t rotgs4(uint32_t X) ;
# pragma aux rotgs4 = \
      "rol eax,20" \
      parm [eax] \
      value [eax] \
      modify exact [eax] ;
extern uint32_t roths1(uint32_t X) ;
# pragma aux roths1 = \
      "rol eax,4" \
      parm [eax] \
      value [eax] \
      modify exact [eax] ;
extern uint32_t roths2(uint32_t X) ;
# pragma aux roths2 = \
      "rol eax,11" \
      parm [eax] \
      value [eax] \
      modify exact [eax] ;
extern uint32_t roths3(uint32_t X) ;
# pragma aux roths3 = \
      "rol eax,16" \
      parm [eax] \
      value [eax] \
      modify exact [eax] ;
extern uint32_t roths4(uint32_t X) ;
# pragma aux roths4 = \
      "rol eax,23" \
      parm [eax] \
      value [eax] \
      modify exact [eax] ;
extern uint32_t rotis1(uint32_t X) ;
# pragma aux rotis1 = \
      "rol eax,6" \
      parm [eax] \
      value [eax] \
      modify exact [eax] ;
extern uint32_t rotis2(uint32_t X) ;
# pragma aux rotis2 = \
      "rol eax,10" \
      parm [eax] \
      value [eax] \
      modify exact [eax] ;
extern uint32_t rotis3(uint32_t X) ;
# pragma aux rotis3 = \
      "rol eax,15" \
      parm [eax] \
      value [eax] \
      modify exact [eax] ;
extern uint32_t rotis4(uint32_t X) ;
# pragma aux rotis4 = \
      "rol eax,21" \
      parm [eax] \
      value [eax] \
      modify exact [eax] ;
#else
#if defined(_MSC_VER) && _MSC_VER >= 800
inline uint32_t rot(uint32_t X, int S)
{
   uint32_t result ;
   _asm {
          mov eax,X ;
	  mov cl,S ;
	  rol eax,cl ;
	  mov result,eax ;
        } ;
   return result ;
}
#elif defined(__GNUC__) && defined(__386__)
inline uint32_t rot(uint32_t X, int S)
{
   uint32_t result ;
   __asm__("mov %b2,%%cl\n\t"
	   "rol %%cl,%1" : "=g" (result) : "0" (X), "g" (S) : "ecx", "cc") ;
   return result ;
}

#else
inline uint32_t rot(uint32_t X, int S)
{
   uint32_t tmp = X ;
   return (tmp<<S) | (tmp>>(32-S)) ;
}
#endif /* _MSC_VER */
#define rotfs1(X) rot(X,fs1)
#define rotfs2(X) rot(X,fs2)
#define rotfs3(X) rot(X,fs3)
#define rotfs4(X) rot(X,fs4)
#define rotgs1(X) rot(X,gs1)
#define rotgs2(X) rot(X,gs2)
#define rotgs3(X) rot(X,gs3)
#define rotgs4(X) rot(X,gs4)
#define roths1(X) rot(X,hs1)
#define roths2(X) rot(X,hs2)
#define roths3(X) rot(X,hs3)
#define roths4(X) rot(X,hs4)
#define rotis1(X) rot(X,is1)
#define rotis2(X) rot(X,is2)
#define rotis3(X) rot(X,is3)
#define rotis4(X) rot(X,is4)
#endif /* __WATCOMC__ && __386__ */

/* Compile-time macro declarations for MD5.
*/
#define	f(X,Y,Z)             ((X&Y) | ((~X)&Z))
#define	g(X,Y,Z)             ((X&Z) | (Y&(~Z)))
#define h(X,Y,Z)             (X^Y^Z)
#define i_(X,Y,Z)            (Y ^ ((X) | (~Z)))
//#define nextX(a,b)	       (X += (a-b))
#define nextX(a,b)
#define ff(A,B,C,D,i,s,lp) A = rot##s((f(B,C,D)+ FrLoadLongLE(X+i)+ lp+ A)) + B
#define gg(A,B,C,D,i,s,lp) A = rot##s((FrLoadLongLE(X+i)+ lp+ A+ g(B,C,D))) + B
#define hh(A,B,C,D,i,s,lp) A = rot##s((FrLoadLongLE(X+i)+ lp+ A+ h(B,C,D))) + B
#define ii(A,B,C,D,i,s,lp) A = rot##s((FrLoadLongLE(X+i)+ lp+ A+ i_(B,C,D))) + B

//----------------------------------------------------------------------

/* MDblock(MDp,X)
** Update message digest buffer MDp->buffer using 16-word data block X.
** Assumes all 16 words of X are full of data.
*/
static void MDblock(register uint32_t const * volatile X,MDptr mdp)
{
   static MDptr MDp ;
   MDp = mdp ;
   register uint32_t A, B, C, D;
   A = MDp->buffer[0];
   B = MDp->buffer[1];
   C = MDp->buffer[2];
   D = MDp->buffer[3];

   /* Update the message digest buffer */
   /* Round 1 */
   nextX( 0, 0) ; ff(A , B , C , D ,  0 , fs1 , 3614090360U);
   nextX( 1, 0) ; ff(D , A , B , C ,  1 , fs2 , 3905402710U);
   nextX( 2, 1) ; ff(C , D , A , B ,  2 , fs3 ,  606105819U);
   nextX( 3, 2) ; ff(B , C , D , A ,  3 , fs4 , 3250441966U);
   nextX( 4, 3) ; ff(A , B , C , D ,  4 , fs1 , 4118548399U);
   nextX( 5, 4) ; ff(D , A , B , C ,  5 , fs2 , 1200080426U);
   nextX( 6, 5) ; ff(C , D , A , B ,  6 , fs3 , 2821735955U);
   nextX( 7, 6) ; ff(B , C , D , A ,  7 , fs4 , 4249261313U);
   nextX( 8, 7) ; ff(A , B , C , D ,  8 , fs1 , 1770035416U);
   nextX( 9, 8) ; ff(D , A , B , C ,  9 , fs2 , 2336552879U);
   nextX(10, 9) ; ff(C , D , A , B , 10 , fs3 , 4294925233U);
   nextX(11,10) ; ff(B , C , D , A , 11 , fs4 , 2304563134U);
   nextX(12,11) ; ff(A , B , C , D , 12 , fs1 , 1804603682U);
   nextX(13,12) ; ff(D , A , B , C , 13 , fs2 , 4254626195U);
   nextX(14,13) ; ff(C , D , A , B , 14 , fs3 , 2792965006U);
   nextX(15,14) ; ff(B , C , D , A , 15 , fs4 , 1236535329U);
   /* Round 2 */
   nextX( 1,15) ; gg(A , B , C , D ,  1 , gs1 , 4129170786U);
   nextX( 6, 1) ; gg(D , A , B , C ,  6 , gs2 , 3225465664U);
   nextX(11, 6) ; gg(C , D , A , B , 11 , gs3 ,  643717713U);
   nextX( 0,11) ; gg(B , C , D , A ,  0 , gs4 , 3921069994U);
   nextX( 5, 0) ; gg(A , B , C , D ,  5 , gs1 , 3593408605U);
   nextX(10, 5) ; gg(D , A , B , C , 10 , gs2 ,   38016083U);
   nextX(15,10) ; gg(C , D , A , B , 15 , gs3 , 3634488961U);
   nextX( 4,15) ; gg(B , C , D , A ,  4 , gs4 , 3889429448U);
   nextX( 9, 4) ; gg(A , B , C , D ,  9 , gs1 ,  568446438U);
   nextX(14, 9) ; gg(D , A , B , C , 14 , gs2 , 3275163606U);
   nextX( 3,14) ; gg(C , D , A , B ,  3 , gs3 , 4107603335U);
   nextX( 8, 3) ; gg(B , C , D , A ,  8 , gs4 , 1163531501U);
   nextX(13, 8) ; gg(A , B , C , D , 13 , gs1 , 2850285829U);
   nextX( 2,13) ; gg(D , A , B , C ,  2 , gs2 , 4243563512U);
   nextX( 7, 2) ; gg(C , D , A , B ,  7 , gs3 , 1735328473U);
   nextX(12, 7) ; gg(B , C , D , A , 12 , gs4 , 2368359562U);
   /* Round 3 */
   nextX( 5,12) ; hh(A , B , C , D ,  5 , hs1 , 4294588738U);
   nextX( 8, 5) ; hh(D , A , B , C ,  8 , hs2 , 2272392833U);
   nextX(11, 8) ; hh(C , D , A , B , 11 , hs3 , 1839030562U);
   nextX(14,11) ; hh(B , C , D , A , 14 , hs4 , 4259657740U);
   nextX( 1,14) ; hh(A , B , C , D ,  1 , hs1 , 2763975236U);
   nextX( 4, 1) ; hh(D , A , B , C ,  4 , hs2 , 1272893353U);
   nextX( 7, 4) ; hh(C , D , A , B ,  7 , hs3 , 4139469664U);
   nextX(10, 7) ; hh(B , C , D , A , 10 , hs4 , 3200236656U);
   nextX(13,10) ; hh(A , B , C , D , 13 , hs1 ,  681279174U);
   nextX( 0,13) ; hh(D , A , B , C ,  0 , hs2 , 3936430074U);
   nextX( 3, 0) ; hh(C , D , A , B ,  3 , hs3 , 3572445317U);
   nextX( 6, 3) ; hh(B , C , D , A ,  6 , hs4 ,   76029189U);
   nextX( 9, 6) ; hh(A , B , C , D ,  9 , hs1 , 3654602809U);
   nextX(12, 9) ; hh(D , A , B , C , 12 , hs2 , 3873151461U);
   nextX(15,12) ; hh(C , D , A , B , 15 , hs3 ,  530742520U);
   nextX( 2,15) ; hh(B , C , D , A ,  2 , hs4 , 3299628645U);
   /* Round 4 */
   nextX( 0, 2) ; ii(A , B , C , D ,  0 , is1 , 4096336452U);
   nextX( 7, 0) ; ii(D , A , B , C ,  7 , is2 , 1126891415U);
   nextX(14, 7) ; ii(C , D , A , B , 14 , is3 , 2878612391U);
   nextX( 5,14) ; ii(B , C , D , A ,  5 , is4 , 4237533241U);
   nextX(12, 5) ; ii(A , B , C , D , 12 , is1 , 1700485571U);
   nextX( 3,12) ; ii(D , A , B , C ,  3 , is2 , 2399980690U);
   nextX(10, 3) ; ii(C , D , A , B , 10 , is3 , 4293915773U);
   nextX( 1,10) ; ii(B , C , D , A ,  1 , is4 , 2240044497U);
   nextX( 8, 1) ; ii(A , B , C , D ,  8 , is1 , 1873313359U);
   nextX(15, 8) ; ii(D , A , B , C , 15 , is2 , 4264355552U);
   nextX( 6,15) ; ii(C , D , A , B ,  6 , is3 , 2734768916U);
   nextX(13, 6) ; ii(B , C , D , A , 13 , is4 , 1309151649U);
   nextX( 4,13) ; ii(A , B , C , D ,  4 , is1 , 4149444226U);
   nextX(11, 4) ; ii(D , A , B , C , 11 , is2 , 3174756917U);
   nextX( 2,11) ; ii(C , D , A , B ,  2 , is3 ,  718787259U);
   nextX( 9, 2) ; ii(B , C , D , A ,  9 , is4 , 3951481745U);

   MDp->buffer[0] += A;
   MDp->buffer[1] += B;
   MDp->buffer[2] += C;
   MDp->buffer[3] += D;
   return ;
}

//----------------------------------------------------------------------

/* MDupdate(MDp,X,count)
** Input: MDp -- an MDptr
**        X -- a pointer to an array of unsigned characters.
**        count -- the number of bits of X to use.
**                 (if not a multiple of 8, uses high bits of last byte.)
** Update MDp using the number of bits of X given by count.
** The routine completes the MD computation when count < 512, so
** every MD computation should end with one call to MDupdate with a
** count less than 512.
*/
static void MDupdate(MDptr MDp,unsigned char *X,unsigned int count)
{
   /* Add count to MDp->count */
   unsigned int tmp = count ;
   unsigned char *p = MDp->count ;
   while (tmp)
      {
      tmp += *p;
      *p++ = (unsigned char)tmp;
      tmp = tmp >> 8;
      }
   /* Process data */
   if (count == 512)			/* Full block of data ? */
      MDblock((uint32_t*)X,MDp);
   else /* partial block -- must be last block so finish up */
      { /* Find out how many bytes and residual bits there are */
      unsigned int i, bit, byte, mask;
      unsigned char XX[64];
      byte = count >> 3;
      bit =  count & 7;
      /* Copy X into XX since we need to modify it */
      for (i = 0 ; i <= byte ; i++)
	 XX[i] = X[i];
      for (/*i = byte+1*/ ; i < sizeof(XX) ; i++)
	 XX[i] = 0;
      /* Add padding '1' bit and low-order zeros in last byte */
      mask = 1 << (7 - bit);
      XX[byte] = (unsigned char)((XX[byte] | mask) & ~( mask - 1)) ;
      /* If room for bit count, finish up with this block */
      if (byte >= 56) /* need to do two blocks to finish up? */
	 {
	 MDblock((uint32_t *)XX,MDp);
	 for (i=0;i<56;i++)
	    XX[i] = 0;
	 }
      for (i=0;i<8;i++)
	 XX[56+i] = MDp->count[i];
      MDblock((uint32_t *)XX,MDp);
      }
   return ;
}

/***  End of md5.c ***/

/************************************************************************/
/************************************************************************/

FrMD5Signature::FrMD5Signature(uint32_t sig[4])
{
   FrStoreLongLE(sig[0],signature) ;
   FrStoreLongLE(sig[1],signature+4) ;
   FrStoreLongLE(sig[2],signature+8) ;
   FrStoreLongLE(sig[3],signature+12) ;
}

//----------------------------------------------------------------------

void FrMD5Signature::print(FILE *fp) const
{
   // Print message digest buffer MDp as 32 hexadecimal digits.
   // Order is from low-order byte to high-order byte.
   for (size_t i = 0 ; i < sizeof(signature) ; i++)
      fprintf(fp,"%2.02x",signature[i] & 0xFF) ;
   return ;
}

//----------------------------------------------------------------------

ostream &FrMD5Signature::print(ostream &out) const
{
   // Print message digest buffer MDp as 32 hexadecimal digits.
   // Order is from low-order byte to high-order byte.
   for (size_t i = 0 ; i < sizeof(signature) ; i++)
      out << hex << ((signature[i]&0xF0)>>4) << hex << (signature[i] & 0x0F) ;
   return out ;
}

/************************************************************************/
/************************************************************************/

FrMD5Signature *FrMD5(char *data, size_t length)
{
   // compute signature of given block of data
   MDstruct MD ;
   MD.buffer[0] = I0;
   MD.buffer[1] = I1;
   MD.buffer[2] = I2;
   MD.buffer[3] = I3;
   // zero out the count of bits processed
   unsigned int i ;
   for (i = 0 ; i < 8 ; i++)
      MD.count[i] = 0;
   for (i = 0 ; i+64 <= length ; i += 64)
      MDupdate(&MD,(unsigned char*)(data+i),512) ;
   MDupdate(&MD,(unsigned char*)(data+i),(length-i)*8) ;
   return new FrMD5Signature(MD.buffer) ;
}

//----------------------------------------------------------------------
// compute signature of NUL-terminated string

FrMD5Signature *FrMD5(char *string)
{
   return FrMD5(string,string ? strlen(string) : 0) ;
}

// end of file frmd5.cpp //
