/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frsarray.cpp	    class FrSparseArray				*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1997,1998,2001,2002,2004,2006,2008,2009,2012,2013,	*/
/*		2014,2015 Ralf Brown/Carnegie Mellon University		*/
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

#include <stdlib.h>
#include "framerr.h"
#include "frarray.h"
#include "frassert.h"
#include "frcmove.h"
#include "frnumber.h"
#include "frprintf.h"
#include "frreader.h"
#include "frpcglbl.h"

#ifdef FrSTRICT_CPLUSPLUS
#  include <iomanip>
#else
#  include <iomanip.h>
#endif

/**********************************************************************/
/*    Manifest constants					      */
/**********************************************************************/

#define MIN_MAX_SIZE (sizeof(FrArrayFrag)/sizeof(FrArrayFrag*))

#define array_frags(n) (((n)+FrARRAY_FRAG_SIZE-1)/FrARRAY_FRAG_SIZE)

/**********************************************************************/
/*    Global variables local to this module			      */
/**********************************************************************/

FrAllocator FrSparseArray::allocator("FrSparseArray",sizeof(FrSparseArray)) ;

static FrObject *read_FrSparseArray(istream &input, const char *) ;
static FrObject *string_to_FrSparseArray(const char *&input, const char *) ;
static bool verify_FrSparseArray(const char *&input, const char *,bool) ;

static FrReader FrSparseArray_reader(string_to_FrSparseArray,
				     read_FrSparseArray, verify_FrSparseArray,
				     FrREADER_LEADIN_LISPFORM,"SpArr") ;

static const char errmsg_bounds[]
      = "array access out of bounds" ;

/**********************************************************************/
/*    Global variables which are visible from other modules	      */
/**********************************************************************/

FrAllocator FrArrayFrag::allocator("FrArrayFrag",sizeof(FrArrayFrag)) ;

/**********************************************************************/
/*    Helper functions						      */
/**********************************************************************/

#define update_arrayhash FramepaC_update_ulong_hash

/************************************************************************/
/*	methods for class FrArrayFrag					*/
/************************************************************************/

void FrArrayFrag::reserve(size_t N)
{
   size_t objs_per_block = allocator.objectsPerPage() ;
   for (size_t i = allocator.objects_allocated() ;
	i < N ;
	i += objs_per_block)
      {
      allocator.preAllocate() ;
      }
   return ;
}

/************************************************************************/
/*	methods for class FrSparseArray					*/
/************************************************************************/

FrSparseArray::FrSparseArray()
{
   m_index_cap = 1 ;
   items = 0 ;
   m_nonobject = false ;
   allocFragmentList() ;
   return ;
}

//----------------------------------------------------------------------

FrSparseArray::FrSparseArray(size_t max)
{
   m_index_cap = array_frags(max) ;
   items = 0 ;
   m_nonobject = false ;
   allocFragmentList() ;
   return ;
}

//----------------------------------------------------------------------

FrSparseArray::FrSparseArray(const FrSparseArray *init, size_t start,
			     size_t stop)
{
   m_length = stop-start+1 ;
   m_index_cap = array_frags(arrayLength()) ;
   items = 0 ;
   m_nonobject = false ;
   allocFragmentList() ;
   size_t dest = 0 ;
   for (size_t i = start ; i <= stop ; i = init->nextItem(i))
      {
      FrArrayItem &itm = init->item(i) ;
      uintptr_t index = itm.getIndex() ;
      FrObject *value = itm.getValue() ;
      new (&item(dest)) FrArrayItem(index, value ? value->deepcopy() : 0) ;
      ++dest ;
      }
   return ;
}

//----------------------------------------------------------------------

FrSparseArray::FrSparseArray(const FrList *initializer)
{
   size_t len = initializer->listlength() ;
   m_index_cap = array_frags(len) ;
   m_index_size = 0 ;
   items = 0 ;
   m_nonobject = false ;
   allocFragmentList() ;
   m_length = 0 ;
   uintptr_t highest_index = 0 ;
   for ( ; initializer ; initializer = initializer->rest())
      {
      const FrObject *init = initializer->first() ;
      if (!init)
	 continue ;
      FrObject *value = 0 ;
      uintptr_t index ;
      if (init->consp())
	 {
	 FrObject *init1 = ((FrList*)init)->first() ;
	 if (!init1)
	    continue ;
	 else if (init1->numberp())
	    index = (uintptr_t)init1->intValue() ;
	 else if (init1->symbolp())
	    index = (uintptr_t)init1 ;
	 else
	    continue ;
	 value = ((FrList*)init)->rest() ;
	 }
      else if (init->numberp())
	 {
	 index = (uintptr_t)init->intValue() ;
	 }
      else if (init->symbolp())
	 {
	 index = (uintptr_t)init ;
	 }
      else
	 continue ;
      if (index > highest_index)
	 {
	 highest_index = index ; 
	 new (&item(m_length)) FrArrayItem(index,value ? value->deepcopy() : 0) ;
	 }
      else
	 {
	 size_t idx = m_length ; //FIXME: find correct in-order position
	 new (&item(idx)) FrArrayItem(index,value ? value->deepcopy() : 0) ;
	 }
      ++m_length ;
      }
   return ;
}

//----------------------------------------------------------------------

FrSparseArray::~FrSparseArray()
{
   FrFree(m_fill_counts) ;
   m_fill_counts = 0 ;
   if (items)
      {
      for (size_t i = 0 ; i < m_index_size ; ++i)
	 {
	 delete items[i] ;
	 }
      if (m_index_size <= MIN_MAX_SIZE)
	 {
	 FrArrayFrag *frag = (FrArrayFrag*)items ;
	 delete frag ;
	 }
      else
	 FrFree(items) ;
      items = 0 ;
      }
   m_length = 0 ;
   m_index_cap = m_index_size = 0 ;
   return ;
}

//----------------------------------------------------------------------

const char *FrSparseArray::objTypeName() const
{
   return "FrSparseArray" ;
}

//----------------------------------------------------------------------

FrObjectType FrSparseArray::objType() const
{
   return OT_FrSparseArray ;
}

//----------------------------------------------------------------------

FrObjectType FrSparseArray::objSuperclass() const
{
   return OT_FrArray ;
}

//----------------------------------------------------------------------

unsigned long FrSparseArray::hashValue() const
{
   // the hash value of an array is a combination of the hash values of all
   // its elements
   unsigned long hash = 0 ;
   if (arrayLength() > 0)
      {
      // iterate through the array fragments
      for (size_t i = 0 ; i < m_index_size ; ++i)
	 {
	 FrArrayFrag *frag = items[i] ;
	 if (!frag)
	    continue ;
	 // in each fragment, iterate over just the elements actually in use
	 for (size_t pos = 0 ; pos < m_fill_counts[i]; ++pos)
	    {
	    FrArrayItem &curritem = frag->items[pos] ;
	    uintptr_t idx = curritem.getIndex() ;
	    FrObject *val = curritem.getValue() ;
	    hash = update_arrayhash(hash,idx) ;
	    hash = update_arrayhash(hash,(uintptr_t)val) ;
	    }
	 }
      }
   return hash ;
}

//----------------------------------------------------------------------

void FrSparseArray::freeObject()
{
   delete this ;
   return ;
}

//----------------------------------------------------------------------

FrObject *FrSparseArray::copy() const
{
   return new FrSparseArray(this,0,arrayLength()-1) ;
}

//----------------------------------------------------------------------

FrObject *FrSparseArray::deepcopy() const
{
   return new FrSparseArray(this,0,arrayLength()-1) ;
}

//----------------------------------------------------------------------

bool FrSparseArray::expand(size_t increment)
{
   return expandTo(m_index_cap * FrARRAY_FRAG_SIZE + increment) ;
}

//----------------------------------------------------------------------

void FrSparseArray::allocFragmentList()
{
   if (m_index_cap <= MIN_MAX_SIZE)
      {
      m_index_cap = MIN_MAX_SIZE ;
      if (!items)
	 items = (FrArrayFrag**)new FrArrayFrag ;
      memset(items,'\0',sizeof(*items)) ;
      }
   else
      items = FrNewC(FrArrayFrag*,m_index_cap) ;
   m_fill_counts = FrNewC(uint8_t,m_index_cap+1) ;
   m_index_size = 0 ;
   m_packed = true ;
   return ;
}

//----------------------------------------------------------------------

bool FrSparseArray::reallocFragmentList(size_t new_frags)
{
   if (new_frags <= m_index_cap)
      return true ;			// for simplicity, never shrink the list
   // expand the per-fragment usage counts
   uint8_t *new_fill = FrNewR(uint8_t,m_fill_counts,new_frags+1) ;
   if (!new_fill)
      return false ;
   memset(new_fill+m_index_cap,'\0',new_frags-m_index_cap+1) ;
   m_fill_counts = new_fill ;
   // expand the fragment list
   if (m_index_cap == MIN_MAX_SIZE)
      {
      // special case, since we used a FrArrayFrag instead of malloc()
      FrArrayFrag **newfrags = FrNewN(FrArrayFrag*,new_frags) ;
      if (!newfrags)
	 return false ;
      memcpy(newfrags,items,m_index_size*sizeof(FrArrayFrag*)) ;
      FrArrayFrag *fr = (FrArrayFrag*)items ;
      delete fr ;
      items = newfrags ;
      }
   else
      {
      FrArrayFrag **newfrags = FrNewR(FrArrayFrag*,items,new_frags) ;
      if (!newfrags)
	 return false ;
      items = newfrags ;
      }
   m_index_cap = new_frags ;
   return true ;
}

//----------------------------------------------------------------------

bool FrSparseArray::expandTo(size_t newsize)
{
   newsize = array_frags(newsize) ;
   if (newsize <= m_index_cap)
      return true ;			// trivially successful
   return reallocFragmentList(newsize) ;
}

//----------------------------------------------------------------------

FrObject *FrSparseArray::car() const
{
   return (arrayLength() > 0) ? (FrObject*)item(0).getValue() : 0 ;
}

//----------------------------------------------------------------------

FrObject *FrSparseArray::reverse()
{
   return this ;			// reverse is meaningless for sparse
					// arrays....
}

//----------------------------------------------------------------------

FrObject *FrSparseArray::lookup(uintptr_t index) const
{
   size_t pos = findIndexOrHigher(index) ;
   size_t frag = pos / FrARRAY_FRAG_SIZE ;
   if (frag >= m_index_size)
      return 0 ;
   pos %= FrARRAY_FRAG_SIZE ;
   if (pos >= m_fill_counts[frag])
      return 0 ;
   FrArrayItem &itm = items[frag]->items[pos] ;
   return (itm.getIndex() == index) ? itm.getValue() : 0 ;
}

//----------------------------------------------------------------------

bool FrSparseArray::createAndFillGap(size_t pos, size_t N,
				     const FrObject *value)
{
   size_t fragnum = pos / FrARRAY_FRAG_SIZE ;
   pos %= FrARRAY_FRAG_SIZE ;
   size_t fill = m_fill_counts[fragnum] ;
   if (fill < FrARRAY_FRAG_SIZE)
      {
      // fast case: we only need to shift elements within the current fragment, or even just
      //   append the new item to the fragment
      if (fragnum >= m_index_size || !items[fragnum])	    // are we creating a new fragment?
	 {
	 // if the current fragment list is full, expand it
	 if (m_index_size >= m_index_cap &&
	     !reallocFragmentList(5 * m_index_cap / 4))
	    {
	    return false ;
	    }
	 items[fragnum] = new FrArrayFrag ;
	 if (fragnum >= m_index_size)
	    {
	    ++m_index_size ;
	    }
	 }
      }
   else if (fragnum + 1 < m_index_size && m_fill_counts[fragnum+1] < FrARRAY_FRAG_SIZE)
      {
      // the following fragment has room, so avoid splitting by moving
      //   the last item in the current fragment into the following
      //   fragment
      memmove(&items[fragnum+1]->items[1],&items[fragnum+1]->items[0],
	      m_fill_counts[fragnum+1] * sizeof(items[0]->items[0])) ;
      items[fragnum+1]->items[0] = items[fragnum]->items[fill-1] ;
      ++m_fill_counts[fragnum+1] ;
      fill = --m_fill_counts[fragnum] ;
      }
   else if (fragnum > 0 && m_fill_counts[fragnum-1] < FrARRAY_FRAG_SIZE)
      {
      // previous fragment has space, so avoid splitting by moving one
      //   item to the previous fragment to create our needed gap
      if (pos == 0)
	 {
	 // the new item needs to be inserted before the first item
	 //   in the current fragment, so it's the one we'll store in
	 //   the previous fragment
	 --fragnum ;
	 pos = fill = m_fill_counts[fragnum] ;
	 }
      else
	 {
	 // move the first item in the current fragment into the end of the
	 //   previous fragment
	 size_t prevfill = m_fill_counts[fragnum-1]++ ;
	 items[fragnum-1]->items[prevfill] = items[fragnum]->items[0] ;
	 //  we'll be creating a gap just PRIOR to the computed position
	 //    by moving down entries, so adjust the pointer; update 'fill'
	 //    so that we don't do any moving while creating the new entry later
	 fill = --pos ;
	 --m_fill_counts[fragnum] ;	// will get re-incremented
	 if (pos > 0)
	    {
	    memmove(&items[fragnum]->items[0],&items[fragnum]->items[1],
		   pos * sizeof(items[0]->items[0])) ;
	    }
	 fill = pos ;
	 }
      }
   else
      {
      // slowest case: split the current fragment
      // if the current fragment list is full, expand it
      if (m_index_size >= m_index_cap &&
	  !reallocFragmentList(5 * m_index_cap / 4))
	 {
	 return false ;
	 }
      size_t to_move = (m_index_size - fragnum - 1) ;
      if (to_move)
	 {
	 // we need to shift the following fragments to make room for the
	 //   one we are about to create
	 memmove(&items[fragnum+2],&items[fragnum+1],sizeof(items[0])*to_move) ;
	 memmove(&m_fill_counts[fragnum+2],&m_fill_counts[fragnum+1],sizeof(m_fill_counts[0])*to_move) ;
	 }
      // create the new fragment and copy half of the items from the current
      //   to the new fragment
      ++m_index_size ;
      m_packed = false ;
      items[fragnum+1] = new FrArrayFrag ;
      for (size_t i = FrARRAY_FRAG_SIZE/2 ; i < fill ; ++i)
	 {
	 items[fragnum+1]->items[i-FrARRAY_FRAG_SIZE/2] = items[fragnum]->items[i] ;
	 }
      m_fill_counts[fragnum] = FrARRAY_FRAG_SIZE / 2 ;
      m_fill_counts[fragnum+1] = fill - (FrARRAY_FRAG_SIZE/2) ;
      // select the appropriate fragment into which to insert the new value
      if (pos >= FrARRAY_FRAG_SIZE / 2)
	 {
	 // we wound up in the new fragment
	 ++fragnum ;
	 pos -= (FrARRAY_FRAG_SIZE / 2) ;
	 }
      fill = m_fill_counts[fragnum] ;
      }

   // shift the entries following the insertion point
   size_t to_move = fill - pos ;
   FrArrayItem *new_item = &items[fragnum]->items[pos] ;
   if (to_move)
      memmove(new_item+1,new_item,sizeof(items[0]->items[0])*to_move) ;
   // finally, add the new item into the gap we just created
   ++m_fill_counts[fragnum] ;
   ++m_length;
   new (new_item) FrArrayItem(N,(FrObject*)value) ;
   return true ;
}

//----------------------------------------------------------------------

bool FrSparseArray::addItem(uintptr_t index, const FrObject *value,
			    int if_exists)
{
   size_t pos = findIndexOrHigher(index) ;
   size_t frag = pos / FrARRAY_FRAG_SIZE ;
   size_t ofs = pos % FrARRAY_FRAG_SIZE ;

   if (frag < m_index_size && ofs < m_fill_counts[frag])
      {
      FrArrayItem &itm = items[frag]->items[ofs] ;
      if (itm.getIndex() == index)
	 {
	 // the item exists, so follow callers instructions on what to do
	 if (if_exists == FrArrAdd_INCREMENT)
	    itm.addOccurrence((uintptr_t)value) ;
	 else if (if_exists == FrArrAdd_REPLACE)
	    {
	    free_object(itm.getValue()) ;
	    itm.setValue(value ? value->deepcopy() : 0) ;
	    }
	 else // if (if_exists == FrArrAdd_RETAIN)
	    {
	    // leave as-is
	    }
	 return true ;
	 }
      }
   // if we get here, the desired index was not yet in our list, and it
   // should go into location 'pos' after shifting 'pos' and following up
   // by one position
   return createAndFillGap(pos,index,value) ;
}

//----------------------------------------------------------------------

FrObject *FrSparseArray::getNth(size_t N) const
{
   size_t pos = findIndexOrHigher(N) ;
   if (pos < highestPosition())
      {
      FrArrayItem &itm = item(pos) ;
      if (itm.getIndex() == N)
	 return itm.getValue() ;
      }
   else if (range_errors)
      FrWarning(errmsg_bounds) ;
   return 0 ;
}

//----------------------------------------------------------------------

bool FrSparseArray::setNth(size_t N, const FrObject *elt)
{
   return addItem(N,elt) ;
}

//----------------------------------------------------------------------

bool FrSparseArray::nextItem(size_t &frag_num, size_t &elt_in_frag) const
{
   if (frag_num >= m_index_size)
      return false ;
   else if (frag_num + 1 == m_index_size && elt_in_frag >= m_fill_counts[frag_num])
      return false ;
   // if there are  more elements in the current fragment, simply increment the offset
   if (++elt_in_frag < m_fill_counts[frag_num])
      {
      return true ;
      }
   // else, we have to advance to the next fragment with elements in use
   elt_in_frag = 0 ;
   while (++frag_num < m_index_size)
      {
      if (items[frag_num] && m_fill_counts[frag_num] > 0)
	 {
	 return true ;
	 }
      }
   // no suitable fragment found
   return false ;
}

//----------------------------------------------------------------------

size_t FrSparseArray::nextItem(size_t curritem) const
{
   size_t fragnum = curritem / FrARRAY_FRAG_SIZE ;
   size_t ofs = curritem % FrARRAY_FRAG_SIZE ;
   if (nextItem(fragnum,ofs))
      {
      return (fragnum * FrARRAY_FRAG_SIZE) + ofs ;
      }
   else
      return (size_t)~0 ;
}

//----------------------------------------------------------------------

size_t FrSparseArray::highestPosition() const
{
   if (m_index_size > 0)
      {
      size_t idx = m_index_size - 1 ;
      while (idx > 0 && (!items[idx] || m_fill_counts[idx] == 0))
	 {
	 --idx ;
	 }
      return (idx * FrARRAY_FRAG_SIZE) + m_fill_counts[idx] ;
      }
   return 0 ;
}

//----------------------------------------------------------------------

size_t FrSparseArray::findIndex(size_t N)
{
   size_t pos = findIndexOrHigher(N) ;
   if (pos <= highestPosition())
      {
      FrArrayItem &itm = item(pos) ;
      if (itm.getIndex() == N)
	 return pos ;
      }
   // if we get here, the desired index was not yet in our list, and it
   // should go into location 'pos' after shifting 'pos' and following up
   // by one position
   return createAndFillGap(pos,N,0) ? pos : (size_t)~0 ;
}

//----------------------------------------------------------------------

size_t FrSparseArray::findIndexOrHigher(size_t N) const
{
   // two-phase binary search on the array's items, looking for the index
   // first phase: find the correct fragment
   size_t hi = m_index_size ;
   size_t lo = 0 ;
   if (hi == 0)				// is array empty?
      return 0 ;			// if so, insertion point is first position	
   while (hi > lo)
      {
      size_t mid = lo + (hi-lo)/2 ;
      FrArrayItem &miditem = items[mid]->items[0] ;
      if (N > miditem.getIndex())
	 {
	 size_t fill = m_fill_counts[mid] ;
	 FrArrayItem &miditm = items[mid]->items[fill-1] ;
	 if (N <= miditm.getIndex())
	    {
	    lo = hi = mid ;
	    break ;
	    }
	 lo = mid + 1 ;
	 if (lo >= m_index_size)
	    {
	    return mid * FrARRAY_FRAG_SIZE + fill ;
	    }
	 }
      else
	 {
	 hi = mid ;
	 }
      }
   // second phase: search the fragment for the correct element
   const FrArrayFrag *frag = items[lo] ;
   size_t fragbase = lo * FrARRAY_FRAG_SIZE ;
   hi = m_fill_counts[lo] ;
   lo = 0 ;
#if 1
   // to deal with the corner case of searching for a value higher than the highest
   //   one in the fragment, we need to start at the full fragment size instead of
   //   half the size, resulting in one additional iteration
   for (size_t step = FrARRAY_FRAG_SIZE ; step ; step >>= 1)
      {
      size_t mid = lo + step - 1 ;
#if defined(VALGRIND) || defined(PURIFY)
      // use short-circuited evaluation to avoid accessing uninit data; this will
      //  add branching, but the compiler will probably still unroll the loop
      size_t move = (mid < hi && N > frag->items[mid].getIndex()) ;
#else
      // a pure branchless version if the CPU supports conditional moves or
      //   setting register value from flags; can access items which are not
      //   currently valid, but the incorrect comparison gets nullified by the
      //   first comparison
      size_t move = (mid < hi) & (N > frag->items[mid].getIndex()) ;
#endif
      lo += (step * move) ;
      }
#else // OLD version with branching
   while (hi > lo)
      {
      size_t mid = lo + (hi-lo)/2 ;
      const FrArrayItem &miditem = frag->items[mid] ;
      uintptr_t mididx = miditem.getIndex() ;
      if (N > mididx)
	 {
	 lo = mid + 1 ;
	 }
      else
	 {
	 hi = mid ;
	 }
      }
   // when we get here, 'lo' points at the smallest element which is
   //   greater than or equal to the value we searched for
#endif
   return fragbase + lo ;
}

//----------------------------------------------------------------------

FrObject *&FrSparseArray::operator [] (size_t N)
{
   size_t idx = findIndex(N) ;
   return *item(idx).getValueRef() ;
}

//----------------------------------------------------------------------

size_t FrSparseArray::locate(const FrObject *finditem, size_t start) const
{
   if (!items || arrayLength() == 0)
      return (size_t)-1 ;
   if (start == (size_t)-1)
      start = 0 ;
   else
      ++start ;
   start = findIndexOrHigher(start) ;
   size_t highestpos = highestPosition() ;
   if (finditem && finditem->consp())
      {
      for (size_t pos = start ; pos < highestpos ; pos = nextItem(pos))
	 {
	 FrArrayItem &itm = item(pos) ;
	 if (itm.getValue() == ((FrList*)finditem)->first())
	    return (size_t)itm.getIndex() ;
	 }
      }
   else
      {
      for (size_t pos = start ; pos < highestpos ; pos = nextItem(pos))
	 {
	 FrArrayItem &itm = item(pos) ;
	 if (itm.getValue() == finditem)
	    return itm.getIndex() ;
	 }
      }
   return (size_t)-1 ;
}

//----------------------------------------------------------------------

size_t FrSparseArray::locate(const FrObject *finditem, FrCompareFunc cmp,
			     size_t start) const
{
   if (!items || arrayLength() == 0)
      return (size_t)-1 ;
   if (start == (size_t)-1)
      start = 0 ;
   else
      ++start ;
   start = findIndexOrHigher(start) ;
   size_t highestpos = highestPosition() ;
   if (finditem && finditem->consp())
      {
      for (size_t pos = start ; pos < highestpos ; pos = nextItem(pos))
	 {
	 FrArrayItem &itm = item(pos) ;
	 if (cmp(itm.getValue(),((FrList*)finditem)->first()))
	    return (size_t)itm.getIndex() ;
	 }
      }
   else
      {
      for (size_t pos = start ; pos < highestpos ; pos = nextItem(pos))
	 {
	 FrArrayItem &itm = item(pos) ;
	 if (cmp(itm.getValue(),finditem))
	    return itm.getIndex() ;
	 }
      }
   return (size_t)-1 ;
}

//----------------------------------------------------------------------

bool FrSparseArray::iterateVA(FrIteratorFunc func, va_list args) const
{
   // iterate over the array fragments
   for (size_t i = 0 ; i < m_index_size ; ++i)
      {
      const FrArrayFrag *frag = items[i] ;
      if (!frag)
	 continue ;
      // now iterate over the elements of the fragment actually in use
      for (size_t pos = 0 ; pos < m_fill_counts[i] ; ++pos)
	 {
	 const FrArrayItem &itm = frag->items[pos] ;
	 FrSafeVAList(args) ;
	 bool success = func(itm.getValue(),FrSafeVarArgs(args)) ;
	 FrSafeVAListEnd(args) ;
	 if (!success)
	    return false ;
	 }
      }
   return true ;
}

//----------------------------------------------------------------------

ostream &FrSparseArray::printValue(ostream &output) const
{
   output << "#SpArr(" ;
   size_t loc = FramepaC_initial_indent + 3 ;
   size_t orig_indent = FramepaC_initial_indent ;
   // iterate over the array fragments
   for (size_t i = 0 ; i < m_index_size ; ++i)
      {
      const FrArrayFrag *frag = items[i] ;
      if (!frag)
	 continue ;
      // now iterate over the elements of the fragment actually in use
      for (size_t pos = 0 ; pos < m_fill_counts[i] ; ++pos)
	 {
	 const FrArrayItem &itm = frag->items[pos] ;
	 FramepaC_initial_indent = loc ;
	 FrInteger indexnum(itm.getIndex()) ;
	 FrObject *value = (FrObject*)itm.getValue() ;
	 size_t indexlen = indexnum.displayLength() ;
	 size_t valuelen ;
	 size_t len ;
	 if (nonObjectArray())
	    {
	    valuelen = Fr_sprintf(0,0,"%ld",((uintptr_t)value)) ;
	    }
	 else
	    {
	    valuelen = value ? value->displayLength() : 2 ;
	    }
	 len = (indexlen + valuelen + 3) ;
	 loc += len ;
	 if (loc > FramepaC_display_width && pos != 0)
	    {
	    loc = orig_indent+7 ;
	    output << '\n' << setw(loc) << " " ;
	    FramepaC_initial_indent = loc ;
	    loc += len ;
	    }
	 output << '(' << indexnum << ' ' ;
	 if (nonObjectArray())
	    {
	    output << ((uintptr_t)value) ;
	    }
	 else if (value)
	    value->printValue(output) ;
	 else
	    output << "()" ;
	 output << ')' ;
	 if (pos < arrayLength()-1)
	    output << ' ' ;
	 }
      }
   FramepaC_initial_indent = orig_indent ;
   return output << ")" ;
}

//----------------------------------------------------------------------

char *FrSparseArray::displayValue(char *buffer) const
{
   memcpy(buffer,"#SpArr(",7) ;
   buffer += 7 ;
   // iterate over the array fragments
   for (size_t i = 0 ; i < m_index_size ; ++i)
      {
      const FrArrayFrag *frag = items[i] ;
      if (!frag)
	 continue ;
      // now iterate over the elements of the fragment actually in use
      for (size_t pos = 0 ; pos < m_fill_counts[i] ; ++pos)
	 {
	 const FrArrayItem &itm = frag->items[pos] ;
	 *buffer++ = '(' ;
	 FrInteger indexnum(itm.getIndex()) ;
	 FrObject *value = (FrObject*)itm.getValue() ;
	 buffer = indexnum.displayValue(buffer) ;
	 *buffer++ = ' ' ;
	 if (nonObjectArray())
	    {
	    buffer += Fr_sprintf(buffer,30,"%ld",((uintptr_t)value)) ;
	    }
	 else if (value)
	    buffer = value->displayValue(buffer) ;
	 else
	    {
	    *buffer++ = '(' ;
	    *buffer++ = ')' ;
	    }
	 *buffer++ = ')' ;
	 if (pos < arrayLength()-1)
	    *buffer++ = ' ' ;		// separate the elements
	 }
      }
   *buffer++ = ')' ;
   *buffer = '\0' ;			// ensure proper string termination
   return buffer ;
}

//----------------------------------------------------------------------

size_t FrSparseArray::displayLength() const
{
   size_t len = 8 ;			// for the #A()
   if (arrayLength() > 1)
      len += (arrayLength()-1) ;		// also count the blanks we add
   // iterate over the array fragments
   for (size_t i = 0 ; i < m_index_size ; ++i)
      {
      const FrArrayFrag *frag = items[i] ;
      if (!frag)
	 continue ;
      // now iterate over the elements of the fragment actually in use
      for (size_t pos = 0 ; pos < arrayLength() ; ++pos)
	 {
	 const FrArrayItem &itm = frag->items[pos] ;
	 FrInteger indexnum(itm.getIndex()) ;
	 FrObject *value = (FrObject*)itm.getValue() ;
	 len += indexnum.displayLength() + 3 ;
	 if (nonObjectArray())
	    {
	    len += Fr_sprintf(0,0,"%ld",((uintptr_t)value)) ;
	    }
	 else if (value)
	    len += value->displayLength() ;
	 else
	    len += 2 ;
	 }
      }
   return len ;
}

//----------------------------------------------------------------------

FrObject *FrSparseArray::insert(const FrObject *itm, size_t pos,
				bool copyitem)
{
   if (!items)
      {
      if (range_errors)
	 FrWarning(errmsg_bounds) ;
      return this ;			// can't insert!
      }
   if (!itm || !itm->consp())
      {
      // single item, so just hand off to addItem()
      addItem(pos,itm,FrArrAdd_REPLACE) ;
      return this ;
      }
   const FrList *itmlist = (const FrList*)itm ;
   if (!itmlist->rest())
      {
      // single item, so just hand off to addItem()
      addItem(pos,itmlist->first(),FrArrAdd_REPLACE) ;
      return this ;
      }
#if 1
   //FIXME: this code is much slower than generating the appropriate-sized gap,
   //  but still faster than before adding gaps to the array; it also doesn't take
   //  into account possible overlap of the auto-incremented index with existing items
   for ( ; itmlist ; ++pos )
      {
      addItem(pos,itmlist->first(),FrArrAdd_REPLACE) ;
      itmlist = itmlist->rest() ;
      }
   (void)copyitem ;			// keep compiler happy
#else
  //FIXME: need to do a proper insertion that takes possible overlap in
   pos = findIndexOrHigher(pos) ;
   size_t numitems ;
   numitems = ((FrList*)itm)->listlength() ;
   // indices into account
   expandTo(arrayLength()+numitems) ;
   m_length += numitems ;
   // make room for the new items
   if (arrayLength() > 0)
      {
      for (size_t i = arrayLength()-1 ; i >= pos+numitems  ; i--)
	 array[i] = array[i-numitems] ;
      }
   // and now copy them into the hole we just made
   if (numitems == 1)
      {
      array[pos] = (copyitem && itm) ? itm->deepcopy() : (FrObject*)itm ;
      }
   else
      {
      for (size_t i = pos ; i < pos+numitems ; ++i)
	 {
	 array[i] = ((FrList*)itm)->first() ;
	 itm = ((FrList*)itm)->rest() ;
	 }
      }
#endif /* 1 */
   return this ;
}

//----------------------------------------------------------------------

FrObject *FrSparseArray::elide(size_t start, size_t end)
{
   if (end < start)
      end = start ;
   start = findIndexOrHigher(start) ;
   end = findIndexOrHigher(end) ;
   size_t highestpos = highestPosition() ;
   if (start >= highestpos)
      {
      if (range_errors)
	 FrWarning(errmsg_bounds) ;
      return this ;			// can't elide that!
      }
   if (end >= highestpos)
      end = highestpos - 1 ;
   size_t elide_size = end-start+1 ;
   // remove the requested section of the array
   size_t i ;
   for (i = start ; i <= end ; i = nextItem(i))
      {
      FrArrayItem &itm = item(i) ;
      FrObject *value = itm.getValue() ;
      if (value)
	 {
	 value->freeObject() ;
	 itm.setValue(0) ;
         }
      }
//FIXME:
   for (i = start ; i < highestpos - elide_size ; ++i)
      item(i) = item(i+elide_size) ;
   // resize our storage downward, freeing any fragments which are no longer
   // in use
   size_t newlength = arrayLength() - elide_size ;
   size_t frags = array_frags(arrayLength()) ;
   size_t newfrags = array_frags(newlength) ;
   m_length = newlength ;
   for (i = frags ; i < newfrags ; ++i)
      {
      delete items[i] ;
      items[i] = 0 ;
      }
   return this ;
}

//----------------------------------------------------------------------

FrObject *FrSparseArray::subseq(size_t start, size_t stop) const
{
   if (stop < start)
      {
      stop = start ;
      }
   start = findIndexOrHigher(start) ;
   stop = findIndexOrHigher(stop) ;
   size_t highestpos = highestPosition() ;
   if (stop >= highestpos)
      {
      stop = highestpos - 1 ;
      }
   if (items && start < highestpos)
      {
      return new FrSparseArray(this,start,stop) ;
      }
   else
      {
      if (range_errors)
	 FrWarning(errmsg_bounds) ;
      return 0 ;			// can't take that subsequence!
      }
}

//----------------------------------------------------------------------

void FrSparseArray::pack()
{
   size_t dest = 0 ;
   // iterate over the fragments
   for (size_t i = 0 ; i < m_index_size ; ++i)
      {
      FrArrayFrag *frag = items[i] ;
      if (!frag)
	 continue ;
      // iterate over each item in the fragment which is actually in use
      for (size_t pos = 0 ; pos < m_fill_counts[i] ; ++pos)
	 {
         item(dest) = frag->items[pos] ;
	 ++dest ;
         }
      }
   // fix up the fill counts
   size_t num_frags = array_frags(dest) ;
   for (size_t i = 0 ; i+1 < num_frags ; ++i)
      {
      m_fill_counts[i] = FrARRAY_FRAG_SIZE ;
      }
   m_fill_counts[num_frags>0 ? num_frags-1 : 0] = dest % FrARRAY_FRAG_SIZE ;
   for (size_t i = num_frags ; i < m_index_size ; ++i)
      {
      m_fill_counts[i] = 0 ;
      // release fragments which are no longer needed
      delete items[i] ;
      items[i] = 0 ;
      }
   m_index_size = num_frags ;
   return ;
}

//----------------------------------------------------------------------

bool FrSparseArray::equal(const FrObject *obj) const
{
   if (obj == this)
      return true ;
   else if (!obj || !obj->arrayp())
      return false ;			// can't be equal to non-array
   else if (obj->objType() == OT_FrSparseArray)
      {
      FrSparseArray *arr = (FrSparseArray*)obj ;
      if (arrayLength() != arr->arrayLength())
	 return false ;
      FrSparseArrayIndexIter iter1(this) ;
      FrSparseArrayIndexIter iter2(arr) ;
      for (size_t i = 0 ; i < arrayLength() ; ++i)
	 {
	 FrArrayItem &itm1 = item(*iter1) ;
	 FrArrayItem &itm2 = arr->item(*iter2) ;
	 if (itm1.getIndex() != itm2.getIndex() ||
	     !equal_inline(itm1.getValue(),itm2.getValue()))
	    {
	    return false ;
	    }
	 ++iter1 ;
	 ++iter2 ;
	 if (iter1.atEnd() || iter2.atEnd())
	    break ;
	 }
      return true ;
      }
   else
      {
      return false ; //!!! for now
      }
}

//----------------------------------------------------------------------

int FrSparseArray::compare(const FrObject *obj) const
{
   if (!obj)
      return 1 ;			// anything is gt NIL / empty-list
   else if (!obj->arrayp())
      return -1 ;			// non-arrays come after arrays
   else if (obj->objType() == OT_FrSparseArray)
      {
      FrSparseArray *arr = (FrSparseArray*)obj ;
      size_t min_length = FrMin(arr->arrayLength(),arrayLength()) ;
      FrSparseArrayIndexIter iter1(this) ;
      FrSparseArrayIndexIter iter2(arr) ;
      for (size_t i = 0 ; i < min_length ; ++i)
	 {
	 FrArrayItem &itm1 = item(*iter1) ;
	 FrArrayItem &itm2 = arr->item(*iter2) ;
	 uintptr_t idx1 = itm1.getIndex() ;
	 uintptr_t idx2 = itm2.getIndex() ;
	 if (idx1 < idx2)		// first list has nonzero entry
	    return 1 ;			//    where second has NIL
	 else if (idx1 > idx2)		// second list has nonzero entry
	    return -1 ;			//    where first has NIL
	 int cmp = itm1.getValue()->compare(itm2.getValue()) ;
	 if (cmp)
	    return cmp ;
	 ++iter1 ;
	 ++iter2 ;
	 if (iter1.atEnd() || iter2.atEnd())
	    break ;
	 }
      // if we got here, the common portion is equal
      if (arrayLength() == arr->arrayLength())
	 return 0 ;
      else
	 return arrayLength() > arr->arrayLength() ? 1 : -1 ; // longer array comes later
      }
   else
      {
      return -1 ;			// non-arrays come after arrays
      }
}

//----------------------------------------------------------------------

FrObject *FrSparseArray::removeDuplicates() const
{
   FrSparseArray *result = new FrSparseArray(arrayLength()) ;
   if (result)
      {
      if (arrayLength() > 0 && items)
	 {
	 size_t j = 0 ;
	 size_t highestpos = highestPosition() ;
	 for (size_t i = 0 ; i < highestpos ; i = nextItem(i))
	    {
	    FrArrayItem &itm = item(i) ;
	    FrObject *value = itm.getValue() ;
	    if (result->locate(value) != (size_t)-1)
	       {
	       new (&result->item(j++))
		     FrArrayItem(itm.getIndex(),value ? value->deepcopy() : 0);
	       }
	    }
	 }
      }
   else
      {
      FrNoMemory("in FrSparseArray::removeDuplicates") ;
      }
   return result ;
}

//----------------------------------------------------------------------

FrObject *FrSparseArray::removeDuplicates(FrCompareFunc cmp) const
{
   FrSparseArray *result = new FrSparseArray(arrayLength()) ;
   if (result)
      {
      if (arrayLength() > 0 && items)
	 {
	 size_t j = 0 ;
	 size_t highestpos = highestPosition() ;
	 for (size_t i = 0 ; i < highestpos ; i = nextItem(i))
	    {
	    FrArrayItem &itm = item(i) ;
	    FrObject *value = itm.getValue() ;
	    if (result->locate(value,cmp) != (size_t)-1)
	       {
	       new (&result->item(j++))
		   FrArrayItem(itm.getIndex(),value ? value->deepcopy() : 0);
	       }
	    }
	 }
      }
   else
      {
      FrNoMemory("in FrSparseArray::removeDuplicates") ;
      }
   return result ;
}

/************************************************************************/
/************************************************************************/

static FrObject *read_FrSparseArray(istream &input, const char *)
{
   FrObject *initializer = read_FrObject(input) ;
   FrArray *array ;
   if (initializer && initializer->consp())
      array = new FrSparseArray((FrList*)initializer) ;
   else
      array = new FrSparseArray ;
   free_object(initializer) ;
   return array ;
}

//----------------------------------------------------------------------

static FrObject *string_to_FrSparseArray(const char *&input, const char *)
{
   FrObject *initializer = string_to_FrObject(input) ;
   FrArray *array ;
   if (initializer && initializer->consp())
      array = new FrSparseArray((FrList*)initializer) ;
   else
      array = new FrSparseArray ;
   free_object(initializer) ;
   return array ;
}

//----------------------------------------------------------------------

static bool verify_FrSparseArray(const char *&input, const char *,
				   bool strict)
{
   return valid_FrObject_string(input,strict) ;
}

//----------------------------------------------------------------------

// end of file frsarray.cpp //
