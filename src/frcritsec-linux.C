/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frcritsec-linux.C	Linux-specific  critical section mutex	*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 2010,2013,2014,2015					*/
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

#include "frcritsec.h"

#ifdef __linux__

/************************************************************************/
/************************************************************************/

// code for sys_futex from http://locklessinc.com/articles/obscure_synch/
#include <linux/futex.h>
#include <stdio.h>
#include <stdint.h>
#include <limits.h>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif /* _GNU_SOURCE */
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/time.h>
static long sys_futex(void *addr1, int op, int val1, struct timespec *timeout, void *addr2, int val3)
{
   return syscall(SYS_futex, addr1, op, val1, timeout, addr2, val3);
}

/************************************************************************/
/*	Methods for class FrSynchEvent					*/
/************************************************************************/

bool FrSynchEvent::isSet() const
{
   return m_event.m_set ;
}

//----------------------------------------------------------------------

void FrSynchEvent::clearAll()
{
   FrCriticalSection::store(m_futex_val,0U) ;
   return ;
}

//----------------------------------------------------------------------

void FrSynchEvent::clear()
{
   FrCriticalSection::barrier() ;	// prevent code motion
   m_event.m_set = false ;
   FrCriticalSection::barrier() ;
   return ;
}

//----------------------------------------------------------------------

void FrSynchEvent::set()
{
   _atomic_store(m_event.m_set,true) ;
   if (_atomic_load(m_event.m_waiters))
      {
      _atomic_store(m_event.m_waiters,false) ;
      sys_futex(this, FUTEX_WAKE_PRIVATE, INT_MAX, NULL, NULL, 0) ;
      }
   return ;
}

//----------------------------------------------------------------------

void FrSynchEvent::wait()
{
   while (!_atomic_load(m_event.m_set))
      {
      _atomic_store(m_event.m_waiters,true) ;
      // wait as long as m_set is still false and m_waiters is still true
      sys_futex(this, FUTEX_WAIT_PRIVATE, 1, NULL, NULL, 0) ;
      }
   return ;
}

/************************************************************************/
/*	Methods for class FrSynchEventCounted				*/
/************************************************************************/

bool FrSynchEventCounted::isSet() const
{
   return (m_futex_val & m_mask_set) != 0 ;
}

//----------------------------------------------------------------------

unsigned FrSynchEventCounted::numWaiters() const
{
   return (m_futex_val & ~m_mask_set) ;
}

//----------------------------------------------------------------------

void FrSynchEventCounted::clear()
{
   (void)FrCriticalSection::testAndClearMask(m_futex_val,m_mask_set) ;
   return  ;
}

//----------------------------------------------------------------------

void FrSynchEventCounted::clearAll()
{
   FrCriticalSection::store(m_futex_val,(unsigned)0) ;
   return  ;
}

//----------------------------------------------------------------------

void FrSynchEventCounted::set()
{
   unsigned status = FrCriticalSection::testAndSetMask(m_futex_val,m_mask_set) ;
   if ((status & m_mask_set) == 0 && status > 0)
      {
      // event hasn't triggered yet, but there are waiters, so wake them up
      sys_futex(&m_futex_val, FUTEX_WAKE_PRIVATE, INT_MAX, NULL, NULL, 0) ;
      }
   return ;
}

//----------------------------------------------------------------------

void FrSynchEventCounted::wait()
{
   // we need to loop, since sys_futex can experience spurious wakeups
   while ((FrCriticalSection::increment(m_futex_val) & m_mask_set) == 0)
      {
      size_t wait_while = m_futex_val & ~m_mask_set ;
      // wait as long as 'set' bit is still false and numWaiters is unchanged
      sys_futex(&m_futex_val, FUTEX_WAIT_PRIVATE, wait_while, NULL, NULL, 0) ;
      FrCriticalSection::decrement(m_futex_val) ;
      }
   // we're done waiting
   FrCriticalSection::decrement(m_futex_val) ;
   return ;
}

//----------------------------------------------------------------------

void FrSynchEventCounted::waitForWaiters()
{
   set() ;
   while ((_atomic_load(m_futex_val) & ~m_mask_set) > 0)
      {
      FrThreadYield() ;
      }
   return  ;
}

/************************************************************************/
/*	Methods for class FrSynchEventCountdown				*/
/************************************************************************/

void FrSynchEventCountdown::consume()
{
   if (FrCriticalSection::decrement(m_futex_val,2) <= 3)
      {
      // if counter was 1 before the decrement, wake everyone who is waiting
      consumeAll() ;
      }
   return ;
}

//----------------------------------------------------------------------

void FrSynchEventCountdown::consumeAll()
{
   if (FrCriticalSection::swap(m_futex_val,0) & 1)
      {
      // someone is waiting, so wake all the waiters
      sys_futex(this, FUTEX_WAKE_PRIVATE, INT_MAX,NULL, NULL, 0) ;
      }
   return ;
}

//----------------------------------------------------------------------

void FrSynchEventCountdown::wait()
{
   // loop until the countdown has expired, because sys_futex can wake
   //   spuriously, or it may have never gone to sleep because someone
   //   called consume() during the interval when we set the 'waiting'
   //   flag
   while (FrCriticalSection::load(m_futex_val) > 1)
      {
      int counter = FrCriticalSection::testAndSetMask(m_futex_val,1) ;
      if (counter > 1)
	 {
	 // wait as long as 'waiting' bit is still set and count > 0
	 sys_futex(this, FUTEX_WAIT_PRIVATE, (counter|1), NULL, NULL, 0) ;
	 }
      }
   return ;
}

#endif /* __linux__ */

// end of file frcritsec-linux.C //
