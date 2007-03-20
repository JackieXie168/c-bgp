// ===========================================================
// @(#)link.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be),
// @author Stefano Iasi (stefanoia@tin.it)
// @date 24/02/2004
// @lastdate 23/01/2007
// ===========================================================

#ifndef __NET_LINK_H__
#define __NET_LINK_H__

#include <stdlib.h>
#include <net/net_types.h>
#include <net/message.h>
#include <net/prefix.h>
#include <libgds/types.h>
#include <libgds/array.h>
#include <libgds/log.h>
#include <libgds/memory.h>

#define NET_LINK_FLAG_UP      0x01
#define NET_LINK_FLAG_NOTIFY  0x02
#define NET_LINK_FLAG_IGP_ADV 0x04
#define NET_LINK_FLAG_TUNNEL  0x08
#define NET_LINK_FLAG_PREFIX  0x10
#define NET_LINK_FLAG_IFACE   0x20
// Link type distinguishes between Router, Transit Network and
// Stub Network to compute IGP route in OSPF
#define NET_LINK_TYPE_ROUTER   0
#define NET_LINK_TYPE_TRANSIT  1
#define NET_LINK_TYPE_STUB     2
#define NET_LINK_TYPE_ANY      2

#define NET_LINK_MAX_DEPTH     10
#define NET_LINK_DEFAULT_DEPTH 1

#define NET_LINK_UNIDIR        0
#define NET_LINK_BIDIR         1

//#define link_is_to_node(L)           (L->uDestinationType == NET_LINK_TYPE_ROUTER)
//#define link_belongs_to_area(L, A)   ((L)->tArea == A)

#ifdef __cplusplus
extern "C" {
#endif

  // ----- net_link_create_ptp --------------------------------------
  int net_link_create_ptp(SNetNode * pSrcNode, SNetNode * pDstNode,
			  net_link_delay_t tDelay,
			  net_link_load_t tCapacity,
			  uint8_t tDepth, SNetLink ** ppLink);
  // ----- net_link_create_ptp2 -------------------------------------
  int net_link_create_ptp2(SNetNode * pSrcNode, net_addr_t tDstAddr,
			   net_link_delay_t tDelay,
			   net_link_load_t tCapacity,
			   uint8_t tDepth, SNetLink ** ppLink);
  // ----- net_link_create_mtp --------------------------------------
  SNetLink * net_link_create_mtp(SNetNode * pSrcNode, SNetSubnet * pSubnet,
				 net_addr_t tIfaceAddr,
				 net_link_delay_t tdelay,
				 net_link_load_t tCapacity,
				 uint8_t tDepth);
  // ----- net_link_destroy -----------------------------------------
  void net_link_destroy(SNetLink ** ppLink);

  // ----- net_link_get_id ------------------------------------------
  SPrefix net_link_get_id(SNetLink * pLink);
  // ----- link_get_iface -------------------------------------------
  net_addr_t link_get_iface(SNetLink * pLink);
  // ----- link_get_address -----------------------------------------
  net_addr_t link_get_address(SNetLink * pLink);
  // ----- net_link_get_prefix --------------------------------------
  SPrefix net_link_get_prefix(SNetLink * pLink);
  // ----- link_get_subnet ------------------------------------------
  SNetSubnet * link_get_subnet(SNetLink * pLink);
  // ----- link_set_type --------------------------------------------
  void link_set_type(SNetLink * pLink, uint8_t uType);
  // ----- link_get_type --------------------------------------------
  int link_check_type(SNetLink * pLink, uint8_t uType);

  // ----- link_to_router_has_ip_prefix -----------------------------
  /* COMMENT ON 19/01/2007
     int link_to_router_has_only_iface(SNetLink * pLink);*/
  // ----- link_to_router_has_ip_prefix -----------------------------
  /* COMMENT ON 19/01/2007
     int link_to_router_has_ip_prefix(SNetLink * pLink);*/
  // ----- link_get_ip_prefix ---------------------------------------
  /* COMMENT ON 19/01/2007
     SPrefix link_get_ip_prefix(SNetLink * pLink);*/


  ///////////////////////////////////////////////////////////////////
  // LINK ATTRIBUTES
  ///////////////////////////////////////////////////////////////////

  // ----- net_link_get_weight --------------------------------------
  net_igp_weight_t net_link_get_weight(SNetLink * pLink, net_tos_t tTOS);
  // ----- net_link_set_weight --------------------------------------
  void net_link_set_weight(SNetLink * pLink, net_tos_t tTOS,
			   net_igp_weight_t tWeight);
  // ----- net_link_get_depth ---------------------------------------
  unsigned int net_link_get_depth(SNetLink * pLink);
  // ----- net_link_get_delay ---------------------------------------
  net_link_delay_t net_link_get_delay(SNetLink * pLink);
  // ----- net_link_set_delay ---------------------------------------
  void net_link_set_delay(SNetLink * pLink, net_link_delay_t tDelay);
  // ----- net_link_set_state ---------------------------------------
  void net_link_set_state(SNetLink * pLink, uint8_t uFlag, int iState);
  // ----- net_link_get_state ---------------------------------------
  int net_link_get_state(SNetLink * pLink, uint8_t uFlag);
  // ----- net_link_get_capacity ------------------------------------
  net_link_load_t net_link_get_capacity(SNetLink * pLink);
  // ----- net_link_set_load ----------------------------------------
  void net_link_set_load(SNetLink * pLink, net_link_load_t tLoad);
  // ----- net_link_add_load ----------------------------------------
  void net_link_add_load(SNetLink * pLink, net_link_load_t tIncLoad);
  // ----- net_link_get_load ----------------------------------------
  net_link_load_t net_link_get_load(SNetLink * pLink);


  ///////////////////////////////////////////////////////////////////
  // LINK DUMP
  ///////////////////////////////////////////////////////////////////

  // ----- link_dst_dump --------------------------------------------
  void link_dst_dump(SLogStream * pStream, SNetLink * pLink);
  // ----- net_link_dump --------------------------------------------
  void net_link_dump(SLogStream * pStream, SNetLink * pLink);
  // ----- net_link_dump_load -----------------------------------------
  void net_link_dump_load(SLogStream * pStream, SNetLink * pLink);
  // ----- net_link_dump_info ---------------------------------------
  void net_link_dump_info(SLogStream * pStream, SNetLink * pLink);


  ///////////////////////////////////////////////////////////////////
  // LINK OSPF ATTRIBUTES
  ///////////////////////////////////////////////////////////////////

#ifdef OSPF_SUPPORT
  /* COMMENT 15/01/2007
  // ----- link_find_backword ---------------------------------------
  SNetLink * link_find_backword(SNetLink * pLink);
  */
  /* COMMAND 15/01/2007
  // ----- link_ospf_set_area ---------------------------------------
  void link_ospf_set_area(SNetLink * pLink, ospf_area_t tArea);
  */
#endif  

#ifdef __cplusplus
}
#endif

#endif /* __NET_LINK_H__ */
