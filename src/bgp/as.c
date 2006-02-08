// ==================================================================
// @(#)as.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @author Sebastien Tandel (standel@info.ucl.ac.be)
// @date 22/11/2002
// @lastdate 07/02/2006
// ==================================================================
// TO-DO LIST:
// - change pLocalNetworks's type to SRoutes (routes_list.h)
// - do not keep in pLocalNetworks a _copy_ of the BGP routes locally
//   originated. Store a reference instead => reduce memory footprint.
// - move local-pref based rule of decision process in xxx_tie_break()
//   function.
// - re-check IGP/BGP interaction
// - check if tie-break ID (uTBID) is still in use or not. Remove it
//   if possible.
// - check initialization and comparison of Router-ID (it was
//   previously not exchanged in the OPEN message).
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <stdlib.h>
//#include <stdio.h>
#include <string.h>

#include <libgds/array.h>
#include <libgds/list.h>
#include <libgds/log.h>
#include <libgds/memory.h>
#include <libgds/tokenizer.h>

#include <bgp/as.h>
#include <bgp/comm.h>
#include <bgp/domain.h>
#include <bgp/dp_rules.h>
#include <bgp/ecomm.h>
#include <bgp/mrtd.h>
#include <bgp/peer.h>
#include <bgp/qos.h>
#include <bgp/routes_list.h>
#include <bgp/tie_breaks.h>
#include <net/network.h>
#include <net/link.h>
#include <net/node.h>
#include <net/protocol.h>
#include <ui/output.h>

// -----[ decision process ]-----------------------------------------
char * DP_RULE_NAME[DP_NUM_RULES]= {
  "Highest LOCAL-PREF",
  "Shortest AS-PATH",
  "Lowest ORIGIN",
  "Lowest MED",
  "eBGP over iBGP",
  "Nearest NEXT-HOP",
  "Lowest ROUTER-ID",
  "Shortest CLUSTER-ID-LIST",
  "Lowest neighbor address",
};
FDPRule DP_RULES[DP_NUM_RULES]= {
  dp_rule_highest_pref,
  dp_rule_shortest_path,
  dp_rule_lowest_origin,
  dp_rule_lowest_med,
  dp_rule_ebgp_over_ibgp,
  dp_rule_nearest_next_hop,
  dp_rule_lowest_router_id,
  dp_rule_shortest_cluster_list,
  dp_rule_lowest_neighbor_address,
};

// ----- options -----
FTieBreakFunction BGP_OPTIONS_TIE_BREAK	    = TIE_BREAK_DEFAULT;
uint8_t BGP_OPTIONS_NLRI		    = BGP_NLRI_BE;
uint32_t BGP_OPTIONS_DEFAULT_LOCAL_PREF	    = 0;
uint8_t BGP_OPTIONS_MED_TYPE		    = BGP_MED_TYPE_DETERMINISTIC;
uint8_t BGP_OPTIONS_SHOW_MODE		    = ROUTE_SHOW_CISCO;
uint8_t BGP_OPTIONS_RIB_OUT		    = 1;
uint8_t BGP_OPTIONS_AUTO_CREATE		    = 0;
uint8_t BGP_OPTIONS_VIRTUAL_ADJ_RIB_OUT	    = 0;
uint8_t BGP_OPTIONS_ADVERTISE_EXTERNAL_BEST = 0;
//#define NO_RIB_OUT

// ----- _bgp_router_peers_compare -----------------------------------
static int _bgp_router_peers_compare(void * pItem1, void * pItem2,
				     unsigned int uEltSize)
{
  SPeer * pPeer1= *((SPeer **) pItem1);
  SPeer * pPeer2= *((SPeer **) pItem2);

  if (pPeer1->tAddr < pPeer2->tAddr)
    return -1;
  else if (pPeer1->tAddr > pPeer2->tAddr)
    return 1;
  else
    return 0;
}

// ----- _bgp_router_peers_destroy ----------------------------------
static void _bgp_router_peers_destroy(void * pItem)
{
  SBGPPeer * pPeer= *((SBGPPeer **) pItem);

  bgp_peer_destroy(&pPeer);
}

// ----- bgp_router_create ------------------------------------------
/**
 * Create a BGP router in the given node.
 *
 * Arguments:
 * - the AS-number of the domain the router will belong to
 * - the underlying node
 * - a tie-break ID (this not used anymore)
 */
SBGPRouter * bgp_router_create(uint16_t uNumber, SNetNode * pNode,
			       uint32_t uTBID)
{
  SBGPRouter * pRouter= (SBGPRouter*) MALLOC(sizeof(SBGPRouter));

  pRouter->uNumber= uNumber;
  // The Router-ID of this router is the highest IP address of the
  // router with preference to loopback interfaces. There is only one
  // loopback interface in C-BGP and its IP address is the node's
  // identifier.
  pRouter->tRouterID= pNode->tAddr;
  // The Tie-Break ID should be removed...
  pRouter->uTBID= uTBID;
  pRouter->pPeers= ptr_array_create(ARRAY_OPTION_SORTED |
				    ARRAY_OPTION_UNIQUE,
				    _bgp_router_peers_compare,
				    _bgp_router_peers_destroy);
  pRouter->pLocRIB= rib_create(0);
  pRouter->pLocalNetworks= ptr_array_create_ref(0);
  pRouter->fTieBreak= BGP_OPTIONS_TIE_BREAK;
  pRouter->tClusterID= pNode->tAddr;
  pRouter->iRouteReflector= 0;

  // Reference to the router running this BGP router
  pRouter->pNode= pNode;
  // Register the router into its AS
  bgp_domain_add_router(get_bgp_domain(uNumber), pRouter);

  return pRouter;
}

// ----- bgp_router_destroy -----------------------------------------
/**
 *
 */
void bgp_router_destroy(SBGPRouter ** ppRouter)
{
  int iIndex;

  if (*ppRouter != NULL) {
    ptr_array_destroy(&(*ppRouter)->pPeers);
    rib_destroy(&(*ppRouter)->pLocRIB);
    for (iIndex= 0; iIndex < ptr_array_length((*ppRouter)->pLocalNetworks);
	 iIndex++) {
      route_destroy((SRoute **) &(*ppRouter)->pLocalNetworks->data[iIndex]);
    }
    ptr_array_destroy(&(*ppRouter)->pLocalNetworks);
    FREE(*ppRouter);
    *ppRouter= NULL;
  }
}

// ----- bgp_router_find_peer ---------------------------------------
/**
 *
 */
SPeer * bgp_router_find_peer(SBGPRouter * pRouter, net_addr_t tAddr)
{
  unsigned int uIndex;
  net_addr_t * pAddr= &tAddr;
  SPeer * pPeer= NULL;

  if (ptr_array_sorted_find_index(pRouter->pPeers, &pAddr, &uIndex) != -1)
    pPeer= (SPeer *) pRouter->pPeers->data[uIndex];
  return pPeer;
}

// ----- bgp_router_add_peer ----------------------------------------
/**
 *
 */
SBGPPeer * bgp_router_add_peer(SBGPRouter * pRouter, uint16_t uRemoteAS,
			       net_addr_t tAddr, uint8_t uPeerType)
{
  SBGPPeer * pNewPeer= bgp_peer_create(uRemoteAS, tAddr,
				       pRouter, uPeerType);

  if (ptr_array_add(pRouter->pPeers, &pNewPeer) < 0) {
    bgp_peer_destroy(&pNewPeer);
    return NULL;
  } else
    return pNewPeer;
}

// ----- bgp_router_peer_set_filter ---------------------------------
/**
 *
 */
int bgp_router_peer_set_filter(SBGPRouter * pRouter, net_addr_t tAddr,
			       SFilter * pFilter, int iIn)
{
  SPeer * pPeer;

  if ((pPeer= bgp_router_find_peer(pRouter, tAddr)) != NULL) {
    if (iIn == FILTER_IN) {
      peer_set_in_filter(pPeer, pFilter);
    } else {
      peer_set_out_filter(pPeer, pFilter);
    }
    return 0;
  }
  return -1;
}

// ----- bgp_router_add_network -------------------------------------
/**
 *
 */
int bgp_router_add_network(SBGPRouter * pRouter, SPrefix sPrefix)
{
  SRoute * pRoute= route_create(sPrefix, NULL,
				pRouter->pNode->tAddr,
				ROUTE_ORIGIN_IGP);
  route_flag_set(pRoute, ROUTE_FLAG_BEST, 1);
  route_flag_set(pRoute, ROUTE_FLAG_FEASIBLE, 1);
  route_flag_set(pRoute, ROUTE_FLAG_INTERNAL, 1);
  route_localpref_set(pRoute, BGP_OPTIONS_DEFAULT_LOCAL_PREF);
#ifdef __EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
  route_flag_set(pRoute, ROUTE_FLAG_EXTERNAL_BEST, 0);
#endif
  ptr_array_append(pRouter->pLocalNetworks, pRoute);
  rib_add_route(pRouter->pLocRIB, route_copy(pRoute));
  bgp_router_decision_process_disseminate(pRouter, sPrefix, pRoute);
  return 0;
}

// ----- bgp_router_del_network -------------------------------------
/**
 *
 */
int bgp_router_del_network(SBGPRouter * pRouter, SPrefix sPrefix)
{
  SRoute * pRoute= NULL;
  int iIndex;

  // Looks for the route and mark it as unfeasible
  for (iIndex= 0; iIndex < ptr_array_length((SPtrArray *)
					    pRouter->pLocalNetworks);
       iIndex++) {
    if (ip_prefix_equals(((SRoute *)
			 pRouter->pLocalNetworks->data[iIndex])->sPrefix,
	sPrefix)) {
      pRoute= (SRoute *) pRouter->pLocalNetworks->data[iIndex];
      route_flag_set(pRoute, ROUTE_FLAG_FEASIBLE, 0);
      break;
    }
  }

  // If the network exists:
  if (pRoute != NULL) {

    // Withdraw this route
    // NOTE: this should be made through a call to
    // as_decision_process, but some details need to be fixed before:
    // pOriginPeer is NULL, and the local networks should be
    // advertised as other routes...
    /// *****

    // Remove the route from the Loc-RIB
    rib_remove_route(pRouter->pLocRIB, sPrefix);

    // Remove the route from the list of local networks.
    ptr_array_remove_at((SPtrArray *) pRouter->pLocalNetworks,
			iIndex);

    // Free route
    route_destroy(&pRoute);

    bgp_router_decision_process_disseminate(pRouter, sPrefix, NULL);

    return 0;
  }

  return -1;
}

// ----- bgp_router_add_route ---------------------------------------
/**
 * Add a route in the Loc-RIB
 */
int bgp_router_add_route(SBGPRouter * pRouter, SRoute * pRoute)
{
  ptr_array_append(pRouter->pLocalNetworks, pRoute);
  rib_add_route(pRouter->pLocRIB, route_copy(pRoute));
  bgp_router_decision_process_disseminate(pRouter, pRoute->sPrefix, pRoute);
  return 0;
}

// ----- as_add_qos_network -----------------------------------------
/**
 *
 */
#ifdef BGP_QOS
int as_add_qos_network(SAS * pAS, SPrefix sPrefix,
		       net_link_delay_t tDelay)
{
  SRoute * pRoute= route_create(sPrefix, NULL,
				pAS->pNode->tAddr, ROUTE_ORIGIN_IGP);
  route_flag_set(pRoute, ROUTE_FLAG_FEASIBLE, 1);
  route_localpref_set(pRoute, BGP_OPTIONS_DEFAULT_LOCAL_PREF);

  qos_route_update_delay(pRoute, tDelay);
  pRoute->tDelay.uWeight= 1;
  qos_route_update_bandwidth(pRoute, 0);
  pRoute->tBandwidth.uWeight= 1;

  ptr_array_append(pAS->pLocalNetworks, pRoute);
  rib_add_route(pAS->pLocRIB, route_copy(pRoute));
  return 0;
}
#endif

// ----- bgp_router_ecomm_process -----------------------------------
/**
 * Apply output extended communities:
 *   - REDISTRIBUTION COMMUNITIES
 *
 * Returns:
 *   0 => Ignore route (destroy)
 *   1 => Redistribute
 */
int bgp_router_ecomm_process(SPeer * pPeer, SRoute * pRoute)
{
  int iIndex;
  uint8_t uActionParam;

  if (pRoute->pECommunities != NULL) {

    for (iIndex= 0; iIndex < ecomm_length(pRoute->pECommunities); iIndex++) {
      SECommunity * pComm= ecomm_get_index(pRoute->pECommunities, iIndex);
      switch (pComm->uTypeHigh) {
      case ECOMM_RED:
	if (ecomm_red_match(pComm, pPeer)) {
	  switch ((pComm->uTypeLow >> 3) & 0x07) {
	  case ECOMM_RED_ACTION_PREPEND:
	    uActionParam= (pComm->uTypeLow & 0x07);
	    route_path_prepend(pRoute, pPeer->pLocalRouter->uNumber,
			       uActionParam);
	    break;
	  case ECOMM_RED_ACTION_NO_EXPORT:
	    route_comm_append(pRoute, COMM_NO_EXPORT);
	    break;
	  case ECOMM_RED_ACTION_IGNORE:
	    return 0;
	    break;
	  default:
	    abort();
	  }
	}
	break;
      }
    }
  }
  return 1;
}

// ----- bgp_router_advertise_to_peer -------------------------------
/**
 * Advertise a route to given peer:
 *
 *   + route-reflectors:
 *     - do not redistribute a route from a non-client peer to
 *       non-client peers
 *     - do not redistribute a route from a client peer to the
 *       originator client peer
 *   - do not redistribute a route learned through iBGP to an
 *     iBGP peer
 *   - avoid sending to originator peer
 *   - avoid sending to a peer in AS-Path (SSLD)
 *   - check standard communities (NO_ADVERTISE and NO_EXPORT)
 *   - apply redistribution communities
 *   - strip non-transitive extended communities
 *   - update Next-Hop (next-hop-self/next-hop)
 *   - prepend AS-Path (if redistribution to an external peer)
 */
int bgp_router_advertise_to_peer(SBGPRouter * pRouter, SPeer * pPeer,
				 SRoute * pRoute)
{
  net_addr_t tOriginator;
  SRoute * pNewRoute= NULL;
  net_addr_t tOriginPeerAddr;
  int iExternalSession= (pRouter->uNumber != pPeer->uRemoteAS);
  int iLocalRoute= (pRoute->tNextHop == pRouter->pNode->tAddr);
  int iExternalRoute= ((!iLocalRoute) &&
		       (pRouter->uNumber != route_peer_get(pRoute)->uRemoteAS));
  /* [route/session attributes]
   * iExternalSession == the dst peer is external
   * iExternalRoute   == the src peer is external
   * iLocalRoute      == the route is locally originated
   */

  LOG_DEBUG("advertise_to_peer (");
  LOG_ENABLED_DEBUG() ip_prefix_dump(log_get_stream(pMainLog), pRoute->sPrefix);
  LOG_DEBUG(") from ");
  LOG_ENABLED_DEBUG() bgp_router_dump_id(log_get_stream(pMainLog), pRouter);
  LOG_DEBUG(" to ");
  LOG_ENABLED_DEBUG() bgp_peer_dump_id(log_get_stream(pMainLog), pPeer);
  LOG_DEBUG(" (flags:%d,%d,%d)\n", iExternalSession, iLocalRoute, iExternalRoute);
  
#ifdef __ROUTER_LIST_ENABLE__
  // Append the router-list with the IP address of this router
  route_router_list_append(pRoute, pRouter->pNode->tAddr);
#endif

  // Do not redistribute to the next-hop peer
  if (pPeer->tAddr == pRoute->tNextHop) {
    LOG_DEBUG("out-filtered(next-hop-peer)\n");
    return -1;
  }

  // Do not redistribute to other peers
  if (route_comm_contains(pRoute, COMM_NO_ADVERTISE)) {
    LOG_DEBUG("out-filtered(comm_no_advertise)\n");
    return -1;
  }

  // Do not redistribute outside confederation (here AS)
  if ((iExternalSession) &&
      (route_comm_contains(pRoute, COMM_NO_EXPORT))) {
    LOG_DEBUG("out-filtered(comm_no_export)\n");
    return -1;
  }

  // Avoid loop creation (SSLD, Sender-Side Loop Detection)
  if ((iExternalSession) &&
      (route_path_contains(pRoute, pPeer->uRemoteAS))) {
    LOG_DEBUG("out-filtered(ssld)\n");
    return -1;
  }

  // If this route was learned through an iBGP session, do not
  // redistribute it to an internal peer
  if ((!pRouter->iRouteReflector) && (!iLocalRoute) &&
      (!iExternalRoute) && (!iExternalSession)) {
    LOG_DEBUG("out-filtered(iBGP-peer --> iBGP-peer)\n");
    return -1;
  }

  // Copy the route. This is required since subsequent filters may
  // alter the route's attribute !!
  pNewRoute= route_copy(pRoute);

  if ((pRouter->iRouteReflector) && (!iExternalRoute)) {
    // Route-Reflection: update Originator field
    if (route_originator_get(pNewRoute, NULL) == -1) {
      if (iLocalRoute)
	tOriginPeerAddr= pRouter->tRouterID;
      else
	tOriginPeerAddr= route_peer_get(pRoute)->tRouterID;
      route_originator_set(pNewRoute, tOriginPeerAddr);
    }
    // Route-Reflection: append Cluster-ID to Cluster-ID-List field
    if ((iExternalRoute || route_flag_get(pNewRoute, ROUTE_FLAG_RR_CLIENT)) &&
	(!bgp_peer_flag_get(pPeer, PEER_FLAG_RR_CLIENT)))
      route_cluster_list_append(pNewRoute, pRouter->tClusterID);
  }

  if ((pRouter->iRouteReflector) && (!iLocalRoute) &&
      (!iExternalRoute) && (!iExternalSession)) {
    // Route-reflectors: do not redistribute a route from a client peer
    // to the originator client peer
    if (route_flag_get(pNewRoute, ROUTE_FLAG_RR_CLIENT)) {
      if (bgp_peer_flag_get(pPeer, PEER_FLAG_RR_CLIENT)) {
	assert(route_originator_get(pNewRoute, &tOriginator) == 0);
	if (pPeer->tAddr == tOriginator) {
	  LOG_DEBUG("out-filtered (RR: client --> originator-client)\n");
	  route_destroy(&pNewRoute);
	  return -1;
	}
      }
    }

    // Route-reflectors: do not redistribute a route from a non-client
    // peer to non-client peers (becoz non-client peers MUST be fully
    // meshed)
    else {
      if (!bgp_peer_flag_get(pPeer, PEER_FLAG_RR_CLIENT)) {
	LOG_DEBUG("out-filtered (RR: non-client --> non-client)\n");
	route_destroy(&pNewRoute);
	return -1;
      }
    }
  }

  // Check output filter and extended communities
  if (bgp_router_ecomm_process(pPeer, pNewRoute)) {

    route_ecomm_strip_non_transitive(pNewRoute);

    // Discard MED if advertising to an external peer
    if (iExternalSession)
      route_med_clear(pNewRoute);

    if (filter_apply(pPeer->pOutFilter, pRouter, pNewRoute)) {

      // Change the route's next-hop to this router
      // - if advertisement from an external peer
      // - if the 'next-hop-self' option is set for this peer
      // Note: in the case of route-reflectors, the next-hop will only
      // be changed for eBGP learned routes
      if (iExternalSession) {
	if (pPeer->tNextHop != 0)
	  route_nexthop_set(pNewRoute, pPeer->tNextHop);
	else
	  route_nexthop_set(pNewRoute, pRouter->pNode->tAddr);
      } else if (!pRouter->iRouteReflector || iExternalRoute) {
	if (!iLocalRoute &&  bgp_peer_flag_get(route_peer_get(pNewRoute),
					       PEER_FLAG_NEXT_HOP_SELF)) {
	  route_nexthop_set(pNewRoute, pRouter->pNode->tAddr);
	} else if (pPeer->tNextHop != 0) {
	  route_nexthop_set(pNewRoute, pPeer->tNextHop);
	}
      }

      // Prepend AS-Number if external peer (eBGP session)
      if (iExternalSession)
	route_path_prepend(pNewRoute, pRouter->uNumber, 1);
      
      // Route-Reflection: clear Originator and Cluster-ID-List fields
      // if external peer (these fields are non-transitive)
      if (iExternalSession) {
	route_originator_clear(pNewRoute);
	route_cluster_list_clear(pNewRoute);
      }
      
      bgp_peer_announce_route(pPeer, pNewRoute);
      return 0;
    } else
      LOG_DEBUG("out-filtered (policy)\n");

  } else
    LOG_DEBUG("out-filtered (ext-community)\n");
  
  route_destroy(&pNewRoute);
  return -1;
}
  
// ----- bgp_router_withdraw_to_peer --------------------------------
/**
 * Withdraw the given prefix to the given peer router.
 */
void bgp_router_withdraw_to_peer(SBGPRouter * pRouter, SBGPPeer * pPeer,
				 SPrefix sPrefix)
{
  LOG_DEBUG("*** AS%d withdraw to AS%d ***\n", pRouter->uNumber,
	    pPeer->uRemoteAS);

  bgp_peer_withdraw_prefix(pPeer, sPrefix);
}

// ----- bgp_router_start -------------------------------------------
/**
 * Run the BGP router. The router starts by sending its local prefixes
 * to its peers.
 */
int bgp_router_start(SBGPRouter * pRouter)
{
  int iIndex;

  // Open all BGP sessions
  for (iIndex= 0; iIndex < ptr_array_length(pRouter->pPeers); iIndex++)
    if (bgp_peer_open_session((SPeer *) pRouter->pPeers->data[iIndex]) != 0)
      return -1;

  return 0;
}

// ----- bgp_router_stop --------------------------------------------
/**
 * Stop the BGP router. Each peering session is brought to IDLE
 * state.
 */
int bgp_router_stop(SBGPRouter * pRouter)
{
  int iIndex;
  
  // Close all BGP sessions
  for (iIndex= 0; iIndex < ptr_array_length(pRouter->pPeers); iIndex++)
    if (bgp_peer_close_session((SPeer *) pRouter->pPeers->data[iIndex]) != 0)
      return -1;

  return 0;
}

// ----- bgp_router_reset -------------------------------------------
/**
 * This function shuts down every peering session, then restarts
 * them.
 */
int bgp_router_reset(SBGPRouter * pRouter)
{
  // Stop the router
  if (bgp_router_stop(pRouter))
    return -1;

  // Start the router
  if (bgp_router_start(pRouter))
    return -1;

  return -1;
}

#ifdef __EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
typedef struct {
  SBGPRouter * pRouter;
  SRoutes * pBestEBGPRoutes;
} SSecondBestCtx;

int bgp_router_copy_ebgp_routes(void * pItem, void * pContext)
{
  SSecondBestCtx * pSecondRoutes = (SSecondBestCtx *) pContext;
  SRoute * pRoute = *(SRoute ** )pItem;
  
  if (pRoute->pPeer->uRemoteAS != pSecondRoutes->pRouter->uNumber) {
    if (pSecondRoutes->pBestEBGPRoutes == NULL)
      pSecondRoutes->pBestEBGPRoutes = routes_list_create(ROUTES_LIST_OPTION_REF);
    routes_list_append(pSecondRoutes->pBestEBGPRoutes, pRoute);
  }
  return 0;
}
#endif

// ----- bgp_router_decision_process_run ----------------------------
/**
 * Select one route with the following rules (see the actual
 * definition of the decision process in the global array DP_RULES[]):
 *
 *   1. prefer highest LOCAL-PREF
 *   2. prefer shortest AS-PATH
 *   3. prefer lowest ORIGIN (IGP < EGP < INCOMPLETE)
 *   4. prefer lowest MED
 *   5. prefer eBGP over iBGP
 *   6. prefer nearest next-hop (IGP)
 *   7. prefer shortest CLUSTER-ID-LIST
 *   8. prefer lowest neighbor router-ID
 *   9. final tie-break (prefer lowest neighbor IP address)
 *
 * Note: Originator-ID should be substituted to router-ID in rule (8)
 * if route has been reflected (i.e. if Originator-ID is present in
 * the route)
 *
 * The function returns the index of the rule that broke the ties
 * incremented by 1. If the returned value is 0, that means that there
 * was a single rule (no choice). Otherwise, if 1 is returned, that
 * means that the Local-Pref rule broke the ties, and so on...
 */
int bgp_router_decision_process_run(SBGPRouter * pRouter,
				    SRoutes * pRoutes)
{
  int iRule;
#ifdef __EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
/*  uint16_t uLen, uRoute;
  SSecondBestCtx sSecondBest, sSecondBestOld;
  SRoutes * pEBGPRoutes;
  SRoute * pBestRoute;

  sSecondBest.pRouter = pRouter;*/
#endif

  // Apply the decision process rules in sequence until there is 1 or
  // 0 route remaining or until all the rules were applied.
  for (iRule= 0; iRule < DP_NUM_RULES; iRule++) {
    if (ptr_array_length(pRoutes) <= 1)
      break;
    LOG_DEBUG("rule: [ %s ]\n", DP_RULE_NAME[iRule]);
    DP_RULES[iRule](pRouter, pRoutes);
#ifdef __EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
    //Phase of collection of EBGP routes ... as we don't know the best EBGP
    //routes, we collect for each step the remaining EBGP routes. If there is
    //no EBGP routes in one step then we have our final set of EBGP routes.
    //If there are additional routes then we can destroy the previous set of
    //EBGP selected routes and save the new set.
/*    sSecondBestOld.pBestEBGPRoutes = sSecondBest.pBestEBGPRoutes;
    sSecondBestOld.pRouter = sSecondBest.pRouter;
    sSecondBest.pBestEBGPRoutes = NULL;
    if (routes_list_for_each(pRoutes, bgp_router_copy_ebgp_routes, &sSecondBest) != 0) {
      LOG_FATAL("Error : Can't determine if there are EBGP learned routes\n");
      abort();
    }
    if (sSecondBest.pBestEBGPRoutes == NULL) {
      sSecondBest.pBestEBGPRoutes = sSecondBestOld.pBestEBGPRoutes;
      sSecondBestOld.pBestEBGPRoutes = NULL;
    } else {
      routes_list_destroy(&(sSecondBestOld.pBestEBGPRoutes));
    }*/
#endif
  }

  // Check that at most a single best route will be returned.
  if (ptr_array_length(pRoutes) > 1) {
    LOG_FATAL("Error: decision process did not return a single best route\n");
    abort();
  }

/*#ifdef __EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
  pEBGPRoutes = sSecondBest.pBestEBGPRoutes
  pBestRoute = routes_list_get_at(pRoutes, 0);
  //As we advertise the EBGP best route to internal router if the overall best
  //is already an EBGP one, we don't have to do the dp rules again as we
  //already know it.
  if (pBestRoute->pPeer->uRemoteAS != pRouter->uNumber)
    route_list_destroy(&pEBGPRoutes);

  }
#endif*/
  
  return iRule;
}

// -----[ bgp_router_peer_rib_out_remove ]---------------------------
/**
 * Check in the peer's Adj-RIB-in if it has received from this router
 * a route towards the given prefix.
 */
int bgp_router_peer_rib_out_remove(SBGPRouter * pRouter,
				   SBGPPeer * pPeer,
				   SPrefix sPrefix)
{
  int iWithdrawRequired= 0;
#ifdef NO_RIB_OUT
  SNetwork * pNetwork= network_get();
  SNetNode * pNode;
  SNetProtocol * pProtocol;
  SBGPRouter * pPeerRouter;
  SBGPPeer * pThePeer;
#endif

#ifndef NO_RIB_OUT
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  if (rib_find_exact(pPeer->pAdjRIBOut, sPrefix) != NULL) {
    rib_remove_route(pPeer->pAdjRIBOut, sPrefix, NULL);
    iWithdrawRequired = 1;
  }
#else
  if (rib_find_exact(pPeer->pAdjRIBOut, sPrefix) != NULL) {
    rib_remove_route(pPeer->pAdjRIBOut, sPrefix);
    iWithdrawRequired= 1;
  }
#endif
#else
  pNode= network_find_node(pPeer->tAddr);
  assert(pNode != NULL);
  pProtocol= protocols_get(pNode->pProtocols, NET_PROTOCOL_BGP);
  assert(pProtocol != NULL);
  pPeerRouter= (SBGPRouter *) pProtocol->pHandler;
  assert(pPeerRouter != NULL);
  pThePeer= bgp_router_find_peer(pPeerRouter, pRouter->pNode->tAddr);
  assert(pThePeer != NULL);
  
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  //TODO Walton : what to do when there is no more Adj-RIB-Out
#else
  if (rib_find_exact(pThePeer->pAdjRIBIn, sPrefix) != NULL)
#endif
    iWithdrawRequired= 1;
#endif

  return iWithdrawRequired;
}

// -----[ bgp_router_peer_rib_out_replace ]--------------------------
/**
 *
 */
void bgp_router_peer_rib_out_replace(SBGPRouter * pRouter,
				     SBGPPeer * pPeer,
				     SRoute * pNewRoute)
{
#ifndef NO_RIB_OUT
  rib_replace_route(pPeer->pAdjRIBOut, pNewRoute);
#endif  
}

// ----- bgp_router_decision_process_disseminate_to_peer ------------
/**
 * This function is responsible for the dissemination of a route to a
 * peer. The route is only propagated if the session with the peer is
 * in state ESTABLISHED.
 *
 * If the route is NULL, the function will send an explicit withdraw
 * to the peer if a route for the same prefix was previously sent to
 * this peer.
 *
 * If the route is not NULL, the function will check if it can be
 * advertised to this peer (checking with output filters). If the
 * route is accepted, it is sent to the peer. Otherwise, an explicit
 * withdraw is sent if a route for the same prefix was previously sent
 * to this peer.
 */
void bgp_router_decision_process_disseminate_to_peer(SBGPRouter * pRouter,
						     SPrefix sPrefix,
						     SRoute * pRoute,
						     SBGPPeer * pPeer)
{
  SRoute * pNewRoute;

  if (pPeer->uSessionState == SESSION_STATE_ESTABLISHED) {
    LOG_DEBUG("DISSEMINATE (");
    LOG_ENABLED_DEBUG() ip_prefix_dump(log_get_stream(pMainLog), sPrefix);
    LOG_DEBUG(") from ");
    LOG_ENABLED_DEBUG() bgp_router_dump_id(log_get_stream(pMainLog), pRouter);
    LOG_DEBUG(" to ");
    LOG_ENABLED_DEBUG() bgp_peer_dump_id(log_get_stream(pMainLog), pPeer);
    LOG_DEBUG("\n");

    if (pRoute == NULL) {
      // A route was advertised to this peer => explicit withdraw
      if (bgp_router_peer_rib_out_remove(pRouter, pPeer, sPrefix)) {
	bgp_peer_withdraw_prefix(pPeer, sPrefix);
	LOG_DEBUG("\texplicit-withdraw\n");
      }
    } else {
      pNewRoute= route_copy(pRoute);
      if (bgp_router_advertise_to_peer(pRouter, pPeer, pNewRoute) == 0) {
	LOG_DEBUG("\treplaced\n");
	bgp_router_peer_rib_out_replace(pRouter, pPeer, pNewRoute);
      } else {
	route_destroy(&pNewRoute);
	LOG_DEBUG("\tfiltered\n");
	if (bgp_router_peer_rib_out_remove(pRouter, pPeer, sPrefix)) {
	  LOG_DEBUG("\texplicit-withdraw\n");
	  bgp_peer_withdraw_prefix(pPeer, sPrefix);
	}
      }
    }
  }
}


// ----- bgp_router_decision_process_disseminate --------------------
/**
 * Disseminate route to Adj-RIB-Outs.
 *
 * If there is no best route, then
 *   - if a route was previously announced, send an explicit withdraw
 *   - otherwize do nothing
 *
 * If there is one best route, then send an update. If a route was
 * previously announced, it will be implicitly withdrawn.
 */
void bgp_router_decision_process_disseminate(SBGPRouter * pRouter,
					     SPrefix sPrefix,
					     SRoute * pRoute)
{
  int iIndex;
  SPeer * pPeer;

  for (iIndex= 0; iIndex < ptr_array_length(pRouter->pPeers); iIndex++) {
    pPeer= (SPeer *) pRouter->pPeers->data[iIndex];

    if (!bgp_peer_flag_get(pPeer, PEER_FLAG_VIRTUAL))
      bgp_router_decision_process_disseminate_to_peer(pRouter, sPrefix,
						      pRoute, pPeer);
  }
}

#ifdef __EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
void bgp_router_decision_process_disseminate_external_best(SBGPRouter * pRouter,
							  SPrefix sPrefix,
							  SRoute * pRoute)
{
  int iIndex;
  SPeer * pPeer;

  for (iIndex= 0; iIndex < ptr_array_length(pRouter->pPeers); iIndex++) {
    pPeer= (SPeer *) pRouter->pPeers->data[iIndex];

    if (pRouter->uNumber == pPeer->uRemoteAS)
      bgp_router_decision_process_disseminate_to_peer(pRouter, sPrefix,
						      pRoute, pPeer);
  }

}
#endif

// ----- bgp_router_rt_add_route ------------------------------------
/**
 * This function inserts a BGP route into the node's routing
 * table. The route's next-hop is resolved before insertion.
 */
void bgp_router_rt_add_route(SBGPRouter * pRouter, SRoute * pRoute)
{
  SNetRouteInfo * pOldRouteInfo;
  SNetRouteNextHop * pNextHop= node_rt_lookup(pRouter->pNode,
					      pRoute->tNextHop);
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
  iResult= node_rt_add_route_link(pRouter->pNode, pRoute->sPrefix,
				  pNextHop->pIface, pRoute->tNextHop,
				  0, NET_ROUTE_BGP);
  if (iResult) {
    LOG_SEVERE("Error: could not add route (");
    rt_perror(stderr, iResult);
    LOG_SEVERE(")\n");
    abort();
  }
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

// ----- bgp_router_best_flag_off -----------------------------------
/**
 * Clear the BEST flag from the given Loc-RIB route. Clear the BEST
 * flag from the corresponding Adj-RIB-In route as well.
 *
 * PRE: the route is in the Loc-RIB (=> the route is local or there
 * exists a corresponding route in an Adj-RIB-In).
 */
void bgp_router_best_flag_off(SRoute * pOldRoute)
{
  SRoute * pAdjRoute;

  /* Remove BEST flag from old route in Loc-RIB. */
  route_flag_set(pOldRoute, ROUTE_FLAG_BEST, 0);

  /* If the route is not LOCAL, then there is a reference to the peer
     that announced it. */
  if (pOldRoute->pPeer != NULL) {
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
    pAdjRoute = rib_find_one_exact(pOldRoute->pPeer->pAdjRIBIn, 
			    pOldRoute->sPrefix, pOldRoute->tNextHop);
#else
    pAdjRoute= rib_find_exact(pOldRoute->pPeer->pAdjRIBIn,
			      pOldRoute->sPrefix);
#endif

    /* It is possible that the route does not exist in the Adj-RIB-In
       if the best route has been withdrawn. So check if the route in
       the Adj-RIB-In exists... */
    if (pAdjRoute != NULL) {    
    
      /* Remove BEST flag from route in Adj-RIB-In. */
      route_flag_set(pAdjRoute, ROUTE_FLAG_BEST, 0);

    }
  }
}

// ----- bgp_router_feasible_route ----------------------------------
/**
 * This function updates the ROUTE_FLAG_FEASIBLE flag of the given
 * route. If the next-hop of the route is reachable, the flag is set
 * and the function returns 1. Otherwise, the flag is cleared and the
 * function returns 0.
 */
int bgp_router_feasible_route(SBGPRouter * pRouter, SRoute * pRoute)
{
  SNetRouteNextHop * pNextHop;

  /* Get the route towards the next-hop */
  pNextHop= node_rt_lookup(pRouter->pNode, pRoute->tNextHop);

  if (pNextHop != NULL) {
    route_flag_set(pRoute, ROUTE_FLAG_FEASIBLE, 1);
    return 1;
  } else {
    route_flag_set(pRoute, ROUTE_FLAG_FEASIBLE, 0);
    return 0;
  }
}

// ----- bgp_router_get_best_routes ---------------------------------
/**
 * This function retrieves from the Loc-RIB the best route(s) towards
 * the given prefix.
 */
SRoutes * bgp_router_get_best_routes(SBGPRouter * pRouter, SPrefix sPrefix)
{
  SRoutes * pRoutes;
  SRoute * pRoute;

  pRoutes= routes_list_create(ROUTES_LIST_OPTION_REF);

#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  pRoute= rib_find_one_exact(pRouter->pLocRIB, sPrefix);
#else
  pRoute= rib_find_exact(pRouter->pLocRIB, sPrefix);
#endif

  if (pRoute != NULL)
    routes_list_append(pRoutes, pRoute);

  return pRoutes;
}

// ----- bgp_router_get_feasible_routes -----------------------------
/**
 * This function retrieves from the Adj-RIB-ins the feasible routes
 * towards the given prefix.
 *
 * Note: this function will update the 'feasible' flag of the eligible
 * routes.
 */
#ifdef __EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
  SRoutes * bgp_router_get_feasible_routes(SBGPRouter * pRouter, SPrefix sPrefix, const uint8_t uOnlyEBGP)
#else
  SRoutes * bgp_router_get_feasible_routes(SBGPRouter * pRouter, SPrefix sPrefix)
#endif
{
  SRoutes * pRoutes;
  SBGPPeer * pPeer;
  SRoute * pRoute;
  int iIndex;

  pRoutes= routes_list_create(ROUTES_LIST_OPTION_REF);

  // Get from the Adj-RIB-ins the list of available routes.
  for (iIndex= 0; iIndex < ptr_array_length(pRouter->pPeers); iIndex++) {

    pPeer= (SPeer*) pRouter->pPeers->data[iIndex];

    /* Check that the peering session is in ESTABLISHED state */
    if (pPeer->uSessionState == SESSION_STATE_ESTABLISHED) {

#ifdef __EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
      if (pPeer->uRemoteAS != pRouter->uNumber || uOnlyEBGP == 0) {
#endif
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
      pRoutes = rib_find_exact(pPeer->pAdjRIBIn, sPrefix);
      if (pRoutes != NULL) {
	for (uIndex = 0; uIndex < routes_list_get_num(pRoutes); uIndex++) {
	  pRoute = routes_list_get_at(pRoutes, uIndex);
#else
      pRoute= rib_find_exact(pPeer->pAdjRIBIn, sPrefix);
#endif

      /* Check that a route is present in the Adj-RIB-in of this peer
	 and that it is eligible (according to the in-filters) */
      if ((pRoute != NULL) &&
	  (route_flag_get(pRoute, ROUTE_FLAG_ELIGIBLE))) {
	
	
	/* Check that the route is feasible (next-hop reachable, and
	   so on). Note: this call will actually update the 'feasible'
	   flag of the route. */
	if (bgp_router_feasible_route(pRouter, pRoute))
	  routes_list_append(pRoutes, pRoute);

      }
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
	}
      }
#endif
    }
#ifdef __EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
    }
#endif
  }

  return pRoutes;
}

#ifdef __EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
void bgp_router_check_dissemination_external_best(SBGPRouter * pRouter, 
			      SRoutes * pEBGPRoutes, SRoute * pOldEBGPRoute, 
			      SRoute * pEBGPRoute, SPrefix sPrefix)
{
  pEBGPRoute = route_copy((SRoute *) routes_list_get_at(pEBGPRoutes, 0));
  LOG_DEBUG("\tnew-external-best: ");
  LOG_ENABLED_DEBUG() route_dump(log_get_stream(pMainLog), pEBGPRoute);
  LOG_DEBUG("\n");

  if ((pOldEBGPRoute == NULL) || 
    !route_equals(pOldEBGPRoute, pEBGPRoute)) {

    if (pOldEBGPRoute != NULL)
      LOG_DEBUG("\t*** UPDATED EXTERNAL BEST ROUTE ***\n");
    else
      LOG_DEBUG("\t*** NEW EXTERNAL BEST ROUTE ***\n");

    if (pOldEBGPRoute != NULL)
      route_flag_set(pOldEBGPRoute, ROUTE_FLAG_EXTERNAL_BEST, 0);

    route_flag_set(pEBGPRoute, ROUTE_FLAG_EXTERNAL_BEST, 1);
    route_flag_set(pEBGPRoutes->data[0], ROUTE_FLAG_EXTERNAL_BEST, 1);

    bgp_router_decision_process_disseminate_external_best(pRouter, sPrefix, pEBGPRoute);
  } else {
    route_destroy(&pEBGPRoute);
  }

}

SRoute * bgp_router_get_external_best(SRoutes * pRoutes)
{
  int iIndex;

  for (iIndex = 0; iIndex < routes_list_get_num(pRoutes); iIndex++) {
    if (route_flag_get(routes_list_get_at(pRoutes, iIndex), ROUTE_FLAG_EXTERNAL_BEST))
      return route_copy(routes_list_get_at(pRoutes, iIndex));
  }
  return NULL;
}
#endif

// ----- bgp_router_decision_process --------------------------------
/**
 * Phase I - Calculate degree of preference (LOCAL_PREF) for each
 *           single route. Operate on separate Adj-RIB-Ins.
 *           This phase is carried by 'peer_handle_message' (peer.c).
 *
 * Phase II - Selection of best route on the basis of the degree of
 *            preference and then on tie-breaking rules (AS-Path
 *            length, Origin, MED, ...).
 *
 * Phase III - Dissemination of routes.
 *
 * In our implementation, we distinguish two main cases:
 * - a withdraw has been received from a peer for a given prefix. In
 * this case, if the best route towards this prefix was received by
 * the given peer, the complete decision process has to be
 * run. Otherwise, nothing more is done (the route has been removed
 * from the peer's Adj-RIB-In by 'peer_handle_message');
 * - an update has been received. The complete decision process has to
 * be run.
 */
int bgp_router_decision_process(SBGPRouter * pRouter, SPeer * pOriginPeer,
				SPrefix sPrefix)
{
  SRoutes * pRoutes;
  int iIndex;
  SRoute * pRoute, * pOldRoute;
  int iRank;

#ifdef __EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
  SRoutes * pEBGPRoutes= NULL;
  SRoute * pOldEBGPRoute= NULL, * pEBGPRoute= NULL;
  int iRankEBGP;
#endif

  /* BGP QoS decision process */
#ifdef BGP_QOS
  if (BGP_OPTIONS_NLRI == BGP_NLRI_QOS_DELAY)
    return qos_decision_process(pRouter, pOriginPeer, sPrefix);
#endif

  LOG_DEBUG("----------------------------------------"
	    "---------------------------------------\n");
  LOG_DEBUG("DECISION PROCESS for ");
  LOG_ENABLED_DEBUG() ip_prefix_dump(log_get_stream(pMainLog), sPrefix);
  LOG_DEBUG(" in ");
  LOG_ENABLED_DEBUG() bgp_router_dump_id(log_get_stream(pMainLog), pRouter);
  LOG_DEBUG("\n");

#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  pOldRoute= rib_find_one_exact(pRouter->pLocRIB, sPrefix);
#else
  pOldRoute= rib_find_exact(pRouter->pLocRIB, sPrefix);
#endif
  LOG_DEBUG("\told-best: ");
  LOG_ENABLED_DEBUG() route_dump(log_get_stream(pMainLog), pOldRoute);
  LOG_DEBUG("\n");

  // Local routes can not be overriden and must be kept in Loc-RIB.
  // Decision process stops here in this case.
  if ((pOldRoute != NULL) &&
      route_flag_get(pOldRoute, ROUTE_FLAG_INTERNAL)) {
    return 0;
  }

  // *** lock all Adj-RIB-Ins ***

  /* Build list of eligible routes */
#ifdef __EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
  pRoutes= bgp_router_get_feasible_routes(pRouter, sPrefix, 0);

  //Is this route also the best EBGP route?
  if (BGP_OPTIONS_ADVERTISE_EXTERNAL_BEST) {
    //If the old best route also was the best external route, it has not been
    //announced as a best external route but as the main best route!
    if ((pOldRoute != NULL) && 
	route_flag_get(pOldRoute, ROUTE_FLAG_EXTERNAL_BEST))
      pOldEBGPRoute = NULL;
    else
      pOldEBGPRoute = bgp_router_get_external_best(pRoutes);
    LOG_DEBUG("\told-external-best: ");
    LOG_ENABLED_DEBUG() route_dump(log_get_stream(pMainLog), pOldEBGPRoute);
    LOG_DEBUG("\n");
  }

#else
  pRoutes= bgp_router_get_feasible_routes(pRouter, sPrefix);
#endif

  /* Reset DP_IGP flag, log eligibles */
  for (iIndex= 0; iIndex < routes_list_get_num(pRoutes); iIndex++) {
    pRoute= (SRoute *) routes_list_get_at(pRoutes, iIndex);

    /* Clear flag that indicates that the route depends on the
       IGP. See 'dp_rule_nearest_next_hop' and 'bgp_router_scan_rib'
       for more information. */
    route_flag_set(pRoute, ROUTE_FLAG_DP_IGP, 0);

    /* Log eligible route */
    LOG_DEBUG("\teligible: ");
    LOG_ENABLED_DEBUG() route_dump(log_get_stream(pMainLog), pRoute);
    LOG_DEBUG("\n");
  }

  // If there is a single eligible & feasible route, it depends on the
  // IGP (see 'dp_rule_nearest_next_hop' and 'bgp_router_scan_rib')
  // for more information.
  if (ptr_array_length(pRoutes) == 1)
    route_flag_set((SRoute *) routes_list_get_at(pRoutes, 0), ROUTE_FLAG_DP_IGP, 1);

  // Compare eligible routeS
  iRank= 0;
  if (ptr_array_length(pRoutes) > 1)
    iRank= bgp_router_decision_process_run(pRouter, pRoutes);
  assert((ptr_array_length(pRoutes) == 0) ||
	 (ptr_array_length(pRoutes) == 1));


  // If one best-route has been selected
  if (ptr_array_length(pRoutes) > 0) {
    pRoute= route_copy((SRoute *) routes_list_get_at(pRoutes, 0));

#ifdef __EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
    if (BGP_OPTIONS_ADVERTISE_EXTERNAL_BEST) {
      //If the Best route selected is not an EBGP one, run the decision process
      //against the set of EBGP routes.
      if (pRoute->pPeer->uRemoteAS == pRouter->uNumber)
	pEBGPRoutes = bgp_router_get_feasible_routes(pRouter, sPrefix, 1);
      else
	route_flag_set(pRoute, ROUTE_FLAG_EXTERNAL_BEST, 1);
    
      if (pEBGPRoutes != NULL) {
	for (iRank = 0; iRank < routes_list_get_num(pEBGPRoutes); iRank++){
	  LOG_DEBUG("\teligible external : ");
	  LOG_ENABLED_DEBUG() route_dump(log_get_stream(pMainLog), pEBGPRoutes->data[iRank]);
	  LOG_DEBUG("\n");
      }
	if (routes_list_get_num(pEBGPRoutes) > 1) {
	  iRankEBGP = bgp_router_decision_process_run(pRouter, pEBGPRoutes);
	}
	assert((ptr_array_length(pEBGPRoutes) == 0) ||
	   (ptr_array_length(pEBGPRoutes) == 1));
      }
    }
#endif

    LOG_DEBUG("\tnew-best: ");
    LOG_ENABLED_DEBUG() route_dump(log_get_stream(pMainLog), pRoute);
    LOG_DEBUG("\n");

    // New/updated route: install in Loc-RIB & advertise to peers
    if ((pOldRoute == NULL) ||
	!route_equals(pOldRoute, pRoute)) {

      /**************************************************************
       * In this case, the route is new or one of its BGP attributes
       * has changed. Thus, we must update the route into the routing
       * table and we must propagate the BGP route to the peers.
       **************************************************************/

      if (pOldRoute != NULL)
	LOG_DEBUG("\t*** UPDATED BEST ROUTE ***\n");
      else
	LOG_DEBUG("\t*** NEW BEST ROUTE ***\n");

     if (pOldRoute != NULL) 
	bgp_router_best_flag_off(pOldRoute);

      /* Mark route in Loc-RIB and Adj-RIB-In as best. This must be
	 done after the call to 'bgp_router_best_flag_off'. */
      route_flag_set(pRoute, ROUTE_FLAG_BEST, 1);
      route_flag_set(routes_list_get_at(pRoutes, 0), ROUTE_FLAG_BEST, 1);

#ifdef __BGP_ROUTE_INFO_DP__
      pRoute->tRank= (uint8_t) iRank;
      ((SRoute *) pRoutes->data[0])->tRank= (uint8_t) iRank;
#endif

      /* Insert in Loc-RIB */
      assert(rib_add_route(pRouter->pLocRIB, pRoute) == 0);

      /* Insert in the node's routing table */
      bgp_router_rt_add_route(pRouter, pRoute);

      bgp_router_decision_process_disseminate(pRouter, sPrefix, pRoute);

#ifdef __EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
      if (pEBGPRoutes != NULL && routes_list_get_num(pEBGPRoutes) > 0) {
	pEBGPRoute = routes_list_get_at(pEBGPRoutes, 0);
	bgp_router_check_dissemination_external_best(pRouter, pEBGPRoutes, pOldEBGPRoute, pEBGPRoute, sPrefix);
      }
#endif

    } else {

      /**************************************************************
       * In this case, both routes (old and new) are equal from the
       * BGP point of view. There is thus no need to send BGP
       * messages.
       *
       * However, it is possible that the IGP next-hop of the route
       * has changed. If so, the route must be updated in the routing
       * table!
       **************************************************************/

      LOG_DEBUG("\t*** BEST ROUTE UNCHANGED ***\n");

      /* Mark route in Adj-RIB-In as best (since it has probably been
	 replaced). */
      route_flag_set(routes_list_get_at(pRoutes, 0), ROUTE_FLAG_BEST, 1);

      /* Update ROUTE_FLAG_DP_IGP of old route */
      route_flag_set(pOldRoute, ROUTE_FLAG_DP_IGP,
		     route_flag_get(pRoute, ROUTE_FLAG_DP_IGP));

#ifdef __BGP_ROUTE_INFO_DP__
      pOldRoute->tRank= (uint8_t) iRank;
      ((SRoute *) pRoutes->data[0])->tRank= (uint8_t) iRank;
#endif

      /* If the IGP next-hop has changed, we need to re-insert the
	 route into the routing table with the new next-hop. */
      bgp_router_rt_add_route(pRouter, pRoute);

      /* Route has not changed. */
      route_destroy(&pRoute);
      pRoute= pOldRoute;

#ifdef __EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
      if (pEBGPRoutes != NULL && routes_list_get_num(pEBGPRoutes) > 0) {
	pEBGPRoute = routes_list_get_at(pEBGPRoutes, 0);
	bgp_router_check_dissemination_external_best(pRouter, pEBGPRoutes, pOldEBGPRoute, pEBGPRoute, sPrefix);
      }
#endif

    }

  } else {

    LOG_DEBUG("\t*** NO BEST ROUTE ***\n");

    // If a route towards this prefix was previously installed, then
    // withdraw it. Otherwise, do nothing...
    if (pOldRoute != NULL) {
#ifdef __EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
      if (BGP_OPTIONS_ADVERTISE_EXTERNAL_BEST && pOldEBGPRoute != NULL) {
	route_flag_set(pOldEBGPRoute, ROUTE_FLAG_EXTERNAL_BEST, 0);
      }
#endif
      rib_remove_route(pRouter->pLocRIB, sPrefix);
      bgp_router_best_flag_off(pOldRoute);
      bgp_router_rt_del_route(pRouter, sPrefix);
      bgp_router_decision_process_disseminate(pRouter, sPrefix, NULL);
    }
  }
  

  // *** unlock all Adj-RIB-Ins ***

  routes_list_destroy(&pRoutes);


#ifdef __EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
  routes_list_destroy(&pEBGPRoutes);
#endif

  LOG_DEBUG("----------------------------------------"
	    "---------------------------------------\n");

  return 0;
}

// ----- bgp_router_handle_message ----------------------------------
/**
 * Handle a BGP message received from the lower layer (network layer
 * at this time). The function looks for the destination peer and
 * delivers it for processing...
 */
int bgp_router_handle_message(void * pHandler, SNetMessage * pMessage)
{
  SBGPRouter * pRouter= (SBGPRouter *) pHandler;
  SBGPMsg * pMsg= (SBGPMsg *) pMessage->pPayLoad;
  SPeer * pPeer;

  if ((pPeer= bgp_router_find_peer(pRouter, pMessage->tSrcAddr)) != NULL) {
    bgp_peer_handle_message(pPeer, pMsg);
  } else {
    LOG_WARNING("WARNING: BGP message received from unknown peer !\n");
    LOG_WARNING("WARNING:   destination=");
    LOG_ENABLED_WARNING()
      bgp_router_dump_id(log_get_stream(pMainLog), pRouter);
    LOG_WARNING("\n");
    LOG_WARNING("WARNING:   source=");
    LOG_ENABLED_WARNING()
      ip_address_dump(log_get_stream(pMainLog), pMessage->tSrcAddr);
    LOG_WARNING("\n");
    bgp_msg_destroy(&pMsg);
    return -1;
  }
  return 0;
}

// ----- bgp_router_num_providers -----------------------------------
/**
 *
 */
uint16_t bgp_router_num_providers(SBGPRouter * pRouter) {
  int iIndex;
  uint16_t uNumProviders= 0;

  for (iIndex= 0; iIndex < ptr_array_length(pRouter->pPeers); iIndex++)
    if (((SPeer *) pRouter->pPeers->data[iIndex])->uPeerType ==
	PEER_TYPE_PROVIDER)
      uNumProviders++;
  return uNumProviders;
}

// ----- bgp_router_dump_id -------------------------------------------------
/**
 *
 */
void bgp_router_dump_id(FILE * pStream, SBGPRouter * pRouter)
{
  fprintf(pStream, "AS%d:", pRouter->uNumber);
  ip_address_dump(pStream, pRouter->pNode->tAddr);
}

typedef struct {
  SBGPRouter * pRouter;
  SArray * pPrefixes;
} SScanRIBCtx;

// ----- bgp_router_scan_rib_for_each -------------------------------
/**
 * This function is a helper function for 'bgp_router_scan_rib'. This
 * function checks if the best route used to reach a given prefix must
 * be updated due to an IGP change.
 *
 * The function works in two step:
 * (1) checks if the current best route was selected based on the IGP
 *     (i.e. the route passed the IGP rule) and checks if its next-hop
 *     is still reachable and if not, goes to step (2) otherwise,
 *     the function returns
 *
 * (2) looks in the Adj-RIB-Ins for a prefix whose IGP cost is lower
 *     than these of the current best route. If one is found, run the
 *     decision process
 *
 * The function does not run the BGP decision process itself but add
 * to a list the prefixes for which the decision process must be run.
 */
int bgp_router_scan_rib_for_each(uint32_t uKey, uint8_t uKeyLen,
				 void * pItem, void * pContext)
{
  SScanRIBCtx * pCtx= (SScanRIBCtx *) pContext;
  SBGPRouter * pRouter= pCtx->pRouter;
  SPrefix sPrefix;
  SRoute * pRoute;
  int iIndex;
  SPeer * pPeer;
  unsigned int uBestWeight;
  SNetRouteInfo * pRouteInfo;
  SNetRouteInfo * pCurRouteInfo;

#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  uint16_t uIndexRoute;
  SRoutes * pRoutes;
#endif
  SRoute * pAdjRoute;

  sPrefix.tNetwork= uKey;
  sPrefix.uMaskLen= uKeyLen;

  /*
  fprintf(stderr, "SCAN PREFIX ");
  bgp_router_dump_id(stderr, pRouter);
  fprintf(stderr, " ");
  ip_prefix_dump(stderr, sPrefix);
  fprintf(stderr, "\n");
  */
  
  /* Looks up for the best BGP route towards this prefix. If the route
     does not exist, schedule the current prefix for the decision
     process */
#if defined __EXPERIMENTAL__ && __EXPERIMENTAL_WALTON__
  pRoute= rib_find_one_exact(pRouter->pLocRIB, sPrefix);
#else
  pRoute= rib_find_exact(pRouter->pLocRIB, sPrefix);
#endif
  if (pRoute == NULL) {
    _array_append(pCtx->pPrefixes, &sPrefix);
    return 0;
  } else {

    /* Check if the peer that has announced this route is down */
    if ((pRoute->pPeer != NULL) &&
	(pRoute->pPeer->uSessionState != SESSION_STATE_ESTABLISHED)) {
      fprintf(stdout, "*** SESSION NOT ESTABLISHED [");
      ip_prefix_dump(stdout, pRoute->sPrefix);
      fprintf(stdout, "] ***\n");
      _array_append(pCtx->pPrefixes, &sPrefix);
      return 0;
    }
    
    pRouteInfo= rt_find_best(pRouter->pNode->pRT, pRoute->tNextHop,
			     NET_ROUTE_ANY);    
    
    /* Check if the IP next-hop of the BGP route has changed. Indeed,
       the BGP next-hop is not always the IP next-hop. If this next-hop
       has changed, the decision process must be run. */
    if (pRouteInfo != NULL) { 
      pCurRouteInfo= rt_find_exact(pRouter->pNode->pRT,
				   pRoute->sPrefix, NET_ROUTE_BGP);
      assert(pCurRouteInfo != NULL);
      
      if (route_nexthop_compare(pCurRouteInfo->sNextHop,
				pRouteInfo->sNextHop)) {
	_array_append(pCtx->pPrefixes, &sPrefix);
	//fprintf(stderr, "NEXT-HOP HAS CHANGED\n");
	return 0;
      }
    }

    /* If the best route has been chosen based on the IGP weight, then
       there is a possible BGP impact */
    if (route_flag_get(pRoute, ROUTE_FLAG_DP_IGP)) {

      /* Is there a route (IGP ?) towards the destination ? */
      if (pRouteInfo == NULL) {
	/* The next-hop of the best route is now unreachable, thus
	   trigger the decision process */
	_array_append(pCtx->pPrefixes, &sPrefix);
	return 0;	
      }

      uBestWeight= pRouteInfo->uWeight;

      /* Lookup in the Adj-RIB-Ins for routes that were also selected
	 based on the IGP, that is routes that were compared to the
	 current best route based on the IGP cost to the next-hop. These
	 routes are marked with the ROUTE_FLAG_DP_IGP flag */
      for (iIndex= 0; iIndex < ptr_array_length(pRouter->pPeers);
	   iIndex++) {

	pPeer= (SPeer *) pRouter->pPeers->data[iIndex];
	
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
	pRoutes= rib_find_exact(pPeer->pAdjRIBIn, pRoute->sPrefix);
	for (uIndexRoute = 0; uIndexRoute < routes_list_get_num(pRoutes); uIndexRoute++) {
	  pAdjRoute = routes_list_get_at(pRoutes, uIndexRoute);
#else
	pAdjRoute= rib_find_exact(pPeer->pAdjRIBIn, pRoute->sPrefix);
#endif
		
	if (pAdjRoute != NULL) {
	  
	  /* Is there a route (IGP ?) towards the destination ? */
	  pRouteInfo= rt_find_best(pRouter->pNode->pRT,
				   pAdjRoute->tNextHop,
				   NET_ROUTE_ANY);
	  
	  /* Three cases are now possible:
	     (1) route becomes reachable => run DP
	     (2) route no more reachable => do nothing (becoz the route is
	     not the best
	     (3) IGP cost is below cost of the best route => run DP
	  */
	  if ((pRouteInfo != NULL) &&
	      !route_flag_get(pAdjRoute, ROUTE_FLAG_FEASIBLE)) {
	    /* The next-hop was not reachable (route unfeasible) and is
	       now reachable, thus run the decision process */
	    _array_append(pCtx->pPrefixes, &sPrefix);
	    return 0;
	    
	  } else if (pRouteInfo != NULL) {
	    
	    /* IGP cost is below cost of the best route, thus run the
	       decision process */
	    if (pRouteInfo->uWeight < uBestWeight) {
	      _array_append(pCtx->pPrefixes, &sPrefix);
	      return 0;
	    }	
	    
	  }
	  
	}
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
	}
#endif
	
      }
      
    }
  }
    
  return 0;
}

// -----[ bgp_router_prefixes_for_each ]-----------------------------
/**
 *
 */
int bgp_router_prefixes_for_each(uint32_t uKey, uint8_t uKeyLen,
				 void * pItem, void * pContext)
{
  SRadixTree * pPrefixes= (SRadixTree *) pContext;
  SRoute * pRoute= (SRoute *) pItem;

  radix_tree_add(pPrefixes, pRoute->sPrefix.tNetwork,
		 pRoute->sPrefix.uMaskLen, (void *) 1);

  return 0;
}

// -----[ _bgp_router_alloc_prefixes ]-------------------------------
static void _bgp_router_alloc_prefixes(SRadixTree ** ppPrefixes)
{
  /* If list (radix-tree) is not allocated, create it now. The
     radix-tree is created without destroy function and acts thus as a
     list of references. */
  if (*ppPrefixes == NULL) {
    *ppPrefixes= radix_tree_create(32, NULL);
  }
}

// -----[ _bgp_router_free_prefixes ]--------------------------------
static void _bgp_router_free_prefixes(SRadixTree ** ppPrefixes)
{
  if (*ppPrefixes != NULL) {
    radix_tree_destroy(ppPrefixes);
    *ppPrefixes= NULL;
  }
}

// -----[ _bgp_router_get_peer_prefixes ]----------------------------
/**
 * This function builds a list of all prefixes received from this
 * peer (in Adj-RIB-in). The list of prefixes is implemented as a
 * radix-tree in order to guarantee that each prefix will be present
 * at most one time (uniqueness).
 */
static int _bgp_router_get_peer_prefixes(SBGPRouter * pRouter,
					 SPeer * pPeer,
					 SRadixTree ** ppPrefixes)
{
  int iIndex;
  int iResult= 0;

  _bgp_router_alloc_prefixes(ppPrefixes);

  if (pPeer != NULL) {
    iResult= rib_for_each(pPeer->pAdjRIBIn, bgp_router_prefixes_for_each,
			  *ppPrefixes);
  } else {
    for (iIndex= 0; iIndex < ptr_array_length(pRouter->pPeers); iIndex++) {
      pPeer= (SPeer *) pRouter->pPeers->data[iIndex];
      iResult= rib_for_each(pPeer->pAdjRIBIn, bgp_router_prefixes_for_each,
			    *ppPrefixes);
      if (iResult != 0)
	break;
    }
  }

  return iResult;
}

// -----[ _bgp_router_get_local_prefixes ]---------------------------
static int _bgp_router_get_local_prefixes(SBGPRouter * pRouter,
					  SRadixTree ** ppPrefixes)
{
  _bgp_router_alloc_prefixes(ppPrefixes);
  
  return rib_for_each(pRouter->pLocRIB, bgp_router_prefixes_for_each,
		      *ppPrefixes);
}

// -----[ _bgp_router_prefixes ]-------------------------------------
/**
 * This function builds a list of all the prefixes known in this
 * router (in Adj-RIB-Ins and Loc-RIB).
 */
SRadixTree * _bgp_router_prefixes(SBGPRouter * pRouter)
{
  SRadixTree * pPrefixes= NULL;

  /* Get prefixes from all Adj-RIB-Ins */
  _bgp_router_get_peer_prefixes(pRouter, NULL, &pPrefixes);

  /* Get prefixes from the RIB */
  _bgp_router_get_local_prefixes(pRouter, &pPrefixes);

  return pPrefixes;
}

// ----- _bgp_router_refresh_sessions -------------------------------
/*
 * This function scans the peering sessions and checks that the peer
 * router is still reachable. If it is not, the sessions is teared
 * down.
 */
static void _bgp_router_refresh_sessions(SBGPRouter * pRouter)
{
  int iIndex;

  for (iIndex= 0; iIndex < ptr_array_length(pRouter->pPeers); iIndex++) {
    bgp_peer_session_refresh((SBGPPeer *) pRouter->pPeers->data[iIndex]);
  }
}

// ----- bgp_router_scan_rib ----------------------------------------
/**
 * This function scans the RIB of the BGP router in order to find
 * routes for which the distance to the next-hop has changed. For each
 * route that has changed, the decision process is re-run.
 */
int bgp_router_scan_rib(SBGPRouter * pRouter)
{
  SScanRIBCtx sCtx;
  int iIndex;
  int iResult;
  SRadixTree * pPrefixes;
  SPrefix sPrefix;

  /* Scan peering sessions */
  _bgp_router_refresh_sessions(pRouter);

  /* initialize context */
  sCtx.pRouter= pRouter;
  sCtx.pPrefixes= _array_create(sizeof(SPrefix), 0, NULL, NULL);

  /* Build a list of all available prefixes in this router */
  pPrefixes= _bgp_router_prefixes(pRouter);

  /* Traverses the whole Loc-RIB in order to find prefixes that depend
     on the IGP (links up/down and metric changes) */
  iResult= radix_tree_for_each(pPrefixes,
			       bgp_router_scan_rib_for_each,
			       &sCtx);

  /* For each route in the list, run the BGP decision process */
  if (iResult == 0)
    for (iIndex= 0; iIndex < _array_length(sCtx.pPrefixes); iIndex++) {
      _array_get_at(sCtx.pPrefixes, iIndex, &sPrefix);
      bgp_router_decision_process(pRouter, NULL, sPrefix);
    }

  _bgp_router_free_prefixes(&pPrefixes);
    
  _array_destroy(&sCtx.pPrefixes);
  
  return iResult;
}

// -----[ _bgp_router_rerun_for_each ]-------------------------------
static int _bgp_router_rerun_for_each(uint32_t uKey, uint8_t uKeyLen,
				      void * pItem, void * pContext)
{
  SBGPRouter * pRouter= (SBGPRouter *) pContext;
  SPrefix sPrefix;

  sPrefix.tNetwork= uKey;
  sPrefix.uMaskLen= uKeyLen;  

  printf("decision-process [");
  ip_prefix_dump(stdout, sPrefix);
  printf("]\n");
  fflush(stdout);

  return bgp_router_decision_process(pRouter, NULL, sPrefix);
}

// -----[ bgp_router_rerun ]-----------------------------------------
/**
 * Rerun the decision process for the given prefixes. If the length of
 * the prefix mask is 0, rerun for all known prefixes (from Adj-RIB-In
 * and Loc-RIB). Otherwize, only rerun for the specified prefix.
 */
int bgp_router_rerun(SBGPRouter * pRouter, SPrefix sPrefix)
{
  int iResult;
  SRadixTree * pPrefixes= NULL;

  printf("rerun [");
  ip_address_dump(stdout, pRouter->pNode->tAddr);
  printf("]\n");
  fflush(stdout);

  _bgp_router_alloc_prefixes(&pPrefixes);

  /* Populate with all prefixes */
  if (sPrefix.uMaskLen == 0) {
    assert(!_bgp_router_get_peer_prefixes(pRouter, NULL, &pPrefixes));
    assert(!_bgp_router_get_local_prefixes(pRouter, &pPrefixes));
  } else {
    radix_tree_add(pPrefixes, sPrefix.tNetwork,
		   sPrefix.uMaskLen, (void *) 1);
  }

  printf("prefix-list ok\n");
  fflush(stdout);

  /* For each route in the list, run the BGP decision process */
  iResult= radix_tree_for_each(pPrefixes, _bgp_router_rerun_for_each, pRouter);

  /* Free list of prefixes */
  _bgp_router_free_prefixes(&pPrefixes);

  return iResult;
}

// -----[ bgp_router_peer_readv_prefix ]-----------------------------
/**
 *
 */
int bgp_router_peer_readv_prefix(SBGPRouter * pRouter, SPeer * pPeer,
				 SPrefix sPrefix)
{
  SRoute * pRoute;

  if (sPrefix.uMaskLen == 0) {
    LOG_WARNING("Error: not yet implemented\n");
    return -1;
  } else {
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
    pRoute= rib_find_one_exact(pRouter->pLocRIB, sPrefix);
#else
    pRoute= rib_find_exact(pRouter->pLocRIB, sPrefix);
#endif
    if (pRoute != NULL) {
      bgp_router_decision_process_disseminate_to_peer(pRouter, sPrefix,
						      pRoute, pPeer);
    }
  }

  return 0;
}

/////////////////////////////////////////////////////////////////////
// DUMP FUNCTIONS
/////////////////////////////////////////////////////////////////////

// ----- bgp_router_dump_networks -----------------------------------
/**
 *
 */
void bgp_router_dump_networks(FILE * pStream, SBGPRouter * pRouter)
{
  int iIndex;

  for (iIndex= 0; iIndex < ptr_array_length(pRouter->pLocalNetworks);
       iIndex++) {
    route_dump(pStream, (SRoute *) pRouter->pLocalNetworks->data[iIndex]);
    fprintf(pStream, "\n");
  }

  flushir(pStream);
}

// ----- bgp_router_dump_peers --------------------------------------
/**
 *
 */
void bgp_router_dump_peers(FILE * pStream, SBGPRouter * pRouter)
{
  int iIndex;

  for (iIndex= 0; iIndex < ptr_array_length(pRouter->pPeers); iIndex++) {
    bgp_peer_dump(pStream, (SPeer *) pRouter->pPeers->data[iIndex]);
    fprintf(pStream, "\n");
  }

  flushir(pStream);
}

// ----- bgp_router_peers_for_each ----------------------------------
/**
 *
 */
int bgp_router_peers_for_each(SBGPRouter * pRouter, FArrayForEach fForEach,
			      void * pContext)
{
  return _array_for_each((SArray *) pRouter->pPeers, fForEach, pContext);
}

typedef struct {
  SBGPRouter * pRouter;
  FILE * pStream;
  char * cDump;
} SRouteDumpCtx;

// ----- bgp_router_dump_route --------------------------------------
/**
 *
 */
int bgp_router_dump_route(uint32_t uKey, uint8_t uKeyLen,
			  void * pItem, void * pContext)
{
  SRouteDumpCtx * pCtx= (SRouteDumpCtx *) pContext;

  route_dump(pCtx->pStream, (SRoute *) pItem);
  fprintf(pCtx->pStream, "\n");

  flushir(pCtx->pStream);

  return 0;
}

// ----- bgp_router_dump_route_string --------------------------------------
/**
 *
 */
int bgp_router_dump_route_string(uint32_t uKey, uint8_t uKeyLen,
			  void * pItem, void * pContext)
{
  SRouteDumpCtx * pCtx= (SRouteDumpCtx *) pContext;
  uint32_t iPtr;
  char * cCharTmp;

  if (pCtx->cDump == NULL) {
    pCtx->cDump = MALLOC(255);
    iPtr = 0;
  } else {
    iPtr = strlen(pCtx->cDump);
    iPtr += 255;
    pCtx->cDump = REALLOC(pCtx->cDump, iPtr);
  }

  cCharTmp = route_dump_string((SRoute *) pItem);
  strcpy((pCtx->cDump)+iPtr, cCharTmp);
  iPtr += strlen(cCharTmp);
  strcpy((pCtx->cDump)+iPtr, "\n");

  return 0;
}


// ----- bgp_router_dump_rib ----------------------------------------
/**
 *
 */
void bgp_router_dump_rib(FILE * pStream, SBGPRouter * pRouter)
{
  SRouteDumpCtx sCtx;
  sCtx.pRouter= pRouter;
  sCtx.pStream= pStream;
  rib_for_each(pRouter->pLocRIB, bgp_router_dump_route, &sCtx);

  flushir(pStream);
}

// ----- bgp_router_dump_rib_address --------------------------------
/**
 *
 */
void bgp_router_dump_rib_address(FILE * pStream, SBGPRouter * pRouter,
				 net_addr_t tAddr)
{
  SRoute * pRoute;
  SPrefix sPrefix;

  sPrefix.tNetwork= tAddr;
  sPrefix.uMaskLen= 32;
  pRoute= rib_find_best(pRouter->pLocRIB, sPrefix);
  if (pRoute != NULL) {
    route_dump(pStream, pRoute);
    fprintf(pStream, "\n");
  }

  flushir(pStream);
}

// ----- bgp_router_dump_rib_prefix ---------------------------------
/**
 *
 */
void bgp_router_dump_rib_prefix(FILE * pStream, SBGPRouter * pRouter,
				SPrefix sPrefix)
{
  SRoute * pRoute;

#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  pRoute= rib_find_one_exact(pRouter->pLocRIB, sPrefix);
#else
  pRoute= rib_find_exact(pRouter->pLocRIB, sPrefix);
#endif
  if (pRoute != NULL) {
    route_dump(pStream, pRoute);
    fprintf(pStream, "\n");
  }

  flushir(pStream);
}

// ----- bgp_router_dump_adjrib -------------------------------------
/**
 *
 */
void bgp_router_dump_adjrib(FILE * pStream, SBGPRouter * pRouter,
			    SPeer * pPeer, SPrefix sPrefix,
			    int iInOut)
{
  int iIndex;

  if (pPeer == NULL) {
    for (iIndex= 0; iIndex < ptr_array_length(pRouter->pPeers); iIndex++) {
      bgp_peer_dump_adjrib(pStream, (SPeer *)
			  pRouter->pPeers->data[iIndex], sPrefix, iInOut);
    }
  } else {
    bgp_peer_dump_adjrib(pStream, pPeer, sPrefix, iInOut);
  }

  flushir(pStream);
}

// ----- bgp_router_info --------------------------------------------
void bgp_router_info(FILE * pStream, SBGPRouter * pRouter)
{
  fprintf(pStream, "router-id : ");
  ip_address_dump(stdout, pRouter->tRouterID);
  fprintf(pStream, "\n");
  fprintf(pStream, "as-number : %u\n", pRouter->uNumber);
  fprintf(pStream, "cluster-id: ");
  ip_address_dump(pStream, pRouter->tClusterID);
  fprintf(pStream, "\n");
  if (pRouter->pNode->pcName != NULL)
    fprintf(pStream, "name      : %s\n", pRouter->pNode->pcName);
  flushir(pStream);
}

// -----[ bgp_router_show_route_info ]-------------------------------
/**
 * Show information about a best BGP route from the Loc-RIB of this
 * router:
 * - decision-process-rule: rule of the decision process that was
 *   used to select this route as best.
 */
int bgp_router_show_route_info(FILE * pStream, SBGPRouter * pRouter,
				SPrefix sPrefix)
{
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  SRoute * pRoute= rib_find_one_exact(pRouter->pLocRIB, sPrefix);
#else
  SRoute * pRoute= rib_find_exact(pRouter->pLocRIB, sPrefix);
#endif
  if (pRoute == NULL)
    return -1;
#ifdef __BGP_ROUTE_INFO_DP__
  if (pRoute->tRank <= 0) {
    fprintf(pStream, "decision-process-rule: %d [ Single choice ]\n",
	    pRoute->tRank);
  } else if (pRoute->tRank >= DP_NUM_RULES) {
    fprintf(pStream, "decision-process-rule: %d [ Invalid rule ]\n",
	    pRoute->tRank);
  } else {
    fprintf(pStream, "decision-process-rule: %d [ %s ]\n",
	    pRoute->tRank, DP_RULE_NAME[pRoute->tRank-1]);
  }
#endif
  flushir(pStream);
  return 0;
}

/////////////////////////////////////////////////////////////////////
// LOAD/SAVE FUNCTIONS
/////////////////////////////////////////////////////////////////////

// ----- bgp_router_load_rib ----------------------------------------
/**
 * This function loads an existant BGP routing table into the given
 * bgp instance. The routes are considered local and will not be
 * replaced by routes received from peers. The routes are marked as
 * best and feasible and are directly installed into the Loc-RIB.
 */
int bgp_router_load_rib(char * pcFileName, SBGPRouter * pRouter)
{
  SRoutes * pRoutes= mrtd_ascii_load_routes(pRouter, pcFileName);
  SRoute * pRoute;
  int iIndex;

  if (pRoutes != NULL) {
    for (iIndex= 0; iIndex < routes_list_get_num(pRoutes); iIndex++) {
      pRoute= (SRoute *) pRoutes->data[iIndex];
      route_flag_set(pRoute, ROUTE_FLAG_FEASIBLE, 1);
      route_flag_set(pRoute, ROUTE_FLAG_ELIGIBLE,
		     bgp_peer_route_eligible(pRoute->pPeer, pRoute));
      route_flag_set(pRoute, ROUTE_FLAG_BEST,
		     bgp_peer_route_feasible(pRoute->pPeer, pRoute));
      // TODO: shouldn't we take into account the soft-restart flag ?
      // If the (virtual) session is down, we should still store the
      // received routes in the Adj-RIB-in, but not run the decision
      // process.
      rib_replace_route(pRoute->pPeer->pAdjRIBIn, pRoute);
      bgp_router_decision_process(pRouter, pRoute->pPeer, pRoute->sPrefix);
    }
    routes_list_destroy(&pRoutes);
  } else
    return -1;
  return 0;
}

#ifdef __EXPERIMENTAL__
int bgp_router_load_ribs_in(char * pcFileName, SBGPRouter * pRouter)
{
  SRoute * pRoute;
  SPeer * pPeer;
  //SNetNode * pNode;
  FILE * phFiRib;
  char acFileLine[1024];
  
  if ( (phFiRib = fopen(pcFileName, "r")) == NULL) {
    LOG_SEVERE("Error: rib File doest not exist\n");
    return -1;
  }

  while (!feof(phFiRib)) {
    if (fgets(acFileLine, sizeof(acFileLine), phFiRib) == NULL)
      break;

    if ( (pRoute = mrtd_route_from_line(pRouter, acFileLine)) != NULL ) {
      if (pRoute == NULL) {
	LOG_SEVERE("Error: could not build message from input\n%s\n", acFileLine);
	fclose(phFiRib);
	return -1;
      }

      if ( (pPeer = bgp_router_find_peer(pRouter, route_nexthop_get(pRoute))) != NULL) {
	if (bgp_peer_flag_get(pPeer, PEER_FLAG_VIRTUAL)) {
/*	    route_localpref_set(pRoute, BGP_OPTIONS_DEFAULT_LOCAL_PREF);
	    // Check route against import filter
	    route_flag_set(pRoute, ROUTE_FLAG_BEST, 0);
	    route_flag_set(pRoute, ROUTE_FLAG_INTERNAL, 0);
	    route_flag_set(pRoute, ROUTE_FLAG_ELIGIBLE,
		   peer_route_eligible(pPeer, pRoute));
	    route_flag_set(pRoute, ROUTE_FLAG_FEASIBLE,
		   peer_route_feasible(pPeer, pRoute));
	    // Update route delay attribute (if BGP-QoS)
	    //peer_route_delay_update(pPeer, pRoute);
	    // Update route RR-client flag
	    peer_route_rr_client_update(pPeer, pRoute);


	  rib_replace_route(pPeer->pAdjRIBIn, route_copy(pRoute));
	  bgp_router_decision_process(pPeer->pLocalRouter, pPeer,
				pRoute->sPrefix);*/


//	  pNode = network_find_node(pPeer->tAddr);
//	  bgp_msg_send(pNode, pRouter->pNode->tAddr, 
	  bgp_peer_handle_message(pPeer,
				  bgp_msg_update_create(pRouter->uNumber,
							pRoute));
	}
      }
    }
  }
  fclose(phFiRib);
  return 0;
}
#endif

// ----- bgp_router_save_route_mrtd ---------------------------------
/**
 *
 */
int bgp_router_save_route_mrtd(uint32_t uKey, uint8_t uKeyLen,
			       void * pItem, void * pContext)
{
  SRouteDumpCtx * pCtx= (SRouteDumpCtx *) pContext;

  fprintf(pCtx->pStream, "TABLE_DUMP|%u|", 0);
  route_dump_mrt(pCtx->pStream, (SRoute *) pItem);
  fprintf(pCtx->pStream, "\n");
  return 0;
}

// ----- bgp_router_save_rib ----------------------------------------
/**
 *
 */
int bgp_router_save_rib(char * pcFileName, SBGPRouter * pRouter)
{
  FILE * pFile;
  SRouteDumpCtx sCtx;

  pFile= fopen(pcFileName, "w");
  if (pFile == NULL)
    return -1;

  sCtx.pStream= pFile;
  sCtx.pRouter= pRouter;
  rib_for_each(pRouter->pLocRIB, bgp_router_save_route_mrtd, &sCtx);
  fclose(pFile);
  return 0;
}

// -----[ bgp_router_show_stats ]------------------------------------
/**
 * Show statistics about the BGP router:
 * - num-peers: number of peers.
 *
 * - num-prefixes/peer: number of prefixes received from each
 *   peer. For each peer, a line with the number of best routes and
 *   the number of non-selected routes is shown. 
 *
 * - rule-stats: number of best routes selected by each decision
 *   process rule. This is showned as a set of numbers. The first one
 *   gives the number of routes with no choice. The second one, the
 *   number of routes selected based on the LOCAL-PREF, etc.
 */
void bgp_router_show_stats(FILE * pStream, SBGPRouter * pRouter)
{
  int iIndex;
  SBGPPeer * pPeer;
  SEnumerator * pEnum;
  SRoute * pRoute;
  int iNumPrefixes, iNumBest;
  int aiNumPerRule[DP_NUM_RULES+1];
  int iRule;

  memset(&aiNumPerRule, 0, sizeof(aiNumPerRule));
  fprintf(pStream, "num-peers: %d\n",
	  ptr_array_length(pRouter->pPeers));
  fprintf(pStream, "num-networks: %d\n",
	  ptr_array_length(pRouter->pLocalNetworks));

  // Number of prefixes (best routes)
  iNumBest= 0;
  pEnum= trie_get_enum(pRouter->pLocRIB);
  while (enum_has_next(pEnum)) {
    pRoute= *(SRoute **) enum_get_next(pEnum);
    if (route_flag_get(pRoute, ROUTE_FLAG_BEST)) {
      iNumBest++;
      if (pRoute->tRank <= DP_NUM_RULES)
	aiNumPerRule[pRoute->tRank]++;
    }
  }
  enum_destroy(&pEnum);
  fprintf(pStream, "num-best: %d\n", iNumBest);

  // Classification of best route selections
  fprintf(pStream, "rule-stats:");
  for (iRule= 0; iRule <= DP_NUM_RULES; iRule++) {
    fprintf(pStream, " %d", aiNumPerRule[iRule]);
  }
  fprintf(pStream, "\n");

  // Number of best/non-best routes per peer
  fprintf(pStream, "num-prefixes/peer:\n");
  for (iIndex= 0; iIndex < ptr_array_length(pRouter->pPeers); iIndex++) {
    pPeer= (SPeer *) pRouter->pPeers->data[iIndex];
    iNumPrefixes= 0;
    iNumBest= 0;
    pEnum= trie_get_enum(pPeer->pAdjRIBIn);
    while (enum_has_next(pEnum)) {
      pRoute= *(SRoute **) enum_get_next(pEnum);
      iNumPrefixes++;
      if (route_flag_get(pRoute, ROUTE_FLAG_BEST)) {
	iNumBest++;
	if (pRoute->tRank <= DP_NUM_RULES)
	  aiNumPerRule[pRoute->tRank]++;
      }
    }
    enum_destroy(&pEnum);
    bgp_peer_dump_id(pStream, pPeer);
    fprintf(pStream, ": %d / %d\n",  iNumBest, iNumPrefixes);
  }
}


/////////////////////////////////////////////////////////////////////
// ROUTING TESTS
/////////////////////////////////////////////////////////////////////

// ----- bgp_router_record_route ------------------------------------
/**
 * This function records the AS-path from one BGP router towards a
 * given prefix. The function has two modes:
 * - records all ASes
 * - records ASes once (do not record iBGP session crossing)
 */
int bgp_router_record_route(SBGPRouter * pRouter,
			    SPrefix sPrefix, SBGPPath ** ppPath,
			    int iPreserveDups)
{
  SBGPRouter * pCurrentRouter= pRouter;
  SBGPRouter * pPreviousRouter= NULL;
  SRoute * pRoute;
  SBGPPath * pPath= path_create();
  SNetNode * pNode;
  SNetProtocol * pProtocol;
  int iResult= AS_RECORD_ROUTE_UNREACH;

  *ppPath= NULL;

  while (pCurrentRouter != NULL) {
    
    // Is there, in the current node, a BGP route towards the given
    // prefix ?
    pRoute= rib_find_best(pCurrentRouter->pLocRIB, sPrefix);
    if (pRoute != NULL) {
      
      // Record current node's AS-Num ??
      if ((pPreviousRouter == NULL) ||
	  (iPreserveDups ||
	   (pPreviousRouter->uNumber != pCurrentRouter->uNumber))) {
	if (path_append(&pPath, pCurrentRouter->uNumber)) {
	  iResult= AS_RECORD_ROUTE_TOO_LONG;
	  break;
	}
      }
      
      // If the route's next-hop is this router, then the function
      // terminates.
      if (pRoute->tNextHop == pCurrentRouter->pNode->tAddr) {
	iResult= AS_RECORD_ROUTE_SUCCESS;
	break;
      }
      
      // Otherwize, looks for next-hop router
      pNode= network_find_node(pRoute->tNextHop);
      if (pNode == NULL)
	break;
      
      // Get the current node's BGP instance
      pProtocol= protocols_get(pNode->pProtocols, NET_PROTOCOL_BGP);
      if (pProtocol == NULL)
	break;
      pPreviousRouter= pCurrentRouter;
      pCurrentRouter= (SBGPRouter *) pProtocol->pHandler;
      
    } else
      break;
  }
  *ppPath= pPath;

  return iResult;
}

// ----- bgp_router_dump_recorded_route -----------------------------
/**
 * This function dumps the result of a call to
 * 'bgp_router_record_route'.
 */
void bgp_router_dump_recorded_route(FILE * pStream,
				    SBGPRouter * pRouter,
				    SPrefix sPrefix,
				    SBGPPath * pPath,
				    int iResult)
{
  // Display record-route results
  ip_address_dump(pStream, pRouter->pNode->tAddr);
  fprintf(pStream, "\t");
  ip_prefix_dump(pStream, sPrefix);
  fprintf(pStream, "\t");
  switch (iResult) {
  case AS_RECORD_ROUTE_SUCCESS: fprintf(pStream, "SUCCESS"); break;
  case AS_RECORD_ROUTE_TOO_LONG: fprintf(pStream, "TOO_LONG"); break;
  case AS_RECORD_ROUTE_UNREACH: fprintf(pStream, "UNREACHABLE"); break;
  default:
    fprintf(pStream, "UNKNOWN_ERROR");
  }
  fprintf(pStream, "\t");
  path_dump(pStream, pPath, 0);
  fprintf(pStream, "\n");

  flushir(pStream);
}

// ----- bgp_router_bm_route ----------------------------------------
/**
 * *** EXPERIMENTAL ***
 *
 * Returns the best route among the routes in the Loc-RIB that match
 * the given prefix with a bound on the prefix length.
 */
#ifdef __EXPERIMENTAL__
SRoute * bgp_router_bm_route(SBGPRouter * pRouter, SPrefix sPrefix,
			     uint8_t uBound)
{
  SRoutes * pRoutes;
  SPrefix sBoundedPrefix;
  SRoute * pRoute= NULL;
  int iLocalExists= 0;

  sBoundedPrefix.tNetwork= sPrefix.tNetwork;

  pRoutes= routes_list_create(ROUTES_LIST_OPTION_REF);

  /* Select routes that match the prefix with a bound on the prefix
     length */
  while (uBound <= sPrefix.uMaskLen) {
    sBoundedPrefix.uMaskLen= uBound;
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
    pRoute= rib_find_one_exact(pRouter->pLocRIB, sBoundedPrefix);
#else
    pRoute= rib_find_exact(pRouter->pLocRIB, sBoundedPrefix);
#endif

    if (pRoute != NULL) {
      /*
	fprintf(stdout, "AVAIL: ");
	route_dump(stdout, pRoute);
	fprintf(stdout, "\n");
      */
      routes_list_append(pRoutes, pRoute);
      
      /* If the route towards the more specific prefix is local, this
	 is the best */
      if (route_flag_get(pRoute, ROUTE_FLAG_INTERNAL)/* &&
							(pRoute->sPrefix.uMaskLen == sPrefix.uMaskLen)*/) {
	iLocalExists= 1;
	break;
      }
    }

    uBound++;
  }

  /* BGP-DP: prefer local routes towards the more specific prefix */
  if (iLocalExists) {
    

  } else {

    /* BGP-DP: other rules */
    if (routes_list_get_num(pRoutes) > 0)
      bgp_router_decision_process_run(pRouter, pRoutes);

    /* Is there a single route chosen ? */
    if (routes_list_get_num(pRoutes) >= 1) {
      pRoute= (SRoute *) pRoutes->data[0];

      /*
	fprintf(stdout, "BEST: ");
	route_dump(stdout, pRoute);
	fprintf(stdout, "\n");
      */
      
    } else
      pRoute= NULL;

  }

  routes_list_destroy(&pRoutes);

  return pRoute;
}
#endif

// ----- bgp_router_record_route_bounded_match ----------------------
/**
 * *** EXPERIMENTAL ***
 *
 * This function records the AS-path from one BGP router towards a
 * given prefix. The function has two modes:
 * - records all ASes
 * - records ASes once (do not record iBGP session crossing)
 */
#ifdef __EXPERIMENTAL__
int bgp_router_record_route_bounded_match(SBGPRouter * pRouter,
					  SPrefix sPrefix,
					  uint8_t uBound,
					  SBGPPath ** ppPath,
					  int iPreserveDups)
{
  SBGPRouter * pCurrentRouter= pRouter;
  SBGPRouter * pPreviousRouter= NULL;
  SRoute * pRoute;
  SBGPPath * pPath= path_create();
  SNetNode * pNode;
  SNetProtocol * pProtocol;
  int iResult= AS_RECORD_ROUTE_UNREACH;

  *ppPath= NULL;

  while (pCurrentRouter != NULL) {
    
    /* Is there, in the current node, a BGP route towards the given
       prefix ? */
    pRoute= bgp_router_bm_route(pCurrentRouter, sPrefix, uBound);

    if (pRoute != NULL) {
      
      // Record current node's AS-Num ??
      if ((pPreviousRouter == NULL) ||
	  (iPreserveDups ||
	   (pPreviousRouter->uNumber != pCurrentRouter->uNumber))) {
	if ((path_length(pPath) >= 30) ||
	    path_append(&pPath, pCurrentRouter->uNumber)) {
	  iResult= AS_RECORD_ROUTE_TOO_LONG;
	  break;
	}
      }
      
      // If the route's next-hop is this router, then the function
      // terminates.

      /*
	fprintf(stdout, "NH: ");
	ip_address_dump(stdout, pRoute->tNextHop);
	fprintf(stdout, "\t\tRT: ");
	ip_address_dump(stdout, pCurrentRouter->pNode->tAddr);
	fprintf(stdout, "\n");
      */

      if ((pRoute->tNextHop == pCurrentRouter->pNode->tAddr) ||
	  (route_flag_get(pRoute, ROUTE_FLAG_INTERNAL))) {
	iResult= AS_RECORD_ROUTE_SUCCESS;
	break;
      }
      
      // Otherwize, looks for next-hop router
      pNode= network_find_node(pRoute->tNextHop);
      if (pNode == NULL)
	break;
      
      // Get the current node's BGP instance
      pProtocol= protocols_get(pNode->pProtocols, NET_PROTOCOL_BGP);
      if (pProtocol == NULL)
	break;
      pPreviousRouter= pCurrentRouter;
      pCurrentRouter= (SBGPRouter *) pProtocol->pHandler;
      
    } else
      break;
  }
  *ppPath= pPath;

  return iResult;
}
#endif

/////////////////////////////////////////////////////////////////////
// TEST
/////////////////////////////////////////////////////////////////////
