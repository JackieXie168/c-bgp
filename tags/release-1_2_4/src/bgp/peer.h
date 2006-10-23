// ==================================================================
// @(#)peer.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @auhtor Sebastien Tandel (standel@info.ucl.ac.be)
// 
// @date 24/11/2002
// @lastdate 11/09/2006
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
  net_addr_t tRouterID;       // ROUTER-ID of the neighbor. The
			      // ROUTER-ID is initialy 0. It is set
			      // when an OPEN message is received. For
			      // virtual peers is it set to the peer's
			      // IP address when the session is
			      // opened.
  uint16_t uRemoteAS;         // AS-number of the neighbor.
  uint8_t uPeerType;
  SBGPRouter * pLocalRouter;  // Reference to the local router.
  SFilter * pInFilter;        // Input and output filters
  SFilter * pOutFilter;
  SRIB * pAdjRIBIn;           // Input and output Adj-RIB
  SRIB * pAdjRIBOut;
  uint8_t uSessionState;      // Session state (handled by the FSM)
  uint8_t uFlags;             // Configuration flags
  net_addr_t tNextHop;        // BGP next-hop to advertise to this
			      // peer.
  SLogStream * pRecordStream;
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  uint16_t uWaltonLimit;
#endif
};

// ----- bgp_peer_create --------------------------------------------
extern SBGPPeer * bgp_peer_create(uint16_t uRemoteAS,
				  net_addr_t tAddr,
				  SBGPRouter * pLocalRouter,
				  uint8_t uPeerType);
// ----- bgp_peer_destroy -------------------------------------------
extern void bgp_peer_destroy(SBGPPeer ** ppPeer);

// ----- bgp_peer_flag_set ------------------------------------------
extern void bgp_peer_flag_set(SBGPPeer * pPeer, uint8_t uFlag, int iState);
// ----- bgp_peer_flag_get ------------------------------------------
extern int bgp_peer_flag_get(SBGPPeer * pPeer, uint8_t uFlag);
// ----- bgp_peer_set_nexthop ---------------------------------------
extern int bgp_peer_set_nexthop(SBGPPeer * pPeer, net_addr_t tNextHop);
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
void peer_set_walton_limit(SBGPPeer * pPeer, unsigned int uWaltonLimit);
uint16_t peer_get_walton_limit(SBGPPeer * pPeer);
#endif
/////////////////////////////////////////////////////////////////////
// BGP FILTERS
/////////////////////////////////////////////////////////////////////

// ----- bgp_peer_set_in_filter -------------------------------------
extern void bgp_peer_set_in_filter(SBGPPeer * pPeer, SFilter * pFilter);
// ----- bgp_peer_in_filter_get -------------------------------------
extern SFilter * bgp_peer_in_filter_get(SBGPPeer * pPeer);
// ----- bgp_peer_set_out_filter ------------------------------------
extern void bgp_peer_set_out_filter(SBGPPeer * pPeer, SFilter * pFilter);
// ----- bgp_peer_out_filter_get ------------------------------------
extern SFilter * bgp_peer_out_filter_get(SBGPPeer * pPeer);

/////////////////////////////////////////////////////////////////////
// BGP SESSION MANAGEMENT FUNCTIONS
/////////////////////////////////////////////////////////////////////

// ----- bgp_peer_session_ok ----------------------------------------
extern int bgp_peer_session_ok(SBGPPeer * pPeer);
// ----- bgp_peer_session_refresh -----------------------------------
extern void bgp_peer_session_refresh(SBGPPeer * pPeer);
// ----- peer_open_session ------------------------------------------
extern int bgp_peer_open_session(SBGPPeer * pPeer);
// ----- peer_close_session -----------------------------------------
extern int bgp_peer_close_session(SBGPPeer * pPeer);
// ----- peer_rescan_adjribin ---------------------------------------
extern void peer_rescan_adjribin(SBGPPeer * pPeer, int iClear);

/////////////////////////////////////////////////////////////////////
// BGP MESSAGE HANDLING
/////////////////////////////////////////////////////////////////////

// ----- bgp_peer_announce_route ------------------------------------
extern void bgp_peer_announce_route(SBGPPeer * pPeer, SRoute * pRoute);
// ----- bgp_peer_withdraw_prefix -----------------------------------
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
extern void bgp_peer_withdraw_prefix(SBGPPeer * pPeer, SPrefix sPrefix,
				      net_addr_t * tNextHop);

#else
extern void bgp_peer_withdraw_prefix(SBGPPeer * pPeer, SPrefix sPrefix);
#endif
// ----- bgp_peer_handle_message ------------------------------------
extern int bgp_peer_handle_message(SBGPPeer * pPeer, SBGPMsg * pMsg);
// -----[ bgp_peer_send ]--------------------------------------------
extern int bgp_peer_send(SBGPPeer * pPeer, SBGPMsg * pMsg);

// ----- bgp_peer_route_eligible ------------------------------------
extern int bgp_peer_route_eligible(SBGPPeer * pPeer, SRoute * pRoute);
// ----- bgp_peer_route_feasible ------------------------------------
extern int bgp_peer_route_feasible(SBGPPeer * pPeer, SRoute * pRoute);


/////////////////////////////////////////////////////////////////////
// INFORMATION RETRIEVAL
/////////////////////////////////////////////////////////////////////

// ----- bgp_peer_dump_id -------------------------------------------
extern void bgp_peer_dump_id(SLogStream * pStream, SBGPPeer * pPeer);
// ----- bgp_peer_dump ----------------------------------------------
extern void bgp_peer_dump(SLogStream * pStream, SBGPPeer * pPeer);
// ----- bgp_peer_dump_adjrib ---------------------------------------
extern void bgp_peer_dump_adjrib(SLogStream * pStream, SBGPPeer * pPeer,
				 SPrefix sPrefix, int iInOut);
// ----- bgp_peer_dump_in_filters -----------------------------------
extern void bgp_peer_dump_in_filters(SLogStream * pStream, SBGPPeer * pPeer);
// ----- bgp_peer_dump_out_filters ----------------------------------
extern void bgp_peer_dump_out_filters(SLogStream * pStream, SBGPPeer * pPeer);

// -----[ bgp_peer_set_record_stream ]-------------------------------
extern int bgp_peer_set_record_stream(SBGPPeer * pPeer,
				      SLogStream * pStream);
// -----[ bgp_peer_send_enabled ]------------------------------------
extern int bgp_peer_send_enabled(SBGPPeer * pPeer);

#endif
