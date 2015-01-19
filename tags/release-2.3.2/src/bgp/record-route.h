// ==================================================================
// @(#)record-route.h
//
// AS-level record-route.
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 22/05/2007
// $Id: record-route.h,v 1.4 2009-06-25 14:27:58 bqu Exp $
// ==================================================================

#ifndef __BGP_RECORD_ROUTE_H__
#define __BGP_RECORD_ROUTE_H__

#include <libgds/stream.h>

#include <net/prefix.h>
#include <bgp/types.h>

#define AS_RECORD_ROUTE_SUCCESS  0
#define AS_RECORD_ROUTE_LOOP     1
#define AS_RECORD_ROUTE_TOO_LONG 2
#define AS_RECORD_ROUTE_UNREACH  3

/** Preserve duplicate ASNs in the trace */
#define AS_RECORD_ROUTE_OPT_PRESERVE_DUPS 0x01
/** Perform exact match instead of best-match (i.e. longest-match) */
#define AS_RECORD_ROUTE_OPT_EXACT_MATCH   0x02

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ bgp_record_route ]---------------------------------------
  /**
   * This function records the AS-path from one BGP router towards a
   * given destination prefix.
   *
   * @param router   is the source router.
   * @param prefix   is the destination prefix.
   * @param path_ref is the resulting AS-level trace.
   * @param options  is a set of options.
   * @return AS_RECORD_ROUTE_SUCCESS in case of success and a
   * negative error code in case of error.
   */
  int bgp_record_route(bgp_router_t * router, ip_pfx_t prefix,
		       bgp_path_t ** path_ref, uint8_t options);

  // -----[ bgp_dump_recorded_route ]--------------------------------
  void bgp_dump_recorded_route(gds_stream_t * stream,
			       bgp_router_t * router,
			       ip_pfx_t prefix, bgp_path_t * path,
			       int result);

#ifdef __cplusplus
}
#endif

#endif /* __BGP_RECORD_ROUTE_H__ */
