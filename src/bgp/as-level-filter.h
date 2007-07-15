// ==================================================================
// @(#)as-level-filter.h
//
// Provide filtering capabilities to prune AS-level topologies from
// specific classes of nodes / edges. Example of such filters include
// - remove all stubs
// - remove all single-homed stubs
// - remove all peer-to-peer links
// - remove all but top-level domains
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 21/06/2007
// @lastdate 21/06/2007
// ==================================================================

#ifndef __BGP_ASLEVEL_FILTER_H__
#define __BGP_ASLEVEL_FILTER_H__

#include <bgp/as-level.h>

// ----- Topology filters -----
#define ASLEVEL_FILTER_STUBS    0
#define ASLEVEL_FILTER_SHSTUBS  1
#define ASLEVEL_FILTER_P2P      2
#define ASLEVEL_FILTER_KEEP_TOP 3

typedef uint8_t aslevel_filter_t;

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ aslevel_filter_str2filter ]------------------------------
  int aslevel_filter_str2filter(const char * pcFilter,
				aslevel_filter_t * puFilter);
  // -----[ aslevel_filter_topo ]------------------------------------
  int aslevel_filter_topo(SASLevelTopo * pTopo, aslevel_filter_t uFilter);

#ifdef __cplusplus
}
#endif

#endif /* __BGP_ASLEVEL_FILTER_H__ */
