# Compile experimental features ?
if WITH_EXPERIMENTAL
  OPT_EXPERIMENTAL = -D__EXPERIMENTAL__
else
  OPT_EXPERIMENTAL =
endif

if WITH_OSPF
  OPT_WITH_OSPF = -DOSPF_SUPPORT
else
  OPT_WITH_OSPF =
endif

if WITH_EXTERNAL_BEST
  OPT_EXTERNAL_BEST = -D__EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
else
  OPT_EXTERNAL_BEST = 
endif


# Compile with debugging symbols ?
if WITH_DEBUGGING
  OPT_DEBUGGING = -g
else
  OPT_DEBUGGING =
endif

AM_CFLAGS = -Wall -Werror -O2 $(OPT_EXPERIMENTAL) $(OPT_EXTERNAL_BEST) $(OPT_WITH_OSPF) $(OPT_DEBUGGING) 

noinst_LTLIBRARIES = libcli.la

INCLUDES = -I..

libcli_la_SOURCES = \
	bgp.c \
	bgp.h \
	common.c \
	common.h \
	net.c \
	net.h \
	net_domain.c \
	net_domain.h \
	net_ospf.c \
	net_ospf.h \
	sim.c \
	sim.h
