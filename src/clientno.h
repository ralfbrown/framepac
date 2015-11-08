/************************************************************************/
/*									*/
/*  Mikrokosmos Client-Server Protocol					*/
/*									*/
/*  File clientno.h	Well-known client identifiers			*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1994 Ralf Brown/Carnegie Mellon University		*/
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

#ifndef __CLIENTNO_H
#define __CLIENTNO_H

#define MIKCLIENT_UNKNOWN  	0x0000
#define MIKCLIENT_VFRAME   	0x0001 // VFrame class using server as backing store
#define MIKCLIENT_FRAMEPAC 	0x0002 // FramepaC test program
#define MIKCLIENT_KARAT    	0x0003 // MikroKARAT environment
#define MIKCLIENT_TALKWIDGET   	0x0004 // MikroKARAT talk widget
#define MIKCLIENT_QUERY		0x0005 // MikroKARAT search interface

#define MIKCLIENT_UNREGISTERED 	0xFFFF

// client-to-client message types

#define CLIMSG_SOCKETADDR       0x0000
// types 0x0001 to 0x00FF reserved for future use
#define CLIMSG_TALKREQUEST	0x0100

#endif /* __CLIENTNO_H */

// end of file clientno.h //
