@comment[
<html>
<head>
<title>FramepaC User's Reference</title>
<LINK REV=OWNER HREF="mailto:ralf+@cs.cmu.edu">
<META name="robots" content="noindex">
<META name="description" content="User's Reference for the FramepaC C++
 frame-manipulation and Lisp-like data-structures package by Ralf Brown.">
</head>
<body>
<H1>FramepaC User's Reference</H1>
<pre>
]
@make{manual}
@libraryfile{MultiLevelIndex}
@style{fontfamily=TimesRoman,size=10}
@modify{HD0,PageBreak Before} @comment{instead of UntilOdd, for ToC etc.}
@modify{HD1,PageBreak Before} @comment{instead of UntilOdd, for chapters}
@modify{IndexEnv,boxed,columns=2}

@use{bibliography="/afs/cs/user/ralf/bib/ralf.bib"}
@pageheading{even,right="DRAFT of @value{date}",left="@value{page}"}
@pageheading{odd,left="DRAFT of @value{date}",right="@value{page}"}

@form[index2=[@IndexSecondary(Primary=<@parm'p'>,Secondary=<@parm's'>)]]
@textform[indexfunc=[@IndexSecondary(Primary="Functions",
                                Secondary=<@parm'text'>)
                        @Index<@parm'text'>]]
@textform[indexmeth=[@IndexSecondary(Primary="Methods",
                                Secondary=<@parm'text'>)
                        @Index<@parm'text'>]]
@textform[indexvirt=[@IndexSecondary(Primary="Virtual Methods",
                                Secondary=<@parm'text'>)
                        @Index<@parm'text'>
                        @IndexSecondary(Primary="Methods",
                                Secondary=<@parm'text'>)
                        @Index<@parm'text'>]]
@textform[indexctor=[@IndexSecondary(Primary="Constructors",
                                Secondary=<@parm'text'>)
                        @Index<@parm'text'>]]
@textform[indexdtor=[@IndexSecondary(Primary="Destructors",
                                Secondary=<@parm'text'>)
                        @Index<@parm'text'>]]
@textform[indexpp=[@IndexSecondary(Primary='Preprocessor Symbols',
                                Secondary=<@parm'text'>)
                        @Index<@parm'text'>]]
@textform[indextype=[@IndexSecondary(Primary="Types",
                                Secondary=<@parm'text'>)
                        @Index<@parm'text'>]]
@textform[indexmacro=[@IndexSecondary(Primary="Macros",
                                Secondary=<@parm'text'>)
                        @Index<@parm'text'>]]
@textform[indexoper=[@IndexSecondary(Primary="Operators",
                                Secondary='@parm<text>')]]
@define{FuncDesc=text, leftmargin=0.3in, rightmargin=0.1in, indent=+0,
        spread=0.5, above=2lines, below=1line}
@define{Func=format, facecode=i, indent=0in, leftmargin=0in,
        break=after, continue=allowed}

@comment{---------------------------------------------------------------}

@begin{titlepage}

@begin{titlebox}
@begin{center}
@heading{FramepaC User's Reference}
@heading{Version 1.10}
@heading{(Document Version 39a)}
@subheading{CMU-CMT-98-MEMO}
@blankspace(0.5in)
Ralf D. Brown
@value{date}
@end{center}
@end{titlebox}

@begin{abstract}
FramepaC is a frame manipulation and utility package for C++
programmers which has been optimized for speed over ultimate
generality.  In addition to frame manipulation, it provides nearly all
of the data types supported by Common Lisp.  Despite the emphasis on raw
speed, it still provides a rich set of features for frames, including
inheritance, a Lisp-like reader, demand-loaded frames using either a
disk file or a network server as backing store, and optionally demons.
@end{abstract}

@copyright{1995,1996,1997,1998 Ralf Brown}

@end{titlepage}

@comment{---------------------------------------------------------------}

@chapter{Introduction}

FramepaC is a highly efficient frame manipulation package for C++
programmers which was originally designed for use in the MikroKARAT
knowledge acquisition environment@cite{MikroKARAT}.  Although designed
primarily for the author's needs in various NLP programs, FramepaC is
still quite general and should be valuable in a variety of other
applications.

FramepaC's core frame function set has been patterened after FrameKit
@cite{FrameKit}, a Common Lisp package used in a number of previous
projects, including the ONTOS knowledge acquisition system
@cite{ONTOS88,ONTOSrm} which MikroKARAT was designed to supplant.
FrameKit's primary drawback has been its extreme slowness, due to
handling the most general cases imaginable; even fairly simple
modifications have on more than one occasion resulted in a system-wide
speedup by a factor of three or more.  FramepaC omits one of the most
expensive features in FrameKit (automatic copying of all retrieved
data--which FrameKit does to permit destructive modification of the
result, something that happens rather infrequently), and makes another
one (demons) a compile-time option.  It also omits views, which have
never found much use (ONTOS does not use them), and always stores
certain slots and facets in a frame even when they have not been
explicitly used in order to allow direct lookups in place of searches
when accessing those most-common fillers.

The remainder of this document contains the following chapters:
@begin{itemize}
an overview of the data types available in FramepaC

a brief tutorial on programming with FramepaC

an overview of demons

an overview of FramepaC memory-allocation functions

an overview of Motif-related user-interface functions

a complete reference of all the functions provided by FramepaC

a reference of the various configuration-related preprocessor symbols

a trouble-shooting Q&A

instructions for using the test/demo program supplied with FramepaC

a review of benchmark results

instructions for compiling FramepaC
@end{itemize}

@comment{---------------------------------------------------------------}

@chapter{The FramepaC Type System}

FramepaC provides a number of data types, largely patterned after the
Lisp types, which allow flexible storage of information.  The three
primary types used by FramepaC functions are the @t{FrObject}, the
@t{FrSymbol}, and the @t{FrList}.  The FramepaC types are arranged in a
class hierarchy (described in the next section), so that a more
specific type of object may be passed to a function anywhere a general
type is expected.  In general, a pointer to the object is used rather
than the object itself.

@section{Class Hierarchy}

@begin{table,float=top}
@begin{verbatim}
   FrSymbolTable                                Public Types
   FrReadTable
   FrReader
   FrConfiguration
   FrEventList
   FrTimer
   FrMemoryPool
      |
      +------ FrAllocator

   FrObject
      |
      +------ FrArray
      |        |
      |        +----- FrSparseArray
      |
      +------ FrAtom
      |        |
      |        +----- FrSymbol
      |        +----- FrString
      |        +----- FrNumber
      |                 |
      |                 +----- FrFloat
      |                 +----- FrInteger
      +------ FrBitVector
      +------ FrCons
      |        |
      |        +----- FrList
      |
      +------ FrHashTable
      +------ FrHashEntry
      |        |
      |        +----- FrHashEntryObject
      |
      +------ FrFrame
      |        |
      |        +----- VFrame
      |
      +------ FrQueue
      +------ FrStruct
      +------ FrWidget ---- (see Figure @ref{class_hier2})
      |
      +------ FrISockStream \
      +------ FrOSockStream / FrSockStream
@end{verbatim}
@caption{The FramepaC Data-Type Hierarchy}
@tag{class_hier}
@end{table}
@indextype{FrConfiguration}
@indextype{FrCons}
@indextype{FrFrame}
@indextype{FrAllocator}
@indextype{FrBitVector}
@indextype{FrEventList}
@indextype{FrFloat}
@indextype{FrHashEntryObject}
@indextype{FrHashEntry}
@indextype{FrHashTable}
@indextype{FrInteger}
@indextype{FrList}
@indextype{FrMemoryPool}
@indextype{FrNumber}
@indextype{FrQueue}
@indextype{FrReadTable}
@indextype{FrSparseArray}
@indextype{FrStruct}
@indextype{FrSymbolTable}
@indextype{FrSymbol}
@indextype{FrString}
@indextype{FrTimer}
@indextype{FrWidget}
@indextype{VFrame}

@begin{table}
@begin{verbatim,size -1}
   FrObject
      |
      +------ FrWidget
                |
                +----- FrWButtonBar
                +----- FrWForm
                +----- FrWFrame
                +----- FrWLabel
                +----- FrWList
                +----- FrWMainWindow
                +----- FrWOptionMenu
                +----- FrWRadioBox
                +----- FrWRowColumn
                +----- FrWScrollBar
                +----- FrWScrollWindow
                +----- FrWSelectionBox
                +----- FrWSeparator
                +----- FrWSlider
                +----- FrWArrow
                |       |
                |       +----- FrWArrowG
                |
                +----- FrWPopupMenu
                |       |
                |       +----- FrWPulldownMenu
                |       |
                |       +----- FrWPromptPopup
                |               |
                |               +----- FrWDialogPopup
                |               +----- FrWFramePrompt
                |
                +----- FrWCascadeButton
                +----- FrWPushButton
                |       |
                |       +----- FrWPushButtonG
                |       +----- FrWToggleButton
                |
                +----- FrWText
                |       |
                |       +----- FrWShadowText
                |       +----- FrWFrameCompleter
                |
                +----- FrWProgressIndicator
                        |
                        +----- FrWProgressPopup
                
@end{verbatim}
@indextype{FrObject}
@indextype{FrWArrowG}           
@indextype{FrWArrow}            
@indextype{FrWButtonBar}        
@indextype{FrWDialogPopup}      
@indextype{FrWForm}             
@indextype{FrWFrameCompleter}   
@indextype{FrWFramePrompt}      
@indextype{FrWFrame}            
@indextype{FrWidget}            
@indextype{FrWLabel}            
@indextype{FrWList}             
@indextype{FrWMainWindow}       
@indextype{FrWOptionMenu}       
@indextype{FrWPopupMenu}        
@indextype{FrWProgressIndicator}
@indextype{FrWProgressPopup}    
@indextype{FrWPromptPopup}      
@indextype{FrWPulldownMenu}     
@indextype{FrWPushButtonG}      
@indextype{FrWPushButton}       
@indextype{FrWRadioBox}         
@indextype{FrWRowColumn}        
@indextype{FrWScrollBar}        
@indextype{FrWScrollWindow}     
@indextype{FrWSelectionBox}     
@indextype{FrWSeparator}        
@indextype{FrWSeparator}        
@indextype{FrWShadowText}       
@indextype{FrWSlider}           
@indextype{FrWText}             
@indextype{FrWCascadeButton}     
@indextype{FrWToggleButton}     
@caption{The FramepaC Data-Types (Motif Widgets)}
@tag{class_hier2}
@end{table}

@begin{table}
@begin{verbatim,size -1}
   (FrObject)
      |         
      +------ VFrameInfo
               |
               +----- VFrameInfoFile
               +----- VFrameInfoServer
@hinge()
   (FrHashEntry)
      |
      +------ HashEntryVFrame
      |
      +------ HashEntryServer

   FrSlot
   FrFacet

   FrServer
   FrClient

   FrSymHashTable
@end{verbatim}
@indextype{HashEntryServer}
@indextype{HashEntryVFrame}
@indextype{FrSymHashTable}
@indextype{VFrameInfoFile}
@indextype{VFrameInfoServer}
@indextype{VFrameInfo}
@caption{FramepaC-internal Types}
@tag{internal_types}
@end{table}

@indextype{FrObject}
A @t{FrObject}, as its name implies, is a generalized object which can
be of any of the Lisp-like types shown in Table @ref{class_hier},
including @t{FrSymbol} and @t{FrList}.  This is the most general of the
data types used in the FramepaC interface.

@indextype{FrFrame}
The @t{FrFrame} is the main data type, which most of the other types were
created to support in some fashion.  A frame contains one or more
slots, each with one or more facets containing some number of fillers.
A @t{VFrame} is a virtualized frame, which need not exist in memory
until explicitly accessed.

@indextype{FrSymbol}
A @t{FrSymbol} is the exact equivalent of a Lisp symbol.  For any given
sequence of characters, there is exactly one @t{FrSymbol}@foot<More
accurately, one per symbol table, since the program may explicitly create
additional symbol tables after it starts.>, so a @t{FrSymbol} may always
be retrieved by name, no matter where that name is actually stored.
Each @t{FrSymbol} may have a @t{FrFrame} associated with it, in which case
the @t{FrSymbol} is the name of that frame.

@indextype{FrList}
@indextype{FrCons}
A @t{FrList} is the basic data type used to store values in a frame.  It
contains a sequence of @t{FrObject}s, which may naturally be
@t{FrList}s themselves.  The type @t{FrCons} is similar to @t{FrList},
joining two arbitrary objects, whereas @t{FrList} joins an arbitrary
object to the front of a shorter list.

@indextype{FrNumber}
@indextype{FrInteger}
@indextype{FrFloat}
Numbers may be included in lists and frames by creating an object of the
appropriate subclass of type @t{FrNumber}: an @t{FrInteger} for any value that
may be held in a @t{signed long}, or a @t{FrFloat} for any other number that
may be held in a @t{double}.

@section{Enumerated Types}
@label{enumtypes}
@index{enumerated types}

In addition to the class hierarchy described in the previous section,
FramepaC exports a number of enumerated types.

@indextype{FrBool}
@index{True}
@index{False}
Most predicates return a boolean value of type @t{FrBool}, which may be:
@begin{verbatim}
   False
   True
@end{verbatim}

@indextype{ObjectType}
The type of an object may not be known until run time, so each object
can return its own type as a value of @t{ObjectType} using the
@t{objType()} method:
@begin{verbatim}
   OT_FrObject,
   OT_Frame,
   OT_VFrame,
   OT_VFrameInfo,
   OT_FrAtom,
   OT_FrCons,
   OT_FrNumber,
   OT_FrString,
   OT_FrSymbol,
   OT_FrList,
   OT_FrStruct,
   OT_FrFloat,
   OT_FrInteger,
   OT_FrQueue,
   OT_FrArray,
   OT_FrBitVector,
   OT_FrHashTable,
   OT_FrHashEntry,
   OT_FrStream,
   OT_FrISockStream,
   OT_FrOSockStream,
   OT_FrSockStream,
   OT_FrWidget,
   OT_FrWSeparator,
   OT_FrWFrame,
   OT_FrWArrow, OT_FrWArrowG,
   OT_FrWLabel,
   OT_FrWRowColumn,
   OT_FrWList,
   OT_FrWOptionMenu,
   OT_FrWPopupMenu,
   OT_FrWPulldownMenu,
   OT_FrWPromptPopup,
   OT_FrWDialogPopup,
   OT_FrWPushButton,
   OT_FrWPushButtonG,
   OT_FrWToggleButton,
   OT_FrWCascadeButton,
   OT_FrWForm,
   OT_FrWSlider,
   OT_FrWButtonBar,
   OT_FrWText,
   OT_FrWShadowText,
   OT_FrWFramePrompt,
   OT_FrWFrameCompleter,
   OT_FrWTextWindow,
   OT_FrWMainWindow,
   OT_FrWScrollWindow,
   OT_FrWScrollBar,
   OT_FrWRadioBox,
   OT_FrWSelectionBox,               
   OT_FrWProgressIndicator,
   OT_FrWProgressPopup,
@end{verbatim}

@indextype{InheritanceType}
@index{depth-first search}
@index{breadth-first search}
When a value is not present in the frame being accessed, it may
optionally be retrieved from another frame according to the current
inheritance method, which is one of the values in @t{InheritanceType}:
@begin{verbatim}
   NoInherit,        // no inheritance
   InheritSimple,    // only look at first filler of IS-A/INSTANCE-OF slots
   InheritDFS,       // depth-first search on IS-A
   InheritBFS,       // breadth-first search on IS-A
   InheritPartDFS,   // depth-first search on PART-OF
   InheritPartBFS,   // breadth-first search on PART-OF
   InheritUser,      // call user-provided function for inheritance
and optionally
   InheritLocalDFS,  // follow slot's INHERITS facet before doing DFS
   InheritLocalBFS,  // follow slot's INHERITS facet before doing BFS
@end{verbatim}

@indextype{DemonType}
@index{demons}
The optional support for demons defines an enumerated type
@t{DemonType} to specify which of multiple different kinds of demons
is to be added or removed.  The different demon types are
@begin{verbatim}
   DT_IfCreated         // the indicated slot/facet was newly created
   DT_IfAdded           // the indicated filler is about to be added
   DT_IfRetrieved       // the indicated slot/facet has been retrieved
   DT_IfMissing         // the indicated slot/facet needs inheritance
   DT_IfDeleted         // the indicated filler has just been deleted
@end{verbatim}

@section{Miscellaneous Types}

@indextype{FrChar_t}
When accessing @t{FrString}s, the characters in the string may be 8, 16,
or 32 bits wide.  The type @t{FrChar_t} will hold any character in any
@t{FrString}.

@indextype{FrCompareFunc}
Many functions take an optional @t{FrCompareFunc} to determine whether two
@t{FrObject}s are equal.  Such a comparison function is defined as
@begin{verbatim}
  FrBool func(const FrObject *obj1, const FrObject *obj2) ;
@end{verbatim}

@indextype{FrIteratorFunc}
Many of the iteration functions take a function argument of type
@t{FrIteratorFunc} which is called on each iteration.  This iteration
function is defined as
@begin{verbatim}
  FrBool func(const FrObject *, va_list) ;
@end{verbatim}

@indextype{FrListMapFunc}
The @t{mapcar} and @t{mapcan} functions require a mapping function of
type @t{FrListMapFunc}, which is
@begin{verbatim}
  FrObject *func(const FrObject *obj, va_list args) ;
@end{verbatim}

@indextype{DemonFunc}
Demon functions have a particular signature, which is defined as type
@t{DemonFunc}, and is
@begin{verbatim}
   FrBool func(const FrSymbol *frame, const FrSymbol *slot,
                const FrSymbol *facet, const FrObject *filler,
                va_list args) ;
@end{verbatim}
The @t{filler} argument is used only by the IfAdded and IfDeleted types
of demon, and will be 0 for all other types of demons.

@indextype{frame_update_hookfunc}
When calling @t{synchronize_VFrames}, you may optionally provide a function
to be called as each frame is stored; such a function has the following
signature:
@begin{verbatim}
   void frame_update_hookfunc(FrSymbol *framename) ;
@end{verbatim}

@comment{---------------------------------------------------------------}

@chapter{Programming with FramepaC Functions}

This chapter provides an introduction to the use of FramepaC functions;
a complete listing of the available functions is in Chapter
@ref{FuncRef}.  FramepaC has both a procedural interface and an
object-oriented one; with a few exceptions, the procedural interface
provides a subset of the object-oriented interface's functionality (in
fact, most of the procedural interface is a simple veneer or "shim"
that converts the procedural calls into object method calls).  This
chapter will concentrate on the procedural interface.

@indexfunc{initialize_FramepaC}
The very first FramepaC function a program must call is
@t{initialize_FramepaC}, which prepares FramepaC for operation.  Any
attempt to call other functions before this one is considered an
error, with consequences ranging from none to a system crash.

@section{Starting Out: Symbols, Lists, and Frames}
@label{starting_out}

In order to work with @t{FrSymbol}s, one must first generate or find some.
Consider the following code fragment:
@indexfunc{makeSymbol}
@indexfunc{findSymbol}
@begin{programexample}
FrSymbol *sym1 = makeSymbol("TEST") ;
FrSymbol *sym2 = findSymbol("FOO") ;
@end{programexample}
The first line in the example finds or creates the symbol whose name
is TEST, and sets @t{sym1} to point at that symbol.  The second line
attempts to find the symbol whose name is FOO; if no such symbol has
been created yet, it sets @t{sym2} to 0.  Note that, unlike the
FramepaC reader (discussed below), neither @t{makeSymbol} nor
@t{findSymbol} convert the string to uppercase--the exact string
specified, including any special characters, becomes the symbol's name.

@indexoper{>>}
Two other methods of generating a symbol are to read one into FramepaC
with either the overloaded @t{>>} operator (discussed in more detail
in Section @ref{reader}) or @t{string_to_FrObject}, or to create a
unique symbol with @t{gensym}.  For example,
@begin{programexample}
FrObject *object ;
FrSymbol *symbol ;
char *charptr = "SYMBOL1 DATA DATA DATA" ;

cin >> object ;
object = string_to_FrObject(charptr) ;
@indexfunc{gensym}
symbol = gensym("BASENAME") ;
@end{programexample}
Here, the first two functions return a @t{FrObject} which happens to be
a symbol (which can be tested using the function @t{SYMBOLP}), and the
last line returns a symbol whose name is of the form @t{BASENAME1234}.

The three main methods of creating a list are the functions @t{pushlist}
and @t{makelist} and the @t{FrList} constructor.  @t{pushlist}
constructs a list one element at a time, while the other two methods
require that all elements in the list be given at once (@t{makelist}
takes an arbitrary zero-terminated sequence of @t{FrObject}s, while
the @t{FrList} constructor takes a maximum of four items).  The
following code fragment builds three lists, each consisting of the
objects pointed at by @t{obj1}, @t{obj2}, and @t{obj3}, in that order.

@indexfunc{pushlist}
@indexfunc{makelist}
@indexctor{FrList}
@begin{programexample}
FrList *l1, *l2, *l3 ;
FrObject *obj1, *obj2, *obj3 ;

...
// set obj1, obj2, and obj3
...
l1 = 0 ;
pushlist(obj3,l1) ;     // each pushlist() adds one item to
pushlist(obj2,l1) ;     // the front of the list, so the calls
pushlist(obj1,l1) ;     // must be in the opposite order
l2 = makelist(obj1,obj2,obj3,(FrObject*)0) ;
l3 = new FrList(obj1,obj2,obj3) ;
@end{programexample}

Frames are rather complex structures, so building one from scratch
programmatically takes a number of function calls.  They may also be
read into memory from a stream or character string, using either a
native textual representation or a FrameKit-compatible @t{MAKE-FRAME}
or @t{MAKE-FRAME-OLD} representation.

To build a frame, one must first create it, and then add the various
fillers it contains (the necessary slots and facets to hold the fillers
are created automatically as required).  Creating a frame is done with
a call such as
@begin{programexample}
FrFrame *fr = create_frame(makeSymbol("TEST-FRAME")) ;
@end{programexample}
after which fillers may be added either singly or an entire list at a
time:
@begin{programexample}
add_filler(fr,makeSymbol("IS-A"),makeSymbol("VALUE"),
                makeSymbol("OTHER-FRAME")) ;
add_fillers(fr,makeSymbol("DATA"),makeSymbol("VALUE"),list_of_data) ;
@end{programexample}

Reading in a frame is done using the same functions used for reading
in other types of objects; the functions described in Section
@ref{polymorphism} may be used to determine what kind of an object was
read, i.e.
@indexfunc{string_to_FrObject}
@indexfunc{FRAMEP}
@begin{programexample}
FrObject *object ;

cin >> object ;
if (FRAMEP(object)) ...
object = string_to_FrObject(charptr) ;
if (FRAMEP(object)) ...
@end{programexample}
if the frame is in native format.  FramepaC can also manipulate frames
which were printed in FrameKit's @t{MAKE-FRAME} or @t{MAKE-FRAME-OLD}
formats.  In this case, the read function will return a list which may
be converted to an actual frame with the function
@t{FrameKit_to_FramepaC}@indexfunc{FrameKit_to_FramepaC}.  As a code
fragment, this operation would be
@indexfunc{FrameKit_to_FramepaC}
@indexfunc{listhead}
@indexfunc{makeSymbol}
@indexfunc{CONSP}
@indexfunc{free_object}
@begin{programexample}
FrObject *object ;
FrFrame *frame ;

cin >> object ;
if (CONSP(object) && (listhead(object)==makeSymbol("MAKE-FRAME") ||
                      listhead(object)==makeSymbol("MAKE-FRAME-OLD")))
   {
   frame = FrameKit_to_FramepaC(object) ;
   free_object(object) ;
   }
@end{programexample}

@indexoper{<<}
The overloaded operator @t{<<} for streams may be used to output any
FrObject.  This facility is somewhat simplistic in the current
version, and performs a minimum of pretty-printing and no circularity
checks.  Therefore, lists will not wrap to fit on screen and any FrCons
which ultimately points at itself will result in an infinite loop.

@section{Dealing with Polymorphism}
@label{polymorphism}
@index{polymorphism}

Since FramepaC uses C++'s polymorphism to allow an object of a more
general type to actually contain a more specific type of object, we
need a way to determine the exact type of an object.  For this
purpose, FramepaC provides a number of type determination functions.

@indexfunc{ATOMP}
@indexfunc{CONSP}
@indexfunc{SYMBOLP}
@indexfunc{STRINGP}
@indexfunc{NUMBERP}
@indexfunc{is_frame}
@indexmeth{objType}
@t{ATOMP()}, @t{CONSP()}, @t{SYMBOLP()}, @t{STRINGP()}, @t{NUMBERP()},
and @t{FRAMEP()} return @t{True} if the @t{FrObject} they are passed is,
respectively, a FrAtom (symbol, string, or number), a FrCons (cons cell
or list), a symbol, a string, a number (either FrInteger or FrFloat), or a
frame located in memory.  Additionally, the function @t{is_frame}
determines whether the @t{FrSymbol} it is given is the name of a frame,
and the method @t{objType} returns a value indicating the exact type of
the object.

Consider the case where the user is asked to enter an object, and one
function (requiring a symbol) is called if the user enters a symbol,
and another one (able to accept any type of object) is called if it is
not a symbol:
@begin{programexample}
FrObject *object ;

cin >> object ;
if (SYMBOLP(object))
   functionA((FrSymbol *)object) ;
else
   functionB(object) ;
@end{programexample}
or
@begin{programexample}
FrObject *object ;

cin >> object ;
if (object && object->objType() == OT_Symbol)
   functionA((FrSymbol *)object) ;
else
   functionB(object) ;
@end{programexample}

@section{The FramepaC Reader}
@label{reader}
@index{reader}
@index{FramepaC reader}

Because reading and writing Lisp-style objects is a common operation,
FramepaC provides a set of functions to do just that.  Note, however,
that the FramepaC reader is not a complete implementation of a Lisp
reader, so some constructs will not work or produce unexpected results
when given data intended for a Lisp reader@index{Lisp reader}.

@index2{p="FramepaC reader",s="comments"}
@index{comments}
The first character of an object is used to determine its type.  A
left parenthesis indicates a @t{FrList}; a digit indicates a @t{FrNumber},
as does a plus or minus sign (unless the following character is a
non-digit; whether the number is an @t{FrInteger} or a @t{FrFloat} is
determined by examining the number for a decimal point and/or
exponent); a left bracket indicates a @t{FrFrame}; a double, single, or
back- quote indicates a @t{FrString} consisting of eight-, sixteen-,
or 32-bit characters, respectively; a hash mark ('#') indicates the
start of a Lisp-style form; a semicolon indicates the start of
a comment running to the end of the line; and an alphabetic character,
vertical bar, or one of a number of special symbols indicates a
@t{FrSymbol}.  Any other non-whitespace character will be returned as a
one-character @t{FrString}.

The native format for a frame is
@index2{p="frames",s="native format"}
@begin{programexample}
[<framename> <numslots>
   [<slotname>  [<facetname> <filler> ...]
                [<facetname> <filler> ...]
                ...]
   [<slotname>  [<facetname> <filler> ...]
                [<facetname> <filler> ...]
                ...]
   ...]
@end{programexample}
where @t{<numslots>} is the number of slots listed in the remainder of
the frame representation.  This number is optional and may be omitted
when writing frames, but is generated on output to make reading the
frame back into FramepaC more efficient.  In the special case of zero
slots, i.e.
@begin{programexample}
[FOO 0]
@end{programexample}
or
@begin{programexample}
[FOO]
@end{programexample}
the frame @t{FOO} is created if it does not already exist, but not
modifed if it does exist.

FramepaC can also process frames in FrameKit format, though it reads
them as lists rather than directly into frames.  The function
@t{FrameKit_to_FramepaC} will convert a list in FrameKit "make-frame"
or "make-frame-old" format into a FramepaC frame.  The two FrameKit
formats are
@begin{programexample}
(MAKE-FRAME <framename>
    (<slot1> (<facet1> (<view1> <filler> ...) (<view2> <filler> ...))
             (<facet2> (<view1> <filler> ...) ...))
    (<slot2> ...)
    ...)
@end{programexample}
and
@begin{programexample}
(MAKE-FRAME-OLD <framename>
    (<slot1> (<facet1> <filler> ...) (<facet2> <filler> ...) ...)
    (<slot2> (<facet1> <filler> ...) ...)
    ...)
@end{programexample}
where the view for @t{MAKE-FRAME-OLD} is assumed to be @t{COMMON}.

@index2{p="symbols",s="reading"}
@index2{p="symbols",s="and special characters"}
When reading in symbols, FramepaC converts their names to uppercase by
default.  This may be overridden (and characters not normally
considered to be part of a symbol's name may be embedded) by
surrounding the symbol's name with 'pipe' or vertical bar characters
('|').  If the name is quoted in this way, it may contain any character
except NUL or vertical bar, including whitespace and newlines.  Any
symbols whose names contain lowercase letters or characters not
normally considered part of a symbol name will be quoted in this manner
when they are printed.  Note that the functions @t{makeSymbol} and
@t{findSymbol}, used to access symbols by name, do not perform any
uppercasing on the names they are given, and will include any vertical
bar characters as part of the name if present (this is the only way to
create a symbol name containing a vertical bar, which makes them
useful for internal symbols which are never printed out).  Examples:
@begin{programexample}
CAR             ; symbol with name "CAR"
car             ; symbol with name "CAR" -- same as previous
|CAR|           ; symbol with name "CAR" -- same as first
|car|           ; symbol with name "car"
car one         ; two symbols: "CAR" and "ONE"
|car one|       ; symbol with name "car one"
+               ; symbol with name "+"
+1              ; number--not a symbol
@end{programexample}

@index2{p="strings",s="reading"}
Strings may be fed to FramepaC using either single or double quotes or
backquotes.  The type of quotes used indicates the size of the
characters in the string (though the printed representation of the
string always breaks everything into eight-bit bytes): a double quote
is a string consisting of normal eight-bit characters, a single quote
is a string of 16-bit characters (e.g. Unicode), and a backquote is a
string of 32-bit characters (e.g. ISO 10646).  A backslash is used to
quote the following character (allowing quotes to be embedded);
thus, to enter a literal backslash into the string, it must be
doubled.  One special case is recognized so that NUL characters may be
embedded in a string--if the backslash is followed by the character
'0' (zero), the two-character sequence is replaced by an ASCII NUL
(the character whose value is binary zero).  Examples:
@index2{p="strings",s="16-bit characters"}
@index2{p="strings",s="32-bit characters"}
@begin{programexample}
"abc"           ; 8-bit characters, 'a', 'b', and 'c'
"abc\0"         ; 8-bit characters, 'a', 'b', 'c', and NUL
'\0a\0b'        ; 16-bit characters, 'a' and 'b'
`\0\0\0a`       ; 32-bit character 'a'
"\"\\"          ; 8-bit characters, double quote and backslash
@end{programexample}

@index2{p="numbers",s="reading"}
Numbers are automatically stored in the proper type of @t{FrNumber}
object, either a @t{FrInteger} or a @t{FrFloat} depending on the number's
format.  If the number contains no decimal point and no exponent, it
is stored as an integer; if it contains either or both, it is stored
as a floating-point number.  Examples:
@begin{programexample}
1               ; integer, read as an object of type FrInteger
1.0             ; floating-point number, read as a FrFloat
+1.5            ; positive one point five
-1              ; integer, minus one
1.5e6           ; floating-point 1,500,000
-2E-3           ; floating-point -0.002
@end{programexample}

@index2{p="Lisp forms",s="reading"}
The hash character ('#') introduces a Lisp-style form.  FramepaC can
interpret the most common forms, although several are only approximated
(e.g. a #A() array is read as a simple list, since FramepaC does not
have an array type).  Within a Lisp-style form, only eight-bit strings
can be read, since the single quote and backquote are interpreted as Lisp
would, namely as abbreviations of (QUOTE x) and (BACKQUOTE x).

FramepaC supports the following Lisp and Lisp-style forms:
@begin{itemize}
@b{#()}@*
@index{vectors}
Vector.  This form is read as a simple @t{FrList}.  As in Lisp, an
infix argument may be given to specify the length of the vector; in
this case, the list is truncated to the specified length if longer, or
extended by duplicating the last element if shorter than specified.

@b{#`}@*
Function.  This form returns the @t{FrList} (FUNCTION x), where @b{x}
is the object immediately following the @i{#`}.  For example, #`TEST would
return (FUNCTION TEST).

@b{#\}@*
@index{character objects}
Character.  Returns a @t{FrSymbol} whose one-character name is the
character immediately following the backslash, regardless of what that
character may be (whitespace, control character, etc.), and without
converting the character to uppercase.  For example, #\a returns the
@t{FrSymbol} |a|.

@b{#:}@*
Uninterned Symbol.  Since FramepaC has no notion of interned vs.
uninterned, this form simply returns the symbol immediately following
the @i{#:}.

@b{#|}@*
@index{comments}
Balanced Comment.  Ignore all further input until a @i{|#} sequence is
encountered.  Unlike Lisp, FramepaC does not support nested balanced
comments; the first @i{|#} encountered ends the comment.

@b{#@i{n}=}@&
@index{shared objects}
Shared Object Definition.  Returns the object immediately following the
@i{#=}, but also stores it in an association list under number @i{n}
for later reference by a @i{#n#} within the same list or Lisp form.

@b{#@i{n}#}@*
Shared Object Reference.  Returns a pointer to a @i{copy} of the object
defined by a @t{#n=} earlier in the same list or Lisp form.  Since
FramepaC does not have garbage collection, the structure sharing
implied by #= and ## is not implemented to avoid serious problems and
complications in deallocating the object returned by a read operation.

@b{#A()}@*
@index{arrays}
Array.  This form is read as a simple @t{FrList}.

@b{#B@i{nnn}}@*
@index{binary numbers}
Binary rational number.  If the string of binary digits following the
#B contains a forward slash, the return value is a @t{FrFloat}
containing the ratio of numerator and denominator; otherwise, the
return value is a @t{FrInteger}.

@b{#H()}@*
@index{hash tables}
Hash Table.  The items between the parentheses are interpreted as the
entries in a @t{FrHashTable}.

@b{#HEntryObj()}@*
Hash Table Entry.  The value within the parentheses is interpreted as
the key for a @t{FrHashEntryObject}.

@b{#O@i{nnn}}@*
@index{octal numbers}
Octal rational number.  If the string of octal digits following the
#B contains a forward slash, the return value is a @t{FrFloat}
containing the ratio of numerator and denominator; otherwise, the
return value is a @t{FrInteger}.

@b{#Q()}@*
@index{queues}
Queue.  The items between the parentheses are interpreted as the
contents of a @t{FrQueue}, from head to tail.

@b{#@i{m}R@i{nnn}}@*
Variable-radix rational number.  'm' is a number between 2 and 36
indicating the radix in which to read the digits 'nnn' following the
#R.  The digit sequence may include a forward slash, in which case the
returned value is the ratio of the indicated numerator and denominator.

@b{#S()}@*
@index{records}
@index{structures}
Structure.  The contents between the parentheses are interpreted as
field-name and field-value pairs, as in Lisp.  The return value is a
@t{FrStruct}. 

@b{#X@i{nnn}}@*
@index{hexadecimal numbers}
Hexadecimal rational number.  If the string of hexadecimal digits
following the #X contains a forward slash, the return value is a
@t{FrFloat} containing the ratio of numerator and denominator;
otherwise, the return value is a @t{FrInteger}.
@end{itemize}
@index{types which can be read}
@index{reading data}
@index2{p="troubleshooting",s="inability to read an object type"}
Any sequence introduced by a hash ('#') which is not listed above, or
for which the code implementing the type of the return value has not
been linked into the executable, is returned as a @t{FrSymbol} with the
appropriate name (including the leading hash character).  By only
linking in the readers for types which are actually used by the
program, the size of the executable is minimized and the application
will not receive objects which might confuse it because they are
unknown to the application's code.

@index{packages}
It is important to remember that the FramepaC reader is not a full Lisp
reader, so various constructs such as #A() for arrays will not be read
properly (though in most cases, an approximation will be made, e.g.
arrays are read as lists and rationals as floats).  Since FramepaC does
not have anything corresponding to Lisp's packages (which allow
multiple namespaces which are still simultaneously accessible), symbols
of the form "PACKAGE:SYMBOLNAME" will be stored with precisely that
name in the current symbol table.

Like Lisp's reader, FramepaC's is designed to be user-extensible,
either at compile-time or at run-time.  Compile-time extension may be
achieved by instantiating objects of the @t{FrReader} class; run-time
extension is achieved by explicitly setting the functions to be called
when specified characters are encountered or by creating additional
instances of @t{FrReadTable} (see the appropriate sections of Chapter
@ref{funcref} for details).  For example, to transparently convert
FrameKit frames into FramepaC frames when reading, one could use
@begin{programexample}
FrReadStreamFunc list_reader ;
FrObject *list_or_frame(istream &in, const char *digits)
{
   FrObject *list = list_reader(in,digits) ;
   if (list && && list->consp() && ((FrList*)list)->first())
      {
      FrObject *head = ((FrList*)list)->first() ;
      if (head == makeSymbol("MAKE-FRAME") ||
          head == makeSymbol("MAKE-FRAME-OLD"))
         {
         FrFrame *frame = FrameKit_to_FramepaC(list) ;
         list->freeObject() ;
         return frame ;
         }
      }
   return list ;
}

....
list_reader = FramepaC_readtable->getStreamReader('(') ;
FramepaC_readtable->setReader('(',list_or_frame) ;
....
@end{programexample}


@section{Using Virtual Frames}
@index{virtual frames}

Virtual frames allow you to store your frames in a file or on a server
and retrieve them on demand.  Using virtual frames requires a few
changes from using regular frames, but can mostly be treated identically with
regular frames (for examples, regular and virtual frames may be mixed
freely).

First, the virtual-frame functions do not take actual frames as
arguments, but the @i{names} of the frames.  Thus, all variables which
would be of type @t{FrFrame*} for regular frames are instead of type
@t{FrSymbol*}.  Only in exceptional cases would an actual pointer to a
frame be used, in which case its type can be either @t{VFrame*} or
@t{FrFrame*} (since @t{VFrame} is a subclass of @t{FrFrame}).

@index2{p="virtual frames",s="initializing"}
@index2{p="backing store",s="initializing"}
@index{initialization}
Second, the virtual-frame system must be initialized.  Thus, the
program must call either @t{initialize_VFrames_memory},
@t{initialize_VFrames_disk}, or @t{initialize_VFrames_server} before
using virtual frames.  These initialization functions set up the
backing store and load in an index of the existing frames so that
FramepaC knows which frames should be fetched when accessed and which
need to be newly created.  Before the program exits, it must call
@t{shutdown_VFrames} to ensure that all necessary updates to the
backing store are performed.

@index2{p="backing store",s="updating"}
Finally, there should be periodic calls to @t{synchronize_VFrames} or
@t{commit_frame} to ensure that changes are stored back in the file or
on the server in a timely manner.  Although any modified frames will
be stored when @t{shutdown_VFrames} is called, regular calls to
enforce updates of the backing store will minimize the number of
updates lost in case of a crash, and may be required in multi-user
situations when others are accessing the same backing store.

@section{Manipulating Lists}

Section @ref{starting_out} showed how to create a list, but did not go
into details on how to use lists.  This section will do just that.

As in Lisp, a list consists of a series of FrCons cells connecting two
other items.  The left half of the FrCons is the first item in the list,
while the right half is the remainder of the list.  Not surprisingly,
then, FramepaC provides functions to get both the first item in the
list and the remainder after removing the first item, called
@t{listhead} and @t{listtail}.  The empty list is a simple null
pointer, i.e. 0, and both head and tail of the empty list are defined
to be 0 (the empty list) for convenience.

The code fragment
@indexfunc{listhead}
@indexfunc{listtail}
@begin{programexample}
FrList l = new FrList(new FrString("first"),
                  new FrString("second"),
                  new FrString("third")) ;
cout << "The first item in 'l' is " << listhead(l) << endl ;
cout << "and the remainder is " << listtail(l) << endl ;
@end{programexample}
will produce the output
@begin{programexample}
The first item in 'l' is "first"
and the remainder is ("second" "third")
@end{programexample}

There are two main ways of processing each element of a list,
depending on whether the list is to be discarded as it is processed.
If the list is a temporary list or a copy which is not pointed at by
any other variables, the function @t{poplist} will simultaneously
return the first element of the list and remove it from the list,
deallocating the list's first FrCons cell.  To print out each element of
a list, one per line, you would use a loop such as
@begin{programexample}
while (lst)
   {
   FrObject *item = poplist(lst) ;
        ...
   }
@end{programexample}
On the other hand, if the list must be preserved, the usual method
would be
@begin{programexample}
while (lst)
   {
   FrObject *item = listhead(lst) ;
        ...
   lst = listtail(lst) ;
   }
@end{programexample}
or
@begin{programexample}
for (FrList *lst = somelist ; lst ; lst = listtail(lst))
   {
   FrObject *item = listhead(lst) ;
        ...
   }
@end{programexample}

The functions which return entire facets from a frame (@t{get_fillers},
@t{get_values}, and @t{get_sems}) return the actual lists stored in the
frame.  Thus, the returns lists must not be destructively modified.  If
it is necessary to destructively modify such a list of fillers, you
must first copy the list, and then operate on the copy.  The two
functions provided for copying lists are @t{copylist}
(@t{FrList::copy}) and @t{copytree} (@t{FrList::deepcopy}), which
perform shallow and deep copies, respectively.  @t{copylist} will only
copy the top-most level of the list, so the elements pointed at in the
copy will be the actual elements from the original list; in contrast,
@t{copytree} also copies the elements in the list, so the returned copy
will be completely separate from the original list.  In general,
@t{copytree} is the preferred function; @t{copylist} is typically only
useful when the copy will be destroyed with @t{poplist}, which deletes
the nodes of the list but not the elements they point at.
@index{copytree}
@index2{p=copylist,s="usage of"}

!!!

Other useful functions include @t{FrList::listlength}, @t{FrList::member}, and
@t{listreverse}.  The first function, @t{FrList::listlength}, returns the
number of items at the top level of the list (skipping any items in
embedded lists).  Determining whether some item is present in the list
is done with @t{FrList::member}, which returns a pointer to the
end-portion of the list with the desired item as its head, or 0 if the
item is not in the list.  Finally, @t{listreverse} destructively
reverses the items in the list, returning a pointer to the new
beginning of the list.
@indexfunc{listlength}
@indexmeth{listlength}
@indexfunc{listmember}
@indexmeth{member}
@indexfunc{listreverse}
@begin{programexample}
FrList *l = new FrList(symbolONE, symbolTWO, symbolTHREE, symbolFOUR);
cout << "The list's length is " << l->listlength() << " items" <<endl;
cout << "The tail from THREE is " << l->member(symbolTHREE)
     << endl ;
l = listreverse(l) ;  /* must store new head, or lose most of list */
cout << "The reversed list is " << l << endl ;
l->freeObject() ;
@end{programexample}
would produce the following output:
@begin{programexample}
The list's length is 4 items
The tail from THREE is (THREE FOUR)
The reversed list is (FOUR THREE TWO ONE)
@end{programexample}

Note that if all you want to do is determine whether a list is empty or
not, it is easier and much more efficient to simply check whether the pointer
is NULL rather than to call @t{listlength}, i.e.
@begin{programexample}
if (list == 0)
   ...
@end{programexample}
or
@begin{programexample}
if (!list)
   ...
@end{programexample}
instead of
@begin{programexample}
if (list->listlength() == 0)
   ...
@end{programexample}

@section{Manipulating Frames}

Obviously, once a frame has been created as described in Section
@ref{starting_out}, one must be able to do more with it than merely
print it out.  This section covers a variety of ways to access and
modify frames.

@index{accessing frames}
@index{retrieving fillers}
A frame would not be of much use if it were not possible to retrieve
previously-stored information from the frame.  The main function to do
so is @t{get_fillers}, which returns the list of all items stored in a
particular facet within a given slot.  For efficiency, the @t{VALUE}
and @t{SEM} facets may be retrieved directly with @t{get_values} and
@t{get_sems}, respectively.  Since one is commonly interested only in
the first filler in a facet, the variants @t{first_filler},
@t{get_value}, and @t{get_sem} are available to return the first
filler, and are functionally equivalent to
@t{listhead(get_fillers(...))}, etc.  For example,
@begin{programexample}
cout << "The list of SEM fillers in the frame's TEST slot is "
     << get_sems(frame,makeSymbol("TEST")) << endl ;
cout << "The first VALUE filler in the frame's TIME slot is "
     << get_value(frame,makeSymbol("TIME")) << endl ;
cout << "The DEFAULT facet in the frame's DATA slot is "
     << get_fillers(frame,makeSymbol("DATA"),makeSymbol("DEFAULT"))
     << endl ;
@end{programexample}

@index{erasing frame fillers}
The primary means of changing a frame is to delete one or more fillers
and then optionally add new ones.  The set of functions to erase
portions of a frame includes @t{erase_filler}, @t{erase_value},
@t{erase_sem}, @t{erase_facet}, and @t{erase_slot}.  The first three
functions remove a specific item from an arbitrary facet, the
@t{VALUE} facet, and the @t{SEM} facet, respectively.  The fourth,
@t{erase_facet} removes all fillers from a particular facet, while the
last, @t{erase_slot}, removes all fillers from all facets of the
indicated slot.
@begin{programexample}
/* remove XYZZY from the VALUE facet of the TEST slot */
erase_value(frame,makeSymbol("TEST"),makeSymbol("XYZZY")) ;
/* remove all fillers from the SEM facet of the DATA slot */
erase_facet(frame,makeSymbol("DATA"),makeSymbol("SEM")) ;
/* remove the entire TIME slot and all its facets */
erase_slot(frame,makeSymbol("TIME")) ;
@end{programexample}

Three functions are provided to process portions of a frame with no
prior knowledge of the slots and facets in the frame, as well as two
functions to retrieve a list of the slots and facets in the frame.
These are @t{do_facets}, which iterates over the facets of a slot;
@t{do_slots}, which iterates over the slots in a frame;
@t{do_all_facets}, which iterates over all facets of each and every
slot in the frame; @t{slots_in_frame}, which returns a list of the
slots in the frame; and @t{facets_in_slot}, which returns a list of the
facets in a particular slot .  Each of these takes a pointer to a
function as one of its arguments, and invokes the specified function
for each slot or facet it iterates over.  Additional parameters may be
given to the @t{do_*} functions, which are passed through unchanged to
the user function in the form of a variable argument list
(@t{va_list}), which the standard macro @t{va_arg}@indexmacro{va_arg}
can decompose into the individual parameters.  For example, consider
the following brief function to print all of the slot and facet names
in a frame to a specified stream:
@begin{programexample}
FrBool show_facet_name(const FrFrame *frame, const FrSymbol *slot,
                     const FrSymbol *facet, va_list args)
{
   ostream *out = va_arg(args,ostream*) ;
   *out << slot << " " << facet << endl ;
   return True ;
}

        ...
        do_all_facets(frame,show_facet_name,&cout) ;
        ...
@end{programexample}

@section{Socket Streams}

For convenience in creating distributed applications, FramepaC provides
three classes which extend the standard I/O streams to functions over
BSD Unix-style network sockets: @t{FrISockStream}, @t{FrOSockStream},
and @t{FrSockStream}.  These are derived from, respectively,
@t{istream}, @t{ostream}, and @t{iostream}.

Once opened, socket streams may be used in exactly the same manner as
any other simple stream.  Thus -- other than a few lines of setup --
the identical source code may be used whether your program reads/writes
standard input/output or is acting as a network server.

!!!


@section{A More Complex Example}

!!!

@section{Efficiency Considerations}
@label{efficiency}
@index{efficiency}

If you have a constant symbol which will be used repeatedly in the
course of execution (i.e. @t{VALUE} or @t{SEM}), you can save the overhead of
looking up the symbol each time by storing the result of a
@t{makeSymbol()} or @t{findSymbol()} in a variable at the beginning of
the program run, such as in an initialization function.  Then, use the
value of that variable whenever you need that particular
symbol--symbols are guaranteed never to change while the symbol table
containing them remains in existence.  FramepaC uses this approach
internally, with variables such as @t{symbolISA} and @t{symbolVALUE}
(which are not accessible outside FramepaC, however).

@index2{p="symbol tables",s="switching"}
If you switch symbol tables, you will need to refresh the variables
you use to store symbols, since symbols with the same names will
differ between symbol tables, and can not validly be compared.

Virtual frames are somewhat slower than regular frames even after they have
been loaded into memory, because there is an extra level of lookups on each
frame access.  Thus, it can be useful in time-critical code to force a
virtual frame which is accessed multiple times in a row into memory, and
then operate on the frame as if it were a regular frame.  In a multi-user
environment, where other users might access the same backing store, the
frame should be locked while these manipulations are in progress.  The
function @t{lock_frame} will simultaneously lock the frame and force it
into memory, returning a pointer to the actual frame.

@comment{---------------------------------------------------------------}

@chapter{Demons}
@label{demons}

FramepaC can optionally support daemon functions which are activated
whenever a particular slot is created, deleted, accessed, modified, or
found to be absent.  Because the support for demons slows down the
entire system (by about 10 percent even when no demons have been
defined, more when demons are active@foot{In FrameKit, the overhead
imposed by checking for demons was far larger, on the order of 300%
for the faster functions such as @t{get-fillers}; FramepaC uses a
different and much faster method of storing and checking for
demons.}), a compile-time option specifies whether to include the
daemon support.  A user program may determine whether demons are
supported by testing for the existence of the preprocessor variable
@t{FrDEMONS}.
@indexpp{FrDEMONS}

Using demons is fairly simple; using demons well is an art form.

!!!

@comment{---------------------------------------------------------------}

@chapter{Memory Allocation}

Virtually all memory allocation in FramepaC is performed through its
own internal memory allocation routines, which are also made available
to applications.  There are two separate but closely-related sets of
routines: a standard, variable-size memory allocator equivalent to
@t{malloc}/@t{free}, and an extremely fast allocator for fixed-size
objects.   To avoid excessive memory fragmentation, it is advisable to
use FramepaC's routines instead of the standard @t{malloc}/@t{free} or
@t{new}/@t{delete}. 

@section{Standard Memory Allocations}
@label{stdmemalloc}

The following convenience macros are available for memory allocations:

@begin{FuncDesc}
@indexmacro{FrNew}
@index2{p="memory allocation",s="FrNew"}
@Func{@t{type}* FrNew(@t{type})}
Allocate an object of the indicated type, and return a pointer to the
newly-allocated object.
@end{FuncDesc}

@begin{FuncDesc}
@indexmacro{FrNewC}
@index2{p="memory allocation",s="FrNewC"}
@Func{@t{type}* FrNewC(@t{type},int)}
Allocate an array of objects of the indicated type, clear the allocated
memory to all zero bytes, and return a pointer to the first object in
the newly-allocated array.
@end{FuncDesc}

@begin{FuncDesc}
@indexmacro{FrNewN}
@index2{p="memory allocation",s="FrNewN"}
@Func{@t{type}* FrNewN(@t{type},int)}
Allocate an array of objects of the indicated type, and return a
pointer to the first object in the newly-allocated array.
@end{FuncDesc}

@begin{FuncDesc}
@indexmacro{FrNewR}
@index2{p="memory allocation",s="FrNewR"}
@Func{@t{type}* FrNewR(@t{type},void *blk,int newsize)}
Resize a previously-allocated block of memory to the specified new size
in bytes.  Returns a pointer to the resized block (which may differ
from the original pointer), or 0 if unable to resize the block (in
which case the original remains untouched).
@end{FuncDesc}

Additional functions, such as @t{FrMalloc} and the like on which the
above macros are based, related to memory allocation are listed in Section
@ref{memalloc}.

@section{Memory Pools}
@label{mempool}

Sometimes, it is useful to have a separate pool of memory from which
allocations are made for a specific purpose.  To this end, FramepaC
provides the class @t{FrMemoryPool}, which essentially provides a
wrapper around the standard memory allocation functions in such a way
that the allocations made through an instance of @t{FrMemoryPool} are
made from different regions of memory than those allocations made
through any other instance of @t{FrMemoryPool} or through @t{FrMalloc}
or the @t{FrNew}X convenience macros.

In addition to providing a separate region of memory for allocation,
instances of @t{FrMemoryPool} may be set to maintain a reserve of
memory which is allocated immediately and never used until a request
for memory cannot be satisfied either from existing blocks in the
memory pool or by reqeusting more memory from the system.  This allows
critical operations to complete successfully even when memory has
otherwise been exhausted.

@begin{FuncDesc}
@indextype{FrMemoryPool}
@indexctor{FrMemoryPool}
@Func{FrMemoryPool::FrMemoryPool(const char *name, int reserve = 0,
                                 int maxblocks = 0)}
Create a new memory pool, specifying a name for identification
purposes, how many blocks of memory to hold in reserve, and the maximum
number of blocks to allocate from the system before attempting to
reclaim memory which has already been allocated.  Each block used by
the memory pool contains @t{FrBLOCKING_SIZE} bytes; allocation request
for more than that amount (less internal overhead) are passed directly
to the system for fulfillment, and will not be affected by either the
reserve or the limit settings.

The @t{reserve} parameter indicates how many blocks to allocate
immediately but not use until requests for additional memory cannot be
satisfied by system memory allocation calls.

The @t{maxblocks} parameter, if nonzero, indicates how many blocks may
be allocated for the memory pool before attempting to reclaim memory.
If the memory pool contains at least @t{maxblocks} allocation units and
a request for memory cannot be satisfied from the existing pool, then
memory compaction and/or garbage collection (depending on capabilities
added by subclasses of @t{FrMemoryPool} such as @t{FrAllocator}) will
be attempted and the system will be asked for additional memory only if
the memory reclamation did not make sufficient memory available.
@end{FuncDesc}     

@begin{FuncDesc}
@indextype{FrMemoryPool}
@indexdtor{FrMemoryPool}
@Func{FrMemoryPool::~FrMemoryPool()}
Delete the memory pool and return any memory it has allocated to the
available global memory for the program.
@end{FuncDesc}     

@begin{FuncDesc}
@indexmeth{allocate}
@Func{void *FrMemoryPool::allocate(size_t size)}
Request a block containing at least @t{size} bytes of memory from the
memory pool.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{release}
@Func{void FrMemoryPool::release(void *item)}
Return the specified item (previously allocated with @t{allocate}) to
the memory pool for re-use.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{reallocate}
@Func{void *FrMemoryPool::reallocate(void *item, size_t newsize)}
Resize the specified memory block @t{item} to contain at least
@t{newsize} bytes.  If it is not possible to expand a memory block in
place, a new allocation will be made and the contents of the existing
memory block copied into the new one.  This function returns the
address of the resulting resized block.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{set_max_blocks}
@Func{void FrMemoryPool::set_max_blocks(int maxblocks)}
Set the limit to the number of allocation units the memory pool may
contain before this instance of @t{FrMemoryPool} attempts to reclaim
memory prior to requesting additional memory from the system.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{reserve_blocks}
@Func{void FrMemoryPool::reserve_blocks(int reserved)}
Set the number of blocks to hold in reserve against the possibility of
the system memory being exhausted.  If the new reserve is larger than
the existing reserve, the necessary additional memory blocks will be
allocated from the system immediately; if smaller, any extraneous
blocks will be returned to the system.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{setCallback}
@Func{void FrMemoryPool::setCallback(FrMemoryPoolCallback *cb)}
Specify a function to be called whenever the status of the memory
reserve changes.  The three occurrences are @t{FrMPCR_UsingReserve} (a
block from the reserve pool was used to satisfy a request),
@t{FrMPCR_ReservingBlock} (a block was freed, but was added to the
reserve pool rather than being returned to the system), and
@t{FrMPCR_IncreasingReserve} (another block was added to the reserve
pool).
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{blocks_allocated}
@Func{int FrMemoryPool::blocks_allocated() const}
Determine how many allocation units are currently in use by the memory
pool (not counting any reserve).
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{bytes_allocated}
@Func{long FrMemoryPool::bytes_allocated() const}
Determine the total number of bytes currently in use by the memory
pool, including any free memory being managed by the pool, but not any
reserve.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{max_blocks}
@Func{int FrMemoryPool::max_blocks() const}
Determine how many allocation units are allowed before the memory pool
attempts to reclaim memory prior to requesting additional memory from
the system.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{reserved_blocks}
@Func{int FrMemoryPool::reserved_blocks() const}
Determine how many allocation units are to be kept in reserve for use
by the memory pool when system memory is exhausted.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{reserve_available}
@Func{int FrMemoryPool::reserve_available() const}
Determine the number of allocation units currently available as a
reserve.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{typeName}
@Func{const char *FrMemoryPool::typeName() const}
Retrieve the name for the instance of the memory pool, which was set at
the time it was created.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{nextPool}
@Func{FrMemoryPool *FrMemoryPool::nextPool() const}
Return the next memory pool in the internal list of instances of
@t{FrMemoryPool}.  This function is not intended to be useful to
applications. 
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{getCallback}
@Func{FrMemoryPoolCallback *FrMemoryPool::getCallback() const}
Retrieve the currently-active callback function for reporting changes
to the memory pool's reserve.
@end{FuncDesc}


@section{The Suballocator}
@label{memsuballoc}
@indextype{FrAllocator}

FramepaC includes its own very fast memory suballocator, and makes this
suballocator available to applications for use with their own objects.
The class @b{FrAllocator}@indextype{FrAllocator} implements a
suballocation strategy within fixed-size blocks.  What this means is
that a block of memory is allocated from the system and treated as an
array of smaller objects, each individually allocatable and stored on a
free list when not actually in use by the application.  This
organization permits the smaller units to be allocated and deallocated
with a handful of inline machine instructions, only rarely calling a
function to allocate another block of memory.

The typical use of @t{FrAllocator} for allocating objects of a
particular class requires three lines in the class' declaration and a
fourth line elsewhere.  In the class definition, one should declare a
static variable of type @t{FrAllocator} and define @t{new} and
@t{delete} in terms of @t{allocate} and @t{release}, as in:

@begin{programexample}
@indexmeth{allocate}
@indexmeth{release}
class Foo
   {
   private:
      static FrAllocator allocator ;
      ...
   public:
      void *operator new(size_t) { return allocator.allocate() ; }
      void operator delete(void *blk) { allocator.release(blk) ; }
      ...
   }

FrAllocator Foo::allocator("Foo",sizeof(Foo)) ;
@end{programexample}

Note that @t{FrAllocator} is not recommended for objects of more than
1000 bytes or so, due to excessive internal fragmentation in the blocks
from which the objects are suballocated.

@begin{FuncDesc}
@indexctor{FrAllocator}
@Func{FrAllocator::FrAllocator(const char *name, int objsize)}
Prepare for suballocations of objects which are each @t{objsize} bytes.
The indicated name is used to identify the suballocator instance in
@t{show_memory_usage}'s output, and should ordinarily indicate the type
of objects being allocated.
@end{FuncDesc}

@begin{FuncDesc}
@indexdtor{FrAllocator}
@Func{FrAllocator::~FrAllocator()}
Release all memory still allocated by the instance of the suballocator.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{allocate}
@Func{void *FrAllocator:allocate()}
Retrieve one object from the suballocator's memory pool and return it
to the caller.  If no more objects are left in the pool, a new block of
memory is allocated from system memory and divided into objects, and
one of the new objects is returned.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{release}
@Func{void FrAllocator:release()}
Place the indicated object back into the suballocator's memory pool.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{blocks_allocated}
@Func{int FrAllocator::blocks_allocated()}
Determine how many memory blocks the given instance of the suballocator
is controlling.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{bytes_allocated}
@Func{int FrAllocator:bytes_allocated()}
Determine the total number of bytes contained in the memory the
suballocator is controlling.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{objects_allocated}
@Func{int FrAllocator:objects_allocated()}
Determine the total number of objects contained in the memory the
suballocator is controlling.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{freelist_length}
@Func{int FrAllocator:freelist_length()}
Determine how many objects are currently available for allocation
without requesting further memory from the system.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{typeName}
@Func{const char *FrAllocator:typeName()}
Retrieve the object type name specified at instantiation time.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{objectSize}
@Func{int FrAllocator:objectSize()}
Determine the size, in bytes, of one object, as specified at
instantiation time.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{nextAllocator}
@Func{FrAllocator *FrAllocator:nextAllocator()}
Follow an internal list of all suballocator instances which are
currently instantiated.  This function is for internal use, and there
is no means of retrieving the first instance on the internal list.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{async_gc}
@Func{int FrAllocator:async_gc()}
(not yet implemented, meant for internal use)
!!!
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{gc}
@Func{int FrAllocator:gc()}
(not yet fully implemented)@*
At this time, @t{gc} performs a memory compaction, returning any entirely
unused suballocator blocks to the global memory pool.

!!!
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{compact}
@Func{int FrAllocator:compact()}
Release any currently-unused memory blocks controlled by the given
instance of the suballocator.
@end{FuncDesc}

@comment{---------------------------------------------------------------}

@chapter{User-Interface Functions}
@label{UserInterface}

When compiled under Unix with X Windows, FramepaC can provide a set of
classes for manipulating Motif user-interface objects.  When compiled
with the @t{FrMOTIF} symbol @i{#define}d in @t{frconfig.h}, the
@t{FrWidget} class and all of its subclasses (as shown in Figure
@ref{class_hier2}) are available, but will only be linked into the
executable if actually used by the application.

Using @t{FrWidget} and its subclasses instead of direct Xlib and Motif
calls can not only simplify programming and potentially improve
portability, it will also speed up compilations by avoiding the many
thousands of lines of X and Motif header files.

!!!

It is hoped that a future revision of FramepaC may provide support for
Microsoft Windows through the @t{FrWidget} heirarchy, thus allowing
applications to be ported from X to Microsoft Windows with no
source-code changes other than the initialization call.



@comment{---------------------------------------------------------------}

@chapter{Function Reference}
@label{FuncRef}

The functions provided by FramepaC are declared in the include file
@t{FramepaC.h}.  This file includes various other files, so the
directory containing it must be placed on the compiler's include-file
path (i.e. with the @t{-I} switch).

@comment{-------------------------------}
@section{Initialization Functions}

FramepaC must be initialized before any of the remaining functions
described in this document may be used.  This is accomplished by
calling one of the following functions:

@begin{FuncDesc}
@indexfunc{initialize_FramepaC}
@Func{void initialize_FramepaC(int max_symbols = 0)}
Initialize the FramepaC system and create a symbol table for
the specified number of symbols (which will be expanded
automatically if needed--the only detrimental effect of too
small a size will be increased run-time).  If 'max_symbols' is
omitted or 0, the default size set at compile-time will be used.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{FrInitializeMotif}
@Func{void FrInitializeMotif(const char *window_name, Widget parent,
                               int max_symbols = 0)} Initialize the
FramepaC sytem to use the Motif user interface.  This function should
be used if the Motif system will be explicitly initialized before
FramepaC is initialized; @t{parent} is the Widget for the toplevel
application window, and @t{window_name} is the title which should be
given to the FramepaC message window (which displays all @t{FrError}
and @t{FrMessage} output).  The next function may be used if you do not
wish to concern yourself with Motif initialization.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{FrInitializeMotif}
@Func{Widget FrInitializeMotif(int *orig_argc, char **orig_argv,
                                 const char *maintitle, const char *msgtitle,
                                 const char *icon_name,
                                 FrBool allow_resize = True)}
Initialize both FramepaC and the Motif user interface.  This function
requires both the original @t{argc} and @t{argv} passed to @t{main},
which it will update to remove any Motif-specific commandline
arguments.  The other three strings give the titles for the full
toplevel window, the FramepaC message window, and the application's
icon, respectively.  Finally, @t{allow_resize} indicates whether the
user should be allowed to resize the application's main window using
the X window manager.
@end{FuncDesc}

If operation with virtual frames is desired, one of the following
functions must be called before the first attempt to use virtual
frames.  @b{Failure to call one of these functions before attempting to
create or load virtual frames is an error and can cause a variety of
undesired behavior}.

@begin{FuncDesc}
@indexfunc{initialize_VFrames_memory}
@Func{void *initialize_VFrames_memory(int symtabsize)}
Set up the VFrames system to operate only in memory, without
any kind of backing store.  After being set up with this
functions, @t{VFrame} calls will operate identically to the
corresponding @t{FrFrame} calls.  This function returns a
newly-created symbol table of the specified size (default size
if 0 specified) if successful or 0 on error.

Each call to @t{initialize_VFrames_*()} creates a new symbol table
and makes it the current symbol table, permitting multiple sets
of virtual frames each with a different kind of backing store.
It is up to the user to switch between symbol tables as
appropriate before attempting to access frames.

This function should be called after @t{initialize_FramepaC} but before
any use of virtual frames functions.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{initialize_VFrames_disk}
@indexpp{FrNO_DATABASE}
@Func{void *initialize_VFrames_disk(char *filename, int symtabsize, FrBool transactions,
        FrBool force_create = True)}
Set up the VFrames system to use a disk file as backing store for the
frames.  This function returns a newly-created symbol table of the
specified size (default size if 0) if successful or 0 on error.  If
FramepaC has been compiled without disk storage support (preprocessor
symbol FrNO_DATABASE is defined), this function always indicates an error.
If @t{force_create} is @t{True}, the backing store file will be created if it
does not exist; if @t{False}, the call will fail.  The flag @t{transactions}
indicates whether or not to maintain a transaction log file which may be used
to roll back incomplete operations in the event of a crash.

This function should be called after @t{initialize_FramepaC} but before
any use of virtual frames functions.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{initialize_VFrames_server}
@indexpp{FrNO_SERVER}
@Func{void *initialize_VFrames_server(char *server, int port, char *database, int symtabsize,
                                        FrBool transactions, FrBool force_create = True)}
Set up the VFrames system to use a database server for
retrieving and storing frames.
This function returns a newly-created symbol table of the
specified size if successful or 0 on error.  If FramepaC has been compiled
without network server support (preprocessor symbol FrNO_SERVER is
defined), this function always indicates an error.  If @t{force_create} is
@t{True}, the database will be created if it does not yet exist.

This function should be called after @t{initialize_FramepaC} but before
any use of virtual frames functions.
@end{FuncDesc}

A program which uses virtual frames must also call the @t{shutdown_VFrames}
function once for each @t{initialize_VFrames_XX} call before it
terminates.  Failure to do so will likely result in lost updates, and
can potentially corrupt the backing store.

@begin{FuncDesc}
@indexfunc{shutdown_VFrames}
@Func{int shutdown_VFrames()}
For the frames associated with the current symbol table, commit
any dirty frames to backing store, close access to the backing
store, and destroy the symbol table, making the default symbol
table the current symbol table.  Returns 0 if successful or a
nonzero error code.
@end{FuncDesc}


@comment{-------------------------------}
@section{Input/Output Functions}
@label{IOFuncs}

@begin{FuncDesc}
@indexoper{<<}
@Func{ostream << FrObject}
Send a printed representation of the given object to the output
stream in the same manner as normal C++ streams operation.  If it is
known  that the output will always be to a string, it will probably
be preferable to use the much faster function @t{FrObject::print(char*)}
instead of a @t{strstream}.
@end{FuncDesc}

@begin{FuncDesc}
@indexoper{<<}
@Func{ostream << FrObject*}
Send a printed representation of the object pointed at by the argument
to the output stream in the same manner as normal C++ streams
operation.  If it is known that output will always be to a string,
it will probably be preferable to use the much faster function
@t{FrObject::print(char*)} instead of a @t{strstream}.
@end{FuncDesc}

@begin{FuncDesc}
@indexoper{>>}
@Func{istream >> FrObject*&}
Read the printed representation of a @t{FrObject} (FrString, FrList,
FrSymbol, FrFrame, FrNumber, FrQueue, FrArray, FrStruct, etc.) from the input
stream and store it in the specified pointer.  For converting a string
into an object, it will usually be preferable to use the function
@t{string_to_FrObject} instead of creating a @t{strstream} consisting
of the string (the latter takes four to five times as long to execute).

See Section @ref{reader} for details on the formats of items
acceptable to the FramepaC reader.
@end{FuncDesc}

@begin{FuncDesc}
@indexoper{>>}
@Func{istream >> FrSymbol*&}
Read the printed representation of a FrSymbol from the input
stream and store a pointer to the symbol in the specified
pointer.  For converting a string into a symbol, it will usually be
preferable to use either @t{makeSymbol} or @t{string_to_Symbol} rather
than creating a @t{strstream} consisting of the string (the latter
takes much longer to execute than @t{string_to_Symbol}).
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{FrObject_string_length}
@index2{p="length",s="of printed output"}
@Func{int FrObject_string_length(const FrObject *obj)}
Return the length of the printed representation for the given
object.  @t{FrObject::print(char*)} requires a buffer one longer
than this (for the terminating NUL).  When @t{obj} is not 0, this
function is identical to the @t{displayLength} method.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{print}
@Func{char *FrObject::print(char *buffer) const}
Place the printed representation of the given object into the
user-supplied buffer (which must be large enough for the entire
representation).  This function returns a pointer to the
character immediately following the last character in the
printed representation (which will be the terminating NUL).  When
@t{obj} is not 0, this method is identical to the @t{displayValue}
method. 
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{print}
@Func{char *FrObject::print() const}
Place the printed representation of the given object into a
newly-allocated buffer.  This function returns a pointer to the buffer
containing the NUL-terminated printed representation; this buffer must
be deallocated with @t{FrFree} when no longer required.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{string_to_FrObject}
@Func{FrObject *string_to_FrObject(char *&input)}
Read the printed representation of a @t{FrObject} (FrString, FrList,
FrSymbol, FrFrame, FrNumber, FrArray, FrStruct, etc.) from the supplied
input string and return a pointer to the object.  The pointer to the
input string is updated to point at the character immediately following
the last one consumed in reading the printed representation of the
object.

See Section @ref{reader} for details on the formats of items
acceptable to the FramepaC reader.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{string_to_Symbol}
@Func{FrSymbol *string_to_Symbol(char *&input)}
Read the printed representation of a FrSymbol from the supplied
input string and return a pointer to it.  The pointer to the
input string is updated to point at the character immediately
following the last one consumed in reading the symbol.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{import_FrameKit_frames}
@Func{FrBool import_FrameKit_frames(istream &in, ostream &echo)}
Read a file containing textual representations of frames (in either FrameKit
or FramepaC format) from the stream @t{in} into the current symbol table as
virtual frames.  If @t{echo} is nonzero, a one-line progress report is written
to the stream for each frame read.

Returns @t{True} if the file was successfully read, or @t{False} if an error
occurred.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{export_FrameKit_frames}
@Func{FrBool export_FrameKit_frames(ostream &output, FrList *frames)}
Writes each of the frames named in @t{frames} to the output stream in FrameKit
@t{MAKE-FRAME} format.  Returns @t{True} if successful, or @t{False} if any
of the elements of the list is not a symbol naming a frame.
@end{FuncDesc}

@comment{-------------------------------}
@section{Type Determination Functions}
@label{typedeterm}
@index2{p="Types",s="determining"}

@begin{FuncDesc}
@indexfunc{ARRAYP}
@Func{int ARRAYP(const FrObject*)}
@indexmeth{arrayp}
@indextype{FrArray}
@index{arrays}
@Func{virtual FrBool FrObject::arrayp() const}
Return @t{True} if the object pointed at by the given pointer is an
instance of the class @t{FrArray} or some subclass thereof.  The second
form is the object-oriented function underlying the first form, and
should only be called if the pointer is already known to be non-NULL
(which is what @t{ATOMP} does).
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{ATOMP}
@Func{int ATOMP(const FrObject*)}
@indexmeth{atomp}
@indextype{FrAtom}
@Func{virtual FrBool FrObject::atomp() const}
Return @t{True} if the object pointed at by the given pointer
is an instance of the class @t{FrAtom} or some subclass of it (@t{FrSymbol},
@t{FrString}, @t{FrNumber}, etc.).  The second form is the object-oriented
function underlying the first form, and should only be called if the
pointer is already known to be non-NULL (which is what @t{ATOMP}
does).
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{CONSP}
@Func{int CONSP(const FrObject*)}
@indextype{FrCons}
@indextype{FrList}
@indexmeth{consp}
@Func{virtual FrBool FrObject::consp() const}
Return @t{True} if the object pointed at by the given pointer
is an instance of the class @t{FrCons} or some subclass of @t{FrCons}
(only @t{FrList} at this time).  The second form is the object-oriented function
underlying the first form, and should only be called if the pointer
is already known to be non-NULL (which is what @t{CONSP} does).
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{FRAMEP}
@Func{int FRAMEP(const FrObject*)}
@indexmeth{framep}
@indextype{FrFrame}
@indextype{VFrame}
@Func{virtual FrBool FrObject::framep() const}
Return @t{True} if the object pointed at by the given pointer
is an instance of the class @t{FrFrame} or its subclass @t{VFrame}.  The
second form is the object-oriented function underlying the first form,
and should only be called if the pointer is already known to be non-NULL
(which is what @t{FRAMEP} does).  Use @t{is_frame} or @t{FrSymbol::isFrame}
to determine whether a particular symbol is the name of a frame.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{HASHP}
@Func{int HASHP(const FrObject*)}
@indexmeth{hashp}
@indextype{FrHashTable}
@Func{virtual FrBool FrObject::hashp() const}
Return @t{True} if the object pointed at by the given pointer
is an instance of the class @t{FrHashTable}.  The
second form is the object-oriented function underlying the first form,
and should only be called if the pointer is already known to be non-NULL
(which is what @t{HASHP} does).
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{NUMBERP}
@Func{int NUMBERP(const FrObject*)}
@indextype{FrNumber}
@indexmeth{numberp}
@Func{virtual FrBool FrObject::numberp() const}
Return @t{True} if the object pointed at by the given pointer
is an instance of the class @t{FrNumber} or one of its subclasses (@t{FrFloat}
or @t{FrInteger}).  The second form is the object-oriented
function underlying the first form, and should only be called if the
pointer is already known to be non-NULL (which is what @t{NUMBERP}
does).
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{objType}
@index{object type}
@Func{virtual ObjectType FrObject::objType() const}
Return a value identifying the exact object type, such as @t{OT_Frame},
@t{OT_FrQueue}, etc.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{QUEUEP}
@Func{int QUEUEP(const FrObject*)}
@indexmeth{queuep}
@indextype{FrQueue}
@Func{virtual FrBool FrObject::queuep() const}
Return @t{True} if the object pointed at by the given pointer
is an instance of the class @t{FrQueue}.  The
second form is the object-oriented function underlying the first form,
and should only be called if the pointer is already known to be non-NULL
(which is what @t{QUEUEP} does).
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{STRINGP}
@Func{int STRINGP(const FrObject*)}
@indextype{FrString}
@indexmeth{stringp}
@Func{virtual FrBool FrObject::stringp()}
Return @t{True} if the object pointed at by the given pointer is an
instance of the class @t{FrString}.  The second form is the object-oriented
function underlying the first form, and should only be called if the
pointer is already known to be non-NULL (which is what @t{STRINGP}
does).
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{STRUCTP}
@Func{int STRUCTP(const FrObject*)}
@indexmeth{structp}
@indextype{FrStruct}
@index{structures}
@Func{virtual FrBool FrObject::structp()}
Return a true value if the object pointed at by the given pointer
is an instance of the class @t{FrStruct}.  The
second form is the object-oriented function underlying the first form,
and should only be called if the pointer is already known to be non-NULL
(which is what @t{STRUCTP} does).
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{SYMBOLP}
@Func{int SYMBOLP(const FrObject*)}
@indextype{FrSymbol}
@indexmeth{symbolp}
@Func{virtual FrBool FrObject::symbolp()}
Return a true value if the object pointed at by the given pointer
is an instance of the class @t{FrSymbol}.  The second form is the object-oriented
function underlying the first form, and should only be called if the
pointer is already known to be non-NULL (which is what @t{SYMBOLP}
does).
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{VECTORP}
@Func{int VECTORP(const FrObject*)}
@indextype{FrBitVector}
@indexmeth{vectorp}
@Func{virtual FrBool FrObject::vectorp()}
Return a true value if the object pointed at by the given pointer is an
instance of the class @t{FrBitVector} or some subclass thereof.  The
second form is the object-oriented function underlying the first form,
and should only be called if the pointer is already known to be
non-NULL (which is what @t{VECTORP} does).
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{WIDGETP}
@Func{int WIDGETP(const FrObject*)}
@indextype{FrWidget}
@indexmeth{vectorp}
@Func{virtual FrBool FrObject::widgetp()}
Return a true value if the object pointed at by the given pointer is an
instance of the class @t{FrWidget} or some subclass thereof.  The
second form is the object-oriented function underlying the first form,
and should only be called if the pointer is already known to be
non-NULL (which is what @t{WIDGETP} does).
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{is_frame}
@Func{FrBool is_frame(const FrSymbol *name)}
@indexmeth{isFrame}
@Func{FrBool FrSymbol::isFrame()}
Return @t{True} if the specified symbol is the name of a frame,
@t{False} if not.  If the specified frame is a virtual frame which is
not currently in memory, it will not be fetched from the backing store.
@end{FuncDesc}

@comment{-------------------------------}
@section{Common Object Functions}

Rather than @t{delete}, you should always use @t{free_object(FrObject
*)} or @t{object->freeObject()}, since some object types (i.e.
@t{FrSymbol}) may never be freed, and some (i.e. @t{FrFrame}) should not
normally be freed.

@subsection{Procedural Interface}
@label{commonproc}

@begin{FuncDesc}
@indexfunc{free_object}
@Func{void free_object(FrObject *) ;}
Deallocate the item pointed at by its argument, if appropriate for
the type of object actually being referenced.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{eql}
@Func{FrBool eql(const FrObject*, const FrObject*)}
Determine whether the two objects are identical or are both numbers
and have the same value.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{equal}
@Func{FrBool equal(const FrObject*, const FrObject*)}
Determine whether the two objects are identical or are both of the
same type and have the same value.  This is the most general of the object
comparison functions, and thus the slowest; if the types of the two
numbers are known, it is usually preferable to use either @t{eql()} or
@t{==} instead.  The @t{==} operator is valid between two @t{FrSymbol}s, two
@t{FrNumber}s, and two @t{FrString}s.
@indexoper{==}
@end{FuncDesc}

@subsection{Object-Oriented Interface}
@label{commonobj}

The functions described in this section are all virtual functions,
which allows you to use them on a @t{FrObject*} without the need to
worry about what type of object is actually present.  However, you
do need to ensure that the pointer you are using is not NULL before
calling the function.

@begin{FuncDesc}
@indexvirt{objType}
@Func{virtual ObjectType objType() const}
Determine what type of object is actually present.
@end{FuncDesc}

@begin{FuncDesc}
@indexvirt{objTypeName}
@Func{virtual const char *objTypeName() const}
Return a printed string containing the name of the type of object,
for example, "FrObject", "FrFrame", or "FrSymbol".
@end{FuncDesc}

@begin{FuncDesc}
@indexvirt{objSuperclass}
@Func{virtual ObjectType objSuperclass() const}
Determine the type of object from which the object instance is derived,
if any.
@end{FuncDesc}

@begin{FuncDesc}
@indexvirt{freeObject}
@Func{virtual void freeObject()}
Erase or otherwise deallocate the object, as appropriate for its type.
@end{FuncDesc}

@begin{FuncDesc}
@indexvirt{copy}
@Func{virtual FrObject *copy() const}
Make a shallow copy of the object.  Only the top-level structure of
the object will be copied, if appropriate.  For example, a shallow
copy of a list will copy only the cons cells forming the list, not any
of the items in the list.  For most objects, @t{copy} and @t{deepcopy}
are the same.
@end{FuncDesc}

@begin{FuncDesc}
@indexvirt{deepcopy}
@Func{virtual FrObject *deepcopy() const}
Make a copy of the complete structure of an object, if appropriate.
For example, a deep copy of a list will copy the items in the list as
well as the cons cells forming the list.  For most objects, @t{copy}
and @t{deepcopy} are the same.
@end{FuncDesc}

@begin{FuncDesc}
@indexvirt{subseq}
@Func{virtual FrObject *subseq(size_t start, size_t stop)}
Make a copy of the indicated portion of the object, if appropriate.
Both @i{start} and @i{stop} are zero-based indices into the object,
indicating which elements (inclusively) are to be placed in the result.
For example, @b{subseq(1,2)} of the list @b{(A B C D)} will return the
new list @b{(B C)}.
@end{FuncDesc}

@begin{FuncDesc}
@indexvirt{reverse}
@Func{virtual FrObject *reverse()}
For compound objects whose component objects are ordered, destructively
reverse the components.  Returns an object pointer which should be used
on subsequent references instead of the original pointer (since e.g.
the original pointer to a reversed list points at the last item in the
reversed list).
@end{FuncDesc}

@begin{FuncDesc}
@indexvirt{printValue}
@Func{virtual ostream &printValue(ostream &output) const}
Output a printed representation of the object's value to the supplied
stream.  This is the function underlying the overloading of the @t{<<}
operator for streams to FramepaC objects.
@end{FuncDesc}

@begin{FuncDesc}
@indexvirt{displayLength}
@index2{p="length",s="of printed output"}
@Func{virtual size_t displayLength() const}
Determine how many bytes the printed representation of the object will
require.  This function allows you to allocate the correct amount of
buffer space when creating a string containing the object's value with
@t{displayValue}.
@end{FuncDesc}

@begin{FuncDesc}
@indexvirt{displayValue}
@Func{virtual char *displayValue(char *buffer) const}
Store a printed representation of the object's value in the supplied
buffer.  The required size of the buffer may be determined with
@t{displayLength}.
@end{FuncDesc}

@begin{FuncDesc}
@indexvirt{length}
@index2{p="length",s="of an object"}
@Func{virtual int length() const}
If appropriate for the object, return a measure of its size.  Returns
the length of a list, number of characters in a string, number of items
in a queue or hash table, etc.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{iterate}
@Func{FrBool iterate(FrIteratorFunc func, ...) const}
If appropriate for the object, call @b{func} once for each subobject of
the given object; i.e. once for each item stored in a hashtable, array,
queue, etc.  This function returns @i{True} if the provided function
returns @i{True} for each and every call, @i{False} otherwise.  If
@t{func} ever returns @i{False}, the iteration is terminated immediately.

For objects for which the operation is not meaningful, this function
returns @i{True} (the operation was trivially successful).
@end{FuncDesc}

@begin{FuncDesc}
@indexvirt{iterateVA}
@Func{virtual FrBool iterateVA(FrIteratorFunc func, va_list args) const}
This function is the same as @b{iterate}, but accepts a
variable-argument list rather than a list of arguments.
@end{FuncDesc}

@begin{FuncDesc}
@indexvirt{hashValue}
@Func{virtual unsigned long hashValue() const}
Return a hashing key value which is used by @t{FrHashTable} to locate
the object.
@end{FuncDesc}

@begin{FuncDesc}
@indexvirt{car}
@Func{virtual FrObject *car() const}
If appropriate for the object, return the first subobject it contains.
Returns the first item in a list, or the item at the head of a queue.

For objects for which the operation is not meaningful, this function
returns 0.  If the object is known to be a @t{FrCons} or @t{FrList},
the method @t{first} may be used instead of @t{car} to avoid the
overhead of a virtual function call.
@end{FuncDesc}

@begin{FuncDesc}
@indexvirt{cdr}
@Func{virtual FrObject *cdr() const}
If appropriate for the object, return the remainder after removing the
first subobject it contains.  Returns the remainder of a list.

For objects for which the operation is not meaningful, this function
returns 0.  If the object is known to be a @t{FrList}, the method
@t{rest} may be used instead of @t{cdr} to avoid the overhead of a
virtual function call.
@end{FuncDesc}

@begin{FuncDesc}
@indexvirt{equal}
@Func{virtual FrBool equal(const FrObject*) const}
Determine whether the indicated object is equivalent to the object to
which the method is applied.
@end{FuncDesc}

@begin{FuncDesc}
@indexvirt{compare}
@Func{virtual int compare(const FrObject *) const}
Determine the sort-order relationship between the indicated object and
the object to which the method is applied.  Returns -1 if the applied
object should sort before the indicated object, +1 if the reverse, and
0 if either order is acceptable (generally because the two items are
equivalent).
@end{FuncDesc}

@begin{FuncDesc}
@indexvirt{getNth}
@Func{virtual FrObject *getNth(size_t N) const}
If appropriate for the object, return a copy of the @t{N}th component
(which must be released with @t{freeObject} or @t{freeObject} when no
longer needed).  @t{N} may range from 0 to one less than the logical
size of the object.

This method returns 0 if the specified index is out of range or the
operation is not meaningful for the object.  It is currently
useful for @t{FrCons} (for N=0,1), @t{FrList}, @t{FrQueue}, and
@t{FrString} (where it returns a one-character FrString).
@end{FuncDesc}

@begin{FuncDesc}
@indexvirt{setNth}
@Func{virtual FrBool setNth(size_t N, const FrObject *newelt) const}
If appropriate for the object, destructively modify the @t{N}th
component, setting it to a copy of @t{newelt}.  Returns @t{True} if the
object was modified, @t{False} otherwise.  This method returns
@t{False} if the operation is not meaningful for the object, the
specified index is out of range, or the given @t{newelt} is of the
wrong type.

@t{setNth} is currently useful for @t{FrArray}, @t{FrCons} (for N=0,1),
@t{FrList}, @t{FrQueue}, and @t{FrString} (which requires a @t{FrString} for
@t{newelt}, and uses its first character).
@end{FuncDesc}

@begin{FuncDesc}
@indexvirt{locate}
@index{locating elements of an object}
@Func{virtual size_t locate(const FrObject *item, size_t start = (size_t)-1) const}
@Func{virtual size_t locate(const FrObject *item, FrCompareFunc func, size_t start = (size_t)-1) const}
Locate a component or subsequence of a sequence object (array, list,
queue, or string).  This method is currently effective on @t{FrList},
@t{FrQueue}, and @t{FrString}.  The valid types for @t{item} vary
slightly between classes, so see the description of @t{locate} under
the appropriate class for more details.  @t{start} indicates where in
the sequence to start searching: if @t{-1}, search the entire object,
else search the portion of the sequence @i{after} index @t{start}.
This permits all matches in an object to be iterated over by repeatedly
calling this method with the return value of the previous call as the
value of @t{start}.

This method returns @t{(size_t)-1} if no match was found, or the object
is not of an appropriate type.  If a comparison function is provided,
it is called repeatedly to determine matching elements of the sequence
object; if no function is provided, a simple pointer equality test
(equivalent to Lisp's @t{eq}) is used.
@end{FuncDesc}

@begin{FuncDesc}
@indexvirt{insert}
@Func{virtual FrObject *insert(const FrObject *item, size_t pos)}
Destructively insert a new item or list of items into a sequence object
(array, list, queue, or string) at the indicated position, which may range
from 0 (insert prior to first element) to @t{length()} (insert after
last element).  A pointer to the modified object is returned; it will
normally be identical to the pointer to the object on which the method
was called, but will differ for @t{FrList}s if @t{pos} is 0.

The object will not be modified if @t{pos} is greater than @t{length()}
of the object, or if the object is not of an appropriate type.
@end{FuncDesc}

@begin{FuncDesc}
@indexvirt{elide}
@Func{virtual FrObject *elide(size_t start, size_t end)}
Destructively remove the indicated subsequence of the object, if
appropriate, from the @t{start}h component to the @t{end}th component,
inclusive.  A pointer to the modified object is returned; it will
normally be identical to the pointer to the object on which the method
was called, but will differ for @t{FrList}s if @t{start} is 0.

If @t{start} is greater than or equal to @t{length()} of the object,
the object will not be modified.  If @t{end} is greater than the size
of the object, all elements after @t{start} are removed.
@end{FuncDesc}

@begin{FuncDesc}
@indexvirt{expand}
@Func{virtual FrBool expand(size_t increment)}
If appropriate for the object, expand its size by @t{increment}
elements.  Returns @t{True} if the object was expanded, @t{False} if not.
@end{FuncDesc}

@begin{FuncDesc}
@indexvirt{expandTo}
@Func{virtual FrBool expandTo(size_t newsize)}
If appropriate for the object, and the object's current size is less
than @t{newsize} elements, expand its size to @t{newsize}
elements.  Returns @t{True} if the object was expanded, @t{False} if not.
@end{FuncDesc}

@begin{FuncDesc}
@indexoper{>>}
@Func{istream &operator >> (istream &input, FrObject *&obj)}
Read an object from the given @t{istream}, and set the supplied
@t{FrObject*} to point at the object read.
@end{FuncDesc}

@begin{FuncDesc}
@indexoper{<<}
@Func{ostream &operator << (ostream &output, const FrObject *object)}
Print the value of an object to the given output stream.  This is equivalent to
@t{printValue} when @t{object} is not 0, and outputs "()" when
@t{object} is 0.
@end{FuncDesc}

@begin{FuncDesc}
@indexoper{<<}
@Func{ostream &operator << (ostream &output, const FrObject &object)}
Print the value of the object to the given output stream.
@end{FuncDesc}

@comment{-------------------------------}
@section{FrArray Functions}
@indextype{FrArray}
@index{arrays}

@t{FrArray} is an extensible array class which stores @t{FrObject}s as
array elements.

@begin{FuncDesc}
@indexctor{FrArray}
@Func{FrArray::FrArray(size_t size, const FrList *init = 0)}
@Func{FrArray::FrArray(size_t size, const FrObject **init, FrBool copyitems = True)}
Create a new array capable of storing @t{size} items.  For the first
variant of the constructor, the elements of @t{init} are copied into
the array; if @t{init} contains fewer than @t{size} elements, the final
element of the list is copied into all remaining locations of the
array.  If @t{init} is 0, all elements of the array are initialized to
0. 

For the second variant of the constructor, @t{init} is an array of
pointers to @t{FrObject}s which must contain at least @t{size}
elements.   If @t{copyitems} is @t{False}, only the pointers in
@t{init} are copied into the array; otherwise, the objects pointed at
by @t{init} are copied prior to being placed into the new @t{FrArray}.
@end{FuncDesc}

@begin{FuncDesc}
@indexdtor{FrArray}
@Func{FrArray::~FrArray()}
Deallocate the array.
@end{FuncDesc}

@begin{FuncDesc}
@indexvirt{arrayp}
@Func{virtual FrBool FrArray::arrayp() const}
Returns @t{True} to indicate that the object is an array.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{length}
@index2{p="length",s="arrays"}
@Func{virtual size_t FrArray::length() const}
Return the number of elements that the array currently contains,
including any entries set to the NULL pointer (0).
@end{FuncDesc}

@begin{FuncDesc}
@indexvirt{car}
@Func{virtual FrObject *FrArray::car() const}
Returns @t{array[0]}.
@end{FuncDesc}

@begin{FuncDesc}
@indexvirt{reverse}
@Func{virtual FrObject *FrArray::reverse()}
Reverse the order of the elements in the array.  Returns a pointer to
the reversed @t{FrArray}.
@end{FuncDesc}

@begin{FuncDesc}
@indexoper{[]}
@Func{FrObject *&FrArray::operator[] (size_t N)}
Create a reference to the @t{N}th element of the array, which may then
be retrieved or modified as with @t{getNth} or @t{setNth}.
@end{FuncDesc}

@begin{FuncDesc}
@indexvirt{expand}
@Func{virtual FrBool FrArray:expand(size_t increment)}
Expand the @t{FrArray}'s size by @t{increment} elements, setting the
new elements to 0.  Returns @t{True} if the array was expanded,
@t{False} if not.

The array may be shrunk using @t{elide}.  To shrink an array to size
@t{N}, use
@begin{programexample}
array->elide(N, array->length()) ;
@end{programexample}
@end{FuncDesc}

@begin{FuncDesc}
@indexvirt{expandTo}
@Func{virtual FrBool FrArray::expandTo(size_t newsize)}
If the @t{FrArray}'s current size is less than @t{newsize} elements,
expand its size to @t{newsize} elements and set the new elements to 0.
Returns @t{True} if the array was expanded, @t{False} if not.
@end{FuncDesc}

@begin{FuncDesc}
@indexvirt{subseq}
@Func{virtual FrObject *FrArray::subseq(size_t start, size_t end)}
Return a new @t{FrArray} containing only elements @t{start} through
@t{end} (inclusive) of the original array.  A complete deep copy of the
array elements is made.
@end{FuncDesc}

@begin{FuncDesc}
@indexvirt{insert}
@Func{virtual FrObject *FrArray::insert(const FrObject *newelt, size_t location, FrBool copyitem = True)}
Insert the object or objects specified by @t{newelt} into the array
prior to element @t{location}, expanding the array as needed and
shifting all elements from @t{location} to the end of the array to
accomodate the new element(s).  @t{location} may range from 0 to
@t{length()}; location 0 prepends the new element(s) to be beginning of
the array, while location @t{length()} appends the new element(s) to
the end of the array.

If @t{newelt} is @t{consp()}, then it is assumed to be a list of
elements to be inserted; otherwise, it is assumed to be a single new
element to be inserted.  If you wish to insert a single element which
is itself a list, you must wrap it inside a single-element list.  If
@t{copyitem} is @t{True}, a copy of the item(s) is made prior to
insertion into the array; otherwise, the object itself is used and may
not be deleted as long as it remains in the array (an exception is that
the cons cells making up the top-level list for @t{newelt} may be
freed), nor may it be referenced after the array is deleted.
@end{FuncDesc}

@begin{FuncDesc}
@indexvirt{elide}
@Func{virtual FrObject *FrArray::elide(size_t start, size_t end)}
Remove elements @t{start} through @t{end} (inclusive) from the array,
and shift all remaining elements down to close the gap created by the
removal.  This method reduces the size of the array by
(@t{end}-@t{start}+1) elements.  A pointer to the modified @t{FrArray}
is returned.
@end{FuncDesc}

@begin{FuncDesc}
@indexvirt{locate}
@index{locating elements of an object}
@Func{virtual size_t FrArray::locate(const FrObject *item, size_t start = (size_t)-1) const}
@Func{virtual size_t FrArray::locate(const FrObject *item, FrCompareFunc func,
        size_t start = (size_t)-1) const}
Locate an element or sequence of elements in the array.  If @t{item} is
@t{consp()}, this method locates the first occurrence of sequential
elements in the array after @t{start} matching the list pointed at by
@t{item}; otherwise, it locates the single element matching @t{item}.
Note that in order to locate a single element which happens to be a
list, you must wrap it in another, one-element list.  The search begins
following the @t{start}h element of the array, which permits the result
of a @t{locate} to be passed to a subsequent call in order to search
for additional matches.  The return value is the index into the array
of the first match after @t{start}, or @t{(size_t)-1} if no match could
be found.

If provided, the comparison function @t{func} is called repeately to
determine which element(s) of the array match @t{item}; otherwise, a simple
pointer comparison (equivalent to Lisp's @t{eq}) is used.
@end{FuncDesc}

@comment{-------------------------------}
@section{FrBitVector Functions}
@indextype{FrBitVector}
@index{vectors}
@index{bit vectors}

@t{FrBitVector} is an extensible array class which stores a string of bits
as array elements.

Whenever a boolean value is required as input to be stored in the
@t{FrBitVector}, a variety of different @t{FrObject}s may be supplied.
The input value is converted to a @t{FrBool} by the function
@t{FrBooleanValue}, which accepts as @t{True} the following:
@begin{enumerate}
the symbol @t{T}

any symbol whose printed name begins with either 'T' or 'Y' (upper- or
lower-case) 

any @t{FrString} whose first character is 'T' or 'Y' (either case)

any @t{FrNumber} whose integer portion is non-zero
@end{enumerate}
and as @t{False}
@begin{enumerate}
@end{enumerate}
the NULL pointer

the symbol @t{NIL}

any other @t{FrSymbol} not mentioned already

any @t{FrString} whose first character is neither 'T' nor 'Y'

any @t{FrNumber} whose integer portion is zero

any other type of object

@begin{FuncDesc}
@indexctor{FrBitVector}
@Func{FrBitVector::FrBitVector(size_t size = 0)}
@Func{FrBitVector::FrBitVector(size_t size, const FrList *init)}
Create a new bit vector capable of storing @t{size} bits.  For the
first variant of the constructor, the array is cleared to all zero
bits.  For the second variant, the elements of @t{init} are passed
through @t{FrBooleanValue} in turn, and placed in successive locations
of the bit vector; if @t{size} is greater than the length of @t{init},
any values between the end of @t{init} and the specified size of the
array remain cleared to zero.
@end{FuncDesc}

@begin{FuncDesc}
@indexctor{FrBitVector}
@Func{FrBitVector::FrBitVector(const FrBitVector &vect)}
Create a new copy of the bit vector @t{vect}.
@end{FuncDesc}

@begin{FuncDesc}
@indexdtor{FrBitVector}
@Func{FrVector::~FrBitVector()}
Deallocate the bit vector.
@end{FuncDesc}

@begin{FuncDesc}
@indexvirt{vectorp}
@Func{virtual FrBool FrBitVector::vectorp() const}
Returns @t{True} to indicate that the object is a bit vector.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{length}
@index2{p="length",s="vectors"}
@Func{virtual size_t FrBitVector::length() const}
Return the number of elements that the bit vector currently contains.
@end{FuncDesc}

@begin{FuncDesc}
@indexvirt{reverse}
@Func{virtual FrObject *FrBitVector::reverse()}
Reverse the order of the elements in the vector.  Returns a pointer to
the reversed @t{FrBitVector}.
@end{FuncDesc}

@begin{FuncDesc}
@indexvirt{expand}
@Func{virtual FrBool FrBitVector:expand(size_t increment)}
Expand the @t{FrBitVector}'s size by @t{increment} bits, setting the
new elements to 0.  Returns @t{True} if the vector was expanded,
@t{False} if not.

The vector may be shrunk using @t{elide}.  To shrink a vector to size
@t{N}, use
@begin{programexample}
vector->elide(N, vector->length()) ;
@end{programexample}
@end{FuncDesc}

@begin{FuncDesc}
@indexvirt{expandTo}
@Func{virtual FrBool FrBitVector::expandTo(size_t newsize)}
If the @t{FrVector}'s current size is less than @t{newsize} bits,
expand its size to @t{newsize} bits and set the new elements to 0.
Returns @t{True} if the vector was expanded, @t{False} if not.
@end{FuncDesc}

@begin{FuncDesc}
@indexvirt{subseq}
@Func{virtual FrObject *FrBitVector::subseq(size_t start, size_t end)}
Return a new @t{FrBitVector} containing only bits @t{start} through
@t{end} (inclusive) of the original vector.
@end{FuncDesc}

@begin{FuncDesc}
@indexvirt{insert}
@Func{virtual FrObject *FrBitVector::insert(const FrObject *newelt, size_t location, FrBool copyitem = True)}
Insert the bit or bits specified by @t{newelt} into the bit vector
prior to bit number @t{location}, expanding the vector as needed and
shifting all bits from @t{location} to the end of the vector to
accomodate the new bit(s).  @t{location} may range from 0 to
@t{length()}; location 0 prepends the new bit(s) to be beginning of
the vector, while location @t{length()} appends the new bit(s) to
the end of the vector.

If @t{newelt} is @t{consp()}, then it is assumed to be a list of
bits to be inserted; otherwise, it is assumed to be a single new
bit to be inserted.
@end{FuncDesc}

@begin{FuncDesc}
@indexvirt{elide}
@Func{virtual FrObject *FrBitVector::elide(size_t start, size_t end)}
Remove bits @t{start} through @t{end} (inclusive) from the vector,
and shift all remaining bits down to close the gap created by the
removal.  This method reduces the size of the vector by
(@t{end}-@t{start}+1) bits.  A pointer to the modified @t{FrBitVector}
is returned.
@end{FuncDesc}

@begin{FuncDesc}
@indexvirt{locate}
@index{locating elements of an object}
@Func{virtual size_t FrArray::locate(const FrObject *item, size_t start = (size_t)-1) const}
@Func{virtual size_t FrArray::locate(const FrObject *item, FrCompareFunc func,
        size_t start = (size_t)-1) const}
Locate a bit or sequence of bits in the array.  If @t{item} is
@t{consp()}, this method locates the first occurrence of sequential
bits in the array after @t{start} matching the list pointed at by
@t{item}; otherwise, it locates the next occurrence of the single bit
specified by @t{item}.  The search begins
following the @t{start}h bit of the array, which permits the result
of a @t{locate} to be passed to a subsequent call in order to search
for additional matches.  The return value is the index into the vector
of the first match after @t{start}, or @t{(size_t)-1} if no match could
be found.

For bit vectors, the comparison function is ignored, making the second
form equivalent to the first.
@end{FuncDesc}

@begin{FuncDesc}
@indexvirt{iterateVA}
@Func{virtual FrBool FrBitVector::iterateVA(FrIteratorFunc func, va_list args) const}
This method invokes @t{func} once for each bit in the vector, passing
as the function's first argument an @t{FrInteger} with value 0 or 1,
depending on the value of the bit.  This method may also be invoked
indirectly by calling @t{FrObject::iterate} when the object is a bit
vector.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{getBit}
@Func{FrBool FrBitVector::getBit(size_t N) const}
Return the value of the @t{N}th bit in the vector.  If @t{N} is greater
than or equal to the length of the vector, this method return @t{False}.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{setBit}
@Func{FrBool FrBitVector::setBit(size_t N, const FrBool value) const}
Set the value of the @t{N}th bit to @t{value}.  This method returns
@t{True} if the bit was successfully updated, @t{False} if unable to
make the change (i.e. @t{N} is out of range).
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{vectorlength}
@Func{size_t FrBitVector::vectorlength() const}
Return the number of bits currently stored in the vector.  This is a
non-virtual version of @t{length()} for greater efficiency when the
object is known to be a @t{FrBitVector}.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{intersection}
@Func{FrBitVector *FrBitVector::intersection(const FrBitVector *vect) const} 
@indexoper{*}
@Func{FrBitVector *FrBitVector::operator * (const FrBitVector &vect) const}
Return a newly-constructed bit vector which contain the logical AND of
corresponding bits in the two given vectors.  The length of the
returned vector is the lesser of the lengths of the two input vectors,
since all bits which are present in only one vector are by definition
0 after ANDing.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{vectorunion}
@Func{FrBitVector *FrBitVector::vectorunion(const FrBitVector *vect) const} 
@indexoper{+}
@Func{FrBitVector *FrBitVector::operator + (const FrBitVector &vect) const}
Return a newly-constructed bit vector which contain the logical OR of
corresponding bits in the two given vectors.  The length of the
returned vector is the larger of the lengths of the two given vectors.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{difference}
@Func{FrBitVector *FrBitVector::difference(const FrBitVector *vect) const} 
@indexoper{-}
@Func{FrBitVector *FrBitVector::operator - (const FrBitVector &vect) const}
Return a newly-constructed bit vector which contain the 1 bits in each
location containing a 1 bit in this vector and a 0 bit in @t{vect}, and
a 0 bit in every other location.  This method is equivalent to a set
difference operation.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{negate}
@Func{void FrBitVector::negate()}
Invert all of the bits in the vector.
@end{FuncDesc}


@comment{-------------------------------}
@section{FrAtom Functions}
@indextype{FrAtom}

@t{FrAtom} is an abstract class, which should never be instantiated
directly.  Use an object of the appropriate subclass (@t{FrSymbol},
@t{FrString}, or one of @t{FrNumber}'s subclasses) instead.

@comment{-------------------------------}
@section{FrSymbol Functions}

Most slot fillers, as well as frame, slot, and facet names, will be symbols
rather than strings or numbers.  The following functions are provided for
manipulating symbols.

FramepaC internally maintains a collection of all symbols ever created
during the lifetime of a symbol table, and re-uses symbols whenever
possible.  Thus there is always a unique symbol for any given string
of characters unless you switch symbol tables (a capability intended
for use primarily by database servers, which may need to keep multiple
conflicting frames under a single name).

Because symbols are internally managed and only destroyed when the
symbol table containing them is destroyed, it is an error to call
@t{new} or @t{delete} for a @t{FrSymbol}, and the program will be
aborted if you attempt to do so.

@subsection{Construction Functions}

@begin{FuncDesc}
@indexfunc{makeSymbol}
@indexmeth{makeSymbol}
@Func{FrSymbol *makeSymbol(const char *name)}
@Func{FrSymbol *FrSymbol::makeSymbol(const char *name) const}
Return a pointer to the symbol with the specified name in the currently
active symbol table, creating it if it is not already present in the
system.  @t{makeSymbol} does not modify the name in any way; in
particular, it will not convert it to uppercase as the FramepaC reader
(see Section @ref{reader}) does by default.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{gensym}
@Func{FrSymbol *gensym(char *basename) ;}
Return a newly-created symbol which is guaranteed to be unique.
The symbol's name will be the given string followed by one or more
digits.  If the argument is 0, the default base name of "GENSYM"
will be used.

Unlike most Lisp implementations, FramepaC actually ensures that the
created symbol does not yet exist.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{gensym}
@Func{FrSymbol *gensym(FrSymbol *basename) ;}
Return a newly-created symbol which is guaranteed to be unique.
The new symbol's name will be the given symbol's name followed by
one or more digits.
@end{FuncDesc}

@subsection{Basic Functions}

@begin{FuncDesc}
@indexfunc{findSymbol}
@indexmeth{findSymbol}
@Func{FrSymbol *findSymbol(char *name)}
@Func{FrSymbol *FrSymbol::findSymbol(char *name) const}
Return a pointer to the symbol with the specified name, or 0 if the
symbol has never been created, in the currently active symbol table.
@t{findSymbol} does not modify the name in any way; in particular, it
will not convert it to uppercase as the FramepaC reader (see Section
@ref{reader}) does by default.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{symbol_name}
@indexmeth{symbolName}
@Func{const char *symbol_name(FrSymbol *symbol)}
@Func{const char *FrSymbol::symbolName() const}
Return a pointer to the character string representing the symbol's name.
Because only a single symbol is ever created for a given name,
@t{makeSymbol(symbol_name(S)) == S} for all strings S which do not
exceed the maximum length of a symbol's name.
@end{FuncDesc}

@begin{FuncDesc}
@index{symbol comparison}
@indexoper{==}
@Func{FrSymbol == char*}
For ease of writing, the above expression is equivalent to
@t{FrSymbol* == findSymbol(char*)}.  Note that due to a limitation
imposed by C++, you will need to dereference the FrSymbol* you
actually have in order to use this feature, i.e.
@begin{programexample}
        *sym == "IS-A"
@end{programexample}
For maximum execution speed, it is usually preferable to use either
@t{findSymbol} or @t{makeSymbol} once early in the program and compare
against a variable containing the result rather than using the
implicit @t{findSymbol} of this overloaded operator.
@end{FuncDesc}

@begin{FuncDesc}
@index{symbol comparison}
@indexoper{!=}
@Func{FrSymbol != char*}
As above, but testing for inequality.
For maximum execution speed, it is usually preferable to use either
@t{findSymbol} or @t{makeSymbol} once early in the program and compare
against a variable containing the result rather than using the
implicit @t{findSymbol} of this overloaded operator.
@end{FuncDesc}

@begin{FuncDesc}
@index{symbol representation}
@indexmeth{nameNeedsQuoting}
@Func{static FrBool FrSymbol::nameNeedsQuoting(const char *name)}
Determine whether the printed representation of the given symbol name
requires quoting with vertical bars when printed.
@end{FuncDesc}

@comment{-------------------------------}
@section{FrSymbolTable Functions}
@label{advsymfuncs}

The following functions are primarily intended for use by database
servers, which may need to store multiple frames on the same symbol
name.  By allowing multiple symbol tables, each database can thus have
its own name space.  However, symbols will no longer be unique
program-wide, but only within a particular symbol table.

@index2{p="symbols",s="predefined"}
Note that for purposes of reading in symbols, certain symbols (mainly
those with standard uses in frames) are preloaded into each symbol
table and will thus appear to be the same from symbol table to symbol
table, though they in fact are not and will therefore not compare as
equal.

@begin{FuncDesc}
@indexfunc{create_symbol_table}
@indexctor{FrSymbolTable}
@index2{p="symbol tables",s="creating"}
@Func{FrSymbolTable *create_symbol_table(int size)}
@Func{FrSymbolTable::FrSymbolTable(int size = 0)}
Create a new symbol table of the specified initial size (or the default
size, if 0), and return a pointer to a record describing the symbol
table.  The caller is responsible for remembering any and all
additional symbol tables created in this manner.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{default_symbol_table}
@indexmeth{selectDefault}
@index2{p="symbol tables",s="switching"}
@Func{FrSymbolTable *default_symbol_table() ;}
@Func{static FrSymbolTable FrSymbolTable::selectDefault()}
Select the default symbol table created at FramepaC initialization
as the one to use for subsequent FramepaC calls for the purposes of
symbol manipulation, and return the symbol table which was active
before this call.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{destroy_symbol_table}
@indexdtor{FrSymbolTable}
@index2{p="symbol tables",s="destroying"}
@Func{void destroy_symbol_table(FrSymbolTable *symboltable)}
@Func{FrSymbolTable::~FrSymbolTable()}
Delete the specified symbol table and all frames associated with it.
Before using this function, you must be absolutely certain that you
have freed any objects other than frames which reference symbols
from the named table; failure to do so will leave FramepaC very
confused and could even crash your program.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{select_symbol_table}
@indexmeth{select}
@index2{p="symbol tables",s="switching"}
@Func{FrSymbolTable *select_symbol_table(FrSymbolTable *newtable)}
@Func{FrSymbolTable *FrSymbolTable::select()}
Make the specified symbol table the one which will be used by
subsequent FramepaC calls for the purposes of symbol manipulation, and
return the current symbol table.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{makeSymbol}
@Func{FrSymbol *makeSymbol(const FrSymbol *sym)}
Make a new symbol with the same name as the given symbol in the current
symbol table.  This function allows you to copy symbols from one symbol
table to another in order to correctly make comparisons between symbols
in different symbol tables.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{expand}
@Func{FrBool FrSymbolTable::expand(int increment)}
Expand the symbol table's capacity by the indicated number of symbols.  The
return value indicates whether the expansion was successful.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{expandTo}
@Func{FrBool FrSymbolTable::expandTo(int newsize)}
Expand the symbol table's capacity to the indicated number of symbols.  The
return value indicates whether the expansion was successful.  If the symbol
table is already at least the indicated size, this function does nothing and
returns successfully.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{current_symbol_table}
@indexmeth{current}
@Func{FrSymbolTable *current_symbol_table()}
@Func{static FrSymbolTable *FrSymbolTable::current()}
Return the currently active symbol table.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{add}
@Func{static FrSymbol *FrSymbolTable::add(const char *name)}
Insert a new symbol with the indicated name into the symbol table if it
does not already exist.  Return a pointer to the symbol in the symbol
table, whether newly created or pre-existing.  Unlike the FramepaC
reader, no conversions are performed on the name, so the symbol's name
will be the exact contents of the supplied string, even if the string
contains lowercase letters or special symbols.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{add}
@Func{static FrSymbol *FrSymbolTable::add(const char *name, 
                                        const FrObject *value)}
Insert a new symbol with the indicated name into the symbol table if it
does not already exist, or find the symbol if already present.  Set the
symbol's associated value to @t{value}, and return a pointer to the
symbol.  This function is only available if @t{FrSYMBOL_VALUE} is
#define'd.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{gensym}
@Func{FrSymbol *FrSymbolTable::gensym(const char *name)}
Generate a unique symbol, whose name begins with the indicated
characters.  This function generates a new symbol by appending an
incrementing number to the end of the supplied name.  Unlike most Lisp
implementations, FramepaC's @t{gensym} guarantees that the symbol did
not already exist in the symbol table (by repeated attempts with
differing distinguishing numbers).
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{lookup}
@Func{FrSymbol *FrSymbolTable::lookup(const char *name)}
Find the symbol with the specified name in the symbol table, and return
a pointer to it, or 0 if the symbol is not present in the symbol table.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{setNotify}
@Func{void FrSymbolTable::setNotify(VFrameNotifyType type,
                                    VFrameNotifyFunc *func)}
Set the function which should be called when a database server
notification of the indicated type of event (e.g. frame
creation/deletion/update) is received for the symbol table.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{getNotify}
@Func{VFrameNotifyFunc *FrSymbolTable::getNotify(VFrameNotifyType type)}
Retrieve the function which is currently being called when a database
server notification of the indicated type is received for the symbol table.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{setProxy}
@Func{void FrSymbolTable::setProxy(VFrameNotifyType type,
                                   VFrameProxyFunc *func)}
(network backing-store support function)@*
This function is used to specify the function which will be called when the
database server requires that the calling client perform some frame-update
action on behalf of another client.

!!!
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{getProxy}
@Func{VFrameProxyFunc *FrSymbolTable::getProxy(VFrameNotifyType type)}
(network backing-store support function)@*
Determine which function will be called when the database server requires
this client to perform a frame update on behalf of another client.

!!!
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{setDeleteHook}
@Func{void FrSymbolTable::setDeleteHook(FrSymTabDeleteFunc *delhook)}
Specify the function (if any) to be called when the symbol table is
deleted.  The @t{delhook} function, if not 0, will be called with the
symbol table as its sole argument when the symbol table is deleted,
before the frames and symbols associated with the symbol table are deleted.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{getDeleteHook}
@Func{FrSymTabDeleteFunc *FrSymbolTable::getDeleteHook() const}
Determine which function (if any) will be called when the symbol table
is deleted.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{setShutdown}
@Func{void FrSymbolTable::setShutdown(VFrameShutdownFunc *func)}
Specify which function should be called when the database server
providing the backing store for the symbol table indicates that it is
about to shut down.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{getShutdown}
@Func{VFrameShutdownFunc *FrSymbolTable::getShutdown()}
Retrieve the function which will be called when the database server for
the symbol table indicates that it is shutting down.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{isReadOnly}
@Func{FrBool FrSymbolTable::isReadOnly() const}
Determine whether the symbol table is associated with a read-only
backing store.  If this function returns @t{True}, then any changes
made to frames will be lost when the symbol table is closed with
@t{shutdown_VFrames} or a modified frame is discarded from memory.
Frames are still retrieved from the backing store on demand, but
modifications will be as fleeting as they are for a symbol table
without backing store.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{iterateFrame}
@Func{FrBool FrSymbolTable::iterateFrame(FrIteratorFunc func, ...) const}
Iterate through the symbol table in the same manner as @t{iterate}, but
call the indicated function once for each frame currently loaded in
memory which is named by a symbol in the symbol table.  Returns
@t{True} if every call to the specified function returned @t{True}, and
@t{False} otherwise.  The method @t{iterate} calls the specified
function once for each symbol in the symbol table, regardless of
whether a frame is associated with the symbol.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{iterateFrameVA}
@Func{FrBool FrSymbolTable::iterateFrameVA(FrIteratorFunc func, va_list args) const}
Same as @t{iterateFrame}, but this function accepts a variable argument
list rather than a list of arguments, which may be useful when called
from a function which itself takes a variable number of arguments.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{listRelations}
@Func{FrList *FrSymbolTable::listRelations() const}
Return a list of all of the bidirectional relations which have been
defined for the current symbol table; FramepaC automatically maintains
inverse links for the @t{VALUE} facets of any slots whose name is
contained in the list returned by this function.  The returned list
consists of a list of two-element lists, each of which contains the two
symbols naming the inverse links related to each other (note that a
link can be its own inverse, in which case the two elements of the
corresponding sublist are the same).  The returned list has been newly
created and must be explicitly freed (e.g. with @t{freeObject} once no
longer required.  See also @t{FrSymbol::defineRelation} and
@t{FrSymbol::undefineRelation}.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{do_all_symtabs}
@Func{void do_all_symtabs(DoAllSymtabsFunc *func, ...)}
Execute the indicated function once for every symbol table currently in
existence.  Any additional arguments given to @t{do_all_symtabs} are
passed directly to @t{func} as a variable-argument list.
@end{FuncDesc}

@comment{-------------------------------}
@section{FrString Functions}
@index{FrString class}
@index{strings}

The @t{FrString} class provides storage for character strings, where
the character size may be one, two, or four bytes (8-bit, 16-bit, or
32-bit character sets, such as ASCII/ANSI, Unicode, and ISO 10646).

@subsection{Constructors}

@begin{FuncDesc}
@indexctor{FrString}
@Func{FrString::FrString()}
Create an empty string.
@end{FuncDesc}

@begin{FuncDesc}
@indexctor{FrString}
@Func{FrString::FrString(const char *value)}
Create a new string consisting of the characters pointed at by
@t{value}, up to but not including the NUL character terminating the
C-style string.  The new string is assigned the default character size
of one byte.
@end{FuncDesc}

@begin{FuncDesc}
@indexctor{FrString}
@Func{FrString::FrString(FrChar_t value)}
Create a new string consisting of the single character specified by
@t{value}.  The new string's character width is set to the smallest
size which can contain all of the non-zero bits of @t{value}.
@end{FuncDesc}

@begin{FuncDesc}
@indexctor{FrString}
@Func{FrString::FrString(const char *value, int length, int width = 1)}
Create a new string consisting of the @t{length} characters pointed at
by @t{value}, each character consisting of @t{width} bytes (which may
be either 1, 2, or 4).  This string may contain NUL characters.  If
the specified width is other than one byte per character, the supplied
string must be a byte-wise representation of the characters in the
string, with big-endian (most-significant byte first) byte ordering.
@end{FuncDesc}

@begin{FuncDesc}
@index{concatenating strings}
@index2{p=strings,s=concatenating}
@indexoper{+}
@Func{FrString + FrString}
@Func{FrString + FrString*}
@indexmeth{concatenate}
@Func{FrString* FrString::concatenate(FrString*)}
@Func{FrString* FrString::concatenate(FrString&)}
Create a new string consisting of the contents of the two supplied strings.
The characters in the resulting string will be of the same width as
the wider character width in the two original strings.
@end{FuncDesc}

@subsection{String Manipulation}

FramepaC strings may be manipulated without being concerned with
the character width used by the strings, be it 8-bit, 16-bit, or 32-bit
characters.  Character widths are converted as necessary.

@begin{FuncDesc}
@index{appending strings}
@indexoper{+=}
@Func{FrString += char*}
@Func{FrString += FrChar_t}
@Func{FrString* FrString::append(char*)}
@Func{FrString* FrString::append(FrChar_t)}
Destructively append the C-style character string or the @t{FrChar_t}
to the end of the supplied FramepaC string (the C-style character
string remains unchanged).  The character string is assumed to consist
of eight-bit characters, which will be widened as necessary to match
the character size already present in the @t{FrString}.  The
@t{FrChar_t} is assumed to be of the smallest width necessary to
contain all of its non-zero bits, and the string's character width is
increased if necessary.
@end{FuncDesc}

@begin{FuncDesc}
@index{appending strings}
@indexoper{+=}
@Func{FrString += FrString}
@Func{FrString += FrString*}
@Func{FrString* FrString::append(FrString*)}
@Func{FrString* FrString::append(FrString&)}
Destructively append the second string to the end of the first (the
second string remains unchanged).  The characters in the resulting
string will be of the same width as the wider character width in the
two original strings.
@end{FuncDesc}

@begin{FuncDesc}
@index{character width}
@index2{p="strings",s="character width"}
@indexmeth{charWidth}
@Func{FrString::charWidth()}
Determine how many bytes are used to store each character in the specified
string.
@end{FuncDesc}

@begin{FuncDesc}
@index{string manipulation}
@index{string length}
@indexmeth{stringLength}
@index2{p="length",s="strings"}
@index2{p="strings",s="length"}
@Func{size_t FrString::stringLength()}
Determine the number of characters stored in the specified string.
This is a non-virtual function, and is thus faster than the equivalent
virtual function @t{length()} if the object is known to be a @t{FrString}.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{stringSize}
@Func{size_t FrString::stringSize() const}
Determine the number of bytes required to store the string's value,
excluding any terminating NUL character.
@end{FuncDesc}

@begin{FuncDesc}
@index2{p="strings",s="manipulating"}
@indexmeth{stringValue}
@Func{const char *FrString::stringValue()}
Returns a pointer to the buffer used to store the string's value.  This buffer
is @t{charWidth()}*@t{stringLength()} or @t{stringSize()} bytes in length.
@end{FuncDesc}

@begin{FuncDesc}
@indexoper{char*}
@Func{(char*)FrString}
Like @t{stringValue}, this operator returns a pointer to the buffer used to
store the string's value.
@end{FuncDesc}

@begin{FuncDesc}
@indexoper{[]}
@index2{p="strings",s="extracting characters"}
@Func{FrChar_t FrString::operator[size_t index]}
Returns the character (a value of type @t{FrChar_t}) at the indicated position
(counting from 0) in the string.  This operator may only be used as an rvalue;
to change the character at a particular position, use @t{setChar} or @t{setNth}.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{setChar}
@Func{FrString::setChar(size_t index, FrChar_t newchar)}
Destructively modify the string by replacing the indicated character in the
string by the new value.  If a @t{FrString} is to be used as the new
character, call @t{setNth} instead of this method.
@end{FuncDesc}

@subsection{Comparison}

@begin{FuncDesc}
@index{string comparison functions}
@indexfunc{FrString}
@Index2{p="comparison functions",s="FrString"}
@Func{FrString == FrString}
@Func{FrString != FrString}
@Func{FrString < FrString}
@Func{FrString <= FrString}
@Func{FrString > FrString}
@Func{FrString >= FrString}
Perform the appropriate comparison between the values of the two
strings.  Because of a C++ limitation, you will need to dereference
the @t{FrString*} which is what is normally available, i.e. use
@begin{programexample}
        *str1 == *str2
@end{programexample}
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{FrString}
@Index2{p="comparison functions",s="FrString"}
@index{string comparison functions}
@indexoper{==}
@indexoper{!=}
@indexoper{<}
@indexoper{<=}
@indexoper{>}
@indexoper{>=}
@Func{FrString == char*}
@Func{FrString != char*}
@Func{FrString < char*}
@Func{FrString <= char*}
@Func{FrString > char*}
@Func{FrString >= char*}
Perform the appropriate comparison between the value of the @t{FrString} and
the character string.  As before, you will need to dereference
@t{FrString*} variables.
@end{FuncDesc}

@begin{FuncDesc}
@indexvirt{locate}
@index{locating elements of an object}
@Func{virtual size_t FrString::locate(const FrObject *item, size_t start = (size_t)-1) const}
@Func{virtual size_t FrString::locate(const FrObject *item, FrCompareFunc func,
        size_t start = (size_t)-1) const}
Locate a character or substring of the given string.  The @t{item} may
be either a @t{FrString} or a @t{FrList} of @t{FrString}s and/or
@t{FrSymbol}s (which is treated as a single string for the purposes
of searching the given string).  The search begins following the
@t{start}h element of the list, which permits the result of a
@t{locate} to be passed to a subsequent call in order to search for
additional matches.  The return value is the index into the list of the
first match after @t{start}, or @t{(size_t)-1} if no match could be
found.

If provided, the comparison function @t{func} is called repeately to
determine which characters or substrings of the string match;
otherwise, a simple character-wise comparison is used.
@end{FuncDesc}

@subsection{Substrings}

@begin{programexample} !!!
FrString *FrFirstWord(const FrString *words) ;
FrString *FrLastWord(const FrString *words) ;
FrString *FrButLastWord(const FrString *words) ;
@end{programexample}


@comment{-------------------------------}
@section{FrNumber Functions}
@indextype{FrNumber}
@index2{p="classes",s="FrNumber"}

The class @t{FrNumber} is the base class for the two classes which
actually store numbers, @t{FrInteger} and @t{FrFloat}.  An @t{FrInteger}
stores a long integer, while @t{FrFloat} stores a double-precision
floating-point number.  You should never instantiate @t{FrNumber}, only
its subclasses.

@subsection{Constructors}

@begin{FuncDesc}
@indexctor{FrInteger}
@indexctor{FrFloat}
@Func{FrInteger(long value)}
@Func{FrFloat(double value)}
Create a new @t{FrNumber} object with the specified value.
@end{FuncDesc}

@subsection{Manipulation}

@begin{FuncDesc}
@indexmeth{floatValue}
@Func{double floatValue() const}
Determine the number's value as a floating-point number.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{fraction}
@Func{double fraction() const}
Determine the fractional portion of the number's value.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{intValue}
@Func{long intValue() const}
Determine the whole-number portion of the number's value.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{imag}
@Func{double imag() const}
Return the imaginary portion of a number.  @i{This method is intended
to support a future complex-number class.}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{real}
@Func{double real() const}
Return the real portion of a number.  @i{This method is intended to
support a future complex-number class; it is currently identical to
@t{floatValue()}.}
@end{FuncDesc}

@subsection{Comparison}

@begin{FuncDesc}
@indexfunc{FrNumber}
@Index2{p="comparison functions",s="FrNumber"}
@indexoper{==}
@indexoper{!=}
@indexoper{<}
@indexoper{<=}
@indexoper{>}
@indexoper{>=}
@Func{FrNumber == FrNumber}
@Func{FrNumber != FrNumber}
@Func{FrNumber < FrNumber}
@Func{FrNumber <= FrNumber}
@Func{FrNumber > FrNumber}
@Func{FrNumber >= FrNumber}
Perform the appropriate comparison, after coercing both @t{FrNumber}s to be
@t{FrFloat}s if either one is a @t{FrFloat}.  As before,
you'll need to dereference the @t{FrNumber*} which is normally what
you'll have to get C++ to properly recognize the overloaded
operator; for consistency, the right side is thus also a
@t{FrNumber} rather than a @t{FrNumber*}.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{FrNumber}
@Index2{p="comparison functions",s="FrNumber"}
@Func{FrNumber == long}
@Func{FrNumber != long}
@Func{FrNumber < long}
@Func{FrNumber <= long}
@Func{FrNumber > long}
@Func{FrNumber >= long}
Perform the appropriate comparison, coercing the @t{FrNumber} to be
an @t{FrInteger} to match the right-hand side.  As before,
you'll need to dereference the @t{FrNumber*} which is normally what
you'll have to get C++ to properly recognize the overloaded
operator.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{FrNumber}
@Index2{p="comparison functions",s="FrNumber"}
@indexoper{==}
@indexoper{!=}
@indexoper{<}
@indexoper{<=}
@indexoper{>}
@indexoper{>=}
@Func{FrNumber == double}
@Func{FrNumber != double}
@Func{FrNumber < double}
@Func{FrNumber <= double}
@Func{FrNumber > double}
@Func{FrNumber >= double}
Perform the appropriate comparison, coercing the @t{FrNumber} to be
@t{FrFloat} to match the right-hand side.  As before,
you'll need to dereference the @t{FrNumber*} which is normally what
you'll have to get C++ to properly recognize the overloaded
operator.
@end{FuncDesc}

@subsection{Other Operators}

@begin{FuncDesc}
@indexoper{double}
@Func{(double)FrNumber}
Convert the @t{FrNumber}'s value into a floating-point number.
@end{FuncDesc}

@begin{FuncDesc}
@indexoper{long}
@Func{(long)FrNumber}
Convert the @t{FrNumber}'s value into an integer, truncating it if the value is
a floating-point number.
@end{FuncDesc}


@comment{-------------------------------}
@section{FrList Functions}
@indextype{FrList}
@index{lists}

Lists are @i{the} major data structure following frames from the point
of view of FramepaC.  Lists are used to store the fillers in a frame,
and are thus used to specify arguments and return values for many frame
manipulation functions.  To assist in their use, FramepaC provides a
set of Lisp-like list-manipulation functions.

@subsection{Constructing Lists}
@index2{p="lists",s="constructing"}

@begin{FuncDesc}
@indexctor{FrList}
@Func{FrList(const FrObject *elt1)}
@Func{FrList(const FrObject *elt1, const FrObject *elt2)}
@Func{FrList(const FrObject *elt1, const FrObject *elt2, const FrObject *elt3)}
@Func{FrList(const FrObject *elt1, const FrObject *elt2, const FrObject *elt3, const FrObject *elt4)}
Create a new list of one to four elements.  These constructors may be
used as
@begin{programexample}
FrList *l = new FrList(new FrInteger(1),makeSymbol("FOO")) ;
@end{programexample}
Since a program would not use a @t{FrList} object as such (only pointers
to them), use of the constructors as
@begin{programexample}
FrList l(new FrInteger(1),makeSymbol("FOO")) ;
@end{programexample}
is inappropriate.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{copylist}
@Func{FrList *copylist(FrList *source) ;}
Create a new list whose elements are identical to the elements
of the specified list.  This function is equivalent to the virtual
method @t{copy()}.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{copytree}
@Func{FrList *copytree(FrList *source) ;}
Create a new list whose elements are themselves copies of the
elements of the specified list.  This function is equivalent to the virtual
method @t{deepcopy()}.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{makelist}
@Func{FrList *makelist(FrObject *obj1, ...) ;}
Create a list consisting of the specified objects.  The list of
objects to be placed in the list is terminated with a (FrObject*)0.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{nconc}
@Func{FrList *nconc(FrList *l1, FrList *l2)}
Destructively add the list @t{l2} to the end of list @t{l1},
returning a pointer to the combined list (in case @t{l1} is 0).
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{pushlist}
@Func{FrList *pushlist(FrObject *newval, FrList *&list) ;}
Add a new element to the beginning of the list pointed at by
the given pointer 'list' and update the pointer.
@end{FuncDesc}

@subsection{FrList Manipulation}
@index2{p="lists",s="manipulation"}

@begin{FuncDesc}
@indexfunc{listassoc}
@Func{FrCons *listassoc(const FrList *l, const FrObject *item)}
@Func{FrCons *listassoc(const FrList *l, const FrObject *item, FrCompareFunc cmp)}
Find the element of the given association list whose head
matches the specified item.  If given, the function @t{cmp} is called
to determine when a matching item is found; if omitted, a simple
pointer equality test (equivalent to Lisp's @t{eq}) is used.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{listdifference}
@index2{p="lists",s="difference"}
@Func{FrList *listdifference(const FrList *l1, const FrList *l2)}
@Func{FrList *listdifference(const FrList *l1, const FrList *l2, FrCompareFunc cmp)}
Return a list containing those elements of @t{l1} not contained in @t{l2}.
The returned list is always a copy of the values in @t{l1}, and may thus
be destructively modified (including destructive modification of the
list's elements) without affecting @t{l1}.  If given, the function
@t{cmp} is called to determine whether an item in @t{l1} is present in
@t{l2}; if omitted, a simple pointer equality test (equivalent to Lisp's
@t{eq}) is used.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{listequiv}
@index2{p="lists",s="comparing"}
@Func{FrBool listequiv(const FrList *l1, const FrList *l2)}
Determine whether the two lists contain equal elements, ignoring any
differences in order.  I.e.
@begin{programexample}
(A B "C") is equivalent to (A B "C")
(A C 2.5 "Efg" (A C)) is equivalent to ((A C) "Efg" 2.5 A C)
(1.5 42 17) is not equivalent to (17 42)
(1.5 42 17) is not equivalent to (1.5 42 17.1)
(A C 2.5 "Efg" (A C)) is not equivalent to (A C 2.5 "Efg" (C A))
@end{programexample}
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{listhead}
@Func{FrObject *listhead(FrList *list) ;}
Return the first element in the specified list.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{listintersection}
@index2{p="lists",s="intersection"}
@Func{FrList *listintersection(const FrList *l1, const FrList *l2)}
@Func{FrList *listintersection(const FrList *l1, const FrList *l2, FrCompareFunc cmp)}
Return a list containing all elements present in both @t{l1} and @t{l2}.
The returned list is always a copy of the values in the two original lists,
and may thus be destructively modified (including destructive modification of
the list's elements) without affecting either @t{l1} or @t{l2}. 
If given, the function @t{cmp} is called to determine whether an item is
present in both lists; if omitted, a simple pointer equality test
(equivalent to Lisp's @t{eq}) is used.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{listlength}
@index2{p="length",s="lists"}
@index2{p="lists",s="length"}
@Func{int listlength(FrList *list)}
Return the length of the specified list.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{listmember}
@Func{FrList *listmember(FrList *list, FrObject *item)}
@Func{FrList *listmember(FrList *list, FrObject *item, FrCompareFunc cmp)}
Determine whether the given object is an element of the
specified list, returning the sublist starting at the desired
element (if present) or 0 if not present.  If given, the function
@t{cmp} is called to determine when a matching item is found; if
omitted, a simple pointer equality test (equivalent to Lisp's @t{eq})
is used.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{listnth}
@Func{FrObject *listnth(FrList *list, size_t n)}
Retrieve the @i{n}th element of the given list, much as if the list were in
fact an array subscripted beginning with zero.  @t{listnth(l,0)} returns
the first element of the list, @t{listnth(l,1)} returns the second, etc.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{listremove}
@Func{FrList *listremove(const FrList *l, const FrObject *item)}
@Func{FrList *listremove(const FrList *l, const FrObject *item,FrCompareFunc cmp)}
Destructively remove the first element of the given list which matches
the specified item, returning a pointer to the modified list.  This
function should not be used if there are other pointers to the list
besides the one passed in, as the first element of the list may be
removed from the list and freed.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{listreverse}
@Func{FrList *listreverse(FrList *l)}
Destructively reverse the given list, returning a pointer to the
resulting reversed list.  Note that any other pointers to the original
list will end up pointing at a single-element list consisting of only
the first element of the original list.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{listsort}
@Func{FrList *listsort(FrList *list,int (*cmp)(const FrObject*, const FrObject*)}
Destructively sort the specified list, using the given function to determine
the desired ordering of elements, and return a pointer to the new beginning
of the list.  The @t{cmp} function should return zero if the two objects
are equivalent, a negative value if the first object should precede the second,
and a positive value if the first object should follow the second in the
sorted list.

Since this function is destructive, any other pointers to the list which is
sorted will end up pointing at arbitrary positions within the resulting
sorted list.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{listtail}
@Func{FrList *listtail(FrList *list) ;}
Return the remainder of the list, not including the first element.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{listunion}
@Func{FrList *listunion(const FrList *l1, const FrList *l2)}
@Func{FrList *listunion(const FrList *l1, const FrList *l2, FrCompareFunc cmp)}
Return a list containing the elements of @t{l1} plus those elements of
@t{l2} not already contained in @t{l1}.
The returned list is always a copy of the values in the two original lists,
and may thus be destructively modified (including destructive modification of
the list's elements) without affecting either @t{l1} or @t{l2}. 
If given, the function @t{cmp} is called to determine whether an item in
@t{l2} is present in @t{l1}; if omitted, a simple pointer equality test
(equivalent to Lisp's @t{eq}) is used.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{mapcan}
@Func{FrList *mapcan(FrList *list, ListMapFunc mapfunc, ...)}
Apply the given function to each element of the list, and combine the returned
lists with @t{nconc}.  The mapping function takes two arguments (a @t{FrObject}
and a @t{va_list} for the optional arguments) and returns a list.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{mapcar}
@Func{FrList *mapcar(FrList *list, ListMapFunc mapfunc, ...)}
Apply the given function to each element of the list, and return a list
containing the values returned by the mapping function.  The mapping function
takes two arguments (a @t{FrObject} and a @t{va_list} for the optional
arguments) and returns an arbitrary @t{FrObject}.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{poplist}
@Func{FrObject *poplist(FrList *&list)}
@Func{FrObject *poplist(FrList *&list, FrBool destructive)}
Remove the first element from the beginning of the specified
list, updating the list pointer 'list' and returning the
removed object.  The first form of the call and the second form
with a second argument of True will free the first cons cell in
the list; use the second form with @t{False} if you have other pointers
to the list besides the one passed to @t{poplist()}--but in that
case it may be just as easy to simply do
@begin{programexample}
        var = list->car() ;
        list = list->cdr() ;
@end{programexample}
instead of var=poplist(list,False).
@end{FuncDesc}

@subsection{Object-Oriented Interface}

@begin{FuncDesc}
@indexmeth{assoc}
@Func{int FrList::assoc(const FrObject *item)}
@Func{int FrList::assoc(const FrObject *item, FrCompareFunc cmp)}
Find the element of the given association list whose head
matches the specified item.  If given, the function @t{cmp} is called
to determine when a matching item is found; if omitted, a simple
pointer equality test (equivalent to Lisp's @t{eq}) is used.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{car}
@Func{FrObject *FrList::car()}
Retrieve the first item in the list.  This is a virtual function which
may be used on any object of type @t{FrObject} or subclasses thereof.
If the object is known to be a @t{FrCons} or a list, it is more
efficient to use the non-virtual function @t{first}.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{cdr}
@Func{FrObject *FrList::cdr()}
Retrieve the second item in a cons cell; in the case of a list, this
is the remainder of the list after removing the first element (which
is the null pointer 0 if the end of the list has been reached).  This
is a virtual function which may be used on any object of type
@t{FrObject} or subclasses thereof.  If the object is known to be a
list, it is more efficient to use the non-virtual function @t{rest}.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{difference}
@Func{FrList *FrList::difference(const FrList *l2)}
@Func{FrList *FrList::difference(const FrList *l2, FrCompareFunc cmp)}
Return a list containing those elements of the called object which are
not contained in @t{l2}.  The returned list is always a copy of the
values in the called object's list, and may thus
be destructively modified (including destructive modification of the
list's elements) without affecting the original object.  If given, the
function @t{cmp} is called to determine whether an item in the object's
list is present in @t{l2}; if omitted, a simple pointer equality test
(equivalent to Lisp's @t{eq}) is used.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{equal}
@Func{FrBool FrList::equal(const FrList *l2)}
Determine whether the other list contains equal elements in the same order
as the given list.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{equiv}
@Func{FrBool FrList::equiv(const FrList *l2)}
Determine whether the other list contains equal elements to those in the
given list, disregarding any differences in the order of the elements.
See also @t{listequiv}.
@end{FuncDesc}

@begin{FuncDesc}
@indexvirt{locate}
@index{locating elements of an object}
@Func{virtual size_t FrList::locate(const FrObject *item, size_t start = (size_t)-1) const}
@Func{virtual size_t FrList::locate(const FrObject *item, FrCompareFunc func,
        size_t start = (size_t)-1) const}
Locate an element or subsequence of the list.  If @t{item} is
@t{consp()}, this method locates the first sublist after @t{start}
matching the list pointed at by @t{item}; otherwise, it locates the
single element matching @t{item}.  Note that in order to locate a
single element which happens to be a list, you must wrap it in another,
one-element list.  The search begins following the @t{start}h element
of the list, which permits the result of a @t{locate} to be passed to a
subsequent call in order to search for additional matches.  The return
value is the index into the list of the first match after @t{start}, or
@t{(size_t)-1} if no match could be found.

If provided, the comparison function @t{func} is called repeately to
determine which element(s) of the list match; otherwise, a simple
pointer comparison (equivalent to Lisp's @t{eq}) is used.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{first}
@Func{FrObject *FrList::first()}
Retrieve the first element of the list.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{intersect}
@Func{FrBool FrList::intersect(const FrList *l2) const}
@Func{FrBool FrList::intersect(const FrList *l2, FrCompareFunc cmp) const}
Determine whether the two lists have any members in common.  If a
comparison function is provided, it is called repeatedly to determine
whether an equivalent item is present in each list; if omitted, a
simple pointer equality test (equivalent to Lisp's @t{eq}) is used.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{intersection}
@Func{FrList *FrList::intersection(const FrList *l2) const}
@Func{FrList *FrList::intersection(const FrList *l2, FrCompareFunc cmp) const}
Return a list containing all elements present in both the given list and
@t{l2}.
The returned list is always a copy of the values in the two original lists,
and may thus be destructively modified (including destructive modification of
the list's elements) without affecting either list.
If given, the function @t{cmp} is called to determine whether an item
is present in both lists; if omitted, a simple pointer equality test
(equivalent to Lisp's @t{eq}) is used.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{last}
@Func{FrList *FrList::last()}
Retrieve the one-element list containing the last element in the given list.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{length}
@index2{p="length",s="of a list"}
@Func{int FrList::listlength()}
Return the length of the specified list, including an empty list.
Unlike the generic function @t{length}, this method is not a virtual
method and may safely be called on any list, even the empty list (which
is represented as a null pointer).
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{listunion}
@Func{FrList *FrList::listunion(const FrList *l2) const}
@Func{FrList *FrList::listunion(const FrList *l2,FrCompareFunc cmp) const}
Return a list containing the elements of the given list and all
elements of @t{l2} not already contained in the given list.  All
elements of the result are copies of the items in the two original
list.

If given, the function @t{cmp} is called to determine whether an item
of @t{l2} is present in the given list; if omitted, a simple pointer
equality test (equivalent to Lisp's @t{eq}) is used.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{member}
@Func{int FrList::member(const FrObject *item)}
@Func{int FrList::member(const FrObject *item, FrCompareFunc cmp)}
Determine whether the given object is an element of the
specified list, returning the sublist starting at the desired
element (if present) or 0 if not present.  If present, the function
@t{cmp} is called to determine whether a matching item has been found
in the list; if omitted, a simple pointer equality test (equivalent to
Lisp's @t{eq}) is used.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{nconc}
@Func{FrObject *FrList::nconc(FrList *newtail)}
Destructively append the list @t{newtail} to the end of the original list.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{nth}
@Func{FrObject *FrList::nth(size_t n)}
Return the @i{n}th item in the list, counting from zero.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{nthcdr}
@Func{FrList *FrList::nthcdr(size_t n)}
Return the result of performing N calls to the @t{rest} method.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{}
@Func{FrObject *FrList::second()}
Returns the second element of the list, or 0 if the list has fewer than two
elements.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{third}
@Func{FrObject *FrList::third()}
Returns the third element of the list, or 0 if the list has fewer than three
elements.
@end{FuncDesc}

@begin{FuncDesc}
@indexoper{-}
@Func{FrList *FrList::operator - (const FrList *l2) const}
Return a new list containing all elements of the given list which are
not contained in the second list.  A simple pointer equality test
(equivalent to Lisp's @t{eq}) is used.  The returned list has been
newly created and must be explicitly freed once no longer required.
@end{FuncDesc}

@begin{FuncDesc}
@indexoper{+}
@Func{FrList *FrList::operator + (const FrList *l2) const}
Return a new list containing the union of the elments in the two given
lists.  A simple pointer equality test (equivalent to Lisp's @t{eq}) is
used to test membership.
@end{FuncDesc}

@begin{FuncDesc}
@indexoper{*}
@Func{FrList *FrList::operator * (const FrList *l2) const}
Return a new list containing the intersection of the elements in the two
given lists.  A simple pointer equality test is used to determine common
membership.
@end{FuncDesc}

@begin{FuncDesc}
@indexoper{[]}
@Func{FrObject *&FrList::operator [] (size_t position) const}
Return a reference to the element at the @t{position}th location in the list,
counting from zero.  Thus, @t{FrList[0]} is the list's head element, @t{FrList[1]}
is the second element, etc.  Modifying the returned value is equivalent
to calling @t{replaca} on the appropriate element of the list; as with
@t{replaca}, you must first free the object before replacing it in
order to avoid a memory leak.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{position}
@Func{int FrList::position(const FrObject *item) const}
@Func{int FrList::position(const FrObject *item, FrCompareFunc cmp) const}
@Func{int FrList::position(const FrObject *item, FrBool from_end) const}
@Func{int FrList::position(const FrObject *item, FrCompareFunc cmp,
        FrBool from_end) const}
Determine the relative position of @t{item} within the list (0 = first
element of list, 1 = second, etc.); returns -1 if no matching item is
present in the list.  If @t{from_end} is omitted or is @t{False}, this
method returns the position of the first occurrence of a matching item;
if present and @t{True}, this method returns the position of hte last
occurrence.  If @t{cmp} is specified, it is called for each item of the
list to determine whether a matching item is present in the list;
otherwise, a simple pointer comparison (equivalent to Lisp's @t{eq}) is
used.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{replaca}
@Func{FrObject *FrList::replaca()}
Destructively replace the first element in the original list.  You must
first free the object being replaced in order to avoid a memory leak
(unless there is another pointer to that object).
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{replacd}
@Func{FrObject *FrList::replacd()}
Destructively replace the tail of the given list.  If there is no other
pointer to the tail, you must first free it before replacing it in
order to avoid a memory leak.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{rest}
@Func{FrObject *FrList::rest()}
Retrieve the remainder of the list, i.e. the list that would be
created by removing the first element of the original list.  If the
specified list has only one element, the null pointer 0 is returned.
Any destructive modifications to the returned list will also affect the
original list.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{sort}
@Func{FrList *FrList::sort(int (*cmpfunc)(const FrObject *,const FrObject *))}
Destructively sort the specified list, using the given function to determine
the desired ordering of elements, and return a pointer to the new beginning
of the list.  The @t{cmp} function should return zero if the two objects
are equivalent, a negative value if the first object should precede the second,
and a positive value if the first object should follow the second in the
sorted list.

Since this function is destructive, any other pointers to the list which is
sorted will end up pointing at unpredictable positions within the resulting
sorted list.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{subsetOf}
@Func{FrBool FrList::subsetOf(const FrList *l2) const}
@Func{FrBool FrList::subsetOf(const FrList *l2, FrCompareFunc cmp) const}
Determine whether the list is a subset of the indicated list (contains
only items which are also in @t{l2}).  If a comparison function is
provided, it is called repeatedly to determine whether an equivalent
item is present in each list; if omitted, a simple pointer equality
test (equivalent to Lisp's @t{eq}) is used.
@end{FuncDesc}

@comment{-------------------------------}
@section{Structure Functions}
@indextype{FrStruct}
@index{structures}
@index{records}

FramepaC's @t{FrStruct} type is the equivalent of Lisp's structure, and
may be read from a file or written to a file in a Lisp-compatible
format.  Unlike Lisp's @t{(defstruct)} however, @t{FrStruct} does not
have a statically-declared set of fields -- arbitrary fields may be
added to any @t{FrStruct} instance at any time.  In this respect,
@t{FrStruct} is more akin to an associative array than to a record.

@begin{FuncDesc}
@indexctor{FrStruct}
@Func{FrStruct::FrStruct(const FrSymbol *type)}
Create a new structure with the specified type tag.  The structure is
initially empty.
@end{FuncDesc}

@begin{FuncDesc}
@indexdtor{FrStruct}
@Func{FrStruct::~FrStruct()}
Destroy the indicated structure, removing all items stored in it.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{put}
@Func{void FrStruct::put(const FrSymbol *fieldname, const FrObject *value)}
Add the specified field to the structure if it does not already exist,
and set the field's value to be a copy of the indicated object.  If the
field already exists and it not empty, the existing value will be
discarded via @t{freeObject()}.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{get}
@Func{FrObject *FrStruct::get(const FrSymbol *fieldname)}
Retrieve the value associated with the indicated field, if the field
exists.  Returns 0 if the field does not exist in the structure or the
field is empty.  The returned pointer points at the actual object
stored in the structure, so any destructive modification of the return
value will be reflected in any future retrievals (such an action is not
recommended and not guaranteed to work in the future).
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{typeName}
@Func{FrSymbol *FrStruct::typeName() const}
Get the type tag associated with the structure when it was created.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{fieldNames}
@Func{FrList *FrStruct::fieldNames() const}
Return a list consisting of the names of the fields currently present
in the structure.  This list is newly-created and must be deleted (e.g.
with @t{freeObject}) when no longer required.
@end{FuncDesc}

@begin{FuncDesc}
@indexvirt{iterateVA}
@Func{virtual FrBool FrStruct::iterateVA(FrIteratorFunc func, va_list args) const}
Call the specified function @t{func} once for each field in the
structure.  The function is called with a @t{FrCons} of the field name
and field value and the variable-argument list @t{args}.  If any
invocation of @t{func} returns @t{False}, the iteration is terminated
immediately and @t{iterateVA} returns @t{False}; otherwise, this method
returns @t{True}.

DO NOT MODIFY THE CONTENTS OF THE FrCONS PASSED TO THE ITERATION
FUNCTION.   You should also make a copy of the @t{FrCons} if you need
to refer to it again after modifying the structure with @t{put}.
@end{FuncDesc}

@comment{-------------------------------}
@section{FrQueue Functions}

Since queues are a useful data structure for many algorithms, FramepaC
includes a @t{FrQueue} data type providing a single-ended queue.

@begin{FuncDesc}
@indexctor{FrQueue}
@Func{FrQueue::FrQueue()}
Create an empty queue.
@end{FuncDesc}

@begin{FuncDesc}
@indexctor{FrQueue}
@Func{FrQueue::FrQueue(const FrList *items)}
Create a queue containing the specified items.  The first element of
the list becomes the queue's head (first item to be returned on
reading) and the last element of the list becomes the tail of the queue.
@end{FuncDesc}

@begin{FuncDesc}
@indexdtor{FrQueue}
@Func{FrQueue::~FrQueue()}
Destroy the queue.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{add}
@Func{void FrQueue::add(const FrObject *item, FrBool copy = True)}
Add the specified item to the tail end of the queue.  By default, a
complete copy of the item is made and that copy actually placed on the
queue; if the supplied object was dynamically allocated and will never
again be modified by the calling program, this extra copying step may
be bypassed by supplying @t{False} as the second argument.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{addFront}
@Func{void FrQueue::addFront(const FrObject *item, FrBool copy = True)}
Add the specified item to the head end of the queue, where it becomes
the next item to be returned by @t{pop} or @t{peek}.  By default, a
complete copy of the item is made and that copy actually placed on the
queue; if the supplied object was dynamically allocated and will never
again be modified by the calling program, this extra copying step may
be bypassed by supplying @t{False} as the second argument.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{remove}
@Func{FrBool FrQueue::remove(const FrObject *item)}
Remove the first item in the queue which is identical to (pointer
equality) the specified item from the queue.  This function returns
@t{True} if anything was removed, @t{False} if not.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{remove}
@Func{FrBool FrQueue::remove(const FrObject *item, FrCompareFunc cmp)}
Remove the first item in the queue which compares as equal to the
specified item (via a call to the supplied function @t{cmp}) from the queue.
This function returns @t{True} if anything was removed, @t{False} if not.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{find}
@Func{FrObject *FrQueue::find(const FrObject *item, FrCompareFunc cmp) const}
Find the first item in the queue which compares as equal to the
specified item (via a call to the supplied function @t{cmp}), and
return a pointer to that item, or 0 if no matching item is contained in
the queue.
@end{FuncDesc}

@begin{FuncDesc}
@indexvirt{locate}
@index{locating elements of an object}
@Func{virtual size_t FrQueue::locate(const FrObject *item, size_t start = (size_t)-1) const}
@Func{virtual size_t FrQueue::locate(const FrObject *item, FrCompareFunc func,
        size_t start = (size_t)-1) const}
Locate an element or subsequence of the queue.  If @t{item} is
@t{consp()}, this method locates the first subsequence in the queue
after @t{start} matching the list pointed at by @t{item}; otherwise, it
locates the single element matching @t{item}.  Note that in order to
locate a single element which happens to be a list, you must wrap it in
another, one-element list.  The search begins following the @t{start}h
element of the queue, which permits the result of a @t{locate} to be
passed to a subsequent call in order to search for additional matches.
The return value is the index into the queue of the first match after
@t{start}, or @t{(size_t)-1} if no match could be found.

If provided, the comparison function @t{func} is called repeately to
determine which element(s) of the queue match; otherwise, a simple
pointer comparison (equivalent to Lisp's @t{eq}) is used.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{pop}
@Func{FrObject *FrQueue::pop()}
Remove the first item in the queue and return a pointer to it.  The
returned item must be deallocated with @t{freeObject} or
@t{free_object} once it is no longer needed.  This method returns 0 if
the queue is already empty.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{peek}
@Func{FrObject *FrQueue::peek() const}
Return the first item in the queue without removing it, or 0 if the
queue is currently empty.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{clear}
@Func{void FrQueue::clear()}
Remove all items from the queue.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{queueLength}
@index2{p="length",s="queues"}
@Func{size_t FrQueue::queueLength()}
Return the number of items currently in the queue.
@end{FuncDesc}

@comment{-------------------------------}
@section{Frame Functions}

Since frame manipulation is the main purpose of FramepaC, it provides
an extensive set of functions which operate on instances of the class
@t{FrFrame}.  Each frame is associated with a symbol, which is its
name.  Since frames are associated with symbols, they are also
associated with a symbol table, and will thus automatically be deleted
when their symbol table is destroyed.  Unless otherwise specified, all
frame-related functions operate using symbols from the current symbol
table, which allows multiple sets of frames to coexist even if some of
their names clash.

@subsection{Creating and Loading Frames}

@begin{FuncDesc}
@indexfunc{create_frame}
@Func{FrFrame *create_frame(FrSymbol *name)}
Create a new frame if one with the given name does not exist,
or return the existing frame by that name.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{create_vframe}
@Func{FrFrame *create_vframe(FrSymbol *name)}
Create a new virtual frame if no frame with the given name exists,
or return the existing frame by that name (fetching it from
the server or backing store if necessary).
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{find_frame}
@Func{FrFrame *find_frame(FrSymbol *name)}
Return a pointer to the frame associated with the given
symbol (i.e. the frame with the given name) or 0 if there
is no frame by that name currently in memory.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{find_vframe}
@Func{VFrame *find_vframe(FrSymbol *name)}
Return a pointer to the frame associated with the given
symbol (i.e. the frame with the given name) or 0 if there
is no frame by that name.  If necessary, the frame will be
fetched from the server or backing store.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{get_old_frame}
@Func{FrFrame *get_old_frame(FrSymbol *frame,int generation)}
If the symbol is the name of a virtual frame with backing store, this
function reloads the requested prior version of the frame from the backing
store (generation 0 is the most recently saved copy of the frame, generation
1 is the next most-recent copy, generation 2 is the version prior to that,
etc.).  This function has no effect if there is no backing store for the
desired frame.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{create_slot}
@Func{void create_slot(FrFrame *frame, FrSymbol *slotname)}
@Func{void create_slot(FrSymbol *frame, FrSymbol *slotname)}
Create a slot with the specified name if it does not already
exist in the frame.  Note that it is generally not necessary to
call this function, since all the @t{add_XX} functions create slots
as needed.

A slot always contains either two or three facets (depending on
compile-time options), even when newly-created, though they are
initially empty; additional facets will be created as fillers for them
are added.  The facets which always exist are @t{VALUE}, @t{SEM},
and optionally @t{INHERITS} (used for local or "sideways"
inheritance).

The first variant operates only on in-memory frames, while the second
variant is for virtual frames and will fetch the frame if necessary.
@end{FuncDesc}

@subsection{Testing Frames}

@begin{FuncDesc}
@indexfunc{frame_is_empty}
@Func{FrBool frame_is_empty(const FrFrame *frame)}
@Func{FrBool frame_is_empty(const FrSymbol *name)}
Determine whether there are any fillers in the frame at
all.  This will normally return True only for frames which
have just been created or erased.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{is_a_p}
@Func{FrBool is_a_p(FrFrame *frame, FrFrame *possible_parent)}
@Func{FrBool is_a_p(FrSymbol *frame, FrSymbol *possible_parent)}
Determine whether one frame is an ancestor of another by following
the @t{INSTANCE-OF} and @t{IS-A} links.  Returns @t{True} if
@t{possible_parent} can be reached from @t{frame} through some chain of
@t{IS-A} links, possibly with an initial @t{INSTANCE-OF} link.

The former variant operates only on in-memory frames, while the latter
variant is for virtual frames and will fetch the frame as necessary.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{part_of_p}
@Func{FrBool part_of_p(FrFrame *frame, FrFrame *possible_container)}
@Func{FrBool part_of_p(FrSymbol *frame, FrSymbol *possible_container)}
Determine whether one entity described by a frame is included as a
part of another by following the @t{PART-OF} link.  Returns @t{True}
if @t{possible_container} can be reached from @t{frame} through some
chain of @t{PART-OF} links.

The former variant operates only on in-memory frames, while the latter
variant is for virtual frames and will fetch the frame as necessary.
@end{FuncDesc}


@subsection{Manipulating Frames}

@begin{FuncDesc}
@indexfunc{add_filler}
@Func{void add_filler(FrFrame *frame, const FrSymbol *slot, const FrSymbol *facet,FrObject *filler)}
@Func{void add_filler(FrSymbol *frame, const FrSymbol *slot, const FrSymbol *facet,FrObject *filler)}
Append the specified filler to the end of the list of fillers
for the indicated slot and facet if it is not already present.
The caller may discard or otherwise destructively modify the
specified filler once this function returns, as the value actually
added to the frame is a copy.

The first variant operates only on in-memory frames, while the second
variant is for virtual frames and will fetch the frame if necessary.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{add_value}
@Func{void add_value(FrFrame *frame, const FrSymbol *slot,FrObject *filler)}
@Func{void add_value(FrSymbol *frame, const FrSymbol *slot,FrObject *filler)}
Append the specified filler to the end of the list of fillers
for the indicated slot's @t{VALUE} facet if it is not already present.
The caller may discard or otherwise destructively modify the
specified filler once this function returns, as the value actually
added to the frame is a copy.

The first variant operates only on in-memory frames, while the second
variant is for virtual frames and will fetch the frame if necessary.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{add_sem}
@Func{void add_sem(FrFrame *frame, const FrSymbol *slot,FrObject *filler)}
@Func{void add_sem(FrSymbol *frame, const FrSymbol *slot,FrObject *filler)}
Append the specified filler to the end of the list of fillers
for the indicated slot's @t{SEM} facet if it is not already present.
The caller may discard or otherwise destructively modify the
specified filler once this function returns, as the value actually
added to the frame is a copy.

The first variant operates only on in-memory frames, while the second
variant is for virtual frames and will fetch the frame if necessary.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{add_fillers}
@Func{void add_fillers(FrFrame *frame, const FrSymbol *slot, const FrSymbol *facet,FrList *fillers)}
@Func{void add_fillers(FrSymbol *frame, const FrSymbol *slot, const FrSymbol *facet,FrList *fillers)}
Append each item in the given list of fillers to the specified slot and
facet.  The caller may discard or otherwise destructively modify the
specified fillers once this function returns, as the values actually
added to the frame are copies of the supplied values.

The first variant operates only on in-memory frames, while the second
variant is for virtual frames and will fetch the frame if necessary.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{add_values}
@Func{void add_values(FrFrame *frame, const FrSymbol *slot,FrList *fillers)}
@Func{void add_values(FrSymbol *frame, const FrSymbol *slot,FrList *fillers)}
Append each item in the given list of fillers to the specified slot's
@t{VALUE} facet.  The caller may discard or otherwise destructively modify
the specified fillers once this function returns, as the values
actually added to the frame are copies of the supplied values.

The first variant operates only on in-memory frames, while the second
variant is for virtual frames and will fetch the frame if necessary.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{add_sems}
@Func{void add_sems(FrFrame *frame, const FrSymbol *slot,FrList *fillers)}
@Func{void add_sems(FrSymbol *frame, const FrSymbol *slot,FrList *fillers)}
Append each item in the given list of fillers to the specified slot's
@t{SEM} facet.  The caller may discard or otherwise destructively modify
the specified fillers once this function returns, as the values
actually added to the frame are copies of the supplied values.

The first variant operates only on in-memory frames, while the second
variant is for virtual frames and will fetch the frame if necessary.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{erase_facet}
@Func{void erase_facet(FrFrame *frame, const FrSymbol *slot, const FrSymbol *facet)}
@Func{void erase_facet(FrSymbol *frame, const FrSymbol *slot, const FrSymbol *facet)}
Delete all fillers from the specified facet.

The first variant operates only on in-memory frames, while the second
variant is for virtual frames and will fetch the frame if necessary.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{erase_filler}
@Func{void erase_filler(FrFrame *frame, FrSymbol *slot, FrSymbol *facet, FrObject *filler)}
@Func{void erase_filler(FrFrame *frame, FrSymbol *slot, FrSymbol *facet, FrObject *filler, FrCompareFunc cmp)}
@Func{void erase_filler(FrSymbol *frame, FrSymbol *slot, FrSymbol *facet, FrObject *filler)}
@Func{void erase_filler(FrSymbol *frame, FrSymbol *slot, FrSymbol *facet, FrObject *filler, FrCompareFunc cmp)}
Delete the specified filler in the indicated slot's facet, using the
specified comparison function (currently @t{eql} or @t{equal}, see
Section @ref{commonproc}) for the second form.  The first form always
uses a simple pointer comparison, the equivalent of Lisp's @t{eq}.

The first two variants operate only on in-memory frames, while the
final two variants are for virtual frames and will fetch the frame if
necessary.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{erase_frame}
@Func{void erase_frame(FrFrame *frame)}
@Func{void erase_frame(FrSymbol *frame)}
Delete all fillers for the entire frame, but do not deallocate the
frame proper.

The first variant operates only on in-memory frames, while the second
variant is for virtual frames and will fetch the frame if necessary.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{erase_sem}
@Func{void erase_sem(FrFrame *frame, const FrSymbol *slot, FrObject *filler)}
@Func{void erase_sem(FrFrame *frame, const FrSymbol *slot, FrObject *filler, FrCompareFunc cmp)}
@Func{void erase_sem(FrSymbol *frame, const FrSymbol *slot, FrObject *filler)}
@Func{void erase_sem(FrSymbol *frame, const FrSymbol *slot,FrObject *filler, FrCompareFunc cmp)}
Delete the specified filler from the indicated slot's @t{SEM} facet,
using the specified comparison function for the second form.  The two
comparison functions provided standard with FramepaC are @t{eql} and
@t{equal} (see Section @ref{commonproc}); if no comparison function is
specified, a simple pointer equality test (equivalent to Lisp's
@t{eq}) is used. 

The first two variants operate only on in-memory frames, while the
final two variants are for virtual frames and will fetch the frame if
necessary.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{erase_slot}
@Func{void erase_slot(FrFrame *frame, const FrSymbol *slot)}
@Func{void erase_slot(FrSymbol *frame, const FrSymbol *slot)}
Delete all fillers from all facets of the specified slot.

The first variant operates only on in-memory frames, while the second
variant is for virtual frames and will fetch the frame if necessary.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{erase_value}
@Func{void erase_value(FrFrame *frame, const FrSymbol *slot, FrObject *filler)}
@Func{void erase_value(FrFrame *frame, const FrSymbol *slot, FrObject *filler, FrCompareFunc cmp)}
@Func{void erase_value(FrSymbol *frame, const FrSymbol *slot, FrObject *filler)}
@Func{void erase_value(FrSymbol *frame, const FrSymbol *slot,FrObject *filler, FrCompareFunc cmp)}
Delete the specified filler from the indicated slot's @t{VALUE} facet,
using the specified comparison function for the second form.  The two
comparison functions provided standard with FramepaC are @t{eql} and
@t{equal} (see Section @ref{commonproc}); if no comparison function is
specified, a simple pointer equality test (equivalent to Lisp's
@t{eq}) is used. 

The first two variants operate only on in-memory frames, while the
final two variants are for virtual frames and will fetch the frame if
necessary.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{first_filler}
@Func{FrObject *first_filler(FrFrame *frame, const FrSymbol *slot, const FrSymbol *facet)}
@Func{FrObject *first_filler(FrFrame *frame, const FrSymbol *slot, const FrSymbol *facet, FrBool inherit)}
@Func{FrObject *first_filler(FrSymbol *frame, const FrSymbol *slot, const FrSymbol *facet)}
@Func{FrObject *first_filler(FrSymbol *frame, const FrSymbol *slot, const FrSymbol *facet, FrBool inherit)}
Retrieve the first filler for the given slot and facet.
DO NOT DESTRUCTIVELY MODIFY THE RETURNED FILLER.
For the second and fourth forms, if @t{inherit} is @t{False}, the value
(FrList *)0 will be returned if there are no fillers for the facet in the
given frame; otherwise, if there are no fillers, FramepaC attempts to
retrieve the first inherited filler.

The first two variants operate only on in-memory frames, while the
final two variants are for virtual frames and will fetch the frame if
necessary.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{get_fillers}
@Func{FrList *get_fillers(FrFrame *frame, FrSymbol *slot, FrSymbol *facet)}
@Func{FrList *get_fillers(FrFrame *frame, FrSymbol *slot, FrSymbol *facet,FrBool inherit)}
@Func{FrList *get_fillers(FrSymbol *frame, FrSymbol *slot, FrSymbol *facet)}
@Func{FrList *get_fillers(FrSymbol *frame, FrSymbol *slot, FrSymbol *facet, FrBool inherit)}
Retrieve the list of fillers for the given slot and facet.
DO NOT MODIFY THE RETURNED LIST.  For the second form and fourth forms, if
@t{inherit} is @t{False}, the value (FrList *)0 will be returned if
there are no fillers for that facet in the frame; otherwise, if there
are no fillers, FramepaC attempts to retrieve inherited
fillers by following the INSTANCE-OF and IS-A links.

The first two variants operate only on in-memory frames, while the
final two variants are for virtual frames and will fetch the frame if
necessary.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{get_sem}
@Func{FrList *get_sem(FrFrame *frame, FrSymbol *slot)}
@Func{FrList *get_sem(FrFrame *frame, FrSymbol *slot,FrBool inherit)}
@Func{FrList *get_sem(FrSymbol *frame, FrSymbol *slot)}
@Func{FrList *get_sem(FrSymbol *frame, FrSymbol *slot,FrBool inherit)}
Retrieve the list of fillers for the given slot's @t{SEM} facet.
DO NOT MODIFY THE RETURNED LIST.  For the second form and fourth forms, if
@t{inherit} is @t{False}, the value (FrList *)0 will be returned if
there are no fillers for that facet; otherwise, if there
are no fillers, FramepaC attempts to retrieve inherited
fillers by following the INSTANCE-OF and IS-A links.

The first two variants operate only on in-memory frames, while the
final two variants are for virtual frames and will fetch the frame if
necessary.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{get_values}
@Func{FrList *get_values(FrFrame *frame, FrSymbol *slot)}
@Func{FrList *get_values(FrFrame *frame, FrSymbol *slot,FrBool inherit)}
@Func{FrList *get_values(FrSymbol *frame, FrSymbol *slot)}
@Func{FrList *get_values(FrSymbol *frame, FrSymbol *slot,FrBool inherit)}
Retrieve the list of fillers for the given slot's @t{VALUE} facet.
DO NOT MODIFY THE RETURNED LIST.  For the second form and fourth forms, if
'inherit' is @t{False}, the value (FrList *)0 will be returned if
there are no fillers for that facet; otherwise, if there
are no fillers, FramepaC attempts to retrieve inherited
fillers by following the INSTANCE-OF and IS-A links.

The first two variants operate only on in-memory frames, while the
final two variants are for virtual frames and will fetch the frame if
necessary.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{get_value}
@Func{FrObject *get_value(FrFrame *frame, const FrSymbol *slot)}
@Func{FrObject *get_value(FrFrame *frame, const FrSymbol *slot, FrBool inherit)}
@Func{FrObject *get_value(FrSymbol *frame, const FrSymbol *slot)}
@Func{FrObject *get_value(FrSymbol *frame, const FrSymbol *slot, FrBool inherit)}
Retrieve the first filler for the given slot's @t{VALUE} facet.
DO NOT DESTRUCTIVELY MODIFY THE RETURNED FILLER.

The first two variants operate only on in-memory frames, while the
final two variants are for virtual frames and will fetch the frame if
necessary.
@end{FuncDesc}

@subsection{Processing Entire Frames}

@begin{FuncDesc}
@indexfunc{do_slots}
@Func{FrBool do_slots(FrFrame *frame, FrBool (*func)(const FrFrame *frame, const FrSymbol *slot, va_list args), ...)}
@Func{FrBool do_slots(FrSymbol *frame, FrBool (*func)(const FrFrame *frame, const FrSymbol *slot, va_list args), ...)}
Call the indicated function once for each slot in the given frame.
Returns @t{True} if all invocations of the function returned @t{True},
@t{False} if any of them returned @t{False} (in which case no further
slots are processed).  The final parameter passed to the
indicated function is a variable argument list consisting of
all the remaining arguments passed to @t{do_slots()}; these may be
accessed with @t{va_start}, @t{va_arg}, and @t{va_end}.

The former variant operates only on in-memory frames, while the latter
variant is for virtual frames and will fetch the frame as necessary.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{do_facets}
@Func{FrBool do_facets(FrFrame *frame, FrSymbol *slot,
                FrBool (*func)(const FrFrame *frame, const FrSymbol *slot, const FrSymbol *facet, va_list args), ...)}
@Func{FrBool do_facets(FrSymbol *frame, FrSymbol *slot,
                FrBool (*func)(const FrFrame *frame, const FrSymbol *slot, const FrSymbol *facet, va_list args), ...)}
Call the indicated function once for each nonempty facet in the
given slot. Returns @t{True} if all invocations of the function
returned @t{True}, @t{False} if any of them returned @t{False} (in which case
no further facets are processed).  The final parameter passed to the
indicated function is a variable argument list consisting of
all the remaining arguments passed to @t{do_facets()}; these may be
accessed with @t{va_start}, @t{va_arg}, and @t{va_end}.

The former variant operates only on in-memory frames, while the latter
variant is for virtual frames and will fetch the frame as necessary.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{do_all_facets}
@Func{FrBool do_all_facets(FrFrame *frame,
                     FrBool (*func)(const FrFrame *frame, const FrSymbol *slot, const FrSymbol *facet, va_list args),
                     ...)}
@Func{FrBool do_all_facets(FrSymbol *frame,
                     FrBool (*func)(const FrFrame *frame, const FrSymbol *slot, const FrSymbol *facet, va_list args),
                     ...)}
Call the indicated function once for each nonempty facet in the
given frame. Returns @t{True} if all invocations of the function
returned @t{True}, @t{False} if any of them returned @t{False} (in which case
no further facets are processed.  The final parameter passed to the
indicated function is a variable argument list consisting of
all the remaining arguments passed to @t{do_all_facets()}; these may be
accessed with @t{va_start}, @t{va_arg}, and @t{va_end}.

The former variant operates only on in-memory frames, while the latter
variant is for virtual frames and will fetch the frame as necessary.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{doAllFrames}
@Func{void doAllFrames(void (*func)(const FrFrame *frame, va_list args), ...)}
Invoke the specified function for each frame which is currently
instantiated for the active symbol table (see Section
@ref{advsymfuncs}).  Note that when using virtual frames, only those
frames which are currently in memory will be processed; frames which
are still in the backing store will be skipped.  The indicated
function is called with a pointer to the frame and any additional
arguments provided to @t{doAllFrames()}.
@end{FuncDesc}

@begin{FuncDesc}
@indexvirt{iterateVA}
@Func{virtual FrBool FrFrame::iterateVA(FrIteratorFunc func, va_list args) const}
Call the specified function @t{func} once for each facet currently in the
frame which have fillers.  The function is called with a @t{FrList} of
the frame's name, the slot name, the facet name, and the filler list
for the facet, as well as the variable-argument list @t{args}.  If any
invocation of @t{func} returns @t{False}, the iteration is terminated
immediately and @t{iterateVA} returns @t{False}; otherwise, this method
returns @t{True}.

DO NOT MODIFY THE CONTENTS OF THE FrList PASSED TO THE ITERATION
FUNCTION.  You should also make a deep copy of the @t{FrList} if you
need to refer to it again after modifying the frame with any of the
other methods.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{allSlots}
@Func{FrList *FrFrame::allSlots() const}
@Func{FrList *FrSymbol::allSlots()}
Returns a list consisting of the names of all slots within the given frame,
whether or not the slot has any fillers at all (generally, only the predefined
slots will ever exist without having any fillers).  The returned list
has been newly created and must be explicitly freed once no longer required.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{facets_in_slot}
@Func{FrList *facets_in_slot(const FrFrame *frame, const FrSymbol *slotname)}
@Func{FrList *facets_in_slot(FrSymbol *framename, const FrSymbol *slotname)}
Returns a list consisting of the names of all facets within the
indicated slot, whether or not the facet has any fillers at all
(generally, only the predefined slots--@t{VALUE}, @t{SEM}, and
optionally @t{INHERITS}--will ever exist without having any fillers).
The returned list has been newly created and must be explicitly freed
once no longer required.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{slots_in_frame}
@Func{FrList *slots_in_frame(const FrFrame *frame)}
@Func{FrList *slots_in_frame(FrSymbol *framename)}
Returns a list consisting of the names of all slots within the given
frame, whether or not the slot has any fillers at all (generally, only
the predefined slots will ever exist without having any fillers).  The
returned list has been newly created and must be explicitly freed once
no longer required.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{slotFacets}
@Func{FrList *FrFrame::slotFacets(const FrSymbol *slotname) const}
@Func{FrList *FrSymbol::slotFacets(const FrSymbol *slotname)}
Returns a list consisting of the names of all facets within the
indicated slot, whether or not the facet has any fillers at all
(generally, only the predefined slots--@t{VALUE}, @t{SEM}, and
optionally @t{INHERITS}--will ever exist without having any fillers).
The returned list has been newly created and must be explicitly freed
once no longer required.
@end{FuncDesc}

@subsection{Inheritance}

Inheritance is a powerful capability afforded by frames.  When a value
is not know for the current frame, the system can search for a value
in other frames specified by some inheritance link and search
mechanism.  If a value is found in this way, it is returned as if it
had been in the frame from the beginning.

@begin{FuncDesc}
@indexfunc{set_inheritance_type}
@Func{void set_inheritance_type(InheritanceType inherit)}
Specify how FramepaC should search for values for a slot which has no
values in the current frame.  The currently supported values are
@t{NoInherit}, @t{InheritSimple} (only follow first filler of
INSTANCE-OF and IS-A slots), @t{InheritDFS} (depth-first search on
IS-A), @t{InheritBFS} (breadth-first search on IS-A),
@t{InheritPartDFS} (depth-first search on PART-OF), and
@t{InheritLocalDFS} (first try to follow slot's @t{INHERITS} facet, then do
regular depth-first search).  In addition, the symbols @t{InheritPartBFS}
and @t{InheritLocalBFS} are defined but not yet supported.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{get_inheritance_type}
@Func{InheritanceType get_inheritance_type()}
Determine how values are inherited for slots with no values of
their own.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{inheritable_slots}
@Func{FrList *inheritable_slots(const FrFrame *frame, InheritanceType inherit)}
@Func{FrList *inheritable_slots(FrSymbol *frame, InheritanceType inherit)}
Determine which slots and facets the given frame may inherit from one or more
of its ancestors using the indicated inheritance method.  The result is
returned as an association list of slots with the facets available for the
slot.  For example,
@begin{programexample}
((IS-A VALUE SEM)
 (SUBCLASSES VALUE SEM)
 (INSTANCE-OF VALUE SEM)
 (INSTANCES VALUE SEM)
 (WEIGHT VALUE SEM M-UNIT DEFAULT)
 )
@end{programexample}

The former variant operates only on in-memory frames, while the latter
variant is for virtual frames and will fetch the frame as necessary.
The returned list has been newly created and must be explicitly freed
once no longer required.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{inherit_all_fillers}
@Func{void inherit_all_fillers(const FrFrame *frame)}
@Func{void inherit_all_fillers(const FrFrame *frame, InheritanceType inherit)}
@Func{void inherit_all_fillers(FrSymbol *frame)}
@Func{void inherit_all_fillers(FrSymbol *frame, InheritanceType inherit)}
Find all fillers which the given frame can possibly inherit using specified 
(or the current) inheritance method, and place those fillers directly in the
given frame.  The one-argument variants use the current inheritance
method to retrieve the fillers.

The first two variants operate only on in-memory frames, while the
final two variants are for virtual frames and will fetch the frame if
necessary.
@end{FuncDesc}

@subsection{Object-Oriented Interface}

As has been mentioned previously, FramepaC provides both a procedural
and an object-orient interface; this section describes the object
methods to which most of the preceding procedural calls expand.

Most of the frame-related methods exist in both class @t{FrFrame} and
class @t{FrSymbol}.  The former operate only on in-memory frames, while
the latter can also function with virtual frames, fetching them as
required.  The trade-off is naturally somewhat slower execution due to
the overhead of finding and possibly fetching the frame corresponding
to the symbol and then calling the equivalent method in class
@t{FrFrame}.

@begin{FuncDesc}
@indexctor{FrFrame}
@indexmeth{FrFrame}
@Func{FrFrame::FrFrame(const char *framename)}
@Func{FrFrame::FrFrame(const FrSymbol *framename)}
@indexctor{VFrame}
@indexmeth{VFrame}
@Func{VFrame::VFrame(const char *framename)}
@Func{VFrame::VFrame(const FrSymbol *framename)}
Create a new frame with the given name.  It is an error to create a
frame by the same name as one which
already exists in the current symbol table; therefore, it is usually
preferable to call either @t{create_frame}, @t{create_vframe},
@t{FrSymbol::createFrame}, or @t{FrSymbol::createVFrame}.  These functions
will return an existing frame rather than create a duplicate.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{FrFrame}
@indexdtor{FrFrame}
@Func{FrFrame::~FrFrame}
@indexmeth{VFrame}
@indexdtor{VFrame}
@Func{VFrame::~VFrame}
Destroy the frame and deallocate its storage.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{createFrame}
@indexmeth{createVFrame}
@Func{FrFrame *FrSymbol::createFrame()}
@Func{FrFrame *FrSymbol::createVFrame()}
Create a new frame or virtual frame with the given name if it does not yet
exist, or return a pointer to the existing frame by that name.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{addFiller}
@Func{void FrFrame::addFiller(const FrSymbol *slotname, const FrSymbol *facetname, const FrObject *filler)}
@Func{void FrSymbol::addFiller(const FrSymbol *slotname, const FrSymbol *facetname, const FrObject *filler)}
Append the specified filler to the end of the list of fillers
for the indicated slot and facet if it is not already present.
The caller may discard or otherwise destructively modify the
specified filler once this method call returns, as the value actually
added to the frame is a copy.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{addFillers}
@Func{void FrFrame::addFillers(const FrSymbol *slotname, const FrSymbol *facetname, const FrList *fillers)}
@Func{void FrSymbol::addFillers(const FrSymbol *slotname, const FrSymbol *facetname, const FrList *fillers)}
Append each item in the given list of fillers to the specified slot and
facet.  The caller may discard or otherwise destructively modify the
specified fillers once this method call returns, as the values actually
added to the frame are copies of the supplied values.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{addSem}
@Func{void FrFrame::addSem(const FrSymbol *slotname, const FrObject *filler)}
@Func{void FrSymbol::addSem(const FrSymbol *slotname, const FrObject *filler)}
Append the specified filler to the end of the list of fillers
for the indicated slot's @t{SEM} facet if it is not already present.
The caller may discard or otherwise destructively modify the
specified filler once this function returns, as the value actually
added to the frame is a copy.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{addSems}
@Func{void FrFrame::addSems(const FrSymbol *slotname, const FrList *fillers)}
@Func{void FrSymbol::addSems(const FrSymbol *slotname, const FrList *fillers)}
Append each item in the given list of fillers to the specified slot's
@t{SEM} facet.  The caller may discard or otherwise destructively modify
the specified fillers once this function returns, as the values
actually added to the frame are copies of the supplied values.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{addValue}
@Func{void FrFrame::addValue(const FrSymbol *slotname, const FrObject *filler)}
@Func{void FrSymbol::addValue(const FrSymbol *slotname, const FrObject *filler)}
Append the specified filler to the end of the list of fillers
for the indicated slot's @t{VALUE} facet if it is not already present.
The caller may discard or otherwise destructively modify the
specified filler once this function returns, as the value actually
added to the frame is a copy.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{addValues}
@Func{void FrFrame::addValues(const FrSymbol *slotname, const FrList *fillers)}
@Func{void FrSymbol::addValues(const FrSymbol *slotname, const FrList *fillers)}
Append each item in the given list of fillers to the specified slot's
@t{VALUE} facet.  The caller may discard or otherwise destructively modify
the specified fillers once this function returns, as the values
actually added to the frame are copies of the supplied values.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{collectSlots}
@Func{FrList *FrFrame::collectSlots(InheritanceType inherit, FrList *allslots=0)}
@Func{FrList *FrSymbol::collectSlots(InheritanceType inherit, FrList *allslots=0)}
Determine which slots and facets the given frame may inherit from one or more
of its ancestors using the indicated inheritance method.  The result is
returned as an association list of slots with the facets available for the
slot.  For example,
@begin{programexample}
((IS-A VALUE SEM)
 (SUBCLASSES VALUE SEM)
 (INSTANCE-OF VALUE SEM)
 (INSTANCES VALUE SEM)
 (WEIGHT VALUE SEM M-UNIT DEFAULT)
 )
@end{programexample}
The returned list has been newly created and must be explicitly freed
once no longer required.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{commitFrame}
@Func{int FrFrame::commitFrame()}
@Func{int FrSymbol::commitFrame()}
Store the frame to the backing store if its "dirty" flag indicates that
the backing store needs to be updated.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{copyFrame}
@Func{FrFrame *Frame::copyFrame(FrSymbol *newname)}
@Func{FrFrame *FrSymbol::copyFrame(FrSymbol *newname)}
Make an exact copy of the given old frame into the specified new
frame.  Links will not be updated, as the purpose of this function
is to maintain backup copies of a frame which is changing.  Because the
links are not updated, the copy of the frame should not be modified, as that
will cause internal data to become inconsistent.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{createSlot}
@Func{FrSlot *createSlot(const FrSymbol *slotname)}
Create a slot with the given name if it does not already exist in the
frame.  The return value is used internally and should be ignored--it is
an error to modify the @t{FrSlot} structure.

This function is rarely required, since all functions which add fillers
to the frame will create slots as needed.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{deleteFrame}
@Func{int FrSymbol::deleteFrame()}
Remove the specified frame from main memory and from the backing
store.  The frame will be lost for all time unless the backing store
preserves old versions of frames.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{dirtyFrame}
@Func{FrBool FrFrame::dirtyFrame()}
@Func{FrBool FrSymbol::dirtyFrame()}
Determine whether the frame has been modified since it was created,
loaded, or last committed to the backing store.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{delete_frame}
@Func{int delete_frame(FrSymbol *frame)}
Remove the specified frame from main memory and from the backing
store.  The frame will be lost for all time unless the backing store
preserves old versions of frames.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{discardFrame}
@Func{int FrSymbol::discardFrame()}
@indexfunc{discard_frame}
@Func{int discard_frame(FrSymbol *frame)}
Remove the specified frame from main memory without first updating it
in the backing store.  If the frame is not a virtual frame or the
VFrame system is being operated without backing store, the frame is
lost for all time.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{displayLength}
@index2{p="length",s="of printed output"}
@Func{size_t FrFrame::displayLength()}
Determine how many bytes will be required to store the printed
representation generated by @t{displayValue}.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{displayValue}
@Func{char *FrFrame::displayValue(char *buffer)}
Store a printed representation of the frame in the indicated buffer,
and return a pointer to the byte following the last one stored there.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{doAllFacets}
@Func{FrBool doAllFacets(FrBool (*func)(const FrFrame *frame,
                                    const FrSymbol *slot, const FrSymbol *facet, va_list args),
                       va_list args)}
Call the indicated function once for each nonempty facet in the
given frame. Returns @t{True} if all invocations of the function
returned @t{True}, @t{False} if any of them returned @t{False} (in which case
no further facets are processed.  The final parameter passed to the
indicated function is the same variable argument list passed to
@t{doAllFacets}; the arguments in the list may be accessed with
@t{va_start}, @t{va_arg}, and @t{va_end}. 
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{doFacets}
@Func{FrBool doFacets(const FrSymbol *slotname,
                    FrBool (*func)(const FrFrame *frame, const FrSymbol *slot, const FrSymbol *facet, va_list args),
                    va_list args)}
Call the indicated function once for each nonempty facet in the
given slot. Returns @t{True} if all invocations of the function
returned @t{True}, @t{False} if any of them returned @t{False} (in which case
no further facets are processed).  The final parameter passed to the
indicated function is the same variable argument list passed to
@t{doFacets}; the arguments in the list may be accessed with
@t{va_start}, @t{va_arg}, and @t{va_end}. 
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{doSlots}
@Func{FrBool doSlots(FrBool (*func)(const FrFrame *frame, const FrSymbol *slot, va_list args), va_list args)}
Call the indicated function once for each slot in the given frame.
Returns @t{True} if all invocations of the function returned @t{True},
@t{False} if any of them returned @t{False} (in which case no further
slots are processed).  The final parameter passed to the
indicated function is the same variable argument list passed to
@t{doSlots}; the arguments in the list may be accessed with
@t{va_start}, @t{va_arg}, and @t{va_end}.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{emptyFrame}
@Func{FrBool FrFrame::emptyFrame()}
@Func{FrBool FrSymbol::emptyFrame()}
Determine whether there are any non-empty slots in the frame.  Returns
@t{True} if the frame is completely empty, @t{False} if it contains any
fillers.  This check does not test for inherited fillers.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{eraseFacet}
@Func{void FrFrame::eraseFacet(const FrSymbol *slotname, const FrSymbol *facetname)}
@Func{void FrSymbol::eraseFacet(const FrSymbol *slotname, const FrSymbol *facetname)}
Delete all fillers from the specified facet of the given slot.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{eraseFiller}
@Func{void FrFrame::eraseFiller(const FrSymbol *slotname, const FrSymbol *facetname, const FrObject *filler)}
@Func{void FrFrame::eraseFiller(const FrSymbol *slotname, const FrSymbol *facetname, const FrObject *filler,
                FrCompareFunc cmp)}
@Func{void FrSymbol::eraseFiller(const FrSymbol *slotname, const FrSymbol *facetname, const FrObject *filler)}
@Func{void FrSymbol::eraseFiller(const FrSymbol *slotname, const FrSymbol *facetname, const FrObject *filler,
                FrCompareFunc cmp)}
Delete the specified filler in the indicated slot's facet, using the
given comparison function if supplied.  The two
comparison functions provided standard with FramepaC are @t{eql} and
@t{equal} (see Section @ref{commonproc}); if no comparison function is
specified, a simple pointer equality test (equivalent to Lisp's
@t{eq}) is used. 
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{eraseFrame}
@Func{void FrFrame::eraseFrame()}
@Func{void FrSymbol::eraseFrame()}
Delete all fillers for the entire frame, but do not delete the frame
itself (for that, use either @t{FrSymbol::deleteFrame},
@t{delete_frame(FrSymbol*)}, or @t{delete FrFrame*}).
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{eraseSlot}
@Func{void FrFrame::eraseSlot(const char *slotname)}
@Func{void FrFrame::eraseSlot(const FrSymbol *slotname)}
@Func{void FrSymbol::eraseSlot(const char *slotname)}
@Func{void FrSymbol::eraseSlot(const FrSymbol *slotname)}
Delete all fillers from all facets of the specified slot.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{eraseSem}
@Func{void FrFrame::eraseSem(const FrSymbol *slotname, const FrObject *filler)}
@Func{void FrFrame::eraseSem(const FrSymbol *slotname, const FrObject *filler, FrCompareFunc cmp)}
@Func{void FrSymbol::eraseSem(const FrSymbol *slotname, const FrObject *filler)}
@Func{void FrSymbol::eraseSem(const FrSymbol *slotname, const FrObject *filler, FrCompareFunc cmp)}
Delete the specified filler from the indicated slot's @t{SEM} facet,
using the specified comparison function if supplied.  The two
comparison functions provided standard with FramepaC are @t{eql} and
@t{equal} (see Section @ref{commonproc}); if no comparison function is
specified, a simple pointer equality test (equivalent to Lisp's
@t{eq}) is used. 
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{eraseValue}
@Func{void eraseValue(const FrSymbol *slotname, const FrObject *filler)}
@Func{void eraseValue(const FrSymbol *slotname, const FrObject *filler, FrCompareFunc cmp)}
Delete the specified filler from the indicated slot's @t{VALUE} facet,
using the specified comparison function if supplied.  The two
comparison functions provided standard with FramepaC are @t{eql} and
@t{equal} (see Section @ref{commonproc}); if no comparison function is
specified, a simple pointer equality test (equivalent to Lisp's
@t{eq}) is used. 
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{firstFiller}
@Func{FrObject *FrFrame::firstFiller(const FrSymbol *slotname, const FrSymbol *facet, FrBool inherit = True)}
@Func{FrObject *FrSymbol::firstFiller(const FrSymbol *slotname, const FrSymbol *facet, FrBool inherit = True)}
Retrieve the first filler for the given slot and facet, performing the
current inheritance method if the facet is absent or empty and
@t{inherit} is @t{True}.  DO NOT DESTRUCTIVELY MODIFY THE RETURNED FILLER.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{frameName}
@Func{FrSymbol *FrFrame::frameName()}
Get the frame's name.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{getFillers}
@Func{FrList *FrFrame::getFillers(const FrSymbol *slotname, const FrSymbol *facet, FrBool inherit = True)}
@Func{FrList *FrSymbol::getFillers(const FrSymbol *slotname, const FrSymbol *facet, FrBool inherit = True)}
Retrieve the list of fillers present in the given slot and facet,
performing the current inheritance method if the facet is absent or
empty in the specified frame and @t{inherit} is @t{True}.  @b{DO NOT
MODIFY THE RETURNED LIST}.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{getImmedFillers}
@Func{FrList *FrFrame::getImmedFillers(const FrSymbol *slotname, const FrSymbol *facet)}
@Func{FrList *FrSymbol::getImmedFillers(const FrSymbol *slotname, const FrSymbol *facet)}
Retrieve the list of fillers present in the given slot and facet,
without attempting to inherit the fillers if the facet is absent or
empty.  @b{DO NOT MODIFY THE RETURNED LIST}.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{getSem}
@Func{FrList *FrFrame::getSem(const FrSymbol *slotname,FrBool inherit = True)}
@Func{FrList *FrSymbol::getSem(const FrSymbol *slotname,FrBool inherit = True)}
Retrieve the list of fillers for the given slot's @t{SEM} facet,
performing the current inheritance method if the facet is empty or the
slot does not exist and @t{inherit} is @t{True}.
@b{DO NOT MODIFY THE RETURNED LIST}.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{getValue}
@Func{FrObject *FrFrame::getValue(const FrSymbol *slotname,FrBool inherit = True)}
@Func{FrObject *FrSymbol::getValue(const FrSymbol *slotname,FrBool inherit = True)}
Retrieve the first filler for the given slot's @t{VALUE} facet,
performing the current inheritance method if the facet is empty or the
slot does not exist and @t{inherit} is @t{True}.
@b{DO NOT DESTRUCTIVELY MODIFY THE RETURNED FILLER}.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{getValues}
@Func{FrList *FrFrame::getValues(const FrSymbol *slotname,FrBool inherit = True)}
@Func{FrList *FrSymbol::getValues(const FrSymbol *slotname,FrBool inherit = True)}
Retrieve the list of fillers for the given slot's @t{VALUE} facet,
performing the current inheritance method if the facet is empty or the
slot does not exist and @t{inherit} is @t{True}.
@b{DO NOT MODIFY THE RETURNED LIST}.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{inheritAll}
@Func{void FrFrame::inheritAll()}
@Func{void FrFrame::inheritAll(InheritanceType inherit)}
@Func{void FrSymbol::inheritAll()}
@Func{void FrSymbol::inheritAll(InheritanceType inherit)}
Find all fillers which the given frame can possibly inherit using specified 
(or the current) inheritance method, and place those fillers directly in the
given frame.  The parameterless variants use the current inheritance
method to retrieve the fillers.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{isA_p}
@Func{FrBool FrFrame::isA_p(const FrFrame *possible_parent)}
@Func{FrBool FrSymbol::isA_p(const FrSymbol *possible_parent)}
Determine whether one frame is an ancestor of another by following
the @t{INSTANCE-OF} and @t{IS-A} links.  Returns @t{True} if
@t{possible_parent} can be reached from @t{frame} through some chain of
@t{IS-A} links, possibly with an initial @t{INSTANCE-OF} link.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{isFrame}
@Func{FrBool FrSymbol::isFrame()}
Determine whether the symbol is the name of a frame.  If the frame is
virtual, it is not loaded into memory if it was not already there.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{markDirty}
@Func{void FrFrame::markDirty(FrBool dirty = True)}
Modify the flag indicating whether the frame needs to be written out to
the backing store.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{numberOfSlots}
@Func{int FrFrame::numberOfSlots()}
Determine how many slots are present in the frame.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{objType}
@Func{ObjectType objType()}
Determine whether the frame is a regular or virtual frame.  Returns
@t{OT_Frame} for regular, in-memory, frames and @t{OT_VFrame} for
virtual frames.  Symbols always return @t{OT_Symbol}, regardless of
whether they name a frame.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{objTypeName}
@Func{char *objTypeName()}
Return a string containing the printable name of the type of frame.
Returns "FrFrame" for regular frames and "VFrame" for virtual frames
(and "FrSymbol" for symbols regardless of whether they name a frame).
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{oldFrame}
@Func{FrFrame *FrSymbol::oldFrame(int generation)}
If the symbol is the name of a virtual frame with backing store, this
function reloads the requested prior version of the frame from the backing
store (generation 0 is the most recently saved copy of the frame, generation
1 is the next most-recent copy, generation 2 is the version prior to that,
etc.).  This function has no effect if there is no backing store for the
desired frame.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{partOf_p}
@Func{FrBool FrFrame::partOf_p(const FrFrame *possible_container)}
@Func{FrBool FrSymbol::partOf_p(const FrSymbol *possible_container)}
Determine whether one entity described by a frame is included as a
part of another by following the @t{PART-OF} link.  Returns @t{True}
if @t{possible_container} can be reached from the given frame through
some chain of @t{PART-OF} links. 
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{printValue}
@Func{ostream &FrFrame::printValue(ostream &output)}
Output a printed representation of the frame to the indicated stream.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{replaceFiller}
@Func{void FrFrame::replaceFiller(const FrSymbol *slot, const FrSymbol *facet,
                        const FrObject *old, const FrObject *newfiller)}
@Func{void FrFrame::replaceFiller(const FrSymbol *slot, const FrSymbol *facet,
                        const FrObject *old, const FrObject *newfiller,
                        FrCompareFunc cmp)}
If the specified filler @t{old} exists in the indicated slot's facet, remove
it and add the new filler.  This method has no effect if the old filler is
not present in the indicated location.

The first variation requires that
the supplied old filler be identical to one in the frame (in practice, this
will only be available for symbols and when processing the results of a 
getFillers); the second variation uses the indicated function to determine
whether a filler in the frame is equal to the supplied filler.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{replaceSem}
@Func{void FrFrame::replaceSem(const FrSymbol *slot,
                        const FrObject *old, const FrObject *newfiller)}
@Func{void FrFrame::replaceSem(const FrSymbol *slot,
                        const FrObject *old, const FrObject *newfiller,
                        FrCompareFunc cmp)}
If the specified filler @t{old} exists in the indicated slot's @t{SEM}
facet, remove it and add the new filler.  This method has no effect if the
old filler is not present in the indicated location.

The first variation requires that
the supplied old filler be identical to one in the frame (in practice, this
will only be available for symbols and when processing the results of a 
getFillers); the second variation uses the indicated function to determine
whether a filler in the frame is equal to the supplied filler.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{replaceValue}
@Func{void FrFrame::replaceValue(const FrSymbol *slot,
                        const FrObject *old, const FrObject *newfiller)}
@Func{void FrFrame::replaceValue(const FrSymbol *slot,
                        const FrObject *old, const FrObject *newfiller,
                        FrCompareFunc cmp)}
If the specified filler @t{old} exists in the indicated slot's @t{VALUE}
facet, remove it and add the new filler.  This method has no effect if the
old filler is not present in the indicated location.

The first variation requires that
the supplied old filler be identical to one in the frame (in practice, this
will only be available for symbols and when processing the results of a 
getFillers); the second variation uses the indicated function to determine
whether a filler in the frame is equal to the supplied filler.
@end{FuncDesc}

@subsection{Advanced Functions}

@begin{FuncDesc}
@indexfunc{collect_prefix_matching_frames}
@index{name completion}
@Func{FrList *collect_prefix_matching_frames(char *prefix, char *longest, int buflen)}
Return a list of the names of all frames in the current symbol table
whose names start with the given prefix.  If @t{longest} is nonzero,
also return the longest prefix all of the returned frames have in
common, up to a maximum of @t{buflen}-1 characters (the final character
is reserved for the terminating NUL).  The returned list has been newly
created and must be explicitly freed once no longer required.

For example, if the current symbol table contains frames named ABACUS,
COMPUTING, COMPUTER, and COMPUTER-REPAIR, then a search for the prefix
"COM" would yield (COMPUTING COMPUTER COMPUTER-REPAIR) and the longest
prefix would be "COMPUT".
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{complete_frame_name}
@index{name completion}
@Func{char *complete_frame_name(const char *prefix)}
Return a string containing the longest prefix in common among all frames
for the current symbol table having the indicated string as the beginning
of their names, or 0 if there are no matching frames.  The returned string
is allocated from the heap, and must be released with @t{FrFree} (see
Section @ref{memalloc}).
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{copy_frame}
@Func{void copy_frame(const FrSymbol *oldframe, const FrSymbol *newframe)}
Make an exact copy of the given old frame into the specified new
frame.  Links will not be updated, as the purpose of this function
is to maintain backup copies of a frame which is changing.  Because the
links are not updated, the copy of the frame should not be modified, as that
will cause internal data to become inconsistent.  This function does nothing
if @t{oldframe} is not present in memory already; use @t{copy_vframe} instead
if the frame must be retrieved from backing store.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{copy_vframe}
@Func{void copy_vframe(const FrSymbol *oldframe, const FrSymbol *newframe)}
Make an exact duplicate (except for the frame name) of the
given old frame into the specified new frame.  Links are not
updated, as the purpose of this function is to make backup
copies of a frame which is changing.  Because the links are not updated,
the copy of the frame should not be modified, as that
will cause internal data to become inconsistent.  Unlike @t{copy_frame},
this frame will fetch the frame from backing store if it is not currently
in memory.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{define_relation}
@indexmeth{defineRelation}
@Func{void define_relation(char *relation, char *inverse)}
@Func{void FrSymbol::defineRelation(const FrSymbol *inverse)}
Define a link and its inverse which FramepaC is to maintain
automatically.  Whenever a filler which is the name of a frame is
added to or deleted from the VALUE facet of a relation slot,
FramepaC automatically adds/deletes the current frame to the VALUE
facet of the inverse slot in that other frame.  Links are symmetrical,
so operating on the inverse slot will affect the relation slot in
the other frame.

By default, the relations IS-A/SUBCLASSES, INSTANCE-OF/INSTANCES,
and PART-OF/HAS-PARTS are defined.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{undefine_relation}
@Func{void undefine_relation(char *relation, char *inverse)}
@indexmeth{undefineRelation}
@Func{void FrSymbol::undefineRelation(const FrSymbol *inverse)}
Specify that FramepaC should no longer maintain the specified link
automatically.  DO NOT UNDEFINE THE STANDARD RELATIONS
(IS-A/SUBCLASSES, INSTANCE-OF/INSTANCES, and PART-OF/HAS-PARTS) or
various functions will no longer work correctly because the data
consistency necessary to their operation will not be maintained.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{inverseRelation}
@Func{FrSymbol *FrSymbol::inverseRelation()}
Determine the name of the frame link which is the inverse of the one
with the indicated name, or 0 if the given name is not that of a
relation with an inverse (either predefined or specified with the
@t{define_relation} function).  For example,
@begin{programexample}
makeSymbol("IS-A")->inverseRelation() == makeSymbol("SUBCLASSES")
@end{programexample}
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{FramepaC_to_FrameKit}
@Func{FrList *FramepaC_to_FrameKit(FrFrame *frame)}
Return a MAKE-FRAME list corresponding to the given frame.
When printed to a file, the returned list can be read in by
FrameKit.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{FrameKit_to_FramepaC}
@Func{FrFrame *FrameKit_to_FramepaC(FrList *framespec)}
Given a list containing a FrameKit-style MAKE-FRAME or
MAKE-FRAME-OLD frame definition, return the FramepaC frame with
the equivalent contents.
@end{FuncDesc}

@subsection{Virtual Frame Control Functions}

@begin{FuncDesc}
@indexfunc{abort_transaction}
@Func{int abort_transaction(int transaction_handle)}
Indicate that some update of the backing store for the currently active
symbol table since the indicated transaction was begun failed, and that
all updates in the transaction must be undone.  Any subtransactions within
the indicated transaction which have not yet been terminated are closed
and undone as well.  Virtual frames currently in memory will also revert
to their state at the beginning of the failed transaction (they may either
be restored directly or discarded from memory and reloaded on the next
access).

This function has no effect if the current symbol table is not using backing
store, and is thus operating only on in-memory frames.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{commit_all_frames}
@Func{int commit_all_frames()}
Force all modified frames to be written out to the backing
store.  Returns 0 if successful or -1 if any frames could not
be successfully committed.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{commit_frame}
@Func{int commit_frame(FrSymbol *frame)}
Force the indicated frame to be written out to the backing
store if it is dirty.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{discard_frame}
@Func{int discard_frame(FrSymbol *frame)}
Throw out the named frame, forcing it to be re-read from
backing store on the next access.  This function does nothing
when operating without backing store.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{discard_LRU_frames}
@Func{void discard_LRU_frames()}
Throw out the least-recently used virtual frames in every symbol table,
preferentially discarding unmodified frames and writing out any dirty
frames it is forced to discard.  This function has no effect unless the
preprocessor symbol @t{FrLRU_DISCARD} is defined.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{end_transaction}
@Func{int end_transaction(int transaction_handle)}
Indicate that all updates of the backing store for the currently active
symbol table since the indicated transaction was begun with
@t{start_transaction} have been successful.  Any subtransactions within
the indicated transactions which have not yet been ended are also terminated
successfully.

This function has no effect if the current symbol table is not using backing
store, and is thus operating only on in-memory frames.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{frame_is_dirty}
@Func{FrBool frame_is_dirty(FrSymbol *frame)}
@indexmeth{dirtyFrame}
@Func{FrBool FrSymbol::dirtyFrame()}
Determine whether the frame has been changed since it was last
read into memory from the backing store.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{frame_locked}
@Func{FrBool frame_locked(const FrSymbol *frame)}
@indexmeth{isLocked}
@Func{FrBool FrSymbol::isLocked()}
Determine whether the indicated frame is currently locked.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{lock_frame}
@Func{FrFrame *lock_frame(FrSymbol *frname)}
@indexmeth{lockFrame}
@Func{FrFrame *FrSymbol::lockFrame()}
Ensure that no one else modifies the indicated frame until it is unlocked.
This function also forces the frame to be loaded into memory if it was not
already there, and prevents it from being swapped out of memory until it is
unlocked.  The frame should be unlocked as soon as practical, and the number
of simultaneously locked frames should be kept to a minimum to avoid
interfering with other users.  Locking a frame can also be useful in
time-critical code, since it allows the use of a pointer to the frame
instead of the frame's name, eliminating one level of indirection from
each frame access.  For example:
@begin{programexample}
FrFrame *fr = lock_frame(name) ;
...
add_value(fr,symbolXYZ,filler1) ;
add_value(fr,symbolYZW,filler2) ;
...
unlock_frame(name) ;
@end{programexample}
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{start_transaction}
@Func{int start_transaction(int transaction handle)}
Indicate that any updates of the backing store for the currently active
symbol table from this point forward should be treated as a single atomic
operation.  That is, when the transaction ends, either all updates will
have been successful, or all updates will have been undone (so effectively,
they never happened).  The transaction continues until either
@t{end_transaction} (successful) or @t{abort_transaction} (failure, undo
changes) is called with the handle returned by this function.

This function has no effect if the current symbol table is not using backing
store, and is thus operating only on in-memory frames.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{synchronize_VFrames}
@Func{int synchronize_VFrames(frame_update_hookfunc *update_hook = 0)}
Ensure that the backing store for the current symbol table has an
up-to-date copy of all frames in the symbol table.  If @t{update_hook} is
nonzero, the function will be called for each frame processed.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{unlock_frame}
@Func{FrFrame *unlock_frame(FrSymbol *frname)}
@indexmeth{unlockFrame}
@Func{FrFrame *FrSymbol::unlockFrame()}
If the named frame is currently locked, unlock it and allow other users access
to the frame.  This function also allows the frame to be swapped out of memory
if it is a virtual frame and some memory must be freed to satisfy another
memory allocation request.
@end{FuncDesc}

@comment{-------------------------------}
@section{Hash Table Functions}
@label{hashtable}
@indextype{FrHashTable}
@index{hash tables}

Another data type which is useful on many occasions is the hash table.
FramepaC provides the @t{FrHashTable} class for performing hash table
operations with @t{FrObject}s.

@begin{FuncDesc}
@indexctor{FrHashTable}
@Func{FrHashTable::FrHashTable()}
Create a new hash table with the default fill factor (see below).
@end{FuncDesc}

@begin{FuncDesc}
@indexctor{FrHashTable}
@Func{FrHashTable::FrHashTable(int fill_factor)}
Create a new hash table, initially with a maximum size of zero
elements.  The @t{fill_factor} specifies the average number of elements
per hash bin before the table is automatically expanded.  Before any
elements are added, you must call either @t{expand} or @t{expandTo}.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{entryType}
@Func{FrHashEntryType FrHashTable::entryType() const}
Determine the type of hash table entry this instance of a hash table
expects to store.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{expand}
@Func{void FrHashTable::expand(int increment)}
Expand the hash table so that it is able to store an additional
@t{increment} items.  All items presently in the hash table are re-hashed.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{expandTo}
@Func{void FrHashTable::expandTo(int newsize)}
Expand the hash table until it is able to store @t{newsize} items.  If
the new size is less than or equal to the present size, the hash table
will not be altered.  If the hash table was expanded, all items
presently in the table are re-hashed.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{add}
@Func{FrHashEntry *FrHashTable::add(const FrHashEntry *entry)}
Insert a @i{copy} of the given hash table entry into the hash table.
The hash table can hold objects of any subclass of @t{FrHashEntry}, so
a @t{FrHashTable} can be used to hash any data type.  Since a copy is
inserted, the provided @t{entry} may be destroyed or modified after it
is added to the hash table.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{remove}
@Func{int FrHashTable::remove(const FrHashEntry *entry)}
Remove the entry currently in the hash table with the same key as the
given entry.  Returns 0 if a matching item was removed, -1 if there was
no matching item.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{lookup}
@Func{FrHashEntry *FrHashTable::lookup(const FrHashEntry *entry)}
Find the item in the hash table with the same key as the given hash
entry.  Returns a pointer to the item or 0 if no matching item is
present in the hash table.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{prefixMatches}
@Func{FrList *FrHashTable::prefixMatches(const char *prefix) const}
Return a list of the hash keys for the items in the hash table which in
some sense match the specified prefix.  This function is used in frame
name completion, for instance, by returning a list of symbols stored in
the table whose names begin with the indicated string.  The returned
list has been newly created and must be explicitly freed once no longer
required.

This function will only return the hash keys for those hash table
entries for which the entry's @t{prefixMatch} method returns @t{True},
unless the prefix is the empty string, in which case @b{all} hash keys
are returned.  The default @t{FrHashEntryObject} entry type returns
@t{True} if the key is an @t{FrSymbol} or @t{FrFrame} and the prefix
matches the beginning of the object's printing name, @t{False} in all
other cases.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{completionFor}
@Func{char *FrHashTable::completionFor(const char *prefix) const}
Retrieve the longest string starting with @t{prefix} which all entries
in the hash table have in common, or 0 if there are no entries matching
@t{prefix}.  As a special case, if @t{prefix} is 0 (rather than an
empty string), the longest common prefix is returned even if the entry
would otherwise have indicated that the prefix does not match.

This function will only return those hash table entries for which the
entry's @t{prefixMatch} method returns @t{True} and @t{entryName} is
not 0.  The default @t{FrHashEntryObject} entry type always returns
@t{False} for @t{prefixMatch}, and only returns an entry name if the
entry's key is a @t{FrSymbol} or @t{FrFrame}.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{doHashEntries}
@Func{FrBool FrHashTable::doHashEntries(DoHashEntriesFunc *func, ...)}
Call the indicated function @t{func} once for each entry in the hash
table.  The function is called with the hash table entry as its first
argument and a variable-argument list of any additional arguments given
to @t{doHashEntries} as its second argument.
@end{FuncDesc}

@indextype{FrHashEntry}
@indextype{FrHashEntryObject}
As indicated in the descriptions of various @t{FrHashTable} methods,
the elements stored in the hash table are instances of some subclass of
@t{FrHashEntry} (which itself is intended as an abstract base class,
since it contains no fields for storing data).  The predefined subclass
@t{FrHashEntryObject} will suffices for most uses, since it associates
an @t{FrObject} key with an arbitrary pointer, which may be used to
specify another @t{FrObject} or some non-FramepaC data.

@begin{FuncDesc}
@indexmeth{hashp}
@Func{virtual FrBool FrHashEntry::hashp() const}
Returns @t{True} to indicate that this object is associated with a hash
table.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{entryType}
@Func{virtual FrHashEntryType FrHashEntry::entryType() const}
Overridden by subclasses to indicate what type of hash table entry the
object represents.  This method returns @t{HE_base} for @t{FrHashEntry}
and @t{HE_FrObject} for @t{FrHashEntryObject}.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{entryName}
@Func{virtual FrSymbol *FrHashEntry::entryName() const}
Returns the @t{FrSymbol} corresponding to the hash key for the hash
table entry.  @t{FrHashEntry} always returns @t{0};
@t{FrHashEntryObject} returns the key if it is a symbol, the frame name
if the key is a frame, or @t{0} for other object types used as a key.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{sizeOf}
@Func{virtual int FrHashEntry::sizeOf() const}
Return the size of the object in bytes.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{hashValue}
@Func{virtual unsigned int FrHashEntry::hashValue(int size) const}
Compute the hash table index for the entry.  (used internally)
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{keycmp}
@Func{virtual int FrHashEntry::keycmp(const FrHashEntry *entry) const}
Determine the ordering relation between the hash keys of the given hash
table entry and @t{entry}.  Returns -1 if the given entry orders before
@t{entry}, +1 if after, or 0 if the two entries have equal keys.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{prefixMatch}
@Func{virtual FrBool FrHashEntry::prefixMatch(const char *prefix,int length) const}
Determine whether the hash key or entry name begins with the specified
@t{length}-byte character sequence.  Always returns @t{False} for
@t{FrHashEntry} and @t{FrHashEntryObject}.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{copy}
@Func{virtual FrObject *FrHashEntry::copy() const}
Make a duplicate of the specified hash table entry.
@end{FuncDesc}

@indextype{FrHashEntryObject}
@begin{FuncDesc}
@indexctor{FrHashEntryObject}
@Func{FrHashEntryObject()}
Construct a new hash table entry whose key and associated data are both
@t{0}.
@end{FuncDesc}

@begin{FuncDesc}
@indexctor{FrHashEntryObject}
@Func{FrHashEntryObject(const FrObject *key)}
Construct a new hash table entry with the specified key and a NULL
associated data pointer.  The key is copied as needed whenever the
entry is copied (such as while placing the entry into the hash table).
@end{FuncDesc}

@begin{FuncDesc}
@indexctor{FrHashEntryObject}
@Func{FrHashEntryObject(const FrObject *key, void *data)}
Construct a new hash table entry with the specified key and associated
data.  The key is copied as needed, but the associated data is not
(since it could be anything, including non-FramepaC types).
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{getObject}
@Func{const FrObject *FrHashEntryObject::getObject()}
Retrieve the key value for the hash table entry.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{getUserData}
@Func{void *FrHashEntryObject::getUserData() const}
Retrieve the associated data for the hash table entry.  Since the
return value is a @t{void*}, it must be cast appropriately; i.e., if
the hash table is constructed using @t{FrNumber}s as the associated
data, one would use
@begin{programexample}
FrNumber *num = (FrNumber*)entry->getUserData() ;
@end{programexample}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{setUserData}
@Func{void FrHashEntryObject::setUserData(void *data)}
Modify the associated data for the hash table entry.  This function
merely replaces the pointer stored in the entry; if the previous
associated data must be deallocated (to avoid memory leaks), the
calling code must do so itself, by first retrieving the previous value
with @t{getUserData()}.
@end{FuncDesc}

@comment{-------------------------------}
@section{FrSockStream Functions}

FramepaC provides three @t{iostream} derivative classes which operate over
Unix-style network sockets rather than connecting to a file or character
device: @t{FrSockStream}, @t{FrISockStream}, and @t{FrOSockStream}.

@begin{FuncDesc}
@indexctor{FrISockStream}
@Func{FrISockStream::FrISockStream(FrSocket s)}
Open an input stream which will use the socket @t{s} as the source from
which data is read.  For portability, @t{s} should not be a file descriptor,
since only Unix treats file descriptors and socket descriptors
interchangeably.

If @t{s} is @t{INVALID_SOCKET}, then the constructed stream is in a closed
state; you must use @t{FrISockStream::setSocket} before attempting input
from the stream.
@end{FuncDesc}

@begin{FuncDesc}
@indexctor{FrISockStream}
@Func{FrISockStream::FrISockStream(FrSocket s, int port)}
Open a socket listening on @t{port} for connections from a remote
process.  If @t{s} is @t{INVALID_SOCKET}, then a new socket is created
and prepared to listen on the specified port; otherwise, the given
socket (assumed to be unbound) is used.  To check for connection
requests -- and actually start listening for the requests -- use
@t{FrISockStream::awaitConnection}.  Prior to the first call to
@t{awaitConnection}, remote processes will receive a "Connection
Refused" error because nobody is listening to the port.
@end{FuncDesc}

@begin{FuncDesc}
@indexctor{FrISockStream}
@Func{FrISockStream::FrISockStream(const char *hostname, int port)}
Open a connection to the specified port on @t{hostname}, and set it to
read-only mode.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{socketNumber}
@Func{FrSocket FrISockStream::socketNumber() const}
Determine the socket descriptor being used by the stream.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{connectionDied}
@Func{FrSocket FrISockStream::connectionDied() const}
Determine whether the socket connection is still alive.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{inputAvailable}
@Func{FrSocket FrISockStream::inputAvailable() const}
Determine whether it is possible to read at least one byte from the stream
without blocking on the socket.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{awaitConnection}
@Func{FrSocket FrISockStream::awaitConnection(int timeout, ostream &err, ostream *out = 0)}
Start a stream created with @t{FrISockStream(socket,port)} listening on
its port (if it is not already listening), and await a connection
request from a remote process for up to @t{timeout} seconds.  Returns
the socket which was created to accept the request, or
@t{INVALID_SOCKET} if no request was pending or arrived during the
timeout period.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{reset}
@Func{FrSocket FrISockStream::reset()}
@end{FuncDesc}

@begin{FuncDesc}
@indexctor{FrOSockStream}
@Func{FrOSockStream::FrOSockStream(FrSocket s)}
@end{FuncDesc}

@begin{FuncDesc}
@indexctor{FrOSockStream}
@Func{FrOSockStream::FrOSockStream(const char *hostname, int port)}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{socketNumber}
@Func{FrSocket FrOSockStream::socketNumber() const}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{connectionDied}
@Func{FrSocket FrOSockStream::connectionDied() const}
@end{FuncDesc}

@begin{FuncDesc}
@indexctor{FrSockStream}
@Func{FrSockStream::FrSockStream(FrSocket s)}
@end{FuncDesc}

@begin{FuncDesc}
@indexctor{FrSockStream}
@Func{FrSockStream::FrSockStream(const char *hostname, int port)}
Open a read/write stream to the indicated port on host @t{hostname}.
!!!
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{socketNumber}
@Func{FrSocket FrSockStream::socketNumber() const}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{connectionDied}
@Func{FrSocket FrSockStream::connectionDied() const}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{inputAvailable}
@Func{FrSocket FrSockStream::inputAvailable() const}
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{await_socket_connection}
@Func{void await_socket_conection(int port, istream *&in, ostream *&out, ostream *&err)}
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{await_socket_connection}
@Func{FrSocket await_socket_conection(int port, int timeout, ostream &err)}
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{listening_socket}
@Func{FrSocket listening_socket()}
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{close_sockets}
@Func{void close_sockets(istream *&in, ostream *&out, ostream *&err)}
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{connect_to_port}
@Func{FrSocket connect_to_port(const char *hostname, int port_number)}
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{disconnect_port}
@Func{int disconnect_port(FrSocket s)}
!!!
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{input_available}
@Func{int input_available(FrSocket s)}
Determine whether any input is available for reading from the socket.
If this function returns @t{True}, at least one character may be read
from the socket without blocking.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{await_activity}
@Func{int await_activity(FrSocket *fds, int numfds, int timeout)}
Pause until any of the @t{numfds} sockets in the array @t{fds} have
input available or a pending exception condition, or @t{timeout}
seconds have elapsed.  Returns @t{True} if any of the sockets are
active, @t{False} if the timeout elapsed.  If @t{timeout} is 0, the
function returns immediately; if @t{timeout} is negative, it is forced
to "forever" (eight hours).
@end{FuncDesc}


@comment{-------------------------------}
@section{FrReadTable Functions}
@indextype{FrReadTable}

Like Lisp, FramepaC uses a read-table which defines the actions to take
when specific characters are encountered in the input.  One default
read table is always defined, and is used by @t{read_FrObject} and
@t{string_to_FrObject}; additional tables may be created as desired.

@begin{FuncDesc}
@indexctor{FrReadTable}
@Func{FrReadTable::FrReadTable()}
Create a new read table.  The read table is initialized to a minimal
set of functions for both strings and streams: semicolons are treated
as comment introducers, valid characters for symbols cause a symbol to
be read, and all other characters are treated as whitespace.
@end{FuncDesc}

@begin{FuncDesc}
@indexdtor{FrReadTable}
@Func{FrReadTable::~FrReadTable()}
@end{FuncDesc}

@begin{FuncDesc}
@indextype{FrReadStringFunc}
@Func{typedef FrObject *FrReadStringFunc(char *&input, const char *digits)}
This is the type of a function which will be called by the FramepaC
reader when reading from a string.  The pointer @t{input} may be
advanced to consume any additional input required to read the object;
it initially points at the first character of the object, or the first
character following a '@t{#}' which specifies the Lisp macro dispatch
function desired.  The second argument, @t{digits}, is 0 unless the
function was called for a Lisp macro dispatch, in which case it points
at a NUL-terminated string containing the digits of any infix numeric
argument following the '@t{#}' of the macro dispatch.
@end{FuncDesc}

@begin{FuncDesc}
@indextype{FrReadStreamFunc}
@Func{typedef FrObject *FrReadStreamFunc(istream &input, const char *digits)}
This is the type of a function which will be called by the FramepaC
reader when reading from a stream.  The stream @t{input} may be read
as needed to get any additional input required to read the object;
it is initially located at the first character of the object, or the first
character following a '@t{#}' which specifies the Lisp macro dispatch
function desired.  The second argument, @t{digits}, is 0 unless the
function was called for a Lisp macro dispatch, in which case it points
at a NUL-terminated string containing the digits of any infix numeric
argument following the '@t{#}' of the macro dispatch.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{setReader}
@Func{void FrReadTable::setReader(char c, FrReadStringFunc *)}
@Func{void FrReadTable::setReader(char c, FrReadStreamFunc *)}
@Func{void FrReadTable::setReader(char c, FrReadStringFunc *, FrReadStreamFunc)}
Set the function to be called when the character @t{c} is encountered
while reading an object from a string, a stream, or either
(respectively).  The application should normally set both types of read
function to perform equivalent actions to avoid inconsistencies between
reads from a string and reads from a stream.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{getStringReader}
@Func{FrReadStringFunc *FrReadTable::getStringReader(char c)}
Determine the function which will be called when character @t{c} is
encountered while reading from a string.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{getStreamReader}
@Func{FrReadStreamFunc *FrReadTable::getStreamReader(char c)}
Determine the function which will be called when character @t{c} is
encountered while reading from a stream.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{readString}
@Func{FrObject *FrReadTable::readString(char *&input)}
Read the given string and convert the first object found.  This
method returns the symbol @t{*EOF*} if the end of the string is
encountered before any objects, and advances the pointer @t{input} to
point at the string's terminating NUL or to the character immediately
following the last one forming a part of representation of the returned
object.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{readStream}
@Func{FrObject *FrReadTable::readStream(istream &input)}
Read the given stream and convert the first object found.  This method
returns the symbol @t{*EOF*} if an end-of-file condition is encountered
before any objects, and advances the location of @t{input} to the
character immediately following the last one forming a part of
representation of the returned object.
@end{FuncDesc}


@comment{-------------------------------}
@section{FrReader Functions}
@indextype{FrReader}

Global instances of the @t{FrReader} class are used to extend the
default read table at compile time.  The constructor for @t{FrReader}
updates the default read table as specified by its arguments when the
program's global initialization is performed, before @t{main} is executed.

An advantage of this type of read-table setup is that only those
classes which are actually used by the program need to be linked in,
which considerably reduces code bloat.

@begin{FuncDesc}
@indexctor{FrReader}
@Func{FrReader::FrReader(FrReadStringFunc *stringfunc, FrReadStreamFunc *streamfunc,
        int leadin_char, char *leadin_string)}
Add the two read functions specified to the default FramepaC read table
as follows:
@begin{itemize}
If @t{leadin_char} is @t{FrREADER_LEADIN_LISPFORM}, this instance
defines a Lisp macro-dispatcher string @t{leadin_string}, which (unlike
Lisp) may consist of multiple characters (unless it begins with 'R' or
'X').  Thus, @t{FrReader(f1,f2,FrREADER_LEADIN_LISPFORM,"Example")}
defines the functions to be called when @t{#Example} is encountered.
@t{leadin_string} is not case-sensitive.

If @t{leadin_char} is @t{FrREADER_LEADIN_CHARSET}, this instance
defines the read functions for each and every character contained in
@t{leadin_string}.   Thus, @t{FrReader(f1,f2,FrREADER_LEADIN_CHARSET,"ABCde")}
defines the functions to be called when 'A', 'B', 'C', 'd', or 'e' are
encountered (in this case, @t{leadin_string} is case-sensitive).

If @t{leadin_char} is any other character, @t{leadin_string} is ignored
and this instance defines the read functions for the specified character.
@end{itemize}
@end{FuncDesc}

@begin{FuncDesc}
@indexdtor{FrReader}
@Func{FrReader::~FrReader()}
Destroy the instance of @t{FrReader}.  Destroying a @t{FrReader} does
@b{not} restore the entries in the read table which it overrode when it
was constructed, but will remove a Lisp macro-dispatcher definition if
originally constructed with @t{FrREADER_LEADIN_LISPFORM}.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{lispFormName}
@Func{const char *FrReader::lispFormName() const}
Retrieve the Lisp macro-dispatcher lead-in string defined if this
instance was constructed with @t{FrREADER_LEADIN_LISPFORM}, 0 otherwise.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{readString}
@Func{FrObject *FrReader::readString(char *&input, const char *digits)}
Read an object from the given input string if this instance was
constructed with @t{FrREADER_LEADIN_LISPFORM}; returns 0 otherwise.
This method is called internally by the FramepaC reader's Lisp
macro-dispatcher function.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{readString}
@Func{FrObject *FrReader::readStream(istream &input, const char *digits)}
Read an object from the given input stream if this instance was
constructed with @t{FrREADER_LEADIN_LISPFORM}; returns 0 otherwise.
This method is called internally by the FramepaC reader's Lisp
macro-dispatcher function.
@end{FuncDesc}


@comment{-------------------------------}
@section{Demon Functions}

It is possible to have an arbitrary function invoked automatically
whenever a certain action is performed.  Once installed, such a demon
function will be invoked again and again until explicitly removed,
allowing it to monitor (and even affect) operations on frames.

@indexpp{FrDEMONS}
The functions described in this section are optional, and will only be
available if the preprocessor macro @t{FrDEMONS} is defined.
Your installation may have disabled demons because of the additional
overhead required to support demons even when none are active.

Five types of action may be monitored through the use of demons.
These actions are creating a slot or facet, adding a filler, removing
a filler, retrieving a value from a frame, and a missing filler
triggering inheritance.

Having a demon for missing fillers can be especially useful, because the
demon can add values to the frame before inheritance is attempted.  If the
demon adds fillers to the facet for which it was triggered, FramepaC will
use those fillers instead of performing inheritance.

!!!

@begin{FuncDesc}
@indexfunc{add_demon}
@Func{FrBool add_demon(FrSymbol *slotname, DemonType type, DemonFunc *func, va_list args = 0)}
Insert a new demon of the specified type to be called whenever the
indicated action is performed on a slot of the given name.  For
example,
@begin{programexample}
add_demon(makeSymbol("TEST"),DT_IfAdded,added_func,0) ;
@end{programexample}
would call @t{added_func} any time a filler is added to any slot named
@t{TEST}.  Note that the installed demon function only affects frames created
while the current symbol table is active; thus, each symbol table has
an independent set of demons.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{remove_demon}
@Func{FrBool remove_demon(FrSymbol *slotname, DemonType type, DemonFunc *func)}
Remove the specified demon from the list of demons to be invoked on
the indicated action on any slot with the given name.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{addDemon}
@Func{FrBool FrSymbol::addDemon(DemonType type, DemonFunc *func, va_list args = 0)}
Insert a new demon of the specified type to be called whenever the
indicated action is performed on a slot of the given name.  Note that
the installed demon function only affects frames created while the current
symbol table is active; thus, each symbol table has an independent set
of demons.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{removeDemon}
@Func{FrBool FrSymbol::removeDemon(DemonType type, DemonFunc *func)}
Remove the specified demon from the list of demons to be invoked on
the indicated action on any slot with the given name.
@end{FuncDesc}

@comment{-------------------------------}
@section{Configuration-File Functions}
@index{configuration files}

FramepaC provides the necessary mechanisms to read and interpret
configuration files which affect the operation of the program.  The
basic method for implementing a custom configuration file is to derive
a subclass of @t{FrConfiguration} with one or more
@t{FrConfigurationTable}s defining the actual keywords and data types
present in the configuration file.

@subsection{FrConfiguration}
@indextype{FrConfiguration}

The @t{FrConfiguration} class is an abstract type from which you can
easily derive classes to parse configuration files containing default
settings for your program.

@indexmeth{init}
@indexmeth{resetState}
In order to create a tailored configuration-file parser, you must
derive a subclass from @t{FrConfiguration} which overrides at least the
methods @t{init} and @t{resetState}.  The former method clears all data
members added by the derived class to their default values, and the
latter indicates which @t{FrConfigurationTable} to use when parsing
begins (the configuration table itself can cause a switch to another
table, creating a state machine).  The derived class may optionally
override @t{maxline} and @t{dump}, and may add additional parsing
functions.


@begin{FuncDesc}
@indexctor{FrConfiguration}
@Func{FrConfiguration::FrConfiguration()}
Derived classes will normally not require an explicit constructor
unless you wish to e.g. combine the construction and @t{load}
operations in one.
@end{FuncDesc}

@begin{FuncDesc}
@indexdtor{FrConfiguration}
@Func{FrConfiguration::~FrConfiguration()}
If a derived class contains any @t{char*} or @t{FrList*} members, it
will require a destructor which frees those members to avoid a memory leak.
@end{FuncDesc}

@begin{FuncDesc}
@indexvirt{init}
@Func{virtual void FrConfiguration::init()}
This method must be overridden by any derived classes, and must reset
all added data members to their default values (e.g. setting all
@t{char*} and @t{FrList*} to 0).
@end{FuncDesc}

@begin{FuncDesc}
@indexvirt{resetState}
@Func{virtual void FrConfiguration::resetState()}
This method must be overridden by any derived classes, and must set the
@t{curr_state} pointer to the @t{FrConfigurationTable} on which the
configuration-file parsing is to begin.  The configuration table may
indicate other configuration tables to which the parsing should switch
on encountering particular keywords, allowing you to create a state
machine whose initial state is the one indicated by @t{resetState}.
@end{FuncDesc}

@begin{FuncDesc}
@indexvirt{maxline}
@Func{virtual size_t FrConfiguration::maxline() const}
By default, @t{FrConfiguration} allows individual lines to be up to
2048 characters in length, but permits logical lines to continue across
several lines in the file.  By overriding this method, a derived class
may change the maximum length of an individual line (though logical
lines remain unlimited due to continuations).
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{load}
@Func{FrBool load(const char *filename, FrBool reset = True)}
If the indicated file exists and is readable, it is opened and parsed,
with the results of the parsing placed into the object's data members.
This method returns @t{True} if the file was successfully read,
@t{False} otherwise.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{load}
@Func{FrBool load(istream &instream, FrBool reset = True)}
The indicated stream is parsed, with the results of the parsing placed
into the object's data members.  This method returns @t{True} if the
file was successfully read, @t{False} otherwise.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{good}
@Func{FrBool good() const}
Determine whether the contents of the object are valid, i.e. has a
configuration file been parsed into its data members?
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{lineNumber}
@Func{int lineNumber() const}
Return the current line number within the configuration file.  This
method is intended for use in an overloaded @t{warn} method to report
where in the file an error occurred.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{dumpFlags}
@Func{ostream &dumpFlags(long flags, FrCommandBit *bits, ostream &output) const}
Convert the specified @t{flags} (as created by the @t{bitflags} method)
back into a list of the flag names written to the specified stream.
@end{FuncDesc}

@begin{FuncDesc}
@indexvirt{dump}
@Func{virtual ostream &dump(ostream &out) const}
Print the parsed contents of the configuration file to the specified
stream.   This method must be overloaded by subclasses of
@t{FrConfiguration} if you wish to have a dump of the configuration for
debugging purposes.
@end{FuncDesc}


The following functions may be referenced from the
@t{FrConfigurationTable}(s) for a derived class:

@begin{FuncDesc}
@indexmeth{integer}
@Func{FrBool integer(const char *line, void *location, void *extra)}
The data value for a keyword is expected to be an integer, which will
be converted into a @t{long int} stored at the address pointed at by
@t{location}. 
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{real}
@Func{FrBool real(const char *line, void *location, void *extra)}
The data value for the associated keyword is expected to be a
floating-point number, which will be converted into a @t{double} stored
at the address pointed at by @t{location}. 
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{basedir}
@Func{FrBool basedir(const char *line, void *location, void *extra)}
The data value for the associated keyword is expected to be a quoted
string, which will be converted into a @t{char*} and stored internally,
as well asoptionally stored at @t{location} (which, unlike for any
other conversion function described here, may be @t{0}).  The base
directory specified by the keyword's data value is used internally to
expand the "+" convention for the @t{filename} conversion function.

If @t{location} is non-NULL, the string pointed at by the @t{char*}
must be freed with @t{FrFree}, usually in the configuration object's
destructor.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{filename}
@Func{FrBool filename(const char *line, void *location, void *extra)}
The data value for the associated keyword is expected to be a quoted
string, which will be converted into a @t{char*} stored at @t{location}.
Should the string begin with a plus sign ("+"), the plus sign is
replaced with the entire contents of the base directory string
specified with a keyword using the @b{basedir} conversion function.
Since the configuration file is only read once, a single keyword with
the @t{basedir} conversion may be specified multiple times, changing
the value of the "+" expansion each time.

The string pointed at by the @t{char*} must be freed with @t{FrFree},
usually in the configuration object's destructor.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{string}
@Func{FrBool string(const char *line, void *location, void *extra)}
The data value for a keyword is expected to be a quoted string, which will
be converted into a @t{char*} stored at the address pointed at by
@t{location}.  The string pointed at by the @t{char*} must be freed
with @t{FrFree}, usually in the configuration object's destructor.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{bitflags}
@Func{FrBool bitflags(const char *line, void *location, void *extra)}
The data value for the associated keyword is assumed to be a list of
symbols from the array of @t{FrCommandBit}s pointed at by @t{extra}.
The result placed in @t{location} is an @t{long}, each of whose bits
corresponds to the presence or absence of one of the symbols specified
in the @t{FrCommandBit} array.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{list}
@Func{FrBool list(const char *line, void *location, void *extra)}
The data value for the associated keyword is assumed to be a list of
arbitrary values.
!!!
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{assoclist}
@Func{FrBool assoclist(const char *line, void *location, void *extra)}
The data value for the associated keyword is assumed to be a list of
lists, where the first element of each sublist is used as a key value.
!!!
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{}
@Func{FrBool symbol(const char *line, void *location, void *extra)}
The data value for a keyword is expected to be a symbol, which will
be converted into a @t{FrSymbol*} stored at the address pointed at by
@t{location}. 
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{}
@Func{FrBool symlist(const char *line, void *location, void *extra)}
The data value for the associated keyword is expected to be a list of
symbols.
!!!
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{}
@Func{FrBool yesno(const char *line, void *location, void *extra)}
Sets a variable of type @t{FrBool} at @t{location} to indicate whether
the setting is affirmative or negative.  If the first non-whitespace
character following the keyword is 'Y', 'y', 'T', 't', '1', or '+', the
variable is set to @t{True}; if the character is 'N', 'n', 'F', 'f',
'0', or '-', the variable is set to @t{False}; otherwise, an error
message is displayed. 
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{}
@Func{FrBool invalid(const char *line, void *location, void *extra)}
Warn the user that an invalid keyword was encountered.  Always returns
@t{True}.
@end{FuncDesc}

@subsection{FrCommandBit}
@indextype{FrCommandBit}

The @t{FrCommandBit} structure is used to define bit flags for the
@t{FrConfiguration} class.  It consists of two data members: a
@t{char*} for the name, which is used to read the list of flags and in
writing them out again with @t{FrConfiguration::dumpFlags}, and a
@t{long} containing the bit mask for the associated flag.  @i{Exactly
one} bit should be set in each bit mask.

@t{FrCommandBit} structures are always used in an array.  The
terminating element of the array is a @t{FrCommandBit} with both name
and bit mask set to 0.

A sample command-bit array might be:

@begin{programexample}
static FrCommandBit options[] =
   {
     { "VERBOSE",       0x00000001 },
     { "SHORTCUTS",     0x00000002 },
     { "NETWORK_MODE",  0x00000004 },
     { 0,               0 }
   } ;
@end{programexample}


@comment{-------------------------------}
@section{Widget class FrWidget}
@indextype{FrWidget}

This is the base class for all of the remaining @t{FrW*} classes
described in the following sections.  It is basically a wrapper for the
X-Windows @t{Widget} object.

!!!

@begin{FuncDesc}
@indexctor{FrWidget}
@Func{FrWidget::FrWidget(Widget w)}
Create a FramepaC widget object from the indicated X-Windows widget.
@end{FuncDesc}

@begin{FuncDesc}
@indexctor{FrWidget}
@Func{FrWidget::FrWidget(Widget w, FrBool m)}
@end{FuncDesc}

@begin{FuncDesc}
@indexdtor{~FrWidget}
@Func{virtual FrWidget::~FrWidget()}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{objType}
@Func{virtual FrObjectType FrWidget::objType() const}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{objTypeName}
@Func{virtual const char *FrWidget::objTypeName() const}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{objSuperclass}
@Func{virtual FrObjectType FrWidget::objSuperclass() const}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{printValue}
@Func{virtual ostream &FrWidget::printValue(ostream &output) const}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{displayValue}
@Func{virtual char *FrWidget::displayValue(char *buffer) const}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{displayLength}
@Func{virtual size_t FrWidget::displayLength() const}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{widgetp}
@Func{virtual FrBool FrWidget::widgetp() const}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{}
@Func{Widget FrWidget::operator * () const}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{destroy}
@Func{void FrWidget::destroy()}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{keep}
@Func{void FrWidget::keep()}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{retain}
@Func{void FrWidget::retain()}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{manage}
@Func{void FrWidget::manage()}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{unmanage}
@Func{void FrWidget::unmanage()}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{isManaged}
@Func{FrBool FrWidget::isManaged() const}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{setManaged}
@Func{void FrWidget::setManaged(FrBool m)}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{map}
@Func{void FrWidget::map()}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{unmap}
@Func{void FrWidget::unmap()}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{isMapped}
@Func{FrBool FrWidget::isMapped() const}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{mapWhenManaged}
@Func{void FrWidget::mapWhenManaged(FrBool do_map) const}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{setValue}
@Func{void FrWidget::setValue(XtPointer value) const}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{setUserData}
@Func{void FrWidget::setUserData(XtPointer data) const}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{getValue}
@Func{XtPointer FrWidget::getValue() const}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{getUserData}
@Func{XtPointer FrWidget::getUserData() const}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{getLabel}
@Func{char *FrWidget::getLabel() const}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{getWidth}
@Func{int FrWidget::getWidth(FrBool include_border = False) const}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{getHeight}
@Func{int FrWidget::getHeight(FrBool include_border = False) const}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{getSize}
@Func{void FrWidget::getSize(int *width, int *height, FrBool include_border = False) const}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{getPosition}
@Func{void FrWidget::getPosition(int *x, int *y) const}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{setSensitive}
@Func{void FrWidget::setSensitive(FrBool sensitive) const}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{setLabel}
@Func{void FrWidget::setLabel(const char *label) const}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{setTextString}
@Func{void FrWidget::setTextString(const char *text) const}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{setAlignment}
@Func{void FrWidget::setAlignment(int align) const}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{setEnds}
@Func{void FrWidget::setEnds(Widget from, Widget to) const}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{setOrientation}
@Func{void FrWidget::setOrientation(int orientation) const}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{setVertical}
@Func{void FrWidget::setVertical(FrBool vertical) const}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{setShadow}
@Func{void FrWidget::setShadow(int thickness) const}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{setTitle}
@Func{void FrWidget::setTitle(const char *title) const}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{setIconName}
@Func{void FrWidget::setIconName(const char *title) const}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{setPosition}
@Func{void FrWidget::setPosition(int x, int y) const}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{setWidth}
@Func{void FrWidget::setWidth(int width) const}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{setHeight}
@Func{void FrWidget::setHeight(int height) const}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{allowResize}
@Func{void FrWidget::allowResize(FrBool state) const}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{invertVideo}
@Func{void FrWidget::invertVideo() const}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{setTraversal}
@Func{void FrWidget::setTraversal(FrBool state) const}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{traverseCurrent}
@Func{void FrWidget::traverseCurrent() const}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{traverseNext}
@Func{void FrWidget::traverseNext() const}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{traversePrev}
@Func{void FrWidget::traversePrev() const}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{traverseNextGroup}
@Func{void FrWidget::traverseNextGroup() const}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{traversePrevGroup}
@Func{void FrWidget::traversePrevGroup() const}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{traverseUp}
@Func{void FrWidget::traverseUp() const}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{traverseDown}
@Func{void FrWidget::traverseDown() const}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{navigationTabGroup}
@Func{void FrWidget::navigationTabGroup() const}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{removeCallbacks}
@Func{void FrWidget::removeCallbacks(char *type) const}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{addCallback}
@Func{void FrWidget::addCallback(char *type, XtCallbackProc cb_func, XtPointer cb_data)}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{removeCallback}
@Func{void FrWidget::removeCallback(char *type, XtCallbackProc cb_func, XtPointer cb_data) const}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{armCallback}
@Func{void FrWidget::armCallback(XtCallbackProc cb_func, XtPointer cb_data, FrBool remove_old = False)}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{disarmCallback}
@Func{void FrWidget::disarmCallback(XtCallbackProc cb_func, XtPointer cb_data, FrBool remove_old = False)}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{activateCallback}
@Func{void FrWidget::activateCallback(XtCallbackProc cb_func, XtPointer cb_data, FrBool remove_old = False)}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{activateCallback}
@Func{void FrWidget::activateCallback(XtCallbackProc cb_func)}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{okCallback}
@Func{void FrWidget::okCallback(XtCallbackProc cb_func, XtPointer cb_data, FrBool remove_old = False)}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{cancelCallback}
@Func{void FrWidget::cancelCallback(XtCallbackProc cb_func, XtPointer cb_data, FrBool remove_old = False)}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{helpCallback}
@Func{void FrWidget::helpCallback(XtCallbackProc cb_func, XtPointer cb_data, FrBool remove_old = False)}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{nomatchCallback}
@Func{void FrWidget::nomatchCallback(XtCallbackProc cb_func, XtPointer cb_data, FrBool remove_old= False)}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{changedCallback}
@Func{void FrWidget::changedCallback(XtCallbackProc cb_func, XtPointer cb_data, FrBool remove_old = False)}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{verifyCallback}
@Func{void FrWidget::verifyCallback(XtCallbackProc cb_func, XtPointer cb_data, FrBool remove_old = False)}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{focusCallback}
@Func{void FrWidget::focusCallback(XtCallbackProc cb_func, XtPointer cb_data, FrBool remove_old = False)}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{loseFocusCallback}
@Func{void FrWidget::loseFocusCallback(XtCallbackProc cb_func, XtPointer cb_data, FrBool remove_old = False)}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{resizeCallback}
@Func{void FrWidget::resizeCallback(XtCallbackProc cb_func, XtPointer cb_data, FrBool remove_old = False)}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{destroyCallback}
@Func{void FrWidget::destroyCallback(XtCallbackProc cb_func, FrBool remove_old = False)}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{destroyCallback}
@Func{void FrWidget::destroyCallback(FrBool remove_old = False)}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{setFont}
@Func{void FrWidget::setFont(const char *fontname)}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{attach}
@Func{void FrWidget::attach(Widget top,Widget bottom,Widget left,Widget right)}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{attach}
@Func{void FrWidget::attach(FrWidget *top,FrWidget *bottom,FrWidget *left,
		  FrWidget *right)}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{attachOpposite}
@Func{void FrWidget::attachOpposite(Widget top,Widget bottom,Widget left,Widget right)}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{attachOpposite}
@Func{void FrWidget::attachOpposite(FrWidget *top,FrWidget *bottom,FrWidget *left,
		  FrWidget *right)}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{attachOffsets}
@Func{void FrWidget::attachOffsets(int top, int bottom, int left, int right)}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{attachPosition}
@Func{void FrWidget::attachPosition(int top, int bottom, int left, int right)}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{detach}
@Func{void FrWidget::detach(FrBool top, FrBool bottom, FrBool left, FrBool right)}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{warpPointer}
@Func{void FrWidget::warpPointer(int x, int y) const}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{warpPointerCenter}
@Func{void FrWidget::warpPointerCenter() const}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{getPointer}
@Func{void FrWidget::getPointer(int *x, int *y) const}
@end{FuncDesc}

@begin{FuncDesc}
@Indexmeth{parentWidget}
@Func{Widget FrWidget::parentWidget() const}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{grandparent}
@Func{Widget FrWidget::grandparent() const}
@end{FuncDesc}


@comment{-------------------------------}
@section{Widget class FrWArrow}
@indextype{FrWArrow}

This class encapsulates the arrow-head widget, used in scrollbars and
similar visual elements on the screen.

In addition to the methods inherited from @t{FrWidget} (not all of
which apply to arrows), @t{FrWArrow} provides the following methods of
its own:

@begin{FuncDesc}
@indexctor{FrWArrow}
@Func{FrWArrow::FrWArrow(Widget parent, ArrowDirection direction)}
!!!
@end{FuncDesc}

@begin{FuncDesc}
@indexctor{FrWArrow}
@Func{FrWArrow::FrWArrow(FrWidget *parent, ArrowDirection direction)}
!!!
@end{FuncDesc}


@comment{-------------------------------}
@section{Widget class FrWArrowG}
@indextype{FrWArrowG}

This class encaspulates the arrow-head gadget, which is logically
equivalent to the arrow-head widget for @t{FrWArrow}, but slightly more
restricted due to the limitations of X-Windows gadgets vs. widgets.

In addition to the methods inherited from @t{FrWidget} (not all of
which apply to arrows) and @t{FrWArrow}, @t{FrWArrowG} provides the
following methods of its own:

@begin{FuncDesc}
@indexctor{FrWArrow}
@Func{FrWArrowG::FrWArrowG(Widget parent, ArrowDirection direction)}
!!!
@end{FuncDesc}

@begin{FuncDesc}
@indexctor{FrWArrow}
@Func{FrWArrowG::FrWArrowG(FrWidget *parent, ArrowDirection direction)}
!!!
@end{FuncDesc}


@comment{-------------------------------}
@section{Widget class FrWButtonBar}
@indextype{FrWButtonBar}

This class encapsulates a linear bar containing a series of buttons on
which the user may click to select various actions.

In addition to the methods inherited from @t{FrWidget} (not all of
which apply to button bars), @t{FrWButtonBar} provides the following
methods of its own:

@begin{FuncDesc}
@indexctor{FrWButtonBar}
@Func{FrWButtonBar::FrWButtonBar(Widget parent, const FrButtonsAndCommands *buttons,
		   int bcount, FrBool managed, XtPointer default_data)}
@end{FuncDesc}

@begin{FuncDesc}
@indexctor{FrWButtonBar}
@Func{FrWButtonBar::FrWButtonBar(FrWidget *parent, const FrButtonsAndCommands *buttons,
		   int bcount, FrBool managed, XtPointer default_data)}
@end{FuncDesc}

@begin{FuncDesc}
@indexdtor{FrWButtonBar}
@Func{virtual FrWButtonBar::~FrWButtonBar()}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{nthButton}
@Func{FrWPushButton *FrWButtonBar::nthButton(int n)}
@end{FuncDesc}


@comment{-------------------------------}
@section{Widget class FrWDialogPopup}
@indextype{FrWDialogPopup}

This class encapsulates a popup window containing a user dialog.

In addition to the methods inherited from @t{FrWidget} (not all of
which apply to popups), @t{FrWDialogPopup} provides the following
methods of its own:

@begin{FuncDesc}
@indexctor{FrWDialogPopup}
@Func{FrWDialogPopup::FrWDialogPopup(Widget w)}
@end{FuncDesc}

@begin{FuncDesc}
@indexctor{FrWDialogPopup}
@Func{FrWDialogPopup::FrWDialogPopup(Widget parent, const char *name, const char *title = 0,
		     const char *icon = 0, FrBool auto_unmanage = False)}
@end{FuncDesc}

@begin{FuncDesc}
@indexctor{FrWDialogPopup}
@Func{FrWDialogPopup::FrWDialogPopup(FrWidget *parent, const char *name, const char *title = 0,
		     const char *icon = 0, FrBool auto_unmanage = False)}
@end{FuncDesc}

@comment{-------------------------------}
@section{Widget class FrWForm}
@indextype{FrWForm}
@index{forms}

This class encapsulates a window containing a form.

In addition to the methods inherited from @t{FrWidget} (not all of
which apply to forms), @t{FrWForm} provides the following
methods of its own:

@begin{FuncDesc}
@indexctor{FrWForm}
@Func{FrWForm::FrWForm(Widget parent, const char *name, FrBool managed = False,
	      const char *icon_name = 0)}
@end{FuncDesc}

@begin{FuncDesc}
@indexctor{FrWForm}
@Func{FrWForm::FrWForm(FrWidget *parent, const char *name, FrBool managed = False,
	      const char *icon_name = 0)}
@end{FuncDesc}


@comment{-------------------------------}
@section{Widget class FrWFrameCompleter}
@indextype{FrWFrameCompleter}

This class encapsulates a fill-in text field for which the TAB key
performs frame-name completion.  On pressing TAB when at least two
characters of a frame's name have been entered, as many additional
characters as are uniquely identified by the already-entered prefix are
inserted into the fill-in field, and if the result is not a complete
frame name, a completion dialog is popped up.  The dialog contains
either a list of all valid completions or a list of valid continuations
(if there are too many matching names).

In addition to the methods inherited from @t{FrWidget} (not all of
which apply to completion fields), @t{FrWFrameCompleter} provides the
following methods of its own:

@begin{FuncDesc}
@indexctor{FrWFrameCompleter}
@Func{FrWFrameCompleter::FrWFrameCompleter(Widget parent, const char *initial, 
		int columns=30, XtCallbackProc done_cb=0, XtPointer done_data=0)}
Create a frame-completion text fill-in field as a child of the
indicated widget, setting its initial contents to @t{initial} and its
width to @t{columns} characters.  If @t{done_cb} is not NULL, it will
be called with @t{done_data} when the user accepts the input value in
the text field.
@end{FuncDesc}

@begin{FuncDesc}
@indexctor{FrWFrameCompleter}
@Func{FrWFrameCompleter::FrWFrameCompleter(FrWidget *parent, const char *initial, 
		int columns=30, XtCallbackProc done_cb=0, XtPointer done_data=0)}
Create a frame-completion text fill-in field as a child of the
indicated @t{FrWidget}, setting its initial contents to @t{initial} and its
width to @t{columns} characters.  If @t{done_cb} is not NULL, it will
be called with @t{done_data} when the user accepts the input value in
the text field.
@end{FuncDesc}


@comment{-------------------------------}
@section{Widget class FrWFramePrompt}
@indextype{FrWFramePrompt}

This class encapsulates a popup dialog containing a fill-in text field
for which the TAB key performs frame-name completion as described for
@t{FrWFrameCompleter}. 

In addition to the methods inherited from @t{FrWidget} (not all of
which apply to prompt popups), @t{FrWFramePrompt} provides the following
methods of its own:

@begin{FuncDesc}
@indexctor{FrWFramePrompt}
@Func{FrWFramePrompt::FrWFramePrompt(Widget parent, const char *label, const char *def,
		     XtCallbackProc ok_cb = 0, XtPointer ok_data = 0,
		     XtPointer help_data = 0,
		     FrBool auto_unmanage = True, FrBool modal = False,
		     FrBool managed = True)}
@end{FuncDesc}

@begin{FuncDesc}
@indexctor{FrWFramePrompt}
@Func{FrWFramePrompt::FrWFramePrompt(FrWidget *parent, const char *label, const char *def,
		     XtCallbackProc ok_cb = 0, XtPointer ok_data = 0,
		     XtPointer help_data = 0,
		     FrBool auto_unmanage = True, FrBool modal = False,
		     FrBool managed = True)}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{setFrameName}
@Func{void FrWFramePrompt::setFrameName(_FrSymbol *framename) const}
@end{FuncDesc}

@comment{-------------------------------}
@section{Widget class FrWFrame}
@indextype{FrWFrame}

This class encapsulates a window-frame object which may be placed
around another window object.

In addition to the methods inherited from @t{FrWidget} (not all of
which apply to window frames), @t{FrWFrame} provides the following
methods of its own:

@begin{FuncDesc}
@indexctor{FrWFrame}
@Func{FrWFrame::FrWFrame(Widget parent)}
@end{FuncDesc}

@begin{FuncDesc}
@indexctor{FrWFrame}
@Func{FrWFrame::FrWFrame(FrWidget *parent)}
@end{FuncDesc}


@comment{-------------------------------}
@section{Widget class FrWLabel}
@indextype{FrWLabel}

This class encapsulates a Motif textual-label widget.

In addition to the methods inherited from @t{FrWidget} (not all of
which apply to text labels), @t{FrWLabel} provides the following
methods of its own:

@begin{FuncDesc}
@indexctor{FrWLabel}
@Func{FrWLabel::FrWLabel(Widget parent, const char *label, FrBool managed = True)}
Create a new label as a child of the indicated widget, with the given
text string as the contents of the new label.  If @t{managed} is
@t{False}, the new label widget is left unmanaged, and will not be
displayed until explicitly managed.
@end{FuncDesc}

@begin{FuncDesc}
@indexctor{FrWLabel}
@Func{FrWLabel::FrWLabel(FrWidget *parent, const char *label, FrBool managed = True)}
Create a new label as a child of the indicated @t{FrWidget}, with the given
text string as the contents of the new label.  If @t{managed} is
@t{False}, the new label widget is left unmanaged, and will not be
displayed until explicitly managed.
@end{FuncDesc}


@comment{-------------------------------}
@section{Widget class FrWList}
@indextype{FrWList}

This class encapsulates a Motif list box.

In addition to the methods inherited from @t{FrWidget} (not all of
which apply to list boxes), @t{FrWList} provides the following
methods of its own:

@begin{FuncDesc}
@indexctor{FrWList}
@Func{FrWList::FrWList(Widget parent, const char *name, int visible, int max = 0)}
Create a new list box with the indicated name as a child of the given
widget, showing a maximum of @t{visible} items on-screen at a time
(allowing scrolling to any additional items).  If @t{max} is non-zero,
then the total number of items in the list box will be limited to
@t{max} items at any time by removing the top-most item each time a new
item is added at the bottom of the list.
@end{FuncDesc}

@begin{FuncDesc}
@indexctor{FrWList}
@Func{FrWList::FrWList(FrWidget *parent, const char *name, int visible, int max = 0)}
Create a new list box with the indicated name as a child of the given
@t{FrWidget}, showing a maximum of @t{visible} items on-screen at a
time (allowing scrolling to any additional items).  If @t{max} is
non-zero, then the total number of items in the list box will be
limited to @t{max} items at any time by removing the top-most item each
time a new item is added at the bottom of the list.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{add}
@Func{void FrWList::add(const char *item)}
Add the indicated string as a new item at the bottom of the item list
already contained by the list box.  If the maximum number of items was
limited at the time the list box was constructed and the list already
contained the maximum number of items, the top-most item will be
removed. 
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{totalItems}
@Func{int FrWList::totalItems() const}
Determine how many items are currently stored in the list box.
@end{FuncDesc}


@comment{-------------------------------}
@section{Widget class FrWMainWindow}
@indextype{FrWMainWindow}

This class encapsulates a Motif application's main window.

In addition to the methods inherited from @t{FrWidget} (not all of
which apply to main windows), @t{FrWMainWindow} provides the following
methods of its own:

@begin{FuncDesc}
@indexctor{FrWMainWindow}
@Func{FrWMainWindow::FrWMainWindow(Widget parent, const char *name, const char *title,
 		    const char *icon = 0,
		    int height = 0, int width = 0, FrBool managed = False,
		    FrBool show_sep = False)}
@end{FuncDesc}

@begin{FuncDesc}
@indexctor{FrWMainWindow}
@Func{FrWMainWindow::FrWMainWindow(FrWidget *parent, const char *name, const char *title,
 		    const char *icon = 0,
		    int height = 0, int width = 0, FrBool managed = False,
		    FrBool show_sep = False)}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{setWorkWindow}
@Func{void FrWMainWindow::setWorkWindow(Widget) const}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{setMenuBar}
@Func{void FrWMainWindow::setMenuBar(Widget) const}
@end{FuncDesc}

@comment{-------------------------------}
@section{Widget class FrWOptionMenu}
@indextype{FrWOptionMenu}

In addition to the methods inherited from @t{FrWidget} (not all of
which apply to option menus), @t{FrWOptionMenu} provides the following
methods of its own:

@begin{FuncDesc}
@indexctor{FrWOptionMenu}
@Func{FrWOptionMenu::FrWOptionMenu(Widget parent, const char *name, FrList *options,
		    FrObject *def_option = 0, FrBool managed = False)}
@end{FuncDesc}

@begin{FuncDesc}
@indexctor{FrWOptionMenu}
@Func{FrWOptionMenu::FrWOptionMenu(FrWidget *parent, const char *name, FrList *options,
		    FrObject *def_option = 0, FrBool managed = False)}
@end{FuncDesc}

@begin{FuncDesc}
@indexdtor{FrWOptionMenu}
@Func{virtual FrWOptionMenu::~FrWOptionMenu()}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{currentSelection}
@Func{FrObject *FrWOptionMenu::currentSelection() const}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{setSelection}
@Func{void FrWOptionMenu::setSelection(const char *selection_text)}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{changeCallback}
@Func{void FrWOptionMenu::changeCallback(XtCallbackProc cb_func, XtPointer cb_data,
			  FrBool remove_old = False)}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{changeCallback}
@Func{void FrWOptionMenu::changeCallback(XtCallbackProc cb_func)}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{callChangeCallback}
@Func{void FrWOptionMenu::callChangeCallback(XtPointer call_data) const}
@end{FuncDesc}


@comment{-------------------------------}
@section{Widget class FrWPopupMenu}
@indextype{FrWPopupMenu}

In addition to the methods inherited from @t{FrWidget} (not all of
which apply to popup menus), @t{FrWPopupMenu} provides the following
methods of its own:

@begin{FuncDesc}
@indexctor{FrWPopupMenu}
@Func{FrWPopupMenu::FrWPopupMenu(Widget parent, const char *name, FrBool managed = False)}
@end{FuncDesc}

@begin{FuncDesc}
@indexctor{FrWPopupMenu}
@Func{FrWPopupMenu::FrWPopupMenu(FrWidget *parent, const char *name, FrBool managed = False)}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{popup}
@Func{virtual void FrWPopupMenu::popup()}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{menuPosition}
@Func{void FrWPopupMenu::menuPosition(XtPointer call_data)}
@end{FuncDesc}


@comment{-------------------------------}
@section{Widget class FrWProgressIndicator}
@indextype{FrWProgressIndicator}

This class encapsulates a progress indicator in which a horizontal bar
progressively changes color as some "lengthy" process is performed.

In addition to the methods inherited from @t{FrWidget} (not all of
which apply to progress indicators), @t{FrWProgressIndicator} provides
the following methods of its own:

@begin{FuncDesc}
@indexctor{FrWProgressIndicator}
@Func{FrWProgressIndicator::FrWProgressIndicator(Widget parent)}
@end{FuncDesc}

@begin{FuncDesc}
@indexctor{FrWProgressIndicator}
@Func{FrWProgressIndicator::FrWProgressIndicator(Widget parent, FrBool managed,
			   const char *piclass = 0)}
@end{FuncDesc}

@begin{FuncDesc}
@indexctor{FrWProgressIndicator}
@Func{FrWProgressIndicator::FrWProgressIndicator(FrWidget *parent, FrBool managed = True, 
			   const char *piclass = 0)}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{setProgress}
@Func{void FrWProgressIndicator::setProgress(int percentage) const}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{getProgress}
@Func{int FrWProgressIndicator::getProgress() const}
@end{FuncDesc}


@comment{-------------------------------}
@section{Widget class FrWProgressPopup}
@indextype{FrWProgressPopup}

This class encapsulates a progress indicator as described under
@t{FrWProgressIndicator} within its own popup window.

In addition to the methods inherited from @t{FrWidget} (not all of
which apply to progress indicators), @t{FrWProgressPopup} provides the
following methods of its own:

@begin{FuncDesc}
@indexctor{FrWProgressPopup}
@Func{FrWProgressPopup::FrWProgressPopup(Widget parent, const char *label,
		       const char *ppclass = 0)}
@end{FuncDesc}

@begin{FuncDesc}
@indexctor{FrWProgressPopup}
@Func{FrWProgressPopup::FrWProgressPopup(FrWidget *parent, const char *label,
		       const char *ppclass = 0)}
@end{FuncDesc}

@begin{FuncDesc}
@indexdtor{FrWProgressPopup}
@Func{virtual FrWProgressPopup::~FrWProgressPopup()}
@end{FuncDesc}


@comment{-------------------------------}
@section{Widget class FrWPromptPopup}
@indextype{FrWPromptPopup}

This class encapsulates a popup window containing a user prompt dialog.

In addition to the methods inherited from @t{FrWidget} (not all of
which apply to popups), @t{FrWPromptPopup} provides the following
methods of its own:

@begin{FuncDesc}
@indexctor{FrWPromptPopup}
@Func{FrWPromptPopup::FrWPromptPopup(Widget w)}
@end{FuncDesc}

@begin{FuncDesc}
@indexctor{FrWPromptPopup}
@Func{FrWPromptPopup::FrWPromptPopup(Widget parent, const char *label, const char *def,
		     XtCallbackProc ok_cb = 0, XtPointer ok_data = 0,
		     XtPointer help_data = 0,
		     FrBool auto_unmanage = True, FrBool modal = False,
		     FrBool managed = True)}
@end{FuncDesc}

@begin{FuncDesc}
@indexctor{FrWPromptPopup}
@Func{FrWPromptPopup::FrWPromptPopup(FrWidget *parent, const char *label, const char *def,
		     XtCallbackProc ok_cb = 0, XtPointer ok_data = 0,
		     XtPointer help_data = 0,
		     FrBool auto_unmanage = True, FrBool modal = False,
		     FrBool managed = True)}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{popup}
@Func{virtual void FrWPromptPopup::popup()}
@end{FuncDesc}


@comment{-------------------------------}
@section{Widget class FrWPulldownMenu}
@indextype{FrWPulldownMenu}

In addition to the methods inherited from @t{FrWidget} (not all of
which apply to pulldown menus), @t{FrWPulldownMenu} provides the following
methods of its own:

@begin{FuncDesc}
@indexctor{FrWPulldownMenu}
@Func{FrWPulldownMenu::FrWPulldownMenu(Widget parent, const char *name, FrBool managed = False)}
@end{FuncDesc}

@begin{FuncDesc}
@indexctor{FrWPulldownMenu}
@Func{FrWPulldownMenu::FrWPulldownMenu(FrWidget *parent, const char *name, FrBool managed=False)}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{popup}
@Func{virtual void FrWPulldownMenu::popup()}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{pulldown}
@Func{void FrWPulldownMenupulldown(int button)}
@end{FuncDesc}


@comment{-------------------------------}
@section{Widget class FrWPushButton}
@indextype{FrWPushButton}

In addition to the methods inherited from @t{FrWidget} (not all of
which apply to pushbuttons), @t{FrWPushButton} provides the following
methods of its own:

@begin{FuncDesc}
@indexctor{FrWPushButton}
@Func{FrWPushButton::FrWPushButton(Widget button)}
@end{FuncDesc}

@begin{FuncDesc}
@indexctor{FrWPushButton}
@Func{FrWPushButton::FrWPushButton(Widget parent, const char *label, FrBool managed = True,
		    FrBool centered = True, const char *pbclass = 0)}
@end{FuncDesc}

@begin{FuncDesc}
@indexctor{FrWPushButton}
@Func{FrWPushButton::FrWPushButton(FrWidget *parent, const char *label, FrBool managed = True,
		    FrBool centered = True, const char *pbclass = 0)}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{setAlignment}
@Func{void FrWPushButton::setAlignment(int align) const}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{setMnemonic}
@Func{void FrWPushButton::setMnemonic(char mnem) const}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{setDualMouseButton}
@Func{void FrWPushButton::setDualMouseButton() const}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{setTripleMouseButton}
@Func{void FrWPushButton::setTripleMouseButton() const}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{pressedButton}
@Func{static int FrWPushButton::pressedButton(XtPointer call_data)}
@end{FuncDesc}


@comment{-------------------------------}
@section{Widget class FrWPushButtonG}
@indextype{FrWPushButtonG}

In addition to the methods inherited from @t{FrWidget} (not all of
which apply to pushbuttons) and from @t{FrWPushbutton},
@t{FrWPushButtonG} provides the following methods of its own:

@begin{FuncDesc}
@indexctor{FrWPushButtonG}
@Func{FrWPushButtonG::FrWPushButtonG(Widget parent, const char *label, FrBool managed = True,
		    FrBool centered = True, const char *pbclass = 0)}
@end{FuncDesc}

@begin{FuncDesc}
@indexctor{FrWPushButtonG}
@Func{FrWPushButtonG::FrWPushButtonG(FrWidget *parent, const char *label, FrBool managed = True,
		    FrBool centered = True, const char *pbclass = 0)}
@end{FuncDesc}


@comment{-------------------------------}
@section{Widget class FrWCascadeButton}
@indextype{FrWCascadeButton}

In addition to the methods inherited from @t{FrWidget} (not all of
which apply to buttons), @t{FrWCascadeButton} provides the following
methods of its own:

@begin{FuncDesc}
@indexctor{FrWCascadeButton}
@Func{FrWCascadeButton::FrWCascadeButton(Widget parent, const char *label, char mnemonic = '\0',
		       Widget submenuID = 0)}
@end{FuncDesc}

@begin{FuncDesc}
@indexctor{FrWCascadeButton}
@Func{FrWCascadeButton::FrWCascadeButton(FrWidget *parent, const char *label,
		       char mnemonic = '\0', FrWidget *submenuID = 0)}
@end{FuncDesc}


@comment{-------------------------------}
@section{Widget class FrWRadioBox}
@indextype{FrWRadioBox}

In addition to the methods inherited from @t{FrWidget} (not all of
which apply to radio boxes), @t{FrWRadioBox} provides the following
methods of its own:

@begin{FuncDesc}
@indexctor{FrWRadioBox}
@Func{FrWRadioBox::FrWRadioBox(Widget parent, const char *title, const char *name = 0,
		  FrBool vertical = True, FrBool managed = True)}
@end{FuncDesc}

@begin{FuncDesc}
@indexctor{FrWRadioBox}
@Func{FrWRadioBox::FrWRadioBox(FrWidget *parent, const char *title, const char *name = 0,
		  FrBool vertical = True, FrBool managed = True)}
@end{FuncDesc}


@comment{-------------------------------}
@section{Widget class FrWRowColumn}
@indextype{FrWRowColumn}

In addition to the methods inherited from @t{FrWidget} (not all of
which apply to matrix objects), @t{FrWRowColumn} provides the following
methods of its own:

@begin{FuncDesc}
@indexctor{FrWRowColumn}
@Func{FrWRowColumn::FrWRowColumn(Widget parent, const char *name, FrBool vertical=False)}
@end{FuncDesc}

@begin{FuncDesc}
@indexctor{FrWRowColumn}
@Func{FrWRowColumn::FrWRowColumn(FrWidget *parent, const char *name, FrBool vertical=False)}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{setColumns}
@Func{void FrWRowColumn::setColumns(int columns) const}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{setPacked}
@Func{void FrWRowColumn::setPacked(int packed) const}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{setAlignment}
@Func{void FrWRowColumn::setAlignment(int align) const}
@end{FuncDesc}


@comment{-------------------------------}
@section{Widget class FrWScrollBar}
@indextype{FrWScrollBar}

In addition to the methods inherited from @t{FrWidget} (not all of
which apply to scroll bars), @t{FrWScrollBar} provides the following
methods of its own:

@begin{FuncDesc}
@indexctor{FrWScrollBar}
@Func{FrWScrollBar::FrWScrollBar(Widget scrollbar)}
@end{FuncDesc}

@begin{FuncDesc}
@indexctor{FrWScrollBar}
@Func{FrWScrollBar::FrWScrollBar(Widget parent, FrBool vertical, int limit = 100,
	FrBool managed = True, const char *sbclass = 0)}
@end{FuncDesc}

@begin{FuncDesc}
@indexctor{FrWScrollBar}
@Func{FrWScrollBar::FrWScrollBar(FrWidget *parent, FrBool vertical, int limit = 100,
	FrBool managed = True, const char *sbclass = 0)}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{setLimit}
@Func{void FrWScrollBar::setLimit(int limit) const}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{getLimit}
@Func{int FrWScrollBar::getLimit() const}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{setStart}
@Func{void FrWScrollBar::setStart(int start) const}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{setLength}
@Func{void FrWScrollBar::setLength(int length) const}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{setThumb}
@Func{void FrWScrollBar::setThumb(int start, int length) const}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{getThumb}
@Func{void FrWScrollBar::getThumb(int *start, int *length) const}
@end{FuncDesc}


@comment{-------------------------------}
@section{Widget class FrWScrollWindow}
@indextype{FrWScrollWindow}

In addition to the methods inherited from @t{FrWidget} (not all of
which apply to scrollable windows), @t{FrWScrollWindow} provides the
following methods of its own:

@begin{FuncDesc}
@indexctor{FrWScrollWindow}
@Func{FrWScrollWindow::FrWScrollWindow(Widget w)}
@end{FuncDesc}

@begin{FuncDesc}
@indexctor{FrWScrollWindow}
@Func{FrWScrollWindow::FrWScrollWindow(Widget parent, const char *title,
	FrBool autoscroll = True, FrBool managed = True)}
@end{FuncDesc}

@begin{FuncDesc}
@indexctor{FrWScrollWindow}
@Func{FrWScrollWindow::FrWScrollWindow(FrWidget *parent, const char *title,
	FrBool autoscroll = True, FrBool managed = True)}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{workWindow}
@Func{FrWidget *FrWScrollWindow::workWindow() const}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{scrollBorder}
@Func{void FrWScrollWindow::scrollBorder(int width) const}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{forceScrollBar}
@Func{void FrWScrollWindow::forceScrollBar(FrBool force) const}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{scrollTo}
@Func{void FrWScrollWindow::scrollTo(Widget w, int horz_margin, int vert_margin) const}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{scrollTo}
@Func{void FrWScrollWindow::scrollTo(FrWidget *w, int horz_margin, int vert_margin) const}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{scrollBar}
@Func{Widget FrWScrollWindow::scrollBar(FrBool vertical) const }
@end{FuncDesc}


@comment{-------------------------------}
@section{Widget class FrWSelectionBox}
@indextype{FrWSelectionBox}

In addition to the methods inherited from @t{FrWidget} (not all of
which apply to selection boxes), @t{FrWSelectionBox} provides the following
methods of its own:

@begin{FuncDesc}
@indexctor{FrWSelectionBox}
@Func{FrWSelectionBox::FrWSelectionBox(Widget parent, FrList *items, char *label,
		   XtCallbackProc ok_cb, XtPointer ok_data,
		   XtCallbackProc nomatch_cb,XtPointer nomatch_data,
		   XtCallbackProc apply_cb, XtPointer apply_data,
		   const char **helptexts = 0,
		   FrBool must_match = True,int visitems = 10,
	           FrBool managed = True)}
@end{FuncDesc}

@begin{FuncDesc}
@indexctor{FrWSelectionBox}
@Func{FrWSelectionBox::FrWSelectionBox(FrWidget *parent, FrList *items, char *label,
		   XtCallbackProc ok_cb, XtPointer ok_data,
		   XtCallbackProc nomatch_cb,XtPointer nomatch_data,
		   XtCallbackProc apply_cb, XtPointer apply_data,
		   const char **helptexts = 0,
		   FrBool must_match = True,int visitems = 10,
	           FrBool managed = True)}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{unmanageChild}
@Func{void FrWSelectionBox::unmanageChild(FrWSelBoxChild child) const}
!!!
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{buttonLabel}
@Func{void FrWSelectionBox::buttonLable(FrWSelBoxChild button, const char *label) const}
!!!
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{selectionText}
@Func{char *FrWSelectionBox::selectionText(XtPointer call_data)}
!!!
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{selectionFrObject}
@Func{FrObject *FrWSelectionBox::selectionFrObject(XtPointer call_data)}
!!!
@end{FuncDesc}


@comment{-------------------------------}
@section{Widget class FrWSeparator}
@indextype{FrWSeparator}

In addition to the methods inherited from @t{FrWidget} (not all of
which apply to separators), @t{FrWSeparator} provides the following
methods of its own:

@begin{FuncDesc}
@indexctor{FrWSeparator}
@Func{FrWSeparator::FrWSeparator(Widget parent, int linestyle = -1)}
!!!
@end{FuncDesc}

@begin{FuncDesc}
@indexctor{FrWSeparator}
@Func{FrWSeparator::FrWSeparator(FrWidget *parent, int linestyle = -1)}
!!!
@end{FuncDesc}


@comment{-------------------------------}
@section{Widget class FrWShadowText}
@indextype{FrWShadowText}

In addition to the methods inherited from @t{FrWidget} (not all of
which apply to shadowed-text fields), @t{FrWShadowText} provides the
following methods of its own:

@begin{FuncDesc}
@indexctor{FrWShadowText}
@Func{FrWShadowText::FrWShadowText(Widget w)}
!!!
@end{FuncDesc}

@begin{FuncDesc}
@indexctor{FrWShadowText}
@Func{FrWShadowText::FrWShadowText(Widget parent, const char *text, int columns = 30,
      int rows = 1, FrBool editable = True, 
      FrBool traversal = True, FrBool wrap = True, 
      const char *name = 0)}
@end{FuncDesc}

@begin{FuncDesc}
@indexctor{FrWShadowText}
@Func{FrWShadowText::FrWShadowText(FrWidget *parent, const char *text, int columns = 30,
      int rows = 1, FrBool editable = True, 
      FrBool traversal = True, FrBool wrap = True, 
      const char *name = 0)}
@end{FuncDesc}

@begin{FuncDesc}
@indexdtor{FrWShadowText}
@Func{virtual FrWShadowText::~FrWShadowText()}
!!!
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{getText}
@Func{char *FrWShadowText::getText() const}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{getText}
@Func{void FrWShadowText::getText(char *buf, int buflen) const}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{setText}
@Func{void FrWShadowText::setText(const char *t, FrBool to_end = True)}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{verifyCallback}
@Func{void FrWShadowText::verifyCallback(FrWText_VerifyFunc *verify)}
@end{FuncDesc}


@comment{-------------------------------}
@section{Widget class FrWSlider}
@indextype{FrWSlider}

In addition to the methods inherited from @t{FrWidget} (not all of
which apply to sliders), @t{FrWSlider} provides the following
methods of its own:

@begin{programexample} !!!
FrWSlider(Widget parent, const char *name, int max, int min, 
	int def_value, int precision, FrBool vertical = False)
FrWSlider(FrWidget *parent, const char *name, int max, int min, 
	int def_value, int precision, FrBool vertical = False)
static int sliderValue(XtPointer call_data) ;
@end{programexample}

@comment{-------------------------------}
@section{Widget class FrWText}
@indextype{FrWText}

In addition to the methods inherited from @t{FrWidget} (not all of
which apply to text windows), @t{FrWText} provides the following
methods of its own:

@begin{programexample} !!!
 FrWText(Widget w) ;
 FrWText(Widget parent, const char *text, int columns = 30,
      int rows = 1, FrBool editable = True,
      FrBool traversal = True, FrBool wrap = True,
      const char *name = 0) ;
 FrWText(FrWidget *parent, const char *text, int columns = 30,
      int rows = 1, FrBool editable = True,
      FrBool traversal = True, FrBool wrap = True,
      const char *name = 0) ;
 char *getText() const ;
 void getText(char *buf, int buflen) const ;
 void setText(const char *t,FrBool to_end = True) ;
 void limitLength(int maxlen) ;
 int lengthLimit() const ;
 void verifyCallback(FrWText_VerifyFunc *verify) ;
 void setSensitive(FrBool sensitive) const ;
 FrWText_VerifyFunc *getVerifyCallback() const ;
@end{programexample}


@comment{-------------------------------}
@section{Widget class FrWToggleButton}
@indextype{FrWToggleButton}

In addition to the methods inherited from @t{FrWidget} (not all of
which apply to toggle buttons), @t{FrWToggleButton} provides the
following methods of its own:

@begin{programexample} !!!
      FrWToggleButton(Widget parent, const char *label,
                      FrBool set = False,
		      FrBool managed = True) ;
      FrWToggleButton(FrWidget *parent, const char *label,
		      FrBool set = False,
		      FrBool managed = True) ;
      FrBool getState() const { return toggle_set ; }
      void setState(FrBool state) ;
      void alwaysVisible(FrBool vis) ;
@end{programexample}


@comment{-------------------------------}
@section{Character Manipulation}
@index{character manipulation}

FramepaC provides a number of 8-bit clean character manipulation
functions equivalent to those found in the standard C library (which
often fail on characters with the high bit set).  It also provides a
number of functions for dealing with 16-bit Unicode characters.

Note that unlike the ANSI C library's is...() functions, the results
from the FramepaC equivalents are not defined when given EOF as input.

@subsection{Eight-Bit Characters}
@index2{p="character size",s="8 bits"}

@begin{FuncDesc}
@indexfunc{Fr_isupper}
@indexmacro{Fr_isupper}
@Func{@i{macro} int Fr_isupper(char c)}
Determine whether the specified character is uppercase or not.  @t{c} must
be a character value in the range 0-255 (i.e. EOF is not allowed).
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{Fr_islower}
@indexmacro{Fr_islower}
@Func{@i{macro} int Fr_islower(char c)}
Determine whether the specified character is lowercase or not.  @t{c} must
be a character value in the range 0-255 (i.e. EOF is not allowed, and will
result in undefined behavior).
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{Fr_isdigit}
@indexmacro{Fr_isdigit}
@Func{@i{macro} int Fr_isdigit(char c)}
Determine whether the specified character is a numeric digit.  @t{c} must
be a character value in the range 0-255 (i.e. EOF is not allowed, and will
result in undefined behavior).
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{Fr_ispunct}
@indexmacro{Fr_ispunct}
@Func{@i{macro} int Fr_ispunct(char c)}
Determine whether the specified character is a punctuation mark.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{Fr_isspace}
@indexmacro{Fr_isspace}
@Func{@i{macro} int Fr_isspace(char c)}
Determine whether the specified character is whitespace.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{Fr_isalpha}
@indexmacro{Fr_isalpha}
@Func{@i{macro} int Fr_isalpha(char c)}
Determine whether the specified character is alphabetic.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{Fr_tolower}
@indexmacro{Fr_tolower}
@Func{@i{macro} char Fr_tolower(char c)}
Convert the specified character to lowercase if it is not already lowercase.
@t{c} must be a character value in the range 0-255 (i.e. EOF is not allowed,
and will result in undefined behavior).
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{Fr_toupper}
@indexmacro{Fr_toupper}
@Func{@i{macro} char Fr_toupper(char c)}
Convert the specified character to uppercase if it is not already uppercase.
@t{c} must be a character value in the range 0-255 (i.e. EOF is not allowed,
and will result in undefined behavior).
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{Fr_unacent_Latin1}
@indexmacro{Fr_unacent_Latin1}
@Func{@i{macro} char Fr_unaccent_Latin1(char c)}
Convert an accented character in the Latin-1 encoding into the
equivalent ASCII character without the accent, i.e. a-acute returns
'a', A-grave returns 'A', etc.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{Fr_unacent_Latin2}
@indexmacro{Fr_unacent_Latin2}
@Func{@i{macro} char Fr_unaccent_Latin2(char c)}
Convert an accented character in the Latin-2 encoding into the
equivalent ASCII character without the accent, i.e. a-acute returns
'a', A-grave returns 'A', etc.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{Fr_unacent}
@indexmacro{Fr_unacent}
@Func{@i{macro} char Fr_unaccent(char c)}
Convert an accented character in either the Latin-1 or the Latin-2
encoding (depending on whether FrDEFAULT_CHARSET_Latin1 has been
defined in frconfig.h) into the equivalent ASCII character without the
accent, i.e. a-acute returns 'a', A-grave returns 'A', etc.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{Fr_is8bit}
@indexmacro{Fr_is8bit}
@index{eight-bit characters}
@index2{p="character size", s="eight bits"}
@Func{@i{macro} bool Fr_is8bit(int c)}
Determine whether the indicated value is valid as an eight-bit
character (which allows it to be used for the above functions).
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{Fr_stricmp}
@indexfunc{stricmp}
@Func{int Fr_stricmp(const char *s1, const char *s2)}
Determine whether two strings of eight-bit characters are equivalent,
ignoring differences between uppercase and lowercase characters.  The
return value is 0 if the two strings are equivalent; a negative return
value indicates that @t{s1} precedes @t{s2} in the collating sequence,
while a positive return value indicates that @t{s2} precedes @t{s1}.
@end{FuncDesc}


@subsection{Eight-bit Strings}
@index2{p="strings",s="eight-bit"}

!!!

@begin{FuncDesc}
@indexfunc{FrCanonicalizeSentence}
@Func{char *FrCanonicalizeSentence(const char *string,FrBool force_uppercase = False,
			     const char *possible_delim /*[256]*/ = 0)}
!!!
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{FrDecanonicalizeSentence}
@Func{FrString *FrDecanonicalizeSentence(const char *string,
				   FrBool force_lowercase = False,
				   FrBool Unicode = False)}
@end{FuncDesc}

@begin{FuncDesc}
@index2{p="conversions",s="string to symbol-list"}
@indexfunc{FrCvtSentence2Symbollist}
@Func{FrList *FrCvtSentence2Symbollist(char *sentence, FrBool force_uppercase = True)}
!!!

THIS FUNCTION DESTROYS ITS ARGUMENT!
@end{FuncDesc}

@begin{FuncDesc}
@index2{p="conversions",s="string to word-list"}
@indexfunc{FrCvtSentence2Wordlist}
@Func{FrList *FrCvtSentence2Wordlist(const char *sentence)}
!!!
@end{FuncDesc}

@begin{FuncDesc}
@index2{p="conversions",s="string to word-list"}
@indexfunc{FrCvtString2Wordlist}
@Func{FrList *FrCvtString2Wordlist(const char *sentence,
			      const char *possible_delim /*[256]*/ = 0)}
!!!
@end{FuncDesc}

@begin{FuncDesc}
@index2{p="conversions",s="string to symbol-list"}
@indexfunc{FrCvtString2Symbollist}
@Func{FrList *FrCvtString2Symbollist(const char *sentence,
			        const char *possible_delim /*[256]*/ = 0)}
!!!
@end{FuncDesc}

@begin{FuncDesc}
@index2{p="conversions",s="string to symbol"}
@indexfunc{FrCvtString2Symbol}
@Func{FrSymbol *FrCvtString2Symbol(const char *word, FrBool force_uppercase = True)}
!!!
@end{FuncDesc}


@subsection{Sixteen-Bit Characters}
@index2{p="character size",s="16 bits"}
@index{wide characters}

Normally, ``wide'' characters use the @t{wchar_t} type, however, on
some architectures, @t{wchar_t} is in fact 32 bits!  FramepaC therefore
uses its own @t{FrChar16} type, which is exactly 16 bits regardless of
how big @t{wchar_t} is (in fact, it is usually defined as @t{wchar_t}).

@begin{FuncDesc}
@indexfunc{Fr_highbyte}
@indexmacro{Fr_highbyte}
@Func{@i{macro} char Fr_highbyte(FrChar16 c)}
Retrieve the high half of the 16-bit character @t{c}.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{Fr_lowbyte}
@indexmacro{Fr_lowbyte}
@Func{@i{macro} char Fr_lowbyte(FrChar16 c)}
Retrieve the low half of the 16-bit character @t{c}.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{Fr_isspace16}
@indexmacro{Fr_isspace16}
@Func{@i{macro} int Fr_isspace16(FrChar16 c)}
Determine whether the specified 16-bit character is a whitespace character.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{Fr_isdigit16}
@indexmacro{Fr_isdigit16}
@Func{@i{macro} int Fr_isdigit16(FrChar16 c)}
Determine whether the specified 16-bit character is a numeric digit.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{Fr_quoteUnicode}
@Func{FrChar16 Fr_quoteUnicode(FrChar16 c)}
Transform the input character in such a manner that it can be stored
into a string of eight-bit characters and be processed correctly.  This
function converts any wide characters for which either byte (or both)
is NUL, Newline, or '|' into a character within the Unicode "private
use" range which does not contain the offending byte value.  Use
@t{Fr_unquoteUnicode} to recover the original 16-bit character.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{Fr_unquoteUnicode}
@Func{FrChar16 Fr_unquoteUnicode(FrChar16 quoted_c)}
Unmangle a previously quoted Unicode character to recover the original value.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{Fr_quoteUnicode}
@Func{char* Fr_quoteUnicode(char *buffer, size_t numchars)}
Transform the input string of wide characters in such a manner that it
can be stored into a string of eight-bit characters and be processed
correctly.  This function converts any wide characters for which either
byte (or both) is NUL, Newline, or '|' into a character within the
Unicode "private use" range which does not contain the offending byte
value.  Use @t{Fr_unquoteUnicode} to recover the original 16-bit
character.

This function destructively modifies the buffer it is given.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{Fr_unquoteUnicode}
@Func{char* Fr_unquoteUnicode(char *buffer, size_t numchars)}
Unmangle a string which has previously been processed by
@t{Fr_quoteUnicode} to recover the original string of Unicode characters.

This function destructively modifies the buffer it is given.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{Fr_quoteUnicode}
@Func{void Fr_quoteUnicode(char *dest, const char *buffer, size_t numchars)}
Copy the input string of wide characters, transforming the individual
characters in such a manner that they can be stored into a string of
eight-bit characters and be processed correctly.  This function
converts any wide characters for which either byte (or both) is NUL,
Newline, or '|' into a character within the Unicode "private use" range
which does not contain the offending byte value.  Use
@t{Fr_unquoteUnicode} to recover the original 16-bit character.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{Fr_unquoteUnicode}
@Func{void Fr_unquoteUnicode(char *dest, const char *buffer, size_t numchars)}
Copy the input string of quoted wide characters, un-transforming the
individual characters to recover the original Unicode characters.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{Fr_ugetc}
@Func{FrChar16 Fr_ugetc(FILE *in, FrBool &byteswap)}
@Func{FrChar16 Fr_ugetc(istream &in, FrBool &byteswap)}
!!!
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{Fr_ugets}
@Func{FrChar16 *Fr_ugets(FILE *in, FrChar16 *buffer, size_t maxline, FrBool &byteswap)}
@Func{FrChar16 *Fr_ugets(istream &in, FrChar16 *buffer, size_t maxline, FrBool &byteswap)}
Read up to @t{maxline}-1 sixteen-bit characters from the input file or
stream and store them in @t{buffer}.  The read ends when a Newline or
NUL character (extended to 16 bits) is encountered on the input or
@t{maxline}-1 wide characters have been read.  A sixteen-bit NUL is
appended to the data read from the input source to terminate the
string.  The parameter @t{byteswap} indicates whether or not
byte-swapping is required; if @t{True}, then the input source is
assumed to be in little-endian byte order (low byte first) rather than
big-endian byte order (high byte first); if a Unicode stream marker is
encountered while reading input, @t{byteswap} is updated to reflect the
actual state of byte swapping on the input source.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{Fr_ustrlen}
@indexfunc{strlen}
@index2{p="strings",s="Unicode"}
@Func{size_t Fr_ustrlen(const FrChar16 *unicode_string)}
Return the number of characters (not bytes!) in the given Unicode
string.  This function is a Unicode equivalent of the standard runtime
function @t{strlen}.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{Fr_ustrchr}
@indexfunc{strchr}
@index2{p="strings",s="Unicode"}
@Func{FrChar16 *Fr_ustrchr(const FrChar16 *unicode_string, FrChar16 unicode_char)}
Search the given string for the desired character, returning a pointer
to the first occurrence of the character within the string, or 0 if
there are none.  This function is a Unicode equivalent of the standard
runtime function @t{strchr}.
@end{FuncDesc}

@subsection{Sixteen-bit Strings}
@index2{p="strings",s="16-bit"}
@index2{p="strings",s="Unicode"}

!!!

@begin{FuncDesc}
@indexfunc{FrCanonicalizeUSentence}
@Func{char *FrCanonicalizeUSentence(const FrChar16 *string, size_t length = 0,
			      FrBool force_uppercase = False,
			      const char *possible_delim /*[256]*/ = 0)}
!!!
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{FrDecanonicalizeUSentence}
@Func{FrString *FrDecanonicalizeUSentence(const char *string,
				    FrBool force_lowercase = False)}
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{FrASCII2Unicode}
@Func{char *FrASCII2Unicode(const char *string, FrBool canonicalize = False)}
@end{FuncDesc}

@begin{FuncDesc}
@index2{p="conversions",s="string to word-list"}
@indexfunc{FrCvtUString2Wordlist}
@Func{FrList *FrCvtUString2Wordlist(const FrChar16 *sentence, size_t length = 0,
			       const char *possible_delim /*[256]*/ = 0)}
!!!
@end{FuncDesc}

@begin{FuncDesc}
@index2{p="conversions",s="string to symbol-list"}
@indexfunc{FrCvtUString2Symbollist}
@Func{FrList *FrCvtUString2Symbollist(const FrChar16 *sentence,
				 const char *possible_delim /*[256]*/ = 0)}
@end{FuncDesc}

FrCvt

@subsection{Byte-Order Independence}
@index{byte order}
@index{byte-swapping}

To aid in interchanging binary data between different hardware
platforms, FramepaC provides the following functions for storing and
retrieving multi-byte values in a byte-order independent fashion.

@begin{FuncDesc}
@indexfunc{FrByteSwap16}
@Func{short FrByteSwap16(short int value)}
Byte-swap a 16-bit value on little-endian machines; on big-endian
machines, this function leaves the value unchanged.  This is useful
for converting a value after loading it from a network-byte-order
buffer or prior to storing a value into such a buffer; in general,
however, it is preferable to use the FrLoad...() and FrStore...()
functions instead of this function, since those functions also ensure
consistent data sizes regardless of architecture.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{FrByteSwap32}
@Func{long int FrByteSwap32(long int value)}
Same as @t{FrByteSwap16}, but for 32-bit values: byte-swap on
little-endian machines, leave @t{value} unchanged on big-endian
machines.  The notes for @t{FrByteSwap16} also apply here.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{FrStoreByte}
@Func{void FrStoreByte(int value, void *buffer)}
Store the low eight bits of the indicated value into the buffer.
(For architectures where the smallest addressable unit [char] is other
than eight bits, this function differs from simply storing the character)
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{FrStoreShort}
@Func{void FrStoreShort(short int value, void *buffer)}
Store the indicated 16-bit value into the buffer in canonical
big-endian byte order.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{FrStoreAlignedShort}
@Func{void FrStoreAlignedShort(short int value, void *buffer)}
Store the indicated short integer into a properly-aligned buffer (for
many architectures, @t{buffer} must be on a two-byte boundary).
This function is faster than @t{FrStoreShort}, but can only be used if
the buffer is known to be properly aligned.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{FrStoreThreebyte}
@Func{void FrStoreThreebyte(UINT32 value, void *buffer)}
Store the indicated 24-bit value into the buffer in canonical
big-endian byte order.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{FrStoreLong}
@Func{void FrStoreLong(UINT32 value, void *buffer)}
Store the indicated 32-bit value into the buffer in canonical
big-endian byte order.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{FrStoreAlignedLong}
@Func{void FrStoreAlignedLong(UINT32 value, void *buffer)}
Store the indicated 32-bit integer into a properly-aligned buffer (for
many architectures, @t{buffer} must be on a four-byte boundary).
This function is faster than @t{FrStoreLong}, but can only be used if
the buffer is known to be properly aligned.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{FrStoreFloat}
@Func{void FrStoreFloat(float value, void *buffer)}
Store the indicated single-precision (32-bit) floating-point value into
the buffer in canonical big-endian byte order.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{FrStoreDouble}
@Func{void FrStoreDouble(double value, void *buffer)}
Store the indicated double-precision (64-bit) floating-point value into
the buffer in canonical big-endian byte order.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{FrLoadByte}
@Func{int FrLoadByte(void *buffer)}
Load the low eight bits of the first character in the indicated buffer.
(For architectures where the smallest addressable unit [char] is other
than eight bits, this function differs from simply loading the character)
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{FrLoadShort}
@Func{short FrLoadShort(const void *buffer)}
Read a 16-bit value from the buffer where it is stored in canonical
big-endian byte order into memory in machine byte order.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{FrLoadAlignedShort}
@Func{int FrLoadAlignedShort(const void *buffer)}
Read a 16-bit value from the buffer where it is stored in canonical
big-endian byte order into memory in machine byte order.  The buffer
must be properly aligned (i.e. on a two-byte boundary) to avoid
generating a bus error or similar fatal exception on many architectures.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{FrLoadThreebyte}
@Func{long FrLoadThreebyte(const void *buffer)}
Read a 24-bit value from the buffer where it is stored in canonical
big-endian byte order into memory in machine byte order.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{FrLoadLong}
@Func{long FrLoadLong(const void *buffer)}
Read a 32-bit value from the buffer where it is stored in canonical
big-endian byte order into memory in machine byte order.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{FrLoadAlignedLong}
@Func{long FrLoadAlignedLong(const void *buffer)}
Read a 32-bit value from the buffer where it is stored in canonical
big-endian byte order into memory in machine byte order.  The buffer
must be properly aligned (i.e. on a four-byte boundary) to avoid
generating a bus error or similar fatal exception on many architectures.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{FrLoadFloat}
@Func{float FrLoadFloat(void *buffer)}
Load the indicated single-precision (32-bit) floating-point value from
the buffer, where it is stored in canonical big-endian byte order.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{FrLoadDouble}
@Func{double FrLoadDouble(void *buffer)}
Load the indicated double-precision (64-bit) floating-point value from
the buffer, where it is stored in canonical big-endian byte order.
@end{FuncDesc}


@subsection{Character Mappings}

@begin{programexample} !!!
char *FrMakeCharacterMap(const FrList *map) ;
void  FrDestroyCharacterMap(char mapping[]) ;
// destructively apply the indicated character mapping to the string
char *FrMapString(char *string, const char mapping[]) ;
@end{programexample}

@comment{-------------------------------}
@section{Signal Handling}
@index{signal handling}
@indextype{FrSignalHandler}

Because various flavors of Unix (not to mention non-Unix operating
systems) provide slightly different variants on signal handling,
FramepaC includes a class which hides the differences behind a
uniform interface -- @t{FrSignalHandler}.

@indextype{FrSignalHandlerFunc}
The typedef @t{FrSignalHandlerFunc} defines the type of the function
which will handle the signal; it accepts an @t{int} argument and
returns nothing.

@begin{FuncDesc}
@indexctor{FrSignalHandler}
@Func{FrSignalHandler::FrSignalHandler(int signal, FrSignalHandlerFunc *handler)}
Create a new instance of the signal handler which will manage the
specified signal (e.g. SIGINT, SIGHUP, etc.), and will call @t{handler}
whenever the signal occurs.  The handler may be 0 or
@t{SIG_IGN} to ignore the signal, or @t{SIG_ERR} to generate an error exception.
@end{FuncDesc}

@begin{FuncDesc}
@indexdtor{FrSignalHandler}
@Func{FrSignalHandler::~FrSignalHandler()}
Restore signal handling for the managed signal to its default state,
and free the resources used by the signal handler.
@end{FuncDesc}

@begin{FuncDesc}
@indextype{FrSignalHandler}
@indexmeth{set}
@Func{FrSignalHandlerFunc *FrSignalHandler::set(FrSignalHandlerFunc *new_handler)}
Specify a new function to be invoked whenever the signal occurs;
returns the previous function.  The new function may be 0 or
@t{SIG_IGN} to ignore the signal, or @t{SIG_ERR} to throw an exception.
A return value of 0 means that the signal was previously ignored.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{raise}
@Func{void FrSignalHandler::raise(int arg) const}
Invoke the signal-handling function with the specified integer
argument, just as if the operating system had triggered the signal.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{signalNumber}
@Func{int FrSignalHandler::signalNumber() const}
Return the number of the signal which the particular instance of
@t{FrSignalHandler} is managing.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{currentHandler}
@Func{FrSignalHandlerFunc *currentHandler() const}
Determine which function will be invoked when the managed signal occurs.
A return value of 0 means that the signal is being ignored.
@end{FuncDesc}


@comment{-------------------------------}
@section{Timed Event Handling}
@index{events}
@index{timed events}
@indextype{FrEvent}
@indextype{FrEventList}

!!!

typedef time_t FrEventFunc(void *client_data) ;

@begin{FuncDesc}
@indexctor{FrEventList}
@Func{FrEventList::FrEventList()}
!!!
@end{FuncDesc}

@begin{FuncDesc}
@indexdtor{FrEventList}
@Func{FrEventList::~FrEventList()}
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{addEvent}
@Func{FrEvent *addEvent(time_t time, FrEventFunc *f, void *client_data, FrBool delta = False)}
Add a new event to the event list.  The new event will be scheduled to
execute at @t{time}, and will call function @t{f} with argument
@t{client_data}.  If @t{delta} is @t{True}, the specified time is an
increment from the current time rather than an absolute time.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{removeEvent}
@Func{FrBool FrEventList::removeEvent(FrEvent *event)}
Remove the specified event from the event list; returns @t{True} if the
event was successfully removed, @t{False} if the event is not on the
event list.

After this method is called and returns @t{True}, @t{event} will no
longer be a valid pointer.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{reschedule}
@Func{void FrEventList::reschedule(FrEvent *event, time_t newtime)}
Change the execution time of the specified event to be @t{newtime}.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{postpone}
@Func{void FrEventList::postpone(FrEvent *event, time_t delta)}
Delay the execution time of the specified event by @t{delta} seconds.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{executeEvents}
@Func{void FrEventList::executeEvents()}
Each time this function is called, any events on the list whose
scheduled time is less than or equal to the current time will be
executed and then removed from the list (unless the event handler
specifies a new time in the future).

This function must be called regularly.
@end{FuncDesc}

@comment{-------------------------------}
@section{Memory-Mapped File Functions}
@index{memory-mapped files}
@index{mmap}

On platforms which support mapping disk files directly into a process'
address space, the following functions provide a simple, consistent
interface to the memory-mapping feature:

@begin{FuncDesc}
@indexfunc{FrMapFile}
@Func{FrFileMapping *FrMapFile(const char *filename, FrMapMode mode)}
Map the specified file into the caller's address space using the access
mode specified by the enumeration @t{mode} (which may be
@t{FrM_READONLY}, @t{FrM_READWRITE}, or @t{FrM_COPYONWRITE}).  Returns
a pointer to an instance of a private class used to track the file
mapping, or 0 if the mapping failed or memory-mapped files are not
supported by the operating system.  If this function returns 0, the
caller must arrange to read in data from the file with usual
file-access functions; for a nonzero return, the next two functions 
indicate which portion of the caller's address space is being used by
the file.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{FrMappedAddress}
@Func{void *FrMappedAddress(const FrFileMapping *fmap)}
Return the starting address of the region of memory into which the
@t{FrMapFile} call producing the indicated @t{FrFileMapping} placed
the file.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{FrMappingSize}
@Func{size_t FrMappingSize(const FrFileMapping *fmap)}
Return the size (in bytes) of the region of memory into which the
@t{FrMapFile} call producing the indicated @t{FrFileMapping} placed
the file.  The memory from @t{FrMappedAddress} to
@t{FrMappedAddress}+@t{FrMappingSize}-1 may safely be accessed; no
access should be attempted outside this range, nor should any writes
be attempted even within the range if the mapping was set up with the
@t{FrM_READONLY} option.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{FrUnmapFile}
@Func{FrBool FrUnmapFile(FrFileMapping *fmap)}
Clean up the specified mapping of file into address space.  Returns
@t{True} if successful and @t{False} if it should for some reason fail.
After a successful @t{FrUnmapFile}, the file will no longer be
accesssible in the caller's address space, and no accesses should be
attempted in the range indicated by @t{FrMappedAddress} and @t{FrMappingSize}.
@end{FuncDesc}


@comment{-------------------------------}
@section{Performance Monitoring}
@indextype{FrTimer}

FramepaC provides the @t{FrTimer} class for measuring how much time has
been spent in an indicated section of the program.  @t{FrTimer} records
the CPU time, rather than "wall-clock" time (except under MS-DOS, where
the two are equivalent anyway) to a resolution -- though not
necessarily accuracy -- of 0.1 milliseconds.

@begin{FuncDesc}
@indexctor{FrTimer}
@Func{FrTimer::FrTimer()}
Create a new timer, and start it running.  The elapsed CPU time may be
retrieved via @t{read}, and is returned when the timer is stopped or
paused.  By default, the elapsed time includes the value of any nested
timers (see @t{FrTimer(FrTimer*)} below), but this may be modified with
@t{includeSubTimers}.
@end{FuncDesc}

@begin{FuncDesc}
@indexctor{FrTimer}
@Func{FrTimer::FrTimer(FrTimer *parent)}
Create a new timer nested within the timing of the indicated parent
timer, and start it running.  If the parent timer's value does not
include any nested timers (see @t{includeSubTimers} below), then the
parent will be paused while this timer is running.  The elapsed CPU
time may be retrieved via @t{read}, and is returned when the timer is
stopped or paused.
@end{FuncDesc}

@begin{FuncDesc}
@indexdtor{FrTimer}
@Func{FrTimer::~FrTimer()}
Destroy the timer, and restart the parent timer (if any) if it was
paused by the running of the deleted timer.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{start}
@Func{void FrTimer::start()}
Start a timer running.  The elapsed time is reset to 0, even if the
timer was already running, and the parent timer (if any) is paused if
it was running and does not include the timings of nested timers.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{stop}
@Func{UINT32 FrTimer::stop()}
Stop a timer running, and return the total elapsed CPU time since it
was last started (not including times during which it was explicitly or
implicitly paused).  The returned value increments at a rate of
@t{FrTICKS_PER_SEC} counts each second.  If the timer has a parent
which was paused for this timer, the parent timer resumes running.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{pause}
@Func{UINT32 FrTimer::pause()}
Temporarily suspend the timer, and return the total elapsed CPU time
since it was last started (not including times during which it was
explicitly or implicitly paused).  The returned value increments at a
rate of @t{FrTICKS_PER_SEC} counts each second.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{resume}
@Func{void FrTimer::resume()}
Restart a previously suspended timer.  If the timer was stopped, this
method is equivalent to @t{start}; if the timer was already running, it
has no effect.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{read}
@Func{UINT32 FrTimer::read()}
Retrieve the current elapsed CPU time during which the timer was
running since it was last started.  The returned value increments at a
rate of @t{FrTICKS_PER_SEC} counts each second.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{includeSubTimers}
@Func{void FrTimer::includeSubTimers(FrBool include)}
Specify whether the timer's run-time value should include the values of
any nested timers (those constructed with this timer as a parent) or
not.  If @t{include} is @t{True} (the default), then any nested timers
are ignored for the purposes of computing the elapsed CPU time.  If
@t{include} is @t{False}, then this timer is suspended whenever a
nested timer is running, and thus records the CPU time @i{only} when no
nested timers are active.  This may be used, for example, to determine
how much time is spent in a particular function without including the
time spent in other functions called by it.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{includesSubTimers}
@Func{FrBool FrTimer::includesSubTimers() const}
Determine whether the reported time for the @t{FrTimer} includes the
elapsed time during which subtimers were running or not.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{isRunning}
@Func{FrBool FrTimer::isRunning() const}
Determine whether the timer is currently running.
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{isPaused}
@Func{FrBool FrTimer::isPaused() const}
Determine whether the timer is currently paused (either explicitly by
@t{pause} or implicitly by a nested timer when
@t{includeSubTimer(False)} has been called).
@end{FuncDesc}

@begin{FuncDesc}
@indexmeth{isStopped}
@Func{FrBool FrTimer::isStopped() const}
Determine whether the timer is currently stopped.
@end{FuncDesc}


@comment{-------------------------------}
@section{Debugging Support}
@index{debugging}

@begin{FuncDesc}
@indexmeth{_}
@Func{void FrObject::_() const}
For convenience in @t{gbd}; this method is equivalent to
@t{FrObject::print(cerr)}.
@end{FuncDesc}

@begin{FuncDesc}
@index{assertions}
@indexfunc{assert}
@Func{assert(condition)}
Test the specified condition (which may be any valid rvalue), and print
an error message showing the source file, line number, and failing
condition if the value is zero.  The program is normally terminated
after the error message is printed (but see @t{FrAssertionFailureFatal}
below).

Like the standard @t{assert}, the FramepaC version is a null macro if
@t{NDEBUG} is defined prior to #include'ing @t{FramepaC.h}.

Source files which use assert() multiple times can save data space by
adding the following lines after #include'ing @t{FramepaC.h} and before
invoking assert() or assertq():
@begin{programexample}
#ifndef NDEBUG
# undef _FrCURRENT_FILE
static const char _FrCURRENT_FILE[] = __FILE__ ;
#endif /* NDEBUG */
@end{programexample}
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{assertq}
@Func{assertq(condition)}
@t{assertq} is the same as @t{assert} except that it does not print the
violated condition if the assertion fails.
@end{FuncDesc}

@begin{FuncDesc}
@index{assertions}
@indexfunc{FrAssertionFailureFatal}
@Func{int FrAssertionFailureFatal(int is_fatal)}
By default, assertion failures (@t{assert} and @t{assertq}) cause the
program to be terminated.  This function allows the program to continue
executing after an assertion failure if @t{is_fatal} is zero.  The
function returns the previous value of the @t{is_fatal} flag.
@end{FuncDesc}


@comment{-------------------------------}
@section{Miscellaneous Functions}

This section covers those function which do not easily fit in any of
the previous sections.

@subsection{Messages}

FramepaC internally uses a number of functions to provide a consistent
method for displaying informative and error messages, which may also be
redirected to other destinations.  When @t{FrInitializeMotif}
has been used, for example, these functions display their messages in a
separate window, which can also be useful for programs incorporating
FramepaC. 

@begin{FuncDesc}
@indexfunc{FrMessage}
@Func{void FrMessage(const char *message)}
Display the provided message on a separate line.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{FrMessageVA}
@Func{void FrMessageVA(const char *message, ...)}
Display the result of applying @t{sprintf} to the provided message and
any additional arguments on a separate line.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{FrWarning}
@Func{void FrWarning(const char *message)}
Display an indication of a warning followed by the given message.  This
function is useful for informing the user of unusual but non-fatal
conditions.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{FrWarningVA}
@Func{void FrWarningVA(const char *message, ...)}
Display an indication of a warning followed by the given message.  The
message is composed by applying @t{sprintf()} to the message string and
any additional arguments given to @t{FrWarningVA}.  This function is
useful for informing the user of unusual but non-fatal conditions.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{FrNoMemory}
@Func{void FrNoMemory(const char *circumstance)}
If the application runs out of memory and is unable to recover (for
example, @t{FrMalloc} returns 0), this message may be used to inform
the user and then terminate the program.  Its parameter is a
description of the circumstance under which the program ran out of
memory, such as "while allocating X" or "in X::Y".  Note that the
caller should be prepared to deal with a possible return from this
function in case the default handler has been overridden.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{FrNoMemoryVA}
@Func{void FrNoMemoryVA(const char *circumstance, ...)}
If the application runs out of memory and is unable to recover (for
example, @t{FrMalloc} returns 0), this message may be used to inform
the user and then terminate the program.  Its parameters are an
@t{sprintf} format string and any additional arguments required by it,
which together form a description of the circumstance under which the
program ran out of memory, such as "while allocating %d bytes for X" or
"in X::Y".  Note that the caller should be prepared to deal with a
possible return from this function in case the default handler has been
overridden.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{FrProgError}
@Func{void FrProgError(const char *message)}
Inform the user that a fatal internal error (such as a missing case in
a @t{switch} statement) has been encountered, and then terminate the
program. Note that the
caller should be prepared to deal with a possible return from this
function in case the display handler has been overridden.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{FrProgErrorVA}
@Func{void FrProgErrorVA(const char *message, ...)}
Inform the user that a fatal internal error (such as a missing case in
a @t{switch} statement) has been encountered, and then terminate the
program.  The error message is generated by applying @t{sprintf} to the
message string and any additional arguments passed to
@t{FrProgErrorVA}.  Note that the caller should be prepared to deal
with a possible return from this function in case the display handler
has been overridden.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{FrError}
@Func{void FrError(const char *message)}
Inform the user than an unrecoverable error (for which none of the
above functions would be applicable) has occurred, and then terminate
the program.  Note that the
caller should be prepared to deal with a possible return from this
function in case the display handler has been overridden.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{FrErrorVA}
@Func{void FrErrorVA(const char *message, ...)}
Inform the user than an unrecoverable error (for which none of the
above functions would be applicable) has occurred, and then terminate
the program.  The text of the error message is generated by applying
@t{sprintf} to the message string and any additional arguments passed
to @t{FrErrorVA}.  Note that the caller should be prepared to deal with
a possible return from this function in case the display handler has
been overridden.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{FrUndefined}
@Func{void FrUndefined(const char *func_name)}
Inform the user that the program has attempted to invoke an as-yet
undefined or unimplemented function.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{FrInvalidVirtualFunction}
@Func{void FrInvalidVirtualFunction(const char *class_and_method)}
Inform the user of the internal error of invoking an invalid virtual
method.  The argument is a string indicating the class name and method,
such as "Foo::bar".
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{FrMissedCase}
@Func{void FrMissedCase(const char *function)}
Inform the user of the programming error of having missed a case in a
@t{switch} statement.  Its argument is the name of the function in
which the switch statement is located.

This function is normally called from a @t{default} case in a switch
statement, i.e.
@begin{programexample}
void foo(int arg)
{
   switch (arg)
     {
     case 0:
         ...
     case 1:
         ...
     default:
         FrMissedCase("foo") ;
     }
@end{programexample}
will invoke the error message whenever @t{foo} is called with an
argument other than 0 or 1.
@end{FuncDesc}

In addition to the wrapper functions listed above, there is a hooking
function for each of the above, which will set the actual function to
be invoked and return the current handler:
@begin{FuncDesc}
@indexfunc{set_message_handler}
@indexfunc{set_warning_handler}
@indexfunc{set_fatal_error_handler}
@indexfunc{set_prog_error_handler}
@indexfunc{set_undef_function_handler}
@indexfunc{set_out_of_memory_handler}
@indexfunc{set_invalid_function_handler}
@indexfunc{set_missed_case_handler}
@Func{FramepaC_error_handler set_message_handler(FramepaC_error_handler)}
@Func{FramepaC_error_handler set_warning_handler(FramepaC_error_handler)}
@Func{FramepaC_error_handler set_out_of_memory_handler(FramepaC_error_handler)}
@Func{FramepaC_error_handler set_prog_error_handler(FramepaC_error_handler)}
@Func{FramepaC_error_handler set_undef_function_handler(FramepaC_error_handler)}
@Func{FramepaC_error_handler set_fatal_error_handler(FramepaC_error_handler)}
@Func{FramepaC_error_handler set_invalid_function_handler(FramepaC_error_handler)}
@Func{FramepaC_error_handler set_missed_case_handler(FramepaC_error_handler)}
@end{FuncDesc}


@subsection{Database}

@begin{FuncDesc}
@indexfunc{database_index_name}
@Func{char *database_index_name(char *database_name, int indexnumber)}
Generate the filename of the indicated index for the database.  The returned
buffer containing the name has been dynamically allocated and must be
explicitly freed with @t{FrFree}.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{VFrames_indexfile}
@index{indexing}
@Func{char *VFrames_indexfile()}
Return the name of the main index file (listing frames by name) for the
current symbol table, or 0 if the current symbol table is not using a
file for backing store.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{VFrames_indexstream}
@index{indexing}
@index{streams}
@Func{fstream *VFrames_indexstream(int indextype)}
Return an open stream referencing the indicated auxiliary index file
for the current symbol table, or 0 if the current symbol table is not
using a file for backing store or the indicated index type is not being
maintained for the symbol table.  The valid index types are
@begin{programexample}
INDEX_INVSLOTS          inverted index by slot and facet names
INDEX_INVFILLERS        inverted index by filler values
INDEX_INVWORDS          index by words within string fillers
@end{programexample}
@end{FuncDesc}


@subsection{Memory Allocation}
@label{memalloc}
@index{memory allocation}

A few of the FramepaC functions return a block of allocated memory which
must be explicitly freed.  This may be done with @t{FrFree}.  If desired,
@t{FrMalloc} may be used to allocate memory instead of @t{new}, particularly
in overridden class-specific @t{new} and @t{delete} operators.  @t{FrMalloc}
can be much faster than @t{new char[]}, especially for small objects
(under 2000 or so bytes, which covers the majority of all allocations).
@t{FrMalloc} has the additional advantage that it minimizes memory
fragmentation conflicting with FramepaC's own memory allocations.  Even
faster allocations for @t{new} are available for uniformly-size objects
with the @b{FrAllocator} class, described in Chapter @ref{memsuballoc}.

To use @t{FrMalloc} for all object allocations, add the following two lines
to the object at the top of each inheritance hierarchy in your program:
@begin{programexample}
void *operator new(size_t size) { return FrMalloc(size) ;}
void operator delete(void *block) { FrFree(block) ;}
@end{programexample}

@begin{FuncDesc}
@indexfunc{FramepaC_gc}
@index{garbage collection}
@index2{p="memory allocation",s="FramepaC_gc"}
@Func{void FramepaC_gc()}
Return as much FramepaC-allocated memory as possible to the system.  If
enabled, FramepaC will also attempt to rearrange its internal memory
allocations to create the largest possible blocks of free memory.  This
function is primarily intended for use in a @t{new_handler} (see
@t{set_new_handler} in your C++ manual) to make additional memory available
to allocations which do not use @t{FrMalloc}.  Unrestrained use of this
function can severely impact performance and fragment memory.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{FrCalloc}
@index2{p="memory allocation",s="FrCalloc"}
@func{void *FrCalloc(size_t nitems, size_t size)}
Allocate an array of @t{nitems} items, each of the indicated size, and return
a pointer to the first item in the array.  Returns 0 if unable to allocate
the requested amount of memory even after compacting free space and optionally
discarding the least-recently used virtual frames.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{FrFree}
@index2{p="memory allocation",s="FrFree"}
@func{void FrFree(void *block)}
Deallocate the indicated block, which was previously obtained from
@t{FrMalloc}.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{FrMalloc}
@index2{p="memory allocation",s="FrMalloc"}
@func{void *FrMalloc(size_t numbytes)}
Allocate a block of @t{numbytes} bytes and return a pointer to its beginning,
or 0 if unable to allocate the requested amount of memory even after compacting
free space.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{FrRealloc}
@index2{p="memory allocation",s="FrRealloc"}
@Func{void *FrRealloc(void *block, size_t newsize, FrBool copydata = True)}
Change the size of the indicated block of memory, and return a pointer to
the resulting memory block (which need not be the same one passed in).  If
a new block must be allocated, @t{copydata} indicates whether or not to copy
the contents of the original block into the new block; if the contents need
not be preserved, you will save time by setting @t{copydata} to @t{False}.

If a new block must be allocated but @t{FrRealloc} is unable to obtain one
of the required size, it returns 0 and leaves the original block unchanged.
If the new allocation was successful, the original block will be freed.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{show_memory_usage}
@index2{p="memory allocation",s="mapping"}
@Func{void show_memory_usage(ostream &out)}
Display a list of the various memory pools currently in use, including
the number of suballocator blocks currently assigned to the pool, the
total number of bytes in those blocks, the number of objects for the
memory pool which are currently in use, and the number of objects on
the pool's free list.  In addition, statistics for the @t{FrMalloc}
memory pool are printed, listing the number of blocks of various sizes
which are currently in use and currently free (as well as the number of
kilobytes in each category and the total number of blocks and kilobytes
allocated/free over all sizes).

@begin{example,size -2}
Memory Usage
============        Blocks         Bytes       Objs Used  Unallocated 
Symbols                 1          16352
FrCons                  1          16352             6      1356
FrInteger               1          16352             3      2041
FrFloat                 1          16352             1      1361
FrFrame                 1          16352             1       193
FrSlot                  1          16352             1      1021
FrString                1          16352             2      1020
FrMalloc                1          16352
============
   Total                8         130816

Malloc blocks:   30    2    0    0    0    0    0    0    0    0    9 =   41
Malloc KB:        0    0    0    0    0    0    0    0    0    0    8 =    8
Free blocks:      0    0    0    0    0    0    0    0    0    0    1 =    1
Free KB:          0    0    0    0    0    0    0    0    0    0    8 =    8

@end{example}
@end{FuncDesc}

The following convenience macros are also available:

@begin{FuncDesc}
@indexmacro{FrNew}
@index2{p="memory allocation",s="FrNew"}
@Func{@t{type}* FrNew(@t{type})}
Allocate an object of the indicated type, and return a pointer to the
newly-allocated object.  Unlike @t{new}, this macro does not execute
any constructors which might be associated with @t{type}.
@end{FuncDesc}

@begin{FuncDesc}
@indexmacro{FrNewC}
@index2{p="memory allocation",s="FrNewC"}
@Func{@t{type}* FrNewC(@t{type},int)}
Allocate an array of objects of the indicated type, clear the allocated
memory to all zero bytes, and return a pointer to the first object in
the newly-allocated array.
@end{FuncDesc}

@begin{FuncDesc}
@indexmacro{FrNewN}
@index2{p="memory allocation",s="FrNewN"}
@Func{@t{type}* FrNewN(@t{type},int)}
Allocate an array of objects of the indicated type, and return a
pointer to the first object in the newly-allocated array.  Unlike
@t{new}, this macro does not execute any constructors which might be
associated with @t{type}.
@end{FuncDesc}

@begin{FuncDesc}
@indexmacro{FrNewR}
@index2{p="memory allocation",s="FrNewR"}
@Func{@t{type}* FrNewR(@t{type},void *blk,int newsize)}
Resize a previously-allocated block of memory to the specified new size
in bytes.  Returns a pointer to the resized block (which may differ
from the original pointer), or 0 if unable to resize the block (in
which case the original remains untouched).
@end{FuncDesc}


@subsection{Motif Interface}
@label{motif}
@index{Motif}

FramepaC was intended for use in an application using the Motif user interface,
and it is thus useful to have support for that interface.

@begin{FuncDesc}
@indexfunc{FrInitializeMotif}
@Func{void FrInitializeMotif(char *window_name, Widget parent, int max_symbols)}
This performs the same function as @t{initialize_FramepaC}, and additionally
prepares FramepaC to use a separate window for any error or warning messages
it may generate (instead of writing them to the Xterm or similar window from
which the application was started).  It is not necessary to use
@t{initialize_FramepaC} when using this function.

If @t{window_name} is 0, FramepaC supplies a default window name.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{FrInitializeMotif}
@Func{Widget FrInitializeMotif(int *orig_argc, char **orig_argv,
                                 const char *maintitle, const char *msgtitle,
                                 const char *icon_name,
                                 FrBool allow_resize = True)}
Initialize both FramepaC and the Motif user interface.  This function
requires both the original @t{argc} and @t{argv} passed to @t{main},
which it will update to remove any Motif-specific commandline
arguments.  The other three strings give the titles for the full
toplevel window, the FramepaC message window, and the application's
icon, respectively.  Finally, @t{allow_resize} indicates whether the
user should be allowed to resize the application's main window using
the X window manager.
@end{FuncDesc}

@begin{FuncDesc}
@indexfunc{FrShutdownMotif}
@Func{void FrShutdownMotif()}
This performs the same function as @t{FrShutdown}, and additionally
cleans up after FramepaC's use of Motif.  It is not necessary to use
@t{FrShutdown} when using this function.
@end{FuncDesc}

@indexfunc{XtMalloc}
@indexfunc{XtCalloc}
@indexfunc{XtRealloc}
@indexfunc{XtFree}
In addition to the above functions, FramepaC also overrides the Xt memory
allocations functions to use its own instead.  Thus, @t{XtMalloc},
@t{XtCalloc}, @t{XtRealloc}, and @t{XtFree} are all integrated into the
FramepaC memory management system and can take advantage of its greater speed,
memory compaction, and automatic frame discarding to free memory.  In order
for the override to work properly, the FramepaC library must be linked in
before the Xt library, i.e. (for Unix) @t{-lframepac} must precede @t{-lXt}
on the linking commandline, and you must use @t{FrInitializeMotif}.

@comment{---------------------------------------------------------------}

@chapter{Preprocessor Symbols}

The following configuration options may be set when FramepaC is
compiled, and may be checked in applications using FramepaC:

@indexpp{FramepaC_Version}
@b{FramepaC_Version}@*
Specifies the version number of FramepaC in use.  The value of this symbol
is 100 times the actual version number, i.e. 90 is version 0.90, 111 is
version 1.11, etc.

@indexpp{FramepaC_Version_string}
@b{FramepaC_Version_string}@*
Specifies the version number of FramepaC as a string, i.e. "1.11".

@indexpp{FrDEMONS}
@b{FrDEMONS}@*
If defined, support for demon functions has been compiled into FramepaC.

@indexpp{FrREPLACE_MALLOC}
@b{FrREPLACE_MALLOC}
If defined, the standard library functions @t{malloc}, @t{calloc},
@t{realloc}, and @t{free} have been overridden to use @t{FrMalloc},
@t{FrCalloc}, @t{FrRealloc}, and @t{FrFree} instead.

@indexpp{FrMEMORY_CHECKS}
@b{FrMEMORY_CHECKS}
If defined, the internal housekeeping variables associated with each
block of memory are checked for consistency on every @t{FrMalloc}, etc.
function call.  If not defined, much less checking is performed, which
slightly improves execution speed but can cause memory to become much
more corrupted before the error is detected.  When defined, FramepaC
can detect many memory overruns and most duplicate deallocations as
soon as the affected memory block is deallocated.

@indexpp{FrEXTRA_INDEXES}
@b{FrEXTRA_INDEXES}
If defined, support for database indexes beyond the simply by-name index
required for retrieving frames on demand has been compiled into FramepaC.

@indexpp{FrLRU_DISCARD}
@b{FrLRU_DISCARD}
If defined, virtual frames will be discarded when memory is exhausted,
beginning with the least-recently accessed frame.  When not defined,
frames will not be discarded automatically, but only when
@t{discard_frame} is called explicitly.  The automatic discarding is an
option because it increases the overhead of virtual frames by the time
required to maintain access-time information.

@indexpp{FrMOTIF}
@b{FrMOTIF}
If defined, support for the Motif user interface has been compiled into
FramepaC.  If not defined, @t{FrInitializeMotif} and
@t{FrShutdownMotif} become identical in operation to
@t{initialize_FramepaC} and @t{FrShutdown}.

@indexpp{FrREPLACE_XTMALLOC}
@b{FrREPLACE_XTMALLOC}
If defined, the standard Xlib memory-allocation functions @t{XtMalloc},
@t{XtRealloc}, and @t{XtFree} have been overridden to use @t{FrMalloc},
etc. instead.

@indexpp{FrSEPARATE_XLIB_ALLOC}
@b{FrSEPARATE_XLIB_ALLOC}
If defined, Xlib memory allocation is performed from a different memory
pool than the one used by @t{FrMalloc}, allowing @t{show_memory_usage}
to distinguish between memory allocated by/for X/Motif functions and
that allocated by/for FramepaC functions.

@indexpp{FrDATABASE}
@b{FrDATABASE}@*
If defined (default), disk-based virtual frames were enabled at
compile-time, and all of the associated functions and database-handling
code are available.

@indexpp{FrSERVER}
@b{FrSERVER}@*
If defined, server-based virtual frames were enabled at compile-time,
and all of the associated functions and network code are available.

@indexpp{FrSYMBOL_VALUE}
@b{FrSYMBOL_VALUE}@*
If defined, each symbol may have an associated value (in Lisp terms,
a binding).

@indexpp{FrLITTLEENDIAN}
@b{FrLITTLEENDIAN}
Defined if the architecture on which FramepaC is running stores
multi-byte values least-significant byte first.

@indexpp{FrBIGENDIAN}
@b{FrBIGENDIAN}
Defined if the architecture on which FramepaC is running is known to store
multi-byte values most-significant byte first.

@indexpp{FrMAX_NUMSTRING_LEN}
@b{FrMAX_NUMSTRING_LEN}@*
This constant indicates the maximum number of characters the FramepaC reader
will process when reading a number.

@indexpp{FrMAX_SYMBOLNAME_LEN}
@b{FrMAX_SYMBOLNAME_LEN}@*
This constant indicates the maximum number of characters the FramepaC reader
will process when reading a symbol's name.

@indexpp{FrMAX_ULONG_STRING}
@b{FrMAX_ULONG_STRING}
This constant indicates the maximum number of characters required to
represent a UINT32 in base 10.

@indexpp{FrMAX_DOUBLE_STRING}
@b{FrMAX_DOUBLE_STRING}
This constant indicates the maximum number of characters required to
represent a @t{double} in base 10.

@indexpp{FrLONG_IS_32BITS}
Defined if the type @t{long} is known to contain exactly 32 bits.


@comment{---------------------------------------------------------------}

@chapter{Troubleshooting}
@label{troubleshooting}
@index{troubleshooting}

@section{Questions and Answers}

@index2{p="troubleshooting",s="unexpected list of NILs"}
@b{Q: Why do I get ((NIL)(NIL)(NIL)(NIL)(NIL)(NIL)(NIL)(NIL)...) as
output instead of the expected list?}

@b{A:} You have freed the list at which your variable pointed.  Unlike
Lisp, with its automatic garbage collection that frees memory only when
it is no longer referenced, FramepaC requires you to manually
deallocate memory.  If you had another pointer to the list, or re-used
the original pointer after freeing the list without first changing it,
you could see output similar to that above, because your pointer will
be pointing somewhere inside FramepaC's internal list of free cells
until FramepaC reuses the list cell to which the pointer is pointing.
One method for avoiding this error is to set the pointer whose list
you free to 0 if you are not immediately assigning a new value to it
for other reasons.

@b{Q: Why is my program's memory usage constantly increasing?}

@b{A:} As above, FramepaC does not perform garbage collection, so it is
up to you to ensure that every object which is created is deallocated
when it is no longer required.  The function
@t{show_memory_usage(ostream&)} can give you an idea of what types of
objects are not being deallocated.  A prime candidate is usually
forgetting to free lists which are the return value of some function
which created the list specifically to be returned to the caller (as
many of FramepaC's functions do).

@b{Q: Why can't my program read objects of certain types?  It gets one
or more symbols and/or lists instead.}

@b{A:} Your program makes no use whatsoever of that particular class of
object, so it has linked in none of the code for the class -- including
the functions to read in objects of that type.  If you really need to
read that type of object, add a dummy global variable of the
appropriate type to one of your program's source files.


@comment{---------------------------------------------------------------}

@chapter{Test/Demo Program}
@index{test program}

The file @t{test.C} in the FramepaC source distribution contains a
simple interactive loop with which you may enter objects and have them
echoed back, or perform a number of commands.  To enter a command
instead of a symbol or other object, type "*" (asterisk) followed by a
space, followed by the command name and any arguments.  The following
commands are supported at this time:

@begin{FuncDesc}
@index2{p="test program",s="commands"}
@index2{p="commands",s="ALL-FRAMES"}
@Func{ALL-FRAMES}
FrList all frames currently defined in the active symbol table.  All of
these frames will have been entered by the user during the current
program run, or have been accessed and retrieved from the backing store.
@end{FuncDesc}

@begin{FuncDesc}
@index2{p="commands",s="ALL-SLOTS"}
@Func{ALL-SLOTS <frame>}
FrList all slots (and their facets) which the named frame is capable of
inheriting.
@end{FuncDesc}

@begin{FuncDesc}
@index2{p="commands",s="ALL-SYMBOLS"}
@Func{ALL-SYMBOLS}
Display a listing of all of the symbols in the current symbol table 
(this can become extremely long if any data files have been loaded).
@end{FuncDesc}

@begin{FuncDesc}
@index2{p="commands",s="BENCH"}
@Func{BENCH}
Display a menu of benchmark tests (described in Chapter @ref{benchmarks}).
@end{FuncDesc}

@begin{FuncDesc}
@index2{p="commands",s="CHECKMEM"}
@index2{p="memory",s="consistency checks"}
@Func{CHECKMEM}
Perform a consistency check on the FramepaC memory chain, and indicate
whether memory is OK or corrupted.
@end{FuncDesc}

@begin{FuncDesc}
@index2{p="commands",s="CLIENT"}
@Func{CLIENT}
Display a menu for testing the client side of the networking code.
@end{FuncDesc}

@begin{FuncDesc}
@index2{p="commands",s="COMPLETE"}
@index{completing frame names}
@index{name completion}
@Func{COMPLETE}
FrList all frames in the current symbol table which have names beginning with
the user-provided string.  Also show the longest common prefix among the
returned frames, which is what a name-completion function could provide if
invoked on the original string.
@end{FuncDesc}

@begin{FuncDesc}
@index2{p="commands",s="DEMONS"}
@Func{DEMONS}
(this command is only available if demon support has been compiled into
FramepaC)
!!!
@end{FuncDesc}

@begin{FuncDesc}
@index2{p="commands",s="EXPORT"}
@index{storing frames}
@index{exporting frames}
@index{FrameKit frames}
@Func{EXPORT}
Store all of the frames in the current symbol table in the user-selected
file in FrameKit format.
@end{FuncDesc}

@begin{FuncDesc}
@index2{p="commands",s="EXPORT-NATIVE"}
@index{storing frames}
@index{exporting frames}
@index{FramepaC frames}
@Func{EXPORT-NATIVE}
Store all of the frames in the current symbol table in the user-selected
file in FramepaC's native format.
@end{FuncDesc}

@begin{FuncDesc}
@index2{p="commands",s="GC"}
@index{garbage collection}
@Func{GC}
Force a memory compaction (as FramepaC does not have true garbage
collection) and return any blocks of memory which could be freed to the
system or the global pool of unallocated memory.
@end{FuncDesc}

@begin{FuncDesc}
@index2{p="commands",s="GENSYM"}
@Func{GENSYM}
Create a new, unique symbol and display it.
@end{FuncDesc}

@begin{FuncDesc}
@index2{p="commands",s="IMPORT"}
@index{importing frames}
@index{FrameKit frames}
@Func{IMPORT}
Load all of the FrameKit-format frames in the user-selected file into the
current symbol table.  This is useful for converting an old ONTOS ontology
into FramepaC format; simply use the following sequence of commands to the
test program:
@begin{programexample}
* bench 8 <dbname> 4
* import <ontosdb>
save
nil 0
@end{programexample}
A 2000-frame ONTOS ontology can be converted into a FramepaC database in
under two minutes in this way.
@end{FuncDesc}

@begin{FuncDesc}
@index2{p="commands",s="INHERIT"}
@Func{INHERIT <frame> <slot>}
Show the fillers for the VALUE, SEM, and DEFAULT facets of
the specified slot, as inherited from any ancestor frames
entered previously in this program run.
@end{FuncDesc}

@begin{FuncDesc}
@index2{p="commands",s="INHERITANCE"}
@Func{INHERITANCE}
Display a menu of the inheritance types, and select a new type.
@end{FuncDesc}

@begin{FuncDesc}
@index2{p="commands",s="IS-A-P"}
@Func{IS-A-P <frame1> <frame2>}
Determine whether the frame named by the symbol @t{frame1} can inherit
from the frame named by the symbol @t{frame2} through a chain of
@t{IS-A} links. 
@end{FuncDesc}

@begin{FuncDesc}
@index2{p="commands",s="LOCK"}
@Func{LOCK}
Display a menu allowing you to lock or unlock frames.
@end{FuncDesc}

@begin{FuncDesc}
@index2{p="commands",s="LOGIN"}
@Func{LOGIN}
Identify yourself as a particular user to the FramepaC access control
system.  You will be prompted for a username and password (which will
both be echoed to the screen).
@end{FuncDesc}

@begin{FuncDesc}
@index2{p="commands",s="MEM"}
@index{memory usage}
Display a table showing how much memory is being used by FramepaC for
various purposes.
@end{FuncDesc}

@begin{FuncDesc}
@index2{p="commands",s="MEMBLOCKS"}
@Func{MEMBLOCKS}
Display a list of the memory blocks under FramepaC's control (this can
become quite long!), showing the size and free/allocated status of each
block.
@end{FuncDesc}

@begin{FuncDesc}
@index2{p="commands",s="NEWUSER"}
@Func{NEWUSER}
Add a new user record to the FramepaC access control system.
@end{FuncDesc}

@begin{FuncDesc}
@index2{p="commands",s="PASSWD"}
@Func{PASSWD}
Change a particular user's password in the FramepaC access control
system.  You will be prompted for the user's name, the old password,
and the new password (which will all be echoed to the screen).
@end{FuncDesc}

@begin{FuncDesc}
@index2{p="commands",s="PART-OF-P"}
@Func{PART-OF-P <frame1> <frame2>}
Determine whether the frame named by the symbol @t{frame1} can inherit
from the frame named by the symbol @t{frame2} through a chain of
@t{PART-OF} links. 
@end{FuncDesc}

@begin{FuncDesc}
@index2{p="commands",s="RELATIONS"}
@Func{RELATIONS}
Display all of the relations for which FramepaC automatically maintains
inverse links.  This command prints a list of two-element lists; each
sublist defines the two slots which are in an inverse relation with
each other.  A slot may be its own inverse.
@end{FuncDesc}

@begin{FuncDesc}
@index2{p="commands",s="RENAME"}
@Func{RENAME}
Change the name of a frame, and update any inverse links pointing at
the frame.  Any other links pointing at the frame's original name will
@b{not} be updated.
@end{FuncDesc}

@begin{FuncDesc}
@index2{p="commands",s="REVERT"}
@Func{REVERT}
For a virtual frame with backing store, revert the frame to a specified
prior version.
@end{FuncDesc}

@begin{FuncDesc}
@index2{p="commands",s="SYMTAB"}
@Func{SYMTAB}
Display a menu of symbol table manipulation options.  You may create,
delete, switch, and list symbol tables.
@end{FuncDesc}

@begin{FuncDesc}
@index2{p="commands",s="TRANSACT"}
@Func{TRANSACT}
Display a menu of database transaction operations.  This allows you to
test, for example, the capability to roll back the backing store after
an error.
@end{FuncDesc}

When not entering one of the above commands, the test program accepts
any type of FramepaC object and echoes it back along with some
pertinent information about the object, such as its type, how many
bytes it takes to print its value, etc.

@comment{---------------------------------------------------------------}

@chapter{Benchmark Results}
@label{benchmarks}

The following benchmark tests are available in the test program.   Each 
allows you to specify a size @t{N} and a number of repetitions (to increase
the total run time for more accurate timing):
@begin{enumerate}
@b{makeSymbol} (Test 1)@*
Call makeSymbol on the string "RELATION" <repetitions> times. 

@b{Creation} (Test 2)@*
Create N frames and then delete all of them (note that the
deletion actually frees the frame record proper, unlike
@t{erase_frame} or @t{free_object(FrFrame*)}, which affect at most
the lists of fillers for the frame).

@b{Relations} (Test 3)@*
Create a "hub" frame, and then create N frames, adding the
hub as a filler for the @t{IS-A}, @t{INSTANCE-OF}, and @t{PART-OF}
slots in each frame; after creating all N frames, delete
them as in Test 2.

@b{Inheritance} (Test 4)@*
Create a top-level frame and then N frames, each of which
@t{IS-A} the previous frame.  Add a filler for the @t{TEST} slot
in the top-level frame, and retrieve the fillers for the
@t{TEST} slot in the bottom-most frame (with inheritance) 100
times.  Finally, delete all frames as in Test 2.

@b{Output Speed} (Tests 5a to 5c)@*
Repeatedly convert each of three objects to a string
containing its printed representation.  The three
objects are a symbol; a list containing a symbol, a
string, a cons of a symbol and a string, a floating
point number, and an integer; and a frame with various
slots filled.

@b{Input Speed} (Tests 6a to 6c)@*
Repeatedly convert each of three objects from their
string representations to their actual values and then
free the created object.  The three objects are the
same as for the previous test, except that the list
contains a sublist rather than a cons (the FramepaC
reader did not handle dotted lists until very recently).

@b{Virtual Frames (memory)} (Tests 7-1 to 7-3)@*
The three tests in this category are identical to
Tests 2 through 4, but use objects of type @t{VFrame}
instead of @t{FrFrame}.  All frames are created and
manipulated entirely in memory, to show the overhead
of using @t{VFrame} instead of @t{FrFrame}.

@b{Virtual Frames (disk)}@*
This item is the same as the previous, except that the frames are stored in
a disk file.  This benchmark was not tested because it is highly dependent
on factors beyond the actual speed of the program, such as the location of
files on disk and the contents of the disk cache.

@b{Virtual Frames (server)}@*
This item is the same as "Virtual Frames (memory)", except that the frames
are stored on a remote machine and accessed through the network interface.
This benchmark also was not tested because of its dependence on outside
factors.

@b{FrameKit/LOOM Benchmarks}@*
A set of benchmarks which are equivalent to a set of benchmarks use to
gauge FrameKit and LOOM speeds.  Note that the original Lisp
implemention of the benchmarks contains some bugs which cause
performance to be overstated on some tests and understated on others.

@b{Memory Allocation Speed} (Tests 11a to 11i)@*
This item causes repeated allocation and deallocation of variously
sized blocks of memory in various orders.  In the execution-time tables
below, Tests 11a to 11c allocate blocks of 20,000 bytes each, Tests 11d
to 11f use blocks of 200 bytes each, and Tests 11g to 11i use blocks of
20 bytes each (which often creates a working set small enough to fit
entirely in the RAM cache).  Tests 11a, 11d, and 11g allocate all
memory blocks, then release them in the same order in which they were
allocated.  Tests 11b, 11e, and 11h allocate all memory blocks, then
release them in the opposite order from which they were allocated.
Finally, Tests 11c, 11f, and 11i allocate and release memory blocks in
random order (freeing any still-allocated blocks once the specified
number of memory blocks has been allocated).
@end{enumerate}

The test machines were
@begin{itemize}
@begin{comment}
an Intel 386 running at 33 MHz, using the MS-DOS operating system and
the Borland C++ v3.1 compiler in large memory model (Table
@ref{intel_times})
@end{comment}

an Intel Pentium running at 90 MHz, using the MS-DOS operating system and
the Borland C++ v3.1 compiler in large memory model
(Table @ref{pentium_times16}) and the Watcom C++32 v10.0a compiler
(32-bit code) in flat memory model (Tables @ref{pentium_times32} and @ref{pentium_times32w}).

a Sun SPARCstation LX running SunOS 4.1.3, using GCC (Table @ref{sparc_times})
@end{itemize}

@begin{comment}
@begin{table,float=top}
@index2{p="benchmarks",s="Intel 386"}
@bar()
@begin{verbatim}
    Test    Size    Repetitions     Total Time      Time/Iteration (ms)
    1       1       50,000           2.03 s             0.041
    2       1000    20               3.35 s           167
    3       1000    20              10.71 s           536
    4 Simp  1000    20              58.30 s          2915
    4 DFS   1000    20              61.81 s          3091
    4 BFS   1000    20             117.75 s          5887
    5a      1       50,000           2.64 s             0.052
    5b*     1       50,000          40.99 s             0.820
    5c      1       50,000          43.13 s             0.863
    6a      1       50,000           7.69 s             0.154
    6b*     1       50,000          60.38 s             1.208
    6c      1       50,000         151.92 s             3.038
    7-1     1000    20               4.29 s           214
    7-2     1000    20              12.03 s           602
    7-3 S   1000    20              59.29 s          2964
    7-3 DF  1000    20              62.80 s          3140
    7-3 BF  1000    20             120.93 s          6047
  --- these timings are very old, and do not reflect current performance ---
@end{verbatim}
@blankspace(0.1in)
[*] Note: The floating point number in the list distorts the timing because
floating point is very slow on this machine due to the lack of a math coprocessor.
@bar()
@blankspace(0.1in)
@caption{Timings for the Intel 386}
@tag{intel_times}
@end{table}

From the Test 2 and Test 3 times for the Intel 386, I infer that
adding a filler to a relation slot and automatically inserting its
inverse takes about 31 microseconds (63 to add and erase).  Similarly,
from the Test 2 and Test 4 times, I infer that an inheritance step
[one get-fillers followed by moving to the parent frame] takes less
than 28 microseconds.)

Overhead for VFrame on the 386: creation 28.1% (47/167), relations
12.3% (66/536), inheritance 1.6% (49/2915).
@end{comment}

@begin{table,float=top}
@index2{p="benchmarks",s="Intel Pentium (16-bit code)"}
@bar()
@begin{verbatim}
    Test    Size    Repetitions     Total Time      Time/Iteration (ms)
    1       1       50,000           0.331 s            0.0066
    2       1000    200              4.152 s           20.8
    3       1000    200             14.109 s           70.5
    4 Simp  1000    40              16.004 s          400.1
    4 DFS   1000    40              18.087 s          452.2
    4 BFS   1000    40              28.869 s          721.7
    5a      1       50,000           0.456 s            0.0091
    5b      1       50,000           2.299 s            0.046
    5c      1       50,000           4.839 s            0.097
    6a      1       50,000           0.998 s            0.020
    6b      1       50,000           6.242 s            0.125
    6c      1       50,000          22.086 s            0.442
    7-1     1000    200              5.458 s           27.3
    7-2     1000    200             15.807 s           79.0
    7-3 S   1000    40              16.253 s          406.3
    7-3 DF  1000    40              19.334 s          483.4
    7-3 BF  1000    40              28.856 s          721.4
-- these timings are very old, and do not reflect current performance --
@end{verbatim}
@bar()
@blankspace(0.1in)
@caption{Timings for Intel Pentium (16-bit code)}
@tag{pentium_times16}
@end{table}

Overhead for VFrame on the Pentium: creation 31.3% (6.5/20.8),
relations 12.1% (8.5/70.5), inheritance 1.5% (6.2/400.1).

@begin{table,float=top}
@index2{p="benchmarks",s="Intel Pentium (Watcom 32-bit code, MS-DOS)"}
@bar()
@begin{verbatim}
    Test    Size    Repetitions    Total Time       Time/Iteration (ms)
    1       1       2,000,000       3.535 s            0.00177
    2       1000    300             1.869 s            6.23
    3       1000    300             4.837 s           16.1
    4 Simp  1000    100            21.698 s          217.0
    4 DFS   1000    100            18.213 s          182.1
    4 BFS   1000    100            21.998 s          220.0
    5a      1       500,000         1.248 s            0.0025
    5b      1       500,000        26.919 s            0.0538
    5c      1       500,000        15.949 s            0.0319
    6a      1       200,000         0.802 s            0.00401
    6b      1       200,000         6.801 s            0.0340
    6c      1       200,000        22.790 s            0.114
    7-1     1000    300             1.930 s            6.43
    7-2     1000    300             5.246 s           17.5
    7-3 S   1000    100            20.498 s          205.0
    7-3 DF  1000    100            16.966 s          169.7
    7-3 BF  1000    100            21.112 s          211.1
    11a     250     1000            0.779 s           0.78 (D4G = 27.062)
    11b     250     1000            0.621 s           0.62 (D4G = 26.869)
    11c     250     1000            0.524 s           0.52 (D4G = 24.907)
    11d     2500    1000            7.109 s           7.11 (D4G = 22.434)
    11e     2500    1000            7.034 s           7.03 (D4G = 22.185)
    11f     2500    1000            6.268 s           6.27 (D4G = 23.812)
    11g     5000    1000            8.728 s           8.73 (D4G = 17.786)
    11h     5000    1000            8.961 s           8.96 (D4G = 17.984)
    11i     5000    1000            9.194 s           9.19 (D4G = 32.314)
@end{verbatim}
@bar()
@blankspace(0.1in)
@caption{Timings for Intel Pentium (Watcom 32-bit code, MS-DOS)}
@tag{pentium_times32}
@end{table}

Overhead for VFrame on the Pentium: creation 2.7% (0.17/6.26),
relations 8.0% (1.3/16.2).  Inheritance is faster for VFrame, which
appears to be an artifact due to a slightly different execution path
when a backing-store is present.

@begin{table,float=top}
@index2{p="benchmarks",s="Intel Pentium (Watcom 32-bit code, Win32)"}
@bar()
@begin{verbatim}
    Test    Size    Repetitions    Total Time       Time/Iteration (ms)
    1       1       2,000,000       3.50  s            0.00175
    2       1000    300             2.06  s            6.87
    3       1000    300             5.56  s           18.5
    4 Simp  1000    100            32.10  s          321.0
    4 DFS   1000    100            25.58  s          255.8
    4 BFS   1000    100            27.56  s          275.6
    5a      1       500,000         1.23  s            0.0025
    5b      1       500,000        26.25  s            0.0525
    5c      1       500,000        16.03  s            0.0321
    6a      1       200,000         0.79  s            0.0040
    6b      1       200,000         7.85  s            0.0393
    6c      1       200,000        21.37  s            0.107
    7-1     1000    300             2.19  s            7.30
    7-2     1000    300             6.47  s           21.57
    7-3 S   1000    100            30.87  s          308.7
    7-3 DF  1000    100            23.06  s          230.6
    7-3 BF  1000    100            25.28  s          252.8
    11a     250     1000           40.68  s          40.68
    11b     250     1000           43.31  s          43.31
    11c     250     1000           19.68  s          19.68
    11d     2500    1000            6.47  s           6.47
    11e     2500    1000            6.60  s           6.60
    11f     2500    1000            6.69  s           6.69
    11g     5000    1000            9.985 s          10.55
    11h     5000    1000           10.461 s          10.32
    11i     5000    1000           10.103 s          10.92
@end{verbatim}
@bar()
@blankspace(0.1in)
@caption{Timings for Intel Pentium (Watcom 32-bit code, Win32)}
@tag{pentium_times32w}
@end{table}

@begin{table,float=top}
@index2{p="benchmarks",s="Intel Pentium (Microsoft 32-bit code)"}
@bar()
@begin{verbatim}
    Test    Size    Repetitions    Total Time       Time/Iteration (ms)
    1       1       2,000,000       3.90  s            0.00195
    2       1000    300             2.20  s            7.33
    3       1000    300             5.77  s           19.2
    4 Simp  1000    100            32.46  s          324.6
    4 DFS   1000    100            23.95  s          239.5
    4 BFS   1000    100            28.67  s          286.7
    5a      1       500,000         1.70  s            0.0034
    5b      1       500,000        40.98  s            0.0820
    5c      1       500,000        15.22  s            0.0304
    6a      1       200,000         0.99  s            0.0049
    6b      1       200,000        13.08  s            0.0654
    6c      1       200,000        18.95  s            0.0948
    7-1     1000    300             2.42  s            8.07
    7-2     1000    300             6.15  s           20.5
    7-3 S   1000    100            25.65  s          256.5
    7-3 DF  1000    100            22.35  s          223.5
    7-3 BF  1000    100            26.20  s          262.0
    11a     250     1000          108.59  s         108.59 (Windows DPMI)
    11b     250     1000           75.80  s          75.80 (Windows DPMI)
    11c     250     1000           59.71  s          59.71 (Windows DPMI)
    11d     2500    1000            5.71  s           5.71
    11e     2500    1000            5.72  s           5.72
    11f     2500    1000            6.15  s           6.15
    11g     5000    1000            9.56  s           9.56
    11h     5000    1000            9.01  s           9.01
    11i     5000    1000           10.05  s          10.05
@end{verbatim}
@bar()
@blankspace(0.1in)
@caption{Timings for Intel Pentium (Microsoft 32-bit code)}
@tag{pentium_times32ms}
@end{table}

@begin{table}
@index2{p="benchmarks",s="SPARCstation LX"}
@bar()
@begin{verbatim}
    Test    Size    Repetitions     Total Time      Time/Iteration (ms)
    1       1       2,000,000        19.06 s            0.0095
    2       1000    300               3.12 s           10.4
            10,000  30                3.53 s
            50,000  6                 3.74 s
           100,000  3                 3.67 s
    3       1000    300              16.58 s           55.3
            10,000  30               16.55 s
            50,000  6                16.92 s
           100,000  3                16.62 s
    4 Sim   1000    100              49.99 s          499.9
    4 DFS   1000    100              91.48 s          914.8
    4 BFS   1000    100              74.94 s          749.4
    5a      1       1,000,000         5.45 s            0.0055
    5b*     1       200,000          94.35 s            0.472
    5c      1       200,000          19.75 s            0.099
    6a      1       200,000           3.21 s            0.0161
    6b*     1       200,000         118.31 s            0.592
    6c      1       200,000         129.81 s            0.649
    7-1     1000    300               4.75 s           15.8
    7-2     1000    300              18.39 s           61.3
    7-3 S   1000    100              47.94 s          479.4
    7-3 DFS 1000    100              78.57 s          785.7
    7-3 BFS 1000    100              73.12 s          731.2
    11a     250     1000              2.28 s            2.28 (GCC: 2.83)
    11b     250     1000              2.32 s            2.32 (GCC: 2.46)
    11c     250     1000              3.36 s            3.36 (GCC: 5.95)
    11d     2500    1000             15.33 s           15.33 (GCC: 24.44)
    11e     2500    1000             14.62 s           14.62 (GCC: 22.62)
    11f     2500    1000             22.01 s           22.01 (GCC: 60.67)
    11g     5000    1000             28.96 s           28.96 (GCC: 52.69)
    11h     5000    1000             29.62 s           29.62 (GCC: 44.98)
    11i     5000    1000             42.33 s           42.33 (GCC: 124.31)
@end{verbatim}
@blankspace(0.1in)
[*] Note: The floating point number in the list distorts the timing because
floating point is very slow on this machine due to the lack of a math coprocessor.
@bar()
@blankspace(0.1in)
@caption{Timings for SPARCstation LX}
@tag{sparc_times}
@end{table}

Overhead for VFrame on the Sparc: creation 35%, relations 5.8%.
Inheritance is faster for VFrame, which appears to be an artifact due
to a slightly different execution path when a backing-store is present.

@comment{---------------------------------------------------------------}

@chapter{Compiling and Installing FramepaC}

Installing FramepaC consists of the following general steps:
@begin{enumerate}
Obtain the FramepaC source code distribution

Create directories for the source code and compiled library

Unpack (if necessary) and copy the source code into the source code
directory

Edit the Makefile to suit your environment

Edit @b{frconfig.h}, @b{frpcglbl.h}, and @b{frmem.h} to suit your needs
and environment.  Usually only @b{frconfig.h} will require changes.

Type 'make install' to build and install the FramepaC library; 'make all'
to just build the library and the FramepaC test program.

(optional) Type 'make clean' to remove unnecessary files created during
compilation.
@end{enumerate}

!!!

The Makefile contains a number of customization options at the
beginning.  The main options you will need to set are the locations of
various directories and the compiler to use (there are various subfiles
for different compilers and target environments; select the appropriate
one).  You may also wish to inspect the selected sub-makefile, as these
also contain settable options.

The main configuration options in @b{frconfig.h} are whether to
associate a value with each symbol, whether to enable 'sideways
inheritance', whether to include support for using a separate server
and/or a disk database for backing store on virtual frames, and whether
to completely replace the standard memory allocation functions.
Secondary options specify the limits on various string sizes, such as
the maximum length numbers require when printed out.

The final step, 'make install', creates an archive file @b{framepac.a}
(or @b{framepac.lib}) which may be linked with your program.  It and
all header files needed to use FramepaC are copied into the specified
installation directory.

@section{Portability}

FramepaC was jointly developed under both MS-DOS on an Intel 386 (and
later a Pentium) and SunOS Unix on a Sun SPARCstation.  Since these
machines differ in word size, endianness, and numerous other respects,
the code should be portable to most systems with few, if any, problems.

@comment{---------------------------------------------------------------}

@chapter{Bibliography}

@bibliography{}


@comment[
</pre>
<hr>
[<a href="http://www.cs.cmu.edu/afs/cs.cmu.edu/user/ralf/pub/WWW/ralf-home.html">Home Page</a>]
<br>
(Last changed 26-Nov-96)
</body>
</html>
]
@comment{---END OF FILE---END OF FILE---}
