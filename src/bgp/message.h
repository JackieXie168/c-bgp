// ==================================================================
// @(#)message.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @author Sebastien Tandel (standel@info.ucl.ac.be)
// 
// @date 19/05/2003
// $Id: message.h,v 1.16 2009-03-24 14:23:55 bqu Exp $
// ==================================================================

#ifndef __BGP_MESSAGE_H__
#define __BGP_MESSAGE_H__

#include <libgds/types.h>

#include <net/network.h>
#include <bgp/types.h>

#ifdef _cplusplus
extern "C" {
#endif
  
  // ----- bgp_msg_update_create ------------------------------------
  bgp_msg_t * bgp_msg_update_create(uint16_t peer_asn,
				    bgp_route_t * route);
  // ----- bgp_msg_withdraw_create ----------------------------------
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  bgp_msg_t * bgp_msg_withdraw_create(uint16_t peer_asn,
				      ip_pfx_t prefix,
				      net_addr_t * next_hop);
#else
  bgp_msg_t * bgp_msg_withdraw_create(uint16_t peer_asn,
				      ip_pfx_t prefix);
#endif
  // ----- bgp_msg_close_create -------------------------------------
  bgp_msg_t * bgp_msg_close_create(uint16_t peer_asn);
  // ----- bgp_msg_open_create --------------------------------------
  bgp_msg_t * bgp_msg_open_create(uint16_t peer_asn,
				  net_addr_t router_id);

  // ----- bgp_msg_destroy ------------------------------------------
  void bgp_msg_destroy(bgp_msg_t ** msg_ref);
  // ----- bgp_msg_send ---------------------------------------------
  int bgp_msg_send(net_node_t * node, net_addr_t src_addr,
		   net_addr_t dst_addr, bgp_msg_t * msg);
  // ----- bgp_msg_dump ---------------------------------------------
  void bgp_msg_dump(gds_stream_t * stream, net_node_t * node,
		    bgp_msg_t * msg);

  // ----- bgp_msg_monitor_open -------------------------------------
  int bgp_msg_monitor_open(const char * file_name);
  // -----[ bgp_msg_monitor_close ]----------------------------------
  void bgp_msg_monitor_close();
  // ----- bgp_msg_monitor_write ------------------------------------
  void bgp_msg_monitor_write(bgp_msg_t * msg, net_node_t * node,
			     net_addr_t addr);

  // -----[ _message_destroy ]---------------------------------------
  void _message_destroy();

#ifdef _cplusplus
}
#endif

#endif
