noinst_LTLIBRARIES = libjni.la

INCLUDES = -I.. 


libjni_la_SOURCES = \
	jni_interface.c \
	be_ac_ucl_ingi_cbgp_CBGP.h
AM_CFLAGS = -Wall -Werror -O2

JARFILE = CBGP.jar
BUILT_SOURCES = $(JARFILE)
#foo_DATA = $(JARFILE)

JAVAFILES= \
	be/ac/ucl/ingi/cbgp/CBGP.java

METAFILE= META-INF/MANIFEST.MF
	
CLASSFILES = $(JAVAFILES:.java=.class)
EXTRA_DIST = $(JAVAFILES) $(METAFILE)

%.class: %.java
	$(JAVAC) $<

SUFFIXES= .java .class

$(JARFILE): $(CLASSFILES)
	(cd be/ac/ucl/ingi/cbgp; for x in `find ./ -name \*.class`; do $(JAVAH) `echo $$x | sed "s/\.\///g" | sed "s/.class//g"`; done;)
	(jar cvfm $@ $(METAFILE) $(CLASSFILES))

install-exec-local:
	$(INSTALL) CBGP.jar $(libdir)/CBGP.jar

CLEANFILES = $(JARFILE) $(CLASSFILES)

