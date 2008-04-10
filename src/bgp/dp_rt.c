// ==================================================================
// @(#)dp_rt.c
//
// This file contains the routines used to install/remove entries in
// the node's routing table when BGP routes are installed, removed or
// updated.
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 19/01/2007
// $Id: dp_rt.c,v 1.5 2008-04-10 11:27:00 bqu Exp $
// ==================================================================

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <stdlib.h>

#include <libgds/log.h>
#include <net/link.h>
#include <net/node.h>
#include <bgp/as.h>
#include <bgp/as_t.h>
#include <bgp/dp_rt.h>
#include <bgp/route.h>

// ----- _bgp_router_rt_add_route_error -----------------------------
static void _bgp_router_rt_add_route_error(bgp_router_t * router,
					   bgp_route_t * route,
					   const rt_entry_t * rtentry,
					   int iErrorCode)
{
  if (log_enabled(pLogErr, LOG_LEVEL_FATAL)) {
    log_printf(pLogErr, "Error: could not install BGP route in RT of ");
    bgp_router_dump_id(pLogErr, router);
    log_printf(pLogErr, "\n");
    log_printf(pLogErr, "  BGP route: ");
    route_dump(pLogErr, route);
    log_printf(pLogErr, ")\n");
    log_printf(pLogErr, "  RT entry : nh:");
    ip_address_dump(pLogErr, rtentry->gateway);
    log_printf(pLogErr, ", if:");
    net_iface_dump_id(pLogErr, rtentry->oif);
    log_printf(pLogErr, ")\n");
    log_printf(pLogErr, "  reason   : ");
    network_perror(pLogErr, iErrorCode);
    log_printf(pLogErr, ")\n");
  }
  abort();
}

// ----- bgp_router_rt_add_route ------------------------------------
/**
 * This function inserts a BGP route into the node's routing
 * table. The route's next-hop is resolved before insertion.
 */
void bgp_router_rt_add_route(bgp_router_t * router, bgp_route_t * route)
{
  rt_info_t * old_rtinfo;
  const rt_entry_t * rtentry= node_rt_lookup(router->pNode,
					     route->pAttr->tNextHop);
  net_addr_t gateway;
  int result;

  /* Check that the next-hop is reachable. It MUST be reachable at
     this point (checked upon route reception). */
  assert(rtentry != NULL);
  
  /* Get the previous route if it exists */
  old_rtinfo= rt_find_exact(router->pNode->rt, route->sPrefix,
			    NET_ROUTE_BGP);
  if (old_rtinfo != NULL) {
    if (!route_nexthop_compare(old_rtinfo->next_hop, *rtentry))
      return;
    // Remove the previous route (if it exists)
    node_rt_del_route(router->pNode, &route->sPrefix,
		      NULL, NULL, NET_ROUTE_BGP);
  }
  
  // Insert the route
  gateway= rtentry->gateway;
  if (rtentry->oif->type == NET_IFACE_PTMP)
    gateway= route->pAttr->tNextHop;

  result= node_rt_add_route_link(router->pNode, route->sPrefix,
				 rtentry->oif, gateway,
				 0, NET_ROUTE_BGP);
  if (result)
    _bgp_router_rt_add_route_error(router, route, rtentry, result);
}

// ----- bgp_router_rt_del_route ------------------------------------
/**
 * This function removes from the node's routing table the BGP route
 * towards the given prefix.
 */
void bgp_router_rt_del_route(bgp_router_t * router, ip_pfx_t prefix)
{
  /*rt_info_t * pRouteInfo;

  fprintf(stderr, "DEL ROUTE towards ");
  ip_prefix_dump(stderr, sPrefix);
  fprintf(stderr, " ");
  pRouteInfo= rt_find_exact(router->pNode->rt, sPrefix, NET_ROUTE_ANY);
  if (pRouteInfo != NULL) {
    net_route_info_dump(stderr, pRouteInfo);
    fprintf(stderr, "\n");
  } else {
    fprintf(stderr, "*** NONE ***\n");
    }*/

  assert(!node_rt_del_route(router->pNode, &prefix,
			    NULL, NULL, NET_ROUTE_BGP));
}
