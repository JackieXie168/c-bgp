// ==================================================================
// @(#)link.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @author Stefano Iasi (stefanoia@tin.it)
// @date 24/02/2004
// @lastdate 01/09/2005
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
#include <libgds/memory.h>

/////////////////////////////////////////////////////////////////////
//
// LINK METHODS
//
/////////////////////////////////////////////////////////////////////

// ----- create_link_toRouter ---------------------------------------
SNetLink * create_link_toRouter(SNetNode * pSrcNode,
				SNetNode * pDstNode)
{
  return create_link_toRouter_byAddr(pSrcNode, pDstNode->tAddr);
}

// ----- create_link_toSubnet ---------------------------------------
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

// ----- create_link_toAny ------------------------------------------
SNetLink * create_link_toAny(SPrefix * pPrefix)
{
  SNetLink * pLink = (SNetLink *) MALLOC(sizeof(SNetLink));
  pLink->uDestinationType = NET_LINK_TYPE_ANY;
  (pLink->UDestId).pSubnet = subnet_create(pPrefix->tNetwork,
					   pPrefix->uMaskLen,
					   NET_SUBNET_TYPE_TRANSIT);
  return pLink;
}

//  ----- create_link_toRouter_byAddr -------------------------------
SNetLink * create_link_toRouter_byAddr(SNetNode * pSrcNode,
				       net_addr_t tAddr)
{
  SNetLink * pLink= (SNetLink *) MALLOC(sizeof(SNetLink));
  pLink->uDestinationType = NET_LINK_TYPE_ROUTER;
  pLink->UDestId.tAddr= tAddr;
  pLink->pSrcNode= pSrcNode;
  pLink->tIfaceAddr= 0;
  pLink->pContext= pLink;
  pLink->fForward= _link_forward;
  pLink->uIGPweight= UINT_MAX;
  pLink->tDelay= 0;
  pLink->uFlags= NET_LINK_FLAG_UP | NET_LINK_FLAG_IGP_ADV;

#ifdef OSPF_SUPPORT
  pLink->tArea = OSPF_NO_AREA;
#endif

  return pLink;
}

// ----- link_get_address -------------------------------------------
net_addr_t link_get_address(SNetLink * pLink)
{
  if (pLink->uDestinationType == NET_LINK_TYPE_ROUTER ) {
    return (pLink->UDestId).tAddr;
  } else {
    assert(pLink->UDestId.pSubnet != NULL);
    return ((pLink->UDestId).pSubnet->sPrefix).tNetwork;
  }
}

// ----- link_get_iface --------------------------------------------
net_addr_t link_get_iface(SNetLink * pLink)
{
  if (pLink->uDestinationType == NET_LINK_TYPE_ROUTER ) {
    return (pLink->UDestId).tAddr;
  } else {
    return pLink->tIfaceAddr;
  }
}	

// ----- link_destroy -----------------------------------------------
void link_destroy(SNetLink ** ppLink)
{
  if (*ppLink != NULL) {
    /*if ((*ppLink)->uDestinationType == NET_LINK_TYPE_ANY)
      subnet_destroy(&(((*ppLink)->UDestId).pSubnet));*/
    FREE(*ppLink);
    *ppLink= NULL;
  }
}

// ----- link_get_prefix --------------------------------------------
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

// ----- link_set_type ----------------------------------------------
void link_set_type(SNetLink * pLink, uint8_t uDestinationType)
{
  pLink->uDestinationType= uDestinationType;
}

// ----- link_set_state ---------------------------------------------
void link_set_state(SNetLink * pLink, uint8_t uFlag, int iState)
{
  if (iState)
    pLink->uFlags|= uFlag;
  else
    pLink->uFlags&= ~uFlag;
}

// ----- link_get_state ---------------------------------------------
int link_get_state(SNetLink * pLink, uint8_t uFlag)
{
  return (pLink->uFlags & uFlag) != 0;
}

// ----- link_set_igp_weight ----------------------------------------
/**
 *
 */
void link_set_igp_weight(SNetLink * pLink, uint32_t uIGPweight)
{
  pLink->uIGPweight= uIGPweight;
}

// ----- link_get_igp_weight ----------------------------------------
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
  fprintf(stderr, "*** \033[31;1mMESSAGE DROPPED\033[0m ***\n");
  fprintf(stderr, "message: ");
  message_dump(stderr, pMsg);
  //if (pMessage->uProtocol == NET_PROTOCOL_BGP)
  //  bgp_msg_dump(stderr, NULL, pMessage->pPayLoad);
  fprintf(stderr, "\n");
  fprintf(stderr, "link   : ");
  link_dst_dump(stderr, pLink);
  fprintf(stderr, "\n");
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
void link_dst_dump(FILE * pStream, SNetLink * pLink)
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
void link_dump(FILE * pStream, SNetLink * pLink)
{
  SPrefix sPrefix;

  /* Link type & destination address:
     - router: IP address
     - transit: IP prefix
     - subnet: IP prefix
   */
  if (pLink->uDestinationType == NET_LINK_TYPE_ROUTER) {
    fprintf(pStream, "ROUTER\t");
    ip_address_dump(pStream, link_get_address(pLink));
  } else {
    if (pLink->uDestinationType == NET_LINK_TYPE_TRANSIT) {
      fprintf(pStream, "TRANSIT\t");
    } else {
      fprintf(pStream, "SUBNET\t");
    }
    link_get_prefix(pLink, &sPrefix);
    ip_prefix_dump(pStream, sPrefix);
  }

  /* Link weight & delay */
  fprintf(pStream, "\t%u\t%u", pLink->tDelay, pLink->uIGPweight);

  /* Link state (up/down) */
  if (link_get_state(pLink, NET_LINK_FLAG_UP))
    fprintf(pStream, "\tUP");
  else
    fprintf(pStream, "\tDOWN");

  /* Optional attributes */
  if (link_get_state(pLink, NET_LINK_FLAG_TUNNEL))
    fprintf(pStream, "\ttunnel");
  if (link_get_state(pLink, NET_LINK_FLAG_IGP_ADV))
    fprintf(pStream, "\tadv:yes");

#ifdef OSPF_SUPPORT
  if (pLink->tArea != OSPF_NO_AREA)
    fprintf(pStream, "\tarea:%u", pLink->tArea);
#endif
}
