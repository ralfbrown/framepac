/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC								*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frbwtcmp.cpp	    compress/decompress BWT n-gram index	*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 2003,2004,2005,2006,2007,2009,2012			*/
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

#include <errno.h>
#include "frbwt.h"
#include "frassert.h"
#include "frfilutl.h"
#include "frmmap.h"
#include "fr_bwt.h"

#ifdef FrSTRICT_CPLUSPLUS
#  include <cstring>
#else
#  include <string.h>	// for GCC 2.x, strcmp()
#endif

/************************************************************************/
/*	Manifest Constants						*/
/************************************************************************/

#define COMPRESS_BUFSIZE 	(16*BUFSIZ)

/************************************************************************/
/*	Types								*/
/************************************************************************/

class CompressionBuffers
{
   public:
      char bytebuf[COMPRESS_BUFSIZE] ;
      char bucketbuf[COMPRESS_BUFSIZE] ;
      char poolbuf[COMPRESS_BUFSIZE] ;
      size_t bytecount ;
      size_t bytebufcount ;
      size_t bucketbytes ;
      size_t bucketcount ;
      size_t poolbytes ;
      size_t poolcount ;
      size_t poolindex ;
      size_t bucketnum ;
      size_t overflow ;
      size_t maxdiff ;
      size_t maxsecondary ;
      size_t bucketsize ;
      size_t abs_pointers ;
      size_t comp_EORs ;
      uint32_t firstEOR ;
      bool mergeEOR ;
   public:
      CompressionBuffers(size_t bucket_size) ;
      ~CompressionBuffers() ;

      void setupEOR(bool merge, uint32_t first)
	    { mergeEOR = merge ; firstEOR = first ;
      maxsecondary = merge ? 0x0E : 0x0F ; }

      void setBucketStart()
	    { FrStoreLong(poolindex,bucketbuf+bucketbytes) ;
      bucketbytes += FrBWTIndex::bytesPerPointer() ;
      abs_pointers = comp_EORs = 0 ; }
      void nextBucket()
	    { bucketnum++ ; overflow = 0 ; setBucketStart() ; }
      bool dumpByteBuf(FILE *fp, size_t startoffset, bool force = false) ;
      bool dumpBuckets(FILE *fp, size_t startoffset, bool force = false) ;

      void addPoolItem(uint32_t value)
	    { FrStoreLong(value,poolbuf+poolbytes) ; poolindex++ ;
      poolbytes += FrBWTIndex::bytesPerPointer() ; }
      bool dumpPool(FILE *fp, size_t startoffset, bool force = false) ;
      size_t poolSize() const
	    { return poolcount * COMPRESS_BUFSIZE + poolbytes ; }

      size_t encode(size_t loc, size_t prev, size_t curr) ;
} ;

/************************************************************************/
/*	Helper functions						*/
/************************************************************************/

//----------------------------------------------------------------------

static bool copy_bytes(FILE *infp, FILE *outfp, size_t numbytes = ~0)
{
   while (!feof(infp))
      {
      char buffer[COMPRESS_BUFSIZE] ;
      size_t to_copy = sizeof(buffer) ;
      if (numbytes < to_copy)
	 to_copy = numbytes ;
      size_t count = fread(buffer,sizeof(char),to_copy,infp) ;
      if (count > 0)
	 {
	 if (!Fr_fwrite(buffer,sizeof(char),count,outfp))
	    return false ;
	 }
      }
   return true ;
}

//----------------------------------------------------------------------

static bool dump_buffers(FILE *fp, CompressionBuffers &comp,
			   size_t bytearrayoffset, size_t bucketoffset,
			   size_t pooloffset, bool force = false)
{
   if (fp)
      {
      if (!comp.dumpBuckets(fp,bucketoffset,force) ||
	  !comp.dumpPool(fp,pooloffset,force) ||
	  !comp.dumpByteBuf(fp,bytearrayoffset,force))
	 return false ;
      }
   return true ;
}

/************************************************************************/
/*	Methods for class CompressionBuffers				*/
/************************************************************************/

CompressionBuffers::CompressionBuffers(size_t bucket_size)
{
   bucketsize = bucket_size ;
   maxdiff = 255 - bucket_size ;
   bytecount = bytebufcount = 0 ;
   bucketbytes = bucketcount = 0 ;
   poolbytes = poolcount = poolindex = 0 ;
   bucketnum = overflow = 0 ;
   setBucketStart() ;
   setupEOR(false,0x80000000UL) ;
   return ;
}

//----------------------------------------------------------------------

CompressionBuffers::~CompressionBuffers()
{
   return ;
}

//----------------------------------------------------------------------

bool CompressionBuffers::dumpByteBuf(FILE *fp, size_t byteoffset,
				       bool force)
{
   if (force || bytecount >= COMPRESS_BUFSIZE)
      {
      off_t position = byteoffset + bytebufcount * COMPRESS_BUFSIZE ;
      fseek(fp,position,SEEK_SET) ;
      if (!Fr_fwrite(bytebuf,sizeof(char),bytecount,fp))
	 return false ;
      bytecount = 0 ;
      bytebufcount++ ;
      }
   return true ;
}

//----------------------------------------------------------------------

bool CompressionBuffers::dumpBuckets(FILE *fp, size_t bucketoffset,
				       bool force)
{
   if (force || bucketbytes >= COMPRESS_BUFSIZE)
      {
      off_t position = bucketoffset + bucketcount * COMPRESS_BUFSIZE ;
      fseek(fp,position,SEEK_SET) ;
      if (!Fr_fwrite(bucketbuf,sizeof(char),bucketbytes,fp))
	 return false ;
      bucketbytes = 0 ;
      bucketcount++ ;
      }
   return true ;
}

//----------------------------------------------------------------------

bool CompressionBuffers::dumpPool(FILE *fp, size_t pooloffset,
				    bool force)
{
   if (force || poolbytes >= COMPRESS_BUFSIZE)
      {
      long position = pooloffset + poolcount * COMPRESS_BUFSIZE ;
      fseek(fp,position,SEEK_SET) ;
      if (!Fr_fwrite(poolbuf,sizeof(char),poolbytes,fp))
	 return false ;
      poolbytes = 0 ;
      poolcount++ ;
      }
   return true ;
}

//----------------------------------------------------------------------

size_t CompressionBuffers::encode(size_t loc, size_t prev, size_t curr)
{
   size_t byte ;
   if (curr == firstEOR || (curr > firstEOR && mergeEOR))
      {
      comp_EORs++ ;
      byte = COMPRESSED_EOR ;
      }
   else if (curr <= prev || curr > prev + maxdiff ||
	    ((loc+1)%bucketsize == 0 && (abs_pointers + comp_EORs == 0)))
      {
      // need to store the actual successor in bucket
      byte = maxdiff + (++overflow) ;
      abs_pointers++ ;
      addPoolItem(curr) ;
      }
   else
      byte = curr - prev ;
   bytebuf[bytecount++] = (unsigned char)byte ;
   return byte ;
}

/************************************************************************/
/*	Methods for class FrBWTIndex					*/
/************************************************************************/

//----------------------------------------------------------------------

bool FrBWTIndex::compress()
{
   if (m_compressed)
      return true ;
   m_bucketsize = DEFAULT_BUCKET_SIZE ;
   m_maxdelta = 255 - m_bucketsize ;
   m_numbuckets = (numItems() + bucketsize() - 1) / bucketsize() ;

   // figure out how big the pool of absolute pointers will be
   uint32_t prev_succ = ~0 ;
   size_t abs_pointers = 0 ;
   size_t comp_EORs = 0 ;
   m_poolsize = 0 ;
   FrAdviseMemoryUse(m_items,bytesPerPointer()*numItems(),FrMADV_SEQUENTIAL) ;
   for (size_t i = 0 ; i < numItems() ; i++)
      {
      if ((i % m_bucketsize) == 0)
	 {
	 abs_pointers = 0 ;
	 comp_EORs = 0 ;
	 }
      uint32_t succ = getUncompSuccessor(i) ;
      if (succ == m_EOR || (succ > m_EOR && m_eor_state == FrBWT_MergeEOR))
	 comp_EORs++ ; // will be stored without using an absolute pointer
      else if (succ <= prev_succ ||
	       succ - prev_succ > m_maxdelta ||
	       ((i+1)%m_bucketsize == 0 && (abs_pointers + comp_EORs == 0)))
	 {      // above enforces at least one absolute pointer per bucket
	 abs_pointers++ ;
	 m_poolsize++ ;
	 }
      prev_succ = succ ;
      }

   size_t bpp = bytesPerPointer() ;
   // now that we know how big the pool is, check whether we will actually
   //   save any space by compressing
   if ((m_poolsize + m_numbuckets) * bpp + numItems() >= numItems() * bpp)
      return false ;			// can't (usefully) compress

   // allocate the various buffers for the compressed data
   m_buckets = FrNewN(char,bpp * m_numbuckets) ;
   unsigned char *comp_items = FrNewN(unsigned char,numItems()) ;
   m_bucket_pool = FrNewN(char,bpp * m_poolsize) ;
   if (comp_items && m_buckets && m_bucket_pool)
      {
      size_t bucket = 0 ;
      size_t ptr_count = 0 ;
      size_t ptr_index = 0 ;
      prev_succ = ~0 ;
      for (size_t i = 0 ; i < numItems() ; i++)
	 {
	 if ((i % m_bucketsize) == 0)
	    {
	    FrStoreLong(ptr_count,m_buckets + bpp * bucket++) ;
	    ptr_index = 0 ;
	    comp_EORs = 0 ;
	    }
	 if ((i % CHUNK_SIZE) == 0 && i > 0)
	    {
	    // let OS know we're done with another chunk of m_items
	    FrDontNeedMemory(m_items + bpp*(i-CHUNK_SIZE), bpp*CHUNK_SIZE,
			     (i > CHUNK_SIZE)) ;
	    // and tell it to prefetch the next chunk
	    FrWillNeedMemory(m_items + bpp*i, bpp*CHUNK_SIZE) ;
	    }
	 uint32_t succ = getUncompSuccessor(i) ;
	 if (succ == m_EOR ||
	     (succ > m_EOR && m_eor_state == FrBWT_MergeEOR))
	    {
	    comp_items[i] = COMPRESSED_EOR ;
	    comp_EORs++ ;
	    }
	 else if (succ <= prev_succ ||
		  succ - prev_succ > m_maxdelta ||
		  ((i+1)%m_bucketsize == 0 && (ptr_index + comp_EORs == 0)))
	    // (above ensures at least one abs.ptr per bucket)
	    {
	    FrStoreLong(succ,m_bucket_pool + bpp * ptr_count++);
	    comp_items[i] = (unsigned char)(m_maxdelta + (++ptr_index)) ;
	    }
	 else
	    comp_items[i] = (unsigned char)(succ - prev_succ) ;
	 prev_succ = succ ;
	 }
      assertq(ptr_count == m_poolsize) ;
      if (!m_fmap)
	 FrFree(m_items) ;
      m_items = comp_items ;
      m_compressed = true ;
      return true ;
      }
   else	// memory alloc failed
      {
      FrWarning("out of memory while compressing index, "
		"will remain uncompressed") ;
      FrFree(comp_items)  ;
      FrFree(m_buckets) ;	m_buckets = 0 ;
      FrFree(m_bucket_pool) ;	m_bucket_pool = 0 ;
      m_numbuckets = 0 ;
      m_poolsize = 0 ;
      return false ;
      }
}

//----------------------------------------------------------------------

bool FrBWTIndex::uncompress()
{
   size_t bpp = bytesPerPointer() ;
   unsigned char *new_items = FrNewN(unsigned char, bpp * numItems()) ;
   if (new_items)
      {
      uint32_t succ = 0 ;
      for (size_t i = 0 ; i < numItems() ; i++)
	 {
	 succ = getCompressedSuccessor(i,succ) ;
	 FrStoreLong(succ,new_items + bpp * i) ;
	 }
      FrFree(m_items) ;
      m_items = new_items ;
      FrFree(m_buckets) ;
      m_buckets = 0 ;
      FrFree(m_bucket_pool) ;
      m_bucket_pool = 0 ;
      m_compressed = false ;
      m_numbuckets = 0 ;
      m_poolsize = 0 ;
      return false ;
      }
   else
      {
      FrNoMemory("while uncompressing FrBWTIndex") ;
      return false ;
      }
}

//----------------------------------------------------------------------

bool FrBWTIndex::compress(const char *infile, const char *outfile) const
{
   if (infile && *infile && outfile && *outfile &&
       strcmp(infile,outfile) != 0)
      {
      FILE *infp = fopen(infile,FrFOPEN_READ_MODE) ;
      if (!infp)
	 return false ;

      // step 1: copy the header to the output file, updating
      //   compression-parameter fields
      BWTHeader header ;
      Fr_read_BWT_header(infp,&header,signatureString()) ;
      if (header.m_compression != BWT_UNCOMP)
	 {
	 fclose(infp) ;
	 errno = EINVAL ;
	 return false ;			// already compressed
	 }
      FILE *outfp = fopen(outfile,FrFOPEN_WRITE_MODE) ;
      if (!outfp)
	 {
	 int e = errno ;
	 fclose(infp) ;
	 errno = e ;
	 return false ;
	 }

      // (for now, we'll just hard-code some reasonable values for the params)
      header.m_bucketsize = DEFAULT_BUCKET_SIZE ;
      header.m_maxdelta = 255 - header.m_bucketsize ;

      // while we're at it, put in the pre-determined locations of the
      //   bucket pointers and bucket pool
      size_t numbuckets = ((header.m_FL_length + header.m_bucketsize - 1) /
			   header.m_bucketsize) ;
      header.m_buckets_offset = (header.m_FL_offset + header.m_FL_length) ;
      header.m_bucketpool_offset = (header.m_buckets_offset +
				    numbuckets * bytesPerPointer()) ;
      Fr_write_BWT_header(outfp,&header,signatureString()) ;

      // copy m_C to the output file
      copy_bytes(infp,outfp,
		 (header.m_C_length + 1) * FrBWTIndex::bytesPerPointer()) ;

      // step 2: convert m_items values to differences and encode in bytes,
      //   writing the results to the output file
      CompressionBuffers compbuffers(header.m_bucketsize) ;
      compbuffers.setupEOR(header.m_eor_handling != 0,header.m_EOR) ;
      size_t prev = 0 ;
      bool success = true ;
      for (size_t i = 0 ; i < header.m_FL_length ; i++)
	 {
	 size_t succ ;
	 if (!Fr_read_long(infp,succ))
	    {
	    success = false ;
	    break ;
	    }
	 (void)compbuffers.encode(i,prev,succ) ;
	 if (!dump_buffers(outfp, compbuffers, header.m_FL_offset,
			   header.m_buckets_offset, header.m_bucketpool_offset))
	    {
	    success = false ;
	    break ;
	    }
	 if (((i+1) % header.m_bucketsize) == 0)
	    compbuffers.nextBucket() ;
	 prev = succ ;
	 }
      success = dump_buffers(outfp,compbuffers,header.m_buckets_offset,
			     header.m_bucketpool_offset,true) ;
      header.m_bucketpool_length = compbuffers.poolSize() / bytesPerPointer() ;

      // step 3: update header pointers
      fseek(infp,0L,SEEK_END) ;
      size_t udata_size = ftell(infp) - header.m_userdata ;
      fseek(infp,header.m_userdata,SEEK_SET) ;
      header.m_userdata = (size_t)ftell(outfp) ;
      Fr_write_BWT_header(outfp,&header,signatureString()) ;

      // step 4: copy any user data from the uncompressed file into new file
      copy_bytes(infp,outfp,udata_size) ;
      fclose(infp) ;
      fclose(outfp) ;
      return success ;
      }
   return false ;
}

//----------------------------------------------------------------------

bool FrBWTIndex::uncompress(const char *infile, const char *outfile) const
{
   if (infile && *infile && outfile && *outfile &&
       strcmp(infile,outfile) != 0)
      {
      FILE *infp = fopen(infile,FrFOPEN_READ_MODE) ;
      if (!infp)
	 return false ;
      bool success = false ;

      // step 1: copy the header to the output file
      BWTHeader header ;
      Fr_read_BWT_header(infp,&header,signatureString()) ;
      if (header.m_compression == BWT_UNCOMP)
	 {
	 fclose(infp) ;
	 errno = EINVAL ;
	 return false ;			// already uncompressed
	 }
      FILE *outfp = fopen(outfile,FrFOPEN_WRITE_MODE) ;
      if (!outfp)
	 {
	 int e = errno ;
	 fclose(infp) ;
	 errno = e ;
	 return false ;
	 }

      // copy m_C to the output file
      copy_bytes(infp,outfp,
		 (header.m_C_length + 1) * FrBWTIndex::bytesPerPointer()) ;

      // step 2: decode each byte and write the resulting pointers to the
      //   output file
      FrBWTIndex compr ;
      compr.parseHeader(header,infile) ;
      compr.loadCompressionData(infp,header) ;
      uint32_t succ = 0 ;
      for (size_t i = 0 ; i < header.m_FL_length ; i++)
	 {
	 int byte = fgetc(infp) ;
	 succ = compr.getCompressedSuccessor(i,byte,succ) ;
	 Fr_write_long(outfp,succ) ;
	 }
      // step 3: update header pointers
      header.m_compression = BWT_UNCOMP ;
      header.m_bucketsize = 0 ;
      header.m_buckets_offset = header.m_bucketpool_offset = 0 ;
      header.m_bucketpool_length = 0;
      header.m_maxdelta = 0 ;
      fseek(infp,0L,SEEK_END) ;
      size_t udata_size = ftell(infp) - header.m_userdata ;
      fseek(infp,header.m_userdata,SEEK_SET) ;
      header.m_userdata = (size_t)ftell(outfp) ;
      Fr_write_BWT_header(outfp,&header,signatureString()) ;

      // step 4: copy any user data from the uncompressed file into new file
      copy_bytes(infp,outfp,udata_size) ;
      fclose(infp) ;
      fclose(outfp) ;
      return success ;
      }
   return false ;
}

// end of file frbwtcmp.cpp //
