// ==================================================================
// @(#)net_types.h
//
// @author Stefano Iasi (stefanoia@tin.it)
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 30/06/2005
// @lastdate 23/07/2007
// ==================================================================

#ifndef __NET_TYPES_H__
#define __NET_TYPES_H__

#include <libgds/array.h>
#include <libgds/radix-tree.h>
#include <net/link_attr.h>
#include <net/prefix.h>
#include <net/routing_t.h>
#include <sim/simulator.h>

typedef uint8_t net_tos_t;
typedef uint8_t   ospf_dest_type_t;
typedef uint32_t  ospf_area_t;
typedef uint8_t   ospf_path_type_t;
typedef SPtrArray next_hops_list_t;
typedef SPtrArray SOSPFRouteInfoList;

typedef SPtrArray SNetLinks;
typedef SPtrArray SNetSubnets;

typedef uint32_t net_link_delay_t;
#define NET_LINK_DELAY_INFINITE UINT32_MAX
typedef uint32_t net_link_load_t;
#define NET_LINK_MAX_CAPACITY UINT32_MAX
#define NET_LINK_MAX_LOAD UINT32_MAX


//////////////////////////////////////////////////////////////////////
//
// MESSAGE STRUCTURE
//
//////////////////////////////////////////////////////////////////////

// ----- FPayLoadDestroy -----
typedef void (*FPayLoadDestroy)(void ** pPayLoad);

// ----- SNetMessage -----
typedef struct {
  net_addr_t tSrcAddr;
  net_addr_t tDstAddr;
  uint8_t uProtocol;
  uint8_t uTTL;
  net_tos_t tTOS;
  void * pPayLoad;
  FPayLoadDestroy fDestroy;
} SNetMessage;

// ----- FNetNodeHandleEvent -----
typedef int (*FNetNodeHandleEvent)(SSimulator * pSimulator,
				   void * pHandler,
				   SNetMessage * pMessage);
// ----- FNetNodeHandlerDestroy -----
typedef void (*FNetNodeHandlerDestroy)(void ** ppHandler);

// ----- SNetProtocol -----
typedef struct {
  void * pHandler;
  FNetNodeHandleEvent fHandleEvent;
  FNetNodeHandlerDestroy fDestroy;
} SNetProtocol;

// Maximum number of supported protocols
#define NET_PROTOCOL_MAX  3

// ----- SNetProtocols -----
typedef struct {
  SNetProtocol * data[NET_PROTOCOL_MAX];
} SNetProtocols;

//////////////////////////////////////////////////////////////////////
//
// MAIN NETWORK STRUCTURE
//
//////////////////////////////////////////////////////////////////////

typedef struct {
  STrie * pNodes;
  SNetSubnets  * pSubnets;
  SSimulator * pSimulator;
} SNetwork;


//////////////////////////////////////////////////////////////////////
//
// IGP DOMAIN TYPE DEFINITION
//
//////////////////////////////////////////////////////////////////////

// Supported domain types
typedef enum { DOMAIN_IGP, DOMAIN_OSPF } EDomainType;

typedef struct {
  uint16_t uNumber;
  char * pcName;
  STrie * pRouters;
  EDomainType tType;
  uint8_t uECMP;
} SIGPDomain;


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
#define ospf_ri_get_cost(R) ((R)->uWeight)
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
//  net_addr_t      ospf_id;
#endif
  SUInt16Array  * pIGPDomains; //TODO define type for list of domains numbers
  SNetProtocols * pProtocols;
  SIGPDomain    * pDomain;
  float           fLatitude;
  float           fLongitude;
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
 *
 * The arguments of FNetLinkForward are as follows:
 *   tPhysAddr: physical address of the next-hop on the link (only used
 *              on point-to-multipoint links)
 *   pContext : the link context
 *   ppNextHop: the node to which the message shall be forwarded
 */
typedef int (*FNetLinkForward)(net_addr_t tPhysAddr,
			       void * pContext,
			       SNetNode ** ppNextHop,
			       SNetMessage ** ppMsg);

// ----- FNetLinkHandle ---------------------------------------------
typedef void (*FNetLinkHandle)();

// -----[ FNetLinkDestroy ]------------------------------------------
/**
 * This type defines a callback that is used to destroy the context
 * of a link. Providing this function is optional.
 */
typedef void (*FNetLinkDestroy)(void * pContext);

// --- link destination type ---
/**
 * A link can bind a node to another node or a to a subnet. In the
 * first case, the link's destination is an IP address. In the second
 * case, the destination is a pointer to the subnet.
 */
typedef union {
  SNetNode * pNode;      // ptp links
  SNetSubnet * pSubnet;  // ptmp links
  net_addr_t tEndPoint;  // tunnels
} net_link_dst_t;

// ----- SNetLink --------------------------------------------
/**
 * This type defines a link object. A link is attached to a
 * source node. On this node, the link is identified by the
 * interface IP address (tIfaceAddr) and by the length of the
 * mask.
 */
typedef struct {
  uint8_t            uType;      // Type of link
                                 //   ptp : NET_LINK_TYPE_ROUTER
                                 //   tun : NET_LINK_TYPE_TUNNEL
                                 //   ptmp: NET_LINK_TYPE_TRANSIT/STUB

  // --- Identification of source ---
  SNetNode         * pSrcNode;   // Node to which the link is attached
  net_addr_t         tIfaceAddr; // Local interface address:
                                 //   ptp: remote destination addr
                                 //   ptmp: local interface addr
                                 //   tun: local interface addr
  net_mask_t         tIfaceMask; // Local interface mask
                                 //   ptp,tun: 32
                                 //   ptmp: <32

  // --- Identification of destination ---
  net_link_dst_t     tDest;      // Depends on uType
                                 //   ptp,tun: addr
                                 //   ptmp: pointer to subnet

  // --- Physical attributes ---
  net_link_delay_t   tDelay;     // Propagation delay
  net_link_load_t    tCapacity;  // Link capacity
  net_link_load_t    tLoad;      // Link load
  uint8_t            uFlags;     // Flags: state (up/down), IGP_ADV

  // --- IGP protocol attributes ---
  SNetIGPWeights   * pWeights;   // List of IGP weights (1/topo)

  // --- Context and methods ---
  void             * pContext;
  FNetLinkForward    fForward;   // Send message function
  FNetLinkHandle     fHandle;    // Recv message function
  FNetLinkDestroy    fDestroy;

#ifdef OSPF_SUPPORT
  ospf_area_t      tArea;  
#endif
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
