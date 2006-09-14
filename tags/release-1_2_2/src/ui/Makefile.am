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

noinst_LTLIBRARIES = libui.la

libui_la_SOURCES = \
	output.c \
	output.h \
	rl.c \
	rl.h


