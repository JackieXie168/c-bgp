// ==================================================================
// @(#)dp_rt.c
//
// This file contains the routines used to install/remove entries in
// the node's routing table when BGP routes are installed, removed or
// updated.
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 19/01/2007
// @lastdate 16/04/2007
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
static void _bgp_router_rt_add_route_error(SBGPRouter * pRouter,
					   SRoute * pRoute,
					   SNetRouteNextHop * pNextHop,
					   int iErrorCode)
{
  if (log_enabled(pLogErr, LOG_LEVEL_FATAL)) {
    log_printf(pLogErr, "Error: could not install BGP route in RT of ");
    bgp_router_dump_id(pLogErr, pRouter);
    log_printf(pLogErr, "\n");
    log_printf(pLogErr, "  BGP route: ");
    route_dump(pLogErr, pRoute);
    log_printf(pLogErr, ")\n");
    log_printf(pLogErr, "  RT entry : nh:");
    ip_address_dump(pLogErr, pNextHop->tGateway);
    log_printf(pLogErr, ", if:");
    ip_prefix_dump(pLogErr, net_link_get_id(pNextHop->pIface));
    log_printf(pLogErr, ")\n");
    log_printf(pLogErr, "  reason   : ");
    rt_perror(pLogErr, iErrorCode);
    log_printf(pLogErr, ")\n");
  }
  abort();
}

// ----- bgp_router_rt_add_route ------------------------------------
/**
 * This function inserts a BGP route into the node's routing
 * table. The route's next-hop is resolved before insertion.
 */
void bgp_router_rt_add_route(SBGPRouter * pRouter, SRoute * pRoute)
{
  SNetRouteInfo * pOldRouteInfo;
  SNetRouteNextHop * pNextHop= node_rt_lookup(pRouter->pNode,
					      pRoute->pAttr->tNextHop);
  net_addr_t tGateway;
  int iResult;

  /* Check that the next-hop is reachable. It MUST be reachable at
     this point (checked upon route reception). */
  assert(pNextHop != NULL);
  
  /* Get the previous route if it exists */
  pOldRouteInfo= rt_find_exact(pRouter->pNode->pRT, pRoute->sPrefix,
			       NET_ROUTE_BGP);
  if (pOldRouteInfo != NULL) {
    if (!route_nexthop_compare(pOldRouteInfo->sNextHop, *pNextHop))
      return;
    // Remove the previous route (if it exists)
    node_rt_del_route(pRouter->pNode, &pRoute->sPrefix,
		      NULL, NULL, NET_ROUTE_BGP);
  }
  
  // Insert the route
  tGateway= pNextHop->tGateway;
  if ((pNextHop->pIface->uType == NET_LINK_TYPE_TRANSIT) ||
      (pNextHop->pIface->uType == NET_LINK_TYPE_STUB))
    tGateway= pRoute->pAttr->tNextHop;

  iResult= node_rt_add_route_link(pRouter->pNode, pRoute->sPrefix,
				  pNextHop->pIface, tGateway,
				  0, NET_ROUTE_BGP);
  if (iResult)
    _bgp_router_rt_add_route_error(pRouter, pRoute, pNextHop, iResult);
}

// ----- bgp_router_rt_del_route ------------------------------------
/**
 * This function removes from the node's routing table the BGP route
 * towards the given prefix.
 */
void bgp_router_rt_del_route(SBGPRouter * pRouter, SPrefix sPrefix)
{
  /*SNetRouteInfo * pRouteInfo;

  fprintf(stderr, "DEL ROUTE towards ");
  ip_prefix_dump(stderr, sPrefix);
  fprintf(stderr, " ");
  pRouteInfo= rt_find_exact(pRouter->pNode->pRT, sPrefix, NET_ROUTE_ANY);
  if (pRouteInfo != NULL) {
    net_route_info_dump(stderr, pRouteInfo);
    fprintf(stderr, "\n");
  } else {
    fprintf(stderr, "*** NONE ***\n");
    }*/

  assert(!node_rt_del_route(pRouter->pNode, &sPrefix,
			    NULL, NULL, NET_ROUTE_BGP));
}
