// ==================================================================
// @(#)record-route.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 22/05/2007
// $Id: record-route.c,v 1.3 2008-04-11 11:03:06 bqu Exp $
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <libgds/log.h>

#include <bgp/as_t.h>
#include <bgp/record-route.h>
#include <bgp/rib.h>
#include <net/prefix.h>
#include <net/protocol.h>

// -----[ bgp_record_route ]-----------------------------------------
/**
 * This function records the AS-path from one BGP router towards a
 * given prefix. The function has two modes:
 * - records all ASes
 * - records ASes once (do not record iBGP session crossing)
 */
int bgp_record_route(bgp_router_t * router,
		     ip_pfx_t sPrefix,
		     SBGPPath ** path_ref,
		     int iPreserveDups)
{
  bgp_router_t * cur_router= router;
  bgp_router_t * prev_router= NULL;
  bgp_route_t * route;
  SBGPPath * path= path_create();
  net_node_t * node;
  net_protocol_t * protocol;
  int result= AS_RECORD_ROUTE_UNREACH;
  network_t * network= router->pNode->network;

  *path_ref= NULL;

  while (cur_router != NULL) {
    
    // Is there, in the current node, a BGP route towards the given
    // prefix ?
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
    route= rib_find_one_best(cur_router->pLocRIB, sPrefix);
#else
    route= rib_find_best(cur_router->pLocRIB, sPrefix);
#endif
    if (route != NULL) {
      
      // Record current node's AS-Num ??
      if ((prev_router == NULL) ||
	  (iPreserveDups ||
	   (prev_router->uASN != cur_router->uASN))) {
	if (path_append(&path, cur_router->uASN) < 0) {
	  result= AS_RECORD_ROUTE_TOO_LONG;
	  break;
	}
      }
      
      // If the route's next-hop is this router, then the function
      // terminates.
      if (route->pAttr->tNextHop == cur_router->pNode->addr) {
	result= AS_RECORD_ROUTE_SUCCESS;
	break;
      }
      
      // Otherwize, looks for next-hop router
      node= network_find_node(network, route->pAttr->tNextHop);
      if (node == NULL)
	break;
      
      // Get the current node's BGP instance
      protocol= protocols_get(node->protocols, NET_PROTOCOL_BGP);
      if (protocol == NULL)
	break;
      prev_router= cur_router;
      cur_router= (bgp_router_t *) protocol->pHandler;
      
    } else
      break;
  }
  *path_ref= path;

  return result;
}

// -----[ bgp_dump_recorded_route ]----------------------------------
/**
 * This function dumps the result of a call to
 * 'bgp_router_record_route'.
 */
void bgp_dump_recorded_route(SLogStream * stream, bgp_router_t * router,
			     ip_pfx_t sPrefix, SBGPPath * path,
			     int result)
{
  // Display record-route results
  ip_address_dump(stream, router->pNode->addr);
  log_printf(stream, "\t");
  ip_prefix_dump(stream, sPrefix);
  log_printf(stream, "\t");
  switch (result) {
  case AS_RECORD_ROUTE_SUCCESS: log_printf(stream, "SUCCESS"); break;
  case AS_RECORD_ROUTE_TOO_LONG: log_printf(stream, "TOO_LONG"); break;
  case AS_RECORD_ROUTE_UNREACH: log_printf(stream, "UNREACHABLE"); break;
  default:
    log_printf(stream, "UNKNOWN_ERROR");
  }
  log_printf(stream, "\t");
  path_dump(stream, path, 0);
  log_printf(stream, "\n");

  log_flush(stream);
}
