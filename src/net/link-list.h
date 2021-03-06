// ==================================================================
// @(#)link-list.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 05/08/2003
// $Id: link-list.h,v 1.9 2009-03-24 16:14:59 bqu Exp $
// ==================================================================

#ifndef __NET_LINK_LIST_H__
#define __NET_LINK_LIST_H__

#include <libgds/array.h>
#include <libgds/enumerator.h>
#include <libgds/stream.h>

#include <net/iface.h>
#include <net/net_types.h>

#ifdef __cplusplus
extern "C" {
#endif

  // ----- net_links_create -----------------------------------------
  net_ifaces_t * net_links_create();
  // ----- net_links_destroy ----------------------------------------
  void net_links_destroy(net_ifaces_t ** links_ref);
  // ----- net_links_add --------------------------------------------
  net_error_t net_links_add(net_ifaces_t * links, net_iface_t * link);
  // ----- net_links_dump -------------------------------------------
  void net_links_dump(gds_stream_t * stream, net_ifaces_t * links);
  // -----[ net_ifaces_get_smallest ]--------------------------------
  int net_ifaces_get_smallest(net_ifaces_t * ifaces, net_addr_t * addr);
  // -----[ net_ifaces_get_highest ]-----------------------------------
  int net_ifaces_get_highest(net_ifaces_t * ifaces, net_addr_t * addr);

  // -----[ net_links_find ]-----------------------------------------
  net_iface_t * net_links_find(net_ifaces_t * links,
			       net_iface_id_t tIfaceID);

  // ----- net_links_get_enum ---------------------------------------
  gds_enum_t * net_links_get_enum(net_ifaces_t * links);
  // -----[ net_links_find_iface ]-------------------------------------
  net_iface_t * net_links_find_iface(net_ifaces_t * links,
				     net_addr_t addr);

#ifdef __cplusplus
}
#endif

// -----[ net_ifaces_size ]------------------------------------------
static inline unsigned int net_ifaces_size(net_ifaces_t * ifaces) {
  return ptr_array_length(ifaces);
}
// -----[ net_ifaces_at ]--------------------------------------------
static inline net_iface_t * net_ifaces_at(net_ifaces_t * ifaces,
					  unsigned int index) {
  return ifaces->data[index];
}

#endif /*  __NET_LINK_LIST_H__ */
