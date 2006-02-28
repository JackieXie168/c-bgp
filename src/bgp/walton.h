// ==================================================================
// @(#)walton.h
//
// @author Sebastien Tandel (standel@info.ucl.ac.be)
// @date 15/02/2006
// @lastdate 28/02/2006
// ==================================================================

#ifndef __WALTON_H__
#define __WALTON_H__

#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

//#include <libgds/types.h>
#include <libgds/array.h>

#include <bgp/as.h>
#include <bgp/peer.h>

// ----- bgp_router_walton_init --------------------------------------
void bgp_router_walton_init(SBGPRouter * pRouter);
// ----- bgp_router_walton_finalize ----------------------------------
void bgp_router_walton_finalize(SBGPRouter * pRouter);
// ---- bgp_router_walton_peer_remove --------------------------------
void bgp_router_walton_peer_remove(SBGPRouter * pRouter, SPeer * pPeer);
// ----- bgp_router_walton_unsynchronized_all ------------------------
void bgp_router_walton_unsynchronized_all(SBGPRouter * pRouter);
// ----- bgp_router_walton_peer_set ----------------------------------
int bgp_router_walton_peer_set(SPeer * pPeer, unsigned int uWaltonLimit);
// ----- bgp_router_walton_dp_disseminate ----------------------------
void bgp_router_walton_dp_disseminate(SBGPRouter * pRouter, 
					SPtrArray * pPeers,
					SPrefix sPrefix,
					SRoutes * pRoutes);
// ---- bgp_router_walton_disseminate_select_peers -------------------
void bgp_router_walton_disseminate_select_peers(SBGPRouter * pRouter, 
					    SRoutes * pRoutes, 
					    uint16_t iNextHopCount);
#endif // EXPERIMENTAL_WALTON

#endif //__WALTON_H__
