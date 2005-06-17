// ==================================================================
// @(#)link.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 24/02/2004
// @lastdate 09/03/2004
// ==================================================================

#ifndef __NET_LINK_H__
#define __NET_LINK_H__

#include <stdlib.h>
#include <net/subnet.h>
#include <net/message.h>
#include <net/prefix.h>
#include <libgds/types.h>
#include <libgds/array.h>

#define NET_LINK_DELAY_INT
#define NET_LINK_FLAG_UP      0x01
#define NET_LINK_FLAG_NOTIFY  0x02
#define NET_LINK_FLAG_IGP_ADV 0x04
#define NET_LINK_FLAG_TUNNEL  0x08

// Link type distiguishs between Router, Transit Network and
// Stub Network to compute IGP route in OSPF
#define NET_LINK_TYPE_ROUTER   0
#define NET_LINK_TYPE_TRANSIT  1
#define NET_LINK_TYPE_STUB     2

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
  uint8_t uFlags;
  uint32_t uIGPweight;      // Store IGP weight here
  void * pContext;
  FNetLinkForward fForward;
} SNetLink;


// ----- link_get_addr ----------------------------------------
extern net_addr_t link_get_addr(SNetLink * pLink);
// ----- link_get_addr ----------------------------------------
extern net_addr_t link_get_addr(SNetLink * pLink);
// ----- link_set_type ----------------------------------------
extern void link_set_type(SNetLink * pLink, uint8_t uType);
// ----- link_get_type ----------------------------------------
extern int link_check_type(SNetLink * pLink, uint8_t uType);
// ----- link_set_state ---------------------------------------------
extern void link_set_state(SNetLink * pLink, uint8_t uFlag, int iState);
// ----- link_get_state ---------------------------------------------
extern int link_get_state(SNetLink * pLink, uint8_t uFlag);
// ----- link_set_igp_weight ----------------------------------------
extern void link_set_igp_weight(SNetLink * pLink, uint32_t uIGPweight);
// ----- link_get_igp_weight ----------------------------------------
extern uint32_t link_get_igp_weight(SNetLink * pLink);
// ----- link_dump --------------------------------------------------
extern void link_dump(FILE * pStream, SNetLink * pLink);




// ----- create_link_Router ----------------------------------------
extern SNetLink * create_link_toRouter(SNetNode * pNode);
// ----- create_link_Router ----------------------------------------
extern SNetLink * create_link_toRouter_byAddr(net_addr_t tAddr);
// ----- create_link_Subnet ----------------------------------------
extern SNetLink * create_link_toSubnet(SNetSubnet * pSubnet);


#endif
