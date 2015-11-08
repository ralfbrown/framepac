/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frqueue.cpp	class FrQueue					*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,1998,2001,2004,2009		*/
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

#if defined(__GNUC__)
#  pragma implementation "frqueue.h"
#endif

#include "frqueue.h"
#include "frpcglbl.h"

#ifdef FrSTRICT_CPLUSPLUS
#  include <iostream>
#else
#  include <iostream.h>
#endif

/************************************************************************/
/*	Global variables for class FrQueue				*/
/************************************************************************/

static FrObject *read_Queue(istream &input, const char *) ;
static FrObject *string_to_Queue(const char *&input, const char *) ;
static bool verify_Queue(const char *&input, const char *, bool strict) ;

static FrReader FrQueue_reader(string_to_Queue, read_Queue, verify_Queue,
			       FrREADER_LEADIN_LISPFORM,"Q") ;

/************************************************************************/
/*    Member functions for class FrQueue				*/
/************************************************************************/

FrQueue::FrQueue(const FrList *items)
{
   qhead = items ? (FrList*)items->deepcopy() : 0 ;
   qtail = qhead ? qhead->last() : 0 ;
   qlength = qhead->listlength() ;
}

//----------------------------------------------------------------------

FrQueue::~FrQueue()
{
   qhead->eraseList(true) ;
   qhead = qtail = 0 ;
   qlength = 0 ;
}

//----------------------------------------------------------------------

FrObjectType FrQueue::objType() const
{
   return OT_FrQueue ;
}

//----------------------------------------------------------------------

const char *FrQueue::objTypeName() const
{
   return "FrQueue" ;
}

//----------------------------------------------------------------------

FrObjectType FrQueue::objSuperclass() const
{
   return OT_FrObject ;
}

//----------------------------------------------------------------------

void FrQueue::freeObject()
{
   delete this ;
}

//----------------------------------------------------------------------

bool FrQueue::queuep() const
{
   return true ;
}

//----------------------------------------------------------------------

FrObject *FrQueue::copy() const
{
   return new FrQueue(qhead) ;
}

//----------------------------------------------------------------------

FrObject *FrQueue::deepcopy() const
{
   return new FrQueue(qhead) ;
}

//----------------------------------------------------------------------

unsigned long FrQueue::hashValue() const
{
   // the hash value of a queue is the hash value of the list of items
   // in the queue
   return qhead->hashValue() ;
}

//----------------------------------------------------------------------

void FrQueue::clear()
{
   qhead->eraseList(true) ;
   qhead = qtail = 0 ;
   qlength = 0 ;
}

//----------------------------------------------------------------------

FrObject *FrQueue::car() const
{
   return qhead ? qhead->first() : 0 ;
}

//----------------------------------------------------------------------

size_t FrQueue::length() const
{
   return qlength ;
}

//----------------------------------------------------------------------

FrObject *FrQueue::subseq(size_t start, size_t stop) const
{
   FrList *items = qhead ? (FrList*)qhead->subseq(start,stop) : 0 ;
   FrQueue *newqueue = new FrQueue ;
   newqueue->qhead = items ;
   newqueue->qtail = items->last() ;
   return newqueue ;
}

//----------------------------------------------------------------------

FrObject *FrQueue::reverse()
{
   if (qhead)
      {
      qhead = listreverse(qhead) ;
      qtail = qhead->last() ;
      }
   return this ;
}

//----------------------------------------------------------------------

FrObject *FrQueue::getNth(size_t N) const
{
   if (qhead)
      return qhead->getNth(N) ;
   else
      return 0 ;
}

//----------------------------------------------------------------------

bool FrQueue::setNth(size_t N, const FrObject *newelt)
{
   if (qhead)
      return qhead->setNth(N,newelt) ;
   else
      return false ;			// not updated
}

//----------------------------------------------------------------------

size_t FrQueue::locate(const FrObject *item, size_t startpos) const
{
   if (qhead)
      return qhead->locate(item,startpos) ;
   else
      return (size_t)-1 ;
}

//----------------------------------------------------------------------

size_t FrQueue::locate(const FrObject *item, FrCompareFunc cmp,
			size_t startpos) const
{
   if (qhead)
      return qhead->locate(item,cmp,startpos) ;
   else
      return (size_t)-1 ;
}

//----------------------------------------------------------------------

FrObject *FrQueue::insert(const FrObject *newelts, size_t pos, bool cp)
{
   if (qhead)
      qhead = (FrList*)qhead->insert(newelts,pos,cp) ;
   else
      {
      if (newelts)
	 {
	 if (cp)
	    newelts = newelts->deepcopy() ;
	 if (newelts->consp())
	    qhead = (FrList*)newelts ;
	 else
	    qhead = new FrList(newelts) ;
	 }
      else
	 qhead = new FrList(0) ;
      }
   qtail = qhead->last() ;
   return this ;
}

//----------------------------------------------------------------------

FrObject *FrQueue::elide(size_t start, size_t end)
{
   if (qhead)
      {
      qhead = (FrList*)qhead->elide(start,end) ;
      qtail = qhead->last() ;
      }
   return this ;
}

//----------------------------------------------------------------------

FrObject *FrQueue::removeDuplicates() const
{
   FrQueue *result = new FrQueue ;
   if (result)
      {
      for (const FrList *q = qhead ; q ; q = q->rest())
	 {
	 FrObject *obj = q->first() ;
	 if (result->qhead->member(obj))
	    result->add(obj ? obj->deepcopy() : 0) ;
	 }
      }
   else
      FrNoMemory("in FrQueue::removeDuplicates") ;
   return result ;
}

//----------------------------------------------------------------------

FrObject *FrQueue::removeDuplicates(FrCompareFunc cmp) const
{
   FrQueue *result = new FrQueue ;
   if (result)
      {
      for (const FrList *q = qhead ; q ; q = q->rest())
	 {
	 FrObject *obj = q->first() ;
	 if (result->qhead->member(obj,cmp))
	    result->add(obj ? obj->deepcopy() : 0) ;
	 }
      }
   else
      FrNoMemory("in FrQueue::removeDuplicates") ;
   return result ;
}

//----------------------------------------------------------------------

void FrQueue::add(const FrObject *item,bool do_copy)
{
   FrList *newitem = new FrList((item && do_copy) ? item->deepcopy() : item) ;
   if (qhead)
      {
      qtail->replacd(newitem) ;
      qtail = newitem ;
      }
   else
      qhead = qtail = newitem ;
   qlength++ ;
}

//----------------------------------------------------------------------

void FrQueue::addFront(const FrObject *item, bool do_copy)
{
   FrList *newitem = new FrList ;
   newitem->replaca((item && do_copy) ? item->deepcopy() : item) ;
   newitem->replacd(qhead) ;
   if (qhead)
      qhead = newitem ;
   else
      qhead = qtail = newitem ;
   qlength++ ;
}

//----------------------------------------------------------------------

bool FrQueue::remove(const FrObject *item)
{
   FrList *prev = 0 ;
   FrList *curr = qhead ;

   while (curr && curr->first() != item)
      {
      prev = curr ;
      curr = curr->rest() ;
      }
   if (curr)
      {
      if (prev)
	 prev->replacd(curr->rest()) ;
      else
	 qhead = curr->rest() ;
      curr->replaca(0) ;
      curr->replacd(0) ;
      delete curr ;
      qlength-- ;
      return true ;
      }
   else
      return false ;
}

//----------------------------------------------------------------------

bool FrQueue::remove(const FrObject *item,FrCompareFunc cmp)
{
   FrList *prev = 0 ;
   FrList *curr = qhead ;

   while (curr && !cmp(curr->first(),item))
      {
      prev = curr ;
      curr = curr->rest() ;
      }
   if (curr)
      {
      if (prev)
	 prev->replacd(curr->rest()) ;
      else
	 qhead = curr->rest() ;
      curr->replacd(0) ;
      delete curr ;
      qlength-- ;
      return true ;
      }
   else
      return false ;
}

//----------------------------------------------------------------------

FrObject *FrQueue::find(const FrObject *item, FrCompareFunc cmp) const
{
   for (FrList *q = qhead ; q ; q = q->rest())
      if (cmp(item,q->first()))
	 return q->first() ;
   return 0 ;  // not found
}

//----------------------------------------------------------------------

FrObject *FrQueue::pop()
{
   FrObject *item = poplist(qhead) ;
   if (!qhead)
      qtail = 0 ;
   qlength-- ;
   return item ;
}

//----------------------------------------------------------------------
// call a specified function for every item in the queue

bool FrQueue::iterateVA(FrIteratorFunc func, va_list args) const
{
   for (const FrList *l = qhead ; l ; l = l->rest())
      {
      FrSafeVAList(args) ;
      bool success = func(l->first(),FrSafeVarArgs(args)) ;
      FrSafeVAListEnd(args) ;
      if (!success)
	 return false ;
      }
   return true ;
}

//----------------------------------------------------------------------

ostream &FrQueue::printValue(ostream &out) const
{
   size_t orig_indent = FramepaC_initial_indent ;
   FramepaC_initial_indent += 2 ;
   out << "#Q" << qhead ;
   FramepaC_initial_indent = orig_indent ;
   return out ;
}

//----------------------------------------------------------------------

size_t FrQueue::displayLength() const
{
   return qhead->displayLength() + sizeof("#Q")-1 ;
}

//----------------------------------------------------------------------

char *FrQueue::displayValue(char *buffer) const
{
   *buffer++ = '#' ;
   *buffer++ = 'Q' ;
   return qhead->displayValue(buffer) ;
}

//----------------------------------------------------------------------

static FrObject *read_Queue(istream &input, const char *)
{
   FrObject *result = read_FrObject(input) ;
   if (result && result->consp())
      return new FrQueue((FrList*)result) ;
   else
      return new FrQueue(0) ;
}

//----------------------------------------------------------------------

static FrObject *string_to_Queue(const char *&input, const char *)
{
   FrObject *result = string_to_FrObject(input) ;
   if (result && result->consp())
      return new FrQueue((FrList*)result) ;
   else
      return new FrQueue(0) ;
}


//----------------------------------------------------------------------

static bool verify_Queue(const char *&input, const char *, bool strict)
{
   return valid_FrObject_string(input,strict) ;
}

// end of file frqueue.cpp //
