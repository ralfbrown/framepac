/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File vframe.h    -- "virtual memory" frames for FramepaC            */
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,2001,2002,2009			*/
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

#ifndef __VFRAME_H_INCLUDED
#define __VFRAME_H_INCLUDED

#ifndef __FRFRAME_H_INCLUDED
#include "frframe.h"
#endif

#ifndef __FRSYMTAB_H_INCLUDED
#include "frsymtab.h"
#endif

#if defined(__GNUC__)
#  pragma interface
#endif

/**********************************************************************/
/**********************************************************************/

#define NO_FRAME_ID (-1L)

/**********************************************************************/
/**********************************************************************/

enum BackingStore
   {
   BS_none,
   BS_diskfile,
   BS_server,
   BS_peer
   } ;

struct DBFILE ;

/**********************************************************************/
/**********************************************************************/

struct DBUserData
   {
   char type[2] ;
   char browse_cfgnum[2] ;
   char editor_cfgnum[2] ;
   char browse_cfgfile[16] ;
   char editor_cfgfile[16] ;
   char rootframe[32] ;
   char password[16] ;
   char readwrite_levels ;
   char admin_level ;
   char pad[8] ;    // pad out to 96 bytes
   void *operator new(size_t size) { return FrMalloc(size) ; }
   void operator delete(void *blk) { FrFree(blk) ; }
   } ;

inline int getDBUserData_readlevel(DBUserData *udata)
   { return (udata->readwrite_levels >> 4) & 15 ; }
inline int getDBUserData_writelevel(DBUserData *udata)
   { return udata->readwrite_levels & 15 ; }
inline int getDBUserData_adminlevel(DBUserData *udata)
   { return udata->admin_level & 15 ; }
inline void setDBUserData_readlevel(DBUserData *udata,int newlevel)
   { udata->readwrite_levels = (char)((udata->readwrite_levels & 15) |
				      ((newlevel & 15) << 4)) ; }
inline void setDBUserData_writelevel(DBUserData *udata,int newlevel)
   { udata->readwrite_levels = (char)((udata->readwrite_levels & ~15) |
				      (newlevel & 15)) ; }
inline void setDBUserData_adminlevel(DBUserData *udata,int newlevel)
   { udata->admin_level = (char)((udata->admin_level & ~15) |
				 (newlevel & 15)) ; }

/**********************************************************************/
/**********************************************************************/

typedef void frame_update_hookfunc(FrSymbol *framename) ;

/**********************************************************************/
/*       Declarations for class VFrame                               */
/**********************************************************************/

class VFrame : public FrFrame
   {
   private:
      // none--all data members are in FrFrame
   public:
      VFrame(FrSymbol *framename) ;
      virtual ~VFrame() ;
      virtual FrObjectType objType() const ;
      virtual const char *objTypeName() const ;
      virtual FrObjectType objSuperclass() const ;
//    virtual ostream &printValue(ostream &output) const ;
//    virtual char *displayValue(char *buffer) const ;
//    virtual size_t displayLength() const ;
   } ;

//----------------------------------------------------------------------
// procedural interface to FramepaC virtual frame manipulation capabilities

bool __FrCDECL do_slots(FrSymbol *frame,FrSlotsFuncFrame *func, ...) ;
bool __FrCDECL do_facets(FrSymbol *frame,const FrSymbol *slotname,
			   FrFacetsFuncFrame *func, ...) ;
// use frame->iterate() or frame->iterateVA() instead of do_all_facets
//bool __FrCDECL do_all_facets(FrSymbol *frame, FrAllFacetsFuncFrame *func,
//				 ...) ;
int commit_all_frames() ;
VFrame *load_frame(const char *framerep, bool locked = false) ;

//----------------------------------------------------------------------

FrSymbolTable *initialize_VFrames_memory(int symtabsize) ;
FrSymbolTable *initialize_VFrames_disk(const char *filename, int symtabsize,
				       bool transactions = true,
				       bool force_create = true,
   				       const char *password = 0) ;
FrSymbolTable *initialize_VFrames_server(const char *servername, int port,
					 const char *username,
					 const char *password,
					 const char *database,
					 int symtabsize,
					 bool transactions = true,
					 bool force_create = true) ;

bool store_VFrame(const FrSymbol *name) ;
int synchronize_VFrames(frame_update_hookfunc *hook = 0) ;
int shutdown_VFrames(FrSymbolTable *symtab = 0) ;
bool get_DB_user_data(DBUserData *user_data) ;
bool set_DB_user_data(DBUserData *user_data) ;
int prefetch_frames(FrList *frames) ;

//----------------------------------------------------------------------

FrList *collect_prefix_matching_frames(const char *prefix,char *longest_prefix,
				     int longprefix_buflen) ;
char *complete_frame_name(const char *prefix) ;

/**********************************************************************/
/**********************************************************************/

char *database_index_name(char *dbname, int indexnumber) ;
long int frameID_for_symbol(const FrSymbol *sym) ;
FrSymbol *symbol_for_frameID(long int ID) ;

//----------------------------------------------------------------------

#define INDEX_BYNAME 		0
#define INDEX_INVSLOTS 		1
#define INDEX_INVFILLERS 	2
#define INDEX_INVWORDS 		3

char *VFrames_indexfile() ;
int VFrames_indexstream(int idxtype) ;
void VFrames_set_indexes(bool byslot, bool byfiller, bool byword) ;
void VFrames_get_indexes(bool *byslot, bool *byfiller, bool *byword) ;

/**********************************************************************/
/*	Miscellaneous declarations				      */
/**********************************************************************/

FrList *available_databases(FrSymbolTable *symtab = 0) ;
FrList *available_databases(const char *servername) ;
const char *FramepaC_get_db_dir() ;
void FramepaC_set_db_dir(const char *dir) ;

#endif /* __VFRAME_H_INCLUDED */

// end of file vframe.h //
