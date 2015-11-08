/****************************** -*- C++ -*- *****************************/
/*									*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frslfreg.h	    class for self-registering obj instances	*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 2009,2012 Ralf Brown/Carnegie Mellon University	*/
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

#ifndef __FRSLFREG_H_INCLUDED
#define __FRSLFREG_H_INCLUDED

#include "frsymtab.h"
#include "frutil.h"

enum FrComparisonType
   {
      FrComp_DontCare,
      FrComp_Equal,
      FrComp_Less,
      FrComp_LessEqual,
      FrComp_Greater,
      FrComp_GreaterEqual,
      FrComp_NotEqual
   } ;

bool FrComparePriority(int prio1, int prio2, FrComparisonType sense,
		       bool *done) ;

template <class T> class FrSelfRegistering
   {
   private:
      static FrSelfRegistering<T> s_instances ;
      static unsigned		  s_numinstances ;
   private:
      FrSelfRegistering<T> *m_next ;
      FrSelfRegistering<T> *m_prev ;
      FrSelfRegistering<T> *m_self ;
      T			   *m_instance ;
      int		    m_priority ;
      int	     	    m_id ;
      FrSymbol		   *m_tag ;
      char		   *m_name ;
   private: // methods
      FrSelfRegistering() { init() ; }
      void init()
	 { if (m_self != this)
	    {
	    // only init if not already initialized; we need this because it
	    //   might be possible for an instance of the class using this
	    //   class to be instantiated by a global constructor before our
	    //   own global dtor gets executed
	    m_next = this ; m_prev = this ; m_self = this ; m_instance = 0 ;
	    m_priority = -INT_MAX ; m_id = 0 ; m_name = 0 ; m_tag = 0 ; }
	 }
      void init(T *inst, int prio, int id, const char *nm,FrSymbol *tg)
	 {
	 m_self = this ;
	 m_instance = inst ;
	 m_priority = prio ;
	 m_id = id ;
	 m_name = FrDupString(nm) ;
	 m_tag = tg ;
	 insert() ;
	 return ;
	 }
   public:
      FrSelfRegistering(T *inst, int prio, int id, const char *nm,
			FrSymbol *tg = 0)
	 { init(inst,prio,id,nm,tg) ; }
      FrSelfRegistering(T *inst, int prio, int id, const char *nm,
			const char *tg)
	 { init(inst,prio,id,nm,tg?FrSymbolTable::add(tg):0) ; }
      ~FrSelfRegistering()
	 {
	 FrFree(m_name) ;	m_name = 0 ;
	 unlink() ;
	 m_self = 0 ;
	 return ;
	 }

      // accessors
      static unsigned numInstances() { return s_numinstances ; }
      static FrSelfRegistering<T> *first() { return s_instances.next() ; }
      FrSelfRegistering<T> *next() const { return m_next ; }
      FrSelfRegistering<T> *prev() const { return m_prev ; }
      T *instance() const { return m_instance ; }
      int priority() const { return m_priority ; }
      int ID() const { return m_id ; }
      const char *name() const { return m_name ; }
      FrSymbol *tag() const { return m_tag ; }

      static FrSelfRegistering<T> *find(int id)
	 {
	 FrSelfRegistering<T> *inst = first() ;
	 for ( ; inst != &s_instances ; inst = inst->next())
	    {
	    if (inst->ID() == id)
	       return inst ;
	    }
	 // if we get here, there was no matching instance
	 return 0 ;
	 }
      static FrSelfRegistering<T> *find(FrSymbol *tag)
	 {
	 FrSelfRegistering<T> *inst = first() ;
	 for ( ; inst != &s_instances ; inst = inst->next())
	    {
	    if (inst->tag() == tag)
	       return inst ;
	    }
	 // if we get here, there was no matching instance
	 return 0 ;
	 }
      static FrSelfRegistering<T> *find(const char *tag)
	 { return find(tag ? FrSymbolTable::add(tag) : 0) ; }
      static FrSelfRegistering<T> *findVA(bool (*cmp)(const T*,va_list),
					  va_list args)
	 {
	 FrSelfRegistering<T> *inst = first() ;
	 for ( ; inst != &s_instances ; inst = inst->next())
	    {
	    if (cmp(inst,args))
	       return inst ;
	    }
	 // if we get here, there was no matching instance
	 return 0 ;
	 }
      static FrSelfRegistering<T> *find(bool (*cmp)(const T*,va_list), ...)
	 {
	 FrVarArgs(cmp) ;
	 FrSelfRegistering<T> *inst = findVA(cmp,args) ;
	 FrVarArgEnd() ;
	 return inst ;
	 }
      static FrSelfRegistering<T> *findByName(const char *name)
	 {
	 FrSelfRegistering<T> *inst = first() ;
	 for ( ; inst != &s_instances ; inst = inst->next())
	    {
	    if (name && inst->name() && strcmp(inst->name(),name) == 0)
	       return inst ;
	    else if (!name && !inst->name())
	       return inst ;
	    }
	 // if we get here, there was no matching instance
	 return 0 ;
	 }

      // manipulators
      void setPrev(FrSelfRegistering<T> *p) { m_prev = p ; }
      void setNext(FrSelfRegistering<T> *n) { m_next = n ; }
      void insert()
	 {
	 s_instances.init() ;
	 FrSelfRegistering<T> *inst = first() ;
	 while (inst != &s_instances && inst->priority() > priority())
	    {
	    inst = inst->next() ;
	    }
	 // OK, we're now pointing at the instance we want to insert ourself
	 //   before, so update the links
	 m_prev = inst->prev() ;
	 m_next = inst ;
	 m_prev->m_next = this ;
	 inst->m_prev = this ;
	 s_numinstances++ ;
	 return ;
	 }
      void unlink()
	 {
	 FrSelfRegistering<T> *prv = m_prev ;
	 FrSelfRegistering<T> *nxt = m_next ;
	 nxt->setPrev(prv) ;
	 prv->setNext(nxt) ;
	 m_prev = this ;
	 m_next = this ;
	 if (s_numinstances > 0) s_numinstances-- ;
	 return ;
	 }
      void setName(const char *nm)
	 {
	 FrFree(m_name) ;
	 m_name = FrDupString(nm) ;
	 return ;
	 }

      // iterators
      static bool iterateVA(int prio, FrComparisonType sense,
			    bool (*fn)(T *,va_list), va_list args)
	 {
	 FrSelfRegistering<T> *inst = first() ;
	 bool done = false ;
	 FrSelfRegistering<T> *nxt ;
	 for ( ; inst != &s_instances && !done ; inst = nxt)
	    {
	    nxt = inst->next() ;
	    if (FrComparePriority(inst->priority(),prio,sense,&done))
	       {
	       FrSafeVAList(args) ;
	       bool success = fn(inst->instance(),FrSafeVarArgs(args)) ;
	       FrSafeVAListEnd(args) ;
	       if (!success)
		  return false ;
	       }
	    }
	 return true ;
	 }
      static bool iterate(int prio, FrComparisonType sense,
			  bool (*fn)(T *,va_list), ...)
	 {
	 FrVarArgs(fn) ;
	 bool success = iterateVA(prio,sense,fn,args) ;
	 FrVarArgEnd() ;
	 return success ;
	 }
      static bool iterateAllVA(bool (*fn)(T *,va_list), va_list args)
	 {
	 FrSelfRegistering<T> *inst = first() ;
	 FrSelfRegistering<T> *nxt ;
	 for ( ; inst != &s_instances ; inst = nxt)
	    {
	    nxt = inst->next() ;
	    FrSafeVAList(args) ;
	    bool success = fn(inst->instance(),FrSafeVarArgs(args)) ;
	    FrSafeVAListEnd(args) ;
	    if (!success)
	       return false ;
	    }
	 return true ;
	 }
      static bool iterateAll(bool (*fn)(T *,va_list), ...)
	 {
	 FrVarArgs(fn) ;
	 bool success = iterateAllVA(fn,args) ;
	 FrVarArgEnd() ;
	 return success ;
	 }
   } ;

template <class T> FrSelfRegistering<T> FrSelfRegistering<T>::s_instances ;
template <class T> unsigned FrSelfRegistering<T>::s_numinstances(0) ;

#endif /* !__FRSLFREG_H_INCLUDED */

// end of file frslfreg.h //
