/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC								*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frtxtspn.h	classes FrTextSpan and FrTextSpans		*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 2005,2006,2007,2008,2009,2010				*/
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

#ifndef FRTXTSPN_H_INCLUDED
#define FRTXTSPN_H_INCLUDED

#ifndef __FRSTRUCT_H_INCLUDED
#include "frstruct.h"
#endif

/************************************************************************/
/************************************************************************/

enum FrTextSpan_Operation
   {
      FrTSOp_Default,
      FrTSOp_None,
      FrTSOp_Add,
      FrTSOp_Max,
      FrTSOp_Min,
      FrTSOp_Product,
      FrTSOp_Average
   } ;

//----------------------------------------------------------------------

class FrTextSpans ;
typedef char *FrTextSpanUpdateFn(char *prev_text) ;

class FrTextSpan : public FrObject
// note: only minimal FrObject support -- enough to let us use this class
//   in FrList and FrArray
   {
   private:
      static FrAllocator allocator ;
      double m_score ;
      double m_weight ;
      size_t m_start ;
      size_t m_end ;
      char *m_text ;
      FrStruct *m_metadata ;
      FrTextSpans *m_spans ;
   public:
      void *operator new(size_t,void *where) { return where ; }
      void *operator new(size_t) { return allocator.allocate() ; }
      void operator delete(void *blk,size_t) { allocator.release(blk) ; }
      FrTextSpan() {} ; // for internal use
      FrTextSpan(const FrTextSpan *old) ;
      FrTextSpan(size_t sp_start, size_t sp_end, double scor, const char *txt,
		 FrTextSpans *spans, double wt = 1.0)
	 { init(sp_start,sp_end,scor,wt,txt,spans) ; }
      virtual ~FrTextSpan() { clear() ; }
      virtual void freeObject() { delete this ; }
      virtual const char *objTypeName() const ;
      void init(const FrTextSpan *old, FrTextSpans *spans)
	 { if (old) init(old->start(),old->end(),old->score(),
			 old->weight(),old->text(),spans) ; }
      void init(size_t start, size_t end, double score, double weight,
		const char *text, FrTextSpans *spans) ;
      void clearMetaData() ;
      void clear()
	 { FrFree(m_text) ; m_text = 0 ; m_spans = 0 ; clearMetaData() ; }
      // accessors
      size_t start() const { return m_start ; }
      size_t end() const { return m_end ; }
      size_t pastEnd() const { return m_end + 1 ; }
      virtual size_t length() const { return pastEnd() - start() ; }
      size_t spanLength() const { return pastEnd() - start() ; }
      double score() const { return m_score ; }
      double weight() const { return m_weight ; }
      double weightedScore() const { return score() * weight() ; }
      const char *text() const { return m_text ; }
      const FrStruct *metaData() const { return m_metadata ; }
      const FrObject *getMetaData(const char *key) const ;
      const FrObject *getMetaData(FrSymbol *key) const ;
      const FrObject *getMetaDataSingle(const char *key) const  ;
      const FrObject *getMetaDataSingle(FrSymbol *key) const ;
      FrList *metaDataKeys() const ;
      FrTextSpans * container() const { return m_spans ; }

      // use FrFree() when done with the following results
      char *getText() const ;
      char *initialText() const ;
      char *originalText() const ;

      // statistics
      size_t wordCount() const ;
      size_t wordCountInitial() const ;
      size_t wordCountOriginal() const ;

      // manipulators
      static void swap(FrTextSpan &s1, FrTextSpan &s2) ;
      void setScore(double sc) { m_score = sc ; }
      void setWeight(double wt) { m_weight = wt ; }
      void updateScore(double incr, FrTextSpan_Operation = FrTSOp_Add) ;
      void updateWeight(double wt, FrTextSpan_Operation = FrTSOp_Product) ;
      void setMetaData(FrStruct *meta)
	 { free_object(m_metadata) ; m_metadata = meta ; }
      void updateText(const char *new_text) ;
      void updateText(char *new_text, bool copy) ;
      void updateText(FrTextSpanUpdateFn *fn) ;
      bool setMetaData(FrSymbol *key, const FrObject *value) ;
      bool setMetaData(FrSymbol *key, FrObject *value, bool copy) ;
      bool addMetaData(FrSymbol *key, const FrObject *value) ;
      bool updateMetaData(const FrStruct *meta) ;

      static FrTextSpan *merge(const FrTextSpan *span1,const FrTextSpan *span2,
			       FrTextSpan_Operation score_op,
			       FrTextSpan_Operation weight_op=FrTSOp_Default) ;
      // append 'other' to the end of the span
      bool merge(const FrTextSpan *other, FrTextSpan_Operation score_op,
		   FrTextSpan_Operation weight_op = FrTSOp_Default) ;

      // testing
      bool equal(const FrTextSpan *other, bool ignore_text = false) const ;
      static int compare(const FrTextSpan &s1, const FrTextSpan &s2) ;

      // I/O
      virtual ostream &printValue(ostream &) const ;
      FrList *printable() const ;	// dump to printable format
      bool parse(const FrList *printed, class FrTextSpans *container = 0) ;
   } ;

//----------------------------------------------------------------------

typedef bool FrTextSpanIterFunc(const FrTextSpan *, va_list) ;
typedef bool FrTextSpanMatchFunc(const FrTextSpan *, va_list) ;
typedef int FrTextSpanCompareFunc(const FrTextSpan *, const FrTextSpan *) ;

class FrTextSpans
  {
   private:
      char *m_text ;
      FrTextSpan *m_spans ;
      FrSymbolTable *m_symtab ;
      size_t m_spancount ;
      size_t m_textlength ;
      size_t *m_positions ;
      FrStruct m_metadata ;
      bool m_sorted ;

   private: // methods
      void clear() ;
      bool setPositionMap() ;
      void makeWordSpans() ;
      void makeWordSpans(const char *text, FrCharEncoding, const char *delim) ;
      void makeWordSpans(const FrList *defn) ;
      void parseSpans(const FrList *spans, size_t numstrings = 1) ;

   public:
      FrTextSpans(const FrTextSpans *old) ; // copy over base text from old
      FrTextSpans(const char *text, FrCharEncoding = FrChEnc_RawOctets,
		  const char *word_delim = 0) ;
      FrTextSpans(const FrObject *span_defn,
		  FrCharEncoding = FrChEnc_RawOctets,
		  const char *word_delim = 0) ;
      ~FrTextSpans() ;

      // accessors
      bool OK() const
	 { return m_spans && m_text && m_positions ; }
      const char *originalString() const { return m_text ; }
      size_t textLength() const { return m_textlength ; }
      char getChar(size_t index) const { return m_text[m_positions[index]] ; }
      const size_t *positionMap() const { return m_positions ; }
      char *getText(size_t start, size_t end) const ; // use FrFree() on result
      char *getText(size_t start, size_t end) ; // use FrFree() on result
      FrList *wordList() const ;
      size_t spanCount() const { return m_spancount ; }
      FrTextSpan *getSpan(size_t N)
	 { return (N < spanCount()) ? &m_spans[N] : 0 ; }
      const FrTextSpan *getSpan(size_t N) const
	 { return (N < spanCount()) ? &m_spans[N] : 0 ; }
      char *getEvalText(size_t N) const
	 { return (N < spanCount()) ? m_spans[N].getText() : 0 ; }
      FrObject *metaData(FrSymbol *key) const ;
      FrObject *metaData(const char *key) const ;
      const FrStruct *allMetaData() const { return &m_metadata ; }

      // since we may have changed symbol tables since creating the structure,
      //   the following methods give us access to the correct FrSymbols to
      //   access metadata records
      FrSymbol *makeSymbol(const char *symname) const ;
      FrSymbol *makeSymbol(FrSymbol *sym) const ;
      FrSymbolTable *symbolTable() const { return m_symtab ; }

      // use FrFree when done with the results of the following
      size_t *wordMapping(bool skip_whitespace = false) const ;

      // manipulators
      FrTextSpan *newSpan(const FrList *span_spec, const FrStruct *meta = 0) ;
      bool newSpans(const FrList *span_specs) ;
      FrTextSpan *addSpan(const FrTextSpan *span,
			  bool only_if_unique = false,
			  FrTextSpan_Operation score_op = FrTSOp_None,
			  FrTextSpan_Operation weight_op = FrTSOp_Default) ;
      bool removeSpan(const FrTextSpan *span) ;
      bool removeMatchingSpansVA(FrTextSpanMatchFunc *fn, va_list) ;
      bool removeMatchingSpans(FrTextSpanMatchFunc *fn, ...) ;
      void updateSpans(FrTextSpanUpdateFn *fn) ;
      bool merge(const FrTextSpans *other, bool remove_dups = false,
		   FrTextSpan_Operation score_op = FrTSOp_None,
		   FrTextSpan_Operation weight_op = FrTSOp_Default) ;
      bool setMetaData(FrSymbol *key, const FrObject *value) ;
      bool setMetaData(FrSymbol *key, FrObject *value, bool copy) ;
      bool addMetaData(FrSymbol *key, const FrObject *value) ;
      bool addMetaData(const FrStruct *meta) ;
      bool addAllMetaData(FrSymbol *key, const FrObject *value) ;
      bool insertMetaData(FrSymbol *key, FrObject *value,
			    bool copy = true) ;
      bool copyMetaData(const FrTextSpans *other) ;
      bool clearMetaData(FrSymbol *key, bool was_copied = true) ;

      void sort(FrTextSpanCompareFunc * = 0) ;
      // NOTE: optional compare function should use FrTextSpan::compare()
      //   first, and only do additional comparisons if that says equal,
      //   or lots of stuff may break
      bool iterateVA(FrTextSpanIterFunc *fn, va_list args) const ;
      bool iterate(FrTextSpanIterFunc *fn, ...) const ;

      // testing
      bool compatibleWith(const FrTextSpans *otherspans) const ;

      // I/O
      FrList *printable() const ;
      void _() const ;			// dump printable version to cout
   } ;

/************************************************************************/
/************************************************************************/

#endif /* !FRTXTSPN_H_INCLUDED */

// end of file frtxtspn.h //
