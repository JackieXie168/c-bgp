// ==================================================================
// @(#)net_types.h
//
// @author Stefano Iasi (stefanoia@tin.it)
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 30/06/2005
// @lastdate 05/08/2005
// ==================================================================

#ifndef __NET_TYPES_H__
#define __NET_TYPES_H__

#include <libgds/array.h>
#include <net/prefix.h>
#include <net/message.h>
#include <net/network_t.h>
#include <net/routing_t.h>


typedef uint8_t   ospf_dest_type_t;
typedef uint32_t  ospf_area_t;
typedef uint8_t   ospf_path_type_t;
typedef SPtrArray next_hops_list_t;
typedef SPtrArray SOSPFRouteInfoList;

typedef SPtrArray links_list_t; // to be removed
typedef SPtrArray SNetLinks;

#define NET_LINK_DELAY_INT

#ifdef NET_LINK_DELAY_INT
typedef uint32_t net_link_delay_t;
#define NET_LINK_DELAY_INFINITE MAX_UINT32_T
#else
typedef double net_link_delay_t;
#endif


//////////////////////////////////////////////////////////////////////
//
// OSPF TYPES DEFINITION
//
//////////////////////////////////////////////////////////////////////

#define ospf_ri_route_type_is(N, T) ((N)->tType == T)
#define ospf_ri_dest_type_is(N, T) ((N)->tOSPFDestinationType == T)
#define ospf_ri_area_is(N, A) ((N)->tOSPFArea == A)
#define ospf_ri_pathType_is(N, T) ((N)->tOSPFPathType == T)

#define ospf_ri_get_area(R) ((R)->tOSPFArea)
#define ospf_ri_get_prefix(R) ((R)->sPrefix)
#define ospf_ri_get_weight(R) ((R)->uWeight)
#define ospf_ri_get_pathType(R) ((R)->tOSPFPathType)
#define ospf_ri_get_NextHops(R) ((R)->aNextHops)

typedef struct {
  ospf_dest_type_t    tOSPFDestinationType;
  SPrefix             sPrefix;
  net_link_delay_t    uWeight;
  ospf_area_t         tOSPFArea;
  ospf_path_type_t    tOSPFPathType;
  next_hops_list_t *  aNextHops;
  net_route_type_t    tType;
} SOSPFRouteInfo;

typedef STrie SOSPFRT;


//////////////////////////////////////////////////////////////////////
//
// NODE TYPE DEFINITION
//
//////////////////////////////////////////////////////////////////////

// ----- SNetNode ---------------------------------------------------
/**
 * This type defines a node object.
 */
typedef struct {
  net_addr_t tAddr;
  char * pcName;
  SNetwork * pNetwork;
  SNetLinks * pLinks;
  SNetRT * pRT;
#ifdef OSPF_SUPPORT
  SUInt32Array  * pOSPFAreas; 
  SOSPFRT       * pOspfRT;     //OSPF routing table
#endif
  SUInt16Array  * pIGPDomains; //TODO define type for list of domains numbers
  SNetProtocols * pProtocols;
  SNetDomain    * pDomain;
} SNetNode;


//////////////////////////////////////////////////////////////////////
//
// SUBNET TYPE DEFINITION
//
//////////////////////////////////////////////////////////////////////

// ----- SNetSubnet -------------------------------------------------
/**
 * Describe transit or stub networks: if there is only one router this
 * object describe a stub network. A transit network otherwise.
 */
typedef struct {
  SPrefix     sPrefix;
  uint8_t     uType;
  ospf_area_t uOSPFArea; /* we have only an area for a network */
  SNetLinks * pLinks;    /* links towards routers */
} SNetSubnet;


//////////////////////////////////////////////////////////////////////
//
// LINK TYPE DEFINITION
//
//////////////////////////////////////////////////////////////////////

// ----- FNetLinkForward --------------------------------------------
/**
 * This type defines a callback that is used to forward messages along
 * links of different types.
 * - point-to-point links use 'link_forward'
 * - subnets use 'subnet_forward'
 * - tunnels use 'ipip_forward'
 */
typedef int (*FNetLinkForward)(net_addr_t tNextHop, void * pContext,
			       SNetNode ** ppNextHop);

/*
  A link can point to a router or a subnet.
  Router can be reached in topology db by IP address (tAddr),
  Subnet can be reached in topology db by memory pointer.
  
  To obtain IP address link_get_address() function MUST BE USED
*/
typedef union {
  net_addr_t tAddr;
  SNetSubnet * pSubnet;
} UDestinationId;

// ----- SNetLink ---------------------------------------------------
/**
 * This type defines a link object.
 */
typedef struct {
  uint8_t uDestinationType;    /* ROUTER / TRANSIT / STUB */
  UDestinationId UDestId;      /* depends on uDestinationType*/
  SNetNode * pSrcNode;
  net_addr_t tIfaceAddr;
  net_link_delay_t tDelay;
  uint8_t          uFlags;
  uint32_t         uIGPweight;      // Store IGP weight here
#ifdef OSPF_SUPPORT
  ospf_area_t      tArea;  
#endif
  void * pContext;
  FNetLinkForward fForward;
} SNetLink;


//////////////////////////////////////////////////////////////////////
//
// MISC UTILITY TYPES DEFINITION
//
//////////////////////////////////////////////////////////////////////

// ----- SNetSendContext --------------------------------------------
/**
 * This type defines a context record used to send messages. The
 * context holds the node to which the message must be delivered as
 * well as the message itself.
 * Upon delivery, the node's receive callback is called with the
 * recorded node as context.
 */
typedef struct {
  SNetNode * pNode;
  SNetMessage * pMessage;
} SNetSendContext;

#endif /* __NET_TYPES_H__ */
