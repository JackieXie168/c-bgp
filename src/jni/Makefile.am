# Compile experimental features ?
if WITH_EXPERIMENTAL
  OPT_EXPERIMENTAL = -D__EXPERIMENTAL__
else
  OPT_EXPERIMENTAL =
endif

AM_CFLAGS = $(OPT_EXPERIMENTAL) $(OPT_DEBUGGING)

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
	jni_util.h \
	exceptions.c \
	exceptions.h \
	listener.c \
	listener.h
libjni_la_LIBADD = impl/libjni_impl.la
libjni_la_CFLAGS = $(LIBGDS_CFLAGS) -Wall -Werror -O2 -g

JARFILE = CBGP.jar
BUILT_SOURCES = $(JARFILE)

JAVA_PACKAGES = \
	be.ac.ucl.ingi.cbgp \
	be.ac.ucl.ingi.cbgp.bgp \
	be.ac.ucl.ingi.cbgp.exceptions \
	be.ac.ucl.ingi.cbgp.net

JAVAFILES= \
	be/ac/ucl/ingi/cbgp/BGPMsgListener.java \
	be/ac/ucl/ingi/cbgp/CBGP.java \
	be/ac/ucl/ingi/cbgp/exceptions/CBGPException.java \
	be/ac/ucl/ingi/cbgp/exceptions/CBGPScriptException.java \
	be/ac/ucl/ingi/cbgp/exceptions/InvalidDestinationException.java \
	be/ac/ucl/ingi/cbgp/ConsoleEvent.java \
	be/ac/ucl/ingi/cbgp/ConsoleEventListener.java \
	be/ac/ucl/ingi/cbgp/AbstractConsoleEventListener.java \
	be/ac/ucl/ingi/cbgp/IPAddress.java \
	be/ac/ucl/ingi/cbgp/IPPrefix.java \
	be/ac/ucl/ingi/cbgp/IPRoute.java \
	be/ac/ucl/ingi/cbgp/IPTrace.java \
	be/ac/ucl/ingi/cbgp/IPTraceElement.java \
	be/ac/ucl/ingi/cbgp/IPTraceNode.java \
	be/ac/ucl/ingi/cbgp/IPTraceSubnet.java \
	be/ac/ucl/ingi/cbgp/LinkMetrics.java \
	be/ac/ucl/ingi/cbgp/exceptions/UnknownMetricException.java \
	be/ac/ucl/ingi/cbgp/ProxyObject.java \
	be/ac/ucl/ingi/cbgp/Route.java \
	be/ac/ucl/ingi/cbgp/RouteInterface.java \
	be/ac/ucl/ingi/cbgp/bgp/ASPath.java \
	be/ac/ucl/ingi/cbgp/bgp/ASPathSegment.java \
	be/ac/ucl/ingi/cbgp/bgp/Communities.java \
	be/ac/ucl/ingi/cbgp/bgp/Domain.java \
	be/ac/ucl/ingi/cbgp/bgp/Route.java \
	be/ac/ucl/ingi/cbgp/bgp/Filter.java \
	be/ac/ucl/ingi/cbgp/bgp/FilterRule.java \
	be/ac/ucl/ingi/cbgp/bgp/MessageUpdate.java \
	be/ac/ucl/ingi/cbgp/bgp/MessageWithdraw.java \
	be/ac/ucl/ingi/cbgp/bgp/Peer.java \
	be/ac/ucl/ingi/cbgp/bgp/Router.java \
	be/ac/ucl/ingi/cbgp/net/Element.java \
	be/ac/ucl/ingi/cbgp/net/IGPDomain.java \
	be/ac/ucl/ingi/cbgp/net/Interface.java \
	be/ac/ucl/ingi/cbgp/net/Message.java \
	be/ac/ucl/ingi/cbgp/net/Node.java \
	be/ac/ucl/ingi/cbgp/net/Subnet.java

JAVATESTFILES= \
	be/ac/ucl/ingi/cbgp/testing/TestBGPDomain.java \
	be/ac/ucl/ingi/cbgp/testing/TestBGPRouter.java \
	be/ac/ucl/ingi/cbgp/testing/TestIGPDomain.java \
	be/ac/ucl/ingi/cbgp/testing/TestLink.java \
	be/ac/ucl/ingi/cbgp/testing/TestMisc.java \
	be/ac/ucl/ingi/cbgp/testing/TestNode.java \
	be/ac/ucl/ingi/cbgp/testing/TestSession.java \
	be/ac/ucl/ingi/cbgp/testing/TestSubnet.java \
	be/ac/ucl/ingi/cbgp/testing/AllTests.java

# Java classes that need to be processed by javah to produce a
# C header file must be listed below
JNICLASSFILES= \
	be.ac.ucl.ingi.cbgp.CBGP \
	be.ac.ucl.ingi.cbgp.IPTrace \
	be.ac.ucl.ingi.cbgp.ProxyObject \
	be.ac.ucl.ingi.cbgp.bgp.Domain \
	be.ac.ucl.ingi.cbgp.bgp.Filter \
	be.ac.ucl.ingi.cbgp.bgp.FilterRule \
	be.ac.ucl.ingi.cbgp.bgp.Peer \
	be.ac.ucl.ingi.cbgp.bgp.Router \
	be.ac.ucl.ingi.cbgp.net.IGPDomain \
	be.ac.ucl.ingi.cbgp.net.Interface \
	be.ac.ucl.ingi.cbgp.net.Node \
	be.ac.ucl.ingi.cbgp.net.Subnet

METAFILE= META-INF/MANIFEST.MF

CLASSFILES = $(JAVAFILES:.java=.class) $(JAVATESTFILES:.java=.class)
EXTRA_DIST = \
	$(JAVAFILES) \
	$(JAVATESTFILES) \
	$(METAFILE) \
	jni-test-wrapper.sh

CLASSPATH = .:$(top_builddir)/lib/junit-4.4.jar

# Invoke javac in order to build Java classes. In addition, invoke
# javah in order to build the native function headers.
.java.class:
	$(JAVAC) -classpath $(CLASSPATH) $<

# Old way to build JNI headers was through the generic .class -> .java
# rule. This was not working since 'javac' sometimes not only builds
# the specified target but also dependencies.
#(CLASSNAME=`echo $@ | sed -e 's/\//./g' -e 's/.class//g'`; \
#  $(JAVAH) -d headers $$CLASSNAME)

SUFFIXES= .java .class

# Build the jar file
$(JARFILE): $(CLASSFILES)
	@(for f in $(JNICLASSFILES); do \
	 echo "$(JAVAH) -d headers $$f"; \
	 $(JAVAH) -classpath $(CLASSPATH) -d headers $$f; \
	done)
	(jar cvfm $@ $(METAFILE) $(CLASSFILES))

CLEANFILES = $(JARFILE) $(CLASSFILES)


# -------------------------------------------------------------------
# Java tests
# -------------------------------------------------------------------

#TESTS = jni-test-wrapper.sh


# -------------------------------------------------------------------
# Java documentation
# -------------------------------------------------------------------

doc:
	$(JAVADOC) -d javadoc -classpath $(CLASSPATH) $(JAVA_PACKAGES)

