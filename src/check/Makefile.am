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

INCLUDES = -I..

check_PROGRAMS = \
	libcbgp_test
TESTS = $(check_PROGRAMS)

libcbgp_test_SOURCES = main.c
libcbgp_test_CFLAGS = -D_LIBCBGP_ $(AM_CFLAGS)
libcbgp_test_LDADD = ../libcsim.la


