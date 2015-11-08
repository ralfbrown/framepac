/************************************************************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frctype.cpp		character-manipulation functions	*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1996,1997,2000,2009,2013				*/
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

#include "frctype.h"

/************************************************************************/
/*    Global data for this module					*/
/************************************************************************/

//----------------------------------------------------------------------

// Latin-1/ANSI character set
const unsigned char  FramepaC_unaccent_table_Latin1[] =
   {
      0,   1,	2,   3,	  4,   5,   6,	 7, // ^@-^G
      8,   9,  10,  11,	 12,  13,  14,	15, // ^H-^O
     16,  17,  18,  19,	 20,  21,  22,	23, // ^P-^W
     24,  25,  26,  27,	 28,  29,  30,	31, // ^X-^_
     32,  33,  34,  35,	 36,  37,  38,	39, // SP-
     40,  41,  42,  43,	 44,  45,  46,	47, // - /
     48,  49,  50,  51,	 52,  53,  54,	55, // 0 - 7
     56,  57,  58,  59,	 60,  61,  62,	63, // 8 - ?
     64, 'A',  66,  67,	 68,  69,  70,	71, // @ - G
     72,  73,  74,  75,	 76,  77,  78,	79, // H - O
     80,  81,  82,  83,	 84,  85,  86,	87, // P - W
     88,  89, 'Z',  91,	 92,  93,  94,	95, // X - _
     96, 'a',  98,  99, 100, 101, 102, 103, // ` to g
    104, 105, 106, 107, 108, 109, 110, 111, // h to o
    112, 113, 114, 115, 116, 117, 118, 119, // p to w
    120, 121, 'z', 123, 124, 125, 126, 127, // x to DEL
    128, 129, 130, 131, 132, 133, 134, 135,
    136, 137, 'S', 139, 140, 141, 142, 143,
    144, 145, 146, 147, 148, 149, 150, 151,
    152, 153, 's', 155, 156, 157, 158, 'Y',
    '!', 161, 162, 163, 164, 165, 166, 167,
    168, 169, 170, 171, 172, 173, 174, 175,
    176, 177, 178, 179, 180, 181, 182, 183,
    184, 185, 186, 187, 188, 189, 190, '?',
    'A', 'A', 'A', 'A', 'A', 'A', 198, 'C',
    'E', 'E', 'E', 'E', 'I', 'I', 'I', 'I',
    'D', 'N', 'O', 'O', 'O', 'O', 'O', 215,
    'O', 'U', 'U', 'U', 'U', 'Y', 222, 223,
    'a', 'a', 'a', 'a', 'a', 'a', 230, 'c',
    'e', 'e', 'e', 'e', 'i', 'i', 'i', 'i',
    'd', 'n', 'o', 'o', 'o', 'o', 'o', 247,
    'o', 'u', 'u', 'u', 'u', 'y', 254, 'y'
   } ;

// Latin-2 character set
const unsigned char FramepaC_unaccent_table_Latin2[] =
   {
      0,   1,	2,   3,	  4,   5,   6,	 7, // ^@-^G
      8,   9,  10,  11,	 12,  13,  14,	15, // ^H-^O
     16,  17,  18,  19,	 20,  21,  22,	23, // ^P-^W
     24,  25,  26,  27,	 28,  29,  30,	31, // ^X-^_
     32,  33,  34,  35,	 36,  37,  38,	39, // SP-
     40,  41,  42,  43,	 44,  45,  46,	47, // - /
     48,  49,  50,  51,	 52,  53,  54,	55, // 0 - 7
     56,  57,  58,  59,	 60,  61,  62,	63, // 8 - ?
     64,  65,  66,  67,	 68,  69,  70,	71, // @ - G
     72,  73,  74,  75,	 76,  77,  78,	79, // H - O
     80,  81,  82,  83,	 84,  85,  86,	87, // P - W
     88,  89,  90,  91,	 92,  93,  94,	95, // X - _
     96, 'a', 'b', 'c', 'd', 'e', 'f', 'g', // ` to g
    'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', // h to o
    'p', 'q', 'r', 's', 't', 'u', 'v', 'w', // p to w
    'x', 'y', 'z', 123, 124, 125, 126, 127, // x to DEL
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
    0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
    0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
    '!',  0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7,
    0xA8, 'S',  0xAA, 0xAB, 0xAC, 0xAD, 'Z',  0xAF,
    0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7,
    0xB8, 's',  0xBA, 0xBB, 0xBC, 0xBD, 'z',  '?',
    'A',  'A',  'A',  'A',  'A',  'A',  'C',  'C',
    'C',  'E',  'E',  'E',  'I',  'I',  'I',  'I',
    'D',  'N',  'O',  'O',  'O',  'O',  'O',  '*',
    'O',  'U',  'U',  'U',  'U',  'Y',  0xDE, 0xDF,
    'a',  'a',  'a',  'a',  'a',  'a',  'c',  'c',
    'c',  'e',  'e',  'e',  'i',  'i',  'i',  'i',
    'd',  'n',  'o',  'o',  'o',  'o',  'o',  0xF7,
    'u',  'u',  'u',  'u',  'u',  'y',  0xFE, 'y'
   } ;

/************************************************************************/
/************************************************************************/


//----------------------------------------------------------------------


// end of file frctype2.cpp //
