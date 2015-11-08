/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC								*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frmorphp.cpp	morphology- and NE-marker parsing functions	*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 2006,2007,2009,2011,2012				*/
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

#include "frmorphp.h"

/************************************************************************/
/*	Global data for this module					*/
/************************************************************************/

static FrSymbol *symbolMORPH = 0 ;
static char *intro_str = 0 ;
static char *separator_str = 0 ;
static char *assignment_str = 0 ;
static bool use_morphology = false ;
static bool uppercase_morphology = true ;
static const unsigned char *uppercase_table = 0 ;

/************************************************************************/
/************************************************************************/

inline int fr_stricmp(const char *s1, const char *s2)
{
   int diff = 0 ;
   register const unsigned char *map = uppercase_table ;
   while ((diff = map[(unsigned char)*s1] - map[(unsigned char)*s2]) == 0 &&
	  *s1)
      {
      s1++ ;
      s2++ ;
      }
   return diff ;
}

//----------------------------------------------------------------------
// perform a case-insensitive equality comparison between two FrStrings
// or between a FrString and a Symbol (for other type combinations, fall
// back to default FramepaC comparison)

static bool casefold_equal(const FrObject *s1, const FrObject *s2)
{
   if (s1 == s2)
      return true ;
   else if (!s1 || !s2)		// can't be equal if only one is NULL
      return false ;
   else
      {
      const char *str1 = s1->printableName() ;
      const char *str2 = s2->printableName() ;
      if (str1 && str2)
	 return fr_stricmp(str1,str2) == 0 ;
      else if (str1 || str2)
	 return false ;
      else if (s1->consp() && s2->consp())
	 {
	 const FrList *l1 = (FrList*)s1 ;
	 const FrList *l2 = (FrList*)s2 ;
	 while (l1 && l2)
	    {
	    if (!casefold_equal(l1->first(),l2->first()))
	       return false ;
	    l1 = l1->rest() ;
	    l2 = l2->rest() ;
	    }
	 return (l1 == 0 && l2 == 0) ;
	 }
      }
   return s1->equal(s2) ;
}

//----------------------------------------------------------------------

void FrSetMorphologyMarkers(const char *intro, const char *separator,
			    const char *assignment)
{
   FrFree(intro_str) ;
   intro_str = FrDupString(intro) ;
   FrFree(separator_str) ;
   separator_str = FrDupString(separator) ;
   FrFree(assignment_str) ;
   assignment_str = FrDupString(assignment) ;
   symbolMORPH = FrSymbolTable::add("MORPH") ;
   return ;
}

//----------------------------------------------------------------------

void FrClearMorphologyMarkers()
{
   FrFree(intro_str) ;		intro_str = 0 ;
   FrFree(separator_str) ;	separator_str = 0 ;
   FrFree(assignment_str) ;	assignment_str = 0 ;
   return ;
}

//----------------------------------------------------------------------

void FrSetMorphologyFeatures(bool use, const unsigned char *uppercase_map)
{
   use_morphology = use ;
   uppercase_morphology = uppercase_map != 0 ;
   uppercase_table = uppercase_map ;
   return ;
}

//----------------------------------------------------------------------

FrSymbol *FrClassifyMorphologyTag(FrSymbol *morph, FrSymbol *&value,
				  const FrList *morphology_classes)
{
   FrSymbol *mclass = symbolMORPH ;	// default catch-all class
   FrList *mapping = (FrList*)morphology_classes->assoc(morph) ;
   if (!value)
      value = morph ;
   if (mapping && mapping->consp())
      {
      // pick out the class for this morphology tag
      if (mapping->second())
	 {
	 FrSymbol *m = FrCvt2Symbol(mapping->second()) ;
	 if (m)
	    mclass = m ;
	 }
      // if a third element is present, remap the tag's value to that value
      if (mapping->third())
	 {
	 FrSymbol *m = FrCvt2Symbol(mapping->third()) ;
	 if (m)
	    value = m ;
	 }
      }
   return mclass ;
}

//----------------------------------------------------------------------

static void add_to_morph(FrList *&all_morph, FrSymbol *key, FrObject *value)
{
   FrList *morph = (FrList*)all_morph->assoc(key) ;
   if (morph)
      {
      FrList *values = morph->rest() ;
      if (!values->member(value))
	 {
	 pushlist(value,values) ;
	 morph->replacd(values) ;
	 }
      else
	 free_object(value) ;
      }
   else
      pushlist(new FrList(key,value), all_morph) ;
   return ;
}

//----------------------------------------------------------------------

void FrClassifyMorphologyTag(FrSymbol *morph, FrSymbol *value,
			     const FrList *morphology_classes,
			     FrList *&by_class, bool keep_global_info)
{
   const FrList *mapping
      = (FrList*)morphology_classes->assoc(morph,casefold_equal) ;
   if (!mapping || !mapping->consp())
      {
      if (value && value != morph && morph != symbolMORPH)
	 add_to_morph(by_class,symbolMORPH,new FrList(morph,value)) ;
      else
	 add_to_morph(by_class,symbolMORPH,value?value:morph) ;
      }
   else
      {
      do {
         // first time, skip the assoc key; subsequently, skip either the
         // sublist or the second value
         mapping = mapping->rest() ;
	 if (!mapping)
	    break ;
	 if (!mapping->first())
	    continue ;
	 const FrList *map = mapping ;
	 if (mapping->first()->consp())
	    map = (FrList*)mapping->first() ;
	 else
	    mapping = mapping->rest() ;	// we need to skip two values total
	 // pick out the class for this morphology tag
	 FrSymbol *m = FrCvt2Symbol(map->first()) ;
	 FrSymbol *mclass = symbolMORPH ;	// default catch-all class
	 if (m)
	    mclass = m ;
	 // if the optional element is present, remap the tag to that value
	 FrSymbol *valuesym = value ? value : morph ;
	 if (map->second())
	    {
	    m = FrCvt2Symbol(map->second()) ;
	    if (m)
	       valuesym = m ;
	    }
	 add_to_morph(by_class,mclass,valuesym) ;
	 if (keep_global_info && mclass != symbolMORPH)
	    add_to_morph(by_class,symbolMORPH,valuesym) ;
         } while (mapping && mapping->consp()) ;
      }
   return ;
}

//----------------------------------------------------------------------

void FrClassifyMorphologyTag(const char *morph,
			     const FrList *morphology_classes,
			     FrList *&by_class, bool keep_global_info)
{
   if (morph && *morph)
      {
      FrSymbol *morphsym ;
      FrSymbol *valuesym ;
      const char *assign = strstr(morph,assignment_str) ;
      if (assign)
	 {
	 size_t morphlen = (assign - morph) ;
	 assign += strlen(assignment_str) ; // skip the assignment marker
	 FrLocalAlloc(char,morphstr,1024,morphlen+1) ;
	 if (!morphstr)
	    {
	    by_class = 0 ;
	    FrNoMemory("while classifying morphology tag") ;
	    return ;
	    }
	 memcpy(morphstr,morph,morphlen) ;
	 morphstr[morphlen] = '\0' ;
	 morphsym = FrSymbolTable::add(morphstr) ;
	 FrLocalFree(morphstr) ;
	 valuesym = FrSymbolTable::add(assign) ;
	 }
      else
	 {
	 morphsym = FrSymbolTable::add(morph) ;
	 valuesym = morphsym ;
	 }
      FrClassifyMorphologyTag(morphsym,valuesym,morphology_classes,by_class,
			      keep_global_info) ;
      }
   return ;
}

//----------------------------------------------------------------------

void FrClassifyMorphologyTag(FrSymbol *morph,
			     const FrList *morphology_classes,
			     FrList *&by_class, bool keep_global_info)
{
   if (morph)
      FrClassifyMorphologyTag(morph->symbolName(),morphology_classes,
			      by_class,keep_global_info) ;
   return ;
}

//----------------------------------------------------------------------

static FrList *split_off_morph(const char *text,
			       const FrList *morphology_classes,
			       bool keep_global_info)
{
   FrList *tokens = 0 ;
   char *morph = intro_str ? (char*)strstr(text,intro_str) : 0 ;
   if (morph && morph != text)
      {
      size_t morphlen = strlen(intro_str) ;
      if (morph[morphlen] == '\0')
	 return tokens ;		// no tags following the intro
      *morph = '\0' ;			// split off the morphology info
      morph += morphlen ;	        // skip the introducing sequence
      size_t sep_len = strlen(separator_str) ;
      while (morph && *morph)
	 {
	 // find the next separator
	 char *end = strstr(morph,separator_str) ;
	 if (!end)
	    end = strchr(morph,'\0') ;
	 char *next = (*end) ? end + sep_len : 0 ;
	 *end = '\0' ;			// split into two strings
	 if (uppercase_morphology)
	    Fr_strupr(morph) ;
	 // figure out where to place the tag
	 FrClassifyMorphologyTag(morph,morphology_classes,tokens,
				 keep_global_info) ;
	 morph = next ;
	 }
      pushlist(new FrString(text),tokens) ;
      }
   return tokens ;
}

//----------------------------------------------------------------------

bool FrParseMorphologyData(FrTextSpan *span, FrTextSpans *lattice,
			     const FrList *morphology_classes,
			     bool replace_original_span,
			     bool keep_global_info)
{
   if (!span || !intro_str)
      return false ;			// nothing to parse!
   if (!replace_original_span && !lattice)
      return false ;			// nowhere to put the new span....
   char *text = span->getText() ;
   FrList *morph_tokens = split_off_morph(text,morphology_classes,
					  keep_global_info) ;
   FrFree(text) ;
   if (!morph_tokens)
      return false ;			// no morphology info to parse
   FrTextSpan *target = replace_original_span ? span : new FrTextSpan(span) ;
   if (!target)
      {
      FrNoMemory("while parsing embedded morphology information") ;
      return false ;
      }
   FrString *newtext = (FrString*)poplist(morph_tokens) ;
   target->updateText(newtext->stringValue()) ;
   free_object(newtext) ;
   if (use_morphology)
      {
      while (morph_tokens)
	 {
	 FrList *morph = (FrList*)poplist(morph_tokens) ;
	 FrSymbol *morphclass = (FrSymbol*)poplist(morph) ;
	 target->addMetaData(morphclass,morph) ; //!!!maybe use setMetaData?
	 free_object(morph) ;
	 }
      }
   else
      free_object(morph_tokens) ;	// ignore the tags, we just want stems
   if (target != span)
      {
      lattice->addSpan(target) ;
      delete target ;
      }
   return true ;
}

//----------------------------------------------------------------------

FrList *FrParseMorphologyData(const char *word,
			      const FrList *morphology_classes,
			      bool keep_global_info)
{
   FrList *morph = split_off_morph(word,morphology_classes,keep_global_info) ;
   return morph ? morph : new FrList(new FrString(word)) ;
}

//----------------------------------------------------------------------

bool FrParseMorphologyData(FrTextSpans *lattice,
			     const FrList *morphology_classes,
			     bool replace_original_span,
			     bool keep_global_info)
{
   bool have_morph = false ;
   if (lattice)
      {
      size_t count = lattice->spanCount() ;
      for (size_t i = 0 ; i < count ; i++)
	 {
	 if (FrParseMorphologyData(lattice->getSpan(i),lattice,
				   morphology_classes,replace_original_span,
				   keep_global_info))
	    have_morph = true ;
	 }
      if (lattice->spanCount() != count)
	 lattice->sort() ;
      }
   return have_morph ;
}

/************************************************************************/
/************************************************************************/

#define NE_SOURCE '\f'
#define NE_TARGET '\t'
#define NE_CLASS  '\r'
#define NE_PROB   '\n'

class FrNamedEntitySpec
   {
   private:
      char *m_intro ;
      char *m_remainder ;
      double m_defaultconf ;
      size_t m_introlen ;
      bool m_casesensitive ;
   private:
      void init(bool case_sensitive, double default_conf) ;
   public:
      FrNamedEntitySpec(const char *spec, bool case_sensitive = false,
			double default_conf = 0.0)
	 { init(case_sensitive,default_conf) ; parse(spec) ; }
      ~FrNamedEntitySpec() { FrFree(m_intro) ; }

      void defaultConfidence(double conf) { m_defaultconf = conf ; }
      bool parse(const char *spec) ;
      bool match(const char *text, size_t &matchlen,
		   char *&ne_type, char *&ne_source,
		   char *&ne_target, double &confidence) const ;

      // accessors
      bool OK() const { return m_introlen > 0 ; }
      size_t introLength() const { return m_introlen ; }
      const char *intro() const { return m_intro ; }
      const char *remainder() const { return m_remainder ; }
      double defaultConfidence() const { return m_defaultconf ; }
   } ;

//----------------------------------------------------------------------

void FrNamedEntitySpec::init(bool case_sensitive, double conf)
{
   m_intro = 0 ;
   m_introlen = 0 ;
   m_remainder = 0 ;
   m_casesensitive = case_sensitive ;
   m_defaultconf = conf ;
   return ;
}

//----------------------------------------------------------------------

bool FrNamedEntitySpec::parse(const char *spec)
{
   FrFree(m_intro) ;
   m_intro = 0 ;
   m_introlen = 0 ;
   m_remainder = 0 ;
   if (spec)
      {
      size_t speclen = strlen(spec) ;
      m_intro = FrNewN(char,speclen+2) ;
      if (m_intro)
	 {
	 size_t dest = 0 ;
	 for (size_t i = 0 ; i <= speclen ; i++)
	    {
	    if (spec[i] != '%')
	       m_intro[dest++] = spec[i] ;
	    else
	       {
	       i++ ;			// consume the percent sign
	       if (spec[i] == '%')
		  m_intro[dest++] = '%' ;
	       else
		  {
		  // we break the spec string at the first variable, so check
		  //   whether we've already performed the break
		  if (!m_remainder)
		     {
		     m_introlen = dest ;
		     m_intro[dest++] = '\0' ;
		     m_remainder = m_intro + dest ;
		     }
		  switch (spec[i])
		     {
		     case 's':
			m_intro[dest++] = NE_SOURCE ;
			break ;
		     case 't':
			m_intro[dest++] = NE_TARGET ;
			break ;
		     case 'c':
			m_intro[dest++] = NE_CLASS ;
			break ;
		     case 'p':
			m_intro[dest++] = NE_PROB ;
			break ;
		     default:
			m_intro[dest++] = spec[i] ;
			break ;
		     }
		  }
	       }
	    }
	 if (m_remainder)
	    return true ;
	 }
      FrFree(m_intro) ;
      m_intro = 0 ;
      }
   return false ;
}

//----------------------------------------------------------------------

bool FrNamedEntitySpec::match(const char *text, size_t &matchlen,
				char *&ne_type, char *&ne_source,
				char *&ne_target, double &confidence) const
{
   matchlen = 0 ;
   confidence = defaultConfidence() ;
   if (text && m_introlen > 0)
      {
      bool matched_intro ;
      if (m_casesensitive)
	 matched_intro = (strncmp(text,intro(),introLength()) == 0) ;
      else
	 matched_intro = (Fr_strnicmp(text,intro(),introLength()) == 0) ;
      if (matched_intro)
	 {
	 matchlen = introLength() ;
	 text += introLength() ;
	 const char *remain = m_remainder ;
	 size_t remlen = strlen(remain) + 1 ;
	 FrLocalAlloc(char,buf,1024,remlen) ;
	 char *conf = 0 ;
	 for ( ; *remain ; remain++)
	    {
	    char **data ;
	    switch (*remain)
	       {
	       case NE_SOURCE:  data = &ne_source ; 	break ;
	       case NE_TARGET:  data = &ne_target ; 	break ;
	       case NE_CLASS:  	data = &ne_type ; 	break ;
	       case NE_PROB:  	data = &conf ;	 	break ;
	       default:		data = 0 ;		break ;
	       }
	    if (data)
	       {
	       size_t i ;
	       for (i = 1 ; remain[i] && !Fr_isspace(remain[i]) ; i++)
		  buf[i-1] = remain[i] ;
	       buf[i-1] = '\0' ;
	       // skip leading whitespace
	       const char *start_text = text ;
	       FrSkipWhitespace(text) ;
	       const char *sep ;
	       if (m_casesensitive)
		  sep = strstr(text,buf) ;
	       else
		  sep = Fr_stristr(text,buf) ;
	       if (!sep)
		  {
		  matchlen = 0 ;
		  break ;
		  }
	       *data = FrNewN(char,(sep-text)+1) ;
	       if (*data)
		  {
		  memcpy(*data,text,sep-text) ;
		  (*data)[sep-text] = '\0' ;
		  char *lastch = &(*data)[sep-text] ;
		  // remove trailing whitespace
		  while (lastch > *data && Fr_isspace(lastch[-1]))
		     *--lastch = '\0' ;
		  }
	       if (data == &conf)
		  {
		  if (*conf)
		     confidence = strtod(conf,0) ;
		  FrFree(*data) ;
		  }
	       matchlen += (sep - start_text) ;
	       text = sep ;
	       }
	    else if (*text == *remain)
	       {
	       matchlen++ ;
	       text++ ;
	       }
            else
	       {
	       // the match failed!
	       matchlen = 0 ;
	       break ;
	       }
	    }
	 FrLocalFree(buf) ;
	 return (matchlen > 0) ;
	 }
      }
   return false ;
}

//----------------------------------------------------------------------

FrNamedEntitySpec *FrParseNamedEntitySpec(const char *spec, bool case_sens,
					  double def_conf)
{
   return (spec && *spec) ? new FrNamedEntitySpec(spec,case_sens,def_conf) : 0;
}

//----------------------------------------------------------------------

void FrNamedEntitySetConf(FrNamedEntitySpec *spec, double conf)
{
   if (spec)
      spec->defaultConfidence(conf) ;
   return ;
}

//----------------------------------------------------------------------

void FrFreeNamedEntitySpec(FrNamedEntitySpec *spec)
{
   delete spec ;
   return ;
}

//----------------------------------------------------------------------

static bool check_position(const FrTextSpan *span, va_list args)
{
   FrVarArg(size_t,startpos) ;
   FrVarArg(size_t,endpos) ;
   if (span->start() >= startpos && span->start() <= endpos &&
       span->end() >= startpos && span->end() <= endpos)
      return true ;
   return false ;
}

//----------------------------------------------------------------------

char *FrStripNamedEntityData(const char *string,
			     const FrNamedEntitySpec *entity_spec)
{
   if (!string)
      return 0 ;
   if (!entity_spec)
      return FrDupString(string) ;
   char *stripped = FrDupString(string) ;
   if (!stripped)
      {
      FrNoMemory("while removing named entity tags from string") ;
      return 0 ;
      }
   size_t textlength = strlen(string) ;
   size_t intro_len = entity_spec->introLength() ;
   size_t striplen = 0 ;
   for (size_t i = 0 ; i < textlength ; i++)
      {
      size_t matchlen = 0 ;
      char *ne_type = 0 ;
      char *source = 0 ;
      char *target = 0 ;
      double conf = 0.0 ;
      if (i + intro_len < textlength &&
	  entity_spec->match(string+i,matchlen,ne_type,source,target,conf))
	 {
	 if (source)
	    {
	    size_t len = strlen(source) ;
	    memcpy(stripped+striplen,source,len) ;
	    striplen += len ;
	    }
	 if (matchlen > 0)
	    i += matchlen - 1 ;
	 }
      else
	 stripped[striplen++] = string[i] ;
      FrFree(ne_type) ;
      FrFree(source) ;
      FrFree(target) ;
      }
   stripped[striplen] = '\0' ;		// properly terminate the string
   return stripped ;
}

//----------------------------------------------------------------------

bool FrParseNamedEntityData(FrTextSpans *lattice,
			      const FrNamedEntitySpec *entity_spec,
			      bool replace_original_span)
{
   bool have_NEs = false ;
   if (lattice && entity_spec && entity_spec->OK())
      {
      const char *text = lattice->originalString() ;
      size_t textlength = lattice->textLength() ;
      const size_t *map = lattice->positionMap() ;
      size_t intro_len = entity_spec->introLength() ;
      FrSymbol *symXLAT = FrSymbolTable::add("XLAT") ;
      FrSymbol *symCAT = FrSymbolTable::add("CAT") ;
      size_t prev_offset = ~0 ;
      for (size_t i = 0 ; i < textlength ; i++)
	 {
	 size_t offset = map[i] ;
	 if (offset == prev_offset)
	    continue ;
	 if (offset + intro_len >= textlength)
	    break ;
	 size_t matchlen = 0 ;
	 char *ne_type = 0 ;
	 char *source = 0 ;
	 char *target = 0 ;
	 double confidence ;
	 if (entity_spec->match(text+offset,matchlen,ne_type,source,target,
				confidence))
	    {
	    FrList *newspan = 0 ;
	    if (target)
	       pushlist(new FrList(symXLAT,new FrString(target)),newspan) ;
	    if (ne_type)
	       pushlist(new FrList(symCAT,new FrString(ne_type)),newspan) ;
	    if (source)
	       pushlist(new FrString(source),newspan) ;
	    pushlist(new FrFloat(confidence),newspan) ;
	    size_t endpos = offset+1 ;
	    while (endpos < textlength && map[endpos] < offset+matchlen)
	       endpos++ ;
	    pushlist(new FrInteger(endpos-1),newspan) ;
	    pushlist(new FrInteger(i),newspan) ;
	    if (replace_original_span)
	       lattice->removeMatchingSpans(check_position,i,endpos-1) ;
	    lattice->newSpan(newspan) ;
	    free_object(newspan) ;
	    have_NEs = true ;
	    }
	 FrFree(ne_type) ;
	 FrFree(source) ;
	 FrFree(target) ;
	 }
      }
   return have_NEs ;
}

//----------------------------------------------------------------------

bool FrParseNamedEntityData(FrTextSpans *lattice,
			      const char *entity_spec,
			      bool replace_original_span)
{
   FrNamedEntitySpec spec(entity_spec) ;
   return FrParseNamedEntityData(lattice,&spec,replace_original_span) ;
}

// end of file frmorphp.cpp //
