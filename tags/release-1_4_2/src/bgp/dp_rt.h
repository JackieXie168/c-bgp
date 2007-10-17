// ==================================================================
// @(#)dp_rt.h
//
// This file contains the routines used to install/remove entries in
// the node's routing table when BGP routes are installed, removed or
// updated.
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 19/01/2007
// @lastdate 19/01/2007
// ==================================================================

#ifndef __BGP_DP_RT_H__
#define __BGP_DP_RT_H__

#include <bgp/as_t.h>

#ifdef __cplusplus
extern "C" {
#endif

  // ----- bgp_router_rt_add_route ----------------------------------
  void bgp_router_rt_add_route(SBGPRouter * pRouter, SRoute * pRoute);
  // ----- bgp_router_rt_del_route ----------------------------------
  void bgp_router_rt_del_route(SBGPRouter * pRouter, SPrefix sPrefix);

#ifdef __cplusplus
}
#endif

#endif /* __BGP_DP_RT_H__ */
