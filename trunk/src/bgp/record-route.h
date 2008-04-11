// ==================================================================
// @(#)record-route.h
//
// AS-level record-route.
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 22/05/2007
// $Id: record-route.h,v 1.2 2008-04-11 11:03:06 bqu Exp $
// ==================================================================

#ifndef __BGP_RECORD_ROTUE_H__
#define __BGP_RECORD_ROUTE_H__

#include <libgds/log.h>

#include <bgp/as_t.h>
#include <bgp/path.h>
#include <net/prefix.h>

#define AS_RECORD_ROUTE_SUCCESS  0
#define AS_RECORD_ROUTE_LOOP     1
#define AS_RECORD_ROUTE_TOO_LONG 2
#define AS_RECORD_ROUTE_UNREACH  3

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ bgp_record_route ]---------------------------------------
  int bgp_record_route(bgp_router_t * router, ip_pfx_t prefix,
		       SBGPPath ** ppPath, int iPreserveDups);
  // -----[ bgp_dump_recorded_route ]--------------------------------
  void bgp_dump_recorded_route(SLogStream * stream, bgp_router_t * router,
			       ip_pfx_t prefix, SBGPPath * pPath,
			       int iResult);

#ifdef __cplusplus
}
#endif

#endif /* __BGP_RECORD_ROUTE_H__ */
