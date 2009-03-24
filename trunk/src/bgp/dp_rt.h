// ==================================================================
// @(#)dp_rt.h
//
// This file contains the routines used to install/remove entries in
// the node's routing table when BGP routes are installed, removed or
// updated.
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 19/01/2007
// $Id: dp_rt.h,v 1.3 2009-03-24 14:13:05 bqu Exp $
// ==================================================================

#ifndef __BGP_DP_RT_H__
#define __BGP_DP_RT_H__

#include <bgp/types.h>

#ifdef __cplusplus
extern "C" {
#endif

  // ----- bgp_router_rt_add_route ----------------------------------
  void bgp_router_rt_add_route(bgp_router_t * router, bgp_route_t * route);
  // ----- bgp_router_rt_del_route ----------------------------------
  void bgp_router_rt_del_route(bgp_router_t * router, ip_pfx_t prefix);

#ifdef __cplusplus
}
#endif

#endif /* __BGP_DP_RT_H__ */
