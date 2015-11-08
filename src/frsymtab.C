/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frsymtab.cpp	       class FrSymbolTable			*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,1998,1999,2000,2001,2002,2003,	*/
/*		2004,2005,2006,2009,2010,2011,2013,2015			*/
/*	   Ralf Brown/Carnegie Mellon University	*/
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

#if defined(__GNUC__)
#  pragma implementation "frsymtab.h"
#endif

#include <stdlib.h>
#include <string.h>
#include "frassert.h"
#include "frlist.h"
#include "frsymtab.h"
#include "frutil.h"
#include "frpcglbl.h"

#include "frprintf.h"

//----------------------------------------------------------------------

// force the global constructors for this module to run before those of
// the application program
#ifdef __WATCOMC__
#pragma initialize before library ;
#endif /* __WATCOMC__ */

#if defined(_MSC_VER) && _MSC_VER >= 800
#pragma warning(disable : 4073)    // don't warn about following #pragma ....
#pragma init_seg(lib)
#endif /* _MSC_VER >= 800 */

/************************************************************************/
/*	Types for this module						*/
/************************************************************************/

/************************************************************************/
/*	Global variables shared with other modules in FramepaC		*/
/************************************************************************/

size_t FrSymbolTable_blocks = 0 ;

_FrFrameInheritanceFunc null_inheritance ;
FrInheritanceType FramepaC_inheritance_type = NoInherit ;
_FrFrameInheritanceFunc *FramepaC_inheritance_function = null_inheritance ;

extern int FramepaC_symbol_blocks ;

void (*FramepaC_destroy_all_symbol_tables)() ;

/************************************************************************/
/*	Global data local to this module				*/
/************************************************************************/

static const char stringISA[] = "IS-A" ;
static const char stringVALUE[] = "VALUE" ;
static const char stringINSTANCEOF[] = "INSTANCE-OF" ;
static const char stringSUBCLASSES[] = "SUBCLASSES" ;
static const char stringINSTANCES[] = "INSTANCES" ;
static const char stringHASPARTS[] = "HAS-PARTS" ;
static const char stringPARTOF[] = "PART-OF" ;
static const char stringSEM[] = "SEM" ;
static const char stringCOMMON[] = "COMMON" ;
static const char stringEOF[] = "*EOF*" ;
static const char stringINHERITS[] = "INHERITS" ;

static const char *default_symbols[] =
   {
   //"NIL" is added by create_symbol_table regardless
   "?", stringISA, stringINSTANCEOF, stringSUBCLASSES, stringINSTANCES,
   stringHASPARTS, stringPARTOF, stringVALUE, stringSEM, stringCOMMON,
   stringEOF, stringINHERITS, ".",
   } ;

#ifdef FrSYMBOL_RELATION
static const char *default_relations[][2] =
   {  { stringISA, stringSUBCLASSES },
      { stringINSTANCEOF, stringINSTANCES },
      { stringPARTOF, stringHASPARTS },
   } ;
#endif /* FrSYMBOL_RELATION */

static const char nomem_palloc[]
   = "in palloc()" ;

/************************************************************************/
/*	Global variables local to this module				*/
/************************************************************************/

static FrSymbolTable *symbol_tables = 0 ;
static FrSymbolTable *default_symbol_table = 0 ;

static FrSymbolTable *DefaultSymbolTable = 0 ;
FrSymbolTable *ActiveSymbolTable = 0 ;

/************************************************************************/
/*	Forward declarations						*/
/************************************************************************/

void _FramepaC_destroy_all_symbol_tables() ;

/************************************************************************/
/*	Utility functions						*/
/************************************************************************/

#if defined(__WATCOMC__) && defined(__386__)
extern unsigned long symboltable_hashvalue(const char *name,size_t *length) ;
#pragma aux symboltable_hashvalue = \
	"xor  eax,eax" \
	"xor  edx,edx" \
        "push ebx" \
    "l1: ror  eax,5" \
	"mov  dl,[ebx]" \
	"inc  ebx" \
	"add  eax,edx" \
	"test dl,dl" \
	"jne  l1" \
        "pop  edx" \
	"sub  ebx,edx" \
	"mov  [ecx],ebx" \
	parm [ebx][ecx] \
	value [eax] \
	modify exact [eax ebx edx] ;

#elif defined(__GNUC__) && defined(__386__)
inline unsigned long symboltable_hashvalue(const char *name, size_t *symlength)
{
   unsigned long hashvalue ;
   unsigned long t ;
   __asm__("xor %1,%1\n\t"
	   "xor %2,%2\n\t"
	   "push %3\n"
     ".LBL%=:\n\t"
           "ror $5,%1\n\t"
	   "movb (%3),%b2\n\t"
	   "inc %3\n\t"
	   "add %2,%1\n\t"
	   "test %b2,%b2\n\t"
	   "jne .LBL%=\n\t"
	   "pop %2\n\t"
	   "sub %2,%3"
	   : "=r" (*symlength), "=&r" (hashvalue), "=&q" (t)
	   : "0" (name)
	   : "cc" ) ;
   return hashvalue ;
}

#elif defined(_MSC_VER) && _MSC_VER >= 800
inline unsigned long symboltable_hashvalue(const char *name,size_t *symlength)
{
   unsigned long hash ;
   _asm {
          mov ebx,name ;
	  xor eax,eax ;
	  xor edx,edx ;
	  push ebx ;
        } ;
     l1:
   _asm {
          ror eax,5
	  mov dl,[ebx]
	  inc ebx
	  add eax,edx
	  test dl,dl
	  jne l1
	  pop edx
          mov ecx,symlength
	  sub ebx,edx
	  mov hash,eax
	  mov [ecx],ebx
        } ;
   return hash ;
}

#else // neither Watcom C++ nor Visual C++ nor GCC on x86

#define rotr5(X) ((sum << 27) | ((sum >> 5) & 0x07FFFFFF))

// BorlandC++ won't inline functions containing while loops
#ifdef __BORLANDC__
static
#else
inline
#endif /* __BORLANDC__ */
unsigned long symboltable_hashvalue(const char *name,size_t *length)
{
   register unsigned long sum = 0 ;
   const char *name_start = name ;
   int c ;
   do
      {
      c = *(unsigned char*)name++ ;
      sum = rotr5(sum) + c ;
      } while (c) ;
   *length = (name-name_start) ;
   return sum ;
}
#endif /* __WATCOMC__ && __386__ */

//----------------------------------------------------------------------
// Don't do any inheritance at all

/* ARGSUSED */
const FrList *null_inheritance(FrFrame * /*frame*/,
			       const FrSymbol * /*slot*/,
			       const FrSymbol * /*facet*/)
{
   return 0 ;
}

/************************************************************************/
/*	Methods for class FrSymbolTableX				*/
/************************************************************************/

size_t Fr_symboltable_hashvalue(const char *symname)
{
   assert(symname != 0) ;
   size_t namelen ;
   return (size_t)symboltable_hashvalue(symname,&namelen) ;
}

template <>
size_t FrSymbolTableX::hashVal(const char *symname, size_t *namelen)
{
   assert(symname != 0) ;
   return (size_t)symboltable_hashvalue(symname,namelen) ;
}

//----------------------------------------------------------------------

template <>
bool FrSymbolTableX::isEqual(const char *symname, size_t namelen, const FrSymbol *sym)
{
   assert(symname != 0) ;
   return sym && isActive(sym) && memcmp(symname,sym->symbolName(),namelen) == 0 ;
}

/************************************************************************/
/*	Methods for class FrSymbolTable					*/
/************************************************************************/

FrSymbolTable::FrSymbolTable(unsigned int max_symbols) : FrSymbolTableX(max_symbols)
{
   size_t i ;

   m_name = 0 ;
#ifdef FrLRU_DISCARD
   LRUclock = 0 ;
#endif /* FrLRU_DISCARD */
   bstore_info = 0 ;
   inheritance_type = FramepaC_inheritance_type ;
   inheritance_function = FramepaC_inheritance_function ;
   delete_hook = 0 ;
   for (i = 0 ; i < FrSYMTAB_SIZE_BINS ; i++)
      {
      char namebuf[100] ;
      if (i == 0) strcpy(namebuf,"short symbols") ;
      else if (i == 1) strcpy(namebuf,"medium symbols") ;
      else
	 {
	 strcpy(namebuf,"symbols ") ;
	 char *endptr = strchr(namebuf,'\0') ;
	 ultoa(i,endptr,10) ;
	 }
      m_allocators[i] = new FrAllocator(namebuf,sizeof(FrSymbol)+i*FrSYMTAB_SIZE_GRAN) ;
      //m_allocators[i]->preAllocate() ;
      }
   init_palloc() ;
   std_symbols[0] = 0 ;
   std_symbols[0] = add("NIL",this
#ifdef FrSYMBOL_VALUE
			,0
#endif
                        ) ;
   std_symbols[1] = add("T",this
#ifdef FrSYMBOL_VALUE
			,0
#endif
			) ;
#ifdef FrSYMBOL_VALUE
   std_symbols[1].setValue(symbolT,this) ;
#endif /* FrSYMBOL_VALUE */
   for (i = 0 ; i < lengthof(default_symbols) ; i++)
      std_symbols[i+2] = add(default_symbols[i],this) ;
#ifdef FrSYMBOL_RELATION
   for (i = 0 ; i < lengthof(default_relations) ; i++)
      {
      FrSymbol *relation = add(default_relations[i][0],this) ;
      FrSymbol *inverse =  add(default_relations[i][1],this) ;

      if (relation)
	 relation->defineRelation(inverse) ;
      }
#endif /* FrSYMBOL_RELATION */
   m_prev = 0 ;
   m_next = symbol_tables ;
   if (symbol_tables)
      symbol_tables->m_prev = this ;
   if (!FramepaC_destroy_all_symbol_tables)
      FramepaC_destroy_all_symbol_tables = _FramepaC_destroy_all_symbol_tables;
   return ;
}

//----------------------------------------------------------------------

void FramepaC_delete_symbols(FrMemFooter*);

FrSymbolTable::~FrSymbolTable()
{
   if (DefaultSymbolTable && this == current())
      DefaultSymbolTable->select() ;
   if (delete_hook)
      {
      delete_hook(this) ;
      delete_hook = 0 ;
      }
   if (FramepaC_delete_all_frames)
      FramepaC_delete_all_frames() ;
   FrFree(m_name) ;
   m_name = 0 ;
   free_palloc() ;
   for (size_t i = 0 ; i < FrSYMTAB_SIZE_BINS ; ++i)
      {
      delete m_allocators[i] ;
      m_allocators[i] = 0 ;
      }
   if (this == DefaultSymbolTable)
      DefaultSymbolTable = 0 ;
   if (this == ActiveSymbolTable)
      {
      if (DefaultSymbolTable)
	 DefaultSymbolTable->select() ;
      else if (m_next)
	 m_next->select() ;
      else if (m_prev)
	 m_prev->select() ;
      else
	 ActiveSymbolTable = 0 ;
      }
   if (m_next)
      m_next->m_prev = m_prev ;
   if (m_prev)
      m_prev->m_next = m_next ;
   else
      symbol_tables = m_next ;
   return ;
}

//----------------------------------------------------------------------

void FrSymbolTable::setName(const char *nam)
{
   FrFree(m_name) ;
   m_name = FrDupString(nam) ;
   return ;
}

//----------------------------------------------------------------------

void FrSymbolTable::setDeleteHook(FrSymTabDeleteFunc *delhook)
{
   delete_hook = delhook ;
   return ;
}

//----------------------------------------------------------------------

FrSymbolTable *FrSymbolTable::select()
{
   if (this == 0)
      {
      if (DefaultSymbolTable)
	 return DefaultSymbolTable->select() ;
      else
         return current() ;
      }
   FrSymbolTable *active = current() ;
   if (this != active)
      {
      FrSymbolTable *oldtable = active ;
      ActiveSymbolTable = this ;
      return oldtable ? oldtable : this ;
      }
   else
      return active ;
}

//----------------------------------------------------------------------

FrSymbolTable *FrSymbolTable::selectDefault()
{
   if (!DefaultSymbolTable)
      return current()->select() ;
   return DefaultSymbolTable->select() ;
}

//----------------------------------------------------------------------

FrSymbolTable *FrSymbolTable::current()
{
   if (!ActiveSymbolTable)
      {
      default_symbol_table = new FrSymbolTable(0) ;
      ActiveSymbolTable = default_symbol_table ;
      if (!DefaultSymbolTable)
	 DefaultSymbolTable = ActiveSymbolTable ;
      }
   return ActiveSymbolTable ;
}

//----------------------------------------------------------------------

const FrSymbol *FrSymbolTable::allocate_symbol(const char *newname, size_t len)
{
   char *symbuf ;
   FrSymbol *newsym ;
   unsigned numbytes = Fr_offsetof(*newsym,m_name[0]) + len ;
   int bin = (numbytes - sizeof(FrSymbol) + FrSYMTAB_SIZE_GRAN - 1) / FrSYMTAB_SIZE_GRAN ;
   if (bin < 0) bin = 0 ;
   if (bin < FrSYMTAB_SIZE_BINS)
      {
      symbuf = reinterpret_cast<char*>(m_allocators[bin]->allocate()) ;
      if (symbuf)
	 {
	 newsym = new(symbuf) FrSymbol(newname,len) ;
	 return newsym ;
	 }
      }
   // we have a long symbol name for which there isn't a specific allocator, so
   //   allocate from the general list
   numbytes = round_up(numbytes,FrALIGN_SIZE_PTR);
   m_palloc_lock.acquire() ;
   FrMemFooter *p_list = palloc_list ;
   if (p_list->freecount() >= numbytes)
      {
      p_list->adjObjectStartDown(numbytes) ;
      symbuf = p_list->objectStartPtr() ;
      }
   else
      {
      symbuf = (char*)palloc(numbytes) ;
      if (!symbuf)
	 {
	 m_palloc_lock.release() ;
	 return 0 ;			// out of memory!
	 }
      }
   m_palloc_lock.release() ;
   newsym = new(symbuf) FrSymbol(newname,len) ;
   return newsym ;
}

//----------------------------------------------------------------------

const FrSymbol *Fr_allocate_symbol(FrSymbolTable *symtab, const char *name, size_t namelen)
{
   return symtab->allocate_symbol(name,namelen) ;
}

//----------------------------------------------------------------------

FrSymbol *FrSymbolTable::add(const char *symname, FrSymbolTable *symtab)
{
   if (!symtab)
      symtab = current() ;
   assertq(symname != 0 && symtab != 0) ;
   return const_cast<FrSymbol*>(symtab->addKey(symname)) ;
}

//----------------------------------------------------------------------

FrSymbol *FrSymbolTable::add(const char *symname)
{
   return add(symname,current()) ;
}

//----------------------------------------------------------------------

#ifdef FrSYMBOL_VALUE
FrSymbol *FrSymbolTable::add(const char *name,const FrObject *value)
{
   FrSymbol *sym = FrSymbolTable::add(name,current()) ;
   sym->setValue(value) ;
   return sym ;
}
#endif

//----------------------------------------------------------------------

#ifdef FrSYMBOL_VALUE
FrSymbol *FrSymbolTable::add(const char *name,FrSymbolTable *symtab,
			      const FrObject *value)
{
   FrSymbol *sym = FrSymbolTable::add(name,symtab) ;
   sym->setValue(value) ;
   return sym ;
}
#endif

//----------------------------------------------------------------------

FrSymbol *FrSymbolTable::gensym(const char *basename, const char *user_suffix)
{
   static size_t gensym_counter = 0 ;

   char newname[FrMAX_SYMBOLNAME_LEN+1] ;
   char suffix[FrMAX_SYMBOLNAME_LEN+1] ;

   if (!basename || !*basename)
      basename = "GS" ;
   size_t baselen = strlen(basename) ;
   if (baselen > sizeof(newname) - FrMAX_ULONG_STRING - 3)
      baselen = sizeof(newname) - FrMAX_ULONG_STRING - 3 ;
   memcpy(newname,basename,baselen) ;
   if (baselen > 0 && Fr_isdigit(newname[baselen-1]))
      newname[baselen++] = '_' ;
   if (baselen > FrMAX_SYMBOLNAME_LEN-FrMAX_ULONG_STRING)
      baselen = FrMAX_SYMBOLNAME_LEN-FrMAX_ULONG_STRING ;
   size_t suffixlen = 0 ;
   if (user_suffix && *user_suffix)
      {
      memcpy(suffix,user_suffix,sizeof(suffix)) ;
      suffixlen = strlen(user_suffix) ;
      // ensure that we don't overflow our buffer
      if (suffixlen > FrMAX_SYMBOLNAME_LEN-FrMAX_ULONG_STRING-baselen)
	 suffix[FrMAX_SYMBOLNAME_LEN-FrMAX_ULONG_STRING-baselen] = '\0' ;
      }
   char *digits = newname + baselen ;
   bool existed ;
   const FrSymbol *newsym ;
   do {
      size_t count = m_palloc_lock.increment(gensym_counter) ;
      ultoa(count,digits,10) ;
      if (suffixlen > 0)
	 strcat(digits,suffix) ;
      existed = false ;
      newsym = FrSymbolTableX::addKey(newname,&existed) ;
      } while (existed) ;
   return const_cast<FrSymbol*>(newsym) ;
}

//----------------------------------------------------------------------

FrSymbol *FrSymbolTable::lookup(const char *symname) const
{
   assert(symname != 0) ;
   return const_cast<FrSymbol*>(lookupKey(symname)) ;
}

//----------------------------------------------------------------------

void FrSymbolTable::setNotify(VFrameNotifyType type, VFrameNotifyFunc *func)
{
   if (VFrame_Info)
      VFrame_Info->setNotify(type,func) ;
   return ;
}

//----------------------------------------------------------------------

VFrameNotifyPtr FrSymbolTable::getNotify(VFrameNotifyType type) const
{
   return (VFrame_Info) ? VFrame_Info->getNotify(type) : 0 ;
}

//----------------------------------------------------------------------

bool FrSymbolTable::isReadOnly() const
{
   return (VFrame_Info) ? VFrame_Info->isReadOnly() : false ;
}

//----------------------------------------------------------------------

bool FrSymbolTable::iterateFrameVA(FrIteratorFunc func, va_list args) const
{
   if (this == current())
      {
      // force updates to be copied from our global static buffer into the
      // symbol table
      FrSymbolTable *old_symtab = FrSymbolTable::selectDefault() ;
      old_symtab->select() ;
      }
   Table *table = m_table.load() ;
   size_t siz = table->m_size ;
   if (siz > 0)
      {
      for (size_t i = 0 ; i < siz ; i++)
	 {
	 const Entry *ent = &table->m_entries[i] ;
	 if (ent->isActive())
	    {
	    FrFrame *frame = (FrFrame*)ent->getKey()->symbolFrame() ;
	    if (frame)
	       {
	       FrSafeVAList(args) ;
	       bool success = func(frame,FrSafeVarArgs(args)) ;
	       FrSafeVAListEnd(args) ;
	       if (!success)
		  return false ;
	       }
	    }
	 }
      }
   return true ;
}

//----------------------------------------------------------------------

bool __FrCDECL FrSymbolTable::iterateFrame(FrIteratorFunc func, ...) const
{
   va_list args ;
   va_start(args,func) ;
   bool success = iterateFrameVA(func,args) ;
   va_end(args) ;
   return success ;
}

//----------------------------------------------------------------------

static bool list_relations(const FrSymbol *sym, FrNullObject, va_list args)
{
   FrVarArg(FrList **,relations) ;
   FrSymbol *inv = sym->inverseRelation() ;
   if (inv && !(*relations)->assoc(inv))
      pushlist(new FrList(sym,inv),*relations) ;
   return true ;	// continue iterating
}

//----------------------------------------------------------------------

FrList *FrSymbolTable::listRelations() const
{
   if (this == current())
      {
      // force updates to be copied from our global static buffer into the
      // symbol table
      FrSymbolTable *old_symtab = FrSymbolTable::selectDefault() ;
      old_symtab->select() ;
      }
   FrList *relations = 0 ;
   (void)iterate(list_relations,&relations) ;
   return relations ;
}

//----------------------------------------------------------------------

void FrSymbolTable::setProxy(VFrameNotifyType type, VFrameProxyFunc *func)
{
   if (VFrame_Info)
      VFrame_Info->setProxy(type,func) ;
   return ;
}

//----------------------------------------------------------------------

VFrameProxyPtr FrSymbolTable::getProxy(VFrameNotifyType type) const
{
   return (VFrame_Info) ? VFrame_Info->getProxy(type) : 0 ;
}

//----------------------------------------------------------------------

void FrSymbolTable::setShutdown(VFrameShutdownFunc *func)
{
   if (VFrame_Info)
      VFrame_Info->setShutdown(func) ;
   return ;
}

//----------------------------------------------------------------------

VFrameShutdownPtr FrSymbolTable::getShutdown() const
{
   return (VFrame_Info) ? VFrame_Info->getShutdown() : 0 ;
}

//----------------------------------------------------------------------

#ifdef FrLRU_DISCARD
void FrSymbolTable::adjust_LRUclock()
{
   if (this == current())
      {
      // force updates to be copied from our global static buffer into the
      // symbol table
      FrSymbolTable *old_symtab = FrSymbolTable::selectDefault() ;
      old_symtab->select() ;
      }
   Table *table = m_table.load();
   for (size_t i = 0 ; i < table->m_size ; ++i)
      {
      const Entry *ent = &table->m_entries[i] ;
      if (ent->isActive())
	 {
	 FrFrame *frame = ent->getKey()->symbolFrame() ;
         if (frame)
	    frame->adjust_LRUclock() ;
	 }
      }
   return ;
}
#endif /* FrLRU_DISCARD */

//----------------------------------------------------------------------

int FrSymbolTable::countFrames() const
{
   if (this == current())
      {
      // force updates to be copied from our global static buffer into the
      // symbol table
      FrSymbolTable *old_symtab = FrSymbolTable::selectDefault() ;
      old_symtab->select() ;
      }
   int count = 0 ;
   Table *table = m_table.load() ;
   for (size_t i = 0 ; i < table->m_size ; i++)
      {
      const Entry *ent = &table->m_entries[i] ;
      if (ent->isActive() && ent->getKey()->symbolFrame())
	 {
	 ++count ;
	 }
      }
   return count ;
}

//----------------------------------------------------------------------

#ifdef FrLRU_DISCARD
uint32_t FrSymbolTable::oldestFrameClock() const
{
   if (this == current())
      {
      // force updates to be copied from our global static buffer into the
      // symbol table
      FrSymbolTable *old_symtab = FrSymbolTable::selectDefault() ;
      old_symtab->select() ;
      }
   uint32_t oldest = UINT32_MAX ;
   Table *table = m_table.load() ;
   for (size_t i = 0 ; i < table->m_size ; i++)
      {
      const Entry *ent = &table->m_entries[i] ;
      if (ent->isActive())
	 {
	 FrFrame *frame = ent->getKey()->symbolFrame() ;
	 if (frame)
	    {
	    uint32_t clock = frame->getLRUclock() ;
	    if (clock < oldest)
	       oldest = clock ;
	    }
	 }
      }
   return oldest ;
}
#endif /* FrLRU_DISCARD */

//----------------------------------------------------------------------

bool FrSymbolTable::checkSymbols() const
{
   // scan all symbol objects, checking that they have the correct VMT
   for (FrMemFooter *symblock=palloc_list ; symblock ; symblock=symblock->next())
      {
      char *base = FrBLOCK_PTR(symblock) ;
      for (FrSubAllocOffset i = symblock->objectStart() ; i < FrFOOTER_OFS ; )
	 {
	 FrSymbol *sym = (FrSymbol*)(base + i) ;
	 if (*((int*)sym) != *((int*)std_symbols[0]))
	    return false ;
	 size_t len = strlen(sym->symbolName()) + 1 ;
	 i += round_up(Fr_offsetof(*sym,m_name[0]) + len, FrALIGN_SIZE_PTR) ;
	 }
      }
   return true ;
}

/************************************************************************/
/*      Member functions for class FrSymbolTable                        */
/************************************************************************/

void FrSymbolTable::init_palloc()
{
   palloc_pool = 0 ;
   palloc_list = allocate_block("initializing symbol table") ;
   if (palloc_list)
      {
      FramepaC_symbol_blocks++ ;
      palloc_list->next(0) ;
      palloc_list->setObjectStart(FrFOOTER_OFS) ;
      }
   return ;
}

//----------------------------------------------------------------------
//  make small, "permanent" (never freed unless symbol table is destroyed)
//  memory allocations for symbol names

void *FrSymbolTable::palloc(unsigned int numbytes)
{
   register FrMemFooter *newpalloc ;
   m_palloc_mutex.lock() ;
   if (palloc_pool)
      {
      newpalloc = palloc_pool ;
      palloc_pool = palloc_pool->next() ;
      }
   else
      {
      newpalloc = allocate_block(nomem_palloc) ;
      if (newpalloc)
	 FramepaC_symbol_blocks++ ;
      else
	 {
	 m_palloc_mutex.unlock() ;
	 return 0 ;
	 }
      }
   newpalloc->next(palloc_list) ;
   newpalloc->setObjectStart(FrFOOTER_OFS - numbytes) ;
   palloc_list = newpalloc ;
   m_palloc_mutex.unlock() ;
   return newpalloc->objectStartPtr() ;
}

//----------------------------------------------------------------------

void FrSymbolTable::free_palloc()
{
   FrMemFooter *block ;
   while (palloc_list)
      {
      block = palloc_list ;
      palloc_list = block->next() ;
      FrMemoryPool::addFreePage(block) ;
      FramepaC_symbol_blocks-- ;
      }
   while (palloc_pool)
      {
      block = palloc_pool ;
      palloc_pool = palloc_pool->next() ;
      FrMemoryPool::addFreePage(block) ;
      FramepaC_symbol_blocks-- ;
      }
   return ;
}

/**********************************************************************/
/*	Multiple FrSymbol Table Support				      */
/**********************************************************************/

void name_symbol_table(const char *name)
{
   FrSymbolTable *symt = FrSymbolTable::current() ;
   if (symt)
      symt->setName(name) ;
   return ;
}

//----------------------------------------------------------------------

FrSymbolTable *select_symbol_table(const char *table_name)
{
   if (!table_name)
      return 0 ;
   for (FrSymbolTable *symtab = symbol_tables ; symtab ; symtab = symtab->m_next)
      {
      if (symtab->m_name && strcmp(symtab->m_name,table_name) == 0)
	 return symtab->select() ;
      }
   return 0 ;
}

//----------------------------------------------------------------------

void destroy_symbol_table(FrSymbolTable *symtab)
{
   if (symtab && symtab != DefaultSymbolTable)
      {
      if (symtab == FrSymbolTable::current())
	 {
	 if (DefaultSymbolTable)
	    DefaultSymbolTable->select() ;
         else
	    FrError("unable to switch to default symbol table in "
		    "destroy_symbol_table()") ;
         }
      delete symtab ;
      }
   return ;
}

//----------------------------------------------------------------------

void _FramepaC_destroy_all_symbol_tables()
{
   FrSymbolTable *prev = 0 ;

   FrSymbolTable::selectDefault() ;
   // loop until no more symbol tables exist, or the only one is the
   // default symbol table (which never gets destroyed)
   while (symbol_tables && symbol_tables != prev)
      {
      prev = symbol_tables ;
      if (symbol_tables != DefaultSymbolTable)
	 destroy_symbol_table(symbol_tables) ;
      else
	 symbol_tables = symbol_tables->m_next ;
      }
   return ;
}

//----------------------------------------------------------------------

void __FrCDECL do_all_symtabs(DoAllSymtabsFunc *func, ...)
{
   FrSymbolTable *symtab, *next ;
   FrSymbolTable *orig = FrSymbolTable::current() ;

   symtab = symbol_tables ;
   while (symtab && symtab->m_next != symtab)
      {
      next = symtab->m_next ;
      symtab->select() ;
      // note: under Watcom C++, use of the va_list inside the called
      // function affects its value here!  So, we have to reset it each
      // time through the loop
      va_list args ;
      va_start(args, func) ;
      func(symtab,args) ;
      va_end(args) ;
      symtab = next ;
      }
   orig->select() ;
   return ;
}

//----------------------------------------------------------------------

void FramepaC_shutdown_symboltables()
{
   delete default_symbol_table ;
   default_symbol_table = 0 ;
   return ;
}

// end of file frsymtab.cpp //
