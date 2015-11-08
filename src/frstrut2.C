/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File: frstrut2.cpp	 	string-manipulation utility functions	*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994,1995,1996,1997,1998,1999,2000,2001,2002,2009	*/
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

#include <string.h>
#include "framerr.h"
#include "frbytord.h"
#include "frctype.h"
#include "frstring.h"
#include "frunicod.h"

/************************************************************************/
/*	Helper Functions						*/
/************************************************************************/

static void bad_char_width(const char *func)
{
   FrProgErrorVA("bad character width in %s",func) ;
}

/************************************************************************/
/************************************************************************/

FrString *FrDecanonicalizeSentence(const char *sentence, FrCharEncoding enc)
{
   if (!sentence)
      return new FrString("") ;
   else if (FrChEnc_Unicode == enc)
      return FrDecanonicalizeUSentence(sentence,true) ;
   else
      {
      size_t length = strlen(sentence) ;
      char *result = FrNewN(char,length+1) ;
      const unsigned char *map = FrLowercaseTable(enc) ;
      for (size_t i = 0 ; i <= length ; i++)
	 result[i] = (char)map[(unsigned char)sentence[i]] ;
      return new FrString(result,length,sizeof(char),false) ;
      }
}

//----------------------------------------------------------------------

FrString *FrFirstWord(const FrString *words)
{
   if (!words)
      return 0 ;
   switch (words->charWidth())
      {
      case 1:
	 {
	 const char *wstring = (char*)words->stringValue() ;
	 const char *space = (char*)memchr(wstring,' ',words->stringLength()) ;
	 if (space)
	    return new FrString(wstring,space-wstring) ;
	 }
	 break ;
      case 2:
	 {
	 const FrChar16 *wstring = (FrChar16*)words->stringValue() ;
	 const FrChar16 *space = wstring ;
	 const FrChar16 *end = wstring + words->stringLength() ;
	 while (space < end && *space != ' ')
	    space++ ;
	 if (space < end)
	    return new FrString(wstring,space-wstring,2) ;
	 }
	 break ;
      case 4:
	 {
	 const FrChar_t *wstring = (FrChar_t*)words->stringValue() ;
	 const FrChar_t *space = wstring ;
	 const FrChar_t *end = wstring + words->stringLength() ;
	 while (space < end && *space != ' ')
	    space++ ;
	 if (space < end)
	    return new FrString(wstring,space-wstring,4) ;
	 }
	 break ;
      default:
	 bad_char_width("FrFirstWord") ;
	 return 0 ;
      }
   // if we get here, the string had no blanks, or was of a type we can't
   // handle yet, so just duplicate it
   return (FrString*)words->deepcopy() ;
}

//----------------------------------------------------------------------

FrString *FrLastWord(const FrString *words)
{
   if (!words)
      return 0 ;
   if (!words->stringValue())
      return (FrString*)words->deepcopy() ;
   switch (words->charWidth())
      {
      case 1:
	 {
	 const char *wstring = (char*)words->stringValue() ;
	 const char *space = strrchr(wstring,' ') ;
	 if (space)
	    return new FrString(space+1,strlen(wstring)-(space-wstring)-1) ;
	 }
	 break ;
      case 2:
	 {
	 size_t length = words->stringLength() ;
	 if (length == 0)
	    break ;
	 const FrChar16 *wstring = (FrChar16*)words->stringValue() ;
	 const FrChar16 *end = wstring + length ;
	 const FrChar16 *space = end-1 ;
	 // scan for the last blank in the string
	 while (space >= wstring)
	    {
	    if (*space == ' ')
	       return new FrString(space+1,end-space-1,2) ;
	    else
	       space-- ;
	    }
	 }
	 break ;
      case 4:
	 {
	 size_t length = words->stringLength() ;
	 if (length == 0)
	    break ;
	 const FrChar_t *wstring = (FrChar_t*)words->stringValue() ;
	 const FrChar_t *end = wstring + length ;
	 const FrChar_t *space = end-1 ;
	 // scan for the last blank in the string
	 while (space >= wstring)
	    {
	    if (*space == ' ')
	       return new FrString(space+1,end-space-1,4) ;
	    else
	       space-- ;
	    }
	 }
	 break ;
      default:
	 bad_char_width("FrLastWord") ;
	 return (FrString*)words->deepcopy() ;
      }
   // if we get here, the string had no blanks, or was of a type we can't
   // handle yet, so just duplicate it
   return (FrString*)words->deepcopy() ;
}

//----------------------------------------------------------------------

FrString *FrButLastWord(const FrString *words, size_t count)
{
   if (!words)
      return 0 ;
   else if (count == 0)
      return (FrString*)words->deepcopy() ;
   switch (words->charWidth())
      {
      case 1:
	 {
	 const char *wstring = (char*)words->stringValue() ;
	 const char *space = strchr(wstring,'\0') ;
	 for (size_t i = 0 ; i < count && space > wstring ; i++)
	    {
	    // scan forward to next blank
	    space-- ;
	    while (space > wstring && !Fr_isspace(space[-1]))
	       space-- ;
	    // if multiple consecutive blanks, skip forward to first one
	    while (space > wstring && Fr_isspace(space[-1]))
	       space-- ;
	    }
	 if (space > wstring)
	    return new FrString(wstring,space-wstring) ;
	 }
	 break ;
      case 2:
	 {
	 size_t length = words->stringLength() ;
	 if (length == 0)
	    break ;
	 const FrChar16 *wstring = (FrChar16*)words->stringValue() ;
	 const FrChar16 *end = wstring + length ;
	 const FrChar16 *space = end ;
	 // scan for the Nth-last blank in the string
	 for (size_t i = 0 ; i < count && space > wstring ; i++)
	    {
	    // scan forward to next blank
	    space-- ;
	    for ( ; space > wstring ; space--)
	       {
	       FrChar16 c = FrLoadShort(&space[-1]) ;
	       if (Fr_is8bit(c) && Fr_isspace(c))
		  break ;
	       }
	    // if multiple consecutive blanks, skip forward to first one
	    for ( ; space > wstring ; space--)
	       {
	       FrChar16 c = FrLoadShort(&space[-1]) ;
	       if (!Fr_is8bit(c) || !Fr_isspace(c))
		  break ;
	       }
	    }
	 if (space > wstring)
	    return new FrString((char*)wstring,space-wstring,2) ;
	 }
	 break ;
      case 4:
	 {
	 size_t length = words->stringLength() ;
	 if (length == 0)
	    break ;
	 const FrChar_t *wstring = (FrChar_t*)words->stringValue() ;
	 const FrChar_t *end = wstring + length ;
	 const FrChar_t *space = end-1 ;
	 // scan for the Nth-last blank in the string
	 for (size_t i = 0 ; i < count && space > wstring ; i++)
	    {
	    // scan forward to next blank
	    space-- ;
	    for ( ; space > wstring ; space--)
	       {
	       FrChar_t c = FrLoadLong(&space[-1]) ;
	       if (Fr_is8bit(c) && Fr_isspace(c))
		  break ;
	       }
	    // if multiple consecutive blanks, skip forward to first one
	    for ( ; space > wstring ; space--)
	       {
	       FrChar_t c = FrLoadLong(&space[-1]) ;
	       if (!Fr_is8bit(c) || !Fr_isspace(c))
		  break ;
	       }
	    }
	 if (space > wstring)
	    return new FrString((char*)wstring,space-wstring,4) ;
	 }
	 break ;
      default:
	 bad_char_width("FrButLastWord") ;
	 return (FrString*)words->deepcopy() ;
      }
   // if we get here, the string had fewer blanks than the number of words
   //   we were asked to remove, or was of a type we can't handle yet, so
   //   return an empty string
   return new FrString("",0,words->charWidth()) ;
}

// end of file frstrut2.cpp //
