# Compile experimental features ?
if WITH_EXPERIMENTAL
  OPT_EXPERIMENTAL = -D__EXPERIMENTAL__
else
  OPT_EXPERIMENTAL =
endif

AM_CFLAGS = $(OPT_EXPERIMENTAL)

noinst_LTLIBRARIES = libtracer.la
libtracer_la_CFLAGS = $(LIBGDS_CFLAGS)

INCLUDES = -I..

libtracer_la_SOURCES = \
	state.c \
	state.h \
	routing_state.c \
	routing_state.h \
	queue_state.c \
	queue_state.h \
	graph.c \
	graph.h \
	tracer.c \
	tracer.h \
	transition.c \
	transition.h
	
