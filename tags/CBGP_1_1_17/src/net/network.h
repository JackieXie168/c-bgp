// ==================================================================
// @(#)network.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 4/07/2003
// @lastdate 15/09/2004
// ==================================================================

#ifndef __NET_NETWORK_H__
#define __NET_NETWORK_H__

#include <stdio.h>

#include <libgds/array.h>
#include <libgds/radix-tree.h>
#include <libgds/types.h>

#include <net/prefix.h>
#include <net/link.h>
#include <net/message.h>
#include <net/net_path.h>
#include <net/protocol.h>
#include <net/routing.h>
#include <sim/simulator.h>

#define NET_SUCCESS                0
#define NET_ERROR_UNKNOWN_PROTOCOL -1
#define NET_ERROR_NO_ROUTE_TO_HOST -2
#define NET_ERROR_TTL_EXPIRED      -3
#define NET_ERROR_LINK_DOWN        -4
#define NET_ERROR_PROTOCOL_ERROR   -5

extern const net_addr_t MAX_ADDR;

typedef struct {
  SRadixTree * pNodes;
} SNetwork;

typedef struct {
  net_addr_t tAddr;
  SNetwork * pNetwork;
  SPtrArray * pLinks;
  SNetRT * pRT;
  SNetProtocols * pProtocols;
} SNetNode;

// ----- options -----
extern uint8_t NET_OPTIONS_MAX_HOPS;
extern uint8_t NET_OPTIONS_IGP_INTER;

// ----- node_create ------------------------------------------------
extern SNetNode * node_create(net_addr_t tAddr);
// ----- node_add_link ----------------------------------------------
extern int node_add_link(SNetNode * pNodeA, SNetNode * pNodeB,
			 net_link_delay_t tDelay, int iRecurse);
// ----- node_find_link ---------------------------------------------
extern SNetLink * node_find_link(SNetNode * pNode,
				 net_addr_t tAddr);
// ----- node_post_event --------------------------------------------
extern int node_post_event(SNetNode * pNode);

// ----- node_links_dump --------------------------------------------
extern void node_links_dump(FILE * pStream, SNetNode * pNode);
// ----- node_links_lookup ------------------------------------------
extern SNetLink * node_links_lookup(SNetNode * pNode,
				    net_addr_t tDstAddr);
// ----- node_add_route ---------------------------------------------
extern int node_rt_add_route(SNetNode * pNode, SPrefix sPrefix,
			     net_addr_t tNextHop,
			     uint32_t uWeight, uint8_t uType);
// ----- node_rt_del_route ------------------------------------------
extern int node_rt_del_route(SNetNode * pNode, SPrefix * pPrefix,
			     net_addr_t * pNextHop, uint8_t uType);
// ----- node_rt_lookup ---------------------------------------------
extern SNetLink * node_rt_lookup(SNetNode * pNode, net_addr_t tDstAddr);
// ----- node_rt_dump -----------------------------------------------
extern void node_rt_dump(FILE * pStream, SNetNode * pNode,
			 SPrefix sPrefix);
// ----- node_register_protocol -------------------------------------
extern int node_register_protocol(SNetNode * pNode, uint8_t uNumber,
				  void * pHandler,
				  FNetNodeHandlerDestroy fDestroy,
				  FNetNodeHandleEvent fHandleEvent);
// ----- network_create ---------------------------------------------
extern SNetwork * network_create();
// ----- network_destroy --------------------------------------------
extern void network_destroy(SNetwork ** ppNetwork);
// ----- network_get ------------------------------------------------
extern SNetwork * network_get();
// ----- network_add_node -------------------------------------------
extern int network_add_node(SNetwork * pNetwork,
			    SNetNode * pNode);
// ----- network_find_node ------------------------------------------
extern SNetNode * network_find_node(SNetwork * pNetwork,
				    net_addr_t tAddr);
// ----- node_send --------------------------------------------------
extern int node_send(SNetNode * pNode, net_addr_t tAddr,
		     uint8_t uProtocol, void * pPayLoad,
		     FPayLoadDestroy fDestroy);
// ----- node_recv --------------------------------------------------
extern int node_recv(SNetNode * pNode, SNetMessage * pMessage);
// ----- network_to_file --------------------------------------------
extern int network_to_file(FILE * pStream, SNetwork * pNetwork);
// ----- network_from_file ------------------------------------------
extern SNetwork * network_from_file(FILE * pStream);
// ----- network_shortest_path --------------------------------------
extern int network_shortest_path(SNetwork * pNetwork, FILE * pStream,
				 net_addr_t tSrcAddr);
// ----- network_dijkstra -------------------------------------------
extern int network_dijkstra(SNetwork * pNetwork, FILE * pStream,
			    net_addr_t tSrcAddr);
// ----- node_record_route ------------------------------------------
extern int node_record_route(SNetNode * pNode, net_addr_t tDstAddr,
			     SNetPath ** ppPath,
			     net_link_delay_t * pDelay,
			     uint32_t * pWeight);
// ----- node_dump_recorded_route -----------------------------------
extern void node_dump_recorded_route(FILE * pStream, SNetNode * pNode,
				     net_addr_t tDstAddr, int iDelay);

// ----- node_ipip_enable -------------------------------------------
extern int node_ipip_enable(SNetNode * pNode);
// ----- node_add_tunnel --------------------------------------------
extern int node_add_tunnel(SNetNode * pNode, net_addr_t tDstPoint);


#endif
