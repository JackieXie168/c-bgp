# Compile experimental features ?
if WITH_EXPERIMENTAL
  OPT_EXPERIMENTAL = -D__EXPERIMENTAL__
else
  OPT_EXPERIMENTAL =
endif

AM_CFLAGS = $(OPT_EXPERIMENTAL)

noinst_LTLIBRARIES = libsim.la
libsim_la_CFLAGS = $(LIBGDS_CFLAGS)

INCLUDES = -I..

libsim_la_SOURCES = \
	state.c \
	state.h \
	graph.c \
	graph.h \
	tracer.c \
	tracer.h
	
