# Makefile for FramepaC
#   (using GCC under *nix, MSVC++ under Windows, or Borland/Watcom C++ under MS-DOS)
# Last change: 08nov2015

default:	help

#########################################################################
# define the locations of all the files

# the top of the source tree
# TOP = ../..
TOP = ..

# # do we use a /src directory or not?
# SRCSUBDIR=/src
# #SRCSUBDIR=

# # where to put the compiled library and required header files
INSTDIR = $(TOP)/include
#LIBINSTDIR = $(TOP)/lib
LIBINSTDIR=./lib

# # where to look for header files from other modules when compiling
# INCLUDEDIRS = -I$(TOP)/include

# #########################################################################
# define the compiler and its options
#
# uncomment the appropriate line for your compiler/OS below:
# (also check the 'include' file for further options)

#include makefile.unx		# GNU C under Unix (default is Solaris)
include makefile.lnx		# GNU C under Linux
#!include makefile.bcc		# Borland C++ under MS-DOS
#!include makefile.wcc	  	# Watcom C++ under MS-DOS, for DOS/4GW extender
#!include makefile.nt		# Watcom C++ under Windows NT/95
#!include makefile.vc		# Visual C++ under Windows (NT/95)

#########################################################################
#  define the various files to be used

# the name of the package being built by this makefile
PACKAGE = framepac

# the object modules to be included in the library file
OBJS = frobject$(OBJ) frlist$(OBJ) frlist2$(OBJ) frlist3$(OBJ) filist$(OBJ) \
	frnumber$(OBJ) frnum64$(OBJ) frfloat$(OBJ) frfeatvec$(OBJ) \
	frstring$(OBJ) frstrng1$(OBJ) frstrng2$(OBJ) frstrng3$(OBJ) \
	frsymtab$(OBJ) frgcc3$(OBJ) frprintf$(OBJ) frlocate$(OBJ) \
	frsymbol$(OBJ) framerr$(OBJ) frframe$(OBJ) frmd5$(OBJ) frframev$(OBJ) \
	frclient$(OBJ) frserver$(OBJ) frconnec$(OBJ) frhasht$(OBJ) \
	frlru$(OBJ) frlru2$(OBJ) frcmap$(OBJ) fralloc$(OBJ) \
	frmalloc$(OBJ) frmem$(OBJ) frmemp$(OBJ) frmemuse$(OBJ) frstruct$(OBJ) \
	frpacket$(OBJ) frmotif$(OBJ) frdathsh$(OBJ) frsntbrk$(OBJ) \
	frtimer$(OBJ) frtimer2$(OBJ) frbpriq$(OBJ) frtxtspn$(OBJ) \
	mikro_db$(OBJ) frameid$(OBJ) frdatfil$(OBJ) frmath$(OBJ) \
	frnetwrk$(OBJ) frnethsh$(OBJ) frqueue$(OBJ) frfinddb$(OBJ) \
	frunicod$(OBJ) fruniutl$(OBJ) framepac$(OBJ) vframe$(OBJ) \
	vframev$(OBJ) frexec$(OBJ) frsymfrm$(OBJ) frsymfr2$(OBJ) \
	vfinfo$(OBJ) vfinfo0$(OBJ) frtfidf$(OBJ) frvecsim$(OBJ) \
	finumber$(OBJ) fistring$(OBJ) fistruct$(OBJ) frhsort$(OBJ) \
	frinline$(OBJ) frutil$(OBJ) frpasswd$(OBJ) friindex$(OBJ) \
	frreader$(OBJ) fiframe$(OBJ) frevent$(OBJ) frpeer$(OBJ) \
	frsocket$(OBJ) frsckstr$(OBJ) frurl$(OBJ) frcognat$(OBJ) frcogn2$(OBJ)\
	frmswin$(OBJ) frhelp$(OBJ) frctype$(OBJ) frctype2$(OBJ) frctype3$(OBJ)\
	framekit$(OBJ) frsignal$(OBJ) frarray$(OBJ) frsarray$(OBJ) \
	frstrut1$(OBJ) frstrut2$(OBJ) frstrut3$(OBJ) frstrut4$(OBJ) \
	frstrut5$(OBJ) frstrut6$(OBJ) frstrut7$(OBJ) frstrutl$(OBJ) \
	frhtml$(OBJ) frslfreg$(OBJ) frcritsec$(OBJ) frspell$(OBJ) \
	frstack$(OBJ) frbitvec$(OBJ) frgeturl$(OBJ) frhttp$(OBJ) frftp$(OBJ) \
	frbwt$(OBJ) frbwt2$(OBJ) frbwtgen$(OBJ) frbwtloc$(OBJ) frbwtlc2$(OBJ) \
	frbwtdmp$(OBJ) frbwtcmp$(OBJ) frvocab$(OBJ) frcpfile$(OBJ) \
	frfilutl$(OBJ) frfilut2$(OBJ) frfilut3$(OBJ) frfilut4$(OBJ) \
	frfilut5$(OBJ) frabbrev$(OBJ) frmorphp$(OBJ) \
	frcfgfil$(OBJ) frslot0$(OBJ) frvars$(OBJ) frassert$(OBJ) \
	frmmap$(OBJ) frregexp$(OBJ) frwctype$(OBJ) frunistr$(OBJ) \
	frthresh$(OBJ) frtrmvec$(OBJ) frclusim$(OBJ) frclust$(OBJ) \
	frclust1$(OBJ) frclust2$(OBJ) frclust4$(OBJ) \
	frrandom$(OBJ) frtxtfil$(OBJ) frfcache$(OBJ) frhash$(OBJ) \
	frnetsrv$(OBJ) frthread$(OBJ) \
	$(EXTRAOBJS)
## not in LGPL version:
#	frclust3$(OBJ) frclust5$(OBJ) inv$(OBJ)

# the header files needed by applications using this library
HEADERS = FramepaC.h frconfig.h frcommon.h frobject.h frlist.h frstring.h \
	frnumber.h frfloat.h frsymbol.h frevent.h frsymtab.h frframe.h \
	frhasht.h frdathsh.h vframe.h framerr.h clientno.h frmotif.h \
	frqueue.h frmd5.h frstruct.h frlru.h frmem.h frpasswd.h frinline.h \
	frreader.h frexec.h frarray.h frsckstr.h frhelp.h frmswin.h \
	frctype.h frcfgfil.h frunicod.h frclisrv.h frbytord.h frtimer.h \
	frfilutl.h frbitvec.h frsignal.h frutil.h frassert.h frmmap.h \
	frregexp.h frhash.h frstack.h frnetsrv.h frtrmvec.h frspell.h \
	frclust.h frrandom.h frurl.h frlowio.h frovrrid.h frqsort.h \
	frhsort.h frllist.h frbpriq.h frcmove.h frbwt.h frvocab.h frsstrm.h \
	frmorphp.h frcritsec.h frfeatvec.h frmath.h
# workaround for systems where the built-in copy command doesn't handle multiple files
HEADERS_CP = *.h

# files to be included in the source distribution archive
DISTFILES = README COPYING GPL.txt makefile makefile.?? makefile.??? *.h *$(C) objs.lst vcobjs.lst \
	rblib.bat

# the library archive file for this module
LIBRARY = $(PACKAGE)$(LIB)

# the program to be built to test the package
TESTPROG = testfp

# the object modules needed to build the test program
TESTOBJS = $(TESTPROG)$(OBJ) benchmrk$(OBJ) testnet$(OBJ) parmem$(OBJ) parhash$(OBJ)

#########################################################################

all:	$(TESTPROG)$(EXE)

help:
	@echo "The makefile for $(PACKAGE) understands the following targets:"
	@echo "  all       perform a complete build of library and test program"
	@echo "  lib       build the library only"
	@echo "  install   build and install the library"
	@echo "  system    install the test program in the top-level directory"
	@echo "  clean     erase intermediate files from the build"
	@echo "  veryclean erase all intermediate and backup files"
	@echo "  bootstrap install files needed by packages on which we depend"
	@echo "  tags      rebuild TAGS database"
	@echo "  tar       pack up all distribution files in a tar archive"
	@echo "  compress  pack up all distribution files in a compressed tar file"
	@echo "  zip       pack up all distribution files in a PKzip archive"
	@echo "  purify    build a Purified executable (requires Purify)"
	@echo "  help      you're looking at it..."

lib:	$(LIBRARY)

clean:
	$(RM) *$(OBJ)
	$(RM) $(LIBRARY)

veryclean: clean
	$(RM) $(TESTPROG)$(EXE)
	$(RM) *.BAK
	$(RM) "#*#"
	$(RM) *.CKP *~

bootstrap:
	$(CP) $(HEADERS_CP) $(INSTDIR)

install: $(LIBINSTDIR)/$(LIBRARY)
	-$(TOUCH) $(LIBINSTDIR)/$(LIBRARY) $(BITBUCKET)

system: $(TOP)/$(TESTPROG)$(EXE)

tags:
	etags --c++ *.h *.C

tar:
	$(RM) $(PACKAGE).tar
	tar chf $(PACKAGE).tar $(DISTFILES)

compress:	tar
	compress $(PACKAGE).tar

zip:
	$(RM) $(PACKAGE).zip
	$(ZIP) $(ZIPFLAGS) $(PACKAGE) $(DISTFILES)

purify:	$(LIBRARY) $(TESTOBJS)
	purify $(CCLINK) $(LINKFLAGS) $(CFLAGEXE) $(TESTPROG)$(OBJ) \
		$(TESTOBJS) $(LIBRARY) $(USELIBS)

$(TESTPROG)$(EXE): $(LIBRARY) $(TESTOBJS)
	$(CCLINK) $(LINKFLAGS) $(CFLAGEXE) $(TESTOBJS) $(LIBRARY) $(USELIBS)

$(LIBRARY): $(OBJS)
	-$(RM) $(LIBRARY)
	$(LIBRARIAN) $(LIBFLAGS) $(LIBRARY) $(LIBOBJS)
	$(LIBINDEXER) $(LIBIDXFLAGS)

$(LIBINSTDIR)/$(LIBRARY): $(LIBRARY)
	$(CP) $(LIBRARY) $(LIBINSTDIR)
	$(CP) $(HEADERS_CP) $(INSTDIR)

$(TOP)/$(TESTPROG)$(EXE): $(TESTPROG)$(EXE)
	$(CP) $(TESTPROG)$(EXE) $(TOP)/$(TESTPROG)$(EXE)


#########################################################################

frabbrev$(OBJ):	 frabbrev$(C) frstring.h frfilutl.h frutil.h
fralloc$(OBJ):	 fralloc$(C) fr_mem.h
frameid$(OBJ):	 frameid$(C) frameid.h vframe.h
framepac$(OBJ):  framepac$(C) frpcglbl.h frpasswd.h frserver.h
framekit$(OBJ):	 framekit$(C) frprintf.h frpcglbl.h
framerr$(OBJ):   framerr$(C) framerr.h frprintf.h frconfig.h
frarray$(OBJ):	 frarray$(C) frarray.h framerr.h frreader.h frcmove.h \
		frpcglbl.h
frassert$(OBJ):	 frassert$(C) frassert.h frcommon.h framerr.h
frbitvec$(OBJ):	 frbitvec$(C) frbitvec.h frctype.h frnumber.h frreader.h \
		frcmove.h frpcglbl.h
frbpriq$(OBJ):	 frbpriq$(C) frbpriq.h
frbwt$(OBJ):	 frbwt$(C) frbwt.h fr_bwt.h frassert.h frfilutl.h frmmap.h \
		frutil.h
frbwt2$(OBJ):	 frbwt2$(C) frbwt.h frsymbol.h frstring.h frutil.h \
		frvocab.h
frbwtcmp$(OBJ):	 frbwtcmp$(C) frbwt.h fr_bwt.h frassert.h frfilutl.h frmmap.h
frbwtdmp$(OBJ):	 frbwtdmp$(C) frbwt.h
frbwtgen$(OBJ):	 frbwtgen$(C) frbwt.h frframe.h frsymtab.h frvocab.h
frbwtloc$(OBJ):  frbwtloc$(C) frbwt.h
frbwtlc2$(OBJ):  frbwtlc2$(C) frbwt.h
frcfgfil$(OBJ):	 frcfgfil$(C) framerr.h frctype.h frcfgfil.h frlist.h \
		frnumber.h frreader.h frstring.h frsymtab.h frutil.h \
		frprintf.h frfilutl.h
frclient$(OBJ):	 frclient$(C) frobject.h frconnec.h frclisrv.h frclient.h \
		frqueue.h frsignal.h frutil.h frpcglbl.h
frclusim$(OBJ):	 frclusim$(C) frclust.h frclustp.h frassert.h frnumber.h
frclust$(OBJ):	 frclust$(C) frclust.h frclustp.h frassert.h frnumber.h \
		frhash.h frsymtab.h frtimer.h frbpriq.h frcmove.h
frclust1$(OBJ):	 frclust1$(C) frclust.h frclustp.h frassert.h frqsort.h \
		frrandom.h frtimer.h
frclust2$(OBJ):	 frclust2$(C) frclust.h frstring.h frutil.h
frclust4$(OBJ):	 frclust4$(C) frclust.h frclustp.h
frcmap$(OBJ):	frcmap$(C) framerr.h frlist.h frnumber.h frstring.h
frcognat$(OBJ):  frcognat$(C) frstring.h frsymbol.h frctype.h frreader.h \
		frcmove.h frutil.h framerr.h
frcogn2$(OBJ):   frcogn2$(C) frlist.h frstring.h
frconnec$(OBJ):	 frconnec$(C) frclisrv.h frconnec.h frmswin.h frsckstr.h \
		framerr.h frctype.h frsignal.h
frcpfile$(OBJ):	 frcpfile$(C) frfilutl.h
frcritsec$(OBJ): frcritsec$(C) frcritsec.h frthread.h \
		frcritsec-linux$(C) frcritsec-pthread$(C) frcritsec-windows$(C)
frctype$(OBJ):	 frctype$(C) frctype.h
frctype2$(OBJ):	 frctype2$(C) frctype.h
frctype3$(OBJ):	 frctype3$(C) frctype.h frstring.h
frdatfil$(OBJ):  frdatfil$(C) frdatfil.h frstring.h frpcglbl.h mikro_db.h \
		frfinddb.h frlru.h frdathsh.h
frdathsh$(OBJ):	 frdathsh$(C) frpcglbl.h frdathsh.h frcmove.h
frevent$(OBJ):	 frevent$(C) frevent.h frcommon.h
frexec$(OBJ):	 frexec$(C) frexec.h frmem.h frsckstr.h framerr.h frutil.h \
		frfilutl.h frmswin.h frprintf.h
frfcache$(OBJ):	 frfcache$(C) frfilutl.h frutil.h frmem.h frbytord.h \
		frstring.h frsymbol.h
frfeatvec$(OBJ): frfeatvec$(C) frfeatvec.h framerr.h frqsort.h
frfilut2$(OBJ):	 frfilut2$(C) frfilutl.h frmem.h frstring.h frctype.h \
		frunicod.h frutil.h
frfilut3$(OBJ):	 frfilut3$(C) frfilutl.h frctype.h
frfilut4$(OBJ):	 frfilut4$(C) frfilutl.h framerr.h frctype.h
frfilut5$(OBJ):	 frfilut5$(C) frfilutl.h frlowio.h
frfilutl$(OBJ):	 frfilutl$(C) frfilutl.h frmem.h framerr.h frprintf.h frutil.h
frfinddb$(OBJ):	 frfinddb$(C) frfinddb.h frfilutl.h frlist.h frstring.h \
		framerr.h frutil.h
frfloat$(OBJ): 	 frfloat$(C) frfloat.h frprintf.h frpcglbl.h
frframe$(OBJ):   frframe$(C) frframe.h frpcglbl.h frlist.h frsymtab.h
frframev$(OBJ):  frframev$(C) frframe.h frpcglbl.h frlru.h
frftp$(OBJ):	 frftp$(C) frurl.h frctype.h frsckstr.h frstring.h frfilutl.h \
		frprintf.h
fiframe$(OBJ):	 fiframe$(C) frframe.h framerr.h frutil.h frpcglbl.h
filist$(OBJ):	 filist$(C) frlist.h framerr.h frreader.h frutil.h frpcglbl.h
finumber$(OBJ):	 finumber$(C) frnumber.h framerr.h frreader.h frpcglbl.h
fistring$(OBJ):	 fistring$(C) frctype.h frreader.h frstring.h frpcglbl.h
fistruct$(OBJ):  fistruct$(C) frstruct.h frutil.h frpcglbl.h
frgcc3$(OBJ):	 frgcc3$(C) frfilutl.h
frgeturl$(OBJ):	 frgeturl$(C) frcommon.h frctype.h frutil.h frmem.h frurl.h \
		framerr.h frstring.h frlist.h frnumber.h
frhasht$(OBJ):	 frhasht$(C) frhasht.h frcmove.h
frhelp$(OBJ):	 frhelp$(C) frhelp.h framerr.h frfilutl.h frprintf.h frutil.h
frhsort$(OBJ):	 frhsort$(C) frhsort.h
frhtml$(OBJ):	 frhtml$(C) frurl.h framerr.h frctype.h frlist.h frstring.h \
		frsymtab.h frutil.h
frhttp$(OBJ):	 frhttp$(C) frurl.h framerr.h frfilutl.h frsckstr.h \
		frstring.h frutil.h
friindex$(OBJ):	 friindex$(C) frlist.h frmem.h frhash.h frhasht.h frnumber.h
frinline$(OBJ):  frinline$(C) frinline.h frframe.h vframe.h
frlist$(OBJ):	 frlist$(C) frlist.h frqsort.h frutil.h frpcglbl.h
frlist2$(OBJ):	 frlist2$(C) frlist.h
frlist3$(OBJ):	 frlist3$(C) frlist.h
frlocate$(OBJ):	 frlocate$(C) frfilutl.h frprintf.h frutil.h
frlru$(OBJ):	 frlru$(C) frlru.h frconfig.h
frlru2$(OBJ):	 frlru2$(C) frlru.h frframe.h frsymtab.h frpcglbl.h
frmalloc$(OBJ):	 frmalloc$(C) frballoc.h fr_mem.h frmembin.h frmmap.h frprintf.h
frmath$(OBJ):	 frmath$(C) frmath.h
frmd5$(OBJ):	 frmd5$(C) frmd5.h frcommon.h framerr.h frbytord.h
frmem$(OBJ):     frmem$(C) frballoc.h fr_mem.h frmembin.h framerr.h frlru.h \
		frassert.h frprintf.h frpcglbl.h memcheck.h
frmemp$(OBJ):    frmemp$(C) fr_mem.h frmembin.h frballoc.h frassert.h \
		frprintf.h frthread.h frpcglbl.h memcheck.h
frmemuse$(OBJ):  frmemuse$(C) frballoc.h fr_mem.h frpcglbl.h
frmmap$(OBJ):	 frmmap$(C) frmmap.h frpcglbl.h
frmorphp$(OBJ):	 frmorphp$(C) frmorphp.h
frmotif$(OBJ):	 frmotif$(C) frmotif.h framerr.h frmem.h frctype.h frhelp.h \
		frstring.h vframe.h
frmswin$(OBJ):	 frmswin$(C) frconfig.h frcommon.h frtimer.h frmswin.h
frnetsrv$(OBJ):  frnetsrv$(C) frnetsrv.h frevent.h frexec.h frstring.h \
		framerr.h frreader.h frfilutl.h frctype.h
frnetwrk$(OBJ):  frnetwrk$(C) frnetwrk.h frpcglbl.h frsymtab.h frframe.h \
		clientno.h
frnethsh$(OBJ):	 frnethsh$(C) frnethsh.h frhash.h frhasht.h frameid.h frcmove.h \
		frpcglbl.h
frnumber$(OBJ):  frnumber$(C) frnumber.h frpcglbl.h
frnum64$(OBJ):   frnum64$(C) frnumber.h frpcglbl.h
frobject$(OBJ):  frobject$(C) frobject.h frpcglbl.h framerr.h frutil.h
frpacket$(OBJ):	 frpacket$(C) frconnec.h frconfig.h framerr.h
frpasswd$(OBJ):  frpasswd$(C) frpasswd.h frlist.h framerr.h frsymtab.h \
		frstring.h frnumber.h frprintf.h frutil.h
frpeer$(OBJ):	 frpeer$(C) frpeer.h
frprintf$(OBJ):	 frprintf$(C) frconfig.h frmem.h frprintf.h
frqueue$(OBJ):	 frqueue$(C) frqueue.h frpcglbl.h
frrandom$(OBJ):	 frrandom$(C) framerr.h frrandom.h
frreader$(OBJ):	 frreader$(C) frreader.h framerr.h frpcglbl.h \
		frsymbol.h frstring.h frlist.h frnumber.h frutil.h
frregexp$(OBJ):	 frregexp$(C) frregexp.h frassert.h framerr.h frctype.h \
		frstring.h frutil.h frcmove.h
frsarray$(OBJ):	 frsarray$(C) frarray.h frassert.h framerr.h frprintf.h frreader.h \
		frnumber.h frcmove.h frpcglbl.h
frsckstr$(OBJ):  frsckstr$(C) frsckstr.h frmswin.h frsignal.h
frserver$(OBJ):	 frserver$(C) frclisrv.h frconnec.h frserver.h frfinddb.h \
		frpcglbl.h frevent.h frhasht.h frstruct.h
frsignal$(OBJ):	 frsignal$(C) frsignal.h framerr.h
frslfreg$(OBJ):	 frslfreg$(C) frslfreg.h framerr.h
frslot0$(OBJ):	 frslot0$(C) frframe.h
frsntbrk$(OBJ):	 frsntbrk$(C) frctype.h frlist.h frstring.h frutil.h
frsocket$(OBJ):	frsocket$(C) frsckstr.h frprintf.h
frspell$(OBJ):	 frspell$(C) frfloat.h frstring.h frsymtab.h frutil.h
frstack$(OBJ):	 frstack$(C) frstack.h frpcglbl.h
frstring$(OBJ):  frstring$(C) frstring.h frpcglbl.h frctype.h frcmove.h \
		frbytord.h
frstrng1$(OBJ):  frstrng1$(C) frstring.h frbytord.h
frstrng2$(OBJ):  frstrng2$(C) frstring.h framerr.h frbytord.h frcmove.h
frstrng3$(OBJ):  frstrng3$(C) frstring.h
frstruct$(OBJ):  frstruct$(C) frstruct.h frpcglbl.h
frstrutl$(OBJ):	 frstrutl$(C) frstring.h frctype.h frmem.h
frstrut1$(OBJ):	 frstrut1$(C) frstring.h frunicod.h frctype.h frsymtab.h \
		frbytord.h framerr.h
frstrut2$(OBJ):	 frstrut2$(C) frstring.h framerr.h frbytord.h frunicod.h \
		frctype.h
frstrut3$(OBJ):	 frstrut3$(C) frstring.h frctype.h
frstrut4$(OBJ):	 frstrut4$(C) frstring.h frctype.h frsymtab.h frbytord.h \
		frunicod.h framerr.h
frstrut5$(OBJ):	 frstrut5$(C) frstring.h frctype.h
frstrut6$(OBJ):	 frstrut6$(C) frstring.h frsymtab.h frbytord.h frunicod.h \
		frutil.h framerr.h
frstrut7$(OBJ):	 frstrut7$(C) frstring.h
frsymbol$(OBJ):  frsymbol$(C) frsymbol.h frpcglbl.h frsymtab.h frutil.h
frsymfrm$(OBJ):  frsymfrm$(C) frsymbol.h frpcglbl.h frsymtab.h vframe.h
frsymfr2$(OBJ):  frsymfr2$(C) frsymbol.h frpcglbl.h
frhash$(OBJ):	 frhash$(C) frhash.h frreader.h frutil.h frpcglbl.h
frsymtab$(OBJ):  frsymtab$(C) frsymtab.h frlist.h frutil.h frassert.h \
		frpcglbl.h
frtfidf$(OBJ):	 frtfidf$(C) frtrmvec.h framerr.h frfilutl.h frhash.h \
		frsymtab.h frreader.h frutil.h
frthread$(OBJ):	 frthread$(C) frthread.h frassert.h fr_mem.h
frthresh$(OBJ):  frthresh$(C) frclust.h frfloat.h frsymtab.h
frtimer$(OBJ):	 frtimer$(C) frtimer.h framerr.h frmem.h frmswin.h
frtimer2$(OBJ):	 frtimer2$(C) frtimer.h
frtrmvec$(OBJ):	 frtrmvec$(C) frtrmvec.h framerr.h frassert.h frlist.h \
		frbpriq.h frnumber.h frhsort.h
frtxtfil$(OBJ):	 frtxtfil$(C) frfilutl.h framerr.h
frtxtspn$(OBJ):	 frtxtspn$(C) frtxtspn.h framerr.h frfloat.h frqsort.h \
		frstring.h frsymtab.h frutil.h
frunicod$(OBJ):	 frunicod$(C) frunicod.h frbytord.h
frunistr$(OBJ):	 frunistr$(C) frunicod.h frbytord.h frctype.h
fruniutl$(OBJ):	 fruniutl$(C) frunicod.h frbytord.h frlist.h frctype.h \
		frstring.h
frturl$(OBJ):	 frurl$(C) frurl.h framerr.h frctype.h frmem.h
frutil$(OBJ):	 frutil$(C) frutil.h framerr.h frmem.h frctype.h
frvars$(OBJ):	 frvars$(C) frpcglbl.h FramepaC.h
frvecsim$(OBJ):	 frvecsim$(C) frclust.h
frvocab$(OBJ):	 frvocab$(C) frvocab.h frassert.h frfilutl.h frobject.h \
		frutil.h
frwctype$(OBJ):	 frwctype$(C) frctype.h
mikro_db$(OBJ):  mikro_db$(C) frpcglbl.h mikro_db.h vfinfo.h frdathsh.h \
		frfilutl.h frfinddb.h inv.h frpasswd.h frnumber.h frprintf.h
vfinfo0$(OBJ):	 vfinfo0$(C) vfinfo.h frpcglbl.h
vfinfo$(OBJ):	 vfinfo$(C) vfinfo.h frhasht.h frutil.h
vframe$(OBJ):	 vframe$(C) frutil.h mikro_db.h vfinfo.h frfinddb.h frpcglbl.h
vframev$(OBJ):	 vframev$(C) frpcglbl.h mikro_db.h vfinfo.h frfinddb.h
parhash$(OBJ):	 parhash$(C) FramepaC.h
parmem$(OBJ):	 parmem$(C) FramepaC.h
rbtimer$(OBJ):	 rbtimer$(C) rbtimer.h

testnet$(OBJ):	 testnet$(C) testnet.h
benchmrk$(OBJ):	 benchmrk$(C) FramepaC.h
$(TESTPROG)$(OBJ): $(TESTPROG)$(C) FramepaC.h testnet.h benchmrk.h

## not available in LGPL version
#frclust3$(OBJ):	 frclust3$(C) frclust.h frclustp.h frassert.h frrandom.h \
#		frqsort.h frsignal.h frtimer.h
#frclust5$(OBJ):	 frclust5$(C) frclust.h frassert.h frclustp.h frtimer.h
#inv$(OBJ):	 inv$(C) inv.h frstring.h frfilutl.h

FramepaC.h:	frlist.h frstring.h frnumber.h framerr.h frsymtab.h \
		frframe.h vframe.h frmotif.h frlru.h frstruct.h frstack.h \
		frinline.h frarray.h frpasswd.h frctype.h frcfgfil.h \
		frclisrv.h frmswin.h frunicod.h frtimer.h frbytord.h \
		frfilutl.h frbitvec.h frexec.h frsignal.h frutil.h \
		frassert.h frregexp.h frhash.h frmd5.h frnetsrv.h \
		frprintf.h frslfreg.h frfeatvec.h frllist.h frhasht.h \
		frevent.h frclust.h frtxtspn.h frurl.h frfloat.h \
		frhsort.h frqsort.h frmath.h \
		frhelp.h frreader.h frsckstr.h frthread.h frmmap.h \
		frrandom.h frbpriq.h frbwt.h frvocab.h frcfgfil.h
	$(TOUCH) FramepaC.h $(BITBUCKET)

benchmrk.h:	FramepaC.h
	$(TOUCH) benchmrk.h $(BITBUCKET)

frameid.h:	frmem.h
	$(TOUCH) frameid.h $(BITBUCKET)

frarray.h:	frobject.h frmem.h
	$(TOUCH) frarray.h $(BITBUCKET)

frbitvec.h:	frobject.h
	$(TOUCH) frbitvec.h $(BITBUCKET)

frbpriq.h:	frobject.h
	$(TOUCH) frbpriq.h $(BITBUCKET)

frcfgfil.h:	frcommon.h
	$(TOUCH) frcfgfil.h $(BITBUCKET)

frclisrv.h:	frobject.h vframe.h
	$(TOUCH) frclisrv.h $(BITBUCKET)

frclust.h:	frtrmvec.h framerr.h frlist.h
	$(TOUCH) frclust.h $(BITBUCKET)

frcommon.h:	frconfig.h
	$(TOUCH) frcommon.h $(BITBUCKET)

frconnec.h:	frqueue.h frbytord.h frclisrv.h
	$(TOUCH) frconnec.h $(BITBUCKET)

frcritsec.h:	frconfig.h
	$(TOUCH) frcritsec.h $(BITBUCKET)

frctype.h:	frcommon.h
	$(TOUCH) frctype.h $(BITBUCKET)

frdatfil.h:	vfinfo.h frameid.h
	$(TOUCH) frdatfil.h $(BITBUCKET)

frdathsh.h:	frhasht.h
	$(TOUCH) frdathsh.h $(BITBUCKET)

frexec.h:	frcommon.h
	$(TOUCH) frexec.h $(BITBUCKET)

frfeatvec.h:	frbitvec.h frlist.h frstring.h frsymbol.h frutil.h
	$(TOUCH) frfeatvec.h $(BITBUCKET)

frfilutl.h:	frcommon.h frctype.h frlist.h
	$(TOUCH) frfilutl.h $(BITBUCKET)

frfinddb.h:	frcommon.h
	$(TOUCH) frfinddb.h $(BITBUCKET)

frfloat.h:	frnumber.h
	$(TOUCH) frfloat.h $(BITBUCKET)

frframe.h:	frobject.h frsymbol.h frreader.h
	$(TOUCH) frframe.h $(BITBUCKET)

frhasht.h: 	framerr.h frobject.h frmem.h frreader.h
	$(TOUCH) frhasht.h $(BITBUCKET)

frhelp.h:	frmotif.h
	$(TOUCH) frhelp.h $(BITBUCKET)

frlist.h:	frobject.h
	$(TOUCH) frlist.h $(BITBUCKET)

frmd5.h:	frconfig.h
	$(TOUCH) frmd5.h $(BITBUCKET)

frmem.h:	frcommon.h frassert.h frthread.h framerr.h
	$(TOUCH) frmem.h $(BITBUCKET)

fr_mem.h:	frmem.h
	$(TOUCH) fr_mem.h $(BITBUCKET)

frmmap.h:	frcommon.h
	$(TOUCH) frmmap.h $(BITBUCKET)

frmorphp.h:	FramepaC.h
	$(TOUCH) frmorphp.h $(BITBUCKET)

frmotif.h:	frlist.h
	$(TOUCH) frmotif.h $(BITBUCKET)

frnetsrv.h:	frsckstr.h
	$(TOUCH) frnetsrv.h $(BITBUCKET)

frnetwrk.h:	vframe.h vfinfo.h frameid.h frhasht.h frnethsh.h
	$(TOUCH) frnetwrk.h $(BITBUCKET)

frnumber.h:	frobject.h
	$(TOUCH) frnumber.h $(BITBUCKET)

frobject.h:	frcommon.h frctype.h frmem.h
	$(TOUCH) frobject.h $(BITBUCKET)

frpasswd.h:	frstruct.h frlist.h
	$(TOUCH) frpasswd.h $(BITBUCKET)

frpcglbl.h:	framerr.h vframe.h vfinfo.h frctype.h
	$(TOUCH) frpcglbl.h $(BITBUCKET)

frpeer.h:	frnetwrk.h frdatfil.h
	$(TOUCH) frpeer.h $(BITBUCKET)

frqueue.h:	frlist.h
	$(TOUCH) frqueue.h $(BITBUCKET)

frrandom.h:	frlist.h
	$(TOUCH) frrandom.h $(BITBUCKET)

frreader.h:	frmem.h
	$(TOUCH) frreader.h $(BITBUCKET)

frregexp.h:	frsymbol.h
	$(TOUCH) frregexp.h $(BITBUCKET)

frslfreg.h:	frsymtab.h frutil.h
	$(TOUCH) frslfreg.h $(BITBUCKET)

frspell.h:	frhash.h
	$(TOUCH) frspell.h $(BITBUCKET)

frstack.h:	frqueue.h
	$(TOUCH) frstack.h $(BITBUCKET)

frstring.h:	frobject.h
	$(TOUCH) frstring.h $(BITBUCKET)

frstruct.h:	frobject.h
	$(TOUCH) frstruct.h $(BITBUCKET)

frsymbol.h:	frobject.h frlist.h
	$(TOUCH) frsymbol.h $(BITBUCKET)

frhash.h:	frsymbol.h frmem.h frthread.h
	$(TOUCH) frhash.h $(BITBUCKET)

frsymtab.h:	frsymbol.h
	$(TOUCH) frsymtab.h $(BITBUCKET)

frthread.h:	frcommon.h frcritsec.h
	$(TOUCH) frthread.h $(BITBUCKET)

frtimer.h:	frcommon.h
	$(TOUCH) frtimer.h $(BITBUCKET)

frtrmvec.h:	frsymbol.h frhash.h frarray.h
	$(TOUCH) frtrmvec.h $(BITBUCKET)

frtxtspn.h:	frstruct.h
	$(TOUCH) frtxtspn.h $(BITBUCKET)

frunicod.h:	frcommon.h
	$(TOUCH) frunicod.h $(BITBUCKET)

frurl.h:	frcommon.h frlist.h frsckstr.h
	$(TOUCH) frurl.h $(BITBUCKET)

frutil.h:	frhash.h
	$(TOUCH) frutil.h $(BITBUCKET)

frvocab.h:	frcommon.h frbytord.h frmmap.h
	$(TOUCH) frvocab.h $(BITBUCKET)

inv.h:		mikro_db.h
	$(TOUCH) inv.h $(BITBUCKET)

mikro_db.h:	framerr.h frameid.h frhasht.h
	$(TOUCH) mikro_db.h $(BITBUCKET)

testnet.h:	FramepaC.h
	$(TOUCH) testnet.h $(BITBUCKET)

vfinfo.h:	frsymtab.h vframe.h frhash.h
	$(TOUCH) vfinfo.h $(BITBUCKET)

vframe.h:	frframe.h frsymtab.h
	$(TOUCH) vframe.h $(BITBUCKET)

# End of Makefile
