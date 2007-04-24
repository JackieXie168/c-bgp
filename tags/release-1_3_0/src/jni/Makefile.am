# Compile experimental features ?
if WITH_EXPERIMENTAL
  OPT_EXPERIMENTAL = -D__EXPERIMENTAL__
else
  OPT_EXPERIMENTAL =
endif

# Compile with debugging symbols ?
if WITH_DEBUGGING
  OPT_DEBUGGING = -g
else
  OPT_DEBUGGING =
endif

AM_CFLAGS = -Wall -Werror -O2 $(OPT_EXPERIMENTAL) $(OPT_DEBUGGING)

noinst_LTLIBRARIES = libjni.la

INCLUDES = -I.. 

SUBDIRS = impl
DIST_SUBDIRS = $(SUBDIRS)

libjni_la_SOURCES = \
	jni_base.c \
	jni_base.h \
	jni_interface.c \
	jni_proxies.c \
	jni_proxies.h \
	jni_util.c \
	jni_util.h
libjni_la_LIBADD = impl/libjni_impl.la

JARFILE = CBGP.jar
BUILT_SOURCES = $(JARFILE)

JAVAFILES= \
	be/ac/ucl/ingi/cbgp/bgp/ASPath.java \
	be/ac/ucl/ingi/cbgp/bgp/ASPathSegment.java \
	be/ac/ucl/ingi/cbgp/CBGP.java \
	be/ac/ucl/ingi/cbgp/CBGPException.java \
	be/ac/ucl/ingi/cbgp/bgp/Communities.java \
	be/ac/ucl/ingi/cbgp/ConsoleEvent.java \
	be/ac/ucl/ingi/cbgp/ConsoleEventListener.java \
	be/ac/ucl/ingi/cbgp/AbstractConsoleEventListener.java \
	be/ac/ucl/ingi/cbgp/bgp/Domain.java \
	be/ac/ucl/ingi/cbgp/bgp/Route.java \
	be/ac/ucl/ingi/cbgp/net/IGPDomain.java \
	be/ac/ucl/ingi/cbgp/IPAddress.java \
	be/ac/ucl/ingi/cbgp/IPPrefix.java \
	be/ac/ucl/ingi/cbgp/IPRoute.java \
	be/ac/ucl/ingi/cbgp/IPTrace.java \
	be/ac/ucl/ingi/cbgp/bgp/Filter.java \
	be/ac/ucl/ingi/cbgp/bgp/FilterRule.java \
	be/ac/ucl/ingi/cbgp/bgp/Peer.java \
	be/ac/ucl/ingi/cbgp/bgp/Router.java \
	be/ac/ucl/ingi/cbgp/net/Link.java \
	be/ac/ucl/ingi/cbgp/net/Node.java \
	be/ac/ucl/ingi/cbgp/ProxyObject.java \
	be/ac/ucl/ingi/cbgp/Route.java \
	be/ac/ucl/ingi/cbgp/RouteInterface.java

METAFILE= META-INF/MANIFEST.MF

CLASSFILES = $(JAVAFILES:.java=.class)
EXTRA_DIST = $(JAVAFILES) $(METAFILE)

# Invoke javac in order to build Java classes. In addition, invoke
# javah in order to build the native function headers.
%.class: %.java
	$(JAVAC) $<
	(CLASSNAME=`echo $@ | sed -e 's/\//./g' -e 's/.class//g'`; \
	  $(JAVAH) -d headers $$CLASSNAME)

SUFFIXES= .java .class

# Build the jar file
$(JARFILE): $(CLASSFILES)
	(jar cvfm $@ $(METAFILE) $(CLASSFILES))

#	(CLASSNAMES=`echo $(CLASSFILES) | \
#         sed -e 's/\//./g' -e 's/.class//g'`; \
#	 $(JAVAH) -d headers $$CLASSNAMES)

CLEANFILES = $(JARFILE) $(CLASSFILES)

