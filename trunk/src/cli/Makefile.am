noinst_LTLIBRARIES = libcli.la

INCLUDES = -I..

libcli_la_SOURCES = \
	bgp.c \
	bgp.h \
	common.c \
	common.h \
	net.c \
	net.h \
	sim.c \
	sim.h

# Compile experimental features ?
if WITH_EXPERIMENTAL
  OPT_EXPERIMENTAL = -D__EXPERIMENTAL__
else
  OPT_EXPERIMENTAL =
endif

AM_CFLAGS = -Wall -Werror -O2 $(OPT_EXPERIMENTAL)
