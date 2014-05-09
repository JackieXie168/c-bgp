// ==================================================================
// @(#)bgp_assert.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 08/03/2004
// $Id: bgp_assert.h,v 1.6 2009-03-24 14:08:04 bqu Exp $
// ==================================================================

#ifndef __BGP_ASSERT_H__
#define __BGP_ASSERT_H__

#include <net/prefix.h>
#include <bgp/types.h>

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ bgp_assert_reachability ]--------------------------------
  int bgp_assert_reachability();
  // -----[ bgp_assert_peerings ]------------------------------------
  int bgp_assert_peerings();
  // -----[ bgp_router_assert_best ]---------------------------------
  int bgp_router_assert_best(bgp_router_t * router, ip_pfx_t prefix,
			     net_addr_t next_hop);
  // -----[ bgp_router_assert_feasible ]-----------------------------
  int bgp_router_assert_feasible(bgp_router_t * router, ip_pfx_t prefix,
				 net_addr_t next_hop);

#ifdef __cplusplus
}
#endif

#endif /* __BGP_ASSERT_H__ */
