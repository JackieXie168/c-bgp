noinst_LTLIBRARIES = libbgp.la

INCLUDES = -I..

libbgp_la_SOURCES = \
	as.c \
	as.h \
	as_t.h \
	bgp_assert.h \
	bgp_assert.c \
	bgp_debug.h \
	bgp_debug.c \
	brite.h \
	comm.c \
	comm.h \
	comm_t.h \
	dp_rules.c \
	dp_rules.h \
	domain.c \
	domain.h \
	ecomm.c \
	ecomm.h \
	ecomm_t.h \
	filter.c \
	filter.h \
	filter_parser.c \
	filter_parser.h \
	filter_registry.c \
	filter_registry.h \
	filter_t.h \
	message.c \
	message.h \
	mrtd.c \
	mrtd.h \
	path.c \
	path.h \
	path_segment.c \
	path_segment.h \
	peer.c \
	peer.h \
	peer_t.h \
	predicate_parser.c \
	predicate_parser.h \
	qos.c \
	qos.h \
	qos_t.h \
	rexford.c \
	rexford.h \
	rib.c \
	rib.h \
	rib_t.h \
	route.c \
	route.h \
	route_list.c \
	route_reflector.c \
	route_reflector.h \
	route_map.c \
	route_map.h \
	routes_list.c \
	routes_list.h \
	route_t.h \
	stats.h \
	tie_breaks.c \
	tie_breaks.h \
	xml_topology.c \
	xml_topology.h

# Compile experimental features ?
if WITH_EXPERIMENTAL
  OPT_EXPERIMENTAL = -D__EXPERIMENTAL__
else
  OPT_EXPERIMENTAL =
endif

AM_CFLAGS = -Wall -Werror -O2 $(OPT_EXPERIMENTAL)

