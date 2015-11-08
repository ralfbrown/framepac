/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File friindex.cpp	Mikrokosmos database inverted indices		*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1995,1996,1997,1998,2000,2001,2006,2009,2010,2013,	*/
/*		2015 Ralf Brown/Carnegie Mellon University		*/
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

#if defined(__MSDOS__) || defined(__WATCOMC__) || defined(_MSC_VER)
#  include <io.h>
#endif

#if defined(unix) || defined(__linux__) || defined(__GNUC__)
#  include <unistd.h>	// needed by RedHat 7.1
#endif

using namespace std ;

#include "frhash.h"
#include "frhasht.h"
#include "frlist.h"
#include "frmem.h"
#include "frnumber.h"

/************************************************************************/
/************************************************************************/

typedef unsigned char LONGbuf[4] ;

//----------------------------------------------------------------------

struct DiskIndexNode
   {
   LONGbuf name ;
   LONGbuf num_IDs ;
   LONGbuf firstblock ;
   } ;

#define INDEX_NODES_PER_BLOCK 4
#define INDEX_BLOCKSIZE \
      (INDEX_NODES_PER_BLOCK*sizeof(DiskIndexNode)/sizeof(LONGbuf))

#define DiskIndexBlockName_SIZE ((INDEX_BLOCKSIZE+2)*sizeof(LONGbuf)-1)
struct DiskIndexBlockName
   {
   char type ;
   char name[DiskIndexBlockName_SIZE] ;
   } ;

#define DiskIndexBlockBitmap_SIZE ((INDEX_BLOCKSIZE+2)*sizeof(LONGbuf)-1)
struct DiskIndexBlockBitmap
   {
   char type ;
   char bitmap[DiskIndexBlockBitmap_SIZE] ;
   } ;

#define DiskIndexBlockIndex_SIZE (INDEX_BLOCKSIZE+2)
struct DiskIndexBlockIndex
   {
   LONGbuf index[DiskIndexBlockIndex_SIZE] ;
   } ;

#define DiskIndexBlockNode_SIZE (INDEX_NODES_PER_BLOCK)
struct DiskIndexBlockNode
   {
   LONGbuf right, left ;
   DiskIndexNode node[DiskIndexBlockNode_SIZE] ;
   } ;

#define DiskIndexBlockIDList_SIZE (INDEX_BLOCKSIZE)
struct DiskIndexBlockIDList
   {
   LONGbuf right, left ;
   LONGbuf IDs[DiskIndexBlockIDList_SIZE] ;
   } ;

union DiskIndexBlock
   {
   char type ;
   DiskIndexBlockName name ;
   DiskIndexBlockBitmap bitmap ;
   DiskIndexBlockIndex index ;
   DiskIndexBlockNode node ;
   DiskIndexBlockIDList idlist ;
   } ;

#define BITMAP_BITS_PER_BLOCK (DiskIndexBlockBitmap_SIZE*8)

//----------------------------------------------------------------------

enum DiskIndexBlockType
   {
   DIB_name0 = 0, DIB_name1, DIB_name2, DIB_name3,
   DIB_node, DIB_idlist, DIB_bitmap, /* unused = 7, */
   DIB_index0 = 8, DIB_index1, DIB_index2, DIB_index3, DIB_index4, DIB_index5,
   } ;

//----------------------------------------------------------------------

enum FrHashEntryTypeInvIndex
   {
   HE_raw = 100,
   HE_name,
   HE_node,
   HE_bitmap,
   HE_index,
   HE_idlist,
   } ;

//----------------------------------------------------------------------

class FrInvIndexEntry : public FrHashEntry
   {
   private:
      FrInvIndexEntry *nextLRU, *prevLRU ;
      int32_t blocknumber ;
      int32_t parentblock ;
      bool dirty ;
   public:
      FrInvIndexEntry() ;
      FrInvIndexEntry(int32_t blknum)
    	 { blocknumber = blknum ; dirty = false ; }
      virtual ~FrInvIndexEntry() ;
      virtual FrHashEntryType entryType() const ;
      virtual FrSymbol *entryName() const ;
      virtual int sizeOf() const ;
      virtual size_t hashIndex(int size) const ;
      virtual int keycmp(const FrHashEntry *entry) const ;
      virtual FrObject *copy() const ;
      //!!!
      void markDirty() { dirty = true ; }
      void markClean() { dirty = false ; }
      bool isDirty() const { return dirty ; }
      void setParent(int32_t parent) { parentblock = parent ; }
      int32_t parentBlock() const { return parentblock ; }
      int32_t blockNumber() const { return blocknumber ; }
      void flush(FrObjHashTable *cache, int fd) ;
   } ;

//----------------------------------------------------------------------

class FrIIEntryName : public FrInvIndexEntry
   {
   private:
      static FrAllocator allocator ;
      char *name ;
      int extent ;  // number of consecutive blocks used to store name
   public:
      void *operator new(size_t) { return allocator.allocate() ; }
      void operator delete(void *blk) { allocator.release(blk) ; }
      FrIIEntryName() ;
      virtual ~FrIIEntryName() ;
      static void compact(FrMemFooter *pages, FrFreeList **freelist,
			  FrMemFooter **freed) ;
      const char *getName() const { return name ; }
   } ;

//----------------------------------------------------------------------

class FrIIEntryNode : public FrInvIndexEntry
   {
   private:
      static FrAllocator allocator ;
      FrIIEntryNode *left, *right ;
      char *name[INDEX_NODES_PER_BLOCK] ;
      FrIIEntryName *nameblock[INDEX_NODES_PER_BLOCK] ;
      int32_t num_IDs[INDEX_NODES_PER_BLOCK] ;
      int32_t firstblock[INDEX_NODES_PER_BLOCK] ;
   public:
      void *operator new(size_t) { return allocator.allocate() ; }
      void operator delete(void *blk) { allocator.release(blk) ; }
      FrIIEntryNode() ;
      virtual ~FrIIEntryNode() ;
      static void compact(FrMemFooter *pages, FrFreeList **freelist,
			  FrMemFooter **freed) ;
   } ;

//----------------------------------------------------------------------

class FrIIEntryIndex : public FrInvIndexEntry
   {
   private:
      static FrAllocator allocator ;
      int32_t ptrs[DiskIndexBlockIndex_SIZE] ;
   public:
      void *operator new(size_t) { return allocator.allocate() ; }
      void operator delete(void *blk) { allocator.release(blk) ; }
      FrIIEntryIndex() ;
      virtual ~FrIIEntryIndex() ;
      static void compact(FrMemFooter *pages, FrFreeList **freelist,
			  FrMemFooter **freed) ;
   } ;

//----------------------------------------------------------------------

typedef FrList *FrIndexComponentFunc(const FrFrame *) ;

class FrInvertedIndex
   {
   private:
      int fd ;
      int totalblocks ;
      int cachecount ;
      int32_t rootnode ;
      int32_t freelist ;
      FrIndexComponentFunc *explode ;
      FrObjHashTable *cache ;
   public:
      FrInvertedIndex(const char *filename,FrIndexComponentFunc *) ;
      ~FrInvertedIndex() ;
      void addFrame(long int frameID, const FrFrame *frame) ;
      void deleteFrame(long int frameID, const FrFrame *frame) ;
      void updateFrame(long int frameID, const FrFrame *oldvalue,
		       const FrFrame *newvalue) ;
      void flush() ;
      void flushEntry(int32_t blocknum) ;
      FrIIEntryNode *getNode(int32_t blocknum) ;
      FrList *framesForKey(FrObject *key) ;
   } ;

/************************************************************************/
/*	Global variables for various classes				*/
/************************************************************************/

FrAllocator FrIIEntryName::allocator("InvIndex Name",sizeof(FrIIEntryName),
				     &FrIIEntryName::compact) ;
FrAllocator FrIIEntryNode::allocator("InvIndex Node",sizeof(FrIIEntryNode),
				     &FrIIEntryNode::compact) ;
FrAllocator FrIIEntryIndex::allocator("InvIndex Index",
				      sizeof(FrIIEntryIndex),
				      &FrIIEntryIndex::compact) ;

/************************************************************************/
/*		  							*/
/************************************************************************/

/************************************************************************/
/*	Methods for class FrIIEntryName					*/
/************************************************************************/

void FrIIEntryName::compact(FrMemFooter *pages, FrFreeList **freelist,
			    FrMemFooter **freed)
{
   (void)pages ; (void)freelist ; (void)freed ;

   return ;
}

/************************************************************************/
/*	Methods for class FrIIEntryNode					*/
/************************************************************************/

void FrIIEntryNode::compact(FrMemFooter *pages, FrFreeList **freelist,
			    FrMemFooter **freed)
{
   (void)pages ; (void)freelist ; (void)freed ;

   return ;
}

/************************************************************************/
/*	Methods for class FrIIEntryBitmap				*/
/************************************************************************/

#if 0
void FrIIEntryBitmap::compact(FrMemFooter *pages, FrFreeList **freelist,
			      FrMemFooter **freed)
{
   (void)pages ; (void)freelist ; (void)freed ;

   return ;
}
#endif

/************************************************************************/
/*	Methods for class FrIIEntryIndex				*/
/************************************************************************/

void FrIIEntryIndex::compact(FrMemFooter *pages, FrFreeList **freelist,
			     FrMemFooter **freed)
{
   (void)pages ; (void)freelist ; (void)freed ;

   return ;
}

/************************************************************************/
/*    Methods for class FrInvertedIndex					*/
/************************************************************************/

FrInvertedIndex::FrInvertedIndex(const char *filename,
				 FrIndexComponentFunc *componentfunc)
{
   (void)filename;

   explode = componentfunc ;
   return ;
}

//----------------------------------------------------------------------

FrInvertedIndex::~FrInvertedIndex()
{
   // flush the cache
   if (cache)
      {
      flush() ;
      cache->freeObject() ;
      cache = 0 ;
      }
   // close the index file
   close(fd) ;
   return ;
}

//----------------------------------------------------------------------

static void add_IDs(int fd, FrInvertedIndex *index, long int frameID,
		    FrList *add)
{
(void)fd ; (void)index ; (void)frameID ;
   while (add)
      {

      add = add->rest() ;
      }
   return ;
}

//----------------------------------------------------------------------

static void remove_IDs(int fd, FrInvertedIndex *index, long int frameID,
		       FrList *remove)
{
(void)fd ; (void)index ; (void)frameID ;
   while (remove)
      {

      remove = remove->rest() ;
      }
   return ;
}

//----------------------------------------------------------------------

void FrInvertedIndex::addFrame(long int frameID, const FrFrame *frame)
{
   if (explode)
      {
      FrList *to_add = explode(frame) ;
      add_IDs(fd,this,frameID,to_add) ;
      free_object(to_add) ;
      }
   return ;
}

//----------------------------------------------------------------------

void FrInvertedIndex::deleteFrame(long int frameID, const FrFrame *frame)
{
   if (explode)
      {
      FrList *to_delete = explode(frame) ;
      remove_IDs(fd,this,frameID,to_delete) ;
      free_object(to_delete) ;
      }
   return ;
}

//----------------------------------------------------------------------

void FrInvertedIndex::updateFrame(long int frameID, const FrFrame *oldvalue,
				  const FrFrame *newvalue)
{
   if (explode)
      {
      FrList *old_list = explode(oldvalue) ;
      FrList *new_list = explode(newvalue) ;
      FrList *to_delete = old_list->difference(new_list) ;
      FrList *to_add = new_list->ndifference(old_list) ;
      if (to_delete)
	 remove_IDs(fd,this,frameID,to_delete) ;
      if (to_add)
	 add_IDs(fd,this,frameID,to_add) ;
      free_object(to_delete) ;
      free_object(to_add) ;
      }
   return ;
}

//----------------------------------------------------------------------

void FrInvertedIndex::flushEntry(int32_t blocknum)
{
   FrObject *info ;
   FrInteger key(blocknum) ;
   (void)cache->lookup((FrObject*)&key,&info) ;
   FrInvIndexEntry *entry = (FrInvIndexEntry*)info ;
   if (entry)
      entry->flush(cache,fd) ;
   return ;
}

//----------------------------------------------------------------------

static bool flush_block(FrObject *blknum, FrObject *ent, va_list args)
{
   FrInvIndexEntry *entry = (FrInvIndexEntry*)ent ;
   FrVarArg(FrSymHashTable *,cache) ;
   FrVarArg(int,fd) ;
   if (entry->isDirty())
      {
      (void)blknum ;
      (void)fd ; (void)cache; //!!!
      entry->markClean() ;
      }
   return true ;
}

//----------------------------------------------------------------------

void FrInvertedIndex::flush()
{
   if (cache)
      {
      cache->iterate(flush_block,cache,fd) ;
      }
   return ;
}

//----------------------------------------------------------------------


// end of file friindex.cpp //
