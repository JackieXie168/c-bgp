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

noinst_LTLIBRARIES = libnet.la

INCLUDES = -I..

libnet_la_SOURCES = \
	dijkstra.c \
	dijkstra.h \
	domain.c \
	domain.h \
	domain_t.h \
	icmp.c \
	icmp.h \
	igp.c \
	igp.h \
	ipip.c \
	ipip.h \
	link.c \
	link.h \
	message.c \
	message.h \
	net_path.c \
	net_path.h \
	network.c \
	network.h \
	ntf.c \
	ntf.h \
	prefix.c \
	prefix.h \
	protocol.c \
	protocol.h \
	routing.c \
	routing.h 
