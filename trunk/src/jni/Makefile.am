noinst_LTLIBRARIES = libjni.la

INCLUDES = -I.. 


libjni_la_SOURCES = \
	jni_interface.c \
	jni_util.c \
	jni_util.h \
	be_ac_ucl_ingi_cbgp_CBGP.h

AM_CFLAGS = -Wall -Werror -O2

JARFILE = CBGP.jar
BUILT_SOURCES = $(JARFILE)
#foo_DATA = $(JARFILE)

JAVAFILES= \
	be/ac/ucl/ingi/cbgp/ASPath.java \
	be/ac/ucl/ingi/cbgp/ASPathSegment.java \
	be/ac/ucl/ingi/cbgp/CBGP.java \
	be/ac/ucl/ingi/cbgp/CBGPException.java \
	be/ac/ucl/ingi/cbgp/BGPPeer.java \
	be/ac/ucl/ingi/cbgp/BGPRoute.java \
	be/ac/ucl/ingi/cbgp/IPAddress.java \
	be/ac/ucl/ingi/cbgp/IPPrefix.java \
	be/ac/ucl/ingi/cbgp/IPRoute.java \
	be/ac/ucl/ingi/cbgp/IPTrace.java \
	be/ac/ucl/ingi/cbgp/Link.java \
	be/ac/ucl/ingi/cbgp/Route.java \
	be/ac/ucl/ingi/cbgp/RouteInterface.java

METAFILE= META-INF/MANIFEST.MF
	
CLASSFILES = $(JAVAFILES:.java=.class)
EXTRA_DIST = $(JAVAFILES) $(METAFILE)

%.class: %.java
	$(JAVAC) $<

SUFFIXES= .java .class

$(JARFILE): $(CLASSFILES)
	(jar cvfm $@ $(METAFILE) $(CLASSFILES))

install-exec-local:
	$(INSTALL) CBGP.jar $(libdir)/CBGP.jar

CLEANFILES = $(JARFILE) $(CLASSFILES)

