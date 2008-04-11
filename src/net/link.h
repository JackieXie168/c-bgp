// ===========================================================
// @(#)link.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be),
// @author Stefano Iasi (stefanoia@tin.it)
// @date 24/02/2004
// $Id: link.h,v 1.21 2008-04-11 11:03:06 bqu Exp $
// ===========================================================

#ifndef __NET_LINK_H__
#define __NET_LINK_H__

#include <stdlib.h>

#include <net/iface.h>
#include <net/net_types.h>
#include <net/message.h>
#include <net/prefix.h>
#include <libgds/types.h>
#include <libgds/array.h>
#include <libgds/log.h>
#include <libgds/memory.h>

#define NET_LINK_FLAG_UP      0x01

#define NET_LINK_MAX_DEPTH     10
#define NET_LINK_DEFAULT_DEPTH 1

#define NET_LINK_UNIDIR        0
#define NET_LINK_BIDIR         1

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ net_link_create_rtr ]------------------------------------
  int net_link_create_rtr(net_node_t * pSrcNode, net_node_t * pDstNode,
			  net_iface_dir_t dir, net_iface_t ** ppIface);
  // -----[ net_link_create_ptp ]------------------------------------
  int net_link_create_ptp(net_node_t * pSrcNode,
			  net_iface_id_t tSrcIfaceID,
			  net_node_t * pDstNode,
			  net_iface_id_t tDstIfaceID,
			  net_iface_dir_t dir,
			  net_iface_t ** ppIface);
  // -----[ net_link_create_ptmp ]-----------------------------------
  int net_link_create_ptmp(net_node_t * pSrcNode, net_subnet_t * pSubnet,
			   net_addr_t tIfaceAddr, net_iface_t ** ppIface);
  // -----[ net_link_set_phys_attr ]---------------------------------
  int net_link_set_phys_attr(net_iface_t * pIface, net_link_delay_t tDelay,
			     net_link_load_t tCapacity,
			     net_iface_dir_t dir);
  // ----- net_link_destroy -----------------------------------------
  void net_link_destroy(net_iface_t ** ppLink);


  ///////////////////////////////////////////////////////////////////
  // LINK DUMP
  ///////////////////////////////////////////////////////////////////

  // ----- net_link_dump --------------------------------------------
  void net_link_dump(SLogStream * pStream, net_iface_t * pLink);
  // ----- net_link_dump_load ---------------------------------------
  void net_link_dump_load(SLogStream * pStream, net_iface_t * pLink);
  // ----- net_link_dump_info ---------------------------------------
  void net_link_dump_info(SLogStream * pStream, net_iface_t * pLink);


#ifdef __cplusplus
}
#endif

#endif /* __NET_LINK_H__ */
