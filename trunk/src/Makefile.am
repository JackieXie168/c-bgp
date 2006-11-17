# Compile with JNI ?
if WITH_JNI
  OPT_JNI_DIR = jni
  OPT_JNI_LIBADD = jni/libjni.la
else
  OPT_JNI_DIR =
  OPT_JNI_LIBADD =
endif

# Compile with liballoc ?
if WITH_LIBALLOC
  OPT_LIBALLOC_DIR = liballoc
  OPT_LIBALLOC_LDADD = liballoc/liballoc.la
else
  OPT_LIBALLOC_DIR =
  OPT_LIBALLOC_LDADD =
endif

# Compile with ospf support?
if WITH_OSPF
  OPT_WITH_OSPF = -DOSPF_SUPPORT
else
  OPT_WITH_OSPF =
endif

# Compile with Electric Fence ?
if WITH_EFENCE
  OPT_EFENCE = -lefence
else
  OPT_EFENCE =
endif

# Compile with libgpdump ?
if WITH_BGPDUMP
  OPT_BGPDUMP_LIBADD = external/libexternal.la
  OPT_BGPDUMP_DIR = external
else
  OPT_BGPDUMP_LIBADD =
  OPT_BGPDUMP_DIR = 
endif

SUBDIRS = bgp cli net sim ui util $(OPT_JNI_DIR) $(OPT_LIBALLOC_DIR) $(OPT_BGPDUMP_DIR) check
DIST_SUBDIRS = bgp cli external net sim ui util jni liballoc check 

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

if WITH_MEMORY_DEBUG
  OPT_MEMORY_DEBUG = -D__MEMORY_DEBUG__
else
  OPT_MEMORY_DEBUG =
endif

AM_CFLAGS = -Wall -Werror -O2 $(OPT_EXPERIMENTAL) $(OPT_DEBUGGING) $(OPT_WITH_OSPF) $(OPT_MEMORY_DEBUG)

lib_LTLIBRARIES = libcsim.la
libcsim_la_SOURCES = api.c
libcsim_la_LIBADD = -lgds bgp/libbgp.la cli/libcli.la sim/libsim.la net/libnet.la ui/libui.la util/libutil.la $(OPT_JNI_LIBADD) $(OPT_EFENCE) $(OPT_BGPDUMP_LIBADD)
libcsim_la_LDFLAGS = -no-undefined $(pcre_libs) $(readline_libs)
libcsiminclude_HEADERS = api.h
libcsimincludedir = $(includedir)/cbgp

bin_PROGRAMS = cbgp
cbgp_SOURCES = \
	main.c
cbgp_LDADD = libcsim.la $(OPT_LIBALLOC_LDADD) $(pcre_libs) $(readline_libs)

# -------------------------------------------------------------------
# Install hook (called after all install targets/hooks)
# 1). Copy the CBPG.jar Java archive to the JNI install dir.
# 2). Under Mac OS X (Darwin), create a symlink from libcsim.jnilib to
# libcsim.dylib in $libdir . This is required for correct JNI
# operation.
# -------------------------------------------------------------------
if WITH_JNI
install-exec-local: 
	$(INSTALL) jni/CBGP.jar $(libdir)/CBGP.jar
	(case @host_os@ in \
	 darwin*) \
	 rm -f $(libdir)/libcsim.jnilib; \
	 cd $(libdir); \
	 @LN_S@ $(libdir)/libcsim.dylib libcsim.jnilib; \
	 cd -; \
	 ;; \
	 esac)
endif
