/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frtimer.h	    execution-time measurements			*/
/*  LastEdit: 08nov2015	  						*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,1998,2000,2001,2006,2009,2010,	*/
/*		2015 Ralf Brown/Carnegie Mellon University		*/
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

#ifndef __FRTIMER_H_INCLUDED
#define __FRTIMER_H_INCLUDED

#ifndef __FRCOMMON_H_INCLUDED
#include "frcommon.h"
#endif

#if defined(__GNUC__)
#  pragma interface
#  undef _POSIX_C_SOURCE
#  define _POSIX_C_SOURCE 200809L
#endif

#include <time.h>

//----------------------------------------------------------------------

#define FrTICKS_PER_SEC 10000

enum FrTimerState
   { FrTS_stopped, FrTS_running, FrTS_paused, FrTS_paused_for_subtimer } ;

typedef uint64_t FrTime ;

class FrTimer
   {
   private:
      FrTime start_time ;
      FrTimer *parent ;
      int num_subtimers ;
      bool includes_subtimers ;
      FrTimerState state ;
   public:
      void *operator new(size_t size) ;
      void operator delete(void *blk) ;
      FrTimer() ;
      FrTimer(FrTimer *parent) ;
      ~FrTimer() ;

      // running the timer
      void start() ;			// clear and start the timer
      clock_t read() ;			// return current elapsed time in ticks
      double readsec() ;		// return current el. time in seconds
      clock_t stop() ;			// stop timer and return elapsed time
					//   in 100 us (0.1ms) units
      double stopsec() ;		// stop timer & ret time in seconds
      clock_t pause() ;			// pause timer and return split time
      double pausesec() ;		// pause timer and return split time
      void resume() ;			// restart a paused timer

      // affecting the timer's operation
      bool includesSubTimers() const { return includes_subtimers ; }

      // access to internal state
      void includeSubTimers(bool inc) { includes_subtimers = inc ; }
      bool isRunning() const { return state == FrTS_running ; }
      bool isPaused() const
	    { return state==FrTS_paused || state==FrTS_paused_for_subtimer ; }
      bool isStopped() const { return state == FrTS_stopped ; }
   } ;

//----------------------------------------------------------------------

class FrElapsedTimer
   {
   private:
      struct timespec	m_starttime ;
      double		m_split_time ;
      FrTimerState	m_state ;
   public:
      FrElapsedTimer() { start() ; }
      ~FrElapsedTimer() {}

      // running the timer
      void start() ;			// clear and start the timer
      double read() const ;		// return current el. time in seconds
      double read100ths() const ;	// return time rounded to 0.01 seconds
      double stop() ;			// stop timer and return elapsed time
      double pause() ;			// pause timer and return split time
      void resume() ;			// restart paused timer

      // access to internal state
      bool isRunning() const { return m_state == FrTS_running ; }
      bool isPaused() const { return m_state == FrTS_paused ; }
      bool isStopped() const { return m_state == FrTS_stopped ; }
   } ;

//----------------------------------------------------------------------

int start_timer() ;
double stop_timer() ;

#endif /* !__FRTIMER_H_INCLUDED */

// end of file frtimer.h //
