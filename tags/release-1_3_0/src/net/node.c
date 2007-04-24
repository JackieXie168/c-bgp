// ==================================================================
// @(#)node.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 08/08/2005
// @lastdate 23/01/2007
// ==================================================================

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libgds/str_util.h>

#include <net/link.h>
#include <net/link-list.h>
#include <net/net_types.h>
#include <net/network.h>
#include <net/node.h>
#include <net/subnet.h>

// ----- node_mgmt_perror -------------------------------------------
/**
 *
 */
void node_mgmt_perror(SLogStream * pStream, int iErrorCode)
{
  switch (iErrorCode) {
  case NET_SUCCESS:
    log_printf(pStream, "success"); break;
  case NET_ERROR_MGMT_INVALID_NODE:
    log_printf(pStream, "invalid node"); break;
  case NET_ERROR_MGMT_INVALID_LINK:
    log_printf(pStream, "invalid link"); break;
  case NET_ERROR_MGMT_INVALID_SUBNET:
    log_printf(pStream, "invalid subnet"); break;
  case NET_ERROR_MGMT_NODE_ALREADY_EXISTS:
    log_printf(pStream, "node already exists"); break;
  case NET_ERROR_MGMT_LINK_ALREADY_EXISTS:
    log_printf(pStream, "link already exists"); break;
  case NET_ERROR_MGMT_LINK_LOOP:
    log_printf(pStream, "link endpoints are equal"); break;
  case NET_ERROR_MGMT_INVALID_OPERATION:
    log_printf(pStream, "invalid operation"); break;
  default:
    log_printf(pStream, "unknown error (%i)", iErrorCode);
  }
}

// ----- node_create ------------------------------------------------
/**
 *
 */
SNetNode * node_create(net_addr_t tAddr)
{
  SNetNode * pNode= (SNetNode *) MALLOC(sizeof(SNetNode));
  pNode->tAddr= tAddr;
  pNode->pcName= NULL;
  
  pNode->pLinks= net_links_create();
  pNode->pRT= rt_create();

#ifdef OSPF_SUPPORT
  pNode->pOSPFAreas  = uint32_array_create(ARRAY_OPTION_SORTED|ARRAY_OPTION_UNIQUE);
  pNode->pOspfRT     = OSPF_rt_create();
#endif

  pNode->pIGPDomains = uint16_array_create(ARRAY_OPTION_SORTED|ARRAY_OPTION_UNIQUE);
  pNode->pProtocols= protocols_create();
  node_register_protocol(pNode, NET_PROTOCOL_ICMP, pNode,
			 NULL, icmp_event_handler);
  return pNode;
}

// ----- node_destroy -----------------------------------------------
/**
 *
 */
void node_destroy(SNetNode ** ppNode)
{
  if (*ppNode != NULL) {
    rt_destroy(&(*ppNode)->pRT);
    protocols_destroy(&(*ppNode)->pProtocols);
    net_links_destroy(&(*ppNode)->pLinks);

#ifdef OSPF_SUPPORT
    _array_destroy((SArray **)(&(*ppNode)->pOSPFAreas));
    OSPF_rt_destroy(&(*ppNode)->pOspfRT);
#endif

    uint16_array_destroy(&(*ppNode)->pIGPDomains);
    if ((*ppNode)->pcName)
      str_destroy(&(*ppNode)->pcName);
    FREE(*ppNode);
    *ppNode= NULL;
  }
}

// ----- node_get_name ----------------------------------------------
char * node_get_name(SNetNode * pNode)
{
  return pNode->pcName;
}

// ----- node_set_name ----------------------------------------------
void node_set_name(SNetNode * pNode, const char * pcName)
{
  if (pNode->pcName)
    str_destroy(&pNode->pcName);
  if (pcName != NULL)
    pNode->pcName= str_create(pcName);
  else
    pNode->pcName= NULL;
}

// ----- node_dump ---------------------------------------------------
/**
 *
 */
void node_dump(SLogStream * pStream, SNetNode * pNode)
{ 
  ip_address_dump(pStream, pNode->tAddr);
}

// ----- node_info --------------------------------------------------
/**
 *
 */
void node_info(SLogStream * pStream, SNetNode * pNode)
{
  unsigned int uIndex;

  log_printf(pStream, "loopback : ");
  ip_address_dump(pStream, pNode->tAddr);
  log_printf(pStream, "\n");
  log_printf(pStream, "domain   :");
  for (uIndex= 0; uIndex < uint16_array_length(pNode->pIGPDomains); uIndex++) {
    log_printf(pStream, " %d", pNode->pIGPDomains->data[uIndex]);
  }
  log_printf(pStream, "\n");
  if (pNode->pcName != NULL)
    log_printf(pStream, "name     : %s\n", pNode->pcName);
  log_printf(pStream, "addresses: ");
  node_addresses_dump(pStream, pNode);
  log_printf(pStream, "\n");
}


/////////////////////////////////////////////////////////////////////
//
// NODE LINKS FUNCTIONS
//
/////////////////////////////////////////////////////////////////////

// ----- node_add_link_ptp ------------------------------------------
/**
 * Helper function to add a point-to-point link to a node.
 */
int node_add_link_ptp(SNetNode * pNodeA, SNetNode * pNodeB, 
		      net_link_delay_t tDelay,
		      net_link_load_t tCapacity,
		      uint8_t tDepth,
		      int iMutual)
{
  SNetLink * pLink;
  int iResult;

  iResult= net_link_create_ptp(pNodeA, pNodeB, tDelay, tCapacity,
			       tDepth, &pLink);
  if (iResult < 0)
    return iResult;

  if (iMutual) {
    // If link has to be created in both directions, perform
    // recursive call with reverse source and destination.
    iResult= node_add_link_ptp(pNodeB, pNodeA, tDelay, tCapacity,
			       tDepth, 0);
    if (iResult)
      return iResult;
  }

  if (net_links_add(pNodeA->pLinks, pLink) < 0)
    return NET_ERROR_MGMT_LINK_ALREADY_EXISTS;

  return NET_SUCCESS;
}

// ----- node_add_link_mtp ------------------------------------------
/**
 *
 */
static inline int node_add_link_mtp(SNetNode * pNode,
				    SNetSubnet * pSubnet,
				    net_addr_t tIfaceAddr,
				    net_link_delay_t tDelay,
				    net_link_load_t tCapacity,
				    uint8_t tDepth,
				    int iMutual)
{
  SNetLink * pLink= net_link_create_mtp(pNode, pSubnet, tIfaceAddr,
					tDelay, tCapacity, tDepth);

  if (net_links_add(pNode->pLinks, pLink) < 0)
    return NET_ERROR_MGMT_LINK_ALREADY_EXISTS;

  else if (iMutual)
    return subnet_add_link(pSubnet, pLink, tIfaceAddr);
  else 
    return NET_SUCCESS;
}

// ----- node_add_link ----------------------------------------------
/**
 * Add a link to a destination. If the destination is an IP address, a
 * point-to-point link is created. If the destination is an IP prefix,
 * a link to a subnet is created.
 */
int node_add_link(SNetNode * pNode, SNetDest sDest,
		  net_link_delay_t tDelay, net_link_load_t tCapacity,
		  uint8_t tDepth)
{
  SNetNode * pDestNode;
  SNetSubnet * pDestSubnet;

  switch (sDest.tType) {

  case NET_DEST_ADDRESS:
    // Find destination node
    pDestNode= network_find_node(sDest.uDest.tAddr);
    if (pDestNode == NULL)
      return NET_ERROR_MGMT_INVALID_NODE;
    return node_add_link_ptp(pNode, pDestNode, tDelay, tCapacity, tDepth, 1);

  case NET_DEST_PREFIX:
    // Find destination subnet
    pDestSubnet= network_find_subnet(sDest.uDest.sPrefix);
    if (pDestSubnet == NULL)
      return NET_ERROR_MGMT_INVALID_SUBNET;
    return node_add_link_mtp(pNode, pDestSubnet,
			     sDest.uDest.sPrefix.tNetwork, tDelay, tCapacity,
			     tDepth, 1);

  default:
    return NET_ERROR_MGMT_INVALID_OPERATION;
  }
  return 0;
}

// ----- node_find_link_ptp -----------------------------------------
SNetLink * node_find_link_ptp(SNetNode * pNode,
			      net_addr_t tAddr)
{
  return net_links_find_ptp(pNode->pLinks, tAddr);
}

// ----- _node_find_link_mtp ----------------------------------------
static SNetLink * _node_find_link_mtp(SNetNode * pNode,
				      net_addr_t tIfaceAddr,
				      uint8_t uMaskLen)
{
  return net_links_find_mtp(pNode->pLinks, tIfaceAddr, uMaskLen);
}

// ----- node_find_link ---------------------------------------------
/**
 * Find a link based on the destination address / prefix.
 */
SNetLink * node_find_link(SNetNode * pNode, SNetDest sDest)
{
  switch (sDest.tType) {
  case NET_DEST_ADDRESS:
    return node_find_link_ptp(pNode, sDest.uDest.tAddr);
  case NET_DEST_PREFIX:
    return _node_find_link_mtp(pNode,
			       sDest.uDest.sPrefix.tNetwork,
			       sDest.uDest.sPrefix.uMaskLen);
  default:
    return NULL;
  }
}

// -----[ node_find_iface ]------------------------------------------
/**
 * Find a link based on the local address.
 */
SNetLink * node_find_iface(SNetNode * pNode, SNetDest sDest)
{
  switch (sDest.tType) {
  case NET_DEST_ADDRESS:
    return net_links_find_iface(pNode->pLinks, sDest.uDest.tAddr);
  case NET_DEST_PREFIX:
    return _node_find_link_mtp(pNode, sDest.uDest.sPrefix.tNetwork,
			       sDest.uDest.sPrefix.uMaskLen);
  default:
    return NULL;
  }
}

// ----- node_links_enum --------------------------------------------
SNetLink * node_links_enum(SNetNode * pNode, int state)
{
  static SEnumerator * pEnum;
  SNetLink * pLink;

  if (state == 0)
    pEnum= net_links_get_enum(pNode->pLinks);
  if (enum_has_next(pEnum)) {
    pLink= *((SNetLink **) enum_get_next(pEnum));
    return pLink;
  }
  enum_destroy(&pEnum);
  return NULL;
}

// ----- node_links_clear -------------------------------------------
void node_links_clear(SNetNode * pNode)
{
  SEnumerator * pEnum= net_links_get_enum(pNode->pLinks);
  SNetLink * pLink;

  while (enum_has_next(pEnum)) {
    pLink= *((SNetLink **) enum_get_next(pEnum));
    net_link_set_load(pLink, 0);
  }
  enum_destroy(&pEnum);
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
int node_rt_add_route(SNetNode * pNode, SPrefix sPrefix,
		      net_addr_t tNextHopIface, net_addr_t tNextHop,
		      uint32_t uWeight, uint8_t uType)
{
  SNetLink * pIface;

  // Lookup the next-hop's interface (no recursive lookup is allowed,
  // it must be a direct link !)
  pIface= node_find_link_ptp(pNode, tNextHopIface);
  if (pIface == NULL)
    return NET_RT_ERROR_IF_UNKNOWN;

  return node_rt_add_route_link(pNode, sPrefix, pIface, tNextHop,
				uWeight, uType);
}

// ----- node_rt_add_route_dest -------------------------------------
/**
 * Add a route to the given prefix into the given node. The node must
 * have a direct link towards the given next-hop address. The weight
 * of the route can be specified as it can be different from the
 * outgoing link's weight.
 */
int node_rt_add_route_dest(SNetNode * pNode, SPrefix sPrefix,
			   SNetDest sIfaceDest, net_addr_t tNextHop,
			   uint32_t uWeight, uint8_t uType)
{
  SNetLink * pIface;

  // Lookup the outgoing interface (no recursive lookup is allowed,
  // it must be a direct link !)
  pIface= node_find_iface(pNode, sIfaceDest);
  if (pIface == NULL)
    return NET_RT_ERROR_IF_UNKNOWN;

  return node_rt_add_route_link(pNode, sPrefix, pIface, tNextHop,
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
int node_rt_add_route_link(SNetNode * pNode, SPrefix sPrefix,
			   SNetLink * pIface, net_addr_t tNextHop,
			   uint32_t uWeight, uint8_t uType)
{
  SNetRouteInfo * pRouteInfo;
  SNetLink * pSubLink;

  // If a next-hop has been specified, check that it is reachable
  // through the given interface.
  if (tNextHop != 0) {
    if (pIface->uType == NET_LINK_TYPE_ROUTER) {
      if (pIface->tDest.tAddr != tNextHop)
	return NET_RT_ERROR_NH_UNREACH;
    } else {
      pSubLink= subnet_find_link(pIface->tDest.pSubnet, tNextHop);
      if ((pSubLink == NULL) || (pSubLink->tIfaceAddr == pIface->tIfaceAddr))
	return NET_RT_ERROR_NH_UNREACH;
    }
  }

  // Build route info
  pRouteInfo= net_route_info_create(sPrefix, pIface, tNextHop,
				    uWeight, uType);

  return rt_add_route(pNode->pRT, sPrefix, pRouteInfo);
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
int node_rt_del_route(SNetNode * pNode, SPrefix * pPrefix,
		      SNetDest * psIface, net_addr_t * ptNextHop,
		      uint8_t uType)
{
  SNetLink * pIface= NULL;

  // Lookup the outgoing interface (no recursive lookup is allowed,
  // it must be a direct link !)
  if (psIface != NULL)
    pIface= node_find_link(pNode, *psIface);

  return rt_del_route(pNode->pRT, pPrefix, pIface, ptNextHop, uType);
}


/////////////////////////////////////////////////////////////////////
//
// PROTOCOLS
//
/////////////////////////////////////////////////////////////////////

// ----- node_register_protocol -------------------------------------
/**
 * Register a new protocol into the given node. The protocol is
 * identified by a number (see protocol_t.h for definitions). The
 * destroy callback function is used to destroy PDU corresponfing to
 * this protocol in case the message carrying such PDU is not
 * delivered to the handler (message dropped or incorrectly routed for
 * instance). The protocol handler is composed of the handle_event
 * callback function and the handler context pointer.
 */
int node_register_protocol(SNetNode * pNode, uint8_t uNumber,
			   void * pHandler,
			   FNetNodeHandlerDestroy fDestroy,
			   FNetNodeHandleEvent fHandleEvent)
{
  return protocols_register(pNode->pProtocols, uNumber, pHandler,
			    fDestroy, fHandleEvent);
}

// -----[ node_get_protocol ]----------------------------------------
/**
 * Return the handler for the given protocol. If the protocol is not
 * supported, NULL is returned.
 */
SNetProtocol * node_get_protocol(SNetNode * pNode, uint8_t uNumber)
{
  return protocols_get(pNode->pProtocols, uNumber);
}
