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

noinst_LTLIBRARIES = libexternal.la

libexternal_la_SOURCES = \
	bgpdump-config.h \
	bgpdump.h \
	bgpdump_attr.h \
	bgpdump_formats.h \
	bgpdump_lib.h \
	bgpdump_mstream.h \
	bgpdump_lib.c \
	bgpdump_mstream.c
