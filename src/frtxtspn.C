/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC								*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frtxtspn.C	classes FrTextSpan and FrTextSpans		*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 2005,2006,2007,2008,2009,2010,2012			*/
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

#include "framerr.h"
#include "frfloat.h"
#include "frqsort.h"
#include "frstring.h"
#include "frsymtab.h"
#include "frtxtspn.h"
#include "frutil.h"

/************************************************************************/
/*	Manifest Constants for this module				*/
/************************************************************************/

#define DEFAULT_SCORE 1.0
#define DEFAULT_WEIGHT 1.0

#define METADATA_TYPENAME "META"

/************************************************************************/
/*	Global Data for this module					*/
/************************************************************************/

static const char init_text_tag[] = "INIT_TEXT" ;

FrAllocator FrTextSpan::allocator("FrTextSpan",sizeof(FrTextSpan)) ;

/************************************************************************/
/*	Helper functions						*/
/************************************************************************/

static size_t count_strings(const FrList *list)
{
   size_t count = 0 ;
   for ( ; list ; list = list->rest())
      {
      const FrObject *item = list->first() ;
      if (item && (item->stringp() || item->symbolp()))
	 count++ ;
      }
   return count ;
}

/************************************************************************/
/*	Methods for class FrTextSpan					*/
/************************************************************************/

void FrTextSpan::init(size_t sp_start, size_t sp_end, double sc, double wt,
		      const char *txt, FrTextSpans *spans)
{
   if (spans)
      {
      if (sp_start >= spans->textLength())
	 sp_start = spans->textLength() - 1 ;
      if (sp_end >= spans->textLength())
	 sp_end = spans->textLength() - 1 ;
      }
   if (sp_end < sp_start)
      sp_end = sp_start ;
   if (wt < 0.0)
      wt = 0.0 ;
   m_start = sp_start ;
   m_end = sp_end ;
   m_score = sc ;
   m_weight = wt ;
   m_text = FrDupString(txt) ;
   m_spans = spans ;
   m_metadata = 0 ;
   return ;
}

//----------------------------------------------------------------------

FrTextSpan::FrTextSpan(const FrTextSpan *old)
{
   if (old)
      {
      init(old->start(),old->end(),old->score(),old->weight(),old->text(),
	   old->container()) ;
      if (old->metaData())
	 {
	 m_metadata = (FrStruct*)old->metaData()->deepcopy() ;
	 m_metadata->remove(FrSymbolTable::add(init_text_tag)) ;
	 }
      }
   return ;
}

//----------------------------------------------------------------------

const char *FrTextSpan::objTypeName() const
{
   return "FrTextSpan" ;
}

//----------------------------------------------------------------------

ostream &FrTextSpan::printValue(ostream &output) const
{
   FrList *span = printable() ;
   output << span ;
   free_object(span) ;
   return output ;
}

//----------------------------------------------------------------------

void FrTextSpan::clearMetaData()
{
   m_score = 0.0 ;
   m_weight = 0.0 ;
   free_object(m_metadata) ;
   m_metadata = 0 ;
   return ;
}

//----------------------------------------------------------------------

static double update_value(FrTextSpan_Operation op, double oldval,
			   double update)
{
   switch (op)
      {
      case FrTSOp_Default:
      case FrTSOp_None:
	 return oldval ;
      case FrTSOp_Add:
	 return oldval + update ;
      case FrTSOp_Product:
	 return oldval * update ;
      case FrTSOp_Min:
	 return (oldval < update) ? oldval : update ;
      case FrTSOp_Max:
	 return (oldval > update) ? oldval : update ;
      case FrTSOp_Average:
	 return (oldval + update) / 2 ;
      default:
	 FrMissedCase("update_value(FrTextSpan_Operation)") ;
      }
   return oldval ;
}

//----------------------------------------------------------------------

void FrTextSpan::updateScore(double amount, FrTextSpan_Operation op)
{
   setScore(update_value(op,score(),amount)) ;
}

//----------------------------------------------------------------------

void FrTextSpan::updateWeight(double amount, FrTextSpan_Operation op)
{
   setWeight(update_value(op,weight(),amount)) ;
}

//----------------------------------------------------------------------

void FrTextSpan::updateText(const char *new_text)
{
   FrSymbol *symINIT = FrSymbolTable::add(init_text_tag) ;
   if (!getMetaData(symINIT))
      {
      char *txt = getText() ;
      setMetaData(symINIT,new FrString(txt,strlen(txt),1,false),false) ;
      }
   FrFree(m_text) ;
   m_text = FrDupString(new_text) ;
   return ;
}

//----------------------------------------------------------------------

void FrTextSpan::updateText(char *new_text, bool copy_text)
{
   FrSymbol *symINIT = FrSymbolTable::add(init_text_tag) ;
   if (!getMetaData(symINIT))
      {
      char *txt = getText() ;
      setMetaData(symINIT,new FrString(txt,strlen(txt),1,false),false) ;
      }
   FrFree(m_text) ;
   m_text = copy_text ? FrDupString(new_text) : new_text ;
   return ;
}

//----------------------------------------------------------------------

void FrTextSpan::updateText(FrTextSpanUpdateFn *fn)
{
   if (fn)
      {
      FrSymbol *symINIT = FrSymbolTable::add(init_text_tag) ;
      if (!getMetaData(symINIT))
	 {
	 char *txt = getText() ;
	 setMetaData(symINIT,new FrString(txt,strlen(txt),1,false),false) ;
	 }
      if (m_text)
	 {
	 char *new_text = fn(m_text) ;
	 if (!new_text)
	    return ;
	 if (new_text != m_text)
	    {
	    FrFree(m_text) ;
	    m_text = new_text ;
	    }
	 return ;
	 }
      else
	 {
	 char *old_text = getText() ;
	 char *new_text = fn(old_text) ;
	 if (!new_text)
	    {
	    FrFree(old_text) ;
	    return ;
	    }
	 if (new_text != old_text)
	    FrFree(old_text) ;
	 m_text = new_text ;
	 }
      }
   return ;
}

//----------------------------------------------------------------------

bool FrTextSpan::setMetaData(FrSymbol *key, const FrObject *value)
{
   if (!metaData())
      {
      setMetaData(new FrStruct(FrSymbolTable::add(METADATA_TYPENAME))) ;
      if (!metaData())
	 return false ;
      }
   m_metadata->put(key,value) ;
   return true ;
}

//----------------------------------------------------------------------

bool FrTextSpan::setMetaData(FrSymbol *key, FrObject *value,
			       bool copy_data)
{
   if (!metaData())
      {
      setMetaData(new FrStruct(FrSymbolTable::add(METADATA_TYPENAME))) ;
      if (!metaData())
	 return false ;
      }
   m_metadata->put(key,value,copy_data) ;
   return true ;
}

//----------------------------------------------------------------------

bool FrTextSpan::addMetaData(FrSymbol *key, const FrObject *value)
{
   if (!metaData())
      {
      setMetaData(new FrStruct(FrSymbolTable::add(METADATA_TYPENAME))) ;
      if (!metaData())
	 return false ;
      }
   FrObject *oldvalue = m_metadata->get(key) ;
   if (oldvalue)
      {
      if (oldvalue->consp())
	 {
	 FrList *list = (FrList*)oldvalue->deepcopy() ;
	 if (value && value->consp())
	    {
	    FrList *newvalue = ((FrList*)value)->difference(list) ;
	    list = newvalue->nconc(list) ;
	    m_metadata->put(key,list,false) ;
	    }
	 else if (!list->member(value,::equal))
	    {
	    pushlist(value?value->deepcopy():0,list) ;
	    m_metadata->put(key,list,false) ;
	    }
	 else
	    free_object(list) ;
	 }
      else
	 return false ;
      }
   else if (value && value->consp())
      m_metadata->put(key,value->deepcopy(),false) ;
   else
      m_metadata->put(key,new FrList(value?value->deepcopy():0),false) ;
   return true ;
}

//----------------------------------------------------------------------

bool FrTextSpan::updateMetaData(const FrStruct *meta)
{
   bool changed = false ;
   if (meta)
      {
      FrList *fields = meta->fieldNames() ;
      while (fields)
	 {
	 FrSymbol *field = (FrSymbol*)poplist(fields) ;
	 FrObject *value = meta->get(field) ;
	 if (value &&addMetaData(field,value))
	    changed = true ;
	 }
      }
   return changed ;
}

//----------------------------------------------------------------------

double combine_values(double val1, double val2, FrTextSpan_Operation op)
{
   switch (op)
      {
      case FrTSOp_None:
	 // do nothing, keep original value
	 return val1 ;
      case FrTSOp_Default:
      case FrTSOp_Add:
	 return val1 + val2 ;
      case FrTSOp_Max:
	 return (val1 > val2) ? val1 : val2 ;
      case FrTSOp_Min:
	 return (val1 < val2) ? val1 : val2 ;
      case FrTSOp_Product:
	 return val1 * val2 ;
      case FrTSOp_Average:
	 return (val1 + val2) / 2.0 ;
      default:
	 FrMissedCase("combine_values") ;
	 return val1 ;
      }
}

//----------------------------------------------------------------------

bool FrTextSpan::merge(const FrTextSpan *other,
			 FrTextSpan_Operation score_op,
			 FrTextSpan_Operation weight_op)
{
   if (other && other->start() == pastEnd())
      {
      if (weight_op == FrTSOp_Default)
	 weight_op = score_op ;
      size_t len1 = strlen(text()) ;
      size_t len2 = strlen(other->text()) ;
      char *newtext = FrNewN(char,len1+len2+2) ;
      if (!newtext)
	 return false ;
      memcpy(newtext,text(),len1) ;
      newtext[len1] = ' ' ;
      memcpy(newtext+len1+1,other->text(),len2+1) ;
      m_text = newtext ;
      m_end = other->end() ;
      m_score = combine_values(score(),other->score(),score_op) ;
      m_weight = combine_values(weight(),other->weight(),weight_op) ;
      return true ;
      }
   return false ;
}

//----------------------------------------------------------------------

FrTextSpan *FrTextSpan::merge(const FrTextSpan *span1, const FrTextSpan *span2,
			 FrTextSpan_Operation score_op,
			 FrTextSpan_Operation weight_op)
{
   if (span1 && span2 && span1->pastEnd() == span2->start())
      {
      if (weight_op == FrTSOp_Default)
	 weight_op = score_op ;
      size_t len1 = strlen(span1->text()) ;
      size_t len2 = strlen(span2->text()) ;
      char *text = FrNewN(char,len1+len2+2) ;
      if (!text)
	 return 0 ;
      FrTextSpan *span = new FrTextSpan ;
      if (!span)
	 {
	 FrFree(text) ;
	 return 0 ;
	 }
      memcpy(text,span1->text(),len1) ;
      text[len1] = ' ' ;
      memcpy(text+len1+1,span2->text(),len2+1) ;
      span->m_text = text ;
      span->m_start = span1->start() ;
      span->m_end = span2->end() ;
      span->m_score = combine_values(span1->score(),span2->score(),score_op) ;
      span->m_weight = combine_values(span1->weight(),span2->weight(),
				      weight_op) ;
      return span ;
      }
   return 0 ;
}

//----------------------------------------------------------------------

const FrObject *FrTextSpan::getMetaData(FrSymbol *key) const
{
   return (key && metaData()) ? metaData()->get(key) : 0 ;
}

//----------------------------------------------------------------------

const FrObject *FrTextSpan::getMetaData(const char *key) const
{
   return (key && metaData()) ? metaData()->get(key) : 0 ;
}

//----------------------------------------------------------------------

const FrObject *FrTextSpan::getMetaDataSingle(FrSymbol *key) const
{
   const FrObject *meta = getMetaData(key) ;
   return (meta && meta->consp()) ? ((FrList*)meta)->first() : meta ;
}

//----------------------------------------------------------------------

const FrObject *FrTextSpan::getMetaDataSingle(const char *key) const
{
   const FrObject *meta = getMetaData(key) ;
   return (meta && meta->consp()) ? ((FrList*)meta)->first() : meta ;
}

//----------------------------------------------------------------------

FrList *FrTextSpan::metaDataKeys() const
{
   return metaData() ? metaData()->fieldNames() : 0 ;
}

//----------------------------------------------------------------------

char *FrTextSpan::getText() const
{
   return m_text ? FrDupString(m_text) : originalText() ;
}

//----------------------------------------------------------------------

char *FrTextSpan::initialText() const
{
   const FrObject *txt = getMetaData(init_text_tag) ;
   if (txt && txt->consp() && ((FrList*)txt)->first() &&
       ((FrList*)txt)->first()->stringp())
      txt = ((FrList*)txt)->first() ;
   const char *printed = FrPrintableName(txt) ;
   return printed ? FrDupString(printed) : originalText() ;
}

//----------------------------------------------------------------------

char *FrTextSpan::originalText() const
{
   return m_spans ? m_spans->getText(m_start,m_end) : 0 ;
}

//----------------------------------------------------------------------

static size_t word_count(const char *text)
{
   if (!text || !*text)
      return 0 ;
   size_t count = 1 ;
   while (*text)
      {
      while (*text && !Fr_isspace(*text))
	 text++ ;
      while (*text && Fr_isspace(*text))
	 text++ ;
      if (*text)
	 count++ ;
      }
   return count ;
}

//----------------------------------------------------------------------

size_t FrTextSpan::wordCount() const
{
   return m_text ? word_count(m_text) : wordCountOriginal() ;
}

//----------------------------------------------------------------------

size_t FrTextSpan::wordCountInitial() const
{
   const FrObject *txt = getMetaData(init_text_tag) ;
   if (txt && txt->consp() && ((FrList*)txt)->first() &&
       ((FrList*)txt)->first()->stringp())
      txt = ((FrList*)txt)->first() ;
   const char *printed = FrPrintableName(txt) ;
   return printed ? word_count(printed) : wordCountOriginal() ;
}

//----------------------------------------------------------------------

size_t FrTextSpan::wordCountOriginal() const
{
   char *words = originalText() ;
   size_t count = word_count(words) ;
   FrFree(words) ;
   return count ;
}

//----------------------------------------------------------------------

bool FrTextSpan::equal(const FrTextSpan *other, bool ignore_text) const
{
   if (!other)
      return (this == 0) ;
   if (start() != other->start() || end() != other->end())
      return false ;
   if (!ignore_text && strcmp(text(),other->text()) != 0)
      return false ;
   return true ;
}

//----------------------------------------------------------------------

int FrTextSpan::compare(const FrTextSpan &s1, const FrTextSpan &s2)
{
   // sort first by increasing start position, then decreasing length
   size_t start1 = s1.start() ;
   size_t start2 = s2.start() ;
   if (start1 < start2)
      return -1 ;
   else if (start1 > start2)
      return +1 ;
   size_t end1 = s1.end() ;
   size_t end2 = s2.end() ;
   if (end1 < end2)
      return +1 ;
   else if (end1 > end2)
      return -1 ;
   // finally, for spans covering the same text, sort by decreasing score
   double score1 = s1.score() ;
   double score2 = s2.score() ;
   if (score1 < score2)
      return +1 ;
   else if (score1 > score2)
      return -1 ;
   // And then sort by decreasing weight
   double weight1 = s1.weight() ;
   double weight2 = s2.weight() ;
   if (weight1 < weight2)
      return +1 ;
   else if (weight1 > weight2)
      return -1 ;
   return 0 ;
}

//----------------------------------------------------------------------

void FrTextSpan::swap(FrTextSpan &s1, FrTextSpan &s2)
{
   // the straightforward "FrTextSpan tmp(s1);s1=s2;s2=tmp;return;" causes
   //   the destructor to be called on "tmp", which in turn results in a double
   //   free down the line
   char tmpspace[sizeof(FrTextSpan)] ;
   FrTextSpan *tmp = (FrTextSpan*)tmpspace ;
#if 0
   *tmp = s1 ;
   s1 = s2 ;
   s2 = *tmp ;
#else
   memcpy(tmp,&s1,sizeof(FrTextSpan)) ;
   memcpy(&s1,&s2,sizeof(FrTextSpan)) ;
   memcpy(&s2,tmp,sizeof(FrTextSpan)) ;
#endif
   return ;
}

//----------------------------------------------------------------------

bool FrTextSpan::parse(const FrList *span, FrTextSpans *contain)
{
   if (span && span->consp() && span->simplelistlength() >= 2 &&
       span->first() && span->second() &&
       span->first()->numberp() && span->second()->numberp())
      {
      size_t sp_start = span->first()->intValue() ;
      size_t sp_end = span->second()->intValue() ;
      span = span->rest()->rest() ;
      // we'll allow rather free-form input from the rest of the span's
      //   description: the first two numbers are the score and weight,
      //   respectively, the first string is the span's text, the second
      //   string (if present) becomes the INIT_TEXT metadata.  Then,
      //   the first structure (if present) is the metadata, and any lists
      //   starting with a symbol become additional metadata fields
      double sp_score = DEFAULT_SCORE ;
      double sp_weight = DEFAULT_WEIGHT ;
      // scan for the first two numbers
      for (const FrList *sp = span ; sp ; sp = sp->rest())
	 {
	 if (sp->first() && sp->first()->numberp())
	    {
	    sp_score = sp->first()->floatValue() ;
	    for (sp = sp->rest() ; sp ; sp = sp->rest())
	       {
	       if (sp->first() && sp->first()->numberp())
		  {
		  sp_weight = sp->first()->floatValue() ;
		  break ;
		  }
	       }
	    break ;
	    }
	 }
      const char *curr_text = 0 ;
      const char *orig_text = 0 ;
      // scan for the first two strings or symbols
      for (const FrList *sp = span ; sp ; sp = sp->rest())
	 {
	 FrObject *item = sp->first() ;
	 if (item && (item->stringp() || item->symbolp()))
	    {
	    curr_text = item->printableName() ;
	    for (sp = sp->rest() ; sp ; sp = sp->rest())
	       {
	       item = sp->first() ;
	       if (item && (item->stringp() || item->symbolp()))
		  {
		  orig_text = item->printableName() ;
		  break ;
		  }
	       }
	    break ;
	    }
	 }
      if (curr_text)
	 (void)FrSkipWhitespace(curr_text) ;
      if (orig_text)
	 (void)FrSkipWhitespace(orig_text) ;
      // scan for the first structure
      const FrStruct *meta = 0 ;
      for (const FrList *sp = span ; sp ; sp = sp->rest())
	 {
	 if (sp->first() && sp->first()->structp())
	    {
	    meta = (FrStruct*)sp->first() ;
	    break ;
	    }
	 }
      init(sp_start,sp_end,sp_score,sp_weight,curr_text,contain) ;
      free_object(m_metadata) ;
      if (meta)
	 {
	 FrSymbol *symMETATYPE = FrSymbolTable::add(METADATA_TYPENAME) ;
	 if (meta->typeName() == symMETATYPE)
	    m_metadata = (FrStruct*)meta->deepcopy() ;
	 else
	    {
	    // copy the keywords one by one
	    FrList *keys = meta->fieldNames() ;
	    while (keys)
	       {
	       FrSymbol *key = (FrSymbol*)poplist(keys) ;
	       setMetaData(key,meta->get(key)) ;
	       }
	    }
	 }
      if (orig_text)
	 setMetaData(FrSymbolTable::add(init_text_tag),
		     new FrString(orig_text),false) ;
      // finally, scan for any embedded lists and add them as metadata fields
      for (const FrList *sp = span ; sp ; sp = sp->rest())
	 {
	 FrList *item = (FrList*)sp->first() ;
	 if (item && item->consp() && item->first() &&
	     item->first()->symbolp())
	    {
	    FrSymbol *key = (FrSymbol*)item->first() ;
	    setMetaData(key,item->rest()) ;
	    }
	 }
      return true ;
      }
   return false ;
}

//----------------------------------------------------------------------

FrList *FrTextSpan::printable() const
{
   FrList *span = 0 ;
   const FrStruct *meta = metaData() ;
   FrList *fields = meta ? meta->fieldNames() : 0 ;
   while (fields)
      {
      FrSymbol *key = (FrSymbol*)poplist(fields) ;
      FrObject *data = meta->get(key) ;
      if (data)
	 {
	 data = data->deepcopy() ;
	 if (!data->consp())
	    data = new FrList(data) ;
	 FrList *datalist = (FrList*)data ;
	 pushlist(key,datalist) ;
	 pushlist(datalist,span) ;
	 }
      }
   pushlist(new FrFloat(weight()),span) ;
   pushlist(new FrFloat(score()),span) ;
   pushlist(new FrString(text()),span) ;
   pushlist(new FrInteger(end()),span) ;
   pushlist(new FrInteger(start()),span) ;
   return span ;
}

/************************************************************************/
/*	Methods for class FrTextSpans					*/
/************************************************************************/

FrTextSpans::FrTextSpans(const char *text, FrCharEncoding enc,
			 const char *word_delim)
{
   clear() ;
   makeWordSpans(text,enc,word_delim) ;
   return ;
}

//----------------------------------------------------------------------

FrTextSpans::FrTextSpans(const FrObject *span_defn, FrCharEncoding enc,
			 const char *word_delim)
{
   clear() ;
   if (span_defn)
      {
      if (span_defn->stringp())
	 makeWordSpans(((FrString*)span_defn)->stringValue(),enc,word_delim) ;
      else if (span_defn->symbolp())
	 makeWordSpans(((FrSymbol*)span_defn)->symbolName(),enc,word_delim) ;
      else if (span_defn->consp())
	 {
	 // parse the given list into the original text and individual
	 //   spans
	 const FrList *defn = (FrList*)span_defn ;
	 size_t num_strings = count_strings(defn) ;
	 size_t defn_len = defn->simplelistlength() ;
	 // if the list consists solely of FrString or FrSymbol, then we
	 //   concatenate them to form the original text and make each one
	 //   a separate span
	 if (num_strings == defn_len)
	    makeWordSpans(defn) ;
	 // if there's exactly one FrString or FrSymbol, that is our original
	 //   text, and the rest of the list elements define the spans over
	 //   that text
	 else if (num_strings == 1)
	    parseSpans(defn) ;
	 else
	    FrWarning("span definition for FrTextSpans ctor must contain\n"
		      "\teither exactly one string or only strings") ;
	 }
      else
	 {
	 FrWarning("invalid span definition given to FrTextSpans ctor") ;
	 }
      }
   return ;
}

//----------------------------------------------------------------------

FrTextSpans::FrTextSpans(const FrTextSpans *old)
{
   clear() ;
   if (old)
      {
      m_text = FrDupString(old->m_text) ;
      setPositionMap() ;
      if (m_text && m_positions)
	 {
	 FrList *fields = old->m_metadata.fieldNames() ;
	 while (fields)
	    {
	    FrSymbol *key = (FrSymbol*)poplist(fields) ;
	    setMetaData(key,old->metaData(key)) ;
	    }
	 }
      else
	 {
	 FrFree(m_text) ;	m_text = 0 ;
	 FrFree(m_positions) ;	m_positions = 0 ;
	 m_textlength = 0 ;
	 }
      }
   return ;
}

//----------------------------------------------------------------------

FrTextSpans::~FrTextSpans()
{
   for (size_t i = 0 ; i < spanCount() ; i++)
      {
      m_spans[i].clear() ;
      }
   FrFree(m_spans) ;		m_spans = 0 ;
   FrFree(m_text) ;		m_text = 0 ;
   FrFree(m_positions) ;	m_positions = 0 ;
   m_spancount = 0 ;
   m_textlength = 0 ;
   return ;
}

//----------------------------------------------------------------------

void FrTextSpans::clear()
{
   m_text = 0 ;
   m_spans = 0 ;
   m_symtab = FrSymbolTable::current() ;
   m_spancount = 0 ;
   m_textlength = 0 ;
   m_positions = 0 ;
   m_metadata.setStructType(makeSymbol("META")) ;
   m_sorted = true ;
   return ;
}

//----------------------------------------------------------------------

bool FrTextSpans::setPositionMap()
{
   if (!m_text)
      {
      m_textlength = 0 ;
      return false ;
      }
   m_textlength = strlen(m_text) ;
   m_positions = FrNewC(size_t,m_textlength+1) ;
   if (m_positions)
      {
      size_t pos = 0 ;
      for (size_t i = 0 ; i < m_textlength ; i++)
	 {
	 if (m_text[i] && !Fr_isspace(m_text[i]))
	    m_positions[pos++] = i ;
	 }
      m_positions[pos] = m_textlength ;
      // adjust for the blanks in the original text
      m_textlength = pos ;
      return true ;
      }
   return false ;
}

//----------------------------------------------------------------------

void FrTextSpans::makeWordSpans(const FrList *defn)
{
   FrString *concat = new FrString(defn) ;
   m_text = FrDupString(concat->stringValue()) ;
   free_object(concat) ;
   if (setPositionMap())
      {
      m_spancount = defn->simplelistlength() ;
      m_spans = FrNewN(FrTextSpan,m_spancount) ;
      if (!m_spans)
	 {
	 m_spancount = 0 ;
	 return ;
	 }
      size_t spannum = 0 ;
      size_t start = 0 ;
      size_t end = 0 ;
      for ( ; defn ; defn = defn->rest())
	 {
	 FrObject *def = defn->first() ;
	 if (def && def->structp())
	    {
	    addMetaData((FrStruct*)def) ;
	    continue ;
	    }
	 const char *item = FrPrintableName(def) ;
	 if (!item)
	    item = "" ;
	 for (const char *i = item ; *i ; i++)
	    {
	    if (!Fr_isspace(*i))
	       end++ ;
	    }
	 if (end > 0)
	    m_spans[spannum++].init(start,end-1,DEFAULT_SCORE,DEFAULT_WEIGHT,
				    item,this) ;
	 m_sorted = false ;
	 start = end ;
	 }
      m_spancount = spannum ;
      }
   return ;
}

//----------------------------------------------------------------------

void FrTextSpans::makeWordSpans(const char *text, FrCharEncoding enc,
				const char *word_delim)
{
   m_text = FrDupString(text) ;
   if (m_text)
      {
      if (!setPositionMap())
	 {
	 m_textlength = 0 ;
	 return ;
	 }
      char *canon = FrCanonicalizeSentence(m_text,enc,false,word_delim) ;
      if (canon && *canon)
	 {
	 m_spancount = 1 ;
	 for (char *cptr = canon ; *cptr ; cptr++)
	    {
	    if (' ' == *cptr)
	       m_spancount++ ;
	    }
	 m_spans = FrNewN(FrTextSpan,m_spancount) ;
	 if (m_spans)
	    {
	    size_t tpos = 0 ;
	    size_t cpos = 0 ;
	    size_t start = 0 ;
	    size_t end = 0 ;
	    for (size_t i = 0 ; i < m_spancount ; i++)
	       {
	       // scan over the nonwhitespace chars at the current location
	       for ( ; canon[cpos] && canon[cpos] != ' ' ; cpos++)
		  {
		  tpos++ ;
		  end++ ;		// counts toward m_positions index
		  }
	       // skip over any trailing whitespace
	       while (m_text[tpos] && Fr_isspace(m_text[tpos]))
		  tpos++ ;
	       if (canon[cpos] == ' ')
		  cpos++ ;
	       m_spans[i].init(start,end-1,DEFAULT_SCORE,DEFAULT_WEIGHT,
			       0,this) ;
	       m_sorted = false ;
	       start = end ;
	       }
	    }
	 }
      FrFree(canon) ;
      }
   return ;
}

//----------------------------------------------------------------------

void FrTextSpans::parseSpans(const FrList *spans, size_t numstrings)
{
   for (const FrList *sp = spans ; sp ; sp = sp->rest())
      {
      const FrObject *item = sp->first() ;
      if (item && (item->stringp() || item->symbolp()))
	 {
	 m_text = FrDupString(FrPrintableName(item)) ;
	 if (m_text && setPositionMap())
	    break ;
	 return ;
	 }
      }
   m_spancount = spans->simplelistlength() - numstrings ;
   m_spans = FrNewN(FrTextSpan,m_spancount) ;
   if (!m_spans)
      {
      m_spancount = 0 ;
      return ;
      }
   size_t count = 0 ;
   for ( ; spans ; spans = spans->rest())
      {
      FrList *span = (FrList*)spans->first() ;
      if (!span)
	 continue ;
      if (span->consp() && m_spans[count].parse(span,this))
	 {
	 count++ ;
	 m_sorted = false ;
	 }
      else if (span->structp())
	 {
	 FrStruct *meta = (FrStruct*)span ;
	 if (meta->typeName() &&
	     Fr_stricmp(FrPrintableName(meta->typeName()),"META") == 0)
	    addMetaData(meta) ;
	 }
      }
   m_spancount = count ;
   return ;
}

//----------------------------------------------------------------------

char *FrTextSpans::getText(size_t start, size_t end) const
{
   if (end < start)
      end = start ;
   if (m_text)
      {
      if (m_positions && end < m_textlength)
	 {
	 start = m_positions[start] ;
	 end = m_positions[end+1] ;
	 }
      // strip trailing whitespace left by the m_positions mapping
      while (end > start && Fr_isspace(m_text[end-1]))
	 end-- ;
      char *buf = FrNewN(char,end-start+1) ;
      if (buf)
	 {
	 strncpy(buf,m_text+start,end-start) ;
	 buf[end-start] = '\0' ;
	 }
      return buf ;
      }
   // no original text stored, and we're not allowed to change anything,
   //   so give up
   return 0 ;
}

//----------------------------------------------------------------------

char *FrTextSpans::getText(size_t start, size_t end)
{
   if (end < start)
      end = start ;
   if (m_text)
      {
      if (m_positions && end < m_textlength)
	 {
	 start = m_positions[start] ;
	 end = m_positions[end+1] ;
	 }
      // strip trailing whitespace left by the m_positions mapping
      while (end > start && Fr_isspace(m_text[end-1]))
	 end-- ;
      char *buf = FrNewN(char,end-start+1) ;
      if (buf)
	 {
	 strncpy(buf,m_text+start,end-start) ;
	 buf[end-start] = '\0' ;
	 }
      return buf ;
      }
   // no original text stored, so try to assemble a string from the spans,
   //   if all spans are unambiguous
   sort() ;				// ensure that spans are sorted by posn
   bool unambig = true ;
   for (size_t i = start ; i <= end ; i++)
      {
      if (m_positions[i+1] > m_positions[i] + 1)
	 {
	 unambig = false ;
	 break ;
	 }
      }
   if (unambig)
      {
      size_t i ;
      size_t len = 0 ;
      for (i = start ; i <= end ; i++)
	 {
	 char *text = m_spans[i].originalText() ;
	 if (text)
	    {
	    len += strlen(text) + 1 ;
	    FrFree(text) ;
	    }
	 }
      char *buf = FrNewN(char,len+1) ;
      if (buf)
	 {
	 *buf = '\0' ;
	 char *bufptr = buf ;
	 for (i = start ; i <= end ; i++)
	    {
	    char *text = m_spans[i].originalText() ;
	    if (text)
	       {
	       len = strlen(text) ;
	       memcpy(bufptr,text,len) ;
	       bufptr[len] = ' ' ;
	       bufptr += len + 1 ;
	       }
	    }
	 if (bufptr > buf)		// change trailing blank into string
	    bufptr[-1] = '\0' ;		//   terminator
	 }
      return buf ;
      }
   // if we get here, we were unable to satisfy the request
   return 0 ;
}

//----------------------------------------------------------------------

FrList *FrTextSpans::wordList() const
{
   FrList *words = 0 ;
   FrList **end = &words ;
   for (size_t i = 0 ; i < spanCount() ; i++)
      {
      const FrTextSpan *span = getSpan(i) ;
      char *text = span->getText() ;
      words->pushlistend(new FrString(text,strlen(text),1,false),end) ;
      }
   *end = 0 ;				// properly terminate the list
   return words ;
}

//----------------------------------------------------------------------

FrTextSpan *FrTextSpans::newSpan(const FrList *span_spec, const FrStruct *meta)
{
   if (!span_spec)
      return 0 ;
   FrTextSpan *new_spans = FrNewR(FrTextSpan,m_spans,m_spancount+1) ;
   if (new_spans)
      {
      m_spans = new_spans ;
      new (&m_spans[m_spancount]) FrTextSpan ;
      if (meta)
	 {
	 FrList *fields = meta->fieldNames() ;
	 while (fields)
	    {
	    FrSymbol *key = (FrSymbol*)poplist(fields) ;
	    m_spans[m_spancount].setMetaData(key,meta->get(key)) ;
	    }
	 }
      // any metadata in the span_spec will override the separately-passed
      //   metadata; this allows e.g. copying over existing data from an old
      //   span that has been processed into the new spec but may not have
      //   carried along all the metadata
      FrTextSpan *span = &m_spans[m_spancount] ;
      m_spans[m_spancount++].parse(span_spec,this) ;
      m_sorted = false ;
      return span ;
      }
   return 0 ;
}

//----------------------------------------------------------------------

bool FrTextSpans::newSpans(const FrList *span_specs)
{
   if (!span_specs)
      return false ;			// no spans added
   size_t newcount = span_specs->simplelistlength() ;
   FrTextSpan *new_spans = FrNewR(FrTextSpan,m_spans,m_spancount+newcount) ;
   bool added = false ;
   if (new_spans)
      {
      m_spans = new_spans ;
      for (const FrList *specs = span_specs ; specs ; specs = specs->rest())
	 {
	 FrList *span_spec = (FrList*)specs->first() ;
	 if (!span_spec || !span_spec->consp())
	    continue ;
	 new (&m_spans[m_spancount]) FrTextSpan ;
	 m_spans[m_spancount++].parse(span_spec,this) ;
	 m_sorted = false ;
	 added = true ;
	 }
      }
   return added ;
}

//----------------------------------------------------------------------

FrTextSpan *FrTextSpans::addSpan(const FrTextSpan *span, bool only_if_unique,
				 FrTextSpan_Operation score_op,
				 FrTextSpan_Operation weight_op)
{
   if (weight_op == FrTSOp_Default)
      weight_op = score_op ;
   if (only_if_unique || score_op != FrTSOp_None || weight_op != FrTSOp_None)
      {
      // check whether the span already exists
      for (size_t i = 0 ; i < spanCount() ; i++)
	 {
	 if (getSpan(i)->equal(span))
	    {
	    if (only_if_unique)
	       return 0 ;		// didn't add the span
	    else
	       {
	       m_spans[i].updateScore(span->score(),score_op) ;
	       m_spans[i].updateWeight(span->weight(),weight_op) ;
	       m_spans[i].updateMetaData(span->metaData()) ;
	       return &m_spans[i] ;
	       }
	    }
	 }
      }
   // adding the new span
   FrTextSpan *new_spans = FrNewR(FrTextSpan,m_spans,m_spancount+1) ;
   if (new_spans)
      {
      m_spans = new_spans ;
      FrTextSpan *newspan = &m_spans[m_spancount] ;
      m_spans[m_spancount].init(span,this) ;
      m_spans[m_spancount].updateMetaData(span->metaData()) ;
      m_spancount++ ;
      m_sorted = false ;
      return newspan ;
      }
   else
      return 0 ;
}

//----------------------------------------------------------------------

bool FrTextSpans::removeSpan(const FrTextSpan *span)
{
   if (span)
      {
      for (size_t i = 0 ; i < spanCount() ; i++)
	 {
	 if (&m_spans[i] == span)
	    {
	    for (i++ ; i < spanCount() ; i++)
	       {
	       m_spans[i-1] = m_spans[i] ;
	       }
	    m_spans[--m_spancount].clear() ;
	    return true ;
	    }
	 }
      }
   return false ;
}

//----------------------------------------------------------------------

bool FrTextSpans::removeMatchingSpansVA(FrTextSpanMatchFunc *fn,va_list args)
{
   bool removed = false ;
   if (fn)
      {
      size_t dest = 0 ;
      for (size_t i = 0 ; i < spanCount() ; i++)
	 {
	 FrTextSpan *span = getSpan(i) ;
	 FrSafeVAList(args) ;
	 if (fn(span,FrSafeVarArgs(args)))
	    {
	    m_spans[i].clear() ;
	    removed = true ;
	    }
	 else
	    m_spans[dest++] = m_spans[i] ;
	 FrSafeVAListEnd(args) ;
	 }
      m_spancount = dest ;
      }
   return removed ;
}

//----------------------------------------------------------------------

bool FrTextSpans::removeMatchingSpans(FrTextSpanMatchFunc *fn, ...)
{
   va_list args ;
   va_start(args,fn) ;
   bool status = removeMatchingSpansVA(fn,args) ;
   va_end(args) ;
   return status ;
}

//----------------------------------------------------------------------

void FrTextSpans::updateSpans(FrTextSpanUpdateFn *fn)
{
   if (fn)
      {
      for (size_t i = 0 ; i < spanCount() ; i++)
	 {
	 FrTextSpan *span = &m_spans[i] ;
	 if (span)
	    span->updateText(fn) ;
	 }
      }
   return ;
}

//----------------------------------------------------------------------

bool FrTextSpans::merge(const FrTextSpans *other, bool remove_dups,
			  FrTextSpan_Operation score_op,
			  FrTextSpan_Operation weight_op)
{
   if (!compatibleWith(other))
      return false ;
   for (size_t i = 0 ; i < other->spanCount() ; i++)
      {
      const FrTextSpan *span = other->getSpan(i) ;
      if (!addSpan(span,remove_dups,score_op,weight_op) && !remove_dups)
	 return false ;
      }
   return true ;
}

//----------------------------------------------------------------------

FrObject *FrTextSpans::metaData(FrSymbol *key) const
{
   return m_metadata.get(key) ;
}

//----------------------------------------------------------------------

FrObject *FrTextSpans::metaData(const char *key) const
{
   return m_metadata.get(key) ;
}

//----------------------------------------------------------------------

FrSymbol *FrTextSpans::makeSymbol(const char *symname) const
{
   if (!symname)
      symname = "" ;
   if (FrSymbolTable::current() != symbolTable())
      {
      FrSymbolTable *symtab = symbolTable()->select() ;
      FrSymbol *sym = FrSymbolTable::add(symname) ;
      symtab->select() ;
      return sym ;
      }
   return FrSymbolTable::add(symname) ;

}

//----------------------------------------------------------------------

FrSymbol *FrTextSpans::makeSymbol(FrSymbol *symbol) const
{
   if (FrSymbolTable::current() != symbolTable() && symbol)
      {
      FrSymbolTable *symtab = symbolTable()->select() ;
      FrSymbol *sym = FrSymbolTable::add(symbol->symbolName()) ;
      symtab->select() ;
      return sym ;
      }
   else
      return symbol ;
}

//----------------------------------------------------------------------

bool FrTextSpans::setMetaData(FrSymbol *key, const FrObject *value)
{
   m_metadata.put(key,value) ;
   return true ;
}

//----------------------------------------------------------------------

bool FrTextSpans::setMetaData(FrSymbol *key, FrObject *value,
			       bool copy)
{
   m_metadata.put(key,value,copy) ;
   return true ;
}

//----------------------------------------------------------------------

bool FrTextSpans::addMetaData(FrSymbol *key, const FrObject *value)
{
   FrStructField *field = m_metadata.getField(key) ;
   if (!field)
      return setMetaData(key,value) ;
   FrObject *oldvalue = field->get() ;
   if (oldvalue)
      {
      if (oldvalue->consp())
	 {
	 FrList *list = (FrList*)oldvalue ;
	 if (value && value->consp())
	    {
	    FrList *newvalue = ((FrList*)value)->difference(list) ;
	    list = newvalue->nconc(list) ;
	    }
	 else if (!list->member(value,::equal))
	    pushlist(value?value->deepcopy():0,list) ;
	 field->set(list) ;
	 }
      else
	 return false ;
      }
   else if (value && value->consp())
      field->set(value->deepcopy()) ;
   else
      field->set(new FrList(value?value->deepcopy():0)) ;
   return true ;
}

//----------------------------------------------------------------------

bool FrTextSpans::addMetaData(const FrStruct *meta)
{
   bool success = false ;
   if (meta)
      {
      FrList *fields = meta->fieldNames() ;
      while (fields)
	 {
	 FrSymbol *key = (FrSymbol*)poplist(fields) ;
	 addMetaData(key,meta->get(key)) ;
	 success = true ;
	 }
      }
   return success ;
}

//----------------------------------------------------------------------

bool FrTextSpans::addAllMetaData(FrSymbol *key, const FrObject *value)
{
   bool success = true ;
   for (size_t i = 0 ; i < spanCount() ; i++)
      m_spans[i].addMetaData(key,value) ;
   return success ;
}

//----------------------------------------------------------------------

bool FrTextSpans::insertMetaData(FrSymbol *key, FrObject *value,
				 bool copy)
{
   FrStructField *field = m_metadata.getField(key) ;
   if (!field)
      return setMetaData(key,value,copy) ;
   FrObject *oldvalue = field->get() ;
   if (oldvalue)
      {
      if (!oldvalue->consp())
	 {
	 oldvalue = new FrList(oldvalue) ;
	 field->set(oldvalue) ;
	 }
      FrList *list = (FrList*)oldvalue ;
      if (!list->member(value,::equal))
	 {
	 pushlist(value&&copy?value->deepcopy():value,list) ;
	 field->set(list) ;
	 }
      }
   else if (value && value->consp())
      field->set(copy?value->deepcopy():value) ;
   else
      field->set(new FrList((value&&copy)?value->deepcopy():value)) ;
   return true ;
}

//----------------------------------------------------------------------

bool FrTextSpans::copyMetaData(const FrTextSpans *other)
{
   return other ? addMetaData(&other->m_metadata) : false ;
}

//----------------------------------------------------------------------

bool FrTextSpans::clearMetaData(FrSymbol *key, bool was_copied)
{
   FrStructField *field = m_metadata.getField(key) ;
   if (field)
      {
      FrObject *val = field->get() ;
      field->set(0) ;
      if (was_copied)
	 free_object(val) ;
      else if (val && val->consp())
	 ((FrList*)val)->eraseList(false) ;
      return true ;
      }
   return false ;			// not removed
}

//----------------------------------------------------------------------

size_t *FrTextSpans::wordMapping(bool skip_white) const
{
   if (!originalString() || textLength() == 0)
      return 0 ;
   size_t *map = FrNewN(size_t,textLength()+1) ;
   if (map)
      {
      size_t count = 0 ;
      map[0] = count ;
      if (skip_white)
	 {
	 size_t i ;
	 for (i = 1 ; i <= textLength() ; i++)
	    {
	    if (m_positions[i] != m_positions[i-1] + 1)
	       count++ ;
	    map[i] = count ;
	    }
	 }
      else
	 {
	 for (size_t i = 1 ; i <= textLength() ; i++)
	    {
	    if (Fr_isspace(m_text[i-1]) && !Fr_isspace(m_text[i]))
	       count++ ;
	    map[i] = count ;
	    }
	 }
      }
   return map ;
}

//----------------------------------------------------------------------

void FrTextSpans::sort(FrTextSpanCompareFunc *cmp)
{
   if (m_sorted == false)
      {
      if (cmp)
	 {
	 FrQuickSort(m_spans,m_spancount,cmp) ;
	 }
      else
	 {
	 FrQuickSort(m_spans,m_spancount) ;
	 }
      m_sorted = true ;
      }
   return ;
}

//----------------------------------------------------------------------

bool FrTextSpans::iterateVA(FrTextSpanIterFunc *fn, va_list args) const
{
   bool success = (fn != 0) ;
   for (size_t i = 0 ; i < spanCount() && success ; i++)
      {
      FrSafeVAList(args) ;
      success = fn(getSpan(i),FrSafeVarArgs(args)) ;
      FrSafeVAListEnd(args) ;
      }
   return success ;
}

//----------------------------------------------------------------------

bool FrTextSpans::iterate(FrTextSpanIterFunc *fn, ...) const
{
   va_list args ;
   va_start(args,fn) ;
   bool success = this ? iterateVA(fn,args) : true ;
   va_end(args) ;
   return success ;
}

//----------------------------------------------------------------------

bool FrTextSpans::compatibleWith(const FrTextSpans *other) const
{
   if (other && textLength() == other->textLength())
      {
      // same number of nonwhitespace chars, so check if they're the same chars
      for (size_t i = 0 ; i < textLength() ; i++)
	 {
	 char c1 = getChar(i) ;
	 char c2 = other->getChar(i) ;
	 if (c1 != c2)
	    return false ;
	 }
      return true ;
      }
   return false ;
}

//----------------------------------------------------------------------

FrList *FrTextSpans::printable() const
{
   FrList *result = 0 ;
   for (size_t i = 0 ; i < spanCount() ; i++)
      {
      pushlist(getSpan(i)->printable(),result) ;
      }
   FrList *meta_fields = m_metadata.fieldNames() ;
   if (meta_fields)
      {
      pushlist(m_metadata.deepcopy(),result) ;
      meta_fields->freeObject() ;
      }
   result = listreverse(result) ;
   pushlist(new FrString(originalString()),result) ;
   return result ;
}

//----------------------------------------------------------------------

void FrTextSpans::_() const
{
   FrList *printed = printable() ;
   cout << printed << endl ;
   free_object(printed) ;
   return ;
}

//----------------------------------------------------------------------

// end of file frtxtspn.C //
