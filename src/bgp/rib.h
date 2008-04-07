// ==================================================================
// @(#)rib.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @author Sebastien Tandel (standel@info.ucl.ac.be)
// 
// @date 04/12/2002
// @lastdate 12/03/2008
// ==================================================================

#ifndef __BGP_RIB_H__
#define __BGP_RIB_H__

#include <net/prefix.h>
#include <bgp/rib_t.h>
#include <bgp/route.h>

#define RIB_OPTION_ALIAS 0x01

#ifdef __cplusplus
extern "C" {
#endif

  // ----- rib_create -----------------------------------------------
  SRIB * rib_create(uint8_t uOptions);
  // ----- rib_destroy ----------------------------------------------
  void rib_destroy(SRIB ** ppRIB);
  // ----- rib_find_best --------------------------------------------
#if defined __EXPERIMENTAL__ && __EXPERIMENTAL_WALTON__
  bgp_route_ts * rib_find_best(SRIB * pRIB, SPrefix sPrefix);
  bgp_route_t * rib_find_one_best(SRIB * pRIB, SPrefix sPrefix);
#else
  bgp_route_t * rib_find_best(SRIB * pRIB, SPrefix sPrefix);
#endif
  // ----- rib_find_exact -------------------------------------------
#if defined __EXPERIMENTAL__ && __EXPERIMENTAL_WALTON__
  bgp_route_ts * rib_find_exact(SRIB * pRIB, SPrefix sPrefix);
  bgp_route_t * rib_find_one_exact(SRIB * pRIB, SPrefix sPrefix,
			      net_addr_t *tNextHop);
#else
  bgp_route_t * rib_find_exact(SRIB * pRIB, SPrefix sPrefix);
#endif
  // ----- rib_add_route --------------------------------------------
  int rib_add_route(SRIB * pRIB, bgp_route_t * pRoute);
  // ----- rib_replace_route ----------------------------------------
  int rib_replace_route(SRIB * pRIB, bgp_route_t * pRoute);
  // ----- rib_remove_route -----------------------------------------
#if defined __EXPERIMENTAL__ && __EXPERIMENTAL_WALTON__
  int rib_remove_route(SRIB * pRIB, SPrefix sPrefix, net_addr_t * tNextHop);
#else
  int rib_remove_route(SRIB * pRIB, SPrefix sPrefix);
#endif
  // ----- rib_for_each ---------------------------------------------
  int rib_for_each(SRIB * pRIB, FRadixTreeForEach fForEach,
		   void * pContext);

#ifdef __cplusplus
}
#endif

#endif /* __BGP_RIB_H__ */
