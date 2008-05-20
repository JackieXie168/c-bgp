// ==================================================================
// @(#)link.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @author Stefano Iasi (stefanoia@tin.it)
// @date 24/02/2004
// $Id: link.c,v 1.26 2008-05-20 12:17:06 bqu Exp $
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
void net_link_destroy(net_iface_t ** link_ref)
{
  if (*link_ref != NULL) {

    // Destroy context if callback provided
    //if ((*ppLink)->fDestroy != NULL)
    //  (*ppLink)->fDestroy((*ppLink)->pContext);

    // Destroy IGP weights
    //net_igp_weights_destroy(&(*ppLink)->pWeights);

    net_iface_destroy(link_ref);
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
		       net_iface_t ** iface_ref)
{
  net_error_t error;

  // Does the interface already exist ?
  *iface_ref= node_find_iface(node, tID);

  if (*iface_ref == NULL) {

    // If the interface does not exist, create it.
    error= net_iface_factory(node, tID, type, iface_ref);
    if (error != ESUCCESS)
      return error;
    error= node_add_iface2(node, *iface_ref);
    if (error != ESUCCESS)
      return error;

  } else {

    // If the interface exists, check that its type corresponds to
    // the requested type.
    if ((*iface_ref)->type != type)
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
net_error_t net_link_create_ptp(net_node_t * src_node,
				net_iface_id_t tSrcIfaceID,
				net_node_t * dst_node,
				net_iface_id_t tDstIfaceID,
				net_iface_dir_t dir,
				net_iface_t ** iface_ref)
{
  net_iface_t * src_iface;
  net_iface_t * dst_iface;
  net_error_t error;

  // Check that endpoints are different
  if (src_node == dst_node)
    return ENET_LINK_LOOP;

  // Check that both prefixes have the same length
  if (tSrcIfaceID.uMaskLen != tDstIfaceID.uMaskLen)
    return EUNEXPECTED;

  // Check that both masked prefixes are equal
  if (ip_prefix_cmp(&tSrcIfaceID, &tDstIfaceID) != 0)
    return EUNEXPECTED;

  // Check that both interfaces are different
  if (tSrcIfaceID.tNetwork == tDstIfaceID.tNetwork)
    return EUNEXPECTED;

  // Create source network interface (if it does not exist)
  error= _net_link_create_iface(src_node, tSrcIfaceID,
				NET_IFACE_PTP, &src_iface);
  if (error != ESUCCESS)
    return error;

  // Create destination network interface
  error= _net_link_create_iface(dst_node, tDstIfaceID,
				NET_IFACE_PTP, &dst_iface);
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

// -----[ net_link_create_ptmp ]-------------------------------------
/**
 * Create a link to a subnet (point-to-multi-point link).
 */
net_error_t net_link_create_ptmp(net_node_t * src_node,
				 net_subnet_t * subnet,
				 net_addr_t iface_addr,
				 net_iface_t ** iface_ref)
{
  ip_pfx_t tIfaceID= net_iface_id_pfx(iface_addr, subnet->prefix.uMaskLen);
  net_iface_t * src_iface;
  net_error_t error;

  error= net_iface_factory(src_node, tIfaceID, NET_IFACE_PTMP, &src_iface);
  if (error != ESUCCESS)
    return error;
  error= node_add_iface2(src_node, src_iface);
  if (error != ESUCCESS)
    return error;

  // Connect interface to subnet
  error= net_iface_connect_subnet(src_iface, subnet);
  if (error != ESUCCESS)
    return error;

  // Connect subnet to interface
  subnet_add_link(subnet, src_iface);

  if (iface_ref != NULL)
    *iface_ref= src_iface;
  return ESUCCESS;
}

// -----[ net_link_set_phys_attr ]-----------------------------------
net_error_t net_link_set_phys_attr(net_iface_t * iface,
				   net_link_delay_t delay,
				   net_link_load_t capacity,
				   net_iface_dir_t dir)
{
  net_error_t error= net_iface_set_delay(iface, delay, dir);
  if (error != ESUCCESS)
    return error;
  return net_iface_set_capacity(iface, capacity, dir);
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
void net_link_dump(SLogStream * stream, net_iface_t * link)
{
  net_iface_dump(stream, link, 1);

  /* Link delay */
  log_printf(stream, "\t%u", link->phys.delay);

  /* Link weight */
  if (link->pWeights != NULL)
    log_printf(stream, "\t%u", link->pWeights->data[0]);
  else
    log_printf(stream, "\t---");

  /* Link state (up/down) */
  if (net_iface_is_enabled(link))
    log_printf(stream, "\tUP");
  else
    log_printf(stream, "\tDOWN");

#ifdef OSPF_SUPPORT
  if (link->tArea != OSPF_NO_AREA)
    log_printf(stream, "\tarea:%u", link->tArea);
#endif
}

// ----- net_link_dump_load -----------------------------------------
/**
 * Dump the link's load and capacity.
 *
 * Format: <load> <capacity>
 */
void net_link_dump_load(SLogStream * stream, net_iface_t * link)
{
  log_printf(stream, "%u\t%u", link->phys.load, link->phys.capacity);
}

// ----- net_link_dump_info -----------------------------------------
/**
 * 
 */
void net_link_dump_info(SLogStream * stream, net_iface_t * link)
{
  unsigned int uIndex;

  log_printf(stream, "iface    : ");
  net_iface_dump_id(stream, link);
  log_printf(stream, "\ntype     : ");
  net_iface_dump_type(stream, link);
  log_printf(stream, "\ndest     : ");
  net_iface_dump_dest(stream, link);
  log_printf(stream, "\ncapacity : %u", link->phys.capacity);
  log_printf(stream, "\ndelay    : %u", link->phys.delay);
  log_printf(stream, "\nload     : %u\n", link->phys.load);
  if (link->pWeights != NULL) {
    log_printf(stream, "igp-depth: %u\n",
	       net_igp_weights_depth(link->pWeights));
    for (uIndex= 0; uIndex < net_igp_weights_depth(link->pWeights); uIndex++)
      log_printf(stream, "tos%.2u    : %u\n", uIndex,
		 link->pWeights->data[uIndex]);
  }
}
