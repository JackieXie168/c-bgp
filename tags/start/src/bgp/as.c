// ==================================================================
// @(#)as.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be), Sebastien Tandel
// @date 22/11/2002
// @lastdate 27/02/2004
// ==================================================================

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <libgds/array.h>
#include <libgds/list.h>
#include <libgds/log.h>
#include <libgds/memory.h>
#include <libgds/tokenizer.h>

#include <bgp/as.h>
#include <bgp/comm.h>
#include <bgp/dp_rules.h>
#include <bgp/ecomm.h>
#include <bgp/mrtd.h>
#include <bgp/peer.h>
#include <bgp/qos.h>
#include <bgp/routes_list.h>
#include <bgp/tie_breaks.h>
#include <net/network.h>

unsigned long route_create_count= 0;
unsigned long route_copy_count= 0;
unsigned long route_destroy_count= 0;

// ----- options -----
FTieBreakFunction BGP_OPTIONS_TIE_BREAK= TIE_BREAK_DEFAULT;
uint8_t BGP_OPTIONS_NLRI= BGP_NLRI_BE;
uint32_t BGP_OPTIONS_DEFAULT_LOCAL_PREF= 0;

// ----- as_peers_compare -------------------------------------------
/**
 *
 */
int as_peers_compare(void * pItem1, void * pItem2,
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

// ----- as_peer_list_destroy ---------------------------------------
/**
 *
 */
void as_peers_destroy(void * pItem)
{
  SPeer * pPeer= *((SPeer **) pItem);

  peer_destroy(&pPeer);
}

// ----- as_create --------------------------------------------------
/**
 *
 */
SAS * as_create(uint16_t uNumber, SNetNode * pNode, uint32_t uTBID)
{
  SAS * pAS= (SAS*) MALLOC(sizeof(SAS));
  pAS->uNumber= uNumber;
  pAS->uTBID= uTBID;
  pAS->pNode= pNode;
  pAS->pPeers= ptr_array_create(ARRAY_OPTION_SORTED,
				as_peers_compare,
				as_peers_destroy);
  pAS->pLocRIB= rib_create(0);
  pAS->pLocalNetworks= ptr_array_create_ref(0);
  pAS->fTieBreak= BGP_OPTIONS_TIE_BREAK;
  pAS->tClusterID= pNode->tAddr;
  pAS->iRouteReflector= 0;
  return pAS;
}

// ----- as_destroy -------------------------------------------------
/**
 *
 */
void as_destroy(SAS ** ppAS)
{
  int iIndex;

  if (*ppAS != NULL) {
    ptr_array_destroy(&(*ppAS)->pPeers);
    rib_destroy(&(*ppAS)->pLocRIB);
    for (iIndex= 0; iIndex < ptr_array_length((*ppAS)->pLocalNetworks);
	 iIndex++) {
      route_destroy_count++;
      route_destroy((SRoute **) &(*ppAS)->pLocalNetworks->data[iIndex]);
    }
    _array_destroy((SArray **) &(*ppAS)->pLocalNetworks);
    FREE(*ppAS);
    *ppAS= NULL;
  }
}

// ----- as_find_peer -----------------------------------------------
/**
 *
 */
SPeer * as_find_peer(SAS * pAS, net_addr_t tAddr)
{
  int iIndex;
  net_addr_t * pAddr= &tAddr;
  SPeer * pPeer= NULL;

  if (ptr_array_sorted_find_index(pAS->pPeers, &pAddr, &iIndex) != -1)
    pPeer= (SPeer *) pAS->pPeers->data[iIndex];
  return pPeer;
}

// ----- as_add_peer ------------------------------------------------
/**
 *
 */
int as_add_peer(SAS * pAS, uint16_t uRemoteAS, net_addr_t tAddr,
		uint8_t uPeerType)
{
  SPeer * pNewPeer= peer_create(uRemoteAS, tAddr, pAS, uPeerType);

  if (ptr_array_add(pAS->pPeers, &pNewPeer) == -1) {
    peer_destroy(&pNewPeer);
    return -1;
  } else
    return 0;
}

// ----- as_peer_set_in_filter --------------------------------------
/**
 *
 */
int as_peer_set_in_filter(SAS * pAS, net_addr_t tAddr,
			  SFilter * pFilter)
{
  SPeer * pPeer;

  if ((pPeer= as_find_peer(pAS, tAddr)) != NULL) {
    peer_set_in_filter(pPeer, pFilter);
    return 0;
  }
  return -1;
}

// ----- as_peer_set_out_filter --------------------------------------
/**
 *
 */
int as_peer_set_out_filter(SAS * pAS, net_addr_t tAddr,
			   SFilter * pFilter)
{
  SPeer * pPeer;

  if ((pPeer= as_find_peer(pAS, tAddr)) != NULL) {
    peer_set_out_filter(pPeer, pFilter);
    return 0;
  }
  return -1;
}

// ----- as_add_network ---------------------------------------------
/**
 *
 */
int as_add_network(SAS * pAS, SPrefix sPrefix)
{
  SRoute * pRoute= route_create(sPrefix, NULL,
				pAS->pNode->tAddr, ROUTE_ORIGIN_IGP);
  route_flag_set(pRoute, ROUTE_FLAG_BEST, 1);
  route_flag_set(pRoute, ROUTE_FLAG_FEASIBLE, 1);
  route_flag_set(pRoute, ROUTE_FLAG_INTERNAL, 1);
  route_localpref_set(pRoute, BGP_OPTIONS_DEFAULT_LOCAL_PREF);
  ptr_array_append(pAS->pLocalNetworks, pRoute);
  route_create_count++;
  rib_add_route(pAS->pLocRIB, route_copy(pRoute));
  route_copy_count++;
  return 0;
}

// ----- as_add_route -----------------------------------------------
/**
 * Add a route in the Loc-RIB
 */
int as_add_route(SAS * pAS, SRoute * pRoute)
{
  ptr_array_append(pAS->pLocalNetworks, pRoute);
  rib_add_route(pAS->pLocRIB, route_copy(pRoute));
  route_copy_count++;
  return 0;
}

// ----- as_add_qos_network -----------------------------------------
/**
 *
 */
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
  route_create_count++;
  rib_add_route(pAS->pLocRIB, route_copy(pRoute));
  route_copy_count++;
  return 0;
}

// ----- as_ecomm_red_process ---------------------------------------
/**
 * 0 => Ignore route (destroy)
 * 1 => Redistribute
 */
int as_ecomm_red_process(SPeer * pPeer, SRoute * pRoute)
{
  int iIndex;
  uint8_t uActionParam;

  if (pRoute->pECommunities != NULL) {

    for (iIndex= 0; iIndex < ecomm_length(pRoute->pECommunities); iIndex++) {
      SECommunity * pComm= ecomm_get_index(pRoute->pECommunities, iIndex);
      if (pComm->uTypeHigh == ECOMM_RED) {

	if (ecomm_red_match(pComm, pPeer)) {
	  switch ((pComm->uTypeLow >> 3) & 0x07) {
	  case ECOMM_RED_ACTION_PREPEND:
	    uActionParam= (pComm->uTypeLow & 0x07);
	    route_path_prepend(pRoute, pPeer->pLocalAS->uNumber, uActionParam);
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
      }
    }
  }
  return 1;
}

// ----- as_advertise_to_peer ---------------------------------------
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
 *   - update Next-Hop (next-hop-self)
 *   - prepend AS-Path (if redistribution to an external peer)
 */
int as_advertise_to_peer(SAS * pAS, SPeer * pPeer, SRoute * pRoute)
{
  net_addr_t tOriginator;
  SRoute * pNewRoute= NULL;
  int iExternalSession= (pAS->uNumber != pPeer->uRemoteAS);
  int iLocalRoute= (pRoute->tNextHop == pAS->pNode->tAddr);
  int iExternalRoute= ((!iLocalRoute) &&
		       (pAS->uNumber != route_peer_get(pRoute)->uRemoteAS));

  LOG_DEBUG("as_advertise_to_peer\n");

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
  if ((!pAS->iRouteReflector) && (!iLocalRoute) &&
      (!iExternalRoute) && (!iExternalSession)) {
    LOG_DEBUG("filtered(iBGP-peer --> iBGP-peer)\n");
    return -1;
  }

  // Route-Reflection: Avoid cluster-loop creation
  // (MUST be done before local cluster-ID is appended)
  if ((!iExternalSession) &&
      (route_cluster_list_contains(pRoute, pAS->tClusterID))) {
    LOG_DEBUG("RR-filtered(cluster-loop)\n");
    return -1;
  }
  
  // Copy the route. This is required since subsequent filters may
  // alter the route's attribute !!
  route_copy_count++;
  pNewRoute= route_copy(pRoute);
  
  
  if ((pAS->iRouteReflector) && (!iExternalSession)) {
    // Route-Reflection: update Originator field
    if (route_originator_get(pNewRoute, NULL) == -1)
      route_originator_set(pNewRoute, pPeer->pLocalAS->pNode->tAddr);
    // Route-Reflection: append Cluster-ID to Cluster-ID-List field
    if ((iExternalRoute || route_flag_get(pNewRoute, ROUTE_FLAG_RR_CLIENT)) &&
	(!peer_flag_get(pPeer, PEER_FLAG_RR_CLIENT)))
      route_cluster_list_append(pNewRoute, pAS->tClusterID);
  }
  
  if ((pAS->iRouteReflector) && (!iLocalRoute) &&
      (!iExternalRoute) && (!iExternalSession)) {
    // Route-reflectors: do not redistribute a route from a client peer
    // to the originator client peer
    if (route_flag_get(pNewRoute, ROUTE_FLAG_RR_CLIENT)) {
      if (peer_flag_get(pPeer, PEER_FLAG_RR_CLIENT)) {
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
      if (!peer_flag_get(pPeer, PEER_FLAG_RR_CLIENT)) {
	LOG_DEBUG("RR-filtered (non-client --> non-client)\n");
	route_destroy(&pNewRoute);
	return -1;
      }
    }
  }
  
  // Check output filter and redistribution communities
  if (as_ecomm_red_process(pPeer, pNewRoute)) {
    
    route_ecomm_strip_non_transitive(pNewRoute);
    
    if (filter_apply(pPeer->pOutFilter, pAS, pNewRoute)) {
      
      // Change the route's next-hop to this router
      // - if advertisement to an external peer
      // - if the 'next-hop-self' option is set for this peer
      // Note: in the case of route-reflectors, the next-hop will only
      // be changed for eBGP learned routes
      if (iExternalSession ||
	  (!iLocalRoute &&
	   peer_flag_get(route_peer_get(pNewRoute),
			 PEER_FLAG_NEXT_HOP_SELF) &&
	   !pAS->iRouteReflector)) {
	route_nexthop_set(pNewRoute, pAS->pNode->tAddr);
      }
      
      // Append AS-Number if external peer (eBGP session)
      if (iExternalSession)
	route_path_append(pNewRoute, pAS->uNumber);
      
      // Route-Reflection: clear Originator and Cluster-ID-List fields
      // if external peer (these fields are non-transitive)
      if (iExternalSession) {
	route_originator_clear(pNewRoute);
	route_cluster_list_clear(pNewRoute);
      }
      
      LOG_DEBUG("*** AS%d:", pAS->uNumber);
      LOG_ENABLED_DEBUG() ip_address_dump(log_get_stream(pMainLog),
					  pAS->pNode->tAddr);
      LOG_DEBUG(" advertise to AS%d:", pPeer->uRemoteAS);
      LOG_ENABLED_DEBUG() ip_address_dump(log_get_stream(pMainLog),
					  pPeer->tAddr);
      LOG_DEBUG(" ***\n");
      
      peer_announce_route(pPeer, pNewRoute);
      return 0;
    }
  }
  
  route_destroy_count++;
  route_destroy(&pNewRoute);

  return -1;
}
  
// ----- as_withdraw_to_peer ----------------------------------------
/**
 *
 */
void as_withdraw_to_peer(SAS * pAS, SPeer * pPeer, SPrefix sPrefix)
{
  LOG_DEBUG("*** AS%d withdraw to AS%d ***\n", pAS->uNumber, pPeer->uRemoteAS);

  peer_withdraw_prefix(pPeer, sPrefix);
}

// ----- as_run -----------------------------------------------------
/**
 * Run the AS. The AS starts by sending its local prefixes to its
 * peers.
 */
int as_run(SAS * pAS)
{
  int iIndex;

  // Open all BGP sessions
  for (iIndex= 0; iIndex < ptr_array_length(pAS->pPeers); iIndex++)
    if (peer_open_session((SPeer *) pAS->pPeers->data[iIndex]) != 0)
      return -1;

  return 0;
}

// ----- as_dump_adj_rib_in -----------------------------------------
/**
 *
 */
int as_dump_adj_rib_in(FILE * pStream, SAS * pAS, SPrefix sPrefix)
{
  int iIndex;
  SPeer * pPeer;
  SRoute * pRoute;

  for (iIndex= 0; iIndex < ptr_array_length(pAS->pPeers); iIndex++) {
    pPeer= (SPeer *) pAS->pPeers->data[iIndex];
    if ((pRoute= rib_find_exact(pPeer->pAdjRIBIn, sPrefix))
	!= NULL) {
      fprintf(pStream, "AS%d|", pAS->uNumber);
      ip_address_dump(pStream, pAS->pNode->tAddr);
      fprintf(pStream, "|");
      route_dump(pStream, pRoute);
      fprintf(pStream, "\n");
    }
  }
  return 0;
}

// ----- as_dump_rt_dp_rule -----------------------------------------
/**
 *
 */
int as_dump_rt_dp_rule(FILE * pStream, SAS * pAS, SPrefix sPrefix)
{
  SRoute * pRoute;

  fprintf(pStream, "%u ", pAS->uNumber);
  ip_prefix_dump(pStream, sPrefix);
  fprintf(pStream, " ");
  if ((pRoute= rib_find_exact(pAS->pLocRIB, sPrefix)) != NULL) {
    //fprintf(pStream, "%u", pRoute->uDPRule);
    fprintf(pStream, "%u %u %u",
	    pRoute->uCntRuleNone,
	    pRoute->uCntRuleLocalPref,
	    pRoute->uCntRuleShortestPath);
  } else {
    fprintf(pStream, "*");
  }
  fprintf(pStream, "\n");
  return 0;
}

// ----- as_decision_process_dop ------------------------------------
/**
 * Select routes on the basis of their local degree of preference:
 * LOCAL-PREF set by another iBGP speaker or by a local policy.
 */
void as_decision_process_dop(SAS * pAS, SRoutes * pRoutes)
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

// ----- as_decision_process_tie_break ------------------------------
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
void as_decision_process_tie_break(SAS * pAS, SRoutes * pRoutes,
				   uint32_t * puCntRuleShortestPath)
{
  // *** shortest AS-PATH ***
  dp_rule_shortest_path(pRoutes);
  *puCntRuleShortestPath= ptr_array_length(pRoutes);
  if (ptr_array_length(pRoutes) <= 1)
    return;

  // *** lowest ORIGIN ***
  dp_rule_lowest_origin(pRoutes);
  if (ptr_array_length(pRoutes) <= 1)
    return;

  // *** lowest MED ***
  dp_rule_lowest_med(pRoutes);
  if (ptr_array_length(pRoutes) <= 1)
    return;

  // *** eBGP over iBGP ***
  dp_rule_ebgp_over_ibgp(pAS, pRoutes);
  if (ptr_array_length(pRoutes) <= 1)
    return;

  // *** nearest NEXT-HOP (lowest IGP-cost) ***
  dp_rule_nearest_next_hop(pAS, pRoutes);
  if (ptr_array_length(pRoutes) <= 1)
    return;

  // *** shortest cluster-ID-list ***
  dp_rule_shortest_cluster_list(pAS, pRoutes);
  if (ptr_array_length(pRoutes) <= 1)
    return;

  // *** lowest neighbor address ***
  dp_rule_lowest_neighbor_address(pAS, pRoutes);
  if (ptr_array_length(pRoutes) <= 1)
    return;

  dp_rule_final(pAS, pRoutes);
}

// ----- as_decision_process_disseminate_to_peer --------------------
/**
 *
 */
void as_decision_process_disseminate_to_peer(SAS * pAS,
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
      if (rib_find_exact(pPeer->pAdjRIBOut, sPrefix) != NULL) {
	rib_remove_route(pPeer->pAdjRIBOut, sPrefix);
	peer_withdraw_prefix(pPeer, sPrefix);
	LOG_DEBUG("\texplicit-withdraw\n");
      }
    } else {
      route_copy_count++;
      pNewRoute= route_copy(pRoute);
      if (as_advertise_to_peer(pAS, pPeer, pNewRoute) == 0) {
	LOG_DEBUG("\treplaced\n");
	rib_replace_route(pPeer->pAdjRIBOut, pNewRoute);
      } else {
	route_destroy_count++;
	route_destroy(&pNewRoute);
	LOG_DEBUG("\tfiltered\n");
	if (rib_find_exact(pPeer->pAdjRIBOut, sPrefix) != NULL) {
	  LOG_DEBUG("\texplicit-withdraw\n");
	  rib_remove_route(pPeer->pAdjRIBOut, sPrefix);
	  peer_withdraw_prefix(pPeer, sPrefix);
	}
      }
    }
  }
}


// ----- as_decision_process_disseminate ----------------------------
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
void as_decision_process_disseminate(SAS * pAS, SPrefix sPrefix,
				     SRoute * pRoute)
{
  int iIndex;
  SPeer * pPeer;

  LOG_DEBUG("> AS%d.as_decision_process_disseminate.begin\n", pAS->uNumber);

  for (iIndex= 0; iIndex < ptr_array_length(pAS->pPeers); iIndex++) {
    pPeer= (SPeer *) pAS->pPeers->data[iIndex];
    as_decision_process_disseminate_to_peer(pAS, sPrefix, pRoute, pPeer);
  }

  LOG_DEBUG("< AS%d.as_decision_process_disseminate.end\n", pAS->uNumber);

}

// ----- as_decision_process ----------------------------------------
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
int as_decision_process(SAS * pAS, SPeer * pOriginPeer, SPrefix sPrefix)
{
  SRoutes * pRoutes;
  int iIndex;
  SPeer * pPeer;
  SRoute * pRoute, * pOldRoute;
  uint32_t uCntRuleNone= 0;
  uint32_t uCntRuleLocalPref= 0;
  uint32_t uCntRuleShortestPath= 0;

  if (BGP_OPTIONS_NLRI == BGP_NLRI_QOS_DELAY)
    return qos_decision_process(pAS, pOriginPeer, sPrefix);

  LOG_DEBUG("> ");
  LOG_ENABLED_DEBUG() as_dump_id(log_get_stream(pMainLog), pAS);
  LOG_DEBUG(" as_decision_process.begin\n");

  LOG_DEBUG("\t<-peer: AS%d\n", pOriginPeer->uRemoteAS);

  pOldRoute= rib_find_exact(pAS->pLocRIB, sPrefix);
  LOG_DEBUG("\tbest: ");
  LOG_ENABLED_DEBUG() route_dump(log_get_stream(pMainLog), pOldRoute);
  LOG_DEBUG("\n");

  // Local routes can not be overriden and must be kept in Loc-RIB
  if ((pOldRoute != NULL) &&
      route_flag_get(pOldRoute, ROUTE_FLAG_INTERNAL))
    return 0;

  // *** lock all Adj-RIB-Ins ***

  // Build list of eligible routes
  pRoutes= routes_list_create();
  for (iIndex= 0; iIndex < ptr_array_length(pAS->pPeers); iIndex++) {
    pPeer= (SPeer*) pAS->pPeers->data[iIndex];
    pRoute= rib_find_exact(pPeer->pAdjRIBIn, sPrefix);
    if ((pRoute != NULL) &&
	(route_flag_get(pRoute, ROUTE_FLAG_FEASIBLE))) {
      routes_list_append(pRoutes, pRoute);
      LOG_DEBUG("\teligible: ");
      LOG_ENABLED_DEBUG() route_dump(log_get_stream(pMainLog), pRoute);
      LOG_DEBUG("\n");
    }
  }

  uCntRuleNone= ptr_array_length(pRoutes);

  // Keep routes with highest degree of preference
  if (ptr_array_length(pRoutes) > 1) {
    as_decision_process_dop(pAS, pRoutes);
    uCntRuleLocalPref= ptr_array_length(pRoutes);
  }

  // Tie-break
  if (ptr_array_length(pRoutes) > 1)
    as_decision_process_tie_break(pAS, pRoutes,
				  &uCntRuleShortestPath);

  assert((ptr_array_length(pRoutes) == 0) ||
    (ptr_array_length(pRoutes) == 1));

  if (ptr_array_length(pRoutes) > 0) {
    route_copy_count++;
    pRoute= route_copy((SRoute *) pRoutes->data[0]);
    route_flag_set(pRoute, ROUTE_FLAG_BEST, 1);

    LOG_DEBUG("\tnew best: ");
    LOG_ENABLED_DEBUG() route_dump(log_get_stream(pMainLog), pRoute);
    LOG_DEBUG("\n");

    // New/updated route
    // => install in Loc-RIB
    // => advertise to peers
    if ((pOldRoute == NULL) || !route_equals(pOldRoute, pRoute)) {
      if (pOldRoute != NULL)
	route_flag_set(pOldRoute, ROUTE_FLAG_BEST, 0);
      assert(rib_add_route(pAS->pLocRIB, pRoute) == 0);
      as_decision_process_disseminate(pAS, sPrefix, pRoute);
    } else {
      route_destroy(&pRoute);
      pRoute= pOldRoute;
    }
    //route_set_dp_rule(pRoute, uDPRule);
    pRoute->uCntRuleNone= uCntRuleNone;
    pRoute->uCntRuleLocalPref= uCntRuleLocalPref;
    pRoute->uCntRuleShortestPath= uCntRuleShortestPath;
  } else {
    LOG_DEBUG("no best\n");
    // If a route towards this prefix was previously installed, then
    // withdraw it. Otherwise, do nothing...
    if (pOldRoute != NULL) {
      //LOG_DEBUG("there was a previous best-route\n");
      rib_remove_route(pAS->pLocRIB, sPrefix);
      //LOG_DEBUG("previous best-route removed\n");
      route_flag_set(pOldRoute, ROUTE_FLAG_BEST, 0);

      as_decision_process_disseminate(pAS, sPrefix, NULL);
    }
  }
  
  // *** unlock all Adj-RIB-Ins ***

  routes_list_destroy(&pRoutes);

  LOG_DEBUG("< AS%d.as_decision_process.end\n", pAS->uNumber);

  return 0;
}

// ----- as_handle_message ------------------------------------------
/**
 * Handle a BGP message.
 */
int as_handle_message(void * pHandler, SNetMessage * pMessage)
{
  SAS * pAS= (SAS *) pHandler;
  SBGPMsg * pMsg= (SBGPMsg *) pMessage->pPayLoad;
  SPeer * pPeer;

  if ((pPeer= as_find_peer(pAS, pMessage->tSrcAddr)) != NULL) {
    LOG_DEBUG("*** ");
    LOG_ENABLED_DEBUG() as_dump_id(log_get_stream(pMainLog), pAS);
    LOG_DEBUG(" handle message from ");
    LOG_ENABLED_DEBUG() peer_dump_id(log_get_stream(pMainLog), pPeer);
    LOG_DEBUG(" ***\n", pMessage->tSrcAddr);

    peer_handle_message(pPeer, pMsg);
  } else {
    LOG_WARNING("WARNING: BGP message received from unknown peer !\n");
    LOG_WARNING("WARNING:   destination=");
    LOG_ENABLED_WARNING() as_dump_id(log_get_stream(pMainLog), pAS);
    LOG_WARNING("\n");
    LOG_WARNING("WARNING:   source=");
    LOG_ENABLED_WARNING() ip_address_dump(log_get_stream(pMainLog),
					  pMessage->tSrcAddr);
    LOG_WARNING("\n");
    return -1;
  }
  return 0;
}

// ----- as_record_route --------------------------------------------
/**
 *
 */
int as_record_route(FILE * pStream, SAS * pAS, SPrefix sPrefix,
		    SPath ** ppPath)
{
  /*
  SAS * pCurrentAS= pAS;
  SRoute * pRoute;
  SPath * pPath= path_create();
  SNetNode * pNode;
  *ppPath= NULL;

  while (pCurrentAS != NULL) {
    if ((pRoute= rib_find_best(pCurrentAS->pLocRIB, sPrefix)) != NULL) {
      if (route_path_length(pRoute) == 0) {
	path_append(&pPath, pCurrentAS->uNumber);
	*ppPath= pPath;
	return AS_RECORD_ROUTE_SUCCESS;
      }
      // Detect too long paths (loop ?)
      if (path_append(&pPath, pCurrentAS->uNumber)) {
	path_destroy(&pPath);
	LOG_WARNING("AS%d --> ", pAS->uNumber);
	LOG_ENABLED_WARNING() ip_prefix_dump(log_get_stream(pMainLog), sPrefix);
	LOG_WARNING(": next-hop is unreachable (path too long ?).\n");
	return AS_RECORD_ROUTE_TOO_LONG;
      }
      pNode= network_find_node(pAS->pNode->pNetwork, pRoute->tNextHop);
      if (pNode == NULL)
	break;
      pCurrentAS= (SAS *) pNode->pHandler;
    } else {
      LOG_WARNING("AS%d --> ", pAS->uNumber);
      LOG_ENABLED_WARNING() ip_prefix_dump(log_get_stream(pMainLog), sPrefix);
      LOG_WARNING(": next-hop is unreachable.\n");
      path_destroy(&pPath);
      return AS_RECORD_ROUTE_UNREACH;
    }
  }
  LOG_WARNING("AS%d --> ", pAS->uNumber);
  LOG_ENABLED_WARNING() ip_prefix_dump(log_get_stream(pMainLog), sPrefix);
  LOG_WARNING(": next-hop is unreachable.\n");
  path_destroy(&pPath);
  */
  return AS_RECORD_ROUTE_UNREACH;
}

// ----- as_num_providers -------------------------------------------
/**
 *
 */
uint16_t as_num_providers(SAS * pAS) {
  int iIndex;
  uint16_t uNumProviders= 0;

  for (iIndex= 0; iIndex < ptr_array_length(pAS->pPeers); iIndex++)
    if (((SPeer *) pAS->pPeers->data[iIndex])->uPeerType ==
	PEER_TYPE_PROVIDER)
      uNumProviders++;
  return uNumProviders;
}

// ----- as_dump_id -------------------------------------------------
/**
 *
 */
void as_dump_id(FILE * pStream, SAS * pAS)
{
  fprintf(pStream, "AS%d:", pAS->uNumber);
  ip_address_dump(pStream, pAS->pNode->tAddr);
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
}

typedef struct {
  SBGPRouter * pRouter;
  FILE * pStream;
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
  if (pRoute == NULL)
    fprintf(pStream, "NO_ROUTE");
  else
    route_dump(pStream, pRoute);
  fprintf(pStream, "\n");
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
  if (pRoute == NULL)
    fprintf(pStream, "NO_ROUTE");
  else
    route_dump(pStream, pRoute);
  fprintf(pStream, "\n");
}

// ----- bgp_router_dump_ribin --------------------------------------
/**
 *
 */
void bgp_router_dump_ribin(FILE * pStream, SBGPRouter * pRouter,
			   SPeer * pPeer, SPrefix sPrefix)
{
  int iIndex;

  if (pPeer == NULL) {
    for (iIndex= 0; iIndex < ptr_array_length(pRouter->pPeers); iIndex++) {
      bgp_peer_dump_ribin(pStream, (SPeer *)
			  pRouter->pPeers->data[iIndex], sPrefix);
    }
  } else {
    bgp_peer_dump_ribin(pStream, pPeer, sPrefix);
  }
}

/////////////////////////////////////////////////////////////////////
// LOAD/SAVE FUNCTIONS
/////////////////////////////////////////////////////////////////////

// ----- as_load_rib ------------------------------------------------
/**
 * This function loads an existant BGP routing table into the given
 * bgp instance. The routes are considered local and will not be
 * replaced by routes received from peers. The routes are marked as
 * best and feasible and are directly installed into the Loc-RIB.
 *
 * Note: the routes that are loaded can have a next-hop that differs
 * from the BGP instance's node so that the loaded networks may
 * finally be unreachable. We should maybe change this, i.e. force the
 * next-hop to be this router ???
 */
int as_load_rib(char * pcFileName, SBGPRouter * pRouter)
{
  SRoutes * pRoutes= mrtd_load_routes(pcFileName);
  SRoute * pRoute;
  int iIndex;

  if (pRoutes != NULL) {
    for (iIndex= 0; iIndex < routes_list_get_num(pRoutes); iIndex++) {
      pRoute= (SRoute *) pRoutes->data[iIndex];
      route_flag_set(pRoute, ROUTE_FLAG_FEASIBLE, 1);
      route_flag_set(pRoute, ROUTE_FLAG_BEST, 1);
      //route_nexthop_set(pRouter->pNode->tAddr);
      as_add_route(pRouter, pRoute);
    }
    routes_list_destroy(&pRoutes);
  } else
    return -1;
  return 0;
}

// ----- bgp_router_save_route_mrtd ---------------------------------
/**
 *
 */
int bgp_router_save_route_mrtd(uint32_t uKey, uint8_t uKeyLen,
			       void * pItem, void * pContext)
{
  SRouteDumpCtx * pCtx= (SRouteDumpCtx *) pContext;

  fprintf(pCtx->pStream, "TABLE_DUMP|%u|", 0);
  route_dump_mrtd(pCtx->pStream, (SRoute *) pItem);
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
  return -1;
}

/////////////////////////////////////////////////////////////////////
// TEST
/////////////////////////////////////////////////////////////////////
