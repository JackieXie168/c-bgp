// ==================================================================
// @(#)record-route.h
//
// AS-level record-route.
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 22/05/2007
// $Id: record-route.h,v 1.3 2009-03-24 15:49:39 bqu Exp $
// ==================================================================

#ifndef __BGP_RECORD_ROTUE_H__
#define __BGP_RECORD_ROUTE_H__

#include <libgds/stream.h>

#include <net/prefix.h>
#include <bgp/types.h>

#define AS_RECORD_ROUTE_SUCCESS  0
#define AS_RECORD_ROUTE_LOOP     1
#define AS_RECORD_ROUTE_TOO_LONG 2
#define AS_RECORD_ROUTE_UNREACH  3

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ bgp_record_route ]---------------------------------------
  int bgp_record_route(bgp_router_t * router, ip_pfx_t prefix,
		       bgp_path_t ** path_ref, int preserve_dups);
  // -----[ bgp_dump_recorded_route ]--------------------------------
  void bgp_dump_recorded_route(gds_stream_t * stream, bgp_router_t * router,
			       ip_pfx_t prefix, bgp_path_t * path,
			       int result);

#ifdef __cplusplus
}
#endif

#endif /* __BGP_RECORD_ROUTE_H__ */
