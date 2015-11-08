/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frmswin.cpp		 Microsoft Windows-specific functions	*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1996,1997,1998,2001,2006,2009				*/
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

#if defined(__WINDOWS__) || defined(__NT__) || defined(_WIN32)

#include <time.h>
#include "frcommon.h"
#include <winsock.h>
#include "frtimer.h"

/************************************************************************/
/*    global variables							*/
/************************************************************************/

int FrWinSock_initialized = false ;

/************************************************************************/
/*	Helper functions						*/
/************************************************************************/

#if defined(__WATCOMC__) && defined(__386__)
extern uint32_t div64_32(uint32_t high, uint32_t low, uint32_t divisor) ;
#pragma aux div64_32 = \
    "div ebx" \
    parm [edx][eax][ebx] \
    modify exact [eax edx] \
    value [eax] ;

#elif defined(_MSC_VER) && _MSC_VER >= 800

inline uint32_t div64_32(uint32_t highhalf,uint32_t lowhalf,uint32_t divisor)
{
   uint32_t result ;
   _asm {
	  mov edx,highhalf ;
	  mov eax,lowhalf ;
	  div divisor ;
	  mov result,eax ;
	} ;
   return result ;
}

#else // neither Watcom C++ nor Visual C++

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
	    if (low < divlo)		// do we have to borrow?
	       high-- ;
	    low -= divlo ;
	    quotlo++ ;			// remember the subtraction
	    }
	 else if (high == divhi && low >= divlo)
	    {
	    high = 0 ;
	    low -= divlo ;
	    quotlo++ ;			// remember the subtraction
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
#endif /* __WATCOMC__ && __386__ */

//----------------------------------------------------------------------

// this function lifted from Eric Thayer
int FramepaC_winsock_init()
{
   if (FrWinSock_initialized)
      return true ;
   WORD ver = MAKEWORD(2,0) ;
   WSADATA data ;
   int32_t err ;

   if ((err = WSAStartup (ver, &data)) != 0)
      return false ;
#if 0
   if ((LOBYTE(data.wVersion) != 2) || (HIBYTE(data.wVersion) != 0))
      {
      WSACleanup() ;
      return false ;
      }
#endif
   FrWinSock_initialized = true ;
   return true ;
}

/************************************************************************/
/************************************************************************/

void FrMessageLoop()
{
   // handle any pending Windows messages
   MSG msg ;
   while (PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE))
      {
      TranslateMessage(&msg) ;
      DispatchMessage(&msg) ;
      }
}

//----------------------------------------------------------------------

#if __WATCOMC__ >= 1200 	 // OpenWatcom
# undef __WATCOMC__
#endif

#ifdef __WATCOMC__
extern "C" HWND _MainWindow ;	 // provided by Watcom C++
#endif /* __WATCOMC__ */

void FrMinimizeWindow()
{
#ifdef __WATCOMC__
   ShowWindow(_MainWindow,SW_MINIMIZE) ;
#endif /* __WATCOMC__ */
   return ;
}

//----------------------------------------------------------------------

void FrHideWindow()
{
#ifdef __WATCOMC__
   ShowWindow(_MainWindow,SW_HIDE) ;
#endif /* __WATCOMC__ */
   return ;
}

//----------------------------------------------------------------------

void FrDestroyWindow()
{
#ifdef __WATCOMC__
   DestroyWindow(_MainWindow) ;
#endif /* __WATCOMC__ */
   return ;
}

//----------------------------------------------------------------------

void FrSleep(int seconds)
{
   Sleep(1000L*seconds) ;
   return ;
}

//----------------------------------------------------------------------

void Fr_usleep(long microseconds)
{
   long ms = (microseconds + 500) / 1000 ;
   Sleep(ms) ;
   return ;
}

//----------------------------------------------------------------------

#if defined(__WATCOMC__)
extern "C" BOOL _SetAppTitle(const char *title) ;
#endif /* __WATCOMC__ */

void FrSetAppTitle(const char *title)
{
#if defined(__WATCOMC__)
   _SetAppTitle(title) ;
#else
   (void)title ;
#endif /* __WATCOMC__ */
   return ;
}

//----------------------------------------------------------------------

void FrExitProcess(int exitcode)
{
   ExitProcess((UINT)exitcode) ;
   return ;
}

//----------------------------------------------------------------------

unsigned long FrGetCPUTime()
{
   FILETIME create ;
   FILETIME destroy ;
   FILETIME kernel ;
   FILETIME user ;

   if (GetProcessTimes(GetCurrentProcess(),&create,&destroy,&kernel,&user))
      {
      DWORD low = kernel.dwLowDateTime ;
      kernel.dwLowDateTime += user.dwLowDateTime ;
      if (kernel.dwLowDateTime < low)	// overflow?
	 kernel.dwHighDateTime++ ;
      kernel.dwHighDateTime += user.dwHighDateTime ;
      // convert from 10MHz timing to FrTICKS_PER_SEC (10 kHz)
#define convfactor  (10000000L / FrTICKS_PER_SEC)
      if (kernel.dwHighDateTime >= convfactor)
	 return (unsigned long)-1L ;	// overflow!
      else
	 return div64_32(kernel.dwHighDateTime,kernel.dwLowDateTime,
			 convfactor) ;
      }
   else
      {
#if defined(CLOCKS_PER_SEC) && CLOCKS_PER_SEC >= FrTICKS_PER_SEC
      return clock() / (CLOCKS_PER_SEC/FrTICKS_PER_SEC) ;
#elif defined(CLOCKS_PER_SEC) && FrTICKS_PER_SEC > CLOCKS_PER_SEC
      return clock() * (FrTICKS_PER_SEC/CLOCKS_PER_SEC) ;
#else
      return (unsigned long)-1L ;
#endif /* CLOCKS_PER_SEC >= FrTICKS_PER_SEC */
      }
}

//----------------------------------------------------------------------

#else /* !__WINDOWS__ && !__NT__ && !_WIN32 */

#ifdef __WATCOMC__
#  include <dos.h>	   // for sleep()
#elif defined(_MSC_VER)
// nothing extra
#elif defined(__SUNOS__)
#  include <sys/unistd.h>  // for sleep()
#elif defined(unix) || defined(__linux__) || defined(__GNUC__)
#  include <unistd.h>	   // for sleep()
#else
#  include <bios.h>	   // for _bios_timeofday()
#endif /* __WATCOMC__, _MSC_VER, unix, other */
#include "frconfig.h"

//----------------------------------------------------------------------

// non-Windows version of FrSleep()
void FrSleep(int seconds)
{
#if defined(unix) || defined(__linux__) || defined(__GNUC__) || defined(__WATCOMC__)
   sleep(seconds) ;
#else
   long endtime ;
   _bios_timeofday(_TIME_GETCLOCK,&endtime) ;
   endtime += (91L*seconds)/5 ;
   long currtime ;
   do {
      _bios_timeofday(_TIME_GETCLOCK,&currtime) ;
      } while (currtime < endtime) ;
#endif /* unix, __WATCOMC__ */
   return ;
}

//----------------------------------------------------------------------

void Fr_usleep(long microseconds)
{
#if defined(unix) || defined(__linux__) || defined(__GNUC__)
   usleep(microseconds) ;
#else
   long ticks = (microseconds + 27500) / 54925 ;
   long endtime ;
   _bios_timeofday(_TIME_GETCLOCK,&endtime) ;
   endtime += ticks ;
   long currtime ;
   do {
      _bios_timeofday(_TIME_GETCLOCK,&currtime) ;
      } while (currtime < endtime) ;
#endif /* unix */
   return ;
}

//----------------------------------------------------------------------

#endif /* __WINDOWS__ || __NT__ */

// end of file frmswin.cpp //
