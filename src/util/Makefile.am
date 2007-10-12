# Compile experimental features ?
if WITH_EXPERIMENTAL
  OPT_EXPERIMENTAL = -D__EXPERIMENTAL__
else
  OPT_EXPERIMENTAL =
endif

AM_CFLAGS = $(OPT_EXPERIMENTAL) $(OPT_DEBUGGING)

noinst_LTLIBRARIES = libutil.la
libutil_la_CFLAGS = $(LIBGDS_CFLAGS) $(PCRE_CFLAGS)

libutil_la_SOURCES = \
	regex.c \
	regex.h


