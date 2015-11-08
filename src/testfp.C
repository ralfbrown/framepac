/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File test.cpp	   Test/Demo program				*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,1998,1999,2000,2009,2010,2013,	*/
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

#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include "FramepaC.h"
#include "frserver.h"
#include "testnet.h"
#include "benchmrk.h"

#ifdef FrSTRICT_CPLUSPLUS
#  include <fstream>
#  include <iomanip>
#else
#  include <fstream.h>
#  include <iomanip.h>
#endif

#ifdef FrMOTIF
XtAppContext app_context ;
#endif /* FrMOTIF */

/************************************************************************/
/*	 Configuration Options						*/
/************************************************************************/

//#define NAIVE_PROG	// use "naive" coding instead of most efficient
			// in some of the benchmarks

/************************************************************************/

// increase stack size from the default of 4K
#ifdef __TURBOC__
unsigned int _stklen = 40960U ;
#endif

/************************************************************************/
/*    Type declarations							*/
/************************************************************************/

typedef void CommandFunc(ostream &out, istream &in) ;

struct CommandDef
   {
    const char *name ;
    CommandFunc *func ;
   } ;

/************************************************************************/
/*	 Forward declarations						*/
/************************************************************************/

void interpret_command(ostream &out, istream &in, bool allow_bench) ;

CommandFunc benchmarks_menu ;
CommandFunc leak_command ;
CommandFunc parmem_command ;
CommandFunc hash_command ;
CommandFunc ihash_command ;
static CommandFunc allslots_command ;
static CommandFunc allframes_command ;
static CommandFunc allsymbols_command ;
static CommandFunc checkmem_command ;
static CommandFunc complete_command ;
#ifdef FrSERVER
CommandFunc client_menu ;
CommandFunc server_menu ;
#endif
#ifdef FrDEMONS
static CommandFunc demons_command ;
#endif
static CommandFunc export_command ;
static CommandFunc export_native_command ;
static CommandFunc gc_command ;
static CommandFunc gensym_command ;
static CommandFunc import_command ;
static CommandFunc inherit_command ;
static CommandFunc inheritance_menu ;
static CommandFunc isap_command ;
static CommandFunc lock_command ;
static CommandFunc login_command ;
static CommandFunc mem_command ;
static CommandFunc memv_command ;
static CommandFunc memblocks_command ;
static CommandFunc newuser_command ;
static CommandFunc partof_command ;
static CommandFunc password_command ;
static CommandFunc relations_command ;
static CommandFunc rename_command ;
static CommandFunc revert_command ;
static CommandFunc rmuser_command ;
static CommandFunc symtab_menu ;
static CommandFunc transact_command ;

/************************************************************************/
/*    Global variables							*/
/************************************************************************/

static CommandDef commands[] =
   {
    { "ALL-SLOTS",   allslots_command },
    { "ALL-FRAMES",  allframes_command },
    { "ALL-SYMBOLS", allsymbols_command },
    { "BENCH",	     benchmarks_menu },
    { "CHECKMEM",    checkmem_command },
    { "COMPLETE",    complete_command },
#ifdef FrSERVER
    { "CLIENT",	     client_menu },
#endif
#ifdef FrDEMONS
    { "DEMONS",	     demons_command },
#endif
    { "EXPORT",	     export_command },
    { "EXPORT-NATIVE", export_native_command },
    { "GC",	     gc_command },
    { "GENSYM",	     gensym_command },
    { "HASH",	     hash_command },
    { "IHASH",	     ihash_command },
    { "IMPORT",	     import_command },
    { "INHERIT",     inherit_command },
    { "INHERITANCE", inheritance_menu },
    { "IS-A-P",	     isap_command },
    { "LEAK",	     leak_command },
    { "LOCK",	     lock_command },
    { "LOGIN",	     login_command },
    { "MEM",	     mem_command },
    { "MEMV",	     memv_command },
    { "MEMBLOCKS",   memblocks_command },
    { "NEWUSER",     newuser_command },
    { "PARMEM",	     parmem_command },
    { "PART-OF-P",   partof_command },
    { "PASSWD",	     password_command },
    { "RELATIONS",   relations_command },
    { "RENAME",	     rename_command },
    { "REVERT",	     revert_command },
    { "RMUSER",	     rmuser_command },
#ifdef FrSERVER
    { "SERVER",	     server_menu },
#endif
    { "SYMTAB",	     symtab_menu },
    { "TRANSACT",    transact_command },
    { 0,	     0 }	  // end-of-list marker
   } ;

//----------------------------------------------------------------------

static FrSignalHandlerFunc sigint_handler ;
FrSignalHandler sigint(SIGINT,sigint_handler) ;

/************************************************************************/
/************************************************************************/

static void sigint_handler(int)
{
   // attempt a graceful shutdown
   cout << "\nShutting down (signal)...." << endl ;
   FrShutdown() ;
   exit(1) ;
}

/************************************************************************/
/************************************************************************/

static FrSymbol *get_symbol(ostream &out, istream &in, const char *prompt)
{
   FrObject *obj ;

   for (;;)
      {
      FramepaC_bgproc() ;		// handle any asynchronous operations
      out << prompt << ' ' << flush ;
      in >> obj ;
      if (obj && obj->symbolp())
	 break ;
      else
	 {
	 free_object(obj) ;
	 out << "\nYou must enter a symbol!  Try again." << endl ;
	 }
      }
   FramepaC_bgproc() ;		// handle any asynchronous operations
   return (FrSymbol *)obj ;
}

//----------------------------------------------------------------------

int get_number(ostream &out, istream &in, const char *prompt,int min,int max)
{
   FrObject *obj ;
   long num = 0 ;

   for (;;)
      {
      FramepaC_bgproc() ;		// handle any asynchronous operations
      out << prompt << ' ' << flush ;
      in >> obj ;
      if (obj && obj->numberp())
	 {
	 num = obj->intValue() ;
	 if (num >= min && num <= max)
	    break ;
	 else
	    out << "\nYou must enter a number between " << min << " and "
	        << max << "!  Try again." << endl ;
	 }
      else
	 out << "\nYou must enter a number!  Try again." << endl ;
      free_object(obj) ;
      obj = 0 ;
      }
   free_object(obj) ;
   FramepaC_bgproc() ;		// handle any asynchronous operations
   return (int)num ;
}

//----------------------------------------------------------------------

int display_menu(ostream &out, istream &in, bool allow_exit,
		 int maxchoice, const char *title, const char *menu)
{
   FrObject *choice = 0 ;
   long selection ;
   int iter = 0 ;

   for (;;)
      {
      FramepaC_bgproc() ;		// handle any asynchronous operations
      if ((iter++ % 3) == 0)
	 {
	 if (title)
	    out << title << endl ;
	 out << menu << flush ;
	 if (allow_exit)
	 out << "\t0. Exit" << endl ;
         }
      FramepaC_bgproc() ;		// handle any asynchronous operations
      out << "Choice: " << flush ;
      in >> choice ;
      FramepaC_bgproc() ;		// handle any asynchronous operations
      if (choice && choice->numberp())
         {
         selection = choice->intValue() ;
         if (selection >= 1 && selection <= maxchoice)
	    break ;
	 else if (allow_exit && selection == 0)
	    {
	    selection = 0 ;
	    break ;
	    }
   	 else
	    out << "That is not an available choice!  Try again." << endl ;
	 }
      else
	 out << "You must enter a number!  Try again." << endl ;
      if (choice) choice->freeObject() ;
      FramepaC_bgproc() ;		// handle any asynchronous operations
      }
   if (choice) choice->freeObject() ;
   return (int)selection ;
}

//----------------------------------------------------------------------

bool prefix_match(FrObject *obj, FrObject */*info*/, va_list args)
{
   FrVarArg(const char*,prefix) ;
   FrVarArg(FrList **,matches) ;
   if (obj && obj->symbolp())
      {
      if (obj->printableName() && memcmp(obj->printableName(),prefix,strlen(prefix)) == 0)
	 {
	 pushlist(obj,*matches) ;
	 }
      }
   return true ;
}

/************************************************************************/
/************************************************************************/

void display_object_info(ostream &out, const FrObject *obj)
{
   FramepaC_bgproc() ;		// handle any asynchronous operations
   FramepaC_bgproc() ;
   size_t objlen = FrObject_string_length(obj) ;
   out << "\n\nYou entered " ;
   if (objlen > 68)
      out << endl ;
   out << obj << endl ;
   if (obj)
      out << "That object is of type " << obj->objTypeName()
          << "; it takes " << objlen << " bytes to print" << endl ;
   else
      return ;
   if (obj->framep())
      {
      FrList *framekit = FramepaC_to_FrameKit((FrFrame *)obj) ;

      out << "As a FrameKit frame, it would be:\n" << framekit << endl ;
      free_object(framekit) ;
      }
   else if (obj->arrayp())
      {
      if (obj->length() > 0)
	 out << "This "
	     << ((obj->objType() == OT_FrSparseArray) ? "sparse " : "")
	     << "array's last element is "
	     << (*(FrArray*)obj)[obj->length()-1] << endl ;
      }
   else if (obj->queuep())
      {
      out << "This queue contains " << ((FrQueue*)obj)->queueLength()
	  << " items" << endl ;
      }
   else if (obj->hashp())
      {
      FrList *matches = 0 ;
      ((FrObjHashTable*)obj)->iterate(prefix_match,"A",&matches) ;
      out << "This hash table contains " << matches->listlength()
	  << " items matching the prefix 'A'" << endl ;
      free_object(matches) ;
      }
   else if (obj->stringp())
      out << "This string uses " << ((FrString*)obj)->charWidth()
	  << "-byte characters." << endl ;
   else if (obj->consp())
      {
      out << "Its length is " << obj->length() << " cons cells.\n"
	  << "Its head is " << obj->car() << " and its tail is "
	  << obj->cdr() << endl << endl ;
      FrObject *obj_car = obj->car() ;
      if (obj_car && obj_car->symbolp() &&
	  (obj_car == findSymbol("MAKE-FRAME") ||
	   obj_car == findSymbol("MAKE-FRAME-OLD")))
	 {
	 FrFrame *frame = FrameKit_to_FramepaC((FrList *)obj) ;
	 out << "It is a FrameKit frame; as a FramepaC frame, it would be:\n"
	     << frame << endl ;
	 free_object(frame) ;
	 }
      }
   else if (obj->vectorp())
      {
      out << "This vector contains " << obj->length() << " bits, of which "
	  << ((FrBitVector*)obj)->countBits() << " are set." << endl ;
      }
   FramepaC_bgproc() ;		// handle any asynchronous operations
   return ;
}

//----------------------------------------------------------------------

static int compare_symbol_names(const FrObject *obj1, const FrObject *obj2)
{
   if (obj1 && obj2 && obj1->symbolp() && obj2->symbolp())
      return strcmp(((FrSymbol *)obj1)->symbolName(),
		    ((FrSymbol *)obj2)->symbolName()) ;
   else
      return 0 ;
}

/************************************************************************/
/************************************************************************/

//----------------------------------------------------------------------

#ifdef FrDEMONS
bool monitor_create(const FrSymbol *frame, const FrSymbol *slot,
		    const FrSymbol *facet, const FrObject *value, va_list args)
{
   (void)args ; (void)value ;
   cout << "Created " << frame << '.' << slot << '.' << facet << endl ;
   return false ;
}

bool monitor_add(const FrSymbol *frame, const FrSymbol *slot,
		    const FrSymbol *facet, const FrObject *value, va_list args)
{
   (void)args ;
   cout << "Added " << value << " to " << frame << '.' << slot << '.' << facet << endl ;
   return false ;
}

bool monitor_get(const FrSymbol *frame, const FrSymbol *slot,
		    const FrSymbol *facet, const FrObject *value, va_list args)
{
   (void)args ; (void)value ;
   cout << "Got " << frame << '.' << slot << '.' << facet << endl ;
   return false ;
}

bool monitor_inherit(const FrSymbol *frame, const FrSymbol *slot,
		    const FrSymbol *facet, const FrObject *value, va_list args)
{
   (void)args ; (void)value ;
   cout << "Must inherit " << frame << '.' << slot << '.' << facet << endl ;
   return false ;
}

bool monitor_delete(const FrSymbol *frame, const FrSymbol *slot,
		    const FrSymbol *facet, const FrObject *value, va_list args)
{
   (void)args ; (void)value ;
   cout << "Deleted " << frame << '.' << slot << '.' << facet << endl ;
   return false ;
}
#endif /* FrDEMONS */

/************************************************************************/
/************************************************************************/

static void inheritance_menu(ostream &out, istream &in)
{
   int choice ;
   static FrInheritanceType types[] =
	        { NoInherit, InheritSimple,
		  InheritDFS, InheritBFS,
		  InheritLocalDFS,
		} ;
   static const char *type_name[] = { "None", "Simple", "DFS", "BFS",
				      "Local-DFS"
   				    } ;

   out << "Select inheritance type (currently "
       << type_name[get_inheritance_type()] << "):" << endl ;
   choice = display_menu(out,in,false,lengthof(types),
			 0,
			 "\t1. None\n"
			 "\t2. Simple IS-A lookup\n"
			 "\t3. Full Depth-First Search\n"
			 "\t4. Breadth-First Search\n"
			 "\t5. Per-slot inheritance, then full DFS\n"
			 ) ;
   set_inheritance_type(types[choice-1]) ;
   return ;
}

/************************************************************************/
/************************************************************************/

static FrSymbolTable *symtab_select(istream &in,const FrList *symtabs)
{
   FrSymbol *sym = get_symbol(cout,in,"FrSymbol table's name:") ;
   FrCons *assoc = listassoc(symtabs,sym) ;

   if (assoc)
      return ((FrSymbolTable *)assoc->cdr())->select() ;
   else
      {
      cout << "\nYou have not defined a symbol table called " << sym
           << "!" << endl ;
      }
   return FrSymbolTable::current() ;
}

//----------------------------------------------------------------------

static FrCons *symtab_create(istream &in,const FrList *symtabs)
{
   FrSymbol *name = get_symbol(cout,in,"New symbol table's name:") ;
   FrSymbolTable *old_symtab = FrSymbolTable::selectDefault() ;
   FrCons *result = 0 ;

   if (!listassoc(symtabs,name))
      {
      int size = get_number(cout,in,"Initial size:",0,INT_MAX) ;
      result = new FrCons(name,(FrObject*)new FrSymbolTable(size)) ;
      }
   else
      cout << "\nThat symbol table has already been defined." << endl ;
   old_symtab->select() ;
   return result ;
}

//----------------------------------------------------------------------

static FrList *symtab_delete(istream &in,const FrList *symtabs)
{
   FrSymbol *sym = get_symbol(cout,in,"FrSymbol table's name:") ;
   FrSymbolTable *old_symtab = FrSymbolTable::selectDefault() ;
   FrCons *assoc = listassoc(symtabs,sym,equal) ;

   if (assoc)
      {
      old_symtab->select() ;
      destroy_symbol_table((FrSymbolTable *)assoc->cdr()) ;
      return listremove(symtabs,assoc,equal) ;
      }
   else
      {
      cout << "\nYou have not defined a symbol table by that name!" << endl ;
      old_symtab->select() ;
      return (FrList *)symtabs ;
      }
}

//----------------------------------------------------------------------

static void symtab_menu(ostream &out, istream &in)
{
   int choice ;
   static FrList *symtabs = 0 ;
   FrCons *newtab ;
   FrSymbolTable *old_symtab = FrSymbolTable::current() ;

   do {
      FrSymbolTable::selectDefault() ;  // read into default symbol table
      choice = display_menu(out,in,true,5,
			    "FrSymbol Table Options:",
			    "\t1. Select default symbol table\n"
			    "\t2. Create a new symbol table\n"
			    "\t3. FrList symbol tables\n"
			    "\t4. Select symbol table by name\n"
			    "\t5. Destroy a symbol table\n"
			    ) ;
      old_symtab->select() ;	// restore symbol table after reading input
      switch (choice)
	 {
	 case 0:
	    // do nothing
	    break ;
	 case 1:
	    FrSymbolTable::selectDefault() ;
	    break ;
	 case 2:
	    newtab = symtab_create(in,symtabs) ;
	    if (newtab)
	       pushlist(newtab,symtabs) ;
	    break ;
	 case 3:
	    out << "\nCurrent symbol tables: (default)" ;
	    FrList *sym ;
	    for (sym = symtabs ; sym ; sym = sym->rest())
	       out << ", " << sym->car()->car() ;
	    out << endl << endl ;
	    break ;
	 case 4:
	    old_symtab = symtab_select(in,symtabs) ;
	    break ;
	 case 5:
	    symtabs = symtab_delete(in,symtabs) ;
	    break ;
	 default:
	    FrMissedCase("symtab_menu") ;
	    break ;
	 }
      } while (choice != 0) ;
   old_symtab->select() ;
   return ;
}

/************************************************************************/
/************************************************************************/

static void dump_frame_names(FrFrame *frame, va_list args)
{
   (void)args ; // circumvent compiler warning
   cout << *(const FrObject*)frame->frameName() << ' ' ;
   return ;
}

//----------------------------------------------------------------------

static void show_slot_with_inheritance(FrSymbol *frname, const FrSymbol *slot)
{
   if (frname->symbolFrame())
      cout << "\nFrame: " << frname << "  Slot: " << slot
	   << "\n\tValue: " << frname->getValues(slot,true)
	   << "\n\tSem: " << frname->getSem(slot,true)
	   << "\n\tDefault: "
	     << frname->getFillers(slot,FrSymbolTable::add("DEFAULT"),true)
	     << endl ;
   else
      cout << "No such frame" << endl ;
   return ;
}

//----------------------------------------------------------------------

static void allslots_command(ostream &out, istream &in)
{
   FrObject *fr ;

   in >> fr ;
   if (fr && fr->symbolp())
      {
      FrFrame *frame = find_vframe((FrSymbol *)fr) ;

      if (frame)
	 {
	 FrList *slots, *s ;
	
	 slots = frame->collectSlots(get_inheritance_type()) ;
	 out << "The slots and facets which may be inherited by " << fr
	     << " are:" << endl ;
	 for (s = slots ; s ; s = s->rest())
	    out << "   " << s->first()->car() << ": " << s->first()->cdr()
		<< endl ;
	 free_object(slots) ;
	 }
      else
	 out << fr << " is not a frame!" << endl ;
      }
   else
      out << "Usage: ALL-SLOTS <frame>" << endl ;
   free_object(fr) ;
   return ;
}

//----------------------------------------------------------------------

static void allframes_command(ostream &out, istream &)
{
   out << "Current frames: " ;
   doAllFrames(dump_frame_names) ;
   out << endl ;
   return ;
}

//----------------------------------------------------------------------

static void allsymbols_command(ostream &out, istream &)
{
   FrList *allsyms = current_symbol_table()->allKeys() ;
   out << "The symbols in the current symbol table are:" << endl
       << allsyms << endl ;
   free_object(allsyms) ;
   return ;
}

//----------------------------------------------------------------------

static void isap_command(ostream &out, istream &in)
{
   FrObject *fr1, *fr2 ;

   in >> fr1 >> fr2 ;
   if (fr1 && fr2 && fr1->symbolp() && fr2->symbolp())
      {
      out << "isA_p(" << fr1 << "," << fr2 << ") = " ;
      if (is_a_p(find_vframe((FrSymbol*)fr1),find_vframe((FrSymbol*)fr2)))
	 out << "true" ;
      else
	 out << "false" ;
      }
   else
      out << "Usage: IS-A-P <frame1> <frame2>\t\t (both frame names)" << endl;
   free_object(fr1) ;
   free_object(fr2) ;
   return ;
}

//----------------------------------------------------------------------

static void partof_command(ostream &out, istream &in)
{
   FrObject *fr1, *fr2 ;

   in >> fr1 >> fr2 ;
   if (fr1 && fr2 && fr1->symbolp() && fr2->symbolp())
      {
      out << "partOf_p(" << fr1 << "," << fr2 << ") = " ;
      if (((FrSymbol*)fr1)->partOf_p((FrSymbol*)fr2))
	 out << "true" ;
      else
	 out << "false" ;
      }
   else
      out << "Usage: PART-OF-P <frame1> <frame2> \t\t (both frame names)"
          << endl ;
   free_object(fr1) ;
   free_object(fr2) ;
   return ;
}

//----------------------------------------------------------------------

static void inherit_command(ostream &out, istream &in)
{
   FrObject *frname, *slot ;

   in >> frname >> slot ;
   if (frname && frname->symbolp() && slot && slot->symbolp())
      show_slot_with_inheritance((FrSymbol*)frname,(FrSymbol*)slot) ;
   else
      out << "Usage: INHERIT <frame> <slot> \t\t (both symbols)" << endl ;
   return ;
}

//----------------------------------------------------------------------

#ifdef FrDEMONS
static void demons_command(ostream &, istream &)
{
   int choice ;
   static bool DemonsInstalled = false ;
   FrSymbol *symbolISA = FrSymbolTable::add("IS-A") ;

   do {
      choice = display_menu(cout,cin,true,2,
			    "Demons",
			    "\t1. Install monitoring demons\n"
			    "\t2. Remove monitoring demons\n"
			    ) ;
      switch (choice)
	 {
	 case 0:
	    // do nothing
	    break ;
	 case 1:
	    if (!DemonsInstalled)
	       {
	       add_demon(symbolISA,DT_IfCreated,monitor_create,0) ;
	       add_demon(symbolISA,DT_IfAdded,monitor_add,0) ;
	       add_demon(symbolISA,DT_IfRetrieved,monitor_get,0) ;
	       add_demon(symbolISA,DT_IfMissing,monitor_inherit,0) ;
	       add_demon(symbolISA,DT_IfDeleted,monitor_delete,0) ;
	       cout << "Demons installed" << endl ;
	       DemonsInstalled = true ;
	       }
	    break ;
	 case 2:
	    if (DemonsInstalled)
	       {
	       remove_demon(symbolISA,DT_IfCreated,monitor_create) ;
	       remove_demon(symbolISA,DT_IfAdded,monitor_add) ;
	       remove_demon(symbolISA,DT_IfRetrieved,monitor_get) ;
	       remove_demon(symbolISA,DT_IfMissing,monitor_inherit) ;
	       remove_demon(symbolISA,DT_IfDeleted,monitor_delete) ;
	       cout << "Demons removed" << endl ;
	       DemonsInstalled = false ;
	       }
	    break ;
	 default:
	    FrMissedCase("demons_menu") ;
	    break ;
	 }
      } while (choice != 0) ;
   return ;
}
#endif /* FrDEMONS */

//----------------------------------------------------------------------

static void gc_command(ostream &, istream &)
{
   FramepaC_gc() ;
   return ;
}

//----------------------------------------------------------------------

static void gensym_command(ostream &out,istream &)
{
   out << "Generated symbol: " << gensym((const char *)0) << endl ;
   return ;
}

//----------------------------------------------------------------------

static void transact_command(ostream &out, istream &in)
{
   int choice ;
   int transaction ;
   int result ;

   do {
      choice = display_menu(out,in,true,3,
			    "Transactions",
			    "\t1. Start a transaction\n"
			    "\t2. End a transaction\n"
			    "\t3. Abort a transaction\n"
			    ) ;
      switch (choice)
	 {
	 case 0:
	    // do nothing
	    break ;
	 case 1:
	    transaction = start_transaction() ;
	    if (transaction == -1)
	       out << "Unable to start transaction" << endl ;
	    else
	       out << "Started transaction number " << transaction << endl ;
 	    break ;
	 case 2:
	    transaction = get_number(out,in,"Transaction number:",-1,
				     SHRT_MAX) ;
	    result = end_transaction(transaction) ;
	    if (result == -1)
	       out << "Failed while ending transaction, error code = "
		   << Fr_errno << endl ;
	    else
	       out << "Transaction ended successfully" << endl ;
 	    break ;
	 case 3:
	    transaction = get_number(out,in,"Transaction number:",-1,
				     SHRT_MAX) ;
	    result = abort_transaction(transaction) ;
	    if (result == -1)
	       {
	       out << "Failed while aborting transaction, error code = "
		   << Fr_errno << endl ;
	       }
	    else
	       out << "Transaction successfully rolled back" << endl ;
 	    break ;
	 default:
	    FrMissedCase("transact_command") ;
	    break ;
	 }
      } while (choice != 0) ;
   return ;
}

//----------------------------------------------------------------------

static void lock_command(ostream &out, istream &in)
{
   int choice ;
   FrSymbol *name ;

   do {
      choice = display_menu(out,in,true,2,
			    "FrFrame Locking",
			    "\t1. Lock FrFrame\n"
			    "\t2. Unlock FrFrame\n"
			   ) ;
      if (choice != 0)
         name = get_symbol(out,in,"FrFrame name:") ;
      else
         name = 0 ;   // avoid "uninit var" warning
      switch (choice)
         {
	 case 0:
	    // do nothing
	    break ;
	 case 1:
	    if (name->isLocked())
	       out << "The frame was already locked." << endl ;
            else
               {
	       name->lockFrame() ;
	       out << "The frame was not yet locked, and is " ;
	       if (name->isLocked())
		  out << "now locked." << endl ;
	       else
		  out << "STILL NOT locked." << endl ;
	       }
	    break ;
	 case 2:
	    if (!frame_locked(name))
	       out << "The frame was not locked." << endl ;
	    else
	       {
	       unlock_frame(name) ;
	       out << "The frame was locked, and is " ;
	       if (frame_locked(name))
		   out << "STILL locked." << endl ;
	       else
		   out << "now unlocked." << endl ;
	       }
 	    break ;
	 default:
	    FrMissedCase("lock_command") ;
	    break ;
	 }
      } while (choice != 0) ;
   return ;
}

//----------------------------------------------------------------------

static void import_command(ostream &out, istream &in)
{
   char filename[128] ;
   ifstream infile ;

   out << "File from which to import FrameKit frames: " << flush ;
   in >> filename ;
   infile.open(filename) ;
   if (infile.good())
      import_FrameKit_frames(infile,cout) ;
   else
      out << "Error opening file." << endl ;
   infile.close() ;
   return ;
}

//----------------------------------------------------------------------

static void export_command(ostream &out, istream &in)
{
   char filename[128] ;
   ofstream outfile ;

   out << "File into which to export all frames: " << flush ;
   in >> filename ;
   outfile.open(filename) ;
   if (outfile.good())
      {
      FrList *all_frames = collect_prefix_matching_frames("",0,0) ;
      if (all_frames)
	 all_frames = all_frames->sort(compare_symbol_names) ;
      export_FrameKit_frames(outfile,all_frames) ;
      free_object(all_frames) ;
      }
   else
      out << "Error opening file." << endl ;
   outfile.close() ;
   return ;
}

//----------------------------------------------------------------------

static void export_native_command(ostream &out, istream &in)
{
   char filename[128] ;
   ofstream outfile ;

   out << "File into which to export all frames: " << flush ;
   in >> filename ;
   outfile.open(filename) ;
   if (outfile.good())
      {
      FrList *all_frames = collect_prefix_matching_frames("",0,0) ;
      export_FramepaC_frames(outfile,all_frames) ;
      free_object(all_frames) ;
      }
   else
      out << "Error opening file." << endl ;
   outfile.close() ;
   return ;
}

//----------------------------------------------------------------------

static void rename_command(ostream &out, istream &in)
{
   FrObject *oldname ;
   FrObject *newname ;
   out << "FrFrame to be renamed: " << flush ;
   in >> oldname ;
   out << "New name for frame: " << flush ;
   in >> newname ;
   if (oldname && oldname->symbolp() && newname && newname->symbolp())
      {
      FrFrame *oldframe = ((FrSymbol*)oldname)->findFrame() ;
      if (!oldframe)
	 out << "The specified frame does not exist!" << endl ;
      else if (oldframe->renameFrame((FrSymbol*)newname))
	 out << "Rename was successful." << endl ;
      else
	 out << "Rename failed!" << endl ;
      }
   else
      out << "Usage: * RENAME <oldname> <newname>  (both symbols)" << endl ;
   return ;
}

//----------------------------------------------------------------------

static void revert_command(ostream &out, istream &in)
{
   FrObject *frame ;
   int gen ;

   out << "FrFrame to be reverted: " << flush ;
   in >> frame ;
   out << "\nNumber of versions back: " << flush ;
   in >> gen ;
   out << endl ;
   if (frame && frame->symbolp() && gen >= 0)
      {
      FrFrame *fr = ((FrSymbol*)frame)->oldFrame(gen) ;
      if (fr)
	 {
	 out << "The version of " << frame << " " << gen << " versions" << endl
	     << "prior to the current version is:" << endl
	     << fr << endl << endl ;
	 }
      else
	 out << "The requested version of the frame could not be loaded."
	     << endl ;
      }
   else
      out << "Usage: * REVERT <frame> <generations>" << endl ;
   return ;
}

//----------------------------------------------------------------------

static void complete_command(ostream &out, istream &in)
{
   FrObject *prefixsym ;

   out << "Find all frames with prefix: " << flush ;
   in >> prefixsym ;
   if (!prefixsym || !prefixsym->symbolp())
      {
      out << "You must enter a symbol for the prefix.  To match with" << endl
	  << "lowercase, use vertical bars like |this|." << endl ;
      return ;
      }
   const char *prefix = ((FrSymbol*)prefixsym)->symbolName() ;
   char longest_prefix[FrMAX_SYMBOLNAME_LEN] ;
   FrList *matches = collect_prefix_matching_frames(prefix,longest_prefix,
	                                          sizeof(longest_prefix)) ;
   matches = listsort(matches,compare_symbol_names) ;
   out << "The matching frames are " << matches << endl ;
   out << "The longest common prefix is " << longest_prefix << endl ;
   free_object(matches) ;
   return ;
}

//----------------------------------------------------------------------

static void relations_command(ostream &out, istream &)
{
   FrList *relations = current_symbol_table()->listRelations() ;
   out << "The defined relations are:" << endl
       << relations << endl ;
   free_object(relations) ;
   return ;
}

//----------------------------------------------------------------------

static void login_command(ostream &out, istream &in)
{
   char username[200] ;
   char password[200] ;

   out << "User name: " << flush ;
   in.ignore(1000,'\n') ;
   in.getline(username,sizeof(username)) ;
   out << "Password: " << flush ;
   in.getline(password,sizeof(password)) ;
   if (login_user(username,password))
      {
      cout << "Successfully logged in, at access level "
	    << get_access_level() << endl ;
      }
   else if (Fr_errno == ME_PASSWORD)
      cout << "\nUnable to login -- bad password" << endl ;
   else if (Fr_errno == ME_NOTFOUND)
      cout << "\nUnable to login -- unknown user name" << endl ;
   else
      cout << "\nUnable to login (reason unknown)." << endl ;
   return ;
}

//----------------------------------------------------------------------

static void password_command(ostream &out, istream &in)
{
   char username[200] ;
   char oldpwd[200] ;
   char newpwd[200] ;

   out << "User name: " << flush ;
   in.ignore(1000,'\n') ;
   in.getline(username,sizeof(username)) ;
   out << "Old password: " << flush ;
   in.getline(oldpwd,sizeof(oldpwd)) ;
   out << "New password: " << flush ;
   in.getline(newpwd,sizeof(newpwd)) ;
   FrStruct *userinfo = retrieve_userinfo(username) ;
   if (!userinfo)
      {
      cout << "Unknown username!" ;
      return ;
      }
   if (set_user_password(oldpwd,newpwd,userinfo))
      {
      cout << "Successfully set password." << endl ;
      if (!store_userinfo())
	 cout << "....but error updating user information file!" << endl ;
      }
   else
      cout << "Password update failed!" << endl ;
   return ;
}

//----------------------------------------------------------------------

static void newuser_command(ostream &out, istream &in)
{
   if (get_access_level() < ADMIN_ACCESS_LEVEL)
      {
      cout << "You must be logged in with administrator privileges\n"
	      "(access level " << ADMIN_ACCESS_LEVEL
	    << " or higher) to add users." << endl ;
      return ;
      }
   char username[200] ;
   char password[200] ;
   FrObject *ulevel ;
   int level ;

   out << "New user's name: " << flush ;
   in.ignore(1000,'\n') ;
   in.getline(username,sizeof(username)) ;
   out << "New password: " << flush ;
   in.getline(password,sizeof(password)) ;
   out << "User's access level: " << flush ;
   in >> ulevel ;
   if (ulevel && ulevel->numberp())
      {
      level = ulevel->intValue() ;
      if (level < GUEST_ACCESS_LEVEL)
	 level = GUEST_ACCESS_LEVEL ;
      else if (level > ROOT_ACCESS_LEVEL)
	 level = ROOT_ACCESS_LEVEL ;
      }
   else
      level = GUEST_ACCESS_LEVEL ;
   FrStruct *userinfo = retrieve_userinfo(username) ;
   if (userinfo)
      {
      cout << "That user is already registered!" ;
      return ;
      }
   userinfo = make_userinfo(username,password,level) ;
   if (update_user(userinfo))
      cout << "User information file updated." << endl ;
   else
      cout << "User information file update failed!" << endl ;
   return ;
}

//----------------------------------------------------------------------

static void rmuser_command(ostream &out, istream &in)
{
   if (get_access_level() < ADMIN_ACCESS_LEVEL)
      {
      cout << "You must be logged in with administrator privileges\n"
	      "(access level " << ADMIN_ACCESS_LEVEL
	    << " or higher) to add users." << endl ;
      return ;
      }
   char username[200] ;

   out << "Name of user to remove: " << flush ;
   in.ignore(1000,'\n') ;
   in.getline(username,sizeof(username)) ;
   if (!retrieve_userinfo(username))
      {
      cout << "No such user!" ;
      return ;
      }
   if (remove_user(username))
      cout << "User information file updated." << endl ;
   else
      cout << "User information file update failed!" << endl ;
   return ;
}

//----------------------------------------------------------------------

static void mem_command(ostream &out, istream &)
{
   FrMemoryStats(out) ;
   return ;
}

//----------------------------------------------------------------------

static void memv_command(ostream &out, istream &)
{
   FrMemoryStats(out,true) ;
   return ;
}

//----------------------------------------------------------------------

static void memblocks_command(ostream &out, istream &)
{
   FrShowMemory(out) ;
   return ;
}

//----------------------------------------------------------------------

static void checkmem_command(ostream &out, istream &)
{
   bool ok = (FramepaC_memory_chain_OK() &&
	      check_FrMalloc(&FramepaC_mempool)) ;
   out << "The memory chain is " << (ok ? "Ok" : "corrupted") << endl ;
   return ;
}

//----------------------------------------------------------------------

static void list_commands(ostream &out, bool allow_bench)
{
   size_t column = 23 ;
   out << "\nThe valid commands are " ;
   for (CommandDef *def = commands ; def->name ; def++)
      {
      if (strcmp(def->name,"BENCH") == 0 && !allow_bench)
	 continue ;
      column += strlen(def->name) + 1 ;
      if (column > 78)
	 {
	 out << endl ;
	 column = strlen(def->name) + 1 ;
	 }
      out << def->name << ' ' ;
      }
   out << endl << endl ;
   return ;
}

//----------------------------------------------------------------------

void interpret_command(ostream &out, istream &in, bool allow_bench)
{
   FrObject *cmd ;

   in >> cmd ;
   if (cmd && cmd->symbolp())
      {
      FrSymbol *cmdsym = (FrSymbol*)cmd ;
      FramepaC_bgproc() ;		// handle any asynchronous operations
      bool found = false ;
      for (CommandDef *def = commands ; def->name ; def++)
	 {
	 if (*cmdsym == def->name)
	    {
	    if (!allow_bench && strcmp(def->name,"BENCH") == 0)
	       break ;
	    found = true ;
	    def->func(out,in) ;
	    break ;
	    }
	 }
      if (!found)
	 {
	 if (cmdsym == findSymbol("BENCH") && allow_bench)
	    benchmarks_menu(out,in) ;
	 else if (cmdsym == findSymbol("?"))
	    list_commands(out,allow_bench) ;
	 else
	    {
	    out << cmd << " is an unknown command." << endl ;
	    list_commands(out,allow_bench) ;
	    }
	 }
      }
   else
      {
      out << cmd << " is not a valid command, because it is not a symbol."
	  << endl ;
      if (cmd) cmd->freeObject() ;
      FramepaC_bgproc() ;		// handle any asynchronous operations
      }
   return ;
}

/************************************************************************/
/*	The Main Program						*/
/************************************************************************/

int
#ifndef __WATCOMC__
   __FrCDECL
#endif
   main(int argc, char **argv)
{
#ifdef __WATCOMC__
   FrDestroyWindow() ;			// for Watcom, run as raw console app
#endif /* __WATCOMC__ */
   if (argc > 1)
      cout << argv[0] << " does not require any arguments." << endl << endl ;
   cout << "\tFramepaC " FramepaC_Version_string " Test Program" << endl
        << "\t==========================" << endl
        << "Use \"* ?\" to list the available commands." << endl ;
   // deliberately start with a tiny symbol table, to force table expansion as
   // we use the test program (in fact, the first expansion will happen during
   // initialization)
#if 0
//#ifdef FrMOTIF
   Widget main_window = XtVaAppInitialize( &app_context,
					   "Framepac Test Program",
					   NULL, 0,
					   &argc, argv,
					   NULL, NULL);
   FrInitializeMotif("FramepaC Messages",main_window,16) ;
#else
   initialize_FramepaC(16) ;
#endif /* FrMOTIF */
   FramepaC_set_userinfo_dir(0) ;  // set default location
   FrObject *obj = 0 ;
   FrSymbol *symbolEOF = FrSymbolTable::add("*EOF*") ;
   do {
      if (obj)
	 {
	 obj->freeObject() ;  // free the object from prev pass thru the loop
	 obj = 0 ;
	 }
      FramepaC_bgproc() ;		// handle any asynchronous operations
      cout << "\nEnter a FrObject, NIL to end: " ;
      cin >> obj ;
      FramepaC_bgproc() ;		// handle any asynchronous operations
      if (obj && obj->symbolp() && obj == FrSymbolTable::add("*"))
	 interpret_command(cout,cin,true) ;
      else
	 display_object_info(cout,obj) ;
      FramepaC_bgproc() ;		// handle any asynchronous operations
      } while (!NIL_symbol(obj) && obj != symbolEOF) ;
   free_object(obj) ;
   // force the inclusion of some types we don't explicitly use in the code
   if (symbolEOF == 0)
      {
      FrInteger64 dummy_int64 ;
      FrStack dummy_stack ;
      FrSparseArray dummy_sparse_array ;
      if ((size_t)&dummy_int64 == (size_t)&dummy_stack ||
	  (size_t)&dummy_stack == (size_t)&dummy_sparse_array)
	 FrProgError("local variables with same address!") ;
      }
#if 0
//#ifdef FrMOTIF
   FrShutdownMotif() ;
#else
   FrShutdown() ;
#endif
   return 0 ;
}

// end of file test.cpp //
