// ==================================================================
// @(#)peer-list.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 10/03/2008
// $Id: peer-list.h,v 1.1 2008-04-07 09:23:14 bqu Exp $
// ==================================================================

#ifndef __BGP_PEER_LIST_H__
#define __BGP_PEER_LIST_H__

#include <libgds/array.h>
#include <libgds/enumerator.h>

#include <bgp/peer_t.h>

typedef SPtrArray bgp_peers_t;

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ bgp_peers_create ]---------------------------------------
  bgp_peers_t * bgp_peers_create();
  // -----[ bgp_peers_destroy ]--------------------------------------
  void bgp_peers_destroy(bgp_peers_t ** ppPeers);
  // -----[ bgp_peers_add ]------------------------------------------
  net_error_t bgp_peers_add(bgp_peers_t * pPeers, bgp_peer_t * pPeer);
  // -----[ bgp_peers_find ]-----------------------------------------
  bgp_peer_t * bgp_peers_find(bgp_peers_t * pPeers, net_addr_t tAddr);
  // -----[ bgp_peers_for_each ]-------------------------------------
  int bgp_peers_for_each(bgp_peers_t * pPeers, FArrayForEach fForEach,
			 void * pContext);
  // -----[ bgp_peers_enum ]-----------------------------------------
  enum_t * bgp_peers_enum(bgp_peers_t * pPeers);
  // -----[ bgp_peers_at ]-------------------------------------------
  static inline bgp_peer_t * bgp_peers_at(bgp_peers_t * pPeers,
					  unsigned int uIndex) {
    return (bgp_peer_t *) pPeers->data[uIndex];
  }
  // -----[ bgp_peers_size ]-----------------------------------------
  static inline unsigned int bgp_peers_size(bgp_peers_t * pPeers) {
    return ptr_array_length(pPeers);
  }

#ifdef __cplusplus
}
#endif

#endif /* __BGP_PEER_LIST_H__ */
