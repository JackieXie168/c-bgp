// ==================================================================
// @(#)bgp_debug.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @author Sebastien Tandel (standel@info.ucl.ac.be)
// 
// @date 09/04/2004
// @lastdate 10/03/2008
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
void bgp_debug_dp(SLogStream * pStream, SBGPRouter * pRouter, SPrefix sPrefix)
{
  bgp_route_t * pOldRoute;
  bgp_routes_t * pRoutes;
  bgp_peer_t * pPeer;
  bgp_route_t * pRoute;
  int iIndex;
  int iNumRoutes, iOldNumRoutes;
  int iRule;

#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  uint16_t uIndex;
  bgp_routes_t * pRoutesRIBIn;
#endif

  log_printf(pStream, "Debug Decision Process\n");
  log_printf(pStream, "----------------------\n");
  log_printf(pStream, "AS%u, ", pRouter->uASN);
  ip_address_dump(pStream, pRouter->pNode->tAddr);
  log_printf(pStream, ", ");
  ip_prefix_dump(pStream, sPrefix);
  log_printf(pStream, "\n\n");

#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  pOldRoute= rib_find_one_exact(pRouter->pLocRIB, sPrefix, NULL);
#else
  pOldRoute= rib_find_exact(pRouter->pLocRIB, sPrefix);
#endif

  /* Display current best route: */
  log_printf(pStream, "[ Current Best route: ]\n");
  if (pOldRoute != NULL) {
    route_dump(pStream, pOldRoute);
    log_printf(pStream, "\n");
  } else
    log_printf(pStream, "<none>\n");

  log_printf(pStream, "\n");

  log_printf(pStream, "[ Eligible routes: ]\n");
  /* If there is a LOCAL route: */
  if ((pOldRoute != NULL) &&
      route_flag_get(pOldRoute, ROUTE_FLAG_INTERNAL)) {
    route_dump(pStream, pOldRoute);
    log_printf(pStream, "\n");
  }

  // Build list of eligible routes received from peers
  pRoutes= routes_list_create(ROUTES_LIST_OPTION_REF);
  for (iIndex= 0; iIndex < ptr_array_length(pRouter->pPeers); iIndex++) {
    pPeer= (bgp_peer_t*) pRouter->pPeers->data[iIndex];
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
    pRoutesRIBIn = rib_find_exact(pPeer->pAdjRIB[RIB_IN], sPrefix);
    if (pRoutesRIBIn != NULL) {
      for (uIndex = 0; uIndex < routes_list_get_num(pRoutesRIBIn); uIndex++) {
	pRoute = routes_list_get_at(pRoutesRIBIn, uIndex);
#else
    pRoute= rib_find_exact(pPeer->pAdjRIB[RIB_IN], sPrefix);
#endif
    if ((pRoute != NULL) &&
	(route_flag_get(pRoute, ROUTE_FLAG_FEASIBLE)))
      routes_list_append(pRoutes, pRoute);
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
      }
    }
#endif
  }

  routes_list_dump(pStream, pRoutes);
  iNumRoutes= ptr_array_length(pRoutes);
  iOldNumRoutes= iNumRoutes;

  log_printf(pStream, "\n");

  // Start decision process
  if ((pOldRoute == NULL) ||
      !route_flag_get(pOldRoute, ROUTE_FLAG_INTERNAL)) {

    for (iRule= 0; iRule < DP_NUM_RULES; iRule++) {
      if (iNumRoutes <= 1)
	break;
      iOldNumRoutes= iNumRoutes;
      log_printf(pStream, "[ %s ]\n", DP_RULES[iRule].pcName);

      DP_RULES[iRule].fRule(pRouter, pRoutes);

      iNumRoutes= ptr_array_length(pRoutes);
      if (iNumRoutes < iOldNumRoutes)
	routes_list_dump(pStream, pRoutes);
    }

  } else
    log_printf(pStream, "*** local route is preferred ***\n");

  log_printf(pStream, "\n");

  log_printf(pStream, "[ Best route ]\n");
  if (iNumRoutes > 0)
    routes_list_dump(pStream, pRoutes);
  else
    log_printf(pStream, "<none>\n");
  
  routes_list_destroy(&pRoutes);

}

