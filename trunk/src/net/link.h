// ===========================================================
// @(#)link.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be),
// @author Stefano Iasi (stefanoia@tin.it)
// @date 24/02/2004
// @lastdate 25/02/2008
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

// ----- Different link types -----
#define NET_LINK_TYPE_TRANSIT  1
#define NET_LINK_TYPE_STUB     2

#define NET_LINK_MAX_DEPTH     10
#define NET_LINK_DEFAULT_DEPTH 1

#define NET_LINK_UNIDIR        0
#define NET_LINK_BIDIR         1

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ net_link_create_rtr ]------------------------------------
  int net_link_create_rtr(SNetNode * pSrcNode, SNetNode * pDstNode,
			  EIfaceDir eDir, SNetIface ** ppIface);
  // -----[ net_link_create_ptp ]------------------------------------
  int net_link_create_ptp(SNetNode * pSrcNode,
			  net_iface_id_t tSrcIfaceID,
			  SNetNode * pDstNode,
			  net_iface_id_t tDstIfaceID,
			  EIfaceDir eDir,
			  SNetIface ** ppIface);
  // -----[ net_link_create_ptmp ]-----------------------------------
  int net_link_create_ptmp(SNetNode * pSrcNode, SNetSubnet * pSubnet,
			   net_addr_t tIfaceAddr, SNetIface ** ppIface);
  // -----[ net_link_set_phys_attr ]---------------------------------
  int net_link_set_phys_attr(SNetIface * pIface, net_link_delay_t tDelay,
			     net_link_load_t tCapacity, EIfaceDir eDir);
  // ----- net_link_destroy -----------------------------------------
  void net_link_destroy(SNetLink ** ppLink);


  ///////////////////////////////////////////////////////////////////
  // LINK DUMP
  ///////////////////////////////////////////////////////////////////

  // ----- net_link_dump --------------------------------------------
  void net_link_dump(SLogStream * pStream, SNetLink * pLink);
  // ----- net_link_dump_load ---------------------------------------
  void net_link_dump_load(SLogStream * pStream, SNetLink * pLink);
  // ----- net_link_dump_info ---------------------------------------
  void net_link_dump_info(SLogStream * pStream, SNetLink * pLink);


#ifdef __cplusplus
}
#endif

#endif /* __NET_LINK_H__ */
