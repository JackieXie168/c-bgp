// ==================================================================
// @(#)filter.h
//
// Provide filtering capabilities to prune AS-level topologies from
// specific classes of nodes / edges. Example of such filters include
// - remove all stubs
// - remove all single-homed stubs
// - remove all peer-to-peer links
// - remove all but top-level domains
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 21/06/2007
// $Id: filter.h,v 1.2 2009-08-31 09:35:40 bqu Exp $
// ==================================================================

#ifndef __BGP_ASLEVEL_FILTER_H__
#define __BGP_ASLEVEL_FILTER_H__

#include <bgp/aslevel/types.h>

// ----- Topology filters -----
#define ASLEVEL_FILTER_STUBS    0
#define ASLEVEL_FILTER_SHSTUBS  1
#define ASLEVEL_FILTER_P2P      2
#define ASLEVEL_FILTER_KEEP_TOP 3

typedef uint8_t aslevel_filter_t;

typedef union {
  asn_t asn;
} aslevel_filter_args_t;

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ aslevel_filter_str2filter ]------------------------------
  int aslevel_filter_str2filter(const char * str,
				aslevel_filter_t * filter);
  // -----[ aslevel_filter_topo ]------------------------------------
  int aslevel_filter_topo(as_level_topo_t * topo, aslevel_filter_t filter);

#ifdef __cplusplus
}
#endif

#endif /* __BGP_ASLEVEL_FILTER_H__ */
