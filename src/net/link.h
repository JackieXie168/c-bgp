// ==================================================================
// @(#)link.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be),
//         Stefano Iasi  (stefanoia@tin.it)
// @date 24/02/2004
// @lastdate 09/03/2004
// ==================================================================

#ifndef __NET_LINK_H__
#define __NET_LINK_H__

#include <stdlib.h>
#include <net/net_types.h>
#include <net/message.h>
#include <net/prefix.h>
#include <net/network_t.h>
#include <libgds/types.h>
#include <libgds/array.h>


#define NET_LINK_FLAG_UP      0x01
#define NET_LINK_FLAG_NOTIFY  0x02
#define NET_LINK_FLAG_IGP_ADV 0x04
#define NET_LINK_FLAG_TUNNEL  0x08

// Link type distiguishs between Router, Transit Network and
// Stub Network to compute IGP route in OSPF
#define NET_LINK_TYPE_ROUTER   0
#define NET_LINK_TYPE_TRANSIT  1
#define NET_LINK_TYPE_STUB     2




// ----- link_get_addr ----------------------------------------
extern net_addr_t link_get_addr(SNetLink * pLink);
// ----- link_get_prefix ----------------------------------------
extern void link_get_prefix(SNetLink * pLink, SPrefix * pPrefix);
// ----- link_get_subnet ----------------------------------------
extern SNetSubnet * link_get_subnet(SNetLink * pLink);
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
