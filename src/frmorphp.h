/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC								*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File frmorphp.h	morphology-marker parsing functions		*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 2006,2007,2009 Ralf Brown/Carnegie Mellon University	*/
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

#ifndef __FRMORPHP_H_INCLUDED
#define __FRMORPHP_H_INCLUDED

#ifndef __FRAMEPAC_H_INCLUDED
#include "FramepaC.h"
#endif

#ifndef FRTXTSPN_H_INCLUDED
#include "frtxtspn.h"
#endif

/************************************************************************/
/************************************************************************/

class FrNamedEntitySpec ; // opaque type

/************************************************************************/
/************************************************************************/

void FrSetMorphologyMarkers(const char *intro, const char *separator,
			    const char *assignment = 0) ;
void FrClearMorphologyMarkers() ;
void FrSetMorphologyFeatures(bool use,
			     const unsigned char *uppercase_map = 0) ;

FrSymbol *FrClassifyMorphologyTag(FrSymbol *morph, FrSymbol *&value,
				  const FrList *morphology_classes) ;
void FrClassifyMorphologyTag(FrSymbol *morph, FrSymbol *value,
			     const FrList *morphology_classes,
			     FrList *&by_class,
			     bool keep_global_info = false) ;
void FrClassifyMorphologyTag(const char *morph,
			     const FrList *morphology_classes,
			     FrList *&by_class,
			     bool keep_global_info = false) ;

bool FrParseMorphologyData(FrTextSpan *span, FrTextSpans *lattice,
			     const FrList *morphology_classes,
			     bool replace_original_span = false,
			     bool keep_global_info = false) ;
bool FrParseMorphologyData(FrTextSpans *lattice,
			     const FrList *morphology_classes,
			     bool replace_original_span = false,
			     bool keep_global_info = false) ;
FrList *FrParseMorphologyData(const char *word,
			      const FrList *morphology_classes,
			      bool keep_global_info) ;

FrNamedEntitySpec *FrParseNamedEntitySpec(const char *spec,
					  bool case_sensitive = false,
					  double def_conf = 0.0) ;
void FrFreeNamedEntitySpec(FrNamedEntitySpec *spec) ;
void FrNamedEntitySetConf(FrNamedEntitySpec *spec, double conf) ;

bool FrParseNamedEntityData(FrTextSpans *lattice,
			      const FrNamedEntitySpec *entity_spec,
			      bool replace_original_span = false) ;
bool FrParseNamedEntityData(FrTextSpans *lattice,
			      const char *entity_spec,
			      bool replace_original_span = false) ;

char *FrStripNamedEntityData(const char *string,
			     const FrNamedEntitySpec *entity_spec) ;

#endif /* !__FRMORPHP_H_INCLUDED */

// end of file frmorphp.h //
