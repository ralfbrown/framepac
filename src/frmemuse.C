/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frmemuse.cpp		memory allocation statistics		*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,1998,2000,2001,2004,2006,2007,	*/
/*		2009,2010,2013 Ralf Brown/Carnegie Mellon University	*/
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

#include "fr_mem.h"
#include "frballoc.h"
#include "frpcglbl.h"

#ifdef FrSTRICT_CPLUSPLUS
#  include <iomanip>
#else
#  include <iomanip.h>
#endif

/************************************************************************/
/*	 Global Data for this module					*/
/************************************************************************/

#define AUTO_ALLOC (MAX_AUTO_SUBALLOC+1)
#define BIN_SIZE ((MAX_SMALL_ALLOC - MAX_AUTO_SUBALLOC) / (3*FrMALLOC_BINS))

static const char blank[] = " " ;

extern size_t FrSymbolTable_blocks ;

/************************************************************************/
/*    Memory statistics							*/
/************************************************************************/

static size_t count_blocks(const FrMemFooter *blocks)
{
   size_t count = 0 ;

   while (blocks)
      {
      count++ ;
      blocks = blocks->next() ;
      }
   return count ;
}

//----------------------------------------------------------------------

static void print_blocks(ostream &out, const char *type, int blocks,
			 int free = -1, size_t total_requests = 0)
{
   size_t len = type ? strlen(type) : 0 ;

   out << type << setw(19-len) << blank << setw(6) << blocks
       << setw(14) << (unsigned long)blocks*FrFOOTER_OFS ;  //FIXME: only approximate
   if (free >= 0)
      {
      out << setw(13) << blank ;      // filler for now (in-use field)
      out << setw(15) << free ;
      }
#ifdef FrMEMUSE_STATS
   else
      out << setw(28) << blank << setw(0) ;  // filler
   if (total_requests > 0)
      out << setw(12) << total_requests ;
#endif /* FrMEMUSE_STATS */
   (void)total_requests ;  // keep compiler happy
   out << endl ;
   return ;
}

//----------------------------------------------------------------------

static void print_memory_pools(ostream &out, size_t &total_blocks)
{
   for (const FrMemoryPool *pool = FramepaC_memory_pools ;
	pool ;
	pool = pool->nextPool())
      {
      if (pool == &FramepaC_mempool)
	 continue ;
      const char *type = pool->typeName() ;
      int len = type ? strlen(type) : 0 ;
      int blocks = FrMemoryPool::numPages() ;
      total_blocks += blocks ;
      long bytes = pool->bytesAllocated() ;

      out << type << setw(19-len) << blank << setw(6) << blocks ;
      out << setw(14) << bytes << endl ;
      }
   return ;
}

//----------------------------------------------------------------------

static void print_big_blocks(ostream &out, const char *name, size_t &blocks,
			     size_t &bytes)
{
   blocks = 0 ;
   bytes = 0 ;
#ifdef FrREPLACE_MALLOC
   int free_blocks = 0 ;
   long int free_bytes = 0 ;
   FrBigAllocHdr *head = FramepaC_bigalloc_pool.freelistHead() ;
   for (FrBigAllocHdr *block = head->next() ;
	block && block != head ;
	block = block->next())
      if (block->suballocated == false)
	 {
	 blocks++ ;
	 bytes += block->size() ;
	 if (block->status != Block_In_Use)
	    {
	    free_blocks++ ;
	    free_bytes += block->size() ;
	    }
	 }
   if (blocks > 0)
      {
      int len = name ? strlen(name) : 0 ;
      out << name << setw(19-len) << blank << setw(6) << blocks
	  << setw(14) << bytes
	  << setw(19) << free_blocks
	  << setw(11) << free_bytes
	  << endl ;
      }
#else
   (void)out ; (void)name ;
#endif /* FrREPLACE_MALLOC */
   return ;
}

//----------------------------------------------------------------------

static uint64_t sum_of_requests(const FrSlabPool *slab)
{
   uint64_t sum = 0 ;
   for (FrAllocator *alloc = FramepaC_allocators ; alloc ; alloc = alloc->nextAllocator())
      {
      if (alloc->objectSize() == slab->objectSize() && !alloc->mallocHeader())
	 sum += alloc->requests_made() ;
      }
   return sum ;
}

//----------------------------------------------------------------------

static void print_allocator_blocks(ostream &out,size_t &total_block_count,
				   bool verbose)
{
   for (size_t i = 0 ; i < FrTOTAL_ALLOC_LISTS ; i++)
      {
      FrSlabPool *slab = FrAllocator::slabpool(i) ;
      size_t objsize = slab->objectSize() ;
      if (objsize == 0)
	 continue ;		     // never allocated from this slab
      size_t total_blocks = slab->pageCount() ;
      uint64_t requests = 0 ;
      if (slab->mallocHeader())
	 {
	 requests = FrMalloc_requests[i - FrFIRST_SUBALLOC_LIST] ;
	 }
      else
	 {
	 requests = sum_of_requests(slab) ;
	 }
      if (total_blocks == 0 && requests == 0)
	 continue ;
      if (total_blocks == 0 && !verbose)
	 continue ;
      FrMemFooter *pages = FrAllocator::pageList(i) ;
      FrFreeList *freelist = FrAllocator::freelist(i) ;
      size_t total_bytes = total_blocks * (FrFOOTER_OFS - 48) ; // approximate!
      size_t total_objs = total_blocks * ((FrFOOTER_OFS - 64) / objsize) ; // VERY approximate!
      size_t free = freelist->listlength() ;
      size_t local_blocks = 0 ;
      if (pages)
	 {
	 size_t local_bytes = 0 ;
	 total_objs = 0 ;
	 for ( ; pages ; pages = pages->myNext())
	    {
	    local_blocks++ ;
	    local_bytes += pages->totalBytes() ;
	    total_objs += pages->totalObjects(objsize) ;
	    }
	 if (local_blocks == total_blocks)
	    {
	    // we have a complete picture of the allocations, so we
	    //   can update the earlier estimate
	    total_bytes = local_bytes ;
	    }
	 }
      total_block_count += total_blocks ;
      const char *name = slab->typeName() ;
      if (!name || !*name)
	 name = "{none}" ;
      size_t len = strlen(name) ;
      size_t in_use = (free <= total_objs) ? total_objs - free : 0 ;
      out << name << setw(19-len) << blank
	  << setw(6) << total_blocks ;
      out << (total_blocks == local_blocks ? ' ' : '*') ;
      out << setw(13) << total_bytes ;
      out << setw(13) << in_use
	  << setw(15) << free ;
      if (requests > 0 || verbose)
	 out << setw(12) << requests ;
      out << endl  ;
      }
   return ;
}

//----------------------------------------------------------------------

static size_t malloc_free_bins[FrMALLOC_BINS] ;
static size_t malloc_free_size[FrMALLOC_BINS] ;
static size_t malloc_used_bins[FrMALLOC_BINS] ;
static size_t malloc_used_size[FrMALLOC_BINS] ;

//----------------------------------------------------------------------

static int malloc_bin_number(int size)
{
   int base = round_up(AUTO_ALLOC,FrALIGN_SIZE) ;
   if (size <= base)
      return 0 ;
   int bin = (size - base) / BIN_SIZE ;
   if (bin > FrMALLOC_BINS-1)
      bin = FrMALLOC_BINS-1 ;
   return bin ;
}

//----------------------------------------------------------------------

static void scan_malloc_blocks(const FrMemFooter *pages)
{
   for ( ; pages ; pages = pages->next())
      {
      const char *blk = pages->objectStartPtr() ;
      int size ;
      while ((size = HDR(blk).size & FRFREEHDR_SIZE_MASK) != 0)
	 {
	 int bin = malloc_bin_number(size) ;
	 if ((HDR(blk).size & FRFREEHDR_USED_BIT) != 0)
	    {
	    malloc_used_bins[bin]++ ;
	    malloc_used_size[bin] += size ;
	    }
	 else
	    {
	    malloc_free_bins[bin]++ ;
	    malloc_free_size[bin] += size ;
	    }
	 blk += size ;
	 }	
      }
   return  ;
}

//----------------------------------------------------------------------

static void scan_malloc_blocks(FrMemoryPool *mpi)
{
   for (int i = 0 ; i < FrMALLOC_BINS ; i++)
      {
      malloc_free_bins[i] = 0 ;
      malloc_free_size[i] = 0 ;
      malloc_used_bins[i] = 0 ;
      malloc_used_size[i] = 0 ;
      }
   scan_malloc_blocks(mpi->localMallocPageList()) ;
//   scan_malloc_blocks(mpi->globalMallocPageList()) ;
   return ;
}

//----------------------------------------------------------------------

void FrMemoryStats(ostream &out, bool verbose)
{
   out << endl
       << "Memory Usage\n"
       << "============"
       << setw(14) << "Blocks" << setw(13) << "Bytes" << setw(14) << "Objs InUse"
       << setw(14) << "Objs Avail" << setw(12) << "Requests"
       << setw(0) << endl ;
   size_t i = count_blocks(FrMemoryPool::localMallocPageList()) ;
   size_t total_blocks = i ;
   print_blocks(out,"Symbols",FramepaC_symbol_blocks,-1) ;
   total_blocks += FramepaC_symbol_blocks ;
   print_blocks(out,"SymbolTable Frags",FrSymbolTable_blocks,-1) ;
   total_blocks += FrSymbolTable_blocks ;
   print_allocator_blocks(out,total_blocks,verbose) ;
   print_blocks(out,"FrMalloc",i,-1,FrMalloc_requests[FrAUTO_SUBALLOC_BINS+1]);
   print_memory_pools(out,total_blocks) ;
   i = FrMemoryPool::numFreePages() ;
   if (i)
      print_blocks(out,"Unused",i) ;
   total_blocks += i ;
   size_t big_blocks ;
   size_t big_bytes ;
   print_big_blocks(out,"Large Allocations",big_blocks,big_bytes) ;
   if (FramepaC_num_memmaps > 0)
      out << "Mapped Memory" << setw(45) << FramepaC_num_memmaps
	  << setw(11) << FramepaC_total_memmap_size << endl ;
   out << "============" << endl ;
   out << "   Total" << setw(17) << total_blocks+big_blocks << setw(14)
       << total_blocks*(FrFOOTER_OFS-48) + big_bytes << endl ;  //FIXME: only approximate
   out << endl ;
   if (!FramepaC_memory_chain_OK() ||
       !check_FrMalloc(&FramepaC_mempool))
      {
      out << "FrMalloc memory corrupted!" << endl << endl ;
      return ;
      }
   scan_malloc_blocks(&FramepaC_mempool) ;
   size_t total = 0 ;
   out << "Malloc blocks:" ;
   for (i = 0 ; i < FrMALLOC_BINS ; i++)
      {
      out << setw(5) << malloc_used_bins[i] ;
      total += malloc_used_bins[i] ;
      }
   out << " =" << setw(6) << total << endl
       << "Malloc KB:    " ;
   total = 0 ;
   for (i = 0 ; i < FrMALLOC_BINS ; i++)
      {
      out << setw(5) << ((malloc_used_size[i] + 512) / 1024L) ;
      total += malloc_used_size[i] ;
      }
   out << " =" << setw(6) << ((total + 512) / 1024L) << endl
       << "Free blocks:  " ;
   total = 0 ;
   for (i = 0 ; i < FrMALLOC_BINS ; i++)
      {
      out << setw(5) << malloc_free_bins[i] ;
      total += malloc_free_bins[i] ;
      }
   out << " =" << setw(6) << total << endl
       << "Free KB:      " ;
   total = 0 ;
   for (i = 0 ; i < FrMALLOC_BINS ; i++)
      {
      out << setw(5) << ((malloc_free_size[i] + 512) / 1024L) ;
      total += malloc_free_size[i] ;
      }
   out << " =" << dec << setw(6) << ((total + 512) / 1024L) << endl ;
   return ;
}

//----------------------------------------------------------------------

void FrMemoryStats()
{
   FrMemoryStats(cerr) ;
   return ;
}

//----------------------------------------------------------------------

void FrMemoryStats(bool verb)
{
   FrMemoryStats(cerr,verb) ;
   return ;
}

//----------------------------------------------------------------------

void FrMemStats()
{
   FrMemoryStats(cerr) ;
   return ;
}

//----------------------------------------------------------------------

void FrMemStats(bool verb)
{
   FrMemoryStats(cerr,verb) ;
   return ;
}

//----------------------------------------------------------------------

struct sizet_and_ostream
   {
   size_t count ;
   ostream *out ;
   } ;

#ifdef FrREPLACE_MALLOC
static bool show_memory(FrBigAllocHdr *arena, void *user_data)
{
   sizet_and_ostream *meminfo = (sizet_and_ostream*)user_data ;
   ostream &out = *meminfo->out ;

   out << "Arena: " << hex << setprecision(8) << setw(8) << (uintptr_t)arena
       << dec << " (" << arena->arenaSize() << " bytes)" << endl ;
   for (FrBigAllocHdr *block = arena ; block ; block = block->next())
      {
      meminfo->count++ ;
      out << "  " << hex << setprecision(8) << setw(8) << (uintptr_t)block << ": "
	  << dec << setw(8) << block->size() << " bytes (" ;
      if (block->status == Block_Free)
	 out << "free" ;
      else if (block->status == Block_Reserved)
	 out << "reserved" ;
      else
	 out << "in use" ;
      out << ")" << endl ;
      }
   return true ;
}
#endif /* FrREPLACE_MALLOC */

//----------------------------------------------------------------------

void FrShowMemory(ostream &out)
{
#ifdef FrREPLACE_MALLOC
   FrBigAllocHdr *head = FramepaC_bigalloc_pool.freelistHead() ;
   out <<hex<< "Memory from " << FramepaC_memory_start << " to " << FramepaC_memory_end
       << " (memlist @" << head << ")" << dec << endl ;
   sizet_and_ostream showmem_info ;
   showmem_info.count = 0 ;
   showmem_info.out = &out ;
   if (FrIterateMemoryArenas(show_memory,&showmem_info))
      {
      out << "A total of " << showmem_info.count << " blocks of memory were listed." << endl ;
      }
#else
   out << "Memory map not available (compiled without FrREPLACE_MALLOC)." << endl ;
#endif /* FrREPLACE_MALLOC */
   return ;
}

//----------------------------------------------------------------------

void FrShowMemory() { FrShowMemory(cerr) ; }

//----------------------------------------------------------------------

// end of file frmemuse.cpp //
