/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frcritsec.h		short-duration critical section mutex	*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 2010,2013,2015 Ralf Brown/Carnegie Mellon University	*/
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

#ifndef __FRCRITSEC_H_INCLUDED
#define __FRCRITSEC_H_INCLUDED

#include "frconfig.h"
#ifdef __GNUC__
# pragma GCC diagnostic ignored "-Wunused-parameter"
//# pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#endif /* __GNUC__*/
#if DYNAMIC_ANNOTATIONS_ENABLED == 1
#  include "dynamic_annotations.h"
#else
#  include "helgrind.h"
#endif
#ifdef __GNUC__
# pragma GCC diagnostic warning "-Wunused-parameter"
//# pragma GCC diagnostic warning "-Wunused-but-set-variable"
#endif /* __GNUC__*/

#if __cplusplus >= 201103L && 1
#  include <atomic>
#elif __GNUC__ >= 4 && !defined(HELGRIND)
   // use Gnu-specific intrinsics
#else
#  include <pthread.h>
#endif

#include <stdlib.h>

using namespace std ;

/************************************************************************/
/************************************************************************/

void FrThreadYield() ;

/************************************************************************/
/************************************************************************/

#if __cplusplus < 201103L
namespace std {
enum memory_order {
   memory_order_relaxed,
   memory_order_consume,
   memory_order_acquire,
   memory_orderr_release,
   memory_order_acq_rel,
   memory_order_seq_cst
} ;

template <typename T>
struct atomic
   {
   private:
      T value ;

   public:
      atomic(const T& v) { store(v) ; }
      ~atomic() {}
      T operator = (T v) { store(v) ; return v ; }
      T operator = (T v) volatile { store(v) ; return v ; }
//      atomic &operator = (const atomic& v) { store(v) ; return v ; }
//      atomic &operator = (const atomic& v) volatile { store(v) ; return v ; }
      bool is_lock_free() const { return true ; }
      bool is_lock_free() const volatile { return true ; }

      operator T() const { return load() ; }
      operator T() const volatile { return load() ; }

      T operator++() { return fetch_add((T)1) + 1 ; }
      T operator++() volatile { return fetch_add((T)1) + 1 ; }

      T operator++(int) { return fetch_add((T)1) ; }
      T operator++(int) volatile { return fetch_add((T)1) ; }

      T operator--() { return fetch_sub((T)1) - 1 ; }
      T operator--() volatile { return fetch_sub((T)1) - 1 ; }

      T operator--(int) { return fetch_sub((T)1) ; }
      T operator--(int) volatile { return fetch_sub((T)1) ; }

      T operator+=(T incr) { return fetch_add(incr) + incr ; }
      T operator+=(T incr) volatile { return fetch_add(incr) + incr ; }

      T operator-=(T decr) { return fetch_sub(decr) - decr ; }
      T operator-=(T decr) volatile { return fetch_sub(decr) - decr ; }

      T operator&=(T mask) { return fetch_and(mask) & mask ; }
      T operator&=(T mask) volatile { return fetch_and(mask) & mask ; }

      T operator|=(T mask) { return fetch_or(mask) | mask ; } 
      T operator|=(T mask) volatile { return fetch_or(mask) | mask ; }

      T operator^=(T mask) { return fetch_xor(mask) ^ mask ; }
      T operator^=(T mask) volatile { return fetch_xor(mask) ^ mask ; }

#ifndef FrMULTITHREAD
      void store(const T v, std::memory_order /*mo*/ = std::memory_order_seq_cst) { value = v ; }
      void store(const T v, std::memory_order /*mo*/ = std::memory_order_seq_cst) volatile { value = v ; }

      T load(std::memory_order /*mo*/ = std::memory_order_seq_cst) const { return value ; }
      T load(std::memory_order /*mo*/ = std::memory_order_seq_cst) const volatile { return value ; }

      T exchange(const T newval, std::memory_order /*mo*/ = std::memory_order_seq_cst)
	 {
	    T oldval = value ;
	    value = newval ;
	    return oldval ;
	 }
      T exchange(const T newval, std::memory_order /*mo*/ = std::memory_order_seq_cst) volatile
	 {
	    T oldval = value ;
	    value = newval ;
	    return oldval ;
	 }

      bool compare_exchange_strong( T& expected, T desired,
				    std::memory_order /*mo*/ = std::memory_order_seq_cst)
	 {
	    bool match = (value == expected) ;
	    if (match) value = desired ;
	    return match ;
	 }
      bool compare_exchange_strong( T& expected, T desired,
				    std::memory_order /*mo*/ = std::memory_order_seq_cst) volatile
	 {
	    bool match = (value == expected) ;
	    if (match) value = desired ;
	    return match ;
	 }

      T fetch_add( T incr, std::memory_order /*mo*/ = std::memory_order_seq_cst)
	 { T oldval = value ; value += incr ; return oldval ; }
      T fetch_add( T incr, std::memory_order /*mo*/ = std::memory_order_seq_cst) volatile
	 { T oldval = value ; value += incr ; return oldval ; }

     //      T *fetch_add( std::ptrdiff_t incr, std::memory_order /*mo*/ = std::memory_order_seq_cst) ;
     //      T *fetch_add( std::ptrdiff_t incr, std::memory_order /*mo*/ = std::memory_order_seq_cst) volatile ;

      T fetch_sub( T decr, std::memory_order /*mo*/ = std::memory_order_seq_cst)
	 { T oldval = value ; value -= decr ; return oldval ; }
      T fetch_sub( T decr, std::memory_order /*mo*/ = std::memory_order_seq_cst) volatile
	 { T oldval = value ; value -= decr ; return oldval ; }

     //      T *fetch_sub( std::ptrdiff_t incr, std::memory_order /*mo*/ = std::memory_order_seq_cst) ;
     //      T *fetch_sub( std::ptrdiff_t incr, std::memory_order /*mo*/ = std::memory_order_seq_cst) volatile ;

      T fetch_and( T mask, std::memory_order /*mo*/ = std::memory_order_seq_cst)
	 { T oldval = value ; value &= mask ; return oldval ; }
      T fetch_and( T mask, std::memory_order /*mo*/ = std::memory_order_seq_cst) volatile
	 { T oldval = value ; value &= mask ; return oldval ; }

      T fetch_or( T mask, std::memory_order /*mo*/ = std::memory_order_seq_cst)
	 { T oldval = value ; value |= mask ; return oldval ; }
      T fetch_or( T mask, std::memory_order /*mo*/ = std::memory_order_seq_cst) volatile
	 { T oldval = value ; value |= mask ; return oldval ; }

      T fetch_xor( T mask, std::memory_order /*mo*/ = std::memory_order_seq_cst)
	 { T oldval = value ; value ^= mask ; return oldval ; }
      T fetch_xor( T mask, std::memory_order /*mo*/ = std::memory_order_seq_cst) volatile
	 { T oldval = value ; value ^= mask ; return oldval ; }

#elif defined(__GNUC__) && __GNUC__ >= 4 && defined(__386__)
      void store(const T v, std::memory_order /*mo*/ = std::memory_order_seq_cst)
	 {
	    // on x86, "xchg" may be faster than a store followed by a memory fence!
	    T v2 ;
	    __asm__ __volatile__ ("## ATOMIC STORE\n\t"
				  "xchg %0,%1"
				  : "=r" (v2)
				  : "m" (*(volatile T*)&value), "0" (v)
				  : "memory") ;
	    (void)v2 ;
	    return ;
	 }
      void store(const T v, std::memory_order /*mo*/ = std::memory_order_seq_cst) volatile
	 {
	    // on x86, "xchg" may be faster than a store followed by a memory fence!
	    T v2 ;
	    __asm__ __volatile__ ("## ATOMIC STORE\n\t"
				  "xchg %0,%1"
				  : "=r" (v2)
				  : "m" (*(volatile T*)&value), "0" (v)
				  : "memory") ;
	    (void)v2 ;
	    return ;
	 }

      T load(std::memory_order /*mo*/ = std::memory_order_seq_cst) const
	 {
	    T currvalue ;
	    __asm__ __volatile__ ("## ATOMIC LOAD\n\t"
				  //?"lfence\n\t"
				  "mov %1,%0"
				  : "=r" (currvalue)
				  : "m" (*(volatile T*)&value)
				  : "memory") ;
	    return currvalue ;
	 }
      T load(std::memory_order /*mo*/ = std::memory_order_seq_cst) const volatile
	 {
	    T currvalue ;
	    __asm__ __volatile__ ("## ATOMIC LOAD\n\t"
				  //?"lfence\n\t"
				  "mov %1,%0"
				  : "=r" (currvalue)
				  : "m" (*(volatile T*)&value)
				  : "memory") ;
	    return currvalue ;
	 }

      T exchange(const T newval, std::memory_order /*mo*/ = std::memory_order_seq_cst)
	 {
	    T oldvalue ;
	    __asm__ __volatile__ ("## ATOMIC EXCHANGE\n\t"
				  "lock; xchg %1,%0"
				  : "=r" (oldvalue)
				  : "m" (*(volatile T*)&value), "0" (newval)
				  : "memory") ;
	    return oldvalue ;
	 }
      T exchange(const T newval, std::memory_order /*mo*/ = std::memory_order_seq_cst) volatile
	 {
	    T oldvalue ;
	    __asm__ __volatile__ ("## ATOMIC EXCHANGE\n\t"
				  "lock; xchg %1,%0"
				  : "=r" (oldvalue)
				  : "m" (*(volatile T*)&value), "0" (newval)
				  : "memory") ;
	    return oldvalue ;
	 }

      bool compare_exchange_strong( T& expected, T desired,
				    std::memory_order /*mo*/ = std::memory_order_seq_cst)
	 {
	    return __sync_bool_compare_and_swap(&value,expected,desired) ;
	 }
      bool compare_exchange_strong( T& expected, T desired,
				    std::memory_order /*mo*/ = std::memory_order_seq_cst) volatile
	 {
	    return __sync_bool_compare_and_swap(&value,expected,desired) ;
	 }

      T fetch_add( T incr, std::memory_order /*mo*/ = std::memory_order_seq_cst)
	 { return (T)__sync_fetch_and_add(&value,incr) ; }
      T fetch_add( T incr, std::memory_order /*mo*/ = std::memory_order_seq_cst) volatile
	 { return (T)__sync_fetch_and_add(&value,incr) ; }
     //T *fetch_add( std::ptrdiff_t incr, std::memory_order mo = std::memory_order_seq_cst) ;
     //T *fetch_add( std::ptrdiff_t incr, std::memory_order mo = std::memory_order_seq_cst) volatile ;

      T fetch_sub( T decr, std::memory_order /*mo*/ = std::memory_order_seq_cst)
	 { return (T)__sync_fetch_and_sub(&value,decr) ; }
      T fetch_sub( T decr, std::memory_order /*mo*/ = std::memory_order_seq_cst) volatile
	 { return (T)__sync_fetch_and_sub(&value,decr) ; }
     //T *fetch_sub( std::ptrdiff_t decr, std::memory_order mo = std::memory_order_seq_cst) ;
     //T *fetch_sub( std::ptrdiff_t decr, std::memory_order mo = std::memory_order_seq_cst) volatile ;

      T fetch_and( T mask, std::memory_order /*mo*/ = std::memory_order_seq_cst)
	 { return (T)__sync_fetch_and_and(&value,mask) ; }
      T fetch_and( T mask, std::memory_order /*mo*/ = std::memory_order_seq_cst) volatile
	 { return (T)__sync_fetch_and_and(&value,mask) ; }

      T fetch_or( T mask, std::memory_order /*mo*/ = std::memory_order_seq_cst)
	 { return (T)__sync_fetch_and_or(&value,mask) ; }
      T fetch_or( T mask, std::memory_order /*mo*/ = std::memory_order_seq_cst) volatile
	 { return (T)__sync_fetch_and_or(&value,mask) ; }

      T fetch_xor( T mask, std::memory_order /*mo*/ = std::memory_order_seq_cst)
	 { return (T)__sync_fetch_and_xor(&value,mask) ; }
      T fetch_xor( T mask, std::memory_order /*mo*/ = std::memory_order_seq_cst) volatile
	 { return (T)__sync_fetch_and_xor(&value,mask) ; }
#else
      void store(const T v, std::memory_order mo = std::memory_order_seq_cst) { value = v ; }
      void store(const T v, std::memory_order mo = std::memory_order_seq_cst) volatile { value = v ; }

      T load(std::memory_order mo = std::memory_order_seq_cst) const { return value ; }
      T load(std::memory_order mo = std::memory_order_seq_cst) const volatile { return value ; }

      T exchange(const T newval, std::memory_order mo = std::memory_order_seq_cst)
	 {
	 }
      T exchange(const T newval, std::memory_order mo = std::memory_order_seq_cst) volatile
	 {
	 }

      bool compare_exchange_strong( T& expected, T desired,
				    std::memory_order order = std::memory_order_seq_cst)
	 {
	 }
      bool compare_exchange_strong( T& expected, T desired,
				    std::memory_order order = std::memory_order_seq_cst) volatile
	 {
	 }

      T fetch_add( T incr, std::memory_order mo = std::memory_order_seq_cst) ;
      T fetch_add( T incr, std::memory_order mo = std::memory_order_seq_cst) volatile ;
      T *fetch_add( std::ptrdiff_t incr, std::memory_order mo = std::memory_order_seq_cst) ;
      T *fetch_add( std::ptrdiff_t incr, std::memory_order mo = std::memory_order_seq_cst) volatile ;

      T fetch_sub( T decr, std::memory_order mo = std::memory_order_seq_cst) ;
      T fetch_sub( T decr, std::memory_order mo = std::memory_order_seq_cst) volatile ;
      T *fetch_sub( std::ptrdiff_t decr, std::memory_order mo = std::memory_order_seq_cst) ;
      T *fetch_sub( std::ptrdiff_t decr, std::memory_order mo = std::memory_order_seq_cst) volatile ;

      T fetch_and( T mask, std::memory_order mo = std::memory_order_seq_cst) ;
      T fetch_and( T mask, std::memory_order mo = std::memory_order_seq_cst) volatile ;

      T fetch_or( T mask, std::memory_order mo = std::memory_order_seq_cst) ;
      T fetch_or( T mask, std::memory_order mo = std::memory_order_seq_cst) volatile ;

      T fetch_xor( T mask, std::memory_order mo = std::memory_order_seq_cst) ;
      T fetch_xor( T mask, std::memory_order mo = std::memory_order_seq_cst) volatile ;
#endif /* !FrMULTITHREAD - __GNUC__ - other */
   } ;
// end of namespace std
}
#endif

/************************************************************************/
/************************************************************************/

#if defined(__GNUC__) && defined(__486__)
//#  define _mm_pause() __asm__ __volatile__("pause" : : : "memory") ;
#  include <x86intrin.h>
#else
#  define _mm_pause()
#endif

/************************************************************************/
/************************************************************************/

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 70)
#  define _atomic_load(x) __atomic_load_n((x),__ATOMIC_ACQUIRE)
#  define _atomic_store(x,y) __atomic_store_n((x),(y),__ATOMIC_RELEASE)
#else
#  define _atomic_load(x) (x)
#  define _atomic_store(x,y) ((x) = (y))
#endif

/************************************************************************/
/************************************************************************/

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
// eliminate warnings about _qzz_res in Valgrind 3.6 //
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#endif

#ifndef FrMULTITHREAD
// non-threaded version
class FrCriticalSection
   {
   private:
      // no data members
   public:
      FrCriticalSection() {}
      ~FrCriticalSection() {}
      _fnattr_always_inline void acquire() {}
      _fnattr_always_inline void release() {}
      template <typename T> static void release(T *mutex)
	 { mutex = 0 ;
	   ANNOTATE_HAPPENS_AFTER(mutex) ; }
      _fnattr_always_inline bool locked() { return false ; }
      _fnattr_always_inline static void memoryBarrier() {}
      _fnattr_always_inline static void loadBarrier() {}
      _fnattr_always_inline static void storeBarrier() {}
      _fnattr_always_inline static void barrier()
	 {
#ifdef __GNUC__
	    __asm__ volatile ("" : : : "memory") ;
#endif
	 }
      template <typename T> static void atomicStore(T& var, T value)
	 {
	    var = value ;
	    return ;
	 }
      template <typename T> static void store(T& var, T value)
	 {
	    var = value ;
	    return ;
	 }
      template <typename T> static T load(const T& var)
	 {
	    return var ;
	 }
      template <typename T> static T swap(T& var, const T value)
	 { T old = var ;
	   var = value ;
	   return old ;
	 }
      template <typename T> static T swap(T* var, const T value)
	 { T old = *var ;
	   *var = value ;
	   return old ;
	 }
      template <typename T> static bool compareAndSwap(T* var, const T oldval,
						       const T newval)
	 { bool match = (*var == oldval) ;
	   if (match) *var = newval ;
	   return match ; }
      template <typename T> static T increment(T& var)
	 { T orig = var++ ;
	   return orig ; }
      template <typename T> static T increment(T& var, const T incr)
	 { T orig = var ;
	   var += incr ;
	   return orig ; }
      template <typename T> static T decrement(T& var)
	 { T orig = var-- ;
	   return orig ; }
      template <typename T> static T decrement(T& var, const T decr)
	 { T orig = var ;
	   var -= decr ;
	   return orig ; }
      template <typename T> static bool testAndSet(T &var, unsigned bitnum)
	 {
	    T mask = (T)(1L << bitnum) ;
	    T prev_val = var ;
	    var |= mask ;
	    return (prev_val & mask) == 0 ; // did we set it from clear?
	 }
      template <typename T> static bool testAndClear(T &var, unsigned bitnum)
	 {
	    T mask = (T)(1L << bitnum) ;
	    T prev_val = var ;
	    var &= ~mask ;
	    return (prev_val & mask) == mask ; // did we clear it from set?
	 }
      template <typename T> static T testAndSetMask(T &var, T bitmask)
	 {
	    T prev_val = var ;
	    var |= bitmask ;
	    return prev_val ;
	 }
      template <typename T> static T testAndClearMask(T &var, T bitmask)
	 {
	    T prev_val = var ;
	    var &= ~bitmask ;
	    return prev_val ;
	 }

      // generic functionality built on top of the atomic primitives
      template <typename T> static void push(T** var, T *node)
	 { node->next(*var) ;
	   *var = node ;
	 }
      template <typename T> static void push(T* var, T node)
	 { node.next(*var) ;
	   *var = node.value() ;
	 }
      template <typename T> static void push(T** var, T* nodes, T* tail)
	 { tail->next(*var) ;
	   *var = nodes ;
	 }
      template <typename T> static T* pop(T** var)
	 { T *node = *var ;
	    if (node)
	       *var = node->next() ;
	   return node ;
	 }
      // single-location memory writes are always atomic on x86
      void set(bool *var) { (*var) = true ; }
      void clear(bool *var) { (*var) = false ; }
   } ;
#elif __cplusplus >= 201103L && 1
// threaded version, using C++11 atomic<T>
class FrCriticalSection
   {
   private:
      static size_t s_collisions ;
      atomic<bool> m_mutex ;
   protected:
      bool tryacquire()
	 { return m_mutex.exchange(true,std::memory_order_acquire) ; }
      void backoff_acquire() ;
   public:
      FrCriticalSection() { m_mutex = false ; }
      ~FrCriticalSection() {}
      void acquire()
	 {
	    if (tryacquire())
	       backoff_acquire() ;
	 }
      void release()
	 {
	    m_mutex.store(false,std::memory_order_release) ;
	 }
      template <typename T> static void release(T *mutex)
	 { mutex->store(false,std::memory_order_release) ; }
      bool locked() const { return m_mutex.load() ; }
      _fnattr_always_inline static void memoryBarrier()
	 { atomic_thread_fence(std::memory_order_seq_cst) ; }
      _fnattr_always_inline static void loadBarrier()
	 { atomic_thread_fence(std::memory_order_acquire) ; }
      _fnattr_always_inline static void storeBarrier()
	 { atomic_thread_fence(std::memory_order_release) ; }
      _fnattr_always_inline static void barrier()
	 { atomic_thread_fence(std::memory_order_relaxed) ;/*FIXME*/ }
      template <typename T> static void atomicStore(T& var, T value)
	 { atomic_store_explicit(reinterpret_cast<atomic<T>*>(&var),value,std::memory_order_release) ;/*FIXME*/ }
      template <typename T> static void store(T& var, T value)
	 { atomic_store_explicit(reinterpret_cast<atomic<T>*>(&var),value,std::memory_order_release) ;/*FIXME*/ }
      template <typename T> static T load(const T& var)
	 { return atomic_load_explicit(reinterpret_cast<atomic<T>*>(const_cast<T*>(&var)),std::memory_order_acquire) ;/*FIXME*/ }
      template <typename T> static T swap(T& var, const T value)
	 { return atomic_exchange( reinterpret_cast<atomic<T>*>(&var), value ); }
      template <typename T> static T swap(T* var, const T value)
	 { return atomic_exchange( reinterpret_cast<atomic<T>*>(var), value ); }
      template <typename T> static bool compareAndSwap(T* var, const T oldval,
						const T newval)
	 {
	    T expected = oldval ;
	    return atomic_compare_exchange_strong( reinterpret_cast<atomic<T> *>(var), &expected, newval) ; 
	 }
      template <typename T> static T increment(T& var)
	 {
	    return atomic_fetch_add(reinterpret_cast<atomic<T>*>(&var),(T)1) ; 
	 }
      template <typename T> static T increment(T& var, const T incr)
	 { return atomic_fetch_add(reinterpret_cast<atomic<T>*>(&var),incr) ; }
      template <typename T> static T decrement(T& var)
	 { return atomic_fetch_sub(reinterpret_cast<atomic<T>*>(&var),(T)1) ; }
      template <typename T> static T decrement(T& var, const T decr)
	 { return atomic_fetch_sub(reinterpret_cast<atomic<T>*>(&var),decr) ; }
      template <typename T> static bool testAndSet(T &var, unsigned bitnum)
	 { 
	    T mask = (T)(1L << bitnum) ;
	    T prev_val = testAndSetMask(var,mask) ;
	    return (prev_val & mask) == 0 ; // did we set it from clear?
	 }
      template <typename T> static bool testAndClear(T &var, unsigned bitnum)
	 {
	    T mask = (T)(1L << bitnum) ;
	    T prev_val = testAndClearMask(var,mask) ;
	    return (prev_val & mask) == mask ; // did we clear it from set?
	 }
      template <typename T> static T testAndSetMask(T &var, T bitmask)
	 { return atomic_fetch_or(reinterpret_cast<atomic<T>*>(&var),bitmask) ; }
      template <typename T> static T testAndClearMask(T &var, T bitmask)
	 { return atomic_fetch_and(reinterpret_cast<atomic<T>*>(&var),(T)~bitmask) ; }
      template <typename T> static void push(T** var, T *node)
	 {
	    T *list ;
	    do {
	       list = *var ;
	       node->next(list) ;
	    } while (!compareAndSwap(var,list,node)) ;
	 }
      template <typename T> static void push(T* var, T node)
	 {
	    T list ;
	    do {
	       list = *var ;
	       node.next(list) ;
	    } while (!compareAndSwap(var->valuePtr(),list.value(),node.value())) ;
	 }
      template <typename T> static void push(T** var, T* nodes, T* tail)
	 {
	    T *list ;
	    do {
	       list = *var ;
	       tail->next(list) ;
	    } while (!compareAndSwap(var,list,nodes)) ;
	 }
      template <typename T> static T* pop(T** var)
	 {
	 T *head ;
	 T *rest ;
	 do {
	    head = *var ;
	    if (!head)
	       return head ;
	    rest = head->next() ;
	    } while (!compareAndSwap(var,head,rest)) ;
	 head->next((T*)0) ;
	 return head ;
	 }
      void incrCollisions() { increment(s_collisions) ; }
      // single-location memory writes are always atomic on x86
      void set(bool *var) { (*var) = true ; }
      void clear(bool *var) { (*var) = false ; }
   } ;
#elif __GNUC__ >= 4 && !defined(HELGRIND)
// threaded, using Gnu-specific intrinsics
class FrCriticalSection
   {
   private:
      static size_t s_collisions ;
      bool m_mutex ;
   protected:
      bool tryacquire()
	 { ANNOTATE_HAPPENS_BEFORE(&m_mutex) ;
	    return __sync_lock_test_and_set(&m_mutex,true) ;
	 }
      void backoff_acquire() ;
   public:
      FrCriticalSection() { m_mutex = false ; }
      ~FrCriticalSection() {}
      void acquire()
	 { if (tryacquire())
	      backoff_acquire() ;
	 }
      void release()
	 { __sync_lock_release(&m_mutex) ;
	   ANNOTATE_HAPPENS_AFTER(&m_mutex) ; }
      template <typename T> static void release(T *mutex)
	 { __sync_lock_release(mutex) ;
	   ANNOTATE_HAPPENS_AFTER(mutex) ; }
      bool locked() const { return m_mutex ; }
      _fnattr_always_inline static void memoryBarrier() { __sync_synchronize() ; }
      _fnattr_always_inline static void loadBarrier()
	 {
	    __asm__ __volatile__ ("## LOAD BARRIER\n\t"
				  /*"lfence"*/ : : : "memory") ;
	 }
      _fnattr_always_inline static void storeBarrier()
	 {
	    __asm__ __volatile__ ("## STORE BARRIER\n\t"
				  /*"sfence"*/ : : : "memory") ;
	 }
      _fnattr_always_inline static void barrier() // compiler barrier
	 {
	    __asm__ volatile ("" : : : "memory") ;
	 }
      template <typename T> static void atomicStore(T& var, T value)
	 {
	    // on x86, "xchg" may be faster than a store followed by a memory fence!
	    __asm__ __volatile__ ("## ATOMIC STORE\n\t"
				  "xchg %0,%1"
				  : "=r" (value)
				  : "m" (*(volatile T*)&var), "0" (value)
				  : "memory") ;
	    return ;
	 }
      template <typename T> static void store(T& var, T value)
	 {
	    // on x86, "xchg" may be faster than a store followed by a memory fence!
	    __asm__ __volatile__ ("## ATOMIC STORE\n\t"
				  "xchg %0,%1"
				  : "=r" (value)
				  : "m" (*(volatile T*)&var), "0" (value)
				  : "memory") ;
	    return ;
	 }
      template <typename T> static T load(const T& var)
	 {
	    T value ;
	    __asm__ __volatile__ ("## ATOMIC LOAD\n\t"
				  //?"lfence\n\t"
				  "mov %1,%0"
				  : "=r" (value)
				  : "m" (*(volatile T*)&var)
				  : "memory") ;
	    return value ;
	 }
      template <typename T> static T swap(T& var, const T value)
	 { return (T)__sync_lock_test_and_set(&var,value) ; }
      template <typename T> static T swap(T* var, const T value)
	 { return (T)__sync_lock_test_and_set(var,value) ; }
      template <typename T> static bool compareAndSwap(T* var, const T oldval,
						const T newval)
	 { return __sync_bool_compare_and_swap(var,oldval,newval) ; }
      template <typename T> static T increment(T& var)
	 { return (T)__sync_fetch_and_add(&var,1) ; }
      template <typename T> static T increment(T& var, const T incr)
	 { return (T)__sync_fetch_and_add(&var,incr) ; }
      template <typename T> static T decrement(T& var)
	 { return (T)__sync_fetch_and_sub(&var,1) ; }
      template <typename T> static T decrement(T& var, const T decr)
	 { return (T)__sync_fetch_and_sub(&var,decr) ; }
#ifdef __386__
      template <typename T> static bool testAndSet(T &var, unsigned bitnum)
	 {
	    bool result ;
	    __asm__ __volatile__("# ATOMIC TEST-AND-SET BIT\n\t"
				 "lock; bts %1,%2\n\t"
				 "setnc %%al\n\t"
				 : "=a" (result) : "r" (bitnum), "m" (var) : "cc", "memory" ) ;
	    return result ;
	 }
      template <typename T> static bool testAndClear(T &var, unsigned bitnum)
	 {
	    bool result ;
	    __asm__ __volatile__("# ATOMIC TEST-AND-CLEAR BIT\n\t"
				 "lock; btr %1,%2\n\t"
				 "setc %%al\n\t"
				 : "=a" (result) : "r" (bitnum), "m" (var) : "cc", "memory" ) ;
	    return result ;
	 }
#else
      template <typename T> static bool testAndSet(T &var, unsigned bitnum)
	 {
	    T mask = (T)(1L << bitnum) ;
	    T prev_val = __sync_fetch_and_or(&var,mask) ;
	    return (prev_val & mask) == 0 ; // did we set it from clear?
	 }
      template <typename T> static bool testAndClear(T &var, unsigned bitnum)
	 {
	    T mask = (T)(1L << bitnum) ;
	    T prev_val = __sync_fetch_and_and(&var,~mask) ;
	    return (prev_val & mask) == mask ; // did we clear it from set?
	 }
#endif /* __386__ */
      template <typename T> static T testAndSetMask(T &var, T bitmask)
	 {
	    return __sync_fetch_and_or(&var,bitmask) ;
	 }
      template <typename T> static T testAndClearMask(T &var, T bitmask)
	 {
	    return __sync_fetch_and_and(&var,~bitmask) ;
	 }
      
      // generic functionality built on top of the atomic primitives
      template <typename T> static void push(T** var, T *node)
	 {
	    T *list ;
	    do {
	       list = *var ;
	       node->next(list) ;
	    } while (!compareAndSwap(var,list,node)) ;
	 }
      template <typename T> static void push(T* var, T node)
	 {
	    T list ;
	    do {
	       list = *var ;
	       node.next(list) ;
	    } while (!__sync_bool_compare_and_swap(var->valuePtr(),list.value(),node.value())) ;
	 }
      template <typename T> static void push(T** var, T* nodes, T* tail)
	 {
	    T *list ;
	    do {
	       list = *var ;
	       tail->next(list) ;
	    } while (!compareAndSwap(var,list,nodes)) ;
	 }
      template <typename T> static T* pop(T** var)
	 {
	 T *head ;
	 T *rest ;
	 do {
	    head = *var ;
	    if (!head)
	       return head ;
	    rest = head->next() ;
	    } while (!compareAndSwap(var,head,rest)) ;
	 head->next((T*)0) ;
	 return head ;
	 }

//#if defined(__has_feature) && __has_feature(thread_sanitizer)
//      _fnattr_always_inline void incrCollisions() {}
//#else
      void incrCollisions() { increment(s_collisions) ; }
//#endif /* thread_sanitizer */

      // single-location memory writes are always atomic on x86
      void set(bool *var) { (*var) = true ; }
      void clear(bool *var) { (*var) = false ; }
   } ;
#elif defined(__GNUC__) && defined(__386__) && !defined(HELGRIND)
// threaded, using Gnu inline assembler
class FrCriticalSection
   {
   private:
      static size_t s_collisions ;
      bool m_mutex ;
   public:
      FrCriticalSection() { m_mutex = false ; }
      ~FrCriticalSection() {}
      void acquire()
	 {
	    ANNOTATE_HAPPENS_BEFORE(&m_mutex) ;
	    __asm__("# CRITSECT ACQUIRE\n"
		    "1:\n\t"
		    "cmp $0,%0\n\t"  // wait until lock is free
		    "jz 2f\n\t"
		    "pause\n\t"	     // stall thread for an instant
		    "jmp 1b\n\t"     // before checking again
		    "2:\n\t"
		    "mov 1,%%al\n\t"  // lock is free, attempt to grab it
		    "lock; xchg %%al,%0\n\t"
		    "test %%al,%%al\n\t"
		    "jnz 1b"
		    : : "m" (m_mutex) : "%al", "memory", "cc") ;
	 }
      void release()
	 {
	    __asm__ volatile
	       ("# CRITSECT RELEASE\n\t"
		    "movb $0,%0"
		    : "=m" (m_mutex) : : ) ;
	    ANNOTATE_HAPPENS_AFTER(&m_mutex) ;
	 }
      template <typename T> static void release(T *mutex)
	 { __sync_lock_release(mutex) ;
	   ANNOTATE_HAPPENS_AFTER(mutex) ; }
      bool locked() const { return m_mutex ; }
      _fnattr_always_inline static void memoryBarrier()
	 {
	    ANNOTATE_HAPPENS_BEFORE(&s_collisions) ;
	    __asm__ volatile ("## MEMORY BARRIER\n\t"
			      "mfence" : : : "memory") ;
	    ANNOTATE_HAPPENS_AFTER(&s_collisions) ;
	    return ;
	 }
      _fnattr_always_inline static void loadBarrier()
	 {
	    ANNOTATE_HAPPENS_BEFORE(&s_collisions) ;
	    __asm__ volatile ("## LOAD BARRIER\n\t"
			      /*"lfence"*/ : : : "memory") ;
	    ANNOTATE_HAPPENS_AFTER(&s_collisions) ;
	    return ;
	 }
      _fnattr_always_inline static void storeBarrier()
	 {
	    ANNOTATE_HAPPENS_BEFORE(&s_collisions) ;
	    __asm__ volatile ("## STORE BARRIER\n\t"
			      "sfence" : : : "memory") ;
	    ANNOTATE_HAPPENS_AFTER(&s_collisions) ;
	 }
      _fnattr_always_inline static void barrier() // compiler barrier
	 {
	    ANNOTATE_HAPPENS_BEFORE(&s_collisions) ;
	    __asm__ volatile ("" : : : "memory") ;
	    ANNOTATE_HAPPENS_AFTER(&s_collisions) ;
	    return ;
	 }
      template <typename T> static void atomicStore(T& var, T value)
	 {
	    ANNOTATE_HAPPENS_BEFORE(&var) ;
	    var = value ;
	    ANNOTATE_HAPPENS_AFTER(&var) ;
	    storeBarrier() ;
	    return ;
	 }
      template <typename T> static void store(T& var, T value)
	 {
	    ANNOTATE_HAPPENS_BEFORE(&var) ;
	    var = value ;
	    ANNOTATE_HAPPENS_AFTER(&var) ;
	    storeBarrier() ;
	    return ;
	 }
      template <typename T> static T load(const T& var)
	 {
	    ANNOTATE_HAPPENS_BEFORE(&var) ;
	    loadBarrier() ;
	    T value = var ;
	    ANNOTATE_HAPPENS_AFTER(&var) ;
	    return value ;
	 }
      template <typename T> static T swap(T& var, const T value)
	 {
	    ANNOTATE_HAPPENS_BEFORE(&var) ;
	    T orig ;
	    __asm__ volatile
	       ("# ATOMIC SWAP\n\t"
		"lock; xchg %0,%1\n\t"
		: "=r" (orig) : "0" (value), "m" (var) : ) ;
	    ANNOTATE_HAPPENS_AFTER(&var) ;
	    return orig ;
	 } ;
      template <typename T> static T swap(T* var, const T value)
	 {
	    ANNOTATE_HAPPENS_BEFORE(&var) ;
	    T orig ;
	    __asm__ volatile
	       ("# ATOMIC SWAP\n\t"
		"lock; xchg %0,%1\n\t"
		: "=0" (orig) : "r" (value), "m" (*var) : ) ;
	    ANNOTATE_HAPPENS_AFTER(&var) ;
	    return orig ;
	 } ;
      template <typename T> static bool compareAndSwap(T* var, const T oldval,
						       const T newval)
	 {
	    ANNOTATE_HAPPENS_BEFORE(&var) ;
	    bool success ;
	    if (sizeof(T) == 4)
	       {
	       __asm__ __volatile__("# CRITSECT CAS\n"
				    "movl %1,%%eax\n\t"
				    "lock; cmpxchg %3,%2\n\t"
				    "sete al\n\t"
				    : "=a" (success) : "m" (oldval), "m" (*var), "r" (newval) : "memory" ) ;
	       }
	    else if (sizeof(T) == 8)
	       {
	       __asm__ __volatile__("# CRITSECT CAS\n"
				    "mov %1,%%rax\n\t"
				    "lock; cmpxchg %3,%2\n\t"
				    "sete al\n\t"
				    : "=a" (success) : "m" (oldval), "m" (*var), "r" (newval) : "memory" ) ;
	       }
	    else
	       ::abort() ;

	    //bool success = __sync_bool_compare_and_swap(var,oldval,newval) ; //FIXME
	    ANNOTATE_HAPPENS_AFTER(&var) ;
	    return success ;
	 }
      template <typename T> static T increment(T& var)
	 {
	    ANNOTATE_HAPPENS_BEFORE(&var) ;
	    T orig ;
	    __asm__ __volatile__("# ATOMIC INCR BY 1, RETURN ORIG VALUE\n\t"
				 "mov $1,%0\n\t"
				 "lock; xadd %0,%1\n\t"
				 : "=r" (orig) : "m" (var) : "cc", "memory" ) ;
	    ANNOTATE_HAPPENS_AFTER(&var) ;
	    return orig ;
	 }
      template <typename T> static T increment(T& var, const T incr)
	 {
	    ANNOTATE_HAPPENS_BEFORE(&var) ;
	    T orig ;
	    __asm__ __volatile__("# ATOMIC INCR, RETURN ORIG VALUE\n\t"
				 "lock; xadd %0,%1\n\t"
				 : "=r" (orig) : "0" (incr), "m" (var) : "cc", "memory" ) ;
	    ANNOTATE_HAPPENS_AFTER(&var) ;
	    return orig ;
	 }
      template <typename T> static T decrement(T& var)
	 {
	    ANNOTATE_HAPPENS_BEFORE(&var) ;
	    T orig ;
	    __asm__ __volatile__("# ATOMIC DECR BY 1, RETURN ORIG VALUE\n\t"
				 "mov $-1,%0\n\t"
				 "lock; xadd %0,%1\n\t"
				 : "=r" (orig) : "m" (var) : "cc", "memory" ) ;
	    ANNOTATE_HAPPENS_AFTER(&var) ;
	    return orig ;
	 }
      template <typename T> static T decrement(T& var, const T incr)
	 {
	    T orig ;
	    ANNOTATE_HAPPENS_BEFORE(&var) ;
	    __asm__ __volatile__("# ATOMIC DECR, RETURN ORIG VALUE\n\t"
				 "lock; xadd %0,%1\n\t"
				 : "=r" (orig) : "0" (-incr), "m" (var) : "cc", "memory" ) ;
	    ANNOTATE_HAPPENS_AFTER(&var) ;
	    return orig ;
	 }
      template <typename T> static bool testAndSet(T &var, unsigned bitnum)
	 {
	    bool result ;
	    ANNOTATE_HAPPENS_BEFORE(&var) ;
	    __asm__ __volatile__ ("# ATOMIC TEST-AND-SET\n\t"
				  "lock; bts %1,%2\n\t"
				  "setnc %%al\n\t"
				  : "=a" (result) : "r" (bitnum), "m" (var) : "cc", "memory" ) ;
	    ANNOTATE_HAPPENS_AFTER(&var) ;
	    return result ;
	 }
      template <typename T> static bool testAndClear(T &var, unsigned bitnum)
	 {
	    bool result ;
	    ANNOTATE_HAPPENS_BEFORE(&var) ;
	    __asm__ __volatile__ ("# ATOMIC TEST-AND-CLEAR\n\t"
				  "lock; btr %1,%2\n\t"
				  "setc %%al\n\t"
				  : "=a" (result) : "r" (bitnum), "m" (var) : "cc", "memory" ) ;
	    ANNOTATE_HAPPENS_AFTER(&var) ;
	    return result ;
	 }
      
      // generic functionality built on top of the atomic primitives
      template <typename T> static void push(T** var, T *node)
	 {
	    T *list ;
	    do {
	       list = *var ;
	       node->next(list) ;
	    } while (!compareAndSwap(var,list,node)) ;
	 }
      template <typename T> static void push(T* var, T node)
	 {
	    T list ;
	    do {
	       list = *var ;
	       node.next(list) ;
	    } while (!compareAndSwap(var->valuePtr(),list.value(),node.value())) ;
	 }
      template <typename T> static void push(T** var, T* nodes, T* tail)
	 {
	    T *list ;
	    do {
	       list = *var ;
	       tail->next(list) ;
	    } while (!compareAndSwap(var,list,nodes)) ;
	 }
      template <typename T> static T* pop(T** var)
	 {
	 T *head ;
	 T *rest ;
	 do {
	    head = *var ;
	    if (!head)
	       return head ;
	    rest = head->next() ;
	    } while (!compareAndSwap(var,head,rest)) ;
	 head->next((T*)0) ;
	 return head ;
	 }
      // single-location memory writes are always atomic on x86
      void set(bool *var) { (*var) = true ; }
      void clear(bool *var) { (*var) = false ; }
   } ;
#else
// threaded, default version using Posix-thread mutex object
class FrCriticalSection
   {
   private:
      static size_t s_collisions ;
      mutable pthread_mutex_t m_mutex ;
      static pthread_mutex_t s_mutex ;
   protected: //methods
      static void s_acquire()
	 { 
	    ANNOTATE_HAPPENS_BEFORE(&s_mutex) ;
	    pthread_mutex_lock(&s_mutex) ; 
	 }
      static void s_release()
	 { 
	    pthread_mutex_unlock(&s_mutex) ; 
	    ANNOTATE_HAPPENS_AFTER(&s_mutex) ;
	 }
   public:
      FrCriticalSection() { pthread_mutex_init(&m_mutex,0) ; }
      ~FrCriticalSection() { pthread_mutex_destroy(&m_mutex) ; }
      void acquire() const
	 { 
	    ANNOTATE_HAPPENS_BEFORE(&m_mutex) ;
	    pthread_mutex_lock(&m_mutex) ; 
	 }
      void release() const
	 { 
	    pthread_mutex_unlock(&m_mutex) ; 
	    ANNOTATE_HAPPENS_AFTER(&m_mutex) ;
	 }
      static void memoryBarrier()
	 {
	    s_acquire() ;
	    s_release() ;
	    return ;
	 }
      static void loadBarrier()
	 {
	    s_acquire() ;
	    s_release() ;
	    return ;
	 }
      static void storeBarrier()
	 {
	    s_acquire() ;
	    s_release() ;
	    return ;
	 }
      static void barrier()
	 {
	    //FIXME
	 }
      template <typename T> static void release(T *mutex)
	 { mutex = 0 ;
	   ANNOTATE_HAPPENS_AFTER(mutex) ; }
      template <typename T> static void atomicStore(T& var, T value)
	 {
	    s_acquire() ;
	    var = value ;
	    s_release() ;
	 }
      template <typename T> static void store(T& var, T value)
	 {
	    s_acquire() ;
	    var = value ;
	    s_release() ;
	 }
      template <typename T> static T load(const T& var)
	 {
	    s_acquire() ;
	    T value = var;
	    s_release() ;
	    return value ;
	 }
      template <typename T> static T swap(T& var, const T value)
	 { 
	    s_acquire() ;
	    T old = var ;
	    var = value ;
	    s_release() ;
	    return old ;
	 }
      template <typename T> static T swap(T* var, const T value)
	 {
	    s_acquire() ;
	    T old = *var ;
	    *var = value ;
	    s_release() ;
	    return old ;
	 }
      template <typename T> static bool compareAndSwap(T* var, const T oldval,
						const T newval)
	 {
	    s_acquire() ;
	    bool match = (*var == oldval) ;
	    if (match) *var = newval ;
	    s_release() ;
	    return match ;
	 }
      template <typename T> static T increment(T& var)
	 {
	    s_acquire() ;
	    T newval = ++var ;
	    s_release() ;
	    return newval ;
	 }
      template <typename T> static T increment(T& var, const T incr)
	 {
	    s_acquire() ;
	    var += incr ;
	    T newval = var ;
	    s_release() ;
	    return newval ;
	 }
      template <typename T> static T decrement(T& var)
	 { 
	    s_acquire() ;
	    T newval = --var ;
	    s_release() ;
	    return newval ;
	 }
      template <typename T> static T decrement(T& var, const T decr)
	 {
	    s_acquire() ;
	    var -= decr ;
	    T newval = var ;
	    s_release() ;
	    return newval ;
	 }
      template <typename T> static bool testAndSet(T &var, unsigned bitnum)
	 {
	    s_acquire() ;
	    T mask = (T)(1L << bitnum) ;
	    T prev_val = var ;
	    var |= mask ;
	    bool was_clear = (prev_val & mask) == 0 ; // did we set it from clear?
	    s_release() ;
	    return was_clear ;
	 }
      template <typename T> static bool testAndClear(T &var, unsigned bitnum)
	 {
	    s_acquire() ;
	    T mask = (T)(1L << bitnum) ;
	    T prev_val = var ;
	    var &= ~mask ;
	    bool was_set = (prev_val & mask) == mask ; // did we clear it from set?
	    s_release() ;
	    return was_set ;
	 }
      template <typename T> static T testAndSetMask(T &var, T bitmask)
	 {
	    s_acquire() ;
	    T prev_val = var ;
	    var |= bitmask ;
	    s_release() ;
	    return prev_val ;
	 }
      template <typename T> static T testAndClearMask(T &var, T bitmask)
	 {
	    s_acquire() ;
	    T prev_val = var ;
	    var &= ~bitmask ;
	    s_release() ;
	    return prev_val ;
	 }

      // generic functionality built on top of the atomic primitives
      template <typename T> static void push(T** var, T *node)
	 {
	    s_acquire() ;
	    node->next(*var) ;
	    *var = node ;
	    s_release() ;
	    return ;
	 }
      template <typename T> static void push(T* var, T node)
	 {
	    s_acquire() ;
	    node.next(*var) ;
	    *var = node.value() ;
	    s_release() ;
	    return ;
	 }
      template <typename T> static void push(T** var, T* nodes, T* tail)
	 {
	    s_acquire() ;
	    tail->next(*var) ;
	    *var = nodes ;
	    s_release() ;
	    return ;
	 }
      template <typename T> static T* pop(T** var)
	 {
	    s_acquire() ;
	    T *node = *var ;
	    if (node)
	       *var = node->next() ;
	    s_release() ;
	    return node ;
	 }
      // single-location memory writes are always atomic on x86
      void set(bool *var) { (*var) = true ; }
      void clear(bool *var) { (*var) = false ; }
   } ;
#endif /* !FrMULTITHREAD */

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#  pragma GCC diagnostic pop
#endif

template <>
inline FrNullObject FrCriticalSection::swap(class FrNullObject &v, class FrNullObject) { return v; }
template <>
inline FrNullObject FrCriticalSection::swap(class FrNullObject *v, class FrNullObject) { return *v ; }

/************************************************************************/
/************************************************************************/

class FrScopeCriticalSection : public FrCriticalSection
   {
   public:
      FrScopeCriticalSection() { acquire() ; }
      ~FrScopeCriticalSection() { release() ; }
   } ;

/************************************************************************/
/************************************************************************/

#ifndef FrMULTITHREAD
class FrSynchEvent
   {
   private:
      bool m_set ;
   public:
      FrSynchEvent() { clear() ; }
      ~FrSynchEvent() {}

      bool isSet() const { return m_set ; }
      void clear() { m_set = false ; }
      void set() { m_set = true ; }

      _fnattr_always_inline void wait() {}
   } ;
#else // multi-threaded version
class FrSynchEvent
   {
#ifdef  __linux__
      union
         {
	 unsigned m_futex_val ;
	 struct
	    {
	    bool m_waiters ;
	    bool m_set ;
	    } m_event ;
         } ;
   public:
      FrSynchEvent() { clearAll() ; }
      ~FrSynchEvent() { set() ; }
      void clearAll() ;
#elif defined(__WINDOWS__)
      HANDLE m_eventhandle ;
   public:
      FrSynchEvent() ;
      ~FrSynchEvent() ;
#else // use pthreads
      phtread_mutex_t  m_mutex ;
      pthread_cond_t   m_condvar ;
      bool	       m_set ;
      bool	       m_waiting ;
   public:
      FrSynchEvent() ;
      ~FrSynchEvent() ;
#endif /* __WINDOWS__ */

      bool isSet() const ;
      void clear() ;
      void set() ;

      void wait() ;
   } ;
#endif /* !FrMULTITHREAD */

/************************************************************************/
/************************************************************************/

#ifndef FrMULTITHREAD
// non-threaded version -- wait always returns immediately, since
//   otherwise we'd just block forever
class FrSynchEventCounted
   {
   private:
      bool m_set ;
   public:
      FrSynchEventCounted() { clearAll() ; }
      ~FrSynchEventCounted() { waitForWaiters() ; }

      bool isSet() const { return m_set ; }
      unsigned numWaiters() const { return 0 ; }
      void clear() { m_set = false ; }
      void clearAll() { clear() ; }
      void set() { m_set = true ; }
      _fnattr_always_inline void wait() {}
      _fnattr_always_inline void waitForWaiters() {}
   } ;
#else
// multi-threaded version
class FrSynchEventCounted
   {
   private:
#ifdef __linux__
      unsigned m_futex_val ;
      static const unsigned m_mask_set = (UINT_MAX - INT_MAX) ;
      // high bit is set/clear status of event, remainder is number of waiters
#else
      //FIXME: implementations for other platforms
#endif /* __linux__ */
   public:
      FrSynchEventCounted() { clearAll() ; }
      ~FrSynchEventCounted() { waitForWaiters() ; } // release any waiters

      bool isSet() const ;
      unsigned numWaiters() const ;
      void clear() ;
      void clearAll() ;
      void set() ;
      void wait() ;
      void waitForWaiters() ;
   } ;
#endif /* FrMULTITHREAD */

/************************************************************************/
/************************************************************************/

#ifndef FrMULTITHREAD
// non-threaded version -- wait always returns immediately, since
//   otherwise we'd just block forever
class FrSynchEventCountdown
   {
   private:
      int m_counter ;
   public:
      FrSynchEventCountdown() { clear() ; }
      ~FrSynchEventCountdown() { consumeAll() ; }

      void init(int count) { m_counter = (count << 1) ; }
      bool isDone() const { return m_counter <= 1 ; }
      int countRemaining() const { return m_counter >> 1 ; }
      void clear() { m_counter = 0 ; }
      void consume() { m_counter -= 2 ; }
      void consumeAll() { m_counter = 0 ; }
      bool haveWaiters() const { return (m_counter & 1) != 0 ; }
      _fnattr_always_inline void wait() {}
   } ;
#else
class FrSynchEventCountdown
   {
   private:
#ifdef __linux__
      // low bit indicates whether anybody is blocked on the
      //   countdown, remaining bits are the count until the event
      //   fires
      int m_futex_val ;
#else
      //FIXME: implementation for other platforms
#endif /* __linux__ */
   public: 
      FrSynchEventCountdown() { clear() ; }
      ~FrSynchEventCountdown() { consumeAll() ; }

#ifdef __linux__
      void init(int count) { FrCriticalSection::store(m_futex_val,(count << 1)) ; }
      bool isDone() const { return FrCriticalSection::load(m_futex_val) <= 1 ; }
      int countRemaining() const { return FrCriticalSection::load(m_futex_val) >> 1 ; }
      void clear() { FrCriticalSection::store(m_futex_val,0) ; }
      bool haveWaiters() const { return (FrCriticalSection::load(m_futex_val) & 1) != 0 ; }
#endif /* __linux__ */
      void consume() ;
      void consumeAll() ;
      void wait() ;
   } ;
#endif /* FrMULTITHREAD */

#endif /* !__FRCRITSEC_H_INCLUDED */

// end of file frcritsec.h //
