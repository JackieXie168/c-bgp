# Compile experimental features ?
if WITH_EXPERIMENTAL
  OPT_EXPERIMENTAL = -D__EXPERIMENTAL__
else
  OPT_EXPERIMENTAL =
endif

if WITH_EXTERNAL_BEST
  OPT_EXTERNAL_BEST = -D__EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
else
  OPT_EXTERNAL_BEST = 
endif

if WITH_WALTON
  OPT_WALTON = -D__EXPERIMENTAL_WALTON__
else
  OPT_WALTON = 
endif

AM_CFLAGS = $(OPT_EXPERIMENTAL) $(OPT_EXTERNAL_BEST) $(OPT_WALTON)

noinst_LTLIBRARIES = libbgp.la
libbgp_la_CFLAGS = $(LIBGDS_CFLAGS)

INCLUDES = -I..

libbgp_la_SOURCES = \
	as.c \
	as.h \
	as-level.c \
	as-level.h \
	as-level-filter.c \
	as-level-filter.h \
	as-level-types.h \
	as-level-stat.c \
	as-level-stat.h \
	as-level-util.c \
	as-level-util.h \
	as_t.h \
	attr.c \
	attr.h \
	auto-config.c \
	auto-config.h \
	bgp_assert.h \
	bgp_assert.c \
	bgp_debug.h \
	bgp_debug.c \
	brite.h \
	caida.c \
	caida.h \
	cisco.c \
	cisco.h \
	comm.c \
	comm.h \
	comm_t.h \
	comm_hash.c \
	comm_hash.h \
	dp_rt.c \
	dp_rt.h \
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
	path_hash.c \
	path_hash.h \
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
	record-route.c \
	record-route.h \
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
	route-input.c \
	route-input.h \
	route_t.h \
	stats.h \
	tie_breaks.c \
	tie_breaks.h \
	types.h \
	walton.c \
	walton.h

