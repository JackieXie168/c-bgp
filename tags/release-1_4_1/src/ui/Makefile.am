# Compile experimental features ?
if WITH_EXPERIMENTAL
  OPT_EXPERIMENTAL = -D__EXPERIMENTAL__
else
  OPT_EXPERIMENTAL =
endif

AM_CFLAGS = $(OPT_EXPERIMENTAL)

noinst_LTLIBRARIES = libui.la
libui_la_CFLAGS = $(LIBGDS_CFLAGS) $(READLINE_CFLAGS)

libui_la_SOURCES = \
	completion.c \
	completion.h \
	help.c \
	help.h \
	output.c \
	output.h \
	rl.c \
	rl.h


