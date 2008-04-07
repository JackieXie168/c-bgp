// ==================================================================
// @(#)as.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @author Sebastien Tandel (standel@info.ucl.ac.be)
// @date 22/11/2002
// $Id: as.c,v 1.71 2008-04-07 09:03:11 bqu Exp $
// ==================================================================
// TO-DO LIST:
// - do not keep in pLocalNetworks a _copy_ of the BGP routes locally
//   originated. Store a reference instead => reduce memory footprint.
// - re-check IGP/BGP interaction
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

#include <libgds/types.h>
#include <libgds/array.h>
#include <libgds/list.h>
#include <libgds/log.h>
#include <libgds/memory.h>
#include <libgds/str_util.h>
#include <libgds/tokenizer.h>

#include <bgp/as.h>
#include <bgp/auto-config.h>
#include <bgp/comm.h>
#include <bgp/domain.h>
#include <bgp/dp_rt.h>
#include <bgp/dp_rules.h>
#include <bgp/ecomm.h>
#include <bgp/mrtd.h>
#include <bgp/peer.h>
#include <bgp/peer-list.h>
#include <bgp/qos.h>
#include <bgp/routes_list.h>
#include <bgp/route-input.h>
#include <bgp/tie_breaks.h>
#include <net/network.h>
#include <net/link.h>
#include <net/node.h>
#include <net/protocol.h>
#include <ui/output.h>
#include <bgp/walton.h>

// ----- options -----
FTieBreakFunction BGP_OPTIONS_TIE_BREAK	    = TIE_BREAK_DEFAULT;
uint32_t BGP_OPTIONS_DEFAULT_LOCAL_PREF	    = 0;
uint8_t BGP_OPTIONS_MED_TYPE		    = BGP_MED_TYPE_DETERMINISTIC;
uint8_t BGP_OPTIONS_SHOW_MODE		    = BGP_ROUTES_OUTPUT_CISCO;
char * BGP_OPTIONS_SHOW_FORMAT              = NULL;
uint8_t BGP_OPTIONS_RIB_OUT		    = 1;
uint8_t BGP_OPTIONS_AUTO_CREATE		    = 0;
uint8_t BGP_OPTIONS_VIRTUAL_ADJ_RIB_OUT	    = 0;
#ifdef __EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
uint8_t BGP_OPTIONS_ADVERTISE_EXTERNAL_BEST = 0;
#endif
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
uint8_t BGP_OPTIONS_WALTON_CONVERGENCE_ON_BEST = 0;
#endif
//#define NO_RIB_OUT

static FBGPMsgListener fListener= NULL;
static void * pContext;

// -----[ bgp_router_set_msg_listener ]------------------------------
void bgp_router_set_msg_listener(FBGPMsgListener f, void * p)
{
  fListener= f;
  pContext= p;
}

// -----[ _bgp_router_msg_listener ]---------------------------------
static inline void _bgp_router_msg_listener(net_msg_t * msg)
{
  if (fListener != NULL)
    fListener(msg, pContext);
}

// -----[ _bgp_router_select_rid ]-----------------------------------
/**
 * The BGP Router-ID of this router should be the highest IP address
 * of the router with preference to loopback interfaces.
 * In the current C-BGP implementation, there is only one loopback
 * interface and its IP address is the node's identifier.
 */
static inline net_addr_t _bgp_router_select_rid(net_node_t * node)
{
  return node->tAddr;
}

// -----[ bgp_add_router ]-------------------------------------------
/**
 *
 */
int bgp_add_router(uint16_t uASN, net_node_t * pNode,
		   bgp_router_t ** ppRouter)
{
  net_error_t error;
  bgp_router_t * pRouter;

  pRouter= bgp_router_create(uASN, pNode);

  // Register BGP protocol into the node
  error= node_register_protocol(pNode, NET_PROTOCOL_BGP, pRouter,
				(FNetProtoHandlerDestroy) bgp_router_destroy,
				bgp_router_handle_message);
  if (error != ESUCCESS)
    return error;

  if (ppRouter != NULL)
    *ppRouter= pRouter;
  return ESUCCESS;
}

// -----[ bgp_router_create ]----------------------------------------
/**
 * Create a BGP router in the given node.
 *
 * Arguments:
 * - the AS-number of the domain the router will belong to
 * - the underlying node
 */
bgp_router_t * bgp_router_create(uint16_t uASN, net_node_t * pNode)
{
  bgp_router_t * pRouter= (bgp_router_t*) MALLOC(sizeof(bgp_router_t));

  pRouter->uASN= uASN;
  pRouter->tRouterID= _bgp_router_select_rid(pNode);
  pRouter->pPeers= bgp_peers_create();
  pRouter->pLocRIB= rib_create(0);
  pRouter->pLocalNetworks= routes_list_create(ROUTES_LIST_OPTION_REF);
  pRouter->fTieBreak= BGP_OPTIONS_TIE_BREAK;
  pRouter->tClusterID= pNode->tAddr;
  pRouter->iRouteReflector= 0;

  // Reference to the node running this BGP router
  pRouter->pNode= pNode;
  // Register the router into its AS
  bgp_domain_add_router(get_bgp_domain(uASN), pRouter);

#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  bgp_router_walton_init(pRouter);
#endif 
  return pRouter;
}

// -----[ bgp_router_destroy ]---------------------------------------
/**
 * Free the memory used by the given BGP router.
 */
void bgp_router_destroy(bgp_router_t ** ppRouter)
{
  unsigned int uIndex;
  bgp_route_t * pRoute;

  if (*ppRouter != NULL) {
    bgp_peers_destroy(&(*ppRouter)->pPeers);
    rib_destroy(&(*ppRouter)->pLocRIB);
    for (uIndex= 0; uIndex < bgp_routes_size((*ppRouter)->pLocalNetworks);
	uIndex++) {
      pRoute= bgp_routes_at((*ppRouter)->pLocalNetworks, uIndex);
      route_destroy(&pRoute);
    }
    ptr_array_destroy(&(*ppRouter)->pLocalNetworks);
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
    bgp_router_walton_finalize((*ppRouter));
#endif
    FREE(*ppRouter);
    *ppRouter= NULL;
  }
}

// -----[ bgp_router_find_peer ]-------------------------------------
/**
 * Find a BGP neighbor in a BGP router. Lookup is done based on
 * neighbor's address.
 *
 * Returns:
 *   NULL in case the neighbor was not found.
 *   pointer to neighbor if it exists
 */
bgp_peer_t * bgp_router_find_peer(bgp_router_t * pRouter, net_addr_t tAddr)
{
  return bgp_peers_find(pRouter->pPeers, tAddr);
}

// -----[ bgp_router_add_peer ]--------------------------------------
/**
 * Add a BGP neighbor to a BGP router.
 *
 * Conditions:
 *   - router must not own peer's address.
 *   - same router must not exist
 *
 * Returns:
 *   ESUCCESS in case of success (also returns a pointer to the new
 *                                peer if ppPeer != NULL)
 *   < 0 in case of failure
 */
int bgp_router_add_peer(bgp_router_t * pRouter, uint16_t uRemoteAS,
			net_addr_t tAddr, bgp_peer_t ** ppPeer)
{
  bgp_peer_t * pNewPeer;
  int iResult= ESUCCESS;

  // Router must not own peer's address
  if (node_has_address(pRouter->pNode, tAddr))
    return EBGP_PEER_INVALID_ADDR;

  pNewPeer= bgp_peer_create(uRemoteAS, tAddr, pRouter);
  
  iResult= bgp_peers_add(pRouter->pPeers, pNewPeer);
  if (iResult != ESUCCESS) {
    bgp_peer_destroy(&pNewPeer);
    return iResult;
  }

  if (ppPeer != NULL)
    *ppPeer= pNewPeer;

  return iResult;
}

// ----- bgp_router_peer_set_filter ---------------------------------
/**
 *
 */
int bgp_router_peer_set_filter(bgp_router_t * pRouter, net_addr_t tAddr,
			       bgp_filter_dir_t dir, bgp_filter_t * pFilter)
{
  bgp_peer_t * pPeer;

  if ((pPeer= bgp_peers_find(pRouter->pPeers, tAddr)) != NULL) {
    bgp_peer_set_filter(pPeer, dir, pFilter);
    return ESUCCESS;
  }
  return EBGP_PEER_UNKNOWN;
}

// -----[ _bgp_router_add_route ]------------------------------------
/**
 * Add a route in the Loc-RIB
 */
static inline int _bgp_router_add_route(bgp_router_t * pRouter,
					bgp_route_t * pRoute)
{
  routes_list_append(pRouter->pLocalNetworks, pRoute);
  rib_add_route(pRouter->pLocRIB, route_copy(pRoute));
  bgp_router_decision_process_disseminate(pRouter, pRoute->sPrefix, pRoute);
  return ESUCCESS;
}

// ----- bgp_router_add_network -------------------------------------
/**
 * This function adds a route towards the given prefix. This route
 * will be originated by this router.
 */
int bgp_router_add_network(bgp_router_t * pRouter, SPrefix sPrefix)
{
  bgp_route_t * route;
  unsigned int index;

  // Check that a route with the same prefix does not already exist
  for (index= 0; index < bgp_routes_size(pRouter->pLocalNetworks); index++) {
    route= bgp_routes_at(pRouter->pLocalNetworks, index);
    if (!ip_prefix_cmp(&route->sPrefix, &sPrefix))
      return EBGP_NETWORK_DUPLICATE;
  }

  // Create route and make it feasible (i.e. add flags BEST,
  // FEASIBLE and INTERNAL)
  route= route_create(sPrefix, NULL, pRouter->pNode->tAddr,
		      ROUTE_ORIGIN_IGP);
  route_flag_set(route, ROUTE_FLAG_BEST, 1);
  route_flag_set(route, ROUTE_FLAG_FEASIBLE, 1);
  route_flag_set(route, ROUTE_FLAG_INTERNAL, 1);
  route_localpref_set(route, BGP_OPTIONS_DEFAULT_LOCAL_PREF);
#ifdef __EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
  route_flag_set(route, ROUTE_FLAG_EXTERNAL_BEST, 0);
#endif
  return _bgp_router_add_route(pRouter, route);
}

// ----- bgp_router_del_network -------------------------------------
/**
 * This function removes a locally originated network.
 */
int bgp_router_del_network(bgp_router_t * pRouter, SPrefix sPrefix)
{
  bgp_route_t * pRoute= NULL;
  unsigned int uIndex;

  LOG_DEBUG_ENABLED(LOG_LEVEL_DEBUG) {
    log_printf(pLogDebug, "bgp_router_del_network(");
    bgp_router_dump_id(pLogDebug, pRouter);
    log_printf(pLogDebug, ", ");
    ip_prefix_dump(pLogDebug, sPrefix);
    log_printf(pLogDebug, ")\n");
  }

  // Look for the route and mark it as unfeasible
  for (uIndex= 0; uIndex < bgp_routes_size(pRouter->pLocalNetworks);
      uIndex++) {
    pRoute= bgp_routes_at(pRouter->pLocalNetworks, uIndex);
    if (!ip_prefix_cmp(&pRoute->sPrefix, &sPrefix)) {
      route_flag_set(pRoute, ROUTE_FLAG_FEASIBLE, 0);
      break;
    }
    pRoute= NULL;
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

#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
    rib_remove_route(pRouter->pLocRIB, sPrefix, NULL);
#else
    rib_remove_route(pRouter->pLocRIB, sPrefix);
#endif

    // Remove the route from the list of local networks.
    routes_list_remove_at(pRouter->pLocalNetworks, uIndex);

    // Free route
    route_destroy(&pRoute);

    //log_printf(pLogDebug, "RUN DECISION PROCESS...\n");

    bgp_router_decision_process_disseminate(pRouter, sPrefix, NULL);

    return ESUCCESS;
  }
  return EUNSPECIFIED;
}

// ----- as_add_qos_network -----------------------------------------
/**
 *
 */
#ifdef BGP_QOS
int as_add_qos_network(SAS * pAS, SPrefix sPrefix,
    net_link_delay_t tDelay)
{
  bgp_route_t * pRoute= route_create(sPrefix, NULL,
      pAS->pNode->tAddr, ROUTE_ORIGIN_IGP);
  route_flag_set(pRoute, ROUTE_FLAG_FEASIBLE, 1);
  route_localpref_set(pRoute, BGP_OPTIONS_DEFAULT_LOCAL_PREF);

  qos_route_update_delay(pRoute, tDelay);
  pRoute->tDelay.uWeight= 1;
  qos_route_update_bandwidth(pRoute, 0);
  pRoute->tBandwidth.uWeight= 1;

  return _bgp_router_add_route(pAS, pRoute);
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
int bgp_router_ecomm_process(bgp_peer_t * pPeer, bgp_route_t * pRoute)
{
  int iIndex;
  uint8_t uActionParam;

  if (pRoute->pAttr->pECommunities != NULL) {

    for (iIndex= 0; iIndex < ecomm_length(pRoute->pAttr->pECommunities); iIndex++) {
      SECommunity * pComm= ecomm_get_index(pRoute->pAttr->pECommunities, iIndex);
      switch (pComm->uTypeHigh) {
	case ECOMM_RED:
	  if (ecomm_red_match(pComm, pPeer)) {
	    switch ((pComm->uTypeLow >> 3) & 0x07) {
	      case ECOMM_RED_ACTION_PREPEND:
		uActionParam= (pComm->uTypeLow & 0x07);
		route_path_prepend(pRoute, pPeer->pLocalRouter->uASN,
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

// -----[ _bgp_router_advertise_to_peer ]----------------------------
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
static inline int _bgp_router_advertise_to_peer(bgp_router_t * pRouter,
						bgp_peer_t * pDstPeer,
						bgp_route_t * pRoute)
{
#define INTERNAL 0
#define EXTERNAL 1
#define LOCAL    2
  char * acLocType[3]= { "INT", "EXT", "LOC" };

  bgp_route_t * pNewRoute= NULL;
  int iFrom= LOCAL;
  int iTo;
  bgp_peer_t * pSrcPeer= NULL;

  // Determine route origin type (internal/external/local)
  if (pRoute->pAttr->tNextHop != pRouter->pNode->tAddr) {
    pSrcPeer= route_peer_get(pRoute);
    if (pRouter->uASN == route_peer_get(pRoute)->uRemoteAS)
      iFrom= INTERNAL;
    else
      iFrom= EXTERNAL;
  }
  
  // Determine route destination type (internal/external)
  if (pRouter->uASN == pDstPeer->uRemoteAS)
    iTo= INTERNAL;
  else
    iTo= EXTERNAL;

  LOG_DEBUG_ENABLED(LOG_LEVEL_DEBUG) {
    log_printf(pLogDebug, "advertise_to_peer (");
    ip_prefix_dump(pLogDebug, pRoute->sPrefix);
    log_printf(pLogDebug, ") from ");
    bgp_router_dump_id(pLogDebug, pRouter);
    log_printf(pLogDebug, " to ");
    bgp_peer_dump_id(pLogDebug, pDstPeer);
    log_printf(pLogDebug, " (%s --> %s)\n",
	       acLocType[iFrom], acLocType[iTo]);
  }
  
#ifdef __ROUTER_LIST_ENABLE__
  // Append the router-list with the IP address of this router
  route_router_list_append(pRoute, pRouter->pNode->tAddr);
#endif

  // Do not redistribute to the originator neighbor peer
  if ((iFrom != LOCAL) &&
      (pDstPeer->tRouterID == pSrcPeer->tRouterID)) {
    LOG_DEBUG(LOG_LEVEL_DEBUG, "out-filtered(next-hop-peer)\n");
    return -1;
  }

  // Do not redistribute to other peers
  if (route_comm_contains(pRoute, COMM_NO_ADVERTISE)) {
    LOG_DEBUG(LOG_LEVEL_DEBUG, "out-filtered(comm_no_advertise)\n");
    return -1;
  }

  // Copy the route. This is required since subsequent filters may
  // alter the route's attribute !!
  pNewRoute= route_copy(pRoute);

  // ******************** ROUTE-REDISTRIBUTION **********************
  //
  // +----------+----------+------+------------------------------+
  // | FROM     | TO       | iBGP | ROUTE-REFLECTOR              |
  // +----------+----------+------+------------------------------+
  // | EXTERNAL | EXTERNAL |  OK  |   OK                         | (1)
  // | INTERNAL | EXTERNAL |  OK  |   OK                         | (2)
  // | LOCAL    | EXTERNAL |  OK  |   OK                         | (3)
  // | EXTERNAL | INTERNAL |  OK  |   OK                         | (4)
  // | INTERNAL | INTERNAL | ---- |                              | (5)
  // | client   | client   |      |   OK (update Originator-ID,  | (5a)
  // |          |          |      |       Cluster-ID-List)       |
  // | n-client | client   |      |   OK (update Originator-ID,  | (5b)
  // |          |          |      |       Cluster-ID-List)       |
  // | n-client | n-client |      |  ----                        | (5c)
  // | LOCAL    | INTERNAL |  OK  |   OK                         | (6)
  // +----------+----------+------+------------------------------+
  //
  // ****************************************************************
  switch (iTo) {
  case EXTERNAL: // case (1), (2) and (3)
    // Do not redistribute outside confederation (here AS)
    if (route_comm_contains(pNewRoute, COMM_NO_EXPORT)) {
      LOG_DEBUG(LOG_LEVEL_DEBUG, "out-filtered(comm_no_export)\n");
      route_destroy(&pNewRoute);
      return -1;
    }
    // Avoid loop creation (SSLD, Sender-Side Loop Detection)
    if (route_path_contains(pNewRoute, pDstPeer->uRemoteAS)) {
      LOG_DEBUG(LOG_LEVEL_DEBUG, "out-filtered(ssld)\n");
      route_destroy(&pNewRoute);
      return -1;
    }
    // Clear Originator and Cluster-ID-List fields
    route_originator_clear(pNewRoute);
    route_cluster_list_clear(pNewRoute);
    break;

  case INTERNAL:
    switch (iFrom) {
    case EXTERNAL: // case (4)
      break;
    case INTERNAL: // case (5)
      if (pRouter->iRouteReflector) {

	// Do not redistribute from non-client to non-client (5c)
	if (!route_flag_get(pNewRoute, ROUTE_FLAG_RR_CLIENT) &&
	    !bgp_peer_flag_get(pDstPeer, PEER_FLAG_RR_CLIENT)) {
	  LOG_DEBUG(LOG_LEVEL_DEBUG, "out-filtered (RR: non-client --> non-client)\n");
	  route_destroy(&pNewRoute);
	  return -1;
	}

	// Update Originator-ID if missing (< 0 => missing)
	if (route_originator_get(pNewRoute, NULL) < 0)
	  route_originator_set(pNewRoute, pSrcPeer->tRouterID);
	else {
	  if (originator_equals(pNewRoute->pAttr->pOriginator,
				&pDstPeer->tRouterID)) {
	    LOG_DEBUG(LOG_LEVEL_DEBUG, "out-filtered (originator-id SSLD)\n");
	    route_destroy(&pNewRoute);
	    return -1;
	  }
	}

	// Create or append Cluster-ID-List
	route_cluster_list_append(pNewRoute, pRouter->tClusterID);

      } else {
	LOG_DEBUG(LOG_LEVEL_DEBUG, "out-filtered(iBGP-peer --> iBGP-peer)\n");
	route_destroy(&pNewRoute);
	return -1;
      }
      break;

    case LOCAL: // case (6)
      // Set Originator-ID
      if (pRouter->iRouteReflector)
	route_originator_set(pNewRoute, pRouter->tRouterID);
      break;
    }
    
    break;
    
  }
  
  // Check output filter and extended communities
  if (!bgp_router_ecomm_process(pDstPeer, pNewRoute)) {
    LOG_DEBUG(LOG_LEVEL_DEBUG, "out-filtered (ext-community)\n");
    route_destroy(&pNewRoute);
    return -1;
  }

  // Remove non-transitive communities
  route_ecomm_strip_non_transitive(pNewRoute);

  // Discard MED if advertising to an external peer
  // (must be done before applying policy filters)
  if (iTo == EXTERNAL)
    route_med_clear(pNewRoute);
  
  // Apply policy filters (output)
  if (!filter_apply(pDstPeer->pFilter[FILTER_OUT], pRouter, pNewRoute)) {
    LOG_DEBUG(LOG_LEVEL_DEBUG, "out-filtered (policy)\n");
    route_destroy(&pNewRoute);
    return -1;
  }

  // Update attributes before redistribution (next-hop, AS-Path, RR)
  if (iTo == EXTERNAL) {
	
    // Change the route's next-hop to this router
    // (or optionally to a specified next-hop)
    if (pDstPeer->tNextHop != 0)
      route_set_nexthop(pNewRoute, pDstPeer->tNextHop);
    else
      route_set_nexthop(pNewRoute, pRouter->pNode->tAddr);
    
    // Prepend AS-Number
    route_path_prepend(pNewRoute, pRouter->uASN, 1);
    
  } else if (iTo == INTERNAL) {
    
    // Change the route's next-hop to this router (next-hop-self)
    // or to an optionally specified next-hop. RESTRICTION: this
    // is prohibited if the route is being reflected.
    if (iFrom == EXTERNAL) {
      if (bgp_peer_flag_get(route_peer_get(pNewRoute),
			    PEER_FLAG_NEXT_HOP_SELF)) {
	route_set_nexthop(pNewRoute, pRouter->pNode->tAddr);
      } else if (pDstPeer->tNextHop != 0) {
	route_set_nexthop(pNewRoute, pDstPeer->tNextHop);
      }
    }
    
  }
  
  bgp_peer_announce_route(pDstPeer, pNewRoute);
  return 0;
}

// -----[ _bgp_router_withdraw_to_peer ]-----------------------------
/**
 * Withdraw the given prefix to the given peer router.
 */
static inline void _bgp_router_withdraw_to_peer(bgp_router_t * pRouter,
						bgp_peer_t * pPeer,
						SPrefix sPrefix)
{
  LOG_DEBUG(LOG_LEVEL_DEBUG,
	    "*** AS%d withdraw to AS%d ***\n", pRouter->uASN,
	    pPeer->uRemoteAS);

#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  // WARNING : It is not used anymore, we don't udate then
#else
  bgp_peer_withdraw_prefix(pPeer, sPrefix);
#endif

}

// ----- bgp_router_start -------------------------------------------
/**
 * Run the BGP router. The router starts by sending its local prefixes
 * to its peers.
 */
int bgp_router_start(bgp_router_t * pRouter)
{
  unsigned int uIndex;

  // Open all BGP sessions
  for (uIndex= 0; uIndex < bgp_peers_size(pRouter->pPeers); uIndex++)
    if (bgp_peer_open_session(bgp_peers_at(pRouter->pPeers, uIndex)) != 0)
      return -1;
  return ESUCCESS;
}

// ----- bgp_router_stop --------------------------------------------
/**
 * Stop the BGP router. Each peering session is brought to IDLE
 * state.
 */
int bgp_router_stop(bgp_router_t * pRouter)
{
  unsigned int uIndex;

  // Close all BGP sessions
  for (uIndex= 0; uIndex < bgp_peers_size(pRouter->pPeers); uIndex++)
    if (bgp_peer_close_session(bgp_peers_at(pRouter->pPeers, uIndex)) != 0)
      return -1;
  return ESUCCESS;
}

// ----- bgp_router_reset -------------------------------------------
/**
 * This function shuts down every peering session, then restarts
 * them.
 */
int bgp_router_reset(bgp_router_t * pRouter)
{
  int iResult;

  // Stop the router
  iResult= bgp_router_stop(pRouter);
  if (iResult != ESUCCESS)
    return -1;

  // Start the router
  iResult= bgp_router_start(pRouter);
  if (iResult != ESUCCESS)
    return -1;

  return ESUCCESS;
}

#ifdef __EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
typedef struct {
  bgp_router_t * pRouter;
  bgp_routes_t * pBestEBGPRoutes;
} SSecondBestCtx;

int bgp_router_copy_ebgp_routes(void * pItem, void * pContext)
{
  SSecondBestCtx * pSecondRoutes = (SSecondBestCtx *) pContext;
  bgp_route_t * pRoute = *(bgp_route_t ** )pItem;

  if (pRoute->pPeer->uRemoteAS != pSecondRoutes->pRouter->uASN) {
    if (pSecondRoutes->pBestEBGPRoutes == NULL)
      pSecondRoutes->pBestEBGPRoutes = routes_list_create(ROUTES_LIST_OPTION_REF);
    routes_list_append(pSecondRoutes->pBestEBGPRoutes, pRoute);
  }
  return 0;
}
#endif

// -----[ bgp_router_peer_rib_out_remove ]---------------------------
/**
 * Check in the peer's Adj-RIB-in if it has received from this router
 * a route towards the given prefix.
 */
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
int bgp_router_peer_rib_out_remove(bgp_router_t * pRouter,
    bgp_peer_t * pPeer,
    SPrefix sPrefix,
    net_addr_t * tNextHop)
#else
int bgp_router_peer_rib_out_remove(bgp_router_t * pRouter,
    bgp_peer_t * pPeer,
    SPrefix sPrefix)
#endif
{
  int iWithdrawRequired= 0;
#ifdef NO_RIB_OUT
  network_t * pNetwork= network_get();
  net_node_t * pNode;
  SNetProtocol * pProtocol;
  bgp_router_t * pPeerRouter;
  bgp_peer_t * pThePeer;
#endif

#ifndef NO_RIB_OUT
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  if (tNextHop != NULL) {
    if (rib_find_one_exact(pPeer->pAdjRIB[RIB_OUT], sPrefix, tNextHop) != NULL) {
      rib_remove_route(pPeer->pAdjRIB[RIB_OUT], sPrefix, tNextHop);
      iWithdrawRequired = 1;
    }
  } else {
    if (rib_find_exact(pPeer->pAdjRIB[RIB_OUT], sPrefix) != NULL) {
      rib_remove_route(pPeer->pAdjRIB[RIB_OUT], sPrefix, tNextHop);
      iWithdrawRequired = 1;
    }
  }
#else
  if (rib_find_exact(pPeer->pAdjRIB[RIB_OUT], sPrefix) != NULL) {
    rib_remove_route(pPeer->pAdjRIB[RIB_OUT], sPrefix);
    iWithdrawRequired= 1;
  }
#endif
#else
  pNode= network_find_node(pPeer->tAddr);
  assert(pNode != NULL);
  pProtocol= protocols_get(pNode->pProtocols, NET_PROTOCOL_BGP);
  assert(pProtocol != NULL);
  pPeerRouter= (bgp_router_t *) pProtocol->pHandler;
  assert(pPeerRouter != NULL);
  pThePeer= bgp_router_find_peer(pPeerRouter, pRouter->pNode->tAddr);
  assert(pThePeer != NULL);

#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  //TODO Walton : what to do when there is no more Adj-RIB-Out
#else
  if (rib_find_exact(pThePeer->pAdjRIB[RIB_IN], sPrefix) != NULL)
#endif
    iWithdrawRequired= 1;
#endif

  return iWithdrawRequired;
}

// -----[ _bgp_router_decision_process_run ]-------------------------
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
static inline int _bgp_router_decision_process_run(bgp_router_t * pRouter,
						   bgp_routes_t * pRoutes)
{
  int iRule;

  // Apply the decision process rules in sequence until there is 1 or
  // 0 route remaining or until all the rules were applied.
  for (iRule= 0; iRule < DP_NUM_RULES; iRule++) {
    if (bgp_routes_size(pRoutes) <= 1)
      break;
    LOG_DEBUG(LOG_LEVEL_DEBUG, "rule: [ %s ]\n", DP_RULES[iRule].pcName);
    DP_RULES[iRule].fRule(pRouter, pRoutes);
  }

  // Check that at most a single best route will be returned.
  if (bgp_routes_size(pRoutes) > 1) {
    LOG_ERR(LOG_LEVEL_FATAL, "Error: decision process did not return a single best route\n");
    abort();
  }
  
  return iRule;
}

// -----[ bgp_router_peer_rib_out_replace ]--------------------------
/**
 *
 */
void bgp_router_peer_rib_out_replace(bgp_router_t * pRouter,
				     bgp_peer_t * pPeer,
				     bgp_route_t * pNewRoute)
{
#ifndef NO_RIB_OUT
  rib_replace_route(pPeer->pAdjRIB[RIB_OUT], pNewRoute);
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
void bgp_router_decision_process_disseminate_to_peer(bgp_router_t * pRouter,
						     SPrefix sPrefix,
						     bgp_route_t * pRoute,
						     bgp_peer_t * pPeer)
{
  bgp_route_t * pNewRoute;
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  net_addr_t tNextHop;
#endif

  if (pPeer->tSessionState == SESSION_STATE_ESTABLISHED) {
    LOG_DEBUG_ENABLED(LOG_LEVEL_DEBUG) {
      log_printf(pLogDebug, "DISSEMINATE (");
      ip_prefix_dump(pLogDebug, sPrefix);
      log_printf(pLogDebug, ") from ");
      bgp_router_dump_id(pLogDebug, pRouter);
      log_printf(pLogDebug, " to ");
      bgp_peer_dump_id(pLogDebug, pPeer);
      log_printf(pLogDebug, "\n");
    }


    if (pRoute == NULL) {
      // A route was advertised to this peer => explicit withdraw
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
      if (bgp_router_peer_rib_out_remove(pRouter, pPeer, sPrefix, NULL)) {
	bgp_peer_withdraw_prefix(pPeer, sPrefix, NULL);
#else
      if (bgp_router_peer_rib_out_remove(pRouter, pPeer, sPrefix)) {
	bgp_peer_withdraw_prefix(pPeer, sPrefix);
#endif
	LOG_DEBUG(LOG_LEVEL_DEBUG, "\texplicit-withdraw\n");
      }
    } else {
      pNewRoute= route_copy(pRoute);
      if (_bgp_router_advertise_to_peer(pRouter, pPeer, pNewRoute) == 0) {
	LOG_DEBUG(LOG_LEVEL_DEBUG, "\treplaced\n");
	bgp_router_peer_rib_out_replace(pRouter, pPeer, pNewRoute);
      } else {
	route_destroy(&pNewRoute);

	LOG_DEBUG(LOG_LEVEL_DEBUG, "\tfiltered\n");

#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
	tNextHop= route_get_nexthop(pRoute);
	if (bgp_router_peer_rib_out_remove(pRouter, pPeer, sPrefix, &tNextHop)) {
	  LOG_DEBUG(LOG_LEVEL_DEBUG, "\texplicit-withdraw\n");
	  //As it may be an eBGP session, the Next-Hop may have changed!
	  //If the next-hop has changed the path identifier as well ... :)
	  if (pPeer->uRemoteAS != pRouter->uASN)
	    tNextHop = pRouter->pNode->tAddr;
	  else
	    tNextHop = route_get_nexthop(pRoute);
	  bgp_peer_withdraw_prefix(pPeer, sPrefix, &(tNextHop));
#else
	if (bgp_router_peer_rib_out_remove(pRouter, pPeer, sPrefix)) {
	  LOG_DEBUG(LOG_LEVEL_DEBUG, "\texplicit-withdraw\n");
	  bgp_peer_withdraw_prefix(pPeer, sPrefix);
#endif

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
void bgp_router_decision_process_disseminate(bgp_router_t * pRouter,
					     SPrefix sPrefix,
					     bgp_route_t * pRoute)
{
  unsigned int index;
  bgp_peer_t * pPeer;

  for (index= 0; index < bgp_peers_size(pRouter->pPeers); index++) {
    pPeer= bgp_peers_at(pRouter->pPeers, index);
    if (bgp_peer_send_enabled(pPeer)) {
      bgp_router_decision_process_disseminate_to_peer(pRouter, sPrefix,
						      pRoute, pPeer);
    }
  }
}

#ifdef __EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
void bgp_router_decision_process_disseminate_external_best(bgp_router_t * pRouter,
							  SPrefix sPrefix,
							  bgp_route_t * pRoute)
{
  unsigned int index;
  bgp_peer_t * pPeer;

  for (index= 0; index < bgp_peers_size(pRouter->pPeers); index++) {
    pPeer= bgp_peers_at(pRouter->pPeers, index);
    if (pRouter->uASN == pPeer->uRemoteAS)
      bgp_router_decision_process_disseminate_to_peer(pRouter, sPrefix,
						      pRoute, pPeer);
  }
}
#endif

// ----- bgp_router_best_flag_off -----------------------------------
/**
 * Clear the BEST flag from the given Loc-RIB route. Clear the BEST
 * flag from the corresponding Adj-RIB-In route as well.
 *
 * PRE: the route is in the Loc-RIB (=> the route is local or there
 * exists a corresponding route in an Adj-RIB-In).
 */
void bgp_router_best_flag_off(bgp_route_t * pOldRoute)
{
  bgp_route_t * pAdjRoute;
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  net_addr_t tNextHop;
#endif

  /* Remove BEST flag from old route in Loc-RIB. */
  route_flag_set(pOldRoute, ROUTE_FLAG_BEST, 0);

  /* If the route is not LOCAL, then there is a reference to the peer
     that announced it. */
  if (pOldRoute->pPeer != NULL) {
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
    tNextHop = route_get_nexthop(pOldRoute);
    pAdjRoute = rib_find_one_exact(pOldRoute->pPeer->pAdjRIB[RIB_IN], 
			    pOldRoute->sPrefix, &tNextHop);
#else
    pAdjRoute= rib_find_exact(pOldRoute->pPeer->pAdjRIB[RIB_IN],
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
int bgp_router_feasible_route(bgp_router_t * pRouter, bgp_route_t * pRoute)
{
  const rt_entry_t * rtentry;

  /* Get the route towards the next-hop */
  rtentry= node_rt_lookup(pRouter->pNode, pRoute->pAttr->tNextHop);

  if (rtentry != NULL) {
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
bgp_routes_t * bgp_router_get_best_routes(bgp_router_t * pRouter, SPrefix sPrefix)
{
  bgp_routes_t * pRoutes;
  bgp_route_t * pRoute;

  pRoutes= routes_list_create(ROUTES_LIST_OPTION_REF);

#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  pRoute= rib_find_one_exact(pRouter->pLocRIB, sPrefix, NULL);
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
  bgp_routes_t * bgp_router_get_feasible_routes(bgp_router_t * pRouter, SPrefix sPrefix, const uint8_t uOnlyEBGP)
#else
  bgp_routes_t * bgp_router_get_feasible_routes(bgp_router_t * pRouter, SPrefix sPrefix)
#endif
{
  bgp_routes_t * pRoutes;
  bgp_peer_t * pPeer;
  bgp_route_t * pRoute;
  unsigned int index;
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  bgp_routes_t * pRoutesSeek;
  uint16_t uIndex;
#endif

  pRoutes= routes_list_create(ROUTES_LIST_OPTION_REF);

  // Get from the Adj-RIB-ins the list of available routes.
  for (index= 0; index < bgp_peers_size(pRouter->pPeers); index++) {
    pPeer= bgp_peers_at(pRouter->pPeers, index);

    /* Check that the peering session is in ESTABLISHED state */
    if (pPeer->tSessionState == SESSION_STATE_ESTABLISHED) {

#ifdef __EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
      if (pPeer->uRemoteAS != pRouter->uASN || uOnlyEBGP == 0) {
#endif
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
      pRoutesSeek = rib_find_exact(pPeer->pAdjRIB[RIB_IN], sPrefix);
      if (pRoutesSeek != NULL) {
	for (uIndex = 0; uIndex < bgp_routes_size(pRoutesSeek); uIndex++) {
	  pRoute = bgp_routes_at(pRoutesSeek, uIndex);
#else
      pRoute= rib_find_exact(pPeer->pAdjRIB[RIB_IN], sPrefix);
#endif

      /* Check that a route is present in the Adj-RIB-in of this peer
	 and that it is eligible (according to the in-filters) */
      if ((pRoute != NULL) &&
	  (route_flag_get(pRoute, ROUTE_FLAG_ELIGIBLE))) {
	
	
	/* Check that the route is feasible (next-hop reachable, and
	   so on). Note: this call will actually update the 'feasible'
	   flag of the route. */
	if (bgp_router_feasible_route(pRouter, pRoute)) {
	  routes_list_append(pRoutes, pRoute);
	} 
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
// ----- bgp_router_check_dissemination_external_best ----------------
/**
 *
 */
void bgp_router_check_dissemination_external_best(bgp_router_t * pRouter, 
			  bgp_routes_t * pEBGPRoutes, bgp_route_t * pOldEBGPRoute, 
			  bgp_route_t * pEBGPRoute, SPrefix sPrefix)
{
  pEBGPRoute = (bgp_route_t *) bgp_routes_at(pEBGPRoutes, 0);
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

    bgp_router_decision_process_disseminate_external_best(pRouter, 
						  sPrefix, pEBGPRoute);
  } /*else {
    route_destroy(&pEBGPRoute);
  }*/

}

// ----- bgp_router_get_external_best --------------------------------
/**
 *
 */
bgp_route_t * bgp_router_get_external_best(bgp_routes_t * pRoutes)
{
  int iIndex;

  for (iIndex = 0; iIndex < bgp_routes_size(pRoutes); iIndex++) {
    if (route_flag_get(bgp_routes_at(pRoutes, iIndex), ROUTE_FLAG_EXTERNAL_BEST))
      return bgp_routes_at(pRoutes, iIndex);
  }
  return NULL;
}

// ----- bgp_router_get_old_ebgp_route -------------------------------
/**
 *
 */
bgp_route_t * bgp_router_get_old_ebgp_route(bgp_routes_t * pRoutes, 
					  bgp_route_t * pOldRoute)
{
  bgp_route_t * pOldEBGPRoute= NULL;
  //Is pOldRoute route also the best EBGP route?
  if (BGP_OPTIONS_ADVERTISE_EXTERNAL_BEST) {
    //If the old best route also was the best external route, 
    //it has not been announced as a best external route but 
    //as the main best route!
    if ((pOldRoute != NULL) && 
	route_flag_get(pOldRoute, ROUTE_FLAG_EXTERNAL_BEST))
      pOldEBGPRoute = NULL;
    else
      pOldEBGPRoute = bgp_router_get_external_best(pRoutes);
    LOG_DEBUG("\told-external-best: ");
    LOG_ENABLED_DEBUG() route_dump(log_get_stream(pMainLog), 
						      pOldEBGPRoute);
    LOG_DEBUG("\n");
  }
  return pOldEBGPRoute;
}
#endif

// ---- bgp_router_decision_process_no_best_route --------------------
/**
 *
 */
#ifdef __EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
void bgp_router_decision_process_no_best_route(bgp_router_t * pRouter, 
				SPrefix sPrefix, bgp_route_t * pOldRoute, 
				bgp_route_t * pOldEBGPRoute)
#else
void bgp_router_decision_process_no_best_route(bgp_router_t * pRouter, 
				SPrefix sPrefix, bgp_route_t * pOldRoute)
#endif
{
  LOG_DEBUG(LOG_LEVEL_DEBUG, "\t*** NO BEST ROUTE ***\n");

  // If a route towards this prefix was previously installed, then
  // withdraw it. Otherwise, do nothing...
  if (pOldRoute != NULL) {
#ifdef __EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
    if (BGP_OPTIONS_ADVERTISE_EXTERNAL_BEST && pOldEBGPRoute != NULL) {
      route_flag_set(pOldEBGPRoute, ROUTE_FLAG_EXTERNAL_BEST, 0);
    }
#endif

/*
  log_printf(pLogErr, "OLD-ROUTE[%p]: ", pOldRoute);
  route_dump(pLogErr, pOldRoute);
  log_printf(pLogErr, "; PEER[%p]: ", pOldRoute->pPeer);
  bgp_peer_dump(pLogErr, pOldRoute->pPeer);
  log_printf(pLogErr, "; ADJ-RIB-IN[%p]", pOldRoute->pPeer->pAdjRIBIn);
  log_printf(pLogErr, "\n");
*/

    // THIS FUNCTION CALL MUST APPEAR BEFORE THE ROUTE IS REMOVED FROM
    // THE LOC-RIB (AND DESTROYED) !!!
    bgp_router_best_flag_off(pOldRoute);

#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
    rib_remove_route(pRouter->pLocRIB, sPrefix, NULL);
#else
    rib_remove_route(pRouter->pLocRIB, sPrefix);
#endif

/*
  log_printf(pLogErr, "OLD-ROUTE[%p]: ", pOldRoute);
  route_dump(pLogErr, pOldRoute);
  log_printf(pLogErr, "; PEER[%p]: ", pOldRoute->pPeer);
  bgp_peer_dump(pLogErr, pOldRoute->pPeer);
  log_printf(pLogErr, "; ADJ-RIB-IN[%p]", pOldRoute->pPeer->pAdjRIBIn);
  log_printf(pLogErr, "\n");
*/

    bgp_router_rt_del_route(pRouter, sPrefix);
    bgp_router_decision_process_disseminate(pRouter, sPrefix, NULL);
  }
}

// ----- bgp_router_decision_process_unchanged_best_route ------------
#ifdef __EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
void bgp_router_decision_process_unchanged_best_route(bgp_router_t * pRouter, 
					SPrefix sPrefix, bgp_routes_t * pRoutes, 
					bgp_route_t * pOldRoute, bgp_route_t * pRoute, 
					int iRank, bgp_routes_t * pEBGPRoutes,
					bgp_route_t * pOldEBGPRoute)
#else
void bgp_router_decision_process_unchanged_best_route(bgp_router_t * pRouter, 
					SPrefix sPrefix, bgp_routes_t * pRoutes, 
					bgp_route_t * pOldRoute, bgp_route_t * pRoute, 
					int iRank)
#endif
{

#ifdef __EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
  bgp_route_t * pEBGPRoute;
#endif
  /**************************************************************
   * In this case, both routes (old and new) are equal from the
   * BGP point of view. There is thus no need to send BGP
   * messages.
   *
   * However, it is possible that the IGP next-hop of the route
   * has changed. If so, the route must be updated in the routing
   * table!
   **************************************************************/

  LOG_DEBUG(LOG_LEVEL_DEBUG, "\t*** BEST ROUTE UNCHANGED ***\n");

  /* Mark route in Adj-RIB-In as best (since it has probably been
     replaced). */
  route_flag_set(pRoute, ROUTE_FLAG_BEST, 1);
  route_flag_set(bgp_routes_at(pRoutes, 0), ROUTE_FLAG_BEST, 1);

  /* Update ROUTE_FLAG_DP_IGP of old route */
  route_flag_set(pOldRoute, ROUTE_FLAG_DP_IGP,
		 route_flag_get(pRoute, ROUTE_FLAG_DP_IGP));

#ifdef __BGP_ROUTE_INFO_DP__
  pOldRoute->tRank= (uint8_t) iRank;
  bgp_routes_at(pRoutes, 0)->tRank= (uint8_t) iRank;
#endif

  /* If the IGP next-hop has changed, we need to re-insert the
     route into the routing table with the new next-hop. */
  bgp_router_rt_add_route(pRouter, pRoute);

  /* Route has not changed. */
  route_destroy(&pRoute);
  pRoute= pOldRoute;

#ifdef __EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
  if (pEBGPRoutes != NULL && bgp_routes_size(pEBGPRoutes) > 0) {
    pEBGPRoute = bgp_routes_at(pEBGPRoutes, 0);
    bgp_router_check_dissemination_external_best(pRouter, pEBGPRoutes, 
				    pOldEBGPRoute, pEBGPRoute, sPrefix);
  }
#endif
}

// ----- bgp_router_decision_process_update_best_route ---------------
/**
 *
 */
#ifdef __EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
void bgp_router_decision_process_update_best_route(bgp_router_t * pRouter, 
				      SPrefix sPrefix, bgp_routes_t * pRoutes, 
				      bgp_route_t * pOldRoute, bgp_route_t * pRoute, 
				      int iRank, bgp_routes_t * pEBGPRoutes,
				      bgp_route_t * pOldEBGPRoute)
#else
void bgp_router_decision_process_update_best_route(bgp_router_t * pRouter, 
				      SPrefix sPrefix, bgp_routes_t * pRoutes, 
				      bgp_route_t * pOldRoute, bgp_route_t * pRoute, 
				      int iRank)
#endif
{

#ifdef __EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
  bgp_route_t * pEBGPRoute;
#endif

  /**************************************************************
   * In this case, the route is new or one of its BGP attributes
   * has changed. Thus, we must update the route into the routing
   * table and we must propagate the BGP route to the peers.
   **************************************************************/

  if (pOldRoute != NULL) {
    LOG_DEBUG(LOG_LEVEL_DEBUG, "\t*** UPDATED BEST ROUTE ***\n");
  } else {
    LOG_DEBUG(LOG_LEVEL_DEBUG, "\t*** NEW BEST ROUTE ***\n");
  }

#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  if (pOldRoute != NULL) {
    //As the RIBs accept more than one route we have to delete manually 
    //the old routes in the LOC-RIB!
//	if (rib_find_one_exact(pRouter->pLocRIB, pOldRoute->sPrefix, &(pOldRoute->tNextHop)) != NULL) 
    rib_remove_route(pRouter->pLocRIB, pOldRoute->sPrefix, NULL);
  }
#endif
  
 if (pOldRoute != NULL) 
    bgp_router_best_flag_off(pOldRoute);

  /* Mark route in Loc-RIB and Adj-RIB-In as best. This must be
     done after the call to 'bgp_router_best_flag_off'. */
  route_flag_set(pRoute, ROUTE_FLAG_BEST, 1);
  route_flag_set(bgp_routes_at(pRoutes, 0), ROUTE_FLAG_BEST, 1);

#ifdef __BGP_ROUTE_INFO_DP__
  pRoute->tRank= (uint8_t) iRank;
  bgp_routes_at(pRoutes, 0)->tRank= (uint8_t) iRank;
#endif

  /* Insert in Loc-RIB */
  assert(rib_add_route(pRouter->pLocRIB, pRoute) == 0);

  /* Insert in the node's routing table */
  bgp_router_rt_add_route(pRouter, pRoute);

#ifndef __EXPERIMENTAL_WALTON__
  //It is obsolete when using walton
  //We disseminate the information in another function 
  //see bgp_router_walton_dp_disseminate in walton.c
  bgp_router_decision_process_disseminate(pRouter, sPrefix, pRoute);
#endif

#ifdef __EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
  if (pEBGPRoutes != NULL && bgp_routes_size(pEBGPRoutes) > 0) {
    pEBGPRoute = bgp_routes_at(pEBGPRoutes, 0);
    bgp_router_check_dissemination_external_best(pRouter, pEBGPRoutes, 
				      pOldEBGPRoute, pEBGPRoute, sPrefix);
  }
#endif
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
int bgp_router_decision_process(bgp_router_t * pRouter,
				bgp_peer_t * pOriginPeer,
	 			SPrefix sPrefix)
{
  bgp_routes_t * pRoutes;
  int iIndex;
  bgp_route_t * pRoute, * pOldRoute;
  int iRank;

#ifdef __EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
  bgp_routes_t * pEBGPRoutes= NULL;
  bgp_route_t * pOldEBGPRoute= NULL;
  int iRankEBGP, iEBGPRoutesCount;
#endif

#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  pOldRoute= rib_find_one_exact(pRouter->pLocRIB, sPrefix, NULL);
#else
  pOldRoute= rib_find_exact(pRouter->pLocRIB, sPrefix);
#endif

  LOG_DEBUG_ENABLED(LOG_LEVEL_DEBUG) {
    log_printf(pLogDebug,
	       "----------------------------------------"
	       "---------------------------------------\n");
    log_printf(pLogDebug, "DECISION PROCESS for ");
    ip_prefix_dump(pLogDebug, sPrefix);
    log_printf(pLogDebug, " in ");
    bgp_router_dump_id(pLogDebug, pRouter);
    log_printf(pLogDebug, "\n");
    log_printf(pLogDebug, "\told-best: ");
    route_dump(pLogDebug, pOldRoute);
    log_printf(pLogDebug, "\n");
  }

  // Local routes can not be overriden and must be kept in Loc-RIB.
  // Decision process stops here in this case.
  if ((pOldRoute != NULL) &&
      route_flag_get(pOldRoute, ROUTE_FLAG_INTERNAL)) {
    return 0;
  }

  /* Build list of eligible routes */
#ifdef __EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
  pRoutes= bgp_router_get_feasible_routes(pRouter, sPrefix, 0);

  pOldEBGPRoute = bgp_router_get_old_ebgp_route(pRoutes, pOldRoute);
#else
  pRoutes= bgp_router_get_feasible_routes(pRouter, sPrefix);
#endif

  /* Reset DP_IGP flag, log eligibles */
  for (iIndex= 0; iIndex < bgp_routes_size(pRoutes); iIndex++) {
    pRoute= (bgp_route_t *) bgp_routes_at(pRoutes, iIndex);

    /* Clear flag that indicates that the route depends on the
       IGP. See 'dp_rule_nearest_next_hop' and 'bgp_router_scan_rib'
       for more information. */
    route_flag_set(pRoute, ROUTE_FLAG_DP_IGP, 0);


    /* Log eligible route */
    LOG_DEBUG_ENABLED(LOG_LEVEL_DEBUG) {
      log_printf(pLogDebug, "\teligible: ");
      route_dump(pLogDebug, pRoute);
      log_printf(pLogDebug, "\n");
    }

    route_flag_set(pRoute, ROUTE_FLAG_BEST, 0);
  }

  // If there is a single eligible & feasible route, it depends on the
  // IGP (see 'dp_rule_nearest_next_hop' and 'bgp_router_scan_rib')
  // for more information.
  if (bgp_routes_size(pRoutes) == 1)
    route_flag_set(bgp_routes_at(pRoutes, 0), ROUTE_FLAG_DP_IGP, 1);

#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  bgp_router_walton_unsynchronized_all(pRouter);
  // Compare eligible routes
  iRank= 0;
  if (bgp_routes_size(pRoutes) > 1) {
    iRank= bgp_router_walton_decision_process_run(pRouter, pRoutes);
    assert((bgp_routes_size(pRoutes) == 0) ||
	   (bgp_routes_size(pRoutes) == 1));
  } else {
    //TODO : do a loop on each iNextHopCount ...
    if (bgp_routes_size(pRoutes) != 0) {
      //      LOG_DEBUG(LOG_LEVEL_DEBUG, "only one route known ... disseminating to all : %d\n", bgp_routes_size(pRoutes));
      bgp_router_walton_disseminate_select_peers(pRouter, pRoutes, 1);
    }
  }
#else
  // Compare eligible routes
  iRank= 0;
  if (bgp_routes_size(pRoutes) > 1) 
    iRank= _bgp_router_decision_process_run(pRouter, pRoutes);
  assert((bgp_routes_size(pRoutes) == 0) ||
	 (bgp_routes_size(pRoutes) == 1));
#endif

  // If one best-route has been selected
  if (bgp_routes_size(pRoutes) > 0) {
    pRoute= route_copy(bgp_routes_at(pRoutes, 0));

#ifdef __EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
    if (BGP_OPTIONS_ADVERTISE_EXTERNAL_BEST) {
      //If the Best route selected is not an EBGP one, run the decision process
      //against the set of EBGP routes.
      if (pRoute->pPeer->uRemoteAS == pRouter->uASN)
	pEBGPRoutes = bgp_router_get_feasible_routes(pRouter, sPrefix, 1);
      else
	route_flag_set(pRoute, ROUTE_FLAG_EXTERNAL_BEST, 1);
    
      if (pEBGPRoutes != NULL) {
	for (iEBGPRoutesCount = 0; iEBGPRoutesCount < bgp_routes_size(pEBGPRoutes); 
								      iEBGPRoutesCount++){
	  LOG_DEBUG("\teligible external : ");
	  LOG_ENABLED_DEBUG() route_dump(log_get_stream(pMainLog), 
	      pEBGPRoutes->data[iEBGPRoutesCount]);
	  LOG_DEBUG("\n");
      }
	if (bgp_routes_size(pEBGPRoutes) > 1) {
	  iRankEBGP = bgp_router_walton_decision_process_run(pRouter, pEBGPRoutes);
	}
	assert((ptr_array_length(pEBGPRoutes) == 0) ||
	   (ptr_array_length(pEBGPRoutes) == 1));
      }
    }
#endif //__EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__

    LOG_DEBUG_ENABLED(LOG_LEVEL_DEBUG) {
      log_printf(pLogDebug, "\tnew-best: ");
      route_dump(pLogDebug, pRoute);
      log_printf(pLogDebug, "\n");
    }

#ifdef __EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
    // New/updated route: install in Loc-RIB & advertise to peers
    if ((pOldRoute == NULL) ||
	!route_equals(pOldRoute, pRoute)) {
      bgp_router_decision_process_update_best_route(pRouter, sPrefix, 
			  pRoutes, pOldRoute, pRoute, iRank, pEBGPRoutes,
			  pOldEBGPRoute);
    } else {
      bgp_router_decision_process_unchanged_best_route(pRouter, sPrefix, 
			  pRoutes, pOldRoute, pRoute, iRank, pEBGPRoutes,
			  pOldEBGPRoute);
    }
#else
    // New/updated route: install in Loc-RIB & advertise to peers
    if ((pOldRoute == NULL) ||
	!route_equals(pOldRoute, pRoute)) {
      bgp_router_decision_process_update_best_route(pRouter, sPrefix, 
				      pRoutes, pOldRoute, pRoute, iRank);
    } else {
      bgp_router_decision_process_unchanged_best_route(pRouter, sPrefix, 
				      pRoutes, pOldRoute, pRoute, iRank);
    }
#endif //__EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__

  } else {

#ifdef __EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
    bgp_router_decision_process_no_best_route(pRouter, sPrefix, pOldRoute, 
							    pOldEBGPRoute);
#else
    bgp_router_decision_process_no_best_route(pRouter, sPrefix, pOldRoute);
#endif //__EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__

  }

  routes_list_destroy(&pRoutes);

#ifdef __EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
  routes_list_destroy(&pEBGPRoutes);
#endif

  LOG_DEBUG(LOG_LEVEL_DEBUG,
	    "----------------------------------------"
	    "---------------------------------------\n");
  return 0;
}

// ----- bgp_router_handle_message ----------------------------------
/**
 * Handle a BGP message received from the lower layer (network layer
 * at this time). The function looks for the destination peer and
 * delivers it for processing...
 */
int bgp_router_handle_message(simulator_t * sim,
			      void * handler,
			      net_msg_t * msg)
{
  bgp_router_t * router= (bgp_router_t *) handler;
  bgp_msg_t * bgp_msg= (bgp_msg_t *) msg->payload;
  bgp_peer_t * peer;

  if ((peer= bgp_router_find_peer(router, msg->src_addr)) != NULL) {
    bgp_peer_handle_message(peer, bgp_msg);
    _bgp_router_msg_listener(msg);
  } else {
    LOG_ERR_ENABLED(LOG_LEVEL_WARNING) {
      log_printf(pLogErr, "WARNING: BGP message received from unknown peer !\n");
      log_printf(pLogErr, "WARNING:   destination=");
      bgp_router_dump_id(pLogErr, router);
      log_printf(pLogErr, "\nWARNING:   source=");
      ip_address_dump(pLogErr, msg->src_addr);
      log_printf(pLogErr, "\n");
    }
    bgp_msg_destroy(&bgp_msg);
    return -1;
  }
  return 0;
}

// ----- bgp_router_dump_id -------------------------------------------------
/**
 *
 */
void bgp_router_dump_id(SLogStream * pStream, bgp_router_t * pRouter)
{
  log_printf(pStream, "AS%d:", pRouter->uASN);
  ip_address_dump(pStream, pRouter->pNode->tAddr);
}

typedef struct {
  bgp_router_t * pRouter;
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
  bgp_router_t * pRouter= pCtx->pRouter;
  SPrefix sPrefix;
  bgp_route_t * pRoute;
  unsigned int index;
  bgp_peer_t * pPeer;
  unsigned int uBestWeight;
  SNetRouteInfo * pRouteInfo;
  SNetRouteInfo * pCurRouteInfo;

#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  uint16_t uIndexRoute;
  bgp_routes_t * pRoutes;
  bgp_router_walton_unsynchronized_all(pRouter);
#endif
  bgp_route_t * pAdjRoute;

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
  pRoute= rib_find_one_exact(pRouter->pLocRIB, sPrefix, NULL);
#else
  pRoute= rib_find_exact(pRouter->pLocRIB, sPrefix);
#endif
  if (pRoute == NULL) {
    _array_append(pCtx->pPrefixes, &sPrefix);
    return 0;
  } else {

    /* Check if the peer that has announced this route is down */
    if ((pRoute->pPeer != NULL) &&
	(pRoute->pPeer->tSessionState != SESSION_STATE_ESTABLISHED)) {
      LOG_ERR_ENABLED(LOG_LEVEL_WARNING) {
	log_printf(pLogErr, "*** SESSION NOT ESTABLISHED [");
	ip_prefix_dump(pLogErr, pRoute->sPrefix);
	log_printf(pLogErr, "] ***\n");
      }
      _array_append(pCtx->pPrefixes, &sPrefix);
      return 0;
    }
    
    pRouteInfo= rt_find_best(pRouter->pNode->rt, pRoute->pAttr->tNextHop,
			     NET_ROUTE_ANY);    
    
    /* Check if the IP next-hop of the BGP route has changed. Indeed,
       the BGP next-hop is not always the IP next-hop. If this next-hop
       has changed, the decision process must be run. */
    if (pRouteInfo != NULL) { 
      pCurRouteInfo= rt_find_exact(pRouter->pNode->rt,
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
      for (index= 0; index < bgp_peers_size(pRouter->pPeers); index++) {
	pPeer= bgp_peers_at(pRouter->pPeers, index);
	
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
	pRoutes= rib_find_exact(pPeer->pAdjRIB[RIB_IN], pRoute->sPrefix);
	if (pRoutes != NULL) {
	  for (uIndexRoute = 0; uIndexRoute < bgp_routes_size(pRoutes); uIndexRoute++) {
	    pAdjRoute = bgp_routes_at(pRoutes, uIndexRoute);
#else
	pAdjRoute= rib_find_exact(pPeer->pAdjRIB[RIB_IN], pRoute->sPrefix);
#endif
		
	if (pAdjRoute != NULL) {
	  
	  /* Is there a route (IGP ?) towards the destination ? */
	  pRouteInfo= rt_find_best(pRouter->pNode->rt,
				   pAdjRoute->pAttr->tNextHop,
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
	}
#endif
	
      }
      
    }
  }
    
  return 0;
}

// -----[ bgp_router_clear_adjrib ]----------------------------------
/**
 *
 */
int bgp_router_clear_adjrib(bgp_router_t * pRouter)
{
  unsigned int index;
  bgp_peer_t * pPeer;

  for (index= 0; index < bgp_peers_size(pRouter->pPeers); index++) {
    pPeer= bgp_peers_at(pRouter->pPeers, index);
    rib_destroy(&pPeer->pAdjRIB[RIB_IN]);
    rib_destroy(&pPeer->pAdjRIB[RIB_OUT]);
  }
  return 0;
}

// -----[ _bgp_router_prefixes_for_each ]----------------------------
/**
 *
 */
static int _bgp_router_prefixes_for_each(uint32_t uKey, uint8_t uKeyLen,
					 void * pItem, void * pContext)
{
  SRadixTree * pPrefixes= (SRadixTree *) pContext;
  bgp_route_t * pRoute= (bgp_route_t *) pItem;

  radix_tree_add(pPrefixes, pRoute->sPrefix.tNetwork,
		 pRoute->sPrefix.uMaskLen, (void *) 1);
  return 0;
}

// -----[ _bgp_router_alloc_prefixes ]-------------------------------
static inline void _bgp_router_alloc_prefixes(SRadixTree ** ppPrefixes)
{
  /* If list (radix-tree) is not allocated, create it now. The
     radix-tree is created without destroy function and acts thus as a
     list of references. */
  if (*ppPrefixes == NULL) {
    *ppPrefixes= radix_tree_create(32, NULL);
  }
}

// -----[ _bgp_router_free_prefixes ]--------------------------------
static inline void _bgp_router_free_prefixes(SRadixTree ** ppPrefixes)
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
static inline int _bgp_router_get_peer_prefixes(bgp_router_t * pRouter,
						bgp_peer_t * pPeer,
						SRadixTree ** ppPrefixes)
{
  unsigned int index;
  int iResult= 0;

  _bgp_router_alloc_prefixes(ppPrefixes);

  if (pPeer != NULL) {
    iResult= rib_for_each(pPeer->pAdjRIB[RIB_IN],
			  _bgp_router_prefixes_for_each,
			  *ppPrefixes);
  } else {
    for (index= 0; index < bgp_peers_size(pRouter->pPeers); index++) {
      pPeer= bgp_peers_at(pRouter->pPeers, index);
      iResult= rib_for_each(pPeer->pAdjRIB[RIB_IN],
			    _bgp_router_prefixes_for_each,
			    *ppPrefixes);
      if (iResult != 0)
	break;
    }
  }
  return iResult;
}

// -----[ _bgp_router_get_local_prefixes ]---------------------------
static inline int _bgp_router_get_local_prefixes(bgp_router_t * pRouter,
						 SRadixTree ** ppPrefixes)
{
  _bgp_router_alloc_prefixes(ppPrefixes);
  
  return rib_for_each(pRouter->pLocRIB, _bgp_router_prefixes_for_each,
		      *ppPrefixes);
}

// -----[ _bgp_router_prefixes ]-------------------------------------
/**
 * This function builds a list of all the prefixes known in this
 * router (in Adj-RIB-Ins and Loc-RIB).
 */
static inline SRadixTree * _bgp_router_prefixes(bgp_router_t * pRouter)
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
 * This function scans the peering sessions. For each session, it
 * checks that the peer router is still reachable. If it is not, the
 * session is teared down (see 'bgp_peer_session_refresh' for more
 * details).
 */
static inline void _bgp_router_refresh_sessions(bgp_router_t * pRouter)
{
  unsigned int index;

  for (index= 0; index < bgp_peers_size(pRouter->pPeers); index++)
    bgp_peer_session_refresh(bgp_peers_at(pRouter->pPeers, index));
}

// ----- bgp_router_scan_rib ----------------------------------------
/**
 * This function scans the RIB of the BGP router in order to find
 * routes for which the distance to the next-hop has changed. For each
 * route that has changed, the decision process is re-run.
 */
int bgp_router_scan_rib(bgp_router_t * pRouter)
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
  bgp_router_t * pRouter= (bgp_router_t *) pContext;
  SPrefix sPrefix;

  sPrefix.tNetwork= uKey;
  sPrefix.uMaskLen= uKeyLen;  

  LOG_DEBUG_ENABLED(LOG_LEVEL_DEBUG) {
    log_printf(pLogDebug, "decision-process [");
    ip_prefix_dump(pLogDebug, sPrefix);
    log_printf(pLogDebug, "]\n");
    log_flush(pLogDebug);
  }

  return bgp_router_decision_process(pRouter, NULL, sPrefix);
}

// -----[ bgp_router_rerun ]-----------------------------------------
/**
 * Rerun the decision process for the given prefixes. If the length of
 * the prefix mask is 0, rerun for all known prefixes (from Adj-RIB-In
 * and Loc-RIB). Otherwize, only rerun for the specified prefix.
 */
int bgp_router_rerun(bgp_router_t * pRouter, SPrefix sPrefix)
{
  int iResult;
  SRadixTree * pPrefixes= NULL;

  LOG_DEBUG_ENABLED(LOG_LEVEL_DEBUG) {
    log_printf(pLogDebug, "rerun [");
    ip_address_dump(pLogDebug, pRouter->pNode->tAddr);
    log_printf(pLogDebug, "]\n");
    log_flush(pLogDebug);
  }

  _bgp_router_alloc_prefixes(&pPrefixes);

  /* Populate with all prefixes */
  if (sPrefix.uMaskLen == 0) {
    assert(!_bgp_router_get_peer_prefixes(pRouter, NULL, &pPrefixes));
    assert(!_bgp_router_get_local_prefixes(pRouter, &pPrefixes));
  } else {
    radix_tree_add(pPrefixes, sPrefix.tNetwork,
		   sPrefix.uMaskLen, (void *) 1);
  }

  LOG_DEBUG_ENABLED(LOG_LEVEL_DEBUG) {
    log_printf(pLogDebug, "prefix-list ok\n");
    log_flush(pLogDebug);
  }

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
int bgp_router_peer_readv_prefix(bgp_router_t * pRouter, bgp_peer_t * pPeer,
				 SPrefix sPrefix)
{
  bgp_route_t * pRoute;

  if (sPrefix.uMaskLen == 0) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: not yet implemented\n");
    return -1;
  } else {
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
    pRoute= rib_find_one_exact(pRouter->pLocRIB, sPrefix, NULL);
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
void bgp_router_dump_networks(SLogStream * pStream, bgp_router_t * pRouter)
{
  unsigned int uIndex;

  for (uIndex= 0; uIndex < bgp_routes_size(pRouter->pLocalNetworks);
       uIndex++) {
    route_dump(pStream, bgp_routes_at(pRouter->pLocalNetworks, uIndex));
    log_printf(pStream, "\n");
  }
  log_flush(pStream);
}

// ----- bgp_router_networks_for_each -------------------------------
int bgp_router_networks_for_each(bgp_router_t * pRouter, FArrayForEach fForEach,
				 void * pContext)
{
  return _array_for_each((SArray *) pRouter->pLocalNetworks, fForEach, pContext);
}

// ----- bgp_router_dump_peers --------------------------------------
/**
 *
 */
void bgp_router_dump_peers(SLogStream * pStream, bgp_router_t * pRouter)
{
  unsigned int index;

  for (index= 0; index < bgp_peers_size(pRouter->pPeers); index++) {
    bgp_peer_dump(pStream, bgp_peers_at(pRouter->pPeers, index));
    log_printf(pStream, "\n");
  }
  log_flush(pStream);
}

// -----[ bgp_router_peers_for_each ]--------------------------------
/**
 *
 */
int bgp_router_peers_for_each(bgp_router_t * pRouter,
			      FArrayForEach fForEach,
			      void * pContext)
{
  return bgp_peers_for_each(pRouter->pPeers, fForEach, pContext);
}

typedef struct {
  bgp_router_t * pRouter;
  SLogStream * pStream;
  char * cDump;
} bgp_route_tDumpCtx;

// ----- _bgp_router_dump_route -------------------------------------
/**
 *
 */
static int _bgp_router_dump_route(uint32_t uKey, uint8_t uKeyLen,
				  void * pItem, void * pContext)
{
  bgp_route_tDumpCtx * pCtx= (bgp_route_tDumpCtx *) pContext;

  route_dump(pCtx->pStream, (bgp_route_t *) pItem);
  log_printf(pCtx->pStream, "\n");

  log_flush(pCtx->pStream);

  return 0;
}

// ----- bgp_router_dump_rib ----------------------------------------
/**
 *
 */
void bgp_router_dump_rib(SLogStream * pStream, bgp_router_t * pRouter)
{
  bgp_route_tDumpCtx sCtx;
  sCtx.pRouter= pRouter;
  sCtx.pStream= pStream;
  rib_for_each(pRouter->pLocRIB, _bgp_router_dump_route, &sCtx);

  log_flush(pStream);
}

// ----- bgp_router_dump_rib_address --------------------------------
/**
 *
 */
void bgp_router_dump_rib_address(SLogStream * pStream, bgp_router_t * pRouter,
				 net_addr_t tAddr)
{
  bgp_route_t * pRoute;
  SPrefix sPrefix;

  sPrefix.tNetwork= tAddr;
  sPrefix.uMaskLen= 32;

#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  pRoute = rib_find_one_best(pRouter->pLocRIB, sPrefix);
#else
  pRoute= rib_find_best(pRouter->pLocRIB, sPrefix);
#endif
  if (pRoute != NULL) {
    route_dump(pStream, pRoute);
    log_printf(pStream, "\n");
  }

  log_flush(pStream);
}

// ----- bgp_router_dump_rib_prefix ---------------------------------
/**
 *
 */
void bgp_router_dump_rib_prefix(SLogStream * pStream, bgp_router_t * pRouter,
				SPrefix sPrefix)
{
  bgp_route_t * pRoute;

#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  pRoute= rib_find_one_exact(pRouter->pLocRIB, sPrefix, NULL);
#else
  pRoute= rib_find_exact(pRouter->pLocRIB, sPrefix);
#endif
  if (pRoute != NULL) {
    route_dump(pStream, pRoute);
    log_printf(pStream, "\n");
  }

  log_flush(pStream);
}

// ----- bgp_router_dump_adjrib -------------------------------------
/**
 *
 */
void bgp_router_dump_adjrib(SLogStream * pStream, bgp_router_t * pRouter,
			    bgp_peer_t * pPeer, SPrefix sPrefix,
			    bgp_rib_dir_t dir)
{
  unsigned int index;

  if (pPeer == NULL) {
    for (index= 0; index < bgp_peers_size(pRouter->pPeers); index++) {
      bgp_peer_dump_adjrib(pStream, bgp_peers_at(pRouter->pPeers, index),
			   sPrefix, dir);
    }
  } else {
    bgp_peer_dump_adjrib(pStream, pPeer, sPrefix, dir);
  }
  log_flush(pStream);
}

// ----- bgp_router_info --------------------------------------------
void bgp_router_info(SLogStream * pStream, bgp_router_t * pRouter)
{
  log_printf(pStream, "router-id : ");
  ip_address_dump(pStream, pRouter->tRouterID);
  log_printf(pStream, "\n");
  log_printf(pStream, "as-number : %u\n", pRouter->uASN);
  log_printf(pStream, "cluster-id: ");
  ip_address_dump(pStream, pRouter->tClusterID);
  log_printf(pStream, "\n");
  if (pRouter->pNode->name != NULL)
    log_printf(pStream, "name      : %s\n", pRouter->pNode->name);
  log_flush(pStream);
}

// -----[ bgp_router_show_route_info ]-------------------------------
/**
 * Show information about a best BGP route from the Loc-RIB of this
 * router:
 * - decision-process-rule: rule of the decision process that was
 *   used to select this route as best.
 */
int bgp_router_show_route_info(SLogStream * pStream, bgp_router_t * pRouter,
			       bgp_route_t * pRoute)
{
  bgp_routes_t * pRoutes;

#ifdef __BGP_ROUTE_INFO_DP__
  if (pRoute->tRank <= 0) {
    log_printf(pStream, "decision-process-rule: %d [ Single choice ]\n",
	       pRoute->tRank);
  } else if (pRoute->tRank >= DP_NUM_RULES) {
    log_printf(pStream, "decision-process-rule: %d [ Invalid rule ]\n",
	       pRoute->tRank);
  } else {
    log_printf(pStream, "decision-process-rule: %d [ %s ]\n",
	       pRoute->tRank, DP_RULES[pRoute->tRank-1].pcName);
  }
#endif
  pRoutes= bgp_router_get_feasible_routes(pRouter, pRoute->sPrefix);
  if (pRoutes != NULL) {
    log_printf(pStream, "decision-process-feasibles: %d\n",
	       bgp_routes_size(pRoutes));
    routes_list_destroy(&pRoutes);
  }
  log_flush(pStream);
  return 0;
}

// -----[ _bgp_router_show_route_info ]------------------------------
/**
 *
 */
static int _bgp_router_show_route_info(uint32_t uKey, uint8_t uKeyLen,
				       void * pItem, void * pContext)
{
  bgp_route_tDumpCtx * pCtx= (bgp_route_tDumpCtx *) pContext;
  bgp_route_t * pRoute= (bgp_route_t *) pItem;

  log_printf(pCtx->pStream, "prefix: ");
  ip_prefix_dump(pCtx->pStream, pRoute->sPrefix);
  log_printf(pCtx->pStream, "\n");
  bgp_router_show_route_info(pCtx->pStream, pCtx->pRouter, pRoute);

  log_flush(pCtx->pStream);

  return 0;
}

// -----[ bgp_router_show_routes_info ]------------------------------
/**
 *
 */
int bgp_router_show_routes_info(SLogStream * pStream, bgp_router_t * pRouter,
				SNetDest sDest)
{
  bgp_route_tDumpCtx sCtx;
  bgp_route_t * pRoute;

  switch (sDest.tType) {
  case NET_DEST_ANY:
    sCtx.pRouter= pRouter;
    sCtx.pStream= pStream;
    return rib_for_each(pRouter->pLocRIB, _bgp_router_show_route_info, &sCtx);
    break;
  case NET_DEST_ADDRESS:
//TODO : implements to support walton
#if ! (defined __EXPERIMENTAL_WALTON__)
    pRoute= rib_find_best(pRouter->pLocRIB, sDest.uDest.sPrefix);
    if (pRoute == NULL)
      return -1;
    bgp_router_show_route_info(pStream, pRouter, pRoute);
#endif
    return 0;
    break;
  case NET_DEST_PREFIX:
    
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
    pRoute= rib_find_one_exact(pRouter->pLocRIB, sDest.uDest.sPrefix, NULL);
#else
    pRoute= rib_find_exact(pRouter->pLocRIB, sDest.uDest.sPrefix);
#endif
    
    if (pRoute == NULL)
      return -1;
    bgp_router_show_route_info(pStream, pRouter, pRoute);
    return 0;
    break;
  default:
    return -1;
  }
  return -1;
}


/////////////////////////////////////////////////////////////////////
// LOAD/SAVE FUNCTIONS
/////////////////////////////////////////////////////////////////////

typedef struct {
  bgp_router_t * pTargetRouter;     // Router where routes are loaded
  uint8_t      uOptions;          // Options
  int          iReturnCode;       // Code to return in case of error
  unsigned int uRoutesOk;         // Number of routes loaded without error
  unsigned int uRoutesBadTarget;  //                  with bad target
  unsigned int uRoutesBadPeer;    //                  with bad peer
  unsigned int uRoutesIgnored;    //                  ignored by API (ex: IP6)
} SBGP_LOAD_RIB_CTX;
  
// -----[ _bgp_router_load_rib_handler ]-----------------------------
/**
 * Handle a BGP route destined to a target router.
 *
 * The function performs three actions:
 * 1). Check that the route was collected on the correct router.
 *     To do that, checks that the address of the target router
 *     equals the address of the route's peer. For CISCO dumps, this
 *     information is not available and the check always succeeds.
 * 2). Check that the target router has a peer that corresponds to
 *     the route's next-hop.
 * 3). Inject the route into the target's Adj-RIB-in and runs the
 *     decision process for the route's prefix.
 */
static int _bgp_router_load_rib_handler(int iStatus,
					bgp_route_t * pRoute,
					net_addr_t tPeerAddr,
					unsigned int uPeerAS,
					void * pContext)
{
  SBGP_LOAD_RIB_CTX * pCtx= (SBGP_LOAD_RIB_CTX *) pContext;
  bgp_router_t * pRouter= pCtx->pTargetRouter;
  bgp_peer_t * pPeer= NULL;

  // Check that there is a route to handle (according to API)
  if (iStatus != BGP_ROUTES_INPUT_STATUS_OK) {
    pCtx->uRoutesIgnored++;
    return BGP_ROUTES_INPUT_SUCCESS;
  }

  // 1). Check that the target router (addr/ASN) corresponds to the
  //     route's peer (addr/ASN). Ignored if peer addr is 0 (CISCO).
  if (!(pCtx->uOptions & BGP_ROUTER_LOAD_OPTIONS_FORCE) &&
      ((pRouter->pNode->tAddr != tPeerAddr) ||
       (pRouter->uASN != uPeerAS))) {
    
    LOG_ERR_ENABLED(LOG_LEVEL_SEVERE) {
      log_printf(pLogErr,
		 "Error: invalid peer (IP address/AS number mismatch)\n"
		 "Error: local router = AS%d:", pRouter->uASN);
      ip_address_dump(pLogErr, pRouter->pNode->tAddr);
      log_printf(pLogErr,"\nError: MRT peer router = AS%d:", uPeerAS);
	ip_address_dump(pLogErr, tPeerAddr);
      log_printf(pLogErr, "\n");
    }
    route_destroy(&pRoute);
    pCtx->uRoutesBadTarget++;
    return pCtx->iReturnCode;
  }
  
  // 2). Check that the target router has a peer that corresponds
  //     to the route's next-hop.
  if (pPeer == NULL) {
    
    // Look for the peer in the router...
    pPeer= bgp_router_find_peer(pRouter, pRoute->pAttr->tNextHop);
    // If the peer does not exist, auto-create it if required or
    // drop an error message.
    if (pPeer == NULL) {
      if ((pCtx->uOptions & BGP_ROUTER_LOAD_OPTIONS_AUTOCONF) != 0) {
	bgp_auto_config_session(pRouter, pRoute->pAttr->tNextHop,
				path_last_as(pRoute->pAttr->pASPathRef), &pPeer);
      } else {
	LOG_ERR_ENABLED(LOG_LEVEL_SEVERE) {
	  log_printf(pLogErr, "Error: peer not found \"");
	  ip_address_dump(pLogErr, pRoute->pAttr->tNextHop);
	  log_printf(pLogErr, "\"\n");
	}
	route_destroy(&pRoute);
	pCtx->uRoutesBadPeer++;
	return pCtx->iReturnCode;
      }
    }
  }

  // 3). Update the target router's Adj-RIB-in and
  //     run the decision process
  pRoute->pPeer= pPeer;
  route_flag_set(pRoute, ROUTE_FLAG_FEASIBLE, 1);
  if (pRoute->pPeer == NULL) {
    route_flag_set(pRoute, ROUTE_FLAG_INTERNAL, 1);
    rib_add_route(pRouter->pLocRIB, pRoute);
  } else {
    route_flag_set(pRoute, ROUTE_FLAG_ELIGIBLE,
		   bgp_peer_route_eligible(pRoute->pPeer, pRoute));
    route_flag_set(pRoute, ROUTE_FLAG_BEST,
		   bgp_peer_route_feasible(pRoute->pPeer, pRoute));
    rib_replace_route(pRoute->pPeer->pAdjRIB[RIB_IN], pRoute);
  }

  // TODO: shouldn't we take into account the soft-restart flag ?
  // If the (virtual) session is down, we should still store the
  // received routes in the Adj-RIB-in, but not run the decision
  // process.

  bgp_router_decision_process(pRouter, pRoute->pPeer, pRoute->sPrefix);

  pCtx->uRoutesOk++;
  return BGP_ROUTES_INPUT_SUCCESS;
}

// -----[ bgp_router_load_rib ]--------------------------------------
/**
 * This function loads an existing BGP routing table into the given
 * bgp router instance. The routes are considered local and will not
 * be replaced by routes received from peers. The routes are marked
 * as best and feasible and are directly installed into the Loc-RIB.
 */
int bgp_router_load_rib(bgp_router_t * pRouter, const char * pcFileName,
			uint8_t tFormat, uint8_t uOptions)
{
  int iResult;
  SBGP_LOAD_RIB_CTX sCtx= {
    .pTargetRouter   = pRouter,
    .uOptions        = uOptions,
    .iReturnCode     = 0, // Ignore errors
    .uRoutesOk       = 0,
    .uRoutesBadTarget= 0,
    .uRoutesBadPeer  = 0,
    .uRoutesIgnored  = 0,
  };

  // Load routes
  iResult= bgp_routes_load(pcFileName, tFormat,
			   _bgp_router_load_rib_handler, &sCtx);
  if (iResult != 0)
    return -1;

  // Show summary
  if (uOptions & BGP_ROUTER_LOAD_OPTIONS_SUMMARY) {
    log_printf(pLogOut, "Routes loaded         : %u\n", sCtx.uRoutesOk);
    log_printf(pLogOut, "Routes with bad target: %u\n", sCtx.uRoutesBadTarget);
    log_printf(pLogOut, "Routes with bad peer  : %u\n", sCtx.uRoutesBadPeer);
    log_printf(pLogOut, "Routes ignored        : %u\n", sCtx.uRoutesIgnored);
  }

  return 0;
}

// -----[ bgp_router_load_ribs_in ]----------------------------------
/**
 * Yet another unfinished function by S. Tandel :-(
 */
#ifdef __EXPERIMENTAL__
/* DE-ACTIVATED by BQU on 2007/10/04 */
#ifdef COMMENT_BQU
int bgp_router_load_ribs_in(bgp_router_t * pRouter, const char * pcFileName)
{
  bgp_route_t * pRoute;
  bgp_peer_t * pPeer;
  //net_node_t * pNode;
  FILE * phFiRib;
  char acFileLine[1024];
  
  if ( (phFiRib = fopen(pcFileName, "r")) == NULL) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: rib File doest not exist\n");
    return -1;
  }

  while (!feof(phFiRib)) {
    if (fgets(acFileLine, sizeof(acFileLine), phFiRib) == NULL)
      break;

    if ( (pRoute = mrtd_route_from_line(pRouter, acFileLine)) != NULL ) {
      if (pRoute == NULL) {
	LOG_ERR(LOG_LEVEL_SEVERE,
		"Error: could not build message from input\n%s\n", acFileLine);
	fclose(phFiRib);
	return -1;
      }

      if ( (pPeer = bgp_router_find_peer(pRouter, route_get_nexthop(pRoute))) != NULL) {
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
				  bgp_msg_update_create(pRouter->uASN,
							pRoute));
	}
      }
    }
  }
  fclose(phFiRib);
  return 0;
}
#endif /* COMMENT_BQU */
#endif /* __EXPERIMENTAL__ */

// -----[ _bgp_router_save_route_mrtd ]------------------------------
/**
 *
 */
static int _bgp_router_save_route_mrtd(uint32_t uKey, uint8_t uKeyLen,
				       void * pItem, void * pContext)
{
  bgp_route_tDumpCtx * pCtx= (bgp_route_tDumpCtx *) pContext;

  log_printf(pCtx->pStream, "TABLE_DUMP|%u|", 0);
  route_dump_mrt(pCtx->pStream, (bgp_route_t *) pItem);
  log_printf(pCtx->pStream, "\n");
  return 0;
}

// ----- bgp_router_save_rib ----------------------------------------
/**
 *
 */
int bgp_router_save_rib(bgp_router_t * pRouter, const char * pcFileName)
{
  SLogStream * pStream;
  bgp_route_tDumpCtx sCtx;

  pStream= log_create_file((char *) pcFileName);
  if (pStream == NULL)
    return -1;

  sCtx.pStream= pStream;
  sCtx.pRouter= pRouter;
  rib_for_each(pRouter->pLocRIB, _bgp_router_save_route_mrtd, &sCtx);
  log_destroy(&pStream);
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
void bgp_router_show_stats(SLogStream * pStream, bgp_router_t * pRouter)
{
  unsigned int index;
  bgp_peer_t * pPeer;
  enum_t * pEnum;
  bgp_route_t * pRoute;
  int iNumPrefixes, iNumBest;
  int aiNumPerRule[DP_NUM_RULES+1];
  int iRule;

  memset(&aiNumPerRule, 0, sizeof(aiNumPerRule));
  log_printf(pStream, "num-peers: %d\n",
	     bgp_peers_size(pRouter->pPeers));
  log_printf(pStream, "num-networks: %d\n",
	     bgp_routes_size(pRouter->pLocalNetworks));

  // Number of prefixes (best routes)
  iNumBest= 0;
  pEnum= trie_get_enum(pRouter->pLocRIB);
  while (enum_has_next(pEnum)) {
    pRoute= *(bgp_route_t **) enum_get_next(pEnum);
    if (route_flag_get(pRoute, ROUTE_FLAG_BEST)) {
      iNumBest++;
      if (pRoute->tRank <= DP_NUM_RULES)
	aiNumPerRule[pRoute->tRank]++;
    }
  }
  enum_destroy(&pEnum);
  log_printf(pStream, "num-best: %d\n", iNumBest);

  // Classification of best route selections
  log_printf(pStream, "rule-stats:");
  for (iRule= 0; iRule <= DP_NUM_RULES; iRule++) {
    log_printf(pStream, " %d", aiNumPerRule[iRule]);
  }
  log_printf(pStream, "\n");

  // Number of best/non-best routes per peer
  log_printf(pStream, "num-prefixes/peer:\n");
  for (index= 0; index < bgp_peers_size(pRouter->pPeers); index++) {
    pPeer= bgp_peers_at(pRouter->pPeers, index);
    iNumPrefixes= 0;
    iNumBest= 0;
    pEnum= trie_get_enum(pPeer->pAdjRIB[RIB_IN]);
    while (enum_has_next(pEnum)) {
      pRoute= *(bgp_route_t **) enum_get_next(pEnum);
      iNumPrefixes++;
      if (route_flag_get(pRoute, ROUTE_FLAG_BEST)) {
	iNumBest++;
	if (pRoute->tRank <= DP_NUM_RULES)
	  aiNumPerRule[pRoute->tRank]++;
      }
    }
    enum_destroy(&pEnum);
    bgp_peer_dump_id(pStream, pPeer);
    log_printf(pStream, ": %d / %d\n",  iNumBest, iNumPrefixes);
  }
}


/////////////////////////////////////////////////////////////////////
//
// INITIALIZATION AND FINALIZATION SECTION
//
/////////////////////////////////////////////////////////////////////

// -----[ _bgp_router_destroy ]--------------------------------------
void _bgp_router_destroy()
{
  str_destroy(&BGP_OPTIONS_SHOW_FORMAT);
}

/////////////////////////////////////////////////////////////////////
// TEST
/////////////////////////////////////////////////////////////////////
