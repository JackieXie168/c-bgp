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

noinst_LTLIBRARIES = libsim.la

INCLUDES = -I..

libsim_la_SOURCES = \
	scheduler.c \
	scheduler.h \
	simulator.c \
	simulator.h \
	static_scheduler.c \
	static_scheduler.h
