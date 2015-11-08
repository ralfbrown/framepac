/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File testnet.cpp	   Test/Demo program: network functions		*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,1998,2009				*/
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

#include <signal.h>
#include <time.h>

#include "testnet.h"
#include "frclisrv.h"
#include "clientno.h"

#ifdef FrSTRICT_CPLUSPLUS
#  include <cstdlib>
#  include <iomanip>
#else
#  include <iomanip.h>
#  include <stdlib.h>
#endif

#if defined(__SUNOS__) || defined(__SOLARIS__)
#include <unistd.h>
#endif /* __SUNOS__ || __SOLARIS__ */

/************************************************************************/
/*    Global variables							*/
/************************************************************************/

#ifdef FrSERVER
static long int shutdown_notify_time = -1 ;
static long int shutdown_time = -1 ;
static FrServer *running_server = 0 ;

static FrSignalHandler *sigint ;
#ifdef SIGHUP
static FrSignalHandler *sighup ;
#endif /* SIGHUP */
#endif /* FrSERVER */

/************************************************************************/
/************************************************************************/

#ifdef FrSERVER
static void client_register_command(FrClient *client)
{
   int clientid = 0 ;
   char *username = "user" ;
   char *password = "password" ;
   bool result = client->registerClient(clientid,username,password) ;
   if (!result)
      {
      }
}
#endif /* FrSERVER */

//----------------------------------------------------------------------

#ifdef FrSERVER
static void client_unregister_command(FrClient *client)
{
   bool result = client->unregisterClient() ;
   if (!result)
      {

      }
}
#endif /* FrSERVER */

//----------------------------------------------------------------------

#ifdef FrSERVER
static void list_users_command(FrClient *client)
{
  (void)client ; //!!!
}
#endif /* FrSERVER */

//----------------------------------------------------------------------

#ifdef FrSERVER
static FrRemoteDB *current_database = 0 ;

static void open_database_command(FrClient *client)
{
   char *dbname = "database" ;
   char *password = 0 ;
   if (current_database)
      client->closeDatabase(current_database) ;
   current_database = client->openDatabase(dbname,password) ;
   if (current_database)
      {

      }
   else
      {

      }
}
#endif /* FrSERVER */

//----------------------------------------------------------------------

#ifdef FrSERVER
static void close_database_command(FrClient *client)
{
   if (current_database)
      {
      if (!client->closeDatabase(current_database))
	 cout << "Error closing database" << endl ;
      else
	 cout << "Database closed." << endl ;
      }
   else
      cout << "No database open!" << endl ;
}
#endif /* FrSERVER */

//----------------------------------------------------------------------

void client_menu(ostream &out, istream &in)
{
#ifdef FrSERVER
   // open a connection to a server
   in >> ws ;
   out << "Server to which to connect: " << flush ;
   char servername[200] ;
   servername[0] = '\0' ;
   in.getline(servername,sizeof(servername)) ;
   FrClient *client = new FrClient(*servername ? servername : "localhost",
				   MIKCLIENT_FRAMEPAC) ;
   if (!client || !client->connected())
      {
      out << "Unable to connect to server!" << endl ;
      return ;
      }
   // process various commands on the network connection
   int choice ;
   do {
      choice = display_menu(out,in,true,5,"Client Commands",
			     "\t1. Register\n"
			     "\t2. Unregister\n"
			     "\t3. List Users\n"
			     "\t4. Open Database\n"
			     "\t5. Close Database\n"
			   ) ;
      switch (choice)
	 {
	 case 0:
	    // nothing
	    break ;
	 case 1:
	    client_register_command(client) ;
	    break ;
	 case 2:
	    client_unregister_command(client) ;
	    break ;
	 case 3:
	    list_users_command(client) ;
	    break ;
	 case 4:
	    open_database_command(client) ;
	    break ;
	 case 5:
	    close_database_command(client) ;
	    break ;
	 default:
	    FrMissedCase("client_menu") ;
	    break ;
	 }
      } while (choice > 0) ;
   // finally, close the network connection
   if (!client->disconnect())
      {
      out << "Error disconnecting from server!" << endl ;
      }
   delete client ;
#else /* FrSERVER */
   (void)in ; (void)out ;
#endif /* FrSERVER */
}

//----------------------------------------------------------------------

#ifdef FrSERVER
static void sigint_handler(int)
{
   if (shutdown_time == -1)
      {
      cout << endl ;
      cout << "=========================" << endl ;
      cout << "Server Shutdown Requested" << endl ;
      cout << "=========================" << endl ;
      cout << endl ;
      if (running_server && running_server->numConnections() > 0)
	 {
         cout << "The server will shut down and exit in two minutes.\n"
	      "Please be patient." << endl ;
         shutdown_notify_time = time(0) ;
         shutdown_time = shutdown_notify_time + 120 ;
	 return ;
	 }
      }
   else
      {
      cout << endl ;
      cout << "======================" << endl ;
      cout << "Server Abort Requested" << endl ;
      cout << "======================" << endl ;
      cout << endl ;
      cout << "The server is shutting down immediately.  WARNING: Data may\n"
	      "have been lost." << endl ;
      }
   running_server->shutdown(0) ;  // shut it down NOW
   FrSleep(1) ;
   cout << "The server has now shut down." << endl << flush ;
   exit(1) ;
}
#endif /* FrSERVER */

//----------------------------------------------------------------------

#ifdef FrSERVER
static void server_loop(FrServer *server)
{
   while (shutdown_time == -1)
     {
     server->process(500000) ;		// may block for up to 0.5 sec
     }
   long int t ;
   while ((t = time(0)) < shutdown_time)
     {
     if (t >= shutdown_notify_time)
        {
	server->shutdown(shutdown_notify_time - t) ;
	shutdown_notify_time += 25 ;
	if (shutdown_notify_time > shutdown_time)
	   shutdown_notify_time = shutdown_time - 5 ;
	}
     server->process(500000) ;		// may block for up to 0.5 sec
     }
  server->shutdown(0) ;			// shut it down NOW
  cout << "MikroKARAT Server has been shut down." << endl ;
  FrSleep(2) ;
}
#endif /* FrSERVER */

//----------------------------------------------------------------------

#ifdef FrSERVER
static bool start_server(ostream &out)
{
   sigint = new FrSignalHandler(SIGINT,sigint_handler) ;
   sighup = new FrSignalHandler(SIGHUP,sigint_handler) ;
   // log the server in with full privileges (server checks access levels
   // itself when acting on client's behalf)
   FrStruct *userinfo = make_userinfo("MikroKARAT Server",0,ROOT_ACCESS_LEVEL);
   login_user("MikroKARAT Server",0) ;
   remove_user("MikroKARAT Server") ;
   (void)userinfo ;		// avoid compiler warning
   FrServer *server = new FrServer(false) ;
   if (server)
      {
      server->startup() ;
      out << "The server is running.  Press Control-C to stop server."
	  << endl << endl ;
      server_loop(server) ;
      delete server ;
      return true ;
      }
   else
      {
      out << "UNABLE TO START SERVER!" << endl << endl ;
      return false ;
      }
}
#endif /* FrSERVER */

//----------------------------------------------------------------------

#ifdef FrSERVER
static void shutdown_server(ostream &out)
{
   out << "Shutting down server" << endl ;
   delete sigint ;
   sigint = 0 ;
   delete sighup ;
   sighup = 0 ;
   //!!!
}
#endif /* FrSERVER */

//----------------------------------------------------------------------

void server_menu(ostream &out, istream &in)
{
#ifdef FrSERVER
   int choice ;
   do {
      choice = display_menu(out,in,true,2,"Server Commands",
			     "\t1. Start Server\n"
			     "\t2. Stop Server\n") ;
      switch (choice)
	 {
	 case 1:
	    start_server(out) ;
	    break ;
	 case 2:
	    shutdown_server(out) ;
	    break ;
	 default:
	    FrMissedCase("server_menu") ;
	    break ;
	 }
      } while (choice > 0) ;
#else
   (void)in ; (void)out ;
#endif /* FrSERVER */
}

// end of file testnet.cpp //
