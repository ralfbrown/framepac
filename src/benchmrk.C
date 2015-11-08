/************************************************************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File benchmark.cpp	   Test/Demo program: benchmark functions	*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,2000,2004,2009,2013,2015		*/
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

#include "benchmrk.h"

#ifdef FrSTRICT_CPLUSPLUS
#  include <fstream>
#  include <iomanip>
#  include <cstdlib>
#else
#  include <fstream.h>
#  include <iomanip.h>
#  include <stdlib.h>
#endif


/************************************************************************/
/*	Type declarations						*/
/************************************************************************/

typedef void BenchmarkFunc(istream &in, ostream &out, size_t size,
			   unsigned int iterations, FrList *frames) ;

/************************************************************************/
/*	Forward declarations						*/
/************************************************************************/

/************************************************************************/
/*	Global variables						*/
/************************************************************************/

// how many blocks to (deliberately) leak on each iteration of the
//   memory-allocation benchmark.  This is used to test the
//   interaction of FramepaC's memory allocator with memory-checker
//   programs like Valgrind.
static size_t leakage_per_iteration = 0 ;

/************************************************************************/
/*	Benchmark timing routines					*/
/************************************************************************/

static FrTimer *active_timer = nullptr ;

void start_test(bool verbose = true)
{
   FramepaC_bgproc() ;		// handle any asynchronous operations
   if (verbose)
      cout << "\nRunning..." << flush ;
   if (active_timer)
      active_timer->start() ;
   else
      active_timer = new FrTimer() ;
   return ;
}

//----------------------------------------------------------------------

void stop_test(unsigned int iter,unsigned int size,bool complete)
{
   double elapsed_time, per_iter ;

   elapsed_time = active_timer->stopsec() ;
   delete active_timer ;
   active_timer = nullptr ;
   per_iter = (elapsed_time / iter) * 1000000.0 ;
   cout << " time was "	<< elapsed_time << " seconds" ;
   if (iter > 1)
      {
      cout << ", " ;
      if (per_iter > 1000)
	 cout << setprecision(5) << per_iter/1000 << "ms" ;
      else if (per_iter >= 0.5)
	 cout << setprecision(6) << per_iter << "us" ;
      else
	 cout << setprecision(6) << 1000*per_iter << "ns" ;
      cout << " per iter" ;
      if (size > 0)
	 {
	 if (per_iter/size < 1.0)
	    cout << " (" << setprecision(4) << (1000.0*per_iter/size) << "ns per alloc)" ;
	 else
	    cout << " (" << setprecision(4) << (per_iter/size) << "us per alloc)" ;
	 }
      else
	 cout << "ation" ;
      }
   cout << "." << endl ;
   if (complete)
      cout << "This benchmark is now complete." << endl << endl ;
   FramepaC_bgproc() ;		// handle any asynchronous operations
   return ;
}

/************************************************************************/
/************************************************************************/

static void announce_creation(ostream &out, const char *type, size_t size,
			      unsigned int iterations)
{
   out << "\nBenchmark of " << type << "frame creation and deletion\n\n"
	  "This test will now create " <<size<< " virtual frames without any\n"
	  "relational links, then immediately delete them; this process will\n"
	  "be repeated " << iterations << " times." << endl ;
   return ;
}

//----------------------------------------------------------------------

static void announce_inheritance(ostream &out, size_t size,
				 unsigned int iterations)
{
   out << "\nBenchmark of deep inheritance\n\n"
	  "This test will now create " << size
       << " frames, each IS-A the previous\n"
	  "frame, store a filler in the TEST slot of the top-most frame\n"
	  "and then read that slot in the bottom-most frame 100 times; the\n"
	  "frames are then deleted and the process repeated "
       << iterations << " times." << endl ;
   return ;
}

//----------------------------------------------------------------------

static void announce_relations(ostream &out, size_t size,
			       unsigned int iterations)
{
   out << "\nBenchmark of relation handling\n\n"
	  "We create " << size << " frames, adding to each three relational\n"
	  "links pointing at the same fixed frame, and then immediately\n"
	  "delete all of them; this process will be repeated " << iterations
       << " times." << endl ;
   return ;
}

//----------------------------------------------------------------------

static void announce_memalloc(ostream &out, size_t size,
			      unsigned int iterations)
{
   out << "\nBenchmark of Memory Allocation Speed\n\n"
	  "We first allocate/release " << size << " blocks of 20,000 bytes each\n"
	  "a total of " << iterations << " times, then allocate and release\n"
       << 10*size << " blocks of 200 bytes each a total of " << iterations
       << " times, and finally\nallocate and release " << 20*size
       << " blocks of 20 bytes each a total of " << iterations << " times."
       << endl << endl ;
   return ;
}

//----------------------------------------------------------------------

static void announce_suballoc(ostream &out, size_t size,
			      unsigned int iterations)
{
   out << "\nBenchmark of Memory Sub-Allocator Speed\n\n"
	  "We allocate and then release " << size << " objects a total\n"
	  "of " << iterations << " times." << endl << endl ;
   return ;
}

//----------------------------------------------------------------------

static void benchmark_makesymbol(istream &, ostream &out, size_t,
				 unsigned int iterations, FrList *)
{
   out << "\nMakeSymbol Benchmark\n\n"
	  "This test will indicate the processing overhead for using\n"
	  "makeSymbol() repeatedly instead of storing the result of a single\n"
	  "call, by invoking makeSymbol() " << iterations << " times on the\n"
	  "symbol name RELATION."
       << endl ;
   start_test() ;
   for (unsigned int pass = 0 ; pass < iterations ; pass++)
      {
      (void)makeSymbol("RELATION") ;
      }
   stop_test(iterations,0,true) ;
   return ;
}

//----------------------------------------------------------------------

static void benchmark_creation(istream &, ostream &out, size_t size,
			       unsigned int iterations, FrList *frames)
{
   FrList *fr ;

   announce_creation(out,"",size,iterations) ;
   delete_all_frames() ;
   start_test() ;
   for (unsigned int pass = 0 ; pass < iterations ; pass++)
      {
      for (fr = frames ; fr ; fr = fr->rest())
	 {
	 ((FrSymbol *)fr->first())->createFrame() ;
	 }
      delete_all_frames() ;
//      FramepaC_bgproc() ;		// handle any asynchronous operations
      }
   stop_test(iterations,0,true) ;
   return ;
}

//----------------------------------------------------------------------

static void benchmark_relations(istream &, ostream &out, size_t size,
				unsigned int iterations, FrList *frames)
{
   FrList *fr ;
#ifndef NAIVE_PROG
   FrSymbol *symbolISA = FrSymbolTable::add("IS-A") ;
   FrSymbol *symbolINSTANCEOF = FrSymbolTable::add("INSTANCE-OF") ;
   FrSymbol *symbolPARTOF = FrSymbolTable::add("PART-OF") ;
#endif

   announce_relations(out,size,iterations) ;
   delete_all_frames() ;
   start_test() ;
   for (unsigned int pass = 0 ; pass < iterations ; pass++)
      {
      FrSymbol *hubname = gensym("HUB") ;

      (void)hubname->createFrame() ;
      for (fr = frames ; fr ; fr = fr->rest())
	 {
#ifdef NAIVE_PROG
	 FrFrame *frame = ((FrSymbol *)fr->first())->createFrame() ;
	 frame->addFiller(makeSymbol("IS-A"),makeSymbol("VALUE"),hubname) ;
	 frame->addFiller(makeSymbol("INSTANCE-OF"),makeSymbol("VALUE"),
			  hubname) ;
	 frame->addFiller(makeSymbol("PART-OF"),makeSymbol("VALUE"),hubname) ;
#else
	 FrFrame *frame = ((FrSymbol *)(fr->first()))->createFrame() ;
	 frame->addValue(symbolISA,hubname) ;
	 frame->addValue(symbolINSTANCEOF,hubname) ;
	 frame->addValue(symbolPARTOF,hubname) ;
#endif
	 }
      // delete the frames in the opposite order from that in which
      // we created them
      frames = listreverse(frames) ;
      for (fr = frames ; fr ; fr = fr->rest())
	 {
	 if (delete_frame((FrSymbol *)fr->first()) != 0)
	    cout << '.' << flush ;
	 }
      frames = listreverse(frames) ;
      FramepaC_bgproc() ;		// handle any asynchronous operations
      }
   stop_test(iterations,0,true) ;
   return ;
}

//----------------------------------------------------------------------

static void benchmark_inheritance(istream &, ostream &out, size_t size,
				  unsigned int iterations, FrList *frames)
{
   FrSymbol *symbolISA = FrSymbolTable::add("IS-A") ;
   FrSymbol *symbolTEST = FrSymbolTable::add("TEST") ;
   int i ;

   announce_inheritance(out,size,iterations) ;
   delete_all_frames() ;
   start_test() ;
   for (unsigned int pass = 0 ; pass < iterations ; pass++)
      {
      FrFrame *topmost, *bottom ;
      FrSymbol *current ;

      topmost = (gensym("TEST"))->createFrame() ;
      bottom = 0 ;    // avoid "uninit var" warning
      topmost->addValue(symbolTEST,makeSymbol("SUCCESS")) ;
      current = frame_name(topmost) ;
      for (FrList *fr = frames ; fr ; fr = fr->rest())
	 {
	 bottom = ((FrSymbol *)fr->first())->createFrame() ;
	 bottom->addValue(symbolISA,current) ;
	 current = (FrSymbol *)fr->first() ;
	 }
      for (i = 0 ; i < 100 ; i++)
	 {
#ifdef NAIVE_PROG
	 (void)bottom->getFillers(makeSymbol("TEST"),makeSymbol("VALUE"),true);
#else
	 (void)bottom->getValues(symbolTEST,true) ;
#endif
	 }
      delete_all_frames() ;
      FramepaC_bgproc() ;		// handle any asynchronous operations
      }
   stop_test(iterations,0,true) ;
   return ;
}

//----------------------------------------------------------------------

static void benchmark_output(istream &, ostream &out, size_t,
			     unsigned int iterations, FrList *)
{
   out << "\nOutput Speed Benchmark\n\n"
       << "This test will indicate how long it takes to convert various objects\n"
       << "into their printed representations; several objects will each be converted\n"
       << "to a string " << iterations << " times." << endl ;
   FrObject *obj = makeSymbol("TEST-SYMBOL") ;
   int length = obj->displayLength() ;
   char *buffer = FrNewN(char,length+1) ;
   out << "\nThe first object is the symbol " << obj
       << ", whose printed length" << endl
       << "is " << length << " bytes." << endl ;
   start_test() ;
   FrFrame *frame ;
   unsigned int pass ;
   for (pass = 0 ; pass < iterations ; pass++)
      {
      obj->displayValue(buffer) ;
      }
   stop_test(iterations,0,false) ;
   FrFree(buffer) ;
   free_object(obj) ;
   //--------------------
   obj = makelist(makeSymbol("SYMBOL"),new FrString("String"),
		  new FrCons(makeSymbol("TEST"),new FrString("S2")),
		  new FrFloat(1.5), new FrInteger(42),(FrObject *)0) ;
   length = obj->displayLength() ;
   buffer = FrNewN(char,length+1) ;
   obj->displayValue(buffer) ;
   out << "\nThe second object is the list " << buffer
       << "\nwhose printed length is " << length << " bytes." << endl ;
   start_test() ;
   for (pass = 0 ; pass < iterations ; pass++)
      {
      obj->displayValue(buffer) ;
      }
   stop_test(iterations,0,false) ;
   FrFree(buffer) ;
   free_object(obj) ;
   //--------------------
   frame = (makeSymbol("TEST-FRAME"))->createFrame() ;
   frame->addValue(makeSymbol("IS-A"),makeSymbol("FOO")) ;
   frame->addSem(makeSymbol("HAS-PARTS"),makeSymbol("*PHYSICAL-OBJECT")) ;
   frame->addFiller(makeSymbol("TEST-SLOT1"),makeSymbol("DEFAULT"),
		    new FrInteger(5)) ;
   frame->addFillers(makeSymbol("TEST-SLOT2"),makeSymbol("M-UNIT"),
		     makelist(makeSymbol("GRAMS"),makeSymbol("OUNCES"),
			      makeSymbol("GRAINS"),(FrObject *)0)) ;
   length = frame->displayLength() ;
   buffer = FrNewN(char,length+1) ;
   frame->displayValue(buffer) ;
   out << "\nThe final object is the FrFrame " << buffer
       << "\nwhose printed length is " << length << " bytes." << endl ;
   start_test() ;
   for (pass = 0 ; pass < iterations ; pass++)
      {
      frame->displayValue(buffer) ;
      }
   stop_test(iterations,0,true) ;
   FrFree(buffer) ;
   frame->eraseFrame() ;
   return ;
}

//----------------------------------------------------------------------

static void benchmark_input(istream &, ostream &out, size_t,
			    unsigned int iterations, FrList *)
{
   unsigned int pass ;
   FrObject *obj ;
   const char *str1 = "TEST-SYMBOL" ;
   const char *str2 = "(SYMBOL \"String\" (|Symbol2| \"S2\") 1.5 42)" ;
   const char *str3 = "[TEST-FRAME[IS-A[VALUE FOO]][SUBCLASSES][INSTANCE-OF]"
		"[INSTANCES][HAS-PARTS[SEM *PHYSICAL-OBJECT]][PART-OF]"
		"[TEST-SLOT1[DEFAULT 5]]"
		"[TEST-SLOT2[M-UNIT GRAMS OUNCES GRAINS]]]" ;
   const char *str ;

   out << "\nInput Speed Benchmark\n\n"
       << "This test will indicate how quickly objects may be read into the\n"
	  "system.  Each of three objects will be converted from a string to\n"
	  "the actual object and then freed " << iterations << " times."
       << endl
       << endl ;
   //-----------------
   str = str1 ;
   obj = string_to_FrObject(str) ;
   out << "The first object to be converted is " << obj << endl ;
   free_object(obj) ;
   start_test() ;
   for (pass = 0 ; pass < iterations ; pass++)
      {
      str = str1 ;
      obj = string_to_FrObject(str) ;
      free_object(obj) ;
      }
   stop_test(iterations,0,false) ;
   //-----------------
   str = str2 ;
   obj = string_to_FrObject(str) ;
   out << "\nThe second object to be converted is " << obj << endl ;
   free_object(obj) ;
   start_test() ;
   for (pass = 0 ; pass < iterations ; pass++)
      {
      str = str2 ;
      obj = string_to_FrObject(str) ;
      free_object(obj) ;
      }
   stop_test(iterations,0,false) ;
   //-----------------
   str = str3 ;
   obj = string_to_FrObject(str) ;
   out << "\nThe final object to be converted is\n" << obj << endl ;
   delete (FrFrame*)obj ;
   start_test() ;
   for (pass = 0 ; pass < iterations ; pass++)
      {
      str = str3 ;
      obj = string_to_FrObject(str) ;
      delete (FrFrame*)obj ;	// wipe frame completely out of existence
      }
   stop_test(iterations,0,true) ;
   return ;
}

//----------------------------------------------------------------------

static void benchmark_vframe_creation(istream &, ostream &out, size_t size,
				      unsigned int iterations, FrList *frames)
{
   FrList *fr ;

   announce_creation(out,"virtual ",size,iterations) ;
   delete_all_frames() ;
   start_test() ;
   for (unsigned int pass = 0 ; pass < iterations ; pass++)
      {
      for (fr = frames ; fr ; fr = fr->rest())
	 {
	 (void)((FrSymbol *)fr->first())->createVFrame() ;
	 }
      if (pass == 0)
	 synchronize_VFrames() ;
      FramepaC_bgproc() ;		// handle any asynchronous operations
      for (fr = frames ; fr ; fr = fr->rest())
	 {
	 (void)delete_frame((FrSymbol *)fr->first()) ;
	 }
      FramepaC_bgproc() ;		// handle any asynchronous operations
      }
   stop_test(iterations,0,true) ;
   return ;
}

//----------------------------------------------------------------------

static void benchmark_vframe_relations(istream &, ostream &out, size_t size,
				       unsigned int iterations, FrList *frames)
{
   FrList *fr ;
#ifndef NAIVE_PROG
   FrSymbol *symbolISA = FrSymbolTable::add("IS-A") ;
   FrSymbol *symbolINSTANCEOF = FrSymbolTable::add("INSTANCE-OF") ;
   FrSymbol *symbolPARTOF = FrSymbolTable::add("PART-OF") ;
#endif

   announce_relations(out,size,iterations) ;
   delete_all_frames() ;
   start_test() ;
   for (unsigned int pass = 0 ; pass < iterations ; pass++)
      {
      FrSymbol *hubname = gensym("HUB") ;

      (void)hubname->createFrame() ;
      for (fr = frames ; fr ; fr = fr->rest())
	 {
	 FrSymbol *frame = (FrSymbol *)fr->first() ;

	 (void)frame->createVFrame() ;
#ifdef NAIVE_PROG
	 frame->addFiller(makeSymbol("IS-A"),makeSymbol("VALUE"),hubname) ;
	 frame->addFiller(makeSymbol("INSTANCE-OF"),makeSymbol("VALUE"),
			  hubname) ;
	 frame->addFiller(makeSymbol("PART-OF"),makeSymbol("VALUE"),hubname) ;
#else
	 frame->addValue(symbolISA,hubname) ;
	 frame->addValue(symbolINSTANCEOF,hubname) ;
	 frame->addValue(symbolPARTOF,hubname) ;
#endif
	 }
      // delete the frames in the opposite order from that in which
      // we created them
      frames = listreverse(frames) ;
      for (fr = frames ; fr ; fr = fr->rest())
	 {
	 if (delete_frame((FrSymbol *)fr->first()) != 0)
	    cout << '.' << flush ;
	 }
      frames = listreverse(frames) ;
      FramepaC_bgproc() ;		// handle any asynchronous operations
      }
   stop_test(iterations,0,true) ;
   return ;
}

//----------------------------------------------------------------------

static void benchmark_vframe_inheritance(istream &, ostream &out, size_t size,
					 unsigned int iterations,
					 FrList *frames)
{
   int i ;
   FrList *fr ;
   FrSymbol *symbolISA = makeSymbol("IS-A") ;
   FrSymbol *symbolTEST = makeSymbol("TEST") ;

   announce_inheritance(out,size,iterations) ;
   delete_all_frames() ;
   start_test() ;
   for (unsigned int pass = 0 ; pass < iterations ; pass++)
      {
      FrSymbol *topmost, *bottom ;
      FrSymbol *current ;

      topmost = gensym("TEST") ;
      (void)topmost->createVFrame() ;
      topmost->addValue(symbolTEST,makeSymbol("SUCCESS")) ;
      current = topmost ;
      bottom = 0 ;   // avoid "uninit var" warning
      for (fr = frames ; fr ; fr = fr->rest())
	 {
	 bottom = (FrSymbol *)fr->first() ;

	 (void)bottom->createVFrame() ;
	 bottom->addValue(symbolISA,current) ;
	 current = bottom ;
	 }
      for (i = 0 ; i < 100 ; i++)
	 {
#ifdef NAIVE_PROG
	 (void)bottom->getFiller(makeSymbol("TEST"),makeSymbol("VALUE"),true);
#else
	 (void)bottom->getValues(symbolTEST,true);
#endif
	 }
      for (fr = frames ; fr ; fr = fr->rest())
	 {
	 (void)delete_frame((FrSymbol *)fr->first()) ;
	 }
      FramepaC_bgproc() ;		// handle any asynchronous operations
      }
   stop_test(iterations,0,true) ;
   return ;
}

//----------------------------------------------------------------------

static void vframes_explore(istream &in)
{
   FrObject *obj = nullptr ;
   bool v = read_virtual_frames(true) ;

   do {
      free_object(obj) ; // free the object from the prev pass
      obj = nullptr ;
      cout << "\n(VFrame) Enter a FrObject, NIL to end: " ;
      in >> obj ;
      if (obj && obj->symbolp() && obj == findSymbol("SAVE"))
	 {
	 cout << "Updating Database..." << flush ;
	 synchronize_VFrames() ;
	 cout << "done." << endl ;
	 }
      else if (obj && obj->symbolp() && obj == findSymbol("*"))
	 interpret_command(cout,in,false) ;
      else
	 display_object_info(cout,obj) ;
      } while (!NIL_symbol(obj)) ;
   free_object(obj) ;
   read_virtual_frames(v) ;
   return ;
}

//----------------------------------------------------------------------

static void benchmark_vframe_menu(istream &in, ostream &out,
				  const char *frametype)
{
   int choice ;
   int size ;
   unsigned int iterations ;
   FrList *frames ;

   do {
      cout << "Virtual Frames (" << frametype << ") Benchmarks:" << endl ;
      choice = display_menu(cout,in,true,4,
			    0,
      			    "\t1. FrFrame creation/deletion\n"
			    "\t2. Relation management\n"
			    "\t3. Inheritance\n"
			    "\t4. Interactive Exploration\n"
	 		   ) ;
      frames = nullptr ;
      if (choice != 0 && choice != 4)
	 {
	 cout << "Please enter test size and number of iterations, separated\n"
	      << "by a blank: " << flush ;
	 in >> size >> iterations ;
	 if (size < 1)
	    size = 1 ;
	 // expand the symbol table to the needed size in one fell swoop
	 // to avoid later memory fragmentation
	 FrSymbolTable *symtab = FrSymbolTable::current() ;
	 symtab->expandTo(size+100) ;
	 cout << "\nGENSYM'ing frame names..." << endl ;
	 for (int i = 0 ; i < size ; i++)
	    pushlist(gensym("TEST"),frames) ;
         }
      switch (choice)
	 {
	 case 0:
	    // do nothing
	    break ;
	 case 1:
	    benchmark_vframe_creation(in, out, size,iterations,frames) ;
	    break ;
	 case 2:
	    benchmark_vframe_relations(in, out, size,iterations,frames) ;
	    break ;
	 case 3:
	    benchmark_vframe_inheritance(in, out, size,iterations,frames) ;
	    break ;
	 case 4:
	    vframes_explore(in) ;
	    break ;
	 default:
	    FrMissedCase("benchmark_vframe_menu") ;
	    break ;
	 }
      free_object(frames) ;
      } while (choice != 0) ;
   return ;
}

//----------------------------------------------------------------------

static void benchmark_vframe_mem(istream &in, ostream &out, size_t,
				 unsigned int /*iterations*/, FrList *)
{
   FrSymbolTable *symtab = initialize_VFrames_memory(0) ;
   if (symtab)
      {
      benchmark_vframe_menu(in,out,"memory") ;
      shutdown_VFrames(symtab) ;
      }
   else
      {
      out << "Error while attempting to start virtual frames in memory."
	  << endl ;
      }
   return ;
}

//----------------------------------------------------------------------

static void benchmark_vframe_disk(istream &in, ostream &out, size_t,
				  unsigned int /*iterations*/, FrList *)
{
   char filename[128] ;
   FrSymbolTable *symtab ;

   out << "Name of database containing frames: " << flush ;
   in >> filename ;
   out << endl ;
   symtab = initialize_VFrames_disk(filename,0,true) ;
   if (symtab)
      {
      if (symtab->isReadOnly())
	 out << "Database is read-only" << endl ;
      benchmark_vframe_menu(in,out,"disk") ;
      shutdown_VFrames(symtab) ;
      }
   else if (Fr_errno == ME_PRIVILEGED)
      out << "Insufficient privileges to open the database." << endl ;
   else
      {
      out << "Error while attempting to start virtual frames on disk."
	  << endl ;
      }
   return ;
}

//----------------------------------------------------------------------

static void notify_handler(VFrameNotifyType type, const FrSymbol *frame)
{
   cout << "Someone else just " ;
   switch (type)
      {
      case VFNot_CREATE:
	 cout << "created" ;
	 break ;
      case VFNot_DELETE:
	 cout << "deleted" ;
	 break ;
      case VFNot_UPDATE:
	 cout << "updated" ;
	 break ;
      case VFNot_LOCK:
	 cout << "locked" ;
	 break ;
      case VFNot_UNLOCK:
	 cout << "unlocked" ;
	 break ;
      default:
	 cout << "did something unknown to" ;
      }
   cout << " frame " << frame << endl ;
   return ;
}

//----------------------------------------------------------------------

static void setup_notification_handlers(FrSymbolTable *symtab)
{
   set_notification(symtab,VFNot_CREATE,notify_handler) ;
   set_notification(symtab,VFNot_DELETE,notify_handler) ;
   set_notification(symtab,VFNot_UPDATE,notify_handler) ;
   set_notification(symtab,VFNot_LOCK,notify_handler) ;
   set_notification(symtab,VFNot_UNLOCK,notify_handler) ;
   return ;
}

//----------------------------------------------------------------------

static void remove_notification_handlers(FrSymbolTable *symtab)
{
   set_notification(symtab,VFNot_CREATE,0) ;
   set_notification(symtab,VFNot_DELETE,0) ;
   set_notification(symtab,VFNot_UPDATE,0) ;
   set_notification(symtab,VFNot_LOCK,0) ;
   set_notification(symtab,VFNot_UNLOCK,0) ;
   return ;
}

//----------------------------------------------------------------------

static void benchmark_vframe_server(istream &in, ostream &out, size_t,
				    unsigned int /*iterations*/, FrList *)
{
   char servername[128] ;
   char databasename[128] ;
   char username[128] ;
   char password[128] ;
   int portnumber ;
   FrSymbolTable *symtab ;

   out << "Name of machine running server: " << flush ;
   in >> servername ;
   out << "Port number: " << flush ;
   in >> portnumber ;
   out << "User Name: " << flush ;
   in >> username ;
   out << "Password: " << flush ;
   in >> password ;
   out << "Name of database on server: " << flush ;
   in >> databasename ;
   out << endl ;
   symtab = initialize_VFrames_server(servername,portnumber, username,
				      password,databasename,0, false) ;
   if (symtab)
      {
      if (symtab->isReadOnly())
	 out << "Database is read-only" << endl ;
      setup_notification_handlers(FrSymbolTable::current()) ;
      benchmark_vframe_menu(in,out,"server") ;
      remove_notification_handlers(FrSymbolTable::current()) ;
      shutdown_VFrames(symtab) ;
      }
   else if (Fr_errno == ME_PRIVILEGED)
      out << "Insufficient privileges to open the database." << endl ;
   else
      {
      out << "Error while attempting to start virtual frames on server."
	  << endl ;
      }
   return ;
}

//----------------------------------------------------------------------
// LOOM benchmark 0: overhead-test

static void fkbench_0(int size, FrSymbol *)
{
   for (int i = 0 ; i < size ; i++)
      {
//      FrSymbol *name = gensym("G") ;
      }
   return ;
}

//----------------------------------------------------------------------
// LOOM benchmark 1: create-unrelated-frames

static void fkbench_1(int size, FrSymbol *)
{
   for (int i = 0 ; i < size ; i++)
      {
      FrSymbol *name = gensym("G") ;
      name->createFrame() ;
      name->addFiller(makeSymbol("SLOT"),makeSymbol("VALUE"),makeSymbol("V")) ;
      }
   return ;
}

static void fkbench_1_opt(int size, FrSymbol *)
{
   FrSymbol *symSLOT = makeSymbol("SLOT") ;
   FrSymbol *symV = makeSymbol("V") ;
   for (int i = 0 ; i < size ; i++)
      {
      FrSymbol *name = gensym("G") ;
      name->createFrame() ;
      name->addValue(symSLOT,symV) ;
      }
   return ;
}

//----------------------------------------------------------------------
// LOOM benchmark 2: get-unrelated-frame

static void fkbench_2(int size, FrSymbol *root)
{
   for (int i = 0 ; i < size ; i++)
      {
      (void)root->getFillers(makeSymbol("SLOT"),makeSymbol("VALUE")) ;
      }
   return ;
}

static void fkbench_2_opt(int size, FrSymbol *root)
{
   FrSymbol *symSLOT = makeSymbol("SLOT") ;
   for (int i = 0 ; i < size ; i++)
      {
      (void)root->getValues(symSLOT) ;
      }
   return ;
}

//----------------------------------------------------------------------
// LOOM benchmark 3: set-unrelated-frame

static void fkbench_3(int size, FrSymbol *root)
{
   for (int i = 0 ; i < size ; i++)
      {
      root->eraseFacet(makeSymbol("SLOT"),makeSymbol("VALUE")) ;
      FrInteger filler(i) ;
      // optimization bug in Watcom C++ v10.0a precludes direct use of
      // &FrInteger(i) in addFiller() call....
      root->addFiller(makeSymbol("SLOT"),makeSymbol("VALUE"),&filler) ;
      }
   return ;
}

static void fkbench_3_opt(int size, FrSymbol *root)
{
   FrSymbol *symSLOT = makeSymbol("SLOT") ;
   FrSymbol *symVALUE = makeSymbol("VALUE") ;
   for (int i = 0 ; i < size ; i++)
      {
      root->eraseFacet(symSLOT,symVALUE) ;
      FrInteger filler(i) ;
      root->addValue(symSLOT,&filler) ;
      }
   return ;
}

//----------------------------------------------------------------------
// LOOM benchmark 4: create-unrelated-instances

static void fkbench_4(int size, FrSymbol *root)
{
   for (int i = 0 ; i < size ; i++)
      {
      FrFrame *frame = root->createInstanceFrame() ;
      FrInteger filler(0) ;
      // optimization bug in Watcom C++ v10.0a precludes direct use of
      // &FrInteger(0) in addFiller() call....
      frame->addFiller(makeSymbol("SLOT"),makeSymbol("VALUE"),&filler) ;
      }
   return ;
}

static void fkbench_4_opt(int size, FrSymbol *root)
{
   FrSymbol *symSLOT = makeSymbol("SLOT") ;
   FrInteger filler(0) ;
   for (int i = 0 ; i < size ; i++)
      {
      FrFrame *frame = root->createInstanceFrame() ;
      frame->addValue(symSLOT,&filler) ;
      }
   return ;
}

//----------------------------------------------------------------------
// LOOM benchmark 5: get-unrelated-instance

static void fkbench_5(int size, FrSymbol *root)
{
   FrFrame *frame = root->createInstanceFrame() ;
   FrSymbol *instance = frame->frameName() ;
   for (int i = 0 ; i < size ; i++)
      {
      (void)instance->getFillers(makeSymbol("SLOT"),makeSymbol("VALUE")) ;
      }
   return ;
}

static void fkbench_5_opt(int size, FrSymbol *root)
{
   FrFrame *frame = root->createInstanceFrame() ;
   FrSymbol *instance = frame->frameName() ;
   FrSymbol *symSLOT = makeSymbol("SLOT") ;
   for (int i = 0 ; i < size ; i++)
      {
      (void)instance->getValues(symSLOT) ;
      }
   return ;
}

//----------------------------------------------------------------------
// LOOM benchmark 6: set-unrelated-instance

static void fkbench_6(int size, FrSymbol *root)
{
   FrFrame *frame = root->createInstanceFrame() ;
   FrSymbol *instance = frame->frameName() ;
   for (int i = 0 ; i < size ; i++)
      {
      instance->eraseFacet(makeSymbol("SLOT"),makeSymbol("VALUE")) ;
      FrInteger filler(i) ;
      instance->addFiller(makeSymbol("SLOT"),makeSymbol("VALUE"),&filler) ;
      }
   return ;
}

static void fkbench_6_opt(int size, FrSymbol *root)
{
   FrFrame *frame = root->createInstanceFrame() ;
   FrSymbol *instance = frame->frameName() ;
   FrSymbol *symSLOT = makeSymbol("SLOT") ;
   FrSymbol *symVALUE = makeSymbol("VALUE") ;
   for (int i = 0 ; i < size ; i++)
      {
      instance->eraseFacet(symSLOT,symVALUE) ;
      FrInteger filler(i) ;
      instance->addValue(symSLOT,&filler) ;
      }
   return ;
}

//----------------------------------------------------------------------

static FrList *create_frame_tree(int size, FrSymbol *root)
{
   FrSymbol *parent = root ;
   FrSymbol *child = nullptr ;
   FrInteger filler(0) ;
   root->addFiller(makeSymbol("INHERITED-SLOT"),makeSymbol("VALUE"),&filler) ;
   for (int i = 0 ; i < size ; i++)
      {
      child = gensym("G") ;
      child->createFrame() ;
      child->addFiller(makeSymbol("IS-A"),makeSymbol("VALUE"),parent) ;
      parent = child ;
      }
   return new FrList(root,child) ;
}

static FrList *create_frame_tree_opt(int size, FrSymbol *root)
{
   FrSymbol *symISA = makeSymbol("IS-A") ;
   FrSymbol *symSLOT = makeSymbol("INHERITED-SLOT") ;
   FrSymbol *parent = root ;
   FrSymbol *child = nullptr ;
   FrInteger filler(0) ;
   root->addValue(symSLOT,&filler) ;
   FrSymbolTable *symtab = FrSymbolTable::current() ;
   for (int i = 0 ; i < size ; i++)
      {
      child = symtab->gensym("G") ;
      child->createFrame() ;
      child->addValue(symISA,parent) ;
      parent = child ;
      }
   return new FrList(root,child) ;
}

//----------------------------------------------------------------------
// LOOM benchmark 7: create-frame-tree

static void fkbench_7(int size, FrSymbol *root)
{
   FrList *root_leaf = create_frame_tree(size,root) ;
   root_leaf->freeObject() ;
   return ;
}

static void fkbench_7_opt(int size, FrSymbol *root)
{
   FrList *root_leaf = create_frame_tree_opt(size,root) ;
   root_leaf->freeObject() ;
   return ;
}

//----------------------------------------------------------------------
// LOOM benchmark 8: get-leaf-frame

static void fkbench_8(int size, FrSymbol *root)
{
   FrList *root_leaf = create_frame_tree(size,root) ;
   FrSymbol *leaf = (FrSymbol*)root_leaf->second() ;
   for (int i = 0 ; i < size ; i++)
      {
      (void)leaf->getFillers(makeSymbol("INHERITED-SLOT"),
			     makeSymbol("VALUE")) ;
      }
   root_leaf->freeObject() ;
   return ;
}

static void fkbench_8_opt(int size, FrSymbol *root)
{
   FrSymbol *symSLOT = makeSymbol("INHERITED-SLOT") ;
   FrList *root_leaf = create_frame_tree_opt(size,root) ;
   FrSymbol *leaf = (FrSymbol*)root_leaf->second() ;
   for (int i = 0 ; i < size ; i++)
      {
      (void)leaf->getValues(symSLOT) ;
      }
   root_leaf->freeObject() ;
   return ;
}

//----------------------------------------------------------------------
// LOOM benchmark 9: set-leaf-frame

static void fkbench_9(int size, FrSymbol *root)
{
   FrList *root_leaf = create_frame_tree(size,root) ;
   FrSymbol *leaf = (FrSymbol*)root_leaf->second() ;
   for (int i = 0 ; i < size ; i++)
      {
      leaf->eraseFacet(makeSymbol("INHERITED-SLOT"),makeSymbol("VALUE")) ;
      FrInteger filler(i) ;
      leaf->addFiller(makeSymbol("INHERITED-SLOT"),makeSymbol("VALUE"),
		      &filler) ;
      }
   root_leaf->freeObject() ;
   return ;
}

static void fkbench_9_opt(int size, FrSymbol *root)
{
   FrSymbol *symSLOT = makeSymbol("INHERITED-SLOT") ;
   FrSymbol *symVALUE = makeSymbol("VALUE") ;
   FrList *root_leaf = create_frame_tree_opt(size,root) ;
   FrSymbol *leaf = (FrSymbol*)root_leaf->second() ;
   for (int i = 0 ; i < size ; i++)
      {
      leaf->eraseFacet(symSLOT,symVALUE) ;
      FrInteger filler(i) ;
      leaf->addValue(symSLOT,&filler) ;
      }
   root_leaf->freeObject() ;
   return ;
}

//----------------------------------------------------------------------
// LOOM benchmark 10: set-root-frame

static void fkbench_10(int size, FrSymbol *root)
{
   FrList *root_leaf = create_frame_tree(size,root) ;
   root = (FrSymbol*)root_leaf->first() ;
   for (int i = 0 ; i < size ; i++)
      {
      root->eraseFacet(makeSymbol("INHERITED-SLOT"),makeSymbol("VALUE")) ;
      FrInteger filler(i) ;
      root->addFiller(makeSymbol("INHERITED-SLOT"),makeSymbol("VALUE"),
		      &filler) ;
      }
   root_leaf->freeObject() ;
   return ;
}

static void fkbench_10_opt(int size, FrSymbol *root)
{
   FrSymbol *symSLOT = makeSymbol("INHERITED-SLOT") ;
   FrSymbol *symVALUE = makeSymbol("VALUE") ;
   FrList *root_leaf = create_frame_tree_opt(size,root) ;
   root = (FrSymbol*)root_leaf->first() ;
   for (int i = 0 ; i < size ; i++)
      {
      root->eraseFacet(symSLOT,symVALUE) ;
      FrInteger filler(i) ;
      root->addValue(symSLOT,&filler) ;
      }
   root_leaf->freeObject() ;
   return ;
}

//----------------------------------------------------------------------
// LOOM benchmark 11: create-leaf-instance

static void fkbench_11(int size, FrSymbol *root)
{
   FrList *root_leaf = create_frame_tree(size,root) ;
   FrSymbol *leaf = (FrSymbol*)root_leaf->second() ;
   FrFrame *inst = leaf->createInstanceFrame() ;
   FrSymbol *instance = inst->frameName() ;
   for (int i = 0 ; i < size ; i++)
      {
      FrInteger filler(0) ;
      instance->addFiller(makeSymbol("SLOT"),makeSymbol("VALUE"),&filler) ;
      }
   root_leaf->freeObject() ;
   return ;
}

static void fkbench_11_opt(int size, FrSymbol *root)
{
   FrSymbol *symSLOT = makeSymbol("SLOT") ;
   FrList *root_leaf = create_frame_tree_opt(size,root) ;
   FrSymbol *leaf = (FrSymbol*)root_leaf->second() ;
   FrFrame *inst = leaf->createInstanceFrame() ;
   FrSymbol *instance = inst->frameName() ;
   FrInteger filler(0) ;
   for (int i = 0 ; i < size ; i++)
      {
      instance->addValue(symSLOT,&filler) ;
      }
   root_leaf->freeObject() ;
   return ;
}

//----------------------------------------------------------------------
// LOOM benchmark 12: get-leaf-instance

static void fkbench_12(int size, FrSymbol *root)
{
   FrList *root_leaf = create_frame_tree(size,root) ;
   FrSymbol *leaf = (FrSymbol*)root_leaf->second() ;
   FrFrame *inst = leaf->createInstanceFrame() ;
   FrSymbol *instance = inst->frameName() ;
   for (int i = 0 ; i < size ; i++)
      {
      (void)instance->getFillers(makeSymbol("INHERITED-SLOT"),
			     makeSymbol("VALUE")) ;
      }
   root_leaf->freeObject() ;
   return ;
}

static void fkbench_12_opt(int size, FrSymbol *root)
{
   FrSymbol *symSLOT = makeSymbol("INHERITED-SLOT") ;
   FrList *root_leaf = create_frame_tree_opt(size,root) ;
   FrSymbol *leaf = (FrSymbol*)root_leaf->second() ;
   FrFrame *inst = leaf->createInstanceFrame() ;
   FrSymbol *instance = inst->frameName() ;
   for (int i = 0 ; i < size ; i++)
      {
      (void)instance->getValues(symSLOT) ;
      }
   root_leaf->freeObject() ;
   return ;
}

//----------------------------------------------------------------------
// LOOM benchmark 13: set-leaf-instance

static void fkbench_13(int size, FrSymbol *root)
{
   FrList *root_leaf = create_frame_tree(size,root) ;
   FrSymbol *leaf = (FrSymbol*)root_leaf->second() ;
   FrFrame *inst = leaf->createInstanceFrame() ;
   FrSymbol *instance = inst->frameName() ;
   for (int i = 0 ; i < size ; i++)
      {
      instance->eraseFacet(makeSymbol("SLOT"),makeSymbol("VALUE")) ;
      FrInteger filler(i) ;
      instance->addFiller(makeSymbol("SLOT"),makeSymbol("VALUE"),&filler) ;
      }
   root_leaf->freeObject() ;
   return ;
}

static void fkbench_13_opt(int size, FrSymbol *root)
{
   FrSymbol *symSLOT = makeSymbol("SLOT") ;
   FrSymbol *symVALUE = makeSymbol("VALUE") ;
   FrList *root_leaf = create_frame_tree_opt(size,root) ;
   FrSymbol *leaf = (FrSymbol*)root_leaf->second() ;
   FrFrame *inst = leaf->createInstanceFrame() ;
   FrSymbol *instance = inst->frameName() ;
   for (int i = 0 ; i < size ; i++)
      {
      instance->eraseFacet(symSLOT,symVALUE) ;
      FrInteger filler(i) ;
      instance->addValue(symSLOT,&filler) ;
      }
   root_leaf->freeObject() ;
   return ;
}

//----------------------------------------------------------------------

typedef void FKBenchFunc(int size, FrSymbol *root) ;

static FKBenchFunc *fkbench_funcs[] =
   {
     fkbench_0,  // overhead test
     fkbench_1, fkbench_2, fkbench_3, fkbench_4, fkbench_5, fkbench_6,
     fkbench_7, fkbench_8, fkbench_9, fkbench_10, fkbench_11, fkbench_12,
     fkbench_13
   } ;

static FKBenchFunc *fkbench_funcs_opt[] =
   {
     fkbench_0,  // overhead test
     fkbench_1_opt, fkbench_2_opt, fkbench_3_opt, fkbench_4_opt,
     fkbench_5_opt, fkbench_6_opt, fkbench_7_opt, fkbench_8_opt,
     fkbench_9_opt, fkbench_10_opt, fkbench_11_opt, fkbench_12_opt,
     fkbench_13_opt
   } ;

//----------------------------------------------------------------------

static void fkbench_main(istream &in, ostream &out, size_t, unsigned int,
			 FrList *)
{
   out << "Equivalents to FrameKit/LOOM Benchmarks\n"
	  "=======================================\n"
	  "\n"
	  ""
       << endl ;
   FrList *sizelist ;
   FrList *results = nullptr ;
   int repetitions ;
   out << "Enter a list of test sizes, such as (10 100 1000).  If you\n"
	  "enter the empty list (), the default (10 100 1000 5000) will\n"
	  "be used: " << flush ;
   FrObject *obj ;
   in >> obj ;
   if (obj && obj->consp())
      sizelist = (FrList*)obj ;
   else
      {
      free_object(obj) ;
      sizelist = new FrList(new FrInteger(10),new FrInteger(100),
			  new FrInteger(1000),new FrInteger(5000)) ;
      }
   out << "Repetitions (to increase runtime for more precision): "
       << flush ;
   in >> repetitions ;
   FKBenchFunc **funcs = fkbench_funcs ;
   int which = get_number(out,in,
			  "Use [1] literal translation of Lisp or "
			  "[2] optimized version? ",1,2) ;
   if (which == 2)
      funcs = fkbench_funcs_opt ;
   ostream *o = nullptr ;
   char filename[200] ;
   out << "File in which to store results (Enter for none): " << flush ;
   filename[0] = '\0' ;
   in.ignore(INT_MAX,'\n') ;
   in.getline(filename,sizeof(filename)) ;
   if (*filename)
      {
      o = new ofstream(filename) ;
      if (!o || !o->good())
	 o = nullptr ;
      }
   int decimals = (repetitions > 100) ? 3 : 2 ;
   active_timer = new FrTimer ;
   for (int testnum = 0 ; testnum < (int)lengthof(fkbench_funcs) ; testnum++)
      {
      cerr << "Test " << testnum << ", size " << flush ;
      for (FrList *size = sizelist ; size ; size = size->rest())
	 {
	 int siz = NUMBERP(size->first())
		     ? (int)(*(FrNumber*)size->first())
		     : 100 ;
         cerr << siz << ' ' << flush ;
	 // create/destroy a symbol table before starting the timing loop
	 // to ensure that there is no artifact due to extra time needed to
	 // get the necessary memory from the system on the first pass
	 // through the loop
	 FrSymbolTable *sym_t = new FrSymbolTable(0) ;
	 FrSymbolTable *old_sym_t = sym_t->select() ;
	 FrSymbol *root = gensym("G") ;
	 root->createFrame() ;
	 delete_all_frames() ;
	 old_sym_t->select() ;
	 delete sym_t ;
	 active_timer->start() ;
	 for (int i = 0 ; i < repetitions ; i++)
	    {
	    sym_t = new FrSymbolTable(0) ;
	    old_sym_t = sym_t->select() ;
	    root = gensym("G") ;
	    root->createFrame() ;
	    funcs[testnum](siz,root) ;
	    delete_all_frames() ;
	    old_sym_t->select() ;
	    delete sym_t ;
	    }
	 double elapsed = active_timer->stopsec() ;
	 elapsed /= repetitions ;
	 elapsed *= 1000.0 ;  // convert seconds into milliseconds
	 pushlist(new FrList(new FrInteger(testnum), new FrInteger(siz),
			   new FrFloat(elapsed)),
		  results) ;
	 }
      cerr << "... completed." << endl ;
      }
   delete active_timer ;
   active_timer = nullptr ;
   out << "Test Results:\n"
	  "Test#     Size      Time       Time/Iteration    Net Time/Iter"
       << endl ;
   if (o)
      (*o) << "Test Results:\n"
	      "Test#     Size      Time       Time/Iteration"
	      "    Net Time/Iter"
	   << endl ;
   results = listreverse(results) ;
   FrList *overheads = nullptr ;
   while (results)
      {
      FrList *result = (FrList*)poplist(results) ;
      FrNumber *test = (FrNumber*)result->first() ;
      FrNumber *size = (FrNumber*)result->second() ;
      FrNumber *time = (FrNumber*)result->third() ;
      if ((int)*test == 0)
	 pushlist(result,overheads) ;
      FrNumber *overhead = nullptr ;
      for (FrList *oh = overheads ; oh ; oh = oh->rest())
	 if (equal(((FrList*)oh->first())->second(),size))
	    {
	    overhead = (FrNumber*)((FrList*)oh->first())->third() ;
	    break ;
	    }
      out << setw(4) << test << ' ' << setw(8) << size << '\t'
	  << setw(10) << setprecision(decimals) << setiosflags(ios::fixed)
	  << (double)*time << " ms"
	  << setw(12) << setprecision(6) << setiosflags(ios::fixed)
	  << (double)*time/(double)*size << " ms  " ;
      double net_time ;
      if (overhead)
	 net_time = (double)*time - (double)*overhead ;
      else
	 net_time = (double)*time ;
      out << setw(12) << setprecision(6) << setiosflags(ios::fixed)
	  << net_time/(double)*size << " ms" << endl ;
      if (o)
	 {
	 (*o) << setw(4) << test << ' ' << setw(8) << size << '\t'
	      << setw(10) << setprecision(decimals) << setiosflags(ios::fixed)
	      << (double)*time << " ms"
	      << setw(12) << setprecision(6) << setiosflags(ios::fixed)
	      << (double)*time/(double)*size << " ms  " ;
	 (*o) << setw(12) << setprecision(6) << setiosflags(ios::fixed)
	      << net_time/(double)*size << " ms" << endl ;
	 }
      if ((int)*test != 0)
	 free_object(result) ;
      }
   free_object(overheads) ;
   out << "Note: tests 8-13 include the time for an instance of test 7"
       << endl ;
   if (o)
      {
      (*o) << "Descriptions:\n"
	    " 0. fixed overhead (looping, create/destroy symbol table, etc.)\n"
	    " 1. create N unrelated frames\n"
	    " 2. get facet from a frame\n"
	    " 3. set facet in a frame\n"
	    " 4. create unrelated instances of a frame\n"
	    " 5. get facet from instance frame\n"
	    " 6. set facet in an instance frame\n"
	    " 7. create a list of N frames, each IS-A the previous one\n"
	    " 8. get inherited filler from leaf of N-deep IS-A hierarchy\n"
	    " 9. set local value for inherited slot in leaf frame\n"
	    "10. set facet in root frame of N-deep IS-A hierarchy\n"
	    "11. create instance of IS-A hierarchy's leaf frame\n"
	    "12. get facet from instance of IS-A hierarchy's leaf frame\n"
            "13. set facet in instance of IS-A hierarchy's leaf frame\n"
          << endl << endl ;
      (*o) << "Notes:\n"
	      "all tests include the time to destroy created frames\n"
	      "tests 8-13 include the time for an instance of test 7"
	   << endl ;
      delete o ;
      }
   return ;
}
#undef create_frame_tree

//----------------------------------------------------------------------

static void benchmark_memalloc_random(int numblocks, int size,
				      unsigned int iterations, char **blocks)
{
   unsigned int pass ;
   int i ;

   cout << "\nAllocating " << numblocks << " * " << (size/4) << "-" << size
	<< " bytes, release in random order"
	<< flush ;
   start_test() ;
   for (pass = 0 ; pass < iterations ; pass++)
      {
      for (i = 0 ; i < numblocks ; i++)
	 blocks[i] = 0 ;
      for (i = 0 ; i < numblocks ; i++)
	 {
	 if (blocks[i/2])
	    FrFree(blocks[i/2]) ;
	 size_t sz = FrRandomNumber(3*size/4) + size/4 ;
	 blocks[i/2] = FrNewN(char,sz) ;
	 int j = FrRandomNumber(numblocks/2) ;
	 if (blocks[j])
	    {
	    FrFree(blocks[j]) ;	
	    blocks[j] = 0 ;
	    }
	 }
      for (i = numblocks-1 ; i >= 0 ; i--)
	 {
	 if (blocks[i])
	    FrFree(blocks[i]) ;
	 }
      }
   stop_test(iterations,numblocks,false) ;
   cout << "Shuffling" << flush ;
   start_test(false) ;
   for (pass = 0 ; pass < iterations ; pass++)
      {
      for (i = 0 ; i < numblocks ; i++)
	 blocks[i] = 0 ;
      for (i = 0 ; i < numblocks ; i++)
	 {
	 (void)(FrRandomNumber(3*size/4) + size/4) ; // for consistent timing
	 if (blocks[i/2])
	    blocks[i/2] = 0 ;
	 int j = FrRandomNumber(numblocks/2) ;
	 if (blocks[j])
	    blocks[j] = 0 ;
	 }
      for (i = numblocks-1 ; i >= 0 ; i--)
	 {
	 if (blocks[i])
	    blocks[i] = 0 ; ;
	 }
      }
   stop_test(iterations,numblocks,false) ;
   return ;
}

//----------------------------------------------------------------------

static void benchmark_memalloc_pass(ostream &out, size_t blocksize,
				    size_t size, size_t iterations)
{
   if (size < 5)
      size = 5 ;
   char **blocks = FrNewN(char*,size) ;
   if (!blocks)
      return ;
   size_t leak = leakage_per_iteration ;
   if (leak >= size)
      leak = size ;
   out << "\nAllocating " << size << " * " << blocksize
       << " bytes, release in order allocated"
       << flush ;
   start_test() ;
   for (size_t pass = 0 ; pass < iterations ; pass++)
      {
      for (size_t i = 0 ; i < size ; i++)
	 blocks[i] = FrNewN(char,blocksize) ;
      for (size_t i = 0 ; i < size-leak ; i++)
	 FrFree(blocks[i]) ;
      }
   stop_test(iterations,size,false) ;
   out << "\nAllocating " << size << " * " << blocksize
       << " bytes, release in opposite order"
       << flush ;
   start_test() ;
   for (size_t pass = 0 ; pass < iterations ; pass++)
      {
      for (size_t i = 0 ; i < size ; i++)
	 blocks[i] = FrNewN(char,blocksize) ;
      for (size_t i = size-leak ; i > 0 ; i--)
	 FrFree(blocks[i-1]) ;
      }
   stop_test(iterations,size,false) ;
   out << "\nAllocating " << size << " * " << blocksize
       << " bytes, release alternately"
       << flush ;
   start_test() ;
   for (size_t pass = 0 ; pass < iterations ; pass++)
      {
      for (size_t i = 0 ; i < size ; i++)
	 blocks[i] = FrNewN(char,blocksize) ;
      for (size_t i = 0 ; i < size-leak ; i += 2)
	 FrFree(blocks[i]) ;
      for (size_t i = 1 ; i < size-leak ; i += 2)
	 FrFree(blocks[i]) ;
      }
   stop_test(iterations,size,false) ;
   benchmark_memalloc_random(size,blocksize,iterations,blocks) ;
   FrFree(blocks) ;
#if 1
   if (size > 20)
      {
      out << "Reclaiming memory " ;
      start_test() ;
      void compact_FrMalloc() ;
//      compact_FrMalloc() ;
//      FrMemoryStats(out) ;
      FramepaC_gc() ;
      stop_test(1,0,false) ;
      }
#endif
   return ;
}

//----------------------------------------------------------------------

void benchmark_memalloc(istream &, ostream &out, size_t size,
			unsigned int iterations, FrList *)
{
   announce_memalloc(out,size,iterations) ;
   // test big_malloc allocations
   benchmark_memalloc_pass(out,4000000,(size+120)/160,10*iterations) ;
   benchmark_memalloc_pass(out,1000000,(size+30)/40,5*iterations) ;
   benchmark_memalloc_pass(out,100000,(size+3)/4,iterations) ;
   // test large slab allocations
   benchmark_memalloc_pass(out,20000,size,iterations) ;
   benchmark_memalloc_pass(out,2000,2*size,iterations) ;
   // test small (suballoc) allocations
   benchmark_memalloc_pass(out,200,10*size,iterations) ;
   benchmark_memalloc_pass(out,20,20*size,iterations) ;
   return ;
}

//----------------------------------------------------------------------

void benchmark_suballoc(istream &, ostream &out, size_t size,
			unsigned int iterations, FrList *)
{
   size_t i ;
   unsigned int pass ;

   announce_suballoc(out,size,iterations) ;
   FrLocalAlloc(void *,blocks,20000,size) ;
   FrAllocator allocator("SuballocTest",16) ; // use 16-byte objects
   start_test() ;
   for (pass = 0 ; pass < iterations ; pass++)
      {
      for (i = 0 ; i < size ; i++)
	 blocks[i] = allocator.allocate() ;
      for (i = 0 ; i < size ; i++)
	 allocator.release(blocks[i]) ;
      }
   FrLocalFree(blocks) ;
   stop_test(iterations,size,true) ;
   return ;
}

//----------------------------------------------------------------------

static BenchmarkFunc *benchmark_funcs[] =
   {
    0,
    benchmark_makesymbol,
    benchmark_creation,
    benchmark_relations,
    benchmark_inheritance,
    benchmark_output,
    benchmark_input,
    benchmark_vframe_mem,
    benchmark_vframe_disk,
    benchmark_vframe_server,
    fkbench_main,
    benchmark_memalloc,
    benchmark_suballoc
   } ;

void benchmarks_menu(ostream &out, istream &in)
{
   int choice ;
   int size ;
   unsigned int iterations ;
   FrList *frames ;

   do {
      choice = display_menu(out,in,true,12,
			    "Benchmarks:",
		"\t1. MakeSymbol loop        ""\t 7. Virtual Frames (memory)\n"
		"\t2. FrFrame creation/deletion""\t 8. Virtual Frames (disk)\n"
		"\t3. Relation management    ""\t 9. Virtual Frames (server)\n"
		"\t4. Inheritance            ""\t10. FrameKit/LOOM Benchmarks\n"
		"\t5. Output Speed           ""\t11. Memory Allocation Speed\n"
	        "\t6. Input Speed            ""\t12. Suballocator Speed\n"
			   ) ;
      frames = 0 ;
      if ((choice >= 1 && choice <= 6) || choice == 11 || choice == 12)
	 {
	 out << "Please enter test size and number of iterations, separated\n"
	     << "by a blank: " << flush ;
	 in >> size >> iterations ;
	 if (choice == 2 || choice == 3 || choice == 4)
	    {
	    out << "\nGENSYM'ing frame names..." << endl ;
	    // expand the symbol table to the needed size in one fell swoop
	    // to avoid later memory fragmentation
	    FrSymbolTable *symtab = FrSymbolTable::current() ;
	    symtab->expandTo(size+100) ;
	    for (int i = 0 ; i < size ; i++)
	       pushlist(gensym("TEST"),frames) ;
	    }
         }
      if (choice > 0 && choice < (int)lengthof(benchmark_funcs) &&
	  benchmark_funcs[choice])
	 (benchmark_funcs[choice])(in, out, size, iterations, frames) ;
      free_object(frames) ;
      } while (choice != 0) ;
   return ;
}

//----------------------------------------------------------------------

void leak_command(ostream &out, istream &in)
{
   out << "Number of memory blocks to leak on each iteration: " ;
   in >> leakage_per_iteration ;
   return ;
}

// end of file benchmrk.cpp //
