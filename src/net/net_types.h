// ==================================================================
// @(#)net_types.h
//
// @author Stefano Iasi (stefanoia@tin.it)
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 30/06/2005
// $Id: net_types.h,v 1.17 2008-06-13 14:26:23 bqu Exp $
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

typedef SPtrArray net_ifaces_t;
typedef SPtrArray net_subnets_t;

typedef uint32_t net_link_delay_t;
#define NET_LINK_DELAY_INFINITE UINT32_MAX
typedef uint32_t net_link_load_t;
#define NET_LINK_MAX_CAPACITY UINT32_MAX
#define NET_LINK_MAX_LOAD UINT32_MAX

typedef struct {
  float latitude;    
  float longitude;
} coord_t;

//////////////////////////////////////////////////////////////////////
//
// MESSAGE STRUCTURE
//
//////////////////////////////////////////////////////////////////////

// Protocol numbers
typedef enum {
  NET_PROTOCOL_RAW,
  NET_PROTOCOL_ICMP,
  NET_PROTOCOL_BGP,
  NET_PROTOCOL_IPIP,
  NET_PROTOCOL_MAX,
} net_protocol_id_t;

// ----- FPayLoadDestroy -----
typedef void (*FPayLoadDestroy)(void ** pPayLoad);

typedef struct {
  FPayLoadDestroy destroy;
} net_msg_ops_t;

// -----[ net_msg_t ]-----
typedef struct {
  net_addr_t        src_addr;
  net_addr_t        dst_addr;
  net_protocol_id_t protocol;
  uint8_t           ttl;
  net_tos_t         tos;
  void            * payload;
  net_msg_ops_t     ops;
} net_msg_t;

// -----[ FNetProtoHandleEvent ]-----
typedef int (*FNetProtoHandleEvent)(simulator_t * sim,
				    void * handler,
				    net_msg_t * msg);
// ----- FNetProtoHandlerDestroy -----
typedef void (*FNetProtoHandlerDestroy)(void ** handler_ref);
// -----[ FNetProtoDump ]-----
typedef void (*FNetProtoDump)(SLogStream * stream,
			      void * handler,
			      net_msg_t * msg);

// -----[ net_protocol_ops_t ]-----
typedef struct {
  int  (*handle)  (simulator_t * sim, void * handler, net_msg_t * msg);
  void (*destroy) (void ** handler_ref);
  void (*dump)    (SLogStream * stream, void * handler, net_msg_t * msg);
} net_protocol_ops_t;

// -----[ net_protocol_t ]-----
typedef struct {
  net_protocol_id_t    id;
  const char         * name;
  void               * handler;
  net_protocol_ops_t   ops;
} net_protocol_t;

// -----[ net_protocols_t ]-----
typedef SPtrArray net_protocols_t;


//////////////////////////////////////////////////////////////////////
//
// MAIN NETWORK STRUCTURE
//
//////////////////////////////////////////////////////////////////////

typedef struct {
  STrie         * nodes;      /* Global node index (key=IP address) */
  net_subnets_t * subnets;    /* Global subnet index (key=IP prefix) */
  simulator_t   * sim;
} network_t;


//////////////////////////////////////////////////////////////////////
//
// IGP DOMAIN TYPE DEFINITION
//
//////////////////////////////////////////////////////////////////////

// Supported domain types
typedef enum {
  IGP_DOMAIN_IGP,
  IGP_DOMAIN_OSPF,
IGP_DOMAIN_MAX
} igp_domain_type_t;

typedef struct {
  uint16_t            id;
  char              * name;
  STrie             * routers;
  igp_domain_type_t   type;
  uint8_t             ecmp;
} igp_domain_t;


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
  ospf_dest_type_t   tOSPFDestinationType;
  ip_pfx_t           sPrefix;
  net_link_delay_t   uWeight;
  ospf_area_t        tOSPFArea;
  ospf_path_type_t   tOSPFPathType;
  next_hops_list_t * aNextHops;
  net_route_type_t   tType;
} SOSPFRouteInfo;

typedef STrie SOSPFRT;


//////////////////////////////////////////////////////////////////////
//
// NODE TYPE DEFINITION
//
//////////////////////////////////////////////////////////////////////

// ----- net_node_t ---------------------------------------------------
/**
 * This type defines a node object.
 */
typedef struct {
  net_addr_t        addr;
  char            * name;
  network_t       * network;
  net_ifaces_t    * ifaces;
  net_rt_t        * rt;
#ifdef OSPF_SUPPORT              /* TODO: should be moved to another
				  * data structure */
  SUInt32Array    * pOSPFAreas; 
  SOSPFRT         * pOspfRT;     /* OSPF routing table */
//  net_addr_t      ospf_id;
#endif
  SUInt16Array    * pIGPDomains; /* TODO: define type for list of domains */
  net_protocols_t * protocols;   /* Supported protocol handlers */
  igp_domain_t    * domain;
  coord_t           coord;       /* Geographical coordinates */
  int               syslog_enabled;
} net_node_t;


//////////////////////////////////////////////////////////////////////
//
// SUBNET TYPE DEFINITION
//
//////////////////////////////////////////////////////////////////////

// -----[ net_subnet_t ]---------------------------------------------
/**
 * Describe transit or stub networks: if there is only one router this
 * object describe a stub network. A transit network otherwise.
 */
typedef enum {
  NET_SUBNET_TYPE_TRANSIT,
  NET_SUBNET_TYPE_STUB,
  NET_SUBNET_TYPE_MAX
} net_subnet_type_t;

typedef struct {
  ip_pfx_t            prefix;
  net_subnet_type_t   type;
  net_ifaces_t      * ifaces;    /* links towards routers */
#ifdef OSPF_SUPPORT
  ospf_area_t         uOSPFArea; /* a subnetwork belongs to a single area */
#endif
} net_subnet_t;


//////////////////////////////////////////////////////////////////////
//
// LINK TYPE DEFINITION
//
//////////////////////////////////////////////////////////////////////

// ----- Interface types -----
typedef enum {
  NET_IFACE_LOOPBACK,
  NET_IFACE_RTR,
  NET_IFACE_PTP,
  NET_IFACE_PTMP,
  NET_IFACE_VIRTUAL,
  NET_IFACE_MAX,
} net_iface_type_t;

// -----[ FNetIfaceSend ]--------------------------------------------
/**
 * This type defines a callback that is used to forward messages along
 * links of different types.
 * - RTR and PTP ifaces
 * - PTMP ifaces
 * - VIRTUAL ifaces (tunnels)
 *
 * The arguments of FNetIfaceSend are as follows:
 *   tPhysAddr: physical address of the next-hop on the link (only used
 *              on point-to-multipoint links)
 *   pContext : the link context
 *   ppDstIface: the destination's interface
 */
struct net_iface_t;
typedef int (*FNetIfaceSend)(struct net_iface_t * self,
			     net_addr_t l2_addr,
			     net_msg_t * msg);

// -----[ FNetIfaceRecv ]--------------------------------------------
typedef int (*FNetIfaceRecv)(struct net_iface_t * self,
			     net_msg_t * msg);

// -----[ FNetLinkDestroy ]------------------------------------------
/**
 * This type defines a callback that is used to destroy the context
 * of a link. Providing this function is optional.
 */
typedef void (*FNetLinkDestroy)(void * pContext);

// ---[ Interface destination types ]---
/**
 * A link can bind a node to another node or a to a subnet. In the
 * first case, the link's destination is an IP address. In the second
 * case, the destination is a pointer to the subnet.
 */
typedef union {
  struct net_iface_t * iface;     /* rtr, ptp links */
  net_subnet_t       * subnet;    /* ptmp links */
  net_addr_t           end_point; /* tunnel endpoint */
} net_iface_dst_t;

// -----[ Network interface operations ]-----------------------------
typedef struct {
  FNetIfaceSend   send;    /* Send message function */
  FNetIfaceRecv   recv;    /* Recv message function */
  FNetLinkDestroy destroy; /* Message destroy function */
} net_iface_ops_t;

typedef struct {
  net_link_delay_t delay;    /* Propagation delay */
  net_link_load_t  capacity; /* Link capacity */
  net_link_load_t  load;     /* Link load */
} net_iface_phys_t;

// -----[ net_iface_t ]----------------------------------------------
/**
 * This type defines a link object. A link is attached to a
 * source node. On this node, the link is identified by the
 * interface IP address (tIfaceAddr) and by the length of the
 * mask.
 */
typedef struct net_iface_t {
  net_iface_type_t   type;       // Type of interface
  
  // --- Identification of source ---
  net_node_t       * src_node;   // Node to which the link is attached
  net_addr_t         tIfaceAddr; // Local interface address:
                                 //   ptp: remote destination addr
                                 //   ptmp: local interface addr
                                 //   tun: local interface addr
  net_mask_t         tIfaceMask; // Local interface mask
                                 //   ptp,tun: 32
                                 //   ptmp: <32
  uint8_t            connected;  // Tells if the interface is connected

  // --- Identification of destination ---
  net_iface_dst_t    dest;       // Depends on tType

  // --- Physical attributes ---
  net_iface_phys_t   phys;
  uint8_t            flags;      // Flags: state (up/down), IGP_ADV

  // --- IGP protocol attributes ---
  igp_weights_t    * pWeights;   // List of IGP weights (1/topo)

  void             * user_data;
  net_iface_ops_t    ops;

#ifdef OSPF_SUPPORT
  ospf_area_t      tArea;  
#endif
} net_iface_t;


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
  net_iface_t * dst_iface;
  net_msg_t   * msg;
} net_send_ctx_t;

typedef enum {
  NODE,
  SUBNET
} net_elem_type_t;

typedef struct {
  net_elem_type_t   type;      /* item type: node / subnet */
  union {
    net_node_t      * node;
    net_subnet_t    * subnet;
  };
} net_elem_t;

#endif /* __NET_TYPES_H__ */
