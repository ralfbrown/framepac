/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File mikro_db.cpp	Mikrokosmos database manager			*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,1998,2000,2001,2006,2009,2011,	*/
/*		2013,2015 Ralf Brown/Carnegie Mellon University		*/
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

#include "frconfig.h"
#ifdef FrDATABASE

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "frpcglbl.h"
#include "frfinddb.h"
#include "frdathsh.h"
#include "frfilutl.h"
#include "frnumber.h"
#include "frpasswd.h"
#include "frprintf.h"
#include "vfinfo.h"
#include "mikro_db.h"

#ifdef FrSTRICT_CPLUSPLUS
#  include <fstream>
#else
#  include <fstream.h>
#endif

#ifdef FrEXTRA_INDEXES
#include "inv.h"
#endif /* FrEXTRA_INDEXES */

/**********************************************************************/

#if defined(__MSDOS__) || defined(__WATCOMC__) || defined(_MSC_VER)
#  include <dos.h>
#  include <io.h>
#  include <sys/locking.h>
#  define ftruncate chsize
#elif defined(__SUNOS__) || defined(__SOLARIS__) || defined(unix) || defined(__linux__) || defined(__GNUC__)
#  include <unistd.h>
#endif /* __MSDOS__ || __WATCOMC__ || _MSC_VER */

/**********************************************************************/

#if !defined(O_BINARY)
#  define O_BINARY 0
#endif

/**********************************************************************/
/*	Manifest Constants					      */
/**********************************************************************/

#define DB_FORMAT_VERSION     0x0100	  // version number for DB format
#define MIN_DB_VERSION	      0x0001	  // oldest supported DB format

#define DBCONFIG_FILENAME	"%s/dbcfg-%c%c.cfg"
#define DBCONFIG_TYPE_LOC	6         // change offsets 6&7 to type tag

#define TRANSACTION_SIGNATURE ((unsigned short int)0xBACCu)

#define TRANSACTION_HEADER 	0xFF
#define TRANSACTION_BEGIN 	0x00
#define TRANSACTION_END 	0x01
#define TRANSACTION_ABORT 	0x02
#define TRANSACTION_WRITE 	0x03
#define TRANSACTION_APPEND 	0x04
#define TRANSACTION_CREATEFRAME 0x05
#define TRANSACTION_DELETEFRAME 0x06


#ifdef __MSDOS__
#  define TEMPBUFFER_SIZE (2*FrFOOTER_OFS-16)
#else
#  define TEMPBUFFER_SIZE (2*FrFOOTER_OFS-100)
#endif

/**********************************************************************/
/*	Types local to this module				      */
/**********************************************************************/

typedef unsigned char LONGbuf[4] ;

//----------------------------------------------------------------------

struct DB_FRAME_HEADER
   {
   LONGbuf prev_offset ;
   LONGbuf rec_size ;
   char deleted ;
   } ;
// define the header size explicitly rather than using sizeof() to avoid
// compiler padding
#define DB_FRAME_HEADER_SIZE 9

//----------------------------------------------------------------------

struct DBHeaderInfo
   {
   char	has_byslot_index ;
   char has_byfiller_index ;
   char has_byword_index ;
   char pad[1] ;	// pad out to 100 bytes of info
   DBUserData user_data ;
   } ;

/************************************************************************/
/*    Global variables imported from other modules			*/
/************************************************************************/

extern int Fr_errno ;
extern bool omit_inverse_links ;

/************************************************************************/
/*    Global variables local to this module				*/
/************************************************************************/

DBHeaderInfo default_header_info =
   { 1, 1, 1, {'\0'}, { {""}, {""}, {""}, {""}, {""}, {""}, {""}, '\0', '\0', {""} } } ;
DBHeaderInfo current_header_info ;

/**********************************************************************/
/*    Forward declarations					      */
/**********************************************************************/

static int log_write(int logfd, int fd, int fileID, int len) ;
static int log_append(int logfd, int fileID, long pos) ;

/**********************************************************************/
/*    Utility Functions						      */
/**********************************************************************/

static char tempbuffer[TEMPBUFFER_SIZE] ;
static bool tempbuffer_in_use = false ;

void *tempalloc(size_t size)
{
   char *buf ;

   if (size <= sizeof(tempbuffer))
      {
      if (tempbuffer_in_use)
	 FrProgError("attempt to allocate static buffer twice in tempalloc()") ;
      tempbuffer_in_use = true ;
      return tempbuffer ;
      }
   else if ((buf = FrNewN(char,size)) != 0)
      return buf ;
   else
      {
      Fr_errno = ENOMEM ;
      return 0 ;
      }
}

//----------------------------------------------------------------------

void tempfree(void *buf)
{
   if (buf != tempbuffer)
      FrFree(buf) ;
   else
      {
      if (!tempbuffer_in_use)
	 FrProgError("attempt to free static buffer twice in tempfree()");
      tempbuffer_in_use = false ;
      }
   return ;
}

/************************************************************************/
/*    replacement file I/O function macros				*/
/************************************************************************/

typedef struct
   {
   int fd ;
   int buf_maxsize ;
   char *buf ;
   unsigned long bufoffset ;
   unsigned long filesize ;
   int bufsize ;
   int bufpos ;
   } Fr_FILE ;

#define Fr_putc(c,fp) \
  ((fp)->buf[fp->bufpos++]=(c),\
   ((fp)->bufpos>=(fp)->buf_maxsize&&Fr_flush(fp)==-1)?-1:0)

/************************************************************************/
/*	Replacement file input functions (buffered)			*/
/************************************************************************/

static Fr_FILE *Fr_fdopen(int fd,char *buf, int maxsiz)
{
   Fr_FILE *fp = FrNew(Fr_FILE) ;
   if (fp)
      {
      fp->fd = fd ;
      fp->buf = buf ;
      fp->buf_maxsize = maxsiz ;
      fp->bufsize = 0 ;
      fp->bufpos = 0 ;
      fp->bufoffset = lseek(fd,0L,SEEK_CUR) ;
      fp->filesize = lseek(fd,0L,SEEK_END) ;
      lseek(fd,fp->bufoffset,SEEK_SET) ;
      }
   return fp ;
}

//----------------------------------------------------------------------

inline unsigned long Fr_tell(Fr_FILE *fp)
{
   return fp->bufoffset + fp->bufpos ;
}

//----------------------------------------------------------------------

inline unsigned long Fr_eof(Fr_FILE *fp)
{
   return fp->bufoffset + fp->bufpos >= fp->filesize ;
}

//----------------------------------------------------------------------

inline void Fr_close(Fr_FILE *fp)
{
   FrFree(fp) ;
}

//----------------------------------------------------------------------

unsigned long Fr_read(void *buf, int count, Fr_FILE *fp)
{
   char *bufptr = (char*)buf ;
   int bufpos = fp->bufpos ;
   int bufsize = fp->bufsize ;
   int num_read ;

   if (bufpos + count <= bufsize)
      {
      memcpy(bufptr,fp->buf+bufpos,count) ;
      bufpos += count ;
      num_read = count ;
      }
   else
      {
      int left = bufsize - bufpos ;
      if (left > 0)
	 {
	 memcpy(bufptr,fp->buf+bufpos,left) ;
	 bufptr += left ;
	 count -= left ;
	 }
      num_read = left ;
      while (count > 0)
	 {
	 fp->bufoffset = fdtell(fp->fd) ;
	 fp->bufsize = read(fp->fd,fp->buf,fp->buf_maxsize) ;
	 if (fp->bufsize <= 0)
	    {
	    fp->bufpos = 0 ;
	    return num_read ;
	    }
	 if (count < fp->bufsize)
	    left = count ;
	 else
	    left = fp->bufsize ;
	 memcpy(bufptr,fp->buf,left) ;
	 bufptr += left ;
	 count -= left ;
	 bufpos = left ;
	 num_read += left ;
	 }
      }
   fp->bufpos = bufpos ;
   *((char**)buf) = bufptr ;
   return num_read ;
}

//----------------------------------------------------------------------

static long int read_long(Fr_FILE *fp)
{
   LONGbuf bytes ;

   if (Fr_read(bytes,sizeof bytes,fp) < sizeof bytes)
      return -1 ;
   return FrLoadLong(bytes) ;
}

/************************************************************************/
/*	Methods for struct DBFILE					*/
/************************************************************************/

DBFILE::DBFILE()
{
   logfile_name = indexfile_name = database_name = 0 ;
   entrycount = active_trans = 0 ;
   logfile = -1 ;
   db_file = indexfile = 0 ;
#ifdef FrEXTRA_INDEXES
   byslot_fd = byfiller_fd = byword_fd = -1 ;
   extra_indexes = 0 ;
#endif /* FrEXTRA_INDEXES */
#ifdef FrFRAME_ID
   frame_IDs = 0 ;
#endif
   index = 0 ;
   readonly = false ;
}

//----------------------------------------------------------------------

DBFILE::~DBFILE()
{
   if (logfile_name)
      FrFree(logfile_name) ;
   if (indexfile_name)
      FrFree(indexfile_name) ;
   if (database_name)
      FrFree(database_name) ;
#ifdef FrEXTRA_INDEXES
   if (byslot_fd != -1)
      close(byslot_fd) ;
   if (byfiller_fd != -1)
      close(byfiller_fd) ;
   if (byword_fd != -1)
      close(byword_fd) ;
#endif /* FrEXTRA_INDEXES */
   if (index)
      {
      delete index ;
      index = 0 ;
      }
}

//----------------------------------------------------------------------

int DBFILE::extra_index_stream(int idxtype) const
{
#ifdef FrEXTRA_INDEXES
   switch (idxtype)
      {
      case INDEX_INVSLOTS:
	 return byslot_fd ;
      case INDEX_INVFILLERS:
	 return byfiller_fd ;
      case INDEX_INVWORDS:
	 return byword_fd ;
      default:
	 return 0 ;
      }
#else
   (void)idxtype ;
   return 0 ;
#endif /* FrEXTRA_INDEXES */
}

/**********************************************************************/
/*	Low Level File I/O functions			      	      */
/**********************************************************************/

inline long int rewind_file(int fd)
{
   if (lseek(fd,0L,SEEK_SET) < 0)
      {
      Fr_errno = FE_SEEK ;
      return -1 ;
      }
   else
      return 0 ;
}

//----------------------------------------------------------------------

inline long int seek_to_end(int fd)
{
   long int pos ;

   if ((pos = lseek(fd,0L,SEEK_END)) == -1)
      {
      Fr_errno = FE_SEEK ;
      return -1 ;
      }
   else
      return pos ;
}

//----------------------------------------------------------------------

#if !defined(__MSDOS__) && !defined(_MSC_VER)
int eof(int fd)
{
   long int currpos = lseek(fd,0L,SEEK_CUR) ;
   long int size = seek_to_end(fd) ;

   lseek(fd,currpos,SEEK_SET) ;
   return (currpos < 0) || (currpos >= size) ;
}
#endif /* !__MSDOS__ && !_MSC_VER */

//----------------------------------------------------------------------

int file_sync(int fd)
{
   if (Fr_fsync(fd) == -1)
      {
      Fr_errno = FE_COMMITFAILED ;
      return -1 ;
      }
   else
      return 0 ;
}

//----------------------------------------------------------------------

bool flush_database_file(DBFILE *db)
{
   if (db->readonly)
      return true ;
   if (db->db_file && file_sync(db->db_file) == -1)
      return false ;
   if (db->indexfile && file_sync(db->indexfile) == -1)
      return false ;
   if (db->logfile && file_sync(db->logfile) == -1)
      return false ;
   return true ;
}

//----------------------------------------------------------------------

int file_read(void *buf, int len, int fd)
{
   int count ;

   if ((count = read(fd,buf,len)) < len)
      {
      Fr_errno = FE_READFAULT ;
      return -1 ;
      }
   else
      return count ;
}

//----------------------------------------------------------------------

int file_write(const void *buf, int len, int fd, int logfd)
{
   // add the change in the file to the log file for this transaction
   if (logfd > 0 && log_write(logfd,fd,fd,len) == -1)
      return -1 ;;
   // then make the actual change in the file
   if (!Fr_write(fd,buf,len,false))
      {
      Fr_errno = FE_WRITEFAULT ;
      return -1 ;
      }
   else
      return len ;
}

//----------------------------------------------------------------------

int file_append(void *buf, int len, int fd, int logfd)
{
   long pos ;

   // force the file pointer to the end of the file
   if ((pos = seek_to_end(fd)) == -1)
      return -1 ;
   // add the change in the file to the log file for this transaction
   if (logfd > 0 && log_append(logfd,fd,pos) == -1)
      return -1 ;
   // then make the actual change in the file
   if (!Fr_write(fd,buf,len,false))
      {
      Fr_errno = FE_WRITEFAULT ;
      return -1 ;
      }
   else
      return len ;
}

//----------------------------------------------------------------------

int file_close(int fd, int logfd, bool readonly)
{
   if (!readonly && file_sync(fd) == -1)
      return -1 ;
   if (logfd > 0)
      {

      }
   if (close(fd) == -1)
      {
      Fr_errno = FE_FILECLOSE ;
      return -1 ;
      }
   return 0 ;
}

//----------------------------------------------------------------------

static long int read_long(int fd)
{
   LONGbuf bytes ;

   if ((int)file_read(bytes,sizeof bytes,fd) < (int)sizeof bytes)
      return -1 ;
   return FrLoadLong(bytes) ;
}

//----------------------------------------------------------------------

int write_long(int fd, long int value, int logfd)
{
   LONGbuf bytes ;

   FrStoreLong(value,bytes) ;
   return (file_write(bytes,sizeof bytes,fd,logfd) == sizeof bytes) ? 0 : -1 ;
}

//----------------------------------------------------------------------

int append_long(int fd, long int value, int logfd)
{
   LONGbuf bytes ;

   FrStoreLong(value,bytes) ;
   return (file_append(bytes,sizeof bytes,fd,logfd) == sizeof bytes) ? 0 : -1 ;
}

/**********************************************************************/
/*	Database Transaction functions				      */
/**********************************************************************/

static int log_update(int fd, int type, int datalen, void *data)
{
   int reclen = datalen + 7 ;
   unsigned char *buf = (unsigned char *)tempalloc(reclen) ;

   if (!buf)
      return -1 ;
   FrStoreShort(TRANSACTION_SIGNATURE, buf) ;
   FrStoreShort((short)reclen, buf+2) ;
   buf[4] = (char)type ;
   memcpy(buf+5, data, datalen) ;
   FrStoreShort((short)reclen, buf+5+datalen) ;
   bool success = Fr_write(fd,buf,reclen,false) ;
   tempfree(buf) ;
   if (!success)
      {
      Fr_errno = FE_WRITEFAULT ;
      return -1 ;
      }
   return file_sync(fd) ;
}

//----------------------------------------------------------------------

static int log_write(int logfd, int fd, int fileID, int len)
{
   long pos = lseek(fd,0L,SEEK_CUR) ;
   int reclen = len+12 ;
   unsigned char *buf = (unsigned char *)tempalloc(reclen) ;

   if (!buf)
      return -1 ;
   bool success = true ;
   if (read(fd,buf+10,len) < len)
      success = false ;
   lseek(fd,pos,SEEK_SET) ;
   FrStoreShort(TRANSACTION_SIGNATURE, buf) ;
   FrStoreShort((short)reclen, buf+2) ;
   buf[4] = TRANSACTION_WRITE ;
   buf[5] = (char)fileID ;
   FrStoreLong(pos,buf+6) ;
   FrStoreShort((short)reclen, buf+10+len) ;
   if (!Fr_write(logfd,buf,reclen,false))
      success = false ;
   tempfree(buf) ;
   if (!success)
      {
      Fr_errno = FE_WRITEFAULT ;
      return -1 ;
      }
   return file_sync(logfd) ;
}

//----------------------------------------------------------------------

unsigned char*store_short()
{
   static unsigned char buf[12] ;
   FrStoreShort(sizeof(buf), buf+2) ;
   return buf ;
}

//----------------------------------------------------------------------

static int log_append(int logfd, int fileID, long pos)
{
   unsigned char buf[12] ;

   FrStoreShort(TRANSACTION_SIGNATURE, buf) ;
   FrStoreShort(sizeof(buf), buf+2) ;
   buf[4] = TRANSACTION_APPEND ;
   buf[5] = (char)fileID ;
   FrStoreLong(pos,buf+6) ;
   FrStoreShort(sizeof(buf), buf+10) ;
   if (!Fr_write(logfd,buf,sizeof(buf),false))
      {
      Fr_errno = FE_WRITEFAULT ;
      return -1 ;
      }
   return file_sync(logfd) ;
}

//----------------------------------------------------------------------

int log_frame_create(DBFILE *db, const FrSymbol *frname)
{
   if (!frname)
      return 0 ;			// no frame, so nothing to do
   const char *name = frname->symbolName() ;
   if (name && db->logfile > 0 && db->active_trans > 0)
      return log_update(db->logfile,TRANSACTION_CREATEFRAME,
			strlen(name)+1,(void*)name) ;
   else
      return 0 ;	// signal success because there is nothing to do
}

//----------------------------------------------------------------------

int log_frame_delete(DBFILE *db, const FrSymbol *frname)
{
   if (!frname)
      return 0 ;			// no frame, so nothing to do
   const char *name = frname->symbolName() ;

   if (name && db->logfile > 0 && db->active_trans > 0)
      return log_update(db->logfile,TRANSACTION_DELETEFRAME,
			strlen(name)+1,(void*)name) ;
   else
      return 0 ;	// signal success because there is nothing to do
}

//----------------------------------------------------------------------

static int write_logfile_header(int logfd)
{
   if (rewind_file(logfd) == -1)
      return -1 ;
   return (int)file_write(LOG_SIGNATURE,sizeof(LOG_SIGNATURE),logfd,-1)
	  < (int)sizeof(LOG_SIGNATURE)
             ? -1
	     : 0 ;
}

//----------------------------------------------------------------------

static int write_logfile_init(int logfd,int fd,char *name)
{
   int namelen = strlen(name) + 1 ;
   int len = namelen + 1 ;
   FrLocalAlloc(char,buf,512,len) ;
   int result = -1 ;
   if (buf)
      {
      buf[0] = (char)fd ;
      memcpy(buf+1,name,namelen) ;
      result = log_update(logfd,TRANSACTION_HEADER,len,buf) ;
      FrLocalFree(buf) ;
      }
   return result ;
}

//----------------------------------------------------------------------

static int create_logfile(DBFILE *db)
{
   int count = 0 ;
   bool done = false ;

   size_t namelen = strlen(db->database_name) + LOGFILE_EXT_LEN + 1 ;
   db->logfile_name = FrNewN(char,namelen) ;
   if (db->logfile_name == 0)
      return -1 ;
   do {
      Fr_sprintf(db->logfile_name,namelen,"%s%s%0" LOGFILE_EXT_DIGITS "d",
	      db->database_name,LOGFILE_EXT,count++) ;
      if (!FrFileExists(db->logfile_name)) // does file already exist?
	 {
	 if ((db->logfile = open(db->logfile_name,
				 O_RDWR|O_BINARY|O_CREAT|O_TRUNC,
				 S_IREAD|S_IWRITE)) != -1)
	    {
	    done = true ;
	    }
	 }
      } while (!done && count <= LOGFILE_EXT_MAX) ;
   if (db->logfile)
      {
      int len = strlen(db->database_name) ;

      write_logfile_header(db->logfile) ;
      db->database_name[len] = '.' ;   // temp. restore extension for filename
      write_logfile_init(db->logfile,db->db_file,db->database_name) ;
      db->database_name[len] = '\0' ;  // strip extension again
      write_logfile_init(db->logfile,db->indexfile,db->indexfile_name) ;
//!!! any other index files go here
      if (file_sync(db->logfile) == -1)
	 {
	 FrFree(db->logfile_name) ;
	 db->logfile_name = 0 ;
	 return -1 ;
	 }
      return 0 ;
      }
   else
      {
      FrFree(db->logfile_name) ;
      db->logfile_name = 0 ;
      return -1 ;
      }
}

//----------------------------------------------------------------------

int start_database_transaction(DBFILE *db)
{
   unsigned char tran[2] ;

   if (db->logfile <= 0 || db->active_trans >= MAX_ACTIVE_TRANSACTIONS)
      return -1 ;
   db->transactions[db->active_trans] = lseek(db->logfile,0L,SEEK_CUR) ;
   FrStoreShort((short)db->active_trans,tran) ;
   if (log_update(db->logfile,TRANSACTION_BEGIN,2,tran) == -1)
      return -1 ;
   if (log_append(db->logfile,db->db_file,seek_to_end(db->db_file)) == -1
	||
       log_append(db->logfile,db->indexfile,seek_to_end(db->indexfile)) == -1)
      {
      log_update(db->logfile,TRANSACTION_ABORT,2,tran) ;
      return -1 ;
      }
   return db->active_trans++ ;
}

//----------------------------------------------------------------------

int end_database_transaction(DBFILE *db,int transaction)
{
   unsigned char tran[2] ;

   if (transaction < 0 || transaction > db->active_trans || db->logfile <= 0)
      {
      Fr_errno = ME_NOSUCHTRANSACTION ;
      return -1 ;
      }
   // log the succesful end of the transaction first
   FrStoreShort((short)transaction,tran) ;
   if (log_update(db->logfile,TRANSACTION_END,2,tran) == -1)
      return -1 ;
   // only truncate the log file if the record was successfully written
   //   and this is a main transaction rather than a subtransaction
   int result = 0 ;
   if (transaction == 0)
      {
      if (ftruncate(db->logfile,db->transactions[transaction]) < 0)
	 result = -1 ;
      file_sync(db->logfile) ;
      }
   db->active_trans = transaction ;
   return result ;    // 0 == successful
}

//----------------------------------------------------------------------

static int discard_frame_update(int fd, long offset)
{
   char *buf ;
   const char *tmp ;
   LONGbuf bytes ;
   long int size ;
   FrSymbol *frname ;

   // jump to field containing frame rep's length
   if (lseek(fd,offset+4,SEEK_SET) == -1)
      {
      Fr_errno = FE_SEEK ;
      return -1 ;
      }
   // read in start of frame's record
   if ((int)read(fd,bytes,sizeof(bytes)) < (int)sizeof(bytes))
      {
      Fr_errno = FE_READFAULT ;
      return -1 ;
      }
   size = FrLoadLong(bytes) ;
   if (size > FrMAX_SYMBOLNAME_LEN)
      size = FrMAX_SYMBOLNAME_LEN + 1 ;
   size++ ;  // allow for 'deleted' flag
   if ((buf = (char *)tempalloc((int)size)) == 0)
      return -1 ;
   if (read(fd,buf,(int)size) < (int)size)
      {
      Fr_errno = FE_READFAULT ;
      tempfree(buf) ;
      return -1 ;
      }
   tmp = buf+2 ;   // point at frame name
   frname = string_to_Symbol(tmp) ;
   if (!frname)
      {
      Fr_errno = FE_CORRUPTED ;
      tempfree(buf) ;
      return -1 ;
      }
   else
      frname->discardFrame() ;
   tempfree(buf) ;
   return 0 ;
}

//----------------------------------------------------------------------

#if 0
static int undo_frame_update(int fd, long offset)
{
   char *buf ;
   const char *tmp ;
   LONGbuf bytes ;
   long size ;
   FrSymbol *frname ;
   FrFrame *frame ;

   if (lseek(fd,offset,SEEK_SET) == -1)
      {
      Fr_errno = FE_SEEK ;
      return -1 ;
      }
   // read in offset of previous version of frame
   if (read(fd,bytes,sizeof(bytes)) < sizeof(bytes))
      {
      Fr_errno = FE_READFAULT ;
      return -1 ;
      }
   offset = FrLoadLong(bytes) ;
   if (offset)
      {
      if (lseek(fd,offset+4,SEEK_SET) == -1)
	 {
	 Fr_errno = FE_SEEK ;
	 return -1 ;
	 }
      // read in size of previous version of frame
      if (read(fd,bytes,sizeof(bytes)) < sizeof(bytes))
	 {
	 Fr_errno = FE_READFAULT ;
	 return -1 ;
	 }
      // get size of framerep and add a byte for the deleted flag
      size = FrLoadLong(bytes) + 1 ;
      if ((buf = (char *)tempalloc((int)size)) == 0)
	 return -1 ;
      if (read(fd,buf,(int)size) < (int)size)
	 {
	 Fr_errno = FE_READFAULT ;
	 tempfree(buf) ;
	 return -1 ;
	 }
      tmp = buf+2 ;    // point at frame name
      frname = string_to_Symbol(tmp) ;
      if (!frname)
	 {
	 Fr_errno = FE_CORRUPTED ;
	 tempfree(buf) ;
	 return -1 ;
	 }
      frame = find_frame(frname) ;     // get the frame if currently in memory
      if (frame && buf[1] == '[')      // if frame exists and valid framerep
	 {
	 bool old_omit ;
	
	 frame->eraseFrame() ;	       // clear out the frame
	 tmp = buf+1 ;		       // point at framerep
	 old_omit = omit_inverse_links ;
	 omit_inverse_links = true ;
	 string_to_Frame(tmp) ;	       // and read back old version of the frame
	 omit_inverse_links = old_omit ;
	 frame->markDirty(false) ;     // frame has not changed since read in
	 }
      tempfree(buf) ;
      }
   return 0 ;	     // successful
}
#endif

//----------------------------------------------------------------------

static int undo_transaction_record(int fd,int db_file)
{
   unsigned char type ;
   unsigned char *buf ;
   LONGbuf bytes ;
   char fileno ;
   int reclen ;
   long size, offset ;
   VFrameInfo *oldinfo ;
   FrSymbol *frname ;

   if ((int)read(fd,bytes,sizeof(bytes))
           < (int)sizeof(bytes) ||   			 // get sig & size
       read(fd,&type,1) < 1)				 // read record type
      {
      Fr_errno = FE_READFAULT ;
      return -1 ;
      }
   if ((unsigned short)FrLoadShort(bytes) != (unsigned short)TRANSACTION_SIGNATURE)
      {
      Fr_errno = FE_CORRUPTED ;
      return -1 ;
      }
   reclen = FrLoadShort(bytes+2) ;
   switch (type)
      {
      case TRANSACTION_HEADER:
	 // do nothing....  This record should only occur as the very first
	 // record in the transaction log file, but one never knows.
         break ;
      case TRANSACTION_BEGIN:
	 // backed up to start of subtransaction, so pop out a level
	 // (however, we know where the transaction being rolled back starts
	 //  in the log file, so just work our way through subtransactions)
	 break ;
      case TRANSACTION_END:
      case TRANSACTION_ABORT:
	 // we found a subtransaction, so we need to recurse
	 // (however, we know where the transaction being rolled back starts
	 //  in the log file, so just work our way through subtransactions)
	 break ;
      case TRANSACTION_WRITE:
         if (read(fd,&fileno,1) < 1 ||  // read file handle on which to operate
	    read(fd,bytes,4) < 4)      // get offset in file that was changed
	    {
	    Fr_errno = FE_READFAULT ;
	    return -1 ;
	    }
	 offset = FrLoadLong(bytes) ;
	 if (lseek(fileno,offset,SEEK_SET) == -1)
	    {
	    Fr_errno = FE_SEEK ;
	    return -1 ;
	    }
	 reclen -= 12 ;		        // get length of data to be restored
	 if ((buf = (unsigned char *)tempalloc(reclen)) == 0)
	    return -1 ;
	 if (read(fd,buf,reclen) < reclen ||
	     !Fr_write(fileno,buf,reclen))
	    {
	    tempfree(buf) ;
	    Fr_errno = ME_ROLLBACK ;
	    return -1 ;
	    }
	 tempfree(buf) ;
	 break ;
      case TRANSACTION_APPEND:
         if (read(fd,&fileno,1) < 1 ||  // read file handle on which to operate
	     read(fd,bytes,4) < 4)      // read old file size
	    {
	    Fr_errno = FE_READFAULT ;
	    return -1 ;
	    }
	 size = FrLoadLong(bytes) ;
#if 1
	 if (fileno == db_file && discard_frame_update(fileno,size) == -1)
#else
	 if (fileno == db_file && undo_frame_update(fileno,size) == -1)
#endif
	    return -1 ;
	 if (ftruncate(fileno,size) == -1)  // adjust file size
	    {
	    Fr_errno = FE_WRITEFAULT ;
	    return -1 ;
	    }
	 if (file_sync(fileno) == -1)
	    return -1 ;
	 break ;
      case TRANSACTION_CREATEFRAME:
	 reclen -= 7 ;		        // get length of frame name
	 if ((buf = (unsigned char *)tempalloc(reclen)) == 0)
	    return -1 ;
	 if (read(fd,buf,reclen) < reclen)
	    {
	    tempfree(buf) ;
	    Fr_errno = FE_READFAULT ;
	    return -1 ;
	    }
	 frname = FrSymbolTable::add((char *)buf) ;
	 oldinfo = VFrame_Info ;	// temporarily disable backing store
	 VFrame_Info = 0 ;
	 frname->deleteFrame() ;
	 VFrame_Info = oldinfo ;	// re-enable backing store
	 if (VFrame_Info)		// remove local record of frame
	    VFrame_Info->deleteFrame(frname,false) ;
	 tempfree(buf) ;
	 break ;
      case TRANSACTION_DELETEFRAME:
	 reclen -= 7 ;		        // get length of frame name
	 if ((buf = (unsigned char *)tempalloc(reclen)) == 0)
	    return -1 ;
	 if (read(fd,buf,reclen) < reclen)
	    {
	    tempfree(buf) ;
	    Fr_errno = FE_READFAULT ;
	    return -1 ;
	    }
	 frname = FrSymbolTable::add((char *)buf) ;
//!!!
	 if (VFrame_Info)
	    VFrame_Info->createFrame(frname,false) ;
	 tempfree(buf) ;
	 break ;
      default:
	 Fr_errno = FE_CORRUPTED ;
	 return -1 ;
      }
   return 0 ;
}

//----------------------------------------------------------------------

int abort_database_transaction(DBFILE *db,int transaction)
{
   unsigned char tran[2] ;
   unsigned char bytes[2] ;
   long start, pos ;
   int size ;
   register int logfd = db->logfile ;

   if (transaction < 0 || transaction > db->active_trans || db->logfile <= 0)
      {
      Fr_errno = ME_NOSUCHTRANSACTION ;
      return -1 ;
      }
   // log the succesful end of the transaction first
   FrStoreShort((short)transaction,tran) ;
   if (log_update(db->logfile,TRANSACTION_ABORT,2,tran) == -1)
      return -1 ;
   // now back up to the beginning of the indicated transaction, undoing the
   // logged changes as we go
   start = db->transactions[transaction] ;
   pos = lseek(logfd,-2L,SEEK_CUR) ;
   while (pos >= start)
      {
      lseek(logfd,pos,SEEK_SET) ;		// back up to end of prev rec
      if (read(logfd,bytes,2) < 2)		// get record size
	 {
	 Fr_errno = FE_READFAULT ;
	 return -1 ;
	 }
      size = FrLoadShort(bytes) ;		// convert size to integer
      pos -= size ;				// compute prev record offset
      lseek(logfd,-size,SEEK_CUR) ;		// back up to start of record
      if (undo_transaction_record(logfd,db->db_file) == -1)
         return -1 ;
      }
   // truncate the log file if the transaction was successfully undone
   int result = 0 ;
   if (ftruncate(db->logfile,db->transactions[transaction]) < 0)
      result = -1 ;
   file_sync(db->logfile) ;
   db->active_trans = transaction ;
   return result ;  // 0 == successful
}

/**********************************************************************/
/*	Database I/O functions					      */
/**********************************************************************/

static long int check_database_header(int fd)
{
   char db_signature[sizeof DB_SIGNATURE+1] ;
   unsigned char buf[2] ;

   current_header_info = default_header_info ;
   if (rewind_file(fd) == -1 ||
       file_read(db_signature,sizeof DB_SIGNATURE,fd) == -1 ||
       strcmp(db_signature,DB_SIGNATURE) != 0)
      return -1 ;
   if ((int)file_read(buf,sizeof buf,fd) < (int)sizeof buf ||
       FrLoadShort(buf) < MIN_DB_VERSION)
      return -1 ;
   file_read(&current_header_info,sizeof(DBHeaderInfo),fd) ;
   return lseek(fd,0L,SEEK_CUR) ;		// get file position
}

//----------------------------------------------------------------------

static int write_database_header(int fd, int logfd, DBHeaderInfo *info)
{
   if (rewind_file(fd) == -1)
      return -1 ;
   if (!info)
      info = &default_header_info ;
   unsigned char ver[2] ;
   FrStoreShort(DB_FORMAT_VERSION,ver) ;
   if (file_write(DB_SIGNATURE,sizeof(DB_SIGNATURE),fd,logfd)
		  < (int)sizeof(DB_SIGNATURE)
       || file_write(ver,sizeof(ver),fd,logfd) < (int)sizeof(ver)
       || file_write(info,sizeof(DBHeaderInfo),fd,logfd)
		  < (int)sizeof(DBHeaderInfo)
      )
      return -1 ;
   return 0 ;
}

//----------------------------------------------------------------------

int update_database_header(DBFILE *db)
{
   DBHeaderInfo info ;

#ifdef FrEXTRA_INDEXES
   info.has_byslot_index = (char)(db->byslot_fd != -1) ;
   info.has_byfiller_index = (char)(db->byfiller_fd != -1) ;
   info.has_byword_index = (char)(db->byword_fd != -1) ;
#endif /* FrEXTRA_INDEXES */
   info.user_data = db->user_data ;
   if (db->readonly)
      return 0 ;   // successful (silently ignore the write request)
   return write_database_header(db->db_file,db->logfile,&info) ;
}

//----------------------------------------------------------------------

static int index_header_size()
{
   return sizeof(INDEX_SIGNATURE) ;
}

//----------------------------------------------------------------------

static long int check_index_header(int idx)
{
   unsigned char idx_signature[sizeof(INDEX_SIGNATURE)+1] ;

   if (rewind_file(idx) == -1 ||
       file_read(idx_signature,index_header_size(),idx) == -1 ||
       strncmp((char *)idx_signature,INDEX_SIGNATURE,index_header_size()) != 0)
      return -1 ;
   else
      return 0 ;
}

//----------------------------------------------------------------------

static int write_index_header(int idx, int logfd)
{
   if (rewind_file(idx) == -1)
      return -1 ;
   return file_write(INDEX_SIGNATURE,index_header_size(),idx,logfd) ;
}

//----------------------------------------------------------------------

static char *build_index_read_framerep(int data)
{
   int frlen ;
   long int length ;
   char *framerep ;

   if (eof(data))
      return 0 ;
   (void)read_long(data) ;  // skip offset to previous version of frame
   length = read_long(data) ;
   frlen = (int)length ;
   if ((unsigned)length > INT_MAX)	    // did we overflow 'int' ?
      return 0 ;
   if ((framerep = FrNewN(char,frlen)) == 0)
      return 0 ;
   if (file_read(framerep,frlen,data) < frlen)
      {
      FrFree(framerep) ;
      return 0 ;
      }
   return framerep ;
}

//----------------------------------------------------------------------

static int read_index_byname_entry(Fr_FILE *fp, DBFILE *db)
{
   long int len, offset, frameID, idxpos ;
   unsigned char buf[FrMAX_SYMBOLNAME_LEN+1] ;
   FrSymbol *sym ;
   char deleted ;

   idxpos = Fr_tell(fp) ;		// get file position
   frameID = 0 ;
   len = 0 ;
   if ((offset = read_long(fp)) == -1 ||
       (frameID = read_long(fp)) == -1 ||
       (len = read_long(fp)) == -1)
      {
      if (Fr_eof(fp))
	 return 0 ;
      Fr_errno = FE_READFAULT ;
      return -1 ;
      }
   if (offset < 0 || len <= 0 || len > FrMAX_SYMBOLNAME_LEN)
      {
      Fr_errno = FE_CORRUPTED ;
      return -1 ;
      }
   if (Fr_read(buf,(int)len,fp) < (unsigned long)len)
      {
      return -1 ;
      }
   deleted = buf[len-1] ;
   if (offset == 0)
      deleted = (char)true ;
   buf[len-1] = '\0' ;
   sym = FrSymbolTable::add((char *)buf) ;
   HashEntryVFrame *new_ent = new HashEntryVFrame(sym,offset,idxpos,frameID) ;
   new_ent->deleted = deleted ;
#ifdef FrFRAME_ID
   set_frameID(db->frame_IDs,frameID,sym) ;
#endif
   if (db->index->add(sym,new_ent))
      {
      delete new_ent ;			// already in table
      return -1 ;
      }
   else
      {
      return 0 ;   // successful
      }
}

//----------------------------------------------------------------------

static int write_index_byname_entry(int idx, FrSymbol *name, long int offset,
				    long int frameID, bool deleted, int logfd,
				    bool sync, bool append)
{
   int len ;
   if (name)
      len = strlen(name->symbolName()) + 1 ;
   else
      len = 1 ;
   FrLocalAlloc(char,buf,FrMAX_SYMBOLNAME_LEN+12,len+12) ;
   int written = 0 ;
   if (buf)
      {
      FrStoreLong(offset,buf) ;
      FrStoreLong(frameID,buf+4) ;
      FrStoreLong(len,buf+8) ;
      memcpy((char *)buf+12,name?name->symbolName():"",len) ;
      len += 12 ;
      buf[len-1] = (char)deleted ;
      if (append)
	 written = file_append(buf,len,idx,logfd) ;
      else
	 written = file_write(buf,len,idx,logfd) ;
      FrLocalFree(buf) ;
      }
   if (written < len)
      return -1 ;
   return sync ? file_sync(idx) : 0 ;
}

//----------------------------------------------------------------------

static int symbolnamecompare(const FrObject *obj1, const FrObject *obj2)
{
   if (obj1 && obj1->symbolp())
      {
      if (obj2 && obj2->symbolp())
         return strcmp(((FrSymbol *)obj1)->symbolName(),
		       ((FrSymbol *)obj2)->symbolName()) ;
      else
         return -1 ;   // all symbols sort before all non-symbols
      }
   else if (obj2 && obj2->symbolp())
      return 1 ;       // all non-symbols sort after all symbols
   else
      return (obj1 > obj2) ? 1 : ((obj1 < obj2) ? -1 : 0) ;
}

//----------------------------------------------------------------------
// this function uses a rather nasty hack to avoid the need for an additional
// data structure to store the offset of each frame.  Instead, it stuffs the
// file offset into the pointer normally used to access the frame associated
// with the symbol (which of course assumes that pointers are at least as
// big as unsigned longs).

int build_index_byname(int data, int idx, int logfd)
{
   char *framerep ;
   const char *fr ;
   long int offset ;
   FrSymbol *frname ;
   FrList *frames = 0 ;

   if ((offset = check_database_header(data)) == -1)
      return -1 ;
   write_index_header(idx,logfd) ;
   while ((framerep = build_index_read_framerep(data)) != 0)
      {
      for (fr = framerep ; *fr && *fr != '[' ; fr++)
	 ;
      frname = string_to_Symbol(fr) ;
      if (!frname->symbolFrame()) // add to list of frames if not already there
         pushlist(frname,frames) ;
      frname->setFrame((FrFrame *)offset) ;
      offset = lseek(data,0L,SEEK_CUR) ;		// get file position
      FrFree(framerep) ;
      }
   // sort the list of frame names and then write them to the index file
   // along with the frame offsets
   frames = frames->sort(symbolnamecompare) ;
   append_long(idx,frames->listlength(),logfd) ;
   long int id = 0 ;
   while (frames)
      {
      frname = (FrSymbol *)poplist(frames) ;
      write_index_byname_entry(idx,frname,(long)(frname->symbolFrame()),
			       id++,   // frameID
			       false,  // deleted
			       -1,false,true);
      //no need to log additional appends here
      }
   return file_sync(idx) ;
}

//----------------------------------------------------------------------

#ifdef FrEXTRA_INDEXES
int build_extra_index(int index_type, int data, int idxfd, int logfd)
{
   (void)logfd;
   make_new_file(idxfd) ;
   char *framerep ;
   if (check_database_header(data) == -1)
      return -1 ;
   while ((framerep = build_index_read_framerep(data)) != 0)
      {
      const char *fr = framerep ;
      FrObject *obj = string_to_FrObject(fr) ;
      FrFree(framerep) ;
      if (obj && obj->framep())
	 {
	 add_frame((FrFrame*)obj,index_type,idxfd) ;
	 delete (FrFrame*)obj ;
	 }
      else
	 free_object(obj) ;
      }
   return 0 ;
}
#endif /* FrEXTRA_INDEXES */

//----------------------------------------------------------------------

int build_index_file(DBFILE *db, char *index_name, int index_type)
{
   int fd ;
   int result = 0 ;
   FrSymbolTable *symtab, *oldsymtab ;

   if ((fd=open(index_name,O_RDWR|O_BINARY|O_CREAT|O_TRUNC,
		S_IREAD|S_IWRITE)) != -1)
      {
      symtab = new FrSymbolTable(1000) ;
      oldsymtab = symtab->select() ;
      switch (index_type)
         {
	 case INDEX_BYNAME:
		result = build_index_byname(db->db_file,fd,db->logfile) ;
		break ;
#ifdef FrEXTRA_INDEXES
	 case INDEX_INVSLOTS:
	 case INDEX_INVFILLERS:
	 case INDEX_INVWORDS:
	        open_index(fd) ;
		result = build_extra_index(index_type,db->db_file,fd,
					   db->logfile) ;
		break ;
#endif /* FrEXTRA_INDEXES */
	 default:
	    FrProgError("invalid index type in build_index_file");
	    return -1 ;
         }
      destroy_symbol_table(symtab) ;
      oldsymtab->select() ;
      }
   if (result == -1)
      {
      close(fd) ;
      unlink(index_name) ;
      fd = -1 ;
      }
   return fd ;
}

//----------------------------------------------------------------------

int load_byname_index(DBFILE *db)
{
   long int index_size ;
   int idx ;

   idx = db->indexfile ;
   if ((index_size = seek_to_end(idx)) == -1)
      return -1 ;
   long int num_entries = 0 ;
   if (check_index_header(idx) == -1 || (num_entries = read_long(idx)) < 0)
      {
      Fr_errno = FE_CORRUPTED ;
      return -1 ;
      }
   db->index = new FrSymHashTable(num_entries+100) ;
   db->entrycount = 0 ;
   char load_buffer[4096] ;
   Fr_FILE *fp = Fr_fdopen(idx,load_buffer,sizeof(load_buffer)) ;
   while (Fr_tell(fp) < (unsigned long)index_size)
      {
      if (read_index_byname_entry(fp,db) == -1)
         {
	 Fr_close(fp) ;
	 return -1 ;
         }
      else
	 db->entrycount++ ;
      }
   Fr_close(fp) ;
   db->origentrycount = db->entrycount ;
   return 0 ;   // successful
}

//----------------------------------------------------------------------

HashEntryVFrame *find_entry_byname(DBFILE *db, const FrSymbol *frname)
{
   FrObject *info ;
   (void)db->index->lookup(frname,&info) ;
   return (HashEntryVFrame *)info ;
}

//----------------------------------------------------------------------

char *database_index_name(char *database_name, int index_number)
{
   char numstring[FrMAX_ULONG_STRING+1] ;
   int dblen = strlen(database_name) ;
   int namelen = dblen + sizeof(INDEX_EXTENSION) ;
   int numlen = 0 ;
   char *index_name ;

   if (index_number)
      {
      numlen = strlen(numstring) ;
      ultoa((unsigned long)index_number,numstring,10) ;
      namelen += numlen ;
      }
   index_name = FrNewN(char,namelen) ;
   memcpy(index_name,database_name,dblen) ;
   memcpy(index_name+dblen,INDEX_EXTENSION,sizeof(INDEX_EXTENSION)) ;
   if (index_number)
      memcpy(index_name+dblen+sizeof(INDEX_EXTENSION),numstring,numlen) ;
   return index_name ;
}

//----------------------------------------------------------------------

static void load_relations(FrSymbol *root, FrSymbol *inverse, int limit)
{
#ifdef FrSYMBOL_RELATION
   FrSymbol *invrelation = (FrSymbol*)root->getValue(inverse,false) ;
   if (invrelation && invrelation->symbolp())
      root->defineRelation(invrelation) ;
   if (limit <= 0)
      return ;
   limit-- ;
   const FrList *subclasses = root->getValues(symbolSUBCLASSES,false) ;
   while (subclasses)
      {
      FrSymbol *subclass = (FrSymbol*)subclasses->first() ;
      if (subclass && subclass->symbolp() && subclass->isFrame())
	 load_relations(subclass,inverse,limit) ;
      subclasses = subclasses->rest() ;
      }
#endif /* FrSYMBOL_RELATION */
   return ;
}

//----------------------------------------------------------------------

static void load_relations(FrList *relspec)
{
   FrSymbol *relation_root = (FrSymbol*)relspec->first() ;
   FrSymbol *inverse_slot = (FrSymbol*)relspec->second() ;
   FrObject *max = relspec->third() ;
   if (relation_root && relation_root->symbolp() &&
       inverse_slot && inverse_slot->symbolp())
      {
      int limit ;
      if (max && max->numberp())
	 {
	 limit = (int)((FrNumber*)max)->intValue() ;
	 if (limit <= 0)
	    limit = INT_MAX ;
	 }
      else
	 limit = INT_MAX ;
      if (relation_root->isFrame())
	 load_relations(relation_root,inverse_slot,limit) ;
      }
   else
      FrWarning("Improper RELATIONS option in database configuration file.") ;
}

//----------------------------------------------------------------------

static void load_additional_config(istream &in)
{
   FrObject *obj ;
   do {
      in >> obj ;
      if (obj && obj->consp() && SYMBOLP(((FrCons*)obj)->first()))
	 {
	 FrSymbol *opt = (FrSymbol*)(((FrCons*)obj)->first()) ;
	 if (opt == findSymbol("INHERIT"))
	    set_inheritance_type((FrSymbol*)((FrList*)obj)->second()) ;
	 else if (opt == findSymbol("RELATIONS"))
	    load_relations((FrList*)obj->cdr()) ;
	 else
	    FrWarningVA("Unknown configuration option %s",opt->symbolName()) ;
	 }
      } while ((FrSymbol*)obj != symbolNIL && (FrSymbol*)obj != symbolEOF) ;
}

//----------------------------------------------------------------------

void load_db_configuration(DBFILE *db)
{
   const char *dir = FramepaC_get_db_dir() ;
   if (!dir) dir = "." ;
   char type1 = db->user_data.type[0] ? db->user_data.type[0] : '_' ;
   char type2 = db->user_data.type[1] ? db->user_data.type[1] : '_' ;
   char *configname = Fr_aprintf(DBCONFIG_FILENAME,dir,type1,type2) ;
   if (FrFileReadable(configname))
      {
      ifstream in(configname) ;

      if (in && in.good())
	 {
         FrObject *obj ;
	 do {
	    in >> obj ;
#ifdef FrSYMBOL_RELATION
	    if (obj && obj->consp() && ((FrList*)obj)->listlength() == 2)
	       {
               FrList *rel = (FrList*)obj ;
	       FrSymbol *s1 = (FrSymbol*)rel->first() ;
	       FrSymbol *s2 = (FrSymbol*)rel->second() ;
	       if (s1 && s1->symbolp() && s2 && s2->symbolp())
		  s1->defineRelation(s2) ;
	       }
#endif /* FrSYMBOL_RELATION */
	    } while ((FrSymbol*)obj != symbolNIL && (FrSymbol*)obj != symbolEOF) ;
	 // now, load a set of tagged lists for additional configuration
	 // options
	 load_additional_config(in) ;
	 }
      }
   FrFree(configname) ;
   return ;
}

//----------------------------------------------------------------------

#ifdef FrEXTRA_INDEXES
static int open_index(DBFILE *db, int *indexfd, int idxtype)
{
   int fd ;
   char *index_name = database_index_name(db->database_name,idxtype) ;

   if ((fd = open(index_name,O_RDWR|O_BINARY)) == -1 &&
       (fd = build_index_file(db,index_name,idxtype)) == -1)
      {
      FrFree(index_name) ;
      return 0 ;
      }
   FrFree(index_name);
   if (indexfd)
      *indexfd = fd ;
   open_index(fd) ;  // let indexer know about this index
   return fd ;
}
#endif /* FrEXTRA_INDEXES */

//----------------------------------------------------------------------

static DBFILE *abort_db_open(DBFILE *db, int err)
{
   if (db->db_file != -1)
      close(db->db_file) ;
   delete db ;
   Fr_errno = err ;
   return 0 ;
}

//----------------------------------------------------------------------

#if !(defined(unix) && defined(F_TLOCK) && defined(F_ULOCK)) && !(defined(__BORLANDC__) || defined(__WATCOMC__) || defined(_MSC_VER))
static bool never_true() { return false ; }
#endif

//----------------------------------------------------------------------

DBFILE *open_database(const char *database, bool createnew, bool transactions,
		      const char *password
		
#ifdef FrFRAME_ID
		      ,FrameIdentDirectory *frame_IDs
#endif /* FrFRAME_ID */
		      )
{
   char *database_name ;
   char *index_name ;
   DBFILE *db ;
   int len, fd ;
   bool read_only = false ;

   int dblen = strlen(database) ;
   len = dblen + sizeof(DB_EXTENSION) + 1 ;
   database_name = FrNewN(char,len) ;
   if (!database_name)
      {
      Fr_errno = ENOMEM ;
      return 0 ;
      }
   memcpy(database_name,(char *)database,dblen) ;
   database_name[dblen++] = '.' ;
   memcpy(database_name+dblen,DB_EXTENSION,sizeof(DB_EXTENSION)) ;
   fd = 0 ;  // keep GNU C++ 2.6.3 happy....
   if (!FrFileReadWrite(database_name))
      {
      if (FrFileReadable(database_name))  // read-only database?
	 {
	 read_only = true ;
	 transactions = false ;  // no need for logging if no updates
	 }
      else if (createnew && (fd = open(database_name,O_RDWR|O_BINARY|O_CREAT,
					        S_IREAD|S_IWRITE)) != -1)
	 {
	 write_database_header(fd,0,&default_header_info) ;
	 close(fd) ;
	 }
      else
         {
         FrFree(database_name) ;
	 Fr_errno = ME_CANTCREATE ;
         return 0 ;
         }
      }
   db = new DBFILE ;
   db->readonly = read_only ;
   db->database_name = database_name ;
   int openmode = db->readonly ? O_RDONLY : O_RDWR ;
   if ((db->db_file = open(database_name,openmode|O_BINARY)) == -1)
      return abort_db_open(db,ME_NOTFOUND) ;
   if (check_database_header(db->db_file) == -1)
      return abort_db_open(db,FE_CORRUPTED) ;
   db->user_data = current_header_info.user_data ;
   // check database password, then user level (if incorrect password given)
   if (!password || strcmp(db->user_data.password,password) != 0)
      {
      if (get_access_level() < getDBUserData_writelevel(&db->user_data))
	 {
	 if (get_access_level() >= getDBUserData_readlevel(&db->user_data))
	    {
	    if (!db->readonly)
	       {
	       close(db->db_file) ;
	       db->db_file = open(database_name,O_RDONLY|O_BINARY) ;
	       db->readonly = true ;
	       }
	    }
	 else
	    return abort_db_open(db,ME_PRIVILEGED) ;
         }
      }
   database_name[strlen(database)] = '\0' ;  // strip off extension
   index_name = database_index_name((char *)database,0) ;
   db->indexfile_name = index_name ;
   if ((db->indexfile = open(index_name,openmode|O_BINARY)) == -1 &&
       (db->indexfile = build_index_file(db,index_name,INDEX_BYNAME)) == -1)
      {
      close_database(db) ;
      delete db ;
      Fr_errno = ME_BADINDEX ;
      return 0 ;
      }
   if (!db->readonly)
      {
      lseek(db->db_file,0L,SEEK_SET) ;
      // try to lock the first byte of the file; if unable, someone else is
      // already using this database in read-write mode
#if defined(unix) && defined(F_TLOCK) && defined(F_ULOCK)
      if (lockf(db->db_file,F_TLOCK,1) == -1 && errno == EACCES)
	 {
	 lockf(db->db_file,F_ULOCK,1) ;
#elif defined(__BORLANDC__) || defined(__WATCOMC__) || defined(_MSC_VER)
      if (locking(db->db_file,LK_NBLCK,1) == -1 && errno == EACCES)
	 {
	 locking(db->db_file,LK_UNLCK,1) ;
#else
      if (never_true())
	 {
#endif /* unix */
	 Fr_errno = ME_LOCKED ;
	 db->readonly = true ;
	 }
      }
#ifdef FrEXTRA_INDEXES
   if (!db->readonly)
      {
      init_ii() ;
      if ((current_header_info.has_byslot_index &&
	   open_index(db,&db->byslot_fd,INDEX_INVSLOTS) == 0)
	  ||
	  (current_header_info.has_byfiller_index &&
	   open_index(db,&db->byfiller_fd,INDEX_INVFILLERS) == 0)
	  ||
	  (current_header_info.has_byword_index &&
	   open_index(db,&db->byword_fd,INDEX_INVWORDS) == 0))
	 {
	 close_database(db) ;
	 Fr_errno = ME_BADINDEX ;
	 return 0 ;
	 }
      if (db->byslot_fd != -1)
	 db->extra_indexes++ ;
      if (db->byfiller_fd != -1)
	 db->extra_indexes++ ;
      if (db->byword_fd != -1)
	 db->extra_indexes++ ;
      }
#endif /* FrEXTRA_INDEXES */
#ifdef FrFRAME_ID
   db->frame_IDs = frame_IDs ;
#endif /* FrFRAME_ID */
   if (transactions && create_logfile(db) == -1)
      {
      delete db ;
      Fr_errno = ME_NOTRANSACTIONS ;
      return 0 ;
      }
   if (load_byname_index(db) == -1)
      {
      close_database(db) ;
      db = 0 ;
      Fr_errno = ME_NOINDEX ;
      return 0 ;
      }
   return db ;
}

//----------------------------------------------------------------------

int close_database(DBFILE *db)
{
   if (update_database_header(db) == -1)
      return -1 ;
   // release our lock on the first byte of the file
   lseek(db->db_file,0L,SEEK_SET) ;
   if (!db->readonly)
#if defined(unix) && !defined(__CYGWIN__)
      lockf(db->db_file,F_ULOCK, 1) ;
#elif defined(__BORLANDC__) || defined(__WATCOMC__) || defined(_MSC_VER)
      locking(db->db_file,LK_UNLCK,1) ;
#endif
   if (db->indexfile != -1 &&
       (lseek(db->indexfile,index_header_size(),SEEK_SET) == -1 ||
	(!db->readonly && db->entrycount != db->origentrycount &&
	 write_long(db->indexfile,db->entrycount,db->logfile) == -1) ||
	file_close(db->indexfile,db->logfile,db->readonly) == -1))
      return -1 ;
   if (db->db_file != -1 &&
       file_close(db->db_file,db->logfile,db->readonly) == -1)
      return -1 ;
   if (db->logfile > 0)
      {
      if (db->active_trans > 0)
	 end_database_transaction(db,0) ;
      if (file_close(db->logfile,-1) == -1)
         return -1 ;
      unlink(db->logfile_name) ;
      }
#ifdef FrEXTRA_INDEXES
   if (db->byslot_fd != -1)
      {
      close_index(db->byslot_fd) ;
      close(db->byslot_fd) ;
      db->byslot_fd = -1 ;
      }
   if (db->byfiller_fd != -1)
      {
      close_index(db->byfiller_fd) ;
      close(db->byfiller_fd) ;
      db->byfiller_fd = -1 ;
      }
   if (db->byword_fd != -1)
      {
      close_index(db->byword_fd) ;
      close(db->byword_fd) ;
      db->byword_fd = -1 ;
      }
#endif /* FrEXTRA_INDEXES */
   // if we get here, the close was successful, so delete the file record
   delete db ;
   return 0 ;
}

//----------------------------------------------------------------------

char *read_database(DBFILE *handle, HashEntryVFrame *entry, bool *deleted)
{
   long int offset = 0 ;

   if (entry && (offset = entry->frameOffset()) > 0)
      {
      DB_FRAME_HEADER hdr ;
      long int size ;

      lseek(handle->db_file,offset,SEEK_SET) ;
      if (file_read(&hdr,DB_FRAME_HEADER_SIZE,handle->db_file)
	            < DB_FRAME_HEADER_SIZE)
         {
	 Fr_errno = FE_READFAULT ;
	 return 0 ;
	 }
      offset = FrLoadLong(hdr.prev_offset) ;
      size = FrLoadLong(hdr.rec_size) ;
      entry->deleted = hdr.deleted ;
      if (deleted)
	 *deleted = (bool)hdr.deleted ;
      if (size > 0)
         {
	 char *buf = FrNewN(char,(int)size) ;

	 if (file_read(buf,(int)size,handle->db_file) == -1)
            {
	    FrFree(buf) ;
	    Fr_errno = FE_READFAULT ;
	    return 0 ;
	    }
	 else
	    return buf ;
         }
      else if (hdr.deleted)
	 {
         char *buf = FrNewN(char,1) ;

         buf[0] = '\0' ;
	 return buf ;
	 }
      }
   else if (offset == 0)  // is this a new frame not yet in the index?
      {
      char *buf = FrNewN(char,1) ;

      buf[0] = '\0' ;
      return buf ;
      }
   Fr_errno = FE_CORRUPTED ;
   return 0 ;
}

//----------------------------------------------------------------------

char *read_old_entry(DBFILE *db, HashEntryVFrame *entry, int generation)
{
   long int offset = 0 ;

   if (entry && (offset = entry->frameOffset()) > 0)
      {
      if (generation < 0)
	 {
	 Fr_errno = FE_INVALIDPARM ;
	 return 0 ;
	 }
      DB_FRAME_HEADER hdr ;
      long int size = 0 ;

      while (generation >= 0 && offset > 0)
	 {
         lseek(db->db_file,offset,SEEK_SET) ;
	 if (file_read(&hdr,DB_FRAME_HEADER_SIZE,db->db_file)
		      < DB_FRAME_HEADER_SIZE)
            {
	    Fr_errno = FE_READFAULT ;
	    return 0 ;
            }
	 offset = FrLoadLong(hdr.prev_offset) ;
	 size = FrLoadLong(hdr.rec_size) ;
	 generation-- ;
	 }
      // check if enough stored versions of frame and non-null entry
      if (generation < 0 && size > 0)
	 {
	 char *buf = FrNewN(char,(int)size) ;

	 if (file_read(buf,(int)size,db->db_file) == -1)
            {
            FrFree(buf) ;
	    Fr_errno = FE_READFAULT ;
	    return 0 ;
	    }
	 else
	    return buf ;
	 }
      else
	 {
	 Fr_errno = 0 ;
	 return 0 ;
	 }
      }
   else if (offset == 0)  // is this a new frame not yet in the index?
      {
      char *buf = FrNewN(char,1) ;

      buf[0] = '\0' ;
      return buf ;
      }
   Fr_errno = FE_CORRUPTED ;
   return 0 ;
}

//----------------------------------------------------------------------

static int write_new_record(DBFILE *db,HashEntryVFrame *entry, FrFrame *frame,
			    bool deleted, bool synch)
{
   long int old_offset, new_offset ;
   char *record, *buf ;
   int size ;

   if (deleted)
      size = frame ? FrObject_string_length(frame->frameName())+3 : 1 ;
   else
      size = frame ? FrObject_string_length(frame)+1 : 1 ;
   if ((record = (char *)tempalloc(size+DB_FRAME_HEADER_SIZE)) == 0)
      FrNoMemory("in write_new_record()") ;
   buf = record+DB_FRAME_HEADER_SIZE ;
   if (entry && entry->frameOffset() >= 0)
      old_offset = entry->frameOffset() ;
   else
      old_offset = 0 ;	// if no previous version of frame, indicate w/ zero
   FrStoreLong(old_offset,(unsigned char *)record) ;
   FrStoreLong(size,(unsigned char *)record+4) ;
   record[8] = (char)deleted ;
   if (deleted && frame)
      {
      buf[0] = '[' ;
      frame->frameName()->print((char *)buf+1) ;
      strcat((char *)buf,"]") ;
      }
   else if (frame)
      frame->print((char *)buf) ;
   else
      buf[0] = '\0' ;
   if (deleted)
      {
      new_offset = 0 ;
      }
   else
      {
      if ((new_offset = seek_to_end(db->db_file)) == -1)
	 return -1 ;
      }
   size = file_append(record,size+DB_FRAME_HEADER_SIZE,db->db_file,
		      db->active_trans?-1:db->logfile) ;
   tempfree(record) ;
   if (size == -1)
      return -1 ;
   else
      {
      if (entry)
	 entry->setOffset(new_offset) ;
      return synch ? file_sync(db->db_file) : 0 ;
      }
}

//----------------------------------------------------------------------

static int update_index_byname_entry(DBFILE *db, HashEntryVFrame *entry,
				     bool sync)
{
   if (entry->indexPosition() == 0)
      {
      long int indexpos ;

      if ((indexpos = seek_to_end(db->indexfile)) == -1) // get file position
	 return -1 ;
      entry->setPosition(indexpos) ;
      if (write_index_byname_entry(db->indexfile,
				   entry->frameName(),entry->frameOffset(),
#ifdef FrFRAME_ID
				   entry->frameID,
#else
				   -1,
#endif /* FrFRAME_ID */
				   entry->deleted,
				   db->active_trans?-1:db->logfile,
				   sync,true) == -1)
	 return -1 ;
      db->entrycount++ ;
      }
   else if (entry->deleted)
      {
      lseek(db->indexfile,entry->indexPosition(),SEEK_SET) ;
      if (write_index_byname_entry(db->indexfile,
				   entry->frameName(),entry->frameOffset(),
#ifdef FrFRAME_ID
				   entry->frameID,
#else
				   -1,
#endif /* FrFRAME_ID */
				   entry->deleted,
				   db->active_trans?-1:db->logfile,
				   sync,false) == -1)
	 return -1 ;
      }
   else
      {
      if (db->logfile > 0)
	 {
         unsigned char buf[9] ;

         buf[0] = (char)db->indexfile ;
         FrStoreLong(entry->indexPosition(),buf+1) ;
         FrStoreLong(entry->oldFrameOffset(),buf+5) ;
         if (log_update(db->logfile,TRANSACTION_WRITE,9,buf) == -1)
            return -1 ;
	 }
      lseek(db->indexfile,entry->indexPosition(),SEEK_SET) ;
      if (write_long(db->indexfile,entry->frameOffset(),-1) == -1)
	 return -1 ;
      if (entry->undeleted)
	 {
	 if (db->logfile > 0)
	    {
//!!!
	    }
	 lseek(db->indexfile,
	       entry->indexPosition() +
	         strlen(entry->frameName()->symbolName())+12,
	       SEEK_SET) ;
	 char flag[1] ;
	 flag[0] = (char)0 ;
	 if (!Fr_write(db->indexfile,flag,sizeof(flag)))
	    return -1 ;
	 entry->undeleted = false ;
	 }
      }
   return sync ? file_sync(db->indexfile) : 0 ;
}

//----------------------------------------------------------------------

#ifdef FrEXTRA_INDEXES
int update_extra_index(int idxtype, int idxfd, FrFrame *oldfr, FrFrame *newfr)
{
   if (oldfr && newfr)
      return update_inv(idxtype,idxfd,oldfr,newfr) ;
   else if (!oldfr)
      return add_frame(newfr,idxtype,idxfd) ;
   else
      return delete_frame(oldfr,idxtype,idxfd) ;
}
#endif /* FrEXTRA_INDEXES */

//----------------------------------------------------------------------

bool update_database_record(FrHashEntry *ent, DBFILE *db, bool force, bool synch)
{
   HashEntryVFrame *entry = (HashEntryVFrame *)ent ;
   FrFrame *frame = entry->frameName()->symbolFrame() ;

   if ((frame && frame->isVFrame() && (force || frame->dirtyFrame()))
      ||
       (entry->deleted && entry->frameOffset() != 0)
      )
      {
#ifdef FrEXTRA_INDEXES
      if (db->extra_indexes)
	 {
	 FrFrame *oldframe = 0;
	 if (entry->indexPosition())  // does frame already exist?
	    {
	    if (frame)
  	       frame->frameName()->setFrame(0) ; // disconn. from symbol table
	    oldframe = VFrame_Info->retrieveFrame(entry->frameName()) ;
	    }
	 if (db->byslot_fd != -1)
	    update_extra_index(SLOT_NAMES,db->byslot_fd,oldframe,frame) ;
	 if (db->byfiller_fd != -1)
	    update_extra_index(FILLER_VALUES,db->byfiller_fd,oldframe,frame) ;
	 if (db->byword_fd != -1)
	    update_extra_index(STRING_WORDS,db->byword_fd,oldframe,frame) ;
	 if (oldframe)
	    oldframe->discard() ;
	 if (frame)
	    frame->frameName()->setFrame(frame) ; // reconnect to symtable
	 }
#endif /* FrEXTRA_INDEXES */
      if (write_new_record(db,entry,frame,(bool)entry->deleted,synch)
		      == -1)
	 return false ;
      if (update_index_byname_entry(db,entry,synch) == -1)
	 return false ;
      if (frame)
	 frame->markDirty(false) ;
      }
   return true ;
}

//----------------------------------------------------------------------

bool write_database_entry(DBFILE *db,const FrSymbol *frame)
{
   HashEntryVFrame *entry = find_entry_byname(db,frame) ;

   if (entry)
      return update_database_record(entry,db,true,true) ;
   else
      return 0 ;
}

//----------------------------------------------------------------------

void set_database_header(DBUserData *user_data)
{
   default_header_info.user_data = *user_data ;
}

//----------------------------------------------------------------------

void FramepaC_select_extra_indexes(bool byslot, bool byfiller, bool byword)
{
   default_header_info.has_byslot_index = (char)byslot ;
   default_header_info.has_byfiller_index = (char)byfiller ;
   default_header_info.has_byword_index = (char)byword ;
}

//----------------------------------------------------------------------

void FramepaC_determine_extra_indexes(bool *byslot, bool *byfiller,
				      bool *byword)
{
   if (byslot)
      *byslot = (bool)default_header_info.has_byslot_index ;
   if (byfiller)
      *byfiller = (bool)default_header_info.has_byfiller_index ;
   if (byword)
      *byword = (bool)default_header_info.has_byword_index ;
}

//----------------------------------------------------------------------

#endif /* FrDATABASE */

// end of file mikro-db.cpp //
