// ==================================================================
// @(#)bgp_assert.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 08/03/2004
// @lastdate 22/05/2007
// ==================================================================

#ifndef __BGP_ASSERT_H__
#define __BGP_ASSERT_H__

#include <bgp/as.h>
#include <net/prefix.h>

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ bgp_assert_reachability ]--------------------------------
  int bgp_assert_reachability();
  // -----[ bgp_assert_peerings ]------------------------------------
  int bgp_assert_peerings();
  // -----[ bgp_router_assert_best ]---------------------------------
  int bgp_router_assert_best(SBGPRouter * pRouter, SPrefix sPrefix,
			     net_addr_t tNextHop);
  // -----[ bgp_router_assert_feasible ]-----------------------------
  int bgp_router_assert_feasible(SBGPRouter * pRouter, SPrefix sPrefix,
				 net_addr_t tNextHop);

#ifdef __cplusplus
}
#endif

#endif /* __BGP_ASSERT_H__ */
