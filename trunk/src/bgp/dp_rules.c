// ==================================================================
// @(#)dp_rules.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 13/11/2002
// $Id: dp_rules.c,v 1.19 2008-04-07 09:20:14 bqu Exp $
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

// -----[ decision process ]-----------------------------------------
struct dp_rule_t DP_RULES[DP_NUM_RULES]= {
  { "Highest LOCAL-PREF", dp_rule_highest_pref },
  { "Shortest AS-PATH", dp_rule_shortest_path },
  { "Lowest ORIGIN", dp_rule_lowest_origin },
  { "Lowest MED", dp_rule_lowest_med },
  { "eBGP over iBGP", dp_rule_ebgp_over_ibgp },
  { "Nearest NEXT-HOP", dp_rule_nearest_next_hop },
  { "Lowest ROUTER-ID", dp_rule_lowest_router_id },
  { "Shortest CLUSTER-ID-LIST", dp_rule_shortest_cluster_list },
  { "Lowest neighbor address", dp_rule_lowest_neighbor_address },
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  { "Lowest Nexthop", dp_rule_lowest_nh },
  { "Lowest Path", dp_rule_lowest_path },
#endif
};

// -----[ function prototypes ]--------------------------------------
static uint32_t _dp_rule_igp_cost(bgp_router_t * pRouter, net_addr_t tNextHop);



// ----- bgp_dp_rule_generic ----------------------------------------
/**
 * Optimized generic decision process rule. This function takes a
 * route comparison function as argument, process the best route and
 * eliminates all the routes that are not as best as the best route.
 *
 * PRE: fCompare must not be NULL
 */
static inline void bgp_dp_rule_generic(bgp_router_t * pRouter,
				   bgp_routes_t * pRoutes,
				   FDPRouteCompare fCompare)
{
  unsigned int index;
  int iResult;
  bgp_route_t * pBestRoute= NULL;
  bgp_route_t * pRoute;

  // Determine the best route. This loop already eliminates all the
  // routes that are not as best as the current best route.
  index= 0;
  while (index < bgp_routes_size(pRoutes)) {
    pRoute= bgp_routes_at(pRoutes, index);
    if (pBestRoute != NULL) {
      iResult= fCompare(pRouter, pBestRoute, pRoute);
      if (iResult < 0) {
	pBestRoute= pRoute;
	index++;
      } else if (iResult > 0) {
	routes_list_remove_at(pRoutes, index);
      } else
	index++;
    } else {
      pBestRoute= pRoute;
      index++;
    }
  }

  // Eliminates remaining routes that are not as best as the best
  // route.
  index= 0;
  while (index < bgp_routes_size(pRoutes)) {
    if (fCompare(pRouter, bgp_routes_at(pRoutes, index), pBestRoute) < 0)
      routes_list_remove_at(pRoutes, index);
    else
      index++;
  }
}

// ----- bgp_dp_rule_local_route_cmp --------------------------------
static int bgp_dp_rule_local_route_cmp(bgp_router_t * pRouter,
				       bgp_route_t * pRoute1,
				       bgp_route_t * pRoute2)
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
void bgp_dp_rule_local(bgp_router_t * pRouter, bgp_routes_t * pRoutes)
{
  if (pRouter->uASN == 1) {
    log_printf(pLogOut, "before:\n");
    routes_list_dump(pLogOut, pRoutes);
  }

  bgp_dp_rule_generic(pRouter, pRoutes,
		      bgp_dp_rule_local_route_cmp);

  if (pRouter->uASN == 1) {
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

int dp_rule_no_selection(bgp_router_t * pRouter, bgp_routes_t * pRoutes)
{
  SIntArray * piNextHopCounter = dp_rule_nexthop_counter_create();
  unsigned int index;
  int iNextHopCount;

  for (index = 0; index < bgp_routes_size(pRoutes); index++) {
    dp_rule_nexthop_add(piNextHopCounter,
			route_get_nexthop(bgp_routes_at(pRoutes, index)));
  }
    
  iNextHopCount = dp_rule_nexthop_get_count(piNextHopCounter);
  dp_rule_nexthop_destroy(&piNextHopCounter);
  return iNextHopCount;

}

int dp_rule_lowest_nh(bgp_router_t * pRouter, bgp_routes_t * pRoutes)
{
  net_addr_t tLowestNextHop= MAX_ADDR;
  net_addr_t tNH;
  bgp_route_t * pRoute;
  unsigned int index;
  SIntArray * piNextHopCounter = dp_rule_nexthop_counter_create();
  int iNextHopCount;

  // Calculate lowest NextHop
  for (index= 0; index < bgp_routes_size(pRoutes); index++) {
    pRoute= bgp_routes_at(pRoutes, index); 
    tNH= route_get_nexthop(pRoute); 
    if (tNH < tLowestNextHop)
      tLowestNextHop= tNH;
  }

  // Discard routes from neighbors with an higher NextHop
  index= 0;
  while (index < bgp_routes_size(pRoutes)) {
    pRoute= bgp_routes_at(pRoutes, index);
    tNH= route_get_nexthop(pRoute); 
    if (tNH > tLowestNextHop)
      ptr_array_remove_at(pRoutes, index);
    else
    {
      dp_rule_nexthop_add(piNextHopCounter, route_get_nexthop(bgp_routes_at(pRoutes, index)));
      index++;
    }
  }

  iNextHopCount = dp_rule_nexthop_get_count(piNextHopCounter);
  dp_rule_nexthop_destroy(&piNextHopCounter);
  return iNextHopCount;
}

int dp_rule_lowest_path(bgp_router_t * pRouter, bgp_routes_t * pRoutes)
{ 
  unsigned int index;
  bgp_route_t * pRoute;
  SBGPPath * tPath;
  SBGPPath * tLowestPath;
  SBGPPath * tPathMax;
  SIntArray * piNextHopCounter = dp_rule_nexthop_counter_create();
  int iNextHopCount;
  bgp_route_t * pSavedRoute= NULL;
    
  tPathMax = path_max_value();
  tLowestPath = tPathMax;
  
  // Calculate lowest AS-PATH
  for (index = 0; index < bgp_routes_size(pRoutes); index++) {
    pRoute = bgp_routes_at(pRoutes, index);
    tPath = route_get_path(pRoute); 
    if (path_comparison(tPath, tLowestPath) <= 0) {
      tLowestPath = tPath;
      pSavedRoute = pRoute;
    }
  } 

  assert(pSavedRoute != NULL);
    
  dp_rule_nexthop_add(piNextHopCounter, route_get_nexthop(pSavedRoute));
  path_destroy(&tPathMax);
  
  iNextHopCount = dp_rule_nexthop_get_count(piNextHopCounter);
  dp_rule_nexthop_destroy(&piNextHopCounter);
  return iNextHopCount;
}

#endif
// ----- dp_rule_highest_pref ---------------------------------------
/**
 * Remove routes that do not have the highest LOCAL-PREF.
 */
int dp_rule_highest_pref(bgp_router_t * pRouter, bgp_routes_t * pRoutes)
{
  unsigned int index;
  bgp_route_t * pRoute;
  uint32_t uHighestPref= 0;

#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  SIntArray * piNextHopCounter = dp_rule_nexthop_counter_create();
  int iNextHopCount;
#endif
  
  // Calculate highest degree of preference
  for (index= 0; index < bgp_routes_size(pRoutes); index++) {
    pRoute= bgp_routes_at(pRoutes, index);
    if (route_localpref_get(pRoute) > uHighestPref)
      uHighestPref= route_localpref_get(pRoute);
  }
  // Exclude routes with a lower preference
  index= 0;
  while (index < bgp_routes_size(pRoutes)) {
    if (route_localpref_get(bgp_routes_at(pRoutes, index))
	< uHighestPref)
      ptr_array_remove_at(pRoutes, index);
    else
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
    {
      dp_rule_nexthop_add(piNextHopCounter, route_get_nexthop(bgp_routes_at(pRoutes, index)));
      index++;
    }
#else
      index++;
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
int dp_rule_shortest_path(bgp_router_t * pRouter, bgp_routes_t * pRoutes)
{
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  SIntArray * piNextHopCounter = dp_rule_nexthop_counter_create();
  int iNextHopCount;
#endif

  unsigned int index;
  bgp_route_t * pRoute;
  uint8_t uMinLen= 255;

  // Calculate shortest AS-Path length
  for (index= 0; index < bgp_routes_size(pRoutes); index++) {
    pRoute= bgp_routes_at(pRoutes, index);
    if (route_path_length(pRoute) < uMinLen)
      uMinLen= route_path_length(pRoute);
  }
  // Exclude routes with a longer AS-Path
  index= 0;
  while (index < bgp_routes_size(pRoutes)) {
    if (route_path_length(bgp_routes_at(pRoutes, index)) > uMinLen)
      ptr_array_remove_at(pRoutes, index);
    else
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
    {
      dp_rule_nexthop_add(piNextHopCounter, route_get_nexthop(bgp_routes_at(pRoutes, index)));
      index++;
    }
#else
      index++;
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
int dp_rule_lowest_origin(bgp_router_t * pRouter, bgp_routes_t * pRoutes)
{
  unsigned int index;
  bgp_route_t * pRoute;
  bgp_origin_t tLowestOrigin= ROUTE_ORIGIN_INCOMPLETE;

#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  SIntArray * piNextHopCounter = dp_rule_nexthop_counter_create();
  int iNextHopCount;
#endif

  // Calculate lowest Origin
  for (index= 0; index < bgp_routes_size(pRoutes); index++) {
    pRoute= bgp_routes_at(pRoutes, index);
    if (route_get_origin(pRoute) < tLowestOrigin)
      tLowestOrigin= route_get_origin(pRoute);
  }
  // Exclude routes with a greater Origin
  index= 0;
  while (index < bgp_routes_size(pRoutes)) {
    if (route_get_origin(bgp_routes_at(pRoutes, index))	> tLowestOrigin)
      ptr_array_remove_at(pRoutes, index);
    else
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
    {
      dp_rule_nexthop_add(piNextHopCounter, route_get_nexthop(bgp_routes_at(pRoutes, index)));
      iidex++;
    }
#else
      index++;
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
int dp_rule_lowest_med(bgp_router_t * pRouter, bgp_routes_t * pRoutes)
{
  unsigned int index, index2;
  int iLastAS;
  bgp_route_t * pRoute, * pRoute2;
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
    for (index= 0; index < bgp_routes_size(pRoutes); index++) {
      pRoute= bgp_routes_at(pRoutes, index);
      if (route_med_get(pRoute) < uMinMED)
	uMinMED= route_med_get(pRoute);
    }
    // Discard routes with an higher MED
    index= 0;
    while (index < bgp_routes_size(pRoutes)) {
      if (route_med_get(bgp_routes_at(pRoutes, index)) > uMinMED)
	ptr_array_remove_at(pRoutes, index);
      else
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
    {
      dp_rule_nexthop_add(piNextHopCounter, route_get_nexthop(bgp_routes_at(pRoutes, index)));
      index++;
    }
#else
      index++;
#endif
    }
    break;

  case BGP_MED_TYPE_DETERMINISTIC:

    index= 0;
    while (index < bgp_routes_size(pRoutes)) {
      pRoute= bgp_routes_at(pRoutes, index);
      
      if (pRoute != NULL) {
      
	iLastAS= route_path_last_as(pRoute);
	
	index2= index+1;
	while (index2 < bgp_routes_size(pRoutes)) {
	  pRoute2= bgp_routes_at(pRoutes, index2);
	  
	  if (pRoute2 != NULL) {
	  
	    /* DEBUG-BEGIN */
	    /*
	    printf("route1: ");
	    route_dump(pLogOut, pRoute);
	    printf("\nroute2: ");
	    route_dump(pLogOut, pRoute2);
	    printf("\n");
	    printf("last-AS: %u\n", iLastAS);
	    */
	    /* DEBUG-END */
	    
	    if (iLastAS == route_path_last_as(pRoute2)) {
	      
	      /* DEBUG-BEGIN */
	      /*
	      printf("compare...[%u <-> %u]\n", pRoute->pAttr->uMED, pRoute2->pAttr->uMED);
	      */
	      /* DEBUG-END */
	      
	      if (pRoute->pAttr->uMED < pRoute2->pAttr->uMED) {
		/* DEBUG-BEGIN */
		/*
		printf("route1 < route2 => remove route2\n");
		*/
		/* DEBUG-END */
		pRoutes->data[index2]= NULL;
	      } else if (pRoute->pAttr->uMED > pRoute2->pAttr->uMED) {
		/* DEBUG-BEGIN */
		/*
		printf("(route2 < route1) => remove route1\n");
		*/
		/* DEBUG-END */
		pRoutes->data[index]= NULL;
		break;
	      }
	      
	    }
	  }
	  
	  index2++;
	}
      }
      index++;
    }
    index= 0;
    while (index < bgp_routes_size(pRoutes)) {
      if (pRoutes->data[index] == NULL) {
	ptr_array_remove_at(pRoutes, index);
      } else
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
    {
      dp_rule_nexthop_add(piNextHopCounter, route_get_nexthop(bgp_routes_at(pRoutes, index)));
      index++;
    }
#else
      index++;
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
int dp_rule_ebgp_over_ibgp(bgp_router_t * pRouter, bgp_routes_t * pRoutes)
{
  unsigned int index;
  int iEBGP= 0;
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  SIntArray * piNextHopCounter = dp_rule_nexthop_counter_create();
  int iNextHopCount;
#endif

  // Detect if there is at least one route learned through an eBGP
  // session:
  for (index= 0; index < bgp_routes_size(pRoutes); index++) {
    if (bgp_routes_at(pRoutes, index)->pPeer->uRemoteAS != pRouter->uASN) {
      iEBGP= 1;
      break;
    }
  }
  // If there is at least one route learned over eBGP, discard all the
  // routes learned over an iBGP session:
  if (iEBGP) {
    index= 0;
    while (index < bgp_routes_size(pRoutes)) {
      if (bgp_routes_at(pRoutes, index)->pPeer->uRemoteAS == pRouter->uASN)
	ptr_array_remove_at(pRoutes, index);
      else
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
    {
      dp_rule_nexthop_add(piNextHopCounter, route_get_nexthop(bgp_routes_at(pRoutes, index)));
      index++;
    }
#else
      index++;
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
static uint32_t _dp_rule_igp_cost(bgp_router_t * pRouter, net_addr_t tNextHop)
{
  SNetRouteInfo * pRouteInfo;

  /* Is there a route towards the destination ? */
  pRouteInfo= rt_find_best(pRouter->pNode->rt, tNextHop, NET_ROUTE_ANY);
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
int dp_rule_nearest_next_hop(bgp_router_t * pRouter, bgp_routes_t * pRoutes)
{
  unsigned int index;
  bgp_route_t * pRoute;
  uint32_t uLowestCost= UINT_MAX;
  uint32_t uIGPcost;
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  SIntArray * piNextHopCounter = dp_rule_nexthop_counter_create();
  int iNextHopCount;
#endif

  // Calculate lowest IGP-cost
  for (index= 0; index < bgp_routes_size(pRoutes); index++) {
    pRoute= bgp_routes_at(pRoutes, index);

    uIGPcost= _dp_rule_igp_cost(pRouter, pRoute->pAttr->tNextHop);
    if (uIGPcost < uLowestCost)
      uLowestCost= uIGPcost;

    /* Mark route as depending on the IGP weight. This is done in
       order to more efficiently react to IGP changes. See
       'bgp_router_scan_rib' for more information. */
    route_flag_set(pRoute, ROUTE_FLAG_DP_IGP, 1);

  }

  // Discard routes with an higher IGP-cost 
  index= 0;
  while (index < bgp_routes_size(pRoutes)) {
    pRoute= bgp_routes_at(pRoutes, index);
    uIGPcost= _dp_rule_igp_cost(pRouter, pRoute->pAttr->tNextHop);
    if (uIGPcost > uLowestCost) {
      ptr_array_remove_at(pRoutes, index);
    } else
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
    {
      dp_rule_nexthop_add(piNextHopCounter, route_get_nexthop(bgp_routes_at(pRoutes, index)));
      index++;
    }
#else
      index++;
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
 */
int dp_rule_lowest_router_id(bgp_router_t * pRouter, bgp_routes_t * pRoutes)
{
  net_addr_t tLowestRouterID= MAX_ADDR;
  net_addr_t tID;
  /*net_addr_t tOriginatorID;*/
  bgp_route_t * pRoute;
  unsigned int index;
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  SIntArray * piNextHopCounter = dp_rule_nexthop_counter_create();
  int iNextHopCount;
#endif

  // Calculate lowest ROUTER-ID (or ORIGINATOR-ID)
  for (index= 0; index < bgp_routes_size(pRoutes); index++) {
    pRoute= bgp_routes_at(pRoutes, index);

    /* DEBUG-BEGIN */
    /*
    printf("router-ID: ");
    ip_address_dump(pLogOut, route_peer_get(pRoute)->tRouterID);
    printf("\noriginator-ID-ptr: %p\n", pRoute->pAttr->pOriginator);
    if (route_originator_get(pRoute, &tOriginatorID) == 0) {
      printf("originator-ID: ");
      ip_address_dump(pLogOut, tOriginatorID);
      printf("\n");
      }
    */
    /* DEBUG-END */

    /* Originator-ID or Router-ID ? (< 0 => use router-ID) */
    if (route_originator_get(pRoute, &tID) < 0)
      tID= route_peer_get(pRoute)->tRouterID;
    if (tID < tLowestRouterID)
      tLowestRouterID= tID;
  }

  /* DEBUG-BEGIN */
  /*
  printf("max-addr: ");
  ip_address_dump(pLogOut, tLowestRouterID);
  printf("\n");
  */
  /* DEBUG-END */

  // Discard routes from neighbors with an higher ROUTER-ID (or
  // ORIGINATOR-ID)
  index= 0;
  while (index < bgp_routes_size(pRoutes)) {
    pRoute= bgp_routes_at(pRoutes, index);
    /* Originator-ID or Router-ID ? (< 0 => use router-ID) */
    if (route_originator_get(pRoute, &tID) < 0)
      tID= route_peer_get(pRoute)->tRouterID; 
    if (tID > tLowestRouterID)
      ptr_array_remove_at(pRoutes, index);
    else
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
    {
      dp_rule_nexthop_add(piNextHopCounter, route_get_nexthop(bgp_routes_at(pRoutes, index)));
      index++;
    }
#else
      index++;
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
int dp_rule_shortest_cluster_list(bgp_router_t * pRouter, bgp_routes_t * pRoutes)
{
  bgp_route_t * pRoute;
  unsigned int index;
  uint8_t uMinLen= 255;
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  SIntArray * piNextHopCounter = dp_rule_nexthop_counter_create();
  int iNextHopCount;
#endif

  // Calculate shortest cluster-ID-list
  for (index= 0; index < bgp_routes_size(pRoutes); index++) {
    pRoute= bgp_routes_at(pRoutes, index);
    if (pRoute->pAttr->pClusterList == NULL)
      uMinLen= 0;
    else
      uMinLen= cluster_list_length(pRoute->pAttr->pClusterList);    
  }
  // Discard routes with a longer cluster-ID-list
  index= 0;
  while (index < bgp_routes_size(pRoutes)) {
    pRoute= bgp_routes_at(pRoutes, index);
    if ((pRoute->pAttr->pClusterList != NULL) &&
	(cluster_list_length(pRoute->pAttr->pClusterList) > uMinLen))
      ptr_array_remove_at(pRoutes, index);
    else
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
    {
      dp_rule_nexthop_add(piNextHopCounter, route_get_nexthop(bgp_routes_at(pRoutes, index)));
      index++;
    }
#else
      index++;
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
int dp_rule_lowest_neighbor_address(bgp_router_t * pRouter, bgp_routes_t * pRoutes)
{
  net_addr_t tLowestAddr= MAX_ADDR;
  bgp_route_t * pRoute;
  unsigned int index;
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  SIntArray * piNextHopCounter = dp_rule_nexthop_counter_create();
  int iNextHopCount;
#endif

  // Calculate lowest neighbor address
  for (index= 0; index < bgp_routes_size(pRoutes); index++) {
    pRoute= bgp_routes_at(pRoutes, index);
    if (route_peer_get(pRoute)->tAddr < tLowestAddr)
      tLowestAddr= route_peer_get(pRoute)->tAddr;
  }
  // Discard routes from neighbors with an higher address
  index= 0;
  while (index < bgp_routes_size(pRoutes)) {
    pRoute= bgp_routes_at(pRoutes, index);
    if (route_peer_get(pRoute)->tAddr > tLowestAddr)
      ptr_array_remove_at(pRoutes, index);
    else
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
    {
      dp_rule_nexthop_add(piNextHopCounter, route_get_nexthop(bgp_routes_at(pRoutes, index)));
      index++;
    }
#else
      index++;
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
int dp_rule_final(bgp_router_t * pRouter, bgp_routes_t * pRoutes)
{
  int iResult;

  // *** final tie-break ***
  while (bgp_routes_size(pRoutes) > 1) {
    iResult= pRouter->fTieBreak(bgp_routes_at(pRoutes, 0),
				bgp_routes_at(pRoutes, 1));
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
