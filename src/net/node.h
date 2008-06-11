// ==================================================================
// @(#)node.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 08/08/2005
// $Id: node.h,v 1.12 2008-06-11 15:13:45 bqu Exp $
// ==================================================================

#ifndef __NET_NODE_H__
#define __NET_NODE_H__

#include <net/icmp.h>
#include <net/iface.h>
#include <net/net_types.h>
#include <net/protocol.h>

// ----- Netflow load options -----
#define NET_NODE_NETFLOW_OPTIONS_SUMMARY 0x01
#define NET_NODE_NETFLOW_OPTIONS_DETAILS 0x02

// ----- Node creation options -----
#define NODE_OPTIONS_LOOPBACK 0x01

#ifdef __cplusplus
extern "C" {
#endif

  // ----- node_create ----------------------------------------------
  net_error_t node_create(net_addr_t tAddr, net_node_t ** node_ref,
			  int options);
  // ----- node_destroy ---------------------------------------------
  void node_destroy(net_node_t ** node_ref);
  
  // ----- node_set_name --------------------------------------------
  void node_set_name(net_node_t * node, const char * name);
  // ----- node_get_name --------------------------------------------
  char * node_get_name(net_node_t * node);
  // ----- node_dump ------------------------------------------------
  void node_dump(SLogStream * stream, net_node_t * node);
  // ----- node_info ------------------------------------------------
  void node_info(SLogStream * stream, net_node_t * node);
  
  
  ///////////////////////////////////////////////////////////////////
  // NODE INTERFACES FUNCTIONS
  ///////////////////////////////////////////////////////////////////

  // -----[ node_add_iface ]-----------------------------------------
  int node_add_iface(net_node_t * node, net_iface_id_t tIfaceID,
		     net_iface_type_t tIfaceType);
  // -----[ node_add_iface2 ]----------------------------------------
  int node_add_iface2(net_node_t * node, net_iface_t * iface);
  // -----[ node_find_iface ]----------------------------------------
  net_iface_t * node_find_iface(net_node_t * node, net_iface_id_t tIfaceID);
  // -----[ node_ifaces_load_clear ]---------------------------------
  void node_ifaces_load_clear(net_node_t * node);


  ///////////////////////////////////////////////////////////////////
  // NODE LINKS FUNCTIONS
  ///////////////////////////////////////////////////////////////////

  // ----- node_links_dump ------------------------------------------
  void node_links_dump(SLogStream * stream, net_node_t * node);
  // ----- node_links_save ------------------------------------------
  void node_links_save(SLogStream * stream, net_node_t * node);

  // -----[ node_has_address ]---------------------------------------
  int node_has_address(net_node_t * node, net_addr_t addr);
  // -----[ node_has_prefix ]----------------------------------------
  net_iface_t * node_has_prefix(net_node_t * node, ip_pfx_t pfx);
  // ----- node_addresses_for_each ----------------------------------
  int node_addresses_for_each(net_node_t * node, FArrayForEach for_each,
			      void * ctx);
  // ----- node_addresses_dump --------------------------------------
  void node_addresses_dump(SLogStream * stream, net_node_t * node);
  // -----[ node_ifaces_dump ]---------------------------------------
  void node_ifaces_dump(SLogStream * stream, net_node_t * node);
  
  
  ///////////////////////////////////////////////////////////////////
  // ROUTING TABLE FUNCTIONS
  ///////////////////////////////////////////////////////////////////
  
  // ----- node_rt_add_route ----------------------------------------
  int node_rt_add_route(net_node_t * node, ip_pfx_t pfx,
			net_iface_id_t oif_id, net_addr_t nexthop,
			uint32_t weight, uint8_t type);
  // ----- node_rt_add_route_link -----------------------------------
  int node_rt_add_route_link(net_node_t * node, ip_pfx_t pfx,
			     net_iface_t * iface, net_addr_t nexthop,
			     uint32_t weight, uint8_t type);
  // ----- node_rt_del_route ----------------------------------------
  int node_rt_del_route(net_node_t * node, ip_pfx_t * pfx,
			net_iface_id_t * oif_id, net_addr_t * nexthop,
			uint8_t type);
  // ----- node_rt_dump ---------------------------------------------
  void node_rt_dump(SLogStream * stream, net_node_t * node,
		    SNetDest sDest);
  
  
  ///////////////////////////////////////////////////////////////////
  // PROTOCOL FUNCTIONS
  ///////////////////////////////////////////////////////////////////

  // ----- node_register_protocol -----------------------------------
  int node_register_protocol(net_node_t * node,
			     net_protocol_id_t id,
			     void * pHandler,
			     FNetProtoHandlerDestroy fDestroy,
			     FNetProtoHandleEvent fHandleEvent);
  // -----[ node_get_protocol ]--------------------------------------
  net_protocol_t * node_get_protocol(net_node_t * node,
				     net_protocol_id_t id);


  ///////////////////////////////////////////////////////////////////
  // TRAFFIC LOAD FUNCTIONS
  ///////////////////////////////////////////////////////////////////

  // -----[ node_load_netflow ]--------------------------------------
  int node_load_netflow(net_node_t * node, const char * file_name,
			uint8_t options);


  ///////////////////////////////////////////////////////////////////
  // SYSLOG
  ///////////////////////////////////////////////////////////////////

  // -----[ node_syslog ]--------------------------------------------
  SLogStream * node_syslog(net_node_t * self);
  // -----[ node_syslog_set_enabled ]--------------------------------
  void node_syslog_set_enabled(net_node_t * self, int enabled);
  
#ifdef __cplusplus
}
#endif

#endif /* __NET_NODE_H__ */
