// ==================================================================
// @(#)peer.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 24/11/2002
// @lastdate 15/11/2005
// ==================================================================

#ifndef __PEER_H__
#define __PEER_H__

#include <bgp/as_t.h>
#include <bgp/filter.h>
#include <bgp/message.h>
#include <bgp/peer_t.h>
#include <bgp/route.h>

struct TPeer {
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
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  uint16_t uWaltonLimit;
#endif
};

// ----- bgp_peer_create --------------------------------------------
extern SPeer * bgp_peer_create(uint16_t uRemoteAS,
			       net_addr_t tAddr,
			       SBGPRouter * pLocalRouter,
			       uint8_t uPeerType);
// ----- bgp_peer_destroy -------------------------------------------
extern void bgp_peer_destroy(SPeer ** ppPeer);

// ----- bgp_peer_flag_set ------------------------------------------
extern void bgp_peer_flag_set(SPeer * pPeer, uint8_t uFlag, int iState);
// ----- bgp_peer_flag_get ------------------------------------------
extern int bgp_peer_flag_get(SPeer * pPeer, uint8_t uFlag);
// ----- bgp_peer_set_nexthop ---------------------------------------
extern int bgp_peer_set_nexthop(SBGPPeer * pPeer, net_addr_t tNextHop);
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
void peer_set_walton_limit(SPeer * pPeer, unsigned int uWaltonLimit);
uint16_t peer_get_walton_limit(SPeer * pPeer);
#endif
/////////////////////////////////////////////////////////////////////
// BGP FILTERS
/////////////////////////////////////////////////////////////////////

// ----- peer_set_in_filter -----------------------------------------
extern void peer_set_in_filter(SPeer * pPeer, SFilter * pFilter);
// ----- peer_in_filter_get -----------------------------------------
extern SFilter * peer_in_filter_get(SPeer * pPeer);
// ----- peer_set_out_filter ----------------------------------------
extern void peer_set_out_filter(SPeer * pPeer, SFilter * pFilter);
// ----- peer_out_filter_get ----------------------------------------
extern SFilter * peer_out_filter_get(SPeer * pPeer);

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
extern void peer_rescan_adjribin(SPeer * pPeer, int iClear);

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

// ----- bgp_peer_route_eligible ------------------------------------
extern int bgp_peer_route_eligible(SPeer * pPeer, SRoute * pRoute);
// ----- bgp_peer_route_feasible ------------------------------------
extern int bgp_peer_route_feasible(SPeer * pPeer, SRoute * pRoute);


/////////////////////////////////////////////////////////////////////
// INFORMATION RETRIEVAL
/////////////////////////////////////////////////////////////////////

// ----- bgp_peer_dump_id -------------------------------------------
extern void bgp_peer_dump_id(FILE * pStream, SBGPPeer * pPeer);
// ----- bgp_peer_dump ----------------------------------------------
extern void bgp_peer_dump(FILE * pStream, SBGPPeer * pPeer);
// ----- bgp_peer_dump_adjrib ---------------------------------------
extern void bgp_peer_dump_adjrib(FILE * pStream, SBGPPeer * pPeer,
				 SPrefix sPrefix, int iInOut);
// ----- bgp_peer_dump_in_filters -----------------------------------
extern void bgp_peer_dump_in_filters(FILE * pStream, SBGPPeer * pPeer);
// ----- bgp_peer_dump_out_filters ----------------------------------
extern void bgp_peer_dump_out_filters(FILE * pStream, SBGPPeer * pPeer);

#endif
