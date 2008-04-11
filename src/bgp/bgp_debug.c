// ==================================================================
// @(#)bgp_debug.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @author Sebastien Tandel (standel@info.ucl.ac.be)
// 
// @date 09/04/2004
// $Id: bgp_debug.c,v 1.14 2008-04-11 11:03:06 bqu Exp $
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <bgp/as.h>
#include <bgp/bgp_debug.h>
#include <bgp/dp_rules.h>
#include <bgp/peer.h>
#include <bgp/rib.h>
#include <bgp/route_t.h>
#include <bgp/routes_list.h>
#include <stdio.h>

// ----- bgp_debug_dp -----------------------------------------------
/**
 *
 */
void bgp_debug_dp(SLogStream * stream, bgp_router_t * router, ip_pfx_t prefix)
{
  bgp_route_t * pOldRoute;
  bgp_routes_t * pRoutes;
  bgp_peer_t * pPeer;
  bgp_route_t * pRoute;
  unsigned int index;
  int iNumRoutes, iOldNumRoutes;
  int iRule;

#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  uint16_t uIndex;
  bgp_routes_t * pRoutesRIBIn;
#endif

  log_printf(stream, "Debug Decision Process\n");
  log_printf(stream, "----------------------\n");
  log_printf(stream, "AS%u, ", router->uASN);
  ip_address_dump(stream, router->pNode->addr);
  log_printf(stream, ", ");
  ip_prefix_dump(stream, prefix);
  log_printf(stream, "\n\n");

#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  pOldRoute= rib_find_one_exact(router->pLocRIB, prefix, NULL);
#else
  pOldRoute= rib_find_exact(router->pLocRIB, prefix);
#endif

  /* Display current best route: */
  log_printf(stream, "[ Current Best route: ]\n");
  if (pOldRoute != NULL) {
    route_dump(stream, pOldRoute);
    log_printf(stream, "\n");
  } else
    log_printf(stream, "<none>\n");

  log_printf(stream, "\n");

  log_printf(stream, "[ Eligible routes: ]\n");
  /* If there is a LOCAL route: */
  if ((pOldRoute != NULL) &&
      route_flag_get(pOldRoute, ROUTE_FLAG_INTERNAL)) {
    route_dump(stream, pOldRoute);
    log_printf(stream, "\n");
  }

  // Build list of eligible routes received from peers
  pRoutes= routes_list_create(ROUTES_LIST_OPTION_REF);
  for (index= 0; index < bgp_peers_size(router->pPeers); index++) {
    pPeer= bgp_peers_at(router->pPeers, index);
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
    pRoutesRIBIn = rib_find_exact(pPeer->pAdjRIB[RIB_IN], prefix);
    if (pRoutesRIBIn != NULL) {
      for (uIndex = 0; uIndex < routes_list_get_num(pRoutesRIBIn); uIndex++) {
	pRoute = routes_list_get_at(pRoutesRIBIn, uIndex);
#else
    pRoute= rib_find_exact(pPeer->pAdjRIB[RIB_IN], prefix);
#endif
    if ((pRoute != NULL) &&
	(route_flag_get(pRoute, ROUTE_FLAG_FEASIBLE)))
      routes_list_append(pRoutes, pRoute);
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
      }
    }
#endif
  }

  routes_list_dump(stream, pRoutes);
  iNumRoutes= ptr_array_length(pRoutes);
  iOldNumRoutes= iNumRoutes;

  log_printf(stream, "\n");

  // Start decision process
  if ((pOldRoute == NULL) ||
      !route_flag_get(pOldRoute, ROUTE_FLAG_INTERNAL)) {

    for (iRule= 0; iRule < DP_NUM_RULES; iRule++) {
      if (iNumRoutes <= 1)
	break;
      iOldNumRoutes= iNumRoutes;
      log_printf(stream, "[ %s ]\n", DP_RULES[iRule].pcName);

      DP_RULES[iRule].fRule(router, pRoutes);

      iNumRoutes= ptr_array_length(pRoutes);
      if (iNumRoutes < iOldNumRoutes)
	routes_list_dump(stream, pRoutes);
    }

  } else
    log_printf(stream, "*** local route is preferred ***\n");

  log_printf(stream, "\n");

  log_printf(stream, "[ Best route ]\n");
  if (iNumRoutes > 0)
    routes_list_dump(stream, pRoutes);
  else
    log_printf(stream, "<none>\n");
  
  routes_list_destroy(&pRoutes);

}

