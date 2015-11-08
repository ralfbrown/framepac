/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC -- frame manipulation in C++				*/
/*  Version 2.01 							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frhash.cpp		template class FrHashTable		*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,1998,1999,2000,2001,2004,2005,	*/
/*		2006,2008,2009,2013,2015				*/
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

#if defined(__GNUC__)
#  pragma implementation "frhash.h"
#endif

#include "frhash.h"
#include "frreader.h"
#include "frutil.h"
#include "frpcglbl.h"

/************************************************************************/
/*	Manifest Constants						*/
/************************************************************************/

/************************************************************************/
/*    Global data local to this module					*/
/************************************************************************/

/************************************************************************/
/*    Global variables for template class FrHashTable			*/
/************************************************************************/

static FrObject *read_HashTable(istream &input, const char *digits) ;
static FrObject *string_to_HashTable(const char *&input, const char *digits) ;
static bool verify_HashTable(const char *&input, const char *digits,bool) ;

template <>
FrReader *FrObjHashTable::s_reader = new FrReader(string_to_HashTable,read_HashTable,
						  verify_HashTable,
						  FrREADER_LEADIN_LISPFORM,"H") ;

FrPER_THREAD size_t my_job_id = 0 ;
size_t FramepaC_small_primes[] =  // all odd primes < 2^7
   { 127, 113, 109, 107, 103, 101, 97, 89, 83, 79, 73, 71, 67, 61, 59,
     53, 47, 43, 41, 37, 31, 29, 23, 19, 17, 13, 11, 7, 5, 3, 0 } ;

/************************************************************************/
/*	Methods for class FrHashTable_Stats[_w_Methods]			*/
/************************************************************************/

void FrHashTable_Stats_w_Methods::clear()
{
   insert = 0 ;
   insert_dup = 0 ;
   insert_attempt = 0 ;
   insert_forwarded = 0 ;
   insert_resize = 0 ;
   remove = 0 ;
   remove_found = 0 ;
   remove_forwarded = 0 ;
   contains = 0 ;
   contains_found = 0 ;
   contains_forwarded = 0 ;
   lookup = 0 ;
   lookup_found = 0 ;
   lookup_forwarded = 0 ;
   resize = 0 ;
   resize_assist = 0 ;
   resize_cleanup = 0 ;
   reclaim = 0 ;
   move = 0 ;
   neighborhood_full = 0 ;
   CAS_coll = 0 ;
   chain_lock_count = 0 ;
   chain_lock_coll = 0 ;
   spin = 0 ;
   yield = 0 ;
   sleep = 0 ;
   none = 0 ;
   return ;
}

//----------------------------------------------------------------------

void FrHashTable_Stats_w_Methods::add(const FrHashTable_Stats *stats)
{
   FrCriticalSection::increment(insert,stats->insert) ;
   FrCriticalSection::increment(insert_dup,stats->insert_dup) ;
   FrCriticalSection::increment(insert_attempt,stats->insert_attempt) ;
   FrCriticalSection::increment(insert_forwarded,stats->insert_forwarded) ;
   FrCriticalSection::increment(insert_resize,stats->insert_resize) ;
   FrCriticalSection::increment(remove,stats->remove) ;
   FrCriticalSection::increment(remove_found,stats->remove_found) ;
   FrCriticalSection::increment(remove_forwarded,stats->remove_forwarded) ;
   FrCriticalSection::increment(contains,stats->contains) ;
   FrCriticalSection::increment(contains_found,stats->contains_found) ;
   FrCriticalSection::increment(contains_forwarded,stats->contains_forwarded) ;
   FrCriticalSection::increment(lookup,stats->lookup) ;
   FrCriticalSection::increment(lookup_found,stats->lookup_found) ;
   FrCriticalSection::increment(lookup_forwarded,stats->lookup_forwarded) ;
   FrCriticalSection::increment(resize,stats->resize) ;
   FrCriticalSection::increment(resize_assist,stats->resize_assist) ;
   FrCriticalSection::increment(resize_cleanup,stats->resize_cleanup) ;
   FrCriticalSection::increment(reclaim,stats->reclaim) ;
   FrCriticalSection::increment(move,stats->move) ;
   FrCriticalSection::increment(neighborhood_full,stats->neighborhood_full) ;
   FrCriticalSection::increment(CAS_coll,stats->CAS_coll) ;
   FrCriticalSection::increment(spin,stats->spin) ;
   FrCriticalSection::increment(yield,stats->yield) ;
   FrCriticalSection::increment(sleep,stats->sleep) ;
   return ;
}

/************************************************************************/
/*	Methods for template class FrHashTable				*/
/************************************************************************/

static FrObject *string_to_HashTable(const char *&input, const char *digits)
{
   FrObjHashTable *ht ;
   int table_size ;
   if (digits)
      {
      table_size = atol(digits) ;
      if (table_size < 10)
	 table_size = 10 ;
      }
   else
      table_size = 10 ;
   int c = *input++ ;
   if (c == '(')
      {
      ht = new FrObjHashTable(table_size) ;
      c = FrSkipWhitespace(input) ;
      while (c && c != ')')
	 {
	 FrObject *obj = string_to_FrObject(input) ;
	 if (obj)
	    {
	    ht->add(obj) ;
	    }
	 c = FrSkipWhitespace(input) ;
	 }
      }
   else
      ht = 0 ;
   return ht ;
}

//----------------------------------------------------------------------

static bool verify_HashTable(const char *&input, const char *, bool)
{
   int c = FrSkipWhitespace(input) ;
   if (c == '(')
      {
      input++ ;				// consume the left parenthesis
      c = FrSkipWhitespace(input) ;
      while (c && c != ')')
	 {
	 if (!valid_FrObject_string(input,true))
	    return false ;
	 c = FrSkipWhitespace(input) ;
	 }
      if (c == ')')
	 {
	 input++ ;			// skip terminating right paren
	 return true ;			//   and indicate success
	 }
      }
   return false ;
}

//----------------------------------------------------------------------

static FrObject *read_HashTable(istream &input, const char *digits)
{
   FrObjHashTable *ht ;
   int table_size ;
   if (digits)
      {
      table_size = atol(digits) ;
      if (table_size < 10)
	 table_size = 10 ;
      }
   else
      table_size = 10 ;
   int c = input.get() ;
   if (c == '(')
      {
      ht = new FrObjHashTable(table_size) ;
      c = FrSkipWhitespace(input) ;
      while (c && c != ')')
	 {
	 FrObject *obj ;
	 input >> obj ;
	 if (obj)
	    {
	    ht->add(obj) ;
	    }
	 c = FrSkipWhitespace(input) ;
	 }
      }
   else
      ht = 0 ;
   return ht ;
}

// end of file frhash.cpp //
