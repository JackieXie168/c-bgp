// ==================================================================
// @(#)network.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 4/07/2003
// $Id: network.h,v 1.32 2008-04-07 09:39:27 bqu Exp $
// ==================================================================

#ifndef __NET_NETWORK_H__
#define __NET_NETWORK_H__

#include <stdio.h>

#include <libgds/array.h>
#include <libgds/log.h>
#include <libgds/patricia-tree.h>
#include <libgds/types.h>

#include <net/iface.h>
#include <net/net_types.h>
#include <net/prefix.h>
#include <net/message.h>
#include <net/net_path.h>
#include <net/link.h>
#include <net/routing.h>
#include <sim/simulator.h>

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ thread_set_simulator ]-----------------------------------
  void thread_set_simulator(simulator_t * sim);

  // -----[ network_drop ]-------------------------------------------
  void network_drop(net_msg_t * msg);
  // -----[ network_send ]-------------------------------------------
  void network_send(net_iface_t * dst_iface, net_msg_t * msg);

  // -----[ network_get_simulator ]----------------------------------
  simulator_t * network_get_simulator(network_t * network);
  // ----- node_get_prefix-------------------------------------------
  void node_get_prefix(net_node_t * node, SPrefix * pPrefix);
  // ----- node_post_event ------------------------------------------
  int node_post_event(net_node_t * node);
  // ----- node_links_for_each --------------------------------------
  int node_links_for_each(net_node_t * node, FArrayForEach fForEach,
			  void * ctx);
  // -----[ node_rt_lookup ]-----------------------------------------
  const rt_entry_t * node_rt_lookup(net_node_t * self,
				    net_addr_t dst_addr);

  ///////////////////////////////////////////////////////////////////
  // IP MESSAGE HANDLING
  ///////////////////////////////////////////////////////////////////
  
  // ----- node_send_msg --------------------------------------------
  net_error_t node_send_msg(net_node_t * self,
			    net_addr_t src_addr,
			    net_addr_t dst_addr,
			    net_protocol_id_t proto,
			    uint8_t ttl,
			    void * payload,
			    FPayLoadDestroy f_destroy,
			    simulator_t * sim);
  // ----- node_recv_msg --------------------------------------------
  net_error_t node_recv_msg(net_node_t * self,
			    net_iface_t * iif,
			    net_msg_t * msg);



  // ----- node_ipip_enable -----------------------------------------
  int node_ipip_enable(net_node_t * node);
  // ----- node_igp_domain_add --------------------------------------
  int node_igp_domain_add(net_node_t * node, uint16_t uDomainNumber);
  // ----- node_belongs_to_igp_domain -------------------------------
  int node_belongs_to_igp_domain(net_node_t * node,
				 uint16_t uDomainNumber);
  // ----- node_add_tunnel ------------------------------------------
  int node_add_tunnel(net_node_t * node, net_addr_t tDstPoint,
		      net_addr_t addr, net_iface_id_t * ptOutIfaceID,
		      net_addr_t src_addr);
  // ----- node_destroy ---------------------------------------------
  void node_destroy(net_node_t ** node_ref);
  
  ///////////////////////////////////////////////////////////////////
  // NETWORK METHODS
  ///////////////////////////////////////////////////////////////////
  
  // ----- network_create -------------------------------------------
  network_t * network_create();
  // ----- network_destroy ------------------------------------------
  void network_destroy(network_t ** network_ref);
  // -----[ network_get_default ]------------------------------------
  network_t * network_get_default();
  // -----[ network_add_node ]---------------------------------------
  int network_add_node(network_t * network, net_node_t * node);
  // -----[ network_add_subnet ]-------------------------------------
  int network_add_subnet(network_t * network, net_subnet_t * subnet);
  // -----[ network_find_node ]--------------------------------------
  net_node_t * network_find_node(network_t * network, net_addr_t addr);
  // -----[ network_find_subnet ]------------------------------------
  net_subnet_t * network_find_subnet(network_t * network, SPrefix sPrefix);
  // ----- network_to_file ------------------------------------------
  int network_to_file(SLogStream * stream, network_t * network);

  
  //---- network_dump_subnets ---------------------------------------
  void network_dump_subnets(SLogStream * pStream, network_t * network);
  // ----- _network_create ------------------------------------------
  /** Create default network */
  void _network_create();
  // ----- _network_destroy -----------------------------------------
  /** Destroy default network */
  void _network_destroy();

  ///////////////////////////////////////////////////////////////////
  // FUNCTIONS FOR GLOBAL TOPOLOGY MANAGEMENT
  ///////////////////////////////////////////////////////////////////

  // -----[ network_ifaces_load_clear ]------------------------------
  void network_ifaces_load_clear();
  // ----- network_links_save ---------------------------------------
  int network_links_save(SLogStream * stream);

    
#ifdef __cplusplus
}
#endif

#endif /* __NET_NETWORK_H__ */
