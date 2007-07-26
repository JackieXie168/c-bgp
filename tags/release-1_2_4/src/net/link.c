// ==================================================================
// @(#)link.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @author Stefano Iasi (stefanoia@tin.it)
// @date 24/02/2004
// @lastdate 12/09/2006
// ==================================================================

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <net/net_types.h>
#include <net/network.h>
#include <net/subnet.h>
#include <net/link.h>
#include <net/ospf.h>
#include <net/node.h>
#include <libgds/log.h>
#include <libgds/memory.h>

/////////////////////////////////////////////////////////////////////
//
// LINK METHODS
//
//////////////////////////////////////////////////////////////

// ----- create_link_toRouter ---------------------------------------
/**
 * Note from bqu:
 *  - function name does not follow CODING CONVENTIONS !
 */
int create_link_toRouter(SNetNode * pSrcNode,
			SNetNode * pDstNode,
			SNetLink ** ppLink)
{
  if (pSrcNode == pDstNode)
    return NET_ERROR_MGMT_LINK_LOOP;

  return create_link_toRouter_byAddr(pSrcNode, pDstNode->tAddr, ppLink);
}

// ----- create_link_toSubnet ---------------------------------
/**
 * Note from bqu:
 *  - function name does not follow CODING CONVENTIONS !
 */
SNetLink * create_link_toSubnet(SNetNode * pSrcNode,
				SNetSubnet * pSubnet,
				net_addr_t tIfaceAddr)
{
  SNetLink * pLink= (SNetLink *) MALLOC(sizeof(SNetLink));
  if (subnet_is_transit(pSubnet))
    pLink->uDestinationType= NET_LINK_TYPE_TRANSIT;
  else 
    pLink->uDestinationType= NET_LINK_TYPE_STUB;
  pLink->UDestId.pSubnet= pSubnet;
  pLink->pSrcNode= pSrcNode;
  pLink->tIfaceAddr= tIfaceAddr;
  pLink->pContext= pLink;
  pLink->fForward= _subnet_forward;
  pLink->uIGPweight= UINT_MAX;
  pLink->tDelay= 0;
  pLink->uFlags= NET_LINK_FLAG_UP | NET_LINK_FLAG_IGP_ADV;

#ifdef OSPF_SUPPORT
  pLink->tArea= OSPF_NO_AREA;
#endif

  return pLink;
}

// ----- create_link_toAny -----------------------------------
/**
 * Note from bqu:
 *  - function name does not follow CODING CONVENTIONS !
 */
SNetLink * create_link_toAny(SPrefix * pPrefix)
{
  SNetLink * pLink = (SNetLink *) MALLOC(sizeof(SNetLink));
  pLink->uDestinationType = NET_LINK_TYPE_ANY;
  (pLink->UDestId).pSubnet = subnet_create(pPrefix->tNetwork,
					   pPrefix->uMaskLen,
					   NET_SUBNET_TYPE_TRANSIT);
  return pLink;
}

//  ----- create_link_toRouter_byAddr ------------------------
/**
 * Note from bqu:
 *  - function name does not follow CODING CONVENTIONS !
 */
int create_link_toRouter_byAddr(SNetNode * pSrcNode,
				net_addr_t tAddr,
				SNetLink ** ppLink)
{
  SNetLink * pLink;

  // Check that endpoints addresses are different
  if (pSrcNode->tAddr == tAddr)
    return NET_ERROR_MGMT_LINK_LOOP;

  pLink= (SNetLink *) MALLOC(sizeof(SNetLink));
  pLink->uDestinationType = NET_LINK_TYPE_ROUTER;
  pLink->UDestId.tAddr= tAddr;
  pLink->pSrcNode= pSrcNode;

  // *** Needs to be fixed ***
  // If we allow multiple parallel links, this interface address must
  // become a unique interface address on this side of the link
  // (src-node). For the moment, this interface address is set to the
  // src-node's address. Multiple parallel links are therefore not
  // allowed untils this is fixed.
  pLink->tIfaceAddr= pSrcNode->tAddr;

  // The interface address can _NOT_ be the dst-node's address since
  // this will cause the source of messages to become incorrect. I
  // detected this with BGP message sending. The destination peer
  // detected that the BGP message was received from an unknown peer
  // (itself).
  // See validation scr

  pLink->pContext= pLink;
  pLink->fForward= _link_forward;
  pLink->uIGPweight= UINT_MAX;
  pLink->tDelay= 0;
  pLink->uFlags= NET_LINK_FLAG_UP | NET_LINK_FLAG_IGP_ADV;

#ifdef OSPF_SUPPORT
  pLink->tArea = OSPF_NO_AREA;
#endif

  *ppLink= pLink;
  return NET_SUCCESS;
}

// ----- link_get_address ------------------------------------
net_addr_t link_get_address(SNetLink * pLink)
{
  if (pLink->uDestinationType == NET_LINK_TYPE_ROUTER ) {
    return (pLink->UDestId).tAddr;
  } else {
    assert(pLink->UDestId.pSubnet != NULL);
    return ((pLink->UDestId).pSubnet->sPrefix).tNetwork;
  }
}
// ----- link_get_id -------------------------------------
net_addr_t link_get_id(SNetLink * pLink)
{
  if (pLink->uDestinationType == NET_LINK_TYPE_ROUTER ) {
    return (pLink->UDestId).tAddr;
  } else {
    return pLink->tIfaceAddr;
  }
}
// ----- link_get_iface -------------------------------------
net_addr_t link_get_iface(SNetLink * pLink)
{
//  if (pLink->uDestinationType == NET_LINK_TYPE_ROUTER ) {
//    return (pLink->UDestId).tAddr;
//  } else {
    return pLink->tIfaceAddr;
//  }
}


// ----- link_find_backword --------------------------------------------
SNetLink * link_find_backword(SNetLink * pLink)
{
  SNetNode * pPeer;
  SNetLink * pBckLink = NULL;
  switch(pLink->uDestinationType)
    {
	case NET_LINK_TYPE_ROUTER :
	  pPeer = network_find_node((pLink->UDestId).tAddr);
          pBckLink = node_find_link_to_router(pPeer, pLink->pSrcNode->tAddr);
          break;

	case NET_LINK_TYPE_TRANSIT :
	case NET_LINK_TYPE_STUB    :
	  pBckLink = pLink;
	  break;
  }
  return pBckLink; 
}

// ----- link_set_ip_prefix ----------------------------------
/*
 * Can be used on a link to router to set an ip prefix 
 * or to set only and interface ip on the link.
 * If passed prefix has a field uMaskLen == 32 this 
 * command specifies only an interface for the link.
 * 
 * ASSUME: 
 * - link is towards a ROUTER
 * - link prefix is the same in the other direction!
 * - link prefix is /31
 */
void link_set_ip_prefix(SNetLink * pLink, SPrefix sPrefix)
{
  pLink->tIfaceAddr = sPrefix.tNetwork;
  if (sPrefix.uMaskLen == 32)
  {
    pLink->uFlags |= NET_LINK_FLAG_IFACE;
  }
  else
  {
    pLink->uFlags |= NET_LINK_FLAG_IFACE;
    pLink->uFlags |= NET_LINK_FLAG_PREFIX;
  }
}
// ----- link_set_ip_iface -----------------------------------
void link_set_ip_iface(SNetLink * pLink, net_addr_t tIfaceAddr)
{
  pLink->tIfaceAddr = tIfaceAddr;
  pLink->uFlags |= NET_LINK_FLAG_IFACE;
}
// ----- link_destroy ----------------------------------------
void link_destroy(SNetLink ** ppLink)
{
  if (*ppLink != NULL) {
    /*if ((*ppLink)->uDestinationType == NET_LINK_TYPE_ANY)
      subnet_destroy(&(((*ppLink)->UDestId).pSubnet));*/
    FREE(*ppLink);
    *ppLink= NULL;
  }
}

// ----- link_get_ip_prefix ----------------------------------
// Return ip prefix assciated to link and used in routing 
// process.
// If link is towards subnet prefix is the same as the subnet.
// If link is towards router 
//  - if a prefix is configured (NET_LINK_FLAG_PREFIX)
//       return configured prefix /31
//    else if there is an interface configured
//       return inetrface/32 prefix
//    else
//       return destination_ip/32 predfix
//  - if only an interface is configured 
//       return interfaceIp/32 prefix
//
SPrefix link_get_ip_prefix(SNetLink * pLink)
{ 
  SPrefix sPrefix;
  if (pLink->uDestinationType == NET_LINK_TYPE_ROUTER ) {
    if (pLink->uFlags & NET_LINK_FLAG_PREFIX){ 
      sPrefix.tNetwork = pLink->tIfaceAddr;
      sPrefix.uMaskLen = 30;
      ip_prefix_mask(&sPrefix);
    } 
    else if (pLink->uFlags & NET_LINK_FLAG_IFACE){
      sPrefix.tNetwork = pLink->tIfaceAddr;
      sPrefix.uMaskLen = 32; 
      ip_prefix_mask(&sPrefix);
    }
    else{
      sPrefix.tNetwork = (pLink->UDestId).tAddr;
      sPrefix.uMaskLen = 32; 
      ip_prefix_mask(&sPrefix);
    }
  }
  else {
    assert(pLink->UDestId.pSubnet != NULL);
    return ((pLink->UDestId).pSubnet->sPrefix);
  }
  return sPrefix;
}

// ----- link_to_router_has_ip_prefix ------------------------
/* 
 * ASSUME: function is invoked on a link towards a ROUTER
 * */
int link_to_router_has_ip_prefix(SNetLink * pLink) { 
  return (pLink->uFlags & NET_LINK_FLAG_PREFIX);
}

// ----- link_to_router_has_only_iface -----------------------
/* 
 * ASSUME: function is invoked on a link towards a ROUTER
 */
int link_to_router_has_only_iface(SNetLink * pLink){ 
  return ((pLink->uFlags & NET_LINK_FLAG_IFACE) && 
		 !(pLink->uFlags & NET_LINK_FLAG_PREFIX)) ;
}

// ----- link_get_prefix -------------------------------------
void link_get_prefix(SNetLink * pLink, SPrefix * pPrefix)
{
  if (pLink->uDestinationType == NET_LINK_TYPE_ROUTER ) {
    pPrefix->tNetwork= (pLink->UDestId).tAddr;
    pPrefix->uMaskLen= 32;
  } else {
    assert(pLink->UDestId.pSubnet != NULL);
    pPrefix->tNetwork= ((pLink->UDestId).pSubnet->sPrefix).tNetwork;
    pPrefix->uMaskLen= ((pLink->UDestId).pSubnet->sPrefix).uMaskLen;// tAddr;
  }
}


// ----- link_set_type ---------------------------------------
void link_set_type(SNetLink * pLink, uint8_t uDestinationType)
{
  pLink->uDestinationType= uDestinationType;
}

// ----- link_set_state --------------------------------------
void link_set_state(SNetLink * pLink, uint8_t uFlag, int iState)
{
  if (iState)
    pLink->uFlags|= uFlag;
  else
    pLink->uFlags&= ~uFlag;
}

// ----- link_get_state --------------------------------------
int link_get_state(SNetLink * pLink, uint8_t uFlag)
{
  return (pLink->uFlags & uFlag) != 0;
}

// ----- link_set_igp_weight ---------------------------------
/**
 *
 */
void link_set_igp_weight(SNetLink * pLink, uint32_t uIGPweight)
{
  pLink->uIGPweight= uIGPweight;
}

// ----- link_get_igp_weight ---------------------------------
/**
 *
 */
uint32_t link_get_igp_weight(SNetLink * pLink)
{
  return pLink->uIGPweight;
}

// ----- link_get_subnet --------------------------------------------
SNetSubnet * link_get_subnet(SNetLink * pLink){
  if (pLink->uDestinationType != NET_LINK_TYPE_ROUTER)
    return (pLink->UDestId).pSubnet;
  return NULL;
}
/*
UNetDest link_get_dest(SNetLink * pLink){
  UNetDest uDest;
  if (pLink->uDestinationType == NET_LINK_TYPE_ROUTER)
    uDest.sPrefix = pLink->sPrefix;
  else 
    uDest.sPrefix = pLink->UDestId.tAddr;
}*/


/////////////////////////////////////////////////////////////////////
//
// FORWARDING METHODS
//
/////////////////////////////////////////////////////////////////////

// ----- _link_forward ----------------------------------------------
/**
 * Forward a message along a point-to-point link. The next-hop
 * information is not used.
 */
int _link_forward(net_addr_t tNextHop, void * pContext,
		  SNetNode ** ppNextHop)
{
  SNetLink * pLink= (SNetLink *) pContext;

  // Check link's state
  if (!link_get_state(pLink, NET_LINK_FLAG_UP))
    return NET_ERROR_LINK_DOWN;

  // Node lookup: O(log(n))
  *ppNextHop= network_find_node(pLink->UDestId.tAddr);
  
  return NET_SUCCESS;
}

// ----- _link_drop --------------------------------------------------
/**
 * Drop a message with an error message on STDERR.
 */
void _link_drop(SNetLink * pLink, SNetMessage * pMsg)
{
  LOG_ERR_ENABLED(LOG_LEVEL_SEVERE) {
    log_printf(pLogErr, "*** \033[31;1mMESSAGE DROPPED\033[0m ***\n");
    log_printf(pLogErr, "message: ");
    message_dump(pLogErr, pMsg);
    //if (pMessage->uProtocol == NET_PROTOCOL_BGP)
    //  bgp_msg_dump(pLogErr, NULL, pMessage->pPayLoad);
    log_printf(pLogErr, "\nlink   : ");
    link_dst_dump(pLogErr, pLink);
    log_printf(pLogErr, "\n");
  }
  message_destroy(&pMsg);
}


/////////////////////////////////////////////////////////////////////
//
// DUMP METHODS
//
/////////////////////////////////////////////////////////////////////

// ----- link_dst_dump ----------------------------------------------
/**
 * Show the link's destination. If the link heads towards a router,
 * the destination is an IP address. Otherwise, if it heads towards a
 * subnet (transit/stub), the destination is an IP prefix.
 */
void link_dst_dump(SLogStream * pStream, SNetLink * pLink)
{
  SPrefix sPrefix;

  if (pLink->uDestinationType == NET_LINK_TYPE_ROUTER) {
    ip_address_dump(pStream, link_get_address(pLink));
  } else {
    link_get_prefix(pLink, &sPrefix);
    ip_prefix_dump(pStream, sPrefix);
  }
}

// ----- link_dump --------------------------------------------------
/**
 * Show the attributes of a link.
 *
 * Syntax:
 *   <type> <destination> <weight> <delay> <state>
 *     [<tunnel>] [<igp-adv>] [<ospf-area>]
 * where
 *   <type> is one of 'ROUTER', 'TRANSIT' or 'STUB'
 *   <destination> is an IP address (router) or an IP prefix (transit/stub)
 *   <weight> is the link weight
 *   <delay> is the link delay
 *   <state> is either UP or DOWN
 *   <tunnel> is one of 'DIRECT' or 'TUNNEL'
 *   <igp-adv> is either YES or NO
 *   <ospf-area> is an integer
 */
void link_dump(SLogStream * pStream, SNetLink * pLink)
{
  SPrefix sPrefix;

  /* Link type & destination address:
     - router: IP address
     - transit: IP prefix
     - subnet: IP prefix
   */
  if (pLink->uDestinationType == NET_LINK_TYPE_ROUTER) {
    log_printf(pStream, "ROUTER\t");
    ip_address_dump(pStream, link_get_address(pLink));
  } else {
    if (pLink->uDestinationType == NET_LINK_TYPE_TRANSIT) {
      log_printf(pStream, "TRANSIT\t");
    } else {
      log_printf(pStream, "STUB\t");
    }
    link_get_prefix(pLink, &sPrefix);
    ip_prefix_dump(pStream, sPrefix);
  }

  /* Link weight & delay */
  log_printf(pStream, "\t%u\t%u", pLink->tDelay, pLink->uIGPweight);

  /* Link state (up/down) */
  if (link_get_state(pLink, NET_LINK_FLAG_UP))
    log_printf(pStream, "\tUP");
  else
    log_printf(pStream, "\tDOWN");

  /* Optional attributes */
  if (link_get_state(pLink, NET_LINK_FLAG_TUNNEL))
    log_printf(pStream, "\ttunnel");
  if (link_get_state(pLink, NET_LINK_FLAG_IGP_ADV))
    log_printf(pStream, "\tadv:yes");

#ifdef OSPF_SUPPORT
  if (pLink->tArea != OSPF_NO_AREA)
    log_printf(pStream, "\tarea:%u", pLink->tArea);
#endif
}