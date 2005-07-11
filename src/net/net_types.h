// ==================================================================
// @(#)net_types.h
//
// @author Stefano Iasi (stefanoia@tin.it)
// @date 30/06/2005
// @lastdate 30/06/2005
// ==================================================================

#ifndef __NET_TYPES_H__
#define __NET_TYPES_H__

#include <libgds/array.h>
#include <net/prefix.h>
#include <net/message.h>
#include <net/routing_t.h>

typedef uint8_t   ospf_dest_type_t;
typedef uint32_t  ospf_area_t;
typedef uint8_t   ospf_path_type_t;
typedef SPtrArray next_hops_list_t;
typedef SPtrArray SOSPFRouteInfoList;

typedef SPtrArray links_list_t;

//////////////////////////////////////////////////////////////////////
////// Subnet type definition
//////////////////////////////////////////////////////////////////////

/*
  Describe transit network or stub network: if there are only one 
  router this object describe a stub network. A trasit network
  otherwise.
*/
typedef struct {
  //net_addr_t tAddr;
  SPrefix        sPrefix;
  uint8_t        uType;
  ospf_area_t    uOSPFArea;	/*we have only an area for a network*/
//   SPtrArray *  aNodes;   /*links towards all the router on the subnet*/
  links_list_t * pLinks;     /*links towards routers*/
} SNetSubnet;


//////////////////////////////////////////////////////////////////////
////// Link type definition
//////////////////////////////////////////////////////////////////////
#define NET_LINK_DELAY_INT

#ifdef NET_LINK_DELAY_INT
typedef uint32_t net_link_delay_t;
#define NET_LINK_DELAY_INFINITE MAX_UINT32_T
#else
typedef double net_link_delay_t;
#endif


typedef int (*FNetLinkForward)(net_addr_t tDstAddr, void * pContext,
			       SNetMessage * pMsg);


/*
  A link can point to a router or a subnet.
  Router can be reached in topology db by IP address (tAddr),
  Subnet can be reached in topology db by memory pointer.
  
  To obtain IP address MUST BE USED link_get_addr() function
*/
typedef union {
  net_addr_t tAddr;
  SNetSubnet * pSubnet;
} UDestinationId;

typedef struct {
  uint8_t uDestinationType;    /* TNET_LINK_TYPE_ROUTER , NET_LINK_TYPE_TRANSIT, NET_LINK_TYPE_STUB */
  //net_addr_t tAddr;
  UDestinationId UDestId;      /* depends on uDestinationType*/
  net_link_delay_t tDelay;
  uint8_t          uFlags;
  uint32_t         uIGPweight;      // Store IGP weight here
  void * pContext;
  FNetLinkForward fForward;
} SNetLink;


/*
   Assumption: pSubNet is valid only for destination type Subnet.
   The filed can be used only if pPrefix has a 32 bit netmask.
*/

typedef struct {
  SNetLink * pLink;  //link towards next-hop
  net_addr_t tAddr;  //ip address of next hop
  //TODO add advertising router
} SOSPFNextHop;

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

#ifdef __EXPERIMENTAL__
typedef STrie SOSPFRT;
#else
typedef SRadixTree SOSPFRT;
#endif


#endif

