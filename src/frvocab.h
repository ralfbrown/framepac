/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC								*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frvocab.h	    String-to-ID mapping for vocabularies	*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 2004,2006,2007,2009					*/
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

#ifndef FRVOCAB_H_INCLUDED
#define FRVOCAB_H_INCLUDED

#if defined(__GNUC__)
#  pragma interface
#endif

#ifndef __FRCOMMON_H_INCLUDED
#include "frcommon.h"
#endif

#ifndef __FRBYTORD_H_INCLUDED
#include "frbytord.h"
#endif

#ifndef __FRFILUTL_H_INCLUDED
#include "frfilutl.h"	// ensure proper 64-bit file functions are used
#endif

#ifndef __FRMMAP_H_INCLUDED
#include "frmmap.h"
#endif

/************************************************************************/
/*	Manifest Constants						*/
/************************************************************************/

#define FrVOCAB_OOV		0
#define FrVOCAB_WORD_NOT_FOUND ((uint32_t)~0)

/************************************************************************/
/************************************************************************/

class FrVocabulary
   {
   private:
      char *m_filename ;		// name of file containing data
      FILE *m_fp ;			//
      off_t m_fileoffset ;		// start location in file
      char *m_vocab ;			// raw vocab data (when memory-mapped)
      FrFileMapping *m_mmap ;		// when using memory-mapped file

      char *m_index ;			// the vocabulary's index, w/ metadata
      char *m_revindex ;		// reverse index from ID to offset
      char *m_userdata ;		// array of user data
      char *m_words ;			// base of name strings for vocab

      size_t m_scalefactor ;		// conv between stored&actual offsets
      size_t m_padmask ;		// used for padding up to next boundary

      size_t m_wordloc_size ;		// size of offsets into m_words
      size_t m_ID_size ;		// bytes per ID value in index entry
      size_t m_indexent_size ;		// total bytes per index entry
      size_t m_userdata_size ;		// bytes of user data per index entry

      size_t m_index_size ;		// # of index entries used
      size_t m_index_alloc ;		// # of index entries allocated

      size_t m_words_size ;		// # of bytes in m_words used
      size_t m_words_alloc ;		// # of bytes allocated for m_words

      bool m_good ;			// valid vocabulary data?
      bool m_readonly ;		// disallow changes to vocab?
      bool m_changed ;		// unsaved updates?
      bool m_mapped ;			// is data memory-mapped?
      bool m_revmapped ;		// is revindex memory-mapped?
      bool m_contained ;		// inside another object's memmap?
      bool m_name_is_ID ;
      bool m_batch_update ;

   private: // methods
      void init(const char *filename, size_t userdatasize) ;
      bool load(bool allow_memory_mapping = true) ;
      bool load(FILE *fp) ;
      bool extractParameters(const char *fileheader) ;
      void setDataPointers(char *buffer) ;
      bool createFileHeader(char *&fileheader) ;
      bool unload(bool keep_open = false) ;
      bool unmemmap() ;
      bool expand() ;
      bool expandTo(size_t new_alloc) ;

      void setIndexEntry(size_t N, size_t wordloc, size_t ID) ;
      const char *getNameFromIndexEntry(size_t N) const ;

      size_t addWordName(const char *word) ;
      const char *add(const char *word, size_t N) ;

   public:
      FrVocabulary(size_t userdatasize = 0) { init(0,userdatasize) ; }
      FrVocabulary(const char *filename, off_t start_offset = 0,
		   bool readonly = false,
		   bool allow_memory_mapping = true) ;
      FrVocabulary(FILE *fp, const char *filename = 0,
		   off_t start_offset = 0, bool readonly = true) ;
      FrVocabulary(FrFileMapping *fmap, const char *filename = 0,
		   off_t start_offset = 0, bool readonly = true) ;
      FrVocabulary(const char *buffer, void *container) ;
      virtual ~FrVocabulary() ;

      virtual const char *signatureString() const ;

      const char *find(const char *word) const ;
      size_t findID(const char *word) const ;

      // note: the following two functions potentially invalidate ALL prior
      //    vocab pointers!
      const char *find(const char *word, bool add_if_missing) ;
      size_t findID(const char *word, bool add_if_missing) ;

      size_t addWord(const char *word) ;

      bool startBatchUpdate() ;
      size_t addWord(const char *word, size_t ID) ;
      bool finishBatchUpdate() ;

      bool save(const char *filename, off_t file_offset = 0) ;
      bool save(FILE *fp, off_t file_offset = 0) ;
      bool save(off_t file_offset = 0, bool force = false) ;

      // modifiers
      bool useNameAsID(bool nameID = true) ;
      bool setScaleFactor(size_t scalefac) ;

      // accessors
      bool good() const { return m_good ; }
      bool readonly() const { return m_readonly ; }
      bool changed() const { return m_changed ; }
      bool usingNameAsID() const { return m_name_is_ID ; }
      size_t userDataSize() const { return m_userdata_size ; }
      size_t numWords() const { return m_index_size ; }
      size_t totalDataSize() const ;	// total size on disk

      const char *nameForID(size_t ID) const ;
      size_t locationForID(size_t ID) const
	 { return (m_revindex && ID <= numWords())
	           ? FrLoadLong(m_revindex + 4 * ID) : (size_t)~0 ; }
      const char *userData(size_t ID) const
	 { return m_userdata ? m_userdata + m_userdata_size * ID : 0 ; }

      char *indexEntry(size_t N)
	    { return m_index + N * m_indexent_size ; }
      const char *indexEntry(size_t N) const
	    { return m_index + N * m_indexent_size ; }
      const char *getNameFromIndexEntry(const char *entry) const ;
      size_t getID(const char *indexent) const ;

      // utility funcs & support for indexing
      bool createReverseMapping() ;
      void freeReverseMapping() ;
      const char **makeWordList() ;
      static void freeWordList(const char **wordlist) ;
   } ;

#endif /* !FRVOCAB_H_INCLUDED */

// end of file frvocab.h //
