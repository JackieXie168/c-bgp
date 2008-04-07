// ==================================================================
// @(#)link-list.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 05/08/2003
// @lastdate 11/03/2008
// ==================================================================

#ifndef __NET_LINK_LIST_H__
#define __NET_LINK_LIST_H__

#include <libgds/array.h>
#include <libgds/enumerator.h>
#include <libgds/log.h>

#include <net/iface.h>
#include <net/net_types.h>

#ifdef __cplusplus
extern "C" {
#endif

  // ----- net_links_create -----------------------------------------
  net_ifaces_t * net_links_create();
  // ----- net_links_destroy ----------------------------------------
  void net_links_destroy(net_ifaces_t ** ppLinks);
  // ----- net_links_add --------------------------------------------
  net_error_t net_links_add(net_ifaces_t * pLinks, net_iface_t * pLink);
  // ----- net_links_dump -------------------------------------------
  void net_links_dump(SLogStream * pStream, net_ifaces_t * pLinks);
  // ----- net_links_get_smaller_iface ------------------------------
  net_addr_t net_links_get_smaller_iface(net_ifaces_t * pLinks);

  // -----[ net_links_find ]-----------------------------------------
  net_iface_t * net_links_find(net_ifaces_t * pLinks,
			       net_iface_id_t tIfaceID);

  // ----- net_links_get_enum ---------------------------------------
  enum_t * net_links_get_enum(net_ifaces_t * pLinks);
  // -----[ net_links_find_iface ]-------------------------------------
  net_iface_t * net_links_find_iface(net_ifaces_t * pLinks,
				     net_addr_t tIfaceAddr);

  // -----[ net_ifaces_size ]----------------------------------------
  static inline unsigned int net_ifaces_size(net_ifaces_t * pIfaces) {
    return ptr_array_length(pIfaces);
  }
  // -----[ net_ifaces_at ]------------------------------------------
  static inline net_iface_t * net_ifaces_at(net_ifaces_t * pIfaces,
					    unsigned int uIndex) {
    return pIfaces->data[uIndex];
  }

#ifdef __cplusplus
}
#endif

#endif /*  __NET_LINK_LIST_H__ */
