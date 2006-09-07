// ==================================================================
// @(#)node.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 08/08/2005
// @lastdate 03/03/2006
// ==================================================================

#ifndef __NET_NODE_H__
#define __NET_NODE_H__

#include <net/net_types.h>
#include <net/protocol.h>

#define NET_ERROR_MGMT_INVALID_NODE        -100
#define NET_ERROR_MGMT_INVALID_LINK        -101
#define NET_ERROR_MGMT_INVALID_SUBNET      -102
#define NET_ERROR_MGMT_LINK_ALREADY_EXISTS -110
#define NET_ERROR_MGMT_INVALID_OPERATION   -120

// ----- node_mgmt_perror -------------------------------------------
extern void node_mgmt_perror(SLogStream * pStream, int iErrorCode);
// ----- node_set_name ----------------------------------------------
extern void node_set_name(SNetNode * pNode, const char * pcName);
// ----- node_get_name ----------------------------------------------
extern char * node_get_name(SNetNode * pNode);
// ----- node_dump --------------------------------------------------
extern void node_dump(SLogStream * pStream, SNetNode * pNode);
// ----- node_info --------------------------------------------------
extern void node_info(SLogStream * pStream, SNetNode * pNode);


/////////////////////////////////////////////////////////////////////
//
// NODE LINKS FUNCTIONS
//
/////////////////////////////////////////////////////////////////////

// ----- node_add_link ----------------------------------------------
extern int node_add_link(SNetNode * pNode, SNetDest sDest,
			 net_link_delay_t tDelay);
// ----- node_add_link_to_router ------------------------------------
extern int node_add_link_to_router(SNetNode * pNodeA, SNetNode * pNodeB, 
				   net_link_delay_t tDelay, int iMutual);
// ----- node_add_link_to_subnet ------------------------------------
extern int node_add_link_to_subnet(SNetNode * pNode, SNetSubnet * pSubnet,
				   net_addr_t tIfaceAddr,
				   net_link_delay_t tDelay, int iMutual);
// ----- node_find_link ---------------------------------------------
extern SNetLink * node_find_link(SNetNode * pNode, SNetDest sDest/* net_addr_t tIfaceAddr*/);
// ----- node_find_link_to_router -----------------------------------
extern SNetLink * node_find_link_to_router(SNetNode * pNode,
					   net_addr_t tAddr);
// ----- node_find_link_to_subnet -----------------------------------
extern SNetLink * node_find_link_to_subnet(SNetNode * pNode,
					   SNetSubnet * pSubnet, net_addr_t tIfaceAddr);


/////////////////////////////////////////////////////////////////////
// ROUTING TABLE FUNCTIONS
/////////////////////////////////////////////////////////////////////

// ----- node_rt_add_route ------------------------------------------
extern int node_rt_add_route(SNetNode * pNode, SPrefix sPrefix,
			     net_addr_t tNextHopIface, net_addr_t tNextHop,
			     uint32_t uWeight, uint8_t uType);
// ----- node_rt_add_route_dest -------------------------------------
extern int node_rt_add_route_dest(SNetNode * pNode, SPrefix sPrefix,
				  SNetDest sDestIface, net_addr_t tNextHop,
				  uint32_t uWeight, uint8_t uType);
// ----- node_rt_add_route_link -------------------------------------
extern int node_rt_add_route_link(SNetNode * pNode, SPrefix sPrefix,
				  SNetLink * pIface, net_addr_t tNextHop,
				  uint32_t uWeight, uint8_t uType);
// ----- node_rt_del_route ------------------------------------------
extern int node_rt_del_route(SNetNode * pNode, SPrefix * pPrefix,
			     SNetDest * pIface, net_addr_t * ptNextHop,
			     uint8_t uType);


/////////////////////////////////////////////////////////////////////
//
// PROTOCOLS
//
/////////////////////////////////////////////////////////////////////

// ----- node_register_protocol -------------------------------------
extern int node_register_protocol(SNetNode * pNode, uint8_t uNumber,
				  void * pHandler,
				  FNetNodeHandlerDestroy fDestroy,
				  FNetNodeHandleEvent fHandleEvent);
// -----[ node_get_protocol ]----------------------------------------
extern SNetProtocol * node_get_protocol(SNetNode * pNode, uint8_t uNumber);

#endif /* __NET_NODE_H__ */
