/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC								*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frvocab.cpp	    String-to-ID mapping for vocabularies	*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 2004,2005,2006,2008,2009,2010,2012			*/
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
#   pragma implementation "frvocab.h"
#endif

#include "frassert.h"
#include "frvocab.h"
#include "frobject.h"
#include "frutil.h"

#ifdef FrSTRICT_CPLUSPLUS
# include <cstdlib>
# include <cstring>
#else
# include <stdlib.h>
# include <string.h>
#endif

/************************************************************************/
/*	Manifest Constants						*/
/************************************************************************/

/* COMMENT:

header format:
	ASCIZ	signature string (default "FrVocabulary")
   		padded to 32 bytes with \0
	BYTE	format version (currently 1)
	BYTE	character encoding
	BYTE	# bytes (3,4) used to store word-name offsets ("O" below)
	BYTE	# bytes (3,4) used to store IDs (0=offsets are IDs) ("I" below)
	BYTE	word-offset shift count (0..4) ("S" below)
   	BYTE	nonzero if reverse mapping is stored in file ("R" below)
        2 BYTEs	reserved (0)
	LONG	number of words ("N" below)
	LONG	number of bytes in word-name list ("W" below)
	LONG	bytes of user data per word ("U" below)
	12 BYTEs reserved for future expansion (0)
	Array[N]:   (sorted lexically by word name) ("E" below)
		O BYTEs	offset of name for this word
		I BYTEs unique ID for this word
	W BYTEs	ASCIZ word names; each NUL-padded to multiple of 1<<S bytes
	Array[N]:   (sorted by ID, only present if "R" is nonzero)
	  I BYTEs  index of entry in array "E" for word with ID 'i'
	Array[N]:   (sorted by ID)
	  U BYTEs  user data for word with ID 'i'
end COMMENT */

#define OFFSET_SIGNATURE	0
#define SIZE_SIGNATURE		32
#define OFFSET_FORMATVER	SIZE_SIGNATURE
#define OFFSET_CHARENC		33
#define OFFSET_WORDLOCSIZE	34
#define OFFSET_IDSIZE		35
#define OFFSET_SHIFTCOUNT	36
#define OFFSET_HAVEREVINDEX	37
#define OFFSET_RESERVED1	38
#define OFFSET_NUMWORDS		40
#define OFFSET_WORDLISTBYTES	44
#define OFFSET_UDATASIZE	48
#define OFFSET_RESERVED2	52
#define OFFSET_ENTRYARRAY	64

#define HEADER_SIZE   OFFSET_ENTRYARRAY

/************************************************************************/
/************************************************************************/

FrVocabulary::FrVocabulary(const char *filename, off_t start_offset,
			   bool read_only, bool allow_memory_mapping)
{
   init(filename,0) ;
   m_readonly = read_only ;
   m_fileoffset = start_offset ;
   if (load(allow_memory_mapping))
      m_good = true ;
   return ;
}

//----------------------------------------------------------------------

FrVocabulary::FrVocabulary(FILE *fp, const char *filename, off_t start_offset,
			   bool read_only)
{
   init(filename,0) ;
   m_fp = fp ;
   m_readonly = (read_only || m_filename == 0) ;
   fseek(fp,start_offset,SEEK_SET) ;
   m_good = load(fp) ;
   return ;
}

//----------------------------------------------------------------------

FrVocabulary::FrVocabulary(FrFileMapping *fmap, const char *filename,
			   off_t start_offset, bool read_only)
{
   init(filename,0) ;
   m_readonly = (read_only || m_filename == 0) ;
   m_mmap = fmap ;
   m_contained = true ;
   m_mapped = true ;
   m_changed = false ;
   m_fileoffset = start_offset ;
   m_vocab = (char*)FrMappedAddress(m_mmap) + start_offset ;
   m_good = extractParameters(m_vocab) ;
   setDataPointers(m_vocab) ;
   return ;
}

//----------------------------------------------------------------------

FrVocabulary::FrVocabulary(const char *buffer, void * /*container*/)
{
   init(0,0) ;
   m_readonly = true ;
   m_good = extractParameters(buffer) ;
   setDataPointers((char*)buffer) ;
   return ;
}

//----------------------------------------------------------------------

void FrVocabulary::init(const char *filename, size_t userdatasize)
{
   m_filename = FrDupString(filename) ;
   m_fp = 0 ;
   m_fileoffset = 0 ;
   m_vocab = 0 ;
   m_mmap = 0 ;
   m_index = 0 ;
   m_revindex = 0 ;
   m_userdata = 0 ;
   m_words = 0 ;
   m_scalefactor = 0 ;
   m_padmask = 0 ;
   m_userdata_size = userdatasize ;
   m_index_size = 0 ;
   m_index_alloc = 0 ;
   m_words_size = 0 ;
   m_words_alloc = 0 ;
   useNameAsID(false) ;
   m_readonly = false ;
   m_changed = false ;
   m_mapped = false ;
   m_revmapped = false ;
   m_contained = false ;
   m_batch_update = false ;
   m_good = false ;
   return ;
}

//----------------------------------------------------------------------

FrVocabulary::~FrVocabulary()
{
   if (m_mapped)
      {
      if (!m_contained)
	 FrUnmapFile(m_mmap) ;
      m_mmap = 0 ;
      m_vocab = 0 ;
      }
   else
      {
      FrFree(m_index) ;		m_index = 0 ;
      FrFree(m_words) ;		m_words = 0 ;
      FrFree(m_userdata) ;	m_userdata = 0 ;
      }
   if (!m_revmapped)
      {
      FrFree(m_revindex) ;	m_revindex = 0 ;
      }
   freeReverseMapping() ;
   if (m_fp)
      fclose(m_fp) ;
   FrFree(m_filename) ;
   return ;
}

//----------------------------------------------------------------------

const char *FrVocabulary::signatureString() const
{
   return "FrVocabulary" ;
}

//----------------------------------------------------------------------

bool FrVocabulary::load(bool allow_memory_mapping)
{
   unload() ;
   if (m_filename && *m_filename)
      {
      m_fp = fopen(m_filename,FrFOPEN_READ_MODE) ;
      if (m_fp)
	 {
	 if (allow_memory_mapping)
	    {
	    m_mmap = FrMapFile(m_filename,FrM_READONLY) ;
	    if (m_mmap)
	       {
	       m_mapped = true ;
	       m_changed = false ;
	       m_vocab = (char*)FrMappedAddress(m_mmap) ;
	       m_good = extractParameters(m_vocab) ;
	       setDataPointers(m_vocab) ;
	       }
	    }
	 if (!m_mapped)
	    // we weren't able to memory-map the file, so read it into memory
	    return load(m_fp) ;
	 }
      }
   return false ;
}

//----------------------------------------------------------------------

bool FrVocabulary::load(FILE *fp)
{
   if (fp)
      {
      FrLocalAlloc(char,header,HEADER_SIZE,HEADER_SIZE) ;
      if (!header)
	 {
	 FrNoMemory("loading FrVocabulary") ;
	 return false ;
	 }
      if (fread(header,1,HEADER_SIZE,fp) < HEADER_SIZE)
	 {
	 FrWarningVA("error loading vocabulary") ;
	 FrLocalFree(header) ;
	 return false ;
	 }
      m_good = extractParameters(header) ;
      FrLocalFree(header) ;
      if (m_good)
	 {
	 m_index = FrNewN(char,m_index_size * m_indexent_size) ;
	 m_words = FrNewN(char,m_words_size) ;
	 m_revindex = (m_revmapped
		       ? FrNewN(char,m_index_size * m_wordloc_size) : 0) ;
	 m_userdata = FrNewN(char,m_index_size * m_userdata_size) ;
	 if (!m_index || !m_words || !m_userdata ||
	     (m_revmapped && !m_revindex) ||
	     fread(m_index,m_indexent_size,m_index_size,fp) < m_index_size ||
	     fread(m_words,1,m_words_size,fp) < m_words_size ||
	     (m_revmapped &&
	      fread(m_revindex,m_wordloc_size,m_index_size,fp)
	      < m_index_size) ||
	     (m_userdata_size > 0 &&
	      fread(m_userdata,m_userdata_size,m_index_size,fp)
	      < m_index_size))
	    {
	    m_good = false ;
	    m_words_alloc = 0 ;
	    m_index_alloc = 0 ;
	    FrFree(m_index) ;		m_index = 0 ;
	    FrFree(m_words) ;		m_words = 0 ;
	    FrFree(m_revindex) ;	m_revindex = 0 ;
	    FrFree(m_userdata) ;	m_userdata = 0 ;
	    }
	 else
	    {
	    m_words_alloc = m_words_size ;
	    m_index_alloc = m_index_size ;
	    }
	 }
      }
   return false ;
}

//----------------------------------------------------------------------

bool FrVocabulary::extractParameters(const char *header)
{
   if (strcmp(header + OFFSET_SIGNATURE,signatureString()) != 0)
      return false ;
   if (header[OFFSET_FORMATVER] != (char)1)
      {
      return false ;
      }
//header[OFFSET_CHARENC];
   size_t val = header[OFFSET_WORDLOCSIZE] ;
   if (val != 3 && val != 4)
      return false ;
   m_wordloc_size = val ;
   val = header[OFFSET_IDSIZE] ;
   if (val != 0 && val != 3 && val != 4)
      return false ;
   m_ID_size = val ;
   val = header[OFFSET_SHIFTCOUNT] ;
   if (val > 4)
      return false ;
   m_scalefactor = val ;
   m_padmask = (1 << val) - 1 ;  //!!! only correct for 2s-complement machines
   m_revmapped = (header[OFFSET_HAVEREVINDEX] != 0) ;
   m_index_size = FrLoadLong(header + OFFSET_NUMWORDS) ;
   m_words_size = FrLoadLong(header + OFFSET_WORDLISTBYTES) ;
   m_userdata_size = FrLoadLong(header + OFFSET_UDATASIZE) ;
   m_indexent_size = m_wordloc_size + m_ID_size ;
   m_name_is_ID = (m_ID_size == 0) ;
   return true ;
}

//----------------------------------------------------------------------

void FrVocabulary::setDataPointers(char *buffer)
{
   if (m_good)
      {
      m_index = buffer + HEADER_SIZE ;
      m_words = m_index + m_index_size * m_indexent_size ;
      if (m_revmapped)
	 {
	 m_revindex = m_words + m_words_size ;
	 m_userdata = (m_userdata_size
		       ? m_revindex + m_index_size * m_wordloc_size
		       : 0) ;
	 }
      else
	 {
	 m_revindex = 0 ;
	 m_userdata = (m_userdata_size ? m_words+m_words_size : 0);
	 }
      }
   return ;
}

//----------------------------------------------------------------------

bool FrVocabulary::createFileHeader(char *&header)
{
   char *hdr = FrNewC(char,HEADER_SIZE) ;
   if (!hdr)
      return false ;
   const char *sig = signatureString() ;
   strncpy(hdr+OFFSET_SIGNATURE,sig,SIZE_SIGNATURE) ;
   hdr[OFFSET_SIGNATURE+SIZE_SIGNATURE-1] = '\0' ;
   hdr[OFFSET_FORMATVER] = (char)1 ;
   hdr[OFFSET_CHARENC] = (char)FrChEnc_Latin1 ; //!!!
   hdr[OFFSET_WORDLOCSIZE] = (char)m_wordloc_size ;
   hdr[OFFSET_IDSIZE] = (char)m_ID_size ;
   hdr[OFFSET_SHIFTCOUNT] = (char)m_scalefactor ;
   hdr[OFFSET_HAVEREVINDEX] = (m_revindex != 0) ;
   FrStoreLong(numWords(),hdr+OFFSET_NUMWORDS) ;
   FrStoreLong(m_words_size,hdr+OFFSET_WORDLISTBYTES) ;
   FrStoreLong(m_userdata_size,hdr+OFFSET_UDATASIZE) ;
   header = hdr ;
   return true ;
}

//----------------------------------------------------------------------

bool FrVocabulary::createReverseMapping()
{
   if (!m_revindex && !usingNameAsID())
      {
      m_revindex = FrNewN(char,4*numWords()) ;
      if (m_revindex)
	 {
	 for (size_t i = 0 ; i < numWords() ; i++)
	    {
	    const char *entry = indexEntry(i) ;
	    if (entry)
	       {
	       size_t ID = getID(entry) ;
	       if (ID < numWords())
		  FrStoreLong(i,m_revindex + 4*ID) ;
	       }
	    }
	 m_revmapped = false ;
	 }
      else
	 FrNoMemory("while creating reverse mapping for vocabulary") ;
      }
   return (m_revindex != 0) ;
}

//----------------------------------------------------------------------

void FrVocabulary::freeReverseMapping()
{
   if (m_revindex)
      {
      if (!m_revmapped)
	 FrFree(m_revindex) ;
      else
	 m_revmapped = false ;
      m_revindex = 0 ;
      }
   return ;
}

//----------------------------------------------------------------------

bool FrVocabulary::save(const char *filename, off_t file_offset)
{
   FILE *fp = fopen(filename,FrFOPEN_WRITE_MODE) ;
   bool success = false ;
   if (fp)
      {
      success = save(fp,file_offset) ;
      fclose(fp) ;
      }
   return success ;
}

//----------------------------------------------------------------------

bool FrVocabulary::save(FILE *fp, off_t file_offset)
{
   if (!m_index && m_index_size == 0)
      m_index = FrNewN(char,m_indexent_size * m_index_size) ;
   if (!m_words && m_words_size == 0)
      m_words = FrNewN(char,m_words_size) ;
   if (fp && m_index && m_words)
      {
      char *header ;
      if (!createFileHeader(header))
	 return false ;
      fflush(fp) ;
      fseek(fp,file_offset,SEEK_SET) ;
      bool success = false ;
      if (Fr_fwrite(header,1,HEADER_SIZE,fp) &&
	  Fr_fwrite(m_index,m_indexent_size,numWords(),fp) &&
	  Fr_fwrite(m_words,1,m_words_size,fp) &&
	  (!m_revindex ||
	   (Fr_fwrite(m_revindex,m_wordloc_size,numWords(),fp))) &&
	  (m_userdata_size == 0 || m_userdata == 0 ||
	   (Fr_fwrite(m_userdata,m_userdata_size,numWords(),fp)))
	 )
	 {
	 success = true ;
 	 }
      FrFree(header) ;
      return success ;
      }
   return false ;
}

//----------------------------------------------------------------------

bool FrVocabulary::save(off_t file_offset, bool force)
{
   if (m_changed || force)
      return save(m_fp,file_offset) ;
   return true ;			// trivially successful
}

//----------------------------------------------------------------------

bool FrVocabulary::unmemmap()
{
   if (m_mapped)
      {
      unload(true) ;
      load(m_fp) ;
      }
   return true ;
}

//----------------------------------------------------------------------

bool FrVocabulary::unload(bool keep_open)
{
   if (m_mmap)
      {
      if (!m_contained)
	 FrUnmapFile(m_mmap) ;
      m_mmap = 0 ;
      m_vocab = 0 ;
      }
   else
      {
      FrFree(m_index) ;
      FrFree(m_revindex) ;
      FrFree(m_userdata) ;
      FrFree(m_words) ;
      }
   m_index = 0 ;
   m_revindex = 0 ;
   m_userdata = 0 ;
   m_words = 0 ;
   m_good = false ;
   m_mapped = false ;
   if (m_fp && !keep_open)
      {
      fclose(m_fp) ;
      m_fp = 0 ;
      }
   return true ;
}

//----------------------------------------------------------------------

size_t FrVocabulary::getID(const char *indexent) const
{
   if (indexent)
      {
      if (usingNameAsID())
	 {
	 if (m_wordloc_size == 3)
	    return FrLoadThreebyte(indexent) ;
	 else
	    return FrLoadLong(indexent) ;
	 }
      else
	 {
	 if (m_ID_size == 3)
	    return FrLoadThreebyte(indexent + m_wordloc_size) ;
	 else
	    return FrLoadLong(indexent + m_wordloc_size) ;
	 }
      }
   return FrVOCAB_WORD_NOT_FOUND ;
}

//----------------------------------------------------------------------

const char *FrVocabulary::getNameFromIndexEntry(const char *entry) const
{
   if (m_wordloc_size == 3)
      return m_words + (FrLoadThreebyte(entry) << m_scalefactor) ;
   else
      return m_words + (FrLoadLong(entry) << m_scalefactor) ;
}

//----------------------------------------------------------------------

const char *FrVocabulary::getNameFromIndexEntry(size_t N) const
{
   if (N < numWords())
      return getNameFromIndexEntry(indexEntry(N)) ;
   else
      return 0 ;
}

//----------------------------------------------------------------------

void FrVocabulary::setIndexEntry(size_t N, size_t wordloc, size_t ID)
{
   char *entry = indexEntry(N) ;
   if (m_wordloc_size == 3)
      FrStoreThreebyte(wordloc,entry) ;
   else
      FrStoreLong(wordloc,entry) ;
   if (m_ID_size == 3)
      FrStoreThreebyte(ID,entry + m_wordloc_size) ;
   else if (m_ID_size > 3)
      FrStoreLong(ID,entry + m_wordloc_size) ;
   return ;
}

//----------------------------------------------------------------------

bool FrVocabulary::expandTo(size_t new_alloc)
{
   if (new_alloc <= m_index_alloc)
      return true ;
   unmemmap() ;
   char *new_index = FrNewR(char,m_index,new_alloc*m_indexent_size) ;
   char *new_userdata = (m_userdata_size > 0
			 ? FrNewR(char,m_userdata,new_alloc*m_userdata_size)
			 : 0) ;
   char *new_revindex = (m_revindex
			 ? FrNewR(char,m_revindex,new_alloc*m_wordloc_size)
			 : 0) ;
   bool success = true ;
   if (new_index)
      m_index = new_index ;
   else
      success = false ;
   if (new_userdata)
      m_userdata = new_userdata ;
   else if (m_userdata_size > 0)
      success = false ;
   if (new_revindex)
      m_revindex = new_revindex ;
   else if (m_revindex)
      success = false ;
   if (success)
      {
      m_index_alloc = new_alloc ;
      return true ;
      }
   else
      {
      FrNoMemory("while attempting to expand FrVocabulary") ;
      return false ;
      }
}

//----------------------------------------------------------------------

bool FrVocabulary::expand()
{
   size_t new_alloc = 3 * m_index_alloc / 2 ;
   if (new_alloc < 10000)
      new_alloc += 500 ;
   if (new_alloc < m_index_size)
      new_alloc = m_index_size + 500 ;
   return expandTo(new_alloc) ;
}

//----------------------------------------------------------------------

const char *FrVocabulary::find(const char *word) const
{
   const char *name = getNameFromIndexEntry((size_t)0) ;
   if (!name || !word || strcmp(word,name) < 0)
      return 0 ;			// not in vocabulary!
   // binary search of the sorted vocabulary for the desired word
   size_t lo = 0 ;
   size_t hi = numWords() ;
   while (hi > lo)
      {
      size_t mid = (lo + hi) / 2 ;
      int cmp = strcmp(word,getNameFromIndexEntry(mid)) ;
      if (cmp < 0)
	 hi = mid ;
      else if (cmp > 0)
	 lo = mid+1 ;
      else
	 return indexEntry(mid) ;
      }
   return 0 ;
}

//----------------------------------------------------------------------

size_t FrVocabulary::findID(const char *word) const
{
   const char *entry = find(word) ;
   if (entry)
      return getID(entry) ;
   return FrVOCAB_WORD_NOT_FOUND ;
}

//----------------------------------------------------------------------

size_t FrVocabulary::addWordName(const char *word)
{
   assert(!m_mapped) ;
   if (!word)
      word = "" ;
   size_t len = strlen(word) ;
   size_t new_size = m_words_size + len + 1 ;
   if ((new_size & m_padmask) != 0)
      {
      // pad out to multiple of scale factor
      size_t pad = m_padmask + 1 - (new_size & m_padmask) ;
      new_size += pad ;
      }
   if (new_size > m_words_alloc)
      {
      size_t new_alloc = 3 * m_words_alloc / 2 ;
      if (new_alloc < 10000)
	 new_alloc += 1000 ;
      if (new_alloc < m_words_size)
	 new_alloc += 1000 ;
      char *new_words = FrNewR(char,m_words,new_alloc) ;
      if (!new_words)
	 {
	 FrNoMemory("while expanding FrVocabulary") ;
	 return FrVOCAB_WORD_NOT_FOUND ;
	 }
      m_words = new_words;
      m_words_alloc = new_alloc ;
      }
   m_changed = true ;
   memcpy(m_words + m_words_size,word,len) ;
   memset(m_words + m_words_size + len,'\0',new_size - m_words_size - len) ;
   size_t wordloc = (m_words_size >> m_scalefactor) ;
   m_words_size = new_size ;
   return wordloc ;
}

//----------------------------------------------------------------------

const char *FrVocabulary::add(const char *word, size_t N)
{
   unmemmap() ;
   if (m_index_size >= m_index_alloc && !expand())
      return 0 ;
   char *src = m_index + N * m_indexent_size ;
   char *dst = src + m_indexent_size ;
   memmove(dst,src,(m_index_size - N)*m_indexent_size) ;
   size_t newID = m_index_size ;
   m_index_size++ ;
   size_t wordloc = addWordName(word) ;
   setIndexEntry(N,wordloc,newID) ;
   if (m_revindex)
      {
      if (m_ID_size == 3)
	 FrStoreThreebyte(N,m_revindex + 3 * newID) ;
      else if (m_ID_size == 4)
	 FrStoreLong(N,m_revindex + m_ID_size * newID) ;
      }
   return indexEntry(N) ;
}

//----------------------------------------------------------------------

bool FrVocabulary::startBatchUpdate()
{
   if (!m_batch_update /*&& !usingNameAsID()*/)
      {
      unmemmap() ;
      m_batch_update = true ;
      return true ;
      }
   else
      return false ;
}

//----------------------------------------------------------------------

size_t FrVocabulary::addWord(const char *word, size_t ID)
{
   unmemmap() ;
   if (word && m_batch_update)
      {
      if (m_index_size >= m_index_alloc && !expand())
	 return FrVOCAB_WORD_NOT_FOUND ;
      size_t wordloc = addWordName(word) ;
      setIndexEntry(m_index_size++,wordloc,ID) ;
      return wordloc ;
      }
   return FrVOCAB_WORD_NOT_FOUND ;
}

//----------------------------------------------------------------------

static size_t wordptr_size ;
static size_t wordptr_shift = 0 ;
static const char *word_array ;

static int compare_names(const void *entry1, const void *entry2)
{
   size_t wordloc1 = (wordptr_size == 3
		      ? FrLoadThreebyte(entry1)
		      : FrLoadLong(entry1)) ;
   size_t wordloc2 = (wordptr_size == 3
		      ? FrLoadThreebyte(entry2)
		      : FrLoadLong(entry2)) ;
   if (wordptr_shift)
      {
      wordloc1 <<= wordptr_shift ;
      wordloc2 <<= wordptr_shift ;
      }
   return strcmp(word_array + wordloc1, word_array + wordloc2) ;
}

//----------------------------------------------------------------------

bool FrVocabulary::finishBatchUpdate()
{
   if (m_batch_update)
      {
      // sort the entries after adding a bunch to the end of the index
      wordptr_size = m_wordloc_size ;
      wordptr_shift = m_scalefactor ;
      word_array = m_words;
      if (m_index)
	 qsort(m_index, m_index_size, m_indexent_size, compare_names) ;
      m_batch_update = false ;
      // update the reverse mapping, if it was present before the update
      bool success = true ;
      if (m_revindex)
	 {
	 if (!m_revmapped)
	    {
	    FrFree(m_revindex) ;	m_revindex = 0 ;
	    }
	 success = createReverseMapping() ;
	 }
      return success ;
      }
   return false ;
}

//----------------------------------------------------------------------

const char *FrVocabulary::find(const char *word, bool add_if_missing)
{
   if (m_index_size == 0 ||
       strcmp(word,getNameFromIndexEntry((size_t)0)) < 0)
      {
      if (add_if_missing)
	 return add(word,0) ;
      return 0 ;			// not in vocabulary!
      }
   // binary search of the sorted vocabulary for the desired word
   size_t lo = 0 ;
   size_t hi = numWords() ;
   while (hi > lo)
      {
      size_t mid = (lo + hi) / 2 ;
      int cmp = strcmp(word,getNameFromIndexEntry(mid)) ;
      if (cmp < 0)
	 hi = mid ;
      else if (cmp > 0)
	 lo = mid+1 ;
      else if (cmp == 0)
	 return indexEntry(mid) ;
      }
   // if we get to this point, the requested word is not present in the
   // vocabulary, and 'lo' indicates where to insert it
   if (add_if_missing)
      return add(word,lo) ;
   return 0 ;
}

//----------------------------------------------------------------------

size_t FrVocabulary::findID(const char *word, bool add_if_missing)
{
   const char *entry = find(word,add_if_missing) ;
   return getID(entry) ;
}

//----------------------------------------------------------------------

size_t FrVocabulary::addWord(const char *word)
{
   size_t loc = findID(word,true) ;
   if (loc != FrVOCAB_WORD_NOT_FOUND)
      return getID(indexEntry(loc)) ;
   else
      return FrVOCAB_WORD_NOT_FOUND ;
}

//----------------------------------------------------------------------

bool FrVocabulary::useNameAsID(bool nameID)
{
   if (numWords() != 0)			// can only change if vocab is empty
      return false ;
   m_name_is_ID = nameID ;
   if (nameID)
      {
      m_wordloc_size = 4 ;
      m_ID_size = 0 ;
      }
   else
      {
      m_wordloc_size = 4 ;
      m_ID_size = 4 ;
      }
   m_indexent_size = m_wordloc_size + m_ID_size ;
   return true ;
}


//----------------------------------------------------------------------

bool FrVocabulary::setScaleFactor(size_t scalefac)
{
   if (numWords() != 0 || !usingNameAsID()) // only change if vocab is empty
      return false ;			    //   and setting is relevant
   if (scalefac <= 4)
      {
      m_scalefactor = scalefac ;
      m_padmask = (1 << scalefac) - 1 ;	// only correct for 2s-complement arch.
      }
   return true ;
}

//----------------------------------------------------------------------

size_t FrVocabulary::totalDataSize() const
{
   size_t idxsize = m_indexent_size * numWords() ;
   size_t revsize = (m_revindex ? m_wordloc_size * numWords() : 0) ;
   size_t usersize = m_userdata_size * numWords() ;
   return HEADER_SIZE + m_words_size + idxsize + revsize + usersize ;
}

//----------------------------------------------------------------------

const char *FrVocabulary::nameForID(size_t ID) const
{
   if (usingNameAsID())
      {
      if (ID < (m_words_size >> m_scalefactor))
	 return m_words + (ID << m_scalefactor) ;
      else
	 return 0 ;
      }
   if (ID >= numWords() || !m_revindex)
      return 0 ;
   size_t entry = locationForID(ID) ;
   if (entry < numWords())
      return getNameFromIndexEntry(entry) ;
   else
      return 0 ;
}

//----------------------------------------------------------------------

const char **FrVocabulary::makeWordList()
{
   const char **list = FrNewN(const char *,numWords()+1) ;
   if (list && createReverseMapping())
      {
      for (size_t i = 0 ; i < numWords() ; i++)
	 list[i] = nameForID(i) ;
      list[numWords()] = 0 ;
      }
   return list ;
}

//----------------------------------------------------------------------

void FrVocabulary::freeWordList(const char **wordlist)
{
   FrFree(wordlist) ;
   return ;
}

// end of file frvocab.cpp //
