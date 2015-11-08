/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frtimer.cpp	    high-resolution execution-time measurement	*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,1998,2000,2001,2002,2006,2009,	*/
/*		2010,2013,2015 Ralf Brown/Carnegie Mellon University	*/
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
#  pragma implementation "frtimer.h"
#endif

#include <math.h>

#ifdef __WATCOMC__
#include <bios.h>
#endif
#include "framerr.h"
#include "frmem.h"
#include "frtimer.h"
#if defined(__WINDOWS__) || defined(__NT__) || defined(_WIN32)
#include "frmswin.h"
#endif

/**********************************************************************/
/*       Configuration Options                                        */
/**********************************************************************/

//#define USE_CLOCK     // use clock() instead of getrusage()

/**********************************************************************/

#if defined(__WATCOMC__) && defined(__SW_5)  // compiling 586 code?
#  define PENTIUM			// using Pentium CPU's RDTSC instruc
#endif /* __WATCOMC__ && __SW_5 */

#if defined(_M_IX86) && _M_IX86 >= 500
#  define PENTIUM			// using Pentium CPU's RDTSC instruc
#endif /* _M_IX86 && _M_IX86 >= 500 */

#if defined(__WINDOWS__) || defined(__NT__) || defined(_WIN32)
#  define USE_WINDOWS
#  undef PENTIUM
#endif

#if defined(__MSDOS__) || defined(MSDOS)
#  define USE_CLOCK     /* no getrusage() on MSDOS */
#endif

#if defined(__SOLARIS__)
#  define USE_CLOCK	/* no getrusage() under Solaris */
#endif

#if defined(__linux__) && defined(CLOCKS_PER_SEC)
#  define USE_CLOCK     /* getrusage() can give really weird results */
//!!!# if !defined(_POSIX_C_SOURCE) || _POSIX_C_SOURCE < 199501L
#  undef CLOCK_PROCESS_CPUTIME_ID   /* clock_gettime() not really supported */
//!!!# endif /* _POSIX_C_SOURCE */
#endif

/**********************************************************************/
/**********************************************************************/

#if !defined(USE_CLOCK) && !defined(USE_WINDOWS)
#include <sys/time.h>
#include <sys/resource.h>
#endif /* !USE_CLOCK && !USE_WINDOWS */

/**********************************************************************/
/*      Workarounds for various problems                              */
/**********************************************************************/

// SunOS header files at CMU don't define CLOCKS_PER_SEC !!
#ifndef CLOCKS_PER_SEC
# ifdef CLK_TCK
#  define CLOCKS_PER_SEC CLK_TCK
#else
#  define CLOCKS_PER_SEC 1000000
# endif /* CLK_TCK */
#endif /* !CLOCKS_PER_SEC */

/**********************************************************************/
/*      Timing routines                                               */
/**********************************************************************/

#if defined(PENTIUM)
#if defined(__BORLANDC__)
  inline void RDTSC(unsigned long *time)
  {
     // some BorlandC-specific code to read the number of clock cycles
     __emit__(0x0F,0x31) ; // RDTSC instruction: ReaD TimeStamp Counter
     time[0] = _EAX ;
     time[1] = _EDX ;
  }
#elif defined(__WATCOMC__)
  extern void RDTSC(uint32_t *time) ;
  #pragma aux RDTSC = \
       "db 0fh,31h"  \
       "mov [ebx],eax" \
       "mov [ebx+4],edx" \
       parm [ebx] \
       modify exact [eax edx] ;
#elif defined(_MSC_VER)
  inline void RDTSC(uint32_t *time)
      { unsigned long high, low ;
	_asm db 0fh,31h ;
	_asm mov low,eax ;
	_asm mov high,edx ;
	time[0] = low ;
	time[1] = high ;
      } ;
#else
  inline void RDTSC(uint32_t *time)
	// try a generic inline assembler format, and hope the compiler
	// understands it:
      { unsigned long high, low ;
	asm db 0fh,31h ;
	asm mov low,eax ;
	asm mov high,edx ;
	time[0] = low ;
	time[1] = high ;
      }
#endif /* per-compiler defs */
#endif /* PENTIUM */

//----------------------------------------------------------------------

inline void read_time(FrTime &time)
{
#if defined(USE_WINDOWS)
   time = (FrTime)FrGetCPUTime() ;
#elif defined(PENTIUM)
   RDTSC((unsigned long*)&time) ;
#elif defined(CLOCK_PROCESS_CPUTIME_ID)	// Posix clock_*() supported
   struct timespec tm ;
   if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID,&tm) == 0)
      time = ((1000000000L * (FrTime)tm.tv_sec) + tm.tv_nsec) ;
   else
      time = 0 ;
#elif defined(USE_CLOCK)
   time = (FrTime)clock() ;
#else
   struct rusage usage ;
   getrusage(RUSAGE_SELF,&usage) ;
   time = ((1000000L * (FrTime)usage.ru_utime.tv_sec)
	   + usage.ru_uptime.tv_usec) ;
#endif
   return ;
}

//----------------------------------------------------------------------

#if defined(PENTIUM)
static clock_t half_clocks_per_tick = 5000L ; // default is 100 MHz
static clock_t clocks_per_tick     = 10000L ;

#if defined(__WATCOMC__) && defined(__386__)
extern clock_t TSC_to_ticks(uint32_t *time) ;
#pragma aux TSC_to_ticks = \
    "mov eax,[ebx]" \
    "mov edx,[ebx+4]" \
    "add eax,half_clocks_per_tick" \
    "adc edx,0" \
    "cmp edx,clocks_per_tick" \
    "jae l1" \
    "div clocks_per_tick" \
    "jmp short l2" \
 "l1:" \
    "mov eax,0xFFFFFFFF" \
 "l2:" \
    parm [ebx] \
    value [eax] \
    modify exact [eax edx] ;

extern clock_t TSC_to_clocks(uint32_t tsc) ;
#pragma aux TSC_to_clocks = \
    "mov edx,1000000" \
    "mov ebx,109850" \
    "mul edx" \
    "add eax,54925" /*109850/2*/ \
    "adc edx,0" \
    "div ebx" \
    parm [eax] value [eax] modify exact [eax ebx edx] ;

#elif defined(_MSC_VER)
static clock_t TSC_to_ticks(uint32_t *time)
   { clock_t ticks ;
    _asm {
           mov ebx,time ;
	   mov eax,[ebx] ;
	   mov edx,[ebx+4] ;
	   add eax,half_clocks_per_tick ;
	   adc edx,0 ;
	   cmp edx,clocks_per_tick ;
	   jae l1 ;
	   div clocks_per_tick ;
	   jmp short l2 ;
         }
      l1:
    _asm {
           mov eax,0xFFFFFFFF ;
         } ;
      l2:
    _asm {
           mov ticks,eax
         } ;
    return ticks ;
   } ;

static clock_t TSC_to_clocks(uint32_t tsc)
   { clock_t clocks ;
    _asm {
           mov eax,tsc
           mov edx,1000000
	   mov ebx,109850
	   mul edx
           add eax,54925   /* 109850/2 */
           adc edx,0
           div ebx
           mov clocks,eax
         } ;
    return clocks ;
   } ;

#else
static clock_t quarter_clocks_per_tick = 25L*PENTIUM ;

static uint32_t div64_32(uint32_t high, uint32_t low, uint32_t divisor)
{
   if (high)
      {
      // reduce the problem to a 64/32 -> 32-bit problem, and remember the
      // high 32 bits of the result
      uint32_t quothi = high / divisor ;
      high %= divisor ;
      // now start with the divisor shifted left 32 bits, and progressively
      // shift it right, subtracting as needed and remembering where we
      // subtracted
      uint32_t quotlo = 0UL ;
      uint32_t divhi = divisor ;
      uint32_t divlo = 0UL ;
      for (int i = 32 ; i > 0 ; i--)
	 {
	 // shift divisor right by one bit
	 divlo >>= 1 ;
	 if (divhi & 1)
	    divlo |= 0x80000000UL ;
	 divhi >>= 1 ;
	 quotlo <<= 1 ;
	 if (high > divhi)
	    {
	    high -= divhi ;
	    if (low < divlo)            // do we have to borrow?
	       high-- ;
	    low -= divlo ;
	    quotlo++ ;                  // remember the subtraction
	    }
	 else if (high == divhi && low >= divlo)
	    {
	    high = 0 ;
	    low -= divlo ;
	    quotlo++ ;                  // remember the subtraction
	    }
	 }
      if (quothi > 0)
	 low = 0xFFFFFFFF ;
      else
	 low = quotlo ;
      }
   else // only 32-bit dividend, so can use normal divide
      low /= divisor ;
   return low ;
}

inline clock_t TSC_to_ticks(uint64_t *timeptr)
{
   // given total number of clock cycles elapsed, compute 0.1ms ticks
   //   (rounded to nearest multiple)
   uint64_t time = (*timeptr) + half_clocks_per_tick ;
   if (((clock_t)time) != time)
      {
      FrWarning("Time count overflowed!") ;
      return (clock_t)-1L ;
      }
   else
      return time / clocks_per_tick ;
}
#endif
#endif /* PENTIUM */

//----------------------------------------------------------------------

static clock_t elapsed_time(FrTime &time)
{
#if defined(USE_WINDOWS)
   return (clock_t)time ;
#elif defined(PENTIUM)
   return TSC_to_ticks(&time) ;
#elif defined(CLOCK_PROCESS_CPUTIME_ID)	// Posix clock_*() supported
   return (clock_t)(time / (1000000000L / FrTICKS_PER_SEC)) ;
#elif defined(USE_CLOCK)
# if CLOCKS_PER_SEC >= FrTICKS_PER_SEC
   return time / (CLOCKS_PER_SEC / FrTICKS_PER_SEC) ;
# else
   return time * (FrTICKS_PER_SEC / CLOCKS_PER_SEC) ;
# endif /* CLOCKS_PER_SEC >= FrTICKS_PER_SEC */
#else
   return time / (1000000L / FrTICKS_PER_SEC) ;
#endif /* PENTIUM / USE_CLOCK / */
}

//----------------------------------------------------------------------

#if defined(PENTIUM)
static int TSC_initialized = false ;

static uint32_t clock_speeds[] =
   {  60000000L,
      66000000L,
      75000000L,
      90000000L,
     100000000L,
     120000000L,
     133000000L,
     150000000L,
     166000000L,
     180000000L,
     200000000L,
     233000000L,
     250000000L,
     266000000L,
     300000000L,
     333000000L,
     350000000L,
     400000000L,
     450000000L,
     500000000L,
     550000000L,
     600000000L,
     650000000L,
     666000000L,
     700000000L,
     733000000L,
     750000000L,
     800000000L,
     866000000L,
     900000000L,
     933000000L
    1000000000L,
    1100000000L,
    1200000000L,
    1333000000L,
    1400000000L,
    1433000000L,
    1466000000L,
    1500000000L,
    1533000000L,
    1566000000L,
    1600000000L,
    1700000000L,
    1800000000L,
    2000000000L,
    2200000000L,
    2400000000L,
    2500000000L,
    2600000000L,
    2800000000L,
    3000000000L,
    3160000000L,
    3200000000L,
    3333000000L,
    3400000000L,
    3500000000L,
    3600000000L } ;

static clock_t round_clock_speed(clock_t clocks)
{
   for (size_t i = 0 ; i < lengthof(clock_speeds) ; i++)
      {
      clock_t diff ;
      if (clocks < clock_speeds[i])
	 diff = clock_speeds[i] - clocks ;
      else
	 diff = clocks - clock_speeds[i] ;
      if (diff < 1500*1000L)		// within 1.5 MHz of a standard speed?
	 return clock_speeds[i] ;
      else if (clocks > clock_speeds[i])
	 break ;
      }
   // OK, not a known clock speed, so round to nearest megahertz
   clocks = ((clocks + 500*1000L) / (1000*1000L)) * 1000L * 1000L ;
   return clocks ;
}
#endif /* PENTIUM */

#ifdef PENTIUM
static void initialize_TSC()
{
   if (!TSC_initialized)
      {
      FrTime t1 ;
      FrTime t2 ;
#ifdef __WATCOMC__
      // the fast, Watcom-specific version
      long start_time ;
      long end_time ;
      _bios_timeofday(_TIME_GETCLOCK,&start_time) ;
      start_time += 2 ;
      if (start_time >= 0x1800B0L)	// handle midnight wrap
	 start_time -= 0x1800B0L ;
      end_time = start_time + 2 ;
      if (start_time >= 0x1800B0L)	// handle midnight wrap
	 start_time -= 0x1800B0L ;
      long curr_time ;
      do {
	 _bios_timeofday(_TIME_GETCLOCK,&curr_time) ;
	 } while (curr_time < start_time) ;
      read_time(t1) ;
      do {
	 _bios_timeofday(_TIME_GETCLOCK,&curr_time) ;
	 } while (curr_time < end_time) ;
      read_time(t2) ;
      t2 -= t1 ;
      if (t2.time[1])
	 FrWarning("CPU too fast for timing code....") ;
      else
	 {
	 clock_t clocks_per_sec = TSC_to_clocks(t2) ;
	 clocks_per_sec = round_clock_speed(clocks_per_sec) ;
	 clocks_per_tick = (clocks_per_sec + FrTICKS_PER_SEC/2) /
			   FrTICKS_PER_SEC ;
	 half_clocks_per_tick = clocks_per_tick / 2 ;
	 }
#else
      // the more portable, but slower version....
      unsigned long start_time = clock()+2 ;
      unsigned long end_time = clock()+1+(CLOCKS_PER_SEC/2) ;
      while (clock() < start_time)
	 ;
      read_time(t1) ;
      while (clock() < end_time)
	 ;
      read_time(t2) ;
      t2 -= t1 ;
      if (t2.time[1])
	 FrWarning("CPU too fast for timing code....") ;
      else
	 {
	 clock_t clocks_per_sec = 2*t2.time[0] ; // we timed for half a second
	 clocks_per_sec = round_clock_speed(clocks_per_sec) ;
	 clocks_per_tick = (clocks_per_sec + FrTICKS_PER_SEC/2) /
			   FrTICKS_PER_SEC ;
	 half_clocks_per_tick = clocks_per_tick / 2 ;
	 quarter_clocks_per_tick = (clocks_per_tick + 3) / 4 ;
	 }
#endif /* __WATCOMC__ */
      TSC_initialized = true ;
      }
}
#endif /* PENTIUM */

/************************************************************************/
/*      FrTimer methods							*/
/************************************************************************/

void *FrTimer::operator new(size_t size)
{
   return FrMalloc(size) ;
}

//----------------------------------------------------------------------

void FrTimer::operator delete(void *blk)
{
   FrFree(blk) ;
}

//----------------------------------------------------------------------

FrTimer::FrTimer()
{
#ifdef PENTIUM
   if (!TSC_initialized)
      initialize_TSC() ;
#endif
   parent = 0 ;
   state = FrTS_running ;
   includes_subtimers = true ;
   num_subtimers = 0 ;
   read_time(start_time) ;
}

//----------------------------------------------------------------------

FrTimer::FrTimer(FrTimer *parent_timer)
{
#ifdef PENTIUM
   if (!TSC_initialized)
      initialize_TSC() ;
#endif
   parent = parent_timer ;
   if (parent)
      {
      parent->num_subtimers++ ;
      if (!parent->includes_subtimers)
	 {
	 parent->pause() ;
	 parent->state = FrTS_paused_for_subtimer ;
	 }
      }
   state = FrTS_running ;
   includes_subtimers = true ;
   num_subtimers = 0 ;
   read_time(start_time) ;
   return ;
}

//----------------------------------------------------------------------

FrTimer::~FrTimer()
{
   if (parent)
      {
      if (state == FrTS_running && parent->state == FrTS_paused_for_subtimer)
	 {
	 FrTime currtime ;
	 read_time(currtime) ;
	 currtime -= parent->start_time ; // start_time contains prior elapsed
	 parent->start_time = currtime ;
	 parent->state = FrTS_running ;
	 }
      parent->num_subtimers-- ;
      }
   if (num_subtimers > 0)
      FrWarning("deallocated an FrTimer while subtimers were still instantiated") ;
   return ;
}

//----------------------------------------------------------------------

void FrTimer::start()
{
   if (parent && !parent->includes_subtimers && parent->state == FrTS_running)
      {
      parent->pause() ;
      parent->state = FrTS_paused_for_subtimer ;
      }
   state = FrTS_running ;
   read_time(start_time) ;
   return ;
}

//----------------------------------------------------------------------

clock_t FrTimer::read()
{
   FrTime currtime ;
   read_time(currtime) ;
   currtime -= start_time ;
   return elapsed_time(currtime) ;
}

//----------------------------------------------------------------------

clock_t FrTimer::stop()
{
   FrTime currtime ;
   read_time(currtime) ;
   currtime -= start_time ;
   clock_t elapsed = elapsed_time(currtime) ;
   state = FrTS_stopped ;
   if (parent && parent->state == FrTS_paused_for_subtimer)
      {
      read_time(currtime) ;
      currtime -= parent->start_time ;	// start_time contains prior elapsed
      parent->start_time = currtime ;
      parent->state = FrTS_running ;
      }
   return elapsed ;
}

//----------------------------------------------------------------------

clock_t FrTimer::pause()
{
   if (state == FrTS_running)
      {
      state = FrTS_paused ;
      FrTime currtime ;
      read_time(currtime) ;
      currtime -= start_time ;		// get elapsed time
      start_time = currtime ;
      return elapsed_time(currtime) ;
      }
   else
      {
      FrWarning("asked to pause a timer which was not running") ;
      return read() ;
      }
}

//----------------------------------------------------------------------

void FrTimer::resume()
{
   switch (state)
      {
      case FrTS_stopped:
	 start() ;
	 break ;
      case FrTS_running:
	 // do nothing
	 break ;
      case FrTS_paused_for_subtimer:
	 FrWarning("resumed timer while a subtimer was still active") ;
	 // fall through to FrTS_paused
      case FrTS_paused:
	 {
	 FrTime currtime ;
	 read_time(currtime) ;
	 currtime -= start_time ;	// start_time contains prior elapsed
	 start_time = currtime ;
	 }
	 break ;
      default:
	 FrMissedCase("FrTimer::resume") ;
      }
   return ;
}

/************************************************************************/
/*      FrElapsedTimer methods						*/
/************************************************************************/

static double time_diff(const struct timespec &t1, const struct timespec &t2)
{
   return (t2.tv_sec + 1e-9 * t2.tv_nsec) - (t1.tv_sec + 1e-9 * t1.tv_nsec) ;
}

//----------------------------------------------------------------------

void FrElapsedTimer::start()
{
   m_state = FrTS_stopped ;
   resume() ;
   return  ;
}

//----------------------------------------------------------------------

double FrElapsedTimer::read() const
{
   if (m_state == FrTS_running)
      {
      struct timespec now ;
      clock_gettime(CLOCK_MONOTONIC,&now) ;
      return time_diff(m_starttime,now) + m_split_time ;
      }
   else
      return m_split_time ;
}

//----------------------------------------------------------------------

double FrElapsedTimer::read100ths() const
{
   double walltime = read() ;
   return round(100.0*walltime)/100.0 ;
}

//----------------------------------------------------------------------

double FrElapsedTimer::stop() 
{
   m_split_time = read() ;
   m_state = FrTS_stopped ;
   return m_split_time ;
}

//----------------------------------------------------------------------

double FrElapsedTimer::pause()
{
   m_split_time = read() ;
   m_state = FrTS_paused ;
   return m_split_time ;
}

//----------------------------------------------------------------------

void FrElapsedTimer::resume()
{
   if (m_state == FrTS_stopped)
      {
      m_split_time = 0 ;
      m_state = FrTS_paused ;
      }
   if (m_state == FrTS_paused)
      {
      clock_gettime(CLOCK_MONOTONIC,&m_starttime) ;
      }
   m_state = FrTS_running ;
   return ;
}

// end of file frtimer.cpp //
