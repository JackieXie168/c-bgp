// ==================================================================
// @(#)link.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @author Stefano Iasi (stefanoia@tin.it)
// @date 24/02/2004
// $Id: link.c,v 1.25 2008-04-11 11:03:06 bqu Exp $
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
void net_link_destroy(net_iface_t ** ppLink)
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
static inline net_error_t
_net_link_create_iface(net_node_t * node,
		       net_iface_id_t tID,
		       net_iface_type_t type,
		       net_iface_t ** ppIface)
{
  int iResult;

  // Does the interface already exist ?
  *ppIface= node_find_iface(node, tID);

  if (*ppIface == NULL) {

    // If the interface does not exist, create it.
    iResult= net_iface_factory(node, tID, type, ppIface);
    if (iResult != ESUCCESS)
      return iResult;
    iResult= node_add_iface2(node, *ppIface);
    if (iResult != ESUCCESS)
      return iResult;

  } else {

    // If the interface exists, check that its type corresponds to
    // the requested type.
    if ((*ppIface)->type != type)
      return ENET_IFACE_INCOMPATIBLE;

  }

  return ESUCCESS;
}

// -----[ net_link_create_rtr ]--------------------------------------
/**
 * Create a single router-to-router (RTR) link.
 */
net_error_t net_link_create_rtr(net_node_t * src_node,
				net_node_t * dst_node,
				net_iface_dir_t dir,
				net_iface_t ** iface_ref)
{
  net_iface_t * src_iface;
  net_iface_t * dst_iface;
  net_error_t error;

  if (src_node == dst_node)
    return ENET_LINK_LOOP;

  // Create source network interface
  error= _net_link_create_iface(src_node, net_iface_id_addr(dst_node->addr),
				NET_IFACE_RTR, &src_iface);
  if (error != ESUCCESS)
    return error;

  // Create destination network interface
  error= _net_link_create_iface(dst_node, net_iface_id_addr(src_node->addr),
				NET_IFACE_RTR, &dst_iface);
  if (error != ESUCCESS)
    return error;
  
  // Connect interfaces in both directions
  error= net_iface_connect_iface(src_iface, dst_iface);
  if (error != ESUCCESS)
    return error;
  if (dir == BIDIR) {
    error= net_iface_connect_iface(dst_iface, src_iface);
    if (error != ESUCCESS)
      return error;
  }

  if (iface_ref != NULL)
    *iface_ref= src_iface;
  return ESUCCESS;
}

// -----[ net_link_create_ptp ]--------------------------------------
int net_link_create_ptp(net_node_t * pSrcNode,
			net_iface_id_t tSrcIfaceID,
			net_node_t * pDstNode,
			net_iface_id_t tDstIfaceID,
			net_iface_dir_t dir,
			net_iface_t ** ppIface)
{
  net_iface_t * pSrcIface;
  net_iface_t * pDstIface;
  int iResult;

  // Check that endpoints are different
  if (pSrcNode == pDstNode)
    return ENET_LINK_LOOP;

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
  if (iResult != ESUCCESS)
    return iResult;

  // Create destination network interface
  iResult= _net_link_create_iface(pDstNode, tDstIfaceID,
				  NET_IFACE_PTP, &pDstIface);
  if (iResult != ESUCCESS)
    return iResult;

  // Connect interfaces in both directions
  iResult= net_iface_connect_iface(pSrcIface, pDstIface);
  if (iResult != ESUCCESS)
    return iResult;
  if (dir == BIDIR) {
    iResult= net_iface_connect_iface(pDstIface, pSrcIface);
    if (iResult != ESUCCESS)
      return iResult;
  }

  if (ppIface != NULL)
    *ppIface= pSrcIface;
  return ESUCCESS;
}

// -----[ net_link_create_ptmp ]-------------------------------------
/**
 * Create a link to a subnet (point-to-multi-point link).
 */
int net_link_create_ptmp(net_node_t * pSrcNode,
			 net_subnet_t * subnet,
			 net_addr_t tIfaceAddr,
			 net_iface_t ** ppIface)
{
  ip_pfx_t tIfaceID= net_iface_id_pfx(tIfaceAddr, subnet->prefix.uMaskLen);
  net_iface_t * pSrcIface;
  int iResult;

  iResult= net_iface_factory(pSrcNode, tIfaceID, NET_IFACE_PTMP, &pSrcIface);
  if (iResult != ESUCCESS)
    return iResult;
  iResult= node_add_iface2(pSrcNode, pSrcIface);
  if (iResult != ESUCCESS)
    return iResult;

  // Connect interface to subnet
  iResult= net_iface_connect_subnet(pSrcIface, subnet);
  if (iResult != ESUCCESS)
    return iResult;

  // Connect subnet to interface
  subnet_add_link(subnet, pSrcIface);

  if (ppIface != NULL)
    *ppIface= pSrcIface;
  return ESUCCESS;
}

// -----[ net_link_set_phys_attr ]-----------------------------------
int net_link_set_phys_attr(net_iface_t * pIface, net_link_delay_t tDelay,
			   net_link_load_t tCapacity, net_iface_dir_t dir)
{
  int iResult= net_iface_set_delay(pIface, tDelay, dir);
  if (iResult != ESUCCESS)
    return iResult;
  return net_iface_set_capacity(pIface, tCapacity, dir);
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
void net_link_dump(SLogStream * pStream, net_iface_t * pLink)
{
  net_iface_dump(pStream, pLink, 1);

  /* Link delay */
  log_printf(pStream, "\t%u", pLink->phys.delay);

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
void net_link_dump_load(SLogStream * pStream, net_iface_t * pLink)
{
  log_printf(pStream, "%u\t%u", pLink->phys.load, pLink->phys.capacity);
}

// ----- net_link_dump_info -----------------------------------------
/**
 * 
 */
void net_link_dump_info(SLogStream * pStream, net_iface_t * pLink)
{
  unsigned int uIndex;

  log_printf(pStream, "iface    : ");
  net_iface_dump_id(pStream, pLink);
  log_printf(pStream, "\ntype     : ");
  net_iface_dump_type(pStream, pLink);
  log_printf(pStream, "\ndest     : ");
  net_iface_dump_dest(pStream, pLink);
  log_printf(pStream, "\ncapacity : %u", pLink->phys.capacity);
  log_printf(pStream, "\ndelay    : %u", pLink->phys.delay);
  log_printf(pStream, "\nload     : %u\n", pLink->phys.load);
  if (pLink->pWeights != NULL) {
    log_printf(pStream, "igp-depth: %u\n",
	       net_igp_weights_depth(pLink->pWeights));
    for (uIndex= 0; uIndex < net_igp_weights_depth(pLink->pWeights); uIndex++)
      log_printf(pStream, "tos%.2u    : %u\n", uIndex,
		 pLink->pWeights->data[uIndex]);
  }
}
