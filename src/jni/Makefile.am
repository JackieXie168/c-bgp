noinst_LTLIBRARIES = libjni.la

INCLUDES = -I.. 


libjni_la_SOURCES = \
	jni_interface.c \
	cbgpJNI.h
AM_CFLAGS = -Wall -Werror -O2

JARFILE = CBGP.jar
BUILT_SOURCES = $(JARFILE)
#foo_DATA = $(JARFILE)

JAVAFILES= \
	be/ac/ucl/ingi/cbgp/CBGP.java \
	be/ac/ucl/ingi/cbgp/cbgpJNI.java
	
CLASSFILES = $(JAVAFILES:.java=.class)

%.class: %.java
	$(JAVAC) $<

$(JARFILE): $(CLASSFILES)
	(cd be/ac/ucl/ingi/cbgp; for x in `find ./ -name \*.class`; do $(JAVAH) `echo $$x | sed "s/\.\///g" | sed "s/.class//g"`; done;)
	(jar cf -M -m META-INF/MANIFEST.MF -f $@ $(CLASSFILES))

install-exec-local:
	$(INSTALL) CBGP.jar $(libdir)/CBGP.jar

CLEANFILES = $(JARFILE) $(CLASSFILES)

