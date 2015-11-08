/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frmotif.h	    Motif user-interface code for FramepaC      */
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997					*/
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

#ifndef __FRMOTIF_H_INCLUDED
#define __FRMOTIF_H_INCLUDED

#ifndef __FRLIST_H_INCLUDED
#include "frlist.h"
#endif

#ifdef __WATCOMC__
// turn off warnings caused by taking 'sizeof' classes with virtual method
// table pointers, by setting the level so high (10) that the warning
// isn't issued even with the -wx (all warnings) flag
#pragma warning 549 10 ;
#endif /* __WATCOMC__ */

#if defined(XtSpecificationRelease)
// do nothing, Intrinsic.h has already been included
#elif !defined(FrMOTIF) || !defined(FrUSE_REAL_XDEFS)
  typedef void *XtPointer ;
  typedef struct _WidgetRec *Widget ;
  typedef char *caddr_t ;
  typedef struct _XtAppStruct *XtAppContext ;
  typedef Widget *WidgetList ;
  typedef void (*XtCallbackProc)(Widget,XtPointer,XtPointer) ;
  typedef void (*XtErrorHandler)(char *msg) ;
#ifdef LmUSE_SRILM
  typedef bool Boolean ;
#else
  typedef char Boolean ;
#endif

  extern "C" {
    void XtFree(char*) ;
    void XtDestroyWidget(Widget) ;
    Boolean XtIsManaged(Widget) ;
    void XtManageChild(Widget) ;
    void XtManageChildren(WidgetList,unsigned int) ;
    void XtUnmanageChild(Widget) ;
    void XtUnmanageChildren(WidgetList,unsigned int) ;
#ifndef XtMapWidget
    void XtMapWidget(Widget) ;
#endif
#ifndef XtUnmapWidget
    void XtUnmapWidget(Widget) ;
#endif
    void XtRemoveAllCallbacks(Widget,const char*) ;
    void XtAddCallback(Widget,const char*,XtCallbackProc,XtPointer) ;
    Widget XtParent(Widget) ;
    }
#elif !defined(XtSpecificationRelease)  /* Instrinsic.h already included? */
#  include <X11/Intrinsic.h>
#elif !defined(FrUSE_REAL_XDEFS)
typedef void *Widget ;
typedef char *caddr_t ;
#endif /* !FrMOTIF || !FrUSE_REAL_XDEFS */

/************************************************************************/
/************************************************************************/

struct FrButtonsAndCommands
   {
   char		  *name ;
   XtCallbackProc func ;
   XtPointer	  data ;
   char           mnemonic ;
   char		  active ;
   } ;

class FrWidget ;

#ifdef FrMOTIF

//----------------------------------------------------------------------

#define Attach_to_Form ((Widget)-1)
#define Attach_to_FormW ((FrWidget*)-1)
#define Attach_to_Self ((Widget)-2)
#define Attach_to_SelfW ((FrWidget*)-2)

/************************************************************************/
/*    Enumerated Types							*/
/************************************************************************/

enum FrWSelBoxChild { SelBox_OK, SelBox_Cancel, SelBox_Apply, SelBox_Help } ;

enum ArrowDirection { ArrowUp, ArrowDown, ArrowLeft, ArrowRight } ;

/************************************************************************/
/*    Definitions for class FrWidget					*/
/************************************************************************/

class FrWidget : public FrObject
   {
   protected:
      static FrAllocator allocator ;
      Widget widget ;
      char mapped ;
      char _managed ;
      FrWidget() { mapped = (char)true ; }
   public:
      void *user_data ;
   public:
      void *operator new(size_t size)
	 { return (size == sizeof(FrWidget)) ? allocator.allocate() : FrMalloc(size) ; }
      void operator delete(void *blk,size_t size)
	 { if (size == sizeof(FrWidget)) allocator.release(blk) ; else FrFree(blk) ; }
      FrWidget(Widget w)
         { widget = w ; mapped = (char)true ; _managed = (char)false ; }
      FrWidget(Widget w, bool m)
	 { widget = w ; mapped = (char)true ; _managed = (char)m ; }
      virtual ~FrWidget() ;
      virtual FrObjectType objType() const ;
      virtual const char *objTypeName() const ;
      virtual FrObjectType objSuperclass() const ;
      virtual ostream &printValue(ostream &output) const ;
      virtual char *displayValue(char *buffer) const ;
      virtual size_t displayLength() const ;
      virtual bool widgetp() const ;
      Widget operator * () const { return widget ; }
      void destroy() { if (widget) { XtDestroyWidget(widget) ; widget = 0 ; }}
      void keep() { widget = 0 ; delete this ; }
      void retain() { widget = 0 ; }
      void manage()
	    { if (widget) XtManageChildren(&widget,1) ;
	      _managed = (char)true ; }
      void unmanage()
	    { if (widget) XtUnmanageChildren(&widget,1) ;
	      _managed = (char)false ; }
      bool isManaged() const { return (bool)_managed ; }
      void setManaged(bool m) { _managed = (char)m ; }
      void map() { if (widget) XtMapWidget(widget) ; mapped = (char)true ; }
      void unmap() { if (widget) XtUnmapWidget(widget); mapped = (char)false; }
      bool isMapped() const { return (bool)mapped ; }
      void mapWhenManaged(bool do_map) const ;
      void setValue(XtPointer value) const ;
      void setUserData(XtPointer data) const ;
      XtPointer getValue() const ;
      XtPointer getUserData() const ;
      char *getLabel() const ;
      int getWidth(bool include_border = false) const ;
      int getHeight(bool include_border = false) const ;
      void getSize(int *width, int *height, bool include_border = false) const;
      void getPosition(int *x, int *y) const ;
      void setSensitive(bool sensitive) const ;
      void setLabel(const char *label) const ;
      void setTextString(const char *text) const ;
      void setAlignment(int align) const ;
      void setEnds(Widget from, Widget to) const ;
      void setOrientation(int orientation) const ;
      void setVertical(bool vertical) const ;
      void setShadow(int thickness) const ;
      void setTitle(const char *title) const ;
      void setIconName(const char *title) const ;
      void setPosition(int x, int y) const ;
      void setWidth(int width) const ;
      void setHeight(int height) const ;
      void allowResize(bool state) const ;
      void invertVideo() const ;
      void setTraversal(bool state) const ;
      void traverseCurrent() const ;
      void traverseNext() const ;
      void traversePrev() const ;
      void traverseNextGroup() const ;
      void traversePrevGroup() const ;
      void traverseUp() const ;
      void traverseDown() const ;
      void navigationTabGroup() const ;
      void removeCallbacks(char *type) const
	    { if (widget) XtRemoveAllCallbacks(widget,type) ; }
      void addCallback(char *type, XtCallbackProc cb_func, XtPointer cb_data)
	    { if (widget) XtAddCallback(widget,type,cb_func,cb_data) ; }
      void removeCallback(char *type, XtCallbackProc cb_func,
			  XtPointer cb_data) const ;
      void armCallback(XtCallbackProc cb_func, XtPointer cb_data,
		       bool remove_old = false) ;
      void disarmCallback(XtCallbackProc cb_func, XtPointer cb_data,
			  bool remove_old = false) ;
      void activateCallback(XtCallbackProc cb_func, XtPointer cb_data,
			    bool remove_old = false) ;
      void activateCallback(XtCallbackProc cb_func) ; // pass self to CB
      void okCallback(XtCallbackProc cb_func, XtPointer cb_data,
		      bool remove_old = false) ;
      void cancelCallback(XtCallbackProc cb_func, XtPointer cb_data,
			  bool remove_old = false) ;
      void helpCallback(XtCallbackProc cb_func, XtPointer cb_data,
			bool remove_old = false) ;
      void nomatchCallback(XtCallbackProc cb_func, XtPointer cb_data,
			   bool remove_old= false) ;
      void changedCallback(XtCallbackProc cb_func, XtPointer cb_data,
			   bool remove_old = false) ;
      void verifyCallback(XtCallbackProc cb_func, XtPointer cb_data,
			  bool remove_old = false) ;
      void focusCallback(XtCallbackProc cb_func, XtPointer cb_data,
			 bool remove_old = false) ;
      void loseFocusCallback(XtCallbackProc cb_func, XtPointer cb_data,
			     bool remove_old = false) ;
      void resizeCallback(XtCallbackProc cb_func, XtPointer cb_data,
			  bool remove_old = false) ;
      void destroyCallback(XtCallbackProc cb_func, bool remove_old = false) ;
      void destroyCallback(bool remove_old = false) ;
      void setFont(const char *fontname) ;
      void attach(Widget top,Widget bottom,Widget left,Widget right);
      void attach(FrWidget *top,FrWidget *bottom,FrWidget *left,
		  FrWidget *right);
      void attachOpposite(Widget top,Widget bottom,Widget left,Widget right);
      void attachOpposite(FrWidget *top,FrWidget *bottom,FrWidget *left,
		  FrWidget *right);
      void attachOffsets(int top, int bottom, int left, int right) ;
      void attachPosition(int top, int bottom, int left, int right) ;
      void detach(bool top, bool bottom, bool left, bool right) ;
      void warpPointer(int x, int y) const ;
      void warpPointerCenter() const ;
      void getPointer(int *x, int *y) const ;
      Widget parentWidget() const ;
      Widget grandparent() const ;
   } ;

/************************************************************************/
/*    Definitions for class FrWSeparator				*/
/************************************************************************/

class FrWSeparator : public FrWidget
   {
   public:
#if !defined(FrBUGFIX_GCC_272)
      void *operator new(size_t) { return allocator.allocate() ; }
      void operator delete(void *blk,size_t) { allocator.release(blk) ; }
#endif /* !FrBUGFIX_GCC_272 */
      FrWSeparator(Widget parent, int linestyle = -1) ;
      FrWSeparator(FrWidget *parent, int linestyle = -1) ;
      virtual FrObjectType objType() const ;
      virtual const char *objTypeName() const ;
      virtual FrObjectType objSuperclass() const ;
   } ;

/************************************************************************/
/*    Definitions for class FrWFrame					*/
/************************************************************************/

class FrWFrame : public FrWidget
   {
   public:
#if !defined(FrBUGFIX_GCC_272)
      void *operator new(size_t) { return allocator.allocate() ; }
      void operator delete(void *blk,size_t) { allocator.release(blk) ; }
#endif /* !FrBUGFIX_GCC_272 */
      FrWFrame(Widget parent) ;
      FrWFrame(FrWidget *parent) ;
      virtual FrObjectType objType() const ;
      virtual const char *objTypeName() const ;
      virtual FrObjectType objSuperclass() const ;
   } ;

/************************************************************************/
/*    Definitions for class FrWArrow					*/
/************************************************************************/

class FrWArrow : public FrWidget
   {
   protected:
      ArrowDirection direction ;
      FrWArrow() {} ;
   public:
#if !defined(FrBUGFIX_GCC_272)
      void *operator new(size_t size) { return FrMalloc(size) ; }
      void operator delete(void *blk,size_t) { FrFree(blk) ; }
#endif /* !FrBUGFIX_GCC_272 */
      FrWArrow(Widget parent, ArrowDirection direction) ;
      FrWArrow(FrWidget *parent, ArrowDirection direction) ;
      virtual FrObjectType objType() const ;
      virtual const char *objTypeName() const ;
      virtual FrObjectType objSuperclass() const ;
   } ;

/************************************************************************/
/*    Definitions for class FrWArrowG					*/
/************************************************************************/

class FrWArrowG : public FrWArrow
   {
   public:
#if !defined(FrBUGFIX_GCC_272)
      void *operator new(size_t) { return allocator.allocate() ; }
      void operator delete(void *blk,size_t) { allocator.release(blk) ; }
#endif /* !FrBUGFIX_GCC_272 */
      FrWArrowG(Widget parent, ArrowDirection direction) ;
      FrWArrowG(FrWidget *parent, ArrowDirection direction) ;
      virtual FrObjectType objType() const ;
      virtual const char *objTypeName() const ;
      virtual FrObjectType objSuperclass() const ;
   } ;

/************************************************************************/
/*    Definitions for class FrWLabel					*/
/************************************************************************/

class FrWLabel : public FrWidget
   {
   public:
#if !defined(FrBUGFIX_GCC_272)
      void *operator new(size_t) { return allocator.allocate() ; }
      void operator delete(void *blk,size_t) { allocator.release(blk) ; }
#endif /* !FrBUGFIX_GCC_272 */
      FrWLabel(Widget parent, const char *label, bool managed = true) ;
      FrWLabel(FrWidget *parent, const char *label, bool managed = true) ;
      virtual FrObjectType objType() const ;
      virtual const char *objTypeName() const ;
      virtual FrObjectType objSuperclass() const ;
   } ;

/************************************************************************/
/*    Definitions for class FrWList					*/
/************************************************************************/

class FrWList : public FrWidget
   {
   private:
      int visible_items ;
      int max_items ;
      int curr_items ;
   public:
#if !defined(FrBUGFIX_GCC_272)
      void *operator new(size_t size) { return FrMalloc(size) ; }
      void operator delete(void *blk,size_t) { FrFree(blk) ; }
#endif /* !FrBUGFIX_GCC_272 */
      FrWList(Widget parent, const char *name, int visible, int max=0) ;
      FrWList(FrWidget *parent, const char *name, int visible, int max=0) ;
      virtual FrObjectType objType() const ;
      virtual const char *objTypeName() const ;
      virtual FrObjectType objSuperclass() const ;
      void add(const char *item) ;
      int totalItems() const { return curr_items ; }
   } ;

/************************************************************************/
/*    Definitions for class FrWRowColumn				*/
/************************************************************************/

class FrWRowColumn : public FrWidget
   {
   public:
#if !defined(FrBUGFIX_GCC_272)
      void *operator new(size_t) { return allocator.allocate() ; }
      void operator delete(void *blk,size_t) { allocator.release(blk) ; }
#endif /* !FrBUGFIX_GCC_272 */
      FrWRowColumn(Widget parent, const char *name, bool vertical=false) ;
      FrWRowColumn(FrWidget *parent, const char *name, bool vertical=false) ;
      virtual FrObjectType objType() const ;
      virtual const char *objTypeName() const ;
      virtual FrObjectType objSuperclass() const ;
      void setColumns(int columns) const ;
      void setPacked(int packed) const ;
      void setAlignment(int align) const ;
   } ;

/************************************************************************/
/*    Definitions for class FrWOptionMenu				*/
/************************************************************************/

class FrWOptionMenu : public FrWidget
   {
   private:
      XtCallbackProc change_func ;
      XtPointer change_data ;
      FrList *buttons ;
      FrObject *current_selection ;
   public:
#if !defined(FrBUGFIX_GCC_272)
      void *operator new(size_t size) { return FrMalloc(size) ; }
      void operator delete(void *blk,size_t) { FrFree(blk) ; }
#endif /* !FrBUGFIX_GCC_272 */
      FrWOptionMenu(Widget parent, const char *name, FrList *options,
		    FrObject *def_option = 0, bool managed = false) ;
      FrWOptionMenu(FrWidget *parent, const char *name, FrList *options,
		    FrObject *def_option = 0, bool managed = false) ;
      virtual ~FrWOptionMenu() ;
      virtual FrObjectType objType() const ;
      virtual const char *objTypeName() const ;
      virtual FrObjectType objSuperclass() const ;
      FrObject *currentSelection() const { return current_selection ; }
      void setSelection(const char *selection_text) ;	// private use
      void changeCallback(XtCallbackProc cb_func, XtPointer cb_data,
			  bool remove_old = false) ;
      void changeCallback(XtCallbackProc cb_func) ; 	// pass self to CB
      void callChangeCallback(XtPointer call_data) const ;
   } ;
// IMPORTANT: don't delete the option menu object until the option menu is
// destroyed (i.e. don't use keep() on it).  destroyCallback() is called
// automatically

/************************************************************************/
/*    Definitions for class FrWPopupMenu				*/
/************************************************************************/

class FrWPopupMenu : public FrWidget
   {
   protected:
      FrWPopupMenu() {}
   public:
#if !defined(FrBUGFIX_GCC_272)
      void *operator new(size_t) { return allocator.allocate() ; }
      void operator delete(void *blk,size_t) { allocator.release(blk) ; }
#endif /* !FrBUGFIX_GCC_272 */
      FrWPopupMenu(Widget parent, const char *name, bool managed = false) ;
      FrWPopupMenu(FrWidget *parent, const char *name, bool managed = false) ;
      virtual FrObjectType objType() const ;
      virtual const char *objTypeName() const ;
      virtual FrObjectType objSuperclass() const ;
      virtual void popup() ;
      void menuPosition(XtPointer call_data) ;
   } ;

/************************************************************************/
/*    Definitions for class FrWPulldownMenu				*/
/************************************************************************/

class FrWPulldownMenu : public FrWPopupMenu
   {
   private:
      int grabbed_button ;
   public:
#if !defined(FrBUGFIX_GCC_272)
      void *operator new(size_t size) { return FrMalloc(size) ; }
      void operator delete(void *blk,size_t) { FrFree(blk) ; }
#endif /* !FrBUGFIX_GCC_272 */
      FrWPulldownMenu(Widget parent, const char *name, bool managed = false) ;
      FrWPulldownMenu(FrWidget *parent, const char *name, bool managed=false) ;
      virtual FrObjectType objType() const ;
      virtual const char *objTypeName() const ;
      virtual FrObjectType objSuperclass() const ;
      virtual void popup() ;
      void pulldown(int button) ;	 // pop up and grab button
   } ;

/************************************************************************/
/*    Definitions for class FrWMessagePopup				*/
/************************************************************************/

enum FrWMessageType
   { FrWMessage_None,
     FrWMessage_Working,
     FrWMessage_Info,
     FrWMessage_Question } ;

class FrWMessagePopup : public FrWidget
   {
   public:
#if !defined(FrBUGFIX_GCC_272)
      void *operator new(size_t) { return allocator.allocate() ; }
      void operator delete(void *blk,size_t) { allocator.release(blk) ; }
#endif /* !FrBUGFIX_GCC_272 */
      FrWMessagePopup(FrWMessageType type, Widget parent,
		      const char *message, XtCallbackProc ok = 0,
		      XtCallbackProc cancel = 0, XtPointer client_data = 0,
		      const char **help_data = 0) ;
      FrWMessagePopup(FrWMessageType type, FrWidget *parent,
		      const char *message, XtCallbackProc ok = 0,
		      XtCallbackProc cancel = 0, XtPointer client_data = 0,
		      const char **help_data = 0) ;
      virtual ~FrWMessagePopup() ;
      virtual void freeObject() ;
      virtual FrObjectType objType() const ;
      virtual const char *objTypeName() const ;
      virtual FrObjectType objSuperclass() const ;
      void changeMessage(const char *message) ;
   } ;

/************************************************************************/
/*    Definitions for class FrWPromptPopup				*/
/************************************************************************/

class FrWPromptPopup : public FrWPopupMenu
   {
   protected:
      FrWPromptPopup() {} ;
   public:
#if !defined(FrBUGFIX_GCC_272)
      void *operator new(size_t) { return allocator.allocate() ; }
      void operator delete(void *blk,size_t) { allocator.release(blk) ; }
#endif /* !FrBUGFIX_GCC_272 */
      FrWPromptPopup(Widget w) { widget = w ; _managed = false ; }
      FrWPromptPopup(Widget parent, const char *label, const char *def,
		     XtCallbackProc ok_cb = 0, XtPointer ok_data = 0,
		     XtPointer help_data = 0,
		     bool auto_unmanage = true, bool modal = false,
		     bool managed = true) ;
      FrWPromptPopup(FrWidget *parent, const char *label, const char *def,
		     XtCallbackProc ok_cb = 0, XtPointer ok_data = 0,
		     XtPointer help_data = 0,
		     bool auto_unmanage = true, bool modal = false,
		     bool managed = true) ;
      virtual FrObjectType objType() const ;
      virtual const char *objTypeName() const ;
      virtual FrObjectType objSuperclass() const ;
      virtual void popup() ;
   } ;

/************************************************************************/
/*    Definitions for class FrWVerifyPopup				*/
/************************************************************************/

class FrWVerifyPopup : public FrWidget
   {
   public:
#if !defined(FrBUGFIX_GCC_272)
      void *operator new(size_t) { return allocator.allocate() ; }
      void operator delete(void *blk,size_t) { allocator.release(blk) ; }
#endif /* !FrBUGFIX_GCC_272 */
      FrWVerifyPopup(Widget parent, const char *prompt,
		     XtCallbackProc ok_cb = 0, XtPointer ok_data = 0,
		     XtCallbackProc cancel_cb = 0, XtPointer cancel_data = 0,
		     XtPointer help_data = 0,
		     bool modal = false) ;
      FrWVerifyPopup(FrWidget *parent, const char *prompt,
		     XtCallbackProc ok_cb = 0, XtPointer ok_data = 0,
		     XtCallbackProc cancel_cb = 0, XtPointer cancel_data = 0,
		     XtPointer help_data = 0,
		     bool modal = false) ;
      virtual FrObjectType objType() const ;
      virtual const char *objTypeName() const ;
      virtual FrObjectType objSuperclass() const ;
   } ;

/************************************************************************/
/*    Definitions for class FrWDialogPopup				*/
/************************************************************************/

class FrWDialogPopup : public FrWPromptPopup
   {
   protected:
      FrWDialogPopup() {} ;
   public:
#if !defined(FrBUGFIX_GCC_272)
      void *operator new(size_t) { return allocator.allocate() ; }
      void operator delete(void *blk,size_t) { allocator.release(blk) ; }
#endif /* !FrBUGFIX_GCC_272 */
      FrWDialogPopup(Widget w) { widget = w ; _managed = false ; }
      FrWDialogPopup(Widget parent, const char *name, const char *title = 0,
		     const char *icon = 0, bool auto_unmanage = false) ;
      FrWDialogPopup(FrWidget *parent, const char *name, const char *title = 0,
		     const char *icon = 0, bool auto_unmanage = false) ;
      virtual FrObjectType objType() const ;
      virtual const char *objTypeName() const ;
      virtual FrObjectType objSuperclass() const ;
   } ;

/************************************************************************/
/*    Definitions for class FrWFramePrompt				*/
/************************************************************************/

class FrWFramePrompt : public FrWPromptPopup
   {
   public:
#if !defined(FrBUGFIX_GCC_272)
      void *operator new(size_t) { return allocator.allocate() ; }
      void operator delete(void *blk,size_t) { allocator.release(blk) ; }
#endif /* !FrBUGFIX_GCC_272 */
      FrWFramePrompt(Widget parent, const char *label, const char *def,
		     XtCallbackProc ok_cb = 0, XtPointer ok_data = 0,
		     XtPointer help_data = 0,
		     bool auto_unmanage = true, bool modal = false,
		     bool managed = true) ;
      FrWFramePrompt(FrWidget *parent, const char *label, const char *def,
		     XtCallbackProc ok_cb = 0, XtPointer ok_data = 0,
		     XtPointer help_data = 0,
		     bool auto_unmanage = true, bool modal = false,
		     bool managed = true) ;
      virtual FrObjectType objType() const ;
      virtual const char *objTypeName() const ;
      virtual FrObjectType objSuperclass() const ;
      void setFrameName(FrSymbol *framename) const ;
   } ;

/************************************************************************/
/*    Definitions for class FrWPushButton				*/
/************************************************************************/

class FrWPushButton : public FrWidget
   {
   protected:
      FrWPushButton() {}
   public:
#if !defined(FrBUGFIX_GCC_272)
      void *operator new(size_t) { return allocator.allocate() ; }
      void operator delete(void *blk,size_t) { allocator.release(blk) ; }
#endif /* !FrBUGFIX_GCC_272 */
      FrWPushButton(Widget button) { widget = button ; }
      FrWPushButton(Widget parent, const char *label, bool managed = true,
		    bool centered = true, const char *pbclass = 0) ;
      FrWPushButton(FrWidget *parent, const char *label, bool managed = true,
		    bool centered = true, const char *pbclass = 0) ;
      virtual FrObjectType objType() const ;
      virtual const char *objTypeName() const ;
      virtual FrObjectType objSuperclass() const ;
      void setAlignment(int align) const ;
      void setMnemonic(char mnem) const ;
      void setDualMouseButton() const ;
      void setTripleMouseButton() const ;
      static int pressedButton(XtPointer call_data) ;
   } ;

/************************************************************************/
/*    Definitions for class FrWPushButtonG				*/
/************************************************************************/

class FrWPushButtonG : public FrWPushButton
   {
   public:
#if !defined(FrBUGFIX_GCC_272)
      void *operator new(size_t) { return allocator.allocate() ; }
      void operator delete(void *blk,size_t) { allocator.release(blk) ; }
#endif /* !FrBUGFIX_GCC_272 */
      FrWPushButtonG(Widget parent, const char *label, bool managed = true,
		    bool centered = true, const char *pbclass = 0) ;
      FrWPushButtonG(FrWidget *parent, const char *label, bool managed = true,
		    bool centered = true, const char *pbclass = 0) ;
      virtual FrObjectType objType() const ;
      virtual const char *objTypeName() const ;
   } ;

/************************************************************************/
/*    Definitions for class FrWToggleButton				*/
/************************************************************************/

class FrWToggleButton : public FrWPushButton
   {
   private:
      bool toggle_set ;
      static void activateCB(Widget, XtPointer, XtPointer) ;
   public:
#if !defined(FrBUGFIX_GCC_272)
      void *operator new(size_t size) { return FrMalloc(size) ; }
      void operator delete(void *blk,size_t) { FrFree(blk) ; }
#endif /* !FrBUGFIX_GCC_272 */
      FrWToggleButton(Widget parent, const char *label, bool set = false,
		      bool managed = true) ;
      FrWToggleButton(FrWidget *parent, const char *label, bool set = false,
		      bool managed = true) ;
      virtual FrObjectType objType() const ;
      virtual const char *objTypeName() const ;
      bool getState() const { return toggle_set ; }
      void setState(bool state) ;
      void alwaysVisible(bool vis) ;
   } ;

/************************************************************************/
/*    Definitions for class FrWCascadeButton				*/
/************************************************************************/

class FrWCascadeButton : public FrWidget
   {
   public:
#if !defined(FrBUGFIX_GCC_272)
      void *operator new(size_t) { return allocator.allocate() ; }
      void operator delete(void *blk,size_t) { allocator.release(blk) ; }
#endif /* !FrBUGFIX_GCC_272 */
      FrWCascadeButton(Widget parent, const char *label, char mnemonic = '\0',
		       Widget submenuID = 0) ;
      FrWCascadeButton(FrWidget *parent, const char *label,
		       char mnemonic = '\0', FrWidget *submenuID = 0) ;
      virtual FrObjectType objType() const ;
      virtual const char *objTypeName() const ;
      virtual FrObjectType objSuperclass() const ;
   } ;

/************************************************************************/
/*    Definitions for class FrWForm					*/
/************************************************************************/

class FrWForm : public FrWidget
   {
   public:
#if !defined(FrBUGFIX_GCC_272)
      void *operator new(size_t) { return allocator.allocate() ; }
      void operator delete(void *blk,size_t) { allocator.release(blk) ; }
#endif /* !FrBUGFIX_GCC_272 */
      FrWForm(Widget parent, const char *name, bool managed = false,
	      const char *icon_name = 0) ;
      FrWForm(FrWidget *parent, const char *name, bool managed = false,
	      const char *icon_name = 0) ;
      virtual FrObjectType objType() const ;
      virtual const char *objTypeName() const ;
      virtual FrObjectType objSuperclass() const ;
   } ;

/************************************************************************/
/*    Definitions for class FrWSlider					*/
/************************************************************************/

class FrWSlider : public FrWidget
   {
   public:
#if !defined(FrBUGFIX_GCC_272)
      void *operator new(size_t) { return allocator.allocate() ; }
      void operator delete(void *blk,size_t) { allocator.release(blk) ; }
#endif /* !FrBUGFIX_GCC_272 */
      FrWSlider(Widget parent, const char *name, int max, int min,
		int def_value, int precision, bool vertical = false) ;
      FrWSlider(FrWidget *parent, const char *name, int max, int min,
		int def_value, int precision, bool vertical = false) ;
      virtual FrObjectType objType() const ;
      virtual const char *objTypeName() const ;
      virtual FrObjectType objSuperclass() const ;
      static int sliderValue(XtPointer call_data) ;
   } ;

/************************************************************************/
/*    Definitions for class FrWButtonBar				*/
/************************************************************************/

class FrWButtonBar : public FrWidget
   {
   private:
      FrList *buttons ;
   public:
#if !defined(FrBUGFIX_GCC_272)
      void *operator new(size_t size) { return FrMalloc(size) ; }
      void operator delete(void *blk,size_t) { FrFree(blk) ; }
#endif /* !FrBUGFIX_GCC_272 */
      FrWButtonBar(Widget parent, const FrButtonsAndCommands *buttons,
		   int bcount, bool managed, XtPointer default_data) ;
      FrWButtonBar(FrWidget *parent, const FrButtonsAndCommands *buttons,
		   int bcount, bool managed, XtPointer default_data) ;
      virtual ~FrWButtonBar() ;
      virtual FrObjectType objType() const ;
      virtual const char *objTypeName() const ;
      virtual FrObjectType objSuperclass() const ;
      FrWPushButton *nthButton(int n)
	    { return (FrWPushButton*)(buttons->nth(n)) ; }
   } ;

/************************************************************************/
/*    Definitions for class FrWText					*/
/************************************************************************/

class FrWText ;

typedef bool FrWText_VerifyFunc(FrWText *w, char *newchars, int start,
 				int end, char **displaychars) ;

class FrWText : public FrWidget
   {
   protected:
      int max_length ;
      FrWText_VerifyFunc *verify_cb ;
   protected:
      FrWText() {}
   public:
#if !defined(FrBUGFIX_GCC_272)
      void *operator new(size_t size) { return FrMalloc(size) ; }
      void operator delete(void *blk,size_t) { FrFree(blk) ; }
#endif /* !FrBUGFIX_GCC_272 */
      FrWText(Widget w) { widget = w ; }
      FrWText(Widget parent, const char *text, int columns = 30,
	      int rows = 1, bool editable = true, bool traversal = true,
	      bool wrap = true, const char *name = 0) ;
      FrWText(FrWidget *parent, const char *text, int columns = 30,
	      int rows = 1, bool editable = true, bool traversal = true,
	      bool wrap = true, const char *name = 0) ;
      virtual FrObjectType objType() const ;
      virtual const char *objTypeName() const ;
      virtual FrObjectType objSuperclass() const ;
      char *getText() const ;
      void getText(char *buf, int buflen) const ;
      void setText(const char *t,bool to_end = true) ;
      void limitLength(int maxlen) ;
      int lengthLimit() const { return max_length ; }
      void verifyCallback(FrWText_VerifyFunc *verify) ;
      void setSensitive(bool sensitive) const ;
      FrWText_VerifyFunc *getVerifyCallback() const { return verify_cb ; }
   } ;

/************************************************************************/
/*    Definitions for class FrWShadowText				*/
/************************************************************************/

class FrWShadowText : public FrWText
   {
   private:
      char *shadow_text ;
   protected:
      FrWShadowText() {}
   public:
#if !defined(FrBUGFIX_GCC_272)
      void *operator new(size_t size) { return FrMalloc(size) ; }
      void operator delete(void *blk,size_t) { FrFree(blk) ; }
#endif /* !FrBUGFIX_GCC_272 */
      FrWShadowText(Widget w) { widget = w ; }
      FrWShadowText(Widget parent, const char *text, int columns = 30,
	      int rows = 1, bool editable = true, bool traversal = true,
	      bool wrap = true, const char *name = 0) ;
      FrWShadowText(FrWidget *parent, const char *text, int columns = 30,
	      int rows = 1, bool editable = true, bool traversal = true,
	      bool wrap = true, const char *name = 0) ;
      virtual ~FrWShadowText() ;
      virtual FrObjectType objType() const ;
      virtual const char *objTypeName() const ;
      virtual FrObjectType objSuperclass() const ;
      char *getText() const ;
      void getText(char *buf, int buflen) const ;
      void setText(const char *t,bool to_end = true) ;
      void verifyCallback(FrWText_VerifyFunc *verify) ;
      friend void shadow_text_verify_cb(Widget,XtPointer,XtPointer) ;
   } ;

/************************************************************************/
/*    Definitions for class FrWFrameCompleter				*/
/************************************************************************/

class FrWFrameCompleter : public FrWText
   {
   public:
#if !defined(FrBUGFIX_GCC_272)
      void *operator new(size_t) { return allocator.allocate() ; }
      void operator delete(void *blk,size_t) { allocator.release(blk) ; }
#endif /* !FrBUGFIX_GCC_272 */
      FrWFrameCompleter(Widget parent, const char *initial, int columns = 30,
			XtCallbackProc done_cb = 0, XtPointer done_data = 0) ;
      FrWFrameCompleter(FrWidget *parent, const char *initial,
 			int columns = 30,
			XtCallbackProc done_cb = 0, XtPointer done_data = 0) ;
      virtual FrObjectType objType() const ;
      virtual const char *objTypeName() const ;
      virtual FrObjectType objSuperclass() const ;
   } ;

/************************************************************************/
/*    Definitions for class FrWTextWindow				*/
/************************************************************************/

class FrWTextWindow : public FrWidget
   {
   public:
#if !defined(FrBUGFIX_GCC_272)
      void *operator new(size_t) { return allocator.allocate() ; }
      void operator delete(void *blk,size_t) { allocator.release(blk) ; }
#endif /* !FrBUGFIX_GCC_272 */
      FrWTextWindow(const char *filename) ;
      FrWTextWindow(Widget parent, char *text,
		    XtCallbackProc help_cb = 0, XtPointer help_data = 0) ;
      FrWTextWindow(FrWidget *parent, char *text,
		    XtCallbackProc help_cb = 0, XtPointer help_data = 0) ;
      virtual ~FrWTextWindow() ;
      virtual FrObjectType objType() const ;
      virtual const char *objTypeName() const ;
      virtual FrObjectType objSuperclass() const ;
   } ;

/************************************************************************/
/*    Definitions for class FrWMainWindow				*/
/************************************************************************/

class FrWMainWindow : public FrWidget
   {
   public:
#if !defined(FrBUGFIX_GCC_272)
      void *operator new(size_t) { return allocator.allocate() ; }
      void operator delete(void *blk,size_t) { allocator.release(blk) ; }
#endif /* !FrBUGFIX_GCC_272 */
      FrWMainWindow(Widget parent, const char *name, const char *title,
 		    const char *icon = 0,
		    int height = 0, int width = 0, bool managed = false,
		    bool show_sep = false) ;
      FrWMainWindow(FrWidget *parent, const char *name, const char *title,
 		    const char *icon = 0,
		    int height = 0, int width = 0, bool managed = false,
		    bool show_sep = false) ;
      virtual FrObjectType objType() const ;
      virtual const char *objTypeName() const ;
      virtual FrObjectType objSuperclass() const ;
      void setWorkWindow(Widget) const ;
      void setMenuBar(Widget) const ;
   } ;

/************************************************************************/
/*    Definitions for class FrWScrollWindow				*/
/************************************************************************/

class FrWScrollWindow : public FrWidget
   {
   public:
#if !defined(FrBUGFIX_GCC_272)
      void *operator new(size_t) { return allocator.allocate() ; }
      void operator delete(void *blk,size_t) { allocator.release(blk) ; }
#endif /* !FrBUGFIX_GCC_272 */
      FrWScrollWindow(Widget parent) { widget = parent ; }
      FrWScrollWindow(Widget parent, const char *title, bool autoscroll = true,
		      bool managed = true) ;
      FrWScrollWindow(FrWidget *parent, const char *title,
		      bool autoscroll = true, bool managed = true) ;
      virtual FrObjectType objType() const ;
      virtual const char *objTypeName() const ;
      virtual FrObjectType objSuperclass() const ;
      FrWidget *workWindow() const ;
      void scrollBorder(int width) const ;
      void forceScrollBar(bool force) const ;
      void scrollTo(Widget w,int horiz_margin,int vert_margin) const ;
      void scrollTo(FrWidget *w,int horiz_margin,int vert_margin) const ;
      Widget scrollBar(bool vertical) const ;
   } ;

/************************************************************************/
/*    Definitions for class FrWScrollBar				*/
/************************************************************************/

class FrWScrollBar : public FrWidget
   {
   public:
#if !defined(FrBUGFIX_GCC_272)
      void *operator new(size_t) { return allocator.allocate() ; }
      void operator delete(void *blk,size_t) { allocator.release(blk) ; }
#endif /* !FrBUGFIX_GCC_272 */
      FrWScrollBar(Widget scrollbar) { widget = scrollbar ; }
      FrWScrollBar(Widget parent, bool vertical, int limit = 100,
		   bool managed = true, const char *sbclass = 0) ;
      FrWScrollBar(FrWidget *parent, bool vertical, int limit,
		   bool managed = true, const char *sbclass = 0) ;
      virtual FrObjectType objType() const ;
      virtual const char *objTypeName() const ;
      virtual FrObjectType objSuperclass() const ;
      void setLimit(int limit) const ;
      int getLimit() const ;
      void setStart(int start) const ;
      void setLength(int length) const ;
      void setThumb(int start, int length) const ;
      void getThumb(int *start, int *length) const ;
   } ;

/************************************************************************/
/*    Definitions for class FrWProgressIndicator			*/
/************************************************************************/

class FrWProgressIndicator : public FrWidget
   {
   protected:
      FrWProgressIndicator() {}
   public:
#if !defined(FrBUGFIX_GCC_272)
      void *operator new(size_t) { return allocator.allocate() ; }
      void operator delete(void *blk,size_t) { allocator.release(blk) ; }
#endif /* !FrBUGFIX_GCC_272 */
      FrWProgressIndicator(Widget parent) { widget = parent ; }
      FrWProgressIndicator(Widget parent, bool managed,
			   const char *piclass = 0) ;
      FrWProgressIndicator(FrWidget *parent, bool managed = true,
			   const char *piclass = 0) ;
      virtual FrObjectType objType() const ;
      virtual const char *objTypeName() const ;
      virtual FrObjectType objSuperclass() const ;
      void setProgress(int percentage) const ;
      int getProgress() const ;
   } ;

/************************************************************************/
/*    Definitions for class FrWProgressPopup				*/
/************************************************************************/

class FrWProgressPopup : public FrWProgressIndicator
   {
   public:
#if !defined(FrBUGFIX_GCC_272)
      void *operator new(size_t) { return allocator.allocate() ; }
      void operator delete(void *blk,size_t) { allocator.release(blk) ; }
#endif /* !FrBUGFIX_GCC_272 */
      FrWProgressPopup(Widget parent, const char *label,
		       const char *ppclass = 0) ;
      FrWProgressPopup(FrWidget *parent, const char *label,
		       const char *ppclass = 0) ;
      virtual ~FrWProgressPopup() ;
      virtual FrObjectType objType() const ;
      virtual const char *objTypeName() const ;
      virtual FrObjectType objSuperclass() const ;
   } ;

/************************************************************************/
/*    Definitions for class FrWRadioBox					*/
/************************************************************************/

class FrWRadioBox : public FrWidget
   {
   public:
#if !defined(FrBUGFIX_GCC_272)
      void *operator new(size_t) { return allocator.allocate() ; }
      void operator delete(void *blk,size_t) { allocator.release(blk) ; }
#endif /* !FrBUGFIX_GCC_272 */
      FrWRadioBox(Widget parent, const char *title, const char *name = 0,
		  bool vertical = true, bool managed = true) ;
      FrWRadioBox(FrWidget *parent, const char *title, const char *name = 0,
		  bool vertical = true, bool managed = true) ;
      virtual FrObjectType objType() const ;
      virtual const char *objTypeName() const ;
      virtual FrObjectType objSuperclass() const ;
   } ;


/************************************************************************/
/*    Definitions for class FrWSelectionBox				*/
/************************************************************************/

class FrWSelectionBox : public FrWidget
   {
   public:
#if !defined(FrBUGFIX_GCC_272)
      void *operator new(size_t) { return allocator.allocate() ; }
      void operator delete(void *blk,size_t) { allocator.release(blk) ; }
#endif /* !FrBUGFIX_GCC_272 */
      FrWSelectionBox(Widget parent, FrList *items, char *label,
		      XtCallbackProc ok_cb, XtPointer ok_data,
		      XtCallbackProc nomatch_cb, XtPointer nomatch_data,
		      XtCallbackProc apply_cb, XtPointer apply_data,
		      const char **helptexts = 0,
		      bool must_match = true,int visitems = 10,
	              bool managed = true) ;
      FrWSelectionBox(FrWidget *parent, FrList *items, char *label,
		      XtCallbackProc ok_cb, XtPointer ok_data,
		      XtCallbackProc nomatch_cb, XtPointer nomatch_data,
		      XtCallbackProc apply_cb, XtPointer apply_data,
		      const char **helptexts = 0,
		      bool must_match = true,int visitems = 10,
	              bool managed = true) ;
      virtual FrObjectType objType() const ;
      virtual const char *objTypeName() const ;
      virtual FrObjectType objSuperclass() const ;
      void unmanageChild(FrWSelBoxChild child) const ;
      void buttonLabel(FrWSelBoxChild button, const char *label) const ;
      static char *selectionText(XtPointer call_data) ;
      static FrObject *selectionFrObject(XtPointer call_data) ;
   } ;

#endif /* FrMOTIF */

/************************************************************************/
/************************************************************************/

typedef void FrKillProgHookFunc(const char *msg) ;
typedef Boolean Motif_workproc(XtPointer) ;

//----------------------------------------------------------------------

void FrInitializeMotif(const char *window_name, Widget parent,
		       int max_symbols = 0) ;
Widget FrInitializeMotif(int *orig_argc, char **orig_argv,
			 const char *maintitle, const char *msgtitle,
			 const char *icon_name,
			 bool allow_resize = true) ;
void FrShutdownMotif() ;
void set_killprog_hook(FrKillProgHookFunc *) ;

void run_Motif() ;
void add_Motif_workproc(Motif_workproc *, XtPointer) ;

XtErrorHandler set_Xlib_error_handler(XtErrorHandler) ;
XtErrorHandler set_Xlib_warning_handler(XtErrorHandler) ;

void process_pending_X_event() ;
void process_pending_X_events(Widget w) ;

void FrSoundBell(Widget,int) ;
void FrSoundBell(FrWidget *,int) ;

void FrRealizeWidget(FrWidget *w) ;
void FrRealizeWidget(Widget w) ;
void FrUnrealizeWidget(FrWidget *w) ;
void FrUnrealizeWidget(Widget w) ;
void FrRaiseWindow(FrWidget *w) ;
void FrRaiseWindow(Widget w) ;
void FrMapWidget(Widget w) ;
void FrUnmapWidget(Widget w) ;

Widget create_warning_popup(Widget, const char *message) ;
void remove_warning_popup(Widget) ;

#endif /* !__FRMOTIF_H_INCLUDED */

// end of file frmotif.h //
