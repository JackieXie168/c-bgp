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

SUBDIRS = bgp cli net sim ui $(OPT_JNI_DIR) $(OPT_LIBALLOC_DIR)

AM_CFLAGS = -Wall -Werror -O2

lib_LTLIBRARIES = libcsim.la
libcsim_la_SOURCES =
libcsim_la_LIBADD = -lgds bgp/libbgp.la cli/libcli.la sim/libsim.la net/libnet.la ui/libui.la $(OPT_JNI_LIBADD)
libcsim_la_LDFLAGS = -no-undefined

bin_PROGRAMS = cbgp
cbgp_SOURCES = \
	main.c
cbgp_LDADD = libcsim.la $(OPT_LIBALLOC_LDADD)
