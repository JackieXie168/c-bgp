// ==================================================================
// @(#)peer.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @author Sebastien Tandel (standel@info.ucl.ac.be)
// @date 24/11/2002
// @lastdate 14/02/2005
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

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

char * SESSION_STATES[4]= {
  "IDLE",
  "OPENWAIT",
  "ESTABLISHED",
  "ACTIVE"
};

// ----- peer_create ------------------------------------------------
/**
 * Create a new peer structure and initialize the following
 * structures:
 *   - default input/output filters (that accept everything)
 *   - input/output adjacent RIBs
 */
SPeer * peer_create(uint16_t uRemoteAS, net_addr_t tAddr,
		    SBGPRouter * pLocalRouter, uint8_t uPeerType)
{
  SPeer * pPeer= (SPeer *) MALLOC(sizeof(SPeer));
  pPeer->uRemoteAS= uRemoteAS;
  pPeer->tAddr= tAddr;
  pPeer->uPeerType= uPeerType;
  pPeer->pLocalRouter= pLocalRouter;
  pPeer->pInFilter= NULL; // Default = ACCEPT ANY
  pPeer->pOutFilter= NULL; // Default = ACCEPT ANY
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

    /* Free input and output filters */
    filter_destroy(&(*ppPeer)->pInFilter);
    filter_destroy(&(*ppPeer)->pOutFilter);

    /* Free input and output adjacent RIBs */
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
 * Internal helper function: send the given route to this prefix.
 */
int peer_prefix_disseminate(uint32_t uKey, uint8_t uKeyLen,
			    void * pItem, void * pContext)
{
  SPeer * pPeer= (SPeer *) pContext;
  SRoute * pRoute= (SRoute *) pItem;

  bgp_router_decision_process_disseminate_to_peer(pPeer->pLocalRouter,
						  pRoute->sPrefix,
						  pRoute, pPeer);
  return 0;
}

/////////////////////////////////////////////////////////////////////
//
// BGP SESSION MANAGEMENT FUNCTIONS
//
// One peering session may be in one of 4 states:
//   IDLE       : the session is not established (and administratively
//                down)
//   ACTIVE     : the session is not established due to a network
//                problem 
//   OPENWAIT   : the router has sent an OPEN message to the peer
//                router and awaits an OPEN message reply, or a
//   ESTABLISHED: the router has received an OPEN message or an UPDATE
//                message while being in OPENWAIT state.
/////////////////////////////////////////////////////////////////////

// ----- bgp_peer_session_ok ----------------------------------------
/**
 * Checks if the session with the peer is operational. First, the
 * function checks if there is a route towards the peer router. Then,
 * the function checks if the resulting link is up. If both conditions
 * are met, the BGP session is considered OK.
 */
int bgp_peer_session_ok(SBGPPeer * pPeer)
{
  SNetLink * pNextHopIf= node_rt_lookup(pPeer->pLocalRouter->pNode,
					pPeer->tAddr);
  return ((pNextHopIf != NULL) && (link_get_state(pNextHopIf, NET_LINK_FLAG_UP)));
}

// ----- bgp_peer_session_refresh -----------------------------------
/**
 * Refresh the state of the BGP session. If the session is currently
 * in ESTABLISHED or OPENWAIT state, test is it still operational. If
 * the session is in ACTIVE state, test if it must be restarted.
 */
void bgp_peer_session_refresh(SBGPPeer * pPeer)
{
  if ((pPeer->uSessionState == SESSION_STATE_ESTABLISHED) ||
      (pPeer->uSessionState == SESSION_STATE_OPENWAIT)) {

    /* Check that the peer is reachable (that is, there is a route
       towards the peer). If not, shutdown the peering. */
    if (!bgp_peer_session_ok(pPeer)) {
      assert(!bgp_peer_close_session(pPeer));
      pPeer->uSessionState= SESSION_STATE_ACTIVE;
    }

  } else if (pPeer->uSessionState == SESSION_STATE_ACTIVE) {
    
    /* Check that the peer is reachable (that is, there is a route
       towards the peer). If yes, open the session. */
    if (bgp_peer_session_ok(pPeer))
      assert(!bgp_peer_open_session(pPeer));
    
  }
}

// ----- bgp_peer_open_session --------------------------------------
/**
 * Open the session with the peer. Opening the session includes
 * sending a BGP OPEN message to the peer and switching to OPENWAIT
 * state. Virtual peers are treated in a special way: since the peer
 * does not really exist, no message is sent and the state directly
 * becomes ESTABLISHED if the peer node is reachable.
 *
 * There is also an option for virtual peers. Their routes are learned
 * by the way of the 'recv' command. If the session becomes down, the
 * Adj-RIB-in is normally cleared. However, for virtual peers, when
 * the session is restarted, the routes must be re-sent. Using the
 * soft-restart option prevent the Adj-RIB-in to be cleared. When the
 * session goes up, the decision process is re-run for the routes
 * present in the Adj-RIB-in.
 *
 * Precondition:
 * - the session must be in IDLE or ACTIVE state or an error will be
 *   issued.
 */
int bgp_peer_open_session(SPeer * pPeer)
{
  SBGPMsg * pMsg;

  if ((pPeer->uSessionState == SESSION_STATE_IDLE) ||
      (pPeer->uSessionState == SESSION_STATE_ACTIVE)) {

    /* Send an OPEN message to the peer (except for virtual peers) */
    if (!peer_flag_get(pPeer, PEER_FLAG_VIRTUAL)) {
      pMsg= bgp_msg_open_create(pPeer->pLocalRouter->uNumber);
      if (bgp_msg_send(pPeer->pLocalRouter->pNode, pPeer->tAddr,
		       pMsg) == 0) {
	pPeer->uSessionState= SESSION_STATE_OPENWAIT;
      } else {
	pPeer->uSessionState= SESSION_STATE_ACTIVE;
      }
    } else {

      /* For virtual peers, we check that the peer is reachable
       * through the IGP. If so, the state directly goes to
       * ESTABLISHED. Otherwise, the state goes to ACTIVE. */
      if (1) {
	pPeer->uSessionState= SESSION_STATE_ESTABLISHED;

	/* If the virtual peer is configured with the soft-restart
	   option, scan the Adj-RIB-in and run the decision process
	   for each route. */
	if (peer_flag_get(pPeer, PEER_FLAG_SOFT_RESTART))
	  peer_rescan_adjribin(pPeer, 0);

      }
    }

    return 0;
  } else {
    LOG_WARNING("Warning: session already opened\n");
    return -1;
  }
}

// ----- bgp_peer_close_session -------------------------------------
/**
 * Close the BGP session with the peer. Closing the BGP session
 * includes sending a BGP CLOSE message, withdrawing the routes
 * learned through this peer and clearing the peer's Adj-RIB-in.
 *
 * Precondition:
 * - the peer must not be in IDLE state.
 */
int bgp_peer_close_session(SPeer * pPeer)
{
  SBGPMsg * pMsg;
  int iClear;

  if (pPeer->uSessionState != SESSION_STATE_IDLE) {

    LOG_DEBUG("> AS%d.peer_close_session.begin\n",
	      pPeer->pLocalRouter->uNumber);
    LOG_DEBUG("\tpeer: AS%d\n", pPeer->uRemoteAS);

    /* If the session is in OPENWAIT or ESTABLISHED state, send a
       CLOSE message to the peer (except for virtual peers). */
    if ((pPeer->uSessionState == SESSION_STATE_OPENWAIT) ||
	(pPeer->uSessionState == SESSION_STATE_ESTABLISHED)) {

      if (!peer_flag_get(pPeer, PEER_FLAG_VIRTUAL)) {
	pMsg= bgp_msg_close_create(pPeer->pLocalRouter->uNumber);
	bgp_msg_send(pPeer->pLocalRouter->pNode, pPeer->tAddr, pMsg);
      }

    }    
    pPeer->uSessionState= SESSION_STATE_IDLE;

    /* For virtual peers configured with the soft-restart option, do
       not clear the Adj-RIB-in. */
    iClear= !(peer_flag_get(pPeer, PEER_FLAG_VIRTUAL) &&
	      peer_flag_get(pPeer, PEER_FLAG_SOFT_RESTART));
    peer_rescan_adjribin(pPeer, iClear);

    LOG_DEBUG("< AS%d.peer_close_session.end\n", pPeer->pLocalRouter->uNumber);

    return 0;
  } else {
    LOG_WARNING("Warning: session not opened\n");
    return -1;
  }
}

// ----- bgp_peer_session_error -------------------------------------
/**
 * This function is used to dump information on the peering session
 * (the AS number and the IP address of the local router and the peer
 * router).
 */
void bgp_peer_session_error(SBGPPeer * pPeer)
{
    LOG_FATAL("Error: peer=");
    bgp_peer_dump_id(log_get_stream(pMainLog), pPeer);
    LOG_FATAL("\n");
    LOG_FATAL("Error: router=");
    bgp_router_dump_id(log_get_stream(pMainLog), pPeer->pLocalRouter);
    LOG_FATAL("\n");
}

// ----- peer_session_open_rcvd -------------------------------------
void peer_session_open_rcvd(SPeer * pPeer)
{
  /* Check that the message does not come from a virtual peer */
  if (peer_flag_get(pPeer, PEER_FLAG_VIRTUAL)) {
    LOG_FATAL("Error: OPEN message received from virtual peer\n");
    bgp_peer_session_error(pPeer);
    abort();
  }

  LOG_INFO("BGP_MSG_RCVD: OPEN\n");
  switch (pPeer->uSessionState) {
  case SESSION_STATE_ACTIVE:
    pPeer->uSessionState= SESSION_STATE_ESTABLISHED;

    bgp_msg_send(pPeer->pLocalRouter->pNode, pPeer->tAddr,
		 bgp_msg_open_create(pPeer->pLocalRouter->uNumber));

    rib_for_each(pPeer->pLocalRouter->pLocRIB,
		 peer_prefix_disseminate, pPeer);
    break;
  case SESSION_STATE_OPENWAIT:
    pPeer->uSessionState= SESSION_STATE_ESTABLISHED;
    rib_for_each(pPeer->pLocalRouter->pLocRIB,
		 peer_prefix_disseminate, pPeer);
    break;
  default:
    LOG_FATAL("Error: OPEN received while in %s state\n",
	      SESSION_STATES[pPeer->uSessionState]);
    bgp_peer_session_error(pPeer);
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
    bgp_peer_session_error(pPeer);
    abort();
  }

  LOG_INFO("BGP_MSG_RCVD: CLOSE\n");
  switch (pPeer->uSessionState) {
  case SESSION_STATE_ESTABLISHED:
  case SESSION_STATE_OPENWAIT:
    pPeer->uSessionState= SESSION_STATE_IDLE;
    peer_rescan_adjribin(pPeer, !peer_flag_get(pPeer, PEER_FLAG_VIRTUAL));
    break;
  case SESSION_STATE_ACTIVE:
    break;
  default:
    LOG_FATAL("Error: CLOSE received while in %s state\n",
	      SESSION_STATES[pPeer->uSessionState]);
    bgp_peer_session_error(pPeer);
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
    bgp_peer_session_error(pPeer);
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
    bgp_peer_session_error(pPeer);
    abort();    
  }
  LOG_DEBUG("BGP_FSM_STATE: %s\n", SESSION_STATES[pPeer->uSessionState]);
}

// ----- peer_disable_adjribin_for_each -----------------------------
/**
 *
 */
int peer_disable_adjribin_for_each(uint32_t uKey, uint8_t uKeyLen,
				   void * pItem, void * pContext)
{
  SRoute * pRoute= (SRoute *) pItem;
  SPeer * pPeer= (SPeer *) pContext;

  //route_flag_set(pRoute, ROUTE_FLAG_ELIGIBLE, 0);

  /* Since the ROUTE_FLAG_BEST is handled in the Adj-RIB-In, we only
     need to run the decision process if the route is installed in the
     Loc-RIB (i.e. marked as best) */
  if (route_flag_get(pRoute, ROUTE_FLAG_BEST)) {

    LOG_DEBUG("\trescan: ", pPeer->pLocalRouter->uNumber);
    LOG_ENABLED_DEBUG() route_dump(log_get_stream(pMainLog), pRoute);
    LOG_DEBUG("\n");
    bgp_router_decision_process(pPeer->pLocalRouter, pPeer, pRoute->sPrefix);

  }

  return 0;
}

// ----- peer_enable_adjribin_for_each ------------------------------
/**
 *
 */
int peer_enable_adjribin_for_each(uint32_t uKey, uint8_t uKeyLen,
				  void * pItem, void * pContext)
{
  SRoute * pRoute= (SRoute *) pItem;
  SPeer * pPeer= (SPeer *) pContext;

  //route_flag_set(pRoute, ROUTE_FLAG_ELIGIBLE, 0);

  bgp_router_decision_process(pPeer->pLocalRouter, pPeer, pRoute->sPrefix);

  return 0;
}

// ----- peer_adjrib_clear ------------------------------------------
/**
 *
 */
void peer_adjrib_clear(SPeer * pPeer, int iIn)
{
  if (iIn) {
    rib_destroy(&pPeer->pAdjRIBIn);
    pPeer->pAdjRIBIn= rib_create(0);
  } else {
    rib_destroy(&pPeer->pAdjRIBOut);
    pPeer->pAdjRIBOut= rib_create(0);
  }
}

// ----- peer_rescan_adjribin ---------------------------------------
/**
 *
 */
void peer_rescan_adjribin(SPeer * pPeer, int iClear)
{
  if (pPeer->uSessionState == SESSION_STATE_ESTABLISHED) {

    rib_for_each(pPeer->pAdjRIBIn, peer_enable_adjribin_for_each, pPeer);

  } else {

    // For each route in Adj-RIB-In, mark as unfeasible
    // and run decision process for each route marked as best
    rib_for_each(pPeer->pAdjRIBIn, peer_disable_adjribin_for_each, pPeer);
    
    // Clear Adj-RIB-In ?
    if (iClear)
      peer_adjrib_clear(pPeer, 1);

  }

}

/////////////////////////////////////////////////////////////////////
//
// BGP FILTERS
//
/////////////////////////////////////////////////////////////////////

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

/////////////////////////////////////////////////////////////////////
//
// BGP MESSAGE HANDLING
//
/////////////////////////////////////////////////////////////////////

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
    bgp_msg_send(pPeer->pLocalRouter->pNode, pPeer->tAddr,
		 bgp_msg_update_create(pPeer->pLocalRouter->uNumber,
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
    bgp_msg_send(pPeer->pLocalRouter->pNode, pPeer->tAddr,
		 bgp_msg_withdraw_create(pPeer->pLocalRouter->uNumber,
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

  pLink= node_find_link(pPeer->pLocalRouter->pNode, pRoute->tNextHop);
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
 * bgp_router_advertise_to_peer]
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
 *   - PREF ext-community [ EXPERIMENTAL ]
 *
 * Returns:
 *   0 => Ignore route (destroy)
 *   1 => Redistribute
 */
int peer_comm_process(SRoute * pRoute)
{
  int iIndex;

  /* Classical communities */
  if (pRoute->pCommunities != NULL) {
#ifdef __EXPERIMENTAL__
    if (route_comm_contains(pRoute, COMM_DEPREF)) {
      route_localpref_set(pRoute, 0);
    }
#endif
  }

  /* Extended communities */
  if (pRoute->pECommunities != NULL) {

    for (iIndex= 0; iIndex < ecomm_length(pRoute->pECommunities); iIndex++) {
      SECommunity * pComm= ecomm_get_index(pRoute->pECommunities, iIndex);
      switch (pComm->uTypeHigh) {

#ifdef __EXPERIMENTAL__
      case ECOMM_PREF:
	route_localpref_set(pRoute, ecomm_pref_get(pComm));
	break;
#endif

      }
    }
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
  return (filter_apply(pPeer->pInFilter, pPeer->pLocalRouter, pRoute) &&
	  peer_comm_process(pRoute));
}

// ----- peer_route_feasible ----------------------------------------
/**
 * The route is feasible if and only if the next-hop is reachable
 * (through a STATIC, IGP or BGP route).
 */
int peer_route_feasible(SPeer * pPeer, SRoute * pRoute)
{
  SNetLink * pLink= node_rt_lookup(pPeer->pLocalRouter->pNode, pRoute->tNextHop);

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

  LOG_DEBUG("> AS%d.peer_handle_message.begin\n", pPeer->pLocalRouter->uNumber);

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
    if (pPeer->uRemoteAS != pPeer->pLocalRouter->uNumber)
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
    bgp_router_decision_process(pPeer->pLocalRouter, pPeer,
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
    bgp_router_decision_process(pPeer->pLocalRouter, pPeer,
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

  LOG_DEBUG("< AS%d.peer_handle_message.end\n", pPeer->pLocalRouter->uNumber);

  return 0;
}

/////////////////////////////////////////////////////////////////////
//
// INFORMATION RETRIEVAL
//
/////////////////////////////////////////////////////////////////////

// ----- bgp_peer_dump_id -------------------------------------------
/**
 *
 */
void bgp_peer_dump_id(FILE * pStream, SBGPPeer * pPeer)
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
  int iOptions= 0;

  ip_address_dump(pStream, pPeer->tAddr);
  fprintf(pStream, "\tAS%d\t%s", pPeer->uRemoteAS,
	  SESSION_STATES[pPeer->uSessionState]);
  
  if (peer_flag_get(pPeer, PEER_FLAG_RR_CLIENT)) {
    fprintf(pStream, (iOptions++)?",":"\t(");
    fprintf(pStream, "rr-client");
  }
  if (peer_flag_get(pPeer, PEER_FLAG_NEXT_HOP_SELF)) {
    fprintf(pStream, (iOptions++)?",":"\t(");
    fprintf(pStream, "next-hop-self");
  }
  if (peer_flag_get(pPeer, PEER_FLAG_VIRTUAL)) {
    fprintf(pStream, (iOptions++)?",":"\t(");
    fprintf(pStream, "virtual");
  }
  if (peer_flag_get(pPeer, PEER_FLAG_SOFT_RESTART)) {
    fprintf(pStream, (iOptions++)?",":"\t(");
    fprintf(pStream, "soft-restart");
  }
  fprintf(pStream, (iOptions)?")":"");
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

// ----- bgp_peer_dump_adjrib ---------------------------------------
/**
 * Dump Adj-RIB-In/Out.
 *
 * Parameters:
 * - iInOut, if 1, dump Adj-RIB-In, otherwize, dump Adj-RIB-Out.
 */
void bgp_peer_dump_adjrib(FILE * pStream, SPeer * pPeer,
			  SPrefix sPrefix, int iInOut)
{
  SRouteDumpCtx sCtx;
  SRoute * pRoute;
  SRIB * pRIB;

  if (iInOut)
    pRIB= pPeer->pAdjRIBIn;
  else
    pRIB= pPeer->pAdjRIBOut;

  sCtx.pStream= pStream;
  sCtx.pPeer= pPeer;

  if (sPrefix.uMaskLen == 0) {
    rib_for_each(pRIB, bgp_peer_dump_route, &sCtx);
  } else if (sPrefix.uMaskLen >= 32) {
    pRoute= rib_find_best(pRIB, sPrefix);
    if (pRoute != NULL) {
      route_dump(pStream, pRoute);
      fprintf(pStream, "\n");
    }
  } else {
    pRoute= rib_find_exact(pRIB, sPrefix);
    if (pRoute != NULL) {
      route_dump(pStream, pRoute);
      fprintf(pStream, "\n");
    }
  }
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
