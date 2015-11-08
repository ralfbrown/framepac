/************************************************************************/
/*									*/
/*  FramepaC  -- frame manipulation in C++				*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frctype3.cpp		character-manipulation functions	*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 2003,2006,2008,2009					*/
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

#include "frctype.h"
#include "frstring.h"

/************************************************************************/
/************************************************************************/

FrCharEncoding FrParseCharEncoding(const char *enc_name)
{
   if (enc_name && *enc_name)
      {
      if (Fr_strnicmp(enc_name,"Latin1",6) == 0 ||
	  Fr_strnicmp(enc_name,"Latin-1",7) == 0 ||
	  Fr_strnicmp(enc_name,"iso-8859-1",9) == 0 ||
	  Fr_strnicmp(enc_name,"iso8859-1",8) == 0 ||
	  Fr_stristr(enc_name,"8859-1") != 0)
	 return FrChEnc_Latin1 ;
      if (Fr_strnicmp(enc_name,"Latin2",6) == 0 ||
	  Fr_strnicmp(enc_name,"Latin-2",7) == 0 ||
	  Fr_strnicmp(enc_name,"iso-8859-2",9) == 0 ||
	  Fr_strnicmp(enc_name,"iso8859-2",8) == 0 ||
	  Fr_stristr(enc_name,"8859-2") != 0)
	 return FrChEnc_Latin2 ;
      if (Fr_strnicmp(enc_name,"Unicode",3) == 0 ||
	  Fr_strnicmp(enc_name,"UTF16",5) == 0 ||
	  Fr_strnicmp(enc_name,"UTF-16",6) == 0 ||
	  Fr_strnicmp(enc_name,"UCS-2",5) == 0 ||
	  Fr_strnicmp(enc_name,"UCS2",4) == 0)
	 return FrChEnc_Unicode ;
      if (Fr_strnicmp(enc_name,"EUC",3) == 0 ||
	  Fr_strnicmp(enc_name,"GB-2312",2) == 0 ||
	  Fr_strnicmp(enc_name,"JIS",3) == 0)
	 return FrChEnc_EUC ;
      if (Fr_strnicmp(enc_name,"RawOctets",3) == 0)
	 return FrChEnc_RawOctets ;
      if (Fr_strnicmp(enc_name,"UTF8",4) == 0 ||
	  Fr_strnicmp(enc_name,"UTF-8",5) == 0 ||
	  Fr_strnicmp(enc_name,"U8",2) == 0)
	 return FrChEnc_UTF8 ;
      if (Fr_strnicmp(enc_name,"User",4) == 0)
	 return FrChEnc_User ;
      }
#ifdef FrDEFAULT_CHARSET_Latin1
   return FrChEnc_Latin1 ;
#else
   return FrChEnc_Latin2 ;
#endif
}

//----------------------------------------------------------------------

static const char *encoding_names[] =
   {
      "Latin-1",
      "Latin-2",
      "Unicode",
      "EUC",
      "RawOctets",
      "User",
      "UTF8"
   } ;

const char *FrCharEncodingName(FrCharEncoding enc)
{
   if ((unsigned)enc < lengthof(encoding_names))
      return encoding_names[enc] ;
   else
      return "Unknown" ;
}

// end of file frctype3.cpp //
