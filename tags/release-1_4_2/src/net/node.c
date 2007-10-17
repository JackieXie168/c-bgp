// ==================================================================
// @(#)node.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 08/08/2005
// @lastdate 23/07/2007
// ==================================================================

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

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
  node_register_protocol(pNode, NET_PROTOCOL_ICMP, pNode, NULL,
			 icmp_event_handler);

  pNode->fLatitude= 0;
  pNode->fLongitude= 0;
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

// -----[ node_set_coord ]--------------------------------------------
/**
 *
 */

// -----[ node_get_coord ]--------------------------------------------
/**
 *
 */
void node_get_coord(SNetNode * pNode, float * pfLatitude, float * pfLongitude)
{
  if (pfLatitude != NULL)
    *pfLatitude= pNode->fLatitude;
  if (pfLongitude != NULL)
    *pfLongitude= pNode->fLongitude;
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
  if (iResult != NET_SUCCESS)
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
				    uint8_t tDepth)
{
  SNetLink * pLink;
  int iResult;

  iResult= net_link_create_mtp(pNode, pSubnet, tIfaceAddr,
			       tDelay, tCapacity, tDepth, &pLink);
  if (iResult != NET_SUCCESS)
    return iResult;

  if (net_links_add(pNode->pLinks, pLink) < 0)
    return NET_ERROR_MGMT_LINK_ALREADY_EXISTS;

  return subnet_add_link(pSubnet, pLink);
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
			     tDepth);

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

// ----- node_links_save --------------------------------------------
void node_links_save(SLogStream * pStream, SNetNode * pNode)
{
  SEnumerator * pEnum= net_links_get_enum(pNode->pLinks);
  SNetLink * pLink;

  while (enum_has_next(pEnum)) {
    pLink= *((SNetLink **) enum_get_next(pEnum));
  
    // Skip tunnels
    if (pLink->uType == NET_LINK_TYPE_TUNNEL)
      continue;
  
    // This side of the link
    ip_address_dump(pStream, pNode->tAddr);
    log_printf(pStream, "\t");
    ip_prefix_dump(pStream, net_link_get_id(pLink));
    log_printf(pStream, "\t");

    // Other side of the link
    switch (pLink->uType) {
    case NET_LINK_TYPE_ROUTER:
      ip_address_dump(pStream, pLink->tDest.pNode->tAddr);
      log_printf(pStream, "\t");
      ip_address_dump(pStream, pNode->tAddr);
      log_printf(pStream, "/32");
      break;
    case NET_LINK_TYPE_TRANSIT:
    case NET_LINK_TYPE_STUB:
      ip_prefix_dump(pStream, pLink->tDest.pSubnet->sPrefix);
      log_printf(pStream, "\t---");
      break;
    default:
      abort();
    }

    // Link load
    log_printf(pStream, "\t%u\n", pLink->tLoad);
  }
    
  enum_destroy(&pEnum);
}

// ----- node_has_address -------------------------------------------
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
int node_has_address(SNetNode * pNode, net_addr_t tAddress)
{
  unsigned int uIndex;
  SNetLink * pLink;

  // Check loopback address
  if (pNode->tAddr == tAddress)
    return 1;

  // Check interface addresses
  for (uIndex= 0; uIndex < ptr_array_length(pNode->pLinks); uIndex++) {
    pLink= (SNetLink *) pNode->pLinks->data[uIndex];
    if (pLink->tIfaceAddr == tAddress)
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
int node_addresses_for_each(SNetNode * pNode, FArrayForEach fForEach,
			    void * pContext)
{
  int iResult;
  unsigned int uIndex;
  SNetLink * pLink;

  // Loopback interface
  if ((iResult= fForEach(&pNode->tAddr, pContext)) != 0)
    return iResult;
    

  // Other interfaces
  for (uIndex= 0; uIndex < ptr_array_length(pNode->pLinks); uIndex++) {
    pLink= (SNetLink *) pNode->pLinks->data[uIndex];
    if (pLink->uType == NET_LINK_TYPE_ROUTER)
      continue;
    if ((iResult= fForEach(&pLink->tIfaceAddr, pContext)) != 0)
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
void node_addresses_dump(SLogStream * pStream, SNetNode * pNode)
{
  unsigned int uIndex;
  SNetLink * pLink;

  // Show loopback address
  ip_address_dump(pStream, pNode->tAddr);

  // Show interface addresses
  for (uIndex= 0; uIndex < ptr_array_length(pNode->pLinks); uIndex++) {
    pLink= (SNetLink *) pNode->pLinks->data[uIndex];
    if (pLink->uType == NET_LINK_TYPE_ROUTER)
      continue;

    log_printf(pStream, " ");
    ip_address_dump(pStream, pLink->tIfaceAddr);
  }
}

// -----[ node_ifaces_dump ]-----------------------------------------
/**
 * This function shows the list of interfaces of a given node's, along
 * with their type.
 */
void node_ifaces_dump(SLogStream * pStream, SNetNode * pNode)
{
  unsigned int uIndex;
  SNetLink * pLink;

  // Show loobpack interface
  log_printf(pStream, "lo\t");
  ip_address_dump(pStream, pNode->tAddr);
  log_printf(pStream, "\n");

  // Show link interfaces
  for (uIndex= 0; uIndex < ptr_array_length(pNode->pLinks); uIndex++) {
    pLink= (SNetLink *) pNode->pLinks->data[uIndex];
    if ((pLink->uType == NET_LINK_TYPE_TRANSIT) ||
	(pLink->uType == NET_LINK_TYPE_STUB)) {
      log_printf(pStream, "ptmp\t");
      ip_address_dump(pStream, pLink->tIfaceAddr);
      log_printf(pStream, "\t");
      ip_prefix_dump(pStream, pLink->tDest.pSubnet->sPrefix);
    } else {
      if (pLink->uType == NET_LINK_TYPE_ROUTER)
	log_printf(pStream, "ptp\t");
      else
	log_printf(pStream, "tun\t");
      ip_address_dump(pStream, pLink->tIfaceAddr);
      log_printf(pStream, "\t");
      ip_address_dump(pStream, pLink->tDest.pNode->tAddr);
    }
    log_printf(pStream, "\n");
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
      if (pIface->tDest.pNode->tAddr != tNextHop)
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
// PROTOCOL FUNCTIONS
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


/////////////////////////////////////////////////////////////////////
//
// TRAFFIC LOAD FUNCTIONS
//
/////////////////////////////////////////////////////////////////////

typedef struct {
  SNetNode *   pTargetNode;
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
  SNetNode * pNode= pCtx->pTargetNode;
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
int node_load_netflow(SNetNode * pNode, const char * pcFileName,
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

