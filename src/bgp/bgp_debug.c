// ==================================================================
// @(#)bgp_debug.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 09/04/2004
// @lastdate 27/01/2005
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

  fprintf(pStream, "Debug Decision Process\n");
  fprintf(pStream, "----------------------\n");
  fprintf(pStream, "AS%u, ", pRouter->uNumber);
  ip_address_dump(pStream, pRouter->pNode->tAddr);
  fprintf(pStream, ", ");
  ip_prefix_dump(pStream, sPrefix);
  fprintf(pStream, "\n\n");

  pOldRoute= rib_find_exact(pRouter->pLocRIB, sPrefix);

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
  pRoutes= routes_list_create();
  for (iIndex= 0; iIndex < ptr_array_length(pRouter->pPeers); iIndex++) {
    pPeer= (SPeer*) pRouter->pPeers->data[iIndex];
    pRoute= rib_find_exact(pPeer->pAdjRIBIn, sPrefix);
    if ((pRoute != NULL) &&
	(route_flag_get(pRoute, ROUTE_FLAG_FEASIBLE)))
      routes_list_append(pRoutes, pRoute);
  }

  routes_list_dump(pStream, pRoutes);
  iNumRoutes= ptr_array_length(pRoutes);
  iOldNumRoutes= iNumRoutes;

  fprintf(pStream, "\n");

  // Start decision process
  if ((pOldRoute == NULL) ||
      !route_flag_get(pOldRoute, ROUTE_FLAG_INTERNAL)) {

    // *** higher LOCAL-PREF ***
    if (iNumRoutes > 1) {
      iOldNumRoutes= iNumRoutes;
      fprintf(pStream, "[ Higher LOCAL-PREF ]\n");
      as_decision_process_dop(pRouter, pRoutes);
      iNumRoutes= ptr_array_length(pRoutes);
      if (iNumRoutes < iOldNumRoutes)
	routes_list_dump(pStream, pRoutes);
    }

    // *** shortest AS-PATH ***
    if (iNumRoutes > 1) {
      iOldNumRoutes= iNumRoutes;
      fprintf(pStream, "[ Shortest AS-PATH ]\n");
      dp_rule_shortest_path(pRoutes);
      iNumRoutes= ptr_array_length(pRoutes);
      if (iNumRoutes < iOldNumRoutes)
	routes_list_dump(pStream, pRoutes);
    }

    // *** lowest ORIGIN ***
    if (iNumRoutes > 1) {
      iOldNumRoutes= iNumRoutes;
      fprintf(pStream, "[ Lowest ORIGIN ]\n");
      dp_rule_lowest_origin(pRoutes);
      iNumRoutes= ptr_array_length(pRoutes);
      if (iNumRoutes < iOldNumRoutes)
	routes_list_dump(pStream, pRoutes);
    }

    // *** lowest MED ***
    if (iNumRoutes > 1) {
      iOldNumRoutes= iNumRoutes;
      fprintf(pStream, "[ Lowest MED ]\n");
      dp_rule_lowest_med(pRoutes);
      iNumRoutes= ptr_array_length(pRoutes);
      if (iNumRoutes < iOldNumRoutes)
	routes_list_dump(pStream, pRoutes);
    }

    // *** eBGP over iBGP ***
    if (iNumRoutes > 1) {
      iOldNumRoutes= iNumRoutes;
      fprintf(pStream, "[ eBGP over iBGP ]\n");
      dp_rule_ebgp_over_ibgp(pRouter, pRoutes);
      iNumRoutes= ptr_array_length(pRoutes);
      if (iNumRoutes < iOldNumRoutes)
	routes_list_dump(pStream, pRoutes);
    }
      
    // *** nearest NEXT-HOP (lowest IGP-cost) ***
    if (iNumRoutes > 1) {
      iOldNumRoutes= iNumRoutes;
      fprintf(pStream, "[ Nearest NEXT-HOP ]\n");
      dp_rule_nearest_next_hop(pRouter, pRoutes);
      iNumRoutes= ptr_array_length(pRoutes);
      if (iNumRoutes < iOldNumRoutes)
	routes_list_dump(pStream, pRoutes);
    }
      
    // *** shortest cluster-ID-list ***
    if (iNumRoutes > 1) {
      iOldNumRoutes= iNumRoutes;
      fprintf(pStream, "[ Shortest CLUSTER-ID-LIST ]\n");
      dp_rule_shortest_cluster_list(pRouter, pRoutes);
      iNumRoutes= ptr_array_length(pRoutes);
      if (iNumRoutes < iOldNumRoutes)
	routes_list_dump(pStream, pRoutes);
    }
     
    // *** lowest neighbor address ***
    if (iNumRoutes > 1) {
      iOldNumRoutes= iNumRoutes;
      fprintf(pStream, "[ Lowest Neighbor Address ]\n");
      dp_rule_lowest_neighbor_address(pRouter, pRoutes);
      iNumRoutes= ptr_array_length(pRoutes);
      if (iNumRoutes < iOldNumRoutes)
	routes_list_dump(pStream, pRoutes);
    }
      
    if (iNumRoutes > 1) {
      fprintf(pStream, "[ Final Tie-break ]\n");
      dp_rule_final(pRouter, pRoutes);
      iNumRoutes= ptr_array_length(pRoutes);
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

