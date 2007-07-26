// ==================================================================
// @(#)node.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 08/08/2005
// @lastdate 23/07/2007
// ==================================================================

#ifndef __NET_NODE_H__
#define __NET_NODE_H__

#include <net/icmp.h>
#include <net/net_types.h>
#include <net/protocol.h>

// ----- Netflow load options -----
#define NET_NODE_NETFLOW_OPTIONS_SUMMARY 0x01
#define NET_NODE_NETFLOW_OPTIONS_DETAILS 0x02

#ifdef __cplusplus
extern "C" {
#endif

  // ----- node_create ----------------------------------------------
  SNetNode * node_create(net_addr_t tAddr);
  // ----- node_destroy ---------------------------------------------
  void node_destroy(SNetNode ** ppNode);
  
  // ----- node_set_name --------------------------------------------
  void node_set_name(SNetNode * pNode, const char * pcName);
  // ----- node_get_name --------------------------------------------
  char * node_get_name(SNetNode * pNode);
  // ----- node_dump ------------------------------------------------
  void node_dump(SLogStream * pStream, SNetNode * pNode);
  // ----- node_info ------------------------------------------------
  void node_info(SLogStream * pStream, SNetNode * pNode);
  
  
  ///////////////////////////////////////////////////////////////////
  // NODE LINKS FUNCTIONS
  ///////////////////////////////////////////////////////////////////

  // ----- node_add_link --------------------------------------------
  int node_add_link(SNetNode * pNode, SNetDest sDest,
		    net_link_delay_t tDelay, net_link_load_t tCapacity,
		    uint8_t tDepth);
  // ----- node_add_link_ptp ----------------------------------------
  int node_add_link_ptp(SNetNode * pNodeA, SNetNode * pNodeB,
			net_link_delay_t tDelay, net_link_load_t tCapacity,
			uint8_t tDepth, int iMutual);
  // ----- node_find_link -------------------------------------------
  SNetLink * node_find_link(SNetNode * pNode, SNetDest sDest);
  // ----- node_find_link_ptp ---------------------------------------
  SNetLink * node_find_link_ptp(SNetNode * pNode, net_addr_t tAddr);
  // ----- node_find_link_mtp ---------------------------------------
  SNetLink * node_find_link_mtp(SNetNode * pNode, SNetSubnet * pSubnet,
				net_addr_t tIfaceAddr);
  // -----[ node_find_iface ]----------------------------------------
  SNetLink * node_find_iface(SNetNode * pNode, SNetDest sDest);

  // ----- node_links_clear -----------------------------------------
  void node_links_clear(SNetNode * pNode);
  // ----- node_links_save ------------------------------------------
  void node_links_save(SLogStream * pStream, SNetNode * pNode);

  // ----- node_has_address -----------------------------------------
  int node_has_address(SNetNode * pNode, net_addr_t tAddress);
  // ----- node_addresses_for_each ----------------------------------
  int node_addresses_for_each(SNetNode * pNode,
				     FArrayForEach fForEach,
				     void * pContext);
  // ----- node_addresses_dump --------------------------------------
  void node_addresses_dump(SLogStream * pStream, SNetNode * pNode);
  // -----[ node_ifaces_dump ]---------------------------------------
  void node_ifaces_dump(SLogStream * pStream, SNetNode * pNode);
  
  
  ///////////////////////////////////////////////////////////////////
  // ROUTING TABLE FUNCTIONS
  ///////////////////////////////////////////////////////////////////
  
  // ----- node_rt_add_route ----------------------------------------
  int node_rt_add_route(SNetNode * pNode, SPrefix sPrefix,
			net_addr_t tNextHopIface, net_addr_t tNextHop,
			uint32_t uWeight, uint8_t uType);
  // ----- node_rt_add_route_dest -----------------------------------
  int node_rt_add_route_dest(SNetNode * pNode, SPrefix sPrefix,
			     SNetDest sDestIface, net_addr_t tNextHop,
			     uint32_t uWeight, uint8_t uType);
  // ----- node_rt_add_route_link -----------------------------------
  int node_rt_add_route_link(SNetNode * pNode, SPrefix sPrefix,
			     SNetLink * pIface, net_addr_t tNextHop,
			     uint32_t uWeight, uint8_t uType);
  // ----- node_rt_del_route ----------------------------------------
  int node_rt_del_route(SNetNode * pNode, SPrefix * pPrefix,
			SNetDest * pIface, net_addr_t * ptNextHop,
			uint8_t uType);
  
  
  ///////////////////////////////////////////////////////////////////
  // PROTOCOL FUNCTIONS
  ///////////////////////////////////////////////////////////////////

  // ----- node_register_protocol -----------------------------------
  int node_register_protocol(SNetNode * pNode, uint8_t uNumber,
			     void * pHandler,
			     FNetNodeHandlerDestroy fDestroy,
			     FNetNodeHandleEvent fHandleEvent);
  // -----[ node_get_protocol ]--------------------------------------
  SNetProtocol * node_get_protocol(SNetNode * pNode, uint8_t uNumber);


  ///////////////////////////////////////////////////////////////////
  // TRAFFIC LOAD FUNCTIONS
  ///////////////////////////////////////////////////////////////////

  // -----[ node_load_netflow ]--------------------------------------
  int node_load_netflow(SNetNode * pNode, const char * pcFileName,
			uint8_t uOptions);

  
#ifdef __cplusplus
}
#endif

#endif /* __NET_NODE_H__ */
