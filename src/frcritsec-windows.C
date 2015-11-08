/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frcritsec-windows.C	MSWindows-specific critical sections	*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 2010,2013,2014,2015					*/
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

#include "frcritsec.h"

#ifdef __WINDOWS__

#include "windows.h"
// Windows API calls from https://msdn.microsoft.com/en-us/magazine/jj721588.aspx

/************************************************************************/
/*	Methods for class FrSynchEvent					*/
/************************************************************************/

FrSynchEvent::FrSynchEvent()
   : m_eventhandle(CreateEvent(NULL, true, false, NULL))
{
   return ;
}

//----------------------------------------------------------------------

FrSynchEvent::~FrSynchEvent()
{
   CloseHandle(m_eventhandle) ;
   return ;
}

//----------------------------------------------------------------------

void FrSynchEvent::set()
{
   SetEvent(m_eventhandle) ;
   return ;
}

//----------------------------------------------------------------------

void FrSynchEvent::clear()
{
   ResetEvent(m_eventhandle) ;
   return ;
}

//----------------------------------------------------------------------

void FrSynchEvent::wait()
{
   WaitForSingleObject(m_eventhandle, INFINITE) ;
   return ;
}

/************************************************************************/
/*	Methods for class FrSynchEventCounted				*/
/************************************************************************/

bool FrSynchEventCounted::isSet() const
{
   return false ; //FIXME
}

//----------------------------------------------------------------------

unsigned FrSynchEventCounted::numWaiters() const
{
   return 0 ; //FIXME
}

//----------------------------------------------------------------------

void FrSynchEventCounted::clear()
{
//FIXME
   return  ;
}

//----------------------------------------------------------------------

void FrSynchEventCounted::clearAll()
{
//FIXME
   return  ;
}

//----------------------------------------------------------------------

void FrSynchEventCounted::set()
{
//FIXME
   return ;
}

//----------------------------------------------------------------------

void FrSynchEventCounted::wait()
{
//FIXME
   return ;
}

//----------------------------------------------------------------------

void FrSynchEventCounted::waitForWaiters()
{
//FIXME
   return  ;
}

/************************************************************************/
/*	Methods for class FrSynchEventCountdown				*/
/************************************************************************/

/* looks like InitializeSynchronizationBarrier and
 * EnterSynchronizationBarrier are similar to what I want, but
 * Enter... blocks until all have arrived.  The closest match I can
 * see is semaphores initialized with a negative count [not allowed],
 * where consume() increments the count, but they would only release
 * one thread rather than all.
 */

void FrSynchEventCountdown::consume()
{
//FIXME
   return ;
}

//----------------------------------------------------------------------

void FrSynchEventCountdown::consumeAll()
{
//FIXME
   return ;
}

//----------------------------------------------------------------------

void FrSynchEventCountdown::wait()
{
//FIXME
   return ;
}

#endif /* __WINDOWS__ */

// end of file frcritsec-windows.C //
