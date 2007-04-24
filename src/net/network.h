// ==================================================================
// @(#)network.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 4/07/2003
// @lastdate 18/04/2007
// ==================================================================

#ifndef __NET_NETWORK_H__
#define __NET_NETWORK_H__

#include <stdio.h>

#include <libgds/array.h>
#include <libgds/log.h>
#include <libgds/patricia-tree.h>
#include <libgds/types.h>

#include <net/net_types.h>
#include <net/prefix.h>
#include <net/message.h>
#include <net/net_path.h>
#include <net/link.h>
#include <net/routing.h>
#include <sim/simulator.h>

#define NET_SUCCESS                  0
#define NET_ERROR_UNKNOWN            -1
#define NET_ERROR_NET_UNREACH        -2
#define NET_ERROR_DST_UNREACH        -3
#define NET_ERROR_PORT_UNREACH       -4
#define NET_ERROR_TIME_EXCEEDED      -5
#define NET_ERROR_ICMP_NET_UNREACH   -6
#define NET_ERROR_ICMP_DST_UNREACH   -7
#define NET_ERROR_ICMP_PORT_UNREACH  -8
#define NET_ERROR_ICMP_TIME_EXCEEDED -9
#define NET_ERROR_LINK_DOWN          -10
#define NET_ERROR_PROTOCOL_ERROR     -11
#define NET_ERROR_IF_UNKNOWN         -12
#define NET_ERROR_NO_REPLY           -13

// ----- options -----
extern uint8_t NET_OPTIONS_MAX_HOPS;

#ifdef __cplusplus
extern "C" {
#endif

  // ----- network_perror -------------------------------------------
  void network_perror(SLogStream * pStream, int iErrorCode);
  // -----[ network_get_simulator ]----------------------------------
  SSimulator * network_get_simulator();
  // ----- node_create ----------------------------------------------
  SNetNode * node_create(net_addr_t tAddr);
  // ----- node_get_prefix-------------------------------------------
  void node_get_prefix(SNetNode * pNode, SPrefix * pPrefix);
  // ----- node_post_event ------------------------------------------
  int node_post_event(SNetNode * pNode);
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
  // ----- node_links_dump ------------------------------------------
  void node_links_dump(SLogStream * pStream, SNetNode * pNode);
  // ----- node_links_destroy ---------------------------------------
  void node_links_destroy(void * pItem);
  // ----- node_links_for_each --------------------------------------
  int node_links_for_each(SNetNode * pNode, FArrayForEach fForEach,
				 void * pContext);
  // ----- node_rt_lookup -------------------------------------------
  SNetRouteNextHop * node_rt_lookup(SNetNode * pNode,
				    net_addr_t tDstAddr);
  // ----- node_rt_dump ---------------------------------------------
  void node_rt_dump(SLogStream * pStream, SNetNode * pNode,
			   SNetDest sDest);
  // ----- node_send_msg --------------------------------------------
  int node_send_msg(SNetNode * pNode, net_addr_t tSrcAddr,
		    net_addr_t tDstAddr, uint8_t uProtocol, uint8_t uTTL,
		    void * pPayLoad, FPayLoadDestroy fDestroy,
		    SSimulator * pSimulator);
  // ----- node_recv_msg --------------------------------------------
  int node_recv_msg(SNetNode * pNode, SNetMessage * pMessage,
		    SSimulator * pSimulator);
  // ----- node_ipip_enable -----------------------------------------
  int node_ipip_enable(SNetNode * pNode);
  // ----- node_igp_domain_add --------------------------------------
  int node_igp_domain_add(SNetNode * pNode, uint16_t uDomainNumber);
  // ----- node_belongs_to_igp_domain -------------------------------
  int node_belongs_to_igp_domain(SNetNode * pNode,
					uint16_t uDomainNumber);
  // ----- node_add_tunnel ------------------------------------------
  int node_add_tunnel(SNetNode * pNode, net_addr_t tDstPoint,
		      net_addr_t tAddr, SNetDest * pOutIfaceDest,
		      net_addr_t tSrcAddr);
  // ----- node_destroy ---------------------------------------------
  void node_destroy(SNetNode ** ppNode);
  
  ///////////////////////////////////////////////////////////////////
  //
  // NETWORK METHODS
  //
  ///////////////////////////////////////////////////////////////////
  
  // ----- network_create -------------------------------------------
  SNetwork * network_create();
  // ----- network_destroy ------------------------------------------
  void network_destroy(SNetwork ** ppNetwork);
  // ----- network_get ----------------------------------------------
  SNetwork * network_get();
  // ----- network_add_node -----------------------------------------
  int network_add_node(SNetNode * pNode);
  // ----- network_add_subnet ---------------------------------------
  int network_add_subnet(SNetSubnet * pSubnet);
  // ----- network_find_node ----------------------------------------
  SNetNode * network_find_node(net_addr_t tAddr);
  // ----- network_find_subnet --------------------------------------
  SNetSubnet * network_find_subnet(SPrefix sPrefix);
  // ----- network_to_file ------------------------------------------
  int network_to_file(SLogStream * pStream, SNetwork * pNetwork);

  
  //---- network_dump_subnets ---------------------------------------
  void network_dump_subnets(SLogStream * pStream, SNetwork *pNetwork);
  // ----- network_enum_nodes ---------------------------------------
  char * network_enum_nodes(const char * pcText, int state);
  // ----- network_enum_bgp_nodes -----------------------------------
  char * network_enum_bgp_nodes(const char * pcText, int state);
  // ----- _network_create ------------------------------------------
  void _network_create();
  // ----- _network_destroy -----------------------------------------
  void _network_destroy();

  ///////////////////////////////////////////////////////////////////
  // FUNCTIONS FOR GLOBAL TOPOLOGY MANAGEMENT
  ///////////////////////////////////////////////////////////////////

  // ----- network_links_clear --------------------------------------
  void network_links_clear();

    
#ifdef __cplusplus
}
#endif

#endif /* __NET_NETWORK_H__ */
