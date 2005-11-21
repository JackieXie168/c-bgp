// ==================================================================
// @(#)dp_rules.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 13/11/2002
// @lastdate 09/04/2004
// ==================================================================
// to-do: these routines can be optimized
// to-do: dp_rule_lowest_med()

#include <assert.h>
#include <libgds/log.h>

#include <bgp/as.h>
#include <bgp/dp_rules.h>
#include <bgp/peer.h>
#include <bgp/route.h>
#include <net/network.h>
#include <net/routing.h>

// ----- bgp_dp_rule_generic ----------------------------------------
/**
 * Optimized generic decision process rule. This function takes a
 * route comparison function as argument, process the best route and
 * eliminates all the routes that are not as best as the best route.
 *
 * PRE: fCompare must not be NULL
 */
static inline void bgp_dp_rule_generic(SBGPRouter * pRouter,
				   SRoutes * pRoutes,
				   FDPRouteCompare fCompare)
{
  int iIndex;
  int iResult;
  SRoute * pBestRoute= NULL;
  SRoute * pRoute;

  // Determine the best route. This loop already eliminates all the
  // routes that are not as best as the current best route.
  iIndex= 0;
  while (iIndex < routes_list_get_num(pRoutes)) {
    pRoute= (SRoute *) pRoutes->data[iIndex];
    if (pBestRoute != NULL) {
      iResult= fCompare(pRouter, pBestRoute, pRoute);
      if (iResult < 0) {
	pBestRoute= pRoute;
	iIndex++;
      } else if (iResult > 0) {
	routes_list_remove_at(pRoutes, iIndex);
      } else
	iIndex++;
    } else {
      pBestRoute= pRoute;
      iIndex++;
    }
  }

  // Eliminates remaining routes that are not as best as the best
  // route.
  iIndex= 0;
  while (iIndex < routes_list_get_num(pRoutes)) {
    if (fCompare(pRouter, (SRoute *) pRoutes->data[iIndex], pBestRoute) < 0)
      routes_list_remove_at(pRoutes, iIndex);
    else
      iIndex++;
  }
}

// ----- bgp_dp_rule_local_route_cmp --------------------------------
static int bgp_dp_rule_local_route_cmp(SBGPRouter * pRouter,
				       SRoute * pRoute1,
				       SRoute * pRoute2)
{
  // If both routes are INTERNAL, returns 0
  // If route1 is INTERNAL and route2 is not, returns 1
  // Otherwize, returns -1
  return route_flag_get(pRoute1, ROUTE_FLAG_INTERNAL)-
    route_flag_get(pRoute2, ROUTE_FLAG_INTERNAL);
}

// ----- bgp_dp_rule_local ------------------------------------------
/**
 * This function prefers routes that are locally originated (with the
 * as_add_network function).
 */
void bgp_dp_rule_local(SBGPRouter * pRouter, SRoutes * pRoutes)
{
  if (pRouter->uNumber == 1) {
    fprintf(stdout, "before:\n");
    routes_list_dump(stdout, pRoutes);
  }

  bgp_dp_rule_generic(pRouter, pRoutes,
		      bgp_dp_rule_local_route_cmp);

  if (pRouter->uNumber == 1) {
    fprintf(stdout, "after:\n");
    routes_list_dump(stdout, pRoutes);
  }
}

// ----- dp_rule_shortest_path --------------------------------------
/**
 *
 */
int dp_rule_shortest_path(SPtrArray * pRoutes)
{
  int iIndex;
  SRoute * pRoute;
  uint8_t uMinLen= 255;

  // Calculate shortest AS-Path length
  for (iIndex= 0; iIndex < ptr_array_length(pRoutes); iIndex++) {
    pRoute= (SRoute *) pRoutes->data[iIndex];
    if (route_path_length(pRoute) < uMinLen)
      uMinLen= route_path_length(pRoute);
  }
  // Exclude routes with a longer AS-Path
  iIndex= 0;
  while (iIndex < ptr_array_length(pRoutes)) {
    if (route_path_length((SRoute *) pRoutes->data[iIndex])
	> uMinLen)
      ptr_array_remove_at(pRoutes, iIndex);
    else
      iIndex++;
  }

  return 0;
}

// ----- dp_rule_lowest_origin --------------------------------------
/**
 *
 */
int dp_rule_lowest_origin(SPtrArray * pRoutes)
{
  int iIndex;
  SRoute * pRoute;
  origin_type_t uLowestOrigin= ROUTE_ORIGIN_INCOMPLETE;

  // Calculate lowest Origin
  for (iIndex= 0; iIndex < ptr_array_length(pRoutes); iIndex++) {
    pRoute= (SRoute *) pRoutes->data[iIndex];
    if (route_origin_get(pRoute) < uLowestOrigin)
      uLowestOrigin= route_origin_get(pRoute);
  }
  // Exclude routes with a greater Origin
  iIndex= 0;
  while (iIndex < ptr_array_length(pRoutes)) {
    if (route_origin_get((SRoute *) pRoutes->data[iIndex])
	> uLowestOrigin)
      ptr_array_remove_at(pRoutes, iIndex);
    else
      iIndex++;
  }

  return 0;
}

// ----- dp_rule_lowest_med -----------------------------------------
/**
 *
 */
int dp_rule_lowest_med(SPtrArray * pRoutes)
{
  /*
  int iIndex;
  SRoute * pRoute;
  uint32_t uMinMED= UINT_MAX;
  */

  // NOT YET IMPLEMENTED
  // WARNING: MED can only be compared between routes from the same AS
  // !!!

  /*
  // Calculate lowest MED
  for (iIndex= 0; iIndex < ptr_array_length(pRoutes); iIndex++) {
    pRoute= (SRoute *) pRoutes->data[iIndex];
    if (route_med_get(pRoute) < uMinMED)
      uMinMED= route_med_get(pRoute);
  }
  // Discard routes with an higher MED
  iIndex= 0;
  while (iIndex < ptr_array_length(pRoutes)) {
    if (route_med_get((SRoute *) pRoutes->data[iIndex]) > uMinMED)
      ptr_array_remove_at(pRoutes, iIndex);
    else
      iIndex++;
  }
  */

  return 0;
}

// ----- dp_rule_ebgp_over_ibgp -------------------------------------
/**
 *
 */
int dp_rule_ebgp_over_ibgp(SAS * pAS, SPtrArray * pRoutes)
{
  int iIndex;
  int iEBGP= 0;

  // Detect if there is at least one route learned through an eBGP
  // session:
  for (iIndex= 0; iIndex < ptr_array_length(pRoutes); iIndex++) {
    if (((SRoute *) pRoutes->data[iIndex])->pPeer->uRemoteAS != pAS->uNumber) {
      iEBGP= 1;
      break;
    }
  }
  // If there is at least one route learned over eBGP, discard all the
  // routes learned over an iBGP session:
  if (iEBGP) {
    iIndex= 0;
    while (iIndex < ptr_array_length(pRoutes)) {
      if (((SRoute *) pRoutes->data[iIndex])->pPeer->uRemoteAS == pAS->uNumber)
	ptr_array_remove_at(pRoutes, iIndex);
      else
	iIndex++;
    }
  }

  return 0;
}

// ----- dp_rule_igp_cost -------------------------------------------
/**
 * Helper function which retrieves the IGP cost to the given next-hop
 *
 * WARNING: calling this function can be time-consuming since we have
 * to lookup the peer's table => O(log(n))
 */
uint32_t dp_rule_igp_cost(SAS * pAS, net_addr_t tNextHop)
{
  SNetLink * pLink;
  SNetRouteInfo * pRouteInfo;

  /* Is there a direct link ? */
  pLink= node_links_lookup(pAS->pNode, tNextHop);
  if (pLink != NULL)
    return pLink->uIGPweight;
  
  /* Is there a route towards the destination ? */
  pRouteInfo= rt_find_best(pAS->pNode->pRT, tNextHop);
  if (pRouteInfo != NULL)
    return pRouteInfo->uWeight;


  LOG_FATAL("Error: unable to compute IGP cost to next-hop (");
  LOG_ENABLED_FATAL()
    ip_address_dump(log_get_stream(pMainLog), tNextHop);
  LOG_FATAL(")\n");
  LOG_FATAL("Error: AS%d", pAS->uNumber);
  LOG_FATAL("\n");
  abort();
  return -1;
}

// ----- dp_rule_nearest_next_hop -----------------------------------
/**
 *
 */
int dp_rule_nearest_next_hop(SAS * pAS, SPtrArray * pRoutes)
{
  int iIndex;
  SRoute * pRoute;
  uint32_t uLowestCost= UINT_MAX;
  uint32_t uIGPcost;

  // Calculate lowest IGP-cost
  for (iIndex= 0; iIndex < ptr_array_length(pRoutes); iIndex++) {
    pRoute= (SRoute *) pRoutes->data[iIndex];
    uIGPcost= dp_rule_igp_cost(pAS, pRoute->tNextHop);
    if (uIGPcost < uLowestCost)
      uLowestCost= uIGPcost;
  }
  // Discard routes with an higher IGP-cost 
  iIndex= 0;
  while (iIndex < ptr_array_length(pRoutes)) {
    pRoute= (SRoute *) pRoutes->data[iIndex];
    if (dp_rule_igp_cost(pAS, pRoute->tNextHop) > uLowestCost)
      ptr_array_remove_at(pRoutes, iIndex);
    else
      iIndex++;
  }

  return 0;
}

// ----- dp_rule_shortest_cluster_list ------------------------------
/**
 *
 */
int dp_rule_shortest_cluster_list(SAS * pAS, SPtrArray * pRoutes)
{
  SRoute * pRoute;
  int iIndex;
  uint8_t uMinLen= 255;

  // Calculate shortest cluster-ID-list
  for (iIndex= 0; iIndex < ptr_array_length(pRoutes); iIndex++) {
    pRoute= (SRoute *) pRoutes->data[iIndex];
    if (pRoute->pClusterList == NULL)
      uMinLen= 0;
    else
      uMinLen= cluster_list_length(pRoute->pClusterList);    
  }
  // Discard routes with a longer cluster-ID-list
  iIndex= 0;
  while (iIndex < ptr_array_length(pRoutes)) {
    pRoute= (SRoute *) pRoutes->data[iIndex];
    if ((pRoute->pClusterList != NULL) &&
	(cluster_list_length(pRoute->pClusterList) > uMinLen))
      ptr_array_remove_at(pRoutes, iIndex);
    else
      iIndex++;
  }

  return 0;
}

// ----- dp_rule_lowest_neighbor_address ----------------------------
/**
 *
 */
int dp_rule_lowest_neighbor_address(SAS * pAS, SPtrArray * pRoutes)
{
  net_addr_t tLowestAddr= MAX_ADDR;
  SRoute * pRoute;
  int iIndex;

  // Calculate lowest neighbor address
  for (iIndex= 0; iIndex < ptr_array_length(pRoutes); iIndex++) {
    pRoute= (SRoute *) pRoutes->data[iIndex];
    if (route_peer_get(pRoute)->tAddr < tLowestAddr)
      tLowestAddr= route_peer_get(pRoute)->tAddr;
  }
  // Discard routes from neighbors with an higher address
  iIndex= 0;
  while (iIndex < ptr_array_length(pRoutes)) {
    pRoute= (SRoute *) pRoutes->data[iIndex];
    if (route_peer_get(pRoute)->tAddr > tLowestAddr)
      ptr_array_remove_at(pRoutes, iIndex);
    else
      iIndex++;
  }

  return 0;
}

// ----- dp_rule_final ----------------------------------------------
/**
 *
 */
int dp_rule_final(SAS * pAS, SPtrArray * pRoutes)
{
  int iResult;

  // *** final tie-break ***
  while (ptr_array_length(pRoutes) > 1) {
    iResult= pAS->fTieBreak((SRoute *) pRoutes->data[0],
			    (SRoute *) pRoutes->data[1]);
    if (iResult == 1) // Prefer ROUTE1
      ptr_array_remove_at(pRoutes, 1);
    else if (iResult == -1) // Prefer ROUTE2
      ptr_array_remove_at(pRoutes, 0);
    else {
      LOG_FATAL("Error: final tie-break can not decide\n");
      abort(); // Can not decide !!!
    }
  }
  return 0;
}
