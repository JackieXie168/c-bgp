// ==================================================================
// @(#)link-list.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 05/08/2003
// @lastdate 13/04/2007
// ==================================================================

#ifndef __NET_LINK_LIST_H__
#define __NET_LINK_LIST_H__

#include <libgds/array.h>
#include <libgds/enumerator.h>
#include <libgds/log.h>
#include <net/net_types.h>

#ifdef __cplusplus
extern "C" {
#endif

  // ----- net_links_create -----------------------------------------
  SNetLinks * net_links_create();
  // ----- net_links_destroy ----------------------------------------
  void net_links_destroy(SNetLinks ** ppLinks);
  // ----- net_links_add --------------------------------------------
  int net_links_add(SNetLinks * pLinks, SNetLink * pLink);
  // ----- net_links_dump -------------------------------------------
  void net_links_dump(SLogStream * pStream, SNetLinks * pLinks);
  // ----- net_links_get_smaller_iface ------------------------------
  net_addr_t net_links_get_smaller_iface(SNetLinks * pLinks);
  // ----- net_links_find_ptp ---------------------------------------
  SNetLink * net_links_find_ptp(SNetLinks * pLinks, net_addr_t tDstAddr);
  // ----- net_links_find_mtp ---------------------------------------
  SNetLink * net_links_find_mtp(SNetLinks * pLinks, net_addr_t tIfaceAddr,
				net_mask_t tIfaceMask);
  // ----- net_links_get_enum ---------------------------------------
  SEnumerator * net_links_get_enum(SNetLinks * pLinks);
  // -----[ net_links_find_iface ]-------------------------------------
  SNetLink * net_links_find_iface(SNetLinks * pLinks, net_addr_t tIfaceAddr);

#ifdef __cplusplus
}
#endif

#endif /*  __NET_LINK_LIST_H__ */
