// ==================================================================
// @(#)routing.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 24/02/2004
// $Id: routing.h,v 1.19 2008-06-11 15:13:45 bqu Exp $
// ==================================================================

#ifndef __NET_ROUTING_H__
#define __NET_ROUTING_H__

#include <stdio.h>

#include <libgds/array.h>
#include <libgds/log.h>
#include <libgds/types.h>

#include <net/prefix.h>
#include <net/link.h>
#include <net/routing_t.h>

typedef SPtrArray rt_info_list_t;

// -----[ rt_entry_t ]-----------------------------------------------
/**
 * This type defines a route next-hop. It is composed of the outgoing
 * link (iface) as well as the address of the destination node.
 *
 * If the outgoing link (iface) is a point-to-point link, the next-hop
 * address need not be specified. To the contrary, the next-hop
 * address is mandatory for a multi-point link such as a subnet.
 */
typedef struct {
  net_addr_t    gateway;
  net_iface_t * oif;
} rt_entry_t;

//typedef SPtrArray SNetRouteNextHops;

// ----- rt_info_t ----------------------------------------------
typedef struct {
  ip_pfx_t         prefix;
  uint32_t         metric;
  rt_entry_t       next_hop;
  //SNetRouteNextHops * pNextHops;
  net_route_type_t type;
} rt_info_t;

#include <net/rt_filter.h>

#ifdef __cplusplus
extern "C" {
#endif

  ///////////////////////////////////////////////////////////////////
  // ROUTE (rt_info_t)
  ///////////////////////////////////////////////////////////////////

  // ----- route_nexthop_compare ------------------------------------
  int route_nexthop_compare(rt_entry_t sNH1,
			    rt_entry_t sNH2);
  // ----- net_route_info_create ------------------------------------
  rt_info_t * net_route_info_create(ip_pfx_t prefix,
				    net_iface_t * oif,
				    net_addr_t next_hop,
				    uint32_t metric,
				    net_route_type_t type);
  // ----- net_route_info_destroy -----------------------------------
  void net_route_info_destroy(rt_info_t ** rtinfo_ref);
  // ----- net_route_info_dump --------------------------------------
  void net_route_info_dump(SLogStream * stream, rt_info_t * rtinfo);


  ///////////////////////////////////////////////////////////////////
  // ROUTING TABLE (net_rt_t)
  ///////////////////////////////////////////////////////////////////
  
  // ----- rt_create ------------------------------------------------
  net_rt_t * rt_create();
  // ----- rt_destroy -----------------------------------------------
  void rt_destroy(net_rt_t ** rt_ref);
  // ----- rt_find_best ---------------------------------------------
  rt_info_t * rt_find_best(net_rt_t * rt, net_addr_t addr,
			   net_route_type_t type);
  // ----- rt_find_exact --------------------------------------------
  rt_info_t * rt_find_exact(net_rt_t * rt, ip_pfx_t prefix,
			    net_route_type_t type);
  // ----- rt_add_route ---------------------------------------------
  int rt_add_route(net_rt_t * rt, ip_pfx_t prefix,
		   rt_info_t * rtinfo);
  // ----- rt_del_routes --------------------------------------------
  int rt_del_routes(net_rt_t * rt, rt_filter_t * filter);
  // ----- rt_del_route ---------------------------------------------
  int rt_del_route(net_rt_t * rt, ip_pfx_t * prefix,
		   net_iface_t * oif, net_addr_t * next_hop,
		   net_route_type_t type);
  // ----- net_route_type_dump --------------------------------------
  void net_route_type_dump(SLogStream * stream, net_route_type_t type);
  // ----- rt_dump --------------------------------------------------
  void rt_dump(SLogStream * stream, net_rt_t * rt, SNetDest dest);
  
  // ----- rt_for_each ----------------------------------------------
  int rt_for_each(net_rt_t * rt, FRadixTreeForEach fForEach,
		  void * ctx);
  
#ifdef __cplusplus
}
#endif

#endif /* __NET_ROUTING_H__ */
