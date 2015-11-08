/****************************** -*- C++ -*- *****************************/
/*									*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frthread.h	    thread manipulation				*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 2009,2010,2012,2013,2015				*/
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

#ifndef __FRTHREAD_H_INCLUDED
#define __FRTHREAD_H_INCLUDED

#include <signal.h>   // for sig_atomic_t
#include "frcommon.h"
#include "frcritsec.h"
#if defined(FrMULTITHREAD)
#  include <errno.h>
#  include <pthread.h>
#  include <semaphore.h>
#endif /* FrMULTITHREAD */

//TODO: for C++11, use std::thread instead of pthreads

/************************************************************************/
/************************************************************************/

#define Fr_cacheline_size 64

// length of time (in microseconds) to go to sleep during lock collisions
//#define FrNAP_TIME 100
#define FrNAP_TIME 250

// how many times to spin on a lock before yielding CPU
#define FrSPIN_COUNT 10
#define FrYIELD_COUNT 2


/************************************************************************/
/************************************************************************/

class FrCacheLinePadding
   {
   private:
      char m_pad[Fr_cacheline_size] ;
   } ;

//----------------------------------------------------------------------

class FrMutex
   {
   private:
      bool 	m_OK ;
      bool 	m_owned ;
      bool	m_dontdestroy ;
#if defined(FrMULTITHREAD)
      pthread_mutex_t m_mutex ;
      pthread_t m_owner ;
#if defined(FrTHREAD_COLLISIONS)
      size_t	m_critsect ;
      size_t	m_collisions ;
#endif /* FrTHREAD_COLLISIONS */
#endif /* FrMULTITHREAD */

   public:
      FrMutex(bool allow_recursion = false, bool dont_destroy = false) ;
      ~FrMutex() ;

      // access to internal state
      bool OK() const { return m_OK ; }
      bool owned() const { return m_owned ; }
      bool amOwner() const ;
#if defined(FrMULTITHREAD) && defined(FrTHREAD_COLLISIONS)
      size_t criticalSections() const { return m_critsect ; }
      size_t threadCollisions() const { return m_collisions ; }
#else
      size_t criticalSections() const { return 0 ; }
      size_t threadCollisions() const { return 0 ; }
#endif /* FrMULTITHREAD && FrTHREAD_COLLISIONS */

      // manipulators
      bool init(bool allow_recursion = false, bool dont_destroy = false) ;
      void setOwner() ;
      void clearOwner() ;
      bool destroy() ;
#if defined(FrMULTITHREAD)
      bool lock() ;
      bool trylock() ;
      bool unlock() ;
#else
      bool lock() _fnattr_always_inline { return true ; }
      bool trylock() _fnattr_always_inline { return true ; }
      bool unlock() _fnattr_always_inline { return true ; }
#endif /* FrMULTITHREAD */
#if defined(FrMULTITHREAD) && defined(FrTHREAD_COLLISIONS)
      void incrCritSect() { m_critsect++ ; }
      void incrCollisions() { m_collisions++ ; }
#else
      void incrCritSect() _fnattr_always_inline {}
      void incrCollisions() _fnattr_always_inline {}
#endif /* FrMULTITHREAD && FrTHREAD_COLLISIONS */

#if defined(FrMULTITHREAD)
      // the following assume that the object is already initialized to
      //   maximize speed
      bool fast_lock() { return pthread_mutex_lock(&m_mutex) == 0 ; }
      bool fast_trylock() { return pthread_mutex_trylock(&m_mutex) == 0 ; }
      bool fast_unlock() { return pthread_mutex_unlock(&m_mutex) != EPERM ; }

#if defined(FrTHREAD_COLLISIONS)
      bool fast_lock_stats()
	 { if (fast_trylock())
	      { ++m_critsect ; return true ; }
	   else
	      { ++m_collisions ; return fast_lock() ; } }
      bool fast_trylock_stats()
	 { return fast_trylock() ? ++m_critsect,true : ++m_collisions,false ; }
#endif /* FrTHREAD_COLLISIONS */
#endif /* FrMULTITHREAD */
   } ;

/************************************************************************/
/************************************************************************/

typedef void FrThreadWorkFunc(const void *input, void *output) ;

class FrAllocator ;
class FrThreadWorkOrder ;
class FrThreadPool ;

//----------------------------------------------------------------------

class FrThreadWorkList
   {
   private:
      static FrAllocator allocator ;
      FrThreadWorkList  *m_next ;
      FrThreadWorkOrder *m_order ;
   public:
      void *operator new(size_t) ;
      void operator delete(void *blk) ;
      FrThreadWorkList(FrThreadWorkOrder *ord)
	 { m_next = 0 ; m_order = ord ; }
      ~FrThreadWorkList() {}

      // accessors
      FrThreadWorkList *next() const { return m_next ; }
      FrThreadWorkList *volatileNext() volatile { return m_next ; }
      FrThreadWorkOrder *order() const { return m_order ; }

      // modifiers
      void setNext(FrThreadWorkList *nxt) { m_next = nxt ; }
      void setOrder(FrThreadWorkOrder *ord) { m_order = ord ; }
      void clearOrder() { m_order = 0 ; }
   } ;

//----------------------------------------------------------------------

class FrThreadWorkOrder
   {
   private:
      static FrAllocator allocator ;
      static sig_atomic_t s_serialnum ;
      FrThreadPool      *m_pool ;
      FrThreadWorkFunc  *m_workfunc ;
      void     	        *m_userarg ;
      void     	        *m_userresult ;
      FrThreadWorkList  *m_dependents ;
      volatile sig_atomic_t m_numprereq ;
      sig_atomic_t	 m_serialnum ;
   public:
      void *operator new(size_t) ;
      void operator delete(void *blk) ;
      FrThreadWorkOrder()
	 { m_pool = 0 ; m_workfunc = 0 ; m_userarg = 0 ;
	   m_userresult = 0 ; m_dependents = 0 ; m_numprereq = 0 ;
	   m_serialnum = s_serialnum++ ; }
      FrThreadWorkOrder(FrThreadWorkFunc *work, void *arg, void *res)
	 { m_pool = 0 ; m_workfunc = work ; m_userarg = arg ;
	   m_userresult = res ; m_dependents = 0 ; m_numprereq = 0 ;
	   m_serialnum = s_serialnum++ ; }
      ~FrThreadWorkOrder() ;

      // accessors
      FrThreadWorkFunc *workFunction() const { return m_workfunc ; }
      void *userArg() const { return m_userarg ; }
      void *userResult() const { return m_userresult ; }
      FrThreadPool *pool() const { return m_pool ; }
      bool prerequisitesMet() const { return m_numprereq == 0 ; }
      sig_atomic_t serialNumber() const { return m_serialnum ; }
      static sig_atomic_t nextSerialNumber() { return s_serialnum ; }

      // manipulators
      void setPool(FrThreadPool *p) { m_pool = p ; }
      void setWorkFunction(FrThreadWorkFunc *work) { m_workfunc = work ; }
      void setUserArg(void *arg) { m_userarg = arg ; }
      void setUserResult(void *res) { m_userresult = res ; }
      void addPrerequisite() ;
      void addDependent(FrThreadWorkOrder *dependent) ;
      void prerequisiteMet(FrThreadPool *pool) ;
   } ;

/************************************************************************/
/************************************************************************/

class FrThreadOnce
   {
#ifdef FrMULTITHREAD
   private:
      pthread_once_t m_once ;
   public:
      FrThreadOnce() { m_once = PTHREAD_ONCE_INIT ; }
      ~FrThreadOnce() {}

      pthread_once_t *operator *() { return &m_once ; }
      void doOnce(void(*fn)(void)) { (void)pthread_once(&m_once,fn) ; }
#else // !FrMULTITHREAD
   private:
      bool m_have_run ;
   public:
      FrThreadOnce() { m_have_run = false ; }
      ~FrThreadOnce() {}

      void *operator *() const { return 0 ; }
      void doOnce(void(*fn)(void))
	 { if (!m_have_run) { fn() ; m_have_run = true ; } }
#endif /* FrMULTITHREAD */
   } ;

//----------------------------------------------------------------------

class FrThreadKey
   {
#ifdef FrMULTITHREAD
   private:
      pthread_key_t m_key ;
   public:
      pthread_key_t *operator *() { return &m_key ; }
      pthread_key_t &key() { return _atomic_load(m_key) ; }
#else // !FrMULTITHREAD
      void *key() const { return 0 ; }
#endif /* FrMULTITHREAD */
   public:
      FrThreadKey() {}
      ~FrThreadKey() {}
   } ;

/************************************************************************/
/************************************************************************/

typedef void (*FrThreadFunc)(class FrThread *) ;

class FrThreadPool ;

class FrThread
   {
   private: // data
      FrThreadFunc 	m_mainfunc ;
      FrThreadPool     *m_pool ;	// pool containing this thread or 0
      void	       *m_threadarg ;
      int		m_errcode ;
      unsigned		m_threadnum ;	// thread number in m_pool
      unsigned		m_lockcount ;
      bool		m_waiting ;	// waiting on m_condition?
      bool		m_condition_init ;
      bool		m_mutex_init ;
      FrCriticalSection m_critsect ;
#ifdef FrMULTITHREAD
      pthread_t 	m_thread ;
      // for synchronization in e.g. FrThreadPool:
      pthread_cond_t	m_condition ;
      pthread_mutex_t	m_mutex ;
#if defined(FrTHREAD_COLLISIONS)
      size_t	        m_critsectcount ;
      size_t	        m_collisions ;
#endif /* FrTHREAD_COLLISIONS */
#endif /* FrMULTITHREAD */

   private: // methods
      static void threadSetup(void *) ;
      static void threadCleanup(void *) ;
      static void *thread_fn(void *thr) ;

   public:
      FrThread(FrThreadFunc main_fn, void *arg, bool joinable = true,
	       size_t stacksize = 0, FrThreadPool *pool = 0,
	       unsigned thread_id = 0) ;
      virtual ~FrThread() ;

      // access to internal state
      bool OK() ;
      bool conditionOK() const { return  m_condition_init ; }
      bool mutexOK() const { return m_mutex_init ; }
      FrThreadPool *pool() const { return m_pool ; }
      void *userArg() const { return m_threadarg ; }
      unsigned threadNumber() const { return m_threadnum ; }
      bool locked() const { return m_lockcount > 0 ; }
      unsigned lockCount() const { return m_lockcount ; }
      bool waiting() const { return m_waiting ; }
#if defined(FrMULTITHREAD) && defined(FrTHREAD_COLLISIONS)
      size_t criticalSections() const { return m_critsectcount ; }
      size_t threadCollisions() const { return m_collisions ; }
#else
      size_t criticalSections() const { return 0 ; }
      size_t threadCollisions() const { return 0 ; }
#endif /* FrMULTITHREAD && FrTHREAD_COLLISIONS */

      // modifiers
      void setErrcode(int code) ;
      
      // thread manipulation
      virtual void run() ;
      void yield() const ;
      void sleep(size_t microseconds) const ;

      // thread-local storage
      static bool createKey(FrThreadKey &key, void (*dtor)(void*) = 0) ;
      static bool createKey(FrThreadOnce &once, FrThreadKey &key, void (*dtor)(void*) = 0) ;
      static void *getKey(FrThreadKey &key) ;
      static bool setKey(FrThreadKey &key, void *value) ;

      // synchronization
      bool initMutex() ;
      void destroyMutex() ;
      bool mutexLock() ;
      bool mutexTryLock() ;
      bool mutexUnlock() ;
      bool initCondition() ;
      void destroyCondition() ;
      bool conditionWait(bool have_lock = false) ;
      bool conditionSignal() ;		// wake one or more threads waiting on cond var
      bool conditionBroadcast() ;	// wake all threads waiting on condition variable
      void testCancel() ;		// perform any pending cancellation
      void waiting(bool w) { m_waiting = w ; }
#if defined(FrMULTITHREAD) && defined(FrTHREAD_COLLISIONS)
      void incrCritSect() { m_critsectcount++ ; }
      void incrCollisions() { m_collisions++ ; }
#else
      void incrCritSect() { }
      void incrCollisions() { }
#endif /* FrMULTITHREAD && FrTHREAD_COLLISIONS */
   } ;

/************************************************************************/
/************************************************************************/

class FrThreadPool
   {
   private:
      FrThreadWorkList *m_firstjob ;    // head of workorders list
      FrCriticalSection m_headmutex ;	// protection for job queue head
      sig_atomic_t 	m_numlive ;	// how many threads are still live
#ifdef FrMULTITHREAD
      sem_t		m_jobs ;	// synchron. for end of all work
      sem_t		m_workavail ;	// synchronization betw boss & workers
      sem_t		m_idle ;	// synchronization betw boss & workers
      sem_t		m_paused ;	// synchronization betw boss & workers
#endif /* FrMULTITHREAD */
      FrCriticalSection m_tailmutex ;	// protection for job queue tail
      // members touched only by boss thread, need to be in a different
      //    cache line than m_firstjob and the semaphores
      bool		m_have_depends ;
      char		pad2[Fr_cacheline_size-sizeof(bool)] ;
      FrThread         **m_threads ;	// all the worker threads
      FrThreadWorkList *m_lastjob ;	// tail of workorders list
      size_t		m_stacksize ;	// requested size of stack per thread
      sig_atomic_t      m_numthreads ;	// total number of threads available
      sig_atomic_t	m_availthreads ;// how many threads allowed to work
      sig_atomic_t      m_dependencies ;// how many job deps do we have?
   public:
      FrThreadPool(size_t numthreads, size_t stacksize = 0) ;
      ~FrThreadPool() ;

      // access to internal state
      sig_atomic_t numthreads() const { return m_numthreads ; }
      sig_atomic_t availableThreads() const { return m_availthreads ; }
      sig_atomic_t inactiveThreads() const
	 { return numthreads() - availableThreads() ; }
      sig_atomic_t numLivingThreads() ;
      int numPausedThreads() ;
      bool idle() { return numPausedThreads() >= numthreads() ; }
      bool dispatchableThread()
	 { return numPausedThreads() > inactiveThreads() ; }
      sig_atomic_t numDependencies() volatile { return m_dependencies ; }

      // manipulators
      void haveDependents() { m_have_depends = true ; }
      // (must be called before first dispatch() if using dependencies;
      //  cleared by next waitUntilIdle)
      bool addThreads(sig_atomic_t desired_total) ;
      void limitThreads(sig_atomic_t N) ;
      bool dispatch(FrThreadWorkFunc *, void *input, void *output) ;
      bool dispatch(FrThreadWorkOrder *order) ;
      FrThreadWorkOrder *getNextJob(FrThread *) ; // called by worker when idle
      void pauseThread(FrThread *) ;
      void threadExiting(FrThread *) ;  // called by worker thread on exit
      void waitUntilIdle() ;

      void addDependency(FrThreadWorkOrder *prereq,
			 FrThreadWorkOrder *dependent) ;
      void dependenciesResolved(sig_atomic_t count = 1) ;

      // statistics
      ostream &collisionStatistics(ostream &) const ;
      ostream &collisionStatistics() const  ;
   } ;

/************************************************************************/
/*	Procedural interface						*/
/************************************************************************/

#ifdef FrMULTITHREAD
# ifdef FrTHREAD_COLLISIONS
#  define FrCRITSECT_ENTER(mutex) (mutex).fast_lock_stats()
#  define FrCRITSECT_TRYENTER(mutex) (mutex).fast_trylock_stats()
#  define FrCRITSECT_LEAVE(mutex) (mutex).fast_unlock()
# else
#  define FrCRITSECT_ENTER(mutex) (mutex).fast_lock()
#  define FrCRITSECT_TRYENTER(mutex) (mutex).fast_trylock()
#  define FrCRITSECT_LEAVE(mutex) (mutex).fast_unlock()
# endif /* FrTHREAD_COLLISIONS */
#else
#  define FrCRITSECT_ENTER(mutex)
#  define FrCRITSECT_TRYENTER(mutex) true
#  define FrCRITSECT_LEAVE(mutex)
#endif /* FrMULTITHREAD */

FrThreadPool *FrCreateGlobalThreadPool(size_t numthreads,size_t stacksize = 0);
bool FrDestroyGlobalThreadPool(FrThreadPool *) ;

size_t FrThreadDefaultStacksize() ;
void FrThreadYield() ;
bool FrThreadSleep(size_t microseconds) ;

void FrThreadBackoff(size_t &loops) ;

#endif /* !__FRTHREAD_H_INCLUDED */

// end of file frthread.h //
