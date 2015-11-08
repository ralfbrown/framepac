/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frmembin.h		template FrMemoryBinnedFreelist		*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 2009,2010,2013 Ralf Brown/Carnegie Mellon University	*/
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

#ifndef __FRMEMBIN_H_INCLUDED
#define __FRMEMBIN_H_INCLUDED

/************************************************************************/
/*	Methods for template class FrMemoryBinnedFreelist		*/
/************************************************************************/

template <class T_hdr, unsigned N_factor>
FrMemoryBinnedFreelist<T_hdr,N_factor>::FrMemoryBinnedFreelist(
   size_t minbinsize,
   size_t maxbinsize,
   size_t gran)
   : m_mutex(true,true)
{
   // the default memory pool might get forcibly initialized before its
   //   constructor has a chance to run, so don't run the initialization
   //   again if that happens
   if (((void*)this != (void*)&FramepaC_mempool
#if defined(FrREPLACE_MALLOC) && !defined(PURIFY)
	&& (void*)this != (void*)&FramepaC_bigalloc_pool
#endif
	  )
       || !initialized())
      {
      m_self = 0 ;
      init(minbinsize,maxbinsize,gran) ;
      }
   return ;
}

//----------------------------------------------------------------------

template <class T_hdr, unsigned N_factor>
FrMemoryBinnedFreelist<T_hdr,N_factor>::~FrMemoryBinnedFreelist()
{ 
   m_self = 0 ; 
   if (0
#if defined(FrREPLACE_MALLOC) && !defined(PURIFY)
       || (void*)this == (void*)&FramepaC_bigalloc_pool
#endif
//       || (void*)this == (void*)&FramepaC_mempool
      )
      {
      VALGRIND_DESTROY_MEMPOOL(this) ;
      }
   return ;
}

//----------------------------------------------------------------------

template <class T_hdr, unsigned N_factor>
void FrMemoryBinnedFreelist<T_hdr,N_factor>::init(size_t minbin, size_t maxbin,
						  size_t gran)
{
   if (m_self != this)
      m_mutex.init(true) ;
   initBinSizes(minbin,maxbin,gran) ;
   // the free list is a circular doubly-linked list with special header node,
   //   so the empty free list is the header pointing at itself in both dirs
   m_freelist.initEmpty() ;
   for (size_t i = 0 ; i <= maximumBin() ; i++)
      {
      m_freelist_ptrs[i] = &m_freelist ;
      m_index[i] = maximumBin() ;
      }
   for (size_t i = maximumBin() + 1 ; i < lengthof(m_freelist_ptrs) ; i++)
      m_freelist_ptrs[i] = 0 ;
   m_self = this ;
#if defined(FrREPLACE_MALLOC) && !defined(PURIFY)
   if ((void*)this == (void*)&FramepaC_bigalloc_pool)
      {
      VALGRIND_CREATE_MEMPOOL(this,0,0) ;
      }
   else
#endif
   if ((void*)this == (void*)&FramepaC_mempool)
      {
      // small mallocs have four bytes of admin info between them, so
      //   split them into a pair of two-byte redzones
      VALGRIND_CREATE_MEMPOOL(&FramepaC_mempool,2,0) ;
      }
   return ;
}

//----------------------------------------------------------------------

template <class T_hdr, unsigned N_factor>
void FrMemoryBinnedFreelist<T_hdr,N_factor>::initBinSizes(size_t minbin,
							  size_t maxbin,
							  size_t gran)
{
   s_N_factor_shift = 0 ;
   for (size_t N = N_factor - 1 ; N > 1 ; N >>= 1)
      {
      s_N_factor_shift += 1 ;
      }
   if (maximumBin() == 0)
      {
      setGranularity(gran) ;
      setBinSize(0,minbin) ;
      setMaximumBin(lengthof(m_freelist_ptrs)-1) ;
      size_t max = bin(maxbin) ;
      if (max >= numBins())
	 max = numBins()-1 ;
      setMaximumBin(max) ;
      size_t incr = gran ;
      size_t size = minbin ;
      unsigned count = 0 ;
      for (size_t i = 0 ; i <= maximumBin() ; i++)
	 {
	 if (count >= N_factor)
	    {
	    incr <<= 1 ;
	    count >>= 1 ;
	    }
	 count++ ;
	 setBinSize(i,size) ;
	 size += incr ;
	 if (binsize(i) > maxbin)
	    setBinSize(i,maxbin) ;
	 }
      if (maxbin > MAX_SMALL_BLOCK)
	 maxbin = ULONG_MAX ;
      else
	 maxbin = MAX_SMALL_BLOCK & ~(gran - 1) ;
      setBinSize(maximumBin()+1,maxbin) ;
      }
   return ;
}

//----------------------------------------------------------------------

template <class T_hdr, unsigned N_factor>
size_t FrMemoryBinnedFreelist<T_hdr,N_factor>::bin(size_t size)
{
   if (size <= binsize(0))
      return 0 ;
   size -= binsize(0) ;
   size >>= granularityShift() ;	// all blocks are multiples of this size
#if 0
   size_t sz = size >> m_N_factor_shift ;
   // log2 adapted from Bit Twiddling Hacks: http://graphics.stanford.edu/~seander/bithacks.html
   size_t log2 = 0, shift ;
   if (sizeof(T_hdr) != sizeof(FrFreeHdr) || sizeof(FrSubAllocOffset) > 2)
      {
      shift = (sz>0xFFFFFFFF)<<5; sz>>=shift; log2=shift ;
      shift = (sz>0xFFFF)<<4; sz>>=shift; log2|=shift ;
      }
   shift = (sz>0xFF)<<3; sz>>=shift; log2|=shift;
   shift = (sz>0x0F)<<2; sz>>=shift; log2|=shift;
   shift = (sz>0x03)<<1; sz>>=shift; log2|=shift;
   log2 |= (sz>>1);
   size_t b = (log2 * (N_factor>>1)) + (size >> log2) ;
#else
   size_t b = 0 ;
   // allow for 2**k distinct block sizes per power of two (may waste a
   //   little memory, but considerably speeds up the maintenance of the
   //   freelist pointers)
   for ( ; size >= N_factor ; size >>= 1)
      {
      b += (N_factor >> 1) ;
      }
   b += size ;
#endif
   return (b > maximumBin()) ? maximumBin() : b ;
}

//----------------------------------------------------------------------

template <class T_hdr, unsigned N_factor>
void FrMemoryBinnedFreelist<T_hdr,N_factor>::splitblock(size_t totalsize,
							size_t &desired)
{
   // round up the desired size to the proper bin size
   size_t b = bin(desired) ;
   size_t size = binsize(b) ;
   if (size < desired)
      size = binsize(b+1) ;
   if (size >= desired && totalsize >= size && totalsize - size >= binsize(0))
      {
      desired = size ;
      }
   else
      {
      // not enough to split off a fragment, so use the whole block
      desired = totalsize ;
      }
   return ;
}

//----------------------------------------------------------------------

template <class T_hdr, unsigned N_factor>
T_hdr *FrMemoryBinnedFreelist<T_hdr,N_factor>::pop(size_t size)
{
   size_t b = bin(size) ;
   if (binsize(b) < size)
      {
      if (b >= maximumBin())
	 {
	 // we'll have to scan for a sufficiently large block, as we can't
	 //   guarantee that the first one on the list is big enough
	 T_hdr *block = m_freelist_ptrs[m_index[b]] ;
	 while (block != &m_freelist)
	    {
	    if (blockSize(block) >= size)
	       {
	       remove(block,b) ;
	       return block ;
	       }
	    block = block->nextFree() ;
	    }
	 return 0 ;			// no sufficiently large block available
	 }
      else
	 b++ ;
      }
   size_t idx = m_index[b] ;
   T_hdr *block = m_freelist_ptrs[idx] ;
   if (block == &m_freelist)
      block = 0 ;
   else
      remove(block,b) ;
   return block ;
}

//----------------------------------------------------------------------

template <class T_hdr, unsigned N_factor>
void FrMemoryBinnedFreelist<T_hdr,N_factor>::add(T_hdr *block)
{
   size_t b = blockBin(block) ;
   size_t idx = m_index[b] ;
   T_hdr *next = m_freelist_ptrs[idx] ;
   T_hdr *prev = next->prevFree() ;
   // insert block into doubly-linked freelist
   block->setNextFree(next) ;
   block->setPrevFree(prev) ;
   next->setPrevFree(block) ;
   prev->setNextFree(block) ;
   // fix up the freelist pointers
   size_t first_bin = (prev == &m_freelist) ? 0 : blockBin(prev) + 1 ;
   m_freelist_ptrs[b] = block ;
   for (size_t i = first_bin ; i <= b ; i++)
      m_index[i] = b ;
   markAsFree(block) ;
   return ;
}

//----------------------------------------------------------------------
// Prerequisite: caller must have acquired mutex(); block is free or belongs to caller

template <class T_hdr, unsigned N_factor>
void FrMemoryBinnedFreelist<T_hdr,N_factor>::remove(T_hdr *block,
						    size_t bin_num)
{
   // cut the block out of the freelist
   T_hdr *next = block->nextFree() ;
   T_hdr *prev = block->prevFree() ;
   size_t idx = m_index[bin_num] ;
   prev->setNextFree(next) ;
   next->setPrevFree(prev) ;
   if (m_freelist_ptrs[idx] == block)
      {
      // fix up the freelist pointers
      size_t next_bin =
	 (next == &m_freelist) ? maximumBin() : blockBin(next) ;
      m_freelist_ptrs[next_bin] = next ;
      if (next_bin != bin_num)
	 {
	 idx = (prev == &m_freelist) ? 0 : blockBin(prev)+1 ;
	 for (size_t i = idx ; i <= next_bin ; i++)
	    m_index[i] = next_bin ;
	 }
      }
   return ;
}

//----------------------------------------------------------------------
// Prerequisite: block belongs to caller

template <class T_hdr, unsigned N_factor>
void FrMemoryBinnedFreelist<T_hdr,N_factor>::remove(T_hdr *block)
{
   size_t bin_number = blockBin(block) ;
   remove((T_hdr*)block,bin_number) ;
   return ;
}

//----------------------------------------------------------------------

template <class T_hdr, unsigned N_factor>
size_t FrMemoryBinnedFreelist<T_hdr,N_factor>::removeIfFree(T_hdr *block)
{
   size_t block_size = 0 ;
   if (!blockInUse(block))
      {
      block_size = blockSize(block) ;
      size_t bin_number = bin(block_size) ;
      remove((T_hdr*)block,bin_number) ;
      }
   return block_size ;
}

//----------------------------------------------------------------------

template <class T_hdr, unsigned N_factor>
bool FrMemoryBinnedFreelist<T_hdr,N_factor>::check_idx() const
{
   const uint16_t *idx = m_index ;
   T_hdr * const *ptrs = m_freelist_ptrs ;
   const T_hdr *list = &m_freelist ;
   for (size_t i = 1 ; i <= maximumBin() ; i++)
      {
      if (!(idx[i] == i || (idx[i] > i && idx[i-1] != i)))
	 return false ;
      }
   for (size_t i = 0 ; i <= maximumBin() ; i++)
      {
      if (ptrs[idx[i]] != list)
	 {
	 size_t b = blockBin(ptrs[idx[i]]) ;
	 if (!(b == idx[i]))
	    return false ;
	 }
      }
   return true ;
}

//----------------------------------------------------------------------

template <class T_hdr, unsigned N_factor>
bool FrMemoryBinnedFreelist<T_hdr,N_factor>::containsDuplicates() const
{
   for (T_hdr *free = m_freelist.next ;
	free && free != &m_freelist ;
	free = free->next)
      {
      for (T_hdr *other = free->next ;
	   other && other != &m_freelist ;
	   other = other->next)
	 {
	 if (free == other)
	    return true ;
	 }
      }
   return false ;
}

//----------------------------------------------------------------------

#endif /* !__FRMEMBIN_H_INCLUDED */

// end of file frmembin.h //
