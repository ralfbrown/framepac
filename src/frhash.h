/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC -- frame manipulation in C++				*/
/*  Version 2.01 							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frhash.h	 template class FrHashTable			*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,1998,1999,2001,2002,2003,2005,	*/
/*		2009,2015 Ralf Brown/Carnegie Mellon University		*/
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

#define NHELGRIND

#ifndef __FRHASH_H_INCLUDED
#define __FRHASH_H_INCLUDED

#ifndef __FRSYMBOL_H_INCLUDED
#include "frsymbol.h"
#endif

#ifndef __FRMEM_H_INCLUDED
#include "frmem.h"
#endif

#include "frthread.h"

#if defined(__GNUC__)
#  pragma interface
#endif

#include <iomanip>
#include <math.h>
#include <stdlib.h>

#if DYNAMIC_ANNOTATIONS_ENABLED != 0
#  include "dynamic_annotations.h"
#endif /* DYNAMIC_ANNOTATIONS_ENABLED */

/************************************************************************/
/*	Compile-Time Configuration					*/
/************************************************************************/

// specify how much checking and output you want:
//   0 - completely silent except for critical errors
//   1 - important messsages only
//   2 - status messages
//   3 - extra sanity checking
//   4 - expensive sanity checking
#define FrHASHTABLE_VERBOSITY 0

// uncomment the following line to collect operational statistics (slightly slower)
#define FrHASHTABLE_STATS

// comment out the following to maintain global stats (much slower, since it requires
//    atomic increments of contended locations)
#define FrHASHTABLE_STATS_PERTHREAD

// uncomment the following line to interleave bucket headers and
//   bucket contents rather than using separate arrays (fewer cache
//   misses, but wastes some space on 64-bit machines if the key or
//   value are 64 bits)
#define FrHASHTABLE_INTERLEAVED_ENTRIES

// uncomment the following line to enable 16-bit link pointers instead
//   of 8-bit.  This will increase memory use unless you use
//   interleaved entries on a 64-bit machine.
//#define FrHASHTABLE_BIGLINK

/************************************************************************/
/*	Manifest Constants						*/
/************************************************************************/

#define FrHASHTABLE_MIN_INCREMENT 256

#ifdef FrMULTITHREAD
#  define FrHASHTABLE_MIN_SIZE 256
// number of distinct indirection records to maintain in the
//   FrHashTable proper (if we need more, we'll allocate them, but we
//   mostly want to be able to satisfy needs without that).  We want
//   not just before/after resize, but also a couple extra so that we
//   can allow additional writes even before all old readers complete;
//   if we were to wait for all readers, performance wouldn't scale as
//   well with processor over-subscription.
#  define NUM_TABLES	8
#else
#  define FrHASHTABLE_MIN_SIZE 128
#  define NUM_TABLES	2		// need separate for before/after resize
#endif

#define FrHashTable_DefaultMaxFill 0.97
#define FrHashTable_MinFill        0.25	// lowest allowed value for MaxFill

// starting up the copying of a segment of the current hash array into
//   a new hash table is fairly expensive, so enforce a minimum size
//   per segment
#define FrHASHTABLE_SEGMENT_SIZE 2048

/************************************************************************/

// if we're trying to eke out every last bit of performance by disabling
//   all debugging code, also turn off statistics collection
#ifdef NDEBUG
#  undef FrHASHTABLE_STATS
#endif /* NDEBUG */

/************************************************************************/

// for RedHat's GCC 4.4.7
#ifndef INT8_MIN 
#  define INT8_MIN (-128)
#  define INT16_MIN (-32768)
#endif

#ifndef INT8_MAX
#  define INT8_MAX 127
#  define INT16_MAX 32767
#endif

#ifdef FrHASHTABLE_BIGLINK
#  define FrHASHTABLE_SEARCHRANGE INT16_MAX
#else
#  define FrHASHTABLE_SEARCHRANGE INT8_MAX
#endif

#ifdef FrHASHTABLE_STATS
# define INCR_COUNTstat(x) (++ s_stats.x)
# ifdef FrHASHTABLE_STATS_PERTHREAD
#  define INCR_COUNT(x) (++ s_stats.x)
#  define DECR_COUNT(x) (-- s_stats.x)
# else
#  define INCR_COUNT(x) (cs::increment(m_stats.x))
#  define DECR_COUNT(x) (cs::decrement(m_stats.x))
# endif /* PERTHREAD */
#else
#  define INCR_COUNTstat(x)
#  define INCR_COUNT(x)
#  define DECR_COUNT(x)
#endif

#ifndef ANNOTATE_BENIGN_RACE_STATIC
# define ANNOTATE_BENIGN_RACE_STATIC(var, desc) static var ;
# define ANNOTATE_UNPROTECTED_READ(x) x
#endif

#ifdef FrHASHTABLE_INTERLEAVED_ENTRIES
#  define if_INTERLEAVED(x) x
#  define ifnot_INTERLEAVED(x)
#else
#  define if_INTERLEAVED(x)
#  define ifnot_INTERLEAVED(x) x
#endif /* FrHASHTABLE_INTERLEAVED_ENTRIES */

/************************************************************************/

extern size_t FramepaC_initial_indent ;
extern size_t FramepaC_display_width ;
extern size_t FramepaC_small_primes[] ;

extern FrPER_THREAD size_t my_job_id ;

/************************************************************************/
/*	Forward declaration for FrSymbolTable				*/
/************************************************************************/

const FrSymbol *Fr_allocate_symbol(class FrSymbolTable *symtab, const char *name,
				   size_t namelen) ;

/************************************************************************/
/*	Helper functions						*/
/************************************************************************/

#if FrHASHTABLE_VERBOSITY > 0
#include <cstdio>
#endif /* FrHASHTABLE_VERBOSITY > 0 */

/************************************************************************/
/*  Helper specializations of global functions for use by FrHashTable	*/
/************************************************************************/

inline void free_object(const FrObject*) { return ; }
inline void free_object(const FrSymbol*) { return ; }
inline void free_object(size_t) { return ; }

/************************************************************************/
/*	Structure to store FrHashTable Statistics			*/
/************************************************************************/

// this class can't have any methods, since older compilers won't allow it
//   to be used as a per-thread variable if it does
class FrHashTable_Stats
   {
   public:
      size_t insert ;
      size_t insert_dup ;
      size_t insert_attempt ;
      size_t insert_forwarded ;
      size_t insert_resize ;
      size_t remove ;
      size_t remove_found ;
      size_t remove_forwarded ;
      size_t contains ;
      size_t contains_found ;
      size_t contains_forwarded ;
      size_t lookup ;
      size_t lookup_found ;
      size_t lookup_forwarded ;
      size_t resize ;
      size_t resize_assist ;
      size_t resize_cleanup ;
      size_t reclaim ;
      size_t move ; // how many entries were moved to make room?
      size_t neighborhood_full ;
      size_t CAS_coll ;
      size_t chain_lock_count ;
      size_t chain_lock_coll ;
      size_t spin ;
      size_t yield ;
      size_t sleep ;
      size_t none ;			// dummy for use in macros that require a counter name
   } ;

class FrHashTable_Stats_w_Methods : public FrHashTable_Stats
   {
   public:
      void clear() ;
      void add(const FrHashTable_Stats *other_stats) ;
   } ;

/************************************************************************/
/*	Declarations for template class FrHashTable			*/
/************************************************************************/

template <typename KeyT, typename ValT>
class FrHashTable : public FrObject
   {
   private: // embedded types
   private:
      static size_t numValues() { return (sizeof(ValT) < 2) ? 0 : 1 ; }
   public:
      typedef bool HashKeyValueFunc(KeyT key, ValT value,va_list) ;
      typedef bool HashKVFunc(KeyT key, ValT value) ;
      typedef bool HashKeyPtrFunc(KeyT key, ValT *,va_list) ;
   protected:
      typedef FrCriticalSection cs ;

      // encapsulate the chaining links
#ifdef FrHASHTABLE_BIGLINK
      typedef int16_t Link ;
      static const Link NULLPTR = INT16_MIN ;
#else
      typedef int8_t Link ;
      static const Link NULLPTR = INT8_MIN ;
#endif /* FrHASHTABLE_BIGLINK */
      static const Link searchrange = FrHASHTABLE_SEARCHRANGE ; // full search window (+/-)
      static const size_t NULLPOS = ~0UL ;
      struct HashPtrHead
         {
	    // the following two fields need to be adjacent so that we
	    //   can CAS them together in insertKey(), so we define
	    //   this struct just for that purpose
	    Link	first ;	// offset of first entry in hash bucket
	    uint8_t	status ;// is someone messing with this bucket's chain?
	    HashPtrHead() : status(0) {}
         } ;	    
      union HashPtrInt
         {
	    // C++ before C++11 doesn't allow union members with constructors, so we'll just
	    //   define the int of the appropriate size
	    //HashPtrHead head ;  // .first and .status fields
#ifdef FrHASHTABLE_BIGLINK
	    uint32_t head_int ;
#else
	    uint16_t head_int ;
#endif /* FrHASHTABLE_BIGLINK */
         } ;
      class HashPtr
         {
	 public:
	    Link	next ;	// pointer to next entry in current chain
	    Link	offset ;// this entry's offset in hash bucket (subtract to get bucketnum)
	    HashPtrHead head ;  // .first and .status fields
	 public:
	    static const unsigned lock_bit = 0 ;
	    static const uint8_t lock_mask = (1 << lock_bit) ;
	    static const unsigned reclaimed_bit = 3 ;
	    static const uint8_t reclaimed_mask = (1 << reclaimed_bit) ;
	    static const unsigned deleted_bit = 4 ;
	    static const uint8_t deleted_mask = (1 << deleted_bit) ;
	    static const unsigned inuse_bit = 5 ;
	    static const uint8_t inuse_mask = (1 << inuse_bit) ;
	    static const unsigned copied_bit = 6 ;
	    static const uint8_t copied_mask = (1 << copied_bit) ;
	    static const unsigned stale_bit = 7 ;
	    static const uint8_t stale_mask = (1 << stale_bit) ;
	    void init() { head.first = next = offset = NULLPTR ; head.status = 0 ; }
	    HashPtr() { init() ; }
	    ~HashPtr() {}
	    uint8_t status() const { return cs::load(head.status) ; }
	    const uint8_t *statusPtr() const { return &head.status ; }
	    uint8_t *statusPtr() { return &head.status ; }
	    bool locked() const { return (status() & lock_mask) != 0 ; }
	    bool stale() const { return (status() & stale_mask) != 0 ; }
	    bool copyDone() const { return (status() & copied_mask) != 0 ; }
	    bool markStale() { return cs::testAndSet(head.status,stale_bit) ; }
	    uint8_t markStaleGetStatus() { return cs::testAndSetMask(head.status,stale_mask) ; }
	    bool markCopyDone() { return cs::testAndSet(head.status,copied_bit) ; }
         } ;

      // encapsulate a hash table entry
      class Entry
         {
	 public: // members
	    if_INTERLEAVED(HashPtr  m_info ;)
	    KeyT     m_key ;
	    ValT     m_value[(sizeof(ValT) < 2) ? 0 : 1] ;
	 public: // methods
	    void *operator new(size_t,void *where) { return where ; }
	    static KeyT copy(KeyT obj)
	       {
		  return obj ? static_cast<KeyT>(obj->deepcopy()) : 0 ;
	       }
	    void markUnused() { m_key = UNUSED() ; }
	    void init()
	       {
		  if_INTERLEAVED(m_info.init() ;)
		  markUnused() ;
		  setValue(0) ; 
	       }
	    void init(const Entry &entry)
	       {
		  if_INTERLEAVED(m_info = entry.m_info ;)
		  m_key = entry.m_key ;
		  if (numValues() > 0) m_value[0] = entry.m_value[0] ; 
	       }

	    // constructors/destructors
	    Entry() { init() ; }
	    ~Entry() { if (isActive()) { markUnused() ; } }

	    // variable-setting functions
	    void setValue(ValT value)
	       { if (numValues() > 0) m_value[0] = value ; }
	    ValT incrCount(ValT incr)
	       { return (numValues() > 0) ? (m_value[0] += incr) : incr ; }
	    ValT atomicIncrCount(ValT incr)
	       { return (numValues() > 0) ? (cs::increment(m_value[0],incr), incr) : incr ; }

	    // access to internal state
	    KeyT getKey() const { return m_key ; }
	    KeyT copyName() const { return copy(m_key) ; }
	    ValT getValue() const { return (numValues() > 0) ? m_value[0] : 0 ; }
	    //const ValT *getValuePtr() const { return (numValues() > 0) ? &m_value[0] : (ValT*)0 ; }
	    ValT *getValuePtr() const { return (numValues() > 0) ? (ValT*)&m_value[0] : (ValT*)0 ; }
#if defined(FrMULTITHREAD)
	    ValT swapValue(ValT new_value)
	       { return cs::swap(getValuePtr(),new_value) ; }
#else
	    ValT swapValue(ValT new_value)
	       {
		  ValT old_value = getValue() ;
		  setValue(new_value) ;
		  return old_value ;
	       }
#endif /* FrMULTITHREAD */
	    _fnattr_always_inline static KeyT UNUSED() {return (KeyT)~0UL ; } 
	    _fnattr_always_inline static KeyT DELETED() {return (KeyT)~1UL ; } 
	    _fnattr_always_inline static KeyT RECLAIMING() { return (KeyT)~2UL ; }
	    bool isUnused() const { return ANNOTATE_UNPROTECTED_READ(m_key) == UNUSED() ; }
	    bool isDeleted() const { return ANNOTATE_UNPROTECTED_READ(m_key) == DELETED() ; }
	    bool isBeingReclaimed() const { return  ANNOTATE_UNPROTECTED_READ(m_key) == RECLAIMING() ; }
	    bool isActive() const { return ANNOTATE_UNPROTECTED_READ(m_key) < RECLAIMING() ; }

	    // I/O
	    ostream &printValue(ostream &output) const
	       {
		  output << m_key ;
		  return output ;
	       }
	    char *displayValue(char *buffer) const
	       {
		  buffer = m_key->print(buffer) ;
		  *buffer = '\0' ;
		  return buffer ;
	       }
	    size_t displayLength() const
	       {
		  return m_key ? m_key->displayLength() : 3 ;
	       }
         } ;

      class Table ; // forward declaration
      class ScopedChainLock
         {
	 private:
#ifdef FrMULTITHREAD
	    Table		*m_table ;
	    size_t		 m_pos ;
	    uint8_t		*m_lock ;
#endif /* FrMULTITHREAD */
	    bool		 m_locked ;
	    bool		 m_stale ;
	 public:
#ifdef FrMULTITHREAD
	    ScopedChainLock(Table *ht, size_t bucketnum)
	       {
		  m_table = ht ;
		  m_pos = bucketnum ;
		  m_lock = ht->bucketPtr(bucketnum)->statusPtr() ;
		  INCR_COUNTstat(chain_lock_count) ;
		  uint8_t oldval = cs::testAndSetMask(*m_lock,(uint8_t)HashPtr::lock_mask) ;
		  if (expected((oldval & HashPtr::lock_mask) == 0))
		     {
		     m_locked = true ;
		     m_stale = (oldval & HashPtr::stale_mask) != 0 ;
		     return ;
		     }
		  INCR_COUNTstat(chain_lock_coll) ;
		  m_locked = false ;
		  m_stale = (oldval & HashPtr::stale_mask) != 0 ;
		  return ;
	       }
	    ~ScopedChainLock()
	       {
		  uint8_t status ;
		  if (m_locked)
		     status = cs::testAndClearMask(*m_lock,(uint8_t)HashPtr::lock_mask) ; 
		  else
		     status = cs::load(*m_lock) ;
		  if (!m_stale && (status & HashPtr::stale_mask) != 0)
		     {
		     // someone else tried to copy the chain while we held the lock,
		     //    so copy it for them now
		     m_table->copyChainLocked(m_pos) ;
		     }
	       }
#else
	    ScopedChainLock(class FrHashTable::Table *, size_t, bool) { m_locked = m_stale = false ; }
	    ~ScopedChainLock() {}
#endif /* FrMULTITHREAD */
	    bool locked() const { return m_locked ; }
	    bool stale() const { return m_stale ; }
	    bool busy() const { return !locked() || stale() ; }
#ifdef FrHASHTABLE_STATS
	    static void clearPerThreadStats() { s_stats.chain_lock_count = s_stats.chain_lock_coll = 0 ; }
	    static size_t numberOfLocks() { return s_stats.chain_lock_count ; }
	    static size_t numberOfLockCollisions() { return s_stats.chain_lock_coll ; }
#else
	    static void clearPerThreadStats() {}
	    static size_t numberOfLocks() { return 0 ; }
	    static size_t numberOfLockCollisions() { return 0 ; }
#endif /* FrHASHTABLE_STATS */
         } ;
      // encapsulate all of the fields which must be atomically swapped at the end of a resize()
      class Table
         {
	 public:
	    size_t	   m_size ;		// capacity of hash array [constant for life of table]
	    size_t	   m_resizethresh ;	// at what entry count should we grow? [constant]
	    Entry         *m_entries ;		// hash array [unchanging]
	    ifnot_INTERLEAVED(HashPtr *m_ptrs ;)// links chaining elements of a hash bucket [unchanging]
	    FrHashTable   *m_container ;	// hash table for which this is the content [unchanging]
	    atomic<size_t> m_currsize ;		// number of active entries
	    atomic<Table*> m_next_table ;	// the table which supersedes us
	    atomic<Table*> m_next_free ;
	    atomic<size_t> m_segments_total ;
	    atomic<size_t> m_segments_assigned ;
	    atomic<size_t> m_first_incomplete ;
	    atomic<size_t> m_last_incomplete ;
	    HashKVFunc    *remove_fn ; 		// invoke on removal of entry/value
	    FrSynchEvent   m_resizestarted ;
	    FrSynchEventCountdown m_resizepending ;
	    atomic<bool>   m_resizelock ;	// ensures that only one thread can initiate a resize
	    atomic<bool>   m_resizedone ;
	 public:
	    void *operator new(size_t sz) { return FrMalloc(sz) ; }
	    void *operator new(size_t, void *where) { return where ; }
	    void operator delete(void *blk) { FrFree(blk) ; }
	    void init(size_t size, double max_fill = FrHashTable_DefaultMaxFill)
	       {
		  m_size = size ;
		  m_currsize.store(0) ;
		  m_next_table.store(0) ;
		  m_next_free.store(0) ;
		  m_entries = 0 ;
		  ifnot_INTERLEAVED(m_ptrs = 0 ;)
		  m_resizelock.store(false) ;
		  m_resizedone.store(false) ;
		  m_resizestarted.clear() ;
		  m_resizepending.clear() ;
		  m_resizethresh = (size_t)(size * max_fill + 0.5) ;
		  size_t num_segs = (m_size + FrHASHTABLE_SEGMENT_SIZE - 1) / FrHASHTABLE_SEGMENT_SIZE ;
		  m_resizepending.init(num_segs) ;
		  m_segments_assigned.store((size_t)0) ;
		  m_segments_total.store(num_segs) ;
		  m_first_incomplete.store(size) ;
		  m_last_incomplete.store(0) ;
		  remove_fn = 0 ;
		  if (size > 0)
		     {
		     m_entries = FrNewN(Entry,size) ;
		     ifnot_INTERLEAVED(m_ptrs = FrNewN(HashPtr,size) ;)
		     if (m_entries
			 ifnot_INTERLEAVED(&& m_ptrs)
			)
			{
			for (size_t i = 0 ; i < size ; i++)
			   {
			   m_entries[i].init() ;
			   ifnot_INTERLEAVED(m_ptrs[i].init() ;)
			   }
			}
		     else
			{
			FrFree(m_entries) ;
			ifnot_INTERLEAVED(FrFree(m_ptrs) ;)
			m_entries = 0 ;
			ifnot_INTERLEAVED(m_ptrs = 0 ;)
			m_size = 0 ;
			}
		     }
		  return ;
	       }
	    Table() : m_currsize(0), m_next_table(0), m_next_free(0),
		      m_segments_total(0), m_segments_assigned(0),
		      m_first_incomplete(~0U), m_last_incomplete(0),
		      m_resizelock(false), m_resizedone(false)
	       {
		  init(0) ;
		  return ;
	       }
	    Table(size_t size, double maxfill)
	       : m_currsize(0), m_next_table(0), m_next_free(0),
		 m_segments_total(0), m_segments_assigned(0),
		 m_first_incomplete(~0U), m_last_incomplete(0),
		 m_resizelock(false), m_resizedone(false)
	       {
		  init(size,maxfill) ;
		  return ;
	       } ;
	    void clear()
	       {
		  FrFree(m_entries) ;
		  ifnot_INTERLEAVED(FrFree(m_ptrs) ;)
		  m_entries = 0 ;
		  ifnot_INTERLEAVED(m_ptrs = 0 ;)
		  m_currsize.store(0) ;
		  m_next_table.store(0) ;
		  m_next_free.store(0) ;
		  return ;
	       }
	    ~Table() { clear() ; }
	    bool good() const { return m_entries != 0 ifnot_INTERLEAVED(&& m_ptrs != 0) && m_size > 0 ; }
	    bool superseded() const { return m_next_table.load() != 0 ; }
	    bool resizingDone() const { return m_resizedone.load() ; }
	    Table *next() const { return m_next_table.load() ; }
	    Table *nextFree() const { return m_next_free.load() ; }
	    KeyT getKey(size_t N) const { return cs::load(m_entries[N].m_key) ; }
	    KeyT getKeyNonatomic(size_t N) const { return ANNOTATE_UNPROTECTED_READ(m_entries[N].m_key) ; }
	    void setKey(size_t N, KeyT newkey) { cs::store(m_entries[N].m_key,newkey) ; }
	    ValT getValue(size_t N) const { return m_entries[N].getValue() ; }
	    ValT *getValuePtr(size_t N) const { return m_entries[N].getValuePtr() ; }
	    void setValue(size_t N, ValT value) { m_entries[N].setValue(value) ; }
	    bool updateKey(size_t N, KeyT expected, KeyT desired)
	       { return cs::compareAndSwap(&m_entries[N].m_key,expected,desired) ; }
	    bool activeEntry(size_t N) const { return m_entries[N].isActive() ; }
	    bool deletedEntry(size_t N) const { return m_entries[N].isDeleted() ; }
	    bool unusedEntry(size_t N) const { return m_entries[N].isUnused() ; }
#ifndef FrHASHTABLE_INTERLEAVED_ENTRIES
	    HashPtr *bucketPtr(size_t N) const { return &m_ptrs[N] ; }
#else
	    HashPtr *bucketPtr(size_t N) const { return &m_entries[N].m_info ; }
#endif /* FrHASHTABLE_INTERLEAVED_ENTRIES */
	    bool chainIsStale(size_t N) const { return bucketPtr(N)->stale() ; }
	    bool chainCopied(size_t N) const { return bucketPtr(N)->copyDone() ; }
	    void copyEntry(size_t N, const Table *othertab)
	       {
		  m_entries[N].init(othertab->m_entries[N]) ;
	          ifnot_INTERLEAVED(m_ptrs[N] = othertab->m_ptrs[N] ;)
		  return ;
	       }
	    static size_t normalizeSize(size_t sz)
	       {
		  if (sz < FrHASHTABLE_MIN_SIZE)
		     sz = FrHASHTABLE_MIN_SIZE ;
		  // bump up the size a little to avoid multiples of small
		  //   primes like 2, 3, 5, 7, 11, 13, and 17
		  if (sz % 2 == 0)
		     ++sz ;
		  size_t pre_bump ;
		  do {
		  pre_bump = sz ;
		  for (size_t i = 0 ; FramepaC_small_primes[i] ; ++i)
		     {
		     if (sz % FramepaC_small_primes[i] == 0)
			sz += 2 ;
		     }
		  } while (sz != pre_bump) ;
		  return sz ;
	       }
	 protected:
#ifdef FrMULTITHREAD
	    void announceTable() { cs::store(FrHashTable::s_table,this) ; }
	    static void announceBucketNumber(size_t bucket) { cs::store(FrHashTable::s_bucket,bucket) ; }
	    static void unannounceBucketNumber() { FrHashTable::s_bucket = ~0UL ; }
#else
	    static _fnattr_always_inline void announceTable() {}
	    static _fnattr_always_inline void announceBucketNumber(size_t) {}
	    static _fnattr_always_inline void unannounceBucketNumber() {}
#endif /* FrMULTITHREAD */
	    Link chainHead(size_t N) const { return ANNOTATE_UNPROTECTED_READ(bucketPtr(N)->head.first) ; }
	    Link *chainHeadPtr(size_t N) const { return &bucketPtr(N)->head.first ; }
	    Link chainNext(size_t N) const { return ANNOTATE_UNPROTECTED_READ(bucketPtr(N)->next) ; }
	    Link *chainNextPtr(size_t N) const { return &bucketPtr(N)->next ; }
	    Link chainOwner(size_t N) const { return cs::load(bucketPtr(N)->offset) ; }
	    void setChainNext(size_t N, Link nxt) { cs::store(bucketPtr(N)->next,nxt) ; }
	    void setChainOwner(size_t N, Link ofs) { cs::store(bucketPtr(N)->offset,ofs) ; }
	    void markCopyDone(size_t N) { bucketPtr(N)->markCopyDone() ; }
	    size_t bucketContaining(size_t N) const
	       { Link owner = chainOwner(N) ; return (owner == NULLPTR) ? NULLPOS : N - owner ; }
	    size_t resizeThreshold(size_t sz)
	       {
		  size_t thresh = (size_t)(sz * m_container->m_maxfill + 0.5) ;
		  return (thresh >= sz) ? sz-1 : thresh ;
	       }
	    size_t sizeForCapacity(size_t capacity) const
	       {
		  return (size_t)(capacity / m_container->m_maxfill + 0.99) ;
	       }
	    void autoResize()
	       {
		  size_t currsize = m_currsize.load() ;
		  if (currsize <= m_resizethresh)
		     {
		     // force an increase if we ran out of free slots
		     //   within range of a bucket
		     size_t sz = m_size ;
#ifdef FrSAVE_MEMORY
		     if (sz < 100000)
			currsize = 2.0*sz ;
		     else if (sz < 1000000)
			currsize = 1.5*sz ;
		     else if (sz < 10*1000*1000)
			currsize = 1.4*sz ;
		     else if (sz < 100*1000*1000)
			currsize = 1.3*sz ;
		     else
			currsize = 1.2*sz ;
#else
		     if (sz < 16*1000*1000)
			currsize = 2.0*sz ;
		     else
			currsize = 1.5*sz ;
#endif /* FrSAVE_MEMORY */
		     }
		  else
		     {
		     currsize *= 2 ;
		     }
		  resize(currsize,true) ;
		  return ;
	       }
	    bool bucketsInUse(size_t startbucket, size_t endbucket) const
	       {
#ifdef FrMULTITHREAD
		  // can only have concurrent use when multi-threaded
		  // scan the list of per-thread s_table variables
		  for (const TablePtr *tables = cs::load(s_thread_entries) ; tables ; tables = tables->m_next)
		     {
		     // check whether the hazard pointer is for ourself
		     const Table *ht = tables->table() ;
		     if (this != ht)
			continue ;
		     // check whether it's for a bucket of interest
		     size_t bucket = tables->bucket() ;
		     if (bucket >= startbucket && bucket < endbucket)
			return true ;
		     }
#endif
		  return false ; 
	       }
	 public:
	    void awaitIdle(size_t bucketnum, size_t endpos)
	       {
		  // wait until nobody is announcing that they are
		  //   using one of our hash buckets
#ifdef FrMULTITHREAD
		  debug_msg("await idle %ld\n",my_job_id);
		  size_t loops = 0 ;
		  while (bucketsInUse(bucketnum,endpos))
		     {
		     thread_backoff(loops) ;
		     if (superseded())
			{
			debug_msg(" idle wait canceled %ld\n",my_job_id) ;
			return ;
			}
		     }
		  debug_msg("is idle %ld\n",my_job_id);
#endif /* FrMULTITHREAD */
	          (void)bucketnum ;
	          (void)endpos ;
		  return ;
	       }
	 protected:
	    void removeValue(Entry &element) const
	       {
	          ValT value = element.swapValue((ValT)0) ;
		  if (remove_fn && value)
		     {
		     remove_fn(element.getKey(),value) ;
		     }
		  return ;
	       }
	 public:
	    bool copyChainLocked(size_t bucketnum) _fnattr_hot
	       {
		  Link offset ;
#ifdef FrMULTITHREAD
		  // since add() can still run in parallel, we need to
		  //   start by making any deleted entries in the
		  //   chain unusable by add(), so that it doesn't
		  //   re-use them after we've skipped them during the
		  //   copy loop
		  offset = chainHead(bucketnum) ;
		  while (NULLPTR != offset)
		     {
		     size_t pos = bucketnum + offset ;
		     offset = chainNext(pos) ;
		     (void)updateKey(pos,Entry::DELETED(),Entry::RECLAIMING()) ;
		     }
		  // the chain is now frozen -- add() can't push to
		  //   the start or re-use anything on the chain
#endif
		  // insert all elements of the current hash bucket into the next table
		  offset = chainHead(bucketnum) ;
		  while (NULLPTR != offset)
		     {
		     size_t pos = bucketnum + offset ;
		     offset = chainNext(pos) ;
		     // insert the element, ignoring it if it is deleted or a duplicate
		     if (!activeEntry(pos))
			{
			continue ;
			}
		     KeyT key = getKey(pos) ;
		     size_t hashval = hashValFull(key) ;
		     Entry *element = &m_entries[pos] ;
		     if (next()->reAdd(hashval,key,element->getValue()))
			{
			// duplicate, apply removal function to the value
			removeValue(*element) ;
			}
		     }
		  // we're done copying the bucket, let anybody who is waiting on the
		  //   copy proceed in the new table
		  markCopyDone(bucketnum) ;
		  return true ;
	       }
	 protected:
	    bool copyChain(size_t bucketnum) _fnattr_hot
	       {
#ifdef FrMULTITHREAD
		  HashPtr *bucket = bucketPtr(bucketnum) ;
		  // atomically set the 'stale' bit and get the current status
		  uint8_t status = bucket->markStaleGetStatus() ;
		  if ((status & (HashPtr::stale_mask | HashPtr::lock_mask)) != 0)
		     {
		     if (status & HashPtr::lock_mask) { INCR_COUNTstat(chain_lock_coll) ; }
		     // someone else has already worked on this
		     //   bucket, or a copy/recycle()/hopscotch() is
		     //   currently running, in which case that thread
		     //   will do the copying for us
		     return bucket->copyDone() ;
		     }
#endif /* FrMULTITHREAD */
		  return copyChainLocked(bucketnum) ;
	       }
	    void waitUntilCopied(size_t bucketnum)
	       {
		  size_t loops = 0 ;
		  while (!chainCopied(bucketnum))
		     {
		     thread_backoff(loops) ;
		     }
		  return  ;
	       }
	    // copy the chains for buckets 'bucketnum' to 'endpos',
	    //   inclusive, and wait to ensure that all copying is
	    //   complete even if there are interfering operations in
	    //   progress
	    void copyChains(size_t bucketnum, size_t endpos) _fnattr_hot
	       {
		  bool complete = true ;
		  size_t first_incomplete = endpos ;
		  size_t last_incomplete = bucketnum ;
		  for (size_t i = bucketnum ; i <= endpos ; ++i)
		     {
		     if (!copyChain(i))
			{
			last_incomplete = i ;
			if (complete)
			   {
			   complete = false ;
			   first_incomplete = i ;
			   }
			}
		     }
		  // if we had to skip copying any chains due to a
		  //    concurrent recycle() or hopscotch(), tell the
		  //    lead resizing thread that it need to do a
		  //    cleanup pass
		  if (!complete)
		     {
		     size_t old_first ;
		     do 
			{
			old_first = m_first_incomplete.load() ;
			if (first_incomplete >= old_first)
			   break ;
			} while (!m_first_incomplete.compare_exchange_strong(old_first,first_incomplete)) ;
		     size_t old_last ;
		     do
			{
			old_last = m_last_incomplete.load() ;
			if (last_incomplete <= old_last)
			   break ;
			} while (!m_last_incomplete.compare_exchange_strong(old_last,last_incomplete)) ;
		     }
		  return ;
	       }
#if defined(FrMULTITHREAD)
#define FORWARD(delegate,tab,counter)					\
	    Table *tab = next() ;					\
	    if (tab /*&& chainIsStale(bucketnum)*/)			\
	       {							\
	       /* ensure that our bucket has been copied to 	*/	\
	       /*   the successor table, then add the key to	*/	\
	       /*   that table					*/	\
	       resizeCopySegments() ;					\
	       waitUntilCopied(bucketnum) ;				\
	       INCR_COUNT(counter) ;					\
	       tab->announceTable() ;					\
	       return tab->delegate ;					\
	       }							
#define FORWARD_IF_COPIED(delegate,counter)					\
	    Table *nexttab = next() ;						\
	    if (nexttab)							\
	       {								\
	       /* if our bucket has been fully copied to the	*/ 		\
	       /*   successor table, look for the key in that	*/		\
	       /*   table instead of the current one		*/ 		\
	       /* FIXME: do we want to do any copying to help out the resize?*/ \
	       if (chainCopied(bucketnum))					\
		  {								\
		  INCR_COUNT(counter) ;						\
		  nexttab->announceTable() ;					\
		  return nexttab->delegate ;					\
		  }								\
	       }
#define FORWARD_IF_STALE(delegate,counter)					\
	    Table *nexttab = next() ;						\
	    if (nexttab)							\
	       {								\
	       /* if our bucket has at least started to be copied to	*/	\
	       /*   the successor table, look for the key in that	*/	\
	       /*   table instead of the current one			*/ 	\
	       /* FIXME: do we want to do any copying to help out the resize?*/ \
	       if (chainIsStale(bucketnum))					\
		  {								\
		  INCR_COUNT(counter) ;						\
		  waitUntilCopied(bucketnum) ;					\
		  nexttab->announceTable() ;					\
		  return nexttab->delegate ;					\
		  }								\
	       }
#else
#define FORWARD(delegate,tab,counter)					\
	    Table *tab = next() ;					\
	    if (tab /*&& chainIsStale(bucketnum)*/)			\
	       {							\
	       return tab->delegate ;					\
	       }							
#define FORWARD_IF_COPIED(delegate,counter)
#define FORWARD_IF_STALE(delegate,counter)
#endif /* FrMULTITHREAD */
	    // a separate version of add() for resizing, because FrSymbolTable needs to
	    //   look at the full key->hashValue(), not just 'key' itself
	    bool reAdd(size_t hashval, KeyT key, ValT value = 0)
	       {
		  INCR_COUNT(insert_resize) ;
		  DECR_COUNT(insert_attempt) ; // don't count as a retry unless we need more than one attempt
		  size_t bucketnum = hashval % m_size ;
		  while (true)
		     {
		     FORWARD(reAdd(hashval,key,value),nexttab,insert_forwarded) ;
		     // tell others we're using the bucket
		     announceBucketNumber(bucketnum) ;
		     // scan the chain of items for this hash position
		     Link offset = chainHead(bucketnum) ;
		     Link firstoffset = offset ;
		     while (NULLPTR != offset)
			{
			size_t pos = bucketnum + offset ;
			KeyT key_at_pos = getKey(pos) ;
			if (isEqualFull(key,key_at_pos))
			   {
#if FrHASHTABLE_VERBOSITY > 1
			   cerr << "reAdd skipping duplicate " << key << " [" << hex << (size_t)key
				<< "] - " << key_at_pos << " [" << ((size_t)key_at_pos)
				<< "]" << dec << endl ;
#endif /* FrHASHTABLE_VERBOSITY > 1 */
			   // found existing entry!
			   INCR_COUNT(insert_dup) ;
			   unannounceBucketNumber() ;
			   return true ;		// item already exists
			   }
			// advance to next item in chain
			offset = chainNext(pos) ;
			}
		     // when we get here, we know that the item is not yet in the
		     //   hash table, so try to add it
		     if (insertKey(bucketnum,firstoffset,key,value))
			break ;
		     }
		  unannounceBucketNumber() ;
		  return false ;		// item did not already exist
	       }

	    bool claimEmptySlot(size_t pos, KeyT key) _fnattr_hot
	       {
#ifdef FrMULTITHREAD
		  if (getKeyNonatomic(pos) == Entry::UNUSED())
		     {
		     // check if someone stole the free slot out from under us
		     return updateKey(pos,Entry::UNUSED(),key) ;
		     }
#else
		  if (unusedEntry(pos))
		     {
		     setKey(pos,key) ;
		     return true ;
		     }
#endif /* FrMULTITHREAD */
		  return false ;
	       }
	    Link locateEmptySlot(size_t bucketnum, KeyT key, bool &got_resized) _fnattr_hot
	       {
		  if (superseded())
		     {
		     // a resize snuck in, so caller MUST retry
		     got_resized = true ;
		     return NULLPTR ;
		     }
		  // is the given position free?
		  if (expected(claimEmptySlot(bucketnum,key)))
		     return 0 ;
		  size_t sz = m_size ;
		  // compute the extent of the cache line containing
		  //   the offset-0 entry for the bucket
		  uintptr_t CLstart_addr = ((uintptr_t)(bucketPtr(bucketnum))) & ~(Fr_cacheline_size-1) ;
		  if (CLstart_addr < (uintptr_t)bucketPtr(0))
		     CLstart_addr = (uintptr_t)bucketPtr(0) ;
		  size_t itemsize = (uintptr_t)bucketPtr(1) - (uintptr_t)bucketPtr(0) ;
	          size_t CL_start = (CLstart_addr - (uintptr_t)bucketPtr(0)) / itemsize ;
	          size_t CL_end = (CLstart_addr - (uintptr_t)bucketPtr(0) + Fr_cacheline_size) / itemsize ;
		  // search for a free slot on the same cache line as the bucket header
		  for (size_t i = CL_start ; i < CL_end && i < sz ; ++i)
		     {
		     if (i == bucketnum)
			continue ;
		     if (claimEmptySlot(i,key))
			return (i - bucketnum) ;
		     }
		  // extended search for a free spot following the given position
		  size_t max = bucketnum + searchrange ;
		  if (max >= sz)
		     {
		     max = sz - 1 ;
		     }
		  for (size_t i = max ; i >= CL_end  ; --i)
		     {
		     if (claimEmptySlot(i,key))
			return (i - bucketnum) ;
		     }
		  // search for a free spot preceding the given position
		  size_t min = bucketnum >= (size_t)searchrange ? bucketnum - searchrange : 0 ;
		  for (size_t i = min ; i < CL_start ; ++i)
		     {
		     if (claimEmptySlot(i,key))
			return (i - bucketnum) ;
		     }
		  if (superseded())
		     {
		     // a resize snuck in, so caller MUST retry
		     got_resized = true ;
		     return NULLPTR ;
		     }
		  INCR_COUNT(neighborhood_full) ;
		  // if we get here, we didn't get a free slot, so
		  //    try recycling a deleted slot in the search
		  //    window
		  size_t recycled = recycleDeletedEntry(bucketnum,key) ;
		  if (NULLPOS != recycled)
		     {
		     return recycled - bucketnum ;
		     }
		  // that still didn't get us a slot, so try moving
		  //    an entry on someone else's chain to a free
		  //    slot outside our reach
		  size_t hopped = hopscotch(bucketnum) ;
		  if (NULLPOS != hopped)
		     {
		     setKey(hopped,key) ;
		     return hopped - bucketnum ;
		     }
		  // if we get here, there were no free slots available, and
		  //   none could be made
		  return NULLPTR ;
               }

	    bool insertKey(size_t bucketnum, Link firstptr, KeyT key, ValT value) _fnattr_hot
	       {
		  if (superseded())
		     {
		     // a resize snuck in, so retry
		     return false ;
		     }
	          if (unlikely(m_currsize.load() > m_resizethresh))
		     {
		     autoResize() ;
		     return false ;
		     }
		  INCR_COUNT(insert_attempt) ;
		  bool got_resized = false ;
		  Link offset = locateEmptySlot(bucketnum,key,got_resized) ;
		  if (unlikely(NULLPTR == offset))
		     {
		     if (!got_resized)
			autoResize() ;
		     return false ;
		     }
		  // fill in the slot we grabbed and link it to the head of
		  //   the chain for the hash bucket
		  size_t pos = bucketnum + offset ;
		  setValue(pos,value) ;
		  setChainOwner(pos,offset) ;
#ifdef FrMULTITHREAD
		  setChainNext(pos,firstptr) ;
		  // now that we've done all the preliminaries, try to get
		  //   write access to actually insert the new entry
		  // try to point the hash chain at the new entry
		  HashPtrHead expected_head ;
		  HashPtrHead new_head ;
		  expected_head.first = firstptr ;
		  expected_head.status = 0 ;
		  new_head.first = offset ;
		  new_head.status = 0 ;
		  // we need to "cast" a bit of black magic here so that we
		  //   can CAS both the .first and .status fields in the
		  //   HashPtr, since we require not only that the link
		  //   be unchanged, but that the chain be neither stale
		  //   nor locked
		  HashPtrInt *headptr = (HashPtrInt*)&bucketPtr(bucketnum)->head ;
		  if (unlikely(!cs::compareAndSwap(&headptr->head_int,
						   *((uint16_t*)&expected_head),
						   *((uint16_t*)&new_head))))
		     {
		     // oops, someone else messed with the hash chain, which
		     //   means there could have been a parallel insert of
		     //   the same value, or a resize is in progress and
		     //   copied the bucket while we were working
		     // release the slot we grabbed and tell caller to retry
		     setKey(pos,Entry::UNUSED()) ;
		     INCR_COUNT(CAS_coll) ;
		     debug_msg("insertKey: CAS collision\n") ;
		     return false ;
		     }
#else
		  *chainNextPtr(pos) = firstptr ;
		  // life is much simpler in non-threaded mode: just point
		  //   the chain head at the new node and increment the
		  //   tally of items in the table
		  *chainHeadPtr(bucketnum) = offset ;
#endif /* FrMULTITHREAD */
		  m_currsize++ ;
		  return true ;
	       }
	    void resizeCopySegment(size_t segnum) _fnattr_hot
	       {
		  size_t bucketnum = segnum * FrHASHTABLE_SEGMENT_SIZE ;
		  size_t endpos = (segnum + 1) * FrHASHTABLE_SEGMENT_SIZE ;
		  if (endpos > m_size)
		     endpos = m_size ;
		  copyChains(bucketnum,endpos-1) ;
		  // record the fact that we copied a segment
		  m_resizepending.consume() ;
		  return ;
	       }
	    void resizeCopySegments(size_t max_segs = ~0UL) _fnattr_hot
	       {
		  // is there any work available to be stolen?
		  if (!m_resizelock.load() || m_resizedone.load())
		     return ;
		  // grab the current segment number and increment it
		  //   so the next thread gets a different number; stop
		  //   once all segments have been assigned
		  while (max_segs > 0 && m_segments_assigned.load() < m_segments_total.load())
		     {
		     size_t segnum ;
		     if ((segnum = m_segments_assigned++) < m_segments_total.load())
			{
			resizeCopySegment(segnum) ;
			--max_segs ;
			}
		     }
		  return ;
	       }
            void clearDuplicates(size_t bucketnum)
	       {
		  // scan down the chain for the given hash bucket, marking
		  //   any duplicate entries for a key as deleted
		  announceBucketNumber(bucketnum) ;
		  Link offset = chainHead(bucketnum) ;
		  while (NULLPTR != offset)
		     {
		     size_t pos = bucketnum + offset ;
		     offset = chainNext(pos) ;
		     KeyT currkey = getKey(pos) ;
		     if (!isActive(currkey))
			continue ;
		     Link nxt = offset ;
		     while (NULLPTR != nxt)
			{
			size_t nextpos = bucketnum + nxt ;
			nxt = chainNext(nextpos) ;
			KeyT nextkey = getKey(nextpos) ;
			if (isActive(nextkey) && isEqualFull(currkey,nextkey))
			   {
			   // duplicate, apply removal function to the value
			   removeValue(m_entries[nextpos]) ;
			   setKey(nextpos,Entry::DELETED()) ;
			   }
			}
		     }
		  unannounceBucketNumber() ;
		  return ;
	       }
	    bool relocateEntry(size_t from, size_t to, size_t bucketnum)
	       {
		  announceBucketNumber(bucketnum) ;
		  // copy the contents of the 'from' entry to the 'to' entry
		  setKey(to,getKey(from)) ;
		  setValue(to,getValue(from)) ;
		  setChainNext(to,chainNext(from)) ;
//FIXME
		  INCR_COUNT(move) ;
		  // try to swap the predecessor link for 'from' over to 'to'

		  unannounceBucketNumber() ;
		  return false ; 
	       }
	    size_t hopscotchChain(size_t bucketnum, size_t entrynum)
	       {
		  // try to move an entry within the search range of
		  //   'bucketnum' which is contained in another
		  //   bucket's chain to a position outside the search
		  //   range
#if defined(FrMULTITHREAD) && 0
		  // the following lock serializes to allow only one
		  //   recycle() or hopscotch() on the chain at a time;
		  //   concurrent add() and remove() are OK
		  ScopedChainLock lock(this,bucketnum) ;
		  if (lock.busy())
		     {
		     // someone else is currently reclaiming the chain or
		     //   moving elements to make room, so we can just wait
		     //   for them to finish and then claim success ourselves
		     INCR_COUNT(chain_lock_coll) ;
		     return NULLPOS ;
		     }
#endif /* FrMULTITHREAD */
		  // find an entry within the search range of
		  //   'bucketnum' which is in the chain for a bucket
		  //   that can reach 'entrynum' and move it there
		  if (entrynum > bucketnum)
		     {
		     size_t max = bucketnum + searchrange ;
		     if (max >= m_size)
			max = m_size - 1 ;
		     for (size_t i = max ; i >= bucketnum ; --i)
			{
			size_t buck = bucketContaining(i) ;
			if (NULLPOS == buck || buck + searchrange < entrynum || entrynum + searchrange < buck )
			   continue ;
			if (relocateEntry(i,entrynum,buck))
			   return i ;
			}
		     }
		  else
		     {
		     size_t min = bucketnum > (size_t)searchrange ? bucketnum-searchrange : 0 ;
		     for (size_t i = min ; i <= bucketnum ; ++i)
			{
			size_t buck = bucketContaining(i) ;
			if (NULLPOS == buck || buck + searchrange < entrynum || entrynum + searchrange < buck )
			   continue ;
			if (relocateEntry(i,entrynum,buck))
			   return i ;
			}
		     }
		  return NULLPOS ;
	       }
	    size_t hopscotch(size_t bucketnum)
	       {
return NULLPOS;//FIXME
		  size_t max = bucketnum + 2 * searchrange ;
		  if (max > m_size)
		     max = m_size ;
		  for (size_t i = bucketnum + searchrange ; i < max ; ++i)
		     {
		     if (claimEmptySlot(i,Entry::RECLAIMING()))
			{
			size_t slot = hopscotchChain(bucketnum,i) ;
			if (slot != NULLPOS)
			   {
			   return slot ;
			   }
			else
			   {
			   // release the slot we grabbed, since we
			   //   were unable to use it
			   setKey(i,Entry::UNUSED()) ;
			   }
			}
		     }
		  size_t min = 0 ;
		  if (bucketnum > 2 * searchrange)
		     {
		     min = bucketnum - 2 * searchrange ;
		     }
		  for (size_t i = min ; i + searchrange < bucketnum ; ++i)
		     {
		     if (claimEmptySlot(i,Entry::RECLAIMING()))
			{
			size_t slot = hopscotchChain(bucketnum,i) ;
			if (slot != NULLPOS)
			   {
			   return slot ;
			   }
			else
			   {
			   // release the slot we grabbed, since we
			   //   were unable to use it
			   setKey(i,Entry::UNUSED()) ;
			   }
			}
		     }
		  return NULLPOS ;
	       }

	    bool unlinkEntry(size_t entrynum)
	       {
#ifdef FrMULTITHREAD
		  // the following lock serializes to allow only one
		  //   recycle() or hopscotch() on the chain at a time;
		  //   concurrent add() and remove() are OK
		  size_t bucketnum = bucketContaining(entrynum) ;
		  if (NULLPOS == bucketnum)
		     return true ;
		  ScopedChainLock lock(this,bucketnum) ;
		  if (lock.busy())
		     {
		     return false ; // need to retry
		     }
		  announceBucketNumber(bucketnum) ;
		  Link *prevptr = chainHeadPtr(bucketnum) ;
		  Link offset = cs::load(*prevptr) ;
		  while (NULLPTR != offset)
		     {
		     size_t pos = bucketnum + offset ;
		     Link *nextptr = chainNextPtr(pos) ;
		     if (pos == entrynum)
			{
			// we found the entry, now try to unlink it
			Link nxt = cs::load(*nextptr) ;
			if (unlikely(!cs::compareAndSwap(prevptr,offset,nxt)))
			   {
			   // uh oh, someone else messed with the chain!
			   // restart from the beginning of the chain
			   INCR_COUNT(CAS_coll) ;
			   prevptr = chainHeadPtr(bucketnum) ;
			   offset = cs::load(*prevptr) ;
			   continue ;
			   }
			// mark the entry as not belonging to any chain anymore
			setChainOwner(pos,NULLPTR) ;
			unannounceBucketNumber() ;
			return true ;
			}
		     else
			{
			prevptr = nextptr ;
			}
		     offset = cs::load(*nextptr) ;
		     }
		  unannounceBucketNumber() ;
#endif /* FrMULTITHREAD */
		  return true ;
	       }
	    size_t recycleDeletedEntry(size_t bucketnum, KeyT new_key)
	       {
#ifdef FrMULTITHREAD
		  if (superseded())
		     return NULLPOS ;
		  debug_msg("recycledDeletedEntry (thr %ld)\n",my_job_id) ;
		  // figure out the range of hash buckets to process
		  size_t endpos = bucketnum + searchrange ;
		  if (endpos > m_size)
		     endpos = m_size ;
		  bucketnum = bucketnum >= (size_t)searchrange ? bucketnum - searchrange : 0 ;
		  size_t claimed = NULLPOS ;
		  for (size_t i = bucketnum ; i < endpos ; ++i)
		     {
		     if (deletedEntry(i))
			{
			// try to grab the entry
			if (updateKey(i,Entry::DELETED(),Entry::RECLAIMING()))
			   {
			   claimed = i ;
			   break ;
			   }
			}
		     }
		  if (claimed == NULLPOS)
		     return NULLPOS ;	// unable to find an entry to recycle
		  INCR_COUNT(reclaim) ;
		  // we successfully grabbed the entry, so
		  //   now we need to chop it out of the
		  //   chain currently containing it
		  size_t claimed_bucketnum = bucketContaining(claimed) ;
		  size_t loops = 0 ;
		  while (!unlinkEntry(claimed))
		     {
		     thread_backoff(loops) ;
		     }
		  if (!superseded())
		     {
		     // ensure that any concurrent accesses complete
		     //   before we actually recycle the entry
		     awaitIdle(claimed_bucketnum,claimed_bucketnum+1) ;
		     if (!superseded())
			{
			setKey(claimed,new_key) ;
			return claimed ;
			}
		     }
#else
		  // when single-threaded, we chop out deletions
		  //   immediately, so there is nothing to reclaim
		  (void)bucketnum ;
		  (void)new_key ;
#endif /* FrMULTITHREAD */
		  return NULLPOS ;
	       }
	    bool reclaimChain(size_t bucketnum, size_t &min_reclaimed, size_t &max_reclaimed)
	       {
#ifdef FrMULTITHREAD
		  // the following lock serializes to allow only one
		  //   recycle()/reclaim() or hopscotch() on the chain at a time;
		  //   concurrent add() and remove() are OK
		  ScopedChainLock lock(this,bucketnum) ;
		  if (lock.busy())
		     {
		     // someone else is currently reclaiming the chain,
		     //   moving elements to make room, or copying the
		     //   bucket to a new hash table; so we can just
		     //   claim success ourselves
		     return true ;
		     }
		  Link *prevptr = chainHeadPtr(bucketnum) ;
		  Link offset = cs::load(*prevptr) ;
		  bool reclaimed = false ;
		  while (NULLPTR != offset)
		     {
		     size_t pos = bucketnum + offset ;
		     Link *nextptr = chainNextPtr(pos) ;
		     // while the additional check of the key just below is not strictly necessary, it
		     //   permits us to avoid the expensive CAS except when an entry is actually deleted
		     KeyT key = getKey(pos) ;
		     Link nxt = cs::load(*nextptr) ;
		     if (key == Entry::DELETED() && updateKey(pos,Entry::DELETED(),Entry::RECLAIMING()))
			{
			// we grabbed a deleted entry; now try to unlink it
			if (unlikely(!cs::compareAndSwap(prevptr,offset,nxt)))
			   {
			   // uh oh, someone else messed with the chain!
			   setKey(pos,Entry::DELETED()) ;
			   // restart from the beginning of the chain
			   INCR_COUNT(CAS_coll) ;
			   prevptr = chainHeadPtr(bucketnum) ;
			   offset = cs::load(*prevptr) ;
			   continue ;
			   }
			// mark the entry as not belonging to any chain anymore
			setChainOwner(pos,NULLPTR) ;
			reclaimed = true ;
			if (pos < min_reclaimed)
			   min_reclaimed = pos ;
			if (pos > max_reclaimed)
			   max_reclaimed = pos ;
			}
		     else
			{
			prevptr = nextptr ;
			}
		     offset = cs::load(*nextptr) ;
		     }
		  return reclaimed;
#else
		  (void)bucketnum ;
		  // remove() chops out the deleted entries, so there's never anything to reclaim
		  return false ;
#endif /* !FrMULTITHREAD */
	       }
	    bool assistResize()
   	       {
		  // someone else has already claimed the right to
		  //   resize, so wait until the resizing starts and
		  //   then help them out
		  INCR_COUNT(resize_assist) ;
		  m_resizestarted.wait() ;
		  resizeCopySegments() ;
		  // there's no need to synchronize with the
		  //   completion of the resizing, since we'll
		  //   transparently forward from the older version
		  //   as required
		  return true ;
	       }

	 public:
	    bool resize(size_t newsize, bool enlarge_only = false)
	       {
		  if (superseded())
		     return false ;
		  size_t currsize = m_currsize.load() ;
		  newsize = normalizeSize(newsize) ;
		  if (resizeThreshold(newsize) < currsize)
		     newsize = normalizeSize(sizeForCapacity(currsize+1)) ;
		  if (newsize == m_size || (enlarge_only && newsize < 1.1*m_size))
		     {
		     debug_msg("resize canceled (thr %ld)\n",my_job_id) ;
		     return false ;
		     }
		  if (unlikely(m_resizelock.exchange(true)))
		     {
		     return assistResize() ;
		     }
		  // OK, we've won the right to run the resizing
		  INCR_COUNT(resize) ;
		  if (enlarge_only && currsize <= m_resizethresh)
		     {
		     warn_msg("contention-resize to %ld (currently %ld/%ld - %4.1f%%), thr %ld\n",
			      newsize,currsize,m_size,100.0*currsize/m_size,my_job_id) ;
		     }
		  else
		     {
		     debug_msg("resize to %ld from %ld/%ld (%4.1f%%) (thr %ld)\n",
			       newsize,currsize,m_size,100.0*currsize/m_size,my_job_id) ;
		     }
		  Table *newtable = m_container->allocTable() ;
		  newtable->init(newsize,m_container->m_maxfill) ;
		  newtable->remove_fn = m_container->onRemoveFunc() ;
		  cs::store(newtable->m_container,m_container) ;
		  bool success = true ;
		  if (expected(newtable->good()))
		     {
		     // link in the new table; this also makes
		     //   operations on the current table start to
		     //   forward to the new one
		     m_next_table.store(newtable) ;
		     m_resizestarted.set() ;
		     // grab as many segments as we can and copy them
		     resizeCopySegments() ;
		     // wait for any other threads that have grabbed
		     //   segments to complete them
		     m_resizepending.wait() ;
		     // if necessary, do a cleanup pass to copy any
		     //   buckets which had to be skipped due to
		     //   concurrent reclaim/hopscotch/remove() calls
		     size_t first_incomplete = m_first_incomplete.load() ;
		     size_t last_incomplete = m_last_incomplete.load() ;
		     size_t loops = FrSPIN_COUNT ;
		     while (first_incomplete <= last_incomplete)
			{
			INCR_COUNT(resize_cleanup) ;
			bool complete = true ;
			size_t first = m_size ;
			size_t last = 0 ;
			for (size_t i = first_incomplete ; i < last_incomplete && i < m_size ; ++i)
			   {
			   if (!chainCopied(i) && !copyChain(i))
			      {
			      last = i ;
			      if (complete)
				 {
				 first = i ;
				 complete = false ;
				 }
			      }
			   }
			if (complete)
			   break ;
			first_incomplete = first ;
			last_incomplete = last ;
			thread_backoff(loops) ;
			}
		     m_resizedone.store(true) ;
		     // make the new table the current one for the containing FrHashTable
		     m_container->updateTable() ;
		     debug_msg(" resize done (thr %ld)\n",my_job_id) ;
		     }
		  else
		     {
		     FrWarning("unable to resize hash table--will continue") ;
		     // bump up the resize threshold so that we can
		     //   (hopefully) store a few more before
		     //   (hopefully successfully) retrying the resize
		     m_resizethresh += (m_size - m_resizethresh)/2 ;
		     m_container->releaseTable(newtable) ;
		     success = false ;
		     }
		  return success ;
	       }

	    bool add(size_t hashval, KeyT key, ValT value = 0) _fnattr_hot
	       {
		  INCR_COUNT(insert) ;
		  size_t bucketnum = hashval % m_size ;
		  while (true)
		     {
		     FORWARD(add(hashval,key,value),nexttab,insert_forwarded) ;
		     // scan the chain of items for this hash position
		     if_MULTITHREAD(size_t deleted = NULLPOS ;)
		     // tell others we're using the bucket
		     announceBucketNumber(bucketnum) ;
		     Link offset = chainHead(bucketnum) ;
		     Link firstoffset = offset ;
		     while (NULLPTR != offset)
			{
			size_t pos = bucketnum + offset ;
			KeyT key_at_pos = getKey(pos) ;
			if (isEqual(key,key_at_pos))
			   {
			   // found existing entry!
			   INCR_COUNT(insert_dup) ;
			   unannounceBucketNumber() ;
			   return true ;		// item already exists
			   }
			if_MULTITHREAD(else if (deletedEntry(pos) && deleted == NULLPOS) { deleted = pos ; })
			// advance to next item in chain
			offset = chainNext(pos) ;
			}
		     unannounceBucketNumber() ;
		     // when we get here, we know that the item is not yet in the
		     //   hash table, so try to add it
#ifdef FrMULTITHREAD
		     // first, did we encounter a deleted entry? (can only happen if multithreaded)
		     // If so, try to reclaim it
		     if (deleted != NULLPOS)
			{
			if (unlikely(!updateKey(deleted,Entry::DELETED(),Entry::RECLAIMING())))
			   {
			   INCR_COUNT(CAS_coll) ;
			   continue ;		// someone else reclaimed the entry, so restart
			   }
			// we managed to grab the entry for reclamation
			m_currsize++ ;
			// fill in the value, then the key
			setValue(deleted,value) ;
			cs::storeBarrier();
			setKey(deleted,key) ;
			INCR_COUNT(insert_attempt) ;
			// Verify that we haven't been superseded while we
			//   were working
			FORWARD(add(hashval,key,value),nexttab2,insert_forwarded) ;
			break ;
			}
#endif /* FrMULTITHREAD */
		     // otherwise, try to insert a new key/value entry
		     if (insertKey(bucketnum,firstoffset,key,value))
			break ;
		     }
		  return false ;  // item did not already exist
	       }
	    size_t addCount(size_t hashval, KeyT key, ValT incr) _fnattr_hot
	       {
		  INCR_COUNT(insert) ;
		  size_t bucketnum = hashval % m_size ;
		  while (true)
		     {
		     FORWARD(addCount(hashval,key,incr),nexttab,insert_forwarded) ;
		     // scan the chain of items for this hash position
		     if_MULTITHREAD(size_t deleted = NULLPOS ;)
		     // tell others we're using the bucket
		     announceBucketNumber(bucketnum) ;
		     Link offset = chainHead(bucketnum) ;
		     Link firstoffset = offset ;
		     while (NULLPTR != offset)
			{
			size_t pos = bucketnum + offset ;
			KeyT key_at_pos = getKey(pos) ;
			if (isEqual(key,key_at_pos))
			   {
			   // found existing entry!  Verify that we
			   //   haven't been superseded during the
			   //   search
			   if (superseded())
			      {
			      unannounceBucketNumber() ;
			      return addCount(hashval,key,incr) ;
			      }
			   ValT newcount = m_entries[pos].atomicIncrCount(incr) ;
			   INCR_COUNT(insert_dup) ;
			   return newcount ;
			   }
			// advance to next item in chain
			offset = chainNext(pos) ;
			}
		     unannounceBucketNumber() ;
		     // when we get here, we know that the item is not yet in the
		     //   hash table, so try to add it
#ifdef FrMULTITHREAD
		     // first, did we encounter a deleted entry? (can only happen if multithreaded)
		     // If so, try to reclaim it
		     if (deleted != NULLPOS)
			{
			if (unlikely(!updateKey(deleted,Entry::DELETED(),Entry::RECLAIMING())))
			   {
			   INCR_COUNT(CAS_coll) ;
			   continue ;		// someone else reclaimed the entry, so restart
			   }
			// we managed to grab the entry for reclamation
			m_currsize++ ;
			// fill in the value, then the key
			setValue(deleted,incr) ;
			cs::storeBarrier();
			setKey(deleted,key) ;
			INCR_COUNT(insert_attempt) ;
			// Verify that we haven't been superseded while we
			//   were working
			FORWARD(add(hashval,key,incr),nexttab2,insert_forwarded) ;
			break ;
			}
#endif /* FrMULTITHREAD */
		     // otherwise, try to insert a new key/value entry
		     if (insertKey(bucketnum,firstoffset,key,incr))
			break ;
		     }
		  return incr ;
	       }

	    bool contains(size_t hashval, KeyT key) const _fnattr_hot
	       {
		  size_t bucketnum = hashval % m_size ;
                  FORWARD_IF_COPIED(contains(hashval,key),contains_forwarded) ;
		  INCR_COUNT(contains) ;
                  // tell others we're using the bucket
		  announceBucketNumber(bucketnum) ;
		  // scan the chain of items for this hash position
		  Link offset = chainHead(bucketnum) ;
		  bool success = false ;
		  while (NULLPTR != offset)
		     {
		     size_t pos = bucketnum + offset ;
		     KeyT key_at_pos = getKey(pos) ;
		     if (isEqual(key,key_at_pos))
			{
			INCR_COUNT(contains_found) ;
			success = true ;
			break ;
			}
		     // advance to next item in chain
		     offset = chainNext(pos) ;
		     }
		  unannounceBucketNumber() ;
		  return success ;
	       }
	    ValT lookup(size_t hashval, KeyT key) const _fnattr_hot
	       {
		  size_t bucketnum = hashval % m_size ;
                  FORWARD_IF_COPIED(lookup(hashval,key),lookup_forwarded) ;
		  INCR_COUNT(lookup) ;
                  // tell others we're using the bucket
		  announceBucketNumber(bucketnum) ;
		  // scan the chain of items for this hash position
		  Link offset = chainHead(bucketnum) ;
                  ValT value = 0 ;
		  while (NULLPTR != offset)
		     {
		     size_t pos = bucketnum + offset ;
		     KeyT hkey = getKey(pos) ;
		     if (isEqual(key,hkey))
			{
			value = getValue(pos) ;
			// double-check that a parallel remove() hasn't
			//   deleted the entry while we were fetching the
			//   value
			if (getKey(pos) != hkey)
			   value = 0 ;
			INCR_COUNT(lookup_found) ;
			break ;
			}
		     // advance to next item in chain
		     offset = chainNext(pos) ;
		     }
		  unannounceBucketNumber() ;
		  return value ;
	       }
	    bool lookup(size_t hashval, KeyT key, ValT *value) const _fnattr_hot
	       {
		  if (!value)
		     return false ;
		  size_t bucketnum = hashval % m_size ;
                  FORWARD_IF_COPIED(lookup(hashval,key,value),lookup_forwarded) ;
		  INCR_COUNT(lookup) ;
		  (*value) = 0 ;
		  // tell others we're using the bucket
		  announceBucketNumber(bucketnum) ;
		  // scan the chain of items for this hash position
		  Link offset = chainHead(bucketnum) ;
                  bool found = false ;
		  while (NULLPTR != offset)
		     {
		     size_t pos = bucketnum + offset ;
		     KeyT hkey = getKey(pos) ;
		     if (isEqual(key,hkey))
			{
			(*value) = getValue(pos) ;
			// double-check that a parallel remove() hasn't deleted the
			//   hash entry while we were working with it
			found = (getKey(pos) == hkey) ;
			INCR_COUNT(lookup_found) ;
			break ;
			}
		     // advance to next item in chain
		     offset = chainNext(pos) ;
		     }
		  unannounceBucketNumber() ;
		  return found ;
	       }
	    // NOTE: this lookup() is not entirely thread-safe if clear==true
	    bool lookup(size_t hashval, KeyT key, ValT *value, bool clear_entry) _fnattr_hot
	       {
		  if (!value)
		     return false ;
		  size_t bucketnum = hashval % m_size ;
		  FORWARD_IF_COPIED(lookup(hashval,key,value),lookup_forwarded) ;
		  INCR_COUNT(lookup) ;
		  (*value) = 0 ;
		  // tell others we're using the bucket
		  announceBucketNumber(bucketnum) ;
		  // scan the chain of items for this hash position
		  Link offset = chainHead(bucketnum) ;
		  while (NULLPTR != offset)
		     {
		     size_t pos = bucketnum + offset ;
		     KeyT hkey = getKey(pos) ;
		     if (isEqual(key,hkey))
			{
			if (clear_entry)
			   {
			   (*value) = m_entries[pos].swapValue(0) ;
			   }
			else
			   {
			   (*value) = getValue(pos) ;
			   }
			// double-check that a parallel remove() hasn't deleted the
			//   hash entry while we were working with it
			bool found = (getKey(pos) == hkey) ;
			unannounceBucketNumber() ;
			INCR_COUNT(lookup_found) ;
			return found ;
			}
		     // advance to next item in chain
		     offset = chainNext(pos) ;
		     }
		  unannounceBucketNumber() ;
		  return false ;	// not found
	       }
	    // note: lookupValuePtr is not safe in the presence of parallel
	    //   add() and remove() calls!  Use global synchronization if
	    //   you will be using both this function and add()/remove()
	    //   concurrently on the same hash table.
	    ValT *lookupValuePtr(size_t hashval, KeyT key) const
	       {
		  size_t bucketnum = hashval % m_size ;
		  FORWARD_IF_COPIED(lookupValuePtr(hashval,key),lookup_forwarded) ;
		  INCR_COUNT(lookup) ;
                  // tell others we're using the bucket
		  announceBucketNumber(bucketnum) ;
		  // scan the chain of items for this hash position
		  Link offset = chainHead(bucketnum) ;
		  ValT *val = 0 ; // assume "not found"
		  while (NULLPTR != offset)
		     {
		     size_t pos = bucketnum + offset ;
		     KeyT key_at_pos = getKey(pos) ;
		     if (isEqual(key,key_at_pos))
			{
			val = getValuePtr(pos) ;
			INCR_COUNT(lookup_found) ;
			break ;
			}
		     // advance to next item in chain
		     offset = chainNext(pos) ;
		     }
		  unannounceBucketNumber() ;
		  return val ;
	       }

	    bool remove(size_t hashval, KeyT key)
	       {
		  size_t bucketnum = hashval % m_size ;
		  FORWARD_IF_STALE(remove(hashval,key),remove_forwarded) ;
		  INCR_COUNT(remove) ;
#ifdef FrMULTITHREAD
		  // deletions must be marked rather than removed since we
		  //   can't CAS both the key field and the chain link at
		  //   once
		  // because add() could generate duplicates if there are
		  //   concurrent insertions of the same item with multiple
		  //   deleted records in the bucket's chain, we need to
		  //   remove all occurrences of the key so that the delete
		  //   doesn't make a replaced entry reappear
                  announceBucketNumber(bucketnum) ;
		  Link offset = chainHead(bucketnum) ;
		  bool success = false ;
		  // scan the chain of items for this hash position
		  while (NULLPTR != offset)
		     {
		     size_t pos = bucketnum + offset ;
		     KeyT keyval = getKey(pos) ;
		     if (isEqual(key,keyval))
			{
			ValT value = getValue(pos) ;
			if (expected(updateKey(pos,keyval,Entry::DELETED())))
			   {
			   // update count of items in table
			   m_currsize-- ;
			   INCR_COUNT(remove_found) ;
			   success = true ;
			   // we successfully marked the entry as deleted
			   //   before anybody else changed it, so now we
			   //   can free its value at our leisure by running
			   //   the removal hook if present
			   if (remove_fn && value)
			      {
			      remove_fn(keyval,value) ;
			      }
			   }
			else
			   {
			   INCR_COUNT(CAS_coll) ;
			   }
			}
		     // advance to next item in chain
		     offset = chainNext(pos) ;
		     }
                  unannounceBucketNumber() ;
		  return success ;
#else
		  // since nobody else can mess with the bucket chain while we're traversing it, we can
                  //   just chop out the desired item and don't need to bother with marking entries as
		  //   deleted and reclaiming them later
		  Link *prevptr = chainHeadPtr(bucketnum) ;
		  Link offset = *prevptr ;
		  while (NULLPTR != offset)
		     {
		     size_t pos = bucketnum + offset ;
		     KeyT key_at_pos = getKey(pos) ;
		     if (isEqual(key,key_at_pos))
			{
		        // we found it!
		        // chop entry out of hash chain
		        *prevptr = chainNext(pos) ;
			// delete the item proper
			m_entries[pos].markUnused() ;
			replaceValue(pos,0) ;
			m_currsize-- ; // update count of active items
			INCR_COUNT(remove_found) ;
			return true ;
	                }
		     // advance to next item in chain
		     prevptr = chainNextPtr(pos) ;
		     offset = *prevptr ;
	             }
		  // item not found
		  return false ;
#endif /* FrMULTITHREAD */
	       }

	    bool reclaimDeletions()
	       {
		  if (superseded())
		     return true ;
#ifdef FrMULTITHREAD
		  INCR_COUNT(reclaim) ;
		  debug_msg("reclaimDeletions (thr %ld)\n",my_job_id) ;
		  bool have_reclaimed = false ;
		  size_t min_reclaimed = ~0UL ;
		  size_t max_reclaimed = 0 ;
		  size_t min_bucket = ~0UL ;
		  size_t max_bucket = 0 ;
		  for (size_t i = 0 ; i < m_size ; ++i)
		     {
		     if (deletedEntry(i))
			{
			// follow the chain containing the deleted item,
			//   chopping out any marked as deleted and
			//   resetting their status to Reclaimed.
			Link offset = chainOwner(i) ;
			if (NULLPTR != offset)
			   {
			   size_t bucket = i - offset ;
			   bool reclaimed = reclaimChain(bucket,min_reclaimed,max_reclaimed) ;
			   have_reclaimed |= reclaimed ;
			   if (reclaimed && bucket < min_bucket)
			      min_bucket = bucket ;
			   if (reclaimed && bucket > max_bucket)
			      max_bucket = bucket ;
			   }
			// abandon reclaim if a resize has run since we started
			if (superseded())
			   break ;
			}
		     }
		  if (have_reclaimed && !superseded()) // (if resized, the reclamation was moot)
		     {
		     // ensure that any existing concurrent reads complete
		     //   before we allow writes to the reclaimed entries
		     awaitIdle(min_bucket,max_bucket+1) ;
		     // at this point, nobody is traversing nodes we've chopped
		     //   out of hash chains, so we can go ahead and actually
		     //   reclaim
		     if (!superseded())
			{
			// reclaim entries marked as Reclaimed by switching their
			//   keys from Reclaimed to Unused
			// assumes no active writers and pending writers will be
			//   blocked
			for (size_t i = min_reclaimed ; i <= max_reclaimed ; ++i)
			   {
			   // any entries which no longer belong to a chain get marked as free
			   if (NULLPTR == chainOwner(i))
			      {
			      setKey(i,Entry::UNUSED()) ;
			      }
			   }
			}
		     }
		  else if (have_reclaimed)
		     {
		     debug_msg("reclaimDeletions mooted (thr %ld)\n",my_job_id) ;
		     }
		  debug_msg("reclaimDeletions done (thr %ld)\n",my_job_id) ;
		  return have_reclaimed ;
#else
		  // when single-threaded, we chop out deletions
		  //   immediately, so there is nothing to reclaim
		  return false ;
#endif /* FrMULTITHREAD */
	       }

	    // special support for FrSymbolTableX
	    KeyT addKey(size_t hashval, const char *name, size_t namelen,
			bool *already_existed = 0) _fnattr_hot
	       {
		  if (!already_existed)
		     {
		     // since only gensym() is interested in whether the key was already
		     //   in the table and regular add() calls will usually be for
		     //   symbols which already exist, try a fast-path lookup-only call
		     KeyT key = lookupKey(hashval,name,namelen) ;
		     if (key)
			return key ;
		     }
		  INCR_COUNT(insert) ;
		  size_t bucketnum = hashval % m_size ;
		  KeyT key ;
		  while (true)
		     {
		     FORWARD(addKey(hashval,name,namelen,already_existed),nexttab,insert_forwarded) ;
		     // scan the chain of items for this hash position
		     announceBucketNumber(bucketnum) ;
		     Link offset = chainHead(bucketnum) ;
		     Link firstoffset = offset ;
		     while (NULLPTR != offset)
			{
			size_t pos = bucketnum + offset ;
			KeyT existing = getKey(pos) ;
			if (isEqual(name,namelen,existing))
			   {
			   // found existing entry!
			   INCR_COUNT(insert_dup) ;
			   if (already_existed)
			      *already_existed = true ;
			   unannounceBucketNumber() ;
			   return existing ;
			   }
			// advance to next item in chain
			offset = chainNext(pos) ;
			}
		     // when we get here, we know that the item is not yet in the
		     //   hash table, so try to add it
		     key = Fr_allocate_symbol(static_cast<FrSymbolTable*>(m_container),name,namelen) ;
		     // if the insertKey fails, someone else beat us to
		     //    creating the symbol, so abandon this copy
		     //    (temporarily leaks at bit of memory until the
		     //    hash table is destroyed) and return the other one
		     //    when we loop back to the top
		     if (insertKey(bucketnum,firstoffset,key,(ValT)0))
			break ;
		     unannounceBucketNumber() ;
		     }
		  return key ;
	       }
	    // special support for FrSymbolTable
	    bool contains(size_t hashval, const char *name, size_t namelen) const _fnattr_hot
	       {
		  size_t bucketnum = hashval % m_size ;
		  FORWARD_IF_COPIED(contains(hashval,name,namelen),contains_forwarded)
		  INCR_COUNT(contains) ;
		  // scan the chain of items for this hash position
		  announceBucketNumber(bucketnum) ;
		  Link offset = chainHead(bucketnum) ;
		  bool success = false ;
		  while (NULLPTR != offset)
		     {
		     size_t pos = bucketnum + offset ;
		     KeyT key_at_pos = getKey(pos) ;
		     if (isEqual(name,namelen,key_at_pos))
			{
			INCR_COUNT(contains_found) ;
			success = true ;
			break ;
			}
		     // advance to next item in chain
		     offset = chainNext(pos) ;
		     }
                  unannounceBucketNumber() ;
		  return success ;
	       }
	    // special support for FrSymbolTableX
	    KeyT lookupKey(size_t hashval, const char *name, size_t namelen) const _fnattr_hot
	       {
		  size_t bucketnum = hashval % m_size ;
		  FORWARD_IF_COPIED(lookupKey(hashval,name,namelen),contains_forwarded)
		  INCR_COUNT(lookup) ;
		  // scan the chain of items for this hash position
		  announceBucketNumber(bucketnum) ;
		  Link offset = chainHead(bucketnum) ;
		  while (NULLPTR != offset)
		     {
		     size_t pos = bucketnum + offset ;
		     KeyT hkey = getKey(pos) ;
		     if (isEqual(name,namelen,hkey))
			{
			INCR_COUNT(lookup_found) ;
			unannounceBucketNumber() ;
			return hkey ;
			}
		     // advance to next item in chain
		     offset = chainNext(pos) ;
		     }
		  unannounceBucketNumber() ;
		  return 0 ;	// not found
	       }

	    void replaceValue(size_t pos, ValT new_value)
	       {
		  ValT value = m_entries[pos].swapValue(new_value) ;
		  if (remove_fn && value)
		     {
		     remove_fn(getKey(pos),value) ;
		     }
		  return ;
	       }

            //================= Content Statistics ===============
	    size_t countItems() const _fnattr_cold
	       {
		  if (next())
		     return next()->countItems() ;
		  size_t count = 0 ;
		  for (size_t i = 0 ; i < m_size ; ++i)
		     {
		     if (activeEntry(i))
			count++ ;
		     }
		  return count ;
	       }
	    size_t countItems(bool remove_dups) _fnattr_cold
	       {
		  if (next())
		     return next()->countItems(remove_dups) ;
		  if (remove_dups)
		     {
		     for (size_t i = 0 ; i < m_size ; ++i)
			{
			clearDuplicates(i) ;
			}
		     }
		  size_t count = 0 ;
		  for (size_t i = 0 ; i < m_size ; ++i)
		     {
		     if (activeEntry(i))
			count++ ;
		     }
		  return count ;
	       }
	    size_t countDeletedItems() const _fnattr_cold
	       {
		  if (next())
		     return next()->countDeletedItems() ;
		  size_t count = 0 ;
		  for (size_t i = 0 ; i < m_size ; ++i)
		     {
		     if (deletedEntry(i))
			count++ ;
		     }
		  return count ;
	       }
	    size_t chainLength(size_t bucketnum) const _fnattr_cold
	       {
		  FORWARD_IF_COPIED(chainLength(bucketnum),none) ;
		  size_t len = 0 ;
		  announceBucketNumber(bucketnum) ;
		  Link offset = chainHead(bucketnum) ;
		  while (NULLPTR != offset)
		     {
		     len++ ;
		     size_t pos = bucketnum + offset ;
		     offset = chainNext(pos) ;
		     }
		  unannounceBucketNumber() ;
		  return len ;
	       }
	    size_t *chainLengths(size_t &max_length) const _fnattr_cold
	       {
		  if (next())
		     return next()->chainLengths(max_length) ;
		  size_t *lengths = new size_t[2*searchrange+2] ;
		  for (size_t i = 0 ; i <= 2*searchrange+1 ; i++)
		     {
		     lengths[i] = 0 ;
		     }
		  max_length = 0 ;
		  for (size_t i = 0 ; i < m_size ; ++i)
		     {
		     size_t len = chainLength(i) ;
		     if (len > max_length)
			max_length = len ;
		     lengths[len]++ ;
		     }
		  return lengths ;
	       }
	    size_t *neighborhoodDensities(size_t &num_densities) const _fnattr_cold
	       {
		  if (next())
		     return next()->neighborhoodDensities(num_densities) ;
		  size_t *densities = new size_t[2*searchrange+2] ;
		  num_densities = 2*searchrange+1 ;
		  for (size_t i = 0 ; i <= 2*searchrange+1 ; i++)
		     {
		     densities[i] = 0 ;
		     }
		  // count up the active neighbors of the left-most entry in the hash array
		  size_t density = 0 ;
		  for (size_t i = 0 ; i <= (size_t)searchrange && i < m_size ; ++i)
		     {
		     if (activeEntry(i))
			++density ;
		     }
		  ++densities[density] ;
		  for (size_t i = 1 ; i < m_size ; ++i)
		     {
		     // keep a rolling count of the active neighbors around the current point
		     // did an active entry come into range on the right?
		     if (i + searchrange < m_size && activeEntry(i+searchrange))
			++density ;
		     // did an active entry go out of range on the left?
		     if (i > (size_t)searchrange && activeEntry(i-searchrange-1))
			--density ;
		     ++densities[density] ;
		     }
		  return densities ;
	       }

            //============== Iterators ================
            bool iterateVA(HashKeyValueFunc *func, va_list args) const
	       {
		  bool success = true ;
		  for (size_t i = 0 ; i < m_size && success ; ++i)
		     {
		     if (!activeEntry(i))
			continue ;
		     KeyT key = getKey(i) ;
		     ValT value = getValue(i) ;
		     // if the item hasn't been removed while we were getting it,
		     //   call the processing function
		     if (!activeEntry(i))
			continue ;
		     FrSafeVAList(args) ;
		     success = func(key,value,FrSafeVarArgs(args)) ;
		     FrSafeVAListEnd(args) ;
		     }
		  return success ;
	       }
            bool iterateAndClearVA(HashKeyValueFunc *func, va_list args) const
	       {
		  bool success = true ;
		  for (size_t i = 0 ; i < m_size && success ; ++i)
		     {
		     if (!activeEntry(i))
			continue ;
		     KeyT key = getKey(i) ;
		     ValT value = getValue(i) ;
		     // if the item hasn't been removed while we were getting it,
		     //   call the processing function
		     if (!activeEntry(i))
			continue ;
		     FrSafeVAList(args) ;
		     success = func(key,value,FrSafeVarArgs(args)) ;
		     if (success)
			m_entries[i].setValue(0) ;
		     FrSafeVAListEnd(args) ;
		     }
		  return success ;
	       }
            bool iterateAndModifyVA(HashKeyPtrFunc *func, va_list args) const
	       {
		  bool success = true ;
		  for (size_t i = 0 ; i < m_size && success ; ++i)
		     {
		     if (!activeEntry(i))
			continue ;
		     KeyT key = getKey(i) ;
		     ValT* valptr = getValuePtr(i) ;
		     // if the item hasn't been removed while we were getting it,
		     //   call the processing function
		     if (!activeEntry(i))
			continue ;
		     FrSafeVAList(args) ;
		     success = func(key,valptr,FrSafeVarArgs(args)) ;
		     FrSafeVAListEnd(args) ;
		     }
		  return success ;
	       }
            FrList *allKeys() const
	       {
		  FrList *keys = 0 ;
		  for (size_t i = 0 ; i < m_size ; ++i)
		     {
		     if (activeEntry(i))
			{
			KeyT name = m_entries[i].copyName() ;
			pushlist(name,keys) ;
			}
		     }
		  return keys ;
	       }

            //============== Debugging Support ================
            bool verify() const _fnattr_cold
               {
	          bool success = true ;
	          for (size_t i = 0 ; i < m_size ; ++i)
	             {
	             if (activeEntry(i) && !contains(getKey(i)))
		        {
		        debug_msg("verify: missing @ %ld\n",i) ;
		        success = false ;
		        break ;
		        }
	              }
	          return success ;
	       }

	    // ============= FrObject support =============
	    ostream &printKeyValue(ostream &output, KeyT key) const
	       {
		  return output << key ;
	       }
	    size_t keyDisplayLength(KeyT key) const
	       {
		  return key ? key->displayLength() + 1 : 3 ;
	       }
	    char *displayKeyValue(char *buffer, KeyT key) const
	       {
		  if (key)
		     return key->displayValue(buffer) ;
		  else
		     {
		     *buffer++ = 'N' ;
		     *buffer++ = 'I' ;
		     *buffer++ = 'L' ;
		     *buffer = '\0' ;
		     return  buffer ;
		     }
	       }
	    ostream &printValue(ostream &output) const
	       {
		  output << "#H(" ;
		  size_t orig_indent = FramepaC_initial_indent ;
		  FramepaC_initial_indent += 3 ; //strlen("#H(")
		  size_t loc = FramepaC_initial_indent ;
		  bool first = true ;
		  for (size_t i = 0 ; i < m_size ; ++i)
		     {
		     if (!activeEntry(i))
			continue ;
		     KeyT key = getKey(i) ;
		     size_t len = keyDisplayLength(key) ;
		     loc += len ;
		     if (loc > FramepaC_display_width && !first)
			{
			output << '\n' << setw(FramepaC_initial_indent) << " " ;
			loc = FramepaC_initial_indent + len ;
			}
		     output << key << ' ' ;
		     first = false ;
		     }
		  output << ")" ;
		  FramepaC_initial_indent = orig_indent ;
		  return output ;
	       }
	    char *displayValue(char *buffer) const
	       {
		  strcpy(buffer,"#H(") ;
		  buffer += 3 ;
		  for (size_t i = 0 ; i < m_size ; ++i)
		     {
		     if (!activeEntry(i))
			continue ;
		     KeyT key = getKey(i) ;
		     buffer = displayKeyValue(buffer,key) ;
		     *buffer++ = ' ' ;
		     }
		  *buffer++ = ')' ;
		  *buffer = '\0' ;
		  return buffer ;
	       }
	    // get size of buffer needed to display the string representation of the hash table
	    // NOTE: will not be valid if there are any add/remove/resize calls between calling
	    //   this function and using displayValue(); user must ensure locking if multithreaded
	    size_t displayLength() const
	       {
		  size_t dlength = 4 ; // "#H(" prefix plus ")" trailer
		  for (size_t i = 0 ; i < m_size ; i++)
		     {
		     if (!activeEntry(i))
			continue ;
		     KeyT key = getKey(i) ;
		     dlength += keyDisplayLength(key) + 1 ;
		     }
		  return dlength ;
	       }

         } ;
      class TablePtr
         {
	 public:
	    TablePtr 	 *m_next ;
	    size_t	 *m_bucket ;
	    Table       **m_table ;
	    size_t   	  m_id ;	   // thread ID, for help in debugging
	 public:
	    void init(Table **tab, size_t *bcket, TablePtr *next)
	       { m_table = tab ; m_bucket = bcket ; m_next = next ;
		  m_id = my_job_id ; }
	    size_t bucket() const { return cs::load(*m_bucket) ; }
	    const Table *table() const { return cs::load(*m_table) ; }
         } ;
      class HazardLock
         {
	 private:
	 public:
#ifdef FrMULTITHREAD
	    HazardLock(Table *tab)  { cs::store(FrHashTable::s_table,tab) ; }
	    ~HazardLock() { cs::store(FrHashTable::s_table,(Table*)0) ; }
#else
	    _fnattr_always_inline HazardLock(Table *) {}
	    _fnattr_always_inline ~HazardLock() {}
#endif /* FrMULITTHREAD */
         } ;

   private: // members
      static class FrReader *s_reader ;
   protected: // members
      atomic<Table*>   	  m_table ;	// pointer to currently-active m_tables[] entry
      atomic<Table*>	  m_oldtables ;	// start of list of currently-live hash arrays
      atomic<Table*>	  m_freetables ;
      Table		  m_tables[NUM_TABLES] ;// hash array, chains, and associated data
      HashKeyValueFunc   *cleanup_fn ;	// invoke on destruction of obj
      HashKVFunc         *remove_fn ; 	// invoke on removal of entry/value
      double	          m_maxfill ;	// maximum fill factor before resizing
#ifdef FrMULTITHREAD
      static TablePtr    *s_thread_entries ;
      static size_t       s_registered_threads ;
      static FrPER_THREAD TablePtr s_thread_record ;
      static FrPER_THREAD Table *s_table ;// thread's announcement which hash table it's using
      static FrPER_THREAD size_t s_bucket ;
      static FrMutex	  s_mutex ;
      static FrThreadOnce s_once ;
      static FrThreadKey  s_threadkey ;
#endif /* FrMULTITHREAD */
#ifdef FrHASHTABLE_STATS
      mutable FrHashTable_Stats	  m_stats ;
      static FrPER_THREAD FrHashTable_Stats   s_stats ;
#endif /* FrHASHTABLE_STATS */
      static const size_t m_bit_resize = ((sizeof(size_t)*CHAR_BIT)-1) ;
      static const size_t m_mask_resize = (1UL << m_bit_resize) ; // lock bit for resize
   protected: // debug methods
#if FrHASHTABLE_VERBOSITY > 0
      static void warn_msg(const char *fmt, ...) __attribute__((format(printf,1,2)))
	 {
	    va_list args ;
	    va_start(args,fmt) ;
	    vfprintf(stderr,fmt,args) ;
	    va_end(args) ;
	    return ;
	 }
#else
      static void warn_msg(const char *fmt, ...) __attribute__((format(printf,1,2))) _fnattr_always_inline
	 { (void)fmt; }
#endif /* FrHASHTABLE_VERBOSITY > 0 */
#if FrHASHTABLE_VERBOSITY > 1
      static void debug_msg(const char *fmt, ...) __attribute__((format(printf,1,2)))
	 {
	    va_list args ;
	    va_start(args,fmt) ;
	    vfprintf(stderr,fmt,args) ;
	    va_end(args) ;
	    return ;
	 }
#else
      static void debug_msg(const char *fmt, ...) __attribute__((format(printf,1,2))) _fnattr_always_inline
	 { (void)fmt ; }
#endif /* FrHASHTABLE_VERBOSITY > 1 */
   protected: // methods
      static void thread_backoff(size_t &loops)
	 {
	    ++loops ;
	    // we expect the pending operation(s) to complete in
	    //   well under one microsecond, so start by just spinning
	    if (loops < FrSPIN_COUNT)
	       {
	       INCR_COUNT(spin) ;
	       _mm_pause() ;
	       _mm_pause() ;
	       }
	    else if (loops < FrSPIN_COUNT + FrYIELD_COUNT)
	       {
	       // it's taking a little bit longer, so now yield the
	       //   CPU to any suspended but ready threads
	       //debug_msg("yield\n") ;
	       INCR_COUNT(yield) ;
	       FrThreadYield() ;
	       }
	    else
	       {
	       // hmm, this is taking a while -- maybe an add() had
	       // to do extra work, or we are oversubscribed on
	       // threads and an operation got suspended
	       debug_msg("sleep\n") ;
	       INCR_COUNT(sleep) ;
	       size_t factor = loops - FrSPIN_COUNT - FrYIELD_COUNT + 1 ;
	       FrThreadSleep(factor * FrNAP_TIME) ;
	       }
	    return ;
	 }
      static bool isActive(KeyT p) { return p < Entry::RECLAIMING() ; }
      void init(size_t initial_size, double max_fill, Table *table = 0)
	 {
	    onRemove(0) ;
	    onDelete(0) ;
	    m_table.store(0) ;
	    m_maxfill = max_fill < FrHashTable_MinFill ? FrHashTable_DefaultMaxFill : max_fill ;
	    m_oldtables.store(0) ;
	    m_freetables.store(0) ;
	    clearGlobalStats() ;
	    initial_size = Table::normalizeSize(initial_size) ;
	    for (size_t i = 0 ; i < NUM_TABLES ; i++)
	       {
	       releaseTable(&m_tables[i]) ;
	       }
	    if (!table)
	       {
	       table = allocTable() ;
	       }
	    new (table) Table(initial_size,m_maxfill) ;
	    if (expected(table->good()))
	       {
	       table->m_container = this ;
	       table->remove_fn = onRemoveFunc() ;
	       m_table.store(table) ;
	       m_oldtables.store(table) ;
	       }
	    else
	       {
	       FrNoMemory("creating FrHashTable") ;
	       }
	    return ;
	 }
      Table *allocTable()
	 {
	    // pop a table record off the freelist, if available
	    Table *tab = m_freetables.load() ;
	    while (tab)
	       {
	       Table *nxt = tab->nextFree() ;
	       if (m_freetables.compare_exchange_strong(tab,nxt))
		  return tab ;
	       tab = m_freetables.load() ;
	       }
	    // no table records on the freelist, so create a new one
	    return new Table ;
	 }
      void releaseTable(Table *t)
	 {
	    t->clear() ;
	    Table *freetab ;
	    do
	       {
	       freetab = m_freetables.load() ;
	       t->m_next_free.store(freetab) ;
	       } while (!m_freetables.compare_exchange_strong(freetab,t)) ;
	    return ;
	 }
      void freeTables()
	 {
	    Table *tab ;
	    while ((tab = m_freetables.load()) != 0)
	       {
	       Table *next = tab->nextFree() ;
	       m_freetables.store(next) ;
	       if (tab < &m_tables[0] || tab >= &m_tables[NUM_TABLES])
		  {
		  // not one of the tables inside our own instance, so
		  //   send back to OS
		  delete tab ;
		  }
	       }
	 }
      void updateTable()
         {
	    Table *table = m_table.load() ;
	    bool updated = false ;
	    while (table && table->resizingDone() && table->next())
	       {
	       table = table->next() ;
	       updated = true ;
	       }
	    if (updated)
	       {
	       m_table.store(table) ;
	       }
	    return ;
	 }
      bool reclaimSuperseded()
	 {
	    bool freed = false ;
	    while (true)
	       {
	       Table *tab = m_oldtables.load() ;
	       assert(tab != 0) ;
	       if (tab == m_table.load() || !tab->m_resizedone.load() ||
		   stillLive(tab))
		  break ;
	       Table *next = tab->next() ;
	       if (unlikely(!m_oldtables.compare_exchange_strong(tab,next)))
		  break ;	   // someone else has freed the table
	       releaseTable(tab) ; // put the emptied table on the freelist
	       freed = true ;
	       }
	    return freed ;
	 }

#ifdef FrMULTITHREAD
      static void unregisterThread(void *arg)
	 {
	    s_table = 0 ;
	    const Table **table_ptr = (const Table**)arg ;
	    if (table_ptr)
	       {
	       // unlink from the list of all thread-local table pointers
	       s_mutex.lock() ;
	       TablePtr *prev = 0 ;
	       TablePtr *curr = s_thread_entries ;
	       while (curr && curr->m_table != table_ptr)
		  {
		  prev = curr ;
		  curr = curr->m_next ;
		  }
	       if (curr)
		  {
		  // found a match, so unlink it
		  if (prev)
		     prev->m_next = curr->m_next ;
		  else
		     s_thread_entries = curr->m_next ;
		  s_thread_record.m_table = 0 ;
		  s_thread_record.m_bucket = 0 ;
		  --s_registered_threads ;
		  }
	       s_mutex.unlock() ;
	       }
	    return ;
	 }
      static void threadInitOnce()
	 {
	    FrThread::createKey(s_threadkey,FrHashTable::unregisterThread) ;
	    return ;
	 }
#else // !FrMULTITHREAD
      static void unregisterThread(void*) {}
#endif /* FrMULTITHREAD */
      size_t maxSize() const { return m_table.load()->m_size ; }
      static inline size_t hashVal(KeyT key)
	 {
	    return key ? key->hashValue() : 0 ;
	 }
      static inline size_t hashValFull(KeyT key)
	 {
	    // separate version for use by resizeTo(), which needs to use the full
	    //   hash value and not just the key on FrSymbolTableX
	    // this is the version for everyone *but* FrSymbolTableX
	    return hashVal(key) ;
	 }
      // special support for FrSymbolTableX
      static size_t hashVal(const char *keyname, size_t *namelen) ;
      static inline bool isEqual(KeyT key1, KeyT key2)
	 {
	    return FrHashTable::isActive(key2) && ::equal(key1,key2) ;
	 }
      static inline bool isEqualFull(KeyT key1, KeyT key2)
	 {
	    // separate version for use by resizeTo(), which needs to use the full
	    //   key name and not just the key's pointer on FrSymbolTableX
	    // this is the version for everyone *but* FrSymbolTableX
	    return isEqual(key1,key2) ;
	 }
      // special support for FrSymbolTableX
      // special support for FrSymbolTableX; must be overridden
      static bool isEqual(const char *keyname, size_t namelen, KeyT key) ;


   protected:
//FIXME
      // single-threaded only!!  But since it's only called from a ctor, that's fine
      void copyContents(const FrHashTable &ht)
         {
	    init(ht.maxSize(),ht.m_maxfill) ;
	    Table *table = m_table.load() ;
	    Table *othertab = ht.m_table.load() ;
	    table->m_currsize.store(ht.currentSize()) ;
	    for (size_t i = 0 ; i < table->m_size ; i++)
	       {
	       table->copyEntry(i,othertab) ;
	       }
	    return ;
	 }
      // single-threaded only!!  But since it's only called from a dtor, that's fine
      void clearContents()
         {
	    Table *table = m_table.load() ;
	    if (table && table->good())
	       {
	       if (cleanup_fn)
		  {
		  this->iterate(cleanup_fn) ;
		  cleanup_fn = 0 ;
		  }
	       if (remove_fn)
		  {
		  for (size_t i = 0 ; i < table->m_size ; ++i)
		     {
		     if (table->activeEntry(i))
			{
			table->replaceValue(i,0) ;
			}
		     }
		  }
	       cs::memoryBarrier() ;
	       debug_msg("FrHashTable dtor\n") ;
	       table->awaitIdle(0,table->m_size) ;
	       }
	    table->clear() ;
	    freeTables() ;
	    return ;
	 }

      static size_t stillLive(const Table *version)
	 {
#ifdef FrMULTITHREAD
	    // scan the list of per-thread s_table variables
	    for (const TablePtr *tables = cs::load(s_thread_entries) ; tables ; tables = ANNOTATE_UNPROTECTED_READ(tables->m_next))
	       {
	       // check whether the hazard pointer is for the requested version
	       if (tables->table() == version)
		  {
		  // yep, the requested version is still live
		  return true ;
		  }
	       }
#endif /* FrMULTITHREAD */
	    (void)version ;
	    // nobody else is using that version
	    return false ;
	 }

      // ============== Definitions to reduce duplication ================
      // much of the FrHashTable API just calls through to Table after
      //   setting a hazard pointer
#ifdef FrMULTITHREAD
#define DELEGATE(delegate) 				\
            HazardLock hl(m_table.load()) ; 		\
	    return m_table.load()->delegate ;
#define DELEGATE_HASH(delegate) 			\
	    size_t hashval = hashVal(key) ; 		\
	    HazardLock hl(m_table.load()) ;		\
	    return m_table.load()->delegate ;
#define DELEGATE_HASH_RECLAIM(type,delegate)		\
	    if (m_oldtables.load() != m_table.load())	\
	       reclaimSuperseded() ;			\
	    size_t hashval = hashVal(key) ; 		\
	    HazardLock hl(m_table.load()) ;		\
	    return m_table.load()->delegate ;
#else
#define DELEGATE(delegate) \
            return m_table.load()->delegate ;
#define DELEGATE_HASH(delegate) 			\
	    size_t hashval = hashVal(key) ; 		\
            return m_table.load()->delegate ;
#define DELEGATE_HASH_RECLAIM(type,delegate)		\
	    if (m_oldtables.load() != m_table.load())	\
	       reclaimSuperseded() ;			\
	    size_t hashval = hashVal(key) ; 		\
	    return m_table.load()->delegate ; 
#endif /* FrMULTITHREAD */

      // ============== The public API for FrHashTable ================
   public:
      FrHashTable(size_t initial_size = 1031, double max_fill = 0.0)
	 : m_table(0), m_oldtables(0), m_freetables(0)
	 {
	    init(initial_size,max_fill) ;
	    return ;
	 }
      FrHashTable(const FrHashTable &ht)
	 : m_table(0), m_oldtables(0), m_freetables(0)
	 {
	    if (&ht == 0)
	       return ;
	    copyContents(ht) ;
	    return ;
	 }
      ~FrHashTable()
	 {
	    clearContents() ;
	    remove_fn = 0 ;
	    return ;
	 }

      size_t sizeForCapacity(size_t capacity) const
	 {
	    return (size_t)(capacity / m_maxfill + 0.99) ;
	 }

      bool resizeTo(size_t newsize, bool enlarge_only = false)
	 { DELEGATE(resize(newsize,enlarge_only)) }
      void resizeToFit(size_t new_capacity)
	 {
	    resizeTo(sizeForCapacity(new_capacity)) ;
	    return ;
	 } ;

      bool reclaimDeletions() { DELEGATE(reclaimDeletions()) }

      void loadPacked() {}		// backwards compatibility
      void pack()			// backwards compatibility
	 {
	    // reduce size of hash array to increase loading factor to 97% or so
	    //   in order to save memory
	    setMaxFill(0.97) ;
	    resizeToFit(currentSize()) ;
	    return ;
	 }

      bool add(KeyT key, ValT value = 0) _fnattr_hot { DELEGATE_HASH_RECLAIM(bool,add(hashval,key,value)) }
      size_t addCount(KeyT key, ValT incr) _fnattr_hot { DELEGATE_HASH_RECLAIM(size_t,addCount(hashval,key,incr)) }
      bool remove(KeyT key) { DELEGATE_HASH(remove(hashval,key)) }
      bool contains(KeyT key) const _fnattr_hot { DELEGATE_HASH(contains(hashval,key)) }
      ValT lookup(KeyT key) const _fnattr_hot { DELEGATE_HASH(lookup(hashval,key)) }
      bool lookup(KeyT key, ValT *value) const _fnattr_hot { DELEGATE_HASH(lookup(hashval,key,value)) }
      // NOTE: this lookup() is not entirely thread-safe if clear==true
      bool lookup(KeyT key, ValT *value, bool clear) _fnattr_hot { DELEGATE_HASH(lookup(hashval,key,value,clear)) }
      // NOTE: lookupValuePtr is not safe in the presence of parallel
      //   add() and remove() calls!  Use global synchronization if
      //   you will be using both this function and add()/remove()
      //   concurrently on the same hash table.
      ValT *lookupValuePtr(KeyT key) const { DELEGATE_HASH(lookupValuePtr(hashval,key)) }
      bool add(KeyT key, ValT value, bool replace) _fnattr_hot
	 {
	    if (m_oldtables.load() != m_table.load())
	       reclaimSuperseded() ;
	    size_t hashval = hashVal(key) ;
	    HazardLock hl(m_table.load()) ;
	    if (replace)
	       m_table.load()->remove(hashval,key) ;
	    return m_table.load()->add(hashval,key,value) ;
	 }
      // special support for FrSymbolTableX
      KeyT addKey(const char *name, bool *already_existed = 0) _fnattr_hot
	 {
	    size_t namelen ;
	    size_t hashval = hashVal(name,&namelen) ;
	    DELEGATE(addKey(hashval, name, namelen, already_existed))
	 }
      // special support for FrSymbolTable
      bool contains(const char *name) const _fnattr_hot
         {
	    size_t namelen ;
	    size_t hashval = hashVal(name,&namelen) ;
	    DELEGATE(contains(hashval,name,namelen))
	 }
      // special support for FrSymbolTableX
      KeyT lookupKey(const char *name, size_t namelen, size_t hashval) const _fnattr_hot
         { DELEGATE(lookupKey(hashval,name,namelen)) }
      // special support for FrSymbolTableX
      KeyT lookupKey(const char *name) const _fnattr_hot
         {
	    if (!name)
	       return 0 ;
	    size_t namelen ;
	    size_t hashval = hashVal(name,&namelen) ;
	    DELEGATE(lookupKey(hashval,name,namelen))
	 }

      // set up the per-thread info needed for safe reclamation of
      //   entries and hash arrays, as well as an on-exit callback
      //   to clear that info
      static void registerThread()
	 {
#ifdef FrMULTITHREAD
	    s_once.doOnce(threadInitOnce) ;
	    // check whether we've set the key; if not, we haven't
	    //   initialized the thread-local data yet
	    if (!FrThread::getKey(s_threadkey))
	       {
	       // set a non-NULL value for the key in the current thread, so
	       //   that our cleanup function gets called on thread termination
	       FrThread::setKey(s_threadkey,&s_table) ;
	       // push our local-copy variable onto the list of all such variables
	       //   for use by the resizer
	       unregisterThread(&s_thread_record) ;  // avoid double-linking record
	       s_mutex.lock() ;
	       s_thread_record.init(&s_table,&s_bucket,s_thread_entries) ;
	       /*atomic_store*/(s_thread_entries = &s_thread_record) ;
	       /*atomic_incr*/++s_registered_threads ;
	       s_mutex.unlock() ;
	       }
#endif /* FrMULTITHREAD */
	    return ;
	 }

      size_t countItems(bool remove_dups = false) const _fnattr_cold
	 { DELEGATE(countItems(remove_dups)) }
      size_t countDeletedItems() const _fnattr_cold
	 { DELEGATE(countDeletedItems()) }
      size_t *chainLengths(size_t &max_length) const _fnattr_cold
         { DELEGATE(chainLengths(max_length)) }
      size_t *neighborhoodDensities(size_t &num_densities) const _fnattr_cold
	 { DELEGATE(neighborhoodDensities(num_densities)) }

      // ========== Iterators ===========
      bool iterateVA(HashKeyValueFunc *func, va_list args) const
         { DELEGATE(iterateVA(func,args)) }
      bool __FrCDECL iterate(HashKeyValueFunc *func,...) const
	 {
	    va_list args ;
	    va_start(args,func) ;
	    bool success = iterateVA(func,args) ;
	    va_end(args) ;
	    return success ;
	 }
      bool __FrCDECL iterateAndClearVA(HashKeyValueFunc *func, va_list args)
	 { DELEGATE(iterateAndClearVA(func,args)) }
      bool __FrCDECL iterateAndClear(HashKeyValueFunc *func,...)
	 {
	    va_list args ;
	    va_start(args,func) ;
	    bool success = iterateAndClearVA(func,args) ;
	    va_end(args) ;
	    return success ;
	 }
      //  iterateAndModify is not safe in the presence of parallel remove() calls!
      bool __FrCDECL iterateAndModifyVA(HashKeyPtrFunc *func, va_list args)
	 { DELEGATE(iterateAndModifyVA(func,args)) }
      //  iterateAndModify is not safe in the presence of parallel remove() calls!
      bool __FrCDECL iterateAndModify(HashKeyPtrFunc *func,...)
	 {
	    va_list args ;
	    va_start(args,func) ;
	    bool success = iterateAndModifyVA(func,args) ;
	    va_end(args) ;
	    return success ;
	 }
      FrList *allKeys() const { DELEGATE(allKeys()) }

      // set callback function to be invoked when the hash table is deleted
      void onDelete(HashKeyValueFunc *func) { cleanup_fn = func ; }
      // set callback function to be invoked when an entry is deleted; this
      //   permits the associated value for the entry to be freed
      void onRemove(HashKVFunc *func) { remove_fn = func ; }

      void setMaxFill(double fillfactor)
	 {
	    if (fillfactor > 0.98)
	       fillfactor = 0.98 ;
	    else if (fillfactor < FrHashTable_MinFill)
	       fillfactor = FrHashTable_MinFill ;
	    m_maxfill = fillfactor ;
	    HazardLock hl(m_table.load()) ;
	    Table *table = m_table.load() ;
	    if (table)
	       {
	       table->m_resizethresh = table->resizeThreshold(table->m_size) ;
	       }
	    return ;
	 }

      // access to internal state
      size_t currentSize() const { DELEGATE(m_currsize.load()) }
      size_t maxCapacity() const { DELEGATE(m_size) }
      bool isPacked() const { return false ; }  // backwards compatibility
      HashKeyValueFunc *cleanupFunc() const { return cleanup_fn ; }
      HashKVFunc *onRemoveFunc() const { return remove_fn ; }

      // =============== Operational Statistics ================
      void clearGlobalStats() _fnattr_cold
	 {
#ifdef FrHASHTABLE_STATS
	    ((FrHashTable_Stats_w_Methods*)&m_stats)->clear() ;
#endif /* FrHASHTABLE_STATS */
	    return ;
	 }
      static void clearPerThreadStats() _fnattr_cold
	 {
#if defined(FrHASHTABLE_STATS) && defined(FrHASHTABLE_STATS_PERTHREAD)
	    ((FrHashTable_Stats_w_Methods*)&s_stats)->clear();
	    ScopedChainLock::clearPerThreadStats() ;
#endif /* FrHASHTABLE_STATS && FrHASHTABLE_STATS_PERTHREAD */
	    return ;
	 }
      void updateGlobalStats() _fnattr_cold
	 {
#if defined(FrHASHTABLE_STATS) && defined(FrHASHTABLE_STATS_PERTHREAD)
	    ((FrHashTable_Stats_w_Methods*)&m_stats)->add(&s_stats) ;
	    cs::increment(m_stats.chain_lock_count,ScopedChainLock::numberOfLocks()) ;
	    cs::increment(m_stats.chain_lock_coll,ScopedChainLock::numberOfLockCollisions()) ;
#endif /* FrHASHTABLE_STATS && FrHASHTABLE_STATS_PERTHREAD */
	    return ;
	 }
#ifdef FrHASHTABLE_STATS
      size_t numberOfInsertions() const { return m_stats.insert ; }
      size_t numberOfDupInsertions() const { return m_stats.insert_dup ; }
      size_t numberOfForwardedInsertions() const { return m_stats.insert_forwarded ; }
      size_t numberOfResizeInsertions() const { return m_stats.insert_resize ; }
      size_t numberOfInsertionAttempts() const { return  m_stats.insert_attempt ; }
      size_t numberOfRemovals() const { return m_stats.remove ; }
      size_t numberOfForwardedRemovals() const { return m_stats.remove_forwarded ; }
      size_t numberOfItemsRemoved() const { return m_stats.remove_found ; }
      size_t numberOfContainsCalls() const { return m_stats.contains ; }
      size_t numberOfSuccessfulContains() const { return m_stats.contains_found ; }
      size_t numberOfForwardedContains() const { return m_stats.contains_forwarded ; }
      size_t numberOfLookups() const { return  m_stats.lookup ; }
      size_t numberOfSuccessfulLookups() const { return  m_stats.lookup_found ; }
      size_t numberOfForwardedLookups() const { return m_stats.lookup_forwarded ; }
      size_t numberOfResizes() const { return  m_stats.resize ; }
      size_t numberOfResizeAssists() const { return  m_stats.resize_assist ; }
      size_t numberOfResizeCleanups() const { return  m_stats.resize_cleanup ; }
      size_t numberOfReclamations() const { return m_stats.reclaim ; }
      size_t numberOfEntriesMoved() const { return m_stats.move ; }
      size_t numberOfFullNeighborhoods() const { return  m_stats.neighborhood_full ; }
      size_t numberOfCASCollisions() const { return m_stats.CAS_coll ; }
      size_t numberOfChainLocks() const { return m_stats.chain_lock_count ; }
      size_t numberOfChainLockCollisions() const { return m_stats.chain_lock_coll ; }
      size_t numberOfSpins() const { return  m_stats.spin ; }
      size_t numberOfYields() const { return m_stats.yield ; }
      size_t numberOfSleeps() const { return m_stats.sleep ; }
#else
      static size_t numberOfInsertions() { return 0 ; }
      static size_t numberOfDupInsertions() { return 0 ; }
      static size_t numberOfForwardedInsertions() { return 0 ; }
      static size_t numberOfInsertionAttempts() { return 0 ; }
      static size_t numberOfRemovals() { return 0 ; }
      static size_t numberOfForwardedRemovals() { return 0 ; }
      static size_t numberOfItemsRemoved() { return 0 ; }
      static size_t numberOfContainsCalls() { return 0 ; }
      static size_t numberOfSuccessfulContains() { return 0 ; }
      static size_t numberOfForwardedContains() { return 0 ; }
      static size_t numberOfLookups() { return 0 ; }
      static size_t numberOfSuccessfulLookups() { return 0 ; }
      static size_t numberOfForwardedLookups() { return 0 ; }
      static size_t numberOfResizes() { return 0 ; }
      static size_t numberOfResizeAssists() { return 0 ; }
      static size_t numberOfResizeCleanups() { return 0 ; }
      static size_t numberOfReclamations() { return 0 ; }
      static size_t numberOfEntriesMoved() { return 0 ; }
      static size_t numberOfFullNeighborhoods() { return 0 ; }
      static size_t numberOfCASCollisions() { return 0 ; }
      static size_t numberOfChainLocks() { return 0 ; }
      static size_t numberOfChainLockCollisions() { return 0 ; }
      static size_t numberOfSpins() { return 0 ; }
      static size_t numberOfYields() { return 0 ; }
      static size_t numberOfSleeps() { return 0 ; }
#endif

      // ============= FrObject support =============
      virtual FrObjectType objType() const { return OT_FrHashTable ; }
      virtual const char *objTypeName() const { return "FrHashTable" ; }
      virtual FrObjectType objSuperclass() const { return OT_FrObject ; }
      virtual ostream &printValue(ostream &output) const { DELEGATE(printValue(output)) }
      virtual char *displayValue(char *buffer) const { DELEGATE(displayValue(buffer)) }
      // get size of buffer needed to display the string representation of the hash table
      // NOTE: will not be valid if there are any add/remove/resize calls between calling
      //   this function and using displayValue(); user must ensure locking if multithreaded
      virtual size_t displayLength() const { DELEGATE(displayLength()) }
      virtual FrHashTable *copy() const
	 {
	    return new FrHashTable(*this) ;
	 }
      virtual FrHashTable *deepcopy() const
	 {
	    return new FrHashTable(*this) ;
	 }
      virtual void freeObject() { delete this ; }
      virtual size_t length() const { return currentSize() ; }
      virtual bool expand(size_t incr)
	 {
	    if (incr < FrHASHTABLE_MIN_INCREMENT)
	       incr = FrHASHTABLE_MIN_INCREMENT ;
	    resizeTo(maxSize() + incr) ;
	    return true ;
	 }
      virtual bool expandTo(size_t newsize)
	 {
	    if (newsize > maxSize())
	       resizeTo(newsize) ;
	    return true ;
	 }
      virtual bool hashp() const { return true ; }

      //  =========== Debugging Support ============
      bool verify() const _fnattr_cold { DELEGATE(verify()) }

   } ;

//----------------------------------------------------------------------
// static members

#ifdef FrMULTITHREAD
template <typename KeyT, typename ValT>
FrPER_THREAD typename FrHashTable<KeyT,ValT>::Table *FrHashTable<KeyT,ValT>::s_table = 0 ;
template <typename KeyT, typename ValT>
FrPER_THREAD size_t FrHashTable<KeyT,ValT>::s_bucket = ~0UL ;
template <typename KeyT, typename ValT>
FrMutex FrHashTable<KeyT,ValT>::s_mutex ;
template <typename KeyT, typename ValT>
FrThreadOnce FrHashTable<KeyT,ValT>::s_once ;
template <typename KeyT, typename ValT>
FrThreadKey FrHashTable<KeyT,ValT>::s_threadkey ;
template <typename KeyT, typename ValT>
typename FrHashTable<KeyT,ValT>::TablePtr *FrHashTable<KeyT,ValT>::s_thread_entries = 0 ;
template <typename KeyT, typename ValT>
size_t FrHashTable<KeyT,ValT>::s_registered_threads = 0 ;
template <typename KeyT, typename ValT>
FrPER_THREAD typename FrHashTable<KeyT,ValT>::TablePtr FrHashTable<KeyT,ValT>::s_thread_record ;
#endif /* FrMULTITHREAD */

#if defined(FrHASHTABLE_STATS)
template <typename KeyT, typename ValT>
FrPER_THREAD FrHashTable_Stats FrHashTable<KeyT,ValT>::s_stats ;
#endif /* FrHASHTABLE_STATS */

template <> FrReader *FrHashTable<FrObject,FrObject*>::s_reader ;
template <typename KeyT, typename ValT>
FrReader *FrHashTable<KeyT,ValT>::s_reader = 0 ;

//----------------------------------------------------------------------
// specializations: integer keys

#define FrMAKE_INTEGER_HASHTABLE_CLASS(NAME,K,V) \
\
template <> \
inline size_t FrHashTable<K,V>::hashVal(const K key) { return (size_t)key ; } \
\
template <> \
inline bool FrHashTable<K,V>::isEqual(const K key1, const K key2) \
{ return key1 == key2 ; } \
\
template <> \
inline K FrHashTable<K,V>::Entry::copy(const K obj) { return obj ; } \
\
template <> \
inline size_t FrHashTable<K,V>::Table::keyDisplayLength(const K key) const	\
{ \
   char buffer[200] ; \
   ultoa((size_t)key,buffer,10) ; \
   return strlen(buffer) ; \
} \
\
template <> \
inline char *FrHashTable<K,V>::Table::displayKeyValue(char *buffer,const K key) const \
{ ultoa((size_t)key,buffer,10) ; return strchr(buffer,'\0') ; }	\
\
typedef FrHashTable<K,V> NAME ;

//----------------------------------------------------------------------
// specializations: FrSymbol* keys

#define FrMAKE_SYMBOL_HASHTABLE_CLASS(NAME,V) \
\
template <> \
inline size_t FrHashTable<const FrSymbol *,V>::hashVal(const FrSymbol *key) { return (size_t)key ; } \
\
template <> \
inline bool FrHashTable<const FrSymbol *,V>::isEqual(const FrSymbol *key1, const FrSymbol *key2) \
{ return (size_t)key1 == (size_t)key2 ; }			  \
\
template <> \
inline const FrSymbol *FrHashTable<const FrSymbol *,V>::Entry::copy(const FrSymbol *obj) { return obj ; } \
\
typedef FrHashTable<const FrSymbol *,V> NAME ;

//----------------------------------------------------------------------
// specializations for FrSymbolTableX not included in above macro

size_t Fr_symboltable_hashvalue(const char *symname) ;
template <>
inline size_t FrHashTable<const FrSymbol *,FrNullObject>::hashValFull(const FrSymbol *key)
{ 
   return key ? Fr_symboltable_hashvalue(key->symbolName()) : 0 ;
}

template <>
inline bool FrHashTable<const FrSymbol *,FrNullObject>::isEqualFull(const FrSymbol *key1, const FrSymbol *key2)
{ 
   if (!FrHashTable::isActive(key2))
      return false ;
   if (key1 == key2)
      return true ;
   return (key1 && key2 && strcmp(key1->symbolName(),key2->symbolName()) == 0) ;
}

/************************************************************************/

typedef FrHashTable<FrObject*,FrObject*> FrObjHashTable ;
typedef FrHashTable<FrObject*,size_t> FrObjCountHashTable ;

FrMAKE_SYMBOL_HASHTABLE_CLASS(FrSymHashTable,FrObject *) ;
FrMAKE_SYMBOL_HASHTABLE_CLASS(FrSymCountHashTable,size_t) ;
FrMAKE_SYMBOL_HASHTABLE_CLASS(FrSymbolTableX,FrNullObject) ;

#undef NUM_TABLES
#undef INCR_COUNT
#undef FORWARD
#undef FORWARD_IF_COPIED
#undef FORWARD_IF_STALE
#undef DELEGATE
#undef DELEGATE_HASH
#undef DELEGATE_HASH_RECLAIM

#endif /* !__FRHASH_H_INCLUDED */

// end of file frhash.h //
