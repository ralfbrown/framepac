/************************************************************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File parmem.cpp	   Test/Demo program: parallel memory allloc	*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,2000,2004,2009,2013		*/
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

#include "FramepaC.h"

/************************************************************************/
/*	Type declarations						*/
/************************************************************************/

class MemRequestOrder
   {
   public:
      size_t memsize ;
      size_t cycles ;
      size_t id ;
      size_t current_cycle ;
      size_t numblocks ;
      size_t num_alloc ;
      char   **blocks ;
      FrThreadPool *pool ;
      bool   crossover ;
   public:
      MemRequestOrder(size_t m, size_t c)
	 { memsize = m ; cycles = c ; id = 0 ; current_cycle = 0 ; blocks = 0 ; }
      ~MemRequestOrder() { freeMemory() ; }

      void freeMemory() ;
   } ;

/************************************************************************/
/*	Forward declarations						*/
/************************************************************************/

/************************************************************************/
/*	Members for class MemRequestOrder				*/
/************************************************************************/

void MemRequestOrder::freeMemory()
{
   if (blocks)
      {
      for (size_t i = 0 ; i < numblocks ; i++)
	 {
	 FrFree(blocks[i]) ;
	 blocks[i] = 0 ;
	 }
      FrFree(blocks) ;
      blocks = 0 ;
      }
   return  ;
}

/************************************************************************/
/************************************************************************/

static size_t random_size(size_t avg_size)
{
   double rnd = FrRandomNumber(1.0) ;
   // turn the uniform distribution into an exponential distribution
   double expdist = -log(1.0 - rnd) ;
   // scale and round the random value
   return (size_t)((expdist * avg_size) + 0.5) ;
}

//----------------------------------------------------------------------

static void memory_user(const void *input, void * /*output*/)
{
   size_t avg_size = 5000 ;  // average allocation size (expon. distributed)
   MemRequestOrder *order = (MemRequestOrder*)input ;
   size_t memsize = order->memsize * 1024 * 1024 ;
   if (!order->blocks || !order->numblocks)
      {
      order->numblocks = memsize / (3 * avg_size / 2) ;
      order->blocks = FrNewC(char*,order->numblocks) ;
      order->current_cycle = 1 ;
      order->num_alloc = 0 ;
      if (order->crossover)
	 {
	 if (order->blocks)
	    {
	    order->pool->dispatch(memory_user,order,0) ;
	    }
	 return ;
	 }
      }
   if (!order->blocks)
      return ;
   while (order->current_cycle <= order->cycles)
      {
      size_t bytes_alloc = 0 ;
      while (bytes_alloc < memsize)
	 {
	 size_t slot = FrRandomNumber(order->numblocks) ;
	 if (order->blocks[slot])
	    FrFree(order->blocks[slot]) ;
	 size_t size = random_size(avg_size) ;
	 order->blocks[slot] = FrNewN(char,size) ;
	 order->num_alloc++ ;
	 bytes_alloc += size ;
	 }
      for (size_t i = 0 ; i < order->numblocks ; i++)
	 {
	 if (FrRandomNumber(2))
	    {
	    FrFree(order->blocks[i]) ;
	    order->blocks[i] = 0 ;
	    }
	 }
      if (order->current_cycle % 50 == 0 &&
	  order->current_cycle < order->cycles)
	 cout << "    job" << order->id << " cycle " << order->current_cycle
	      << " alloc=" << order->num_alloc << endl ;
      order->current_cycle++ ;
      if (order->crossover)
	 {
	 order->pool->dispatch(memory_user,order,0) ;
	 return ;
	 }
      }
   if (order->current_cycle > order->cycles)
      {
      order->freeMemory() ;
      cout << "  Job " << order->id << " done. " << order->num_alloc
	   << " memory blocks allocated."  << endl ;
      }
   return ;
}

//----------------------------------------------------------------------

static void alloc_test(ostream &out, size_t threads, size_t memsize, size_t cycles, bool crossover)
{
   FrTimer timer ;
   FrThreadPool tpool(threads) ;
   bool must_wait = (threads != 0) ;
   if (threads == 0) threads = 1 ;
   MemRequestOrder *memorders = FrNewC(MemRequestOrder,threads+1) ;
   out << "  Dispatching threads" << endl ;
   for (size_t i = 1 ; i <= threads ; i++)
      {
      memorders[i].memsize = memsize ;
      memorders[i].cycles = cycles ;
      memorders[i].id = i ;
      memorders[i].crossover = crossover ;
      memorders[i].pool = &tpool ;
      tpool.dispatch(memory_user,&memorders[i],0) ;
      }
   if (must_wait)
      {
      out << "  Waiting for thread completion" << endl ;
      tpool.waitUntilIdle() ;
      }
   for (size_t i = 1 ; i <= threads ; i++)
      {
      if (memorders[i].blocks)
	 {
	 out << "  !! Thread " << i << " did not properly clean up" << endl ;
	 }
      }
   FrFree(memorders) ;
   out << "  CPU time: " << timer.readsec() << " seconds" << endl ;
   return  ;
}

//----------------------------------------------------------------------

void parmem_command(ostream &out, istream &in)
{
   size_t threads ;
   size_t memsize ;
   size_t cycles ;
   out << "Parallel (threaded) Memory Allocation" << endl << endl ;
#ifdef FrMULTITHREAD
   out << "Enter number of threads: " ;
   in >> threads ;
#else
   out << "Compiled without multi-thread support, will run single-threaded" << endl ;
   threads = 0 ;
#endif /* FrMULTITHREAD */
   out << "Enter per-thread megabytes of memory to allocate: " ;
   in >> memsize ;
   out << "Enter allocation cycles per thread: " ;
   in >> cycles ;
   if (memsize > 0)
      {
      out << endl << "Thread-local allocations/frees only" << endl ;
      alloc_test(out,threads,memsize,cycles,false) ;
//      if (threads > 1 && 0)
	 {
	 out << "-----------" << endl ;
	 out << "Cross-thread frees" << endl ;
	 alloc_test(out,threads,memsize,cycles,true) ;
	 }
      }
   return ;
}


// end of file parmem.C //
