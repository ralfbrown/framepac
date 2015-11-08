/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frconfig.h		compile-time configuration		*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,1998,1999,2000,2001,2002,2003,	*/
/*		 2004,2005,2006,2007,2009,2010,2012,2013,2015		*/
/*		 Ralf Brown/Carnegie Mellon University			*/
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

#ifndef __FRCONFIG_H_INCLUDED
#define __FRCONFIG_H_INCLUDED

// note: when building FramepaC, other configuration options are
// available in frmem.h and frpcglbl.h; you should not modify any of these
// files without also recompiling and re-installing FramepaC

/************************************************************************/
/*    Configuration Options						*/
/************************************************************************/

// Checking options
//#define PURIFY	   // app will be built using Purify (some optimizat.
			   //   must be disabled to avoid warnings)
//#define VALGRIND	   // app will be built using valgrind (some optimizat.
			   //   must be disabled to avoid warnings)

// Tradeoffs
//#define FrSAVE_MEMORY	   // reduce memory consumption at expense of speed

// Lisp compatibility options
//#define FrSYMBOL_VALUE   // associate a value (binding) with each symbol
//#define Fr_DEMONS	   // add code to support if-added, etc. demons
#define FrSYMBOL_RELATION  // don't undef unless you will NOT be using frames

// GUI options
//#define FrMOTIF	   // provide Motif UI classes (do not define unless
			   //   you have the Motif libraries and headers)
//#define FrMFC		   // layer Motif-like UI classes on top of Windows MFC
			   //   (not implemented yet)

// multi-threading support
//#define FrMULTITHREAD	   // enable POSIX threads and add thread-safety code
			   //   to memory allocation
//#define FrTHREAD_COLLISIONS  // enable statistics collection for how many
			   // times each mutex permitted an uncontested
			   // entry into a critical section and how many
			   // times it blocked the thread because someone else
			   // already held the lock


// encryption options
//#define FrUSE_SSL	   // use SSLeay or OpenSSL libraries for secure
			   //   network connections in FrSockStream's
			   //   (not implemented yet)

// subsystem-enabling options
#define FrDATABASE	   // include code for VFrames in disk file
//#define FrSERVER	   // include code for VFrames on remote server
#define FrFRAME_ID	   // maintain unique ID number for each vframe
//#define FrEXTRA_INDEXES  // maintain more than just by-name index for DB
//#define FrGC		   // full garbage collection (not implemented yet)

// Memory management options
//#define FrREPLACE_MALLOC   // replace system's malloc() functions with own
			   //   for better speed and better error checking
#define FrMEMORY_CHECKS    // perform consistency checks on all alloc/dealloc
                           //   (only turn off if you need the extra 5% speed)
//#define FrMEMLEAK_CHECKS // check for memory leaks (not implemented yet)
			   //   (slower and uses more memory)
//#define FrMEMWRITE_CHECKS // compile in extra support to check for memory overwrites
			   //  (note: can really slow down program when enabled!)
#define FrLRU_DISCARD	   // discard least-recently used frames if memory
			   //   is exhausted
#define FrMEM_ERRS_FATAL false // are FrFree/FrRealloc errors fatal by default?

#ifdef FrMULTITHREAD
#define FrMEMUSE_STATS   // count total allocation requests (expensive when multi-threaded)
#else
#define FrMEMUSE_STATS	   // count total allocation requests
#endif /* FrMULTITHREAD */

#define FrREPLACE_XTMALLOC // replace Xlib's allocation functions
#define FrSEPARATE_XTMALLOC // put XtMalloc allocs in separate memory pool
//#define FrBUGFIX_XLIB_ALLOC // work around Xlib malloc/XtMalloc misusage

// Compiler-related options
//#define FrANSI_BOOL	     // compiler provides ANSI 'bool' type
//#define FrANSI_NAMESPACE   // compiler supports ANSI 'namespace' feature

// Miscellaneous options
#define FrDEFAULT_CHARSET_Latin1
//#define FrDEFAULT_CHARSET_Latin2

#if !defined(VALGRIND) && !defined(HELGRIND) && !defined(PURIFY)
#  define NVALGRIND
#else
#   define FrMEMWRITE_CHECKS
#endif

//----------------------------------------------------------------------
// MS-DOS specific, to override default assumptions about the MS-DOS environ.
//
//#define FrMSDOS_SOCKETS  // BSD-style socket library available
//#define FrMSDOS_MOTIF	   // Motif library is available

/************************************************************************/
/*	Standard machine-specific types					*/
/************************************************************************/

#define __STDC_LIMIT_MACROS
#include <stdint.h>		// integral types with specified sizes

typedef uintptr_t	FrPTRINT ;	// integer that can hold a pointer

/************************************************************************/
/*	 portability definitions -- Compiler				*/
/************************************************************************/

// make use of predefined platform information macros if available
#if defined(_BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__)
#  if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#    define FrLITTLEENDIAN
#  endif
#endif
#if defined(_BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__)
#  if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#    define FrBIGENDIAN
#  endif
#endif

#ifndef __has_builtin
#  define _has_builtin(x) 0		// compatibility with non-Clang compilers
#endif /* __has_builtin */

#ifndef __has_feature
#  define _has_feature(x) 0		// compatibility with non-Clang compilers
#endif /* __has_feature */

#ifndef __has_extension
#  define _has_extension _has_feature	// compatibility for pre-3.0 Clang
#endif /* __has_extension */

#if __cpluplus >= 201103L
#define _fnattr_noreturn [[noreturn]]
# if defined(__clang__) || __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 80)
#define _fnattr_always_inline [[gnu::always_inline]]
#define _fnattr_cdecl [[gnu::cdecl]]
#define _fnattr_constructor [[gnu::constructor]]
#define _fnattr_destructor [[gnu::destructor]]
#define _fnattr_dllexport [[gnu::dllexport]]
#define _fnattr_dllimport [[gnu::dllimport]]
#define _fnattr_fastcall [[gnu::fastcall]]
#define _fnattr_malloc [[gnu::malloc]]
#define _fnattr_noinline [[gnu::noinline]]
#define _fnattr_nonnull [[gnu::nonnull]]
#define _fnattr_sentinel [[gnu::sentinel]]
#define _fnattr_stdcall [[gnu::stdcall]]
#define _fnattr_unused [[gnu::unused]]
#define _fnattr_used [[gnu::used]
#endif
#endif /* __cplusplus >= 201103L */

#if __cplusplus >= 201400L
#define _fnattr_deprecated [[deprecated]]
#endif

#if defined(__GNUC__)
#  define expected(expr) __builtin_expect(expr,1)
#  define unlikely(expr) __builtin_expect(expr,0)
#  if __GNUC__ >= 3
#    define FrSTRICT_CPLUSPLUS
#  endif
#  if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 82)
#    define FrNEW_THROWS_EXCEPTIONS
#    define FrANSI_BOOL
#endif
#  if __GNUC__ == 2 && __GNUC_MINOR__ < 80
#    define FrBUGFIX_GCC_272  // work around internal compiler err in GCC 2.7.2
#endif
#if defined(_REENTRANT) && !defined(FrMULTITHREAD)
// force thread-safety support if compiled with -pthread compiler option
//#define FrMULTITHREAD
#endif
# ifndef _fnattr_always_inline
# define _fnattr_always_inline __attribute__((__always_inline__))
# endif
# ifndef _fnattr_cdecl
# define _fnattr_cdecl __attribute__((__cdecl__))
# endif
# ifndef _fnattr_constructor
# define _fnattr_constructor __attribute__((__constructor__))
# endif
# ifndef _fnattr_destructor
# define _fnattr_destructor __attribute__((__destructor__))
# endif
# ifndef _fnattr_deprecated
# define _fnattr_deprecated __attribute__((__deprecated__))
# endif
# ifndef _fnattr_dllexport
# define _fnattr_dllexport __attribute__((__dllexport__))
# endif
# ifndef _fnattr_dllimport
# define _fnattr_dllimport __attribute__((__dllimport__))
# endif
# ifndef _fnattr_fastcall
# define _fnattr_fastcall __attribute__((__fastcall__))
# endif
# ifndef _fnattr_malloc
# define _fnattr_malloc __attribute__((__malloc__))
# endif
# ifndef _fnattr_noinline
# define _fnattr_noinline __attribute__((__noinline__))
# endif
# ifndef _fnattr_nonnull
# define _fnattr_nonnull __attribute__((__nonnull__))
# endif
# ifndef _fnattr_sentinel
# define _fnattr_sentinel __attribute__((__sentinel__))
# endif
# ifndef _fnattr_stdcall
# define _fnattr_stdcall __attribute__((__stdcall__))
# endif
# ifndef _fnattr_unused
# define _fnattr_unused __attribute__((__unused__))
# endif
# ifndef _fnattr_used
# define _fnattr_used __attribute__((__used__))
# endif
#define _fnattr_visibility(vis) __attribute__((__visibility(vis__)))
#define _fnattr_warn_unused_result __attribute__((__warn_unused_result__))
#define _fnattr_weak __attribute__((__weak__))
# if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 50)
# define _fnattr_const __attribute__((__const__))
#  ifndef _fnattr_noreturn
#  define _fnattr_noreturn __attribute__((__noreturn__))
#  endif
# endif
#  if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 96)
#define _fnattr_pure __attribute__((__pure__))
#endif
#  if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 30)
#define _fnattr_nothrow __attribute__((__nothrow__))
#endif
#  if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 20)
#define _fnattr_alloc_size1(as) __attribute__((__alloc_size__(as)))
#define _fnattr_alloc_size2(a1,a2) __attribute__((__alloc_size__(a1,a2)))
#endif
#  if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 30)
#define _fnattr_flatten __attribute__((__flatten__))
#define _fnattr_cold __attribute__((__cold__))
#define _fnattr_hot __attribute__((__hot__))
#endif
#endif /* __GNUC__ */

#ifdef FrSTRICT_CPLUSPLUS
#  include <cstdarg>
#  include <sys/types.h>
#  include <limits.h>
#else
#  include <stdarg.h>
#  include <sys/types.h>
#  include <limits.h>
#endif /* FrSTRICT_CPLUSPLUS */

#ifndef __THROW
//#  define __THROW throw ()
#  define __THROW
#endif

#if __cplusplus < 201103L
#define nullptr 0
#endif

/************************************************************************/
/*	 portability definitions -- Platform				*/
/************************************************************************/

// ensure that we have complete feature set on x86

// x86_64 CPUs, e.g. AMD K8 (Athlon64/Opteron) and newest PentiumIV
#if defined(__886__) && !defined(__786__)
#  define FrFAST_MULTIPLY
#  define __786__
#endif

// AMD Athlon (K7)
#if defined(__786__) && !defined(__686__)
#  define __686__
#endif

// Pentium Pro/II/III/IV
#if defined(__686__) && !defined(__586__)
#  define __586__
#endif

// Pentium, Pentium MMX
#if defined(__586__) && !defined(__486__)
#  define __486__
#endif

#if defined(__486__) && !defined(__386__)
#  define __386__
#endif

//------------------------------------------------------------------------
//  definitions for various Unices; edit as required

// RedHat/Fedora need unistd.h in a bunch of cases where others don't
#if defined(__GNUC_RH_RELEASE__)
#  include <unistd.h>
#endif

#if defined(__SPARC__) || defined(__sparc__) || defined(sparc) || defined(__sparclite__)
# if !defined(FrBIGENDIAN) && !defined(FrLITTLEENDIAN)
#  define FrBIGENDIAN
# endif
#  define FrLONG_IS_32BITS
#  define FrMMAP_SUPPORTED
#  define FrHAVE_SRAND48
//#  define FrNEED_MKTEMP		// no mktemp() in runtime lib
#  define FrHAVE_TEMPNAM		// runtime lib includes tempnam()
#  define FrNEED_ULTOA
#  define FrNULL_DEVICE "/dev/null"

#undef __SUNOS__
#undef __SOLARIS__
#if defined(__svr4__) || defined(__svr5__)
#  define __SOLARIS__
   // Solaris headers define wchar_t as a 32-bit type!  But we need 16-bit...
#  define _WCHAR_T
//   typedef unsigned short wchar_t ;
#else
#  define __SUNOS__
#endif

#elif defined(__alpha) || defined(__alpha__) || defined(__mips64)

# if !defined(FrBIGENDIAN) && !defined(FrLITTLEENDIAN)
#  define FrBIGENDIAN
# endif
#  define FrLONG_IS_64BITS
#  define FrMMAP_SUPPORTED
//#  define FrHAVE_SRAND48
//#  define FrNEED_MKTEMP		// no mktemp() in runtime lib
//?#  define FrHAVE_TEMPNAM		// runtime lib includes tempnam()
#  define FrNEED_ULTOA
#  define FrNULL_DEVICE "/dev/null"
#  define CHAR_BITS 8
//#define FrBITS_PER_UINT 32
#define FrBITS_PER_ULONG 64

#elif defined(linux) || defined(__linux__) || defined(__LINUX__)   // Linux x86
#  ifndef __linux__
#    define __linux__
#  endif
#  include <float.h>

# if !defined(FrBIGENDIAN) && !defined(FrLITTLEENDIAN)
#  define FrLITTLEENDIAN
# endif
#if __BITS__ == 64
#  define FrLONG_IS_64BITS
#else
#  define FrLONG_IS_32BITS
#endif /* __BITS__ == 64 */
#  define FrMMAP_SUPPORTED
#  define FrHAVE_SRAND48
//#  define FrNEED_MKTEMP		// no mktemp() in runtime lib
#  define FrHAVE_TEMPNAM		// runtime lib includes tempnam()
#  if __GNUC__ >= 3 || (__GNUC__ == 2 && __GNUC_MINOR_ > 95)
#    define FrHAVE_MKSTEMP		// runtime lib includes mkstemp()
#  endif
#  define FrNEED_ULTOA
#  define FrNEED_TELL
#  define FrOBJECT_VIRTUAL_DTOR		// work around G++ problem
#  define FrNULL_DEVICE "/dev/null"
#  define FrBROKEN_TOWLOWER

#elif defined(_AIX)		// IBM's AIX

# if !defined(FrBIGENDIAN) && !defined(FrLITTLEENDIAN)
#  define FrBIGENDIAN
# endif
#  define FrLONG_IS_32BITS
#  define FrMMAP_SUPPORTED
//#  define FrHAVE_SRAND48
//#  define FrNEED_MKTEMP		// no mktemp() in runtime lib
//?#  define FrHAVE_TEMPNAM		// runtime lib includes tempnam()
#  define FrNEED_ULTOA
#  define FrNULL_DEVICE "/dev/null"
#  define unix

#elif defined(__CYGWIN__)   	// sorta pretends to be Unix, but not quite....

# if !defined(FrBIGENDIAN) && !defined(FrLITTLEENDIAN)
#  define FrLITTLEENDIAN
# endif
#if sizeof(long)*CHAR_BIT == 32
#  define FrLONG_IS_32BITS
#endif /* sizeof(long)*CHAR_BIT */
#  define FrNEED_ULTOA
//#  define FrMMAP_SUPPORTED
//#  define FrHAVE_SRAND48
//#  define FrNEED_MKTEMP		// no mktemp() in runtime lib
//#  define FrHAVE_TEMPNAM		// runtime lib includes tempnam()
#  define FrNULL_DEVICE "/dev/null"

#elif defined(unix) // generic Unix

# if !defined(FrBIGENDIAN) && !defined(FrLITTLEENDIAN)
//#  define FrBIGENDIAN
//#  define FrLITTLEENDIAN
# endif
#if sizeof(long)*CHAR_BIT == 32
#  define FrLONG_IS_32BITS
#endif /* sizeof(long)*CHAR_BIT */
#  define FrNEED_ULTOA
//#  define FrMMAP_SUPPORTED
//#  define FrHAVE_SRAND48
//#  define FrNEED_MKTEMP		// no mktemp() in runtime lib
//#  define FrHAVE_TEMPNAM		// runtime lib includes tempnam()
#  define FrNULL_DEVICE "/dev/null"
#endif /* SPARC / unix */


//------------------------------------------------------------------------
//  definitions for generic 16-bit MS-DOS compilers

#ifdef __MSDOS__
# if !defined(FrBIGENDIAN) && !defined(FrLITTLEENDIAN)
#  define FrLITTLEENDIAN
# endif
#  define FrLONG_IS_32BITS
#  define FrMSDOS_PATHNAMES
#  define FrNULL_DEVICE "/dev/nul"
//#  define FrHAVE_SRAND48
//#  define FrNEED_MKTEMP		// no mktemp() in runtime lib
//?#  define FrHAVE_TEMPNAM		// runtime lib includes tempnam()
#define __FrCDECL _cdecl

// force server support to be disabled unless we've been told that sockets
// are in fact available in this MS-DOS environment
#if defined(FrSERVER) && !defined(FrMSDOS_SOCKETS)
#  undef FrSERVER
#endif

// force Motif interface support to be disabled unless we've been specifically
// told that Motif is available in this MS-DOS environment
#if defined(FrMOTIF) && !defined(FrMSDOS_MOTIF)
#  undef FrMOTIF
#endif

#endif /* __MSDOS__ */

//------------------------------------------------------------------------
// extra definitions for Borland C++ 3.1 (MSDOS)

#if defined(__BORLANDC__)
#  define FrNEED_MKTEMP			// no mktemp() in runtime lib
//?#  define FrHAVE_TEMPNAM		// runtime lib includes tempnam()
#endif /* __BORLANDC__ */

//------------------------------------------------------------------------
// definitions for Watcom C++ v10.0+

#if defined(__WATCOMC__)
# if !defined(FrBIGENDIAN) && !defined(FrLITTLEENDIAN)
#  define FrLITTLEENDIAN
# endif
#  define FrLONG_IS_32BITS
#  define FrMSDOS_PATHNAMES
#  define FrNULL_DEVICE "/dev/nul"
//#  define FrHAVE_SRAND48
#  define FrNEED_MKTEMP			// no mktemp() in runtime lib
//?#  define FrHAVE_TEMPNAM		// runtime lib includes tempnam()
#if __WATCOMC__ >= 1100 && defined(__WATCOM_INT64__)
   typedef signed __int64 int64_t ;
   typedef unsigned __int64 uint6_t ;
#else
#endif /* version >= 11.0 */
#define __FrCDECL _cdecl
#if __WATCOMC__ >= 1100
# define _fnattr_cdecl _cdecl
# define _fnattr_dllexport __declspec(dllexport)
# define _fnattr_dllimport __declspec(dllimport)
#endif /* __WATCOMC__ >= 1100 */

// force server support to be disabled unless we've been told that sockets
// are in fact available in this MS-DOS environment
#if defined(FrSERVER) && !defined(FrMSDOS_SOCKETS) && !defined(__WINDOWS__)
#  undef FrSERVER
#endif

// force Motif interface support to be disabled unless we've been specifically
// told that Motif is available in this MS-DOS environment
#if defined(FrMOTIF) && !defined(FrMSDOS_MOTIF)
#  undef FrMOTIF
#endif

// force use of the ANSI 'bool' type for WC++ 10.6 or higher
#if __WATCOMC__ >= 1060 && !defined(FrANSI_BOOL)
#  define FrANSI_BOOL
#endif

// no ftruncate() in the runtime library, but chsize() is equivalent
#include <io.h>
#define ftruncate chsize

// turn off warnings caused by taking 'sizeof' classes with virtual method
// table pointers, by setting the level so high (10) that the warning
// isn't issued even with the -wx (all warnings) flag
#pragma warning 549 10 ;

// turn off warnings caused by do{}while(0) constructs in winsock.h,
// by setting the level so high (10) that the warning isn't issued even
// with the -wx (all warnings) flag
#pragma warning 555 10 ;

#endif /* __WATCOMC__ */

//------------------------------------------------------------------------
// definitions for Microsoft Visual C++

#if defined(_MSC_VER) && _MSC_VER >= 800

# if !defined(FrBIGENDIAN) && !defined(FrLITTLEENDIAN)
#  define FrLITTLEENDIAN
# endif
#  define FrLONG_IS_32BITS
#  define FrMSDOS_PATHNAMES
#  define FrNULL_DEVICE "/dev/nul"
//#  define FrHAVE_SRAND48
//#  define FrNEED_MKTEMP		// no mktemp() in runtime lib
//?#  define FrHAVE_TEMPNAM		// runtime lib includes tempnam()
#  define __FrCDECL __cdecl

// the MSC libraries have an identifier clash when FramepaC overrides malloc(),
// and the executable doesn't work properly when the link step is forced....
#ifdef FrREPLACE_MALLOC
#  undef FrREPLACE_MALLOC
#endif

// force Motif interface support to be disabled unless we've been specifically
// told that Motif is available in this MS-DOS environment
#if defined(FrMOTIF) && !defined(FrMSDOS_MOTIF)
#  undef FrMOTIF
#endif

#ifdef _WINDOWS
#  define __WINDOWS__
#endif

#ifdef _WIN32
#  define __NT__
#endif

// disable the Visual C++ warning about unreferenced inline functions
// ditto for unexpanded inline functions
#pragma warning(disable : 4514 4710)
// turn off Visual C++'s unreachable-code warning which is erroneously emitted
// for any 'new' of any class using FrAllocator for inline suballocations in
// the class's operator new
#pragma warning(disable : 4702)
// disable the Visual C++ warning about nameless struct/unions, which their
// own header files use....; ditto for the 'constant conditional expr.'
// warning caused by do{}while(0);
#pragma warning(disable : 4201 4127)

#endif /* _MSC_VER && _MSC_VER >= 800 */

/************************************************************************/
/*	 portability definitions -- OS					*/
/************************************************************************/

#if defined(__MSDOS__) || defined(__WINDOWS__) || defined(__NT__)
#  define FrFOPEN_READ_MODE 	"rb"
#  define FrFOPEN_WRITE_MODE 	"wb"
#  define FrFOPEN_UPDATE_MODE 	"rb+"
#  define FrFOPEN_APPEND_MODE	"ab"
#else
#  define FrFOPEN_READ_MODE 	"r"
#  define FrFOPEN_WRITE_MODE 	"w"
#  define FrFOPEN_UPDATE_MODE 	"r+"
#  define FrFOPEN_APPEND_MODE	"a"
#endif /* __MSDOS__ || __WINDOWS__ || __NT__ */
#  define FrMKDIR_MODE		0777

/************************************************************************/
/*    Size Limits							*/
/************************************************************************/

// maximum lengths of the ASCII representation of a number to be read
// and of a symbol name
#define FrMAX_NUMSTRING_LEN 255
#define FrMAX_SYMBOLNAME_LEN 255

// maximum length of ASCII decimal representation for an unsigned long
// 10 digits will hold 2^33, 15 digits will hold 2^49, and 20 digits will
// hold 2^66
#ifdef FrLONG_IS_64BITS
# define FrMAX_ULONG_STRING 20
#else
# define FrMAX_ULONG_STRING 10
#endif

// maximum length of ASCII decimal representation for a 64-bit integer
#define FrMAX_INT64_STRING 20

// maximum length of ASCII decimal representation for a double
#define FrMAX_DOUBLE_STRING 60

/************************************************************************/
//! Note: it should not be necessary to change anything below this     !//
//! point in the header file					       !//

#ifdef FrMULTITHREAD
#  define FrPER_THREAD __thread
#else
#  define FrPER_THREAD
#endif

/************************************************************************/
/*    Ensure all prerequisites are met for various options		*/
/************************************************************************/

// why bother replacing XtMalloc if we're not using Motif?
#if !defined(FrMOTIF) && defined(FrREPLACE_XTMALLOC)
#  undef FrREPLACE_XTMALLOC
#endif /* !FrMOTIF && FrREPLACE_XTMALLOC */

// can't use separate memory pool unless replacing XtMalloc
#if !defined(FrREPLACE_XTMALLOC) && defined(FrSEPARATE_XTMALLOC)
#  undef FrSEPARATE_XTMALLOC
#endif /* !FrREPLACE_XTMALLOC && FrSEPARATE_XTMALLOC */

// don't need workaround unless putting XtMalloc in separate memory pool
#if !defined(FrSEPARATE_XTMALLOC) && defined(FrBUGFIX_XLIB_ALLOC)
#  undef FrBUGFIX_XLIB_ALLOC
#endif /* !FrSEPARATE_XTMALLOC && FrBUGFIX_XLIB_ALLOC */

// no need for extra indices if we aren't using disk backing store at all
#if !defined(FrDATABASE) && defined(FrEXTRA_INDEXES)
#  undef FrEXTRA_INDEXES
#endif /* !FrDATABASE && FrEXTRA_INDEXES */

// Purify gets upset if we use our own malloc()
#if defined(PURIFY)
#  undef FrREPLACE_MALLOC
#endif /* PURIFY */

// ThreadSanitizer doesn't play well with FrMalloc()
#if defined(DYNAMIC_ANNOTATIONS_ENABLED)
#  if DYNAMIC_ANNOTATIONS_ENABLED == 1
#    undef FrREPLACE_MALLOC
#  endif
#endif /* DYNAMIC_ANNOTATIONS_ENABLED */

//  __FrCDECL *must* be defined, even if it is just the null string
#if !defined(__FrCDECL)
#define __FrCDECL
#endif

#if !defined(FrBITS_PER_UINT)
#  if UINT_MAX == 0xFFFFFFFFUL
#    define FrBITS_PER_UINT 32
#  elif UINT_MAX == 0xFFFFU
#    define FrBITS_PER_UINT 16
#  elif UINT_MAX == 0xFFFFFFFFFFFFFFFFUL
#    define FrBITS_PER_UINT 64
#  elif defined(CHAR_BITS)
#    define FrBITS_PER_UINT (sizeof(unsigned int)*CHAR_BITS)
#  else
#    error Must define FrBITS_PER_UINT in frconfig.h!
#  endif /* UINT_MAX */
#endif /* !FrBITS_PER_UINT */

#if !defined(FrBITS_PER_ULONG)
#  if ULONG_MAX == 0xFFFFFFFFUL
#    define FrBITS_PER_ULONG 32
#  elif ULONG_MAX == 0xFFFFFFFFFFFFFFFFUL
#    define FrBITS_PER_ULONG 64
#  elif defined(CHAR_BITS)
#    define FrBITS_PER_ULONG (sizeof(unsigned long)*CHAR_BITS)
#  else
#    error Must define FrBITS_PER_ULONG in frconfig.h!
#  endif /* ULONG_MAX */
#endif /* !FrBITS_PER_ULONG */

#ifndef FrGPL
// a few minor features are not available in the LGPL version of FramepaC
#ifdef FrEXTRA_INDEXES
#  error FrEXTRA_INDEXES not supported in the LGPL version of FramepaC
#endif

#endif /* !FrGPL */

/************************************************************************/
/*    Ensure that options implied by other options are actually ON	*/
/************************************************************************/

#if defined(PURIFY) || defined(VALGRIND)
//   if we're using Purify or Valgrind, we're trying to find problems, so
//   turn on extra checking
#  ifndef FrMEMORY_CHECKS
#  define FrMEMORY_CHECKS
#  endif
#  ifndef FrMEMWRITE_CHECKS
#  define FrMEMWRITE_CHECKS
#  endif
#  ifndef FrMEMUSE_STATS
#  define FrMEMUSE_STATS
#  endif
#endif /* PURIFY || VALGRIND */

/************************************************************************/
/*	Fixes for broken vendor header files				*/
/************************************************************************/

#if defined(__WATCOMC__) && __WATCOMC__ < 1100
extern "C" {
int gethostname(char *,int) ;
}
#endif /* __WATCOMC__ */

#ifdef __SUNOS__
#include <stdio.h>
extern "C" {
int accept(int, struct sockaddr *, int *) ;
void bzero(char *,int) ;
int bind(int, struct sockaddr *, int);
int connect(int, struct sockaddr *, int);
int close(int /*fd*/) ;
char *crypt(char *,char *) ;
void exit(int) ;
int fsync(int /*fd*/) ;
int ftruncate(int /*fd*/, long) ;
int gethostname(char *,int) ;
int getpeername(int, struct sockaddr *, int *);
int getsockname(int, struct sockaddr *, int *);
int ioctl(int /*s*/, int /*func*/, void * /*arg*/) ;
int listen(int, int);
int on_exit(void (*procp)(int,caddr_t), caddr_t arg) ;
int recvmsg(int, struct msghdr *, int);
int shutdown(int sock, int how) ;
int socket(int, int, int);
int socketpair(int, int, int, int *);
// int select(int,fd_set*,fd_set*,fd_set*,timeval*);
void usleep(unsigned);
long ulimit();
int lockf(int /*fd*/, int /*operation*/, long /*size*/) ;
void *memchr(const void *s, int c, size_t n) ;
void *memset(const void *s, int c, size_t n) ;
void memmove(void *, const void *, size_t) ;
int getsockopt(int, int, int, void *optval, int *);
int setsockopt(int, int, int, void *optval, int);
int recv(int, void *, int, int);
int recvfrom(int, void *, int, int, struct sockaddr *, int *);
int send(int, void *, int, int);
int sendto(int, void *, int, int, struct sockaddr *, int);
int sendmsg(int, struct msghdr *, int);
int setvbuf(FILE *, char * /*buf*/, int /*mode*/, size_t /*size*/) ;
int sigvec(int, struct sigvec *, struct sigvec *) ;
const char *strstr(const char * /*string1*/, const char * /*string2*/) ;
long strtol(const char * /*string*/, char ** /*end*/, int /*radix*/) ;
long tell(int /*fd*/) ;
int unlink(const char *) ;
int rename(const char *, const char *) ;
int mkdir(const char *dirname, int mode) ;
char *mktemp(char *tmplate) ;
int select(int size,fd_set *read, fd_set *write, fd_set *exc, timeval *limit) ;
caddr_t sbrk(int incr) ;
int vsprintf(char *s, const char *format, va_list arg) ;
int getrusage(int, struct rusage *) ;
}
#endif /* __SUNOS__ */

#ifdef __SOLARIS__
extern "C" {
int access(const char *, int) ;
int accept(int, struct sockaddr *, int *) ;
int bind(int, struct sockaddr *, int);
int connect(int, struct sockaddr *, int);
int close(int /*fd*/) ;
int eof(int /*fd*/) ;
int fsync(int /*fd*/) ;
int ftruncate(int /*fd*/, long) ;
int gethostname(char *,int) ;
int getpeername(int, struct sockaddr *, int *);
int getsockname(int, struct sockaddr *, int *);
int listen(int, int);
int lockf(int /*fd*/, int /*operation*/, long /*size*/) ;
int recvmsg(int, struct msghdr *, int);
int shutdown(int sock, int how) ;
int socket(int, int, int);
int socketpair(int, int, int, int *);
//int select(int,fd_set*,fd_set*,fd_set*,timeval*);
long tell(int /*fd*/) ;
void usleep(unsigned);
long ulimit();
int unlink(const char *) ;
int rename(const char *, const char *) ;
char *mktemp(char *tmplate) ;
unsigned sleep(unsigned) ;
}
#endif /* __SOLARIS__ */

#if defined(__alpha__) || defined(__alpha)
extern "C" {
int close(int /*fd*/) ;
//caddr_t sbrk(int incr) ;
long tell(int fd) ;
}
#endif /* __alpha__ */

#ifdef __linux__
extern "C" {
//int gethostname(char *,int) ;
//!note from Tom Ault: some versions of Linux have unsigned second arg on mkdir
//!if you get a redeclaration error, simply comment out the declaration here
//int mkdir(const char *dirname, int mode) ;
//caddr_t sbrk(int incr) ;
long tell(int fd) ;
#if __GNUC__ < 2 || (__GNUC__ == 2 && __GNUC_MINOR__ <= 95)
  int access(const char *, int) ;
  int close(int /*fd*/) ;
  int vsprintf(char *s, const char *format, va_list arg) ;
#endif
#if __GNUC__ == 3 && __GNUC_MINOR__ < 10
  int eof(int /*fd*/) ;
#endif /* GCC 3.00-3.10 */
}
#endif /* __linux__ */

/************************************************************************/
/*	fixes for missing functions					*/
/************************************************************************/

#ifdef FrNEED_TELL
#  define fdtell(fd) lseek(fd,0,SEEK_CUR)
#else
#  define fdtell(fd) tell(fd)
#endif /* FrNEED_TELL */

/************************************************************************/
/*	compatibility functions for vararg lists			*/
/************************************************************************/

#if defined(va_copy)
#  define FrCopyVAList(d,s) va_copy(d,s)
#elif defined(__va_copy)
#  define FrCopyVAList(d,s) __va_copy(d,s)
#elif defined(__WATCOMC__)
struct _Fr_va_list_struct
   {
   va_list arglist ;
   } ;

// Watcom C defines va_list as an array of length 1, which is what allows the
// definition below to work properly (otherwise, we'd have to use &v1)
// -- unfortunately, that's also what makes all this necessary, since accessing
// variable args inside a function changes the vararg list in the caller!  Bad
// news if the caller makes repeated calls expecting to pass the same args....
#  define FrCopyVAList(v1,v2) \
   { (*(_Fr_va_list_struct*)v1) = (*(_Fr_va_list_struct*)v2) ; }
#endif

#ifdef FrCopyVAList
#  define FrSafeVAList(valist) \
     va_list valist##_copy_ ; \
     FrCopyVAList(valist##_copy_,valist) ;
#  define FrSafeVarArgs(valist) valist##_copy_
#  define FrSafeVAListEnd(valist) va_end(valist##_copy_)
#else
#  define FrCopyVAList(d,s) ((d) = (s))
#  define FrSafeVAList(valist)
#  define FrSafeVarArgs(valist) valist
#  define FrSafeVAListEnd(valist)
#endif

// a useful bit of syntactic sugar if you've declared your function's
//   variable arguments as 'va_list args':
#define FrVarArg(type,varname) type varname = va_arg(args,type)
// another variant for use when the type you actually want gets promoted
//   while passing through ... (GCC 2.96+)
#define FrVarArg2(type,promoted,varname) \
	type varname = (type)va_arg(args,promoted)

// if the functions formal parameters use "...", the following utility
// macro will set things up for use with FrVarArg
#define FrVarArgs(funcparam) \
   va_list args ; \
   va_start(args,funcparam) ;

// and, for symmetry, clean up after a FrVarArgs():
#define FrVarArgEnd() va_end(args)

/************************************************************************/
/************************************************************************/

// empty class for use in hash tables that don't require an associated value
class FrNullObject
   {
   public:
      FrNullObject(int) {}
      operator bool() { return false ; }
   } ;

/************************************************************************/
/*	Conditional-Compilation Macros					*/
/************************************************************************/

#ifdef DEBUG
#  define if_DEBUG(x) x
#  define if_DEBUG_else(x,y) x
#else
#  define if_DEBUG(x)
#  define if_DEBUG_else(x,y) y
#endif /* DEBUG */

/************************************************************************/
/*	Compiler Optimization hints					*/
/************************************************************************/

// make unsupported function attributes disappear
#ifndef _fnattr_alloc_size1
#define _fnattr_alloc_size1(as)
#endif

#ifndef _fnattr_alloc_size2
#define _fnattr_alloc_size2(a1,a2)
#endif

#ifndef _fnattr_always_inline
# define _fnattr_always_inline
#endif

#ifndef _fnattr_cdecl
# define _fnattr_cdecl
#endif

#ifndef _fnattr_cold
# define _fnattr_cold
#endif

#ifndef _fnattr_const
# define _fnattr_const
#endif

#ifndef _fnattr_constructor
# define _fnattr_constructor
#endif

#ifndef _fnattr_destructor
# define _fnattr_destructor
#endif

#ifndef _fnattr_deprecated
# define _fnattr_deprecated
#endif

#ifndef _fnattr_dllexport
# define _fnattr_dllexport
#endif

#ifndef _fnattr_dllimport
# define _fnattr_dllimport
#endif

#ifndef _fnattr_fastcall
# define _fnattr_fastcall
#endif

#ifndef _fnattr_flatten
# define _fnattr_flatten
#endif

#ifndef _fnattr_hot
# define _fnattr_hot
#endif

#ifndef _fnattr_malloc
# define _fnattr_malloc
#endif

#ifndef _fnattr_noinline
# define _fnattr_noinline
#endif

#ifndef _fnattr_nonnull
# define _fnattr_nonnull
#endif

#ifndef _fnattr_noreturn
# define _fnattr_noreturn
#endif

#ifndef _fnattr_nothrow
# define _fnattr_nothrow
#endif

#ifndef _fnattr_pure
# define _fnattr_pure
#endif

#ifndef _fnattr_sentinel
# define _fnattr_sentinel
#endif

#ifndef _fnattr_stdcall
# define _fnattr_stdcall
#endif

#ifndef _fnattr_unused
# define _fnattr_unused
#endif

#ifndef _fnattr_used
# define _fnattr_used
#endif

#ifndef _fnattr_visibility
# define _fnattr_visibility(x)
#endif

#ifndef _fnattr_warn_unused_result
# define _fnattr_warn_unused_result
#endif

#ifndef _fnattr_weak
# define _fnattr_weak
#endif

#ifndef expected
#define expexted(x) x
#endif

#ifndef unlikely
#define unlikely(x) x
#endif

/************************************************************************/
/************************************************************************/

#endif /* !__FRCONFIG_H_INCLUDED */

// end of file frconfig.h //
