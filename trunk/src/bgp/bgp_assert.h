// ==================================================================
// @(#)bgp_assert.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 08/03/2004
// @lastdate 31/03/2005
// ==================================================================

#ifndef __BGP_ASSERT_H__
#define __BGP_ASSERT_H__

#include <bgp/as.h>
#include <net/prefix.h>

#ifdef __cplusplus
extern "C" {
#endif

// ----- bgp_assert_reachability ------------------------------------
extern int bgp_assert_reachability();
// ----- bgp_assert_peerings ----------------------------------------
extern int bgp_assert_peerings();
// ----- bgp_router_assert_best -------------------------------------
extern int bgp_router_assert_best(SBGPRouter * pRouter,
				  SPrefix sPrefix,
				  net_addr_t tNextHop);
// ----- bgp_router_assert_feasible ---------------------------------
extern int bgp_router_assert_feasible(SBGPRouter * pRouter,
				      SPrefix sPrefix,
				      net_addr_t tNextHop);

#ifdef __cplusplus
}
#endif

#endif
