// ==================================================================
// @(#)peer.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @author Sebastien Tandel (standel@info.ucl.ac.be)
//
// @date 24/11/2002
// @lastdate 12/03/2008
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <stdlib.h>

#include <libgds/log.h>
#include <libgds/memory.h>

#include <net/error.h>
#include <net/icmp.h>
#include <net/record-route.h>
#include <bgp/as.h>
#include <bgp/comm.h>
#include <bgp/ecomm.h>
#include <bgp/message.h>
#include <bgp/peer.h>
#include <bgp/qos.h>
#include <bgp/route.h>
#include <net/network.h>
#include <net/node.h>

char * SESSION_STATES[SESSION_STATE_MAX]= {
  "IDLE",
  "OPENWAIT",
  "ESTABLISHED",
  "ACTIVE",
};

// -----[ function prototypes ] -------------------------------------
static inline void _bgp_peer_rescan_adjribin(bgp_peer_t * peer,
					     int clear);
static inline void _bgp_peer_process_update(bgp_peer_t * peer,
					    bgp_msg_update_t * msg);
static inline void _bgp_peer_process_withdraw(bgp_peer_t * pPeer,
					      bgp_msg_withdraw_t * msg);


// -----[ bgp_peer_create ]------------------------------------------
/**
 * Create a new peer structure and initialize the following
 * structures:
 *   - default input/output filters (accept everything)
 *   - input/output adjacent RIBs
 */
bgp_peer_t * bgp_peer_create(uint16_t uRemoteAS, net_addr_t tAddr,
			     bgp_router_t * pLocalRouter)
{
  bgp_peer_t * pPeer= (bgp_peer_t *) MALLOC(sizeof(bgp_peer_t));
  pPeer->uRemoteAS= uRemoteAS;
  pPeer->tAddr= tAddr;
  pPeer->tSrcAddr= NET_ADDR_ANY;
  pPeer->tRouterID= 0;
  pPeer->pLocalRouter= pLocalRouter;
  pPeer->pFilter[FILTER_IN]= NULL; // Default = ACCEPT ANY
  pPeer->pFilter[FILTER_OUT]= NULL; // Default = ACCEPT ANY
  pPeer->pAdjRIB[RIB_IN]= rib_create(0);
  pPeer->pAdjRIB[RIB_OUT]= rib_create(0);
  pPeer->tSessionState= SESSION_STATE_IDLE;
  pPeer->uFlags= 0;
  pPeer->tNextHop= NET_ADDR_ANY;
  pPeer->pRecordStream= NULL;
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  pPeer->uWaltonLimit = 1;
  bgp_router_walton_peer_set(pPeer, 1);
#endif
  pPeer->last_error= ESUCCESS;
  return pPeer;
}

// -----[ bgp_peer_destroy ]-----------------------------------------
/**
 * Destroy the given peer structure and free the related memory.
 */
void bgp_peer_destroy(bgp_peer_t ** ppPeer)
{
  if (*ppPeer != NULL) {

    /* Free input and output filters */
    filter_destroy(&(*ppPeer)->pFilter[FILTER_IN]);
    filter_destroy(&(*ppPeer)->pFilter[FILTER_OUT]);

    /* Free input and output adjacent RIBs */
    rib_destroy(&(*ppPeer)->pAdjRIB[RIB_IN]);
    rib_destroy(&(*ppPeer)->pAdjRIB[RIB_OUT]);

    FREE(*ppPeer);
    *ppPeer= NULL;
  }
}

   
// ----- bgp_peer_flag_set ------------------------------------------
/**
 * Change the state of a flag of this peer.
 */
void bgp_peer_flag_set(bgp_peer_t * pPeer, uint8_t uFlag, int iState)
{
  if (iState)
    pPeer->uFlags|= uFlag;
  else
    pPeer->uFlags&= ~uFlag;
}

// ----- bgp_peer_flag_get ------------------------------------------
/**
 * Return the state of a flag of this peer.
 */
int bgp_peer_flag_get(bgp_peer_t * pPeer, uint8_t uFlag)
{
  return (pPeer->uFlags & uFlag) != 0;
}

// ----- bgp_peer_set_nexthop ---------------------------------------
/**
 * Set the next-hop to be sent for routes advertised to this peer.
 */
int bgp_peer_set_nexthop(bgp_peer_t * pPeer, net_addr_t tNextHop)
{
  // Check that the local router has this address
  if (!node_has_address(pPeer->pLocalRouter->pNode, tNextHop))
    return -1;

  pPeer->tNextHop= tNextHop;

  return 0;
}

// -----[ bgp_peer_set_source ]--------------------------------------
int bgp_peer_set_source(bgp_peer_t * pPeer, net_addr_t tSrcAddr)
{
  pPeer->tSrcAddr= tSrcAddr;

  return 0;
}

// ----- _bgp_peer_prefix_disseminate -------------------------------
/**
 * Internal helper function: send the given route to this prefix.
 */
static int _bgp_peer_prefix_disseminate(uint32_t uKey, uint8_t uKeyLen,
					void * pItem, void * pContext)
{
  bgp_peer_t * pPeer= (bgp_peer_t *) pContext;
  bgp_route_t * pRoute= (bgp_route_t *) pItem;

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
//                router and awaits an OPEN message reply, or an
//                UPDATE.
//   ESTABLISHED: the router has received an OPEN message or an UPDATE
//                message while being in OPENWAIT state.
/////////////////////////////////////////////////////////////////////

// -----[ bgp_peer_session_ok ]--------------------------------------
/**
 * Checks if the session with the peer is operational. First, the
 * function checks if there is a route towards the peer router. Then,
 * the function checks if the resulting link is up. If both conditions
 * are met, the BGP session is considered OK.
 */
int bgp_peer_session_ok(bgp_peer_t * pPeer)
{
  int iResult;
  SNetDest sDest;
  ip_trace_t * pTrace;

  if (bgp_peer_flag_get(pPeer, PEER_FLAG_VIRTUAL)) {
    sDest.tType= NET_DEST_ADDRESS;
    sDest.uDest.tAddr= pPeer->tAddr;

    // This must be updated: the router is not necessarily
    // adjacent... need to perform a kind of traceroute...
    iResult= icmp_record_route(pPeer->pLocalRouter->pNode,
			       pPeer->tAddr, NULL, 255, 0, &pTrace, 0);
    iResult= (iResult == ESUCCESS);
    ip_trace_destroy(&pTrace);
  } else {
    iResult= icmp_ping_send_recv(pPeer->pLocalRouter->pNode,
				 0, pPeer->tAddr, 0);
    iResult= (iResult == ESUCCESS);
  }
  return iResult;
}

// ----- bgp_peer_session_refresh -----------------------------------
/**
 * Refresh the state of the BGP session. If the session is currently
 * in ESTABLISHED or OPENWAIT state, test if it is still
 * operational. If the session is in ACTIVE state, test if it must be
 * restarted.
 */
void bgp_peer_session_refresh(bgp_peer_t * pPeer)
{
  if ((pPeer->tSessionState == SESSION_STATE_ESTABLISHED) ||
      (pPeer->tSessionState == SESSION_STATE_OPENWAIT)) {

    /* Check that the peer is reachable (that is, there is a route
       towards the peer). If not, shutdown the peering. */
    if (!bgp_peer_session_ok(pPeer)) {
      assert(!bgp_peer_close_session(pPeer));
      pPeer->tSessionState= SESSION_STATE_ACTIVE;
    }

  } else if (pPeer->tSessionState == SESSION_STATE_ACTIVE) {
    
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
int bgp_peer_open_session(bgp_peer_t * pPeer)
{
  bgp_msg_t * pMsg;
  int error;

  // Check that operation is permitted in this state
  if ((pPeer->tSessionState != SESSION_STATE_IDLE) &
      (pPeer->tSessionState != SESSION_STATE_ACTIVE)) {
    LOG_ERR(LOG_LEVEL_WARNING, "Warning: session already opened\n");
    return EBGP_PEER_INVALID_STATE;
  }

  // Initialize "TCP" sequence numbers
  pPeer->uSendSeqNum= 0;
  pPeer->uRecvSeqNum= 0;

  /* Send an OPEN message to the peer (except for virtual peers) */
  if (!bgp_peer_flag_get(pPeer, PEER_FLAG_VIRTUAL)) {
    pMsg= bgp_msg_open_create(pPeer->pLocalRouter->uASN,
			      pPeer->pLocalRouter->tRouterID);
    error= bgp_peer_send(pPeer, pMsg);
    if (error == ESUCCESS) {
      pPeer->tSessionState= SESSION_STATE_OPENWAIT;
    } else {
      pPeer->tSessionState= SESSION_STATE_ACTIVE;
      pPeer->uSendSeqNum= 0;
      pPeer->uRecvSeqNum= 0;
    }
    pPeer->last_error= error;
  } else {
    
    /* For virtual peers, we check that the peer is reachable
     * through the IGP. If so, the state directly goes to
     * ESTABLISHED. Otherwise, the state goes to ACTIVE. */
    if (bgp_peer_session_ok(pPeer)) {
      pPeer->tSessionState= SESSION_STATE_ESTABLISHED;
      pPeer->tRouterID= pPeer->tAddr;
      
      /* If the virtual peer is configured with the soft-restart
	 option, scan the Adj-RIB-in and run the decision process
	 for each route. */
      if (bgp_peer_flag_get(pPeer, PEER_FLAG_SOFT_RESTART))
	_bgp_peer_rescan_adjribin(pPeer, 0);
      
      pPeer->last_error= ESUCCESS;
    } else {
      pPeer->tSessionState= SESSION_STATE_ACTIVE;
      pPeer->last_error= EBGP_PEER_UNREACHABLE;
    }
  }
  
  return ESUCCESS;
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
int bgp_peer_close_session(bgp_peer_t * pPeer)
{
  bgp_msg_t * pMsg;
  int iClear;

  // Check that operation is permitted in this state
  if (pPeer->tSessionState == SESSION_STATE_IDLE) {
    LOG_ERR(LOG_LEVEL_WARNING, "Warning: session not opened\n");
    return EBGP_PEER_INVALID_STATE;
  }

  LOG_DEBUG_ENABLED(LOG_LEVEL_DEBUG) {
    log_printf(pLogDebug, "> AS%d.peer_close_session.begin\n",
	       pPeer->pLocalRouter->uASN);
    log_printf(pLogDebug, "\tpeer: AS%d\n", pPeer->uRemoteAS);
  }

  /* If the session is in OPENWAIT or ESTABLISHED state, send a
     CLOSE message to the peer (except for virtual peers). */
  if ((pPeer->tSessionState == SESSION_STATE_OPENWAIT) ||
      (pPeer->tSessionState == SESSION_STATE_ESTABLISHED)) {
    
    if (!bgp_peer_flag_get(pPeer, PEER_FLAG_VIRTUAL)) {
      pMsg= bgp_msg_close_create(pPeer->pLocalRouter->uASN);
      bgp_peer_send(pPeer, pMsg);
    }
    
  }    
  pPeer->tSessionState= SESSION_STATE_IDLE;
  pPeer->tRouterID= 0;
  pPeer->uSendSeqNum= 0;
  pPeer->uRecvSeqNum= 0;
  
  /* For virtual peers configured with the soft-restart option, do
     not clear the Adj-RIB-in. */
  iClear= !(bgp_peer_flag_get(pPeer, PEER_FLAG_VIRTUAL) &&
	    bgp_peer_flag_get(pPeer, PEER_FLAG_SOFT_RESTART));
  _bgp_peer_rescan_adjribin(pPeer, iClear);
  
  LOG_DEBUG(LOG_LEVEL_DEBUG, "< AS%d.peer_close_session.end\n",
	    pPeer->pLocalRouter->uASN);
  
  return ESUCCESS;
}

// ----- bgp_peer_session_error -------------------------------------
/**
 * This function is used to dump information on the peering session
 * (the AS number and the IP address of the local router and the peer
 * router).
 */
void bgp_peer_session_error(bgp_peer_t * pPeer)
{
  LOG_ERR_ENABLED(LOG_LEVEL_FATAL) {
    log_printf(pLogErr, "Error: peer=");
    bgp_peer_dump_id(pLogErr, pPeer);
    log_printf(pLogErr, "\nError: router=");
    bgp_router_dump_id(pLogErr, pPeer->pLocalRouter);
    log_printf(pLogErr, "\n");
  }
}

// ----- _bgp_peer_session_open_rcvd --------------------------------
static inline void _bgp_peer_session_open_rcvd(bgp_peer_t * pPeer,
					       bgp_msg_open_t * msg)
{
  /* Check that the message does not come from a virtual peer */
  if (bgp_peer_flag_get(pPeer, PEER_FLAG_VIRTUAL)) {
    LOG_ERR(LOG_LEVEL_FATAL,
	    "Error: OPEN message received from virtual peer\n");
    bgp_peer_session_error(pPeer);
    abort();
  }

  LOG_DEBUG(LOG_LEVEL_INFO, "BGP_MSG_RCVD: OPEN\n");
  switch (pPeer->tSessionState) {
  case SESSION_STATE_ACTIVE:
    pPeer->tSessionState= SESSION_STATE_ESTABLISHED;
    bgp_peer_send(pPeer,
		  bgp_msg_open_create(pPeer->pLocalRouter->uASN,
				      pPeer->pLocalRouter->tRouterID));

    rib_for_each(pPeer->pLocalRouter->pLocRIB,
		 _bgp_peer_prefix_disseminate, pPeer);
    break;
  case SESSION_STATE_OPENWAIT:
    pPeer->tSessionState= SESSION_STATE_ESTABLISHED;
    pPeer->tRouterID= msg->router_id;
    rib_for_each(pPeer->pLocalRouter->pLocRIB,
		 _bgp_peer_prefix_disseminate, pPeer);
    break;
  default:
    LOG_ERR(LOG_LEVEL_FATAL, "Error: OPEN received while in %s state\n",
	    SESSION_STATES[pPeer->tSessionState]);
    bgp_peer_session_error(pPeer);
    abort();
  }
  LOG_DEBUG(LOG_LEVEL_DEBUG, "BGP_FSM_STATE: %s\n",
	    SESSION_STATES[pPeer->tSessionState]);
}

// ----- _bgp_peer_session_close_rcvd -------------------------------
static inline void _bgp_peer_session_close_rcvd(bgp_peer_t * pPeer)
{
  /* Check that the message does not come from a virtual peer */
  if (bgp_peer_flag_get(pPeer, PEER_FLAG_VIRTUAL)) {
    LOG_ERR(LOG_LEVEL_FATAL,
	    "Error: CLOSE message received from virtual peer\n");
    bgp_peer_session_error(pPeer);
    abort();
  }

  LOG_DEBUG(LOG_LEVEL_INFO, "BGP_MSG_RCVD: CLOSE\n");
  switch (pPeer->tSessionState) {
  case SESSION_STATE_ESTABLISHED:
  case SESSION_STATE_OPENWAIT:
    pPeer->tSessionState= SESSION_STATE_ACTIVE;
    pPeer->uSendSeqNum= 0;
    pPeer->uRecvSeqNum= 0;
    _bgp_peer_rescan_adjribin(pPeer, !bgp_peer_flag_get(pPeer, PEER_FLAG_VIRTUAL));
    break;
  case SESSION_STATE_ACTIVE:
  case SESSION_STATE_IDLE:
    break;
  default:
    LOG_ERR(LOG_LEVEL_FATAL, "Error: CLOSE received while in %s state\n",
	      SESSION_STATES[pPeer->tSessionState]);
    bgp_peer_session_error(pPeer);
    abort();    
  }
  LOG_DEBUG(LOG_LEVEL_DEBUG, "BGP_FSM_STATE: %s\n",
	    SESSION_STATES[pPeer->tSessionState]);
}

// ----- _bgp_peer_session_update_rcvd ------------------------------
static inline void _bgp_peer_session_update_rcvd(bgp_peer_t * peer,
						 bgp_msg_update_t * msg)
{
  LOG_DEBUG(LOG_LEVEL_INFO, "BGP_MSG_RCVD: UPDATE\n");
  switch (peer->tSessionState) {
  case SESSION_STATE_OPENWAIT:
    peer->tSessionState= SESSION_STATE_ESTABLISHED;
    break;
  case SESSION_STATE_ESTABLISHED:
    break;
  default:
    LOG_ERR(LOG_LEVEL_WARNING,
	    "Warning: UPDATE received while in %s state\n",
	    SESSION_STATES[peer->tSessionState]);
    bgp_peer_session_error(peer);
    abort();
  }
  LOG_DEBUG(LOG_LEVEL_DEBUG, "BGP_FSM_STATE: %s\n",
	    SESSION_STATES[peer->tSessionState]);

  /* Process UPDATE message */
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  _bgp_peer_process_update_walton(peer, msg);
#else
  _bgp_peer_process_update(peer, msg);
#endif

}

// ----- _bgp_peer_session_withdraw_rcvd ----------------------------
static inline void _bgp_peer_session_withdraw_rcvd(bgp_peer_t * peer,
						   bgp_msg_withdraw_t * msg)
{
  LOG_DEBUG(LOG_LEVEL_INFO, "BGP_MSG_RCVD: WITHDRAW\n");
  switch (peer->tSessionState) {
  case SESSION_STATE_OPENWAIT:
    peer->tSessionState= SESSION_STATE_ESTABLISHED;
    break;
  case SESSION_STATE_ESTABLISHED:
    break;
  default:
    LOG_ERR(LOG_LEVEL_FATAL, "Error: WITHDRAW received while in %s state\n",
	    SESSION_STATES[peer->tSessionState]);
    bgp_peer_session_error(peer);
    abort();    
  }
  LOG_DEBUG(LOG_LEVEL_DEBUG, "BGP_FSM_STATE: %s\n",
	    SESSION_STATES[peer->tSessionState]);

  /* Process WITHDRAW message */
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  _bgp_peer_process_withdraw_walton(peer,  msg);
#else
  _bgp_peer_process_withdraw(peer, msg);
#endif

}

// ----- _bgp_peer_disable_adjribin ---------------------------------
/**
 * TODO: write documentation...
 */
static int _bgp_peer_disable_adjribin(uint32_t uKey, uint8_t uKeyLen,
				      void * pItem, void * pContext)
{
  bgp_route_t * pRoute= (bgp_route_t *) pItem;
  bgp_peer_t * pPeer= (bgp_peer_t *) pContext;

  //route_flag_set(pRoute, ROUTE_FLAG_ELIGIBLE, 0);

  /* Since the ROUTE_FLAG_BEST is handled in the Adj-RIB-In, we only
     need to run the decision process if the route is installed in the
     Loc-RIB (i.e. marked as best) */
  if (route_flag_get(pRoute, ROUTE_FLAG_BEST)) {

    LOG_DEBUG_ENABLED(LOG_LEVEL_DEBUG) {
      log_printf(pLogDebug, "\trescan: ", pPeer->pLocalRouter->uASN);
      route_dump(pLogDebug, pRoute);
      log_printf(pLogDebug, "\n");
    }
    bgp_router_decision_process(pPeer->pLocalRouter, pPeer, pRoute->sPrefix);

  }

  return 0;
}

// ----- _bgp_peer_enable_adjribin ----------------------------------
/**
 * TODO: write documentation...
 */
static int _bgp_peer_enable_adjribin(uint32_t uKey, uint8_t uKeyLen,
				     void * pItem, void * pContext)
{
  bgp_route_t * pRoute= (bgp_route_t *) pItem;
  bgp_peer_t * pPeer= (bgp_peer_t *) pContext;

  //route_flag_set(pRoute, ROUTE_FLAG_ELIGIBLE, 0);

  bgp_router_decision_process(pPeer->pLocalRouter, pPeer, pRoute->sPrefix);

  return 0;
}

// ----- _bgp_peer_adjrib_clear -------------------------------------
/**
 * TODO: write documentation...
 */
static void _bgp_peer_adjrib_clear(bgp_peer_t * pPeer, bgp_rib_dir_t dir)
{
  rib_destroy(&pPeer->pAdjRIB[dir]);
  pPeer->pAdjRIB[dir]= rib_create(0);
}

// ----- _bgp_peer_rescan_adjribin ----------------------------------
/**
 * TODO: write documentation...
 */
static inline void _bgp_peer_rescan_adjribin(bgp_peer_t * pPeer, int iClear)
{
  if (pPeer->tSessionState == SESSION_STATE_ESTABLISHED) {

    rib_for_each(pPeer->pAdjRIB[RIB_IN], _bgp_peer_enable_adjribin,
		 pPeer);

  } else {

    // For each route in Adj-RIB-In, mark as unfeasible
    // and run decision process for each route marked as best
    rib_for_each(pPeer->pAdjRIB[RIB_IN], _bgp_peer_disable_adjribin,
		 pPeer);
    
    // Clear Adj-RIB-In ?
    if (iClear)
      _bgp_peer_adjrib_clear(pPeer, 1);

  }

}

/////////////////////////////////////////////////////////////////////
//
// BGP FILTERS
//
/////////////////////////////////////////////////////////////////////

// -----[ bgp_peer_set_filter ]----------------------------------------
/**
 * Change a filter of this peer. The previous filter is destroyed.
 */
void bgp_peer_set_filter(bgp_peer_t * pPeer, bgp_filter_dir_t dir,
			 bgp_filter_t * pFilter)
{
  if (pPeer->pFilter[dir] != NULL)
    filter_destroy(&pPeer->pFilter[dir]);
  pPeer->pFilter[dir]= pFilter;
}

// -----[ bgp_peer_filter_get ]--------------------------------------
/**
 * Return a filter of this peer.
 */
bgp_filter_t * bgp_peer_filter_get(bgp_peer_t * pPeer,
				   bgp_filter_dir_t dir)
{
  return pPeer->pFilter[dir];
}


/////////////////////////////////////////////////////////////////////
//
// BGP MESSAGE HANDLING
//
/////////////////////////////////////////////////////////////////////

// ----- bgp_peer_announce_route ------------------------------------
/**
 * Announce the given route to the given peer.
 *
 * PRE: the route must be a copy that can live its own life,
 * i.e. there must not be references to the route.
 *
 * Note: if the route is not delivered (for instance in the case of
 * a virtual peer), the given route is freed.
 */
void bgp_peer_announce_route(bgp_peer_t * pPeer, bgp_route_t * pRoute)
{
  bgp_msg_t * pMsg;

  LOG_DEBUG_ENABLED(LOG_LEVEL_DEBUG) {
    log_printf(pLogDebug, "announce_route to ");
    bgp_peer_dump_id(pLogDebug, pPeer);
    log_printf(pLogDebug, "\n");
  }

  route_peer_set(pRoute, pPeer);
  pMsg= bgp_msg_update_create(pPeer->pLocalRouter->uASN, pRoute);

  /* Send the message to the peer */
  if (bgp_peer_send(pPeer, pMsg) < 0)
    route_destroy(&pRoute);
}

// ----- bgp_peer_withdraw_prefix -----------------------------------
/**
 * Withdraw the given prefix to the given peer.
 *
 * Note: withdraws are not sent to virtual peers.
 */
void bgp_peer_withdraw_prefix(bgp_peer_t * pPeer, SPrefix sPrefix)
{
  // Send the message to the peer (except if this is a virtual peer)
  if (!bgp_peer_flag_get(pPeer, PEER_FLAG_VIRTUAL)) {
    bgp_peer_send(pPeer,
		  bgp_msg_withdraw_create(pPeer->pLocalRouter->uASN,
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
void peer_route_delay_update(bgp_peer_t * pPeer, bgp_route_t * pRoute)
{
  net_iface_t * iface;

  LOG_ERR(LOG_LEVEL_FATAL,
	  "Error: peer_route_delay_update MUST be modified\n"
	  "Error: to support route-reflection !!!");
  abort();

  /*pLink= node_find_link_to_router(pPeer->pLocalRouter->pNode,
    pRoute->tNextHop);*/
  assert(iface != NULL);

#ifdef BGP_QOS
  qos_route_update_delay(pRoute, iface->tDelay);
#endif
}

// ----- peer_route_rr_client_update --------------------------------
/**
 * Store in the route a flag indicating whether the route has been
 * learned from an RR client or not. This is used to improve the speed
 * of the redistribution step of the BGP decision process [as.c,
 * bgp_router_advertise_to_peer]
 */
void peer_route_rr_client_update(bgp_peer_t * pPeer, bgp_route_t * pRoute)
{
  route_flag_set(pRoute, ROUTE_FLAG_RR_CLIENT,
		 bgp_peer_flag_get(pPeer, PEER_FLAG_RR_CLIENT));
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
int peer_comm_process(bgp_route_t * pRoute)
{
  int iIndex;

  /* Classical communities */
  if (pRoute->pAttr->pCommunities != NULL) {
#ifdef __EXPERIMENTAL__
    if (route_comm_contains(pRoute, COMM_DEPREF)) {
      route_localpref_set(pRoute, 0);
    }
#endif
  }

  /* Extended communities */
  if (pRoute->pAttr->pECommunities != NULL) {

    for (iIndex= 0; iIndex < ecomm_length(pRoute->pAttr->pECommunities); iIndex++) {
      SECommunity * pComm= ecomm_get_index(pRoute->pAttr->pECommunities, iIndex);
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

// ----- bgp_peer_route_eligible ------------------------------------
/**
 * The route is eligible if it passes the input filters. Standard
 * filters are applied first. Then, extended communities actions are
 * taken if any (see 'peer_ecomm_process').
 */
int bgp_peer_route_eligible(bgp_peer_t * pPeer, bgp_route_t * pRoute)
{
  // Check that the route's AS-path does not contain the local AS
  // number.
  if (route_path_contains(pRoute, pPeer->pLocalRouter->uASN)) {
    LOG_DEBUG(LOG_LEVEL_DEBUG, "in-filtered(as-path loop)\n");
    return 0;
  }

  // Route-Reflection: deny routes with an originator-ID equal to local
  // router-ID [RFC2796, section7].
  if ((pRoute->pAttr->pOriginator != NULL) &&
      (*pRoute->pAttr->pOriginator == pPeer->pLocalRouter->tRouterID)) {
    LOG_DEBUG(LOG_LEVEL_DEBUG, "in-filtered(RR: originator-id)\n");
    return 0;
  }

  // Route-Reflection: Avoid cluster-loop creation (MUST be done
  // before local cluster-ID is appended) [RFC2796, section7].
  if ((pRoute->pAttr->pClusterList != NULL) &&
      (route_cluster_list_contains(pRoute, pPeer->pLocalRouter->tClusterID))) {
    LOG_DEBUG(LOG_LEVEL_DEBUG, "in-filtered(RR: cluster-loop)\n");
    return 0;
  }
  
  // Apply the input filters.
  if (!filter_apply(pPeer->pFilter[FILTER_IN], pPeer->pLocalRouter, pRoute)) {
    LOG_DEBUG(LOG_LEVEL_DEBUG, "in-filtered(filter)\n");
    return 0;
  }

  // Process communities.
  if (!peer_comm_process(pRoute)) {
    LOG_DEBUG(LOG_LEVEL_DEBUG, "in-filtered(community)\n");
    return 0;
  }

  return 1;
}

// ----- bgp_peer_route_feasible ------------------------------------
/**
 * The route is feasible if and only if the next-hop is reachable
 * (through a STATIC, IGP or BGP route).
 */
int bgp_peer_route_feasible(bgp_peer_t * self, bgp_route_t * pRoute)
{
  const rt_entry_t * rtentry= node_rt_lookup(self->pLocalRouter->pNode,
					     pRoute->pAttr->tNextHop);
  return (rtentry != NULL);
}

// -----[ _bgp_peer_process_update ]----------------------------------
/**
 * Process a BGP UPDATE message.
 *
 * The operation is as follows:
 * 1). If a route towards the same prefix already exists it is
 *     replaced by the new one. Otherwise the new route is added.
 * 2). Tag the route as received from this peer.
 * 3). Clear the LOCAL-PREF is the session is external (eBGP)
 * 4). Test if the route is feasible (next-hop reachable) and flag
 *     it accordingly.
 * 5). The decision process is run.
 */
static inline void _bgp_peer_process_update(bgp_peer_t * peer,
					    bgp_msg_update_t * msg)
{
  bgp_route_t * pRoute= msg->pRoute;
  bgp_route_t * pOldRoute= NULL;
  SPrefix sPrefix;
  int need_DP_run;

  LOG_DEBUG_ENABLED(LOG_LEVEL_DEBUG) {
    log_printf(pLogDebug, "\tupdate: ");
    route_dump(pLogDebug, pRoute);
    log_printf(pLogDebug, "\n");
  }

  // Tag the route as received from this peer
  route_peer_set(pRoute, peer);

  // If route learned over eBGP, clear LOCAL-PREF
  if (peer->uRemoteAS != peer->pLocalRouter->uASN)
    route_localpref_set(pRoute, BGP_OPTIONS_DEFAULT_LOCAL_PREF);

  // Check route against import filter
  route_flag_set(pRoute, ROUTE_FLAG_BEST, 0);
  route_flag_set(pRoute, ROUTE_FLAG_INTERNAL, 0);
  route_flag_set(pRoute, ROUTE_FLAG_ELIGIBLE,
		 bgp_peer_route_eligible(peer, pRoute));
  route_flag_set(pRoute, ROUTE_FLAG_FEASIBLE,
		 bgp_peer_route_feasible(peer, pRoute));
  
#ifdef __EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
    route_flag_set(pRoute, ROUTE_FLAG_EXTERNAL_BEST, 0);
#endif

  // Update route delay attribute (if BGP-QoS)
  //peer_route_delay_update(pPeer, pRoute);

  // Update route RR-client flag
  peer_route_rr_client_update(peer, pRoute);

  // The decision process need only be run in the following cases:
  // - the old route was the best
  // - the new route is eligible
  need_DP_run= 0;
  pOldRoute= rib_find_exact(peer->pAdjRIB[RIB_IN], pRoute->sPrefix);
  if (((pOldRoute != NULL) &&
       route_flag_get(pOldRoute, ROUTE_FLAG_BEST)) ||
      route_flag_get(pRoute, ROUTE_FLAG_ELIGIBLE)) 
    need_DP_run= 1;

  sPrefix= pRoute->sPrefix;
  
  // Replace former route in Adj-RIB-In
  if (route_flag_get(pRoute, ROUTE_FLAG_ELIGIBLE)) {
    assert(rib_replace_route(peer->pAdjRIB[RIB_IN], pRoute) == 0);
  } else {
    if (pOldRoute != NULL)
      assert(rib_remove_route(peer->pAdjRIB[RIB_IN], pRoute->sPrefix) == 0);
    route_destroy(&pRoute);
  }
  
  // Run decision process for this route
  if (need_DP_run)
    bgp_router_decision_process(peer->pLocalRouter, peer, sPrefix);
}

// -----[ _bgp_peer_process_withdraw ]--------------------------------
/**
 * Process a BGP WITHDRAW message.
 *
 * The operation is as follows:
 * 1). If a route towards the withdrawn prefix already exists, it is
 *     set as 'uneligible'
 * 2). The decision process is ran for the destination prefix.
 * 3). The old route is removed from the Adj-RIB-In.
 */
static inline void _bgp_peer_process_withdraw(bgp_peer_t * peer,
					      bgp_msg_withdraw_t * msg)
{
  bgp_route_t * pRoute;
  
  // Identifiy route to be removed based on destination prefix
  pRoute= rib_find_exact(peer->pAdjRIB[RIB_IN], msg->sPrefix);
  
  // If there was no previous route, do nothing
  // Note: we should probably trigger an error/warning message in this case
  if (pRoute == NULL)
    return;

  // Flag the route as un-eligible
  route_flag_set(pRoute, ROUTE_FLAG_ELIGIBLE, 0);

  // Run decision process in case this route is the best route
  // towards this prefix
  bgp_router_decision_process(peer->pLocalRouter, peer,
			      msg->sPrefix);
  
  LOG_DEBUG_ENABLED(LOG_LEVEL_DEBUG) {
    log_printf(pLogDebug, "\tremove: ");
    route_dump(pLogDebug, pRoute);
    log_printf(pLogDebug, "\n");
  }
  
  assert(rib_remove_route(peer->pAdjRIB[RIB_IN], msg->sPrefix) == 0);
}


// -----[ _bgp_peer_seqnum_check ]-----------------------------------
/**
 * Check BGP message sequence number. The sequence number of the
 * received message must be equal to the recv-sequence-number stored
 * in the peering state. If this condition is not true, the program
 * is aborted.
 *
 * There are two exceptions:
 *   - this check is not performed for messages received from a
 *     virtual peering
 *   
 */
static inline void _bgp_peer_seqnum_check(bgp_peer_t * peer,
					  bgp_msg_t * msg)
{
  if ((bgp_peer_flag_get(peer, PEER_FLAG_VIRTUAL) == 0) &&
      (peer->uRecvSeqNum != msg->uSeqNum)) {
    if (msg->type != BGP_MSG_TYPE_CLOSE) {
      bgp_peer_dump_id(pLogErr, peer);
      log_printf(pLogErr, ": out-of-sequence BGP message (%d)", msg->uSeqNum);
      log_printf(pLogErr, " - expected (%d)", peer->uRecvSeqNum);
      log_printf(pLogErr,  " [state=%s]\n",
		 SESSION_STATES[peer->tSessionState]);
      bgp_msg_dump(pLogErr, peer->pLocalRouter->pNode, msg);
      log_printf(pLogErr, "\n");
      
      log_printf(pLogErr, "\n");
      log_printf(pLogErr, "It is likely that this error occurs due to an "
		 "incorrect setup of the simulation. The most common case "
		 "for this error is when the underlying route of a BGP "
		 "session changes during the simulation convergence (sim "
		 "run). This might also occur with multi-hop eBGP sessions "
		 "where the session is routed over another BGP session.");
      log_printf(pLogErr, "\n");
      abort();
    }
  }
}

// ----- bgp_peer_handle_message ------------------------------------
/**
 * Handle received BGP messages (OPEN, CLOSE, UPDATE, WITHDRAW).
 * Check the sequence number of incoming messages.
 */
int bgp_peer_handle_message(bgp_peer_t * peer, bgp_msg_t * msg)
{
  LOG_DEBUG_ENABLED(LOG_LEVEL_DEBUG) {
    log_printf(pLogDebug, "HANDLE_MESSAGE from ");
    bgp_peer_dump_id(pLogDebug, peer);
    log_printf(pLogDebug, " in ");
    bgp_router_dump_id(pLogDebug, peer->pLocalRouter);
    log_printf(pLogDebug, "\n");
  }

  _bgp_peer_seqnum_check(peer, msg);
  peer->uRecvSeqNum++;

  switch (msg->type) {
  case BGP_MSG_TYPE_UPDATE:
    _bgp_peer_session_update_rcvd(peer, (bgp_msg_update_t *) msg);
    break;

  case BGP_MSG_TYPE_WITHDRAW:
    _bgp_peer_session_withdraw_rcvd(peer, (bgp_msg_withdraw_t *) msg);
    break;

  case BGP_MSG_TYPE_CLOSE:
    _bgp_peer_session_close_rcvd(peer); break;

  case BGP_MSG_TYPE_OPEN:
    _bgp_peer_session_open_rcvd(peer, (bgp_msg_open_t *) msg); break;

  default:
    bgp_peer_session_error(peer);
    LOG_ERR(LOG_LEVEL_FATAL, "Unknown BGP message type received (%d)\n",
	    msg->type);
    abort();
  }
  bgp_msg_destroy(&msg);
  return 0;
}

// -----[ bgp_peer_send ]--------------------------------------------
/**
 * Send a BGP message to the given peer. If activated, this function
 * will tap BGP messages and record them to a file (see
 * 'pRecordStream').
 *
 * Note: if the peer is virtual, the message will be discarded and the
 * function will return an error.
 */
int bgp_peer_send(bgp_peer_t * peer, bgp_msg_t * msg)
{
  msg->uSeqNum= peer->uSendSeqNum++;
  
  // Record BGP messages (optional)
  if (peer->pRecordStream != NULL) {
    bgp_msg_dump(peer->pRecordStream, NULL, msg);
    log_printf(peer->pRecordStream, "\n");
    log_flush(peer->pRecordStream);
  }

  // Send the message
  if (!bgp_peer_flag_get(peer, PEER_FLAG_VIRTUAL)) {
    return bgp_msg_send(peer->pLocalRouter->pNode,
			peer->tSrcAddr,
			peer->tAddr, msg);
  } else {
    bgp_msg_destroy(&msg);
    return EBGP_PEER_INCOMPATIBLE;
  }
}

// -----[ bgp_peer_send_enabled ]------------------------------------
/**
 * Tell if this peer can send BGP messages. Basically, all non-virtual
 * peers are able to send BGP messages. However, if the record
 * functionnality is activated on a virtual peer, this peer will
 * appear as if it was able to send messages. This is needed in order
 * to tap BGP messages sent to virtual peers.
 *
 * Return values:
 *      0 if the peer cannot send messages
 *   != 0 otherwise
 */
int bgp_peer_send_enabled(bgp_peer_t * peer)
{
  return (!bgp_peer_flag_get(peer, PEER_FLAG_VIRTUAL) ||
	  (peer->pRecordStream != NULL));
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
void bgp_peer_dump_id(SLogStream * pStream, bgp_peer_t * pPeer)
{
  log_printf(pStream, "AS%d:", pPeer->uRemoteAS);
  ip_address_dump(pStream, pPeer->tAddr);
}

// ----- bgp_peer_dump ----------------------------------------------
/**
 *
 */
void bgp_peer_dump(SLogStream * pStream, bgp_peer_t * pPeer)
{
  int iOptions= 0;

  ip_address_dump(pStream, pPeer->tAddr);
  log_printf(pStream, "\tAS%d\t%s\t", pPeer->uRemoteAS,
	  SESSION_STATES[pPeer->tSessionState]);
  ip_address_dump(pStream, pPeer->tRouterID);
  if (bgp_peer_flag_get(pPeer, PEER_FLAG_RR_CLIENT)) {
    log_printf(pStream, (iOptions++)?",":"\t(");
    log_printf(pStream, "rr-client");
  }
  if (bgp_peer_flag_get(pPeer, PEER_FLAG_NEXT_HOP_SELF)) {
    log_printf(pStream, (iOptions++)?",":"\t(");
    log_printf(pStream, "next-hop-self");
  }
  if (bgp_peer_flag_get(pPeer, PEER_FLAG_VIRTUAL)) {
    log_printf(pStream, (iOptions++)?",":"\t(");
    log_printf(pStream, "virtual");
  }
  if (bgp_peer_flag_get(pPeer, PEER_FLAG_SOFT_RESTART)) {
    log_printf(pStream, (iOptions++)?",":"\t(");
    log_printf(pStream, "soft-restart");
  }
  log_printf(pStream, (iOptions)?")":"");

  // Session state
  if (pPeer->tSessionState == SESSION_STATE_ESTABLISHED) {
    if (!bgp_peer_session_ok(pPeer)) {
      log_printf(pStream, "\t[DOWN]");
    }
  }

  log_printf(pStream, "\tsnd-seq:%d\trcv-seq:%d", pPeer->uSendSeqNum,
	     pPeer->uRecvSeqNum);
}

typedef struct {
  bgp_peer_t * pPeer;
  SLogStream * pStream;
} bgp_route_tDumpCtx;

// ----- bgp_peer_dump_route ----------------------------------------
/**
 *
 */
int bgp_peer_dump_route(uint32_t uKey, uint8_t uKeyLen,
			void * pItem, void * pContext)
{
  bgp_route_tDumpCtx * pCtx= (bgp_route_tDumpCtx *) pContext;

  route_dump(pCtx->pStream, (bgp_route_t *) pItem);
  log_printf(pCtx->pStream, "\n");
  return 0;
}

// ----- bgp_peer_dump_adjrib ---------------------------------------
/**
 * Dump Adj-RIB-In/Out.
 *
 * Parameters:
 * - iInOut, if 1, dump Adj-RIB-In, otherwize, dump Adj-RIB-Out.
 */
void bgp_peer_dump_adjrib(SLogStream * pStream, bgp_peer_t * pPeer,
			  SPrefix sPrefix, bgp_rib_dir_t dir)
{
  bgp_route_tDumpCtx sCtx;
  bgp_route_t * pRoute;
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  bgp_route_ts * pRoutes;
  uint16_t uIndexRoute;
#endif

  sCtx.pStream= pStream;
  sCtx.pPeer= pPeer;

  if (sPrefix.uMaskLen == 0) {
    rib_for_each(pPeer->pAdjRIB[dir], bgp_peer_dump_route, &sCtx);
  } else if (sPrefix.uMaskLen >= 32) {
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
    pRoutes = rib_find_best(pPeer->pAdjRIB[dir], sPrefix);
    if (pRoutes != NULL) {
      for (uIndexRoute = 0; uIndexRoute < routes_list_get_num(pRoutes); uIndexRoute++) {
	pRoute = routes_list_get_at(pRoutes, uIndexRoute);
#else
    pRoute= rib_find_best(pPeer->pAdjRIB[dir], sPrefix);
#endif
    if (pRoute != NULL) {
      route_dump(pStream, pRoute);
      log_printf(pStream, "\n");
    }
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
      }
    }
#endif
  } else {
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
    pRoutes = rib_find_exact(pPeer->pAdjRIB[dir], sPrefix);
    if (pRoutes != NULL) {
      for (uIndexRoute = 0; uIndexRoute < routes_list_get_num(pRoutes); uIndexRoute++) {
	pRoute = routes_list_get_at(pRoutes, uIndexRoute);
#else
    pRoute= rib_find_exact(pPeer->pAdjRIB[dir], sPrefix);
#endif
    if (pRoute != NULL) {
      route_dump(pStream, pRoute);
      log_printf(pStream, "\n");
    }
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
      }
    }
#endif
  }
}

// ----- bgp_peer_dump_filters -----------------------------------
/**
 *
 */
void bgp_peer_dump_filters(SLogStream * pStream, bgp_peer_t * pPeer,
			   bgp_filter_dir_t dir)
{
  filter_dump(pStream, pPeer->pFilter[dir]);
}

// -----[ bgp_peer_set_record_stream ]-------------------------------
/**
 * Set a stream for recording the messages sent to this neighbor. If
 * the given stream is NULL, the recording will be stopped.
 */
int bgp_peer_set_record_stream(bgp_peer_t * pPeer, SLogStream * pStream)
{
  if (pPeer->pRecordStream != NULL) {
    log_destroy(&(pPeer->pRecordStream));
    pPeer->pRecordStream= NULL;
  }
  if (pStream != NULL)
    pPeer->pRecordStream= pStream;
  return 0;
}

/////////////////////////////////////////////////////////////////////
//
// WALTON DRAFT (EXPERIMENTAL)
//
/////////////////////////////////////////////////////////////////////

#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__

// -----[ _bgp_peer_process_update_walton ]----------------------------------
/**
 * Process a BGP UPDATE message.
 *
 * The operation is as follows:
 * 1). If a route towards the same prefix already exists it is
 *     replaced by the new one. Otherwise the new route is added.
 * 2). Tag the route as received from this peer.
 * 3). Clear the LOCAL-PREF is the session is external (eBGP)
 * 4). Test if the route is feasible (next-hop reachable) and flag
 *     it accordingly.
 * 5). The decision process is run.
 */
static void _bgp_peer_process_update_walton(bgp_peer_t * pPeer,
					    bgp_msg_update_t * msg)
{
  bgp_route_t * pRoute= msg->pRoute;
  bgp_route_t * pOldRoute= NULL;
  SPrefix sPrefix;
  int iNeedDecisionProcess;
  bgp_route_ts * pOldRoutes;
  uint16_t uIndexRoute;
  net_addr_t tNextHop;

  LOG_DEBUG_ENABLED(LOG_LEVEL_DEBUG) {
    log_printf(pLogDebug, "\tupdate: ");
    route_dump(pLogDebug, pRoute);
    log_printf(pLogDebug, "\n");
  }

  // Tag the route as received from this peer
  route_peer_set(pRoute, peer);

  // If route learned over eBGP, clear LOCAL-PREF
  if (peer->uRemoteAS != peer->pLocalRouter->uASN)
    route_localpref_set(pRoute, BGP_OPTIONS_DEFAULT_LOCAL_PREF);

  // Check route against import filter
  route_flag_set(pRoute, ROUTE_FLAG_BEST, 0);
  route_flag_set(pRoute, ROUTE_FLAG_INTERNAL, 0);
  route_flag_set(pRoute, ROUTE_FLAG_ELIGIBLE,
		 bgp_peer_route_eligible(peer, pRoute));
  route_flag_set(pRoute, ROUTE_FLAG_FEASIBLE,
		 bgp_peer_route_feasible(peer, pRoute));
  
#ifdef __EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
    route_flag_set(pRoute, ROUTE_FLAG_EXTERNAL_BEST, 0);
#endif

  // Update route RR-client flag
  peer_route_rr_client_update(peer, pRoute);

  // The decision process need only be run in the following cases:
  // - the old route was the best
  // - the new route is eligible
  iNeedDecisionProcess= 0;
  route_flag_set(pRoute, ROUTE_FLAG_BEST, 0);
  pOldRoutes = rib_find_exact(peer->pAdjRIB[RIB_IN], pRoute->sPrefix);
  if (pOldRoutes != NULL) {
    for (uIndexRoute = 0; uIndexRoute < routes_list_get_num(pOldRoutes);
	 uIndexRoute++) {
      pOldRoute = routes_list_get_at(pOldRoutes, uIndexRoute);
      LOG_DEBUG_ENABLED(LOG_LEVEL_DEBUG) {
	log_printf(pLogDebug, "\tupdate: ");
	route_dump(pLogDebug, pOldRoute);
	log_printf(pLogDebug, "\n");
      }
      if ((pOldRoute != NULL) &&
	  route_flag_get(pOldRoute, ROUTE_FLAG_BEST)) {
	route_flag_set(pOldRoute, ROUTE_FLAG_BEST, 0);
	iNeedDecisionProcess= 1;
	break;
      }
    }
  } else if (route_flag_get(pRoute, ROUTE_FLAG_ELIGIBLE)) {
    iNeedDecisionProcess = 1;
  }

  sPrefix= pRoute->sPrefix;
  
  // Replace former route in Adj-RIB-In
  if (route_flag_get(pRoute, ROUTE_FLAG_ELIGIBLE)) {
    assert(rib_replace_route(peer->pAdjRIB[RIB_IN], pRoute) == 0);
  } else {
    if (pOldRoute != NULL) {
      tNextHop = route_get_nexthop(pRoute);
      assert(rib_remove_route(peer->pAdjRIB[RIB_IN], pRoute->sPrefix, &tNextHop)
	     == 0);
    }
    route_destroy(&pRoute);
  }
  
  // Run decision process for this route
  if (iNeedDecisionProcess)
    bgp_router_decision_process(peer->pLocalRouter, peer, sPrefix);
}

// -----[ _bgp_peer_process_withdraw_walton ]------------------------
/**
 * Process a BGP WITHDRAW message (Walton draft).
 *
 * The operation is as follows:
 * 1). If a route towards the withdrawn prefix and with the specified
 *     next-hop already exists, it is set as 'uneligible'
 * 2). The decision process is ran for the destination prefix.
 * 3). The old route is removed from the Adj-RIB-In.
 */
static void _bgp_peer_process_walton_withdraw(bgp_peer_t * peer,
					      bgp_msg_withdraw_t * msg)
{
  bgp_route_t * pRoute;
  net_addr_t tNextHop;

  //LOG_DEBUG("walton-withdraw: next-hop=");
  //LOG_ENABLED_DEBUG() ip_address_dump(log_get_stream(pMainLog),
  //      			      pMsgWithdraw->tNextHop);
  //LOG_DEBUG("\n");


  // Identifiy route to be removed based on destination prefix and next-hop
  pRoute= rib_find_one_exact(peer->pAdjRIB[RIB_IN], msg->sPrefix,
			     msg->tNextHop);

  // If there was no previous route, do nothing
  // Note: we should probably trigger an error/warning message in this case
  if (pRoute == NULL)
    return;
  
  // Flag the route as un-eligible
  route_flag_set(pRoute, ROUTE_FLAG_ELIGIBLE, 0);

  // Run decision process in case this route is the best route
  // towards this prefix
  bgp_router_decision_process(peer->pLocalRouter, peer,
			      msg->sPrefix);
  
  LOG_DEBUG_ENABLED(LOG_LEVEL_DEBUG) {
    log_printf(pLogDebug, "\tremove: ");
    route_dump(pLogDebug, pRoute);
    log_printf(pLogDebug, "\n");
  }

  assert(rib_remove_route(peer->pAdjRIB[RIB_IN], msg->sPrefix,
			  msg->tNextHop) == 0);
}

void bgp_peer_withdraw_prefix_walton(bgp_peer_t * peer,
				     SPrefix sPrefix,
				     net_addr_t * tNextHop)
{
  // Send the message to the peer (except if this is a virtual peer)
  if (!bgp_peer_flag_get(peer, PEER_FLAG_VIRTUAL))
    bgp_peer_send(peer,
		  bgp_msg_withdraw_create(peer->pLocalRouter->uASN,
					  sPrefix, tNextHop));
}

// -----[ bgp_peer_set_walton_limit ]--------------------------------
void bgp_peer_set_walton_limit(bgp_peer_t * peer, unsigned int uWaltonLimit)
{
  //TODO : check the MAX_VALUE !
  peer->uWaltonLimit = uWaltonLimit;
  bgp_router_walton_peer_set(peer, uWaltonLimit);
}

// -----[ bgp_peer_get_walton_limit ]--------------------------------
uint16_t bgp_peer_get_walton_limit(bgp_peer_t * pPeer)
{
  return pPeer->uWaltonLimit;
}

#endif

