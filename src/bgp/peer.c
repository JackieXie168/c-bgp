// ==================================================================
// @(#)peer.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be), Sebastien Tandel
// @date 24/11/2002
// @lastdate 03/01/2005
// ==================================================================

#include <assert.h>
#include <stdlib.h>

#include <libgds/log.h>
#include <libgds/memory.h>

#include <bgp/as.h>
#include <bgp/comm.h>
#include <bgp/ecomm.h>
#include <bgp/message.h>
#include <bgp/peer.h>
#include <bgp/qos.h>
#include <bgp/route.h>

char * SESSION_STATES[3]= {
  "IDLE",
  "OPENWAIT",
  "ESTABLISHED"
};

// ----- peer_create ------------------------------------------------
/**
 * Create a new peer structure and initialize the following
 * structures:
 *   - default input/output filters (that accept everything)
 *   - input/output adjacent RIBs
 */
SPeer * peer_create(uint16_t uRemoteAS, net_addr_t tAddr,
		    SAS * pLocalAS, uint8_t uPeerType)
{
  SPeer * pPeer= (SPeer *) MALLOC(sizeof(SPeer));
  pPeer->uRemoteAS= uRemoteAS;
  pPeer->tAddr= tAddr;
  pPeer->uPeerType= uPeerType;
  pPeer->pLocalAS= pLocalAS;
  pPeer->pInFilter= NULL; // default = ACCEPT
  pPeer->pOutFilter= NULL; // default = ACCEPT
  pPeer->pAdjRIBIn= rib_create(0);
  pPeer->pAdjRIBOut= rib_create(0);
  pPeer->uSessionState= SESSION_STATE_IDLE;
  pPeer->uFlags= 0;
  return pPeer;
}

// ----- peer_destroy -----------------------------------------------
/**
 * Destroy the given peer structure and free the related memory.
 */
void peer_destroy(SPeer ** ppPeer)
{
  if (*ppPeer != NULL) {
    // Free input and output filters
    filter_destroy(&(*ppPeer)->pInFilter);
    filter_destroy(&(*ppPeer)->pOutFilter);
    // Free input and output adjacent RIBs
    rib_destroy(&(*ppPeer)->pAdjRIBIn);
    rib_destroy(&(*ppPeer)->pAdjRIBOut);
    FREE(*ppPeer);
    *ppPeer= NULL;
  }
}

   
// ----- peer_flag_set ----------------------------------------------
/**
 * Change the state of a flag of this peer.
 */
void peer_flag_set(SPeer * pPeer, uint8_t uFlag, int iState)
{
  if (iState)
    pPeer->uFlags|= uFlag;
  else
    pPeer->uFlags&= ~uFlag;
}

// ----- peer_flag_get ----------------------------------------------
/**
 * Return the state of a flag of this peer.
 */
int peer_flag_get(SPeer * pPeer, uint8_t uFlag)
{
  return (pPeer->uFlags & uFlag) != 0;
}

// ----- peer_prefix_disseminate ------------------------------------
/**
 * Internal helper function.
 *
 * Send the given route to this prefix.
 */
int peer_prefix_disseminate(uint32_t uKey, uint8_t uKeyLen,
			    void * pItem, void * pContext)
{
  SPeer * pPeer= (SPeer *) pContext;
  SRoute * pRoute= (SRoute *) pItem;

  as_decision_process_disseminate_to_peer(pPeer->pLocalAS,
					    pRoute->sPrefix,
					    pRoute, pPeer);

  return 0;
}

// ----- peer_open_session --------------------------------------
/**
 *
 */
int peer_open_session(SPeer * pPeer)
{
  if (pPeer->uSessionState == SESSION_STATE_IDLE) {
    pPeer->uSessionState= SESSION_STATE_OPENWAIT;

    /* Send an OPEN message to the peer (except for virtual peers) */
    if (!peer_flag_get(pPeer, PEER_FLAG_VIRTUAL)) {
      bgp_msg_send(pPeer->pLocalAS->pNode, pPeer->tAddr,
		   bgp_msg_open_create(pPeer->pLocalAS->uNumber));
    }

    return 0;
  } else {
    LOG_WARNING("Warning: session already opened\n");
    return -1;
  }
}

// ----- peer_close_session -----------------------------------------
/**
 *
 */
int peer_close_session(SPeer * pPeer)
{
  if (pPeer->uSessionState != SESSION_STATE_IDLE) {
    
    LOG_DEBUG("> AS%d.peer_close_session.begin\n", pPeer->pLocalAS->uNumber);
    LOG_DEBUG("\tpeer: AS%d\n", pPeer->uRemoteAS);
    pPeer->uSessionState= SESSION_STATE_IDLE;
    peer_clear_adjribin(pPeer);
    LOG_DEBUG("< AS%d.peer_close_session.end\n", pPeer->pLocalAS->uNumber);

    /* Send a CLOSE message to the peer (except for virtual peers) */
    if (!peer_flag_get(pPeer, PEER_FLAG_VIRTUAL)) {    
      bgp_msg_send(pPeer->pLocalAS->pNode, pPeer->tAddr,
		   bgp_msg_close_create(pPeer->pLocalAS->uNumber));
    }

    return 0;
  } else {
    LOG_WARNING("Warning: session not opened\n");
    return -1;
  }
}

/////////////////////////////////////////////////////////////////////
// BGP SESSION MANAGEMENT FUNCTIONS
/////////////////////////////////////////////////////////////////////

// ----- peer_session_open_rcvd -------------------------------------
void peer_session_open_rcvd(SPeer * pPeer)
{
  /* Check that the message does not come from a virtual peer */
  if (peer_flag_get(pPeer, PEER_FLAG_VIRTUAL)) {
    LOG_FATAL("Error: OPEN message received from virtual peer\n");
    LOG_FATAL("Error: virtual peer=");
    peer_dump_id(log_get_stream(pMainLog), pPeer);
    LOG_FATAL("\n");
    LOG_FATAL("Error: as=%d:", pPeer->pLocalAS->uNumber);
    ip_address_dump(log_get_stream(pMainLog), pPeer->pLocalAS->pNode->tAddr);
    LOG_FATAL("\n");
    abort();
  }

  LOG_INFO("BGP_MSG_RCVD: OPEN\n");
  switch (pPeer->uSessionState) {
  case SESSION_STATE_IDLE:
    pPeer->uSessionState= SESSION_STATE_ESTABLISHED;

    bgp_msg_send(pPeer->pLocalAS->pNode, pPeer->tAddr,
		 bgp_msg_open_create(pPeer->pLocalAS->uNumber));

    rib_for_each(pPeer->pLocalAS->pLocRIB,
		 peer_prefix_disseminate, pPeer);
    break;
  case SESSION_STATE_OPENWAIT:
    pPeer->uSessionState= SESSION_STATE_ESTABLISHED;
    rib_for_each(pPeer->pLocalAS->pLocRIB,
		 peer_prefix_disseminate, pPeer);
    break;
  default:
    LOG_FATAL("Error: OPEN received while in %s state\n",
	      SESSION_STATES[pPeer->uSessionState]);
    abort();
  }
  LOG_DEBUG("BGP_FSM_STATE: %s\n", SESSION_STATES[pPeer->uSessionState]);
}

// ----- peer_session_close_rcvd ------------------------------------
void peer_session_close_rcvd(SPeer * pPeer)
{
  /* Check that the message does not come from a virtual peer */
  if (peer_flag_get(pPeer, PEER_FLAG_VIRTUAL)) {
    LOG_FATAL("Error: CLOSE message received from virtual peer\n");
    LOG_FATAL("Error: virtual peer=");
    peer_dump_id(log_get_stream(pMainLog), pPeer);
    LOG_FATAL("\n");
    LOG_FATAL("Error: as=%d:", pPeer->pLocalAS->uNumber);
    ip_address_dump(log_get_stream(pMainLog), pPeer->pLocalAS->pNode->tAddr);
    LOG_FATAL("\n");
    abort();
  }

  LOG_INFO("BGP_MSG_RCVD: CLOSE\n");
  switch (pPeer->uSessionState) {
  case SESSION_STATE_ESTABLISHED:
  case SESSION_STATE_OPENWAIT:
    pPeer->uSessionState= SESSION_STATE_IDLE;
    peer_clear_adjribin(pPeer);
    break;
  case SESSION_STATE_IDLE:
    break;
  default:
    LOG_FATAL("Error: CLOSE received while in %s state\n",
	      SESSION_STATES[pPeer->uSessionState]);
    abort();    
  }
  LOG_DEBUG("BGP_FSM_STATE: %s\n", SESSION_STATES[pPeer->uSessionState]);
}

// ----- peer_session_update_rcvd -----------------------------------
void peer_session_update_rcvd(SPeer * pPeer, SBGPMsg * pMsg)
{
  LOG_INFO("BGP_MSG_RCVD: UPDATE\n");
  switch (pPeer->uSessionState) {
  case SESSION_STATE_OPENWAIT:
    pPeer->uSessionState= SESSION_STATE_ESTABLISHED;
    break;
  case SESSION_STATE_ESTABLISHED:
    break;
  default:
    LOG_WARNING("Warning: UPDATE received while in %s state\n",
	      SESSION_STATES[pPeer->uSessionState]);
    LOG_WARNING("Warning: from peer ");
    LOG_ENABLED_WARNING()
      ip_address_dump(log_get_stream(pMainLog), pPeer->tAddr);
    LOG_WARNING("(AS%u)\n", pPeer->uRemoteAS);
  }
  LOG_DEBUG("BGP_FSM_STATE: %s\n", SESSION_STATES[pPeer->uSessionState]);
}

// ----- peer_session_withdraw_rcvd ---------------------------------
void peer_session_withdraw_rcvd(SPeer * pPeer)
{
  LOG_INFO("BGP_MSG_RCVD: WITHDRAW\n");
  switch (pPeer->uSessionState) {
  case SESSION_STATE_OPENWAIT:
    pPeer->uSessionState= SESSION_STATE_ESTABLISHED;
    break;
  case SESSION_STATE_ESTABLISHED:
    break;
  default:
    LOG_FATAL("Error: WITHDRAW received while in %s state\n",
	      SESSION_STATES[pPeer->uSessionState]);
    abort();    
  }
  LOG_DEBUG("BGP_FSM_STATE: %s\n", SESSION_STATES[pPeer->uSessionState]);
}

// ----- peer_clear_adjribin_for_each -------------------------------
/**
 *
 */
int peer_clear_adjribin_for_each(uint32_t uKey, uint8_t uKeyLen,
				  void * pItem, void * pContext)
{
  SRoute * pRoute= (SRoute *) pItem;
  SPeer * pPeer= (SPeer *) pContext;

  route_flag_set(pRoute, ROUTE_FLAG_ELIGIBLE, 0);

  /* Since the ROUTE_FLAG_BEST is handled in the Adj-RIB-In, we only
     need to run the decision process if the route is installed in the
     Loc-RIB (i.e. marked as best) */
  if (route_flag_get(pRoute, ROUTE_FLAG_BEST)) {

    LOG_DEBUG("\twithdraw: ", pPeer->pLocalAS->uNumber);
    LOG_ENABLED_DEBUG() route_dump(log_get_stream(pMainLog), pRoute);
    LOG_DEBUG("\n");
    as_decision_process(pPeer->pLocalAS, pPeer, pRoute->sPrefix);

  }

  return 0;
}

// ----- peer_clear_adjribin ----------------------------------------
/**
 *
 */
void peer_clear_adjribin(SPeer * pPeer)
{
  // *** lock Adj-RIB-In ***

  // For each route in Adj-RIB-In, mark as unfeasible
  // and run decision process for each route marked as best
  rib_for_each(pPeer->pAdjRIBIn, peer_clear_adjribin_for_each, pPeer);

  // Clear Adj-RIB-In
  rib_destroy(&pPeer->pAdjRIBIn);
  pPeer->pAdjRIBIn= rib_create(0);

  // *** unlock Adj-RIB-In ***
}

// ----- peer_set_in_filter -----------------------------------------
/**
 * Change the input filter of this peer. The previous input filter is
 * destroyed.
 */
void peer_set_in_filter(SPeer * pPeer, SFilter * pFilter)
{
  if (pPeer->pInFilter != NULL)
    filter_destroy(&pPeer->pInFilter);
  pPeer->pInFilter= pFilter;
}

// ----- peer_in_filter_get -----------------------------------------
/**
 * Return the input filter of this peer.
 */
SFilter * peer_in_filter_get(SPeer * pPeer)
{
  return pPeer->pInFilter;
}

// ----- peer_set_out_filter ----------------------------------------
/**
 * Change the output filter of this peer. The previous output filter
 * is destroyed.
 */
void peer_set_out_filter(SPeer * pPeer, SFilter * pFilter)
{
  if (pPeer->pOutFilter != NULL)
    filter_destroy(&pPeer->pOutFilter);
  pPeer->pOutFilter= pFilter;
}

// ----- peer_out_filter_get ----------------------------------------
/**
 * Return the output filter of this peer.
 */
SFilter * peer_out_filter_get(SPeer * pPeer)
{
  return pPeer->pOutFilter;
}

// ----- peer_announce_route ----------------------------------------
/**
 * Announce the given route to the given peer.
 *
 * PRE: the route must be a copy that can live its own life,
 * i.e. there must not be references to the route.
 *
 * Note: if the route is not delivered (for instance in the case of
 * a virtual peer), the given route is freed.
 */
void peer_announce_route(SPeer * pPeer, SRoute * pRoute)
{
  route_peer_set(pRoute, pPeer);

  /* Send the message to the peer (except if this is a virtual peer) */
  if (!peer_flag_get(pPeer, PEER_FLAG_VIRTUAL)) {
    bgp_msg_send(pPeer->pLocalAS->pNode, pPeer->tAddr,
		 bgp_msg_update_create(pPeer->pLocalAS->uNumber,
				       pRoute));
  } else
      route_destroy(&pRoute);
}

// ----- peer_withdraw_prefix ---------------------------------------
/**
 * Withdraw the given prefix to the given peer.
 *
 * Note: withdraws are not sent to virtual peers.
 */
void peer_withdraw_prefix(SPeer * pPeer, SPrefix sPrefix)
{
  /* Send the message to the peer (except if this is a virtual peer) */
  if (!peer_flag_get(pPeer, PEER_FLAG_VIRTUAL)) {
    bgp_msg_send(pPeer->pLocalAS->pNode, pPeer->tAddr,
		 bgp_msg_withdraw_create(pPeer->pLocalAS->uNumber,
					 sPrefix));
  }
}

// ----- peer_route_delay_update ------------------------------------
/**
 * Add the delay between this router and the next-hop. At this moment,
 * the delay is taken from an existing direct link between this router
 * and the next-hop but in the future such a direct link may not exist
 * and the information should be taken from the IGP.
 */
void peer_route_delay_update(SPeer * pPeer, SRoute * pRoute)
{
  SNetLink * pLink;

  LOG_FATAL("Error: peer_route_delay_update MUST be modified\n");
  LOG_FATAL("Error: to support route-reflection !!!");
  abort();

  pLink= node_find_link(pPeer->pLocalAS->pNode, pRoute->tNextHop);
  assert(pLink != NULL);

#ifdef BGP_QOS
  qos_route_update_delay(pRoute, pLink->tDelay);
#endif
}

// ----- peer_route_rr_client_update --------------------------------
/**
 * Store in the route a flag indicating whether the route has been
 * learned from an RR client or not. This is used to improve the speed
 * of the redistribution step of the BGP decision process [as.c,
 * as_advertise_to_peer]
 */
void peer_route_rr_client_update(SPeer * pPeer, SRoute * pRoute)
{
  route_flag_set(pRoute, ROUTE_FLAG_RR_CLIENT,
		 peer_flag_get(pPeer, PEER_FLAG_RR_CLIENT));
}

// ----- peer_comm_process ------------------------------------------
/**
 * Apply input communities.
 *   - DEPREF community [ EXPERIMENTAL ]
 *
 * Returns:
 *   0 => Ignore route (destroy)
 *   1 => Redistribute
 */
int peer_comm_process(SRoute * pRoute)
{
  if (pRoute->pCommunities != NULL) {
#ifdef __EXPERIMENTAL__
    if (route_comm_contains(pRoute, COMM_DEPREF)) {
      route_localpref_set(pRoute, 0);
    }
#endif
  }

  return 1;
}

// ----- peer_route_eligible ----------------------------------------
/**
 * The route is eligible if it passes the input filters. Standard
 * filters are applied first. Then, extended communities actions are
 * taken if any (see 'peer_ecomm_process').
 */
int peer_route_eligible(SPeer * pPeer, SRoute * pRoute)
{
  return (filter_apply(pPeer->pInFilter, pPeer->pLocalAS, pRoute) &&
	  peer_comm_process(pRoute));
}

// ----- peer_route_feasible ----------------------------------------
/**
 * The route is feasible if and only if the next-hop is reachable
 * (through a STATIC, IGP or BGP route).
 */
int peer_route_feasible(SPeer * pPeer, SRoute * pRoute)
{
  SNetLink * pLink= node_rt_lookup(pPeer->pLocalAS->pNode, pRoute->tNextHop);

  return (pLink != NULL);
}

// ----- peer_handle_route ------------------------------------------
/**
 * Handle one route message (UPDATE or WITHDRAW).
 * This is PHASE I of the Decision Process.
 *
 * UPDATE:
 * - if a route towards the same prefix already exists it is replaced
 * by the new one. Otherwise the new route is simply added. The
 * function then tests if the route is feasible and set the route
 * flags accordingly.
 *
 * WITHDRAW:
 * - if a route towards the same prefix already exists, it is set as
 * 'unfeasible', then the decision process (phase-II) is
 * ran. Afterwards, the route is removed from the Adj-RIB-In.
 */
int peer_handle_message(SPeer * pPeer, SBGPMsg * pMsg)
{
  SBGPMsgUpdate * pMsgUpdate;
  SBGPMsgWithdraw * pMsgWithdraw;
  SRoute * pRoute;

  LOG_DEBUG("> AS%d.peer_handle_message.begin\n", pPeer->pLocalAS->uNumber);

  LOG_DEBUG("\tfrom AS%d:", pPeer->uRemoteAS);
  LOG_ENABLED_DEBUG()
    ip_address_dump(log_get_stream(pMainLog), pPeer->tAddr);
  LOG_DEBUG("\n");

  switch (pMsg->uType) {
  case BGP_MSG_UPDATE:
    peer_session_update_rcvd(pPeer, pMsg);
    pMsgUpdate= (SBGPMsgUpdate *) pMsg;
    // Replace former route in AdjRIBIn
    // *** lock Adj-RIB-In ***
    pRoute= pMsgUpdate->pRoute;
    route_peer_set(pRoute, pPeer);

    // If route learned over eBGP, clear LOCAL-PREF
    if (pPeer->uRemoteAS != pPeer->pLocalAS->uNumber)
      route_localpref_set(pRoute, BGP_OPTIONS_DEFAULT_LOCAL_PREF);
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

    assert(rib_replace_route(pPeer->pAdjRIBIn, pRoute) == 0);
    // *** unlock Adj-RIB-In
    // Run decision process for this route
    as_decision_process(pPeer->pLocalAS, pPeer,
			pRoute->sPrefix);
    break;
  case BGP_MSG_WITHDRAW:
    peer_session_withdraw_rcvd(pPeer);
    pMsgWithdraw= (SBGPMsgWithdraw *) pMsg;
    // Remove former route from AdjRIBIn
    // *** lock Adj-RIB-In ***
    pRoute= rib_find_exact(pPeer->pAdjRIBIn, pMsgWithdraw->sPrefix);

    if (pRoute == NULL) break;
    //assert(pRoute != NULL);

    route_flag_set(pRoute, ROUTE_FLAG_ELIGIBLE, 0);
    // *** unlock Adj-RIB-In ***
    // Run decision process in case this route is the best route
    // towards this prefix
    as_decision_process(pPeer->pLocalAS, pPeer,
			pMsgWithdraw->sPrefix);

    LOG_DEBUG("\tremove: ");
    LOG_ENABLED_DEBUG() route_dump(log_get_stream(pMainLog), pRoute);
    LOG_DEBUG("\n");
    assert(rib_remove_route(pPeer->pAdjRIBIn, pMsgWithdraw->sPrefix) == 0);
    break;
  case BGP_MSG_CLOSE:
    peer_session_close_rcvd(pPeer);
    break;
  case BGP_MSG_OPEN:
    peer_session_open_rcvd(pPeer);
    break;
  default:
    LOG_FATAL("Error: unknown message type received\n");
    LOG_FATAL("Error: \n");
    abort();
  }
  bgp_msg_destroy(&pMsg);

  LOG_DEBUG("< AS%d.peer_handle_message.end\n", pPeer->pLocalAS->uNumber);

  return 0;
}

// ----- peer_dump_id -----------------------------------------------
/**
 *
 */
void peer_dump_id(FILE * pStream, SPeer * pPeer)
{
  fprintf(pStream, "AS%d:", pPeer->uRemoteAS);
  ip_address_dump(pStream, pPeer->tAddr);
}

// ----- bgp_peer_dump ----------------------------------------------
/**
 *
 */
void bgp_peer_dump(FILE * pStream, SPeer * pPeer)
{
  ip_address_dump(pStream, pPeer->tAddr);
  fprintf(pStream, "\tAS%d\t%s", pPeer->uRemoteAS,
	  SESSION_STATES[pPeer->uSessionState]);
}

typedef struct {
  SPeer * pPeer;
  FILE * pStream;
} SRouteDumpCtx;

// ----- bgp_peer_dump_route ----------------------------------------
/**
 *
 */
int bgp_peer_dump_route(uint32_t uKey, uint8_t uKeyLen,
			  void * pItem, void * pContext)
{
  SRouteDumpCtx * pCtx= (SRouteDumpCtx *) pContext;

  route_dump(pCtx->pStream, (SRoute *) pItem);
  fprintf(pCtx->pStream, "\n");
  return 0;
}

// ----- bgp_peer_dump_ribin ----------------------------------------
/**
 *
 */
void bgp_peer_dump_ribin(FILE * pStream, SPeer * pPeer,
			 SPrefix sPrefix)
{
  SRouteDumpCtx sCtx;
  SRoute * pRoute;

  sCtx.pStream= pStream;
  sCtx.pPeer= pPeer;

  if (sPrefix.uMaskLen == 0) {
    rib_for_each(pPeer->pAdjRIBIn, bgp_peer_dump_route, &sCtx);
  } else if (sPrefix.uMaskLen >= 32) {
    pRoute= rib_find_best(pPeer->pAdjRIBIn, sPrefix);
    if (pRoute != NULL) {
      route_dump(pStream, pRoute);
      fprintf(pStream, "\n");
    }
  } else {
    pRoute= rib_find_exact(pPeer->pAdjRIBIn, sPrefix);
    if (pRoute != NULL) {
      route_dump(pStream, pRoute);
      fprintf(pStream, "\n");
    }
  }
}

// ----- bgp_peer_dump_string_ribin ---------------------------------
/**
 *
 */
char * bgp_peer_dump_string_ribin(SPeer * pPeer, SPrefix sPrefix)
{
  char * rib = MALLOC(255);
  return rib;
}

// ----- bgp_peer_dump_in_filters -----------------------------------
/**
 *
 */
void bgp_peer_dump_in_filters(FILE * pStream, SPeer * pPeer)
{
  filter_dump(pStream, pPeer->pInFilter);
}

// ----- bgp_peer_dump_out_filters ----------------------------------
/**
 *
 */
void bgp_peer_dump_out_filters(FILE * pStream, SPeer * pPeer)
{
  filter_dump(pStream, pPeer->pOutFilter);
}
