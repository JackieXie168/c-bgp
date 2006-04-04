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

SUBDIRS = bgp cli external net sim ui util $(OPT_JNI_DIR) $(OPT_LIBALLOC_DIR) check
DIST_SUBDIRS = $(SUBDIRS)

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

AM_CFLAGS = -Wall -Werror -O2 $(OPT_EXPERIMENTAL) $(OPT_DEBUGGING) $(OPT_WITH_OSPF)

lib_LTLIBRARIES = libcsim.la
libcsim_la_SOURCES =
libcsim_la_LIBADD = -lgds bgp/libbgp.la cli/libcli.la external/libexternal.la sim/libsim.la net/libnet.la ui/libui.la util/libutil.la $(OPT_JNI_LIBADD) $(OPT_EFENCE)
libcsim_la_LDFLAGS = -no-undefined $(pcre_libs) $(readline_libs)

bin_PROGRAMS = cbgp
cbgp_SOURCES = \
	main.c
cbgp_LDADD = libcsim.la $(OPT_LIBALLOC_LDADD) $(pcre_libs) $(readline_libs)

#dist-hook:
#	find $(distdir) -type f -name *.c -exec cat \{\} | sed 's/\@LICENSE\@/THIS IS THE LICENSE/' \;
