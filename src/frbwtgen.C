/************************************************************************/
/*									*/
/*  FramepaC								*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frbwtgen.cpp	    helper code for making FrBWTIndex from text	*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 2003,2004,2006,2008,2012,2015				*/
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

#include "frframe.h"
#include "frsymtab.h"
#include "frbwt.h"
#include "frvocab.h"

/************************************************************************/
/*	Manifest Constants						*/
/************************************************************************/

#define DUMMY_SYMBOL ((FrFrame*)~0)

/************************************************************************/
/*	Types								*/
/************************************************************************/

/************************************************************************/
/*	Global Variables						*/
/************************************************************************/

size_t FrWordIDList::buffersize = 0 ;
size_t FrWordIDList::elts_per_buffer = 0 ;

/************************************************************************/
/*	Helper Functions						*/
/************************************************************************/

void FrBWTMarkAsDummy(FrSymbol *sym)
{
   if (sym)
      sym->setFrame(DUMMY_SYMBOL) ;
   return ;
}

//----------------------------------------------------------------------

static bool mark_dummysym(const FrSymbol *obj, FrNullObject, va_list)
{
   FrSymbol *sym = (FrSymbol*)obj ;
   if (sym)
      sym->setFrame(DUMMY_SYMBOL) ;
   return true ;			// continue iterating
}

//----------------------------------------------------------------------

static bool update_vocabulary(const FrSymbol *obj, FrNullObject, va_list args)
{
   FrSymbol *sym = (FrSymbol*)obj ;
   FrVarArg(FrVocabulary *,vocab) ;
   if (vocab && sym && sym->symbolFrame() != DUMMY_SYMBOL)
      vocab->addWord(sym->symbolName(),(size_t)sym->symbolFrame()) ;
   if (sym)
      sym->setFrame(0) ;
   return true ;			// continue iterating
}

/************************************************************************/
/*	methods for class FrWordIDList					*/
/************************************************************************/

FrWordIDList *FrWordIDList::newIDListElt(FrWordIDList *next)
{
   if (buffersize == 0 || elts_per_buffer == 0)
      {
      buffersize = FrMaxSmallAlloc() - sizeof(void*) ;
      elts_per_buffer = ((buffersize - sizeof(FrWordIDList)) /
			 sizeof(uint32_t)) + 1 ;
      }
   FrWordIDList *newlist = (FrWordIDList*)FrMalloc(buffersize) ;
   newlist->m_next = next ;
   newlist->m_elts_used = 0 ;
   return newlist ;
}

//----------------------------------------------------------------------

void FrWordIDList::freeIDList()
{
   FrWordIDList *list = this ;
   while (list)
      {
      FrWordIDList *tmp = list ;
      list = list->next() ;
      FrFree(tmp) ;
      }
   return ;
}

//----------------------------------------------------------------------

FrWordIDList *FrWordIDList::reverse()
{
   FrWordIDList *prev = 0 ;
   FrWordIDList *curr = this ;

   while (curr)
      {
      FrWordIDList *nxt = curr->next() ;
      curr->m_next = prev ;
      prev = curr ;
      curr = nxt ;
      }
   return prev ;
}

//----------------------------------------------------------------------

void FrWordIDList::setNth(size_t N, uint32_t value)
{
   if (N < eltsPerBuffer())
      buffer[N] = value ;
   return ;
}

//----------------------------------------------------------------------

bool FrWordIDList::add(uint32_t value)
{
   if (eltsUsed() < eltsPerBuffer())
      {
      buffer[m_elts_used++] = value ;
      return true ;
      }
   else
      return false ;
}

//----------------------------------------------------------------------

uint32_t FrWordIDList::getNth(size_t N) const
{
   if (N < eltsUsed())
      return buffer[N] ;
   else
      return ~0 ;
}

/************************************************************************/
/*	Procedural Interface						*/
/************************************************************************/

bool FrAdd2WordIDList(FrWordIDList *&idlist, size_t id)
{
   if (!idlist || idlist->full())
      {
      FrWordIDList *id_list = FrWordIDList::newIDListElt(idlist) ;
      if (id_list)
	 idlist = id_list ;
      else
	 {
	 FrNoMemory("while converting text to word IDs") ;
	 return false ;
	 }
      }
   idlist->add_safe(id) ;
   return true ;
}

//----------------------------------------------------------------------

size_t FrCountWordIDs(const FrWordIDList *list)
{
   size_t count = 0 ;
   for ( ; list ; list = list->next())
      count += list->eltsUsed() ;
   return count ;
}

//----------------------------------------------------------------------

uint32_t *FrAugmentWordIDArray(const FrWordIDList *list, uint32_t *prevIDs,
			       size_t &num_words, uint32_t eor,
			       bool reverse)
{
   size_t new_words = FrCountWordIDs(list) ;
   // as we may have trouble requesting a memory block for the full count
   //   right off the bat, work around the issue by first allocating a
   //   smallish block and then resizing it
   uint32_t *IDs = prevIDs ;
   if (!IDs && num_words + new_words > 100000)
      IDs = (uint32_t*)FrMalloc(FrMaxSmallAlloc()*2) ;
   IDs = FrNewR(uint32_t,IDs,num_words + new_words) ;
   if (!IDs)
      {
      cout << "*** unable to fill request for "
	   << sizeof(uint32_t) * num_words << " bytes! ***" << endl << endl ;
      FrMemoryStats() ;
      FrNoMemory("while allocating space for word IDs") ;
      return 0 ;
      }
   size_t count = num_words ;
   for ( ; list ; list = list->next())
      {
      for (size_t i = 0 ; i < list->eltsUsed() ; i++)
	 IDs[count++] = list->getNth(i) ;
      }
   if (reverse)
      {
      // since phrases can only efficiently be extended to the left, but we
      //   need to be able to extend on the right to get a conditional
      //   probability, reverse the contents of each record
      size_t record_start = num_words ;
      for (size_t i = num_words ; i < num_words + new_words ; i++)
	 {
	 if (IDs[i] >= eor)
	    {
	    if (i > record_start + 1)	// no need to reverse 1-word records
	       {
	       uint32_t *first = &IDs[record_start] ;
	       uint32_t *last = &IDs[i-1] ;
	       while (first < last)
		  {
		  uint32_t tmp = *first ;
		  *first++ = *last ;
		  *last-- = tmp ;
		  }
	       }
	    record_start = i + 1 ;
	    }
	 }
      }
   num_words += new_words ;
   return IDs ;
}

//----------------------------------------------------------------------

uint32_t *FrMakeWordIDArray(const FrWordIDList *list, size_t &num_words,
			    uint32_t eor, bool reverse)
{
   num_words = 0;
   return FrAugmentWordIDArray(list,0,num_words,eor,reverse) ;
}

//----------------------------------------------------------------------

bool FrBeginText2WordIDs(FrSymbolTable *&old_symtab)
{
   FrSymbolTable *symtab = new FrSymbolTable ;
   if (!symtab)
      {
      FrNoMemory("while setting up vocabulary") ;
      return false ;
      }
   symtab->iterate(mark_dummysym) ;
   old_symtab = symtab->select() ;
   return true ;
}

//----------------------------------------------------------------------

void FrText2WordIDsMarkNew(FrSymbol *sym)
{
   if (sym)
      sym->setFrame(DUMMY_SYMBOL) ;
   return ;
}

//----------------------------------------------------------------------

size_t FrCvtWord2WordID(const char *word, size_t &numIDs)
{
   if (!word)
      return FrVOCAB_WORD_NOT_FOUND ;
   FrSymbol *sym = findSymbol(word) ;
   size_t id ;
   if (!sym || (id = (size_t)sym->symbolFrame()) == (size_t)DUMMY_SYMBOL)
      {
      sym = FrSymbolTable::add(word) ;
      id = numIDs++ ;
      sym->setFrame((FrFrame*)id) ;
      }
   return id ;
}

//----------------------------------------------------------------------

FrVocabulary *FrFinishText2WordIDs(FrSymbolTable *old_symtab)
{
   if (!old_symtab)
      return 0 ;
   FrVocabulary *vocab = new FrVocabulary ;
   if (!vocab)
      {
      FrNoMemory("while finishing vocabulary creation") ;
      return 0 ;
      }
   FrSymbolTable *symtab = old_symtab->select() ;
   vocab->startBatchUpdate() ;
   symtab->iterate(update_vocabulary,vocab) ;
   vocab->finishBatchUpdate() ;
   delete symtab ;
   return vocab ;
}

// end of file frbwtgen.cpp //
