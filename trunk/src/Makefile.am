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

# Compile experimental features ?
if WITH_EXPERIMENTAL
  OPT_EXPERIMENTAL = -D__EXPERIMENTAL__
else
  OPT_EXPERIMENTAL =
endif

# Compile with Electric Fence ?
if WITH_EFENCE
  OPT_EFENCE = -lefence
else
  OPT_EFENCE =
endif

SUBDIRS = bgp cli net sim ui util $(OPT_JNI_DIR) $(OPT_LIBALLOC_DIR) check

AM_CFLAGS = -Wall -Werror -O2 $(OPT_EXPERIMENTAL)

lib_LTLIBRARIES = libcsim.la
libcsim_la_SOURCES =
libcsim_la_LIBADD = -lgds -lpcre bgp/libbgp.la cli/libcli.la sim/libsim.la net/libnet.la ui/libui.la util/libutil.la $(OPT_JNI_LIBADD) $(OPT_EFENCE)
libcsim_la_LDFLAGS = -no-undefined

bin_PROGRAMS = cbgp
cbgp_SOURCES = \
	main.c
cbgp_LDADD = libcsim.la $(OPT_LIBALLOC_LDADD)
