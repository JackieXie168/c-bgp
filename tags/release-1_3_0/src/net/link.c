// ==================================================================
// @(#)link.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @author Stefano Iasi (stefanoia@tin.it)
// @date 24/02/2004
// @lastdate 16/04/2007
// ==================================================================

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <net/net_types.h>
#include <net/network.h>
#include <net/subnet.h>
#include <net/link.h>
#include <net/link_attr.h>
#include <net/ospf.h>
#include <net/node.h>
#include <libgds/log.h>
#include <libgds/memory.h>

// ----- Forward declarations --------------------------------------------
static int _net_link_forward(net_addr_t tPhysAddr, void * pContext,
			     SNetNode ** ppNextHop, SNetMessage ** ppMsg);


/////////////////////////////////////////////////////////////////////
//
// LINK METHODS
//
/////////////////////////////////////////////////////////////////////

// ----- _net_link_create --------------------------------------------
/**
 * Basic link creation function.
 *
 * Required arguments:
 * - source node
 * - propagation delay
 * - depth (number of possible IGP weights)
 * - forwarding callback
 */
static inline SNetLink * _net_link_create(SNetNode * pSrcNode,
					  net_link_delay_t tDelay,
					  net_link_load_t tCapacity,
					  net_tos_t tDepth,
					  FNetLinkForward fForward)
{
  SNetLink * pLink= (SNetLink *) MALLOC(sizeof(SNetLink));
  pLink->pSrcNode= pSrcNode;
  pLink->pWeights= net_igp_weights_create(tDepth);
  pLink->tDelay= tDelay;
  pLink->tCapacity= tCapacity;
  pLink->tLoad= 0;
  pLink->uFlags= NET_LINK_FLAG_UP | NET_LINK_FLAG_IGP_ADV;
  pLink->pContext= pLink;
  pLink->fForward= fForward;
  pLink->fDestroy= NULL;

#ifdef OSPF_SUPPORT
  pLink->tArea= OSPF_NO_AREA;
#endif
  return pLink;
}

// ----- net_link_destroy -------------------------------------------
/**
 *
 */
void net_link_destroy(SNetLink ** ppLink)
{
  if (*ppLink != NULL) {

    // Destroy context if callback provided
    if ((*ppLink)->fDestroy != NULL)
      (*ppLink)->fDestroy((*ppLink)->pContext);

    // Destroy links
    net_igp_weights_destroy(&(*ppLink)->pWeights);

    FREE(*ppLink);
    *ppLink= NULL;
  }
}

//  ----- net_link_create_ptp2 --------------------------------------
/**
 * Create a link to a single router (point-to-point link).
 */
int net_link_create_ptp2(SNetNode * pSrcNode,
			 net_addr_t tAddr,
			 net_link_delay_t tDelay,
			 net_link_load_t tCapacity,
			 uint8_t tDepth,
			 SNetLink ** ppLink)
{
  SNetLink * pLink;

  // Check that endpoints addresses are different
  if (pSrcNode->tAddr == tAddr)
    return NET_ERROR_MGMT_LINK_LOOP;

  pLink= _net_link_create(pSrcNode, tDelay, tCapacity, tDepth,
			  _net_link_forward);
  pLink->uType= NET_LINK_TYPE_ROUTER;
  pLink->tDest.tAddr= tAddr;

  // *** Needs to be fixed ***
  // If we allow multiple parallel links, this interface address must
  // become a unique interface address on this side of the link
  // (src-node). For the moment, this interface address is set to the
  // src-node's address. Multiple parallel links are therefore not
  // allowed until this is fixed.
  pLink->tIfaceAddr= pSrcNode->tAddr;
  pLink->tIfaceMask= 32;

  *ppLink= pLink;

  return NET_SUCCESS;
}

// ----- net_link_create_ptp ----------------------------------------
/**
 * Create a link to a single router (point-to-point link).
 */
int net_link_create_ptp(SNetNode * pSrcNode,
			SNetNode * pDstNode,
			net_link_delay_t tDelay,
			net_link_load_t tCapacity,
			uint8_t tDepth,
			SNetLink ** ppLink)
{
  if (pSrcNode == pDstNode)
    return NET_ERROR_MGMT_LINK_LOOP;

  return net_link_create_ptp2(pSrcNode, pDstNode->tAddr,
			      tDelay, tCapacity, tDepth, ppLink);
}

// ----- net_link_create_mtp ----------------------------------------
/**
 * Create a link to a subnet (point-to-multi-point link).
 */
SNetLink * net_link_create_mtp(SNetNode * pSrcNode,
			       SNetSubnet * pSubnet,
			       net_addr_t tIfaceAddr,
			       net_link_delay_t tDelay,
			       net_link_load_t tCapacity,			       
			       uint8_t tDepth)
{
  SNetLink * pLink= _net_link_create(pSrcNode, tDelay, tCapacity,
				     tDepth, _subnet_forward);
  if (subnet_is_transit(pSubnet))
    pLink->uType= NET_LINK_TYPE_TRANSIT;
  else 
    pLink->uType= NET_LINK_TYPE_STUB;

  // Source
  pLink->tIfaceAddr= tIfaceAddr;
  pLink->tIfaceMask= pSubnet->sPrefix.uMaskLen;

  // Destination
  pLink->tDest.pSubnet= pSubnet;

  pLink->pContext= pLink;

  return pLink;
}

// ----- net_link_get_address ---------------------------------------
/**
 * Return the destination address of the link.
 */
net_addr_t net_link_get_address(SNetLink * pLink)
{
  if ((pLink->uType == NET_LINK_TYPE_ROUTER) ||
      (pLink->uType == NET_LINK_TYPE_TUNNEL)) {
    return (pLink->tDest).tAddr;
  } else {
    assert(pLink->tDest.pSubnet != NULL);
    return ((pLink->tDest).pSubnet->sPrefix).tNetwork;
  }
}

// ----- net_link_get_id --------------------------------------------
/**
 * Return the identifier of the link on the source node. This
 * identifier is a prefix defined as follows, depending on the
 * link type:
 *   ptp: dst address / mask length (32)
 *   mtp,tun: local interface address / mask length
 */
SPrefix net_link_get_id(SNetLink * pLink)
{
  SPrefix sPrefix;

  if (pLink->uType == NET_LINK_TYPE_ROUTER) {
    sPrefix.tNetwork= pLink->tDest.tAddr; // destination address
    sPrefix.uMaskLen= 32;
  } else {
    sPrefix.tNetwork= pLink->tIfaceAddr;  // belongs to destination network
    sPrefix.uMaskLen= pLink->tIfaceMask;
  }
  return sPrefix;
}

// ----- net_link_get_prefix ----------------------------------------
/**
 * Return the destination prefix associated with this link.
 */
SPrefix net_link_get_prefix(SNetLink * pLink)
{
  SPrefix sPrefix;
  if (pLink->uType == NET_LINK_TYPE_ROUTER ) {
    sPrefix.tNetwork= pLink->tDest.tAddr;
    sPrefix.uMaskLen= 32;
  } else {
    // ptmp,tun
    assert(pLink->tDest.pSubnet != NULL);
    sPrefix.tNetwork= ((pLink->tDest).pSubnet->sPrefix).tNetwork;
    sPrefix.uMaskLen= ((pLink->tDest).pSubnet->sPrefix).uMaskLen;
  }
  return sPrefix;
}

// ----- link_get_subnet --------------------------------------------
/**
 * Get the subnet bound to this link. This function will only return
 * a subnet on point-to-multipoint links (TRANSIT/STUB). It will
 * return a NULL pointer on point-to-point and tunnel links.
 */
SNetSubnet * link_get_subnet(SNetLink * pLink){
  if ((pLink->uType != NET_LINK_TYPE_ROUTER) &&
      (pLink->uType != NET_LINK_TYPE_TUNNEL))
    return (pLink->tDest).pSubnet;
  return NULL;
}


/////////////////////////////////////////////////////////////////////
//
// LINK FORWARDING METHODS
//
/////////////////////////////////////////////////////////////////////

// ----- _net_link_forward ------------------------------------------
/**
 * Forward a message along a point-to-point link. The next-hop
 * information is not used.
 */
static int _net_link_forward(net_addr_t tPhysAddr, void * pContext,
			     SNetNode ** ppNextHop, SNetMessage ** ppMsg)
{
  SNetLink * pLink= (SNetLink *) pContext;

  /*log_printf(pLogOut, "NET_LINK_FORWARD.begin(");
  ip_address_dump(pLogOut, pLink->pSrcNode->tAddr);
  log_printf(pLogOut, ",");
  ip_address_dump(pLogOut, pLink->tDest.tAddr);
  log_printf(pLogOut, ")\n");*/

  // Check link's state
  if (!net_link_get_state(pLink, NET_LINK_FLAG_UP))
    return NET_ERROR_LINK_DOWN;

  // Node lookup: O(log(n))
  *ppNextHop= network_find_node(pLink->tDest.tAddr);
  
  return NET_SUCCESS;
}


/////////////////////////////////////////////////////////////////////
//
// LINK ATTRIBUTES
//
/////////////////////////////////////////////////////////////////////

// ----- net_link_get_weight ----------------------------------------
net_igp_weight_t net_link_get_weight(SNetLink * pLink, net_tos_t tTOS)
{
  return pLink->pWeights->data[tTOS];
}

// ----- net_link_set_weight ----------------------------------------
void net_link_set_weight(SNetLink * pLink, net_tos_t tTOS,
		     net_igp_weight_t tWeight)
{
  pLink->pWeights->data[tTOS]= tWeight;
}

// ----- net_link_get_depth -----------------------------------------
unsigned int net_link_get_depth(SNetLink * pLink)
{
  return net_igp_weights_depth(pLink->pWeights);
}

// ----- net_link_get_delay -----------------------------------------
net_link_delay_t net_link_get_delay(SNetLink * pLink)
{
  return pLink->tDelay;
}

// ----- net_link_set_delay -----------------------------------------
void net_link_set_delay(SNetLink * pLink, net_link_delay_t tDelay)
{
  pLink->tDelay= tDelay;
}

// ----- net_link_set_state -----------------------------------------
void net_link_set_state(SNetLink * pLink, uint8_t uFlag, int iState)
{
  if (iState)
    pLink->uFlags|= uFlag;
  else
    pLink->uFlags&= ~uFlag;
}

// ----- net_link_get_state -----------------------------------------
int net_link_get_state(SNetLink * pLink, uint8_t uFlag)
{
  return (pLink->uFlags & uFlag) != 0;
}

// ----- net_link_set_capacity --------------------------------------
void net_link_set_capacity(SNetLink * pLink, net_link_load_t tCapacity)
{
  pLink->tCapacity= tCapacity;
}

// ----- net_link_get_capacity --------------------------------------
net_link_load_t net_link_get_capacity(SNetLink * pLink)
{
  return pLink->tCapacity;
}

// ----- net_link_set_load ------------------------------------------
void net_link_set_load(SNetLink * pLink, net_link_load_t tLoad)
{
  pLink->tLoad= tLoad;
}

// ----- net_link_get_load ------------------------------------------
net_link_load_t net_link_get_load(SNetLink * pLink)
{
  return pLink->tLoad;
}

// ----- net_link_add_load ------------------------------------------
void net_link_add_load(SNetLink * pLink, net_link_load_t tIncLoad)
{
  uint64_t tBigLoad= pLink->tLoad;
  tBigLoad+= tIncLoad;
  if (tBigLoad > NET_LINK_MAX_LOAD)
    pLink->tLoad= NET_LINK_MAX_LOAD;
  else
    pLink->tLoad= (net_link_load_t) tBigLoad;
}


/////////////////////////////////////////////////////////////////////
//
// LINK DUMP FUNCTIONS
//
/////////////////////////////////////////////////////////////////////

// ----- link_dst_dump ----------------------------------------------
/**
 * Show the link's destination. If the link heads towards a router,
 * the destination is an IP address (point-to-point links and tunnels).
 * Otherwise, if it heads towards a subnet (transit/stub), the
 * destination is an IP prefix.
 */
void link_dst_dump(SLogStream * pStream, SNetLink * pLink)
{
  if ((pLink->uType == NET_LINK_TYPE_ROUTER) ||
      (pLink->uType == NET_LINK_TYPE_TUNNEL)) {
    ip_address_dump(pStream, net_link_get_address(pLink));
  } else {
    // ptmp,tun
    ip_prefix_dump(pStream, net_link_get_prefix(pLink));
  }
}

// ----- net_link_dump ----------------------------------------------
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
void net_link_dump(SLogStream * pStream, SNetLink * pLink)
{
  /* Link type & destination address:
     - router: IP address
     - transit: IP prefix
     - subnet: IP prefix
   */
  switch (pLink->uType) {
  case NET_LINK_TYPE_ROUTER:
    log_printf(pStream, "ROUTER\t");
    ip_address_dump(pStream, net_link_get_address(pLink));
    break;
  case NET_LINK_TYPE_TUNNEL:
    log_printf(pStream, "TUNNEL\t");
    ip_address_dump(pStream, net_link_get_address(pLink));
    break;
  case NET_LINK_TYPE_TRANSIT:
    log_printf(pStream, "TRANSIT\t");
    ip_prefix_dump(pStream, net_link_get_prefix(pLink));
    break;
  case NET_LINK_TYPE_STUB:
    log_printf(pStream, "STUB\t");
    ip_prefix_dump(pStream, net_link_get_prefix(pLink));
    break;
  }

  /* Link weight & delay */
  log_printf(pStream, "\t%u\t%u", pLink->tDelay, pLink->pWeights->data[0]);

  /* Link state (up/down) */
  if (net_link_get_state(pLink, NET_LINK_FLAG_UP))
    log_printf(pStream, "\tUP");
  else
    log_printf(pStream, "\tDOWN");

  /* Optional attributes */
  if (net_link_get_state(pLink, NET_LINK_FLAG_IGP_ADV))
    log_printf(pStream, "\tadv:yes");

#ifdef OSPF_SUPPORT
  if (pLink->tArea != OSPF_NO_AREA)
    log_printf(pStream, "\tarea:%u", pLink->tArea);
#endif
}

// ----- net_link_dump_id -------------------------------------------
/**
 * Format: <src-node> <src-iface> <dst-prefix>
 */
void net_link_dump_id(SLogStream * pStream, SNetLink * pLink)
{
  ip_address_dump(pStream, pLink->pSrcNode->tAddr);
  log_printf(pStream, "\t");
  ip_address_dump(pStream, pLink->tIfaceAddr);
  log_printf(pStream, "\t");
  ip_prefix_dump(pStream, net_link_get_prefix(pLink));
}

// ----- net_link_dump_load -----------------------------------------
/**
 * Dump the link's load and capacity.
 *
 * Format: <load> <capacity>
 */
void net_link_dump_load(SLogStream * pStream, SNetLink * pLink)
{
  log_printf(pStream, "%u\t%u", pLink->tLoad, pLink->tCapacity);
}

// ----- net_link_dump_info -----------------------------------------
/**
 *
 */
void net_link_dump_info(SLogStream * pStream, SNetLink * pLink)
{
  log_printf(pStream, "src     : ");
  ip_address_dump(pStream, pLink->tIfaceAddr);
  log_printf(pStream, "\ndst     : ");
  ip_prefix_dump(pStream, net_link_get_prefix(pLink));
  log_printf(pStream, "\ntype    : ");
  switch (pLink->uType) {
  case NET_LINK_TYPE_ROUTER:
    log_printf(pStream, "point-to-point"); break;
  case NET_LINK_TYPE_TUNNEL:
    log_printf(pStream, "tunnel"); break;
  case NET_LINK_TYPE_TRANSIT:
  case NET_LINK_TYPE_STUB:
    log_printf(pStream, "point-to-multipoint"); break;
  default:
    log_printf(pStream, "unknown");
  }
  log_printf(pStream, "\ncapacity: %u\n", pLink->tCapacity);
  log_printf(pStream, "load    : %u\n", pLink->tLoad);
}
