/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC								*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frbwt.cpp	    Burrows-Wheeler Transform n-gram index	*/
/*  LastEdit: 80nov2015							*/
/*									*/
/*  (c) Copyright 2003,2004,2005,2006,2007,2008,2009,2010,2012		*/
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

#include "frassert.h"
#include "frbwt.h"
#include "frmmap.h"
#include "frutil.h"
#include "fr_bwt.h"
#include <errno.h>
#include <string.h>

#define USE_BINSORT

/************************************************************************/
/*	Manifest Constants						*/
/************************************************************************/

#define SIGNATURE "BWT-encoded data"
#define MAX_SIGNATURE	32

#define DISCOUNTMASS_SCALE	4000000000UL
#define DEFAULT_DISCOUNTMASS	1e-3

#define FILE_FORMAT 	1

#define BWT_HEADER_SIZE 96

#define DOT_INTERVAL 500000

/************************************************************************/
/*	Types								*/
/************************************************************************/

typedef char SHORTbuffer[2] ;
typedef char LONGbuffer[4] ;

/************************************************************************/
/*	Global Variables						*/
/************************************************************************/

FrAllocator FrBWTLocationList::allocator("FrBWTLocationList",
					 sizeof(FrBWTLocationList)) ;

/************************************************************************/
/*	Helper Functions						*/
/************************************************************************/

inline void swap(uint32_t *elt1, uint32_t *elt2)
{
   uint32_t tmp = *elt1 ;
   *elt1 = *elt2 ;
   *elt2 = tmp ;
   return ;
}

void FrSortWordIDs(uint32_t elts[], size_t num_elts)
{
   while (num_elts > 2)
      {
      // find a partitioning element
      uint32_t *mid = &elts[num_elts/2] ;
      uint32_t *right = &elts[num_elts-1] ;
      if (elts[0] > *mid)
	 swap(&elts[0],mid) ;
      if (elts[0] > *right)
	 swap(&elts[0],right) ;
      if (*mid > *right)
	 swap(mid,right) ;
      if (num_elts <= 3)
	 return ;			// we already sorted the entire range
      // partition the array
      swap(mid,right) ;			// put partition elt at end of array
      uint32_t *partval = right ;
      uint32_t *left = elts ;
      right-- ;
      do {
	 while (left <= right && *left <= *partval)
	    left++ ;
	 while (left <= right && *partval <= *right)
	    right-- ;
	 swap(left,right) ;
	 } while (left < right) ;
      size_t part = left - elts ;
      swap(left,right) ;
      // move partitioning element to its final location
      swap(left,&elts[num_elts-1]) ;
      // sort each half of the array
      if (part >= num_elts-part)
	 {
	 // left half is larger, so recurse on smaller right half
	 size_t half = num_elts-part-1 ;
	 if (half > 1)			// no need to recurse if only one elt
	    FrSortWordIDs(elts+part+1,half) ;
	 num_elts = part ;
	 }
      else
	 {
	 // right half is larger, so recurse on smaller left half
	 if (part > 1)			// no need to recurse if only one elt
	    FrSortWordIDs(elts,part) ;
	 elts += part+1 ;
	 num_elts -= part+1 ;
	 }
      }
   // when we get to this point, we have at most two items left;
   // a single item is already sorted by definition, and two items need at
   //   most a simple swap
   if (num_elts == 2 && elts[0] > elts[1])
      swap(&elts[0],&elts[1]) ;
   return ;
}

/************************************************************************/
/*	Data I/O Functions						*/
/************************************************************************/

static bool read_byte(FILE *fp, int &value)
{
   if (fp)
      {
      unsigned char buf[1] ;
      if (fread(buf,sizeof(buf),1,fp) == 1)
	 {
	 value = buf[0] ;
	 return true ;
	 }
      }
   return false ;
}

//----------------------------------------------------------------------

bool Fr_read_short(FILE *fp, uint16_t &value)
{
   if (fp)
      {
      SHORTbuffer buf ;
      if (fread(buf,sizeof(buf),1,fp) == 1)
	 {
	 value = FrLoadShort(buf) ;
	 return true ;
	 }
      }
   return false ;
}

//----------------------------------------------------------------------

bool Fr_read_long(FILE *fp, size_t &value)
{
   if (fp)
      {
      LONGbuffer buf ;
      if (fread(buf,sizeof(buf),1,fp) == 1)
	 {
	 value = FrLoadLong(buf) ;
	 return true ;
	 }
      }
   return false ;
}

//----------------------------------------------------------------------

bool Fr_read_BWT_header(FILE *fp, BWTHeader *header, const char *signature)
{
   if (fp && header)
      {
      char sig[MAX_SIGNATURE] ;
      if (fread(sig,sizeof(char),sizeof(sig),fp) == sizeof(sig))
	 {
	 sig[sizeof(sig)-1] = '\0' ;
	 int reserved ;
	 uint16_t reservedshort ;
	 size_t discmass ;
	 uint16_t userdata_hi, buckets_hi, bucketpool_hi ;
	 if (strcmp(sig,signature) == 0 &&
	     read_byte(fp,header->m_fileformat) &&
	     read_byte(fp,header->m_compression) &&
	     read_byte(fp,header->m_eor_handling) &&
	     read_byte(fp,header->m_flags) &&
	     Fr_read_long(fp,header->m_userdata) &&
	     Fr_read_long(fp,header->m_C_offset) &&
	     Fr_read_long(fp,header->m_C_length) &&
	     Fr_read_long(fp,header->m_FL_offset) &&
	     Fr_read_long(fp,header->m_FL_length) &&
	     Fr_read_long(fp,header->m_FL_total) &&
	     Fr_read_long(fp,header->m_EOR) &&
	     Fr_read_long(fp,header->m_bucketsize) &&
	     Fr_read_long(fp,header->m_buckets_offset) &&
	     Fr_read_long(fp,header->m_bucketpool_offset) &&
	     Fr_read_long(fp,header->m_bucketpool_length) &&
	     read_byte(fp,header->m_maxdelta) &&
	     read_byte(fp,header->m_affix_sizes) &&
	     read_byte(fp,reserved) &&
	     read_byte(fp,reserved) &&
	     Fr_read_long(fp,discmass) &&
	     Fr_read_short(fp,userdata_hi) &&
	     Fr_read_short(fp,buckets_hi) &&
	     Fr_read_short(fp,bucketpool_hi) &&
	     Fr_read_short(fp,reservedshort)
	    )
	    {
	    header->m_userdata |= (((off_t)userdata_hi) << 32) ;
	    header->m_buckets_offset |= (((off_t)buckets_hi) << 32) ;
	    header->m_bucketpool_offset |= (((off_t)bucketpool_hi) << 32) ;
	    header->m_discount = discmass / (double)DISCOUNTMASS_SCALE ;
	    return true ;
	    }
	 }
      }
   return false ;
}

//----------------------------------------------------------------------

static bool write_byte(FILE *fp, int value)
{
   if (fp)
      {
      unsigned char buf[1] ;
      buf[0] = (unsigned char)value ;
      if (fwrite(buf,sizeof(buf),1,fp) == 1)
	 return true ;
      }
   return false ;
}

//----------------------------------------------------------------------

bool Fr_write_short(FILE *fp, uint16_t value)
{
   if (fp)
      {
      SHORTbuffer buf ;
      FrStoreShort(value,buf) ;
      if (fwrite(buf,sizeof(buf),1,fp) == 1)
	 return true ;
      }
   return false ;
}

//----------------------------------------------------------------------

bool Fr_write_long(FILE *fp, size_t value)
{
   if (fp)
      {
      LONGbuffer buf ;
      FrStoreLong(value,buf) ;
      if (fwrite(buf,sizeof(buf),1,fp) == 1)
	 return true ;
      }
   return false ;
}

//----------------------------------------------------------------------

bool Fr_write_BWT_header(FILE *fp, BWTHeader *header,
			 const char *signature,
			 long header_offset)
{
   if (fp)
      {
      fseek(fp,header_offset,SEEK_SET) ;
      char sig[MAX_SIGNATURE] ;
      memset(sig,'\0',MAX_SIGNATURE) ;
      size_t siglen = strlen(signature) ;
      if (siglen > MAX_SIGNATURE)
	 siglen = MAX_SIGNATURE ;
      memcpy(sig,signature,siglen) ;
      uint32_t discmass =
	 (uint32_t)(header->m_discount * DISCOUNTMASS_SCALE + 0.5) ;
      if (Fr_fwrite(sig,sizeof(char),sizeof(sig),fp) &&
	  write_byte(fp,header->m_fileformat) &&
	  write_byte(fp,header->m_compression) &&
	  write_byte(fp,header->m_eor_handling) &&
	  write_byte(fp,header->m_flags) &&
	  Fr_write_long(fp,header->m_userdata) &&
	  Fr_write_long(fp,header->m_C_offset) &&
	  Fr_write_long(fp,header->m_C_length) &&
	  Fr_write_long(fp,header->m_FL_offset) &&
	  Fr_write_long(fp,header->m_FL_length) &&
	  Fr_write_long(fp,header->m_FL_total) &&
	  Fr_write_long(fp,header->m_EOR) &&
	  Fr_write_long(fp,header->m_bucketsize) &&
	  Fr_write_long(fp,header->m_buckets_offset) &&
	  Fr_write_long(fp,header->m_bucketpool_offset) &&
	  Fr_write_long(fp,header->m_bucketpool_length) &&
	  write_byte(fp,header->m_maxdelta) &&
	  write_byte(fp,header->m_affix_sizes) &&
	  write_byte(fp,0) &&
	  write_byte(fp,0) &&
	  Fr_write_long(fp,discmass) &&
#if __BITS__ > 32
	  Fr_write_short(fp,header->m_userdata >> 32) &&
	  Fr_write_short(fp,header->m_buckets_offset >> 32) &&
	  Fr_write_short(fp,header->m_bucketpool_offset >> 32) &&
#else
	  Fr_write_short(fp,0) &&
	  Fr_write_short(fp,0) &&
	  Fr_write_short(fp,0) &&
#endif /* __BITS__ > 32 */
	  Fr_write_short(fp,0)
	 )
	 return true ;
      }
   return false ;
}

//----------------------------------------------------------------------
//  tripartite QuickSort with multi-level matching of arbitrary-length
//    suffixes

inline void swap(uint32_t &x, uint32_t &y)
{
   uint32_t tmp ;
   tmp = x ;
   x = y ;
   y = tmp ;
   return ;
}

void Bentley_Sedgewick_Sort(uint32_t *elts, size_t num_elts,
			    const uint32_t *data, uint32_t datasize,
			    size_t dataindex = 0, uint32_t eor = ~0)
{
   uint32_t vA, vB ;
#define value(N) (data[(elts[N]+dataindex)%datasize])
#define out_of_order(A,B) \
	(((vA = value(A)) > (vB = value(B))) || (vA==vB && elts[A] > elts[B]))
   while (num_elts > 2)
      {
      // find a partitioning element
      size_t mid = num_elts / 2 ;
      size_t right = num_elts - 1 ;
      if (out_of_order(0,mid))
	 swap(elts[0],elts[mid]) ;
      if (out_of_order(mid,right))
	 swap(elts[mid],elts[right]) ;
      if (out_of_order(0,mid))
	 swap(elts[0],elts[mid]) ;
      // partition the array
      uint32_t partval = value(mid) ;
      size_t pivot_lo = 0 ;
      size_t pivot_hi = num_elts - 1 ;
      size_t left = 0 ;
      for ( ; ; )
	 {
	 uint32_t val ;
	 while (left <= right && (val = value(left)) <= partval)
	    {
	    if (val == partval)
	       swap(elts[pivot_lo++],elts[left]) ;
	    left++ ;
	    }
	 while (left <= right && (val = value(right)) >= partval)
	    {
	    if (val == partval)
	       swap(elts[pivot_hi--],elts[right]) ;
	    right-- ;
	    }
	 if (left > right)
	    break ;
	 swap(elts[left++],elts[right--]) ;
	 }
      // move all elements equal to the pivot to the middle of the array
      size_t to_move = (pivot_lo<left-pivot_lo) ? pivot_lo : left - pivot_lo ;
      size_t i ;
      for (i = 0 ; i < to_move ; i++)
	 swap(elts[i],elts[left-to_move+i]) ;
      to_move = num_elts - pivot_hi - 1 ;
      size_t equal_pivot = to_move + pivot_lo ;
      if (pivot_hi - right < to_move)
	 to_move = pivot_hi - right ;
      for (i = 0 ; i < to_move ; i++)
	 swap(elts[left+i],elts[num_elts-to_move+i]) ;
      left -= pivot_lo ;
      right += (num_elts - pivot_hi) ;
      // sort each third of the array, starting with the middle, for which we
      //   now move to the next position in the suffix if we're not already
      //   looking at an end-of-record marker
      if (partval < eor && equal_pivot > 1)
	 Bentley_Sedgewick_Sort(elts+left,equal_pivot,
				data,datasize,dataindex+1,eor) ;
      if (left >= num_elts-right)
	 {
	 // left half is larger, so recurse on smaller right half
	 size_t half = num_elts-right ;
	 if (half > 1)			// no need to recurse if only one elt
	    Bentley_Sedgewick_Sort(elts+right,half,data,datasize,dataindex,
				   eor) ;
	 num_elts = left ;
	 }
      else
	 {
	 // right half is larger, so recurse on smaller left half
	 if (left > 1)			// no need to recurse if only one elt
	    Bentley_Sedgewick_Sort(elts,left,data,datasize,dataindex,eor) ;
	 elts += right ;
	 num_elts -= right ;
	 }
      }
   // when we get to this point, we have at most two items left;
   // a single item is already sorted by definition, and two items need at
   //   most a simple swap
   if (num_elts == 2)
      {
      while (value(0) == value(1) && dataindex < datasize)
	 dataindex++ ;
      if (out_of_order(0,1))
	 swap(elts[0],elts[1]) ;
      }
   return ;
#undef value
#undef out_of_order
}

/************************************************************************/
/*	Methods for class FrBWTIndex					*/
/************************************************************************/

FrBWTIndex::FrBWTIndex()
{
   init() ;
   return ;
}

//----------------------------------------------------------------------

FrBWTIndex::FrBWTIndex(const char *filename, bool memory_mapped,
		       bool touch_memory)
{
   init() ;
   (void)load(filename,memory_mapped,touch_memory) ;
   return ;
}

//----------------------------------------------------------------------

FrBWTIndex::FrBWTIndex(uint32_t *items, size_t num_items,
		       FrBWTEORHandling eod_handling, uint32_t eor,
		       ostream *progress)
{
   init() ;
   makeIndex(items,num_items,eod_handling,eor,progress) ;
   return ;
}

//----------------------------------------------------------------------

FrBWTIndex::~FrBWTIndex()
{
   FrFree(m_filename) ;
   if (m_fp_read) fclose(m_fp_read) ;
   if (m_fp_write) fclose(m_fp_write) ;
   if (m_fmap)
      {
      FrUnmapFile(m_fmap) ;
      m_fmap = 0 ;
      }
   else
      {
      FrFree(m_C) ;
      FrFree(m_items) ;
      FrFree(m_buckets) ;
      FrFree(m_bucket_pool) ;
      }
   if (m_buffereduserdata)
      FrFree(m_userdata) ;
   FrFree(m_class_sizes) ;
   init() ;
   return ;
}

//----------------------------------------------------------------------

bool FrBWTIndex::load(const char *filename, bool memory_mapped,
			bool touch_memory)
{
   setDiscounts(0.0) ;
   if (!filename)
      return false ;
   m_filename = FrDupString(filename) ;
   FILE *fp = fopen(filename,FrFOPEN_READ_MODE) ;
   if (!fp)
      return false ;
   BWTHeader header ;
   if (!parseHeader(fp,filename,header))
      {
      fclose(fp) ;
      return false ;
      }
   m_userdataoffset = header.m_userdata ;
   m_userdatasize = FrFileSize(fp) - m_userdataoffset ;
   if (memory_mapped)
      m_fmap = FrMapFile(filename,FrM_READONLY) ;
   if (m_fmap)
      {
      // point at the appropriate locations in the mapped memory
      assertq(FrMappingSize(m_fmap) >= header.m_userdata) ;
      char *base = (char*)FrMappedAddress(m_fmap) ;
      m_C = base + header.m_C_offset ;
      m_items = (unsigned char *)(base + header.m_FL_offset) ;
      if (header.m_compression == BWT_BYTECOMP)
	 {
	 m_buckets = base + header.m_buckets_offset ;
	 m_bucket_pool = base + header.m_bucketpool_offset ;
	 m_poolsize = header.m_bucketpool_length ;
	 }
      m_userdata = base + header.m_userdata ;
      if (touch_memory)
	 FrTouchMappedMemory(m_fmap) ;
      FrAdviseMemoryUse(m_fmap,FrMADV_RANDOM) ;
      }
   else
      {
      // need to read the data into memory
      size_t bpp = bytesPerPointer() ;
      m_C = FrNewN(char,bpp*(m_numIDs+1)) ;
      if (m_C)
	 {
	 bool success = false ;
	 size_t numbytes = 0 ;
	 if (fread(m_C,bpp,m_numIDs+1,fp) == m_numIDs+1)
	    {
	    if (header.m_compression == BWT_UNCOMP)
	       {
	       numbytes = bpp * m_numitems ;
	       m_items = FrNewN(unsigned char,numbytes) ;
	       if (m_items &&
		   fread(m_items,bpp,m_numitems,fp) == m_numitems)
		  success = true ;
	       }
	    else
	       {
	       numbytes = m_numitems ;
	       m_items = FrNewN(unsigned char,numbytes) ;
	       if (m_items &&
		   fread(m_items,1,numbytes,fp) == numbytes &&
		   loadCompressionData(fp,header))
		  success = true ;
	       }
	    }
	 if (success)
	    {
	    FrAdviseMemoryUse(m_C,bpp*(m_numIDs+1),FrMADV_RANDOM) ;
	    if (numbytes > 0)
	       FrAdviseMemoryUse(m_items,numbytes,FrMADV_RANDOM) ;
	    }
	 else
	    {
	    FrFree(m_C) ;	m_C = 0 ;
	    FrFree(m_items) ; 	m_items = 0 ;
	    }
	 }
      }
   if (!good())
      {
      m_numitems = 0 ;
      m_numIDs = 0 ;
      }
   fclose(fp) ;
   return good() ;
}

//----------------------------------------------------------------------

void FrBWTIndex::makeIndex(uint32_t *items, size_t num_items,
			   FrBWTEORHandling /*eod_handling*/, uint32_t eor,
			   ostream *progress)
{
   if (m_fmap)
      {
      FrUnmapFile(m_fmap) ;
      m_fmap = 0 ;
      if (!m_buffereduserdata)
	 {
	 m_userdata = 0 ;
	 m_userdataoffset = 0 ;
	 }
      }
   else
      {
      FrFree(m_C) ;
      FrFree(m_items) ;
      FrFree(m_buckets) ;
      FrFree(m_bucket_pool) ;
      }
   m_C = 0 ;
   m_items = 0 ;
   m_buckets = 0 ;
   m_bucket_pool = 0 ;
   m_numbuckets = 0 ;
   m_poolsize = 0 ;
   m_EOR = eor ;
   uint32_t *idx = FrNewN(uint32_t,num_items) ;
   if (idx)
      {
      size_t i ;
      FrAdviseMemoryUse(items,bytes_per_ptr * num_items,FrMADV_SEQUENTIAL) ;
      // figure out the size of the m_C array by scanning for the highest
      //   non-EOR value in 'items'
      size_t stored_items = 0 ;
      size_t highest = 0 ;
      for (i = 0 ; i < num_items ; i++)
	 {
	 uint32_t item = items[i] ;
	 if (item < EORvalue())
	    {
	    stored_items++ ;
	    if (item > highest)
	       highest = item ;
	    }
	 }
      m_numIDs = highest + 1 ;
      //
      // set up the m_C array now that we know how big it is
      //
      m_C = FrNewC(char,bytes_per_ptr*(m_numIDs+1)) ;
      FrLocalAllocC(size_t,counts,100000,m_numIDs+1) ;
      if (!m_C|| !counts)
	 {
	 FrNoMemory("building BWT index pointer array") ;
	 FrFree(m_C) ;
	 FrLocalFree(counts) ;
	 FrFree(idx) ;
	 return ;
	 }
      if (progress)
	 (*progress) << ";  " << m_numIDs << " unique IDs in index" << endl ;
      //
      // improve virtual-memory performance by doing one step of a radix
      //   sort, so that the Bentley_Sedgewick_Sort operates on relatively
      //   small sections of the 'idx' array at a time
      // we start by constructing the contents of the C array
      //
      for (i = 0 ; i < num_items ; i++)
	 {
	 uint32_t item = items[i] ;
	 if (item <= highest)
	    counts[item]++ ;
	 }
      size_t count = 0 ;
      for (i = 0 ; i <= highest ; i++)
	 {
	 setC(i,count) ;
	 count += counts[i] ;
	 counts[i] = 0 ;
	 }
      setC(m_numIDs,count) ;
      assert(count == stored_items) ;
#ifdef USE_BINSORT
      if (progress)
	 (*progress) << ";  sorting index" << flush ;
      //
      // next, add each item to the appropriate bin defined by m_C
      //
      FrAdviseMemoryUse(idx,bytes_per_ptr * num_items,FrMADV_RANDOM) ;
      for (i = 0 ; i < num_items ; i++)
	 {
	 uint32_t item = items[i] ;
	 size_t bin = (item < m_numIDs) ? item : m_numIDs ;
	 idx[C(bin)+counts[bin]] = i ;
	 counts[bin]++ ;
	 }
      FrLocalFree(counts) ;
      //
      // finally, sort each bin
      //
      FrAdviseMemoryUse(items,bytes_per_ptr * num_items,FrMADV_RANDOM) ;
      FrAdviseMemoryUse(idx,bytes_per_ptr * num_items,FrMADV_SEQUENTIAL) ;
      size_t prev_chunk = 0 ;
      size_t chunk_bound = CHUNK_SIZE ;
      size_t next_dot = DOT_INTERVAL ;
      for (i = 0 ; i < m_numIDs ; i++)
	 {
	 size_t C_i = C(i) ;
	 size_t C_iplus1 = C(i+1) ;
	 if (progress && C_i >= next_dot)
	    {
	    (*progress) << '.' << flush ;
	    next_dot = (C_i > next_dot + DOT_INTERVAL ? C_i + DOT_INTERVAL
			: next_dot + DOT_INTERVAL) ;
	    }
	 // only need to sort a bin if it contains multiple items
	 if (C_iplus1 > C_i + 1)
	    {
	    if (C_i > chunk_bound)
	       {
	       FrDontNeedMemory(idx + prev_chunk,
				(C_i - prev_chunk) * bytes_per_ptr) ;
	       prev_chunk = C_i ;
	       chunk_bound = C_i + CHUNK_SIZE ;
	       if (C_i + CHUNK_SIZE <= num_items)
		  FrWillNeedMemory(idx + C_i, CHUNK_SIZE * bytes_per_ptr) ;
	       }
	    Bentley_Sedgewick_Sort(idx + C_i, C_iplus1 - C_i,
				   items,num_items,1,EORvalue()) ;
	    }
	 }
      if (progress)
	 (*progress) << '.' << flush ;
      Bentley_Sedgewick_Sort(idx + C(m_numIDs), num_items - C(m_numIDs),
			     items,num_items,0,EORvalue()) ;
      if (progress)
	 (*progress) << "done" << endl ;
#else // !USE_BINSORT
      for (i = 0 ; i < num_items ; i++)
	 idx[i] = i ;
      Bentley_Sedgewick_Sort(idx, num_items, items,num_items,0,EORvalue()) ;
      if (progress)
	 (*progress) << ";  sort complete" << endl ;
#endif /* USE_BINSORT */
      size_t bpp = bytes_per_ptr ;
      FrAdviseMemoryUse(idx,bpp * num_items,FrMADV_SEQUENTIAL) ;
      //
      // convert the sorted array of index pointers into a linked chain of
      //   successive elements that encodes the original word order
      // first step: invert the mapping, preserving EOR markers, so that
      //   each element of 'items' points back at the element of 'idx'
      //   that points at it
      //   By virtue of sorting, we only need to look at the first
      //   'stored_items' elements, since everything beyond that is an
      //   EOR
      for (i = 0 ; i < stored_items ; i++)
	 items[idx[i]] = i ;
      //
      // second step: establish successor links
      FrAdviseMemoryUse(items,bpp * num_items,FrMADV_SEQUENTIAL) ;
      FrAdviseMemoryUse(idx,bpp * num_items,FrMADV_RANDOM) ;
      if (num_items > 0 && items[num_items-1] < num_items)
	 idx[items[num_items-1]] = items[0] ;
      for (i = 1 ; i < num_items ; i++)
	 {
	 size_t pred = items[i-1] ;
	 if (pred < num_items)
	    idx[pred] = items[i] ;
	 }
      if (progress)
	 (*progress) << ";  established successor links" << endl ;
      FrDontNeedMemory(items,bpp * num_items) ;
      //
      // since nothing follows EOR items, we don't need to store them
      m_totalitems = num_items ;
      m_numitems = stored_items ;
      m_items = (unsigned char*)idx ;
      // put the resulting list of successor pointers in the 'idx'
      //   array in standard byte order
      FrAdviseMemoryUse(idx,bpp * stored_items,FrMADV_SEQUENTIAL) ;
      size_t shift = pointerShift() ;
      for (i = 0 ; i < stored_items ; i++)
	 FrStoreLong(idx[i],m_items + (i<<shift)) ;
      // and free up the items which are not being stored
      m_items = (unsigned char*)FrRealloc(idx,sizeof(LONGbuffer)*stored_items);
      if (!m_items)			// just in case....
	 m_items = (unsigned char*)idx ;
      }
   else
      FrNoMemory("building BWT index") ;
   return ;
}

//----------------------------------------------------------------------

void FrBWTIndex::init()
{
   m_discount = 0.0 ;
   m_mass_ratio = 1.0 ;
   m_total_mass = 0.0 ;
   m_filename = 0 ;
   m_fp_read = 0 ;
   m_fp_write = 0 ;
   m_fmap = 0 ;
   m_C = 0 ;
   m_items = 0 ;
   m_buckets = 0 ;
   m_bucket_pool = 0 ;
   m_userdata = 0 ;
   m_totalitems = 0 ;
   m_numitems = 0 ;
   m_numIDs = 0;
   m_userdatasize = 0 ;
   m_userdataoffset = 0 ;
   m_EOR = ~0 ;
   m_discount = DEFAULT_DISCOUNTMASS ;
   m_bucketsize = DEFAULT_BUCKET_SIZE ;
   m_numbuckets = 0 ;
   m_poolsize = 0 ;
   m_maxdelta = 255 - m_bucketsize ;
   m_eor_state = FrBWT_KeepEOR ;
   m_flags = 0 ;
   m_compressed = false ;
   m_buffereduserdata = false ;
   m_class_sizes = 0 ;
   m_num_classes = 0 ;
   setAffixSizes(0) ;
   return ;
}

//----------------------------------------------------------------------

void FrBWTIndex::setDiscounts(double discount)
{
   if (discount < 0.0)
      discount = 0.0 ;
   else if (discount > 0.9999999)
      discount = 0.9999999 ;
   m_discount = discount ;
   m_mass_ratio = 1.0 + discount ;
   m_total_mass = numItems() * m_mass_ratio ;
   return ;
}

//----------------------------------------------------------------------

void FrBWTIndex::wordsAreReversed(bool rev)
{
   m_flags &= ~BWTF_WordsReversed ;
   if (rev)
      m_flags |= BWTF_WordsReversed ;
   return ;
}

//----------------------------------------------------------------------

void FrBWTIndex::wordsAreCaseSensitive(bool cased)
{
   m_flags &= ~BWTF_CaseSensitive ;
   if (cased)
      m_flags |= BWTF_CaseSensitive ;
   return ;
}

//----------------------------------------------------------------------

void FrBWTIndex::indexIsCharBased(bool chbased)
{
   m_flags &= ~BWTF_CharBased ;
   if (chbased)
      m_flags |= BWTF_CharBased ;
   return ;
}

//----------------------------------------------------------------------

void FrBWTIndex::setAffixSizes(uint8_t sizes)
{
   m_affix_sizes = sizes ;
   return ;
}

//----------------------------------------------------------------------

const char *FrBWTIndex::signatureString() const
{
   return SIGNATURE ;
}

//----------------------------------------------------------------------

void FrBWTIndex::setC(size_t idx, size_t value)
{
   if (m_C)
      FrStoreLong(value, m_C + (idx << ptr_shift)) ;
   return ;
}

//----------------------------------------------------------------------

bool FrBWTIndex::parseHeader(BWTHeader &header, const char *filename)
{
   if (header.m_fileformat > FILE_FORMAT)
      {
      FrErrorVA("unsupported file format or corrupted file %s",filename) ;
      return false ;
      }
   else if (header.m_fileformat < FILE_FORMAT)
      {
      FrErrorVA("obsolete file format %d in file %s",
		header.m_fileformat,filename) ;
      return false ;
      }
   if (header.m_compression > BWT_HIGHESTCOMP)
      {
      FrErrorVA("unknown compression method %2.02x in file %s",
		header.m_compression,filename) ;
      return false ;
      }
   m_compressed = (header.m_compression > 0) ;
   m_numitems = header.m_FL_length ;
   m_totalitems = header.m_FL_total ;
   m_numIDs = header.m_C_length ;
   m_EOR = header.m_EOR ;
   m_bucketsize = header.m_bucketsize ;
   m_maxdelta = header.m_maxdelta ;
   m_numbuckets = (m_numitems + m_bucketsize - 1) / m_bucketsize ;
   m_eor_state = (FrBWTEORHandling)header.m_eor_handling ;
   m_flags = header.m_flags ;
   m_affix_sizes = (uint8_t)header.m_affix_sizes ;
   setDiscounts(header.m_discount) ;
   return true ;
}

//----------------------------------------------------------------------

bool FrBWTIndex::parseHeader(FILE *fp, const char *filename,
			       BWTHeader &header)
{
   if (!Fr_read_BWT_header(fp,&header,signatureString()))
      {
      FrErrorVA("invalid or corrupted file %s",filename) ;
      return false ;
      }
   return parseHeader(header,filename) ;
}

//----------------------------------------------------------------------

bool FrBWTIndex::merge(FrBWTIndex *other)
{
   (void)other;
//!!!
   return false ;
}

//----------------------------------------------------------------------

bool FrBWTIndex::save(const char *filename,
			bool (*user_write_fn)(FILE *,void *),
			void *user_data)
{
   if (filename && *filename)
      {
      char *savefile = FrForceFilenameExt(filename,"tmp") ;
      if (!savefile)
	 {
	 FrNoMemory("while preparing to save BWTIndex") ;
	 return false ;
	 }
      errno = 0 ;
      FILE *fp = fopen(savefile,FrFOPEN_WRITE_MODE) ;
      if (!fp)
	 {
	 FrWarningVA("unable to write to %s (errno=%d)",savefile,errno) ;
	 return false ;
	 }
      size_t bpp = bytesPerPointer() ;
      BWTHeader header ;
      header.m_fileformat = FILE_FORMAT ;
      header.m_compression = (m_compressed ? BWT_BYTECOMP : BWT_UNCOMP) ;
      header.m_eor_handling = (int)m_eor_state ;
      header.m_flags = m_flags ;
      header.m_C_offset = BWT_HEADER_SIZE ;
      header.m_C_length = m_numIDs ;
      header.m_FL_offset = header.m_C_offset + bpp*(m_numIDs+1) ;
      header.m_FL_length = numItems() ;
      header.m_FL_total = totalItems() ;
      header.m_EOR = EORvalue() ;
      header.m_discount = m_discount ;
      header.m_bucketsize = m_bucketsize ;
      if (m_compressed)
	 {
	 header.m_buckets_offset = header.m_FL_offset + numItems() ;
	 header.m_bucketpool_offset = (header.m_buckets_offset +
				       bpp * m_numbuckets) ;
	 }
      else
	 {
	 header.m_buckets_offset = (header.m_FL_offset + bpp * numItems()) ;
	 header.m_bucketpool_offset = header.m_buckets_offset ;
	 }
      header.m_bucketpool_length = m_poolsize ;
      header.m_maxdelta = m_maxdelta ;
      header.m_affix_sizes = getAffixSizes() ;
      header.m_userdata = (header.m_bucketpool_offset + m_poolsize * bpp) ;
      bool success = false ;
      if (Fr_write_BWT_header(fp,&header,signatureString()))
	 {
fflush(fp) ;
	 off_t items_size = (m_compressed
			      ? numItems() : numItems() * (off_t)bpp) ;
	 if (!m_C)
	    m_C = FrNewC(char,bpp) ;
	 if (Fr_fwrite(m_C,bpp*(m_numIDs+1),fp) &&
	     (!m_items || Fr_fwrite(m_items,items_size,fp)))
	    {
	    if (!m_compressed ||
		(Fr_fwrite(m_buckets,bpp*m_numbuckets,fp) &&
		 Fr_fwrite(m_bucket_pool,bpp*m_poolsize,fp))
	       )
	       {
	       if (!user_write_fn ||
		   user_write_fn(fp,user_data))
		  success = true ;
	       }
	    }
	 }
      fclose(fp) ;
      if (success)
	 success = FrSafelyReplaceFile(savefile,filename,
				       "BWTIndex save file") ;
      else
	 Fr_unlink(savefile) ;
      FrFree(savefile) ;
      return success ;
      }
   return false ;
}

//----------------------------------------------------------------------

bool FrBWTIndex::verifySignature(const char *filename) const
{
   if (!filename || !*filename)
      return false ;
   FILE *fp = fopen(filename,FrFOPEN_READ_MODE) ;
   if (!fp)
      return false ;
   bool valid = false ;
   char buf[MAX_SIGNATURE] ;
   if (fgets(buf,sizeof(buf),fp))
      {
      if (strncmp(buf,signatureString(),sizeof(buf)) == 0)
	 valid = true ;
      }
   fclose(fp) ;
   return valid ;
}

//----------------------------------------------------------------------

bool FrBWTIndex::setEOR(size_t newEOR)
{
   if (newEOR > m_numIDs)
      {
      m_EOR = newEOR ;
      return true ;
      }
   else
      return false ;
}

//----------------------------------------------------------------------

uint32_t FrBWTIndex::getAbsPointer(size_t N, size_t idx) const
{
   assertq(m_buckets != 0 && m_bucket_pool != 0) ;
   size_t bucket = N / m_bucketsize ;
   size_t bucket_offset = FrLoadLong(m_buckets + (bucket << ptr_shift)) ;
   return FrLoadLong(m_bucket_pool + ((bucket_offset + idx) << ptr_shift)) ;
}

//----------------------------------------------------------------------

uint32_t FrBWTIndex::getCompressedSuccessor(size_t N) const
{
   unsigned int byte = m_items[N] ;
   if (byte > m_maxdelta)
      return getAbsPointer(N, byte - m_maxdelta - 1) ;
   else if (byte != COMPRESSED_EOR)
      {
      // value is difference from previous entry, so scan for an entry
      //   that resolves to an absolute value, keeping track of the
      //   cumulative difference
      register size_t diff = byte ;
      unsigned int maxdelta = m_maxdelta ;
      while (N > 0)
	 {
	 byte = m_items[--N] ;
	 if (byte > maxdelta)
	    {
	    // grab entry from secondary table
	    return getAbsPointer(N, byte - maxdelta - 1) + diff ;
	    }
	 else if (byte == COMPRESSED_EOR)
	    return EORvalue() + diff ;
	 else
	    diff += byte ;
	 }
      return diff ;
      }
   else
      return EORvalue() ;
}

//----------------------------------------------------------------------

uint32_t FrBWTIndex::getCompressedSuccessor(size_t N, size_t byte,
					    uint32_t left_succ) const
{
   if (byte > m_maxdelta)
      return getAbsPointer(N, byte - m_maxdelta - 1) ;
   else if (byte == COMPRESSED_EOR)
      return EORvalue() ;
   else // encoded value is difference from previous entry
      return left_succ + byte ;
}

//----------------------------------------------------------------------

uint32_t FrBWTIndex::getNthSuccessor(size_t idx, size_t N) const
{
   if (N > 0)
      {
      size_t max = numItems() ;
      if (m_compressed)
	 {
	 do {
	    idx = getCompressedSuccessor(idx) ;
	    } while (idx < max && --N > 0) ;
	 }
      else
	 {
	 do {
	    idx = getUncompSuccessor(idx) ;
	    } while (idx < max && --N > 0) ;
	 }
      }
   return idx ;
}

//----------------------------------------------------------------------

bool FrBWTIndex::wordsAreReversed() const
{
   return (m_flags & BWTF_WordsReversed) != 0 ;
}

//----------------------------------------------------------------------

bool FrBWTIndex::wordsAreCaseSensitive() const
{
   return (m_flags & BWTF_CaseSensitive) != 0 ;
}

//----------------------------------------------------------------------

bool FrBWTIndex::indexIsCharBased() const
{
   return (m_flags & BWTF_CharBased) != 0 ;
}

//----------------------------------------------------------------------

uint8_t FrBWTIndex::getAffixSizes() const
{
   return m_affix_sizes ;
}

//----------------------------------------------------------------------

bool FrBWTIndex::passesSanityChecks() const
{
   // first occurrence of ID 0 should be first element of FL, and element
   //   after last occurrence of last ID should be just past end of FL
   if (!m_C || firstLocation(0) != 0 || firstLocation(m_numIDs) != numItems())
      return false ;
   // check for correct locations of tables in the file
   if (m_fmap)
      {
      if (!m_C || !m_items || m_C > (char*)m_items || m_C > m_userdata ||
	  (char*)m_items > m_userdata)
	 return false ;
      if (m_compressed)
	 {
	 if (!m_items || (char*)m_items > m_buckets ||
	     (char*)m_items > m_bucket_pool || (char*)m_items > m_userdata)
	    return false ;
	 if (!m_buckets || m_buckets > m_bucket_pool || m_buckets > m_userdata)
	    return false ;
	 if (!m_bucket_pool || m_bucket_pool > m_userdata)
	    return false ;
	 }
      }
   else if (m_fp_read)
      {
      fseek(m_fp_read,0L,SEEK_SET) ;
      BWTHeader header ;
      Fr_read_BWT_header(m_fp_read,&header,signatureString()) ;
      if (header.m_C_offset == 0 || header.m_C_offset > header.m_FL_offset ||
	  header.m_C_offset > header.m_userdata ||
	  header.m_FL_offset > header.m_userdata)
	 return false ;
      if (header.m_compression == BWT_BYTECOMP)
	 {
	 if (header.m_C_offset > header.m_buckets_offset ||
	     header.m_C_offset > header.m_bucketpool_offset ||
	     header.m_C_offset > header.m_userdata)
	    return false ;
	 if (header.m_FL_offset == 0 ||
	     header.m_FL_offset > header.m_buckets_offset ||
	     header.m_FL_offset > header.m_bucketpool_offset ||
	     header.m_FL_offset > header.m_userdata)
	    return false ;
	 if (header.m_buckets_offset == 0 ||
	     header.m_buckets_offset > header.m_bucketpool_offset ||
	     header.m_buckets_offset > header.m_userdata)
	    return false ;
	 if (header.m_bucketpool_offset == 0 ||
	     header.m_bucketpool_offset > header.m_userdata)
	    return false ;
	 }
      }
   // whew, we've passed all consistency checks
   return true ;
}

//----------------------------------------------------------------------

FrBWTLocation FrBWTIndex::unigram(uint32_t id) const
{
   if (id >= EORvalue())
      {
      FrBWTLocation range(C(m_numIDs),totalItems()) ;
      return range ;
      }
   else if (id < m_numIDs)
      {
      FrBWTLocation range(C(id),C(id+1)) ;
      return range ;
      }
   else
      {
      FrBWTLocation range(~0,0) ;	// invalid ID, so null range
      return range ;
      }
}

//----------------------------------------------------------------------

size_t FrBWTIndex::unigramFrequency(uint32_t id) const
{
   if (id < m_numIDs)
      return C(id+1) - C(id) ;
   else if (id >= EORvalue() && id < EORvalue() + totalItems() - numItems())
      return m_eor_state == FrBWT_KeepEOR ? 1 : (totalItems() - numItems()) ;
   else
      return 0 ;			// invalid ID, so null range
}

//----------------------------------------------------------------------

FrBWTLocation FrBWTIndex::bigram(uint32_t id1, uint32_t id2) const
{
   FrBWTLocation range2 = unigram(id1) ;
   if (range2.nonEmpty())
      return extendMatch(range2,unigram(id2)) ;
   else
      {
      FrBWTLocation range(~0,0) ;
      return range ;
      }
}

//----------------------------------------------------------------------

size_t FrBWTIndex::bigramFrequency(uint32_t id1, uint32_t id2) const
{
   FrBWTLocation range(bigram(id1,id2)) ;
   return range.rangeSize() ;
}

//----------------------------------------------------------------------

FrBWTLocation FrBWTIndex::trigram(uint32_t id1, uint32_t id2,
				  uint32_t id3) const
{
   FrBWTLocation range2(bigram(id1,id2)) ;
   if (range2.nonEmpty())
      return extendMatch(range2,unigram(id3)) ;
   else
      {
      FrBWTLocation range(~0,0) ;
      return range ;
      }
}

//----------------------------------------------------------------------

size_t FrBWTIndex::trigramFrequency(uint32_t id1, uint32_t id2,
				    uint32_t id3) const
{
   FrBWTLocation range(trigram(id1,id2,id3)) ;
   return range.rangeSize() ;
}

//----------------------------------------------------------------------

FrBWTLocation FrBWTIndex::lookup(const uint32_t *searchkey,
				 size_t keylength) const
{
   FrBWTLocation loc(~0,0) ;
   if (keylength == 0)
      return loc ;
   loc = unigram(searchkey[0]) ;
   for (size_t i = 1 ; i < keylength && loc.nonEmpty() ; i++)
      {
      loc = extendMatch(loc,searchkey[i]) ;
      }
   return loc ;
}

//----------------------------------------------------------------------

FrBWTLocation FrBWTIndex::revLookup(const uint32_t *searchkey,
				    size_t keylength) const
{
   FrBWTLocation loc(~0,0) ;
   if (keylength == 0)
      return loc ;
   loc = unigram(searchkey[keylength-1]) ;
   for (size_t i = 1 ; i < keylength && loc.nonEmpty() ; i++)
      {
      loc = extendMatch(loc,searchkey[keylength-i-1]) ;
      }
   return loc ;
}

//----------------------------------------------------------------------

size_t FrBWTIndex::frequency(const uint32_t *searchkey,size_t keylength) const
{
   return lookup(searchkey,keylength).rangeSize() ;
}

//----------------------------------------------------------------------

size_t FrBWTIndex::frequency(const FrNGramHistory *history,size_t histlen,
			     uint32_t next_ID) const
{
   if (histlen == 0)
      return 0 ;
   else if (histlen == 1)
      return unigram(next_ID).rangeSize() ;
   if (wordsAreReversed())
      {
      FrBWTLocation loc(history[histlen-1].startLoc(),
			history[histlen-1].endLoc()) ;
      if (loc.nonEmpty())
	 loc = extendMatch(loc,next_ID) ;
      return loc.rangeSize() ;
      }
   else
      {
      FrBWTLocation loc(unigram(history[histlen-1].wordID())) ;
      for (size_t i = 2 ; i < histlen - 1 && loc.nonEmpty() ; i++)
	 {
	 loc = extendMatch(loc,history[histlen-i].wordID()) ;
	 }
      if (loc.nonEmpty())
	 loc = extendMatch(loc,next_ID) ;
      return loc.rangeSize() ;
      }
}

//----------------------------------------------------------------------

size_t FrBWTIndex::revFrequency(const uint32_t *searchkey,
				size_t keylength) const
{
   return revLookup(searchkey,keylength).rangeSize() ;
}

//----------------------------------------------------------------------

FrBWTLocation FrBWTIndex::extendMatch(const FrBWTLocation match,
				      const FrBWTLocation next) const
{
   uint32_t first = match.first() ;
   uint32_t last = match.last() ;
   if (first >= numItems() && last <= totalItems())
      last = ~0 ;
   size_t hi = next.pastEnd() ;
   size_t lo = next.first() ;
   if (hi > numItems())
      hi = numItems() ;
   size_t hi2 = hi ;
   size_t matchrange = last - first + 1 ;
   size_t start ;
   if (!m_compressed)
      {
      // simple binary search within the range of entries for 'nextID' to find
      //   those that point within the range in 'match'
      while (hi > lo)
	 {
	 size_t mid = (hi + lo) / 2 ;
	 uint32_t item = getUncompSuccessor(mid) ;
	 if (item < first)
	    lo = mid + 1 ;
	 else // if (item >= first)
	    {
	    hi = mid ;
	    if (mid < hi2 && item > last)
	       hi2 = mid ;
	    }
	 }
      hi = hi2 ;
      start = lo ;
      if (hi > lo + matchrange)		// new range can't be larger than
	 hi = lo + matchrange ;		//   existing match's range!
      while (hi > lo)
	 {
	 size_t mid = (hi + lo) / 2 ;
	 uint32_t item = getUncompSuccessor(mid) ;
	 if (item <= last)
	    lo = mid + 1 ;
	 else // if (item > last)
	    hi = mid ;
	 }
      }
   else
      {
      // search within the range of entries for 'nextID' to find those that
      //   point within the range in 'match', taking into account that a
      //   random lookup of a successor pointer is generally much more
      //   expensive than a sequential access
      // (for now, though, mostly use the same code as for the uncomp case)
      size_t i ;
      size_t bsize = m_bucketsize / 2 ;
      while (hi > lo + bsize)
	 {
	 size_t mid = (hi + lo) / 2 ;
	 uint32_t item = getCompressedSuccessor(mid) ;
	 if (item < first)
	    lo = mid + 1 ;
	 else // if (item >= first)
	    {
	    hi = mid ;
	    if (mid < hi2 && item > last)
	       hi2 = mid ;
	    }
	 }
      if (hi > lo)
	 {
	 uint32_t succ = getCompressedSuccessor(lo) ;
	 if (succ < first)
	    {
	    for (i = lo+1 ; i < hi ; i++)
	       {
	       succ = getCompressedSuccessor(i,succ) ;
	       if (succ >= first)
		  break ;
	       }
	    lo = i ;
	    }
	 }
      start = lo ;
      hi = hi2 ;
      if (hi > lo + matchrange)		// new range can't be larger than
	 hi = lo + matchrange ;		//   existing match's range!
      while (hi > lo + bsize)
	 {
	 size_t mid = (hi + lo) / 2 ;
	 uint32_t item = getCompressedSuccessor(mid) ;
	 if (item <= last)
	    lo = mid + 1 ;
	 else // if (item > last)
	    hi = mid ;
	 }
      if (hi > lo)
	 {
	 uint32_t succ = getCompressedSuccessor(lo) ;
	 if (succ <= last)
	    {
	    for (i = lo+1 ; i < hi ; i++)
	       {
	       succ = getCompressedSuccessor(i,succ) ;
	       if (succ > last)
		  break ;
	       }
	    hi = i ;
	    }
	 else
	    hi = lo ;
	 }
      }
   FrBWTLocation range(start,hi) ;
   return range ;
}

//----------------------------------------------------------------------

FrBWTLocation FrBWTIndex::extendMatch(const FrBWTLocation match,
				      const FrBWTLocation next,
				      size_t next_words) const
{
   if (next_words == 1)
      return extendMatch(match,next) ;
   else if (next_words == 0)
      return match ;
   uint32_t first = match.first() ;
   uint32_t last = match.last() ;
   if (first >= numItems() && last <= totalItems())
      last = ~0 ;
   size_t hi = next.pastEnd() ;
   size_t lo = next.first() ;
   if (hi > numItems())
      hi = numItems() ;
   size_t hi2 = hi ;
   size_t matchrange = last - first + 1 ;
   size_t start ;
   // simple binary search within the range of entries for 'nextID' to find
   //   those that point within the range in 'match'
   while (hi > lo)
      {
      size_t mid = (hi + lo) / 2 ;
      uint32_t item = getNthSuccessor(mid,next_words) ;
      if (item < first)
	 lo = mid + 1 ;
      else // if (item >= first)
	 {
	 hi = mid ;
	 if (mid < hi2 && item > last)
	    hi2 = mid ;
	 }
      }
   hi = hi2 ;
   start = lo ;
   if (hi > lo + matchrange)		// new range can't be larger than
      hi = lo + matchrange ;		//   existing match's range!
   while (hi > lo)
      {
      size_t mid = (hi + lo) / 2 ;
      uint32_t item = getNthSuccessor(mid,next_words) ;
      if (item <= last)
	 lo = mid + 1 ;
      else // if (item > last)
	 hi = mid ;
      }
   FrBWTLocation range(start,hi) ;
   return range ;
}

//----------------------------------------------------------------------

FrBWTLocation FrBWTIndex::extendMatch(const FrBWTLocation match,
				      uint32_t next_ID) const
{
   if (match.isEmpty())
      return match ;
   return extendMatch(match,unigram(next_ID)) ;
}

//----------------------------------------------------------------------

FrBWTLocation FrBWTIndex::extendRight(FrBWTLocation match, size_t ngram_length,
				      uint32_t next_ID)
   const
{
   if (match.rangeSize() == 0)
      return match ;

   FrBWTLocation uni(unigram(next_ID)) ;
   if (ngram_length == 0 || uni.rangeSize() == 0)
      return uni ;
   size_t start = getNthSuccessor(match.first(),ngram_length-1) ;
   size_t end = getNthSuccessor(match.last(),ngram_length-1) ;
   assertq(end - start - 1 == match.rangeSize()) ;
   size_t lo = start ;
   size_t hi = end ;
   size_t newstart ;
   if (!m_compressed)
      {
      // simple binary search within the range of entries for 'next_ID' to find
      //   those that point within the range in 'match'
      while (hi > lo)
	 {
	 size_t mid = (hi + lo) / 2 ;
	 uint32_t item = getUncompSuccessor(mid) ;
	 if (item < uni.first())
	    lo = mid + 1 ;
	 else // if (item >= uni.first())
	    hi = mid ;
	 }
      newstart = lo ;
      hi = end ;
      if (hi - lo > uni.rangeSize())	// new range can't be larger than
	 hi = lo + uni.rangeSize() ;	//   existing match's range!
      while (hi > lo)
	 {
	 size_t mid = (hi + lo) / 2 ;
	 uint32_t item = getUncompSuccessor(mid) ;
	 if (item <= uni.last())
	    lo = mid + 1 ;
	 else // if (item > uni.last())
	    hi = mid ;
	 }
      }
   else
      {
      // search within the range of entries for 'next_ID' to find those that
      //   point within the range in 'match', taking into account that a
      //   random lookup of a successor pointer is generally much more
      //   expensive than a sequential access
      // (for now, though, mostly use the same code as for the uncomp case)
      size_t i ;
      size_t hi2 = hi ;
      size_t bsize = m_bucketsize / 2 ;
      uint32_t first = uni.first() ;
      while (hi > lo + bsize)
	 {
	 size_t mid = (hi + lo) / 2 ;
	 uint32_t item = getCompressedSuccessor(mid) ;
	 if (item < first)
	    lo = mid + 1 ;
	 else // if (item >= uni.first())
	    hi = mid ;
	 if (hi2 > mid && item >= uni.pastEnd())
	    hi2 = mid ;
	 }
      if (hi > lo)
	 {
	 uint32_t prev = getCompressedSuccessor(lo) ;
	 for (i = lo+1 ; i < hi ; i++)
	    {
	    uint32_t succ = getCompressedSuccessor(i,prev) ;
	    if (succ < first)
	       prev = succ ;
	    else
	       {
	       lo = i ;
	       break ;
	       }
	    }
	 }
      newstart = lo ;
      hi = hi2 ;
      if (hi - lo > uni.rangeSize())	// new range can't be larger than
	 hi = lo + uni.rangeSize() ;	//   existing match's range!
      uint32_t pastend = uni.pastEnd() ;
      while (hi > lo + bsize)
	 {
	 size_t mid = (hi + lo) / 2 ;
	 uint32_t item = getCompressedSuccessor(mid) ;
	 if (item < pastend)
	    lo = mid + 1 ;
	 else // if (item >= pastend)
	    hi = mid ;
	 }
      if (hi > lo)
	 {
	 uint32_t succ = getCompressedSuccessor(lo) ;
	 if (succ < pastend)
	    {
	    for (i = lo+1 ; i < hi ; i++)
	       {
	       succ = getCompressedSuccessor(i,succ) ;
	       if (succ >= pastend)
		  break ;
	       }
	    hi = i ;
	    }
	 else
	    hi = lo ;
	 }
      }
   FrBWTLocation range(match.first() + (newstart - start),
		       match.pastEnd() - (end - hi)) ;
   return range ;
}

//----------------------------------------------------------------------

uint32_t FrBWTIndex::getID(size_t location) const
{
   if (location >= numItems())
      {
      if (m_eor_state == FrBWT_KeepEOR)
	 {
	 if (location >= EORvalue())
	    return location - EORvalue() ;
	 else
	    return location - numItems() ;
	 }
      return (uint32_t)FrBWT_ENDOFDATA ;
      }
   // binary search on m_C
   size_t lo = 0 ;
   size_t hi = numIDs() + 1 ;
   while (hi > lo)
      {
      size_t mid = (hi + lo) / 2 ;
      uint32_t item = C(mid) ;
      if (item <= location)
	 lo = mid + 1 ;
      else // if (item > location)
	 hi = mid ;
      }
   if (lo > numIDs())
      return (uint32_t)FrBWT_ENDOFDATA ;
   else if (lo > 0 && C(lo) > location)
      return lo - 1 ;
   else
      return lo ;
}

//----------------------------------------------------------------------

uint32_t FrBWTIndex::nextID(size_t location, size_t *next_location) const
{
   size_t succ = getSuccessor(location) ;
   if (succ >= EORvalue())
      return (uint32_t)~0 ;
   if (next_location)
      *next_location = succ ;
   return getID(succ) ;
}

//----------------------------------------------------------------------

uint32_t FrBWTIndex::recordNumber(size_t loc, size_t *offset) const
{
   size_t max = numItems() ;
   size_t steps = 0 ;
   if (m_compressed)
      {
      while (loc < max)
	 {
	 steps++ ;
	 loc = getCompressedSuccessor(loc) ;
	 }
      }
   else
      {
      while (loc < max)
	 {
	 steps++ ;
	 loc = getUncompSuccessor(loc) ;
	 }
      }
   if (offset)
      *offset = steps ;
   if (loc >= EORvalue())
      return loc - EORvalue() ;
   else if (loc >= numItems() && m_eor_state == FrBWT_KeepEOR)
      return loc - numItems() ;
   return ~0 ;
}

//----------------------------------------------------------------------

uint32_t FrBWTIndex::recordNumber(FrBWTLocation loc, size_t *offset) const
{
   return recordNumber(loc.first(),offset) ;
}

//----------------------------------------------------------------------

size_t FrBWTIndex::iterateNGrams(size_t currN, size_t maxN,
				 size_t startloc, size_t endloc,
				 bool include_EOR,
				 uint32_t *IDs, size_t minfreq,
				 FrBWTNGramIterFunc *fn, va_list args) const
{
   if (startloc >= endloc)
      return 0 ;
   if (currN >= maxN)
      {
      if (fn)
	 {
	 size_t freq = endloc - startloc ;
	 if (freq >= minfreq)
	    {
	    FrSafeVAList(args) ;
	    (void)fn(IDs,maxN,freq,this,FrSafeVarArgs(args)) ;
	    FrSafeVAListEnd(args) ;
	    }
	 }
      return 1 ;
      }
   size_t count = 0 ;
   if (minfreq < 1)			// don't output zero-count ngrams
      minfreq = 1 ;			// (which can occur with classes)
   if (currN == 0)
      {
      if (maxN > numItems())
	 maxN = numItems() ;
      for (size_t i = 0 ; i < numIDs() ; i++)
	 {
	 IDs[0] = i ;
	 size_t range_lo = C(i) ;
	 if (range_lo >= endloc)
	    break ;
	 size_t range_hi = C(i+1) ;
	 if (range_hi <= startloc)
	    continue ;
	 if (range_lo < startloc)
	    range_lo = startloc ;
	 if (range_hi > endloc)
	    range_hi = endloc ;
	 if (range_hi - range_lo >= minfreq)
	    count += iterateNGrams(1,maxN,range_lo,range_hi,include_EOR,
				   IDs,minfreq,fn,args) ;
	 }
      return count ;
      }
   size_t lo = startloc ;
   uint32_t succ = getNthSuccessor(lo,currN) ;
   if (succ >= numItems())
      {
      if (include_EOR && currN+1 == maxN)
	 {
	 IDs[currN] = (uint32_t)FrBWT_ENDOFDATA ;
	 count += iterateNGrams(maxN,maxN,startloc,endloc,include_EOR,
				IDs,minfreq,fn,args) ;
	 }
      return count ;
      }
   size_t currID = getID(succ) ;
   size_t boundary = C(currID+1) ;
   IDs[currN] = currID ;
   for (size_t i = lo + 1 ; i < endloc ; i++)
      {
      succ = getNthSuccessor(i,currN) ;
      if (succ >= boundary)
	 {
	 // not yet at maximum length, so recurse if frequent enough
	 if (i - lo >= minfreq)
	    count += iterateNGrams(currN+1,maxN,lo,i,include_EOR,
				   IDs,minfreq,fn,args) ;
	 if (succ >= numItems())
	    return count ;
	 currID = getID(succ) ;
	 if (currID >= numIDs())
	    return count ;
	 lo = i ;
	 boundary = C(currID+1) ;
	 IDs[currN] = currID ;
	 }
      }
   if (endloc - lo >= minfreq)
      count += iterateNGrams(currN+1,maxN,lo,endloc,include_EOR,
			     IDs,minfreq,fn,args) ;
   return count ;
}

//----------------------------------------------------------------------

size_t FrBWTIndex::iterateNGramsVA(size_t currN, size_t maxN,
				   size_t startloc, size_t endloc,
				   bool include_EOR,
				   uint32_t *IDs, size_t minfreq,
				   FrBWTNGramIterFunc *fn, ...) const
{
   va_list args ;
   va_start(args,fn) ;
   size_t count = iterateNGrams(currN,maxN,startloc,endloc,include_EOR,
				IDs,minfreq,fn,args) ;
   va_end(args) ;
   return count ;
}

//----------------------------------------------------------------------

size_t FrBWTIndex::countNGrams(size_t N) const
{
   if (N == 0)
      return 0 ;
   else if (N == 1)
      return numIDs() ;
   else
      {
      FrLocalAlloc(uint32_t,IDs,256,N+1) ;
      size_t count = iterateNGramsVA(0,N,0,numItems(),false,IDs,1,0) ;
      FrLocalFree(IDs) ;
      return count ;
      }
}

//----------------------------------------------------------------------

size_t FrBWTIndex::countNGrams(size_t N, size_t startloc, size_t endloc) const
{
   if (N == 0 || endloc <= startloc)
      return 0 ;
   else if (N == 1)
      return endloc - startloc + 1 ;
   else
      {
      FrLocalAlloc(uint32_t,IDs,256,N+1) ;
      size_t count = iterateNGramsVA(0,N,startloc,endloc,false,IDs,1,0) ;
      FrLocalFree(IDs) ;
      return count ;
      }
}

//----------------------------------------------------------------------

size_t FrBWTIndex::countNGrams(size_t N, FrBWTLocation range) const
{
   if (N == 0 || range.isEmpty())
      return 0 ;
   else if (N == 1)
      return range.rangeSize() ;
   else
      {
      FrLocalAlloc(uint32_t,IDs,256,N+1) ;
      size_t count = iterateNGramsVA(0,N,range.first(),range.last(),false,IDs,
				     1,0) ;
      FrLocalFree(IDs) ;
      return count ;
      }
}

//----------------------------------------------------------------------

static bool print_ngrams(uint32_t *IDs, size_t N, size_t freq,
			   const FrBWTIndex *, va_list args)
{
   FrVarArg(ostream *,out) ;
   FrVarArg(const char **,names) ;
   FrVarArg(size_t,num_names) ;
   FrVarArg(int,print_freq) ;
   assertq(out != 0 && names != 0) ;
   if (print_freq)
      (*out) << freq << '\t' ;
   for (size_t i = 0 ; i < N ; i++)
      {
      const char *name = 0 ;
      if (IDs[i] < num_names)
	 name = names[IDs[i]] ;
      if (!name)
	 name = "(null)" ;
      (*out) << name ;
      if (i + 1 < N)
	 (*out) << ' ' ;
      }
   (*out) << endl ;
   return true ;			// continue iterating
}

//----------------------------------------------------------------------

size_t FrBWTIndex::printNGrams(size_t N, ostream &out,
			       const char **names, size_t num_names,
			       size_t min_freq, bool print_freq) const
{
   if (N == 0 || !names)
      return 0 ;
   else // if (N >= 1)
      {
      FrLocalAlloc(uint32_t,IDs,256,N+1) ;
      size_t count = iterateNGramsVA(0,N,0,numItems(),false,IDs,min_freq,
				     print_ngrams, &out,names,num_names,
				     print_freq) ;
      FrLocalFree(IDs) ;
      return count ;
      }
}

//----------------------------------------------------------------------

size_t FrBWTIndex::printNGrams(FrBWTLocation range, size_t N, ostream &out,
			       const char **names, size_t num_names,
			       size_t min_freq, bool print_freq) const
{
   if (N == 0 || !names)
      return 0 ;
   else // if (N >= 1)
      {
      FrLocalAlloc(uint32_t,IDs,256,N+1) ;
      size_t count = iterateNGramsVA(0,N,range.first(),range.pastEnd(),false,
				     IDs, min_freq, print_ngrams, &out,names,
				     num_names, print_freq) ;
      FrLocalFree(IDs) ;
      return count ;
      }
}

//----------------------------------------------------------------------

size_t FrBWTIndex::enumerateNGrams(size_t N,size_t minfreq, bool include_EOR,
				   FrBWTNGramIterFunc fn, va_list args) const
{
   FrLocalAlloc(uint32_t,IDs,256,N+1) ;
   size_t count = iterateNGrams(0,N,0,numItems(),include_EOR,IDs,minfreq,
				fn,args) ;
   FrLocalFree(IDs) ;
   return count ;
}

//----------------------------------------------------------------------

size_t FrBWTIndex::enumerateNGrams(size_t N,size_t minfreq, bool include_EOR,
				   FrBWTNGramIterFunc fn, ...) const
{
   va_list args ;
   va_start(args,fn) ;
   FrLocalAlloc(uint32_t,IDs,256,N+1) ;
   size_t count = iterateNGrams(0,N,0,numItems(),include_EOR,IDs,minfreq,
				fn,args) ;
   FrLocalFree(IDs) ;
   va_end(args) ;
   return count ;
}

//----------------------------------------------------------------------

bool FrBWTIndex::deltaHistogram(size_t maxdiff, size_t *diffcounts) const
{
   if (!diffcounts || maxdiff < 2)
      return false ;
   if (m_compressed)
      return false ;			// not implemented
   size_t i ;
   for (i = 0 ; i <= maxdiff ; i++)
      diffcounts[i] = 0 ;
   uint32_t prev_ptr = ~0 ;
   for (i = 0 ; i < numItems() ; i++)
      {
      uint32_t ptr = getSuccessor(i) ;
      if (ptr == EORvalue() ||
	  (ptr > EORvalue() && m_eor_state == FrBWT_MergeEOR))
	 diffcounts[0]++ ;
      else if (ptr < prev_ptr)
	 diffcounts[maxdiff]++ ;
      else
	 {
	 size_t diff = ptr - prev_ptr ;
	 if (diff > maxdiff)
	    diff = maxdiff ;
	 diffcounts[diff]++ ;
	 }
      prev_ptr = ptr ;
      }
   return true ;
}

//----------------------------------------------------------------------

bool FrBWTIndex::compressionStats(size_t &uncomp, size_t &comp) const
{
   uncomp = BWT_HEADER_SIZE + (m_numIDs + 1 + numItems()) * bytes_per_ptr ;
   if (m_compressed)
      {
      comp = BWT_HEADER_SIZE + (m_numIDs+1) * bytes_per_ptr + numItems() +
	 (m_numbuckets + m_poolsize) * bytes_per_ptr ;
      }
   else
      comp = uncomp ;
   return true ;
}

//----------------------------------------------------------------------

bool FrBWTIndex::loadCompressionData(FILE *fp, const BWTHeader &header)
{
   if (header.m_compression != BWT_BYTECOMP)
      return false ;
   m_numbuckets = ((header.m_FL_length + header.m_bucketsize - 1) /
		   header.m_bucketsize) ;
   m_poolsize = header.m_bucketpool_length ;
   m_buckets = FrNewN(char,bytes_per_ptr*m_numbuckets) ;
   m_bucket_pool = FrNewN(char,bytes_per_ptr*m_poolsize) ;
   m_items = FrNewN(unsigned char,numItems()) ;
   bool success = false ;
   if (m_buckets && m_bucket_pool && header.m_buckets_offset > 0 &&
      header.m_bucketpool_offset > 0)
      {
      success = true ;
      fseek(fp,0L,SEEK_CUR) ;		// workaround for ftell() problems
      fpos_t position ;
      (void)fgetpos(fp,&position) ;	// remember where we are in file
      fseek(fp,header.m_buckets_offset,SEEK_SET) ;
      if (fread(m_buckets,bytes_per_ptr,m_numbuckets,fp) < m_numbuckets)
	 success = false ;
      fseek(fp,header.m_bucketpool_offset,SEEK_SET) ;
      if (success &&
	  fread(m_bucket_pool,bytes_per_ptr,m_poolsize,fp) < m_poolsize)
	 success = false ;
      fsetpos(fp,&position) ;		// restore original position in file
      }
   if (!success)
      {
      FrFree(m_buckets) ; m_buckets = 0;
      FrFree(m_bucket_pool) ; m_bucket_pool = 0 ;
      }
   return success ;
}

//----------------------------------------------------------------------

bool FrBWTIndex::loadUserData()
{
   if (!m_userdata)
      {
      if (!m_fp_read && m_filename)
	 m_fp_read = fopen(m_filename,FrFOPEN_READ_MODE) ;
      if (!m_fp_read)
	 return false ;
      m_userdata = FrNewN(char,m_userdatasize) ;
      if (!m_userdata)
	 {
	 FrNoMemoryVA("loading user data from BWT file %s",m_filename) ;
	 return false ;
	 }
      if (fread(m_userdata,sizeof(char),m_userdatasize,m_fp_read) <
	  m_userdatasize)
	 {
	 FrFree(m_userdata) ;
	 m_userdata = 0 ;
	 return false ;
	 }
      m_buffereduserdata = true ;
      }
   return true ;			// successfully loaded
}

//----------------------------------------------------------------------

bool FrBWTIndex::unmmapUserData()
{
   if (!m_buffereduserdata)
      {
      m_userdata = 0 ;
      return loadUserData() ;
      }
   return true ;
}

//----------------------------------------------------------------------

int FrBWTIndex::readUserData(size_t offset, char *buffer, size_t bufsize)
{
   if (offset < m_userdatasize)
      {
      if (m_userdata)
	 {
	 if (offset + bufsize > m_userdatasize)
	    bufsize = m_userdatasize - offset ;
	 memcpy(buffer,m_userdata+offset,bufsize) ;
	 return bufsize ;
	 }
      else if (m_fp_read)
	 {
	 fseek(m_fp_read,m_userdataoffset + offset,SEEK_SET) ;
	 return fread(buffer,sizeof(char),bufsize,m_fp_read) ;
	 }
      }
   // if we get to this point, we were unable to read any user data
   return -1 ;
}

//----------------------------------------------------------------------

// end of file frbwt.cpp //
