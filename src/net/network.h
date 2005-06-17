// ==================================================================
// @(#)network.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 4/07/2003
// @lastdate 17/05/2005
// ==================================================================

#ifndef __NET_NETWORK_H__
#define __NET_NETWORK_H__

#include <stdio.h>

#include <libgds/array.h>
#ifdef __EXPERIMENTAL__
# include <libgds/patricia-tree.h>
#else
# include <libgds/radix-tree.h>
#endif
#include <libgds/types.h>

#include <net/network_t.h>
#include <net/domain_t.h>
#include <net/prefix.h>
#include <net/link.h>
#include <net/message.h>
#include <net/net_path.h>
#include <net/protocol.h>
#include <net/routing.h>
#include <sim/simulator.h>
#include <net/subnet.h>


#define NET_SUCCESS                0
#define NET_ERROR_UNKNOWN_PROTOCOL -1
#define NET_ERROR_NO_ROUTE_TO_HOST -2
#define NET_ERROR_TTL_EXPIRED      -3
#define NET_ERROR_LINK_DOWN        -4
#define NET_ERROR_PROTOCOL_ERROR   -5



// ----- node_find_link ---------------------------------------------
//extern SNetLink * node_belongs_to_OSPFAREA(SNetNode * pNode, uint32_t OSPFArea);

typedef struct {
  net_addr_t tAddr;
  SPrefix * tMask;
  SPtrArray * pLinks;
} SNetInterface;

// --
void node_dump(SNetNode * pNode);
// ----- options -----
extern uint8_t NET_OPTIONS_MAX_HOPS;
extern uint8_t NET_OPTIONS_IGP_INTER;



// ----- node_create ------------------------------------------------
extern SNetNode * node_create(net_addr_t tAddr);
// ----- node_create ------------------------------------------------
extern int node_compare(void * pItem1, void * pItem2, unsigned int uEltSize);
// ----- node_set_name ----------------------------------------------
void node_set_name(SNetNode * pNode, const char * pcName);
// ----- node_get_name ----------------------------------------------
char * node_get_name(SNetNode * pNode);
// ----- node_get_as ------------------------------------------------
SNetDomain * node_get_as(SNetNode * pNode);
// ----- node_get_as ------------------------------------------------
uint32_t node_get_as_id(SNetNode * pNode);
// ----- node_set_as ------------------------------------------------
void node_set_as(SNetNode * pNode, SNetDomain * pDomain);
// ----- node_interface_add -----------------------------------------
int node_interface_add(SNetNode * pNode, SNetInterface * pInterface);
// ----- node_interface_create ---------------------------------------
SNetInterface * node_interface_create();
// ----- node_interface_link_add ------------------------------------
int node_interface_link_add(SNetNode * pNode, net_addr_t tFromIf, 
    net_addr_t tToIf, net_link_delay_t tDelay);
// ----- node_interface_get_number ----------------------------------
unsigned int node_interface_get_number(SNetNode * pNode);
// ----- node_interface_get -----------------------------------------
SNetInterface * node_interface_get(SNetNode * pNode, 
				const unsigned int uIndex);
// ----- node_add_link ----------------------------------------------
extern int node_add_link(SNetNode * pNodeA, SNetNode * pNodeB,
			 net_link_delay_t tDelay, int iRecurse);
// ----- node_add_link_toSubnet ----------------------------------------------
extern int node_add_link(SNetNode * pNodeA, SNetNode * pNodeB,
			 net_link_delay_t tDelay, int iRecurse);
// ----- node_find_link ---------------------------------------------
extern SNetLink * node_find_link(SNetNode * pNode,
				 net_addr_t tAddr);
// ----- node_post_event --------------------------------------------
extern int node_post_event(SNetNode * pNode);

// ----- node_links_dump --------------------------------------------
extern void node_links_dump(FILE * pStream, SNetNode * pNode);
// ----- node_links_for_each ----------------------------------------
extern int node_links_for_each(SNetNode * pNode, FArrayForEach fForEach,
			       void * pContext);
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
			 SNetDest sDest);
// ----- node_register_protocol -------------------------------------
extern int node_register_protocol(SNetNode * pNode, uint8_t uNumber,
				  void * pHandler,
				  FNetNodeHandlerDestroy fDestroy,
				  FNetNodeHandleEvent fHandleEvent);
// ----- node_send --------------------------------------------------
extern int node_send(SNetNode * pNode, net_addr_t tAddr,
		     uint8_t uProtocol, void * pPayLoad,
		     FPayLoadDestroy fDestroy);
// ----- node_recv --------------------------------------------------
extern int node_recv(SNetNode * pNode, SNetMessage * pMessage);
// ----- node_record_route ------------------------------------------
extern int node_record_route(SNetNode * pNode, SNetDest sDest,
			     SNetPath ** ppPath,
			     net_link_delay_t * pDelay,
			     uint32_t * pWeight);
// ----- node_dump_recorded_route -----------------------------------
extern void node_dump_recorded_route(FILE * pStream, SNetNode * pNode,
				     SNetDest sDest, int iDelay);

// ----- node_ipip_enable -------------------------------------------
extern int node_ipip_enable(SNetNode * pNode);
// ----- node_add_tunnel --------------------------------------------
extern int node_add_tunnel(SNetNode * pNode, net_addr_t tDstPoint);


/////////////////////////////////////////////////////////////////////
///// NETWORK METHODS
/////////////////////////////////////////////////////////////////////

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

// ----- network_domain_add -----------------------------------------
int network_domain_add(SNetwork * pNetwork, uint32_t uAS, 
						      char * pcName);
// ----- network_domain_get -----------------------------------------
SNetDomain * network_domain_get(SNetwork * pNetwork, uint32_t);

// ----- _network_destroy -------------------------------------------
extern void _network_destroy();

//Should be in subnet.h
// ----- node_add_link_toSubnet ------------------------------------
extern int node_add_link_toSubnet(SNetNode * pNode, SNetSubnet * pSubnet, 
                                         net_link_delay_t tDelay, int iRecurse); 





#endif
