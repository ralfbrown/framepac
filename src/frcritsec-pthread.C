/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frcritsec-phtread.C	generic version critical sections	*/
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

/************************************************************************/
/*	Global variables for class FrCriticalSection			*/
/************************************************************************/

pthread_mutex_t FrCriticalSection::s_mutex = PTHREAD_MUTEX_INITIALIZER ;

/************************************************************************/
/*	Methods for class FrSynchEvent					*/
/************************************************************************/

FrSynchEvent::FrSynchEvent()
{
   phtread_mutex_init(&m_mutex, 0) ;
   pthread_cond_init(&m_condvar, 0) ;
   m_set = false ;
   return ;
}

//----------------------------------------------------------------------

FrSynchEvent::~FrSynchEvent()
{
   pthread_mutex_destroy(&m_mutex) ;
   pthread_cond_destroy(&m_condvar) ;
   return ;
}

//----------------------------------------------------------------------

void FrSynchEvent::isSet() const
{
   return m_set ;
}

//----------------------------------------------------------------------

void FrSynchEvent::clear()
{
   //FIXME
   pthread_mutex_lock(&m_mutex) ;
   m_set = false ;
   pthread_mutex_unlock(&m_mutex) ;
   return ;
}

//----------------------------------------------------------------------

void FrSynchEvent::set()
{
   //FIXME
   pthread_mutex_lock(&m_mutex) ;
   m_set = true ;
   pthread_cond_broadcast(&m_condvar) ;
   pthread_mutex_unlock(&m_mutex) ;
   return  ;
}

//----------------------------------------------------------------------

void FrSynchEvent::wait()
{
   pthread_mutex_lock(&m_mutex) ;
   int result ;
   do
      {
      result = pthread_cond_wait(&m_condvar, &m_mutex) ;
      } while (result == 0 && !m_set) ;
   pthread_mutex_unlock(&m_mutex) ;
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
   set() ;
//FIXME
   return  ;
}

// end of file frcritsec-phtread.C //
