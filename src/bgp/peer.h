// ==================================================================
// @(#)peer.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 24/11/2002
// @lastdate 05/03/2004
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
  SAS * pLocalAS;
  SFilter * pInFilter;
  SFilter * pOutFilter;
  SRIB * pAdjRIBIn;
  SRIB * pAdjRIBOut;
  uint8_t uSessionState;
  uint8_t uFlags;
};

// ----- peer_create ------------------------------------------------
extern SPeer * peer_create(uint16_t uRemoteAS,
			   net_addr_t tAddr,
			   SAS * pLocalAS, uint8_t uPeerType);
// ----- peer_destroy -----------------------------------------------
extern void peer_destroy(SPeer ** ppPeer);

// ----- peer_flag_set ----------------------------------------------
extern void peer_flag_set(SPeer * pPeer, uint8_t uFlag, int iState);
// ----- peer_flag_get ----------------------------------------------
extern int peer_flag_get(SPeer * pPeer, uint8_t uFlag);
// ----- peer_set_in_filter -----------------------------------------
extern void peer_set_in_filter(SPeer * pPeer, SFilter * pFilter);
// ----- peer_in_filter_get -----------------------------------------
extern SFilter * peer_in_filter_get(SPeer * pPeer);
// ----- peer_set_out_filter ----------------------------------------
extern void peer_set_out_filter(SPeer * pPeer, SFilter * pFilter);
// ----- peer_out_filter_get ----------------------------------------
extern SFilter * peer_out_filter_get(SPeer * pPeer);

// ----- peer_open_session ------------------------------------------
extern int peer_open_session(SPeer * pPeer);
// ----- peer_close_session -----------------------------------------
extern int peer_close_session(SPeer * pPeer);
// ----- peer_clear_adjribin ----------------------------------------
extern void peer_clear_adjribin(SPeer * pPeer);

// ----- peer_announce_route ----------------------------------------
extern void peer_announce_route(SPeer * pPeer, SRoute * pRoute);
// ----- peer_withdraw_prefix ---------------------------------------
extern void peer_withdraw_prefix(SPeer * pPeer, SPrefix sPrefix);
// ----- peer_handle_message ----------------------------------------
extern int peer_handle_message(SPeer * pPeer, SBGPMsg * pMsg);
// ----- peer_dump_id -----------------------------------------------
extern void peer_dump_id(FILE * pStream, SPeer * pPeer);
// ----- bgp_peer_dump ----------------------------------------------
extern void bgp_peer_dump(FILE * pStream, SPeer * pPeer);
// ----- bgp_peer_dump_ribin ----------------------------------------
extern void bgp_peer_dump_ribin(FILE * pStream, SPeer * pPeer,
				SPrefix sPrefix);
// ----- bgp_peer_dump_in_filters -----------------------------------
extern void bgp_peer_dump_in_filters(FILE * pStream, SPeer * pPeer);
// ----- bgp_peer_dump_out_filters ----------------------------------
extern void bgp_peer_dump_out_filters(FILE * pStream, SPeer * pPeer);

#endif
