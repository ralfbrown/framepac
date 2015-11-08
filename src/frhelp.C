/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frhelp.C	    help system -- interface to Mosaic		*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,1998,2001,2006,2009,2011,2015	*/
/*		 Ralf Brown/Carnegie Mellon University			*/
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

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include "framerr.h"
#include "frfilutl.h"
#include "frhelp.h"
#include "frprintf.h"
#include "frutil.h"

#if defined(unix) || defined(__linux__) || defined(__GNUC__)
#include <unistd.h>
#endif /* unix */

#if defined(__WATCOMC__) || defined(_MSC_VER)
#include <io.h>
#include <process.h>
extern "C" int kill(int,int) ;
extern "C" int fork() ;
extern "C" int vfork() ;
#ifndef SIGUSR1
#  define SIGUSR1 30
#endif /* !SIGUSR1 */
#endif /* __WATCOMC__ || _MSC_VER */

//----------------------------------------------------------------------

#ifdef USE_VFORK
#  include <vfork.h>
#  define FORK vfork
#else
#  define FORK fork
#endif /* USE_VFORK */

/************************************************************************/
/************************************************************************/

static bool Initialized = false ;
static char *basedir ;
static const char *help_directory = FrHELP_SUBDIRECTORY ;
static char *cmdfile_name ;

static const char *default_browser_prog = FrMOSAIC_PROGNAME ;
static char *browser_prog ;

static int MosaicPID = 0 ;

/************************************************************************/
/************************************************************************/

#ifdef __WATCOMC__
// turn off warnings about concatenating string literals during array
// initialization by setting the level so high (10) that the warning
// isn't issued even with the -wx (all warnings) flag
#pragma warning 438 10
#endif /* __WATCOMC__ */

static const char *default_completion_helptexts[2] =
   {
   "This selection box lists the possible continuations\n"
   "or complete frame names matching the partial name\n"
   "you had already entered before pressing Tab.\n"
   "Pick one and click on 'Ok' to enter that selection\n"
   "into the prompt's input field.\n"
   , 0
   } ;

static const char *default_verify_helptexts[2] =
   {
   "This dialog is asking you to verify\n"
   "whether you really want to perform\n"
   "some action.  Click on 'Yes' if you\n"
   "really meant to select the command,\n"
   "'NO' if you've changed your mind."
   , 0 } ;

const char **FramepaC_helptexts[] =
   {
     default_completion_helptexts,
     default_verify_helptexts,
     //!!!
   } ;

/************************************************************************/
/************************************************************************/

//----------------------------------------------------------------------

static char *makeURL(const char *document)
{
   const char *bdir = (document[0] == '/') ? "" : basedir ;
   return Fr_aprintf("file:%s%s",bdir,document) ;
}

//----------------------------------------------------------------------
// fork off a copy of Mosaic, and remember its PID

static bool fork_Mosaic(char *document)
{
   int pid = FORK() ;
   if (pid > 0)
      {
      MosaicPID = pid ;
      cmdfile_name = Fr_aprintf("/tmp/Mosaic.%d",pid) ;
      }
   else if (pid == 0)
      {
      char *docURL = makeURL(document?document:FrMOSAIC_HOMEPAGE) ;
      char *homepage = makeURL(FrMOSAIC_HOMEPAGE) ;
      // exec Mosaic
      if (execlp(browser_prog,browser_prog,docURL,"-home",homepage,
		   "-ngh",(char *)0) == -1)
	 {
	 MosaicPID = 0 ;
	 _exit(1) ;		// exec failed, kill child process
	 }
      return true ;
      }
   else if (pid == -1)
      return false ;
   return true ;
}

//----------------------------------------------------------------------

bool FrInitializeHelp(const char *base_directory, const char *browser)
{
   if (!Initialized)
      {
      static const char localhost[] = "//localhost" ;
      size_t baselen = strlen(base_directory) ;
      size_t helplen = strlen(help_directory) ;
      basedir = FrNewN(char,baselen+helplen+16) ;
      memcpy(basedir,localhost,sizeof(localhost)) ;
      char *subdir = basedir + sizeof(localhost) - 1 ;
      memcpy(subdir, base_directory, baselen) ;
      subdir[baselen++] = '/' ;
      char *end = subdir + baselen ;
      memcpy(end,help_directory,helplen+1) ;
      if (!FrIsDirectory(subdir))
	 *end = 0 ;    // if unable to access help subdir, use main dir
      else
	 strcat(basedir,"/") ;
      browser_prog = FrDupString(browser ? browser : default_browser_prog) ;
      Initialized = true ;
      }
   return true ;
}

//----------------------------------------------------------------------

int popup_help(char *document)
{
   FILE *fp ;

   if (MosaicPID)
      {
      // create the command file as /tmp/mosaic.PID
      if ((fp = fopen(cmdfile_name,"w")) == 0)
	 {
	 cout << "Error opening Mosaic command file" << endl ;
	 return -1 ;
	 }
      // add the command to pop up the desired document
      char *url = makeURL(document) ;
      fprintf(fp,"goto\n%s\n",url) ;
      FrFree(url) ;
      fclose(fp) ;
      // send Mosaic a SIGUSR1 to get its attention
      if (kill(MosaicPID,SIGUSR1) == -1)
	 {
	 // error--maybe Mosaic died?
	 if (!fork_Mosaic(document))
	    {
	    cout << "Error signalling Mosaic" << endl ;
	    }
	 }
      }
   else if (!fork_Mosaic(document))
      FrMessage("Error encountered while starting the help system.") ;
   return 0 ; // for now
}

//----------------------------------------------------------------------

#if 0
static void popup_help(Widget w, XtPointer document, XtPointer)
{
   (void)w ; ;
   popup_help((char*)document) ;
}
#endif

//----------------------------------------------------------------------

void FrHelp1(Widget, XtPointer document, XtPointer)
{
   popup_help((char*)document) ;
}

//----------------------------------------------------------------------

void FrHelp(Widget w, XtPointer client_data, XtPointer)
{
   char *helptext = ((char**)client_data)[0] ;
   char *document = ((char**)client_data)[1] ;
#if defined(FrMOTIF)
   if (document)
      {
      FrWTextWindow helpwin(w,helptext,FrHelp1,(XtPointer)document) ;
      helpwin.retain() ;
      }
   else
      {
      FrWTextWindow helpwin(w,helptext) ;
      helpwin.retain() ;
      }
#else
   (void)w ;
   cout << helptext << endl ;
   if (document)
      {
      cout << endl ;
      cout << "-=- For more information, see file -=-" << endl ;
      cout << "	   " << document << endl ;
      }
#endif /* FrMOTIF */
}

//----------------------------------------------------------------------

void FrSetHelpTexts(int helptype, const char **helptexts)
{
   if (helptype >= 0 && helptype < FrHelptext_numtexts)
      FramepaC_helptexts[helptype] = helptexts ;
}

//----------------------------------------------------------------------

void FrShutdownHelp()
{
   if (Initialized)
      {
      // remove the command file if it still exists
      unlink(cmdfile_name) ;
      FrFree(cmdfile_name) ;
      FrFree(browser_prog) ;
      browser_prog = FrDupString(default_browser_prog) ;
      if (MosaicPID)
	 {
	 // kill off Mosaic
	 if (kill(MosaicPID,SIGTERM) == -1)
	    {
	      // error--maybe Mosaic already dead?
	    }
	 }
      Initialized = false ;
      }
}

//----------------------------------------------------------------------


// end of file frhelp.C //


