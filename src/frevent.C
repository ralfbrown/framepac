/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frevent.cpp	       event lists				*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1995,1996,1997,2001,2009				*/
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

#if defined(__GNUC__)
#  pragma implementation "frevent.h"
#endif

#include "frevent.h"

/************************************************************************/
/*	Types for this module						*/
/************************************************************************/

class FrEvent
   {
   private:
      time_t event_time ;
      FrEventFunc *func ;
      FrEventCleanupFunc *cleanfunc ;
      void *client_data ;
      FrEvent *next, *prev ;
   public:
      FrEvent(time_t time, FrEventFunc *f, void *client_data,
	      bool delta = false, FrEventCleanupFunc *cl = 0) ;
      ~FrEvent() ;
   //friends
      friend class FrEventList ;
   } ;

/************************************************************************/
/*    Methods for class FrEvent						*/
/************************************************************************/

FrEvent::FrEvent(time_t evtime, FrEventFunc *f, void *userdata, bool delta,
		 FrEventCleanupFunc *cl)
{
   if (delta)
      evtime += time(0) ;
   func = f ;
   cleanfunc = cl ;
   client_data = userdata ;
   event_time = evtime ;
   next = 0 ;
   prev = 0 ;
   return ;
}

//----------------------------------------------------------------------

FrEvent::~FrEvent()
{
   if (cleanfunc)
      cleanfunc(client_data) ;
   client_data = 0 ;
   next = 0 ;
   prev = 0 ;
   return ;
}

/************************************************************************/
/*    Methods for class FrEventList					*/
/************************************************************************/

FrEventList::FrEventList()
{
   events = 0 ;
   return ;
}

//----------------------------------------------------------------------

FrEventList::~FrEventList()
{
   while (events)
      removeEvent(events) ;
   return ;
}

//----------------------------------------------------------------------

void FrEventList::insert(FrEvent *event)
{
   if (!events)
      {
      event->next = 0 ;
      event->prev = 0 ;
      events = event ;
      }
   else
      {
      time_t newtime = event->event_time ;
      for (FrEvent *e = events ; e ; e = e->next)
	 {
	 if (e->event_time > newtime)
	    {
	    event->next = e ;
	    FrEvent *p = e->prev ;
	    event->prev = p ;
	    e->prev = event ;
	    if (p)
	       p->next = event ;
	    else
	       events = event ;
	    break ;
	    }
	 else if (!e->next)
	    {
	    event->next = 0 ;
	    event->prev = e ;
	    e->next = event ;
	    break ;
	    }
	 }
      }
   return ;
}

//----------------------------------------------------------------------

FrEvent *FrEventList::addEvent(time_t time, FrEventFunc *f, void *userdata,
			       bool delta, FrEventCleanupFunc *cl)
{
   if (!f)
      return 0 ;
   FrEvent *event = new FrEvent(time,f,userdata,delta,cl) ;
   insert(event) ;
   return event ;
}

//----------------------------------------------------------------------

bool FrEventList::removeEvent(FrEvent *event)
{
   if (!event)
      return false ;
   FrEvent *p, *n ;
   p = event->prev ;
   n = event->next ;
   if (n)
      n->prev = p ;
   if (p)
      p->next = n ;
   else
      events = n ;
   delete event ;
   return true ;
}

//----------------------------------------------------------------------

void FrEventList::reschedule(FrEvent *event, time_t newtime)
{
   if (!event)
      return ;
   // first, unlink the event from the event list
   FrEvent *p, *n ;
   p = event->prev ;
   n = event->next ;
   if (n)
      n->prev = p ;
   if (p)
      p->next = n ;
   else
      events = n ;
   // now, update the event's time
   event->event_time = newtime ;
   // and finally, add the event back to the event list at the proper point
   insert(event) ;
   return ;
}

//----------------------------------------------------------------------

void FrEventList::postpone(FrEvent *event, time_t delta)
{
   if (event)
      reschedule(event,event->event_time + delta) ;
   return ;
}

//----------------------------------------------------------------------

void FrEventList::executeEvents()
{
   time_t t = time(0) ;
   while (events && events->event_time <= t)
      {
      time_t newtime = events->func(events->client_data) ;
      FrEvent *e = events ;
      events = events->next ;
      if (events)
	 events->prev = 0 ;
      if (newtime > t)
	 {
	 e->event_time = newtime ;
	 insert(e) ;
	 }
      else
	 delete e ;
      }
   return ;
}

//----------------------------------------------------------------------

void *FrEventList::getUserData(FrEvent *event) const
{
   return event ? event->client_data : 0 ;
}

// end of file frevent.cpp //
