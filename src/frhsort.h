/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC								*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frhsort.h		templatized Smoothsort var. of Heapsort */
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 2010,2015 Ralf Brown/Carnegie Mellon University	*/
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

#ifndef __FRHSORT_H_INCLUDED
#define __FRHSORT_H_INCLUDED

#include <stdint.h>
#include <stdlib.h>

#include <iostream>
#include <stdio.h>
using namespace std ;

/************************************************************************/
/*	Global data							*/
/************************************************************************/

extern const size_t Fr_leonardo_numbers[] ;

/************************************************************************/
/*	Forward declarations						*/
/************************************************************************/


/************************************************************************/
/*	Helper class to track the sub-heap sizes			*/
/************************************************************************/

// implement the idea of keeping a shifted bitmask as shown in the
//   Wikipedia article (http://en.wikipedia.org/wiki/Smoothsort)

class FrSmoothSortHeapInfo
   {
   private:
      uint64_t m_sizes ;
      unsigned m_shift ;
   public:
      FrSmoothSortHeapInfo() { m_sizes = 1 ; m_shift = 1 ; }
      FrSmoothSortHeapInfo(const FrSmoothSortHeapInfo &orig)
	 { m_sizes = orig.m_sizes >> 1 ; m_shift = orig.m_shift + 1 ; }
      FrSmoothSortHeapInfo(const FrSmoothSortHeapInfo *orig)
	 { m_sizes = orig->m_sizes ; m_shift = orig->m_shift ; }
      ~FrSmoothSortHeapInfo() {}

      // manipulators
      void shiftRight(unsigned N) { m_sizes >>= N ; m_shift += N ; }
      void shiftLeft(unsigned N) { m_sizes <<= N ; m_shift -= N ; }
      void shiftLeft()
	 { unsigned shft = m_shift ; if (shft > 1) shft-- ; shiftLeft(shft) ; }
      void shiftOutTrailing() { shiftRight(trailingZeros(m_sizes & ~1)) ; }
      void splitBlock() { m_sizes = (m_sizes << 2) ^ 0x07 ; m_shift -= 2 ; }
      void addBlock() { m_sizes |= 1 ; }

      // accessors
      unsigned shift() const { return m_shift ; }
      uint64_t sizes() const { return m_sizes ; }
      bool consecutive() const { return (m_sizes & 3) == 3 ; }
      bool singleBlock() const { return m_shift == 1 && m_sizes == 1 ; }

      static size_t rightChild(size_t root) { return root - 1 ; }
      static size_t leftChild(size_t root, size_t shift) { return root - 1 - Fr_leonardo_numbers[shift-2] ; }
      size_t leftChild(size_t root) { return root - 1 - Fr_leonardo_numbers[shift() - 2] ; }
      size_t stepSon(size_t root) { return root - Fr_leonardo_numbers[shift()] ; }
      static unsigned trailingZeros(uint64_t value) ;

      template <class T> static void rebalance(T elts[], unsigned shift,
					       size_t root)
	 {
	    // work our way down until we get to a size-1 subheap
	    while (shift > 1)
	       {
	       size_t right = rightChild(root) ;
	       size_t left = leftChild(root,shift) ;
	       size_t bigchild ;
	       if (T::compare(elts[left],elts[right]) >= 0)
		  {
		  bigchild = left ;
		  shift-- ;		// left child is of size-1
		  }
	       else // (T::compare(elts[left],elts[right]) < 0)
		  {
		  bigchild = right ;
		  shift -= 2 ;		// right child is of size-2
		  }
	       // if the root is larger than the larger child, the
	       //   heap invariant has been restored and we're done
	       if (T::compare(elts[root],elts[bigchild]) >= 0)
		  {
		  return ;
		  }
	       // swap root with larger child
	       T rootval = elts[root] ;
	       elts[root] = elts[bigchild] ;
	       elts[bigchild] = rootval ;
	       // descend down into the bigger child's subheap
	       root = bigchild ;
	       }
	    return ;
	 }
      template <class T> static void rebalance(T* elts[], unsigned shift,
					       size_t root)
	 {
	    // work our way down until we get to a size-1 subheap
	    while (shift > 1)
	       {
	       size_t right = rightChild(root) ;
	       size_t left = leftChild(root,shift) ;
	       size_t bigchild ;
	       if (T::compare(elts[left],elts[right]) >= 0)
		  {
		  bigchild = left ;
		  shift-- ;		// left child is of size-1
		  }
	       else // (T::compare(elts[left],elts[right]) < 0)
		  {
		  bigchild = right ;
		  shift -= 2 ;		// right child is of size-2
		  }
	       // if the root is larger than the larger child, the
	       //   heap invariant has been restored and we're done
	       if (T::compare(elts[root],elts[bigchild]) >= 0)
		  {
		  return ;
		  }
	       // swap root with larger child
	       T rootval = elts[root] ;
	       elts[root] = elts[bigchild] ;
	       elts[bigchild] = rootval ;
	       // descend down into the bigger child's subheap
	       root = bigchild ;
	       }
	    return ;
	 }

      template <class T> void rectify(T elts[], size_t root)
	 {
//cerr<<"rectify "<<root<<endl;
	    // move the root into the correct heap to ensure that roots are
	    //   in ascending order
	    size_t last_shift ;
	    FrSmoothSortHeapInfo info(this) ;
	    while (true)
	       {
	       last_shift = info.shift() ;
//cerr<<"   size "<<last_shift<<", heapsizes="<<info.sizes()<<endl;
	       // is this the first (largest) heap?
	       if (Fr_leonardo_numbers[last_shift]-1 >= root)
		  {
//cerr<<"   bail out, at first heap"<<endl;
		  break ;
		  }
	       size_t largest = root ;
	       if (last_shift > 1)
		  {
		  // we're the root of a tree with children, so see which of
		  //   the two children and the new element are the largest
		  size_t left = info.leftChild(root) ;
		  if (T::compare(elts[left],elts[root]) > 0)
		     {
		     largest = left ;
		     }
		  size_t right = info.rightChild(root) ;
		  if (T::compare(elts[right],elts[largest]) > 0)
		     {
		     largest = right ;
		     }
		  }
	       size_t stepson = info.stepSon(root) ;
	       if ((stepson+1) == 0)
		  {
//cerr<<"  bail out, no pred heap" <<endl ;
		  break ;
		  }
	       if (T::compare(elts[largest],elts[stepson]) >= 0)
		  {
//cerr<<"  bail out, in position"<<endl;
		  break ;
		  }
	       // swap current and prior element
//cerr<<"   swapping("<<root<<","<<stepson<<") "<<(uintptr_t)(elts[root].word)<<" & "<<(uintptr_t)(elts[stepson].word)<<endl;
	       T tmpval = elts[root] ;
	       elts[root] = elts[stepson] ;
	       elts[stepson] = tmpval ;
	       // find the size of the prior heap
	       root = stepson ;
	       info.shiftOutTrailing() ;
	       }
	    // finally, rebalance the heap in which the new element landed
	    rebalance(elts, last_shift, root) ;
//cerr<<"rectify done"<<endl;
	    return ;
	 }
      template <class T> void rectify(T* elts[], size_t root)
	 {
	    // move the root into the correct heap to ensure that roots are
	    //   in ascending order
	    size_t last_shift ;
	    FrSmoothSortHeapInfo info(this) ;
	    while (true)
	       {
	       last_shift = info.shift() ;
	       // is this the first (largest) heap?
	       if (Fr_leonardo_numbers[last_shift]-1 >= root)
		  {
		  break ;
		  }
	       size_t largest = root ;
	       if (last_shift > 1)
		  {
		  // we're the root of a tree with children, so see which of
		  //   the two children and the new element are the largest
		  size_t left = info.leftChild(root) ;
		  if (T::compare(elts[left],elts[root]) > 0)
		     {
		     largest = left ;
		     }
		  size_t right = info.rightChild(root) ;
		  if (T::compare(elts[right],elts[largest]) > 0)
		     {
		     largest = right ;
		     }
		  }
	       size_t stepson = info.stepSon(root) ;
	       if ((stepson+1) == 0)
		  {
		  break ;
		  }
	       if (T::compare(elts[largest],elts[stepson]) >= 0)
		  {
		  break ;
		  }
	       // swap current and prior element
	       T tmpval = elts[root] ;
	       elts[root] = elts[stepson] ;
	       elts[stepson] = tmpval ;
	       // find the size of the prior heap
	       root = stepson ;
	       info.shiftOutTrailing() ;
	       }
	    // finally, rebalance the heap in which the new element landed
	    rebalance(elts, last_shift, root) ;
	    return ;
	 }

      template <class T> void add(T elts[], size_t root, size_t num_elts)
	 {
	    if (consecutive())
	       {
	       shiftRight(2) ;
	       }
	    else if (shift() == 1) // have L(1) but not L(0)
	       {
	       shiftLeft(1) ;
	       }
	    else
	       {
	       shiftLeft(shift()-1) ;
	       }
	    addBlock() ;
	    // is the new tree we just created at its final size?
	    bool last ;
	    if (shift() == 0) // we have L(0)
	       {
	       last = (root + 1 == num_elts) ;
	       }
	    else if (shift() == 1) // we have L(1) but not L(0)
	       {
	       last = (root + 1 == num_elts ||
		       (root + 2 == num_elts && (m_sizes & 2) == 0)) ;
	       }
	    else
	       {
	       last = (num_elts - root - 1) < Fr_leonardo_numbers[shift()-1] + 1 ;
	       }
	    if (last)
	       {
	       rectify(elts, root) ;
	       }
	    else
	       {
	       rebalance(elts, shift(), root) ;
	       }
	    return ;
	 }
      template <class T> void add(T* elts[], size_t root, size_t num_elts)
	 {
	    if (consecutive())
	       {
	       shiftRight(2) ;
	       }
	    else if (shift() == 1) // have L(1) but not L(0)
	       {
	       shiftLeft(1) ;
	       }
	    else
	       {
	       shiftLeft(shift()-1) ;
	       }
	    addBlock() ;
	    // is the new tree we just created at its final size?
	    bool last ;
	    if (shift() == 0) // we have L(0)
	       {
	       last = (root + 1 == num_elts) ;
	       }
	    else if (shift() == 1) // we have L(1) but not L(0)
	       {
	       last = (root + 1 == num_elts ||
		       (root + 2 == num_elts && (m_sizes & 2) == 0)) ;
	       }
	    else
	       {
	       last = (num_elts - root - 1) < Fr_leonardo_numbers[shift()-1] + 1 ;
	       }
	    if (last)
	       {
	       rectify(elts, root) ;
	       }
	    else
	       {
	       rebalance(elts, shift(), root) ;
	       }
	    return ;
	 }
   } ;

/************************************************************************/
/*	Object-array version of SmoothSort				*/
/************************************************************************/

//    requires:  static int T::compare(const T&, const T&) ;
//		 static T &operator = (const T&) ;
template <class T> void FrSmoothSort(T elts[], size_t num_elts)
{
   if (num_elts < 2)
      return ;				// trivially sorted
   // we implicitly use up the first element, since the default heapinfo ctor
   //   generates a single heap of size 1
   size_t head = 1 ;
   FrSmoothSortHeapInfo heapinfo ;
   // first phase: heapify
   for ( ; head < num_elts ; head++)
      {
      heapinfo.add(elts,head,num_elts) ;
      }
   heapinfo.rectify(elts, num_elts - 1) ;
   head = num_elts - 1 ;
   // second phase: extract largest elements while maintaining heap invariant
   while (!heapinfo.singleBlock())
      {
      // "extract" largest element by simply shrinking the heap by one
      --head ;
      if (heapinfo.shift() <= 1)
	 {
	 // the smallest subheap was size one, so just update the heap info
	 heapinfo.shiftOutTrailing() ;
	 }
      else
	 {
	 // split the last heap to expose its subheaps
	 heapinfo.splitBlock() ;
	 // rebalance the subheaps
	 FrSmoothSortHeapInfo subheap(heapinfo) ;
	 subheap.rectify(elts, heapinfo.stepSon(head)) ;
	 heapinfo.rectify(elts, head) ;
	 }
      }
   return ;
}

/************************************************************************/
/*	Pointer-array version of SmoothSort				*/
/************************************************************************/

//    requires:  static int T::compare(const T*, const T*) ;
template <class T> void FrSmoothSort(T* elts[], size_t num_elts)
{
   if (num_elts < 2)
      return ;				// trivially sorted
   // we implicitly use up the first element, since the default heapinfo ctor
   //   generates a single heap of size 1
   size_t head = 1 ;
   FrSmoothSortHeapInfo heapinfo ;
   // first phase: heapify
   for ( ; head < num_elts ; head++)
      {
      heapinfo.add(elts,head,num_elts) ;
      }
   heapinfo.rectify(elts, head-1, false) ;
   // second phase: extract largest elements while maintaining heap invariant
   while (!heapinfo.singleBlock())
      {
      // "extract" largest element by simply shrinking the heap by one
      --head ;
      if (heapinfo.shift() <= 1)
	 {
	 // the smallest subheap was size one, so just update the heap info
	 heapinfo.shiftOutTrailing() ;
	 }
      else
	 {
	 // split the last heap to expose its subheaps
	 heapinfo.splitBlock() ;
	 // rebalance the subheaps
	 FrSmoothSortHeapInfo subheap(heapinfo) ;
	 subheap.rectify(elts, heapinfo.stepSon(head), true) ;
	 heapinfo.rectify(elts, head, true) ;
	 }
      }
   return ;
}

//----------------------------------------------------------------------

#endif /* !__FRHSORT_H_INCLUDED */

/* end of file frhsort.h */
