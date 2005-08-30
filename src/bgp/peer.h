// ==================================================================
// @(#)peer.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 24/11/2002
// @lastdate 05/08/2005
// ==================================================================

#ifndef __PEER_H__
#define __PEER_H__

#include <bgp/as_t.h>
#include <bgp/filter.h>
#include <bgp/message.h>
#include <bgp/peer_t.h>
#include <bgp/route.h>

struct TPeer {
  net_addr_t tAddr;
  uint16_t uRemoteAS;
  uint8_t uPeerType;
  SBGPRouter * pLocalRouter;
  SFilter * pInFilter;
  SFilter * pOutFilter;
  SRIB * pAdjRIBIn;
  SRIB * pAdjRIBOut;
  uint8_t uSessionState;
  uint8_t uFlags;
  net_addr_t tNextHop;
};

// ----- peer_create ------------------------------------------------
extern SPeer * peer_create(uint16_t uRemoteAS,
			   net_addr_t tAddr,
			   SBGPRouter * pLocalRouter,
			   uint8_t uPeerType);
// ----- peer_destroy -----------------------------------------------
extern void peer_destroy(SPeer ** ppPeer);

// ----- peer_flag_set ----------------------------------------------
extern void peer_flag_set(SPeer * pPeer, uint8_t uFlag, int iState);
// ----- peer_flag_get ----------------------------------------------
extern int peer_flag_get(SPeer * pPeer, uint8_t uFlag);
// ----- peer_set_nexthop -------------------------------------------
extern int bgp_peer_set_nexthop(SBGPPeer * pPeer, net_addr_t tNextHop);

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

// ----- peer_announce_route ----------------------------------------
extern void peer_announce_route(SPeer * pPeer, SRoute * pRoute);
// ----- peer_withdraw_prefix ---------------------------------------
extern void peer_withdraw_prefix(SPeer * pPeer, SPrefix sPrefix);
// ----- peer_handle_message ----------------------------------------
extern int peer_handle_message(SPeer * pPeer, SBGPMsg * pMsg);

/////////////////////////////////////////////////////////////////////
// INFORMATION RETRIEVAL
/////////////////////////////////////////////////////////////////////

// ----- bgp_peer_dump_id -------------------------------------------
extern void bgp_peer_dump_id(FILE * pStream, SBGPPeer * pPeer);
// ----- bgp_peer_dump ----------------------------------------------
extern void bgp_peer_dump(FILE * pStream, SPeer * pPeer);
// ----- bgp_peer_dump_adjrib ---------------------------------------
extern void bgp_peer_dump_adjrib(FILE * pStream, SPeer * pPeer,
				 SPrefix sPrefix, int iInOut);
// ----- bgp_peer_dump_in_filters -----------------------------------
extern void bgp_peer_dump_in_filters(FILE * pStream, SPeer * pPeer);
// ----- bgp_peer_dump_out_filters ----------------------------------
extern void bgp_peer_dump_out_filters(FILE * pStream, SPeer * pPeer);

#endif
