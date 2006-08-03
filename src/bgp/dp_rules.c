// ==================================================================
// @(#)dp_rules.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 13/11/2002
// @lastdate 10/04/2006
// ==================================================================
// to-do: these routines could be optimized

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <libgds/log.h>

#include <bgp/as.h>
#include <bgp/dp_rules.h>
#include <bgp/peer.h>
#include <bgp/route.h>
#include <net/network.h>
#include <net/routing.h>

// -----[ function prototypes ]--------------------------------------
static uint32_t _dp_rule_igp_cost(SBGPRouter * pRouter, net_addr_t tNextHop);



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
 * This function prefers routes that are locally originated,
 * i.e. inserted with the bgp_router_add_network() function.
 */
void bgp_dp_rule_local(SBGPRouter * pRouter, SRoutes * pRoutes)
{
  if (pRouter->uNumber == 1) {
    log_printf(pLogOut, "before:\n");
    routes_list_dump(pLogOut, pRoutes);
  }

  bgp_dp_rule_generic(pRouter, pRoutes,
		      bgp_dp_rule_local_route_cmp);

  if (pRouter->uNumber == 1) {
    log_printf(pLogOut, "after:\n");
    routes_list_dump(pLogOut, pRoutes);
  }
}

#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
SIntArray * dp_rule_nexthop_counter_create(){
  return int_array_create(ARRAY_OPTION_UNIQUE | ARRAY_OPTION_SORTED);
}

int dp_rule_nexthop_add(SIntArray * piNextHop, net_addr_t tNextHop)
{
  return int_array_add(piNextHop, &tNextHop);
}

int dp_rule_nexthop_get_count(SIntArray * piNextHop)
{
  return int_array_length(piNextHop);
}

void dp_rule_nexthop_destroy(SIntArray ** ppiNextHop)
{
  int_array_destroy(ppiNextHop);
}

int dp_rule_no_selection(SBGPRouter * pRouter, SRoutes * pRoutes)
{
  SIntArray * piNextHopCounter = dp_rule_nexthop_counter_create();
  int iIndex;
  int iNextHopCount;

  for (iIndex = 0; iIndex < routes_list_get_num(pRoutes); iIndex++) {
    dp_rule_nexthop_add(piNextHopCounter, route_get_nexthop(routes_list_get_at(pRoutes, iIndex)));
  }
    
  iNextHopCount = dp_rule_nexthop_get_count(piNextHopCounter);
  dp_rule_nexthop_destroy(&piNextHopCounter);
  return iNextHopCount;

}

int dp_rule_lowest_nh(SBGPRouter * pRouter, SRoutes * pRoutes)
{
  net_addr_t tLowestNextHop= MAX_ADDR;
  net_addr_t tNH;
  SRoute * pRoute;
  int iIndex;
  SIntArray * piNextHopCounter = dp_rule_nexthop_counter_create();
  int iNextHopCount;

  // Calculate lowest NextHop
  for (iIndex= 0; iIndex < ptr_array_length(pRoutes); iIndex++) {
    pRoute= (SRoute *) pRoutes->data[iIndex]; 
    tNH= route_get_nexthop(pRoute); 
    if (tNH < tLowestNextHop)
      tLowestNextHop= tNH;
  }

  // Discard routes from neighbors with an higher NextHop
  iIndex= 0;
  while (iIndex < ptr_array_length(pRoutes)) {
    pRoute= (SRoute *) pRoutes->data[iIndex];
    tNH= route_get_nexthop(pRoute); 
    if (tNH > tLowestNextHop)
      ptr_array_remove_at(pRoutes, iIndex);
    else
    {
      dp_rule_nexthop_add(piNextHopCounter, route_get_nexthop((routes_list_get_at(pRoutes, iIndex))));
      iIndex++;
    }
  }

  iNextHopCount = dp_rule_nexthop_get_count(piNextHopCounter);
  dp_rule_nexthop_destroy(&piNextHopCounter);
  return iNextHopCount;
}
#endif
// ----- dp_rule_highest_pref ---------------------------------------
/**
 * Remove routes that do not have the highest LOCAL-PREF.
 */
int dp_rule_highest_pref(SBGPRouter * pRouter, SRoutes * pRoutes)
{
  int iIndex;
  SRoute * pRoute;
  uint32_t uHighestPref= 0;

#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  SIntArray * piNextHopCounter = dp_rule_nexthop_counter_create();
  int iNextHopCount;
#endif
  
  // Calculate highest degree of preference
  for (iIndex= 0; iIndex < routes_list_get_num(pRoutes); iIndex++) {
    pRoute= (SRoute *) pRoutes->data[iIndex];
    if (route_localpref_get(pRoute) > uHighestPref)
      uHighestPref= route_localpref_get(pRoute);
  }
  // Exclude routes with a lower preference
  iIndex= 0;
  while (iIndex < routes_list_get_num(pRoutes)) {
    if (route_localpref_get((SRoute *) pRoutes->data[iIndex])
	< uHighestPref)
      ptr_array_remove_at(pRoutes, iIndex);
    else
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
    {
      dp_rule_nexthop_add(piNextHopCounter, route_get_nexthop(routes_list_get_at(pRoutes, iIndex)));
      iIndex++;
    }
#else
      iIndex++;
#endif
  }

#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  iNextHopCount = dp_rule_nexthop_get_count(piNextHopCounter);
  dp_rule_nexthop_destroy(&piNextHopCounter);
  return iNextHopCount;
#else
  return 0;
#endif
}

// ----- dp_rule_shortest_path --------------------------------------
/**
 * Remove routes that do not have the shortest AS-PATH.
 */
int dp_rule_shortest_path(SBGPRouter * pRouter, SRoutes * pRoutes)
{
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  SIntArray * piNextHopCounter = dp_rule_nexthop_counter_create();
  int iNextHopCount;
#endif

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
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
    {
      dp_rule_nexthop_add(piNextHopCounter, route_get_nexthop(routes_list_get_at(pRoutes, iIndex)));
      iIndex++;
    }
#else
      iIndex++;
#endif
  }
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  iNextHopCount = dp_rule_nexthop_get_count(piNextHopCounter);
  dp_rule_nexthop_destroy(&piNextHopCounter);
  return iNextHopCount;
#else
  return 0;
#endif
}

// ----- dp_rule_lowest_origin --------------------------------------
/**
 * Remove routes that do not have the lowest ORIGIN (IGP < EGP <
 * INCOMPLETE).
 */
int dp_rule_lowest_origin(SBGPRouter * pRouter, SRoutes * pRoutes)
{
  int iIndex;
  SRoute * pRoute;
  bgp_origin_t tLowestOrigin= ROUTE_ORIGIN_INCOMPLETE;

#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  SIntArray * piNextHopCounter = dp_rule_nexthop_counter_create();
  int iNextHopCount;
#endif

  // Calculate lowest Origin
  for (iIndex= 0; iIndex < ptr_array_length(pRoutes); iIndex++) {
    pRoute= (SRoute *) pRoutes->data[iIndex];
    if (route_get_origin(pRoute) < tLowestOrigin)
      tLowestOrigin= route_get_origin(pRoute);
  }
  // Exclude routes with a greater Origin
  iIndex= 0;
  while (iIndex < ptr_array_length(pRoutes)) {
    if (route_get_origin((SRoute *) pRoutes->data[iIndex])
	> tLowestOrigin)
      ptr_array_remove_at(pRoutes, iIndex);
    else
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
    {
      dp_rule_nexthop_add(piNextHopCounter, route_get_nexthop(routes_list_get_at(pRoutes, iIndex)));
      iIndex++;
    }
#else
      iIndex++;
#endif
  }
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  iNextHopCount = dp_rule_nexthop_get_count(piNextHopCounter);
  dp_rule_nexthop_destroy(&piNextHopCounter);
  return iNextHopCount;
#else
  return 0;
#endif
}

// ----- dp_rule_lowest_med -----------------------------------------
/**
 * Remove routes that do not have the lowest MED.
 *
 * Note: MED comparison can be done in two different ways. With the
 * deterministic way, MED values are only compared between routes
 * received from teh same neighbor AS. With the always-compare way,
 * MED can always be compared.
 */
int dp_rule_lowest_med(SBGPRouter * pRouter, SRoutes * pRoutes)
{
  int iIndex, iIndex2;
  int iLastAS;
  SRoute * pRoute, * pRoute2;
  uint32_t uMinMED= UINT_MAX;
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  SIntArray * piNextHopCounter = dp_rule_nexthop_counter_create();
  int iNextHopCount;
#endif

  // WARNING: MED can only be compared between routes from the same AS
  // !!!
  // This constraint may be changed by two options :
  //   1) with the option always-compare-med. 
  //     In this case, the med is always compared between two routes even if
  //     they're not announced by the sames AS.
  //   2) the deterministic MED Cisco option.
  //     The router has to group each route by AS. Secondly, it has to
  //     elect the best route of each group and then continue to apply
  //     the remaining rules of the decision process to these best
  //     routes.
  //
  // WARNING : if the two options are set, the MED of the best routes of each
  // group has to be compared too.

  switch (BGP_OPTIONS_MED_TYPE) {
  case BGP_MED_TYPE_ALWAYS_COMPARE:
   
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
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
    {
      dp_rule_nexthop_add(piNextHopCounter, route_get_nexthop(routes_list_get_at(pRoutes, iIndex)));
      iIndex++;
    }
#else
	iIndex++;
#endif
    }
    break;

  case BGP_MED_TYPE_DETERMINISTIC:

    iIndex= 0;
    while (iIndex < ptr_array_length(pRoutes)) {
      pRoute= (SRoute *) pRoutes->data[iIndex];
      
      if (pRoute != NULL) {
      
	iLastAS= route_path_last_as(pRoute);
	
	iIndex2= iIndex+1;
	while (iIndex2 < ptr_array_length(pRoutes)) {
	  pRoute2= (SRoute *) pRoutes->data[iIndex2];
	  
	  if (pRoute2 != NULL) {
	  
	    /*printf("route : ");
	      route_dump(stdout, pRoute);
	      printf("\nroute2: ");
	      route_dump(stdout, pRoute2);
	      printf("\n");*/
	    
	    if (iLastAS == route_path_last_as(pRoute2)) {
	      
	      //printf("compare...[%u <-> %u]\n", pRoute->pAttr->uMED, pRoute2->pAttr->uMED);
	      
	      if (pRoute->pAttr->uMED < pRoute2->pAttr->uMED) {
		//printf("route < route2\n");
		pRoutes->data[iIndex2]= NULL;
	      } else if (pRoute->pAttr->uMED > pRoute2->pAttr->uMED) {
		//printf("route2 < route\n");
		pRoutes->data[iIndex]= NULL;
		break;
	      }
	      
	    }
	  }
	  
	  iIndex2++;
	}
      }
      iIndex++;
    }
    iIndex= 0;
    while (iIndex < ptr_array_length(pRoutes)) {
      if (pRoutes->data[iIndex] == NULL) {
	ptr_array_remove_at(pRoutes, iIndex);
      } else
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
    {
      dp_rule_nexthop_add(piNextHopCounter, route_get_nexthop(routes_list_get_at(pRoutes, iIndex)));
      iIndex++;
    }
#else
	iIndex++;
#endif
    }
    
  }
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  iNextHopCount = dp_rule_nexthop_get_count(piNextHopCounter);
  dp_rule_nexthop_destroy(&piNextHopCounter);
  return iNextHopCount;
#else
  return 0;
#endif
}

// ----- dp_rule_ebgp_over_ibgp -------------------------------------
/**
 * If there is an eBGP route, remove all the iBGP routes.
 */
int dp_rule_ebgp_over_ibgp(SBGPRouter * pRouter, SRoutes * pRoutes)
{
  int iIndex;
  int iEBGP= 0;
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  SIntArray * piNextHopCounter = dp_rule_nexthop_counter_create();
  int iNextHopCount;
#endif

  // Detect if there is at least one route learned through an eBGP
  // session:
  for (iIndex= 0; iIndex < ptr_array_length(pRoutes); iIndex++) {
    if (((SRoute *) pRoutes->data[iIndex])->pPeer->uRemoteAS != pRouter->uNumber) {
      iEBGP= 1;
      break;
    }
  }
  // If there is at least one route learned over eBGP, discard all the
  // routes learned over an iBGP session:
  if (iEBGP) {
    iIndex= 0;
    while (iIndex < ptr_array_length(pRoutes)) {
      if (((SRoute *) pRoutes->data[iIndex])->pPeer->uRemoteAS == pRouter->uNumber)
	ptr_array_remove_at(pRoutes, iIndex);
      else
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
    {
      dp_rule_nexthop_add(piNextHopCounter, route_get_nexthop(routes_list_get_at(pRoutes, iIndex)));
      iIndex++;
    }
#else
	iIndex++;
#endif
    }
  }
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  iNextHopCount = dp_rule_nexthop_get_count(piNextHopCounter);
  dp_rule_nexthop_destroy(&piNextHopCounter);
  return iNextHopCount;
#else
  return 0;
#endif
}

// ----- _dp_rule_igp_cost ------------------------------------------
/**
 * Helper function which retrieves the IGP cost to the given next-hop.
 */
static uint32_t _dp_rule_igp_cost(SBGPRouter * pRouter, net_addr_t tNextHop)
{
  SNetRouteInfo * pRouteInfo;

  /* Is there a route towards the destination ? */
  pRouteInfo= rt_find_best(pRouter->pNode->pRT, tNextHop, NET_ROUTE_ANY);
  if (pRouteInfo != NULL)
    return pRouteInfo->uWeight;

  LOG_ERR_ENABLED(LOG_LEVEL_FATAL) {
    log_printf(pLogErr, "Error: unable to compute IGP cost to next-hop (");
    ip_address_dump(pLogErr, tNextHop);
    log_printf(pLogErr, ")\nError: ");
    ip_address_dump(pLogErr, pRouter->pNode->tAddr);
    log_printf(pLogErr, "\n");
  }
  abort();
  return -1;
}

// ----- dp_rule_nearest_next_hop -----------------------------------
/**
 * Remove the routes that do not have the lowest IGP cost to the BGP
 * next-hop.
 */
int dp_rule_nearest_next_hop(SBGPRouter * pRouter, SRoutes * pRoutes)
{
  int iIndex;
  SRoute * pRoute;
  uint32_t uLowestCost= UINT_MAX;
  uint32_t uIGPcost;
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  SIntArray * piNextHopCounter = dp_rule_nexthop_counter_create();
  int iNextHopCount;
#endif

  // Calculate lowest IGP-cost
  for (iIndex= 0; iIndex < ptr_array_length(pRoutes); iIndex++) {
    pRoute= (SRoute *) pRoutes->data[iIndex];

    uIGPcost= _dp_rule_igp_cost(pRouter, pRoute->pAttr->tNextHop);
    if (uIGPcost < uLowestCost)
      uLowestCost= uIGPcost;

    /* Mark route as depending on the IGP weight. This is done in
       order to more efficiently react to IGP changes. See
       'bgp_router_scan_rib' for more information. */
    route_flag_set(pRoute, ROUTE_FLAG_DP_IGP, 1);

  }

  // Discard routes with an higher IGP-cost 
  iIndex= 0;
  while (iIndex < ptr_array_length(pRoutes)) {
    pRoute= (SRoute *) pRoutes->data[iIndex];
    uIGPcost= _dp_rule_igp_cost(pRouter, pRoute->pAttr->tNextHop);
    if (uIGPcost > uLowestCost) {
      ptr_array_remove_at(pRoutes, iIndex);
    } else
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
    {
      dp_rule_nexthop_add(piNextHopCounter, route_get_nexthop(routes_list_get_at(pRoutes, iIndex)));
      iIndex++;
    }
#else
      iIndex++;
#endif
  }
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  iNextHopCount = dp_rule_nexthop_get_count(piNextHopCounter);
  dp_rule_nexthop_destroy(&piNextHopCounter);
  return iNextHopCount;
#else
  return 0;
#endif
}

// ----- dp_rule_lowest_router_id -----------------------------------
/**
 * Remove routes that do not have the lowest ROUTER-ID (or
 * ORIGINATOR-ID if the route is reflected).
 *
 * TODO: support ORIGINATOR-ID comparison...
 */
int dp_rule_lowest_router_id(SBGPRouter * pRouter, SRoutes * pRoutes)
{
  net_addr_t tLowestRouterID= MAX_ADDR;
  net_addr_t tID;
  SRoute * pRoute;
  int iIndex;
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  SIntArray * piNextHopCounter = dp_rule_nexthop_counter_create();
  int iNextHopCount;
#endif

  // Calculate lowest ROUTER-ID (or ORIGINATOR-ID)
  for (iIndex= 0; iIndex < ptr_array_length(pRoutes); iIndex++) {
    pRoute= (SRoute *) pRoutes->data[iIndex];
    tID= route_peer_get(pRoute)->tRouterID; 
    if (tID < tLowestRouterID)
      tLowestRouterID= tID;
  }
  // Discard routes from neighbors with an higher ROUTER-ID (or
  // ORIGINATOR-ID)
  iIndex= 0;
  while (iIndex < ptr_array_length(pRoutes)) {
    pRoute= (SRoute *) pRoutes->data[iIndex];
    tID= route_peer_get(pRoute)->tRouterID; 
    if (tID > tLowestRouterID)
      ptr_array_remove_at(pRoutes, iIndex);
    else
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
    {
      dp_rule_nexthop_add(piNextHopCounter, route_get_nexthop(routes_list_get_at(pRoutes, iIndex)));
      iIndex++;
    }
#else
      iIndex++;
#endif
  }
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  iNextHopCount = dp_rule_nexthop_get_count(piNextHopCounter);
  dp_rule_nexthop_destroy(&piNextHopCounter);
  return iNextHopCount;
#else
  return 0;
#endif
}

// ----- dp_rule_shortest_cluster_list ------------------------------
/**
 * Remove the routes that do not have the shortest CLUSTER-ID-LIST.
 */
int dp_rule_shortest_cluster_list(SBGPRouter * pRouter, SRoutes * pRoutes)
{
  SRoute * pRoute;
  int iIndex;
  uint8_t uMinLen= 255;
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  SIntArray * piNextHopCounter = dp_rule_nexthop_counter_create();
  int iNextHopCount;
#endif

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
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
    {
      dp_rule_nexthop_add(piNextHopCounter, route_get_nexthop(routes_list_get_at(pRoutes, iIndex)));
      iIndex++;
    }
#else
      iIndex++;
#endif
  }
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  iNextHopCount = dp_rule_nexthop_get_count(piNextHopCounter);
  dp_rule_nexthop_destroy(&piNextHopCounter);
  return iNextHopCount;
#else
  return 0;
#endif
}

// ----- dp_rule_lowest_neighbor_address ----------------------------
/**
 * Remove routes that are not learned through the peer with the lowest
 * IP address.
 */
int dp_rule_lowest_neighbor_address(SBGPRouter * pRouter, SRoutes * pRoutes)
{
  net_addr_t tLowestAddr= MAX_ADDR;
  SRoute * pRoute;
  int iIndex;
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  SIntArray * piNextHopCounter = dp_rule_nexthop_counter_create();
  int iNextHopCount;
#endif

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
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
    {
      dp_rule_nexthop_add(piNextHopCounter, route_get_nexthop(routes_list_get_at(pRoutes, iIndex)));
      iIndex++;
    }
#else
      iIndex++;
#endif
  }
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  iNextHopCount = dp_rule_nexthop_get_count(piNextHopCounter);
  dp_rule_nexthop_destroy(&piNextHopCounter);
  return iNextHopCount;
#else
  return 0;
#endif
}

// ----- dp_rule_final ----------------------------------------------
/**
 *
 */
int dp_rule_final(SBGPRouter * pRouter, SRoutes * pRoutes)
{
  int iResult;

  // *** final tie-break ***
  while (ptr_array_length(pRoutes) > 1) {
    iResult= pRouter->fTieBreak((SRoute *) pRoutes->data[0],
			    (SRoute *) pRoutes->data[1]);
    if (iResult == 1) // Prefer ROUTE1
      ptr_array_remove_at(pRoutes, 1);
    else if (iResult == -1) // Prefer ROUTE2
      ptr_array_remove_at(pRoutes, 0);
    else {
      LOG_ERR(LOG_LEVEL_FATAL, "Error: final tie-break can not decide\n");
      abort(); // Can not decide !!!
    }
  }
  return 0;
}
