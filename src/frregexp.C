/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File: frregexp.cpp	 	class FrRegExp for regular expressions	*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1997,1998,1999,2000,2001,2007,2009,2011,2012		*/
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

#if defined(__GNUC__)
#  pragma implementation "frregexp.h"
#endif

#include "framerr.h"
#include "frassert.h"
#include "frcmove.h"
#include "frctype.h"
#include "frregexp.h"
#include "frstring.h"
#include "frsymtab.h"
#include "frutil.h"

#ifndef NDEBUG
// save space in executable
# undef _FrCURRENT_FILE
static const char _FrCURRENT_FILE[] = __FILE__ ;
#endif /* NDEBUG */

/************************************************************************/
/************************************************************************/

class FrRegExClass
   {
   private:
      char **members ;
      size_t *member_lengths ;
      size_t num_members ;
      bool case_sensitive ;
   public:
      FrRegExClass() ;
      ~FrRegExClass() ;

      bool caseSensitive() const { return case_sensitive ; }
      void caseSensitive(bool cs) { case_sensitive = cs ; }
      bool addMember(const char *newmember, const char *repl) ;
      char *matches(const char *string, size_t &matchnum) const ;
      char *redoMatch(const char *string, size_t matchnum,
		      char *&matchbuf, const char *matchbuf_end) const ;
   } ;

//----------------------------------------------------------------------

class FrRegExElt
   {
   public:
      enum retype { End, Char, CharSet, String, Alt, Class, Accept } ;
   private:
      retype type ;
      size_t min_count ;
      size_t max_count ;
      size_t string_length ;
      int group_number ;
      union
	 {
	 char character ;
	 char *charset ;
	 char *string ;
	 FrRegExElt **alternatives ;
	 FrRegExClass *class_members ;
	 } ;
      FrRegExElt *next ;
   public:
      FrRegExElt() ;
      FrRegExElt(char ch) ;
      FrRegExElt(const char *chset, size_t setsize) ;
      FrRegExElt(const char *string, const char *translation) ;
      FrRegExElt(const char *string, size_t stringlen,
		 const char *translation, size_t translen) ;
      FrRegExElt(FrRegExElt **alts, size_t group_num) ;
      FrRegExElt(FrRegExClass *cls_members) ;
      ~FrRegExElt() ;

      // modifiers
      void setReps(size_t min, size_t max)
	    { min_count = min ; max_count = max ; }
      void setGroup(int group) { group_number = group ; }
      void setNext(FrRegExElt *nxt) { next = nxt ; }

      // accessors
      retype reType() const { return type ; }
      size_t minReps() const { return min_count ; }
      size_t maxReps() const { return max_count ; }
      int groupNumber() const { return group_number ; }
      FrRegExElt *getNext() const { return next ; }
      char getChar() const
	    { if (type == Char) return character ; else return 0 ; }
      const char *getCharSet() const
	    { if (type == CharSet) return charset ; else return 0 ; }
      const char *getString() const
	    { if (type == String) return string ; else return 0 ; }
      FrRegExClass *getClass() const
            { if (type == Class) return class_members ; else return 0 ; }
      size_t stringLength() const { return string_length ; }
      FrRegExElt ** getAlternatives() const
	    { if (type == Alt) return alternatives ; else return 0 ; }
   } ;

/************************************************************************/
/*	forward declarations	 					*/
/************************************************************************/

static FrRegExElt *compile_re(const char *&re, bool in_alt,
			      size_t &num_groups,FrList *&classes);

static const char *re_match(const FrRegExElt *re, const char *candidate,
			    size_t min_reps, size_t &max_reps,
			    char *&match, const char *matchbuf_end,
			    char **groups, size_t num_groups) ;

static const char *re_match(const FrRegExElt *re, const char *candidate,
			    char *&match, const char *matchbuf_end,
			    char **groups, size_t num_groups) ;

/************************************************************************/
/*	helper functions	 					*/
/************************************************************************/

static int Fr_memicmp(const char *s1, const char *s2, size_t len)
{
   int diff = 0 ;
   while (len > 0 && (diff = (Fr_toupper(*s1) - Fr_toupper(*s2))) == 0)
      {
      len-- ;
      s1++ ;
      s2++ ;
      }
   return diff ;
}

/************************************************************************/
/*	methods for class FrRegExClass 					*/
/************************************************************************/

FrRegExClass::FrRegExClass()
{
   num_members = 0 ;
   members = 0 ;
   member_lengths = 0 ;
   case_sensitive = false ;
   return ;
}

//----------------------------------------------------------------------

FrRegExClass::~FrRegExClass()
{
   for (size_t i = 0 ; i < num_members ; i++)
      {
      FrFree(members[i]) ;
      members[i] = 0 ;
      }
   FrFree(members) ;		members = 0 ;
   FrFree(member_lengths) ;	member_lengths = 0 ;
   num_members = 0 ;
   return ;
}

//----------------------------------------------------------------------

bool FrRegExClass::addMember(const char *newmember, const char *repl)
{
   if (newmember)
      {
      size_t newlen = strlen(newmember) ;
      size_t totallen = newlen + 1 ;
      if (repl)
	 totallen += strlen(repl) ;
      else
	 totallen += strlen(newmember) ;
      // check whether it's an exact duplicate, a prefix of an existing
      //   member, or has an existing member as a prefix
      size_t i ;
      for (i = 0 ; i < num_members ; i++)
	 {
	 if ((case_sensitive && strcmp(newmember,members[i]) == 0) ||
	     (!case_sensitive && Fr_stricmp(newmember,members[i]) == 0))
	    return true ;		// already member of the class
	 }
      size_t pos = num_members ;
      // scan backward for an entry which contains the new element as a prefix
      for (i = num_members ; i > 0 ; i++)
	 {
	 if ((case_sensitive && memcmp(newmember,members[i-1],newlen) == 0) ||
	     (!case_sensitive &&
	      Fr_strnicmp(newmember,members[i-1],newlen) == 0))
	    {
	    pos = i ;
	    break ;
	    }
	 }
      // now scan forward for an entry which is a prefix of the new element
      for ( ; i < num_members ; i++)
	 {
	 if ((case_sensitive &&
	      memcmp(newmember,members[i],member_lengths[i]) == 0) ||
	     (!case_sensitive &&
	      Fr_strnicmp(newmember,members[i],member_lengths[i]) == 0))
	    {
	    pos = i ;
	    break ;
	    }
	 }
      // we've found the proper position for inserting the new member; we
      //   want it to precede any existing members which are a
      //   prefix of the new one, so that the longest match is
      //   selected first
      char **new_members = FrNewR(char*,members,num_members+1) ;
      size_t *new_lengths = FrNewR(size_t,member_lengths,num_members+1) ;
      if (!new_members || !new_lengths)
	 {
	 FrNoMemory("expanding regex equivalence class") ;
	 return false ;			// unable to add new member
	 }
      for (i = num_members ; i > pos ; i--)
	 {
	 new_members[i] = members[i-1] ;
	 new_lengths[i] = member_lengths[i-1] ;
	 }
      char *newstr = FrNewN(char,totallen) ;
      if (newstr)
	 {
	 new_members[pos] = newstr ;
	 new_lengths[pos] = newlen ;
	 memcpy(newstr,newmember,newlen+1) ;
	 if (repl)
	    strcpy(newstr+newlen+1,repl) ;
	 else
	    strcpy(newstr+newlen+1,newmember) ;
	 }
      else
	 {
	 new_members[pos] = 0 ;
	 new_lengths[pos] = 0 ;
	 FrNoMemory("adding member to regex class") ;
	 }
      num_members++ ;
      members = new_members ;
      member_lengths = new_lengths ;
      return true ;
      }
   return false ;			// didn't add anything
}

//----------------------------------------------------------------------

char *FrRegExClass::matches(const char *string, size_t &matchnum) const
{
   for (size_t i = matchnum ; i < num_members ; i++)
      {
      if (case_sensitive)
	 {
	 if (memcmp(members[i],string,member_lengths[i]) == 0)
	    {
	    matchnum = i+1 ;
	    return (char*)(string + member_lengths[i]) ;
	    }
	 }
      else
	 {
	 if (Fr_strnicmp(members[i],string,member_lengths[i]) == 0)
	    {
	    matchnum = i+1 ;
	    return (char*)(string + member_lengths[i]) ;
	    }
	 }
      }
   matchnum = (size_t)~0 ;
   return 0 ;
}

//----------------------------------------------------------------------

char *FrRegExClass::redoMatch(const char *string, size_t matchnum,
			      char *&matchbuf, const char *matchbuf_end) const
{
(void)matchbuf ; (void)matchbuf_end ;
   if (matchnum > num_members)
      return 0 ;
   if (matchnum > 0) matchnum-- ;
   if (case_sensitive)
      {
      if (memcmp(members[matchnum],string,member_lengths[matchnum]) == 0)
	 {
	 //!!!
	 return (char*)(string + member_lengths[matchnum]) ;
	 }
      }
   else
      {
      if (Fr_strnicmp(members[matchnum],string,member_lengths[matchnum]) == 0)
	 {
	 //!!!
	 return (char*)(string + member_lengths[matchnum]) ;
	 }
      }
   return 0 ;
}

/************************************************************************/
/*	methods for class FrRegExElt 					*/
/************************************************************************/

FrRegExElt::FrRegExElt()
{
   type = End ;
   min_count = 1 ;
   max_count = 1 ;
   group_number = -1 ;
   string = 0 ;
   string_length = 0 ;
   next = 0 ;
   return ;
}

//----------------------------------------------------------------------

FrRegExElt::FrRegExElt(char ch)
{
   type = Char ;
   min_count = 1 ;
   max_count = 1 ;
   group_number = -1 ;
   character = ch ;
   string_length = 0 ;
   next = 0 ;
   return ;
}

//----------------------------------------------------------------------

FrRegExElt::FrRegExElt(const char *chset, size_t setsize)
{
   type = CharSet ;
   min_count = 1 ;
   max_count = 1 ;
   string_length = 0 ;
   group_number = -1 ;
   size_t size = FrMax(setsize,256) ;
   char *set = FrNewN(char,size) ;
   if (set)
      {
      memcpy(set,chset,setsize) ;
      if (size > setsize)
	 memset(set+setsize,'\0',size-setsize) ;
      }
   charset = set ;
   next = 0 ;
   return ;
}

//----------------------------------------------------------------------

FrRegExElt::FrRegExElt(const char *str, const char *translation)
{
   type = String ;
   min_count = 1 ;
   max_count = 1 ;
   string_length = 0 ;
   group_number = -1 ;
   if (!translation || !*translation)
      translation = str ;
   size_t len1 = strlen(str)+1 ;
   size_t len2 = strlen(translation)+1 ;
   size_t len = len1 + len2 ;
   string = FrNewN(char,len) ;
   if (string)
      {
      memcpy(string,str,len1) ;
      memcpy(string+len1,translation,len2) ;
      string_length = len1 - 1 ;
      }
   next = 0 ;
   return ;
}

//----------------------------------------------------------------------

FrRegExElt::FrRegExElt(const char *str, size_t stringlen,
		       const char *translation, size_t translen)
{
   type = String ;
   min_count = 1 ;
   max_count = 1 ;
   string_length = 0 ;
   group_number = -1 ;
   if (!translation || translen == 0)
      {
      translation = str ;
      translen = stringlen ;
      }
   size_t len = stringlen + translen + 2 ;
   string = FrNewN(char,len) ;
   if (string)
      {
      memcpy(string,str,stringlen) ;
      string[stringlen] = '\0' ;
      memcpy(string+stringlen+1,translation,translen) ;
      string[len-1] = '\0' ;
      string_length = stringlen ;
      }
   next = 0 ;
   return ;
}

//----------------------------------------------------------------------

FrRegExElt::FrRegExElt(FrRegExElt **alts, size_t group_num)
{
   type = Alt ;
   min_count = 1 ;
   max_count = 1 ;
   string_length = 0 ;
   group_number = group_num ;
   alternatives = alts ;
   next = 0 ;
   return ;
}

//----------------------------------------------------------------------

FrRegExElt::FrRegExElt(FrRegExClass *cls_members)
{
   type = Class ;
   min_count = 1 ;
   max_count = 1 ;
   string_length = 0 ;
   group_number = 0 ;
   class_members = cls_members ;
   next = 0 ;
   return ;
}

//----------------------------------------------------------------------

FrRegExElt::~FrRegExElt()
{
   switch (type)
      {
      case CharSet:
	 FrFree(charset) ;
	 break ;
      case String:
	 FrFree(string) ;
	 break ;
      case Alt:
	 if (alternatives)
	    {
	    FrRegExElt **alt = alternatives ;
	    while (*alt)
	       {
	       delete *alt ;
	       alt++ ;
	       }
	    FrFree(alternatives) ;
	    alternatives = 0 ;
	    }
	 break ;
      case Class:
	 delete class_members ;
	 class_members = 0 ;
	 break ;
      default:
	 // no extra action needed
	 break ;
      }
   if (next)
      delete next ;
   return ;
}

/************************************************************************/
/*	methods for class FrRegExp 					*/
/************************************************************************/

FrRegExp::FrRegExp(const char *re, const char *repl, FrSymbol *tok,
		   FrRegExp *nxt)
{
   if (!re)
      re = "" ;
   _classes = 0 ;
   regex = compile(re) ;
   if (!repl)
      repl = "" ;
   size_t len = strlen(repl)+1 ;
   replacement = FrNewN(char,len) ;
   if (replacement)
      memcpy(replacement,repl,len) ;
   _next = nxt ;
   _token = tok ;
   return ;
}

//----------------------------------------------------------------------

FrRegExp::~FrRegExp()
{
   delete regex ; regex = 0 ;
   FrFree(replacement) ; replacement = 0 ;
   while (_classes)
      {
      FrCons *cl = (FrCons*)poplist(_classes) ;
      if (cl)
	 {
	 FrRegExClass *re_class = (FrRegExClass*)cl->consCdr() ;
	 cl->freeObject() ;
	 delete re_class ;
	 }
      }
   _token = 0 ;
   return ;
}

//----------------------------------------------------------------------

static void compile_count(const char *&re, size_t &low, size_t &high)
{
   assertq(*re == FrRE_COUNT_BEG) ;
   low = 0 ;
   high = UINT_MAX ;
   re++ ;
   while (Fr_isdigit(*re))
      low = 10*low + (*re - '0') ;
   if (*re == FrRE_COUNT_END)
      high = low ;
   else if (*re == ',')
      {
      high = 0 ;
      while (Fr_isdigit(*re))
	 high = 10*high + (*re - '0') ;
      if (high < low)
	 high = low ;
      }
   else
      FrWarning("invalid {} specifier in regular expression") ;
   if (*re == FrRE_COUNT_END)
      re++ ;
   else
      FrWarning("unterminated {} specifier in regular expression") ;
   return ;
}

//----------------------------------------------------------------------

static FrRegExElt *compile_alternation(const char *&re, size_t &num_groups,
				       FrList *&classes)
{
   assertq(*re == FrRE_ALT_BEG) ;
   const char *regex = re+1 ;
   FrRegExElt **elts = FrNewN(FrRegExElt*,2) ;
   if (!elts)
      return 0 ;
   elts[0] = 0 ;
   size_t num_elts = 0 ;
   size_t groupnum = ++num_groups ;
   while (*regex)
      {
      FrRegExElt *elt = compile_re(regex,true,num_groups,classes) ;
      FrRegExElt **newelts = FrNewR(FrRegExElt*,elts,num_elts+2) ;
      if (!newelts)
	 {
	 FrNoMemory("while compiling regex grouping/alternation") ;
	 break ;
	 }
      elts = newelts ;
      elts[num_elts++] = elt ;
      elts[num_elts] = 0 ;
      if (elt)
	 elt->setGroup(groupnum) ;
      if (*regex == FrRE_ALT_SEP)
	 regex++ ;
      else if (*regex == FrRE_ALT_END)
	 break ;
      }
   if (*regex == FrRE_ALT_END)
      regex++ ;
   else
      FrWarningVA("unterminated regular expression grouping: %s",re) ;
   re = regex ;
   return new FrRegExElt(elts,groupnum) ;
}

//----------------------------------------------------------------------

static FrRegExElt *compile_class(const char *&re, FrList *&classes)
{
   assertq(*re == FrRE_CLASS_BEG) ;
   re++ ;
   const char *name_end = strchr(re,FrRE_CLASS_END) ;
   if (name_end)
      {
      char name[FrMAX_SYMBOLNAME_LEN+1] ;
      size_t namelen = name_end - re ;
      if (namelen >= sizeof(name))
	 namelen = sizeof(name) - 1 ;
      memcpy(name,re,namelen) ;
      name[namelen] = '\0' ;
      re = name_end + 1 ;
      FrRegExClass *class_members = new FrRegExClass ;
      if (!class_members)
	 return 0 ;			// out of memory!
      FrSymbol *namesym = FrSymbolTable::add(name) ;
      FrList *cl = (FrList*)classes->assoc(namesym) ;
      if (cl)
	 {
	 // add to list of instances of this class
	 FrList *members = cl->rest() ;
	 pushlist((FrObject*)class_members,members) ;
	 cl->replacd(members) ;
	 }
      else
	 {
	 // create a new list of class instances
	 pushlist(new FrList(namesym,(FrObject*)class_members),classes) ;
	 }
      return new FrRegExElt(class_members) ;
      }
   return 0 ;
}

//----------------------------------------------------------------------

static FrRegExElt *compile_charset(const char *&re)
{
   assertq(*re == FrRE_CHARSET_BEG) ;
   const char *orig_re = re ;
   const unsigned char *regex = (unsigned char*)re+1 ;
   bool negation = false ;
   char set[256] ;
   memset(set,'\0',sizeof(set)) ;
   if (*regex == FrRE_CHARSET_NEG)
      {
      negation = true ;
      regex++ ;
      }
   if (*regex == FrRE_CHARSET_END)
      { // can't have empty set
      set[*regex++] = (char)true ;
      }
   if (*regex == '-')
      {	// can't have half range at start of set, so must be literal hyphen
      set[*regex++] = (char)true ;
      }
   while (*regex && *regex != FrRE_CHARSET_END)
      {
      if (*regex == '-' && regex[1])
	 {
	 for (unsigned char c = regex[-1] ; c <= regex[1] ; c++)
	    set[c] = (char)true ;
	 regex++ ;
	 }
      else
	 set[*regex] = (char)true ;
      regex++ ;
      }
   if (negation)
      {
      for (size_t i = 0 ; i < sizeof(set) ; i++)
	 set[i] = !set[i] ;
      }
   FrRegExElt *elt = new FrRegExElt(set,sizeof(set)) ;
   if (*regex == FrRE_CHARSET_END)
      regex++ ;
   else
      FrWarningVA("unterminated regex character set %s",orig_re) ;
   re = (char*)regex ;
   return elt ;
}

//----------------------------------------------------------------------

static FrRegExElt *compile_wildcard(const char *&re)
{
   assertq(*re == '.') ;
   re++ ;				// consume the period
   char set[256] ;
   memset(set,(char)1,sizeof(set)) ;
   FrRegExElt *elt = new FrRegExElt(set,sizeof(set)) ;
   return elt ;
}

//----------------------------------------------------------------------

static FrRegExElt *compile_simple(const char *&re, bool in_alt)
{
   assertq(*re != '\0' && *re != '.') ;
   const char *orig_re = re ;
   const char *regex = re ;
   size_t stringlen = 0 ;
   const char *xlat = 0 ;
   size_t translen = 0 ;
   while (*regex)
      {
      char c = *regex ;
      // check whether we're escaping a special character
      if (c == FrRE_QUOTE)
	 {
	 if (regex[1])
	    {
	    regex++ ;
	    stringlen++ ;
	    }
	 }
      // the wildcard chacter ends the simple RE
      else if (c == '.')
	 break ;
      // as do repetition specifiers
      else if (c == FrRE_MULTIPLE || c == FrRE_KLEENE || c == FrRE_OPTIONAL ||
	       c == FrRE_COUNT_BEG)
	 {
	 if (stringlen > 1)
	    {
	    stringlen-- ;
	    regex-- ;
	    // check for an escaped special character
	    if (regex > re && regex[-1] == FrRE_QUOTE)
	       regex-- ;
	    break ;
	    }
	 else if (stringlen > 0)
	    break ;
	 stringlen++ ;
	 }
      else if (c == FrRE_CHARSET_BEG || c == FrRE_ALT_BEG)
	 break ;
      else if (in_alt)
	 {
	 if (c == '|')
	    {
	    // we have a replacement string for the current alternative
	    const char *tmp = regex+1 ;
	    translen = 0 ;
	    while (*tmp)
	       {
	       // check for an escaped special character
	       if (*tmp == FrRE_QUOTE)
		  {
		  if (tmp[1])
		     {
		     tmp++ ;
		     translen++ ;
		     }
		  }
	       else if (*tmp == FrRE_ALT_SEP || *tmp == FrRE_ALT_END)
		  break ;
	       else
		  translen++ ;
	       tmp++ ;
	       }
	    xlat = regex+1 ;
	    regex = tmp ;
	    break ;
	    }
	 // the simple RE ends with the end of the current alternative
	 else if (c == FrRE_ALT_SEP || c == FrRE_ALT_END)
	    break ;
	 else
	    stringlen++ ;
	 }
      else
	 stringlen++ ;
      if (*regex)
	 regex++ ;
      }
   assertq(stringlen > 0) ;
   if (!xlat)
      {
      xlat = orig_re ;
      translen = stringlen ;
      }
   char *string = FrNewN(char,stringlen+1) ;
   char *trans = FrNewN(char,translen+1) ;
   if (string)
      {
      const char *s = re ;
      for (size_t i = 0 ; i < stringlen ; i++)
	 {
	 // check for an escaped special character
	 if (*s == FrRE_QUOTE)
	    s++ ;
	 string[i] = *s++ ;
	 }
      }
   else
      string = (char*)re ;
   if (trans)
      {
      const char *t = xlat ;
      for (size_t i = 0 ; i < translen ; i++)
	 {
	 // check for an escaped special character
	 if (*t == FrRE_QUOTE)
	    t++ ;
	 trans[i] = *t++ ;
	 }
      }
   else
      trans = (char*)xlat ;
   FrRegExElt *elt ;
   if (stringlen == 1 && translen == 1 && *string == *trans)
      elt = new FrRegExElt(*string) ;
   else
      elt = new FrRegExElt(string,stringlen,trans,translen) ;
   if (string != re)
      FrFree(string) ;
   if (trans != xlat)
      FrFree(trans) ;
   re = regex ;
   return elt ;
}

//----------------------------------------------------------------------

static FrRegExElt *compile_re(const char *&re, bool in_alt,
			      size_t &num_groups, FrList *&classes)
{
   FrRegExElt *elt = 0 ;
   switch (*re)
      {
      case '\0':
	 elt = new FrRegExElt() ;
	 break ;
      case FrRE_ALT_BEG:
	 elt = compile_alternation(re,num_groups,classes) ;
	 break ;
      case FrRE_CLASS_BEG:
	 elt = compile_class(re,classes) ;
	 break ;
      case FrRE_CHARSET_BEG:
	 elt = compile_charset(re) ;
	 break ;
      case '.':
	 elt = compile_wildcard(re) ;
	 break ;
      case FrRE_ALT_SEP:
      case FrRE_ALT_END:
	 if (in_alt)
	    {
	    elt = new FrRegExElt((char*)0,0,(char*)0,0) ;
	    break ;
	    }
	 // fall through to default case
      default:
	 elt = compile_simple(re,in_alt) ;
	 break ;
      }
   if (re && *re)
      {
      FrRegExElt *next_elt ;
      char option = *re ;
      if (option == FrRE_MULTIPLE || option == FrRE_KLEENE ||
	  option == FrRE_OPTIONAL)
	 re++ ;
      else if (option == FrRE_COUNT_BEG)
	 {
	 size_t low, high ;
	 compile_count(re,low,high) ;
	 elt->setReps(low,high) ;
	 }
      if (in_alt && (option == FrRE_ALT_SEP || option == FrRE_ALT_END))
	 next_elt = 0 ;
      else
	 next_elt = compile_re(re,in_alt,num_groups,classes) ;
      if (elt)
	 elt->setNext(next_elt) ;
      else
	 elt = next_elt ;
      if (elt)
	 {
	 if (option == FrRE_MULTIPLE)
	    elt->setReps(1,UINT_MAX) ;
	 else if (option == FrRE_KLEENE)
	    elt->setReps(0,UINT_MAX) ;
	 else if (option == FrRE_OPTIONAL)
	    elt->setReps(0,1) ;
	 }
      }
   return elt ;
}

//----------------------------------------------------------------------

FrRegExElt *FrRegExp::compile(const char *re)
{
   if (!re || !*re)
      return 0 ;
   size_t num_groups = 0 ;
   return compile_re(re,false,num_groups,_classes) ;
}

//----------------------------------------------------------------------

bool FrRegExp::addToClass(FrSymbol *classname, const char *element,
			    const char *repl)
{
   FrCons *cl = _classes->assoc(classname) ;
   if (cl)
      {
      FrRegExClass *re_class = (FrRegExClass*)cl->consCdr() ;
      if (re_class)
	 return re_class->addMember(element,repl) ;
      }
   return false ;
}

//----------------------------------------------------------------------

static const char *re_maxmatch(const FrRegExElt *re, const char *candidate,
			       char *&matchbuf, const char *matchbuf_end,
			       char **groups, size_t num_groups)
{
   char *match = matchbuf ;
   const char *match_end = 0 ;
   const char *end = candidate ;
   size_t max_reps = re->maxReps() ;
   size_t min_reps = re->minReps() ;
   size_t best_reps = 0 ;
   while (max_reps >= min_reps)
      {
      match = matchbuf ;
      const char *split = re_match(re,candidate,min_reps,max_reps,
				   match,matchbuf_end,groups,num_groups) ;
      if (split)
	 {
	 const FrRegExElt *next = re->getNext() ;
	 if (next)
	    end = re_match(next,split,match,matchbuf_end,groups,num_groups) ;
	 else if (*split == '\0')
	    {
	    best_reps = max_reps ;
	    match_end = split ;
	    break ;			// found a complete match
	    }
	 else
	    end = split ;
	 if (end && end > match_end)
	    {
	    match_end = end ;
	    best_reps = max_reps ;
	    if (*end == '\0')
	       break ;
	    }
	 }
      else
	 return 0 ;			// can't match!
      if (max_reps > 0)
	 max_reps-- ;			// try one less next time
      else
	 break ;
      }
   if (best_reps > 0 && best_reps != max_reps)
      {
      // re-compute the best match if necessary
      match = matchbuf ;
      const char *split = re_match(re,candidate,best_reps,best_reps,
				   match,matchbuf_end,groups,num_groups) ;
      const FrRegExElt *next = re->getNext() ;
      if (split && next)
	 (void)re_match(next,split,match,matchbuf_end,groups,num_groups) ;
      }
   assertq(match >= matchbuf && match <= matchbuf_end) ;
   matchbuf = match ;
   return match_end ;
}

//----------------------------------------------------------------------

static const char *re_match_class(const FrRegExElt *re,
				  const char *candidate,
				  size_t min_reps, size_t &max_reps,
				  char *&matchbuf, const char *matchbuf_end,
				  char **groups, size_t num_groups)
{
   assertq(re != 0 && matchbuf != 0 && matchbuf_end != 0) ;
   const FrRegExClass *re_class = re->getClass() ;
   if (re_class)
      {
      size_t matchnum = 0 ;
      const char *max_end = candidate ;
      size_t bestmatch = ~0 ;
      char *submatch ;
      do {
         submatch = matchbuf ;
	 const char *end = re_class->matches(candidate,matchnum) ;
	 if (end)
	    {
	    while (candidate < end)
	       *submatch++ = *candidate++ ;
	    if (min_reps > 1)
	       {
	       size_t mreps = max_reps - 1 ;
	       end = re_match_class(re,candidate,min_reps-1,mreps,
				    submatch,matchbuf_end,groups,num_groups) ;
	       if (!end)
		  continue ;
	       if (end > max_end)
		  {
		  max_end = end ;
		  bestmatch = matchnum ;
		  }
	       if (*end)		// complete match?
		  continue ;		// if not, try another class member
	       }
	    FrRegExElt *next = re->getNext() ;
	    if (next)
	       {
	       end = re_maxmatch(next,candidate,submatch,matchbuf_end,groups,
				 num_groups) ;
	       if (!end && max_reps > min_reps)
		  {
		  size_t mreps = max_reps - 1 ;
		  end = re_match_class(re,candidate,1,mreps,
				       submatch,matchbuf_end,groups,
				       num_groups) ;
		  }
	       }
	    }
	 if (end && end > max_end)
	    {
	    max_end = end ;
	    bestmatch = matchnum ;
	    }
         } while (matchnum != (size_t)~0 && *max_end) ;
      // if we have a successful match of the entire candidate, ensure that
      //   the match buffer is correctly filled; otherwise, indicate failure
      if (bestmatch != (size_t)~0 && *max_end == '\0')
	 {
	 const char *end = re_class->redoMatch(candidate,bestmatch,submatch,
					       matchbuf_end) ;
	 char *sub_match = matchbuf ;
	 if (min_reps > 1)
	    {
	    size_t mreps = max_reps - 1 ;
	    end = re_match_class(re,candidate,min_reps-1,mreps,
				 sub_match,matchbuf_end,groups,num_groups) ;
	    matchbuf = sub_match ;
	    }
	 else
	    {
	    FrRegExElt *next = re->getNext() ;
	    if (next)
	       {
	       end = re_maxmatch(next,candidate,sub_match,matchbuf_end,groups,
				 num_groups) ;
	       if (!end && max_reps > min_reps)
		  {
		  size_t mreps = max_reps - 1 ;
		  end = re_match_class(re,candidate,1,mreps,
				       sub_match,matchbuf_end,groups,
				       num_groups) ;
		  }
	       }
	    }
	 matchbuf = sub_match ;
	 return end ;
	 }
      }
   return 0 ;
}

//----------------------------------------------------------------------

static const char *re_match_alt(FrRegExElt ** const alts, const char *candidate,
				size_t min_reps, size_t &max_reps,
			        char *&matchbuf, const char *matchbuf_end,
			        char **groups, size_t num_groups)
{
   assertq(alts != 0 && matchbuf != 0 && matchbuf_end != 0) ;
   const char *max_end = 0 ;
   size_t i ;
   for (i = 0 ; i < max_reps ; i++)
      {
      char *submatch ;
      FrRegExElt **alt ;
      FrRegExElt *best_alt = 0 ;
      max_end = 0 ;
      for (alt = alts ; *alt ; alt++)
	 {
	 submatch = matchbuf ;
	 const char *end = re_maxmatch(*alt,candidate,submatch,matchbuf_end,
				       groups,num_groups) ;
	 if (end && end > max_end)
	    {
	    max_end = end ;
	    best_alt = *alt ;
	    }
	 }
      if (!max_end || max_end == candidate) // could we match anything?
	 break ;
      if (best_alt && matchbuf_end > matchbuf)
	 {
	 // record the longest match we found
	 if (alt != alts && best_alt != alt[-1])
	    {
	    // if the best alternative was not the last one, we need to
	    // re-generate the best one
	    submatch = matchbuf ;
	    (void)re_maxmatch(best_alt,candidate,submatch,matchbuf_end,
			      groups,num_groups) ;
	    }
	 matchbuf = submatch ;
	 }
      candidate = max_end ;
      }
   max_reps = i ;
   if (i >= min_reps || (i == 0 && max_end == candidate))
      return max_end ;
   else
      return 0 ;
}

//----------------------------------------------------------------------

static const char *re_match(const FrRegExElt *re, const char *candidate,
			    size_t min_reps, size_t &max_reps,
			    char *&match, const char *matchbuf_end,
			    char **groups, size_t num_groups)
{
   assertq(re != 0 && match != 0 && matchbuf_end != 0) ;
   switch (re->reType())
      {
      case FrRegExElt::End:		// match end of word
	 if (*candidate)
	    {
	    max_reps = 0 ;		// uh oh, not at end of word....
	    return 0 ;
	    }
	 else
	    {
	    max_reps = 1 ;
	    return candidate ;
	    }
      case FrRegExElt::Char:
	 {
	 char c = Fr_toupper(re->getChar()) ;
	 size_t i ;
	 for (i = 0 ; i < max_reps ; i++)
	    {
	    if (Fr_toupper(*candidate) != c)
	       break ;
	    if (match < matchbuf_end)
	       *match++ = *candidate ;
	    candidate++ ;
	    }
	 max_reps = i ;
	 if (i >= min_reps )
	    return candidate ;
	 else
	    return 0 ;
	 }
      case FrRegExElt::CharSet:
	 {
	 const char *set = re->getCharSet() ;
	 if (!set)
	    {
	    max_reps = 0 ;
	    return 0 ;
	    }
	 size_t i ;
	 for (i = 0 ; i < max_reps && *candidate ; i++)
	    {
	    if (!set[*(unsigned char*)candidate])
	       break ;
	    if (match < matchbuf_end)
	       *match++ = *candidate ;
	    candidate++ ;
	    }
	 max_reps = i ;
	 if (i >= min_reps)
	    return candidate ;
	 else
	    return 0 ;
	 }
      case FrRegExElt::String:
	 {
	 const char *string = re->getString() ;
	 size_t len = re->stringLength() ;
	 if (!string)
	    {
	    max_reps = 0 ;
	    return 0 ;
	    }
	 size_t i ;
	 for (i = 0 ; i < max_reps ; i++)
	    {
	    if (Fr_memicmp(candidate,string,len) != 0)
	       break ;
	    const char *tstr = string+len+1 ;
	    size_t tlen = strlen(tstr) ;
	    if (match+tlen < matchbuf_end)
	       {
	       memcpy(match,tstr,tlen) ;
	       match += tlen ;
	       }
	    candidate += len ;
	    }
	 max_reps = i ;
	 if (i >= min_reps)
	    return candidate ;
	 else
	    return 0 ;
	 }
      case FrRegExElt::Alt:
	 {
	 FrRegExElt **alts = re->getAlternatives() ;
	 char *matchbuf = match ;
	 const char *end = re_match_alt(alts,candidate,min_reps,max_reps,
					match,matchbuf_end,groups,num_groups) ;
	 if (max_reps >= min_reps)
	    {
	    int group_num = re->groupNumber() ;
	    if (end && group_num >= 0 && group_num < (int)num_groups)
	       {
	       // since the r.e. compiler ensures that only alternations can
	       // be used for grouping, we can get away with only recording
	       // groupings right here
	       *match = '\0' ;
	       FrFree(groups[group_num]) ;
	       groups[group_num] = FrDupString(matchbuf) ;
	       }
	    return end ;
	    }
	 else
	    return 0 ;
	 }
      case FrRegExElt::Class:
         {
	 const char *end = re_match_class(re,candidate,min_reps,max_reps,
					  match,matchbuf_end,groups,
					  num_groups) ;
	 return (max_reps >= min_reps) ? end : 0 ;
	 }
      case FrRegExElt::Accept:
	 return strchr(candidate,'\0') ;
      default:
	 max_reps = 0 ;
	 FrMissedCase("re_match") ;
	 return 0 ;
      }
}

//----------------------------------------------------------------------

static const char *re_match(const FrRegExElt *re, const char *candidate,
			    char *&matchbuf, const char *matchbuf_end,
			    char **groups, size_t num_groups)
{
   assertq(groups != 0) ;
   if (!re || !candidate)
      return 0 ;
   static char _matchbuf[2*FrMAX_SYMBOLNAME_LEN] ;
   if (!matchbuf || !matchbuf_end)
      {
      matchbuf = _matchbuf ;
      matchbuf_end = &_matchbuf[sizeof(_matchbuf)]-1 ;
      }
   return re_maxmatch(re,candidate,matchbuf,matchbuf_end,groups,num_groups) ;
}

//----------------------------------------------------------------------

FrObject *FrRegExp::match(const char *word) const
{
   if (!regex || !word || !*word)
      return 0 ;
   char *groups[10] ;
   for (size_t i = 0 ; i < lengthof(groups) ; i++)
      groups[i] = 0 ;
//   char *end = strchr(word,'\0') ;
   FrObject *result ;
   char *matchbuf = 0 ;
//   if (re_match(regex,word,matchbuf,0,groups,lengthof(groups)) == end)
   const char *end ;
   if ((end = re_match(regex,word,matchbuf,0,groups,lengthof(groups))) != 0 &&
       !*end)
      {
      char translation[FrMAX_SYMBOLNAME_LEN+1] ;
      char *trans_end = &translation[FrMAX_SYMBOLNAME_LEN] ;
      char *xlat = translation ;
      const char *repl ;
      for (repl = replacement ; *repl && xlat < trans_end ; repl++)
	 {
	 char c = *repl ;
	 if (c == FrRE_QUOTE)
	    {
	    // escape-char plus digit specifies a replacement taken from the
	    //   source match
	    c = *++repl ;
	    if (Fr_isdigit(c))
	       {
	       const char *targ = groups[c-'0'] ;
	       if (targ)
		  {
		  size_t len = strlen(targ) ;
		  memcpy(xlat,targ,len) ;
		  xlat += len ;
		  }
	       else
		  FrWarningVA("mismatch in r.e. replacement: %%%c",c) ;
	       }
	    else if (c)
	       *xlat++ = *++repl ;
	    else
	       break ;
	    }
	 else
	    *xlat++ = c ;
	 }
      *xlat = '\0' ;
      result = new FrString(translation) ;
      }
   else
      result = 0 ;
   for (size_t j = 0 ; j < lengthof(groups) ; j++)
      if (groups[j]) FrFree(groups[j]) ;
   return result ;
}

//----------------------------------------------------------------------

char *FrRegExp::replace(char *string) const
{

   return string ; //!!!
}

/************************************************************************/
/************************************************************************/


// end of file frregexp.cpp //
