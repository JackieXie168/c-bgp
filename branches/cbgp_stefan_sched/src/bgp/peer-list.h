// ==================================================================
// @(#)peer-list.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 10/03/2008
// $Id: peer-list.h,v 1.2 2009-03-24 14:28:25 bqu Exp $
// ==================================================================

#ifndef __BGP_PEER_LIST_H__
#define __BGP_PEER_LIST_H__

#include <libgds/array.h>
#include <libgds/enumerator.h>

#include <bgp/types.h>

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ bgp_peers_create ]---------------------------------------
  bgp_peers_t * bgp_peers_create();
  // -----[ bgp_peers_destroy ]--------------------------------------
  void bgp_peers_destroy(bgp_peers_t ** peers);
  // -----[ bgp_peers_add ]------------------------------------------
  net_error_t bgp_peers_add(bgp_peers_t * peers, bgp_peer_t * peer);
  // -----[ bgp_peers_find ]-----------------------------------------
  bgp_peer_t * bgp_peers_find(bgp_peers_t * peers, net_addr_t addr);
  // -----[ bgp_peers_for_each ]-------------------------------------
  int bgp_peers_for_each(bgp_peers_t * peers, gds_array_foreach_f foreach,
			 void * ctx);
  // -----[ bgp_peers_enum ]-----------------------------------------
  gds_enum_t * bgp_peers_enum(bgp_peers_t * peers);

#ifdef __cplusplus
}
#endif


// -----[ bgp_peers_at ]---------------------------------------------
 bgp_peer_t * bgp_peers_at(bgp_peers_t * peers,
					unsigned int index) ;
 unsigned int bgp_peers_size(bgp_peers_t * peers) ;


#endif /* __BGP_PEER_LIST_H__ */
