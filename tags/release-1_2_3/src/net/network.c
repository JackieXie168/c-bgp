// ==================================================================
// @(#)network.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 4/07/2003
// @lastdate 28/09/2006
// ==================================================================

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <string.h>
#include <libgds/log.h>
#include <libgds/memory.h>
#include <libgds/patricia-tree.h>
#include <libgds/stack.h>
#include <libgds/str_util.h>
#include <net/net_types.h>
#include <net/prefix.h>
#include <net/icmp.h>
#include <net/ipip.h>
#include <net/link-list.h>
#include <net/network.h>
#include <net/net_path.h>
#include <net/node.h>
#include <net/ospf.h>
#include <net/ospf_rt.h>
#include <net/subnet.h>
#include <ui/output.h>
#include <bgp/message.h>
#include <libgds/fifo.h>
#include <bgp/as_t.h>
#include <bgp/rib.h>

const net_addr_t MAX_ADDR= MAX_UINT32_T;

SNetwork * pTheNetwork= NULL;

// ----- options -----
uint8_t NET_OPTIONS_MAX_HOPS= 30;
uint8_t NET_OPTIONS_IGP_INTER= 0;

// ----- network_perror ---------------------------------------------
/**
 * Dump an error message for the given error code.
 */
void network_perror(SLogStream * pStream, int iErrorCode)
{
  switch (iErrorCode) {
  case NET_SUCCESS:
    log_printf(pStream, "success"); break;
  case NET_ERROR_UNKNOWN_PROTOCOL:
    log_printf(pStream, "unknown protocol"); break;
  case NET_ERROR_NO_ROUTE_TO_HOST:
    log_printf(pStream, "no route to host"); break;
  case NET_ERROR_TTL_EXPIRED:
    log_printf(pStream, "ttl expired"); break;
  case NET_ERROR_LINK_DOWN:
    log_printf(pStream, "link down"); break;
  case NET_ERROR_PROTOCOL_ERROR:
    log_printf(pStream, "protocol error"); break;
  case NET_ERROR_DST_UNREACHABLE:
    log_printf(pStream, "destination unreachable"); break;
  default:
    log_printf(pStream, "unknown error (%i)", iErrorCode);
  }
}

// ----- _network_node_compare --------------------------------------
/*static int _network_node_compare(void * pItem1, void * pItem2,
				 unsigned int uEltSize)
{
  SNetNode * pNode1= *((SNetNode **) pItem1);
  SNetNode * pNode2= *((SNetNode **) pItem2);

  if (pNode1->tAddr < pNode2->tAddr)
    return -1;
  else if (pNode1->tAddr > pNode2->tAddr)
    return 1;
  else
    return 0;
    }*/

// ----- node_get_prefix---------------------------------------------
void node_get_prefix(SNetNode * pNode, SPrefix * pPrefix){
  pPrefix->tNetwork = pNode->tAddr;
  pPrefix->uMaskLen = 32;
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

// ----- node_add_tunnel --------------------------------------------
/**
 *
 */
int node_add_tunnel(SNetNode * pNode, net_addr_t tDstPoint)
{
  /*  SNetLink * pLink= create_link_toRouter_byAddr(tDstPoint);
  
  //pLink->tAddr= tDstPoint;
  pLink->tDelay= 0;
  pLink->uFlags= NET_LINK_FLAG_UP | NET_LINK_FLAG_TUNNEL;
  pLink->uIGPweight= 0;
  pLink->pContext= pNode;
  //pLink->fForward= ipip_link_forward;
  pLink->fForward= NULL;
  return ptr_array_add(pNode->pLinks, &pLink);
  */
  return -1;
}

// ----- node_ipip_enable -------------------------------------------
/**
 *
 */
int node_ipip_enable(SNetNode * pNode)
{
  /*
  return node_register_protocol(pNode, NET_PROTOCOL_IPIP,
				pNode, NULL,
				ipip_event_handler);
  */
  return -1;
}

// ----- node_igp_domain_add -------------------------------------------
int node_igp_domain_add(SNetNode * pNode, uint16_t uDomainNumber)
{
  return _array_add((SArray*)(pNode->pIGPDomains), &uDomainNumber);
}

// ----- node_igp_domain_add -------------------------------------------
/** Return TRUE (1) if node belongs to igp domain tDomainNumber
 * FALSE (0) otherwise.
 */
int node_belongs_to_igp_domain(SNetNode * pNode, uint16_t uDomainNumber)
{
  unsigned int uIndex;

  if (_array_sorted_find_index((SArray*)(pNode->pIGPDomains),
			       &uDomainNumber, &uIndex) == 0)
    return 1;
  return 0;
}

// ----- node_links_dump --------------------------------------------
/**
 * Dump all the node's links.
 */
void node_links_dump(SLogStream * pStream, SNetNode * pNode)
{
  net_links_dump(pStream, pNode->pLinks);
}

// ----- node_links_for_each ----------------------------------------
int node_links_for_each(SNetNode * pNode, FArrayForEach fForEach, void * pContext)
{
  return _array_for_each((SArray *) pNode->pLinks, fForEach, pContext);
}

// ----- node_links_lookup ------------------------------------------
/**
 * This function looks for a direct link towards a node. This is
 * similar to looking for a node's outgoing interface to a directly
 * connected node.
 *
 * Note: this function is used before forwarding...
 */
SNetLink * node_links_lookup(SNetNode * pNode,
			     net_addr_t tDstAddr)
{
  SNetLink * pLink;

  // Link lookup: O(log(n))
  pLink= node_find_link_to_router(pNode, tDstAddr);
  if (pLink == NULL)
    return NULL; // No link available towards this node

  return pLink;
}

// ----- node_rt_dump -----------------------------------------------
/**
 * Dump the node's routing table.
 */
void node_rt_dump(SLogStream * pStream, SNetNode * pNode, SNetDest sDest)
{
  if (pNode->pRT != NULL)
    rt_dump(pStream, pNode->pRT, sDest);

  log_flush(pStream);
}

// ----- node_rt_lookup ---------------------------------------------
/**
 * This function looks for the next-hop that must be used to reach a
 * destination address. The function looks up the static/IGP/BGP
 * routing table.
 */
SNetRouteNextHop * node_rt_lookup(SNetNode * pNode, net_addr_t tDstAddr)
{
  SNetRouteNextHop * pNextHop= NULL;
  SNetRouteInfo * pRouteInfo;

  // Is there a static or IGP route ?
  if (pNode->pRT != NULL) {
    pRouteInfo= rt_find_best(pNode->pRT, tDstAddr, NET_ROUTE_ANY);
    if (pRouteInfo != NULL)
      pNextHop= &pRouteInfo->sNextHop;
  }

  return pNextHop;
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
    if ((pLink->uDestinationType == NET_LINK_TYPE_TRANSIT) ||
	(pLink->uDestinationType == NET_LINK_TYPE_STUB)) {
      if (pLink->tIfaceAddr == tAddress)
	return 1;
    }
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
    if ((pLink->uDestinationType == NET_LINK_TYPE_TRANSIT) ||
	(pLink->uDestinationType == NET_LINK_TYPE_STUB)) {
      if ((iResult= fForEach(&pLink->tIfaceAddr, pContext)) != 0)
	return iResult;
    }
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
    if ((pLink->uDestinationType == NET_LINK_TYPE_TRANSIT) ||
	(pLink->uDestinationType == NET_LINK_TYPE_STUB)) {
      log_printf(pStream, " ");
      ip_address_dump(pStream, pLink->tIfaceAddr);
    }
  }
}

// ----- node_recv --------------------------------------------------
/**
 * This function handles a message received at this node. If the node
 * is the destination, the message is delivered locally. Otherwize,
 * the function looks up if it has a route towards the destination
 * and, if so, the function forwards the message to the next-hop.
 *
 * Important: this function is responsible for freeing the message in
 * case it is delivered or in case it can not be forwarded.
 *
 * Note: the function also decreases the TTL of the message. If the
 * message is not delivered locally and if the TTL is 0, the message
 * is discarded.
 */
int node_recv(SNetNode * pNode, SNetMessage * pMessage)
{
  SNetRouteNextHop * pNextHop;
  int iResult;
  SNetProtocol * pProtocol;

  assert(pMessage->uTTL > 0);
  pMessage->uTTL--;

  //fprintf(stdout, "(");
  //ip_address_dump(stdout, pNode->tAddr);
  //fprintf(stdout, ") node-recv from ");
  //ip_address_dump(stdout, pMessage->tSrcAddr);
  //fprintf(stdout, " to ");
  //ip_address_dump(stdout, pMessage->tDstAddr);
  //fprintf(stdout, "\n");

  // Local delivery ?
  if (node_has_address(pNode, pMessage->tDstAddr)) {
    assert(pNode->pProtocols != NULL);
    pProtocol= protocols_get(pNode->pProtocols, pMessage->uProtocol);

    if (pProtocol == NULL) {
      LOG_ERR_ENABLED(LOG_LEVEL_SEVERE) {
	log_printf(pLogErr, "Error: message for unknown protocol %u\n",
		   pMessage->uProtocol);
	log_printf(pLogErr, "Error: received by ");
	ip_address_dump(pLogErr, pNode->tAddr);
	log_printf(pLogErr, "\n");
      }
      iResult= NET_ERROR_UNKNOWN_PROTOCOL;
    } else
      iResult= pProtocol->fHandleEvent(pProtocol->pHandler, pMessage);

    // Free message only (not the payload, which is now handled by the
    // protocol)
    pMessage->pPayLoad= NULL;
    message_destroy(&pMessage);
    return iResult;
  }

  // Time-To-Live expired ?
  if (pMessage->uTTL == 0) {
    LOG_ERR_ENABLED(LOG_LEVEL_SEVERE) {
      log_printf(pLogErr, "Info: TTL expired\n");
      log_printf(pLogErr, "Info: message from ");
      ip_address_dump(pLogErr, pMessage->tSrcAddr);
      log_printf(pLogErr, " to ");
      ip_address_dump(pLogErr, pMessage->tDstAddr);
      log_printf(pLogErr, "\n");
    }
    message_destroy(&pMessage);
    return NET_ERROR_TTL_EXPIRED;
  }

  // Find route to destination
  pNextHop= node_rt_lookup(pNode, pMessage->tDstAddr);
  if (pNextHop == NULL) {
    LOG_ERR_ENABLED(LOG_LEVEL_SEVERE) {
      log_printf(pLogErr, "Info: no route to host at ");
      ip_address_dump(pLogErr, pNode->tAddr);
      log_printf(pLogErr, "\n");
      log_printf(pLogErr, "Info: message from ");
      ip_address_dump(pLogErr, pMessage->tSrcAddr);
      log_printf(pLogErr, " to ");
      ip_address_dump(pLogErr, pMessage->tDstAddr);
      log_printf(pLogErr, "\n");
    }
    message_destroy(&pMessage);
    return NET_ERROR_NO_ROUTE_TO_HOST;
  }

  // Forward...
  return network_forward(pNode->pNetwork,
			 pNextHop->pIface,
			 pNextHop->tAddr,
			 pMessage);
}

// ----- node_send --------------------------------------------------
/**
 * Send a message to a node.
 * The node must be directly connected to the sender node or there
 * must be a route towards the node in the sender's routing table.
 */
int node_send(SNetNode * pNode, net_addr_t tSrcAddr, net_addr_t tDstAddr,
	      uint8_t uProtocol, void * pPayLoad, FPayLoadDestroy fDestroy)
{
  SNetRouteNextHop * pNextHop;
  SNetMessage * pMessage;

  // Find outgoing interface & next-hop
  pNextHop= node_rt_lookup(pNode, tDstAddr);
  if (pNextHop == NULL) {
    if (fDestroy != NULL)
      fDestroy(&pPayLoad);
    return NET_ERROR_NO_ROUTE_TO_HOST;
  }

  if (tSrcAddr == 0) {
    if (pNextHop->pIface->tIfaceAddr != 0)
      tSrcAddr= pNextHop->pIface->tIfaceAddr;
    else
      tSrcAddr= pNode->tAddr;
  }

  // Build message
  pMessage= message_create(tSrcAddr, tDstAddr,
			   uProtocol, 255,
			   pPayLoad, fDestroy);

  //fprintf(stdout, "(");
  //ip_address_dump(stdout, pNode->tAddr);
  //fprintf(stdout, ") node-send from ");
  //ip_address_dump(stdout, tSrcAddr);
  //fprintf(stdout, " to ");
  //ip_address_dump(stdout, tDstAddr);
  //fprintf(stdout, "\n");

  // Forward
  return network_forward(pNode->pNetwork,
			 pNextHop->pIface,
			 pNextHop->tAddr,
			 pMessage);
}


/////////////////////////////////////////////////////////////////////
//
// NETWORK METHODS
//
/////////////////////////////////////////////////////////////////////

// ----- network_nodes_destroy --------------------------------------
void network_nodes_destroy(void ** ppItem)
{
  node_destroy((SNetNode **) ppItem);
}

// ----- network_subnets_destroy ------------------------------------
void network_subnets_destroy(void * pItem)
{
  subnet_destroy(((SNetSubnet **) pItem));
}

// ----- network_create ---------------------------------------------
/**
 *
 */
SNetwork * network_create()
{
  SNetwork * pNetwork= (SNetwork *) MALLOC(sizeof(SNetwork));
  
  pNetwork->pNodes= trie_create(network_nodes_destroy);
  pNetwork->pSubnets= ptr_array_create(ARRAY_OPTION_SORTED|
				       ARRAY_OPTION_UNIQUE,
				       subnets_compare,
				       network_subnets_destroy);
  return pNetwork;
}

// ----- network_destroy --------------------------------------------
/**
 *
 */
void network_destroy(SNetwork ** ppNetwork)
{
  if (*ppNetwork != NULL) {
    trie_destroy(&(*ppNetwork)->pNodes);
    ptr_array_destroy(&(*ppNetwork)->pSubnets);
    FREE(*ppNetwork);
    *ppNetwork= NULL;
  }
}

// ----- network_get ------------------------------------------------
/**
 *
 */
SNetwork * network_get()
{
  if (pTheNetwork == NULL)
    pTheNetwork= network_create();
  return pTheNetwork;
}


// ----- network_add_node -------------------------------------------
/**
 *
 */
int network_add_node(SNetNode * pNode)
{
  pNode->pNetwork= pTheNetwork;
  return trie_insert(pTheNetwork->pNodes, pNode->tAddr, 32, pNode);
}

// ----- network_add_subnet -------------------------------------------
/**
 *
 */
int network_add_subnet(SNetSubnet * pSubnet)
{
  return ptr_array_add(pTheNetwork->pSubnets, &pSubnet);
}

// ----- network_find_node ------------------------------------------
/**
 *
 */
SNetNode * network_find_node(net_addr_t tAddr)
{
  return (SNetNode *) trie_find_exact(pTheNetwork->pNodes, tAddr, 32);
}

// ----- network_find_subnet ----------------------------------------
/**
 *
 */
SNetSubnet * network_find_subnet(SPrefix sPrefix)
{ 
  unsigned int uIndex;
  SNetSubnet * pSubnet = NULL;
  SNetSubnet * pWrapSubnet;

  pWrapSubnet= subnet_create(sPrefix.tNetwork, sPrefix.uMaskLen, 0);
  if (ptr_array_sorted_find_index(pTheNetwork->pSubnets,
				  &pWrapSubnet, &uIndex) == 0) 
    ptr_array_get_at(pTheNetwork->pSubnets, uIndex, &pSubnet);
  
  subnet_destroy(&pWrapSubnet);
  return pSubnet;
}

// ----- network_forward --------------------------------------------
/**
 * This function forwards a message through the given link. The
 * function first checks if the link is up.
 */
int network_forward(SNetwork * pNetwork, SNetLink * pLink,
		    net_addr_t tNextHop, SNetMessage * pMsg)
{
  SNetNode * pNextHop= NULL;
  int iResult;

  if (tNextHop == 0)
    tNextHop= pMsg->tDstAddr;

  // Forward along this link...
  assert(pLink->fForward != NULL);
  iResult= pLink->fForward(tNextHop, pLink->pContext, &pNextHop);

  // If one link was DOWN, drop the message.
  if (iResult == NET_ERROR_LINK_DOWN) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Info: link is down, message dropped\n");
    _link_drop(pLink, pMsg);
    return iResult;
  }

  //fprintf(stdout, "(*) network-forward ");
  //ip_address_dump(stdout, pMsg->tSrcAddr);
  //fprintf(stdout, " to ");
  //ip_address_dump(stdout, pMsg->tDstAddr);
  //fprintf(stdout, " next-hop ");
  //ip_address_dump(stdout, tNextHop);
  //fprintf(stdout, " => ");
  //ip_address_dump(stdout, pNextHop->tAddr);
  //fprintf(stdout, "\n");

  // Forward to node...
  if (iResult == NET_SUCCESS) {
    assert(pNextHop != NULL);
    assert(!simulator_post_event(_network_send_callback,
				 _network_send_dump,
				 _network_send_context_destroy,
				 _network_send_context_create(pNextHop,
							      pMsg),
				 0, RELATIVE_TIME));
  }

  return iResult;
}

// ----- _network_nodes_to_file -------------------------------------
/*
static int _network_nodes_to_file(uint32_t uKey, uint8_t uKeyLen,
				  void * pItem, void * pContext)
{
  SNetNode * pNode= (SNetNode *) pItem;
  SLogStream * pStream= (SLogStream *) pContext;
  SNetLink * pLink;
  int iLinkIndex;

  for (iLinkIndex= 0; iLinkIndex < ptr_array_length(pNode->pLinks);
       iLinkIndex++) {
    pLink= (SNetLink *) pNode->pLinks->data[iLinkIndex];
    link_dump(pStream, pLink);
    log_printf(pStream, "\n");
  }
  return 0;
}
*/

// ----- network_to_file --------------------------------------------
/**
 *
 */
int network_to_file(SLogStream * pStream, SNetwork * pNetwork)
{
  SEnumerator * pEnum= trie_get_enum(pTheNetwork->pNodes);
  SNetNode * pNode;
  /*
  return trie_for_each(pNetwork->pNodes, _network_nodes_to_file,
		       pStream);
  */
  while (enum_has_next(pEnum)) {
    pNode= *(SNetNode **) enum_get_next(pEnum);
    node_dump(pStream, pNode);
  }
  enum_destroy(&pEnum);
  return 0;
}

// ----- network_from_file ------------------------------------------
/**
 *
 */
SNetwork * network_from_file(FILE * pStream)
{
  return NULL;
}

typedef struct {
  net_addr_t tAddr;
  SNetPath * pPath;
  net_link_delay_t tDelay;
} SContext;

// ----- _network_shortest_path_destroy -----------------------------
static void _network_shortest_path_destroy(void ** pItem)
{
  net_path_destroy((SNetPath **) pItem);
}

// ----- _network_shortest_for_each ---------------------------------
static int _network_shortest_for_each(uint32_t uKey, uint8_t uKeyLen,
				      void * pItem, void * pContext)
{
  SNetPath * pPath= (SNetPath *) pItem;
  SLogStream * pStream= (SLogStream *) pContext;
  
  log_printf(pStream, "--> %u\t[", uKey);
  net_path_dump(pStream, pPath);
  log_printf(pStream, "]\n");
  return 0;
}
  
// ----- network_shortest_path --------------------------------------
/**
 * Calculate shortest path from the given source AS towards all the
 * other ASes.
 */
int network_shortest_path(SNetwork * pNetwork, SLogStream * pStream,
			  net_addr_t tSrcAddr)
{
  SNetLink * pLink;
  SNetNode * pNode;
  SFIFO * pFIFO;
  SRadixTree * pVisited;
  SContext * pContext, * pOldContext;
  int iIndex;

  pVisited= radix_tree_create(32, _network_shortest_path_destroy);
  
  pFIFO= fifo_create(100000, NULL);
  pContext= (SContext *) MALLOC(sizeof(SContext));
  pContext->tAddr= tSrcAddr;
  pContext->pPath= net_path_create();
  fifo_push(pFIFO, pContext);
  radix_tree_add(pVisited, tSrcAddr, 32, net_path_copy(pContext->pPath));

  // Breadth-first search
  while (1) {
    pContext= (SContext *) fifo_pop(pFIFO);
    if (pContext == NULL)
      break;
    pNode= network_find_node(pContext->tAddr);

    pOldContext= pContext;

    for (iIndex= 0; iIndex < ptr_array_length(pNode->pLinks);
	 iIndex++) {
      pLink= (SNetLink *) pNode->pLinks->data[iIndex];
      if (radix_tree_get_exact(pVisited, link_get_address(pLink), 32) == NULL) {
	pContext= (SContext *) MALLOC(sizeof(SContext));
	pContext->tAddr= link_get_address(pLink);
	pContext->pPath= net_path_copy(pOldContext->pPath);
	net_path_append(pContext->pPath, link_get_address(pLink));
	radix_tree_add(pVisited, link_get_address(pLink), 32,
		       net_path_copy(pContext->pPath));
	assert(fifo_push(pFIFO, pContext) == 0);
      }
    }
    net_path_destroy(&pOldContext->pPath);
    FREE(pOldContext);
  }
  fifo_destroy(&pFIFO);

  radix_tree_for_each(pVisited, _network_shortest_for_each, pStream);

  radix_tree_destroy(&pVisited);
  return 0;
}

//---- network_dump_subnets ---------------------------------------------
void network_dump_subnets(SLogStream * pStream, SNetwork *pNetwork)
{
  //  int iIndex, /*totSub*/;
  SNetSubnet * pSubnet = NULL;
  SEnumerator * pEnum= _array_get_enum((SArray*) pNetwork->pSubnets);
  while (enum_has_next(pEnum)) {
    pSubnet= *(SNetSubnet **) enum_get_next(pEnum);
    ip_prefix_dump(pStream, pSubnet->sPrefix);
    log_printf(pStream, "\n");
  }
  enum_destroy(&pEnum);
  /*
  totSub = ptr_array_length(pNetwork->pSubnets);
  for (iIndex = 0; iIndex < totSub; iIndex++){
    ptr_array_get_at(pNetwork->pSubnets, iIndex, &pSubnet);
    ip_prefix_dump(pStream, pSubnet->sPrefix);
    fprintf(pStream, "\n");
    }*/
}

// ----- network_enum_nodes -----------------------------------------
/**
 * This function can be used by the CLI to enumerate all known nodes.
 */
char * network_enum_nodes(const char * pcText, int state)
{
  static SEnumerator * pEnum= NULL;
  SNetNode * pNode;
  char acNode[16];

  if (state == 0)
    pEnum= trie_get_enum(pTheNetwork->pNodes);
  while (enum_has_next(pEnum)) {
    pNode= *((SNetNode **) enum_get_next(pEnum));
    ip_address_to_string(acNode, pNode->tAddr);
    if (!strncmp(pcText, acNode, strlen(pcText)))
      return strdup(acNode);
  }
  enum_destroy(&pEnum);
  return NULL;
}

// ----- network_enum_bgp_nodes -------------------------------------
/**
 * This function can be used by the CLI to enumerate all known nodes
 * that support BGP.
 */
char * network_enum_bgp_nodes(const char * pcText, int state)
{
  static SEnumerator * pEnum= NULL;
  SNetNode * pNode;
  char acNode[16];

  if (state == 0)
    pEnum= trie_get_enum(pTheNetwork->pNodes);
  while (enum_has_next(pEnum)) {
    pNode= *((SNetNode **) enum_get_next(pEnum));

    // Test if node supports BGP. If not, skip...
    if (node_get_protocol(pNode, NET_PROTOCOL_BGP) == NULL)
      continue;

    ip_address_to_string(acNode, pNode->tAddr);
    if (!strncmp(pcText, acNode, strlen(pcText)))
      return strdup(acNode);
  }
  enum_destroy(&pEnum);
  return NULL;
}

/////////////////////////////////////////////////////////////////////
//
// NETWORK MESSAGE PROPAGATION FUNCTIONS
//
/////////////////////////////////////////////////////////////////////

// ----- _network_send_callback -------------------------------------
/**
 * This function handles a message event from the simulator.
 */
int _network_send_callback(void * pContext)
{
  SNetSendContext * pSendContext= (SNetSendContext *) pContext;
  int iResult;

  // Deliver message to next-hop which will use the message localy or
  // forward it...
  iResult= node_recv(pSendContext->pNode, pSendContext->pMessage);

  // Free the message context. The message MUST be freed by
  // network_node_recv if the message has been delivered or in case
  // the message can not be forwarded.
  FREE(pContext);
  return iResult;
}

// ----- _network_send_dump -----------------------------------------
/**
 * Callback function used to dump the content of a message event. See
 * also 'simulator_dump_events' (sim/simulator.c).
 */
void _network_send_dump(SLogStream * pStream, void * pContext)
{
  SNetSendContext * pSendContext= (SNetSendContext *) pContext;

  log_printf(pStream, "net-msg n-h:");
  ip_address_dump(pStream, pSendContext->pNode->tAddr);
  log_printf(pStream, " [");
  message_dump(pStream, pSendContext->pMessage);
  log_printf(pStream, "]");
  if (pSendContext->pMessage->uProtocol == NET_PROTOCOL_BGP) {
    bgp_msg_dump(pStream, NULL, pSendContext->pMessage->pPayLoad);
  }
}

// ----- _network_send_context_create -------------------------------
/**
 * This function creates a message context to be pushed on the
 * simulator's events queue. The context contains the message and the
 * destination node.
 */
SNetSendContext * _network_send_context_create(SNetNode * pNode,
					       SNetMessage * pMessage)
{
  SNetSendContext * pSendContext=
    (SNetSendContext *) MALLOC(sizeof(SNetSendContext));
  
  pSendContext->pNode= pNode;
  pSendContext->pMessage= pMessage;
  return pSendContext;
}

// ----- _network_send_context_destroy -------------------------------
/**
 * This function is used by the simulator to free events which will
 * not be processed.
 */
void _network_send_context_destroy(void * pContext)
{
  SNetSendContext * pSendContext= (SNetSendContext *) pContext;

  message_destroy(&pSendContext->pMessage);
  FREE(pSendContext);
}

/////////////////////////////////////////////////////////////////////
//
// INITIALIZATION AND FINALIZATION SECTION
//
/////////////////////////////////////////////////////////////////////

// ----- _network_create --------------------------------------------
void _network_create()
{
  pTheNetwork= network_create();
}

// ----- _network_destroy -------------------------------------------
/**
 *
 */
void _network_destroy()
{
  if (pTheNetwork != NULL) {
    network_destroy(&pTheNetwork);
  }
}


/////////////////////////////////////////////////////////////////////
//
// TESTS
//
/////////////////////////////////////////////////////////////////////

// ----- node_handle_event ------------------------------------------
int node_handle_event(void * pHandler, SNetMessage * pMessage)
{
  SNetNode * pNode= (SNetNode *) pHandler;
  int iPayLoad= (int) (long) pMessage->pPayLoad;

  printf("node_event[");
  ip_address_dump(pLogOut, pNode->tAddr);
  printf("]: from ");
  ip_address_dump(pLogOut, pMessage->tSrcAddr);
  printf(", %d", iPayLoad);

  return 0;
}

// ----- _network_test ----------------------------------------------
void _network_test()
{
  SNetwork * pNetwork= network_create();
  SNetNode * pNodeA0= node_create(0);
  SNetNode * pNodeA1= node_create(1);
  SNetNode * pNodeB0= node_create(65536);
  SNetNode * pNodeB1= node_create(65537);
  SNetNode * pNodeC0= node_create(131072);
  SLogStream * pNetStream;

  node_register_protocol(pNodeA0, NET_PROTOCOL_ICMP, pNodeA0, NULL,
			 node_handle_event);
  node_register_protocol(pNodeA1, NET_PROTOCOL_ICMP, pNodeA1, NULL,
			 node_handle_event);
  node_register_protocol(pNodeB0, NET_PROTOCOL_ICMP, pNodeB0, NULL,
			 node_handle_event);
  node_register_protocol(pNodeB1, NET_PROTOCOL_ICMP, pNodeB1, NULL,
			 node_handle_event);

  simulator_init();

  assert(!network_add_node(pNodeA0));
  assert(!network_add_node(pNodeA1));
  assert(!network_add_node(pNodeB0));
  assert(!network_add_node(pNodeB1));
  assert(!network_add_node(pNodeC0));

  LOG_DEBUG(LOG_LEVEL_DEBUG, "nodes attached.\n");

  assert(node_add_link_to_router(pNodeA0, pNodeA1, 100, 1) >= 0);
  assert(node_add_link_to_router(pNodeA1, pNodeB0, 1000, 1) >= 0);
  assert(node_add_link_to_router(pNodeB0, pNodeB1, 100, 1) >= 0);
  assert(node_add_link_to_router(pNodeA1, pNodeC0, 400, 1) >= 0);
  assert(node_add_link_to_router(pNodeC0, pNodeB0, 400, 1) >= 0);

  LOG_DEBUG(LOG_LEVEL_DEBUG, "links attached.\n");

  pNetStream= log_create_file("net-file");
  assert(pNetStream != NULL);
  assert(!network_to_file(pNetStream, pNetwork));
  log_destroy(&pNetStream);

  LOG_DEBUG(LOG_LEVEL_DEBUG, "network to file.\n");

  assert(!node_send(pNodeA0, 0, 1, 0, (void *) 5, NULL));
  simulator_run();

  LOG_DEBUG(LOG_LEVEL_DEBUG, "message sent.\n");

  network_shortest_path(pNetwork, pLogErr, pNodeA0->tAddr);

  LOG_DEBUG(LOG_LEVEL_DEBUG, "shortest-path computed.\n");

  //network_dijkstra(pNetwork, stderr, pNodeA0->tAddr);

  LOG_DEBUG(LOG_LEVEL_DEBUG, "dijkstra computed.\n");

  network_destroy(&pNetwork);

  LOG_DEBUG(LOG_LEVEL_DEBUG, "network done.\n");

  /*
  pNetFile= fopen("net-file", "r");
  assert(pNetFile != NULL);
  pNetwork= network_from_file(pNetFile);
  assert(pNetwork != NULL);
  fclose(pNetFile);

  LOG_DEBUG("network from file.\n");
  */

  simulator_done();

  exit(EXIT_SUCCESS);
}
