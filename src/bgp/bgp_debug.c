// ==================================================================
// @(#)bgp_debug.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 09/04/2004
// @lastdate 15/11/2005
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
void bgp_debug_dp(FILE * pStream, SBGPRouter * pRouter, SPrefix sPrefix)
{
  SRoute * pOldRoute;
  SRoutes * pRoutes;
  SPeer * pPeer;
  SRoute * pRoute;
  int iIndex;
  int iNumRoutes, iOldNumRoutes;
  int iRule;
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  uint16_t uIndex;
  SRoutes * pRoutesRIBIn;
#endif
  fprintf(pStream, "Debug Decision Process\n");
  fprintf(pStream, "----------------------\n");
  fprintf(pStream, "AS%u, ", pRouter->uNumber);
  ip_address_dump(pStream, pRouter->pNode->tAddr);
  fprintf(pStream, ", ");
  ip_prefix_dump(pStream, sPrefix);
  fprintf(pStream, "\n\n");

#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  pOldRoute= rib_find_one_exact(pRouter->pLocRIB, sPrefix);
#else
  pOldRoute= rib_find_exact(pRouter->pLocRIB, sPrefix);
#endif

  /* Display current best route: */
  fprintf(pStream, "[ Current Best route: ]\n");
  if (pOldRoute != NULL) {
    route_dump(pStream, pOldRoute);
    fprintf(pStream, "\n");
  } else
    fprintf(pStream, "<none>\n");

  fprintf(pStream, "\n");

  fprintf(pStream, "[ Eligible routes: ]\n");
  /* If there is a LOCAL route: */
  if ((pOldRoute != NULL) &&
      route_flag_get(pOldRoute, ROUTE_FLAG_INTERNAL)) {
    route_dump(pStream, pOldRoute);
    fprintf(pStream, "\n");
  }

  // Build list of eligible routes received from peers
  pRoutes= routes_list_create(ROUTES_LIST_OPTION_REF);
  for (iIndex= 0; iIndex < ptr_array_length(pRouter->pPeers); iIndex++) {
    pPeer= (SPeer*) pRouter->pPeers->data[iIndex];
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
    pRoutes = rib_find_exact(pPeer->pAdjRIBIn, sPrefix);
    if (pRoutes != NULL) {
      for (uIndex = 0; uIndex < routes_list_get_num(pRoutes); uIndex++) {
	pRoute = routes_list_get_at(pRoutes, uIndex);
#else
    pRoute= rib_find_exact(pPeer->pAdjRIBIn, sPrefix);
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

  fprintf(pStream, "\n");

  // Start decision process
  if ((pOldRoute == NULL) ||
      !route_flag_get(pOldRoute, ROUTE_FLAG_INTERNAL)) {

    for (iRule= 0; iRule < DP_NUM_RULES; iRule++) {
      if (iNumRoutes <= 1)
	break;
      iOldNumRoutes= iNumRoutes;
      fprintf(pStream, "[ %s ]\n", DP_RULE_NAME[iRule]);
      DP_RULES[iRule](pRouter, pRoutes);
      iNumRoutes= ptr_array_length(pRoutes);
      if (iNumRoutes < iOldNumRoutes)
	routes_list_dump(pStream, pRoutes);
    }

  } else
    fprintf(pStream, "*** local route is preferred ***\n");

  fprintf(pStream, "\n");

  fprintf(pStream, "[ Best route ]\n");
  if (iNumRoutes > 0)
    routes_list_dump(pStream, pRoutes);
  else
    fprintf(pStream, "<none>\n");
  
  routes_list_destroy(&pRoutes);

}

