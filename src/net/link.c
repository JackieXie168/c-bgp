// ==================================================================
// @(#)link.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @author Stefano Iasi (stefanoia@tin.it)
// @date 24/02/2004
// @lastdate 25/02/2008
// ==================================================================

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include <libgds/log.h>
#include <libgds/memory.h>

#include <net/error.h>
#include <net/iface.h>
#include <net/net_types.h>
#include <net/network.h>
#include <net/subnet.h>
#include <net/link.h>
#include <net/link_attr.h>
#include <net/ospf.h>
#include <net/node.h>


/////////////////////////////////////////////////////////////////////
//
// LINK METHODS
//
/////////////////////////////////////////////////////////////////////

// ----- net_link_destroy -------------------------------------------
/**
 *
 */
void net_link_destroy(SNetLink ** ppLink)
{
  if (*ppLink != NULL) {

    // Destroy context if callback provided
    //if ((*ppLink)->fDestroy != NULL)
    //  (*ppLink)->fDestroy((*ppLink)->pContext);

    // Destroy IGP weights
    //net_igp_weights_destroy(&(*ppLink)->pWeights);

    net_iface_destroy(ppLink);
  }
}

// -----[ _net_link_create_iface ]-----------------------------------
/**
 * Get the given network interface on the given node. If the
 * interface already exists, check that its type corresponds to the
 * requested interface type.
 *
 * If the interface does not exist, create it.
 */
static inline int _net_link_create_iface(SNetNode * pNode,
					 net_iface_id_t tID,
					 net_iface_type_t tType,
					 SNetIface ** ppIface)
{
  int iResult;

  // Does the interface already exist ?
  *ppIface= node_find_iface(pNode, tID);

  if (*ppIface == NULL) {

    // If the interface does not exist, create it.
    iResult= net_iface_factory(pNode, tID, tType, ppIface);
    if (iResult != NET_SUCCESS)
      return iResult;
    iResult= node_add_iface2(pNode, *ppIface);
    if (iResult != NET_SUCCESS)
      return iResult;

  } else {

    // If the interface exists, check that its type corresponds to
    // the requested type.
    if ((*ppIface)->tType != tType)
      return NET_ERROR_MGMT_IFACE_INCOMPATIBLE;

  }

  return NET_SUCCESS;
}

// -----[ net_link_create_rtr ]--------------------------------------
/**
 * Create a single router-to-router (RTR) link.
 */
int net_link_create_rtr(SNetNode * pSrcNode,
			SNetNode * pDstNode,
			EIfaceDir eDir,
			SNetIface ** ppIface)
{
  SNetIface * pSrcIface;
  SNetIface * pDstIface;
  int iResult;

  // Create source network interface
  iResult= _net_link_create_iface(pSrcNode, net_iface_id_addr(pDstNode->tAddr),
				  NET_IFACE_RTR, &pSrcIface);
  if (iResult != NET_SUCCESS)
    return iResult;

  // Create destination network interface
  iResult= _net_link_create_iface(pDstNode, net_iface_id_addr(pSrcNode->tAddr),
				  NET_IFACE_RTR, &pDstIface);
  if (iResult != NET_SUCCESS)
    return iResult;
  
  // Connect interfaces in both directions
  iResult= net_iface_connect_iface(pSrcIface, pDstIface);
  if (iResult != NET_SUCCESS)
    return iResult;
  if (eDir == BIDIR) {
    iResult= net_iface_connect_iface(pDstIface, pSrcIface);
    if (iResult != NET_SUCCESS)
      return iResult;
  }

  if (ppIface != NULL)
    *ppIface= pSrcIface;
  return NET_SUCCESS;
}

// -----[ net_link_create_ptp ]--------------------------------------
int net_link_create_ptp(SNetNode * pSrcNode,
			net_iface_id_t tSrcIfaceID,
			SNetNode * pDstNode,
			net_iface_id_t tDstIfaceID,
			EIfaceDir eDir,
			SNetIface ** ppIface)
{
  SNetIface * pSrcIface;
  SNetIface * pDstIface;
  int iResult;

  // Check that endpoints are different
  if ((pSrcNode == pDstNode) ||
      (pSrcNode->tAddr == pDstNode->tAddr))
    return NET_ERROR_MGMT_LINK_LOOP;

  // Check that both prefixes have the same length
  if (tSrcIfaceID.uMaskLen != tDstIfaceID.uMaskLen) {
    return -1;
  }

  // Check that both masked prefixes are equal
  if (ip_prefix_cmp(&tSrcIfaceID, &tDstIfaceID) != 0) {
    return -1;
  }

  // Check that both interfaces are different
  if (tSrcIfaceID.tNetwork == tDstIfaceID.tNetwork) {
    return -1;
  }

  // Create source network interface (if it does not exist)
  iResult= _net_link_create_iface(pSrcNode, tSrcIfaceID,
				    NET_IFACE_PTP, &pSrcIface);
  if (iResult != NET_SUCCESS)
    return iResult;

  // Create destination network interface
  iResult= _net_link_create_iface(pDstNode, tDstIfaceID,
				  NET_IFACE_PTP, &pDstIface);
  if (iResult != NET_SUCCESS)
    return iResult;

  // Connect interfaces in both directions
  iResult= net_iface_connect_iface(pSrcIface, pDstIface);
  if (iResult != NET_SUCCESS)
    return iResult;
  if (eDir == BIDIR) {
    iResult= net_iface_connect_iface(pDstIface, pSrcIface);
    if (iResult != NET_SUCCESS)
      return iResult;
  }

  if (ppIface != NULL)
    *ppIface= pSrcIface;
  return NET_SUCCESS;
}

// -----[ net_link_create_ptmp ]-------------------------------------
/**
 * Create a link to a subnet (point-to-multi-point link).
 */
int net_link_create_ptmp(SNetNode * pSrcNode,
			 SNetSubnet * pSubnet,
			 net_addr_t tIfaceAddr,
			 SNetIface ** ppIface)
{
  SPrefix tIfaceID= net_iface_id_pfx(tIfaceAddr, pSubnet->sPrefix.uMaskLen);
  SNetIface * pSrcIface;
  int iResult;

  iResult= net_iface_factory(pSrcNode, tIfaceID, NET_IFACE_PTMP, &pSrcIface);
  if (iResult != NET_SUCCESS)
    return iResult;
  iResult= node_add_iface2(pSrcNode, pSrcIface);
  if (iResult != NET_SUCCESS)
    return iResult;

  // Connect interface to subnet
  iResult= net_iface_connect_subnet(pSrcIface, pSubnet);
  if (iResult != NET_SUCCESS)
    return iResult;

  // Connect subnet to interface
  subnet_add_link(pSubnet, pSrcIface);

  if (ppIface != NULL)
    *ppIface= pSrcIface;
  return NET_SUCCESS;
}

// -----[ net_link_set_phys_attr ]-----------------------------------
int net_link_set_phys_attr(SNetIface * pIface, net_link_delay_t tDelay,
			   net_link_load_t tCapacity, EIfaceDir eDir)
{
  int iResult= net_iface_set_delay(pIface, tDelay, eDir);
  if (iResult != NET_SUCCESS)
    return iResult;
  return net_iface_set_capacity(pIface, tCapacity, eDir);
}


/////////////////////////////////////////////////////////////////////
//
// LINK DUMP FUNCTIONS
//
/////////////////////////////////////////////////////////////////////

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
  net_iface_dump(pStream, pLink, 1);

  /* Link delay */
  log_printf(pStream, "\t%u", pLink->tDelay);

  /* Link weight */
  if (pLink->pWeights != NULL)
    log_printf(pStream, "\t%u", pLink->pWeights->data[0]);
  else
    log_printf(pStream, "\t---");

  /* Link state (up/down) */
  if (net_iface_is_enabled(pLink))
    log_printf(pStream, "\tUP");
  else
    log_printf(pStream, "\tDOWN");

#ifdef OSPF_SUPPORT
  if (pLink->tArea != OSPF_NO_AREA)
    log_printf(pStream, "\tarea:%u", pLink->tArea);
#endif
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
  unsigned int uIndex;

  log_printf(pStream, "iface    : ");
  net_iface_dump_id(pStream, pLink);
  log_printf(pStream, "\ntype     : ");
  net_iface_dump_type(pStream, pLink);
  log_printf(pStream, "\ndest     : ");
  net_iface_dump_dest(pStream, pLink);
  log_printf(pStream, "\ncapacity : %u\n", pLink->tCapacity);
  log_printf(pStream, "load     : %u\n", pLink->tLoad);
  if (pLink->pWeights != NULL) {
    log_printf(pStream, "igp-depth: %u\n",
	       net_igp_weights_depth(pLink->pWeights));
    for (uIndex= 0; uIndex < net_igp_weights_depth(pLink->pWeights); uIndex++)
      log_printf(pStream, "tos%.2u    : %u\n", uIndex,
		 pLink->pWeights->data[uIndex]);
  }
}
