// ==================================================================
// @(#)rib.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @author Sebastien Tandel (standel@info.ucl.ac.be)
// 
// @date 04/12/2002
// $Id: rib.h,v 1.8 2009-03-24 15:50:17 bqu Exp $
// ==================================================================

#ifndef __BGP_RIB_H__
#define __BGP_RIB_H__

#include <net/prefix.h>
#include <bgp/types.h>

#define RIB_OPTION_ALIAS 0x01

#ifdef __cplusplus
extern "C" {
#endif

  // ----- rib_create -----------------------------------------------
  bgp_rib_t * rib_create(uint8_t options);
  // ----- rib_destroy ----------------------------------------------
  void rib_destroy(bgp_rib_t ** rib_ref);
  // ----- rib_find_best --------------------------------------------
#if defined __EXPERIMENTAL__ && __EXPERIMENTAL_WALTON__
  bgp_route_ts * rib_find_best(bgp_rib_t * rib, ip_pfx_t prefix);
  bgp_route_t * rib_find_one_best(bgp_rib_t * rib, ip_pfx_t prefix);
#else
  bgp_route_t * rib_find_best(bgp_rib_t * rib, ip_pfx_t prefix);
#endif
  // ----- rib_find_exact -------------------------------------------
#if defined __EXPERIMENTAL__ && __EXPERIMENTAL_WALTON__
  bgp_route_ts * rib_find_exact(bgp_rib_t * rib, ip_pfx_t prefix);
  bgp_route_t * rib_find_one_exact(bgp_rib_t * rib, ip_pfx_t prefix,
				   net_addr_t *next_hop);
#else
  bgp_route_t * rib_find_exact(bgp_rib_t * rib, ip_pfx_t prefix);
#endif
  // ----- rib_add_route --------------------------------------------
  int rib_add_route(bgp_rib_t * rib, bgp_route_t * route);
  // ----- rib_replace_route ----------------------------------------
  int rib_replace_route(bgp_rib_t * rib, bgp_route_t * route);
  // ----- rib_remove_route -----------------------------------------
#if defined __EXPERIMENTAL__ && __EXPERIMENTAL_WALTON__
  int rib_remove_route(bgp_rib_t * rib, ip_pfx_t prefix,
		       net_addr_t * next_hop);
#else
  int rib_remove_route(bgp_rib_t * rib, ip_pfx_t prefix);
#endif
  // ----- rib_for_each ---------------------------------------------
  int rib_for_each(bgp_rib_t * rib, FRadixTreeForEach fForEach,
		   void * context);

#ifdef __cplusplus
}
#endif

#endif /* __BGP_RIB_H__ */
