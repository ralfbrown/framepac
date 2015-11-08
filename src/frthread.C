/****************************** -*- C++ -*- *****************************/
/*									*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frthread.cpp	    thread manipulation				*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 2009,2010,2012,2013,2015				*/
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

#include <errno.h>
#include <sched.h>
#include <time.h>
#include "frassert.h"
#include "fr_mem.h"
#include "frthread.h"

//#define TRACE_POOL	// trace activities in FrThreadPool
//#define TRACE_POOL2	// verbosely trace activities in FrThreadPool
//#define TRACE_POOL3	// exhaustively trace activities in FrThreadPool

/************************************************************************/
/*  Overrides for single-threaded version to make code more readable	*/
/************************************************************************/

#ifndef FrMULTITHREAD
#  define pthread_mutex_lock(x)
#  define pthread_mutex_unlock(x)
#  define sem_wait(x)
#  define sem_post(x)
#  define sem_getvalue(x,y) (-1)

#  undef ANNOTATE_HAPPENS_BEFORE
#  define ANNOTATE_HAPPENS_BEFORE(x)
#  undef ANNOTATE_HAPPENS_AFTER
#  define ANNOTATE_HAPPENS_AFTER(x)
#endif

/************************************************************************/
/*	Manifest constants for this module				*/
/************************************************************************/

/************************************************************************/
/*	Global data for this module					*/
/************************************************************************/

static FrThreadPool *global_pool = 0 ;
static size_t pool_refcount = 0 ;

FrAllocator FrThreadWorkList::allocator("ThreadWorkList",
					sizeof(FrThreadWorkList)) ;
FrAllocator FrThreadWorkOrder::allocator("ThreadWorkOrder",
					 sizeof(FrThreadWorkOrder)) ;

sig_atomic_t FrThreadWorkOrder::s_serialnum = 0 ;

/************************************************************************/
/*	helper functions						*/
/************************************************************************/

static sig_atomic_t atomic_decr(volatile sig_atomic_t *value, int incr)
{
   return __sync_sub_and_fetch(value,(sig_atomic_t)incr) ;
}

//----------------------------------------------------------------------

#ifdef TRACE_POOL
#ifdef FrMULTITHREAD
pthread_mutex_t trace_mutex = PTHREAD_MUTEX_INITIALIZER ;
#endif /* FrMULTITHREAD */
#include "frprintf.h"
#define TRACE_MSG(x) trace_msg x
#ifdef __GNUC__
static void trace_msg(const char *fmt, ...) __attribute__((format(gnu_printf,1,0))) ;
#endif /* __GNUC__ */
static void trace_msg(const char *fmt, ...)
{
   va_list args ;
   va_start(args,fmt) ;
   pthread_mutex_lock(&trace_mutex) ;
   char *msg = Fr_vaprintf(fmt,args) ;
   va_end(args) ;
   cout << msg << flush ;
   pthread_mutex_unlock(&trace_mutex) ;
   FrFree(msg) ;
   return ;
}
#else
# define TRACE_MSG(x)
#endif

#ifdef TRACE_POOL2
#  define TRACE_MSG2(x) TRACE_MSG(x)
#else
#  define TRACE_MSG2(x)
#endif

#ifdef TRACE_POOL3
#  define TRACE_MSG3(x) TRACE_MSG(x)
#else
#  define TRACE_MSG3(x)
#endif

//----------------------------------------------------------------------

size_t FrThreadDefaultStacksize()
{
#ifdef FrMULTITHREAD
   pthread_attr_t attr ;
   pthread_attr_init(&attr) ;
   size_t stack ;
   if (pthread_attr_getstacksize(&attr,&stack) != 0)
      stack = 0 ;
   (void)pthread_attr_destroy(&attr) ;
   return stack ;
#else
   return 0 ;
#endif /* FrMULTITHREAD */
}

//----------------------------------------------------------------------

bool FrThreadSleep(size_t microseconds)
{
   struct timespec tm ;
   tm.tv_nsec = (microseconds % 1000000) * 1000 ;
   tm.tv_sec = (microseconds / 1000000) ;
   return nanosleep(&tm,0) == 0 ;
}

//----------------------------------------------------------------------

void FrThreadYield()
{
#if defined(_POSIX_PRIORITY_SCHEDULING) || defined(FrMULTITHREAD)
   (void)sched_yield() ;
#endif /* FrMULTITHREAD */
   return ;
}

//----------------------------------------------------------------------

#ifdef FrMULTITHREAD
static bool initialize_mutex(pthread_mutex_t *mutex, bool allow_recursion)
{
   pthread_mutexattr_t attr ;
   (void)pthread_mutexattr_init(&attr) ;
   if (allow_recursion)
      (void)pthread_mutexattr_settype(&attr,PTHREAD_MUTEX_RECURSIVE) ;
   else
      (void)pthread_mutexattr_settype(&attr,PTHREAD_MUTEX_ERRORCHECK) ;
   bool OK = (pthread_mutex_init(mutex,&attr) == 0) ;
   (void)pthread_mutexattr_destroy(&attr) ;
   return OK ;
}
#endif /* FrMULTITHREAD */

//----------------------------------------------------------------------

static void exit_request(const void *, void *)
{
   return ;
}

//----------------------------------------------------------------------

static void thread_wait(const void *t, void *)
{
   FrThread *thread = (FrThread*)t ;
   if (thread)
      {
      FrThreadPool *pool = thread->pool() ;
      if (pool)
	 pool->pauseThread(thread) ;
      }
   return ;
}

//----------------------------------------------------------------------

void thread_pool_dispatcher(FrThread *thread)
{
   if (!thread->pool())
      return ;
   // loop until the work function we are given is our exit request
   for ( ; ; )
      {
      // let our boss know we're waiting for work
      FrThreadWorkOrder *order = thread->pool()->getNextJob(thread) ;
      if (!order)
	 {
	 // nothing left for us to do, so just wait
	 thread_wait(thread,0) ;
	 continue ;
	 }
      // execute the work we're given
      FrThreadWorkFunc *work = order->workFunction() ;
      if (work == &thread_wait)
	 {
	 work(thread,0) ;
	 }
      else if (work == &exit_request)
	 {
	 delete order ;
	 TRACE_MSG(("thr#%u told to exit\n",thread->threadNumber())) ;
	 break ;
	 }
      else
	 {
	 if (work)
	    work(order->userArg(),order->userResult()) ;
	 TRACE_MSG(("thr#%u done with job %d\n",thread->threadNumber(),
		    order->serialNumber())) ;
	 }
      delete order ;
      }
   thread->pool()->threadExiting(thread) ;
   return ;
}

/************************************************************************/
/*	Methods for class FrThreadWorkList				*/
/************************************************************************/

void *FrThreadWorkList::operator new(size_t)
{
   TRACE_MSG3(("ctor: alloc worklist\n")) ;
   return allocator.allocate() ; 
}

//----------------------------------------------------------------------

void FrThreadWorkList::operator delete(void *blk)
{
   TRACE_MSG3(("dtor: delete worklist\n")) ;
   allocator.release(blk) ;
   return ;
}

/************************************************************************/
/*	Methods for class FrThreadWorkOrder				*/
/************************************************************************/

void *FrThreadWorkOrder::operator new(size_t)
{
   return allocator.allocate() ; 
}

//----------------------------------------------------------------------

void FrThreadWorkOrder::operator delete(void *blk)
{
   TRACE_MSG3(("dtor: deleting work order %d\n",((FrThreadWorkOrder*)blk)->serialNumber())) ;
   allocator.release(blk) ;
   return ;
}

//----------------------------------------------------------------------

FrThreadWorkOrder::~FrThreadWorkOrder()
{
   sig_atomic_t count = 1 ;		// include ourself in the count
   while (m_dependents)
      {
      FrThreadWorkList *list = m_dependents ;
      count++ ;
      m_dependents = list->next() ;
      FrThreadWorkOrder *order = list->order() ;
      if (order)
	 order->prerequisiteMet(m_pool) ;
      delete list ; 
      }
   if (m_pool)
      m_pool->dependenciesResolved(count) ;
   return ;
}

//----------------------------------------------------------------------

void FrThreadWorkOrder::addPrerequisite()
{
   //FIXME: needs to be atomic to allow addition from within a worker thread
   m_numprereq++ ; 
   return ;
}

//----------------------------------------------------------------------

void FrThreadWorkOrder::addDependent(FrThreadWorkOrder *dependent)
{
   if (dependent)
      {
      FrThreadWorkList *dep = new FrThreadWorkList(dependent) ;
      if (dep)
	 {
	 dependent->addPrerequisite() ;
	 // the following linkage does not require locking since this
	 //   function can only be called by the boss thread before
	 //   the work order is dispatched and only by the thread
	 //   which received the order after dispatch
	 dep->setNext(m_dependents) ;
	 m_dependents = dep ;
	 }
      }
   return ;
}

//----------------------------------------------------------------------

void FrThreadWorkOrder::prerequisiteMet(FrThreadPool *thrpool)
{
   if (atomic_decr(&m_numprereq,1) <= 0)
      {
      // if we've satisfied all our preconditions and haven't yet been
      //   dispatched (m_pool == 0), dispatch ourselves
      if (thrpool && !m_pool)
	 thrpool->dispatch(this) ;
      }
   return ;
}

/************************************************************************/
/*	Methods for class FrThread					*/
/************************************************************************/

FrThread::FrThread(FrThreadFunc main_fn, void *thread_arg, bool joinable,
		   size_t stacksize, FrThreadPool *thrpool, unsigned thread_id)
{
   m_mainfunc = main_fn ;
   m_threadarg = thread_arg ;
   waiting(false) ;
   m_lockcount = 0 ;
   m_pool = thrpool ;
   m_threadnum = thread_id ;
#ifdef FrMULTITHREAD
   m_mutex_init = false ;
   m_condition_init = false ;
   initMutex() ;
   pthread_attr_t attr ;
   pthread_attr_init(&attr) ;
   if (joinable)
      pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_JOINABLE) ;
   else
      pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED) ;
   if (stacksize)
      pthread_attr_setstacksize(&attr,stacksize) ;
   setErrcode(pthread_create(&m_thread,&attr,thread_fn,(void*)this)) ;
   (void)pthread_attr_destroy(&attr) ;
#if defined(FrTHREAD_COLLISIONS)
   m_critsect = 0 ;
   m_collisions = 0 ;
#endif /* FrTHREAD_COLLISIONS */
#else
   m_mutex_init = true ;
   m_condition_init = true ;
   setErrcode(EINVAL) ;
   (void)joinable ; (void)stacksize ;
#endif /* FrMULTITHREAD */
   return ;
}

//----------------------------------------------------------------------

FrThread::~FrThread()
{
   bool ok = OK() ;
   destroyMutex() ;
   destroyCondition() ;
   if (ok)
      {
#ifdef FrMULTITHREAD
      // the thread is still running, so terminate it
      pthread_t self = pthread_self() ;
      setErrcode(EINVAL) ;		// mark as no longer OK()
      threadCleanup(this) ;
      if (pthread_equal(self,m_thread))
	 pthread_exit(0) ;
      else
	 pthread_cancel(m_thread) ;
#endif /* FrMULTITHREAD */
      }
   return ;
}

//----------------------------------------------------------------------

void FrThread::threadSetup(void * /*arg*/)
{
   //FrThread *thread = (FrThread*)arg ;
   FrAllocator::threadSetup() ;
   return ;
}

//----------------------------------------------------------------------

void FrThread::threadCleanup(void * /*arg*/)
{
   //FrThread *thread = (FrThread*)arg ;
   FrAllocator::reclaimAll() ;
   return ;
}

//----------------------------------------------------------------------

bool FrThread::OK()
{
   m_critsect.acquire() ;
   bool ok = (m_errcode == 0) ;
   m_critsect.release() ;
   return ok ;
}

//----------------------------------------------------------------------

void FrThread::setErrcode(int code)
{
   (void)m_critsect.swap(m_errcode,code) ;
   return ;
}

//----------------------------------------------------------------------

void *FrThread::thread_fn(void *thr)
{
   threadSetup(thr) ;
   ((FrThread*)thr)->run() ;
   threadCleanup(thr) ;
   return 0 ;
}

//----------------------------------------------------------------------

void FrThread::run()
{
#ifdef FrMULTITHREAD
   sigset_t block_all ;
   sigfillset(&block_all) ;
   pthread_sigmask(SIG_SETMASK,&block_all,0) ;
#endif /* FrMULTITHREAD */
   if (m_mainfunc)
      m_mainfunc(this) ;
   setErrcode(EINVAL) ;			// thread no longer exists once
   return ;				// we return
}

//----------------------------------------------------------------------

void FrThread::yield() const
{
   FrThreadYield() ;
   return ;
}

//----------------------------------------------------------------------

void FrThread::sleep(size_t microseconds) const
{
   (void)FrThreadSleep(microseconds) ;
   return ;
}

//----------------------------------------------------------------------

//----------------------------------------------------------------------

bool FrThread::createKey(FrThreadKey &key, void (*dtor)(void*))
{
#ifdef FrMULTITHREAD
   return pthread_key_create(&key.key(),dtor) == 0 ;
#else
   (void)key, (void)dtor ;
   return false ;
#endif /* FrMULTITHREAD */
}

//----------------------------------------------------------------------

bool FrThread::createKey(FrThreadOnce &once, FrThreadKey &key, void (*dtor)(void*))
{
#ifdef FrMULTITHREAD
   (void)once, (void)key, (void)dtor ;

   return false ; //FIXME
#else
   (void)once, (void)key, (void)dtor ;
   return false ;
#endif /* FrMULTITHREAD */
}

//----------------------------------------------------------------------

void *FrThread::getKey(FrThreadKey &key)
{
#ifdef FrMULTITHREAD
   return pthread_getspecific(key.key()) ;
#else
   (void)key ;
   return 0 ;
#endif /* FrMULTITHREAD */
}

//----------------------------------------------------------------------

bool FrThread::setKey(FrThreadKey &key, void *value)
{
#ifdef FrMULTITHREAD
   errno = pthread_setspecific(key.key(),value) ;
   return (errno == 0) ;
#else
   (void)key, (void)value ;
   return false ;
#endif /* FrMULTITHREAD */
}

//----------------------------------------------------------------------

bool FrThread::initMutex()
{
#ifdef FrMULTITHREAD
   if (!mutexOK())
      {
      bool success = (pthread_mutex_init(&m_mutex,0) == 0) ;
      if (success)
	 m_mutex_init = true ;
      return success ;
      }
#endif /* FrMULTITHREAD */
   return true ;
}

//----------------------------------------------------------------------

void FrThread::destroyMutex()
{
#ifdef FrMULTITHREAD
   if (mutexOK())
      {
      pthread_mutex_destroy(&m_mutex) ;
      m_mutex_init = false ;
      }
#endif /* FrMULTITHREAD */
   return ;
}

//----------------------------------------------------------------------

bool FrThread::mutexLock()
{
   if (!mutexOK() && !initMutex())
      return false ;
#ifdef FrMULTITHREAD
   bool success = (pthread_mutex_lock(&m_mutex) == 0) ;
   if (success)
      {
      m_lockcount++ ;
      incrCritSect() ;
      }
   else
      {
      incrCollisions() ;
      }
   return success ;
#else
   return true ;
#endif /* FrMULTITHREAD */
}

//----------------------------------------------------------------------

bool FrThread::mutexTryLock()
{
   if (!mutexOK() && !initMutex())
      return false ;
#ifdef FrMULTITHREAD
   bool success = (pthread_mutex_trylock(&m_mutex) == 0) ;
   if (success)
      {
      m_lockcount++ ;
      incrCritSect() ;
      }
   else
      incrCollisions() ;
   return success ;
#else
   return true ;
#endif /* FrMULTITHREAD */
}

//----------------------------------------------------------------------

bool FrThread::mutexUnlock()
{
   if (!mutexOK() || !locked())
      return false ;
#ifdef FrMULTITHREAD
   bool success = (pthread_mutex_unlock(&m_mutex) == 0) ;
   if (success)
      m_lockcount-- ;
   return success ;
#else
   return true ;
#endif /* FrMULTITHREAD */
}

//----------------------------------------------------------------------

bool FrThread::initCondition()
{
#ifdef FrMULTITHREAD
   if (!mutexOK() && !initMutex())
      return false ;
   int status = pthread_cond_init(&m_condition,0) ;
   if (status == 0)
      m_condition_init = true ;
#endif /* FrMULTITHREAD */
   return conditionOK() ;
}

//----------------------------------------------------------------------

void FrThread::destroyCondition()
{
#ifdef FrMULTITHREAD
   if (conditionOK())
      {
      (void)pthread_cond_destroy(&m_condition) ;
      m_condition_init = false ;
      }
#endif /* FrMULTITHREAD */
   return ;
}

//----------------------------------------------------------------------

bool FrThread::conditionWait(bool have_lock)
{
#ifdef FrMULTITHREAD
   if (!mutexOK() && !initMutex())
      return false ;
   if (!conditionOK() && !initCondition())
      return false ;
   if (!have_lock)
      pthread_mutex_lock(&m_mutex) ;
   waiting(true) ;
   int status ;
   for ( ; ; )
      {
      TRACE_MSG(("thr#%d waiting\n",(int)threadNumber())) ;
      status = pthread_cond_wait(&m_condition,&m_mutex) ;
      TRACE_MSG(("thr#%d awakened\n",(int)threadNumber())) ;
      // check for spurious wake-up by testing the 'waiting' flag, which will
      //   have been cleared for an actual wake-up
      if (!waiting())
	 break ;
      }
   TRACE_MSG(("thr#%d running\n",(int)threadNumber())) ;
   if (!have_lock)
      pthread_mutex_unlock(&m_mutex) ;
   return status == 0 ;
#else
   (void)have_lock ;
   return true ;
#endif /* FrMULTITHREAD */
}

//----------------------------------------------------------------------

bool FrThread::conditionSignal()
{
#ifdef FrMULTITHREAD
   if (!conditionOK())
      return false ;			// we never initialized the cond. var!
   if (!mutexOK())
      return false ;			// we never initialized the mutex!
   if (!waiting())
      {
      TRACE_MSG(("thread #%d not waiting!\n",(int)threadNumber())) ;
      return false ;			// can't signal if not waiting
      }
   pthread_mutex_lock(&m_mutex) ;	// ensure no race condition
   TRACE_MSG(("thread #%d clear wait flag\n",(int)threadNumber())) ;
   waiting(false) ;			// tell thread this is a real wake-up
   bool success = (pthread_cond_signal(&m_condition) == 0) ;
   pthread_mutex_unlock(&m_mutex) ;	// OK for other thread to access again
   TRACE_MSG(("thread #%d signalled\n",(int)threadNumber())) ;
   // mutex ownership passes to signalled thread
   return success ;
#else
   return true ;
#endif /* FrMULTITHREAD */
}

//----------------------------------------------------------------------

bool FrThread::conditionBroadcast()
{
#ifdef FrMULTITHREAD
   if (!conditionOK())
      return false ;			// we never initialized the cond. var!
   if (!mutexOK())
      return false ;			// we never initialized the mutex!
   if (!waiting())
      {
      TRACE_MSG(("thread #%d not waiting!\n",(int)threadNumber())) ;
      return false ;			// can't signal if not waiting
      }
   pthread_mutex_lock(&m_mutex) ;	// ensure no race condition
   TRACE_MSG(("thread #%d clear wait flag\n",(int)threadNumber())) ;
   waiting(false) ;			// tell thread this is a real wake-up
   bool success = (pthread_cond_broadcast(&m_condition) == 0) ;
   pthread_mutex_unlock(&m_mutex) ;	// OK for other thread to access again
   TRACE_MSG(("thread #%d broadcast signal\n",(int)threadNumber())) ;
   // mutex ownership passes to signalled thread
   return success ;
#else
   return true ;
#endif /* FrMULTITHREAD */
}

//----------------------------------------------------------------------

void FrThread::testCancel()
{
#ifdef FrMULTITHREAD
   pthread_testcancel() ;
#endif /* FrMULTITHREAD */
   return ;
}

/************************************************************************/
/*	Methods for class FrThreadPool					*/
/************************************************************************/

FrThreadPool::FrThreadPool(size_t num_threads,
			   size_t stacksize)
{
   m_firstjob = m_lastjob = new FrThreadWorkList(0) ;
   m_numlive = 0 ;
   m_numthreads = m_availthreads = 0 ;
   m_stacksize = stacksize ;
   m_dependencies = 0 ;
   m_have_depends = false ;
#ifdef FrMULTITHREAD
   // set up a thread-shared semaphore with initial value zero.
   // threads wait on m_workavail prior to consuming a work order.
   // the pool posts on m_workavail every time a work order becomes available.
   sem_init(&m_workavail, 0, 0) ;
   // a synchronization semaphore that lets us know when all extant jobs have
   //   completed; init to 1 so that dispatch can do a wait without blocking any
   //   time it adds to an empty job queue
   sem_init(&m_jobs, 0, 1) ;
   // to synchronize with the end of all work, each thread will post on m_idle
   //   and wait on m_paused, and the boss will wait on m_idle and post on
   //   m_paused
   sem_init(&m_idle, 0, 0) ;
   sem_init(&m_paused, 0, 0) ;
   // set up the threads
   m_threads = 0 ;
   (void)addThreads(num_threads) ;
   if (!m_threads)
      {
      m_numthreads = 0 ;
      FrFree(m_threads) ;	m_threads = 0 ;
      }
#else
   (void)num_threads ;
   (void)stacksize ;
   m_threads = 0 ;
#endif /* FrMULTTHREAD */
   m_numthreads = m_numlive ;
#if defined(HAS_PTHREAD_SETCONCURRENCY) && 0
   pthread_setconcurrency(m_numthreads+1+pthread_getconcurrency()) ;
#endif /* HAS_PTHREAD_SETCONCURRENCY */
   return ;
}

//----------------------------------------------------------------------

FrThreadPool::~FrThreadPool()
{
   if (m_threads)
      {
      // wake any paused threads
      limitThreads(numthreads()) ;
      // tell each thread to exit
      TRACE_MSG2(("FrThreadPool shutting down\n")) ;
      for (sig_atomic_t i = 0 ; i < numthreads() ; i++)
	 {
	 dispatch(&exit_request,0,0) ;
	 }
      // wait for the threads to exit
      TRACE_MSG(("pool: waiting for threads to exit\n")) ;
      waitUntilIdle() ;
      // delete the threads
      TRACE_MSG2(("pool: deleting thread structures\n")) ;
      for (sig_atomic_t i = 0 ; i < numthreads() ; i++)
	 {
	 delete m_threads[i] ;
	 }
      FrFree(m_threads) ;	m_threads = 0 ;
      }
   // run inside critical sections to keep Helgrind et al happy
   m_tailmutex.acquire() ;
   assertq(m_firstjob->next() == 0 && m_firstjob->order() == 0) ;
   delete m_firstjob ;
   m_firstjob = 0 ;
   m_tailmutex.release() ;
   (void)m_headmutex.swap(m_lastjob,m_firstjob) ;
   if (m_lastjob)
      {
      delete m_lastjob ;
      m_lastjob = 0 ;
      }
#if defined(HAS_PTHREAD_SETCONCURRENCY) && 0
   size_t concur = pthread_getconcurrency() ;
   if (concur >= m_numthreads + 1)
      pthread_setconcurrency(concur - m_numthreads + 1) ;
#endif /* HAS_PTHREAD_SETCONCURRENCY */
#ifdef FrMULTITHREAD
   TRACE_MSG3(("pool: destroying semaphores\n")) ;
   sem_destroy(&m_workavail) ;
   sem_destroy(&m_jobs) ;
   sem_destroy(&m_idle) ;
   sem_destroy(&m_paused) ;
#endif /* FrMULTITHREAD */
   FrAllocator::reclaimAllForeignFrees() ;
   m_numthreads = 0 ;
   TRACE_MSG3(("pool: done -- %d work orders processed\n",FrThreadWorkOrder::nextSerialNumber())) ;
   return ;
}

//----------------------------------------------------------------------

sig_atomic_t FrThreadPool::numLivingThreads()
{ 
   return m_numlive ; 
}

//----------------------------------------------------------------------

void FrThreadPool::waitUntilIdle()
{
   if (m_numthreads == 0)
      return ;
   TRACE_MSG(("pool: waiting for all work to complete\n")) ;
   // first, wait until all jobs have been farmed out to the workers
   for ( ; ; )
      {
      m_tailmutex.acquire() ;
      // get the value inside a critical section to keep Helgrind happy
      sig_atomic_t deps = m_dependencies ;
      m_tailmutex.release() ;
      if (deps > 0)
	 {
	 TRACE_MSG3(("info: %d deps remain\n",deps)) ;
	 FrThreadSleep(1000) ;
	 }
      else
	 break ;
      }
   // then wait until the workers signal completion
   TRACE_MSG3(("pool: waiting until workers signal completion\n")) ;
   sem_wait(&m_jobs) ;
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
// eliminate warnings about _qzz_res in Valgrind 3.6 //
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#endif
   ANNOTATE_HAPPENS_AFTER(&m_jobs) ;
   // restore m_jobs to its default state, so that we don't block on the next
   //   dispatch
   ANNOTATE_HAPPENS_BEFORE(&m_jobs) ;
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#  pragma GCC diagnostic pop
#endif

   sem_post(&m_jobs) ;
   m_have_depends = false ;
   TRACE_MSG(("pool: idle\n")) ;
   return ;
}

//----------------------------------------------------------------------

int FrThreadPool::numPausedThreads()
{
   int count ;
   if (sem_getvalue(&m_idle,&count) == 0)
      return count ;
   else
      return 0 ;
}

//----------------------------------------------------------------------

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
// eliminate warnings about _qzz_res in Valgrind 3.6 //
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#endif

void FrThreadPool::pauseThread(FrThread *thread)
{
   (void)thread ;
   // signal pool that we've paused
   ANNOTATE_HAPPENS_BEFORE(&m_idle) ;
   sem_post(&m_idle) ;
   TRACE_MSG(("thr#%u paused (%d total)\n",
	      thread->threadNumber(),numPausedThreads())) ;
   // wait until pool wants us to go again
   sem_wait(&m_paused) ;
   ANNOTATE_HAPPENS_AFTER(&m_paused) ;
   TRACE_MSG2(("thr#%u reactivated\n",thread->threadNumber())) ;
   return ;
}

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#  pragma GCC diagnostic pop
#endif

//----------------------------------------------------------------------

void FrThreadPool::addDependency(FrThreadWorkOrder *prereq,
				 FrThreadWorkOrder *dependent)
{
   if (prereq && dependent)
      {
      prereq->addDependent(dependent) ;
      (void)m_tailmutex.increment(m_dependencies) ;
      TRACE_MSG2(("info: incr dep=%d\n",m_dependencies)) ;
      }
   return ;
}


//----------------------------------------------------------------------

void FrThreadPool::dependenciesResolved(sig_atomic_t count)
{
   sig_atomic_t dep = m_tailmutex.decrement(m_dependencies,count) ;
   if (dep <= count)  // decrement() returns value before the decrement
      {
      TRACE_MSG(("info: decr dep=%d, posting\n",dep)) ;
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
// eliminate warnings about _qzz_res in Valgrind 3.6 //
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#endif
      ANNOTATE_HAPPENS_BEFORE(&m_jobs) ;
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
// eliminate warnings about _qzz_res in Valgrind 3.6 //
#  pragma GCC diagnostic pop
#endif
      sem_post(&m_jobs) ;
      }
   else
      {
      TRACE_MSG2(("info: decr dep=%d\n",dep)) ;
      }
   return ;
}

//----------------------------------------------------------------------

void FrThreadPool::limitThreads(sig_atomic_t N)
{
   if (N == availableThreads())
      return ;
   waitUntilIdle() ;
   if (N > numthreads())
      N = numthreads() ;
   if (N < availableThreads())
      {
      // tell some of our threads to pause until reactivated
      for (sig_atomic_t i = N ; i < availableThreads() ; i++)
	 {
	 dispatch(&thread_wait,this,this) ;
	 }
      }
   else if (numthreads() > 0)
      {
      for (sig_atomic_t i = availableThreads() ; i<N && i<numthreads() ; i++)
	 {
	 sem_wait(&m_idle) ;
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
// eliminate warnings about _qzz_res in Valgrind 3.6 //
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#endif
	 ANNOTATE_HAPPENS_AFTER(&m_idle) ;
	 ANNOTATE_HAPPENS_BEFORE(&m_paused) ;
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#  pragma GCC diagnostic pop
#endif
	 sem_post(&m_paused) ;
	 }
      }
   m_availthreads = N ;
   return ;
}

//----------------------------------------------------------------------

bool FrThreadPool::addThreads(sig_atomic_t desired_total)
{
#ifdef FrMULTITHREAD
   if (desired_total > numthreads())
      {
      if (numLivingThreads() < numthreads())
	 return false ;			// can't add if some have died already
      FrThread **new_t = FrNewR(FrThread*,m_threads,desired_total) ;
      if (!new_t)
	 return false ;
      m_threads = new_t ;
      m_availthreads = desired_total ;
      for (sig_atomic_t i = numthreads() ; i < desired_total ; i++)
	 {
	 // try to create a new thread, and add it to the pool if it was
	 //   successfully created
	 FrThread *thread = new FrThread(thread_pool_dispatcher,(void*)0,
					 false,m_stacksize,this,i) ;
	 if (thread)
	    {
	    if (thread->OK())
	       {
	       m_threads[m_numlive] = thread ;
	       m_tailmutex.increment(m_numlive) ;
	       }
	    else
	       delete thread ;
	    }
	 }
      m_numthreads = m_numlive ;
      m_availthreads = m_numlive ;
      return numthreads() == desired_total ;
      }
#else
   (void)desired_total ;
#endif /* FrMULTITHREAD */
   return true ;
}

//----------------------------------------------------------------------

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
// eliminate warnings about _qzz_res in Valgrind 3.6 //
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#endif

bool FrThreadPool::dispatch(FrThreadWorkOrder *order)
{
   if (!order)
      return false ;
   order->setPool(this) ;
   if (availableThreads() == 0)
      {
      // execute the function directly
      FrThreadWorkFunc *work_fn = order->workFunction() ;
      bool success = false ;
      if (work_fn)
	 {
	 work_fn(order->userArg(),order->userResult()) ;
	 success = true ;
	 }
      delete order ;
      return success ;
      }
   if (m_tailmutex.increment(m_dependencies) == 0)
      {
      // the queue may have gone empty since the previous dispatch, in
      //   which case m_jobs got posted, so we need to consume that
      //   posting with a wait
      TRACE_MSG2(("pool: dependencies was 0\n")) ;
      sem_wait(&m_jobs) ;
      ANNOTATE_HAPPENS_AFTER(&m_jobs) ;
      }
   FrThreadWorkList *newlast = new FrThreadWorkList(order) ;
   TRACE_MSG2(("info: dep=%d\n",m_dependencies)) ;
   if (m_have_depends)
      {
      // insert the work order in the queue; we need to grab the
      //   mutex because we can be called from a worker thread when
      //   it has a dependent job whose preconditions have been met
      m_tailmutex.acquire() ;
      m_lastjob->setNext(newlast) ;
      m_lastjob = newlast ;
      m_tailmutex.release() ;
      }
   else
      {
      // only boss will call dispatcher, so no need to lock
      m_lastjob->setNext(newlast) ;
      m_lastjob = newlast ;
      }
   // post the job to any waiting worker thread
   TRACE_MSG(("pool: dispatcher posting job %d\n",order->serialNumber())) ;
   ANNOTATE_HAPPENS_BEFORE(&m_workavail) ;
   sem_post(&m_workavail) ;
   TRACE_MSG2(("pool: dispatcher posted job %d\n",order->serialNumber())) ;
   return true ;
}

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#  pragma GCC diagnostic pop
#endif

//----------------------------------------------------------------------

bool FrThreadPool::dispatch(FrThreadWorkFunc *work_fn,
			    void *input, void *output)
{
   if (!work_fn)
      {
      return false ;
      }
   if (availableThreads() == 0)
      {
      // execute the function directly
      work_fn(input,output) ;
      }
   else
      {
      dispatch(new FrThreadWorkOrder(work_fn,input,output)) ;
      }
   return true ;
}

//----------------------------------------------------------------------

FrThreadWorkOrder *FrThreadPool::getNextJob(FrThread *thread)
{
   unsigned threadnum = thread->threadNumber() ;
   (void)threadnum ;
   TRACE_MSG2(("thr#%u getNextJob\n",threadnum)) ;
   // wait until there is work available
   sem_wait(&m_workavail) ;
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
// eliminate warnings about _qzz_res in Valgrind 3.6 //
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#endif
   ANNOTATE_HAPPENS_AFTER(&m_workavail) ;
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#  pragma GCC diagnostic pop
#endif
   // wait until it is safe to access pool structure
   FrThreadWorkOrder *order = 0 ;
   m_headmutex.acquire() ;
   FrThreadWorkList *first = m_firstjob ;
   FrThreadWorkList *next = first->next() ;
   if (next)
      {
      m_firstjob = next ;
      order = next->order() ;
      next->clearOrder() ;
      delete first ;
      }
   m_headmutex.release() ;
   TRACE_MSG(("thr#%u got job %d\n",threadnum,order ? order->serialNumber() : -1)) ;
   return order ;
}

//----------------------------------------------------------------------

void FrThreadPool::threadExiting(FrThread *)
{
   (void)m_tailmutex.decrement(m_numlive) ;
   return ;
}

//----------------------------------------------------------------------

ostream &FrThreadPool::collisionStatistics(ostream &out) const
{
   if (numthreads() > 0)
      {
      out << "Thread-Collision Statistics" << endl ;
      out << "===========================" << endl ;
      for (sig_atomic_t i = 0 ; i < numthreads() ; i++)
	 {
	 const FrThread *thr = m_threads[i] ;
	 if (thr)
	    out << "  Thread #" << (i+1) << ": " << thr->threadCollisions()
		<< " collisions, " << thr->criticalSections()
		<< " uncontested critsects" << endl ;
	 }
      }
   return out ;
}

//----------------------------------------------------------------------

ostream &FrThreadPool::collisionStatistics() const
{
   return collisionStatistics(cerr) ;
}

/************************************************************************/
/*	Methods for class FrMutex					*/
/************************************************************************/

FrMutex::FrMutex(bool allow_recursion, bool dont_destroy)
{
   m_OK = false ;
   init(allow_recursion, dont_destroy) ;
   return ;
}

//----------------------------------------------------------------------

bool FrMutex::init(bool allow_recursion, bool dont_destroy)
{
   if (!m_OK)
      {
#ifdef FrMULTITHREAD
      m_OK = initialize_mutex(&m_mutex,allow_recursion) ;
#if defined(FrTHREAD_COLLISIONS)
      m_critsect = 0 ;
      m_collisions = 0 ;
#endif /* FrTHREAD_COLLISIONS */
#else
      (void)allow_recursion ;
#endif /* FrMULTITHREAD */
      m_dontdestroy = dont_destroy ;
      m_owned = false ;
      }
   return m_OK ;
}

//----------------------------------------------------------------------

FrMutex::~FrMutex()
{
   if (!m_dontdestroy)
      {
      for (size_t i = 0 ; i < 1000 ; i++)
	 {
	 if (destroy())
	    break ;
	 else
	    FrThreadYield() ;
	 }
      }
   return ;
}

//----------------------------------------------------------------------

bool FrMutex::destroy()
{
#ifdef FrMULTITHREAD
   if (OK())
      {
      // ensure that the mutex is not locked
      int errcode ;
      while ((errcode = pthread_mutex_trylock(&m_mutex)) == EBUSY)
	 {
	 unlock() ;
	 }
      unlock() ;
      // now that the mutex is free, we can destroy it
      if (errcode != 0)
	 return false ;
      if (pthread_mutex_destroy(&m_mutex) == EBUSY)
	 return false ;
      m_OK = false ;
      }
#endif /* FrMULTITHREAD */
   return true ;
}

//----------------------------------------------------------------------

bool FrMutex::amOwner() const
{
#ifdef FrMULTITHREAD
   if (owned())
      {
      pthread_t self = pthread_self() ;
      return pthread_equal(self,m_owner) ;
      }
#endif /* FrMULTITHREAD */
   return true ;
}

//----------------------------------------------------------------------

void FrMutex::setOwner()
{
#ifdef FrMULTITHREAD
   m_owner = pthread_self() ;
   m_owned = true ;
#endif /* FrMULTITHREAD */
   return ;
}

//----------------------------------------------------------------------

void FrMutex::clearOwner()
{
   m_owned = false ;
   return ;
}

//----------------------------------------------------------------------

#ifdef FrMULTITHREAD
bool FrMutex::lock()
{
   bool success = true ;
   if (OK())
      {
      int status = pthread_mutex_lock(&m_mutex) ;
      if (status != 0)
	 success = false ;
      }
   return success ;
}
#endif

//----------------------------------------------------------------------

#ifdef FrMULTITHREAD
bool FrMutex::trylock()
{
   bool success = true ;
   if (OK())
      {
      int status = pthread_mutex_trylock(&m_mutex) ;
      if (status == EBUSY)
	 success = false ;
      }
   return success ;
}
#endif

//----------------------------------------------------------------------

#ifdef FrMULTITHREAD
bool FrMutex::unlock()
{
   bool success = true ;
   if (OK())
      {
      int status = pthread_mutex_unlock(&m_mutex) ;
      if (status == EPERM)
	 success = false ;
      }
   return success ;
}
#endif

/************************************************************************/
/*	Procedural Interface						*/
/************************************************************************/

FrThreadPool *FrCreateGlobalThreadPool(size_t numthreads, size_t stacksize)
{
   TRACE_MSG2(("FrCreateGlobalThreadPool\n")) ;
   if (global_pool)
      {
      pool_refcount++ ;
      }
   else
      {
      global_pool = new FrThreadPool(numthreads,stacksize) ;
      pool_refcount = 1 ;
      }
   return global_pool ;
}

//----------------------------------------------------------------------

bool FrDestroyGlobalThreadPool(FrThreadPool *pool)
{
   TRACE_MSG2(("FrDestroyGlobalThreadPool\n")) ;
   if (pool != global_pool)
      {
      delete pool ;
      return false ;
      }
   if (--pool_refcount > 0)
      return false ;
   TRACE_MSG(("deleting global pool\n")) ;
   delete global_pool ;
   global_pool = 0 ;
   return true ;
}

/************************************************************************/
/************************************************************************/

void FrThreadBackoff(size_t &loops)
{
   ++loops ;
   // we expect the pending operation(s) to complete in
   //   well under one microsecond, so start by just spinning
   if (loops < FrSPIN_COUNT)
      {
      _mm_pause() ;
      _mm_pause() ;
      }
   else if (loops < FrSPIN_COUNT + FrYIELD_COUNT)
      {
      // it's taking a little bit longer, so now yield the
      //   CPU to any suspended but ready threads
      //debug_msg("yield\n") ;
      FrThreadYield() ;
      }
   else
      {
      // hmm, this is taking a while -- maybe an add() had
      // to do extra work, or we are oversubscribed on
      // threads and an operation got suspended
      size_t factor = loops - FrSPIN_COUNT - FrYIELD_COUNT + 1 ;
      FrThreadSleep(factor * FrNAP_TIME) ;
      }
   return ;
}

// end of file frthread.cpp //
