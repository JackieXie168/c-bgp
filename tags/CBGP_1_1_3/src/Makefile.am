SUBDIRS = bgp cli net sim ui liballoc

AM_CFLAGS = -Wall -Werror -O2

lib_LTLIBRARIES = libcsim.la
libcsim_la_SOURCES =
libcsim_la_LIBADD = -lgds bgp/libbgp.la cli/libcli.la sim/libsim.la net/libnet.la ui/libui.la
libcsim_la_LDFLAGS = -no-undefinedd

bin_PROGRAMS = cbgp
cbgp_SOURCES = \
	main.c
cbgp_LDADD = libcsim.la liballoc/liballoc.la
