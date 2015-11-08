/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC								*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frbwt.h	    Burrows-Wheeler Transform n-gram index	*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 2003,2004,2006,2007,2008,2009,2012			*/
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

#ifndef FRBWT_H_INCLUDED
#define FRBWT_H_INCLUDED

#ifndef __FRCOMMON_H_INCLUDED
#include "frcommon.h"
#endif /* __FRCOMMON_H_INCLUDED */

#ifndef __FRMEM_H_INCLUDED
#include "frmem.h"
#endif

#ifndef __FRBYTORD_H_INCLUDED
#include "frbytord.h"
#endif

#ifndef __FRFILUTL_H_INCLUDED
#include "frfilutl.h"	// needed for proper 64-bit file funcs on 32-bit arch
#endif

#ifdef FrSTRICT_CPLUSPLUS
# include <cstdio>
#else
# include <stdio.h>
#endif

/************************************************************************/
/************************************************************************/

#define FrBWT_ENDOFDATA ((size_t)~0)


#if 0 /* COMMENT */
   file header format:
	ASCIZ	signature (normally "BWT-encoded data")
		padded to 32 bytes with NULs
	BYTE	file format version number (1)
	BYTE	compression type
		   0 = uncompressed, 1 = byte compression (see below)
	BYTE	End-of-Record handling
		   0 = keep distinct, 1 = merge (FrBWT_MergeEOR)
	BYTE	Flags
		bit 0: words are in reverse order
		bit 1: words are case-sensitive
	        bit 2: character-based index
	LONG	byte offset of user data (following all BWT data)
		(low 32 bits -- see below for high-order bits)
	LONG	offset of ID-to-entry# mapping in file (BWT "C" array)
	LONG	number of IDs in ID-to-entry# map
   	LONG	offset of first BWT entry in file (BWT "FL" or "V" array)
	LONG	number of data items stored in FL
	LONG	total number of data items, including EOR values
   	LONG	value of first end-of-record indicator
		   (record number equals EORvalue - firstEOR)
	LONG	if byte-compressed: bucket size
		else: reserved (0)
	LONG	if byte-compressed: start offset of secondary table in file
		else: reserved (0)
	LONG	if byte-compressed: start offset of secondary pool in file
		else: reserved (0)
	LONG	if byte-compressed: # entries in secondary pool
		else: reserved (0)
   	BYTE	if byte-compressed: max delta to be stored in byte
		else: reserved (0)
        BYTE	affix lengths (0 = using entire word,
			  else bits 7-4 = prefix chars, bits 3-0 = suffix chars)
		Note: this is informational only, so that caller can
		   appropriately convert text into word IDs
      2 BYTEs	reserved (0)
	LONG	discount mass
	SHORT	bits 47-33 of user data offset
	SHORT	bits 47-33 of secondary table offset
	SHORT	bits 47-33 of secondary pool offset
      2 BYTEs reserved (0)
   (total 96 bytes)

   data format for "C":
	N+1 LONGs  index of first occurrence for each ID, plus one beyond end

   data format for uncompressed index:
       N LONGs	successor pointers

   data format for byte-compressed index:
    a: primary table
	8 bits per word;
   	       0-127 = delta from previous ptr, less one
              127-254/255 = index in secondary pointer table
   		255 = next item is EOR if FrBWT_MergeEOR selected
	   (note: adjust split point to minimize overall size)
	table is notionally divided into equal-sized buckets
    b: secondary pointer table
	one LONG per bucket, plus global pool
	    the LONG contains address of first pointer in global pool
	    global pool consists of LONGs, each an absolute address in primary
		table
	    can do without a count since we are normally accessing a specified
		offset from the indicated pointer, and can recover the
		count if necessary provided the pointers are in ascending
		order

#endif /* COMMENT */

/************************************************************************/
/************************************************************************/

class FrVocabulary ;

//----------------------------------------------------------------------

enum FrLMSmoothing
   {
      FrLMSmooth_None,			// use actual value for N-gram
      FrLMSmooth_Backoff,		// use simple backoff
      FrLMSmooth_Max,			// use maximum of all k-grams (k <= N)
      FrLMSmooth_Mean,			// use mean of all k-grams
      FrLMSmooth_WtMean,		// use length-weighted mean of k-grams
      FrLMSmooth_QuadMean,		// use squared-len-wt mean of k-grams
      FrLMSmooth_CubeMean,		// use cubed-len-wt mean of k-grams
      FrLMSmooth_ExpMean,		// use 2**len-weighted mean of k-grams
      FrLMSmooth_SimpKN,		// use simplified Kneser-Ney
      FrLMSmooth_StupidBackoff		// use Google's Stupid Backoff
   } ;

enum FrBWTEORHandling
   {
      FrBWT_MergeEOR,
      FrBWT_KeepEOR
      //!!!
   } ;

//----------------------------------------------------------------------

class FrBWTLocLen
{
   private:
      size_t m_start ;
      size_t m_end ;
      size_t m_length ;
   public:
      void *operator new(size_t size,void *where)
	    { (void)size; return where ; }
      void *operator new(size_t size) { return FrMalloc(size) ; }
      void operator delete(void *blk) { FrFree(blk) ; }
//      void operator delete(void *blk, size_t size)
//	 { if (size == sizeof(FrBWTLocLen)) FrFree(blk) ; }
      FrBWTLocLen(size_t start, size_t past_end, size_t len)
	    { m_start = start ; m_end = past_end ; m_length = len ; }
      FrBWTLocLen(const FrBWTLocLen &other)
	    { m_start = other.m_start ; m_end = other.m_end ;
      m_length = other.m_length ; }
      ~FrBWTLocLen() {} ;

      // manipulators
      FrBWTLocLen &operator = (const FrBWTLocLen &other)
			      { m_start = other.m_start ; m_end = other.m_end ;
      m_length = other.m_length ; return *this ; }
      void adjustStart(size_t new_start) { m_start = new_start ; }
      void adjustEnd(size_t new_end) { m_end = new_end ; }
      void adjustLength(size_t new_len) { m_length = new_len ; }

      // accessors
      bool nonEmpty() const { return m_end > m_start ; }
      bool isEmpty() const { return m_end <= m_start ; }
      size_t rangeSize() const { return m_end > m_start ? m_end-m_start : 0 ; }
      size_t first() const { return m_start ; }
      size_t last() const { return (m_end > 0) ? (m_end - 1) : 0 ; }
      size_t pastEnd() const { return m_end ; }
      size_t count() const { return m_end - m_start ; }
      size_t length() const { return m_length ; }
      bool inRange(size_t where) const
	    { return (where >= m_start && where < m_end) ; }
      bool inOrAbutsRange(size_t where) const
	    { return (where >= m_start && where <= m_end) ; }

      friend class FrBWTLocation ;
} ;

//----------------------------------------------------------------------

class FrBWTLocation
   {
   private:
      size_t m_start ;
      size_t m_end ;
   public:
      void *operator new(size_t size,void *where)
	 { (void)size; return where ; }
      void *operator new(size_t size) { return FrMalloc(size) ; }
      void operator delete(void *blk) { FrFree(blk) ; }
//      void operator delete(void *blk, size_t size)
//	 { if (size == sizeof(FrBWTLocation)) FrFree(blk) ; }
      FrBWTLocation()
	 { m_start = (size_t)~0 ; m_end = 0 ; }
      FrBWTLocation(size_t loc)
         { m_start = loc ; m_end = loc + 1 ; }
      FrBWTLocation(size_t start, size_t past_end)
         { m_start = start ; m_end = past_end ; }
      FrBWTLocation(const FrBWTLocation &other)
         { m_start = other.m_start ; m_end = other.m_end ; }
      ~FrBWTLocation() {} ;

      // manipulators
      FrBWTLocation &operator = (const FrBWTLocation &other)
         { m_start = other.m_start ; m_end = other.m_end ; return *this ; }
      FrBWTLocation &operator = (const FrBWTLocLen &other)
 	 { m_start = other.m_start ; m_end = other.m_end ; return *this ; }
      void adjustStart(size_t new_start) { m_start = new_start ; }
      void adjustEnd(size_t new_end) { m_end = new_end ; }

      // accessors
      bool nonEmpty() const { return m_end > m_start ; }
      bool isEmpty() const { return m_end <= m_start ; }
      size_t rangeSize() const { return m_end > m_start ? m_end-m_start : 0 ; }
      size_t first() const { return m_start ; }
      size_t last() const { return (m_end > 0) ? (m_end - 1) : 0 ; }
      size_t pastEnd() const { return m_end ; }
      size_t count() const { return m_end - m_start ; }
      bool inRange(size_t where) const
	 { return (where >= m_start && where < m_end) ; }
      bool inOrAbutsRange(size_t where) const
	    { return (where >= m_start && where <= m_end) ; }
      bool operator == (const FrBWTLocation &other)
	 { return first() == other.first() && pastEnd() == other.pastEnd() ; }
   } ;

//----------------------------------------------------------------------

class FrBWTLocationList
   {
   private:
      static FrAllocator allocator ;
      size_t	m_numranges ;
      size_t	m_totalsize ;
      FrBWTLocation *m_ranges ;
//      size_t	m_startpos ;
   public:
      void *operator new(size_t) { return allocator.allocate() ; }
      void operator delete(void *blk) { allocator.release(blk) ; }
      FrBWTLocationList() ;
      FrBWTLocationList(const FrBWTLocation &) ;
      FrBWTLocationList(const FrBWTLocationList &) ;
      ~FrBWTLocationList() ;
      FrBWTLocationList &operator = (const FrBWTLocationList &) ;

      // accessors
      size_t numRanges() const { return m_numranges ; }
      size_t totalMatches() const { return m_totalsize ; }
      size_t totalExtent() const ;
      FrBWTLocation range(size_t which) const ;
      size_t rangeSize(size_t which) const ;
      bool covered(uint32_t location) const ;
      void precedingRange(uint32_t location, size_t &first,
			  size_t &pastend) const ;
//      size_t startPosition() const { return m_startpos ; }

      // manipulators
//      void startPosition(size_t pos) { m_startpos = pos ; }
      bool insert(FrBWTLocation loc) ;
      bool append(uint32_t location) ;
      bool append(FrBWTLocation loc) ;
      bool append(const FrBWTLocationList *locs) ;
      bool merge(const FrBWTLocationList &loc) ;
      bool remove(size_t which) ;
      void clear() ;
      void sort() ;
//!!!
   } ;

//----------------------------------------------------------------------

class FrBWTLocLenList
{
   private:
      static FrAllocator allocator ;
      size_t	m_numranges ;
      size_t	m_totalsize ;
      FrBWTLocLen *m_ranges ;
   public:
      void *operator new(size_t) { return allocator.allocate() ; }
      void operator delete(void *blk) { allocator.release(blk) ; }
      FrBWTLocLenList() ;
      FrBWTLocLenList(const FrBWTLocLen &) ;
      FrBWTLocLenList(const FrBWTLocLenList &) ;
      ~FrBWTLocLenList() ;
      FrBWTLocLenList &operator = (const FrBWTLocLenList &) ;

      // accessors
      size_t numRanges() const { return m_numranges ; }
      size_t totalMatches() const { return m_totalsize ; }
      size_t totalExtent() const ;
      FrBWTLocation range(size_t which) const ;
      size_t rangeSize(size_t which) const ;
      size_t sourceLength(size_t which) const ;

      // manipulators
      bool insert(FrBWTLocation loc, size_t srclen) ;
      bool insert(FrBWTLocLen loc) ;
      bool append(FrBWTLocation loc, size_t srclen) ;
      bool append(FrBWTLocLen loc) ;
      bool append(uint32_t location, size_t srclen) ;
      bool remove(size_t which) ;
      void clear() ;
      void sort() ;
} ;

//----------------------------------------------------------------------

class FrWordIDList
   {
   private:	// static data
      static size_t buffersize ;
      static size_t elts_per_buffer ;

   private:	// member variables
      FrWordIDList *m_next ;
      size_t m_elts_used ;
      uint32_t buffer[1] ;

   private:	// methods
      FrWordIDList(FrWordIDList *nxt = 0) { m_next = nxt ; }
      ~FrWordIDList() {}

   public:	// methods
      static FrWordIDList *newIDListElt(FrWordIDList *nxt = 0) ;
      void freeIDList() ;

      // manipulators
      bool add(uint32_t value) ;
      void add_safe(uint32_t value) { buffer[m_elts_used++] = value ; }
      void setNth(size_t N, uint32_t value) ;
      FrWordIDList *reverse() ;

      // accessors
      FrWordIDList *next() { return m_next ; }
      const FrWordIDList *next() const { return m_next ; }
      uint32_t getNth(size_t N) const ;
      size_t eltsUsed() const { return m_elts_used ; }
      size_t eltsPerBuffer() const { return elts_per_buffer ; }
      bool full() const { return m_elts_used >= elts_per_buffer ; }
   } ;

//----------------------------------------------------------------------

class FrNGramHistory
   {
   private:
      // location in FrBWTIndex (suffix-array model) or LmNgramsFile
      // raw word ID for unsmoothed or joint-probability model
      uint32_t m_hist ;
      uint32_t m_count ;
//      double m_smoothing ;
   public:
      FrNGramHistory() {}

      // accessors
      uint32_t index() const { return m_hist ; }
      uint32_t startLoc() const { return m_hist ; }
      uint32_t endLoc() const { return m_hist + m_count - 1 ; }
      uint32_t wordID() const { return m_hist ; }
      uint32_t count() const { return m_count ; }
//      double smoothing() const { return m_smoothing ; }

      // manipulators
      void setLoc(FrBWTLocation loc)
	 { m_hist = loc.first() ; m_count = loc.rangeSize() ; }
      void setLoc(uint32_t first, uint32_t last)
	 { m_hist = first ; m_count = (last - first) + 1 ; }
      void setIndex(size_t idx) { m_hist = (uint32_t)idx ; }
      void setID(uint32_t ID) { m_hist = ID ; }
      void setCount(size_t cnt) { m_count = (uint32_t)cnt ; }
//      void setSmoothing(double sm) { m_smoothing = sm ; }
   } ;

//----------------------------------------------------------------------

typedef bool FrBWTNGramIterFunc(uint32_t *IDs, size_t N, size_t freq,
				  const class FrBWTIndex *index,
				  va_list args) ;

class FrBWTIndex
   {
   private:
      double m_discount ;		// fract to reserve for OOVs
      double m_mass_ratio ;		// 1 + m_discount
      double m_total_mass ;		// m_mass_ratio * m_numitems
      char *m_filename ;		// name of disk file containing index
      FILE *m_fp_read ;			//
      FILE *m_fp_write ;		//
      class FrFileMapping *m_fmap ;	// memory-mapped file info
      char *m_C ;			// mapping from ID to first occur in FL
      unsigned char *m_items ;		// pointers to successors (FL)
      char *m_buckets ;			// secondary pointer table
      char *m_bucket_pool ;		// pool of secondary pointers
      char *m_userdata ;		//
      size_t *m_class_sizes ;		// how many elements in each equivclass
      size_t m_num_classes ;		// how many equivalence classes?
      size_t m_numitems ;		// size of m_items*
      size_t m_totalitems ;		// # of items, incl unstored EOR values
      size_t m_numIDs ;			// size of m_C
      size_t m_userdatasize ;		// bytes of user data in disk file
      size_t m_userdataoffset ;		// offset of user data in disk file
      size_t m_EOR ;			// first End-of-Record value
      size_t m_maxdelta ;		// max diff storable in compr. m_items
      size_t m_bucketsize ;		// #entries in m_items* per bucket
      size_t m_numbuckets ;		// #entries in m_buckets
      size_t m_poolsize ;		// #entries in m_bucket_pool
      bool m_compressed ;		// is m_items compressed?
      bool m_buffereduserdata ;	// is m_userdata an allocated buffer?
      FrBWTEORHandling m_eor_state ;	// what to do with EOR items
      int  m_flags ;
      uint8_t m_affix_sizes ;

#if 0
      static const int bytes_per_ptr = 4 ;
      static const unsigned ptr_shift = 2 ;	// 1<<2 == 4
#else
      enum { bytes_per_ptr = 4 } ;	// 32-bit pointers
      enum { ptr_shift = 2 } ;		// 1<<2 == 4
#endif

   protected:
      void init() ;
      bool parseHeader(class BWTHeader &header, const char *filename) ;
      bool parseHeader(FILE *, const char *filename,
			 class BWTHeader &header) ;
      bool loadCompressionData(FILE *, const struct BWTHeader &) ;

      uint32_t C(size_t idx) const
	    { return FrLoadLong(m_C + (idx << ptr_shift)) ; }
      uint32_t getAbsPointer(size_t N, size_t idx) const ;
      uint32_t getCompressedSuccessor(size_t idx) const ;
      uint32_t getCompressedSuccessor(size_t idx, size_t byte,
				      uint32_t left_succ) const ;
      uint32_t getCompressedSuccessor(size_t idx, uint32_t left_succ) const
	    { return getCompressedSuccessor(idx,m_items[idx],left_succ) ; }
      uint32_t getUncompSuccessor(size_t idx) const
	    { return FrLoadLong(m_items+(idx<<ptr_shift)) ; }
      uint32_t getSuccessor(size_t idx, uint32_t left_succ) const
	    { if (m_compressed) return getCompressedSuccessor(idx,left_succ) ;
	      else return FrLoadLong(m_items+(idx<<ptr_shift)) ; }
      uint32_t getNthSuccessor(size_t idx, size_t N) const ;
      size_t iterateNGrams(size_t currN, size_t maxN,
			   size_t startloc, size_t endloc,
			   bool include_EOR, uint32_t *IDs, size_t minfreq,
			   FrBWTNGramIterFunc *fn, va_list args) const ;
      size_t iterateNGramsVA(size_t currN, size_t maxN,
			     size_t startloc, size_t endloc,
			     bool include_EOR, uint32_t *IDs, size_t minfreq,
			     FrBWTNGramIterFunc *fn, ...) const ;

      void setC(size_t idx, size_t value) ;

   public:
      void *operator new(size_t size) { return FrMalloc(size) ; }
      void operator delete(void *blk) { FrFree(blk) ; }
      FrBWTIndex() ;
      FrBWTIndex(const char *filename, bool memory_mapped = true,
		 bool touch_memory = false) ;
      FrBWTIndex(uint32_t *items, size_t num_items,
		 FrBWTEORHandling = FrBWT_MergeEOR, uint32_t eor = ~0,
		 ostream *progress = 0) ;
	      // note: 'items' gets overwritten
      virtual ~FrBWTIndex() ;

      virtual const char *signatureString() const ;

      // manipulators
      bool load(const char *filename, bool memory_mapped = true,
		  bool touch_memory = false) ;
      void makeIndex(uint32_t *items, size_t num_items,
		     FrBWTEORHandling = FrBWT_MergeEOR, uint32_t eor = ~0,
		     ostream *progress = 0) ;
	      // note: 'items' gets overwritten
      bool loadUserData() ;
      bool unmmapUserData() ;
      bool compress() ;
      bool uncompress() ;
      bool merge(FrBWTIndex *other) ;
      bool save(const char *filename,
		  bool (*user_write_fn)(FILE *,void *) = 0,
		  void *user_data = 0) ;
      class FrBitVector *findRecordStarts(uint32_t &vectorlen) const ;
      uint32_t *deconstruct(size_t extra_entries_reserved = 0) const ;
      bool deconstruct(uint32_t *buffer, size_t bufsize) const ;

      bool verifySignature(const char *filename) const ;
      bool compress(const char *infile, const char *outfile) const ;
      bool uncompress(const char *infile, const char *outfile) const ;

      bool setEOR(size_t newEOR = ~0) ;
      void setDiscounts(double discount) ;
      void wordsAreReversed(bool rev) ;
      void wordsAreCaseSensitive(bool cased) ;
      void indexIsCharBased(bool chbased) ;
      void setAffixSizes(uint8_t sizes) ;

      void initClassSizes(const class FrVocabulary *vocab) ;
      void freeClassSizes() ;

      // accessors
      bool good() const { return m_C && m_items ; }
      bool passesSanityChecks() const ;
      size_t numIDs() const { return m_numIDs ; }
      size_t numItems() const { return m_numitems ; }
      size_t totalItems() const { return m_totalitems ; }
      double totalMass() const { return m_total_mass ; }
      double massRatio() const { return m_mass_ratio ; }
      double discountMass() const { return m_discount ; }
      size_t classSize(size_t id) const
	 { return (id < m_num_classes) ? m_class_sizes[id] : 1 ; }
      size_t EORvalue() const { return m_EOR ; }
      bool isEOR(size_t N) const { return N >= m_EOR ; }
      bool compressed() const { return m_compressed ; }
      bool wordsAreReversed() const ;
      bool wordsAreCaseSensitive() const ;
      bool indexIsCharBased() const ;
      uint8_t getAffixSizes() const ;
      size_t bucketsize() const { return m_bucketsize ; }
      char *userData() const { return m_userdata ; }
      size_t userDataOffset() const { return m_userdataoffset ; }
      size_t userDataSize() const { return m_userdatasize ; }
      FrFileMapping *mappedFile() const { return m_fmap ; }

      uint32_t getID(size_t location) const ;
      uint32_t firstLocation(size_t ID) const
	 { return (m_C && ID <= m_numIDs)
	      ? FrLoadLong(m_C + (ID<<ptr_shift)) : (size_t)~0 ; }
      uint32_t getSuccessor(size_t idx) const
	 { if (idx >= numItems()) return (uint32_t)FrBWT_ENDOFDATA ;
	   else if (!m_compressed)
	      return FrLoadLong(m_items+(idx<<ptr_shift)) ;
	   else return getCompressedSuccessor(idx) ; }

      FrBWTLocation unigram(uint32_t id) const ;
      size_t unigramFrequency(uint32_t id) const ;
      FrBWTLocation bigram(uint32_t id1, uint32_t id2) const ;
      size_t bigramFrequency(uint32_t id1, uint32_t id2) const ;
      FrBWTLocation trigram(uint32_t id1, uint32_t id2, uint32_t id3) const ;
      size_t trigramFrequency(uint32_t id1, uint32_t id2, uint32_t id3) const ;
      FrBWTLocation lookup(const uint32_t *searchkey, size_t keylength) const ;
      double estimateFrequency(const uint32_t *searchkey, size_t keylength,
			       size_t *numbackoffs = 0) const ;
      size_t frequency(const uint32_t *searchkey, size_t keylength) const ;
      double frequency(const uint32_t *searchkey, size_t keylength,
		       bool allow_backoff, size_t *numbackoffs = 0) const ;
      size_t frequency(const FrNGramHistory *history, size_t histlen,
		       uint32_t nextID) const ;
      double frequency(FrNGramHistory *history, size_t histlen,
		       uint32_t nextID, bool allow_backoff,
		       size_t *numbackoffs = 0) const ;
      // find match for reversed search key (needed when BWT file has not had
      //   words reversed)
      FrBWTLocation revLookup(const uint32_t *searchkey,
			      size_t keylength) const ;
      size_t revFrequency(const uint32_t *searchkey, size_t keylength) const ;
      void initZerogram(FrNGramHistory *history) const ;
      void advanceHistory(FrNGramHistory *history, size_t histlen) const ;
      double rawCondProbability(const uint32_t *searchkey,
				size_t keylength) const ;
      double rawCondProbability(FrNGramHistory *history, size_t histlen,
				uint32_t nextID) const ;
      double condProbabilityBackoff(FrNGramHistory *history,
				    size_t histlen,
				    uint32_t nextID,
				    size_t *max_exist,
				    size_t *numbackoffs,
				    FrLMSmoothing smoothing) const ;
      double condProbabilityMax(FrNGramHistory *history, size_t histlen,
				uint32_t nextID, size_t *max_exist) const ;
      double condProbabilityMean(FrNGramHistory *history, size_t histlen,
				 uint32_t nextID, FrLMSmoothing smoothing,
				 size_t *numbackoffs,size_t *max_exist) const ;
      double condProbabilityKN(const uint32_t *searchkey,
			       size_t keylength, size_t *max_exist = 0) const ;
      double condProbabilityKN(FrNGramHistory *history, size_t histlen,
			       uint32_t nextID, size_t *max_exist = 0) const ;
      double condProbability(const uint32_t *searchkey, size_t keylength,
			     FrLMSmoothing smoothing = FrLMSmooth_None,
			     size_t *numbackoffs = 0,
			     size_t *max_exist = 0) const ;
      double condProbability(FrNGramHistory *history, size_t histlen,
			     uint32_t nextID,
			     FrLMSmoothing smoothing = FrLMSmooth_None,
			     size_t *numbackoffs = 0,
			     size_t *max_exist = 0) const ;
      size_t longestMatch(const uint32_t *searchkey, size_t keylength) const ;
      FrBWTLocation extendMatch(const FrBWTLocation match,
				const FrBWTLocation next) const ;
      FrBWTLocation extendMatch(const FrBWTLocation match,
				const FrBWTLocation next,
				size_t next_words) const ;
      FrBWTLocation extendMatch(const FrBWTLocation match,
				uint32_t nextID) const ;
      FrBWTLocationList *extendMatch(const FrBWTLocationList *match,
				     uint32_t *nextIDs, size_t numIDs,
				     bool IDs_are_sorted = false,
				     FrBWTLocationList *unextendable=0) const ;
      FrBWTLocationList *extendMatch(const FrBWTLocationList *match,
				     FrBWTLocation next,
				     FrBWTLocationList *unextendable=0) const ;
      FrBWTLocationList *extendMatch(const FrBWTLocationList *match,
				     FrBWTLocation next, size_t next_words,
				     FrBWTLocationList *unextendable=0) const ;
      FrBWTLocLenList *extendMatch(const FrBWTLocLenList *match,
				   uint32_t *nextIDs, size_t numIDs,
				   size_t *source_lengths,
				   FrBWTLocLenList *unextendable=0) const ;
      FrBWTLocLenList *extendMatch(const FrBWTLocLenList *match,
				   FrBWTLocation next, size_t newlen = 1,
				   FrBWTLocLenList *unextendable=0) const ;
      FrBWTLocationList *extendMatchGap(const FrBWTLocationList *match,
					uint32_t nextID, size_t gap_size = 1,
					FrBWTLocationList *unextendable=0)
	    			const ;
      FrBWTLocationList *extendMatchGap(const FrBWTLocationList *match,
					FrBWTLocation next, size_t gap_size=1,
					FrBWTLocationList *unextendable=0)
			        const ;
      FrBWTLocLenList *extendMatchGap(const FrBWTLocLenList *match,
				      uint32_t nextID, size_t gap_size = 1,
				      FrBWTLocLenList *unextendable=0) const ;
      FrBWTLocLenList *extendMatchGap(const FrBWTLocLenList *match,
				      FrBWTLocation next, size_t gap_size=1,
				      FrBWTLocLenList *unextendable=0) const ;
      FrBWTLocation extendRight(FrBWTLocation match, size_t ngram_length,
				uint32_t nextID) const ;
      FrBWTLocationList *restrictMatch(const FrBWTLocationList *match,
				       FrBWTLocation next) const ;
      uint32_t nextID(size_t location, size_t *next_location = 0) const ;
      uint32_t recordNumber(size_t location, size_t *offset = 0) const ;
      uint32_t recordNumber(FrBWTLocation location, size_t *offset = 0) const ;

      // utility functions
      size_t countNGrams(size_t N) const ;
      size_t countNGrams(size_t N, FrBWTLocation range) const ;
      size_t countNGrams(size_t N, size_t startloc, size_t endloc) const ;
      size_t printNGrams(size_t N, ostream &out, const char **names,
			 size_t num_names, size_t min_freq = 1,
			 bool print_freq = false) const ;
      size_t printNGrams(FrBWTLocation range, size_t N, ostream &out,
			 const char **names, size_t num_names,
			 size_t min_freq = 1,bool print_freq = false) const ;
      size_t enumerateNGrams(size_t N, size_t minfreq, bool include_EOR,
			     FrBWTNGramIterFunc fn, va_list) const ;
      size_t enumerateNGrams(size_t N, size_t minfreq, bool include_EOR,
			     FrBWTNGramIterFunc fn, ...) const ;

      // debugging/diagnostic info
      bool deltaHistogram(size_t maxdiff, size_t *diffcounts) const ;
      bool compressionStats(size_t &uncomp, size_t &comp) const ;

      int readUserData(size_t offset, char *buffer, size_t bufsize) ;

      static int bytesPerPointer() { return bytes_per_ptr ; }
      static int pointerShift() { return ptr_shift ; }
   } ;

//----------------------------------------------------------------------

extern double FramepaC_StupidBackoff_alpha ;

//----------------------------------------------------------------------

bool FrAdd2WordIDList(FrWordIDList *&idlist, size_t id) ;
uint32_t *FrMakeWordIDArray(const FrWordIDList *list, size_t &num_words,
			    uint32_t eor, bool reverse = false) ;
uint32_t *FrAugmentWordIDArray(const FrWordIDList *list,
			       uint32_t *prevIDs, size_t &num_words,
			       uint32_t eor, bool reverse = false) ;
size_t FrCountWordIDs(const FrWordIDList *list) ;

bool FrBeginText2WordIDs(FrSymbolTable *&old_symtab) ;
void FrText2WordIDsMarkNew(class FrSymbol *sym) ;
FrVocabulary *FrFinishText2WordIDs(FrSymbolTable *old_symtab) ;
size_t FrCvtWord2WordID(const char *word, size_t &numIDs) ;
void FrBWTMarkAsDummy(FrSymbol *sym) ;

void FrSortWordIDs(uint32_t elts[], size_t num_elts) ;

void FrParseLMSmoothingType(class FrSymbol *type, FrLMSmoothing *smoothing) ;

#endif /* !FRBWT_H_INCLUDED */

// end of file frbwt.h //
