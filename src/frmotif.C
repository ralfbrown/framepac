/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frmotif.cpp	    Motif user-interface code for FramepaC	*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,1998,2006,2009,2011,2015		*/
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

#include "frconfig.h"

#ifdef FrMOTIF
#include <new.h>
#include <stdio.h>
#include <sys/stat.h>
#include <iostream.h>
#include <Xm/Xm.h>
#include <Xm/ArrowB.h>
#include <Xm/ArrowBG.h>
#include <Xm/CascadeB.h>
#include <Xm/DialogS.h>
#include <Xm/Frame.h>
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/List.h>
#include <Xm/MainW.h>
#include <Xm/MessageB.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/RowColumn.h>
#include <Xm/Scale.h>
#include <Xm/ScrollBar.h>
#include <Xm/ScrolledW.h>
#include <Xm/SelectioB.h>
#include <Xm/Separator.h>
#include <Xm/Text.h>
#include <Xm/ToggleB.h>
#include <Xm/XmStrDefs.h>
#include "framerr.h"
#include "frctype.h"
#include "frhelp.h"
#include "frlist.h"
#include "frmem.h"
#include "frreader.h"
#include "frstring.h"
#include "frsymtab.h"
#include "vframe.h"
#endif /* FrMOTIF */
#include "frmotif.h"

extern const char **FramepaC_helptexts[] ;

void initialize_FramepaC(int max_symbols) ;
void shutdown_FramepaC() ;

/************************************************************************/
/*    Manifest constants						*/
/************************************************************************/

#define KILL_X_OFFSET 50
#define KILL_Y_OFFSET 25

// maximum size of a scrolled-text window
#define MAX_HEIGHT 22
#define MAX_WIDTH  80

#define MAX_CONTINUATIONS 15
#define MAX_COMPLETIONS 30

#define CONTINUATION_MARKER " ..."

#ifndef XmNfrom
#define XmNfrom "from"
#endif

#ifndef XmNto
#define XmNto "to"
#endif

/************************************************************************/
/*    Global variables shared with other modules in FramepaC		*/
/************************************************************************/

#ifdef FrMOTIF

extern XtAppContext app_context ;
extern Widget toplevel ;

/************************************************************************/
/*    Global variables imported from elsewhere				*/
/************************************************************************/

extern void (*_XErrorFunction)(const char *msg) ;
extern void (*_XIOErrorFunction)(const char *msg) ;

/************************************************************************/
/*    Global variables limited to this module				*/
/************************************************************************/

static const char str_FrWidget[] = "FrWidget" ;

static FramepaC_error_handler message_handler = 0 ;
static FramepaC_error_handler warning_handler = 0 ;
static FramepaC_error_handler fatal_error_handler = 0 ;
static FramepaC_error_handler out_of_memory_handler = 0 ;
static FramepaC_error_handler prog_error_handler = 0 ;
static FramepaC_error_handler undef_function_handler = 0 ;

static XtErrorHandler Xt_warning_handler = 0 ;
static XtErrorHandler Xt_error_handler = 0 ;

static FrKillProgHookFunc *killprog_hook = 0 ;
static const char *fatal_error_message ;

static Widget ancestor ;

static Widget fpshell = 0 ;
static Widget msg_window ;
//static int max_messages = 60 ;

/************************************************************************/
/*	Global data for this module	  				*/
/************************************************************************/

// allow the Tab key to be used to complete a frame name
static const char completion_translations[] =
	"#override\n"
	" <Key>Tab: self-insert() \n" ;

// allow use of right mouse button
static const char blockPBTranslationTable2[]=
  "#override\n"
  " <Btn3Down>: Arm() \n"
  " <Btn3Up> : Activate() Disarm() \n"
  " <Btn3Down>, <Btn3Up> : Activate() Disarm()" ;

// allow use of middle and right mouse buttons
static const char blockPBTranslationTable3[]=
   "#override\n"
   " <Btn2Down>: Arm() \n"
   " <Btn2Up> : Activate() Disarm() \n"
   " <Btn2Down>, <Btn2Up> : Activate() Disarm() \n"
   " <Btn3Down>: Arm() \n"
   " <Btn3Up> : Activate() Disarm() \n"
   " <Btn3Down>, <Btn3Up> : Activate() Disarm()" ;

static const char str_label[] = "label" ;
static const char str_prompt[] = "prompt" ;
static const char str_Prompt[] = "Prompt" ;
static const char str_pushbutton[] = "pushbutton" ;
static const char str_radiobox[] = "radiobox" ;
static const char str_scrollbar[] = "scrollbar" ;
static const char str_scrollwindow[] = "scrollwindow" ;
static const char str_togglebutton[] = "togglebutton" ;
static const char str_Progress[] = "Progress" ;
static const char str_ProgressIndicator[] = "ProgressIndicator" ;
static const char str_Progress_Indicator[] = "Progress Indicator" ;

static char str_arrow[] = "arrow" ; 	// must be non-const
static char str_option[] = "option" ;	// must be non-const
static char str_warning[] = "warning" ; // must be non-const

static const char str_Ok[] = "Ok" ;
static const char str_Cancel[] = "Cancel" ;
static const char str_Help[] = "Help" ;

/************************************************************************/
/*	forward declarations			   			*/
/************************************************************************/

static void frame_completion_cb(Widget w, XtPointer client_data,
				XtPointer call_data) ;
static Widget create_pushbutton(Widget parent, const char *name, bool managed,
				bool centered, const char *pbclass) ;
static Widget create_pulldown_menu(Widget parent, const char *name,
				   bool managed) ;

/************************************************************************/
/*	general functions			   			*/
/************************************************************************/

static void DestroyWindow(Widget, XtPointer window, XtPointer)
{
   XtDestroyWidget((Widget)window) ;
}

//----------------------------------------------------------------------

void FrSoundBell(FrWidget *w, int amplitude)
{
   XBell(XtDisplay(**w), amplitude) ;
}
//----------------------------------------------------------------------

void FrSoundBell(Widget w, int amplitude)
{
   XBell(XtDisplay(w), amplitude) ;
}

//----------------------------------------------------------------------

static void set_icon_name(Widget w, const char *icon_name)
{
   Arg args[1] ;
   XtSetArg(args[0], XmNiconName, icon_name) ;
   XtSetValues(w, args, 1) ;
}

//----------------------------------------------------------------------

Widget create_warning_popup(Widget w, const char *message)
{
   Arg args[4] ;
   int arg_count = 0;
   XmString msg = XmStringCreateLtoR((char*)message,XmSTRING_DEFAULT_CHARSET) ;
   XtSetArg(args[arg_count], XmNmessageString, msg) ; arg_count++ ;
   XtSetArg(args[arg_count], XmNautoUnmanage, true) ; arg_count++ ;
   XtSetArg(args[arg_count], XmNiconName, "Warning") ; arg_count++ ;
   Widget dialog = XmCreateWarningDialog(w, str_warning, args, arg_count);
   XmStringFree(msg) ;
   XtUnmanageChild(XmMessageBoxGetChild(dialog, XmDIALOG_HELP_BUTTON));
   XtUnmanageChild(XmMessageBoxGetChild(dialog, XmDIALOG_CANCEL_BUTTON));
   XtManageChildren(&dialog,1);
   FrWidget button(XmMessageBoxGetChild(dialog, XmDIALOG_OK_BUTTON)) ;
   int x, y ;
   button.setManaged(true) ;
   button.getPosition(&x,&y) ;
   button.retain() ;
   FrWidget dlg(dialog) ;
   dlg.setManaged(true) ;
   dlg.warpPointer(x+5,y+5) ;  // move pointer into OK button
   dlg.retain() ;
   return dialog ;
}

//----------------------------------------------------------------------

void remove_warning_popup(Widget w)
{
   XtDestroyWidget(w) ;
}

/************************************************************************/
/*	 Methods for class FrWidget 					*/
/************************************************************************/

FrAllocator FrWidget::allocator(str_FrWidget,sizeof(FrWidget)) ;

//----------------------------------------------------------------------

FrWidget::~FrWidget()
{
   destroy() ;
}

//----------------------------------------------------------------------

FrObjectType FrWidget::objType() const
{
   return (FrObjectType)OT_FrWidget ;
}

//----------------------------------------------------------------------

const char *FrWidget::objTypeName() const
{
   return str_FrWidget ;
}

//----------------------------------------------------------------------

FrObjectType FrWidget::objSuperclass() const
{
   return OT_FrObject ;
}

//----------------------------------------------------------------------

ostream &FrWidget::printValue(ostream &output) const
{
   return output << objTypeName() << "<" << (long int)widget << ">" ;
}

//----------------------------------------------------------------------

size_t FrWidget::displayLength() const
{
   return strlen(objTypeName()) + 2 + Fr_number_length((int)widget) ;
}

//----------------------------------------------------------------------

char *FrWidget::displayValue(char *buffer) const
{
   Fr_sprintf(buffer,200,"%s<%lu>%c",objTypeName(),(long int)widget,'\0') ;
   return strchr(buffer,'\0')-1 ;
}

//----------------------------------------------------------------------

bool FrWidget::widgetp() const
{
   return true ;
}

//----------------------------------------------------------------------

void FrWidget::mapWhenManaged(bool do_map) const
{
   XtSetMappedWhenManaged(widget,do_map ? 1 : 0) ;
}

//----------------------------------------------------------------------

void FrWidget::setOrientation(int orientation) const
{
   switch (orientation)
      {
      case ArrowUp:
	 orientation = XmARROW_UP ;
	 break ;
      case ArrowDown:
	 orientation = XmARROW_DOWN ;
	 break ;
      case ArrowLeft:
	 orientation = XmARROW_LEFT ;
	 break ;
      case ArrowRight:
	 orientation = XmARROW_RIGHT ;
	 break ;
      default:
	 return ;
      }
   Arg args[1] ;
   XtSetArg(args[0], XmNarrowDirection, orientation) ;
   XtSetValues(widget, args, 1) ;
}

//----------------------------------------------------------------------

void FrWidget::setVertical(bool vertical) const
{
   Arg args[1] ;
   XtSetArg(args[0], XmNorientation, vertical ? XmVERTICAL : XmHORIZONTAL) ;
   XtSetValues(widget, args, 1) ;
}

//----------------------------------------------------------------------

void FrWidget::setAlignment(int align) const
{
   Arg args[1] ;
   int a ;
   if (align < 0)
      a = XmALIGNMENT_BEGINNING ;
   else if (align > 0)
      a = XmALIGNMENT_END ;
   else // align == 0
      a = XmALIGNMENT_CENTER ;
   XtSetArg(args[0], XmNalignment, (XtPointer)a) ;
   XtSetValues(widget, args, 1) ;
}

//----------------------------------------------------------------------

void FrWidget::setEnds(Widget from, Widget to) const
{
   Arg args[2] ;
   XtSetArg(args[0], XmNfrom, from) ;
   XtSetArg(args[1], XmNto,   to) ;
   XtSetValues(widget, args, 2) ;
}

//----------------------------------------------------------------------

void FrWidget::setTitle(const char *title) const
{
  Arg args[1] ;
  XtSetArg(args[0], XmNtitle, title) ;
  XtSetValues(widget, args, 1) ;
}

//----------------------------------------------------------------------

void FrWidget::setIconName(const char *title) const
{
  Arg args[1] ;
  XtSetArg(args[0], XmNiconName, title) ;
  XtSetValues(widget, args, 1) ;
}

//----------------------------------------------------------------------

void FrWidget::setValue(XtPointer value) const
{
   Arg args[1] ;
   XtSetArg(args[0], XmNvalue, value) ;
   XtSetValues(widget, args, 1) ;
}

//----------------------------------------------------------------------

XtPointer FrWidget::getValue() const
{
   Arg args[1] ;
   XtPointer result ;
   XtSetArg(args[0], XmNvalue, &result) ;
   XtGetValues(widget,args,1) ;
   return result ;
}

//----------------------------------------------------------------------

void FrWidget::setUserData(XtPointer data) const
{
   if (widget)
      {
      Arg args[1] ;
      XtSetArg(args[0], XmNuserData, data) ;
      XtSetValues(widget, args, 1) ;
      }
}

//----------------------------------------------------------------------

XtPointer FrWidget::getUserData() const
{
   if (widget)
      {
      XtPointer result ;
      Arg args[1] ;
      XtSetArg(args[0], XmNuserData, &result) ;
      XtGetValues(widget, args, 1) ;
      return result ;
      }
   else
      return 0 ;
}

//----------------------------------------------------------------------

void FrWidget::setLabel(const char *label) const
{
   XmString labelstr = XmStringCreateSimple((char*)label) ;
   Arg args[1] ;
   XtSetArg(args[0], XmNlabelString, labelstr) ;
   XtSetValues(widget, args, 1) ;
   XmStringFree(labelstr) ;
}

//----------------------------------------------------------------------

void FrWidget::setTextString(const char *text) const
{
   XmString textstr = XmStringCreateSimple((char*)text) ;
   Arg args[1] ;
   XtSetArg(args[0], XmNtextString, textstr) ;
   XtSetValues(widget, args, 1) ;
   XmStringFree(textstr) ;
}

//----------------------------------------------------------------------

char *FrWidget::getLabel() const
{
   XmString labelstr ;
   Arg args[1] ;
   XtSetArg(args[0], XmNlabelString, &labelstr) ;
   XtGetValues(widget, args, 1) ;
   char *label ;
   Boolean match = XmStringGetLtoR(labelstr, XmSTRING_DEFAULT_CHARSET, &label);
   XmStringFree(labelstr) ;
   return match ? label : 0 ;
}

//----------------------------------------------------------------------

void FrWidget::setShadow(int thickness) const
{
   Arg args[1] ;
   XtSetArg(args[0], XmNshadowThickness, thickness) ;
   XtSetValues(widget, args, 1) ;
}

//----------------------------------------------------------------------

void FrWidget::setSensitive(bool sensitive) const
{
   XtSetSensitive(widget,sensitive) ;
}

//----------------------------------------------------------------------

void FrWidget::allowResize(bool state) const
{
   Arg args[1] ;
   XtSetArg(args[0], XmNallowResize, state) ;
   XtSetValues(widget, args, 1) ;
}

//----------------------------------------------------------------------

void FrWidget::invertVideo() const
{
   Arg args[2] ;
   XtPointer f, b ;
   XtSetArg(args[0], XmNforeground, &f) ;
   XtSetArg(args[1], XmNbackground, &b) ;
   XtGetValues(widget, args, 2) ;
   XtSetArg(args[0], XmNforeground, b) ;
   XtSetArg(args[1], XmNbackground, f) ;
   XtSetValues(widget, args, 2) ;
}

//----------------------------------------------------------------------

void FrWidget::setTraversal(bool state) const
{
   Arg args[1] ;
   XtSetArg(args[0], XmNtraversalOn, state) ;
   XtSetValues(widget, args, 1) ;
}

//----------------------------------------------------------------------

void FrWidget::traverseCurrent() const
{
   XmProcessTraversal(widget,XmTRAVERSE_CURRENT) ;
   // Motif often has to be told twice for Traverse-Current to 'take'
   XmProcessTraversal(widget,XmTRAVERSE_CURRENT) ;
}

//----------------------------------------------------------------------

void FrWidget::traverseNext() const
{
   XmProcessTraversal(widget,XmTRAVERSE_NEXT) ;
}

//----------------------------------------------------------------------

void FrWidget::traversePrev() const
{
   XmProcessTraversal(widget,XmTRAVERSE_PREV) ;
}

//----------------------------------------------------------------------

void FrWidget::traverseNextGroup() const
{
   XmProcessTraversal(widget,XmTRAVERSE_NEXT_TAB_GROUP) ;

}

//----------------------------------------------------------------------

void FrWidget::traversePrevGroup() const
{
   XmProcessTraversal(widget,XmTRAVERSE_PREV_TAB_GROUP) ;
}

//----------------------------------------------------------------------

void FrWidget::traverseUp() const
{
   XmProcessTraversal(widget,XmTRAVERSE_UP) ;
}

//----------------------------------------------------------------------

void FrWidget::traverseDown() const
{
   XmProcessTraversal(widget,XmTRAVERSE_DOWN) ;
}

//----------------------------------------------------------------------

void FrWidget::navigationTabGroup() const
{
   Arg args[1] ;
   XtSetArg(args[0], XmNnavigationType, XmTAB_GROUP) ;
   XtSetValues(widget, args, 1) ;
}

//----------------------------------------------------------------------

void FrWidget::removeCallback(char *type, XtCallbackProc cb_func,
			      XtPointer cb_data) const
{
   XtRemoveCallback(widget,type,cb_func,cb_data) ;
}

//----------------------------------------------------------------------

void FrWidget::armCallback(XtCallbackProc cb_func, XtPointer cb_data,
			   bool remove)
{
   if (widget)
      {
      if (remove)
         XtRemoveAllCallbacks(widget,XmNarmCallback) ;
      if (cb_func)
	 XtAddCallback(widget,XmNarmCallback,cb_func,cb_data) ;
      }
}

//----------------------------------------------------------------------

void FrWidget::disarmCallback(XtCallbackProc cb_func, XtPointer cb_data,
			      bool remove)
{
   if (widget)
      {
      if (remove)
         XtRemoveAllCallbacks(widget,XmNdisarmCallback) ;
      if (cb_func)
	 XtAddCallback(widget,XmNdisarmCallback,cb_func,cb_data) ;
      }
}

//----------------------------------------------------------------------

void FrWidget::activateCallback(XtCallbackProc cb_func, XtPointer cb_data,
				bool remove)
{
   if (widget)
      {
      if (remove)
         XtRemoveAllCallbacks(widget,XmNactivateCallback) ;
      if (cb_func)
	 XtAddCallback(widget,XmNactivateCallback,cb_func,cb_data) ;
      }
}

//----------------------------------------------------------------------

void FrWidget::activateCallback(XtCallbackProc cb_func)
{
   if (widget && cb_func)
      XtAddCallback(widget,XmNactivateCallback,cb_func,this) ;
}

//----------------------------------------------------------------------

void FrWidget::okCallback(XtCallbackProc cb_func, XtPointer cb_data,
			  bool remove)
{
   if (widget)
      {
      if (remove)
         XtRemoveAllCallbacks(widget,XmNokCallback) ;
      if (cb_func)
	 XtAddCallback(widget,XmNokCallback,cb_func,cb_data) ;
      }
}

//----------------------------------------------------------------------

void FrWidget::cancelCallback(XtCallbackProc cb_func, XtPointer cb_data,
			      bool remove)
{
   if (widget)
      {
      if (remove)
         XtRemoveAllCallbacks(widget,XmNcancelCallback) ;
      if (cb_func)
	 XtAddCallback(widget,XmNcancelCallback,cb_func,cb_data) ;
      }
}

//----------------------------------------------------------------------

void FrWidget::helpCallback(XtCallbackProc cb_func, XtPointer cb_data,
			    bool remove)
{
   if (widget)
      {
      if (remove)
         XtRemoveAllCallbacks(widget,XmNhelpCallback) ;
      if (cb_func)
	 XtAddCallback(widget,XmNhelpCallback,cb_func,cb_data) ;
      }
}

//----------------------------------------------------------------------

void FrWidget::nomatchCallback(XtCallbackProc cb_func, XtPointer cb_data,
			       bool remove)
{
   if (widget)
      {
      if (remove)
         XtRemoveAllCallbacks(widget,XmNnoMatchCallback) ;
      if (cb_func)
	 XtAddCallback(widget,XmNnoMatchCallback,cb_func,cb_data) ;
      }
}

//----------------------------------------------------------------------

void FrWidget::changedCallback(XtCallbackProc cb_func, XtPointer cb_data,
			       bool remove)
{
   if (widget)
      {
      if (remove)
         XtRemoveAllCallbacks(widget,XmNvalueChangedCallback) ;
      if (cb_func)
	 XtAddCallback(widget,XmNvalueChangedCallback,cb_func,cb_data) ;
      }
}

//----------------------------------------------------------------------

void FrWidget::verifyCallback(XtCallbackProc cb_func, XtPointer cb_data,
			      bool remove)
{
   if (widget)
      {
      if (remove)
         XtRemoveAllCallbacks(widget,XmNmodifyVerifyCallback) ;
      if (cb_func)
	 XtAddCallback(widget,XmNmodifyVerifyCallback,cb_func,cb_data) ;
      }
}

//----------------------------------------------------------------------

void FrWidget::focusCallback(XtCallbackProc cb_func, XtPointer cb_data,
			     bool remove)
{
   if (widget)
      {
      if (remove)
         XtRemoveAllCallbacks(widget,XmNfocusCallback) ;
      if (cb_func)
	 XtAddCallback(widget,XmNfocusCallback,cb_func,cb_data) ;
      }
}

//----------------------------------------------------------------------

void FrWidget::loseFocusCallback(XtCallbackProc cb_func, XtPointer cb_data,
			     bool remove)
{
   if (widget)
      {
      if (remove)
         XtRemoveAllCallbacks(widget,XmNlosingFocusCallback) ;
      if (cb_func)
	 XtAddCallback(widget,XmNlosingFocusCallback,cb_func,cb_data) ;
      }
}

//----------------------------------------------------------------------

void FrWidget::resizeCallback(XtCallbackProc cb_func, XtPointer cb_data,
			      bool remove)
{
   if (widget)
      {
      if (remove)
         XtRemoveAllCallbacks(widget,XmNresizeCallback) ;
      if (cb_func)
	 XtAddCallback(widget,XmNresizeCallback,cb_func,cb_data) ;
      }
}

//----------------------------------------------------------------------

void FrWidget::destroyCallback(XtCallbackProc cb_func, bool remove)
{
   if (widget)
      {
      if (remove)
         XtRemoveAllCallbacks(widget,XmNdestroyCallback) ;
      if (cb_func)
	 XtAddCallback(widget,XmNdestroyCallback,cb_func,this) ;
      }
}

//----------------------------------------------------------------------

static void destroy_cb(Widget, XtPointer client_data, XtPointer)
{
   FrWidget *w = (FrWidget*)client_data ;
   if (w)
      {
      w->keep() ; // delete the FrWidget without re-destroying widget
      }
}

//----------------------------------------------------------------------

void FrWidget::destroyCallback(bool remove)
{
   if (widget)
      {
      if (remove)
         XtRemoveAllCallbacks(widget,XmNdestroyCallback) ;
      XtAddCallback(widget,XmNdestroyCallback,destroy_cb,this) ;
      }
}

//----------------------------------------------------------------------

void FrWidget::setFont(const char *fontname)
{
   Arg args[1] ;
   XmFontList fontlist ;
   static char *prevfontname = 0 ;
   static XmFontList prevfontlist = 0 ;

   if (!prevfontname || strcmp(fontname,prevfontname) != 0)
      {
      XFontStruct *font = XLoadQueryFont(XtDisplay(widget),(char*)fontname) ;
      fontlist = XmFontListCreate(font,XmSTRING_DEFAULT_CHARSET) ;
      FrFree(prevfontname) ;
      prevfontname = FrDupString(fontname) ;
      prevfontlist = fontlist ;
      }
   else
      fontlist = prevfontlist ;
   XtSetArg(args[0],XmNfontList,fontlist) ;
   XtSetValues(widget,args,1) ;
}

//----------------------------------------------------------------------

static void attach_widget(Widget w,Widget top,Widget bottom,Widget left,
			  Widget right)
{
   Arg args[10] ;
   int n = 0 ;

   if (top == Attach_to_Form)
      {
      XtSetArg(args[n],XmNtopAttachment,XmATTACH_FORM) ; n++ ;
      }
   else if (top == Attach_to_Self)
      {
      XtSetArg(args[n],XmNtopAttachment,XmATTACH_SELF) ; n++ ;
      }
   else if (top)
      {
      XtSetArg(args[n],XmNtopAttachment,XmATTACH_WIDGET) ; n++ ;
      XtSetArg(args[n],XmNtopWidget, top) ; n++ ;
      }
   if (bottom == Attach_to_Form)
      {
      XtSetArg(args[n],XmNbottomAttachment,XmATTACH_FORM) ; n++ ;
      }
   else if (bottom == Attach_to_Self)
      {
      XtSetArg(args[n],XmNbottomAttachment,XmATTACH_SELF) ; n++ ;
      }
   else if (bottom)
      {
      XtSetArg(args[n],XmNbottomAttachment,XmATTACH_WIDGET) ; n++ ;
      XtSetArg(args[n],XmNbottomWidget, bottom) ; n++ ;
      }
   if (left == Attach_to_Form)
      {
      XtSetArg(args[n],XmNleftAttachment,XmATTACH_FORM) ; n++ ;
      }
   else if (left == Attach_to_Self)
      {
      XtSetArg(args[n],XmNleftAttachment,XmATTACH_SELF) ; n++ ;
      }
   else if (left)
      {
      XtSetArg(args[n],XmNleftAttachment,XmATTACH_WIDGET) ; n++ ;
      XtSetArg(args[n],XmNleftWidget, left) ; n++ ;
      }
   if (right == Attach_to_Form)
      {
      XtSetArg(args[n],XmNrightAttachment,XmATTACH_FORM) ; n++ ;
      }
   else if (right == Attach_to_Self)
      {
      XtSetArg(args[n],XmNrightAttachment,XmATTACH_SELF) ; n++ ;
      }
   else if (right)
      {
      XtSetArg(args[n],XmNrightAttachment,XmATTACH_WIDGET) ; n++ ;
      XtSetArg(args[n],XmNrightWidget, right) ; n++ ;
      }
   if (n)
      XtSetValues(w,args,n) ;
}

//----------------------------------------------------------------------

void FrWidget::attach(Widget top, Widget bottom, Widget left, Widget right)
{
  attach_widget(widget,top,bottom,left,right) ;
}

//----------------------------------------------------------------------

void FrWidget::attach(FrWidget *top, FrWidget *bottom, FrWidget *left,
		      FrWidget *right)
{
   Widget t, b, l, r ;
   if (top == Attach_to_FormW)
      t = Attach_to_Form ;
   else if (top == Attach_to_SelfW)
      t = Attach_to_Self ;
   else
      t = (top ? **top : 0) ;
   if (bottom == Attach_to_FormW)
      b = Attach_to_Form ;
   else if (bottom == Attach_to_SelfW)
      b = Attach_to_Self ;
   else
      b = (bottom ? **bottom : 0) ;
   if (left == Attach_to_FormW)
      l = Attach_to_Form ;
   else if (left == Attach_to_SelfW)
      l = Attach_to_Self ;
   else
      l = (left ? **left : 0) ;
   if (right == Attach_to_FormW)
      r = Attach_to_Form ;
   else if (right == Attach_to_SelfW)
      r = Attach_to_Self ;
   else
      r = (right ? **right : 0) ;
  attach_widget(widget,t,b,l,r) ;
}

//----------------------------------------------------------------------

void FrWidget::attachOpposite(Widget top, Widget bottom, Widget left,
			      Widget right)
{
   Arg args[10] ;
   int n = 0 ;

   if (top == Attach_to_Form)
      {
      XtSetArg(args[n],XmNtopAttachment,XmATTACH_FORM) ; n++ ;
      }
   else if (top == Attach_to_Self)
      {
      XtSetArg(args[n],XmNtopAttachment,XmATTACH_SELF) ; n++ ;
      }
   else if (top)
      {
      XtSetArg(args[n],XmNtopAttachment,XmATTACH_OPPOSITE_WIDGET) ; n++ ;
      XtSetArg(args[n],XmNtopWidget, top) ; n++ ;
      }
   if (bottom == Attach_to_Form)
      {
      XtSetArg(args[n],XmNbottomAttachment,XmATTACH_FORM) ; n++ ;
      }
   else if (bottom == Attach_to_Self)
      {
      XtSetArg(args[n],XmNbottomAttachment,XmATTACH_SELF) ; n++ ;
      }
   else if (bottom)
      {
      XtSetArg(args[n],XmNbottomAttachment,XmATTACH_OPPOSITE_WIDGET) ; n++ ;
      XtSetArg(args[n],XmNbottomWidget, bottom) ; n++ ;
      }
   if (left == Attach_to_Form)
      {
      XtSetArg(args[n],XmNleftAttachment,XmATTACH_FORM) ; n++ ;
      }
   else if (left == Attach_to_Self)
      {
      XtSetArg(args[n],XmNleftAttachment,XmATTACH_SELF) ; n++ ;
      }
   else if (left)
      {
      XtSetArg(args[n],XmNleftAttachment,XmATTACH_OPPOSITE_WIDGET) ; n++ ;
      XtSetArg(args[n],XmNleftWidget, left) ; n++ ;
      }
   if (right == Attach_to_Form)
      {
      XtSetArg(args[n],XmNrightAttachment,XmATTACH_FORM) ; n++ ;
      }
   else if (right == Attach_to_Self)
      {
      XtSetArg(args[n],XmNrightAttachment,XmATTACH_SELF) ; n++ ;
      }
   else if (right)
      {
      XtSetArg(args[n],XmNrightAttachment,XmATTACH_OPPOSITE_WIDGET) ; n++ ;
      XtSetArg(args[n],XmNrightWidget, right) ; n++ ;
      }
   if (n)
      XtSetValues(widget,args,n) ;
}

//----------------------------------------------------------------------

void FrWidget::attachOpposite(FrWidget *top, FrWidget *bottom, FrWidget *left,
			      FrWidget *right)
{
   Widget t, b, l, r ;
   if (top == Attach_to_FormW)
      t = Attach_to_Form ;
   else if (top == Attach_to_SelfW)
      t = Attach_to_Self ;
   else
      t = (top ? **top : 0) ;
   if (bottom == Attach_to_FormW)
      b = Attach_to_Form ;
   else if (bottom == Attach_to_SelfW)
      b = Attach_to_Self ;
   else
      b = (bottom ? **bottom : 0) ;
   if (left == Attach_to_FormW)
      l = Attach_to_Form ;
   else if (left == Attach_to_SelfW)
      l = Attach_to_Self ;
   else
      l = (left ? **left : 0) ;
   if (right == Attach_to_FormW)
      r = Attach_to_Form ;
   else if (right == Attach_to_SelfW)
      r = Attach_to_Self ;
   else
      r = (right ? **right : 0) ;
   attachOpposite(t,b,l,r) ;
}

//----------------------------------------------------------------------

void FrWidget::attachOffsets(int top, int bottom, int left, int right)
{
   Arg args[4] ;
   int n = 0 ;

   if (top >= 0)
      {
      XtSetArg(args[n], XmNtopOffset, top) ; n++ ;
      }
   if (bottom >= 0)
      {
      XtSetArg(args[n], XmNbottomOffset, bottom) ; n++ ;
      }
   if (left >= 0)
      {
      XtSetArg(args[n], XmNleftOffset, left) ; n++ ;
      }
   if (right >= 0)
      {
      XtSetArg(args[n], XmNrightOffset, right) ; n++ ;
      }
   if (n)
      XtSetValues(widget, args, n) ;
}

//----------------------------------------------------------------------

void FrWidget::attachPosition(int top, int bottom, int left, int right)
{
   Arg args[8] ;
   int n = 0 ;

   if (top >= 0)
      {
      XtSetArg(args[n], XmNtopAttachment, XmATTACH_POSITION) ; n++ ;
      XtSetArg(args[n], XmNtopPosition, top) ; n++ ;
      }
   if (bottom >= 0)
      {
      XtSetArg(args[n], XmNbottomAttachment, XmATTACH_POSITION) ; n++ ;
      XtSetArg(args[n], XmNbottomPosition, bottom) ; n++ ;
      }
   if (left >= 0)
      {
      XtSetArg(args[n], XmNleftAttachment, XmATTACH_POSITION) ; n++ ;
      XtSetArg(args[n], XmNleftPosition, left) ; n++ ;
      }
   if (right >= 0)
      {
      XtSetArg(args[n], XmNrightAttachment, XmATTACH_POSITION) ; n++ ;
      XtSetArg(args[n], XmNrightPosition, right) ; n++ ;
      }
   if (n)
      XtSetValues(widget, args, n) ;
}

//----------------------------------------------------------------------

void FrWidget::detach(bool top, bool bottom, bool left, bool right)
{
   Arg args[4] ;
   int n = 0 ;

   if (top)
      {
      XtSetArg(args[n], XmNtopAttachment, XmATTACH_NONE) ; n++ ;
      }
   if (bottom)
      {
      XtSetArg(args[n], XmNbottomAttachment, XmATTACH_NONE) ; n++ ;
      }
   if (left)
      {
      XtSetArg(args[n], XmNleftAttachment, XmATTACH_NONE) ; n++ ;
      }
   if (right)
      {
      XtSetArg(args[n], XmNrightAttachment, XmATTACH_NONE) ; n++ ;
      }
   if (n)
      XtSetValues(widget, args, n) ;
}

//----------------------------------------------------------------------

Widget FrWidget::parentWidget() const
{
   return XtParent(widget) ;
}

//----------------------------------------------------------------------

Widget FrWidget::grandparent() const
{
   return XtParent(XtParent(widget)) ;
}

//----------------------------------------------------------------------

int FrWidget::getWidth(bool include_border) const
{
  Arg args[2] ;
  if (!_managed)
     {
     XtSetMappedWhenManaged(widget,0) ;
     XtManageChild(widget) ;
     }
  Dimension width = 0, border = 0 ;
  int n = 0 ;
  XtSetArg(args[0], XmNwidth, &width) ; n++ ;
  if (include_border)
     {
     XtSetArg(args[n], XmNborderWidth, &border) ; n++ ;
     }
  XtGetValues(widget, args, n) ;
  if (include_border)
     width += border ;
  if (!_managed)
     {
     XtUnmanageChild(widget) ;
     XtSetMappedWhenManaged(widget,-1) ;
     }
  return width ;
}

//----------------------------------------------------------------------

int FrWidget::getHeight(bool include_border) const
{
  Arg args[2] ;
  if (!_managed)
     {
     XtSetMappedWhenManaged(widget,0) ;
     XtManageChild(widget) ;
     }
  Dimension height = 0, border = 0 ;
  int n = 0 ;
  XtSetArg(args[0], XmNheight, &height) ; n++ ;
  if (include_border)
     {
     XtSetArg(args[n], XmNborderWidth, &border) ; n++ ;
     }
  XtGetValues(widget, args, n) ;
  if (include_border)
     height += border ;
  if (!_managed)
     {
     XtSetMappedWhenManaged(widget,-1) ;
     XtUnmanageChild(widget) ;
     }
  return height ;
}

//----------------------------------------------------------------------

void FrWidget::setWidth(int width) const
{
   Arg args[1] ;
   XtSetArg(args[0], XmNwidth, width) ;
   XtSetValues(widget, args, 1) ;
}

//----------------------------------------------------------------------

void FrWidget::setHeight(int height) const
{
   Arg args[1] ;
   XtSetArg(args[0], XmNheight, height) ;
   XtSetValues(widget, args, 1) ;
}

//----------------------------------------------------------------------

void FrWidget::getSize(int *width, int *height, bool include_border) const
{
  Arg args[3] ;
  if (!_managed)
     {
     XtSetMappedWhenManaged(widget,0) ;
     XtManageChild(widget) ;
     }
  Dimension h = 0, w = 0, border = 0 ;
  int n = 0 ;
  if (height)
     {
     XtSetArg(args[n], XmNheight, &h) ; n++ ;
     }
  if (width)
     {
     XtSetArg(args[n], XmNwidth, &w) ; n++ ;
     }
  if (include_border)
     {
     XtSetArg(args[n], XmNborderWidth, &border) ; n++ ;
     }
  XtGetValues(widget, args, n) ;
  if (include_border)
     {
     h += border ;
     w += border ;
     }
  if (height)
     *height = h ;
  if (width)
     *width = w ;
  if (!_managed)
     {
     XtSetMappedWhenManaged(widget,-1) ;
     XtUnmanageChild(widget) ;
     }
}

//----------------------------------------------------------------------

void FrWidget::getPosition(int *x, int *y) const
{
  Arg args[2] ;
  if (!_managed)
     {
     XtSetMappedWhenManaged(widget,0) ;
     XtManageChild(widget) ;
     }
  Position widget_x = 0, widget_y = 0 ;
  int n = 0 ;
  if (x)
     {
     XtSetArg(args[n], XmNx, &widget_x) ; n++ ;
     }
  if (y)
     {
     XtSetArg(args[n], XmNy, &widget_y) ; n++ ;
     }
  XtGetValues(widget, args, n) ;
  if (x)
     *x = widget_x ;
  if (y)
     *y = widget_y ;
  if (!_managed)
     {
     XtSetMappedWhenManaged(widget,-1) ;
     XtUnmanageChild(widget) ;
     }
}

//----------------------------------------------------------------------

void FrWidget::setPosition(int x, int y) const
{
  Arg args[2] ;
  Position widget_x = x, widget_y = y ;
  int n = 0 ;
  XtSetArg(args[n], XmNx, widget_x) ; n++ ;
  XtSetArg(args[n], XmNy, widget_y) ; n++ ;
  XtSetValues(widget, args, n) ;
}

//----------------------------------------------------------------------

void FrWidget::warpPointer(int x, int y) const
{
   if (widget)
      XWarpPointer(XtDisplay(widget),None,XtWindow(widget),0,0,0,0,x,y) ;
}

//----------------------------------------------------------------------

void FrWidget::warpPointerCenter() const
{
   if (widget)
      {
      int width, height ;
      getSize(&width,&height,false) ;
      XWarpPointer(XtDisplay(widget),None,XtWindow(widget),0,0,0,0,
		   width/2,height/2) ;
      }
}

//----------------------------------------------------------------------

void FrWidget::getPointer(int *x, int *y) const
{
   int junk ;
   Window junkwin ;
   int ptr_x, ptr_y ;

   if (widget)
      XQueryPointer(XtDisplay(widget),XtWindow(widget),&junkwin, &junkwin,
		    &ptr_x, &ptr_y, &junk, &junk, (unsigned int *)&junk) ;
   else
      ptr_x = ptr_y = 0 ;
   if (x)
      *x = ptr_x ;
   if (y)
      *y = ptr_y ;
}

/************************************************************************/
/*    Methods for class FrWSeparator					*/
/************************************************************************/

static Widget create_separator(Widget parent, int linestyle)
{
   if (linestyle == -1)
      linestyle = XmSINGLE_LINE ;
   Arg args[1] ;
   XtSetArg(args[0], XmNseparatorType, linestyle) ;
   return XtCreateManagedWidget("sep", xmSeparatorWidgetClass,
				parent, args, 1) ;
}

//----------------------------------------------------------------------

FrWSeparator::FrWSeparator(Widget parent, int linestyle)
{
   widget = create_separator(parent,linestyle) ;
   _managed = true ;
}

//----------------------------------------------------------------------

FrWSeparator::FrWSeparator(FrWidget *parent, int linestyle)
{
   widget = create_separator(**parent,linestyle) ;
   _managed = true ;
}

//----------------------------------------------------------------------

FrObjectType FrWSeparator::objType() const
{
   return (FrObjectType)OT_FrWSeparator ;
}

//----------------------------------------------------------------------

const char *FrWSeparator::objTypeName() const
{
   return "FrWSeparator" ;
}

//----------------------------------------------------------------------

FrObjectType FrWSeparator::objSuperclass() const
{
   return (FrObjectType)OT_FrWidget ;
}

/************************************************************************/
/*    Methods for class FrWFrame					*/
/************************************************************************/

static Widget create_frame_widget(Widget parent)
{
   return XtVaCreateManagedWidget("frame",
                                  xmFrameWidgetClass, parent,
                                  XmNshadowType, XmSHADOW_ETCHED_OUT,
                                  NULL) ;
}

//----------------------------------------------------------------------

FrWFrame::FrWFrame(Widget parent)
{
   widget = create_frame_widget(parent) ;
   _managed = true ;
}

//----------------------------------------------------------------------

FrWFrame::FrWFrame(FrWidget *parent)
{
   widget = create_frame_widget(**parent) ;
   _managed = true ;
}

//----------------------------------------------------------------------

FrObjectType FrWFrame::objType() const
{
   return (FrObjectType)OT_FrWFrame ;
}

//----------------------------------------------------------------------

const char *FrWFrame::objTypeName() const
{
   return "FrWFrame" ;
}

//----------------------------------------------------------------------

FrObjectType FrWFrame::objSuperclass() const
{
   return (FrObjectType)OT_FrWidget ;
}

/************************************************************************/
/*    Methods for class FrWArrow					*/
/************************************************************************/

static Widget create_arrow(Widget parent, ArrowDirection orientation)
{
   int dir ;
   switch (orientation)
      {
      case ArrowUp:
	 dir = XmARROW_UP ;
	 break ;
      case ArrowDown:
	 dir = XmARROW_DOWN ;
	 break ;
      case ArrowLeft:
	 dir = XmARROW_LEFT ;
	 break ;
      case ArrowRight:
	 dir = XmARROW_RIGHT ;
	 break ;
      default:
	 return (Widget)0 ;
      }
   Arg args[5] ;
   int n = 0 ;
   XtSetArg(args[n], XmNarrowDirection, dir) ; n++ ;
   Widget w = XmCreateArrowButton(parent, str_arrow, args, n) ;
   return w ;
}

//----------------------------------------------------------------------

FrWArrow::FrWArrow(Widget parent, ArrowDirection dir)
{
   direction = dir ;
   widget = create_arrow(parent,direction) ;
   _managed = true ;
}

//----------------------------------------------------------------------

FrWArrow::FrWArrow(FrWidget *parent, ArrowDirection dir)
{
   direction = dir ;
   widget = create_arrow(**parent,direction) ;
   _managed = true ;
}

//----------------------------------------------------------------------

FrObjectType FrWArrow::objType() const
{
   return (FrObjectType)OT_FrWArrow ;
}

//----------------------------------------------------------------------

const char *FrWArrow::objTypeName() const
{
   return "FrWArrow" ;
}

//----------------------------------------------------------------------

FrObjectType FrWArrow::objSuperclass() const
{
   return (FrObjectType)OT_FrWidget ;
}

/************************************************************************/
/*    Methods for class FrWArrowG					*/
/************************************************************************/

static Widget create_arrow_gadget(Widget parent, int orientation)
{
   switch (orientation)
      {
      case ArrowUp:
	 orientation = XmARROW_UP ;
	 break ;
      case ArrowDown:
	 orientation = XmARROW_DOWN ;
	 break ;
      case ArrowLeft:
	 orientation = XmARROW_LEFT ;
	 break ;
      case ArrowRight:
	 orientation = XmARROW_RIGHT ;
	 break ;
      default:
	 return (Widget)0 ;
      }
   Arg args[5] ;
   int n = 0 ;
   XtSetArg(args[n], XmNarrowDirection, orientation) ; n++ ;
   Widget w = XmCreateArrowButtonGadget(parent, str_arrow, args, n) ;
   return w ;
}

//----------------------------------------------------------------------

FrWArrowG::FrWArrowG(Widget parent, ArrowDirection dir)
{
   direction = dir ;
   widget = create_arrow_gadget(parent,direction) ;
   _managed = true ;
}

//----------------------------------------------------------------------

FrWArrowG::FrWArrowG(FrWidget *parent, ArrowDirection dir)
{
   direction = dir ;
   widget = create_arrow_gadget(**parent,direction) ;
   _managed = true ;
}

//----------------------------------------------------------------------

FrObjectType FrWArrowG::objType() const
{
   return (FrObjectType)OT_FrWArrowG ;
}

//----------------------------------------------------------------------

const char *FrWArrowG::objTypeName() const
{
   return "FrWArrowG" ;
}

//----------------------------------------------------------------------

FrObjectType FrWArrowG::objSuperclass() const
{
   return (FrObjectType)OT_FrWidget ;
}

/************************************************************************/
/*    Methods for class FrWLabel					*/
/************************************************************************/

static Widget create_label(Widget parent, const char *name, bool managed)
{
   if (!name)
      name = " " ;
   XmString namestring = XmStringCreateLtoR((char*)name,
					    XmSTRING_DEFAULT_CHARSET) ;
   Arg args[2] ;
   int n = 0 ;
   XtSetArg(args[n], XmNlabelString, namestring) ; n++ ;
   Widget label ;
   if (managed)
      label = XtCreateManagedWidget(str_label,xmLabelWidgetClass,parent,
				    args,n) ;
   else
      label = XtCreateWidget(str_label,xmLabelWidgetClass,parent,args,n) ;
   XmStringFree(namestring) ;
   return label ;
}

//----------------------------------------------------------------------

FrWLabel::FrWLabel(Widget parent, const char *label, bool managed)
{
   widget = create_label(parent, label, managed) ;
   _managed = managed ;
}

//----------------------------------------------------------------------

FrWLabel::FrWLabel(FrWidget *parent, const char *label, bool managed)
{
   widget = create_label(**parent, label, managed) ;
   _managed = managed ;
}

//----------------------------------------------------------------------

FrObjectType FrWLabel::objType() const
{
   return (FrObjectType)OT_FrWLabel ;
}

//----------------------------------------------------------------------

const char *FrWLabel::objTypeName() const
{
   return "FrWLabel" ;
}

//----------------------------------------------------------------------

FrObjectType FrWLabel::objSuperclass() const
{
   return (FrObjectType)OT_FrWidget ;
}

/************************************************************************/
/*    Methods for class FrWList						*/
/************************************************************************/

static Widget create_list(Widget parent, const char *name, int visible)
{
   if (!name || *name == '\0')
      name = "list" ;
   Arg args[4] ;
   int n = 0 ;
   XtSetArg(args[n],XmNvisibleItemCount, visible) ; n++ ;
//!!!
   Widget rowcol = XtCreateWidget(name, xmListWidgetClass, parent, args, n) ;
   return rowcol ;
}

//----------------------------------------------------------------------

FrWList::FrWList(Widget parent, const char *name, int visible, int max)
{
   widget = create_list(parent, name, visible) ;
   _managed = false ;
   visible_items = visible ;
   max_items = max ;
   curr_items = 0 ;
}

//----------------------------------------------------------------------

FrWList::FrWList(FrWidget *parent, const char *name, int visible, int max)
{
   widget = create_list(**parent, name, visible) ;
   _managed = false ;
   visible_items = visible ;
   max_items = max ;
   curr_items = 0 ;
}

//----------------------------------------------------------------------

FrObjectType FrWList::objType() const
{
   return (FrObjectType)OT_FrWList ;
}

//----------------------------------------------------------------------

const char *FrWList::objTypeName() const
{
   return "FrWList" ;
}

//----------------------------------------------------------------------

FrObjectType FrWList::objSuperclass() const
{
   return (FrObjectType)OT_FrWidget ;
}

//----------------------------------------------------------------------

void FrWList::add(const char *item)
{
  XmString str = XmStringCreateSimple((char*)item) ;
  XmListAddItemUnselected(widget, str, 0) ;
  XmStringFree(str) ;
  curr_items++ ;
  if (max_items && curr_items > max_items)
     XmListDeleteItemsPos(widget,max_items > 6 ? 5 : max_items,1) ;
  XmListSetBottomPos(widget,0) ;
}

/************************************************************************/
/*    Methods for class FrWRowColumn					*/
/************************************************************************/

static Widget create_rowcolumn(Widget parent, const char *name, bool vertical)
{
   if (!name || *name == '\0')
      name = "rowcol" ;
   Arg args[2] ;
   int n = 0 ;
   XtSetArg(args[n],XmNorientation, vertical ? XmVERTICAL : XmHORIZONTAL); n++;
   Widget rowcol = XmCreateRowColumn(parent, (char*)name, args, n) ;
   return rowcol ;
}

//----------------------------------------------------------------------

FrWRowColumn::FrWRowColumn(Widget parent, const char *name, bool vertical)
{
   widget = create_rowcolumn(parent, name, vertical) ;
   _managed = false ;
}

//----------------------------------------------------------------------

FrWRowColumn::FrWRowColumn(FrWidget *parent, const char *name, bool vertical)
{
   widget = create_rowcolumn(**parent, name, vertical) ;
   _managed = false ;
}

//----------------------------------------------------------------------

FrObjectType FrWRowColumn::objType() const
{
   return (FrObjectType)OT_FrWRowColumn ;
}

//----------------------------------------------------------------------

const char *FrWRowColumn::objTypeName() const
{
   return "FrWRowColumn" ;
}

//----------------------------------------------------------------------

FrObjectType FrWRowColumn::objSuperclass() const
{
   return (FrObjectType)OT_FrWidget ;
}

//----------------------------------------------------------------------

void FrWRowColumn::setColumns(int columns) const
{
   Arg args[1] ;
   XtSetArg(args[0], XmNnumColumns, columns) ;
   XtSetValues(widget, args, 1) ;
}

//----------------------------------------------------------------------

void FrWRowColumn::setPacked(int packed) const
{
   Arg args[1] ;
   XtSetArg(args[0], XmNpacking, packed ? XmPACK_COLUMN : XmNO_PACKING) ;
   XtSetValues(widget, args, 1) ;
}

//----------------------------------------------------------------------

void FrWRowColumn::setAlignment(int align) const
{
   Arg args[2] ;
   int a ;
   if (align < 0)
      a = XmALIGNMENT_BEGINNING ;
   else if (align > 0)
      a = XmALIGNMENT_END ;
   else // align == 0
      a = XmALIGNMENT_CENTER ;
   XtSetArg(args[0], XmNentryAlignment, a) ;
   XtSetArg(args[1], XmNisAligned, true) ;
   XtSetValues(widget, args, 2) ;
}

/************************************************************************/
/*    Methods for class FrWOptionMenu					*/
/************************************************************************/

static void option_menu_change_cb(Widget, XtPointer client_data,
				  XtPointer call_data)
{
   FrWPushButton *button = (FrWPushButton*)client_data ;
   FrWOptionMenu *menu = (FrWOptionMenu*)button->user_data ;
   char *label = button->getLabel() ;
   if (label && *label)
      menu->setSelection(label) ;
   XtFree(label) ;
   if (menu)
      menu->callChangeCallback(call_data) ;
}

//----------------------------------------------------------------------

static Widget create_option_menu(Widget parent, const char *title,
				 FrList *options, FrObject *def_option,
				 bool managed, FrList **button_list)
{
   Widget pulldown = create_pulldown_menu(parent,str_option,false) ;
   Widget def_opt = 0 ;
   FrList *buttons = 0 ;
   for (FrList *opt = options ; opt ; opt = opt->rest())
      {
      FrObject *o = opt->first() ;
      char *option_str = o->print() ;
      FrWPushButton *button = new FrWPushButton(pulldown,option_str,true,false,
						"option_item") ;
      FrFree(option_str) ;
      button->activateCallback(option_menu_change_cb,button) ;
      if (!def_opt && equal(o,def_option))
	 def_opt = **button ;
      pushlist(button,buttons) ;
      }
   Arg args[5] ;
   int n = 0 ;
   XtSetArg(args[n], XmNsubMenuId, pulldown) ; n++ ;
   XmString titlestr ;
   if (title)
      {
      titlestr = XmStringCreateSimple((char*)title) ;
      XtSetArg(args[n], XmNlabelString, titlestr) ; n++ ;
      }
   else
      titlestr = 0 ;
   if (def_opt)
      {
      XtSetArg(args[n], XmNmenuHistory, def_opt) ; n++ ;
      }
   Widget menu = XmCreateOptionMenu(parent, str_option, args, n) ;
   if (titlestr)
      XmStringFree(titlestr) ;
   if (managed)
      XtManageChildren(&menu,1) ;
   *button_list = buttons ;
   return menu ;
}

//----------------------------------------------------------------------

FrWOptionMenu::FrWOptionMenu(Widget parent, const char *name,
			     FrList *options, FrObject *def_option,
			     bool managed)
{
   widget = create_option_menu(parent, name, options, def_option, managed,
			       &buttons) ;
   _managed = managed ;
   current_selection = def_option ? def_option->deepcopy() : 0 ;
   for (FrList *b = buttons ; b ; b = b->rest())
      ((FrWPushButton*)b->first())->user_data = this ;
   destroyCallback() ;
}

//----------------------------------------------------------------------

FrWOptionMenu::FrWOptionMenu(FrWidget *parent, const char *name,
			     FrList *options, FrObject *def_option,
			     bool managed)
{
   widget = create_option_menu(**parent, name, options, def_option, managed,
			       &buttons) ;
   _managed = managed ;
   current_selection = def_option ? def_option->deepcopy() : 0 ;
   for (FrList *b = buttons ; b ; b = b->rest())
      ((FrWPushButton*)b->first())->user_data = this ;
   destroyCallback() ;
}

//----------------------------------------------------------------------

FrWOptionMenu::~FrWOptionMenu()
{
   free_object(buttons) ;
   free_object(current_selection) ;
}

//----------------------------------------------------------------------

FrObjectType FrWOptionMenu::objType() const
{
   return (FrObjectType)OT_FrWOptionMenu ;
}

//----------------------------------------------------------------------

const char *FrWOptionMenu::objTypeName() const
{
   return "FrWOptionMenu" ;
}

//----------------------------------------------------------------------

FrObjectType FrWOptionMenu::objSuperclass() const
{
   return (FrObjectType)OT_FrWidget ;
}

//----------------------------------------------------------------------

void FrWOptionMenu::setSelection(const char *sel_text)
{
   free_object(current_selection) ;
   current_selection = string_to_FrObject(sel_text) ;
}

//----------------------------------------------------------------------

#if 0
FrObject *FrWOptionMenu::currentSelection() const
{
   Widget curr_item ;
   Arg args[1] ;
   XtSetArg(args[0], XmNmenuHistory, &curr_item) ;
   XtGetValues(widget, args, 1) ;
   if (curr_item)
      {
      FrWidget w(curr_item) ;
      char *label = w.getLabel() ;
      char *l = label ;
      FrObject *obj = string_to_FrObject(l) ;
      XtFree(label) ;
      w.retain() ;
      return obj ;
      }
   else
      return 0 ;
}
#endif /* 0 */

//----------------------------------------------------------------------

void FrWOptionMenu::changeCallback(XtCallbackProc cb_func, XtPointer cb_data,
				     bool remove)
{
   if (widget)
      {
      if (remove)
         change_func = 0;
      if (cb_func)
	 {
	 change_func = cb_func ;
	 change_data = cb_data ;
	 }
      }
}

//----------------------------------------------------------------------

void FrWOptionMenu::changeCallback(XtCallbackProc cb_func)
{
   if (widget && cb_func)
      {
      change_func = cb_func ;
      change_data = this ;
      }
}

//----------------------------------------------------------------------

void FrWOptionMenu::callChangeCallback(XtPointer call_data) const
{
   if (change_func)
      change_func(widget,change_data,call_data) ;
}

/************************************************************************/
/*    Methods for class FrWPopupMenu					*/
/************************************************************************/

static Widget create_popup_menu(Widget parent, const char *name, bool managed)
{
   Arg args[2] ;
   int n = 0 ;
   XmString title ;
   if (name)
      {
      title = XmStringCreateSimple((char*)name) ;
      XtSetArg(args[n], XmNlabelString, title) ; n++ ;
      }
   else
      title = 0 ;
   XtSetArg(args[n], XmNautoUnmanage, true) ; n++ ; //!!!
   Widget popup = XmCreatePopupMenu(parent, "popup", args, n) ;
   if (title)
      XmStringFree(title) ;
   if (managed)
      XtManageChildren(&popup,1) ;
   return popup ;
}

//----------------------------------------------------------------------

FrWPopupMenu::FrWPopupMenu(Widget parent, const char *name, bool managed)
{
   widget = create_popup_menu(parent, name, managed) ;
   _managed = managed ;
}

//----------------------------------------------------------------------

FrWPopupMenu::FrWPopupMenu(FrWidget *parent, const char *name, bool managed)
{
   widget = create_popup_menu(**parent, name, managed) ;
   _managed = managed ;
}

//----------------------------------------------------------------------

FrObjectType FrWPopupMenu::objType() const
{
   return (FrObjectType)OT_FrWPopupMenu ;
}

//----------------------------------------------------------------------

const char *FrWPopupMenu::objTypeName() const
{
   return "FrWPopupMenu" ;
}

//----------------------------------------------------------------------

FrObjectType FrWPopupMenu::objSuperclass() const
{
   return (FrObjectType)OT_FrWidget ;
}

//----------------------------------------------------------------------

void FrWPopupMenu::popup()
{
   manage() ;
   warpPointer(0,0) ;
   //!!!
}

//----------------------------------------------------------------------

void FrWPopupMenu::menuPosition(XtPointer call_data)
{
   XmPushButtonCallbackStruct *cbs = (XmPushButtonCallbackStruct*)call_data ;
   XButtonPressedEvent *location = (XButtonPressedEvent*)cbs->event ;
   XmMenuPosition(widget, location) ;
}

/************************************************************************/
/*    Methods for class FrWPulldownMenu					*/
/************************************************************************/

static Widget create_pulldown_menu(Widget parent, const char *name,
				   bool managed)
{
   Arg args[2] ;
   int n = 0 ;
   XmString title ;
   if (name)
      {
      title = XmStringCreateSimple((char*)name) ;
      XtSetArg(args[n], XmNlabelString, title) ; n++ ;
      }
   else
      title = 0 ;
   XtSetArg(args[n], XmNautoUnmanage, true) ; n++ ; //!!!
   Widget popup = XmCreatePulldownMenu(parent, "pulldown", args, n) ;
   if (title)
      XmStringFree(title) ;
   if (managed)
      XtManageChildren(&popup,1) ;
   return popup ;
}

//----------------------------------------------------------------------

FrWPulldownMenu::FrWPulldownMenu(Widget parent, const char *name, bool managed)
{
   widget = create_pulldown_menu(parent, name, managed) ;
   _managed = managed ;
   grabbed_button = 0 ;
}

//----------------------------------------------------------------------

FrWPulldownMenu::FrWPulldownMenu(FrWidget *parent, const char *name,
				 bool managed)
{
   widget = create_pulldown_menu(**parent, name, managed) ;
   _managed = managed ;
   grabbed_button = 0 ;
}

//----------------------------------------------------------------------

FrObjectType FrWPulldownMenu::objType() const
{
   return (FrObjectType)OT_FrWPulldownMenu ;
}

//----------------------------------------------------------------------

const char *FrWPulldownMenu::objTypeName() const
{
   return "FrWPulldownMenu" ;
}

//----------------------------------------------------------------------

FrObjectType FrWPulldownMenu::objSuperclass() const
{
   return (FrObjectType)OT_FrWidget ;
}

//----------------------------------------------------------------------

void FrWPulldownMenu::popup()
{
   manage() ;
   warpPointer(0,0) ;
   //!!!
}

//----------------------------------------------------------------------

void FrWPulldownMenu::pulldown(int button)
{
   grabbed_button = button ;
   manage() ;
   //!!!
//XtGrabButton(widget,button,Modifiers,_XtBoolean owner_events,
//             unsigned int event_mask, int pointer_mode,
//	       int keyboard_mode, Window confine_to, Cursor) ;
//XtUngrabButton(widget,button,Modifiers) ;
}


/************************************************************************/
/*    Methods for class FrWPromptPopup					*/
/************************************************************************/

static Widget create_prompt(Widget parent, const char *title,
			    const char *label, const char *def,
			    XtCallbackProc ok_cb, XtPointer ok_data,
			    XtPointer help_data,
			    bool auto_unmanage, bool modal,
			    bool managed, const char *widget_name)
{
   Arg args[10] ;
   int n = 0 ;
   XmString str = XmStringCreateLtoR((char*)label,XmSTRING_DEFAULT_CHARSET) ;
   XtSetArg(args[n], XmNselectionLabelString, str) ; n++ ;
   XtSetArg(args[n], XmNautoUnmanage, auto_unmanage) ; n++ ;
   XtSetArg(args[n], XmNdeleteResponse, XmDESTROY) ; n++ ;
   XtSetArg(args[n], XmNdialogStyle, modal ? XmDIALOG_FULL_APPLICATION_MODAL
	    				   : XmDIALOG_MODELESS) ; n++ ;
   XmString defvalue ;
   if (def)
      {
      defvalue = XmStringCreateSimple((char*)def) ;
      XtSetArg(args[n], XmNtextString,defvalue) ; n++ ;
      }
   else
      defvalue = 0 ;
   XmString tstr ;
   if (title)
      {
      tstr = XmStringCreateSimple((char*)title) ;
      XtSetArg(args[n], XmNtitle, title) ; n++ ;
      }
   else
      tstr = 0 ;
   XtSetArg(args[n], XmNiconName, str_Prompt) ; n++ ;
   Widget dialog = XmCreatePromptDialog(parent,(char*)widget_name,args,n) ;
   XmStringFree(str);
   if (defvalue)
      XmStringFree(defvalue) ;
   if (tstr)
      XmStringFree(tstr) ;
   XtRemoveAllCallbacks(dialog,XmNokCallback);
   if (ok_cb)
     XtAddCallback(dialog,XmNokCallback,ok_cb,ok_data);
   if (help_data)
      {
      XtRemoveAllCallbacks(dialog,XmNhelpCallback) ;
      XtAddCallback(dialog,XmNhelpCallback,FrHelp,help_data) ;
      }
   else
      XtUnmanageChild(XmSelectionBoxGetChild(dialog,XmDIALOG_HELP_BUTTON)) ;
   if (managed)
      XtManageChildren(&dialog,1);
   return dialog ;
}

//----------------------------------------------------------------------

FrWPromptPopup::FrWPromptPopup(Widget parent, const char *label,
			       const char *def,
			       XtCallbackProc ok_cb, XtPointer ok_data,
			       XtPointer help_data,
			       bool auto_unmanage, bool modal, bool managed)
{
   widget = create_prompt(parent, str_Prompt, label, def, ok_cb, ok_data,
			  help_data, auto_unmanage, modal, managed,
			  str_prompt) ;
   _managed = managed ;
   if (modal && managed)
      warpPointerCenter() ;
}

//----------------------------------------------------------------------

FrWPromptPopup::FrWPromptPopup(FrWidget *parent, const char *label,
			       const char *def,
			       XtCallbackProc ok_cb, XtPointer ok_data,
			       XtPointer help_data,
			       bool auto_unmanage, bool modal,
			       bool managed)
{
   widget = create_prompt(**parent, str_Prompt, label, def, ok_cb, ok_data,
			  help_data, auto_unmanage, modal, managed,
			  str_prompt) ;
   _managed = managed ;
   if (modal && managed)
      warpPointerCenter() ;
}

//----------------------------------------------------------------------

FrObjectType FrWPromptPopup::objType() const
{
   return (FrObjectType)OT_FrWPromptPopup ;
}

//----------------------------------------------------------------------

const char *FrWPromptPopup::objTypeName() const
{
   return "FrWPromptPopup" ;
}

//----------------------------------------------------------------------

FrObjectType FrWPromptPopup::objSuperclass() const
{
   return (FrObjectType)OT_FrWidget ;
}

//----------------------------------------------------------------------

void FrWPromptPopup::popup()
{
   manage() ;
   warpPointerCenter() ;
   XtPopup(XtParent(widget),XtGrabNone) ;
}

/************************************************************************/
/*    Methods for class FrWVerifyPopup					*/
/************************************************************************/

static Widget create_verify_popup(Widget parent, const char *prompt,
				  XtCallbackProc ok_cb, XtPointer ok_data,
				  XtCallbackProc cancel_cb, XtPointer cancel_data,
				  XtPointer help_data, bool modal)
{
   Arg args[6] ;
   int n = 0 ;
   XmString yes = XmStringCreateLtoR("Yes",XmSTRING_DEFAULT_CHARSET);
   XmString no = XmStringCreateLtoR("NO",XmSTRING_DEFAULT_CHARSET);
   XmString str = XmStringCreateLtoR((char*)prompt,XmSTRING_DEFAULT_CHARSET);
   XtSetArg(args[n],XmNokLabelString,yes) ; n++ ;
   XtSetArg(args[n],XmNcancelLabelString,no) ; n++ ;
   XtSetArg(args[n],XmNmessageString,str) ; n++ ;
   if (modal)
      {
      XtSetArg(args[n],XmNdialogStyle,XmDIALOG_FULL_APPLICATION_MODAL) ;
      n++ ;
      }
   Widget dialog = XmCreateQuestionDialog(parent,"verify_dialog",args,n) ;
   XmStringFree(str);
   XmStringFree(no);
   XmStringFree(yes);
   FrWidget popup(dialog) ;
   if (ok_cb)
      popup.okCallback(ok_cb,ok_data) ;
   else
      popup.okCallback((XtCallbackProc)XtDestroyWidget,dialog) ;
   if (cancel_cb)
      popup.cancelCallback(cancel_cb,cancel_data) ;
   else
      popup.cancelCallback((XtCallbackProc)XtDestroyWidget,dialog) ;
   if (help_data)
      popup.helpCallback(FrHelp,help_data) ;
   else
      popup.helpCallback(FrHelp,FramepaC_helptexts[FrHelptext_verify]) ;
   popup.manage() ;
   FrWidget button(XmMessageBoxGetChild(dialog, XmDIALOG_OK_BUTTON)) ;
   button.setManaged(true) ;
   int x, y ;
   button.getPosition(&x,&y) ;
   button.retain() ;
   popup.warpPointer(x+5,y+5) ;  // move pointer into OK button
   popup.retain() ;
   return dialog ;
}

//----------------------------------------------------------------------

FrWVerifyPopup::FrWVerifyPopup(Widget parent, const char *prompt,
			       XtCallbackProc ok_cb = 0, XtPointer ok_data = 0,
			       XtCallbackProc cancel_cb = 0,
			       XtPointer cancel_data = 0,
			       XtPointer help_data = 0,
			       bool modal = false)
{
   widget = create_verify_popup(parent,prompt,ok_cb,ok_data,
				cancel_cb,cancel_data,help_data,modal) ;
   _managed = true ;
}

//----------------------------------------------------------------------

FrWVerifyPopup::FrWVerifyPopup(FrWidget *parent, const char *prompt,
			       XtCallbackProc ok_cb = 0, XtPointer ok_data = 0,
			       XtCallbackProc cancel_cb = 0,
			       XtPointer cancel_data = 0,
			       XtPointer help_data = 0,
			       bool modal = false)
{
   widget = create_verify_popup(**parent,prompt,ok_cb,ok_data,
				cancel_cb,cancel_data,help_data,modal) ;
   _managed = true ;
}

//----------------------------------------------------------------------

FrObjectType FrWVerifyPopup::objType() const
{
   return OT_FrWVerifyPopup ;
}

//----------------------------------------------------------------------

const char *FrWVerifyPopup::objTypeName() const
{
   return "FrWVerifyPopup" ;
}

//----------------------------------------------------------------------

FrObjectType FrWVerifyPopup::objSuperclass() const
{
   return OT_FrWidget ;
}

/************************************************************************/
/*    Methods for class FrWDialogPopup					*/
/************************************************************************/

static Widget create_form_dialog(Widget parent, const char *name,
				 const char *title, const char *icon,
				 bool auto_unmanage)
{
   Arg args[3] ;
   int n = 0 ;

   if (!name || *name == '\0')
      name = "form_dialog" ;
   if (title)
      {
      XtSetArg(args[n], XmNtitle,title) ; n++ ;
      }
   if (icon)
      {
      XtSetArg(args[n], XmNiconName,icon) ; n++ ;
      }
   XtSetArg(args[n], XmNautoUnmanage, auto_unmanage) ; n++ ;
   return XmCreateFormDialog(parent,(char*)name,args,n) ;
}

//----------------------------------------------------------------------

FrWDialogPopup::FrWDialogPopup(Widget parent, const char *name,
			       const char *title, const char *icon,
			       bool auto_unmanage)
{
   widget = create_form_dialog(parent, name, title, icon, auto_unmanage) ;
   _managed = false ;
}

//----------------------------------------------------------------------

FrWDialogPopup::FrWDialogPopup(FrWidget *parent, const char *name,
			       const char *title, const char *icon,
			       bool auto_unmanage)
{
   widget = create_form_dialog(**parent, name, title, icon, auto_unmanage) ;
   _managed = false ;
}

//----------------------------------------------------------------------

FrObjectType FrWDialogPopup::objType() const
{
   return (FrObjectType)OT_FrWDialogPopup ;
}

//----------------------------------------------------------------------

const char *FrWDialogPopup::objTypeName() const
{
   return "FrWDialogPopup" ;
}

//----------------------------------------------------------------------

FrObjectType FrWDialogPopup::objSuperclass() const
{
   return (FrObjectType)OT_FrWidget ;
}

/************************************************************************/
/*    Methods for class FrWMessagePopup					*/
/************************************************************************/

static Widget create_message_popup(FrWMessageType type, Widget w,
				   const char *message,
				   XtCallbackProc ok,
				   XtCallbackProc cancel,
				   XtPointer client_data,
				   const char **help_data)
{
   process_pending_X_events(w) ;
   XmString msg = XmStringCreateLtoR((char*)message,XmSTRING_DEFAULT_CHARSET) ;
   Arg args[3] ;
   XtSetArg(args[0], XmNmessageString,msg) ;
   XtSetArg(args[1], XmNautoUnmanage, true) ;
   Widget popup ;
   switch (type)
      {
      case FrWMessage_Working:
	 XtSetArg(args[2], XmNiconName, "Working") ;
	 popup = XmCreateWorkingDialog(w, "working", args, 3) ;
	 break ;
      case FrWMessage_Info:
	 XtSetArg(args[2], XmNiconName, "Info") ;
	 popup = XmCreateInformationDialog(w, "info_popup", args, 3) ;
	 break ;
      case FrWMessage_Question:
	 XtSetArg(args[2], XmNiconName, "Query") ;
	 popup = XmCreateWarningDialog(w, "query", args, 3) ;
	 break ;
      default:
	 popup = 0 ;
	 break ;
      }
   XmStringFree(msg) ;
   if (ok)
      XtAddCallback(popup,XmNokCallback,ok,client_data) ;
   else
      XtUnmanageChild(XmMessageBoxGetChild(popup,XmDIALOG_OK_BUTTON)) ;
   if (cancel)
      XtAddCallback(popup,XmNcancelCallback,cancel,client_data) ;
   else
      XtUnmanageChild(XmMessageBoxGetChild(popup,XmDIALOG_CANCEL_BUTTON)) ;
   if (help_data)
      XtAddCallback(popup,XmNhelpCallback,FrHelp,help_data) ;
   else
      XtUnmanageChild(XmMessageBoxGetChild(popup,XmDIALOG_HELP_BUTTON)) ;
   XtManageChildren(&popup,1) ;
   process_pending_X_events(popup) ;
   return popup ;
}

//----------------------------------------------------------------------

static void change_message_popup(Widget w,const char *message)
{
   XmString msg = XmStringCreateLtoR((char*)message,XmSTRING_DEFAULT_CHARSET) ;
   Arg args[1] ;
   XtSetArg(args[0], XmNmessageString, msg) ;
   XtSetValues(w, args, 1) ;
   XmStringFree(msg) ;
   process_pending_X_events(w) ;
}

//----------------------------------------------------------------------

FrWMessagePopup::FrWMessagePopup(FrWMessageType type, Widget parent,
				 const char *message, XtCallbackProc ok,
				 XtCallbackProc cancel, XtPointer client_data,
				 const char **help_data)
{
   switch (type)
      {
      case FrWMessage_Working:
      case FrWMessage_Info:
      case FrWMessage_Question:
	 widget = create_message_popup(type,parent,message,ok,cancel,
				       client_data,help_data) ;
	 break ;
      default:
	 widget = 0 ;
	 break ;
      }
}

//----------------------------------------------------------------------

FrWMessagePopup::FrWMessagePopup(FrWMessageType type, FrWidget *parent,
				 const char *message, XtCallbackProc ok,
				 XtCallbackProc cancel, XtPointer client_data,
				 const char **help_data)
{
   switch (type)
      {
      case FrWMessage_Working:
      case FrWMessage_Info:
      case FrWMessage_Question:
	 create_message_popup(type,**parent,message,ok,cancel,client_data,
			      help_data) ;
	 break ;
      default:
	 widget = 0 ;
	 break ;
      }
}

//----------------------------------------------------------------------

FrWMessagePopup::~FrWMessagePopup()
{
   if (widget)
      {
      Widget w = widget ;
      widget = 0 ;		// don't re-enter via process_pending_X_events
      XtUnmapWidget(w) ;
      process_pending_X_events(w) ;
      XtDestroyWidget(w) ;
      process_pending_X_events(w) ;
      }
}

//----------------------------------------------------------------------

void FrWMessagePopup::freeObject()
{
   delete this ;
}

//----------------------------------------------------------------------

FrObjectType FrWMessagePopup::objType() const
{
   return (FrObjectType)OT_FrWMessagePopup ;
}

//----------------------------------------------------------------------

const char *FrWMessagePopup::objTypeName() const
{
   return "FrWMessagePopup" ;
}

//----------------------------------------------------------------------

FrObjectType FrWMessagePopup::objSuperclass() const
{
   return (FrObjectType)OT_FrWidget ;
}

//----------------------------------------------------------------------

void FrWMessagePopup::changeMessage(const char *message)
{
   if (widget)
      change_message_popup(widget,message) ;
}

/************************************************************************/
/*    Methods for class FrWSelectionBox					*/
/************************************************************************/

static Widget create_selection_box(Widget w, FrList *items, char *label,
				   XtCallbackProc ok_cb, XtPointer ok_data,
				   XtCallbackProc nomatch_cb,
				   XtPointer nomatch_data,
				   XtCallbackProc apply_cb,
				   XtPointer apply_data,
				   const char **helptexts,  bool must_match,
				   int visitems, bool managed)
{
   int numitems = items ? items->listlength() : 0 ;
   FrLocalAlloc(char *,table,1000,numitems) ;
   if (!table)
      {
      FrNoMemory("while creating selection box") ;
      return (Widget)0 ;
      }
   int i = 0 ;
   while (items)
      {
      FrObject *item = items->first() ;
      if (item && item->symbolp())
	 table[i] = (char*)XmStringCreateSimple((char*)(*(FrSymbol*)item));
      else if (item && item->stringp())
	 table[i] = (char*)XmStringCreateSimple((char*)(*(FrString*)item)) ;
      else
         {
	 char *str = item->print() ;
	 table[i] = (char*)XmStringCreateSimple(str) ;
	 FrFree(str) ;
	 }
      i++ ;
      items = items->rest() ;
      }
  int n = 0;
  Arg wargs[15];
  XtSetArg(wargs[n],XmNlistItems,numitems ? table : 0); n++;
  XtSetArg(wargs[n],XmNmustMatch,must_match); n++;
  XtSetArg(wargs[n],XmNlistItemCount,numitems); n++;
  XtSetArg(wargs[n],XmNiconName,"Selection") ; n++;
  if (numitems < visitems)
     visitems = numitems ;
  if (visitems <= 0)
     visitems = 1 ;
  XtSetArg(wargs[n],XmNlistVisibleItemCount,visitems); n++ ;
  if (numitems == 1 && must_match)
     {
     XtSetArg(wargs[n],XmNtextString,table[0]) ; n++ ;
     }
  XmString str = XmStringCreateSimple(label) ;
  XtSetArg(wargs[n],XmNlistLabelString,str);n++;
  Widget dialog = XmCreateSelectionDialog(w,"selection",wargs,n);
  XmStringFree(str) ;

  if (ok_cb)
     XtAddCallback(dialog,XmNokCallback, ok_cb, ok_data);
  if (nomatch_cb)
     XtAddCallback(dialog,XmNnoMatchCallback, nomatch_cb, nomatch_data);
  if (apply_cb)
     XtAddCallback(dialog,XmNapplyCallback, apply_cb, apply_data) ;
  else
     XtUnmanageChild(XmSelectionBoxGetChild(dialog,XmDIALOG_APPLY_BUTTON));
  if (helptexts)
    XtAddCallback(dialog,XmNhelpCallback,FrHelp,helptexts) ;
  else
    XtUnmanageChild(XmSelectionBoxGetChild(dialog,XmDIALOG_HELP_BUTTON)) ;
  if (managed)
    XtManageChildren(&dialog,1) ;
  while (--numitems >= 0)
     XtFree(table[numitems]) ;
  FrLocalFree(table) ;
  return dialog ;
}
				
//----------------------------------------------------------------------

FrWSelectionBox::FrWSelectionBox(Widget parent, FrList *items, char *label,
				 XtCallbackProc ok_cb, XtPointer ok_data,
				 XtCallbackProc nomatch_cb, XtPointer nomatch_data,
				 XtCallbackProc apply_cb, XtPointer apply_data,
				 const char **helptexts,
				 bool must_match, int visitems, bool managed)
{
   widget = create_selection_box(parent,items,label,ok_cb,ok_data,
				 nomatch_cb,nomatch_data,apply_cb,apply_data,
				 helptexts,must_match,visitems,managed) ;
   _managed = true ;
}

//----------------------------------------------------------------------

FrWSelectionBox::FrWSelectionBox(FrWidget *parent, FrList *items, char *label,
				 XtCallbackProc ok_cb, XtPointer ok_data,
				 XtCallbackProc nomatch_cb, XtPointer nomatch_data,
				 XtCallbackProc apply_cb, XtPointer apply_data,
				 const char **helptexts,
				 bool must_match, int visitems, bool managed)
{
   widget = create_selection_box(**parent,items,label,ok_cb,ok_data,
				 nomatch_cb,nomatch_data,apply_cb,apply_data,
				 helptexts,must_match,visitems,managed) ;
   _managed = true ;
}

//----------------------------------------------------------------------

FrObjectType FrWSelectionBox::objType() const
{
   return (FrObjectType)OT_FrWSelectionBox ;
}

//----------------------------------------------------------------------

const char *FrWSelectionBox::objTypeName() const
{
   return "FrWSelectionBox" ;
}

//----------------------------------------------------------------------

FrObjectType FrWSelectionBox::objSuperclass() const
{
   return (FrObjectType)OT_FrWidget ;
}

//----------------------------------------------------------------------

void FrWSelectionBox::unmanageChild(FrWSelBoxChild child) const
{
   Widget button ;
   switch (child)
      {
      case SelBox_OK:
	 button = XmSelectionBoxGetChild(widget,XmDIALOG_OK_BUTTON) ;
	 break ;
      case SelBox_Cancel:
	 button = XmSelectionBoxGetChild(widget,XmDIALOG_CANCEL_BUTTON) ;
	 break ;
      case SelBox_Apply:
	 button = XmSelectionBoxGetChild(widget,XmDIALOG_APPLY_BUTTON) ;
	 break ;
      case SelBox_Help:
	 button = XmSelectionBoxGetChild(widget,XmDIALOG_HELP_BUTTON) ;
	 break ;
      default:
	 button = 0 ;
	 break ;
      }
   if (button)
      XtUnmanageChildren(&button,1) ;
}

//----------------------------------------------------------------------

void FrWSelectionBox::buttonLabel(FrWSelBoxChild child, const char *label)
const
{
   XmString str = XmStringCreateSimple((char*)label) ;
   Arg args[2] ;
   int n = 0 ;
   switch (child)
      {
      case SelBox_OK:
	 XtSetArg(args[n], XmNokLabelString, str) ; n++ ;
	 break ;
      case SelBox_Cancel:
	 XtSetArg(args[n], XmNcancelLabelString, str) ; n++ ;
	 break ;
      case SelBox_Apply:
	 XtSetArg(args[n], XmNapplyLabelString, str) ; n++ ;
	 break ;
      case SelBox_Help:
	 XtSetArg(args[n], XmNhelpLabelString, str) ; n++ ;
	 break ;
      default:
	 break ;
      }
   if (n)
     XtSetValues(widget, args, n) ;
   XmStringFree(str) ;
}

//----------------------------------------------------------------------

char *FrWSelectionBox::selectionText(XtPointer call_data)
{
   XmSelectionBoxCallbackStruct *cbs=(XmSelectionBoxCallbackStruct*)call_data ;
   char *text ;
   XmStringGetLtoR(cbs->value, XmSTRING_DEFAULT_CHARSET, &text) ;
   return text ;
}

//----------------------------------------------------------------------

FrObject *FrWSelectionBox::selectionFrObject(XtPointer call_data)
{
   XmSelectionBoxCallbackStruct *cbs=(XmSelectionBoxCallbackStruct*)call_data ;
   char *text ;
   XmStringGetLtoR(cbs->value, XmSTRING_DEFAULT_CHARSET, &text) ;
   FrObject *obj ;
   if (text && *text)
      {
      int len = strlen(text) ;
      char *tmp = text ;
      // skip leading whitespace
      while (Fr_isspace(*tmp))
	 {
	 tmp++ ;
	 len-- ;
	 }
      if (len)
	 {
	 if (tmp[0] == '|' || tmp[0] == '"' || tmp[0] == '\'' || tmp[0] == '`')
	    {
	    // add a vertical bar to ensure that a symbol started with a
	    // vertical bar for quoting can be read correctly, or a quote to
	    // ensure that a string is properly terminated
	    FrLocalAlloc(char,objstring,512,len+2) ;
	    memcpy(objstring,tmp,len) ;
	    objstring[len] = tmp[0] ;
	    objstring[len+1] = '\0' ;
	    tmp = objstring ;
	    obj = string_to_FrObject(tmp) ;
	    FrLocalFree(objstring) ;
	    }
	 else
	    obj = string_to_FrObject(tmp) ;
	 }
      else
 	 obj = 0 ;
      }
   else
      obj = 0 ;
   XtFree(text) ;
   return obj ;
}

/************************************************************************/
/*    Methods for class FrWFramePrompt					*/
/************************************************************************/

//----------------------------------------------------------------------

static int symbol_compare(const FrObject *o1, const FrObject *o2)
{
   if (!o1 || !o1->symbolp())
      return -1 ;
   else if (!o2 || !o2->symbolp())
      return 1 ;
   else
      return strcmp(((FrSymbol*)o1)->symbolName(),
		    ((FrSymbol*)o2)->symbolName()) ;
}

//----------------------------------------------------------------------

static FrList *collect_continuations(FrList *matches, int prefixlen,
				     bool quoted, int *continuations)
{
  int num_cont ;
  FrList *match ;
  FrList *cont = 0 ;
  bool done = false ;
  int len = 0 ;
  do {
     char prefix[FrMAX_SYMBOLNAME_LEN+3] ;
     len++ ;
     strncpy(prefix,((FrSymbol*)matches->first())->symbolName()+prefixlen,
	     sizeof(prefix)) ;
     num_cont = 1 ;
     if (prefix[len] == '\0')
	done = true ;
     for (match = matches->rest() ; match ; match = match->rest())
	{
	const char *name = ((FrSymbol*)match->first())->symbolName() + prefixlen;
	if (strncmp(prefix,name,len) != 0 ||
	    prefix[len] == '\0' || name[len] == '\0')
	   {
	   num_cont++ ;
	   strncpy(prefix,name,sizeof(prefix)) ;
	   if (prefix[len] == '\0')
	      done = true ;
	   }
	}
     } while (num_cont < MAX_CONTINUATIONS && !done) ;
  for (match = matches ; match ; match = match->rest())
     {
     const char *name1 = ((FrSymbol*)match->first())->symbolName() + prefixlen ;
     const char *name2 = match->rest()
       			    ? ((FrSymbol*)match->rest()->first())->symbolName() +
			      prefixlen
			    : "" ;
     if (!match->rest() ||
	 strncmp(name1,name2,len) != 0 ||
	 name1[len] == '\0' || name2[len] == '\0')
	{
	FrSymbol *contsym = (FrSymbol*)(match->first()) ;
	char buf[FrMAX_SYMBOLNAME_LEN+3] ;
	strncpy(buf,contsym->symbolName(),sizeof(buf)) ;
	if (buf[prefixlen+len] != '\0')
	   strncpy(buf+prefixlen+len,CONTINUATION_MARKER,
		   sizeof(buf)-prefixlen-len) ;
	else if (quoted)
	   strncpy(buf+prefixlen+len,"|",
		   sizeof(buf)-prefixlen-len) ;
	pushlist(new FrString(buf),cont) ;
	}
     }
  if (continuations)
    *continuations = num_cont ;
  return listreverse(cont) ;
}

//----------------------------------------------------------------------

static void continuation_ok_cb(Widget,XtPointer client_data,XtPointer calldata)
{
   XmSelectionBoxCallbackStruct *cbs = (XmSelectionBoxCallbackStruct*)calldata;
   Widget text = (Widget)client_data ;
   char *selection ;
   XmStringGetLtoR(cbs->value,XmFONTLIST_DEFAULT_TAG,&selection) ;
   if (selection && *selection)
     {
     if (strcmp(selection+strlen(selection)-sizeof(CONTINUATION_MARKER)+1,
		CONTINUATION_MARKER) == 0)
        // strip trailing " ..."
	selection[strlen(selection)-sizeof(CONTINUATION_MARKER)+1] = '\0' ;
     XmTextSetString(text,selection) ;
     XmTextSetInsertionPosition(text,strlen(selection)) ;
     }
   XmStringFree((XmString)selection) ;
   XmTextSetEditable(text,true) ;
}

//----------------------------------------------------------------------

static void completion_ok_cb(Widget,XtPointer client_data,XtPointer calldata)
{
   XmSelectionBoxCallbackStruct *cbs = (XmSelectionBoxCallbackStruct*)calldata;
   Widget text = (Widget)client_data ;
   char *selection ;
   XmStringGetLtoR(cbs->value,XmFONTLIST_DEFAULT_TAG,&selection) ;
   if (selection && *selection)
      {
      int len = strlen(selection) ;
      if (FrSymbol::nameNeedsQuoting(selection))
         {
	 FrLocalAlloc(char,name,256,len+2) ;
	 name[0] = '|' ;
	 memcpy(name+1,selection,len+1) ;
	 XmTextSetString(text,name) ;
	 FrLocalFree(name) ;
	 }
      else
	 {
	 XmTextSetString(text,selection) ;
         }
      XmTextSetInsertionPosition(text,len) ;
      }
   XmStringFree((XmString)selection) ;
   XmTextSetEditable(text,true) ;
}

//----------------------------------------------------------------------

static void completion_cancel_cb(Widget, XtPointer client_data, XtPointer)
{
   XmTextSetEditable((Widget)client_data,true) ;
}

//----------------------------------------------------------------------

static void frame_completion_cb(Widget w, XtPointer client_data,
				XtPointer call_data)
{
   XmTextVerifyCallbackStruct *cbs = (XmTextVerifyCallbackStruct*)call_data ;

   if (cbs->reason == XmCR_ACTIVATE)
     {

     return ;
     }
   if (cbs->reason != XmCR_MODIFYING_TEXT_VALUE)
      return ;
   FrSymbolTable *oldsym = ((FrSymbolTable*)client_data)->select() ;
   char completion ;
   if (cbs->text->length == 1 && cbs->text->ptr &&
       ((completion = cbs->text->ptr[0] == '\t') || completion == ' '))
     {
     char *prefix = XmTextGetString(w) ;
     char *currprefix ;
     if (!prefix || strlen(prefix) < 2)
        {
	cbs->doit = false ;
	return ;
        }
     cbs->doit = true ;
     if (prefix[0] != '|')
        {
	for (currprefix = prefix ; *currprefix ; currprefix++)
	   *currprefix = Fr_toupper(*currprefix) ;
	currprefix = prefix ;
	XmTextSetString(w,prefix) ;
	}
     else
        currprefix = prefix+1 ;
     int prefixlen = strlen(prefix) ;
     XmTextSetInsertionPosition(w,prefixlen) ;
     cbs->newInsert = prefixlen ;
     char maxprefix[FrMAX_SYMBOLNAME_LEN+3] ;
     FrList *matches = collect_prefix_matching_frames(currprefix,maxprefix,
						      sizeof(maxprefix)) ;
     if (matches)
        {
	int currlen = strlen(currprefix) ;
	int maxlen = strlen(maxprefix) ;
	if (maxlen > currlen)
	  {
	  cbs->text->length = (maxlen - currlen) ;
	  cbs->text->ptr = XtMalloc(maxlen - currlen + 1) ;
	  strcpy(cbs->text->ptr,maxprefix+currlen) ;
	  cbs->startPos = prefixlen ;
	  cbs->endPos = prefixlen ;
	  cbs->newInsert += (maxlen-currlen) ;
	  }
	else
	  {
	  cbs->text->length = 0 ;
	  XtFree(cbs->text->ptr) ;
	  cbs->text->ptr = 0 ;
	  }
	// check how many possibilities are left, and pop up a selection
	// box if more than one
	int num_matches = matches->length() ;
	Widget box = 0 ;
	if (num_matches)
	   matches = matches->sort(symbol_compare) ;
	const char **helptexts = FramepaC_helptexts[FrHelptext_completion] ;
	if (num_matches > MAX_COMPLETIONS)
	  {
	  FrList *matchlist = collect_continuations(matches,currlen,
						    prefix[0] == '|',
						    &num_matches) ;
	  box = create_selection_box(w, matchlist, "Possible Continuations",
				     continuation_ok_cb, w,  // Ok
				     continuation_ok_cb, w,  // nomatch
				     0, 0,   // apply
				     helptexts, true, 15, true) ;
	  free_object(matchlist) ;
	  }
	else if (num_matches > 1)
	  {
	  box = create_selection_box(w, matches, "Possible Completions",
				     completion_ok_cb, w,  // Ok
				     completion_ok_cb, w,  // nomatch
				     0, 0,   // apply
				     helptexts, true, 15, true) ;
	  }
	if (num_matches > 1)
	  {
	  XtAddCallback(box,XmNcancelCallback,completion_cancel_cb,w) ;
	  XtAddCallback(box,XmNapplyCallback,completion_cancel_cb,w) ;
	  // disable editing on the text widget until selection box done
	  XmTextSetEditable(w,false) ;
	  }
        }
     else
       {
       XBell(XtDisplay(w),0) ;
       cbs->doit = false ;
       cbs->newInsert = prefixlen ;
       }
     free_object(matches) ;
     XtFree(prefix) ;
     }
   oldsym->select() ;
   return ;
}

//----------------------------------------------------------------------

static Widget create_frame_prompt_popup(Widget parent,
					const char *label, const char *def,
					XtCallbackProc ok_cb, XtPointer ok_data,
					XtPointer help_data,
					bool auto_unmanage, bool modal,
					bool managed)
{
   Widget dialog = create_prompt(parent,"Frame Prompt",label,def,ok_cb,
				 ok_data,help_data,
				 auto_unmanage,modal,false,"frame_prompt") ;
   set_icon_name(dialog,"Frame Select") ;
   Widget text = XmSelectionBoxGetChild(dialog,XmDIALOG_TEXT) ;
   XtAddCallback(text,XmNmodifyVerifyCallback,
		 frame_completion_cb,(XtPointer)FrSymbolTable::current()) ;
   XtTranslations completionTranslations ;
   completionTranslations = XtParseTranslationTable(completion_translations) ;
   XtOverrideTranslations(text,completionTranslations) ;
   if (managed)
      XtManageChildren(&dialog,1);
   return dialog ;
}

//----------------------------------------------------------------------

FrWFramePrompt::FrWFramePrompt(Widget parent, const char *label,
			       const char *def,
			       XtCallbackProc ok_cb, XtPointer ok_data,
			       XtPointer help_data,
			       bool auto_unmanage, bool modal, bool managed)
{
   widget = create_frame_prompt_popup(parent, label, def, ok_cb, ok_data,
				      help_data, auto_unmanage, modal,
				      managed) ;
   _managed = managed ;
   if (modal && managed)
      warpPointerCenter() ;
   return ;
}

//----------------------------------------------------------------------

FrWFramePrompt::FrWFramePrompt(FrWidget *parent, const char *label,
			       const char *def,
			       XtCallbackProc ok_cb, XtPointer ok_data,
			       XtPointer help_data,
			       bool auto_unmanage, bool modal,
			       bool managed)
{
   widget = create_frame_prompt_popup(**parent, label, def, ok_cb, ok_data,
				      help_data, auto_unmanage, modal,
				      managed) ;
   _managed = managed ;
   if (modal && managed)
      warpPointerCenter() ;
   return ;
}

//----------------------------------------------------------------------

FrObjectType FrWFramePrompt::objType() const
{
   return (FrObjectType)OT_FrWFramePrompt ;
}

//----------------------------------------------------------------------

const char *FrWFramePrompt::objTypeName() const
{
   return "FrWFramePrompt" ;
}

//----------------------------------------------------------------------

FrObjectType FrWFramePrompt::objSuperclass() const
{
   return (FrObjectType)OT_FrWidget ;
}

//----------------------------------------------------------------------

void FrWFramePrompt::setFrameName(FrSymbol *framename) const
{
   Widget text = XmSelectionBoxGetChild(widget,XmDIALOG_TEXT) ;
   if (text)
      {
      FrWText tw(text) ;
      const char *name = framename ? framename->symbolName() : "" ;
      tw.setText(name,true) ;
      tw.retain() ;
      }
   return ;
}

/************************************************************************/
/*    Methods for class FrWPushButton					*/
/************************************************************************/

static Widget create_pushbutton(Widget parent, const char *name, bool managed,
				bool centered, const char *pbclass)
{
   if (!name)
      name = " " ;
   XmString namestring = XmStringCreateSimple((char*)name) ;
   Arg args[3] ;
   int n = 1 ;
   XtSetArg(args[0], XmNlabelString, namestring) ;
#if 1
   XtSetArg(args[1], XmNalignment, centered ? XmALIGNMENT_CENTER
	       				    : XmALIGNMENT_BEGINNING) ; n++ ;
#else
   if (centered)
      {
      XtSetArg(args[1], XmNalignment, XmALIGNMENT_CENTER) ; n++ ;
      }
#endif
   Widget w ;
   if (!pbclass || *pbclass == '\0')
      pbclass = str_pushbutton ;
   if (managed)
      w = XtCreateManagedWidget(pbclass,xmPushButtonWidgetClass,parent,
				args,n) ;
   else
      w = XtCreateWidget(pbclass,xmPushButtonWidgetClass,parent,args,n);
   XmStringFree(namestring) ;
   return w ;
}

//----------------------------------------------------------------------

FrWPushButton::FrWPushButton(Widget parent, const char *label, bool managed,
			     bool centered, const char *pbclass)
{
   widget = create_pushbutton(parent, label, managed, centered, pbclass) ;
   _managed = managed ;
   return ;
}

//----------------------------------------------------------------------

FrWPushButton::FrWPushButton(FrWidget *parent, const char *label, bool managed,
			     bool centered, const char *pbclass)
{
   widget = create_pushbutton(**parent, label, managed, centered, pbclass) ;
   _managed = managed ;
   return ;
}

//----------------------------------------------------------------------

FrObjectType FrWPushButton::objType() const
{
   return (FrObjectType)OT_FrWPushButton ;
}

//----------------------------------------------------------------------

const char *FrWPushButton::objTypeName() const
{
   return "FrWPushButton" ;
}

//----------------------------------------------------------------------

FrObjectType FrWPushButton::objSuperclass() const
{
   return (FrObjectType)OT_FrWidget ;
}

//----------------------------------------------------------------------

void FrWPushButton::setAlignment(int align) const
{
   Arg args[2] ;
   int a ;
   if (align < 0)
      a = XmALIGNMENT_BEGINNING ;
   else if (align > 0)
      a = XmALIGNMENT_END ;
   else // align == 0
      a = XmALIGNMENT_CENTER ;
   XtSetArg(args[0], XmNalignment, a) ;
   XtSetValues(widget, args, 1) ;
   return ;
}

//----------------------------------------------------------------------

void FrWPushButton::setMnemonic(char mnem) const
{
   Arg args[1] ;
   XtSetArg(args[0], XmNmnemonic, mnem) ;
   XtSetValues(widget, args, 1) ;
   return ;
}

//----------------------------------------------------------------------

void FrWPushButton::setDualMouseButton() const
{
   // This code is written to make use of the left and right mouse buttons
   XtTranslations xlat = XtParseTranslationTable(blockPBTranslationTable2) ;
   XtOverrideTranslations(widget,xlat) ;
   return ;
}

//----------------------------------------------------------------------

void FrWPushButton::setTripleMouseButton() const
{
   // This code is written to make use of all three buttons of the mouse
   XtTranslations xlat = XtParseTranslationTable(blockPBTranslationTable3) ;
   XtOverrideTranslations(widget,xlat) ;
   return ;
}

//----------------------------------------------------------------------

int FrWPushButton::pressedButton(XtPointer call_data)
{
   XmPushButtonCallbackStruct *cbs = (XmPushButtonCallbackStruct*)call_data ;
   if (cbs->event->xbutton.button == Button1)
      return 1 ;
   else if (cbs->event->xbutton.button == Button2)
      return 2 ;
   else if (cbs->event->xbutton.button == Button3)
      return 3 ;
   else
      return 0 ;
}

/************************************************************************/
/*    Methods for class FrWPushButtonG					*/
/************************************************************************/

static Widget create_pushbutton_gadget(Widget parent, const char *name,
				       bool managed, bool centered,
				       const char *pbclass)
{
   if (!name)
      name = " " ;
   XmString namestring = XmStringCreateSimple((char*)name) ;
   Arg args[3] ;
   int n = 2 ;
   XtSetArg(args[0], XmNlabelString, namestring) ;
   XtSetArg(args[1], XmNalignment, centered ? XmALIGNMENT_CENTER
	       				    : XmALIGNMENT_BEGINNING) ;
   Widget w ;
   if (!pbclass || *pbclass == '\0')
      pbclass = str_pushbutton ;
   if (managed)
      w = XtCreateManagedWidget(pbclass,xmPushButtonGadgetClass,parent,
				args,n) ;
   else
      w = XtCreateWidget(pbclass,xmPushButtonGadgetClass,parent,args,n);
   XmStringFree(namestring) ;
   return w ;
}

//----------------------------------------------------------------------

FrWPushButtonG::FrWPushButtonG(Widget parent, const char *label, bool managed,
			       bool centered, const char *pbclass)
{
   widget = create_pushbutton_gadget(parent, label, managed, centered,
				     pbclass) ;
   _managed = managed ;
   return ;
}

//----------------------------------------------------------------------

FrWPushButtonG::FrWPushButtonG(FrWidget *parent, const char *label,
			       bool managed, bool centered,
			       const char *pbclass)
{
   widget = create_pushbutton_gadget(**parent, label, managed, centered,
				     pbclass) ;
   _managed = managed ;
   return ;
}

//----------------------------------------------------------------------

FrObjectType FrWPushButtonG::objType() const
{
   return (FrObjectType)OT_FrWPushButtonG ;
}

//----------------------------------------------------------------------

const char *FrWPushButtonG::objTypeName() const
{
   return "FrWPushButtonG" ;
}

/************************************************************************/
/*    Methods for class FrWToggleButton					*/
/************************************************************************/

static Widget create_togglebutton(Widget parent, const char *label, bool set,
				  bool managed)
{
   if (!label)
      label = " " ;
   XmString namestring = XmStringCreateSimple((char*)label) ;
   Arg args[2] ;
   int n = 2 ;
   XtSetArg(args[0], XmNlabelString, namestring) ;
   XtSetArg(args[1], XmNset, set) ;
   Widget w ;
   if (managed)
      w = XtCreateManagedWidget(str_togglebutton,xmToggleButtonWidgetClass,
				parent,args,n) ;
   else
      w = XtCreateWidget(str_togglebutton,xmPushButtonWidgetClass,parent,
			 args,n) ;
   XmStringFree(namestring) ;
   return w ;
}

//----------------------------------------------------------------------

FrWToggleButton::FrWToggleButton(Widget parent, const char *label, bool set,
				 bool managed)
{
   widget = create_togglebutton(parent, label, set, managed) ;
   changedCallback(&FrWToggleButton::activateCB,this,true) ;
   _managed = managed ;
   toggle_set = set ;
   return ;
}

//----------------------------------------------------------------------

FrWToggleButton::FrWToggleButton(FrWidget *parent, const char *label, bool set,
				 bool managed)
{
   widget = create_togglebutton(**parent, label, set, managed) ;
   changedCallback(&FrWToggleButton::activateCB,this,true) ;
   _managed = managed ;
   toggle_set = set ;
   return ;
}

//----------------------------------------------------------------------

void FrWToggleButton::setState(bool state)
{
   toggle_set = state ;
   Arg args[1] ;
   XtSetArg(args[0], XmNset, state) ;
   XtSetValues(widget, args, 1) ;
   return ;
}

//----------------------------------------------------------------------

void FrWToggleButton::alwaysVisible(bool vis)
{
   Arg args[1] ;
   XtSetArg(args[0], XmNvisibleWhenOff,vis) ;
   XtSetValues(widget, args, 1) ;
   return ;
}

//----------------------------------------------------------------------

void FrWToggleButton::activateCB(Widget, XtPointer cldata,XtPointer calldata)
{
   FrWToggleButton *but = (FrWToggleButton*)cldata ;
   XmToggleButtonCallbackStruct *cbs = (XmToggleButtonCallbackStruct*)calldata ;
   but->toggle_set = cbs->set ;
   return ;
}

//----------------------------------------------------------------------

FrObjectType FrWToggleButton::objType() const
{
   return (FrObjectType)OT_FrWToggleButton ;
}

//----------------------------------------------------------------------

const char *FrWToggleButton::objTypeName() const
{
   return "FrWToggleButton" ;
}

/************************************************************************/
/*    Methods for class FrWCascadeButton				*/
/************************************************************************/

static Widget create_cascade_button(Widget parent, const char *label,
				    char mnemonic, Widget submenuID)
{
   XmString str = XmStringCreateSimple((char*)label) ;
   Widget button = XtVaCreateManagedWidget("cascade_button",
					   xmCascadeButtonWidgetClass,
					   parent,
					   XmNlabelString, str,
					   XmNmnemonic, mnemonic,
					   XmNsubMenuId, submenuID,
					   NULL) ;
   XmStringFree(str) ;
   return button ;
}

//----------------------------------------------------------------------

FrWCascadeButton::FrWCascadeButton(FrWidget *parent, const char *label,
				   char mnemonic, FrWidget *submenuID)
{
   widget = create_cascade_button(**parent,label,mnemonic,**submenuID) ;
   _managed = true ;
   return ;
}

//----------------------------------------------------------------------

FrWCascadeButton::FrWCascadeButton(Widget parent, const char *label,
				   char mnemonic, Widget submenuID)
{
   widget = create_cascade_button(parent,label,mnemonic,submenuID) ;
   _managed = true ;
   return ;
}

//----------------------------------------------------------------------

FrObjectType FrWCascadeButton::objType() const
{
   return (FrObjectType)OT_FrWCascadeButton ;
}

//----------------------------------------------------------------------

const char *FrWCascadeButton::objTypeName() const
{
   return "FrWCascadeButton" ;
}

//----------------------------------------------------------------------

FrObjectType FrWCascadeButton::objSuperclass() const
{
   return OT_FrWidget ;
}

/************************************************************************/
/*    Methods for class FrWForm						*/
/************************************************************************/

static Widget create_form(Widget parent,bool managed,const char *name,
			  const char *icon_name)
{
   Arg args[2] ;
   XtSetArg(args[0],XmNiconName,icon_name ? icon_name : "Form") ;
   XtSetArg(args[1],XmNtitle,icon_name ? icon_name : "FramepaC Form") ;
   if (!name || *name == '\0')
      name = "form" ;
   if (managed)
      return XtCreateManagedWidget(name, xmFormWidgetClass, parent, args, 2) ;
   else
      return XtCreateWidget(name, xmFormWidgetClass, parent, args, 2) ;
}

//----------------------------------------------------------------------

FrWForm::FrWForm(Widget parent, const char *name, bool managed,
		 const char *icon_name)
{
   widget = create_form(parent,managed,name,icon_name) ;
   _managed = managed ;
   return ;
}

//----------------------------------------------------------------------

FrWForm::FrWForm(FrWidget *parent, const char *name, bool managed,
		 const char *icon_name)
{
   widget = create_form(**parent,managed,name,icon_name) ;
   _managed = managed ;
   return ;
}

//----------------------------------------------------------------------

FrObjectType FrWForm::objType() const
{
   return (FrObjectType)OT_FrWForm ;
}

//----------------------------------------------------------------------

const char *FrWForm::objTypeName() const
{
   return "FrWForm" ;
}

//----------------------------------------------------------------------

FrObjectType FrWForm::objSuperclass() const
{
   return (FrObjectType)OT_FrWidget ;
}

/************************************************************************/
/*    Methods for class FrWSlider					*/
/************************************************************************/

static Widget create_slider_widget(Widget parent, char * display_string,
				   int max, int min, int def_value,
				   int precision, bool vertical)
{
   int orientation = (vertical ? XmVERTICAL : XmHORIZONTAL) ;
   if (max <= min)
      max = min+1 ;
   if (def_value < min)
      def_value = min ;
   else if (def_value > max)
      def_value = max ;
   if (precision < 0)
      precision = 0 ;
   return XtVaCreateManagedWidget("slider",
				  xmScaleWidgetClass, parent,
				  XtVaTypedArg, XmNtitleString,
				  XmRString, display_string, 4,
				  XmNmaximum, max,
				  XmNminimum, min,
				  XmNvalue, def_value,
				  XmNshowValue, true,
				  XmNdecimalPoints, precision,
				  XmNorientation, orientation,
				  NULL);
}

//----------------------------------------------------------------------

FrWSlider::FrWSlider(Widget parent, const char *name, int max, int min,
		     int def_value, int precision, bool vertical)
{
   widget = create_slider_widget(parent,(char*)name,max,min,def_value,
				 precision, vertical) ;
   _managed = true ;
   return ;
}

//----------------------------------------------------------------------

FrWSlider::FrWSlider(FrWidget *parent, const char *name, int max, int min,
		     int def_value, int precision, bool vertical)
{
   widget = create_slider_widget(**parent,(char*)name,max,min,def_value,
				 precision, vertical) ;
   _managed = true ;
   return ;
}

//----------------------------------------------------------------------

FrObjectType FrWSlider::objType() const
{
   return (FrObjectType)OT_FrWSlider ;
}

//----------------------------------------------------------------------

const char *FrWSlider::objTypeName() const
{
   return "FrWSlider" ;
}

//----------------------------------------------------------------------

FrObjectType FrWSlider::objSuperclass() const
{
   return (FrObjectType)OT_FrWidget ;
}

//----------------------------------------------------------------------

int FrWSlider::sliderValue(XtPointer call_data)
{
   XmScaleCallbackStruct *cbs = (XmScaleCallbackStruct*)call_data ;
   return cbs->value ;
}

/************************************************************************/
/*    Methods for class FrWButtonBar					*/
/************************************************************************/

static Widget create_button_bar(Widget parent,
				const FrButtonsAndCommands *buttonlist,
				int bcount, bool managed,
				XtPointer default_data, FrList *&buttons)
{
   Arg args[6] ;
   int n ;

   XtSetArg(args[0],XmNfractionBase,bcount) ;
   Widget form = XmCreateForm(parent,"buttonbar",args,1) ;

   n = 0 ;
   XtSetArg(args[n],XmNtopAttachment, XmATTACH_FORM) ; n++ ;
   XtSetArg(args[n],XmNbottomAttachment, XmATTACH_FORM) ; n++ ;
   XtSetArg(args[n],XmNleftAttachment, XmATTACH_POSITION) ; n++ ;
   XtSetArg(args[n],XmNrightAttachment, XmATTACH_POSITION) ; n++ ;
   buttons = 0 ;
   for (int i = 0 ; i < bcount ; i++)
      {
      XtSetArg(args[n],XmNleftPosition, i) ;
      XtSetArg(args[n+1],XmNrightPosition, i+1) ;
      Widget but = XmCreatePushButton(form,buttonlist[i].name,args,n+2) ;
      FrWPushButton *button = new FrWPushButton(but) ;
      if (buttonlist[i].func)
	 {
	 XtPointer data = buttonlist[i].data ;
	 if (!data && default_data)
	   data = default_data ;
	 button->activateCallback(buttonlist[i].func, data) ;
	 }
      if (!buttonlist[i].active)
	 button->setSensitive(false) ;
      pushlist(button,buttons) ;
      button->manage() ;
      }
   if (managed)
      XtManageChildren(&form,1);
   buttons = listreverse(buttons) ;
   return form ;
}

//----------------------------------------------------------------------

FrWButtonBar::FrWButtonBar(Widget parent,
			   const FrButtonsAndCommands *buttonlist,
			   int bcount, bool managed, XtPointer default_data)
{
   widget = create_button_bar(parent,buttonlist,bcount,managed,default_data,
			      buttons) ;
   _managed = managed ;
   return ;
}

//----------------------------------------------------------------------

FrWButtonBar::FrWButtonBar(FrWidget *parent,
			   const FrButtonsAndCommands *buttonlist,
			   int bcount, bool managed, XtPointer default_data)
{
   widget = create_button_bar(**parent,buttonlist,bcount,managed,default_data,
			      buttons) ;
   _managed = managed ;
   return ;
}

//----------------------------------------------------------------------

FrWButtonBar::~FrWButtonBar()
{
   while (buttons)
      {
      FrWPushButton *but = (FrWPushButton*)poplist(buttons) ;
      but->keep() ;
      }
   return ;
}

//----------------------------------------------------------------------

FrObjectType FrWButtonBar::objType() const
{
   return (FrObjectType)OT_FrWButtonBar ;
}

//----------------------------------------------------------------------

const char *FrWButtonBar::objTypeName() const
{
   return "FrWButtonBar" ;
}

//----------------------------------------------------------------------

FrObjectType FrWButtonBar::objSuperclass() const
{
   return (FrObjectType)OT_FrWidget ;
}

/************************************************************************/
/*    Methods for class FrWText						*/
/************************************************************************/

static void text_cursor_cb(Widget w, XtPointer client_data, XtPointer)
{
   bool cursor_visible = (bool)client_data ;
   Arg args[1] ;
   XtSetArg(args[0], XmNcursorPositionVisible, cursor_visible) ;
   XtSetValues(w, args, 1) ;
   return ;
}

//----------------------------------------------------------------------

static Widget create_text(Widget parent, const char *text, int columns,
			  int rows, bool editable, bool traversal, bool wrap,
			  const char *name)
{
   Arg args[15] ;
   int n = 0 ;
   if (!text)
      text = "" ;
   bool hscroll = (rows > 1) && !wrap ;
   bool vscroll = (rows > 1) ;
   XtSetArg(args[n],XmNeditable,editable) ; n++ ;
   XtSetArg(args[n],XmNeditMode, (rows > 1) ? XmMULTI_LINE_EDIT
					    : XmSINGLE_LINE_EDIT) ; n++ ;
   XtSetArg(args[n],XmNscrollVertical, vscroll) ; n++ ;
   XtSetArg(args[n],XmNscrollHorizontal, hscroll) ; n++ ;
   XtSetArg(args[n],XmNcursorPositionVisible, editable) ; n++ ;
   XtSetArg(args[n],XmNwordWrap, !hscroll) ; n++ ;
   XtSetArg(args[n],XmNvalue,text) ; n++ ;
   XtSetArg(args[n],XmNcolumns, columns) ; n++ ;
   XtSetArg(args[n],XmNrows, rows) ; n++ ;
   XtSetArg(args[n],XmNcursorPositionVisible, false) ; n++ ;
(void)traversal ; //!!!
   if (!name)
      name = "text" ;
   Widget textw ;
   if (hscroll || vscroll)
      textw = XmCreateScrolledText(parent, (char*)name, args, n) ;
   else
      textw = XmCreateText(parent, (char*)name, args, n) ;
   XtAddCallback(textw,XmNfocusCallback,text_cursor_cb,(XtPointer)true) ;
   XtAddCallback(textw,XmNlosingFocusCallback,
		 text_cursor_cb,(XtPointer)false) ;
   return textw ;
}

//----------------------------------------------------------------------

FrWText::FrWText(Widget parent, const char *text, int columns, int rows,
		 bool editable, bool traversal, bool wrap, const char *name)
{
   widget = create_text(parent,text,columns,rows,editable,traversal,wrap,name);
   _managed = false ;
   verify_cb = 0 ;
   return ;
}

//----------------------------------------------------------------------

FrWText::FrWText(FrWidget *parent, const char *text, int columns, int rows,
		 bool editable, bool traversal, bool wrap, const char *name)
{
   widget = create_text(**parent,text,columns,rows,editable,traversal,wrap,
			name) ;
   _managed = false ;
   verify_cb = 0 ;
   return ;
}

//----------------------------------------------------------------------

FrObjectType FrWText::objType() const
{
   return (FrObjectType)OT_FrWText ;
}

//----------------------------------------------------------------------

const char *FrWText::objTypeName() const
{
   return "FrWText" ;
}

//----------------------------------------------------------------------

FrObjectType FrWText::objSuperclass() const
{
   return (FrObjectType)OT_FrWidget ;
}

//----------------------------------------------------------------------

char *FrWText::getText() const
{
   return XmTextGetString(widget) ;
}

//----------------------------------------------------------------------

void FrWText::getText(char *buf, int buflen) const
{
   char *text = XmTextGetString(widget) ;
   strncpy(buf,text,buflen) ;
   buf[buflen-1] = '\0' ;
   XtFree(text) ;
   return ;
}

//----------------------------------------------------------------------

void FrWText::setText(const char *t, bool to_end)
{
   XmTextSetString(widget,(char*)t) ;
   if (to_end)
      XmTextSetInsertionPosition(widget,strlen(t)) ;
   return ;
}

//----------------------------------------------------------------------

static void text_limit_length_cb(Widget, XtPointer client_data,
				 XtPointer call_data)
{
   XmTextVerifyCallbackStruct *cbs = (XmTextVerifyCallbackStruct*)call_data ;
   if (cbs->reason != XmCR_MODIFYING_TEXT_VALUE)
      return ;
   FrWText *text = (FrWText*)client_data ;
   if (text->lengthLimit() <= 0)
      return ;		// unlimited
   int curlen = XmTextGetLastPosition(**text) ;
   if (curlen + cbs->text->length - (cbs->endPos - cbs->startPos) >
       text->lengthLimit())
      {
      FrSoundBell(text,0) ;
      cbs->doit = false ;
      }
   else
      cbs->doit = true ;
   return ;
}

//----------------------------------------------------------------------

void FrWText::limitLength(int maxlen)
{
   if (maxlen < 0)
      return ;
   if (max_length == 0)
      {
      FrWidget::verifyCallback(text_limit_length_cb,this) ;
      }
   max_length = maxlen ;
   return ;
}

//----------------------------------------------------------------------

static void text_verify_cb(Widget, XtPointer client_data, XtPointer call_data)
{
   XmTextVerifyCallbackStruct *cbs = (XmTextVerifyCallbackStruct*)call_data ;
   FrWText *w = (FrWText*)client_data ;
   FrWText_VerifyFunc *cb = w->getVerifyCallback() ;
   if (cb)
      {
      char *newchars ;
      if (cbs->text->ptr && cbs->text->length)
	 {
	 newchars = FrNewN(char,cbs->text->length+1) ;
	 memcpy(newchars,cbs->text->ptr,cbs->text->length) ;
	 newchars[cbs->text->length] = '\0' ;
	 }
      else
	 newchars = 0 ;
      char *display = 0 ;
      cbs->doit = cb(w, newchars, cbs->startPos, cbs->endPos, &display) ;
      if (display)
	 {
	 XtFree(cbs->text->ptr) ;
	 cbs->text->length = strlen(display) ;
	 cbs->text->ptr = XtMalloc(cbs->text->length) ;
	 memcpy(cbs->text->ptr,display,cbs->text->length) ;
	 }
      if (display != newchars)
	 FrFree(display) ;
      FrFree(newchars) ;
      }
   else
      cbs->doit = true ;
   return ;
}

//----------------------------------------------------------------------

void FrWText::setSensitive(bool sensitive) const
{
   Arg args[1] ;
   XtSetArg(args[0], XmNeditable, sensitive) ;
   XtSetValues(widget, args, 1) ;
   return ;
}

//----------------------------------------------------------------------

void FrWText::verifyCallback(FrWText_VerifyFunc *verify)
{
   if (!verify_cb && verify)
     FrWidget::verifyCallback(text_verify_cb,this) ;
   else if (verify_cb && !verify)
     FrWidget::removeCallback(XmNmodifyVerifyCallback,text_verify_cb,this) ;
   verify_cb = verify ;
   return ;
}

/************************************************************************/
/*    Methods for class FrWShadowText					*/
/************************************************************************/

static Widget create_shadow_text(Widget parent, const char *text,
				 int columns, int rows, bool editable,
				 bool traversal, bool wrap, const char *name)
{
   if (!text)
      text = "" ;
   size_t numstars = strlen(text) + 1 ;
   FrLocalAlloc(char,stars,256,numstars) ;
   for (int i = strlen(text)-1 ; i >= 0 ; i--)
      stars[i] = '*' ;
   Widget w = create_text(parent,stars,columns,rows,editable,traversal,wrap,
			  name) ;
   FrLocalFree(stars) ;
   return w ;
}

//----------------------------------------------------------------------

void shadow_text_verify_cb(Widget, XtPointer client_data, XtPointer call_data)
{
   XmTextVerifyCallbackStruct *cbs = (XmTextVerifyCallbackStruct*)call_data ;
   FrWShadowText *w = (FrWShadowText*)client_data ;
   FrWText_VerifyFunc *cb = w->getVerifyCallback() ;
   if (cb)
      {
      char *newchars ;
      if (cbs->text->ptr && cbs->text->length)
	 {
	 newchars = FrNewN(char,cbs->text->length+1) ;
	 memcpy(newchars,cbs->text->ptr,cbs->text->length) ;
	 newchars[cbs->text->length] = '\0' ;
	 }
      else
	 newchars = 0 ;
      char *display = 0 ;
      cbs->doit = cb(w, newchars, cbs->startPos, cbs->endPos, &display) ;
      if (display)
	 {
	 XtFree(cbs->text->ptr) ;
	 cbs->text->length = strlen(display) ;
	 cbs->text->ptr = XtMalloc(cbs->text->length) ;
	 memcpy(cbs->text->ptr,display,cbs->text->length) ;
	 }
      if (display != newchars)
	 FrFree(display) ;
      FrFree(newchars) ;
      }
   else
      cbs->doit = true ;
   if (cbs->doit)
      {
      int newlen = strlen(w->shadow_text) + cbs->text->length + 1
			           - (cbs->endPos - cbs->startPos) ;
      if (!w->shadow_text)
	 {
	 w->shadow_text = FrNewN(char,newlen) ;
	 w->shadow_text[0] = '\0' ;
	 }
      if (!cbs->text->ptr || !cbs->text->length)
	 strcpy(w->shadow_text + cbs->startPos, w->shadow_text + cbs->endPos) ;
      else if (cbs->startPos < (int)strlen(w->shadow_text)-1)
	 {
	 int offset = cbs->text->length ;
	 char *new_shadow = (char*)FrRealloc(w->shadow_text,newlen) ;
	 if (new_shadow)
	    {
	    w->shadow_text = new_shadow ;
	    for (int i = strlen(w->shadow_text)-1 ; i >= cbs->endPos ; i--)
	       w->shadow_text[i] = w->shadow_text[i-offset] ;
	    strncpy(w->shadow_text+cbs->startPos,
		    cbs->text->ptr,cbs->text->length) ;
	    }
	 }
      else
	 {
	 char *new_shadow = (char*)FrRealloc(w->shadow_text,newlen) ;
	 if (new_shadow)
	    {
	    w->shadow_text = new_shadow ;
	    strncpy(w->shadow_text+cbs->startPos,cbs->text->ptr,
		    cbs->text->length) ;
	    w->shadow_text[cbs->startPos+cbs->text->length] = '\0' ;
	    }
	 }
      for (int j = 0 ; j < cbs->text->length ; j++)
	 cbs->text->ptr[j] = '*' ;
      }
   return ;
}

//----------------------------------------------------------------------

FrWShadowText::FrWShadowText(Widget parent, const char *text, int columns,
			     int rows, bool editable, bool traversal,
			     bool wrap, const char *name)
{
   widget = create_shadow_text(parent,text,columns,rows,editable,traversal,
			       wrap,name);
   _managed = false ;
   shadow_text = FrDupString(text) ;
   verify_cb = 0 ;
   FrWidget::verifyCallback(shadow_text_verify_cb,this) ;
   return ;
}

//----------------------------------------------------------------------

FrWShadowText::FrWShadowText(FrWidget *parent, const char *text, int columns,
			     int rows, bool editable, bool traversal,
			     bool wrap, const char *name)
{
   widget = create_shadow_text(**parent,text,columns,rows,editable,traversal,
			       wrap,name) ;
   _managed = false ;
   shadow_text = FrDupString(text) ;
   verify_cb = 0 ;
   FrWidget::verifyCallback(shadow_text_verify_cb,this) ;
   return ;
}

//----------------------------------------------------------------------

FrWShadowText::~FrWShadowText()
{
   FrFree(shadow_text) ;
   return ;
}

//----------------------------------------------------------------------

FrObjectType FrWShadowText::objType() const
{
   return (FrObjectType)OT_FrWShadowText ;
}

//----------------------------------------------------------------------

const char *FrWShadowText::objTypeName() const
{
   return "FrWShadowText" ;
}

//----------------------------------------------------------------------

FrObjectType FrWShadowText::objSuperclass() const
{
   return (FrObjectType)OT_FrWidget ;
}

//----------------------------------------------------------------------

char *FrWShadowText::getText() const
{
   return FrDupString(shadow_text) ;
}

//----------------------------------------------------------------------

void FrWShadowText::getText(char *buf, int buflen) const
{
   strncpy(buf,shadow_text,buflen) ;
   buf[buflen-1] = '\0' ;
   return ;
}

//----------------------------------------------------------------------

void FrWShadowText::setText(const char *t, bool to_end)
{
   size_t len = strlen(shadow_text) ;
   shadow_text = FrNewR(char,shadow_text,len+1) ;
   memcpy(shadow_text,t,len+1) ;
   XmTextSetString(widget,(char*)t) ;
   if (to_end)
      XmTextSetInsertionPosition(widget,len) ;
   return ;
}

//----------------------------------------------------------------------

void FrWShadowText::verifyCallback(FrWText_VerifyFunc *verify)
{
   verify_cb = verify ;
}

/************************************************************************/
/*    Methods for class FrWFrameCompleter				*/
/************************************************************************/

static Widget create_frame_completion_box(Widget parent, const char *initial,
					  int columns, XtCallbackProc done_cb,
					  XtPointer done_data)
{
   if (!initial)
      initial = "" ;
   Widget text = create_text(parent,initial,columns,1,true,true,false,
			     "completion");
   XmTextSetCursorPosition(text,strlen(initial)) ;
   XtAddCallback(text,XmNmodifyVerifyCallback,
		 frame_completion_cb,(XtPointer)FrSymbolTable::current()) ;
   if (done_cb)
      XtAddCallback(text,XmNactivateCallback, done_cb, done_data) ;
   XtTranslations completionTranslations ;
   completionTranslations = XtParseTranslationTable(completion_translations) ;
   XtOverrideTranslations(text,completionTranslations) ;
   XtManageChildren(&text,1) ;
   return text ;
}

//----------------------------------------------------------------------

FrWFrameCompleter::FrWFrameCompleter(Widget parent, const char *initial,
				     int columns,
				     XtCallbackProc done_cb,
				     XtPointer done_data)
{
   widget = create_frame_completion_box(parent,initial,columns,
					done_cb,done_data) ;
   _managed = true ;
}

//----------------------------------------------------------------------

FrWFrameCompleter::FrWFrameCompleter(FrWidget *parent, const char *initial,
				     int columns,
				     XtCallbackProc done_cb,
				     XtPointer done_data)
{
   widget = create_frame_completion_box(**parent,initial,columns,
					done_cb,done_data) ;
   _managed = true ;
}

//----------------------------------------------------------------------

FrObjectType FrWFrameCompleter::objType() const
{
   return (FrObjectType)OT_FrWFrameCompleter ;
}

//----------------------------------------------------------------------

const char *FrWFrameCompleter::objTypeName() const
{
   return "FrWFrameCompleter" ;
}

//----------------------------------------------------------------------

FrObjectType FrWFrameCompleter::objSuperclass() const
{
   return (FrObjectType)OT_FrWidget ;
}

/************************************************************************/
/*    Methods for class FrWTextWindow					*/
/************************************************************************/

static Widget motif_text_display(Widget parent,char *text,int rows,int columns,
				 XtCallbackProc help_cb, XtPointer help_data)
{
   Arg args[12];
   int n = 0;

   if (XtParent(parent))
      parent = XtParent(parent) ;
   Widget form_dialog = create_form_dialog(parent,"textwindow",
					   "Text Viewer", "Viewer",true);
   XtSetArg(args[n],XmNscrollVertical,true);n++;
   XtSetArg(args[n],XmNscrollHorizontal,false);n++;
   XtSetArg(args[n],XmNeditMode,XmMULTI_LINE_EDIT);n++;
   XtSetArg(args[n],XmNeditable,false);n++;
   XtSetArg(args[n],XmNcursorPositionVisible,false);n++;
   XtSetArg(args[n],XmNwordWrap,true);n++;
   XtSetArg(args[n],XmNvalue,text);n++;
   XtSetArg(args[n],XmNcolumns,columns);n++;
   XtSetArg(args[n],XmNrows,rows);n++;
   Widget scrolled_text = XmCreateScrolledText(form_dialog,"scrolled_text",
					       args,n);
   Widget button_rowcol = create_rowcolumn(form_dialog,"buttons",true) ;
   XtSetArg(args[0], XmNnumColumns, 1) ;
   XtSetValues(button_rowcol, args, 1) ;
   attach_widget(button_rowcol,0,Attach_to_Form,Attach_to_Form,Attach_to_Form) ;
   attach_widget(XtParent(scrolled_text),
		 Attach_to_Form,button_rowcol,Attach_to_Form,Attach_to_Form) ;
   XmTextSetString(scrolled_text,text);
   XtManageChildren(&scrolled_text,1);
   Widget form = XtVaCreateWidget("buttonform",xmFormWidgetClass,button_rowcol,
				  XmNfractionBase, help_cb ? 5 : 3,
				  NULL);
   attach_widget(form,0,Attach_to_Form,Attach_to_Form,Attach_to_Form) ;
   Widget ok = XtVaCreateManagedWidget(str_Ok,
				       xmPushButtonWidgetClass,form,
				       XmNtopAttachment,XmATTACH_FORM,
				       XmNbottomAttachment,XmATTACH_FORM,
				       XmNleftAttachment,XmATTACH_POSITION,
				       XmNleftPosition,1,
				       XmNrightAttachment,XmATTACH_POSITION,
				       XmNrightPosition,2,
				       XmNshowAsDefault,true,
				       XmNdefaultButtonShadowThickness,1,
				       NULL);
   XtAddCallback(ok,XmNactivateCallback,DestroyWindow,form_dialog) ;
   XtSetArg(args[0], XmNdefaultButton,ok) ;
   XtSetValues(form, args, 1) ;
   if (help_cb)
      {
      Widget help = XtVaCreateManagedWidget(str_Help,
					    xmPushButtonWidgetClass,form,
					    XmNleftAttachment,XmATTACH_POSITION,
					    XmNleftPosition,3,
					    XmNrightAttachment,XmATTACH_POSITION,
					    XmNrightPosition,4,
					    XmNtopAttachment,XmATTACH_FORM,
					    XmNbottomAttachment,XmATTACH_FORM,
					    NULL) ;
      XtAddCallback(help,XmNactivateCallback,help_cb,help_data) ;
      XtAddCallback(help,XmNactivateCallback,DestroyWindow,form_dialog) ;
      }
   XtManageChildren(&form,1) ;
   XtManageChildren(&button_rowcol,1) ;
   XtManageChildren(&form_dialog,1);
   XtPopup(XtParent(form_dialog),XtGrabNone);
   return form_dialog ;
}

//----------------------------------------------------------------------

static int count_lines(char *text, int *width)
{
   *width = 0 ;
   if (!text)
      return 0 ;
   int lines = 0 ;
   int column ;
   for (;;)
      {
      lines++ ;
      column = 0 ;
      while (*text && *text != '\n')	// scan for newline or end of string
	 {
	 text++ ;
	 column++ ;			// remember how long line is
	 }
      if (column > *width)		// longest line seen yet?
	 *width = column ;
      if (*text)
	 text++ ;			// skip the newline
      else
	 break ;
      }
   return lines ;
}

//----------------------------------------------------------------------

static Widget motif_text_display(Widget parent, char *text,
				 XtCallbackProc help_cb, XtPointer help_data)
{
   int columns ;
   int rows = count_lines(text,&columns) ;
   if (columns > MAX_WIDTH)
      {
      if (rows == 1)
	 rows = (columns+MAX_WIDTH-1)/MAX_WIDTH ;
      columns = MAX_WIDTH ;
      }
   else if (columns == 0)
      columns = 1 ;
   if (rows > MAX_HEIGHT)
      rows = MAX_HEIGHT ;
   else if (rows == 0)
      rows = 1 ;
   return motif_text_display(parent,text,rows,columns,help_cb,help_data) ;
}

//----------------------------------------------------------------------

FrWTextWindow::FrWTextWindow(const char *filename)
{
   if(!filename || !*filename)
      return ;

   struct stat statb ;
   FILE *fp = 0 ;

   if (stat(filename,&statb) == -1 ||
#if defined(unix) || defined(__linux__) || defined(__GNUC__)
       (statb.st_mode & S_IFMT) != S_IFREG ||
#endif /* unix */
       !(fp = fopen(filename,"r")))
      {
#if defined(unix) || defined(__linux__) || defined(__GNUC__)
      if((statb.st_mode & S_IFMT) != S_IFREG)
	 create_warning_popup(toplevel,
			      "Unable to display the file\n"
			      "because it is not text.") ;
      else
#endif /* unix */
	 perror(filename) ;
      return;
      }
   char *text ;
   if ((text = FrNewN(char,(unsigned)(statb.st_size+1))) == 0)
      {
      fclose(fp);
      FrNoMemory("while reading file to be displayed") ;
      return;
      }
   if (!fread(text,sizeof(char),statb.st_size,fp))
      FrWarning("Error reading file") ;
   fclose(fp) ;
   text[statb.st_size] = 0 ;
   // convert any embedded NULs into blanks
   char *curr = text ;
   while ((curr = strchr(text,'\0')) < &text[statb.st_size])
      *curr = ' ' ;
   widget = motif_text_display(toplevel,text,0,0) ;
   FrFree(text);
   unlink(filename) ;
}

//----------------------------------------------------------------------

FrWTextWindow::FrWTextWindow(Widget parent, char *text,
			     XtCallbackProc help_cb, XtPointer help_data)
{
   widget = motif_text_display(parent,text,help_cb,help_data) ;
}

//----------------------------------------------------------------------

FrWTextWindow::FrWTextWindow(FrWidget *parent, char *text,
			     XtCallbackProc help_cb, XtPointer help_data)
{
   widget = motif_text_display(**parent,text,help_cb,help_data) ;
}

//----------------------------------------------------------------------

FrWTextWindow::~FrWTextWindow()
{
}

//----------------------------------------------------------------------

FrObjectType FrWTextWindow::objType() const
{
   return (FrObjectType)OT_FrWTextWindow ;
}

//----------------------------------------------------------------------

const char *FrWTextWindow::objTypeName() const
{
   return "FrWTextWindow" ;
}

//----------------------------------------------------------------------

FrObjectType FrWTextWindow::objSuperclass() const
{
   return (FrObjectType)OT_FrWidget ;
}

/************************************************************************/
/*    Methods for class FrWMainWindow					*/
/************************************************************************/

static Widget create_main_window(Widget parent, const char *name,
				 const char *title, const char *icon,
				 int height, int width,
				 bool managed, bool show_sep)
{
   Widget widget ;
   Arg args[8] ;
   int n = 0 ;
   XmString tstr, istr ;
   if (title)
      {
      tstr = XmStringCreateSimple((char*)title) ;
      XtSetArg(args[n], XmNtitle, tstr) ; n++ ;
      }
   else
      tstr = 0 ;
   if (!icon)
      icon = "FramepaC App" ;
   istr = XmStringCreateSimple((char*)icon) ;
   XtSetArg(args[n], XmNiconName, istr) ; n++ ;
   XtSetArg(args[n], XmNshowSeparator, show_sep) ; n++ ;
   if (height)
      {
      XtSetArg(args[n], XmNheight, height) ; n++ ;
      }
   if (width)
      {
      XtSetArg(args[n], XmNwidth, width) ; n++ ;
      }
   if (!name || *name == '\0')
      name = "mainwindow" ;
   if (managed)
      widget = XtCreateManagedWidget(name, xmMainWindowWidgetClass, parent,
				     args, n) ;
   else
      widget = XtCreateWidget(name, xmMainWindowWidgetClass, parent,
			      args, n) ;
   if (tstr)
      XmStringFree(tstr) ;
   if (istr)
      XmStringFree(istr) ;
   return widget ;
}

//----------------------------------------------------------------------

FrWMainWindow::FrWMainWindow(Widget parent, const char *name,
			     const char *title, const char *icon,
			     int height, int width,
			     bool managed, bool show_sep)
{
   widget = create_main_window(parent,name,title,icon,height,width,managed,
			       show_sep) ;
   _managed = managed ;
}

//----------------------------------------------------------------------

FrWMainWindow::FrWMainWindow(FrWidget *parent, const char *name,
			     const char *title, const char *icon,
			     int height, int width,
			     bool managed, bool show_sep)
{
   widget = create_main_window(**parent,name,title,icon,height,width,managed,
			       show_sep) ;
   _managed = managed ;
}

//----------------------------------------------------------------------

FrObjectType FrWMainWindow::objType() const
{
   return (FrObjectType)OT_FrWMainWindow ;
}

//----------------------------------------------------------------------

const char *FrWMainWindow::objTypeName() const
{
   return "FrWMainWindow" ;
}

//----------------------------------------------------------------------

FrObjectType FrWMainWindow::objSuperclass() const
{
   return (FrObjectType)OT_FrWidget ;
}

//----------------------------------------------------------------------

void FrWMainWindow::setWorkWindow(Widget w) const
{
   Arg args[1] ;

   XtSetArg(args[0], XmNworkWindow, w) ;
   XtSetValues(widget, args, 1) ;
}

//----------------------------------------------------------------------

void FrWMainWindow::setMenuBar(Widget w) const
{
   Arg args[1] ;

   XtSetArg(args[0], XmNmenuBar, w) ;
   XtSetValues(widget, args, 1) ;
}

/************************************************************************/
/*    Methods for class FrWScrollWindow					*/
/************************************************************************/

static Widget create_scrolled_window(Widget parent, const char *title,
				     bool autoscroll, bool managed)
{
   Widget widget ;
   Arg args[2] ;
   int n = 0 ;
   if (title)
      {
      XtSetArg(args[n], XmNtitle, title) ; n++ ;
      }
   XtSetArg(args[n], XmNscrollingPolicy,
	             autoscroll ? XmAUTOMATIC : XmAPPLICATION_DEFINED) ; n++ ;

   if (managed)
      widget = XtCreateManagedWidget(str_scrollwindow,
				     xmScrolledWindowWidgetClass, parent,
				     args, n) ;
   else
      widget = XtCreateWidget(str_scrollwindow,
			      xmScrolledWindowWidgetClass, parent,
			      args, n) ;
   return widget ;
}

//----------------------------------------------------------------------

FrWScrollWindow::FrWScrollWindow(Widget parent, const char *title,
				 bool autoscroll, bool managed)
{
   widget = create_scrolled_window(parent,title,autoscroll,managed) ;
}

//----------------------------------------------------------------------

FrWScrollWindow::FrWScrollWindow(FrWidget *parent, const char *title,
				 bool autoscroll, bool managed)
{
   widget = create_scrolled_window(**parent,title,autoscroll,managed) ;
}

//----------------------------------------------------------------------

FrObjectType FrWScrollWindow::objType() const
{
   return (FrObjectType)OT_FrWScrollWindow ;
}

//----------------------------------------------------------------------

const char *FrWScrollWindow::objTypeName() const
{
   return "FrWScrollWindow" ;
}

//----------------------------------------------------------------------

FrObjectType FrWScrollWindow::objSuperclass() const
{
   return (FrObjectType)OT_FrWidget ;
}

//----------------------------------------------------------------------

FrWidget *FrWScrollWindow::workWindow() const
{
   Arg args[1] ;
   Widget window ;
   XtSetArg(args[0], XmNworkWindow, &window) ;
   XtGetValues(widget, args, 1) ;
   return window ? new FrWidget(window) : 0 ;
}

//----------------------------------------------------------------------

void FrWScrollWindow::scrollBorder(int width) const
{
   Dimension w = width ;
   Arg args[1] ;
   XtSetArg(args[0], XmNspacing, w) ;
   XtSetValues(widget, args, 1) ;
}

//----------------------------------------------------------------------

void FrWScrollWindow::forceScrollBar(bool stat) const
{
   Arg args[1] ;
   XtSetArg(args[0], XmNscrollBarDisplayPolicy, stat
	    				          ? (XtPointer)XmSTATIC
	    					  : (XtPointer)XmAS_NEEDED) ;
   XtSetValues(widget, args, 1) ;
}

//----------------------------------------------------------------------

void FrWScrollWindow::scrollTo(Widget w, int hmargin, int vmargin) const
{
   XmScrollVisible(widget,w,(Dimension)hmargin, (Dimension)vmargin);
}

//----------------------------------------------------------------------

void FrWScrollWindow::scrollTo(FrWidget *w, int hmargin, int vmargin) const
{
   XmScrollVisible(widget,**w,(Dimension)hmargin, (Dimension)vmargin);
}

//----------------------------------------------------------------------

Widget FrWScrollWindow::scrollBar(bool vertical) const
{
   Arg args[1] ;
   Widget sb ;
   XtSetArg(args[0],vertical ? XmNverticalScrollBar : XmNhorizontalScrollBar,
	    &sb) ;
   XtGetValues(widget, args, 1) ;
   return sb ;
}

/************************************************************************/
/*    Methods for class FrWScrollBar					*/
/************************************************************************/

static Widget create_scroll_bar(Widget parent, bool vertical, int limit,
				bool managed, bool arrows, const char *sbclass)
{
   Arg args[6] ;
   int n = 0 ;

   XtSetArg(args[n],XmNorientation,vertical ? XmVERTICAL : XmHORIZONTAL) ; n++;
   XtSetArg(args[n],XmNminimum,0) ; n++ ;
   XtSetArg(args[n],XmNmaximum,limit) ; n++ ;
   XtSetArg(args[n],XmNvalue,0) ; n++ ;
   XtSetArg(args[n],XmNsliderSize,arrows ? limit : 1) ; n++ ;
   XtSetArg(args[n],XmNshowArrows,arrows) ; n++ ;
   if (!sbclass || !*sbclass)
      sbclass = str_scrollbar ;
   if (managed)
      return XtCreateManagedWidget(sbclass,xmScrollBarWidgetClass,parent,
				   args,n) ;
   else
      return XtCreateWidget(sbclass,xmScrollBarWidgetClass,parent,args,n) ;
}

//----------------------------------------------------------------------

FrWScrollBar::FrWScrollBar(Widget parent, bool vertical, int limit,
			   bool managed, const char *sbclass)
{
   widget = create_scroll_bar(parent,vertical,limit,managed,true,sbclass) ;
   _managed = managed ;
}

//----------------------------------------------------------------------

FrWScrollBar::FrWScrollBar(FrWidget *parent, bool vertical, int limit,
			   bool managed, const char *sbclass)
{
   widget = create_scroll_bar(**parent,vertical,limit,managed,true,sbclass) ;
   _managed = managed ;
}

//----------------------------------------------------------------------

FrObjectType FrWScrollBar::objType() const
{
   return (FrObjectType)OT_FrWScrollBar ;
}

//----------------------------------------------------------------------

const char *FrWScrollBar::objTypeName() const
{
   return "FrWScrollBar" ;
}

//----------------------------------------------------------------------

FrObjectType FrWScrollBar::objSuperclass() const
{
   return (FrObjectType)OT_FrWidget ;
}

//----------------------------------------------------------------------

void FrWScrollBar::setLimit(int limit) const
{
   Arg args[1] ;
   XtSetArg(args[0], XmNmaximum, limit) ;
   XtSetValues(widget,args,1) ;
}

//----------------------------------------------------------------------

int FrWScrollBar::getLimit() const
{
   Arg args[1] ;
   int limit ;
   XtSetArg(args[0], XmNmaximum, &limit) ;
   XtGetValues(widget,args,1) ;
   return limit ;
}

//----------------------------------------------------------------------

void FrWScrollBar::setStart(int start) const
{
   Arg args[1] ;
   XtSetArg(args[0], XmNvalue, start) ;
   XtSetValues(widget,args,1) ;
}

//----------------------------------------------------------------------

void FrWScrollBar::setLength(int length) const
{
   Arg args[1] ;
   XtSetArg(args[0], XmNsliderSize, length) ;
   XtSetValues(widget,args,1) ;
}

//----------------------------------------------------------------------

void FrWScrollBar::setThumb(int start, int length) const
{
   Arg args[2] ;
   XtSetArg(args[0], XmNvalue, start) ;
   XtSetArg(args[1], XmNsliderSize, length) ;
   XtSetValues(widget,args,2) ;
}

//----------------------------------------------------------------------

void FrWScrollBar::getThumb(int *start, int *length) const
{
   Arg args[2] ;
   int n = 0 ;
   if (start)
      {
      XtSetArg(args[0], XmNvalue, start) ; n++ ;
      }
   if (length)
      {
      XtSetArg(args[1], XmNsliderSize, length) ; n++ ;
      }
   if (n)
      XtGetValues(widget,args,n) ;
}

/************************************************************************/
/*    Methods for class FrWProgressIndicator				*/
/************************************************************************/

FrWProgressIndicator::FrWProgressIndicator(Widget parent, bool managed,
					   const char *piclass)
{
   if (!piclass || !*piclass)
      piclass = str_ProgressIndicator ;
   widget = create_scroll_bar(parent,false,100,managed,false,piclass) ;
   _managed = managed ;
}

//----------------------------------------------------------------------

FrWProgressIndicator::FrWProgressIndicator(FrWidget *parent, bool managed,
					   const char *piclass)
{
   if (!piclass || !*piclass)
      piclass = str_ProgressIndicator ;
   widget = create_scroll_bar(**parent,false,100,managed,false,piclass) ;
   _managed = managed ;
}

//----------------------------------------------------------------------

FrObjectType FrWProgressIndicator::objType() const
{
   return (FrObjectType)OT_FrWProgressIndicator ;
}

//----------------------------------------------------------------------

const char *FrWProgressIndicator::objTypeName() const
{
   return "FrWProgressIndicator" ;
}

//----------------------------------------------------------------------

FrObjectType FrWProgressIndicator::objSuperclass() const
{
   return (FrObjectType)OT_FrWidget ;
}

//----------------------------------------------------------------------

void FrWProgressIndicator::setProgress(int percentage) const
{
   if (percentage >= 0 && percentage <= 100)
      {
      Arg args[1] ;
      XtSetArg(args[0], XmNsliderSize, percentage) ;
      XtSetValues(widget,args,1) ;
      }
}

//----------------------------------------------------------------------

int FrWProgressIndicator::getProgress() const
{
   Arg args[1] ;
   int percentage = 0 ;
   XtSetArg(args[0], XmNsliderSize, &percentage) ;
   XtGetValues(widget,args,1) ;
   return percentage ;
}

/************************************************************************/
/*    Methods for class FrWProgressPopup				*/
/************************************************************************/

static Widget create_progress_popup(Widget parent, const char *label,
				    const char *ppclass)
{
   Widget dialog = create_form_dialog(parent,str_Progress,
				      str_Progress_Indicator,str_Progress,
				      false) ;
   Widget form = create_form(dialog,true,str_Progress_Indicator,str_Progress) ;
   FrWLabel labelw(form,label,true) ;
   if (!ppclass || !*ppclass)
      ppclass = str_ProgressIndicator ;
   Widget widget = create_scroll_bar(form,false,100,true,false,ppclass) ;
   labelw.attach(Attach_to_Form,widget,Attach_to_Form,Attach_to_Form) ;
   labelw.attachOffsets(5,5,10,10) ;
   FrWidget sb(widget) ;
   sb.attach(0,Attach_to_Form,Attach_to_Form,Attach_to_Form) ;
   sb.attachOffsets(5,5,10,10) ;
   sb.retain() ;
   labelw.retain() ;
   XtManageChild(form) ;
   XtManageChild(dialog) ;
   return widget ;
}

//----------------------------------------------------------------------

FrWProgressPopup::FrWProgressPopup(Widget parent, const char *label,
				   const char *ppclass)
{
   widget = create_progress_popup(parent,label,ppclass) ;
   _managed = true ;
}

//----------------------------------------------------------------------

FrWProgressPopup::FrWProgressPopup(FrWidget *parent, const char *label,
				   const char *ppclass)
{
   widget = create_progress_popup(**parent,label,ppclass) ;
   _managed = true ;
}

//----------------------------------------------------------------------

FrWProgressPopup::~FrWProgressPopup()
{
   XtDestroyWidget(XtParent(XtParent(widget))) ;
   widget = 0 ;
}

//----------------------------------------------------------------------

FrObjectType FrWProgressPopup::objType() const
{
   return (FrObjectType)OT_FrWProgressPopup ;
}

//----------------------------------------------------------------------

const char *FrWProgressPopup::objTypeName() const
{
   return "FrWProgressPopup" ;
}

//----------------------------------------------------------------------

FrObjectType FrWProgressPopup::objSuperclass() const
{
   return (FrObjectType)OT_FrWProgressIndicator ;
}

/************************************************************************/
/*    Methods for class FrWRadioBox					*/
/************************************************************************/

static Widget create_radio_box(Widget parent, const char *title,
			       const char *name, bool vertical, bool managed)
{
   if (!name || *name == '\0')
      name = str_radiobox ;
   Arg args[3] ;
   int n = 0 ;
   if (title)
      {
      XtSetArg(args[n], XmNtitle, title) ; n++ ;
      }
   XtSetArg(args[n],XmNorientation,vertical ? XmVERTICAL : XmHORIZONTAL); n++;
   Widget widget = XmCreateRadioBox(parent, (char*)title, args, n) ;
   if (managed)
      XtManageChildren(&widget,1) ;
   return widget ;
}

//----------------------------------------------------------------------

FrWRadioBox::FrWRadioBox(Widget parent, const char *title, const char *name,
			 bool vertical, bool managed)
{
   widget = create_radio_box(parent,title,name,vertical,managed) ;
   _managed = managed ;
}

//----------------------------------------------------------------------

FrWRadioBox::FrWRadioBox(FrWidget *parent, const char *title, const char *name,
			 bool vertical, bool managed)
{
   widget = create_radio_box(**parent,title,name,vertical,managed) ;
   _managed = managed ;
}

//----------------------------------------------------------------------

FrObjectType FrWRadioBox::objType() const
{
   return (FrObjectType)OT_FrWRadioBox ;
}

//----------------------------------------------------------------------

const char *FrWRadioBox::objTypeName() const
{
   return "FrWRadioBox" ;
}

//----------------------------------------------------------------------

FrObjectType FrWRadioBox::objSuperclass() const
{
   return (FrObjectType)OT_FrWidget ;
}

/************************************************************************/
/************************************************************************/

void FrRealizeWidget(FrWidget *w)
{
   XtRealizeWidget(**w) ;
   return ;
}

//----------------------------------------------------------------------

void FrRealizeWidget(Widget w)
{
   XtRealizeWidget(w) ;
   return ;
}

//----------------------------------------------------------------------

void FrUnrealizeWidget(FrWidget *w)
{
   XtUnrealizeWidget(**w) ;
   return ;
}

//----------------------------------------------------------------------

void FrUnrealizeWidget(Widget w)
{
   XtUnrealizeWidget(w) ;
   return ;
}

//----------------------------------------------------------------------

void FrRaiseWindow(FrWidget *w)
{
   Widget window = **w ;
   XRaiseWindow(XtDisplay(window),XtWindow(window)) ;
   return ;
}

//----------------------------------------------------------------------

void FrRaiseWindow(Widget w)
{
   XRaiseWindow(XtDisplay(w),XtWindow(w)) ;
   return ;
}

//----------------------------------------------------------------------

void FrMapWidget(Widget w)
{
   XtMapWidget(w) ;
   return ;
}

//----------------------------------------------------------------------

void FrUnmapWidget(Widget w)
{
   XtUnmapWidget(w) ;
   return ;
}

/************************************************************************/
/************************************************************************/

void process_pending_X_event()
{
   XtAppProcessEvent(app_context,XtIMAll) ;
   return ;
}

//----------------------------------------------------------------------

void process_pending_X_events(Widget w)
{
   while (XtAppPending(app_context))
      {
      XtAppProcessEvent(app_context,XtIMAll);
      }
   XSync(XtDisplay(w),0) ;
   XmUpdateDisplay(w) ;
   return ;
}

/************************************************************************/
/************************************************************************/

//----------------------------------------------------------------------

static void add_message(const char *msg)
{
   if (msg && *msg)
      {
      XmString s = XmStringCreateLtoR((char*)msg,XmSTRING_DEFAULT_CHARSET) ;
      XmListAddItemUnselected(msg_window,s,0);
      XmStringFree(s);
      int n = 0 ;
      XtVaGetValues(msg_window,XmNitemCount,&n,NULL) ;
//      if (n > max_messages)
//	 XmListDeleteItemsPos(msg_window,n-max_messages,1);
      XmListSetBottomPos(msg_window,0);
      if (fpshell)
	 {
	 XtPopup(fpshell,XtGrabNone) ;
	 XmUpdateDisplay(msg_window) ;
	 process_pending_X_events(fpshell) ;
	 }
      }
   return ;
}

//----------------------------------------------------------------------

static void really_out_of_memory(const char *where)
{
   cerr << "Completely out of memory while attempting to display "
        << where << " message." << endl ;
   return ;
}

//----------------------------------------------------------------------

static void add_headed_message(const char *header, const char *message,
			       const char *where)
{
   int hdrlen = strlen(header) ;
   int msglen = strlen(message) ;
   int len = hdrlen + msglen ;
   FrLocalAlloc(char,msg,2000,len+2) ;
   if (!msg)
      really_out_of_memory(where) ;
   else
      {
      memcpy(msg,header,hdrlen) ;
      memcpy(msg+hdrlen,message,msglen) ;
      memcpy(msg+hdrlen+msglen,".",2) ;
      add_message(msg) ;
      FrLocalFree(msg) ;
      }
   return ;
}

//----------------------------------------------------------------------

static void kill_program_cb(Widget, XtPointer, XtPointer)
{
   // OK, user has responded, so kill the program
   shutdown_FramepaC() ;
   if (killprog_hook)
      killprog_hook(fatal_error_message) ;
   XtUnrealizeWidget(ancestor) ;   // kill off entire interface
   exit(127) ;
}

//----------------------------------------------------------------------

static void kill_program(const char *errmsg)
{
   // create a warning dialog to pause the program before killing it
   int arg_count = 0 ;
   Arg arg_array[5] ;
   char msg[1000] ;

   fatal_error_message = errmsg ;
   Fr_sprintf(msg,sizeof(msg),
	      "%s\n\nThis program will now be terminated because\n"
	      "it is not possible to recover from the error.",errmsg) ;
   XmString ok_msg = XmStringCreateLtoR("Continue",XmSTRING_DEFAULT_CHARSET);
   XmString text = XmStringCreateLtoR(msg, XmSTRING_DEFAULT_CHARSET) ;
   XtSetArg(arg_array[arg_count],XmNmessageString,text) ; arg_count++ ;
   XtSetArg(arg_array[arg_count],XmNokLabelString, ok_msg) ; arg_count++ ;
   XtSetArg(arg_array[arg_count],XmNdialogStyle,
				 XmDIALOG_FULL_APPLICATION_MODAL); arg_count++;
   Widget dialog = XmCreateWarningDialog(fpshell,"FramepaC_fatal_dialog",
					 arg_array, arg_count) ;
   XtVaSetValues(dialog,
		 XmNtitle, "FramepaC Abort Message",
		 XmNiconName, "FramepaC Abort",
		 NULL) ;
   XmStringFree(text) ;
   XtAddCallback(dialog, XmNokCallback, kill_program_cb, NULL) ;
   XtUnmanageChild(XmMessageBoxGetChild(dialog, XmDIALOG_HELP_BUTTON));
   XtUnmanageChild(XmMessageBoxGetChild(dialog, XmDIALOG_CANCEL_BUTTON));
   // Get message window's absolute coordinates
   int x, y ;
   XtVaGetValues(msg_window, XmNx, &x, XmNy, &y, NULL) ;
   // now offset the dialog from the message window
   x += KILL_X_OFFSET ;
   y += KILL_Y_OFFSET ;
   XtVaSetValues(dialog, XmNx, &x, XmNy, &y, NULL) ;
   // finally, display the dialog
   XtRealizeWidget(dialog) ;
   XtManageChild(dialog) ;
   for (;;)
      {
      process_pending_X_events(dialog) ;
      }
   return ;
}

/************************************************************************/
/************************************************************************/

static void warning(const char *message)
{
   add_headed_message("Warning: ",message,str_warning) ;
   XmUpdateDisplay(msg_window) ;
   return ;
}

//----------------------------------------------------------------------

static void fatal_error(const char *message)
{
   char buf[500] ;
   Fr_sprintf(buf,sizeof(buf),"Error: %s",message) ;
   kill_program(buf) ;
   return ;
}

//----------------------------------------------------------------------

static void out_of_memory(const char *message)
{
   char buf[500] ;
   Fr_sprintf(buf,sizeof(buf),"OUT OF MEMORY %s",message) ;
   kill_program(buf) ;
   return ;
}

//----------------------------------------------------------------------

static void programming_error(const char *message)
{
   char buf[500] ;
   Fr_sprintf(buf,sizeof(buf),
	      "Programming Error: %s\nPlease contact the developers.",
	      message) ;
   kill_program(buf) ;
   return ;
}

//----------------------------------------------------------------------

static void undefined_function(const char *message)
{
   char buf[500] ;
   Fr_sprintf(buf,sizeof(buf),
	      "Undefined Function: %s\nPlease contact the developers.",
	      message) ;
   kill_program(buf) ;
   return ;
}

//----------------------------------------------------------------------

void Xt_Warning(const char *msg)
{
   static const char warning_header[] = "Xlib Warning: %s" ;

   // avoid re-entrance problems by not calling FrMessage again if we are
   // still in the prior invocation
   static bool active = false ;
   if (active)
      fprintf(stderr,warning_header,msg) ;
   else
      {
      active = true ;
      FrMessageVA(warning_header,msg) ;
      active = false ;
      }
   return ;
}

//----------------------------------------------------------------------

void Xt_Error(const char *msg)
{
   size_t buflen = strlen(mgs) + 50 ;
   FrLocalAlloc(char,buf,1024,buflen) ;
   Fr_sprintf(buf,buflen,"Xlib Error: %s",msg) ;
   kill_program(buf) ;
   FrLocalFree(buf) ;
   return ;
}

//----------------------------------------------------------------------

void X_Error(const char *msg)
{
   char *buf = Fr_aprintf("X Error: %s",msg) ;
   kill_program(buf) ;
   FrFree(buf) ;
   return ;
}

//----------------------------------------------------------------------

void XIO_Error(const char *msg)
{
   cerr << "XIO Error: " << msg << endl ;
   fatal_error_message = "XIO Error" ;
   kill_program_cb(0,0,0) ;
   return ;
}

#endif /* FrMOTIF */

/************************************************************************/
/************************************************************************/

//----------------------------------------------------------------------

void add_Motif_workproc(Motif_workproc *wp, XtPointer data)
{
#ifdef FrMOTIF
   XtAppAddWorkProc(app_context,wp,data) ;
#else
   (void)wp ; (void)data ;
#endif /* FrMOTIF */
   return ;
}

//----------------------------------------------------------------------

XtErrorHandler set_Xlib_error_handler(XtErrorHandler proc)
{
#ifdef FrMOTIF
   return XtAppSetErrorHandler(app_context,proc) ;
#else
   (void)proc ;
   return 0 ;
#endif /* FrMOTIF */
}

//----------------------------------------------------------------------

XtErrorHandler set_Xlib_warning_handler(XtErrorHandler proc)
{
#ifdef FrMOTIF
   return XtAppSetWarningHandler(app_context,proc) ;
#else
   (void)proc ;
   return 0 ;
#endif /* FrMOTIF */
}

//----------------------------------------------------------------------

void run_Motif()
{
#ifdef FrMOTIF
   XtAppMainLoop(app_context) ;
#endif /* FrMOTIF */
   return ;
}

/************************************************************************/
/************************************************************************/

void FrInitializeMotif(const char *window_name, Widget parent,
			       int max_symbols)
{
   initialize_FramepaC(max_symbols) ;
#ifdef FrMOTIF
   ancestor = parent ;
   // create windows
   int arg_count = 0 ;
   Arg arg_array[20] ;

   if (!window_name)
      window_name = "FramepaC Messages" ;
   XtSetArg(arg_array[arg_count],XmNtitle,window_name) ; arg_count++ ;
   XtSetArg(arg_array[arg_count],XmNallowResize,true) ; arg_count++ ;
   XtSetArg(arg_array[arg_count],XmNwidth, 500) ; arg_count++ ;
   XtSetArg(arg_array[arg_count],XmNiconName,"FramepaC") ; arg_count++ ;
   fpshell = XtCreatePopupShell(window_name,
			        topLevelShellWidgetClass,
				parent, arg_array, arg_count) ;
   msg_window = XtVaCreateManagedWidget("list",xmListWidgetClass,
				       	fpshell,
				       	XmNvisibleItemCount,5,
					XmNitemCount,0,
					NULL) ;
   XtManageChild(msg_window) ;
   XtRealizeWidget(fpshell) ;
   XtPopdown(fpshell) ;		// hide the message window until needed
   // install message handlers
   message_handler = set_message_handler(add_message) ;
   warning_handler = set_warning_handler(warning) ;
   fatal_error_handler = set_fatal_error_handler(fatal_error) ;
   out_of_memory_handler = set_out_of_memory_handler(out_of_memory) ;
   prog_error_handler = set_prog_error_handler(programming_error) ;
   undef_function_handler = set_undef_function_handler(undefined_function) ;
   Xt_warning_handler = XtAppSetWarningHandler(app_context,Xt_Warning) ;
   Xt_error_handler = XtAppSetErrorHandler(app_context,Xt_Error) ;
   _XErrorFunction = X_Error ;
   _XIOErrorFunction = XIO_Error ;
#else
   (void)window_name ;
   (void)parent ;
#endif /* FrMOTIF */
   return ;
}

//----------------------------------------------------------------------

Widget FrInitializeMotif(int *orig_argc, char **orig_argv,
			 const char *maintitle, const char *msgtitle,
			 const char *icon_name, bool allow_resize)
{
   if (!maintitle)
      maintitle = "FramepaC" ;
#ifdef FrMOTIF
   Widget w = XtVaAppInitialize(&app_context,
				"toplevel",
				NULL,0,
				orig_argc,orig_argv,
				NULL,
				XmNallowResize,allow_resize,
				XmNtitle, maintitle,
				XmNiconName, icon_name,
				NULL) ;
#else
   (void)orig_argc ; (void)orig_argv ; (void)allow_resize ; (void)maintitle ;
   (void)icon_name ;
   Widget w = 0 ;
#endif /* FrMOTIF */
   FrInitializeMotif(msgtitle,w) ;
   return w ;
}

//----------------------------------------------------------------------


void FrShutdownMotif()
{
#ifdef FrMOTIF
   set_message_handler(message_handler) ;
   set_warning_handler(warning_handler) ;
   set_fatal_error_handler(fatal_error_handler) ;
   set_out_of_memory_handler(out_of_memory_handler) ;
   set_prog_error_handler(prog_error_handler) ;
   set_undef_function_handler(undef_function_handler) ;
   // remove window
   if (fpshell)
      XtUnrealizeWidget(fpshell) ;
   XtAppSetWarningHandler(app_context,Xt_warning_handler) ;
   XtAppSetErrorHandler(app_context,Xt_error_handler) ;
   _XErrorFunction = 0 ;
   _XIOErrorFunction = 0 ;
#endif /* FrMOTIF */
   shutdown_FramepaC() ;
   return ;
}

//----------------------------------------------------------------------

#if !defined(__WINDOWS__) && !defined(__NT__)
void FrSetAppTitle(const char *title)
{
#ifdef FrMOTIF
   XtVaSetValues(toplevel, XmNtitle, title, NULL) ;
#else
   (void)title ;
#endif /* FrMOTIF */
   return ;
}
#endif /* !__WINDOWS__ && !__NT__ */

//----------------------------------------------------------------------

void set_killprog_hook(FrKillProgHookFunc *func)
{
#ifdef FrMOTIF
   killprog_hook = func ;
#else
   (void)func ;
#endif
   return ;
}

/************************************************************************/
/*	Overrides for Xt memory allocation functions			*/
/*	(includes overrides for other functions in the same		*/
/*	 object module in the Xt library as the functions we really     */
/*	 want to override)						*/
/************************************************************************/

#ifdef FrREPLACE_XTMALLOC

extern "C" void _XtAllocError(const char *where) ;
extern "C" char *XtMalloc(Cardinal) ;
extern "C" char *XtCalloc(Cardinal,Cardinal) ;
extern "C" char *XtRealloc(char *,Cardinal) ;
extern "C" void XtFree(char *) ;

#ifdef FrBUGFIX_XLIB_ALLOC
extern "C" void *malloc(size_t size) ;
extern "C" void *realloc(void *blk, size_t size) ;
extern "C" void *calloc(size_t n, size_t size) ;
extern "C" int cfree(void *blk) ;
extern "C" void free(void *blk) ;
#endif /* FrBUGFIX_XLIB_ALLOC */

struct XtHeapBlock
   {
   XtHeapBlock *next ;
   char data[1200] ;
   } ;

struct XtHeap
   {
   int avail ;
   char *allocptr ;
   XtHeapBlock *blocklist ;
   } ;

extern "C" void _XtHeapInit(XtHeap *heap) ;
extern "C" char *_XtHeapAlloc(XtHeap *heap,int size) ;
extern "C" void _XtHeapFree(XtHeap *heap) ;

//----------------------------------------------------------------------

void _XtAllocError(const char *where)
{
   static bool active = false ;
   char buf[500] ;
   strncpy(buf,"Xlib: ran out of memory during ",sizeof(buf)) ;
   size_t len = strlen(buf) ;
   strncpy(buf+len,where ? where : "local memory allocation",sizeof(buf)-len) ;
   // avoid re-entrance problems by not calling FrMessage again if we are
   // still in the prior invocation
   if (active)
      fprintf(stderr,buf) ;
   else
      {
      active = true ;
      FrWarning(buf) ;
      active = false ;
      }
   return ;
}

//----------------------------------------------------------------------

void _XtHeapInit(XtHeap *heap)
{
   if (heap)
      {
      heap->blocklist = 0 ;
      heap->avail = 0 ;
      }
   return ;
}

//----------------------------------------------------------------------

char *_XtHeapAlloc(XtHeap *heap,int size)
{
   if (!heap)
      return XtMalloc(size) ;
   else if (size+4 > (int)sizeof(heap->blocklist->data)/2)
      {
      XtHeapBlock *block = (XtHeapBlock*)FrMalloc(size+4) ;
      if (heap->blocklist)
	 {
	 block->next = heap->blocklist->next ;
	 heap->blocklist->next = block ;
	 }
      else
	 {
	 block->next = 0 ;
	 heap->blocklist = block ;
	 }
      return block->data ;
      }
   else if (size > heap->avail)
      {
      XtHeapBlock *block = FrNew(XtHeapBlock) ;
      block->next = heap->blocklist ;
      heap->blocklist = block ;
      heap->allocptr = block->data ;
      heap->avail = sizeof(block->data) ;
      }
   // round up size
   size = (size+sizeof(XtHeapBlock*)-1) & -sizeof(XtHeapBlock*) ;
   char *allocated = heap->allocptr ;
   heap->allocptr += size ;
   heap->avail -= size ;
   return allocated ;
}

//----------------------------------------------------------------------

void _XtHeapFree(XtHeap *heap)
{
   while (heap->blocklist)
      {
      XtHeapBlock *next = heap->blocklist->next ;
      FrFree(heap) ;
      heap->blocklist = next ;
      }
   heap->avail = 0 ;
   return ;
}

//----------------------------------------------------------------------

#ifdef FrSEPARATE_XTMALLOC
static FrMemoryPool *XtMalloc_Pool = 0 ;

static void init_XtMalloc_Pool()
{
   XtMalloc_Pool = new FrMemoryPool("XtMalloc",3) ;
   return ;
}
#endif /* FrSEPARATE_XTMALLOC */

//----------------------------------------------------------------------

#ifdef FrBUGFIX_XLIB_ALLOC
void *malloc(size_t size)
{
   if (!XtMalloc_Pool)
      init_XtMalloc_Pool() ;
   return XtMalloc_Pool->allocate(size) ;
}
#endif /* FrBUGFIX_XLIB_ALLOC */

//----------------------------------------------------------------------

#ifdef FrBUGFIX_XLIB_ALLOC
void *calloc(int n, size_t size)
{
   if (!XtMalloc_Pool)
      init_XtMalloc_Pool() ;
   size_t blocksize = n * size ;
   void *block = XtMalloc_Pool->allocate(blocksize) ;
   if (block)
      memset(block,'\0',blocksize) ;
   return block ;
}
#endif /* FrBUGFIX_XLIB_ALLOC */

//----------------------------------------------------------------------

#ifdef FrBUGFIX_XLIB_ALLOC
void *realloc(void *blk, size_t size)
{
   if (!XtMalloc_Pool)
      init_XtMalloc_Pool() ;
   return XtMalloc_Pool->reallocate(blk,size) ;
}
#endif /* FrBUGFIX_XLIB_ALLOC */

//----------------------------------------------------------------------

#ifdef FrBUGFIX_XLIB_ALLOC
void free(void *blk)
{
   if (!XtMalloc_Pool)
      init_XtMalloc_Pool() ;
   XtMalloc_Pool->release(blk) ;
   return ;
}
#endif /* FrBUGFIX_XLIB_ALLOC */

//----------------------------------------------------------------------

#ifdef FrBUGFIX_XLIB_ALLOC
int cfree(void *blk)
{
   if (!XtMalloc_Pool)
      init_XtMalloc_Pool() ;
   XtMalloc_Pool->release(blk) ;
   return 1 ;
}
#endif /* FrBUGFIX_XLIB_ALLOC */

//----------------------------------------------------------------------

char *XtMalloc(Cardinal size)
{
#ifdef FrSEPARATE_XTMALLOC
   if (!XtMalloc_Pool)
      init_XtMalloc_Pool() ;
   char *block = (char*)XtMalloc_Pool->allocate((size_t)size) ;
#else
   char *block = FrNewN(char,size) ;
#endif /* FrSEPARATE_XTMALLOC */
   if (!block)
      FrNoMemory("in XtMalloc") ;
   return block ;
}

//----------------------------------------------------------------------

char *XtCalloc(Cardinal nitems, unsigned int size)
{
#ifdef FrSEPARATE_XTMALLOC
   if (!XtMalloc_Pool)
      init_XtMalloc_Pool() ;
   size_t blocksize = (size_t) nitems * size ;
   char *block = (char*)XtMalloc_Pool->allocate(blocksize) ;
   if (block)
      memset(block,'\0',blocksize) ;
#else
   char *block = (char*)FrCalloc(nitems,size) ;
#endif /* FrSEPARATE_XTMALLOC */
   if (!block)
      FrNoMemory("in XtCalloc") ;
   return block ;
}

//----------------------------------------------------------------------

char *XtRealloc(char *block, Cardinal newsize)
{
#ifdef FrSEPARATE_XTMALLOC
   if (!XtMalloc_Pool)
      init_XtMalloc_Pool() ;
   char *newblock = (char*)XtMalloc_Pool->reallocate(block,(size_t)newsize) ;
#else
   char *newblock = (char*)FrRealloc(block,newsize,true) ;
#endif /* FrSEPARATE_XTMALLOC */
   if (!newblock && newsize)
      FrNoMemory("in XtRealloc") ;
   return newblock ;
}

//----------------------------------------------------------------------

void XtFree(char *block)
{
   if (block)
#ifdef FrSEPARATE_XTMALLOC
      XtMalloc_Pool->release(block) ;
#else
      FrFree(block) ;
#endif /* FrSEPARATE_XTMALLOC */
   return ;
}

#endif /* FrREPLACE_XTMALLOC */

// end of file frmotif.cpp //
