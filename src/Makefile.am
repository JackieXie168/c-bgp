#SUBDIRS = bgp cli net sim ui liballoc
SUBDIRS = bgp cli net sim ui jni

AM_CFLAGS = -Wall -Werror -O2

lib_LTLIBRARIES = libcsim.la
libcsim_la_SOURCES =
libcsim_la_LIBADD = -lgds bgp/libbgp.la cli/libcli.la sim/libsim.la net/libnet.la ui/libui.la jni/libjni.la
libcsim_la_LDFLAGS = -no-undefined

bin_PROGRAMS = cbgp
cbgp_SOURCES = \
	main.c
#cbgp_LDADD = libcsim.la liballoc/liballoc.la
cbgp_LDADD = libcsim.la
