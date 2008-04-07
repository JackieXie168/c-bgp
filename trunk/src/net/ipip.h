// ==================================================================
// @(#)ipip.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 25/02/2004
// $Id: ipip.h,v 1.4 2008-04-07 09:42:04 bqu Exp $
// ==================================================================

#ifndef __NET_IPIP_H__
#define __NET_IPIP_H__

#include <net/prefix.h>
#include <net/message.h>
#include <net/network.h>

#ifdef __cplusplus
extern "C" {
#endif

  // ----- ipip_send ------------------------------------------------
  int ipip_send(net_node_t * node, net_addr_t dst_addr,
		net_msg_t * msg);
  // ----- ipip_event_handler ---------------------------------------
  int ipip_event_handler(simulator_t * sim,
			 void * handler, net_msg_t * msg);
  // -----[ ipip_link_create ]---------------------------------------
  int ipip_link_create(net_node_t * node, net_addr_t end_point,
		       net_addr_t addr, net_iface_t * oif,
		       net_addr_t src_addr, net_iface_t ** ppLink);

#ifdef __cplusplus
}
#endif

#endif /* __NET_IPIP_H__ */
