// ==================================================================
// @(#)node.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 08/08/2005
// $Id: node.c,v 1.14 2008-04-11 11:03:06 bqu Exp $
// ==================================================================

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <libgds/str_util.h>

#include <net/error.h>
#include <net/link.h>
#include <net/link-list.h>
#include <net/net_types.h>
#include <net/netflow.h>
#include <net/network.h>
#include <net/node.h>
#include <net/record-route.h>
#include <net/subnet.h>

// ----- node_create ------------------------------------------------
/**
 *
 */
net_error_t node_create(net_addr_t addr, net_node_t ** node_ref)
{
  net_node_t * node= (net_node_t *) MALLOC(sizeof(net_node_t));
  net_error_t error;

  if (addr == NET_ADDR_ANY)
    return ENET_NODE_INVALID_ID;

  node->addr= addr;
  node->name= NULL;
  node->ifaces= net_links_create();
  node->protocols= protocols_create();
  node->rt= rt_create();
  node->coord.latitude= 0;
  node->coord.longitude= 0;
  node->syslog_enabled= 0;
  node->pIGPDomains= uint16_array_create(ARRAY_OPTION_SORTED|
					 ARRAY_OPTION_UNIQUE);

  // Activate ICMP protocol
  error= node_register_protocol(node, NET_PROTOCOL_ICMP, node, NULL,
				icmp_event_handler);
  if (error != ESUCCESS)
    return error;

  // Create loopback interface with node's RID
  error= node_add_iface(node, net_iface_id_addr(addr), NET_IFACE_LOOPBACK);
  if (error != ESUCCESS)
    return error;

#ifdef OSPF_SUPPORT
  node->pOSPFAreas= uint32_array_create(ARRAY_OPTION_SORTED|
					 ARRAY_OPTION_UNIQUE);
  node->pOspfRT= OSPF_rt_create();
#endif

  *node_ref= node;
  return ESUCCESS;
}

// ----- node_destroy -----------------------------------------------
/**
 *
 */
void node_destroy(net_node_t ** node_ref)
{
  if (*node_ref != NULL) {
    rt_destroy(&(*node_ref)->rt);
    protocols_destroy(&(*node_ref)->protocols);
    net_links_destroy(&(*node_ref)->ifaces);

#ifdef OSPF_SUPPORT
    _array_destroy((SArray **)(&(*node_ref)->pOSPFAreas));
    OSPF_rt_destroy(&(*node_ref)->pOspfRT);
#endif

    uint16_array_destroy(&(*node_ref)->pIGPDomains);
    if ((*node_ref)->name)
      str_destroy(&(*node_ref)->name);
    FREE(*node_ref);
    *node_ref= NULL;
  }
}

// ----- node_get_name ----------------------------------------------
char * node_get_name(net_node_t * node)
{
  return node->name;
}

// ----- node_set_name ----------------------------------------------
void node_set_name(net_node_t * node, const char * name)
{
  if (node->name)
    str_destroy(&node->name);
  if (name != NULL)
    node->name= str_create(name);
  else
    node->name= NULL;
}

// -----[ node_set_coord ]--------------------------------------------
/**
 *
 */

// -----[ node_get_coord ]--------------------------------------------
/**
 *
 */
void node_get_coord(net_node_t * node, float * latitude, float * longitude)
{
  if (latitude != NULL)
    *latitude= node->coord.latitude;
  if (longitude != NULL)
    *longitude= node->coord.longitude;
}

// ----- node_dump ---------------------------------------------------
/**
 *
 */
void node_dump(SLogStream * stream, net_node_t * node)
{ 
  ip_address_dump(stream, node->addr);
}

// ----- node_info --------------------------------------------------
/**
 *
 */
void node_info(SLogStream * stream, net_node_t * pNode)
{
  unsigned int uIndex;

  log_printf(stream, "loopback : ");
  ip_address_dump(stream, pNode->addr);
  log_printf(stream, "\n");
  log_printf(stream, "domain   :");
  for (uIndex= 0; uIndex < uint16_array_length(pNode->pIGPDomains); uIndex++) {
    log_printf(stream, " %d", pNode->pIGPDomains->data[uIndex]);
  }
  log_printf(stream, "\n");
  if (pNode->name != NULL)
    log_printf(stream, "name     : %s\n", pNode->name);
  log_printf(stream, "addresses: ");
  node_addresses_dump(stream, pNode);
  log_printf(stream, "\n");
}


/////////////////////////////////////////////////////////////////////
//
// NODE INTERFACES FUNCTIONS
//
/////////////////////////////////////////////////////////////////////

// -----[ node_add_iface ]-------------------------------------------
/**
 * This function adds an interface to the node. The interface
 * identifier is an IP address or an IP prefix. The interface type
 * is one of router-to-router (RTR), point-to-point (PTP),
 * point-to-multipoint (PTMP) or virtual.
 *
 * If the identifier is incorrect or an interface with the same
 * identifier already exists, the function fails with error:
 *   NET_ERROR_MGMT_INVALID_IFACE_ID
 *
 * Otherwise, the function should return
 *   ESUCCESS
 */
int node_add_iface(net_node_t * pNode, net_iface_id_t tIfaceID,
		   net_iface_type_t tIfaceType)
{
  net_iface_t * pIface;
  int iResult;

  // Create interface
  iResult= net_iface_factory(pNode, tIfaceID, tIfaceType, &pIface);
  if (iResult != ESUCCESS)
    return iResult;

  return node_add_iface2(pNode, pIface);
}

// -----[ node_add_iface2 ]------------------------------------------
/**
 * Attach an interface to the node.
 *
 * If an interface with the same identifier already exists, the
 * function fails with error
 *   NET_ERROR_MGMT_DUPLICATE_IFACE
 */
int node_add_iface2(net_node_t * pNode, net_iface_t * pIface)
{
  net_error_t error= net_links_add(pNode->ifaces, pIface);
  if (error != ESUCCESS)
    net_iface_destroy(&pIface);
  return error;
}

// -----[ node_find_iface ]------------------------------------------
/**
 * Find an interface link based on its identifier.
 */
net_iface_t * node_find_iface(net_node_t * pNode, net_iface_id_t tIfaceID)
{
  return net_links_find(pNode->ifaces, tIfaceID);
}

// -----[ node_ifaces_dump ]-----------------------------------------
/**
 * This function shows the list of interfaces of a given node's, along
 * with their type.
 */
void node_ifaces_dump(SLogStream * stream, net_node_t * pNode)
{
  unsigned int uIndex;
  net_iface_t * pIface;

  // Show network interfaces
  for (uIndex= 0; uIndex < net_ifaces_size(pNode->ifaces); uIndex++) {
    pIface= net_ifaces_at(pNode->ifaces, uIndex);
    net_iface_dump(stream, pIface, 0);
    log_printf(stream, "\n");
  }
}

// -----[ node_ifaces_load_clear ]-----------------------------------
void node_ifaces_load_clear(net_node_t * pNode)
{
  enum_t * pEnum= net_links_get_enum(pNode->ifaces);
  net_iface_t * pIface;

  while (enum_has_next(pEnum)) {
    pIface= *((net_iface_t **) enum_get_next(pEnum));
    net_iface_set_load(pIface, 0);
  }
  enum_destroy(&pEnum);
}


/////////////////////////////////////////////////////////////////////
//
// NODE LINKS FUNCTIONS
//
/////////////////////////////////////////////////////////////////////

// ----- node_links_dump --------------------------------------------
/**
 * Dump all the node's links. The function works by scanning through
 * all the node's interfaces. If an interface is connected, it is
 * dumped, by using a "link dump" function (i.e. net_link_dump()
 * defined in src/net/link.h).
 */
void node_links_dump(SLogStream * stream, net_node_t * pNode)
{
  unsigned int uIndex;
  net_iface_t * pIface;

  for (uIndex= 0; uIndex < net_ifaces_size(pNode->ifaces); uIndex++) {
    pIface= net_ifaces_at(pNode->ifaces, uIndex);
    if (!net_iface_is_connected(pIface))
      continue;
    net_link_dump(stream, pIface);
    log_printf(stream, "\n");
  }
}

// ----- node_links_enum --------------------------------------------
/**
 * THIS SHOULD BE MOVED TO src/cli/enum.c
net_iface_t * node_links_enum(net_node_t * pNode, int state)
{
  static enum_t * pEnum;
  net_iface_t * pIface;

  if (state == 0)
    pEnum= net_links_get_enum(pNode->ifaces);
  if (enum_has_next(pEnum)) {
    pIface= *((net_iface_t **) enum_get_next(pEnum));
    return pIface;
  }
  enum_destroy(&pEnum);
  return NULL;
}
*/

// ----- node_links_save --------------------------------------------
void node_links_save(SLogStream * stream, net_node_t * node)
{
  enum_t * pEnum= net_links_get_enum(node->ifaces);
  net_iface_t * iface;

  while (enum_has_next(pEnum)) {
    iface= *((net_iface_t **) enum_get_next(pEnum));
  
    // Skip tunnels
    if (iface->type == NET_IFACE_VIRTUAL)
      continue;
  
    // This side of the link
    ip_address_dump(stream, node->addr);
    log_printf(stream, "\t");
    net_iface_dump_id(stream, iface);
    log_printf(stream, "\t");

    // Other side of the link
    switch (iface->type) {
    case NET_IFACE_RTR:
      ip_address_dump(stream, iface->dest.iface->src_node->addr);
      log_printf(stream, "\t");
      ip_address_dump(stream, node->addr);
      log_printf(stream, "/32");
      break;
    case NET_IFACE_PTMP:
      ip_prefix_dump(stream, iface->dest.subnet->prefix);
      log_printf(stream, "\t---");
      break;
    default:
      abort();
    }

    // Link load
    log_printf(stream, "\t%u\n", iface->phys.load);
  }
    
  enum_destroy(&pEnum);
}

// -----[ node_has_address ]-----------------------------------------
/**
 * This function checks if the node has the given address. The
 * function first checks the loopback address (that identifies the
 * node). Then, the function checks the multi-point links of the node
 * for an 'interface' address.
 *
 * Return:
 * 1 if the node has the given address
 * 0 otherwise
 */
int node_has_address(net_node_t * pNode, net_addr_t addr)
{
  unsigned int uIndex;
  net_iface_t * pIface;

  for (uIndex= 0; uIndex < net_ifaces_size(pNode->ifaces); uIndex++) {
    pIface= net_ifaces_at(pNode->ifaces, uIndex);
    if (net_iface_has_address(pIface, addr))
      return 1;
  }
  return 0;
}

// -----[ node_has_prefix ]------------------------------------------
int node_has_prefix(net_node_t * pNode, ip_pfx_t prefix)
{
  unsigned int uIndex;
  net_iface_t * pIface;
  
  for (uIndex= 0; uIndex < net_ifaces_size(pNode->ifaces); uIndex++) {
    pIface= net_ifaces_at(pNode->ifaces, uIndex);
    if (net_iface_has_prefix(pIface, prefix))
      return 1;
  }
  return 0;
}

// ----- node_addresses_for_each ------------------------------------
/**
 * This function calls the given callback function for each address
 * owned by the node. The first address is the node's loopback address
 * (its identifier). The other addresses are the interface addresses
 * (that are set on multi-point links, for instance).
 */
int node_addresses_for_each(net_node_t * node, FArrayForEach fForEach,
			    void * ctx)
{
  int iResult;
  unsigned int index;
  net_iface_t * iface;

  for (index= 0; index < net_ifaces_size(node->ifaces); index++) {
    iface= net_ifaces_at(node->ifaces, index);
    if (iface->type == NET_IFACE_RTR)
      continue;
    if ((iResult= fForEach(&iface->tIfaceAddr, ctx)) != 0)
      return iResult;
  }
  return 0;
}

// ----- node_addresses_dump ----------------------------------------
/**
 * This function shows the list of addresses owned by the node. The
 * first address is the node's loopback address (its identifier). The
 * other addresses are the interface addresses (that are set on
 * multi-point links, for instance).
 */
void node_addresses_dump(SLogStream * stream, net_node_t * node)
{
  unsigned int index;
  net_iface_t * iface;

  // Show interface addresses
  for (index= 0; index < net_ifaces_size(node->ifaces); index++) {
    iface= net_ifaces_at(node->ifaces, index);
    if (iface->type == NET_IFACE_RTR)
      continue;
    log_printf(stream, " ");
    ip_address_dump(stream, iface->tIfaceAddr);
  }
}


/////////////////////////////////////////////////////////////////////
//
// ROUTING TABLE FUNCTIONS
//
/////////////////////////////////////////////////////////////////////

// ----- node_rt_add_route ------------------------------------------
/**
 * Add a route to the given prefix into the given node. The node must
 * have a direct link towards the given next-hop address. The weight
 * of the route can be specified as it can be different from the
 * outgoing link's weight.
 */
int node_rt_add_route(net_node_t * node, ip_pfx_t prefix,
		      net_iface_id_t tOutIfaceID, net_addr_t tNextHop,
		      uint32_t uWeight, uint8_t uType)
{
  net_iface_t * iface;

  // Lookup the next-hop's interface (no recursive lookup is allowed,
  // it must be a direct link !)
  iface= node_find_iface(node, tOutIfaceID);
  if (iface == NULL)
    return ENET_IFACE_UNKNOWN;

  // The interface cannot be a loopback interface
  if (iface->type == NET_IFACE_LOOPBACK)
    return ENET_IFACE_INCOMPATIBLE;

  return node_rt_add_route_link(node, prefix, iface, tNextHop,
				uWeight, uType);
}

// ----- node_rt_add_route_link -------------------------------------
/**
 * Add a route to the given prefix into the given node. The outgoing
 * interface is a link. The weight of the route can be specified as it
 * can be different from the outgoing link's weight.
 *
 * Pre: the outgoing link (next-hop interface) must exist in the node.
 */
int node_rt_add_route_link(net_node_t * node, ip_pfx_t prefix,
			   net_iface_t * iface, net_addr_t tNextHop,
			   uint32_t uWeight, uint8_t uType)
{
  rt_info_t * rtinfo;
  net_iface_t * pSubLink;

  // If a next-hop has been specified, check that it is reachable
  // through the given interface.
  if (tNextHop != 0) {
    switch (iface->type) {
    case NET_IFACE_RTR:
      if (iface->dest.iface->src_node->addr != tNextHop)
	return ENET_RT_NH_UNREACH;
      break;
    case NET_IFACE_PTP:
      if (!node_has_address(iface->dest.iface->src_node, tNextHop))
	return ENET_RT_NH_UNREACH;
      break;
    case NET_IFACE_PTMP:
      pSubLink= net_subnet_find_link(iface->dest.subnet, tNextHop);
      if ((pSubLink == NULL) || (pSubLink->tIfaceAddr == iface->tIfaceAddr))
	return ENET_RT_NH_UNREACH;
      break;
    default:
      abort();
    }
  }

  // Build route info
  rtinfo= net_route_info_create(prefix, iface, tNextHop,
				uWeight, uType);

  return rt_add_route(node->rt, prefix, rtinfo);
}

// ----- node_rt_del_route ------------------------------------------
/**
 * This function removes a route from the node's routing table. The
 * route is identified by the prefix, the next-hop address (i.e. the
 * outgoing interface) and the type.
 *
 * If the next-hop is not present (NULL), all the routes matching the
 * other parameters will be removed whatever the next-hop is.
 */
int node_rt_del_route(net_node_t * node, ip_pfx_t * pPrefix,
		      net_iface_id_t * ptIface, net_addr_t * ptNextHop,
		      uint8_t uType)
{
  net_iface_t * iface= NULL;

  // Lookup the outgoing interface (no recursive lookup is allowed,
  // it must be a direct link !)
  if (ptIface != NULL)
    iface= node_find_iface(node, *ptIface);

  return rt_del_route(node->rt, pPrefix, iface, ptNextHop, uType);
}

// ----- node_rt_dump -----------------------------------------------
/**
 * Dump the node's routing table.
 */
void node_rt_dump(SLogStream * stream, net_node_t * node, SNetDest sDest)
{
  if (node->rt != NULL)
    rt_dump(stream, node->rt, sDest);
  log_flush(stream);
}


/////////////////////////////////////////////////////////////////////
//
// PROTOCOL FUNCTIONS
//
/////////////////////////////////////////////////////////////////////

// ----- node_register_protocol -------------------------------------
/**
 * Register a new protocol into the given node. The protocol is
 * identified by a number (see protocol_t.h for definitions). The
 * destroy callback function is used to destroy PDU corresponding to
 * this protocol in case the message carrying such PDU is not
 * delivered to the handler (message dropped or incorrectly routed for
 * instance). The protocol handler is composed of the handle_event
 * callback function and the handler context pointer.
 */
int node_register_protocol(net_node_t * node, net_protocol_id_t id,
			   void * handler,
			   FNetProtoHandlerDestroy destroy,
			   FNetProtoHandleEvent handle_event)
{
  return protocols_register(node->protocols, id, handler,
			    destroy, handle_event);
}

// -----[ node_get_protocol ]----------------------------------------
/**
 * Return the handler for the given protocol. If the protocol is not
 * supported, NULL is returned.
 */
net_protocol_t * node_get_protocol(net_node_t * node,
				   net_protocol_id_t id)
{
  return protocols_get(node->protocols, id);
}


/////////////////////////////////////////////////////////////////////
//
// TRAFFIC LOAD FUNCTIONS
//
/////////////////////////////////////////////////////////////////////

typedef struct {
  net_node_t *   pTargetNode;
  uint8_t      uOptions;
  unsigned int uFlowsTotal;
  unsigned int uFlowsOk;
  unsigned int uFlowsError;
  unsigned int uOctetsTotal;
  unsigned int uOctetsOk;
  unsigned int uOctetsError;
} SNET_NODE_NETFLOW_CTX;

// -----[ _node_netflow_handler ]------------------------------------
static int _node_netflow_handler(net_addr_t tSrc, net_addr_t tDst,
				 unsigned int uOctets, void * pContext)
{
  SNET_NODE_NETFLOW_CTX * pCtx= (SNET_NODE_NETFLOW_CTX *) pContext;
  net_node_t * pNode= pCtx->pTargetNode;
  SNetRecordRouteInfo * pRRInfo;
  SNetDest sDst= { .tType= NET_DEST_ADDRESS, .uDest.tAddr= tDst };

  pCtx->uFlowsTotal++;
  pCtx->uOctetsTotal+= uOctets;

  pRRInfo= node_record_route(pNode, sDst, 0,
			     NET_RECORD_ROUTE_OPTION_LOAD,
			     uOctets);

  if (pCtx->uOptions & NET_NODE_NETFLOW_OPTIONS_DETAILS) {
    log_printf(pLogOut, "src:");
    ip_address_dump(pLogOut, tSrc);
    log_printf(pLogOut, " dst:");
    ip_address_dump(pLogOut, tDst);
    log_printf(pLogOut, " octets:%u ", uOctets);
    if (pRRInfo != NULL) {
      log_printf(pLogOut, "status:%d\n", pRRInfo->iResult);
    } else {
      log_printf(pLogOut, "status:?\n");
    }
  }
  
  if (pRRInfo == NULL) {
    pCtx->uFlowsError++;
    pCtx->uOctetsError+= uOctets;
    return NETFLOW_ERROR_UNEXPECTED;
  }

  /** IN THE FUTURE, WE SHOULD CHECK TRAFFIC MATRIX ERRORS
      if (pRRInfo->iResult == NET_RECORD_ROUTE_TO_HOST) {
      iError= NET_TM_ERROR_ROUTE;
      break;
      }
  */
  net_record_route_info_destroy(&pRRInfo);

  pCtx->uFlowsOk++;
  pCtx->uOctetsOk+= uOctets;
  
  return NETFLOW_SUCCESS;
}

// -----[ node_load_netflow ]----------------------------------------
int node_load_netflow(net_node_t * pNode, const char * pcFileName,
		      uint8_t uOptions)
{
  int iResult;
  SNET_NODE_NETFLOW_CTX sCtx= {
    .pTargetNode= pNode,
    .uOptions= uOptions,
    .uFlowsTotal= 0,
    .uFlowsOk= 0,
    .uFlowsError= 0,
    .uOctetsTotal= 0,
    .uOctetsOk= 0,
    .uOctetsError= 0,
  };

  iResult= netflow_load(pcFileName, _node_netflow_handler, &sCtx);

  if (uOptions & NET_NODE_NETFLOW_OPTIONS_SUMMARY) {
    log_printf(pLogOut, "Flows total : %u\n", sCtx.uFlowsTotal);
    log_printf(pLogOut, "Flows ok    : %u\n", sCtx.uFlowsOk);
    log_printf(pLogOut, "Flows error : %u\n", sCtx.uFlowsError);
    log_printf(pLogOut, "Octets total: %u\n", sCtx.uOctetsTotal);
    log_printf(pLogOut, "Octets ok   : %u\n", sCtx.uOctetsOk);
    log_printf(pLogOut, "Octets error: %u\n", sCtx.uOctetsError);
  }

  return iResult;
}


/////////////////////////////////////////////////////////////////////
//
// SYSLOG
//
/////////////////////////////////////////////////////////////////////

// -----[ node_syslog ]--------------------------------------------
SLogStream * node_syslog(net_node_t * self)
{
  if (self->syslog_enabled)
    return pLogErr;
  return NULL;
}

// -----[ node_syslog_set_enabled ]--------------------------------
void node_syslog_set_enabled(net_node_t * self, int enabled)
{
  self->syslog_enabled= enabled;
}
