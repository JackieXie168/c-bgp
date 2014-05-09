// ==================================================================
// @(#)as-level-stat.h
//
// Provide structures and functions to report some AS-level
// topologies statistics.
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 21/06/2007
// $Id: stat.h,v 1.1 2009-03-24 13:39:08 bqu Exp $
// ==================================================================

#ifndef __BGP_ASLEVEL_STAT_H__
#define __BGP_ASLEVEL_STAT_H__

#include <libgds/stream.h>

#include <bgp/aslevel/as-level.h>

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ aslevel_stat_degree ]------------------------------------
  void aslevel_stat_degree(gds_stream_t * stream, as_level_topo_t * topo,
			   int cumulated, int normalized,
			   int inverse);

#ifdef __cplusplus
}
#endif

#endif /* __BGP_ASLEVEL_STAT_H__ */
