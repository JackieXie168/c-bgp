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

if WITH_OSPF
  OPT_WITH_OSPF = -DOSPF_SUPPORT
else
  OPT_WITH_OSPF =
endif

AM_CFLAGS = -Wall -Werror -O2 $(OPT_EXPERIMENTAL) $(OPT_DEBUGGING) $(OPT_WITH_OSPF)

noinst_LTLIBRARIES = libnet.la

INCLUDES = -I..

libnet_la_SOURCES = \
	error.c \
	error.h \
	icmp.c \
	icmp.h \
	iface.c \
	iface.h \
	igp.c \
	igp.h \
	igp_domain.c \
	igp_domain.h \
	ipip.c \
	ipip.h \
	link.c \
	link.h \
	link_attr.c \
	link_attr.h \
	link-list.c \
	link-list.h \
	message.c \
	message.h \
	net_path.c \
	net_path.h \
	net_types.h \
	netflow.c \
	netflow.h \
	network.c \
	network.h \
	node.c \
	node.h \
	ntf.c \
	ntf.h \
	ospf.c \
	ospf.h \
	ospf_deflection.c \
	ospf_deflection.h \
	ospf_rt.c \
	ospf_rt.h \
	prefix.c \
	prefix.h \
	protocol.c \
	protocol.h \
	record-route.c \
	record-route.h \
	routing.c \
	routing.h \
	routing_t.h \
	spt_vertex.c \
	spt_vertex.h \
	subnet.c \
	subnet.h \
	tm.c \
	tm.h \
	util.c \
	util.h
	 

