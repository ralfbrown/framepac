/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File mikro_db.h	Mikrokosmos database manager			*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,1999,2009,2015			*/
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

#ifndef __MIKRO_DB_H
#define __MIKRO_DB_H

#ifndef __FRAMERR_H_INCLUDED
#include "framerr.h"
#endif

#ifndef __VFRAME_H_INCLUDED
#include "vframe.h"
#endif

#ifndef __FRHASH_H_INCLUDED
#include "frhash.h"
#endif

#ifdef FrFRAME_ID
#include "frameid.h"
#endif

#ifndef __FRBYTORD_H_INCLUDED
#include "frbytord.h"
#endif

/**********************************************************************/
/**********************************************************************/

class HashEntryVFrame ;

/**********************************************************************/
/*	Manifest constants					      */
/**********************************************************************/

#if defined(__MSDOS__) || defined(MSDOS)
#  define LOGFILE_EXT ".l"
#  define LOGFILE_EXT_LEN 4  // ".lNN"
#  define LOGFILE_EXT_DIGITS "2"
#  define LOGFILE_EXT_MAX 99
#  define INDEX_EXTENSION ".dx"
#else
#  define LOGFILE_EXT ".log"
#  define LOGFILE_EXT_LEN 7    // ".logNNN"
#  define LOGFILE_EXT_DIGITS "3"
#  define LOGFILE_EXT_MAX 999
#  define INDEX_EXTENSION ".idx"
#endif

#define MAX_ACTIVE_TRANSACTIONS 10

//----------------------------------------------------------------------

#define DB_SIGNATURE "<<FramepaC Database File>> do not manually edit\n"
#define INDEX_SIGNATURE "<<FramepaC Database Index>> do not manually edit\n"
#define LOG_SIGNATURE "<<FramepaC Database LogFile>> do not manually edit\n"

/**********************************************************************/
/**********************************************************************/

struct DBFILE
   {
   char *database_name ;
   char *indexfile_name ;
   char *logfile_name ;
   FrSymHashTable *index ;
#ifdef FrFRAME_ID
   FrameIdentDirectory *frame_IDs ;
#endif /* FrFRAME_ID */
   long transactions[MAX_ACTIVE_TRANSACTIONS] ;
#ifdef FrEXTRA_INDEXES
   int byfiller_fd, byslot_fd, byword_fd ;
   int extra_indexes ;
#endif /* FrEXTRA_INDEXES */
   int active_trans ;
   int db_file ;
   int indexfile ;
   int logfile ;
   int entrycount ;
   int origentrycount ;
   DBUserData user_data ;
   bool readonly ;
   //...
   void *operator new(size_t size) { return FrMalloc(size) ; }
   void operator delete(void *obj) { FrFree(obj) ; }
   DBFILE() ;
   ~DBFILE() ;
   int extra_index_stream(int idxtype) const ;
   } ;

/**********************************************************************/
/**********************************************************************/

int file_sync(int fd) ;
int file_read(void *buf, int len, int fd) ;
int file_write(const void *buf, int len, int fd, int logfd = -1) ;
int file_append(void *buf, int len, int fd, int logfd = -1) ;
int file_close(int fd, int logfd = -1, bool readonly = false) ;
int write_long(int fd, long int value, int logfd = -1) ;

int log_frame_create(DBFILE *db,const FrSymbol *frname) ;
int log_frame_delete(DBFILE *db,const FrSymbol *frname) ;

int start_database_transaction(DBFILE *db) ;
int end_database_transaction(DBFILE *db,int transaction) ;
int abort_database_transaction(DBFILE *db,int transaction) ;

DBFILE *open_database(const char *database, bool createnew = false,
		      bool transactions = true, const char *password = 0
#ifdef FrFRAME_ID
		      ,FrameIdentDirectory *frame_IDs = 0
#endif /* FrFrameID */
		      ) ;
void load_db_configuration(DBFILE *db) ;
int update_database_header(DBFILE *db) ;

FrList *find_databases(const char *directory, const char *mask = 0) ;
char *read_database(DBFILE *db, HashEntryVFrame *entry, bool *deleted = 0) ;
char *read_old_entry(DBFILE *db, HashEntryVFrame *entry, int generation) ;
bool write_database_entry(DBFILE *db,const FrSymbol *frname) ;
bool flush_database_file(DBFILE *db) ;
bool update_database_record(FrHashEntry *ent,DBFILE *db,bool force,bool synch) ;
int close_database(DBFILE *db) ;

void set_database_header(DBUserData *user_data) ;
void FramepaC_select_extra_indexes(bool byslot, bool byfiller, bool byword) ;
void FramepaC_determine_extra_indexes(bool *byslot, bool *byfiller,
				      bool *byword) ;

#endif /* !__MIKRO_DB_H */

// end of file mikro-db.h //
