// ==================================================================
// @(#)as.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be), Sebastien Tandel
// @date 22/11/2002
// @lastdate 24/05/2004
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
#include <net/protocol.h>

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
  pAS->pPeers= ptr_array_create(ARRAY_OPTION_SORTED | ARRAY_OPTION_UNIQUE,
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
    ptr_array_destroy(&(*ppAS)->pLocalNetworks);
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

// ----- bgp_router_del_network -------------------------------------
/**
 *
 */
int bgp_router_del_network(SBGPRouter * pRouter, SPrefix sPrefix)
{
  SRoute * pRoute= NULL;
  int iIndex;

  // Looks for the route and mark is as unfeasible
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

    return 0;
  }

  return -1;
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
  route_create_count++;
  rib_add_route(pAS->pLocRIB, route_copy(pRoute));
  route_copy_count++;
  return 0;
}
#endif

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
void as_decision_process_tie_break(SAS * pAS, SRoutes * pRoutes)
{
  // *** shortest AS-PATH ***
  dp_rule_shortest_path(pRoutes);
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

// ----- bgp_router_rt_add_route ------------------------------------
/**
 * This function inserts a BGP route into the node's routing
 * table. The route's next-hop is resolved before insertion.
 */
void bgp_router_rt_add_route(SBGPRouter * pRouter, SRoute * pRoute)
{
  SNetLink * pNextHopIf= node_rt_lookup(pRouter->pNode,
					pRoute->tNextHop);
  int iResult;

  /* Check that the next-hop is reachable. It MUST be reachable at
     this point (checked upon route reception). */
  assert(pNextHopIf != NULL);

  // Remove the previous route (if it exists)
  node_rt_del_route(pRouter->pNode, &pRoute->sPrefix,
		    NULL, NET_ROUTE_BGP);

  // Insert the route
  iResult= node_rt_add_route(pRouter->pNode, pRoute->sPrefix,
			     pNextHopIf->tAddr, 0, NET_ROUTE_BGP);
  if (iResult) {
    LOG_SEVERE("Error: could not add route\nError: ");
    rt_perror(stderr, iResult);
    LOG_SEVERE("\n");
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
  /*
  LOG_WARNING("Debug: BGP removes route towards ");
  ip_prefix_dump(stderr, sPrefix);
  LOG_WARNING("\n");
  */

  assert(!node_rt_del_route(pRouter->pNode, &sPrefix,
			   NULL, NET_ROUTE_BGP));
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
  SNetRouteInfo * pRouteInfo;

  /* Get the route towards the next-hop */
  pRouteInfo= rt_find_best(pRouter->pNode->pRT, pRoute->tNextHop);

  if (pRouteInfo != NULL) {
    route_flag_set(pRoute, ROUTE_FLAG_FEASIBLE, 1);
    return 1;
  } else {
    route_flag_set(pRoute, ROUTE_FLAG_FEASIBLE, 0);
    return 0;
  }
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

  /* BGP QoS decision process */
#ifdef BGP_QOS
  if (BGP_OPTIONS_NLRI == BGP_NLRI_QOS_DELAY)
    return qos_decision_process(pAS, pOriginPeer, sPrefix);
#endif

  LOG_DEBUG("> ");
  LOG_ENABLED_DEBUG() as_dump_id(log_get_stream(pMainLog), pAS);
  LOG_DEBUG(" as_decision_process.begin\n");

  pOldRoute= rib_find_exact(pAS->pLocRIB, sPrefix);
  LOG_DEBUG("\tbest: ");
  LOG_ENABLED_DEBUG() route_dump(log_get_stream(pMainLog), pOldRoute);
  LOG_DEBUG("\n");

  /* Local routes can not be overriden and must be kept in Loc-RIB */
  if ((pOldRoute != NULL) &&
      route_flag_get(pOldRoute, ROUTE_FLAG_INTERNAL))
    return 0;

  // *** lock all Adj-RIB-Ins ***

  /* Build list of eligible routes */
  pRoutes= routes_list_create();
  for (iIndex= 0; iIndex < ptr_array_length(pAS->pPeers); iIndex++) {

    pPeer= (SPeer*) pAS->pPeers->data[iIndex];
    pRoute= rib_find_exact(pPeer->pAdjRIBIn, sPrefix);

    if ((pRoute != NULL) &&
	(route_flag_get(pRoute, ROUTE_FLAG_ELIGIBLE))) {

      /* Clear flag that indicates that the route depends on the
	 IGP. See 'dp_rule_nearest_next_hop' and 'bgp_router_scan_rib'
	 for more information. */
      route_flag_set(pRoute, ROUTE_FLAG_DP_IGP, 0);
      
      /* Update ROUTE_FLAG_FEASIBLE */
      if (bgp_router_feasible_route(pAS, pRoute)) {
	
	routes_list_append(pRoutes, pRoute);

	LOG_DEBUG("\teligible: ");
	LOG_ENABLED_DEBUG() route_dump(log_get_stream(pMainLog), pRoute);
	LOG_DEBUG("\n");

      }
    }
  }

  /* If there is a single eligible & feasible route, it depends on the
     IGP */
  if (ptr_array_length(pRoutes) == 1)
    route_flag_set((SRoute *) pRoutes->data[0], ROUTE_FLAG_DP_IGP, 1);

  // Keep routes with highest degree of preference
  if (ptr_array_length(pRoutes) > 1)
    as_decision_process_dop(pAS, pRoutes);

  // Tie-break
  if (ptr_array_length(pRoutes) > 1)
    as_decision_process_tie_break(pAS, pRoutes);

  assert((ptr_array_length(pRoutes) == 0) ||
    (ptr_array_length(pRoutes) == 1));

  if (ptr_array_length(pRoutes) > 0) {
    route_copy_count++;
    pRoute= route_copy((SRoute *) pRoutes->data[0]);

    LOG_DEBUG("\tnew best: ");
    LOG_ENABLED_DEBUG() route_dump(log_get_stream(pMainLog), pRoute);
    LOG_DEBUG("\n");

    // New/updated route
    // => install in Loc-RIB
    // => advertise to peers
    if ((pOldRoute == NULL) || !route_equals(pOldRoute, pRoute)) {
      if (pOldRoute != NULL)
	bgp_router_best_flag_off(pOldRoute);

      /* Mark route in Loc-RIB and Adj-RIB-In as best. This must be
	 done after the call to 'bgp_router_best_flag_off'. */
      route_flag_set(pRoute, ROUTE_FLAG_BEST, 1);
      route_flag_set(pRoutes->data[0], ROUTE_FLAG_BEST, 1);

      /* Insert in Loc-RIB */
      assert(rib_add_route(pAS->pLocRIB, pRoute) == 0);

      /* Insert in the node's routing table */
      bgp_router_rt_add_route(pAS, pRoute);

      as_decision_process_disseminate(pAS, sPrefix, pRoute);
    } else {

      /* Mark route in Adj-RIB-In as best (since it has probably been
	 replaced). */
      route_flag_set(pRoutes->data[0], ROUTE_FLAG_BEST, 1);

      /* Update ROUTE_FLAG_DP_IGP of old route */
      route_flag_set(pOldRoute, ROUTE_FLAG_DP_IGP,
		     route_flag_get(pRoute, ROUTE_FLAG_DP_IGP));

      // Route has not changed.
      route_destroy(&pRoute);
      pRoute= pOldRoute;

    }

  } else {

    LOG_DEBUG("no best\n");
    // If a route towards this prefix was previously installed, then
    // withdraw it. Otherwise, do nothing...
    if (pOldRoute != NULL) {
      //LOG_DEBUG("there was a previous best-route\n");
      rib_remove_route(pAS->pLocRIB, sPrefix);

      bgp_router_best_flag_off(pOldRoute);

      bgp_router_rt_del_route(pAS, sPrefix);

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
 * Handle a BGP message received from the lower layer (network layer
 * at this time). The function looks for the destination peer and
 * delivers it for processing...
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
    bgp_msg_destroy(&pMsg);
    return -1;
  }
  return 0;
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

// ----- bgp_router_reset -------------------------------------------
/**
 * This function shuts down every peering session, then restarts
 * them.
 */
int bgp_router_reset(SBGPRouter * pRouter)
{
  LOG_SEVERE("Error: not *yet* implemented, sorry.\n");
  return -1;
}

typedef struct {
  SBGPRouter * pRouter;
  SRoutes * pRoutes;
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
  SRoute * pRoute= (SRoute *) pItem;
  SRoute * pAdjRoute;
  int iIndex;
  SPeer * pPeer;
  unsigned int uBestWeight;
  SNetRouteInfo * pRouteInfo;

  /* If the best route has been chosen based on the IGP weight, then
     there is a possible BGP impact */
  if (route_flag_get(pRoute, ROUTE_FLAG_DP_IGP)) {

    fprintf(stderr, "(1) best still reachable ?...\n");

    /* Is there a route (IGP ?) towards the destination ? */
    pRouteInfo= rt_find_best(pRouter->pNode->pRT, pRoute->tNextHop);
    if (pRouteInfo == NULL) {

      /* The next-hop of the best route is now unreachable, thus
	 trigger the decision process */
      routes_list_append(pCtx->pRoutes, pRoute);

      return 0;

    }

    uBestWeight= pRouteInfo->uWeight;

    fprintf(stderr, "(2) look in Adj-RIB-ins...\n");

    /* Lookup in the Adj-RIB-Ins for routes that were also selected
       based on the IGP, that is routes that were compared to the
       current best route based on the IGP cost to the next-hop. These
       routes are marked with the ROUTE_FLAG_DP_IGP flag */
    for (iIndex= 0; iIndex < ptr_array_length(pRouter->pPeers);
	 iIndex++) {

      pPeer= (SPeer *) pRouter->pPeers->data[iIndex];

      pAdjRoute= rib_find_exact(pPeer->pAdjRIBIn, pRoute->sPrefix);

      /* Is there a route (IGP ?) towards the destination ? */
      pRouteInfo= rt_find_best(pRouter->pNode->pRT, pAdjRoute->tNextHop);

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
	routes_list_append(pCtx->pRoutes, pRoute);
	
	return 0;
	
      } else if (pRouteInfo != NULL) {
	
	/* IGP cost is below cost of the best route, thus run the
	   decision process */
	if (pRouteInfo->uWeight < uBestWeight) {
	  
	routes_list_append(pCtx->pRoutes, pRoute);

	  
	  return 0;
	  
	}	

      }

    }

  }
  
  return 0;
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

  /* initialize context */
  sCtx.pRouter= pRouter;
  sCtx.pRoutes= routes_list_create();

  /* Traverses the whole Loc-RIB in order to find prefixes that depend
     on the IGP (links up/down and metric changes) */
  iResult= rib_for_each(pRouter->pLocRIB,
			bgp_router_scan_rib_for_each,
			&sCtx);
  
  /* For each route in the list, run the BGP decision process */
  if (iResult == 0)
    for (iIndex= 0; iIndex < routes_list_get_num(sCtx.pRoutes); iIndex++)
      as_decision_process(pRouter, NULL,
			  ((SRoute *) sCtx.pRoutes->data[iIndex])->sPrefix);

  routes_list_destroy(&sCtx.pRoutes);

  return iResult;
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
  if (pRoute != NULL) {
    route_dump(pStream, pRoute);
    fprintf(pStream, "\n");
  }
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
  SRoutes * pRoutes= mrtd_load_routes(pRouter, pcFileName);
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
			    SPrefix sPrefix, SPath ** ppPath,
			    int iPreserveDups)
{
  SBGPRouter * pCurrentRouter= pRouter;
  SBGPRouter * pPreviousRouter= NULL;
  SRoute * pRoute;
  SPath * pPath= path_create();
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
      pNode= network_find_node(pRouter->pNode->pNetwork, pRoute->tNextHop);
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

// ----- bgp_router_bm_route ----------------------------------------
/**
 * *** EXPERIMENTAL ***
 *
 * Returns the best route among the routes in the Loc-RIB that match
 * the given prefix with a bound on the prefix length.
 */
SRoute * bgp_router_bm_route(SBGPRouter * pRouter, SPrefix sPrefix,
			     uint8_t uBound)
{
  SRoutes * pRoutes;
  SPrefix sBoundedPrefix;
  SRoute * pRoute= NULL;
  int iLocalExists= 0;

  sBoundedPrefix.tNetwork= sPrefix.tNetwork;

  pRoutes= routes_list_create();

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
      as_decision_process_dop(pRouter, pRoutes);
    
    /* BGP-DP: other rules */
    if (routes_list_get_num(pRoutes) > 0)
      as_decision_process_tie_break(pRouter, pRoutes);

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

// ----- bgp_router_record_route_bounded_match ----------------------
/**
 * *** EXPERIMENTAL ***
 *
 * This function records the AS-path from one BGP router towards a
 * given prefix. The function has two modes:
 * - records all ASes
 * - records ASes once (do not record iBGP session crossing)
 */
int bgp_router_record_route_bounded_match(SBGPRouter * pRouter,
					  SPrefix sPrefix,
					  uint8_t uBound,
					  SPath ** ppPath,
					  int iPreserveDups)
{
  SBGPRouter * pCurrentRouter= pRouter;
  SBGPRouter * pPreviousRouter= NULL;
  SRoute * pRoute;
  SPath * pPath= path_create();
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
      pNode= network_find_node(pRouter->pNode->pNetwork, pRoute->tNextHop);
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
				    SPath * pPath,
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
}

/////////////////////////////////////////////////////////////////////
// TEST
/////////////////////////////////////////////////////////////////////
