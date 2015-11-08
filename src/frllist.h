/************************************************************************/
/*									*/
/*  FramepaC								*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frllist.h		Linked-List Utility Templates		*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 2003,2004 Ralf Brown/Carnegie Mellon University	*/
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

#ifndef __FRLLIST_H_INCLUDED
#define __FRLLIST_H_INCLUDED

//========================================================================
//=  All of the templates in this header require the following member    =
//=  functions:								 =
//=	T* T::next() const						 =
//=	void T::setNext(T*)						 =
//========================================================================

template <class T> T* FrLinkListReverse(T *list)
{
   T *prev = 0 ;
   if (!list)
      return 0 ;
   T *next ;
   while ((next = list->next()) != 0)
      {
      list->setNext(prev) ;
      prev = list ;
      list = next ;
      }
   list->setNext(prev) ;
   return list ;
}

//----------------------------------------------------------------------

template <class T> void FrLinkListDelete(T *list)
{
   while (list)
      {
      T *nxt = list->next() ;
      delete list ;
      list = nxt ;
      }
   return ;
}

//----------------------------------------------------------------------

template <class T> T* FrLinkListNconc(T *list1, T *list2)
{
   if (!list1)
      return list2 ;
   else if (!list2)
      return list1 ;
   T *tmp = list1 ;
   T *prev ;
   do {
      prev = tmp ;
      tmp = tmp->next() ;
      } while (tmp) ;
   prev->setNext(list2) ;
   return list1 ;
}

//----------------------------------------------------------------------

template <class T> size_t FrLinkListLength(const T *list)
{
   size_t len = 0 ;
   for ( ; list ; list = list->next())
      len++ ;
   return len ;
}

//----------------------------------------------------------------------

#endif /* !__FRLLIST_H_INCLUDED */

// end of file frllist.h //
