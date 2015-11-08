/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01  							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frpasswd.cpp	password and user info file manipulation	*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,1998,1999,2003,2006,2009,2011	*/
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

#include <errno.h>
#include <fcntl.h>
#include "frpasswd.h"
#include "framerr.h"
#include "frlist.h"
#include "frnumber.h"
#include "frprintf.h"
#include "frstring.h"
#include "frsymtab.h"
#include "frutil.h"

#ifdef FrSTRICT_CPLUSPLUS
#  include <fstream>
#else
#  include <fstream.h>
#endif

#if defined(__MSDOS__) || defined(__WATCOMC__) || defined(_MSC_VER)
#  include <io.h>
#  include <dos.h>
#  define ftruncate chsize
#elif defined(__SUNOS__) || defined(__SOLARIS__) || defined(__linux__)
#  include <unistd.h>
#endif /* __MSDOS__ */

#if defined(unix) || defined(__linux__) || defined(__GNUC__)
#include <crypt.h>
#include <time.h>
#include <unistd.h>
#endif /* unix */

#if !defined(O_BINARY)
#  define O_BINARY 0
#endif

/************************************************************************/
/************************************************************************/

static const char UIFILE_NAME[] = "userinfo.dat" ;

static const char USERINFO_TYPE[] = "USERINFO" ;

static const char PASSWORD_FIELD[] = "PASSWD" ;
static const char LEVEL_FIELD[] = "LEVEL" ;
static const char NAME_FIELD[] = "NAME" ;

/************************************************************************/
/*    Global variables for this module					*/
/************************************************************************/

static char *uinfo_directory = 0 ;
static char *uifile_name = 0 ;

static FrList *user_info_list = 0 ;

static FrStruct *current_userinfo = 0 ;
static int user_access_level = 0 ;   // 0-15: 0=guest, 12=admin, 15=root

/************************************************************************/
/************************************************************************/

const char *FramepaC_uifile_name()
{
   return uifile_name ? uifile_name : UIFILE_NAME ;
}

//----------------------------------------------------------------------

const FrList *FramepaC_get_userlist()
{
   return user_info_list ;
}

//----------------------------------------------------------------------

static char *scramble_password(const char *password,const char *pwdsalt)
{
   if (!password)
      return 0 ;
   size_t passlen = strlen(password) ;
   char *result = FrNewN(char,passlen+14) ; // big enough for crypt()
   if (result)
      {
#if defined(unix) || defined(__linux__) || defined(__GNUC__)
      char salt[3] ;
      if (pwdsalt)
	 {
	 salt[0] = pwdsalt[0] ;
         salt[1] = pwdsalt[1] ;
	 }
      else
	 {
	 time_t tm = time(0) ;
	 salt[0] = 'a' + (tm & 15) ;
	 salt[1] = 'a' + ((tm >> 4) & 15) ;
	 }
      salt[2] = '\0' ;
      memcpy(result,password,passlen+1) ;
      strcpy(result,crypt(result,salt)) ;
#else
      (void)pwdsalt ;
      char *tmp = result ;
      while (*password)
	 {
	 *tmp++ = (char)((256+13)-(*password)) ;
	 password++ ;
	 }
      *tmp = '\0' ;
#endif
      }
   else
      FrNoMemory("while scrambling password") ;
   return result ;
}

//----------------------------------------------------------------------

bool verify_user_password(const char *password, FrStruct *userinfo)
{
   if (!userinfo)
      {
      userinfo = current_userinfo ;
      if (!userinfo)
	 return false ;
      }
   if (!password)
      password = "" ;
   FrSymbolTable *symtab = FrSymbolTable::selectDefault() ;
   FrString *stored ;
   stored = (FrString*)userinfo->get(FrSymbolTable::add(PASSWORD_FIELD)) ;
   char *storedpwd = stored ? (char*)stored->stringValue() : 0 ;
   symtab->select() ;
   if (!stored)
      return true ;
   char *scrambled = scramble_password(password,storedpwd) ;
   bool result = strcmp(scrambled,storedpwd) == 0 ;
   FrFree(scrambled) ;
   return result ;
}

//----------------------------------------------------------------------

bool set_user_password(const char *oldpwd, const char *newpwd,
		       FrStruct *userinfo)
{
   if (!userinfo)
      {
      userinfo = current_userinfo ;
      if (!userinfo)
	 return false ;
      }
   if (!oldpwd)
      oldpwd = "" ;
   if (!newpwd)
      newpwd = "" ;
   if (!verify_user_password(oldpwd, userinfo))
      return false ;
   FrSymbolTable *symtab = FrSymbolTable::selectDefault() ;
   char *scrambled = scramble_password(newpwd,0) ;
   FrSymbol *symPASSWD = FrSymbolTable::add(PASSWORD_FIELD) ;
   userinfo->put(symPASSWD,new FrString(scrambled)) ;
   FrFree(scrambled) ;
   symtab->select() ;
   return true ;
}

//----------------------------------------------------------------------

static bool load_userinfo(istream &in)
{
   free_object(user_info_list) ;
   user_info_list = 0 ;
   FrSymbolTable *symtab = FrSymbolTable::selectDefault() ;
   FrSymbol *uinfo_type = FrSymbolTable::add(USERINFO_TYPE) ;
   while (!in.eof() && !in.fail())
      {
      FrObject *obj ;
      in >> obj ;
      if (obj == FrSymbolTable::add("*EOF*"))
	 break ;
      if (!in.good())
	 {
	 symtab->select() ;
	 return false ;
	 }
      else if (obj && obj->structp() &&
	       ((FrStruct*)obj)->typeName() == uinfo_type)
	 {
	 pushlist(obj,user_info_list) ;
	 }
      }
   symtab->select() ;
   return true ;
}

//----------------------------------------------------------------------

bool load_userinfo(const char *filename)
{
   ifstream in(filename) ;
   if (in.good())
      {
      bool result = load_userinfo(in) ;
      in.close() ;
      return result ;
      }
   else
      return false ;
}

//----------------------------------------------------------------------

static bool store_userinfo(ostream &out)
{
   out << ";; FramepaC User Information" << endl ;
   for (FrList *l = user_info_list ; l ; l = l->rest())
      {
      FrStruct *userinfo = (FrStruct*)l->first() ;
      out << userinfo << endl ;
      if (!out.good())
	 return false ;
      }
   return out.good() != false ;
}

//----------------------------------------------------------------------

bool store_userinfo(const char *filename)
{
   if (!filename)
      filename = uifile_name ;
   ofstream out(filename,ios::out|ios::trunc) ;
   if (out.good())
      {
      bool result = store_userinfo(out) ;
      out.close() ;
      return result ;
      }
   else
      {
      out.close() ;
      return false ;
      }
}

//----------------------------------------------------------------------

FrStruct *retrieve_userinfo(const char *name)
{
   Fr_errno = 0 ;
   FrSymbolTable *symtab = FrSymbolTable::selectDefault() ;
   FrSymbol *symNAME = FrSymbolTable::add(NAME_FIELD) ;
   if (!user_info_list)
      load_userinfo(uifile_name) ;
   for (FrList *l = user_info_list ; l ; l = l->rest())
      {
      FrStruct *userinfo = (FrStruct*)l->first() ;
      FrString *username = (FrString*)userinfo->get(symNAME) ;
      if (strcmp((char*)username->stringValue(),name) == 0)
	 return userinfo ;
      }
   symtab->select() ;
   return 0 ; // not found
}

//----------------------------------------------------------------------

FrStruct *make_userinfo(const char *username, const char *password,
			int level)
{
   FrStruct *userinfo = new FrStruct(FrSymbolTable::add(USERINFO_TYPE)) ;
   FrObject *o = new FrString(username) ;
   userinfo->put(FrSymbolTable::add(NAME_FIELD),o) ;
   free_object(o) ;
   if (password)
      {
      char *scrambled = scramble_password(password,0) ;
      o = new FrString(scrambled) ;
      userinfo->put(FrSymbolTable::add(PASSWORD_FIELD),o) ;
      FrFree(scrambled) ;
      free_object(o) ;
      }
   if (level < GUEST_ACCESS_LEVEL)
      level = GUEST_ACCESS_LEVEL ;
   else if (level > ROOT_ACCESS_LEVEL)
      level = ROOT_ACCESS_LEVEL ;
   o = new FrInteger(level) ;
   userinfo->put(FrSymbolTable::add(LEVEL_FIELD),o) ;
   free_object(o) ;
   return userinfo ;
}

//----------------------------------------------------------------------

bool update_user(FrStruct *userinfo)
{
   // reload the user information from the file in case someone else has
   // updated it since we last loaded it
   if (!load_userinfo(FramepaC_uifile_name()))
      return false ;
   bool result ;
   int fd = open(FramepaC_uifile_name(),O_RDWR|O_BINARY) ;
   if (fd == -1)
      return false ;
   else
      {
#if defined(unix) && !defined(__CYGWIN__) && defined(F_TLOCK) && defined(F_ULOCK)
      // try to lock the first byte of the file; if unable, someone else is
      // already using this database
      lseek(fd,0L,SEEK_SET) ;
      if (lockf(fd,F_TLOCK,1) == -1 && errno == EACCES)
	 {
	 lockf(fd,F_ULOCK,1) ;
	 Fr_errno = ME_LOCKED ;
	 close(fd) ;
	 return false ;
	 }
#endif /* unix && F_TLOCK && F_ULOCK */
      FrSymbol *symNAME = FrSymbolTable::add(NAME_FIELD) ;
      FrString *uname1 = (FrString*)userinfo->get(symNAME) ;
      const char *username1 = uname1 ? (char*)uname1->stringValue() : "" ;
      FrList *uil = user_info_list ;
      while (uil)
	 {
	 FrString *uname2 = (FrString*)
			     ((FrStruct*)uil->first())->get(symNAME) ;
	 if (uname2 && strcmp(username1,(char*)uname2->stringValue()) == 0)
	    break ;
	 else
	    uil = uil->rest() ;
	 }
      if (uil)
	 {
	 free_object(uil->first()) ;
	 uil->replaca(userinfo) ;
	 }
      else
	 pushlist(userinfo,user_info_list) ;
      result = store_userinfo(FramepaC_uifile_name()) ;
#if defined(unix) && defined(F_ULOCK)
      // release our lock on the first byte of the file
      lseek(fd,0L,SEEK_SET) ;
      lockf(fd,F_ULOCK, 1) ;
#endif /* unix */
      close(fd) ;
      }
   return result ;
}

//----------------------------------------------------------------------

bool remove_user(const char *name)
{
   // reload the user information from the file in case someone else has
   // updated it since we last loaded it
   if (!load_userinfo(FramepaC_uifile_name()))
      return false ;
   bool result ;
   int fd = open(FramepaC_uifile_name(),O_RDWR|O_BINARY) ;
   if (fd == -1)
      return false ;
   else
      {
#if defined(unix) && defined(F_TLOCK) && defined(F_ULOCK)
      // try to lock the first byte of the file; if unable, someone else is
      // already using this database
      lseek(fd,0L,SEEK_SET) ;
      if (lockf(fd,F_TLOCK,1) == -1 && errno == EACCES)
	 {
	 lockf(fd,F_ULOCK,1) ;
	 close(fd) ;
	 Fr_errno = ME_LOCKED ;
	 return false ;
	 }
#endif /* unix && F_TLOCK && F_ULOCK */
      FrStruct *userinfo = retrieve_userinfo(name) ;
      if (!userinfo)
	 {
	 Fr_errno = ME_NOTFOUND ;
	 result = false ;
	 }
      else
	 {
	 FrSymbol *symNAME = FrSymbolTable::add(NAME_FIELD) ;
	 FrString *uname1 = (FrString*)userinfo->get(symNAME) ;
	 const char *username1 = uname1
				 ? (char*)uname1->stringValue()
				 : "" ;
	 FrList *uil = user_info_list ;
	 while (uil)
	    {
	    FrString *uname2 = (FrString*)
				((FrStruct*)uil->first())->get(symNAME) ;
	    if (uname2 &&
		strcmp(username1,(char*)uname2->stringValue()) == 0)
	       break ;
	    else
	       uil = uil->rest() ;
	    }
	 if (uil)
	    {
	    user_info_list = listremove(user_info_list,uil->first()) ;
	    result = store_userinfo(FramepaC_uifile_name()) ;
	    }
	 else
	    {
	    Fr_errno = ME_NOTFOUND ;
	    result = false ;
	    }
	 }
      }
#if defined(unix) && defined(F_ULOCK)
   // release our lock on the first byte of the file
   lseek(fd,0L,SEEK_SET) ;
   lockf(fd,F_ULOCK, 1) ;
#endif /* unix */
   close(fd) ;
   return result ;
}

//----------------------------------------------------------------------

bool login_user(const char *name, const char *password)
{
   FrSymbolTable *symtab = FrSymbolTable::selectDefault() ;
   FrStruct *userinfo = retrieve_userinfo(name) ;
   bool ok ;
   if (userinfo)
      {
      if (verify_user_password(password,userinfo))
	 {
	 current_userinfo = userinfo ;
	 FrNumber *level ;
	 level = (FrNumber*)userinfo->get(FrSymbolTable::add(LEVEL_FIELD));
	 if (level && level->numberp())
	    user_access_level = (int)(*level) ;
	 else
	    user_access_level = GUEST_ACCESS_LEVEL ;
	 ok = true ;
	 Fr_errno = 0 ;
	 }
      else // bad password
	 {
	 ok = false ;
	 Fr_errno = ME_PASSWORD ;
	 }
      }
   else // unknown username
      {
      ok = false ;
      Fr_errno = ME_NOTFOUND ;
      }
   symtab->select() ;
   return ok ;
}

//----------------------------------------------------------------------

int get_access_level()
{
   return user_access_level ;
}

//----------------------------------------------------------------------

/************************************************************************/
/*    initialization functions						*/
/************************************************************************/

static void _FramepaC_clear_userinfo_dir()
{
   FrFree(uinfo_directory) ;	uinfo_directory = 0 ;
   FrFree(uifile_name) ;	uifile_name = 0 ;
   return ;
}

//----------------------------------------------------------------------

void FramepaC_set_userinfo_dir(const char *dir)
{
   if (!dir || !*dir)
      dir = "." ;
   FrFree(uinfo_directory) ;
   uinfo_directory = FrDupString(dir) ;
   FrFree(uifile_name) ;
   uifile_name = Fr_aprintf("%s/%s",dir,UIFILE_NAME) ;
   FramepaC_clear_userinfo_dir = _FramepaC_clear_userinfo_dir ;
   return ;
}

//----------------------------------------------------------------------

const char *FramepaC_get_userinfo_dir()
{
   return uinfo_directory ;
}

// end of file frpasswd.cpp //
