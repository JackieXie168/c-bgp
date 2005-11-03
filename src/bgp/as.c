// ==================================================================
// @(#)as.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @author Sebastien Tandel (standel@info.ucl.ac.be)
// @date 22/11/2002
// @lastdate 17/10/2005
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
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <stdlib.h>
//#include <strdio.h>
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

// ----- options -----
FTieBreakFunction BGP_OPTIONS_TIE_BREAK= TIE_BREAK_DEFAULT;
uint8_t BGP_OPTIONS_NLRI= BGP_NLRI_BE;
uint32_t BGP_OPTIONS_DEFAULT_LOCAL_PREF= 0;
uint8_t BGP_OPTIONS_MED_TYPE= BGP_MED_TYPE_DETERMINISTIC;
uint8_t BGP_OPTIONS_SHOW_MODE = ROUTE_SHOW_CISCO;
uint8_t BGP_OPTIONS_RIB_OUT= 1;
uint8_t BGP_OPTIONS_AUTO_CREATE= 0;
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
  pRouter->uTBID= uTBID;
  pRouter->pNode= pNode;
  pRouter->pPeers= ptr_array_create(ARRAY_OPTION_SORTED |
				    ARRAY_OPTION_UNIQUE,
				    _bgp_router_peers_compare,
				    _bgp_router_peers_destroy);
  pRouter->pLocRIB= rib_create(0);
  pRouter->pLocalNetworks= ptr_array_create_ref(0);
  pRouter->fTieBreak= BGP_OPTIONS_TIE_BREAK;
  pRouter->tClusterID= pNode->tAddr;
  pRouter->iRouteReflector= 0;

  /* Register the router into its domain */
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
  SBGPPeer * pOriginPeer;
  int iExternalSession= (pRouter->uNumber != pPeer->uRemoteAS);
  int iLocalRoute= (pRoute->tNextHop == pRouter->pNode->tAddr);
  int iExternalRoute= ((!iLocalRoute) &&
		       (pRouter->uNumber != route_peer_get(pRoute)->uRemoteAS));
  /* [route/session attributes]
   * iExternalSession == the dst peer is external
   * iExternalRoute   == the src peer is external
   * iLocalRoute      == the route is locally originated
   */

  LOG_DEBUG("bgp_router_advertise_to_peer\n");

#ifdef __ROUTER_LIST_ENABLE__
  // Append the router-list with the IP address of this router
  route_router_list_append(pRoute, pRouter->pNode->tAddr);
#endif

  // Do not redistribute to the next-hop peer
  if (pPeer->tAddr == pRoute->tNextHop) {
    LOG_DEBUG("filtered(next-hop-peer)\n");
    return -1;
  }

  // Do not redistribute to other peers
  if (route_comm_contains(pRoute, COMM_NO_ADVERTISE)) {
    LOG_DEBUG("filtered(comm_no_advertise)\n");
    return -1;
  }

  // Do not redistribute outside confederation (here AS)
  if ((iExternalSession) &&
      (route_comm_contains(pRoute, COMM_NO_EXPORT))) {
    LOG_DEBUG("filtered(comm_no_export)\n");
    return -1;
  }

  // Avoid loop creation (SSLD, Sender-Side Loop Detection)
  if ((iExternalSession) &&
      (route_path_contains(pRoute, pPeer->uRemoteAS))) {
    LOG_DEBUG("filtered(ssld)\n");
    return -1;
  }

  // If this route was learned through an iBGP session, do not
  // redistribute it to an internal peer
  if ((!pRouter->iRouteReflector) && (!iLocalRoute) &&
      (!iExternalRoute) && (!iExternalSession)) {
    LOG_DEBUG("filtered(iBGP-peer --> iBGP-peer)\n");
    return -1;
  }

  // Route-Reflection: Avoid cluster-loop creation
  // (MUST be done before local cluster-ID is appended)
  // *** MOVED TO INPUT FILTER ***
  /*if ((!iExternalSession) &&
      (route_cluster_list_contains(pRoute, pRouter->tClusterID))) {
    LOG_DEBUG("RR-filtered(cluster-loop)\n");
    return -1;
  }*/

  // Copy the route. This is required since subsequent filters may
  // alter the route's attribute !!
  pNewRoute= route_copy(pRoute);

  if ((pRouter->iRouteReflector) && (!iExternalRoute)) {
    // Route-Reflection: update Originator field
    if (route_originator_get(pNewRoute, NULL) == -1) {
      pOriginPeer= route_peer_get(pRoute);
      route_originator_set(pNewRoute, pOriginPeer->tAddr);
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
	  LOG_DEBUG("RR-filtered (client --> originator-client)\n");
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
	LOG_DEBUG("RR-filtered (non-client --> non-client)\n");
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
      
      LOG_DEBUG("*** AS%d:", pRouter->uNumber);
      LOG_ENABLED_DEBUG() ip_address_dump(log_get_stream(pMainLog),
					  pRouter->pNode->tAddr);
      LOG_DEBUG(" advertise to AS%d:", pPeer->uRemoteAS);
      LOG_ENABLED_DEBUG() ip_address_dump(log_get_stream(pMainLog),
					  pPeer->tAddr);
      LOG_DEBUG(" ***\n");
      
      peer_announce_route(pPeer, pNewRoute);
      return 0;
    }
  }
  
  route_destroy(&pNewRoute);

  return -1;
}
  
// ----- bgp_router_withdraw_to_peer --------------------------------
/**
 * Withdraw the given prefix to the given peer router.
 */
void bgp_router_withdraw_to_peer(SBGPRouter * pRouter, SPeer * pPeer,
				 SPrefix sPrefix)
{
  LOG_DEBUG("*** AS%d withdraw to AS%d ***\n", pRouter->uNumber,
	    pPeer->uRemoteAS);

  peer_withdraw_prefix(pPeer, sPrefix);
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

// ----- bgp_router_dump_adj_rib_in ---------------------------------
/**
 *
 */
/*
int bgp_router_dump_adj_rib_in(FILE * pStream, SBGPRouter * pRouter,
			       SPrefix sPrefix)
{
  int iIndex;
  SPeer * pPeer;
  SRoute * pRoute;

  for (iIndex= 0; iIndex < ptr_array_length(pRouter->pPeers); iIndex++) {
    pPeer= (SPeer *) pRoluter->pPeers->data[iIndex];
    if ((pRoute= rib_find_exact(pPeer->pAdjRIBIn, sPrefix))
	!= NULL) {
      fprintf(pStream, "AS%d|", pRouter->uNumber);
      ip_address_dump(pStream, pRouter->pNode->tAddr);
      fprintf(pStream, "|");
      route_dump(pStream, pRoute);
      fprintf(pStream, "\n");
    }
  }
  return 0;
}
*/

// ----- as_dump_rt_dp_rule -----------------------------------------
/**
 *
 */
/*
int as_dump_rt_dp_rule(FILE * pStream, SAS * pAS, SPrefix sPrefix)
{
  SRoute * pRoute;

  fprintf(pStream, "%u ", pAS->uNumber);
  ip_prefix_dump(pStream, sPrefix);
  fprintf(pStream, " ");
  if ((pRoute= rib_find_exact(pAS->pLocRIB, sPrefix)) != NULL) {
  } else {
    fprintf(pStream, "*");
  }
  fprintf(pStream, "\n");
  return 0;
}
*/

// ----- bgp_router_decision_process_dop ----------------------------
/**
 * Select routes on the basis of their local degree of preference:
 * LOCAL-PREF set by another iBGP speaker or by a local policy.
 */
void bgp_router_decision_process_dop(SBGPRouter * pRouter, SRoutes * pRoutes)
{
  uint32_t uHighestPref= 0;
  int iIndex;
  SRoute * pRoute;
  
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
      iIndex++;
  }
}

// ----- bgp_router_decision_process_tie_break ----------------------
/**
 * Select one route with the following rules:
 *
 *   - prefer shortest AS-PATH
 *   - prefer lowest ORIGIN (IGP < EGP < INCOMPLETE)
 *   - prefer lowest MED
 *   - prefer eBGP over iBGP
 *   - prefer nearest next-hop (IGP)
 *   - final tie-break
 */
void bgp_router_decision_process_tie_break(SBGPRouter * pRouter,
					   SRoutes * pRoutes)
{
  uint8_t tRank= 0;

  // *** shortest AS-PATH ***
  dp_rule_shortest_path(pRoutes);
  if (ptr_array_length(pRoutes) <= 1)
    return;
  tRank++;

  // *** lowest ORIGIN ***
  dp_rule_lowest_origin(pRoutes);
  if (ptr_array_length(pRoutes) <= 1)
    return;
  tRank++;

  // *** lowest MED ***
  dp_rule_lowest_med(pRoutes);
  if (ptr_array_length(pRoutes) <= 1)
    return;
  tRank++;

  // *** eBGP over iBGP ***
  dp_rule_ebgp_over_ibgp(pRouter, pRoutes);
  if (ptr_array_length(pRoutes) <= 1)
    return;
  tRank++;

  // *** nearest NEXT-HOP (lowest IGP-cost) ***
  dp_rule_nearest_next_hop(pRouter, pRoutes);
  if (ptr_array_length(pRoutes) <= 1)
    return;
  tRank++;

  // *** shortest cluster-ID-list ***
  dp_rule_shortest_cluster_list(pRouter, pRoutes);
  if (ptr_array_length(pRoutes) <= 1)
    return;
  tRank++;

  // *** lowest neighbor address ***
  dp_rule_lowest_neighbor_address(pRouter, pRoutes);
  if (ptr_array_length(pRoutes) <= 1)
    return;
  tRank++;

  dp_rule_final(pRouter, pRoutes);
  tRank++;
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
  if (rib_find_exact(pPeer->pAdjRIBOut, sPrefix) != NULL) {
    rib_remove_route(pPeer->pAdjRIBOut, sPrefix);
    iWithdrawRequired= 1;
  }
#else
  pNode= network_find_node(pPeer->tAddr);
  assert(pNode != NULL);
  pProtocol= protocols_get(pNode->pProtocols, NET_PROTOCOL_BGP);
  assert(pProtocol != NULL);
  pPeerRouter= (SBGPRouter *) pProtocol->pHandler;
  assert(pPeerRouter != NULL);
  pThePeer= bgp_router_find_peer(pPeerRouter, pRouter->pNode->tAddr);
  assert(pThePeer != NULL);
  
  if (rib_find_exact(pThePeer->pAdjRIBIn, sPrefix) != NULL)
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
						     SPeer * pPeer)
{
  SRoute * pNewRoute;

  if (pPeer->uSessionState == SESSION_STATE_ESTABLISHED) {
    LOG_DEBUG("\t->peer: AS%d:", pPeer->uRemoteAS);
    LOG_ENABLED_DEBUG() ip_address_dump(log_get_stream(pMainLog),
					pPeer->tAddr);
    LOG_DEBUG("\n");

    if (pRoute == NULL) {
      // A route was advertised to this peer => explicit withdraw
      if (bgp_router_peer_rib_out_remove(pRouter, pPeer, sPrefix)) {
	peer_withdraw_prefix(pPeer, sPrefix);
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
	  peer_withdraw_prefix(pPeer, sPrefix);
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

  LOG_DEBUG("> AS%d.bgp_router_decision_process_disseminate.begin\n",
	    pRouter->uNumber);

  for (iIndex= 0; iIndex < ptr_array_length(pRouter->pPeers); iIndex++) {
    pPeer= (SPeer *) pRouter->pPeers->data[iIndex];
    bgp_router_decision_process_disseminate_to_peer(pRouter, sPrefix,
						    pRoute, pPeer);
  }

  LOG_DEBUG("< AS%d.bgp_router_decision_process_disseminate.end\n",
	    pRouter->uNumber);
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
    pAdjRoute= rib_find_exact(pOldRoute->pPeer->pAdjRIBIn,
			      pOldRoute->sPrefix);

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

  pRoute= rib_find_exact(pRouter->pLocRIB, sPrefix);

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
SRoutes * bgp_router_get_feasible_routes(SBGPRouter * pRouter, SPrefix sPrefix)
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

      pRoute= rib_find_exact(pPeer->pAdjRIBIn, sPrefix);

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
    }
  }

  return pRoutes;
}

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
  //SPeer * pPeer;
  SRoute * pRoute, * pOldRoute;

  /* BGP QoS decision process */
#ifdef BGP_QOS
  if (BGP_OPTIONS_NLRI == BGP_NLRI_QOS_DELAY)
    return qos_decision_process(pRouter, pOriginPeer, sPrefix);
#endif

  LOG_DEBUG("> ");
  LOG_ENABLED_DEBUG() bgp_router_dump_id(log_get_stream(pMainLog), pRouter);
  LOG_DEBUG(" bgp_router_decision_process.begin\n");

  pOldRoute= rib_find_exact(pRouter->pLocRIB, sPrefix);
  LOG_DEBUG("\told-best: ");
  LOG_ENABLED_DEBUG() route_dump(log_get_stream(pMainLog), pOldRoute);
  LOG_DEBUG("\n");

  /* Local routes can not be overriden and must be kept in Loc-RIB */
  if ((pOldRoute != NULL) &&
      route_flag_get(pOldRoute, ROUTE_FLAG_INTERNAL))
    return 0;

  // *** lock all Adj-RIB-Ins ***

  /* Build list of eligible routes */
  pRoutes= bgp_router_get_feasible_routes(pRouter, sPrefix);

  /* Reset DP_IGP flag, log eligibles */
  for (iIndex= 0; iIndex < routes_list_get_num(pRoutes); iIndex++) {

    pRoute= (SRoute *) pRoutes->data[iIndex];

    /* Clear flag that indicates that the route depends on the
       IGP. See 'dp_rule_nearest_next_hop' and 'bgp_router_scan_rib'
       for more information. */
    route_flag_set(pRoute, ROUTE_FLAG_DP_IGP, 0);

    /* Log eligible route */
    LOG_DEBUG("\teligible: ");
    LOG_ENABLED_DEBUG() route_dump(log_get_stream(pMainLog), pRoute);
    LOG_DEBUG("\n");
    
  }

  /*
  pRoutes= routes_list_create(ROUTES_LIST_OPTION_REF);
  for (iIndex= 0; iIndex < ptr_array_length(pRouter->pPeers); iIndex++) {

    pPeer= (SPeer*) pRouter->pPeers->data[iIndex];

    if (pPeer->uSessionState == SESSION_STATE_ESTABLISHED) {

      pRoute= rib_find_exact(pPeer->pAdjRIBIn, sPrefix);

      if ((pRoute != NULL) &&
	  (route_flag_get(pRoute, ROUTE_FLAG_ELIGIBLE))) {
	
	// Clear flag that indicates that the route depends on the
	// IGP. See 'dp_rule_nearest_next_hop' and 'bgp_router_scan_rib'
	// for more information.
	route_flag_set(pRoute, ROUTE_FLAG_DP_IGP, 0);
	
	// Update ROUTE_FLAG_FEASIBLE
	if (bgp_router_feasible_route(pRouter, pRoute)) {
	  
	  routes_list_append(pRoutes, pRoute);
	  
	  LOG_DEBUG("\teligible: ");
	  LOG_ENABLED_DEBUG() route_dump(log_get_stream(pMainLog), pRoute);
	  LOG_DEBUG("\n");
	  
	}
      }
    }
  }
*/

  /* If there is a single eligible & feasible route, it depends on the
     IGP */
  if (ptr_array_length(pRoutes) == 1)
    route_flag_set((SRoute *) pRoutes->data[0], ROUTE_FLAG_DP_IGP, 1);

  // Keep routes with highest degree of preference
  if (ptr_array_length(pRoutes) > 1)
    bgp_router_decision_process_dop(pRouter, pRoutes);

  // Tie-break
  if (ptr_array_length(pRoutes) > 1)
    bgp_router_decision_process_tie_break(pRouter, pRoutes);

  assert((ptr_array_length(pRoutes) == 0) ||
    (ptr_array_length(pRoutes) == 1));

  if (ptr_array_length(pRoutes) > 0) {
    pRoute= route_copy((SRoute *) pRoutes->data[0]);

    LOG_DEBUG("\tnew best: ");
    LOG_ENABLED_DEBUG() route_dump(log_get_stream(pMainLog), pRoute);
    LOG_DEBUG("\n");

    // New/updated route
    // => install in Loc-RIB
    // => advertise to peers
    if ((pOldRoute == NULL) ||
	!route_equals(pOldRoute, pRoute)) {

      /**************************************************************
       * In this case, the route is new or one of its BGP attributes
       * has changed. Thus, we must update the route into the routing
       * table and we must propagate the BGP route to the peers.
       **************************************************************/

      if (pOldRoute != NULL)
	bgp_router_best_flag_off(pOldRoute);

      /* Mark route in Loc-RIB and Adj-RIB-In as best. This must be
	 done after the call to 'bgp_router_best_flag_off'. */
      route_flag_set(pRoute, ROUTE_FLAG_BEST, 1);
      route_flag_set(pRoutes->data[0], ROUTE_FLAG_BEST, 1);

      /* Insert in Loc-RIB */
      assert(rib_add_route(pRouter->pLocRIB, pRoute) == 0);

      /* Insert in the node's routing table */
      bgp_router_rt_add_route(pRouter, pRoute);

      bgp_router_decision_process_disseminate(pRouter, sPrefix, pRoute);

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

      /* Mark route in Adj-RIB-In as best (since it has probably been
	 replaced). */
      route_flag_set(pRoutes->data[0], ROUTE_FLAG_BEST, 1);

      /* Update ROUTE_FLAG_DP_IGP of old route */
      route_flag_set(pOldRoute, ROUTE_FLAG_DP_IGP,
		     route_flag_get(pRoute, ROUTE_FLAG_DP_IGP));

      /* If the IGP next-hop has changed, we need to re-insert the
	 route into the routing table with the new next-hop. */
      bgp_router_rt_add_route(pRouter, pRoute);

      /* Route has not changed. */
      route_destroy(&pRoute);
      pRoute= pOldRoute;

    }

  } else {

    LOG_DEBUG("no best\n");
    // If a route towards this prefix was previously installed, then
    // withdraw it. Otherwise, do nothing...
    if (pOldRoute != NULL) {
      //LOG_DEBUG("there was a previous best-route\n");
      rib_remove_route(pRouter->pLocRIB, sPrefix);

      bgp_router_best_flag_off(pOldRoute);

      bgp_router_rt_del_route(pRouter, sPrefix);

      bgp_router_decision_process_disseminate(pRouter, sPrefix, NULL);
    }
  }
  
  // *** unlock all Adj-RIB-Ins ***

  routes_list_destroy(&pRoutes);

  LOG_DEBUG("< AS%d.bgp_router_decision_process.end\n", pRouter->uNumber);

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
    LOG_DEBUG("*** ");
    LOG_ENABLED_DEBUG() bgp_router_dump_id(log_get_stream(pMainLog), pRouter);
    LOG_DEBUG(" handle message from ");
    LOG_ENABLED_DEBUG() bgp_peer_dump_id(log_get_stream(pMainLog), pPeer);
    LOG_DEBUG(" ***\n", pMessage->tSrcAddr);

    peer_handle_message(pPeer, pMsg);
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
  SRoute * pAdjRoute;
  int iIndex;
  SPeer * pPeer;
  unsigned int uBestWeight;
  SNetRouteInfo * pRouteInfo;
  SNetRouteInfo * pCurRouteInfo;

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
  pRoute= rib_find_exact(pRouter->pLocRIB, sPrefix);
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
	
	pAdjRoute= rib_find_exact(pPeer->pAdjRIBIn, pRoute->sPrefix);
		
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

void bgp_router_alloc_prefixes(SRadixTree ** ppPrefixes)
{
  /* If list (radix-tree) is not allocated, create it now. The
     radix-tree is created without destroy function and acts thus as a
     list of references. */
  if (*ppPrefixes == NULL) {
    *ppPrefixes= radix_tree_create(32, NULL);
  }
}

void bgp_router_free_prefixes(SRadixTree ** ppPrefixes)
{
  if (*ppPrefixes != NULL) {
    radix_tree_destroy(ppPrefixes);
    *ppPrefixes= NULL;
  }
}

// -----[ bgp_router_get_peer_prefixes ]-----------------------------
/**
 * This function builds a list of all prefixes received from this
 * peer (in Adj-RIB-in). The list of prefixes is implemented as a
 * radix-tree in order to guarantee that each prefix will be present
 * at most one time (uniqueness).
 */
int bgp_router_get_peer_prefixes(SBGPRouter * pRouter, SPeer * pPeer,
				 SRadixTree ** ppPrefixes)
{
  int iIndex;
  int iResult= 0;

  bgp_router_alloc_prefixes(ppPrefixes);

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

int bgp_router_get_local_prefixes(SBGPRouter * pRouter,
				  SRadixTree ** ppPrefixes)
{
  bgp_router_alloc_prefixes(ppPrefixes);
  
  return rib_for_each(pRouter->pLocRIB, bgp_router_prefixes_for_each,
		      *ppPrefixes);
}

// -----[ bgp_router_prefixes ]--------------------------------------
/**
 * This function builds a list of all the prefixes known in this
 * router (in Adj-RIB-Ins and Loc-RIB).
 */
SRadixTree * bgp_router_prefixes(SBGPRouter * pRouter)
{
  SRadixTree * pPrefixes= NULL;

  /* Get prefixes from all Adj-RIB-Ins */
  bgp_router_get_peer_prefixes(pRouter, NULL, &pPrefixes);

  /* Get prefixes from the RIB */
  bgp_router_get_local_prefixes(pRouter, &pPrefixes);

  return pPrefixes;
}

// ----- bgp_router_refresh_sessions --------------------------------
/*
 * This function scans the peering sessions and checks that the peer
 * router is still reachable. If it is not, the sessions is teared
 * down.
 */
void bgp_router_refresh_sessions(SBGPRouter * pRouter)
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
  bgp_router_refresh_sessions(pRouter);

  /* initialize context */
  sCtx.pRouter= pRouter;
  sCtx.pPrefixes= _array_create(sizeof(SPrefix), 0, NULL, NULL);

  /* Build a list of all available prefixes in this router */
  pPrefixes= bgp_router_prefixes(pRouter);

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

  bgp_router_free_prefixes(&pPrefixes);
    
  _array_destroy(&sCtx.pPrefixes);
  
  return iResult;
}

// -----[ bgp_router_rerun_for_each ]--------------------------------
int bgp_router_rerun_for_each(uint32_t uKey, uint8_t uKeyLen,
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

  bgp_router_alloc_prefixes(&pPrefixes);

  /* Populate with all prefixes */
  if (sPrefix.uMaskLen == 0) {
    assert(!bgp_router_get_peer_prefixes(pRouter, NULL, &pPrefixes));
    assert(!bgp_router_get_local_prefixes(pRouter, &pPrefixes));
  } else {
    radix_tree_add(pPrefixes, sPrefix.tNetwork,
		   sPrefix.uMaskLen, (void *) 1);
  }

  printf("prefix-list ok\n");
  fflush(stdout);

  /* For each route in the list, run the BGP decision process */
  iResult= radix_tree_for_each(pPrefixes, bgp_router_rerun_for_each, pRouter);

  /* Free list of prefixes */
  bgp_router_free_prefixes(&pPrefixes);

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
    pRoute= rib_find_exact(pRouter->pLocRIB, sPrefix);
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

// ----- bgp_router_dump_rib_string ----------------------------------------
/**
 *
 */
char * bgp_router_dump_rib_string(SBGPRouter * pRouter)
{
  SRouteDumpCtx sCtx;
  sCtx.pRouter= pRouter;
  sCtx.cDump = NULL;
  rib_for_each(pRouter->pLocRIB, bgp_router_dump_route_string, &sCtx);

  return sCtx.cDump;

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

  pRoute= rib_find_exact(pRouter->pLocRIB, sPrefix);
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
  ip_address_dump(stdout, pRouter->pNode->tAddr);
  fprintf(pStream, "\n");
  fprintf(pStream, "as-number : %u\n", pRouter->uNumber);
  fprintf(pStream, "cluster-id: ");
  ip_address_dump(pStream, pRouter->tClusterID);
  fprintf(pStream, "\n");
  if (pRouter->pNode->pcName != NULL)
    fprintf(pStream, "name      : %s\n", pRouter->pNode->pcName);
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
      route_flag_set(pRoute, ROUTE_FLAG_BEST, 1);
      //route_nexthop_set(pRouter->pNode->tAddr);
      bgp_router_add_route(pRouter, pRoute);
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
	  peer_handle_message(pPeer,
			bgp_msg_update_create(pRouter->uNumber, pRoute));
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
    pRoute= rib_find_exact(pRouter->pLocRIB, sBoundedPrefix);

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

    /* BGP-DP: local-preference rule */
    if (routes_list_get_num(pRoutes) > 0)
      bgp_router_decision_process_dop(pRouter, pRoutes);
    
    /* BGP-DP: other rules */
    if (routes_list_get_num(pRoutes) > 0)
      bgp_router_decision_process_tie_break(pRouter, pRoutes);

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
