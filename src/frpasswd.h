/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frpasswd.h							*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,2009,2013				*/
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

#ifndef __FRPASSWD_H_INCLUDED
#define __FRPASSWD_H_INCLUDED

#ifndef __FRSTRUCT_H_INCLUDED
#include "frstruct.h"
#endif

/************************************************************************/
/*	Manifest constants						*/
/************************************************************************/

#define GUEST_ACCESS_LEVEL  0
#define ADMIN_ACCESS_LEVEL 12
#define ROOT_ACCESS_LEVEL  15

/************************************************************************/
/************************************************************************/

FrStruct *retrieve_userinfo(const char *name) ;
FrStruct *make_userinfo(const char *name, const char *password = 0,
			int level = GUEST_ACCESS_LEVEL) ;
bool update_user(FrStruct *userinfo) ;
bool remove_user(const char *name) ;
bool login_user(const char *name, const char *password) ;
int get_access_level() ;
bool verify_user_password(const char *password, FrStruct *userinfo = 0) ;
bool set_user_password(const char *oldpwd, const char *newpwd,
			 FrStruct *userinfo = 0) ;

void FramepaC_set_userinfo_dir(const char *dir) ;
const char *FramepaC_get_userinfo_dir() ;
extern void (*FramepaC_clear_userinfo_dir)() ;
const char *FramepaC_uifile_name() ;

// support for updating the password file
bool load_userinfo(const char *filename) ;
const FrList *FramepaC_get_userlist() ;
bool store_userinfo(const char *filename = 0) ;


#endif /* !__FRPASSWD_H_INCLUDED */

// end of file frpasswd.h //


