/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frmmap.cpp		memory-mapped files			*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1997,2000,2001,2004,2007,2009,2012,2015		*/
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

#if defined(unix) || defined(__linux__) || defined(__GNUC__)
#  include <fcntl.h>
#  include <stdio.h>
#  include <sys/types.h>
#  include <sys/mman.h>
#  include <unistd.h>
#elif defined(__WINDOWS__) || defined(__NT__)
#  include "frconfig.h"
#  include <windows.h>
#  include "framerr.h"
#endif /* unix, Windows||NT */
#include "frassert.h"
#include "frmmap.h"
#include "frpcglbl.h"
#include "memcheck.h"

#ifdef __WATCOMC__
typedef void *caddr_t ;
#endif /* __WATCOMC__ */

#ifndef MAP_NORESERVE
#define MAP_NORESERVE 0
#endif

/************************************************************************/
/*									*/
/************************************************************************/

class FrFileMapping
   {
   public:
      caddr_t map_address ;
      size_t map_length ;
#if defined(unix) || defined(__linux__) || defined(__GNUC__)
      // nothing else needed
#elif defined(__WINDOWS__) || defined(__NT__)
      HANDLE hMap ;
#endif /* unix, Windows||NT */
   public:
      FrFileMapping() { map_address = 0 ; map_length = 0 ; }
   } ;

/************************************************************************/
/*									*/
/************************************************************************/

FrFileMapping *FrMapFile(const char *filename, FrMapMode mode)
{
   assert(mode==FrM_READONLY || mode==FrM_READWRITE || mode==FrM_COPYONWRITE) ;
   if (!filename || !*filename)
      return 0 ;
   FrFileMapping *fmap = new FrFileMapping ;
   if (!fmap)
      return 0 ;
#if defined(unix) || defined(__linux__) || defined(__GNUC__)
   int fmode = (mode==FrM_READWRITE) ? O_RDWR : O_RDONLY ;
   int fd = open(filename,fmode) ;
   if (fd != EOF)
      {
      size_t len = lseek(fd,0L,SEEK_END) ;
      lseek(fd,0L,SEEK_SET) ;
      int mapmode = (mode==FrM_READONLY) ? PROT_READ : PROT_READ | PROT_WRITE ;
      int mapflags = (mode!=FrM_COPYONWRITE) ? MAP_SHARED
					     : MAP_PRIVATE | MAP_NORESERVE ;
      fmap->map_address = (caddr_t)mmap(0,len,mapmode,mapflags,fd,0) ;
      fmap->map_length = len ;
      close(fd) ;
      if (fmap->map_address == MAP_FAILED)
	 fmap->map_address = 0 ;
      else
	 {
	 (void)VALGRIND_MAKE_MEM_DEFINED(fmap->map_address,fmap->map_length) ;
	 }
      }
#elif defined(__WINDOWS__) || defined(__NT__)
   DWORD fmode = GENERIC_READ ;
   if (mode == FrM_READWRITE)
      fmode |= GENERIC_WRITE ;
   DWORD sharemode = FILE_SHARE_READ ;
   if (mode != FrM_READWRITE)
      sharemode |= FILE_SHARE_WRITE ;
   HANDLE hFile = CreateFile(filename,fmode,sharemode,0,OPEN_EXISTING,
			     FILE_ATTRIBUTE_READONLY,0) ;
   fmap->hMap = 0 ;
   if (hFile && hFile != INVALID_HANDLE_VALUE)
      {
      DWORD mapmode ;
      DWORD protect ;
      switch (mode)
	 {
	 case FrM_READONLY:
	    mapmode = FILE_MAP_READ ;
	    protect = PAGE_READONLY ;
	    break ;
	 case FrM_READWRITE:
	    mapmode = FILE_MAP_WRITE ;
	    protect = PAGE_READWRITE ;
	    break ;
	 case FrM_COPYONWRITE:
	    mapmode = FILE_MAP_COPY ;
	    protect = PAGE_WRITECOPY ;
	    break ;
	 default:
	    mapmode = FILE_MAP_READ ;
	    protect = PAGE_READONLY ;
	    FrProgError("invalid mapping mode given to FrMapFile") ;
	 }
      fmap->hMap = CreateFileMapping(hFile,0,protect,0,0,0) ;
      if (fmap->hMap)
	 {
	 fmap->map_address = MapViewOfFile(fmap->hMap,mapmode,0,0,0) ;
	 if (fmap->map_address)
	    fmap->map_length = SetFilePointer(hFile,0L,NULL,FILE_END) ;
	 if (!fmap->map_address || fmap->map_length == 0xFFFFFFFF)
	    {
	    CloseHandle(fmap->hMap) ;
	    fmap->hMap = 0 ;
	    fmap->map_address = 0 ;
	    fmap->map_length = 0 ;
	    }
	 }
      CloseHandle(hFile) ;
      }
   else if (GetLastError() == ERROR_SHARING_VIOLATION)
      FrMessage("unable to memory-map file -- sharing violation") ;
#else
	// no mmap....
#endif /* unix , Windows/NT , other */
   if (!fmap->map_address)
      {
      delete fmap ;
      return 0 ;
      }
   FramepaC_num_memmaps++ ;
   FramepaC_total_memmap_size += fmap->map_length ;
   return fmap ;
}

//----------------------------------------------------------------------

size_t FrMappingSize(const FrFileMapping *fmap)
{
   return fmap ? fmap->map_length : 0 ;
}

//----------------------------------------------------------------------

void *FrMappedAddress(const FrFileMapping *fmap)
{
   if (fmap)
      return fmap->map_address ;
  else
      return 0 ;
}

//----------------------------------------------------------------------

static int dummy_counter = 0 ;

void FrTouchMappedMemory(FrFileMapping *fmap)
{
   if (fmap)
      {
      char *mapped = (char*)fmap->map_address ;
      size_t size = fmap->map_length ;
      FrWillNeedMemory(mapped,size) ;
      size_t count = 0 ;
      char *end = mapped + size ;
      // code to touch the entire memory-mapped file to force it into RAM
      // some of the convolution is to prevent the compiler from optimizing
      //   away the entire code
      for ( ; mapped < end ; mapped += 1024)
	 count += *mapped ;
      dummy_counter = count ;
      }
   return ;
}

//----------------------------------------------------------------------

bool FrSyncMappedFile(FrFileMapping *fmap)
{
   if (!fmap)
      return false ;
   bool success = true ;
#if defined(unix) || defined(__linux__) || defined(__GNUC__)
   success = (msync(fmap->map_address,fmap->map_length,MS_SYNC) == 0) ;
#elif defined(__WINDOWS__) || defined(__NT__)
   (void)FlushViewOfFile((LPVOID)fmap->map_address,0) ;
#endif /* unix, Windows||NT */
   return success ;
}

//----------------------------------------------------------------------

bool FrUnmapFile(FrFileMapping *fmap)
{
   if (!fmap)
      return false ;
   bool success = true ;
#if defined(unix) || defined(__linux__) || defined(__GNUC__)
   success = (munmap(fmap->map_address,fmap->map_length) == 0) ;
   (void)VALGRIND_MAKE_MEM_NOACCESS(fmap->map_address,fmap->map_length) ;
#elif defined(__WINDOWS__) || defined(__NT__)
   (void)FlushViewOfFile((LPVOID)fmap->map_address,0) ;
   if (!UnmapViewOfFile((LPVOID)fmap->map_address))
      success = false ;
   CloseHandle(fmap->hMap) ;
#endif /* unix, Windows||NT */
   FramepaC_num_memmaps-- ;
   FramepaC_total_memmap_size -= fmap->map_length ;
   delete fmap ;
   return success ;
}

//----------------------------------------------------------------------

bool FrAdviseMemoryUse(void *start, size_t length, FrMemUseAdvice advice)
{
   if (start == 0 || length == 0)
      return false ;
#if defined(__USE_BSD)
   int adv ;
   switch (advice)
      {
      case FrMADV_NORMAL:	adv = MADV_NORMAL ; break ;
      case FrMADV_RANDOM:	adv = MADV_RANDOM ; break ;
      case FrMADV_SEQUENTIAL:	adv = MADV_SEQUENTIAL ; break ;
      default:			adv = MADV_NORMAL ; break ;
      }
   return madvise(start, length, adv) == 0 ;
#elif defined(__USE_XOPEN2K)
   int adv ;
   switch (advice)
      {
      case FrMADV_NORMAL:	adv = POSIX_MADV_NORMAL ; break ;
      case FrMADV_RANDOM:	adv = POSIX_MADV_RANDOM ; break ;
      case FrMADV_SEQUENTIAL:	adv = POSIX_MADV_SEQUENTIAL ; break ;
      default:			adv = POSIX_MADV_NORMAL ; break ;
      }
   return posix_madvise(start, length, adv) == 0 ;
#else
   (void)advice;
   return false ;
#endif
}

//----------------------------------------------------------------------

bool FrAdviseMemoryUse(FrFileMapping *fmap, FrMemUseAdvice advice)
{
   if (fmap)
      return FrAdviseMemoryUse(fmap->map_address, fmap->map_length, advice) ;
   else
      return false ;
}

//----------------------------------------------------------------------

bool FrWillNeedMemory(void *memory, size_t length)
{
#if defined(__USE_BSD)
   return madvise(memory, length, MADV_WILLNEED) == 0 ;
#elif defined(__USE_XOPEN2K)
   return posix_madvise(memory, length, POSIX_MADV_WILLNEED) == 0 ;
#else
   (void)memory; (void)length;
   return true ;
#endif
}

//----------------------------------------------------------------------

bool FrDontNeedMemory(void *memory, size_t length, bool never_again)
{
   (void)never_again ;
#if defined(__USE_BSD)
#  ifdef MADV_FREE
   if (never_again)
      return madvise(memory, length, MADV_FREE) == 0 ;
   else
#endif /* MADV_FREE */
      return madvise(memory, length, MADV_DONTNEED) == 0 ;
#elif defined(__USE_XOPEN2K)
   return posix_madvise(memory, length, POSIX_MADV_DONTNEED) == 0 ;
#else
   (void)memory; (void)length;
   return true ;
#endif
}

// end of file frmmap.cpp //

