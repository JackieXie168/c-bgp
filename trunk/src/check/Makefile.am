INCLUDES = -I..
AM_CFLAGS = -Wall -Werror -O2

check_PROGRAMS = \
	libcbgp_test
TESTS = $(check_PROGRAMS)

libcbgp_test_SOURCES = main.c
libcbgp_test_CFLAGS = -D_LIBCBGP_
libcbgp_test_LDADD = ../libcsim.la


