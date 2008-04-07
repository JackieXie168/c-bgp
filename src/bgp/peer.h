// ==================================================================
// @(#)peer.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @auhtor Sebastien Tandel (standel@info.ucl.ac.be)
// 
// @date 24/11/2002
// @lastdate 27/02/2008
// ==================================================================

#ifndef __PEER_H__
#define __PEER_H__

#include <libgds/log.h>

#include <bgp/as_t.h>
#include <bgp/filter.h>
#include <bgp/message.h>
#include <bgp/peer_t.h>
#include <bgp/route.h>

struct TBGPPeer {
  net_addr_t tAddr;           // IP address that is used to reach the neighbor
  net_addr_t tSrcAddr;        // Source IP address
  net_addr_t tRouterID;       // ROUTER-ID of the neighbor. The
			      // ROUTER-ID is initialy 0. It is set
			      // when an OPEN message is received. For
			      // virtual peers it is set to the peer's
			      // IP address when the session is
			      // opened.
  uint16_t uRemoteAS;         // AS-number of the neighbor.
  bgp_router_t * pLocalRouter;  // Reference to the local router.
  bgp_filter_t * pFilter[FILTER_MAX]; // Input and output filters
  bgp_rib_t    * pAdjRIB[RIB_MAX];    // Input and output Adj-RIBs
  bgp_peer_state_t tSessionState; // Session state (handled by the FSM)
  
  uint8_t uFlags;             // Configuration flags
  net_addr_t tNextHop;        // BGP next-hop to advertise to this
			      // peer.
  SLogStream * pRecordStream;
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  uint16_t uWaltonLimit;
#endif
  unsigned int uSendSeqNum;
  unsigned int uRecvSeqNum;

  int last_error;
};

#ifdef __cplusplus
extern "C" {
#endif

  // ----- bgp_peer_create ------------------------------------------
  bgp_peer_t * bgp_peer_create(uint16_t uRemoteAS,
			     net_addr_t tAddr,
			     bgp_router_t * pLocalRouter);
  // ----- bgp_peer_destroy -----------------------------------------
  void bgp_peer_destroy(bgp_peer_t ** ppPeer);

  // ----- bgp_peer_flag_set ----------------------------------------
  void bgp_peer_flag_set(bgp_peer_t * pPeer, uint8_t uFlag, int iState);
  // ----- bgp_peer_flag_get ----------------------------------------
  int bgp_peer_flag_get(bgp_peer_t * pPeer, uint8_t uFlag);
  // ----- bgp_peer_set_nexthop -------------------------------------
  int bgp_peer_set_nexthop(bgp_peer_t * pPeer, net_addr_t tNextHop);
  // -----[ bgp_peer_set_source ]------------------------------------
  int bgp_peer_set_source(bgp_peer_t * pPeer, net_addr_t tSrcAddr);

  ///////////////////////////////////////////////////////////////////
  // BGP FILTERS
  ///////////////////////////////////////////////////////////////////
  
  // -----[ bgp_peer_set_filter ]------------------------------------
  void bgp_peer_set_filter(bgp_peer_t * pPeer, bgp_filter_dir_t dir,
			   bgp_filter_t * pFilter);
  // -----[ bgp_peer_filter_get ]------------------------------------
  bgp_filter_t * bgp_peer_filter_get(bgp_peer_t * pPeer,
				     bgp_filter_dir_t dir);

  ///////////////////////////////////////////////////////////////////
  // BGP SESSION MANAGEMENT FUNCTIONS
  ///////////////////////////////////////////////////////////////////

  // ----- bgp_peer_session_ok --------------------------------------
  int bgp_peer_session_ok(bgp_peer_t * pPeer);
  // ----- bgp_peer_session_refresh ---------------------------------
  void bgp_peer_session_refresh(bgp_peer_t * pPeer);
  // ----- peer_open_session ----------------------------------------
  int bgp_peer_open_session(bgp_peer_t * pPeer);
  // ----- peer_close_session ---------------------------------------
  int bgp_peer_close_session(bgp_peer_t * pPeer);
  // ----- peer_rescan_adjribin -------------------------------------
  void peer_rescan_adjribin(bgp_peer_t * pPeer, int iClear);

  ///////////////////////////////////////////////////////////////////
  // BGP MESSAGE HANDLING
  ///////////////////////////////////////////////////////////////////

  // ----- bgp_peer_announce_route ----------------------------------
  void bgp_peer_announce_route(bgp_peer_t * pPeer, SRoute * pRoute);
  // ----- bgp_peer_withdraw_prefix ---------------------------------
  void bgp_peer_withdraw_prefix(bgp_peer_t * pPeer, SPrefix sPrefix);
  // ----- bgp_peer_handle_message ----------------------------------
  int bgp_peer_handle_message(bgp_peer_t * pPeer, SBGPMsg * pMsg);
  // -----[ bgp_peer_send ]------------------------------------------
  int bgp_peer_send(bgp_peer_t * pPeer, SBGPMsg * pMsg);
  
  // ----- bgp_peer_route_eligible ----------------------------------
  int bgp_peer_route_eligible(bgp_peer_t * pPeer, SRoute * pRoute);
  // ----- bgp_peer_route_feasible ----------------------------------
  int bgp_peer_route_feasible(bgp_peer_t * pPeer, SRoute * pRoute);


  ///////////////////////////////////////////////////////////////////
  // INFORMATION RETRIEVAL
  ///////////////////////////////////////////////////////////////////
  
  // ----- bgp_peer_dump_id -----------------------------------------
  void bgp_peer_dump_id(SLogStream * pStream, bgp_peer_t * pPeer);
  // ----- bgp_peer_dump --------------------------------------------
  void bgp_peer_dump(SLogStream * pStream, bgp_peer_t * pPeer);
  // ----- bgp_peer_dump_adjrib -------------------------------------
  void bgp_peer_dump_adjrib(SLogStream * pStream, bgp_peer_t * pPeer,
				   SPrefix sPrefix, bgp_rib_dir_t dir);
  // ----- bgp_peer_dump_filters ---------------------------------
  void bgp_peer_dump_filters(SLogStream * pStream, bgp_peer_t * pPeer,
			     bgp_filter_dir_t dir);
  // -----[ bgp_peer_set_record_stream ]-----------------------------
  int bgp_peer_set_record_stream(bgp_peer_t * pPeer,
				 SLogStream * pStream);
  // -----[ bgp_peer_send_enabled ]----------------------------------
  int bgp_peer_send_enabled(bgp_peer_t * pPeer);


  ///////////////////////////////////////////////////////////////////
  // WALTON DRAFT (EXPERIMENTAL)
  ///////////////////////////////////////////////////////////////////
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  void bgp_peer_withdraw_prefix_walton(bgp_peer_t * pPeer, SPrefix sPrefix,
				       net_addr_t * tNextHop);
  // -----[ bgp_peer_set_walton_limit ]------------------------------
  void bgp_peer_set_walton_limit(bgp_peer_t * pPeer, unsigned int uWaltonLimit);
  // -----[ bgp_peer_get_walton_limit ]------------------------------
  uint16_t bgp_peer_get_walton_limit(bgp_peer_t * pPeer);
#endif


#ifdef __cplusplus
}
#endif

#endif
