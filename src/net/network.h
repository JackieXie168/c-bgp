// ==================================================================
// @(#)network.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 4/07/2003
// @lastdate 10/08/2005
// ==================================================================

#ifndef __NET_NETWORK_H__
#define __NET_NETWORK_H__

#include <stdio.h>

#include <libgds/array.h>
#include <libgds/patricia-tree.h>
#include <libgds/types.h>

#include <net/net_types.h>
#include <net/network_t.h>
#include <net/domain_t.h>
#include <net/prefix.h>
#include <net/message.h>
#include <net/net_path.h>
#include <net/protocol.h>
#include <net/link.h>
#include <net/routing.h>
#include <sim/simulator.h>

#define NET_SUCCESS                0
#define NET_ERROR_UNKNOWN_PROTOCOL -1
#define NET_ERROR_NO_ROUTE_TO_HOST -2
#define NET_ERROR_TTL_EXPIRED      -3
#define NET_ERROR_LINK_DOWN        -4
#define NET_ERROR_PROTOCOL_ERROR   -5
#define NET_ERROR_DST_UNREACHABLE  -6

// ----- options -----
extern uint8_t NET_OPTIONS_MAX_HOPS;

// ----- network_perror ---------------------------------------------
extern void network_perror(FILE * pStream, int iErrorCode);
// ----- node_create ------------------------------------------------
extern SNetNode * node_create(net_addr_t tAddr);
// ----- node_create ------------------------------------------------
extern int node_compare(void * pItem1, void * pItem2, unsigned int uEltSize);
// ----- node_set_name ----------------------------------------------
extern void node_set_name(SNetNode * pNode, const char * pcName);
// ----- node_get_name ----------------------------------------------
extern char * node_get_name(SNetNode * pNode);
// ----- node_get_prefix----------------------------------------------
extern void node_get_prefix(SNetNode * pNode, SPrefix * pPrefix);
// ----- node_add_link_toSubnet -------------------------------------
/*extern int node_add_link_toSubnet(SNetNode * pNodeA, SNetSubnet * pSubnet,
				  net_addr_t tIfaceAddr,
				  net_link_delay_t tDelay, int iRecurse);*/
// ----- node_post_event --------------------------------------------
extern int node_post_event(SNetNode * pNode);
// ----- node_has_address -------------------------------------------
extern int node_has_address(SNetNode * pNode, net_addr_t tAddress);
// ----- node_addresses_dump ----------------------------------------
extern void node_addresses_dump(FILE * pStream, SNetNode * pNode);
// ----- node_dump --------------------------------------------------
extern void node_dump(FILE * pStream, SNetNode * pNode);
// ----- node_info --------------------------------------------------
extern void node_info(FILE * pStream, SNetNode * pNode);
// ----- node_links_dump --------------------------------------------
extern void node_links_dump(FILE * pStream, SNetNode * pNode);
// ----- node_links_destroy -----------------------------------------
void node_links_destroy(void * pItem);
// ----- node_links_for_each ----------------------------------------
extern int node_links_for_each(SNetNode * pNode, FArrayForEach fForEach,
			       void * pContext);
// ----- node_links_lookup ------------------------------------------
extern SNetLink * node_links_lookup(SNetNode * pNode,
				    net_addr_t tDstAddr);
// ----- node_rt_lookup ---------------------------------------------
extern SNetRouteNextHop * node_rt_lookup(SNetNode * pNode,
					 net_addr_t tDstAddr);
// ----- node_rt_dump -----------------------------------------------
extern void node_rt_dump(FILE * pStream, SNetNode * pNode,
			 SNetDest sDest);
// ----- node_register_protocol -------------------------------------
extern int node_register_protocol(SNetNode * pNode, uint8_t uNumber,
				  void * pHandler,
				  FNetNodeHandlerDestroy fDestroy,
				  FNetNodeHandleEvent fHandleEvent);
// ----- node_send --------------------------------------------------
extern int node_send(SNetNode * pNode, net_addr_t tSrcAddr,
		     net_addr_t tAddr, uint8_t uProtocol,
		     void * pPayLoad, FPayLoadDestroy fDestroy);
// ----- node_recv --------------------------------------------------
extern int node_recv(SNetNode * pNode, SNetMessage * pMessage);
// ----- node_ipip_enable -------------------------------------------
extern int node_ipip_enable(SNetNode * pNode);
// ----- node_igp_domain_add -------------------------------------------
extern int node_igp_domain_add(SNetNode * pNode, uint16_t uDomainNumber);
// ----- node_belongs_to_igp_domain -------------------------------------------
extern int node_belongs_to_igp_domain(SNetNode * pNode,
				      uint16_t uDomainNumber);
// ----- node_add_tunnel --------------------------------------------
extern int node_add_tunnel(SNetNode * pNode, net_addr_t tDstPoint);
// ----- node_destroy -----------------------------------------------
extern void node_destroy(SNetNode ** ppNode);

/////////////////////////////////////////////////////////////////////
//
// NETWORK METHODS
//
/////////////////////////////////////////////////////////////////////

// ----- network_create ---------------------------------------------
extern SNetwork * network_create();
// ----- network_destroy --------------------------------------------
extern void network_destroy(SNetwork ** ppNetwork);
// ----- network_get ------------------------------------------------
extern SNetwork * network_get();
// ----- network_add_node -------------------------------------------
extern int network_add_node(SNetNode * pNode);
// ----- network_add_subnet -------------------------------------------
extern int network_add_subnet(SNetSubnet * pSubnet);
// ----- network_find_node ------------------------------------------
extern SNetNode * network_find_node(net_addr_t tAddr);
// ----- network_find_subnet ------------------------------------------
extern SNetSubnet * network_find_subnet(SPrefix sPrefix);
// ----- network_to_file --------------------------------------------
extern int network_to_file(FILE * pStream, SNetwork * pNetwork);
// ----- network_from_file ------------------------------------------
extern SNetwork * network_from_file(FILE * pStream);
// ----- network_shortest_path --------------------------------------
extern int network_shortest_path(SNetwork * pNetwork, FILE * pStream,
				 net_addr_t tSrcAddr);
//---- network_dump_subnets -----------------------------------------
extern void network_dump_subnets(FILE * pStream, SNetwork *pNetwork);
// ----- network_enum_nodes -----------------------------------------
extern char * network_enum_nodes(const char * pcText, int state);
// ----- _network_create --------------------------------------------
extern void _network_create();
// ----- _network_destroy -------------------------------------------
extern void _network_destroy();
// ----- network_forward --------------------------------------------
extern int network_forward(SNetwork * pNetwork, SNetLink * pLink,
			   net_addr_t tNextHop, SNetMessage * pMessage);

// ----- _network_send_callback -------------------------------------
extern int _network_send_callback(void * pContext);
// ----- _network_send_dump -----------------------------------------
extern void _network_send_dump(FILE * pStream, void * pContext);
// ----- _network_send_context_create -------------------------------
extern SNetSendContext * _network_send_context_create(SNetNode * pNode,
						      SNetMessage * pMessage);
// ----- _network_send_context_destroy -------------------------------
extern void _network_send_context_destroy(void * pContext);

#endif /* __NET_NETWORK_H__ */
