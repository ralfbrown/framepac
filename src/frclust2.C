/****************************** -*- C++ -*- *****************************/
/*									*/
/*  FramepaC								*/
/*  Version 2.01							*/
/*	by Ralf Brown <ralf@cs.cmu.edu>					*/
/*									*/
/*  File: frclust2.cpp	      term-vector clustering support funcs	*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 2002,2003,2004,2005,2006,2009,2015			*/
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

#include <stdlib.h>
#include "frclust.h"
#include "frstring.h"
#include "frutil.h"

/************************************************************************/
/*	Types for this module						*/
/************************************************************************/

struct Metric_Name
   {
   FrClusteringMeasure measure ;
   const char *name ;
   } ;

/************************************************************************/
/*	Global Data for this module					*/
/************************************************************************/

static Metric_Name metric_names[] =
   { { FrCM_NONE, 		"none" },
     { FrCM_USER, 		"user" },
     { FrCM_COSINE, 		"Cosine" },
     { FrCM_MANHATTAN, 		"Manhattan" },
     { FrCM_EUCLIDEAN, 		"Euclidean" },
     { FrCM_DICE, 		"Dice" },
     { FrCM_ANTIDICE, 		"AntiDice" },
     { FrCM_JACCARD, 		"Jaccard" },
     { FrCM_TANIMOTO, 		"Tanimoto" },
     { FrCM_SIMPSON, 		"Simpson" },
     { FrCM_EXTSIMPSON, 	"ExtSimpson" },
     { FrCM_KULCZYNSKI1, 	"Kulczynski1" },
     { FrCM_KULCZYNSKI2, 	"Kulczynski2" },
     { FrCM_OCHIAI, 		"Ochiai" },
     { FrCM_SOKALSNEATH, 	"Sokal-Sneath" },
     { FrCM_MCCONNAUGHEY, 	"McConnaughey" },
     { FrCM_LANCEWILLIAMS, 	"Lance-Williams" },
     { FrCM_BRAYCURTIS, 	"Bray-Curtis" },
     { FrCM_CANBERRA, 		"Canberra" },
     { FrCM_CIRCLEPROD, 	"CircleProduct" },
     { FrCM_CZEKANOWSKI, 	"Czekanowski" },
     { FrCM_ROBINSON, 		"Robinson" },
     { FrCM_DRENNAN, 		"Drennan" },
     { FrCM_SIMILARITYRATIO, 	"SimilarityRatio" },
     { FrCM_JENSENSHANNON, 	"Jensen-Shannon" },
     { FrCM_MOUNTFORD,		"Mountford" },
     { FrCM_FAGER_MCGOWAN,	"Fager-McGowan" },
     { FrCM_BRAUN_BLANQUET,	"Braun-Blanquet" },
     { FrCM_TRIPARTITE,		"Tripartite" },
     { FrCM_BIN_DICE,		"BinaryDice" },
     { FrCM_BIN_ANTIDICE,	"BinaryAntiDice" },
     { FrCM_BIN_GAMMA,		"BinaryGamma" },	
   } ;

/************************************************************************/
/************************************************************************/

FrClusteringMethod FrParseClusterMethod(const char *meth, ostream *out)
{
   if (!meth)
      meth = "" ;
   FrClusteringMethod method(FrCM_GROUPAVERAGE) ;
   if (Fr_stricmp(meth,"HELP") == 0)
      meth = "" ;	// ensure that we have a method name that shows help
   if (Fr_strnicmp(meth,"GAC2",4) == 0 || Fr_strnicmp(meth,"INCR2",5) == 0 ||
       Fr_strnicmp(meth,"MGAC",4) == 0)
      method = FrCM_MULTIPASS_GAC ;
   else if (Fr_strnicmp(meth,"GAC",3) == 0 || Fr_strnicmp(meth,"INCR",4) == 0)
      method = FrCM_GROUPAVERAGE ;
   else if (Fr_strnicmp(meth,"AGGLOM",3) == 0)
      method = FrCM_AGGLOMERATIVE ;
   else if (Fr_strnicmp(meth,"KMEANS_H",8)  == 0 ||
	    Fr_strnicmp(meth,"HM",2) == 0)
      method = FrCM_KMEANS_HCLUST ;
   else if (Fr_strnicmp(meth,"KMEANS",2)  == 0)
      method = FrCM_KMEANS ;
   else if (Fr_strnicmp(meth,"TIGHT",2) == 0)
      method = FrCM_TIGHT ;
   else if (Fr_strnicmp(meth,"GROWSEEDS",1) == 0)
      method = FrCM_GROWSEED ;
   else if (Fr_strnicmp(meth,"SPECTRAL",1) == 0)
      method = FrCM_SPECTRAL ;
#if 0
   else if (Fr_strnicmp(meth,"XMEANS",2)  == 0)
      method = FrCM_XMEANS ;
   else if (Fr_strnicmp(meth,"DET",3) == 0 || Fr_strnicmp(meth,"ANN",3) == 0)
      method = FrCM_DET_ANNEAL ;
   else if (Fr_strnicmp(meth,"BUCK",4) == 0)
      method = FrCM_BUCKSHOT ;
#endif /* 0 */
   else if (out)
      {
      (*out) << "Unrecognized clustering method; valid method names are:\n"
  	        "\tINCR\t\tincremental group-average clustering (single-link)\n"
	        "\tINCR2\t\ttwo-pass incremental\n"
	        "\tAGG\t\tbottom-up agglomerative clustering\n"
	        "\tKMEANS\t\tk-Means clustering with random seeds\n"
	        "\tKMEANS_H\tk-Means with hierarchical pre-clustering\n"
		"\tTIGHT\t\tTseng's Tight Clustering\n"
		"\tGROWSEEDS\tGrow clusters from specified seeds via nearest neighbors\n"
		"\tSPECTRAL\tadaptive spectral clustering\n"
#if 0
	        "\tXMEANS\tx-Means clustering (k-Means with adaptive k)\n"
	        "\tANNEAL\tdeterministic annealing\n"
	        "\tBUCKSH\tk-Means with subsampling for initial centers\n"
#endif /* 0 */
	     << endl ;
      }
   return method ;
}

//----------------------------------------------------------------------

FrClusteringRep FrParseClusterRep(const char *name, ostream *out)
{
   FrClusteringRep rep(FrCR_CENTROID) ;

   if (Fr_strnicmp(name,"CEN",3) == 0 )
      rep = FrCR_CENTROID ;
   else if (Fr_strnicmp(name,"N",1) == 0)
      rep = FrCR_NEAREST ;
   else if (Fr_strnicmp(name,"AV",2) == 0)
      rep = FrCR_AVERAGE ;
   else if (Fr_strnicmp(name,"RMS",3) == 0)
      rep = FrCR_RMS ;
   else if (Fr_strnicmp(name,"FUR",3) == 0)
      rep = FrCR_FURTHEST ;
   else if (out)
      {
      (*out) << "Unrecognized cluster representative; valid names are:\n"
		"\tCENT\tuse cluster centroid\n"
	   	"\tNEAR\tuse nearest pair of members\n"
		"\tFURTH\tuse furthest pair of members\n"
		"\tAVG\tuse average of members' similarity scores\n"
		"\tRMS\tuse root-mean-square of members' similarity scores\n"
	     << endl ;
      }
   return rep ;
}

//----------------------------------------------------------------------

FrClusteringMeasure FrParseClusterMetric(const char *meas, ostream *out)
{
   FrClusteringMeasure measure(FrCM_COSINE) ;

   if (meas)
      {
      const Metric_Name *found = 0 ;
      size_t namelen = strlen(meas) ;
      const char *comma = strchr(meas,',') ;
      if (comma)
	 namelen = comma - meas ;
      for (size_t i = 0 ; i < lengthof(metric_names) ; i++)
	 {
	 if (Fr_strnicmp(metric_names[i].name,meas,namelen) == 0)
	    {
	    if (found)
	       {
	       found = 0 ;		// uh, oh!  it's ambiguous
	       break ;
	       }
	    else
	       found = &metric_names[i] ;
	    }
	 }
      if (found)
	 {
	 if ((int)found->measure != (found - metric_names))
	    FrProgError("out-dated metric_names array!") ;
	 measure = found->measure ;
	 }
      else if (Fr_strnicmp(meas,"KUL1",4) == 0)
	 measure = FrCM_KULCZYNSKI1 ;
      else if (Fr_strnicmp(meas,"KUL2",4) == 0)
	 measure = FrCM_KULCZYNSKI2 ;
      else if (Fr_stricmp(meas,"BC") == 0)
	 measure = FrCM_BRAYCURTIS ;
      else if (Fr_stricmp(meas,"CP") == 0)
	 measure = FrCM_CIRCLEPROD ;
      else if (Fr_stricmp(meas,"LW") == 0)
	 measure = FrCM_LANCEWILLIAMS ;
      else if (Fr_strnicmp(meas,"SIMRATIO",4) == 0 ||
	       Fr_stricmp(meas,"SR") == 0)
	 measure = FrCM_SIMILARITYRATIO ;
      else if (Fr_stricmp(meas,"JS") == 0)
	 measure = FrCM_JENSENSHANNON ;
      else if (Fr_stricmp(meas,"TSI") == 0)
	 measure = FrCM_TRIPARTITE ;
      else if (out)
	 {
	 (*out) <<"Unrecognized similarity metric " << meas
		<< "; valid names are:\n"
		   "\tANTI\tAnti-Dice measure\n"
	           "\tBC\tBray-Curtis measure\n"
	           "\tCAN\tCanberra measure\n"
	           "\tCOS\tcosine similarity between cluster representatives\n"
	           "\tCP\tCircle Product\n"
		   "\tCZEK\tCzekanowski measure\n"
		   "\tDICE\tDice's coefficient (2 * prod / sum)\n"
		   "\tDRENN\tDrennan's dissimilarity\n"
		   "\tEUCL\tEuclidean distance between cluster representatives\n"
	           "\tEXTSIM\tExtended Simpson's coefficient\n"
		   "\tJACC\tJaccard's coefficient (|intersection|/|union|)\n"
	           "\tJS\tJensen-Shannon divergence\n"
		   "\tKUL1\tKulczynski measure #1\n"
		   "\tKUL2\tKulczynski measure #2\n"
		   "\tLW\tLance-Williams distance\n"
	           "\tMAN\tManhattan distance\n"
		   "\tMCC\tMcConnaughey measure\n"
		   "\tMOUNT\tMountford coefficient\n"
		   "\tOCH\tOchiai measure\n"
		   "\tROB\tRobinson coefficient\n"
		   "\tSIMP\tSimpson's coefficient (|inters|/min(|vec1|,|vec2|))\n"
		   "\tSOK\tSokal-Sneath measure\n"
		   "\tSR\tsimilarity ratio\n"
		   "\tTAN\tTanimoto's coefficient (real-valued Jaccard)\n"
		   "\tTSI\tTripartite Similarity Index\n"
		<< endl ;
	 }

      }
   return measure ;
}

//----------------------------------------------------------------------

const char *FrClusteringMetricName(FrClusteringMeasure meas)
{
   size_t val1 = (size_t)meas ;
   size_t val2 = (size_t)lengthof(metric_names) ;
   if (val1 >= val2)
      return "(invalid)" ;
   if (metric_names[meas].measure == meas)
      return metric_names[meas].name ;
   FrProgError("inconsistency between FrClusteringMeasure values and names") ;
   return 0 ;
}

//----------------------------------------------------------------------

static bool parse_param(const char *value, const char */* end */,
			  bool *variable)
{
   if (Fr_toupper(*value) == 'Y' || *value == '1' || Fr_toupper(*value) == 'T')
      *variable = true ;
   else if (Fr_toupper(*value) == 'N' || *value == '0' ||
	    Fr_toupper(*value) == 'F')
      *variable = false ;
   else
      return false ;
   return true ;
}

//----------------------------------------------------------------------

static bool parse_param(const char *value, const char *end,
			  size_t *variable)
{
   char *convert_end ;
   size_t val = (size_t)strtol(value,&convert_end,0) ;
   if (convert_end  && convert_end != value && convert_end <= end)
      {
      *variable = val ;
      return true ;
      }
   return false ;
}

//----------------------------------------------------------------------

static bool parse_param(const char *value, const char *end,
			  double *variable)
{
   char *convert_end ;
   double val = strtod(value,&convert_end) ;
   if (convert_end  && convert_end != value && convert_end <= end)
      {
      *variable = val ;
      return true ;
      }
   return false ;
}

//----------------------------------------------------------------------

bool FrClusteringParameters::parseParameter(const char *parm,
					      const char *parm_end)
{
   const char *equals = strchr(parm,'=') ;
   if (!equals)
      return false ;
   equals++ ;
   if (Fr_strnicmp(parm,"alpha",1) == 0)
      return parse_param(equals,parm_end,&m_alpha) ;
   else if (Fr_strnicmp(parm,"beta",1) == 0)
      return parse_param(equals,parm_end,&m_beta) ;
   else if (Fr_strnicmp(parm,"threshold",1) == 0)
      return parse_param(equals,parm_end,&default_threshold) ;
   else if (Fr_strnicmp(parm,"clusters",2) == 0)
      return parse_param(equals,parm_end,&desired_clusters) ;
   else if (Fr_strnicmp(parm,"iterations",1) == 0)
      return parse_param(equals,parm_end,&max_iterations) ;
   else if (Fr_strnicmp(parm,"backoffsize",1) == 0)
      return parse_param(equals,parm_end,&backoff_stepsize) ;
   else if (Fr_strnicmp(parm,"cachesize",2) == 0)
      return parse_param(equals,parm_end,&cache_size) ;
   else if (Fr_strnicmp(parm,"sumsizes",1) == 0)
      return parse_param(equals,parm_end,&sum_cluster_sizes) ;
   else if (Fr_strnicmp(parm,"threads",1) == 0)
      return parse_param(equals,parm_end,&num_threads) ;
   else if (Fr_strnicmp(parm,"method",3) == 0)
      {
      clus_method = FrParseClusterMethod(equals,0) ;
      return true ;
      }
   else if (Fr_strnicmp(parm,"measure",3) == 0)
      {
      sim_measure = FrParseClusterMetric(equals,0) ;
      return true ;
      }
   else if (Fr_strnicmp(parm,"representative",1) == 0)
      {
      clus_rep = FrParseClusterRep(equals,0) ;
      return true ;
      }
   else
      return false ;
}

//----------------------------------------------------------------------

bool FrParseClusteringParams(const char *spec,
			       FrClusteringParameters *params)
{
   if (params && spec)
      {
      while (FrSkipWhitespace(spec))
	 {
	 const char *end = strchr(spec,',') ;
	 if (!end)
	    end = strchr(spec,'\0') ;
	 if (!params->parseParameter(spec,end))
	    return false ;
	 spec = end ;
	 if (*spec)
	    spec++ ;
	 }
      return true ;
      }
   return false ;
}

// end of file frclust2.cpp //
