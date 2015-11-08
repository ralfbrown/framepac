/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC								*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File fr_bwt.h	  private defs for  B-W Transform n-gram index	*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 2003,2004,2007,2009					*/
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

/************************************************************************/
/*	Manifest Constants						*/
/************************************************************************/

#define COMPRESSED_EOR		0

#define CHUNK_SIZE (4*1024*1024)

#define DEFAULT_BUCKET_SIZE	64

#define BWT_UNCOMP	0
#define BWT_BYTECOMP 	1
#define BWT_HIGHESTCOMP BWT_BYTECOMP

/************************************************************************/
/*	Types								*/
/************************************************************************/

enum BWTHeader_Flags
   {
      BWTF_WordsReversed = 1,
      BWTF_CaseSensitive = 2,
      BWTF_CharBased = 4
   } ;

class BWTHeader
{
   public:
      int    m_fileformat ;
      int    m_compression ;
      int    m_eor_handling ;
      int    m_flags ;
      size_t m_userdata ;
      size_t m_C_offset ;
      size_t m_C_length ;
      size_t m_FL_offset ;
      size_t m_FL_length ;
      size_t m_FL_total ;
      size_t m_EOR ;
      double m_discount ;
      size_t m_bucketsize ;
      size_t m_buckets_offset ;
      size_t m_bucketpool_offset ;
      size_t m_bucketpool_length ;
      int    m_maxdelta ;
      int    m_affix_sizes ;
} ;

/************************************************************************/
/************************************************************************/

bool Fr_read_long(FILE *fp, size_t &value) ;
bool Fr_write_long(FILE *fp, size_t value) ;

bool Fr_read_BWT_header(FILE *fp, BWTHeader *header,
			const char *signature) ;

bool Fr_write_BWT_header(FILE *fp, BWTHeader *header,
			 const char *signature,
			 long header_offset = 0) ;

// end of file fr_bwt.h //
