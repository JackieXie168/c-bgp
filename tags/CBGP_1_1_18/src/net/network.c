// ==================================================================
// @(#)network.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 4/07/2003
// @lastdate 30/09/2004
// ==================================================================

#include <assert.h>
#include <net/prefix.h>
#include <libgds/log.h>
#include <libgds/memory.h>
#include <libgds/stack.h>
#include <net/icmp.h>
#include <net/ipip.h>
#include <net/network.h>
#include <net/net_path.h>
#include <string.h>
#include <ui/output.h>

#include <net/domain.h>

const net_addr_t MAX_ADDR= MAX_UINT32_T;

SNetwork * pTheNetwork= NULL;

typedef struct {
  SNetNode * pNode;
  SNetMessage * pMessage;
} SNetSendContext;

// ----- options -----
uint8_t NET_OPTIONS_MAX_HOPS= 30;
uint8_t NET_OPTIONS_IGP_INTER= 0;

int network_forward(SNetwork * pNetwork, SNetLink * pLink,
		    SNetMessage * pMessage);


// ----- network_send_callback --------------------------------------
/**
 * This function handles a message event from the simulator.
 */
int network_send_callback(void * pContext)
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

// ----- network_send_context_create --------------------------------
/**
 * This function creates a message context to be pushed on the
 * simulator's events queue. The context contains the message and the
 * destination node.
 */
SNetSendContext * network_send_context_create(SNetNode * pNode,
					      SNetMessage * pMessage)
{
  SNetSendContext * pSendContext=
    (SNetSendContext *) MALLOC(sizeof(SNetSendContext));
  
  pSendContext->pNode= pNode;
  pSendContext->pMessage= pMessage;
  return pSendContext;
}

// ----- network_send_context_destroy -------------------------------
/**
 * This function is used by the simulator to free events which will
 * not be processed.
 */
void network_send_context_destroy(void * pContext)
{
  SNetSendContext * pSendContext= (SNetSendContext *) pContext;

  message_destroy(&pSendContext->pMessage);
  FREE(pSendContext);
}

// ----- node_interface_link_add -------------------------------------
/**
 *
 *
 */
int node_interface_link_add(SNetNode * pNode, net_addr_t tFromIf, 
    net_addr_t tToIf, net_link_delay_t tDelay)
{
  SNetInterface * pInterface = MALLOC(sizeof(SNetInterface));
  SNetLink * pLink= MALLOC(sizeof(SNetLink));
  SPtrArray * pLinks;
  unsigned int uIndex;

  pInterface->tAddr = tFromIf;

  if (ptr_array_sorted_find_index(pNode->aInterfaces, &pInterface, &uIndex)) {
    LOG_DEBUG("node_interface_link_add>interface not found %ul\n", tFromIf);
    return -1;
  }
  pLinks = ((SNetInterface *)pNode->aInterfaces->data[uIndex])->pLinks;

  pLink->tAddr= tToIf;
  pLink->tDelay= tDelay;
  pLink->uFlags= NET_LINK_FLAG_UP | NET_LINK_FLAG_IGP_ADV;
  pLink->uIGPweight= tDelay;
  pLink->pContext= NULL;
  pLink->fForward= NULL;
  return ptr_array_add(pLinks, &pLink);
}

// ----- node_links_compare -----------------------------------------
int node_links_compare(void * pItem1, void * pItem2,
		       unsigned int uEltSize)
{
  SNetLink * pLink1= *((SNetLink **) pItem1);
  SNetLink * pLink2= *((SNetLink **) pItem2);

  if (pLink1->tAddr < pLink2->tAddr)
    return -1;
  else if (pLink1->tAddr > pLink2->tAddr)
    return 1;
  else
    return 0;
}

// ----- node_links_destroy -----------------------------------------
void node_links_destroy(void * pItem)
{
  FREE(*((SNetLink **) pItem));
  *((SNetLink **) pItem)= NULL;
}

// ----- node_get_name -----------------------------------------------
char * node_get_name(SNetNode * pNode)
{
  return pNode->pcName;
}

// ----- node_set_name -----------------------------------------------
void node_set_name(SNetNode * pNode, const char * pcName)
{
  char * pcNameDup = strdup(pcName);
  
  if (pNode->pcName)
    FREE(pNode->pcName);
  
  pNode->pcName = pcNameDup;
}

// ----- node_interface_compare --------------------------------------
/**
 *
 *
 */
int node_interface_compare(void * pItem1, void * pItem2,
				unsigned int uEltSize)
{
  SNetInterface * pInterface1 = *((SNetInterface **)pItem1);
  SNetInterface * pInterface2 = *((SNetInterface **)pItem2);

  if (pInterface1->tAddr < pInterface2->tAddr)
    return -1;
  else if (pInterface1->tAddr > pInterface2->tAddr)
    return 1;
  else
    return 0;
}

// ----- node_interface_destroy --------------------------------------
/**
 *
 *
 */
void node_interface_destroy(void * pItem)
{
  if (*(SNetInterface **)pItem) {
    FREE((*(SNetInterface **)pItem)->tMask);
    ptr_array_destroy(&(*(SNetInterface **)pItem)->pLinks);
    FREE(*(SNetInterface **)pItem);
    *(SNetInterface **)pItem = NULL;
  }
}

// ----- node_interface_create ---------------------------------------
/**
 *
 *
 */
SNetInterface * node_interface_create()
{
  SNetInterface * pInterface = MALLOC(sizeof(SNetInterface));

  pInterface->tAddr = 0;
  pInterface->tMask = NULL;
  pInterface->pLinks = ptr_array_create(ARRAY_OPTION_UNIQUE|ARRAY_OPTION_SORTED,
			  node_links_compare, 
			  node_links_destroy);
  return pInterface;
}

// ----- node_interface_add ------------------------------------------
/**
 *
 *
 */
int node_interface_add(SNetNode * pNode, SNetInterface * pInterface)
{
  //We do not create aInterfaces when creating a node 'cause
  //it's only used while loading a topology via a xml file
  if (pNode->aInterfaces == NULL)
    pNode->aInterfaces = ptr_array_create(ARRAY_OPTION_UNIQUE|
					  ARRAY_OPTION_SORTED,
					  node_interface_compare,
					  node_interface_destroy);

  return ptr_array_add(pNode->aInterfaces, (void *)&pInterface);
}
 
// ----- node_interface_get_number -----------------------------------
/**
 *
 *
 */
unsigned int node_interface_get_number(SNetNode * pNode)
{
  if (pNode->aInterfaces)
    return ptr_array_length(pNode->aInterfaces);
  else
    return 0;
}

// ----- node_interface_get ------------------------------------------
/**
 *
 *
 */
SNetInterface * node_interface_get(SNetNode * pNode, 
				const unsigned int uIndex)
{
  return (SNetInterface *)pNode->aInterfaces->data[uIndex];
}

// ----- node_set_as -------------------------------------------------
/**
 *
 *
 */
void node_set_as(SNetNode * pNode, SNetDomain * pDomain)
{
  if (pNode)
    pNode->pDomain = pDomain;
}

// ----- node_get_as -------------------------------------------------
/**
 *
 *
 */
uint32_t node_get_as_id(SNetNode * pNode)
{
  return domain_get_as(pNode->pDomain);
}

// ----- node_get_as -------------------------------------------------
/**
 *
 *
 */
SNetDomain * node_get_as(SNetNode * pNode)
{
  return pNode->pDomain;
}

// ----- node_create ------------------------------------------------
/**
 *
 */
SNetNode * node_create(net_addr_t tAddr)
{
  SNetNode * pNode= (SNetNode *) MALLOC(sizeof(SNetNode));
  pNode->tAddr= tAddr;
  pNode->aInterfaces = NULL;
  pNode->pcName = NULL;
  pNode->uAS = 0;
  pNode->pLinks= ptr_array_create(ARRAY_OPTION_SORTED|ARRAY_OPTION_UNIQUE,
				  node_links_compare,
				  node_links_destroy);
  pNode->pRT= rt_create();
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
    ptr_array_destroy(&(*ppNode)->pLinks);
    ptr_array_destroy(&(*ppNode)->aInterfaces);
    if ((*ppNode)->pcName)
      FREE((*ppNode)->pcName);
    FREE(*ppNode);
    *ppNode= NULL;
  }
}

// ----- node_add_link ----------------------------------------------
/**
 *
 */
int node_add_link(SNetNode * pNodeA, SNetNode * pNodeB,
		  net_link_delay_t tDelay, int iRecurse)
{
  SNetLink * pLink= (SNetLink *) MALLOC(sizeof(SNetLink));
  
  if (iRecurse)
    if (node_add_link(pNodeB, pNodeA, tDelay, 0))
      return -1;

  pLink->tAddr= pNodeB->tAddr;
  pLink->tDelay= tDelay;
  pLink->uFlags= NET_LINK_FLAG_UP | NET_LINK_FLAG_IGP_ADV;
  pLink->uIGPweight= tDelay;
  pLink->pContext= NULL;
  pLink->fForward= NULL;
  return ptr_array_add(pNodeA->pLinks, &pLink);
}

// ----- node_dump ---------------------------------------------------
/**
 *
 *
 */
void node_dump(SNetNode * pNode)
{ 
  ip_address_dump(stdout, pNode->tAddr);
  fprintf(stdout, "\n");
}

// ----- node_link_change_delay -------------------------------------
/**
 *
 *
 */

// ----- node_add_tunnel --------------------------------------------
/**
 *
 */
int node_add_tunnel(SNetNode * pNode, net_addr_t tDstPoint)
{
  SNetLink * pLink= (SNetLink *) MALLOC(sizeof(SNetLink));

  pLink->tAddr= tDstPoint;
  pLink->tDelay= 0;
  pLink->uFlags= NET_LINK_FLAG_UP | NET_LINK_FLAG_TUNNEL;
  pLink->uIGPweight= 0;
  pLink->pContext= pNode;
  pLink->fForward= ipip_link_forward;
  return ptr_array_add(pNode->pLinks, &pLink);
}

// ----- node_find_link ---------------------------------------------
/**
 *
 */
SNetLink * node_find_link(SNetNode * pNode, net_addr_t tAddr)
{
  int iIndex;
  net_addr_t * pAddr= &tAddr;
  SNetLink * pLink= NULL;
  
  if (ptr_array_sorted_find_index(pNode->pLinks, &pAddr, &iIndex) == 0)
    pLink= (SNetLink *) pNode->pLinks->data[iIndex];
  return pLink;
}

// ----- node_ipip_enable -------------------------------------------
/**
 *
 */
int node_ipip_enable(SNetNode * pNode)
{
  return node_register_protocol(pNode, NET_PROTOCOL_IPIP,
				pNode, NULL,
				ipip_event_handler);
}

// ----- node_links_dump --------------------------------------------
void node_links_dump(FILE * pStream, SNetNode * pNode)
{
  int iIndex;

  for (iIndex= 0; iIndex < ptr_array_length(pNode->pLinks); iIndex++) {
    link_dump(pStream, (SNetLink *) pNode->pLinks->data[iIndex]);
    fprintf(pStream, "\n");
  }
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
  pLink= node_find_link(pNode, tDstAddr);
  if (pLink == NULL)
    return NULL; // No link available towards this node

  return pLink;
}

// ----- node_rt_add_route ------------------------------------------
/**
 *
 */
int node_rt_add_route(SNetNode * pNode, SPrefix sPrefix,
		      net_addr_t tNextHop,
		      uint32_t uWeight, uint8_t uType)
{
  SNetLink * pLink;
  SNetRouteInfo * pRouteInfo;

  // Lookup the next-hop's interface (no recursive lookup is allowed,
  // it must be a direct link !)
  pLink= node_links_lookup(pNode, tNextHop);
  if (pLink == NULL) {
    return NET_RT_ERROR_NH_UNREACH;
  }

  // Build route info
  pRouteInfo= route_info_create(pLink, uWeight, uType);

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
		      net_addr_t * pNextHop, uint8_t uType)
{
  SNetLink * pLink= NULL;

  // Lookup the next-hop's interface (no recursive lookup is allowed,
  // it must be a direct link !)
  if (pNextHop != NULL) {
    pLink= node_links_lookup(pNode, *pNextHop);
    if (pLink == NULL) {
      return NET_RT_ERROR_IF_UNKNOWN;
    }
  }

  return rt_del_route(pNode->pRT, pPrefix, pLink, uType);
}

// ----- node_rt_dump -----------------------------------------------
/**
 *
 */
void node_rt_dump(FILE * pStream, SNetNode * pNode, SPrefix sPrefix)
{
  if (pNode->pRT != NULL)
    rt_dump(pStream, pNode->pRT, sPrefix);

  flushir(pStream);
}

// ----- node_rt_dump_string -----------------------------------------------
/**
 *
 */
char * node_rt_dump_string(SNetNode * pNode, SPrefix sPrefix)
{
  char * cRT = NULL;

  if (pNode->pRT != NULL)
    cRT = rt_dump_string(pNode->pRT, sPrefix);

  return cRT;
}


// ----- node_rt_lookup ---------------------------------------------
/**
 * This function looks for the next-hop that must be used to reach a
 * destination address. The function first looks up the direct links,
 * then the static/IGP/BGP routing table.
 *
 * Note: the BGP routes are not inserted in the node's routing table
 * in order to avoid redundant use of memory.
 */
SNetLink * node_rt_lookup(SNetNode * pNode, net_addr_t tDstAddr)
{
  SNetLink * pLink= NULL;
  SNetRouteInfo * pRouteInfo;

  // Is there a direct link towards the destination ?
  //pLink= node_links_lookup(pNode, tDstAddr);

  // If not, is there a static or IGP route ?
  if (pLink == NULL) {
    if (pNode->pRT != NULL) {
      pRouteInfo= rt_find_best(pNode->pRT, tDstAddr, NET_ROUTE_ANY);
      if (pRouteInfo != NULL)
	pLink= pRouteInfo->pNextHopIf;
    }
  }

  return pLink;
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
  SNetLink * pLink;
  int iResult;
  SNetProtocol * pProtocol;

  assert(pMessage->uTTL > 0);
  pMessage->uTTL--;

  // Local delivery ?
  if (pMessage->tDstAddr == pNode->tAddr) {
    assert(pNode->pProtocols != NULL);
    pProtocol= protocols_get(pNode->pProtocols, pMessage->uProtocol);

    if (pProtocol == NULL) {
      LOG_SEVERE("Error: message for unknown protocol %u\n", pMessage->uProtocol);
      LOG_SEVERE("Error: received by ");
      LOG_ENABLED_SEVERE() ip_address_dump(log_get_stream(pMainLog),
					   pNode->tAddr);
      LOG_SEVERE("\n");
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
    LOG_INFO("Info: TTL expired\n");
    LOG_INFO("Info: message from ");
    LOG_ENABLED_INFO() ip_address_dump(log_get_stream(pMainLog),
				       pMessage->tSrcAddr);
    LOG_INFO(" to ");
    LOG_ENABLED_INFO() ip_address_dump(log_get_stream(pMainLog),
				       pMessage->tDstAddr);
    LOG_INFO("\n");
    message_destroy(&pMessage);
    return NET_ERROR_TTL_EXPIRED;
  }

  // Find route to destination
  pLink= node_rt_lookup(pNode, pMessage->tDstAddr);
  if (pLink == NULL) {
    LOG_INFO("Info: no route to host at ");
    LOG_ENABLED_INFO() ip_address_dump(log_get_stream(pMainLog),
				       pNode->tAddr);
    LOG_INFO("\n");
    LOG_INFO("Info: message from ");
    LOG_ENABLED_INFO() ip_address_dump(log_get_stream(pMainLog),
				       pMessage->tSrcAddr);
    LOG_INFO(" to ");
    LOG_ENABLED_INFO() ip_address_dump(log_get_stream(pMainLog),
				       pMessage->tDstAddr);
    LOG_INFO("\n");
    message_destroy(&pMessage);
    return NET_ERROR_NO_ROUTE_TO_HOST;
  }

  // Forward...
  return network_forward(pNode->pNetwork, pLink, pMessage);
}

// ----- node_send --------------------------------------------------
/**
 * Send a message to a node.
 * The node must be directly connected to the sender node or there
 * must be a route towards the node in the sender's routing table.
 */
int node_send(SNetNode * pNode, net_addr_t tDstAddr,
	      uint8_t uProtocol, void * pPayLoad, FPayLoadDestroy fDestroy)
{
  SNetLink * pLink;
  SNetMessage * pMessage;

  // Find outgoing interface/next-hop
  pLink= node_rt_lookup(pNode, tDstAddr);
  if (pLink == NULL)
    return -1;

  // Build message
  pMessage= message_create(pNode->tAddr, tDstAddr,
			   uProtocol, 255,
			   pPayLoad, fDestroy);

  // Forward
  return network_forward(pNode->pNetwork, pLink, pMessage);
}

// ----- node_register_protocol -------------------------------------
/**
 *
 */
int node_register_protocol(SNetNode * pNode, uint8_t uNumber,
			   void * pHandler,
			   FNetNodeHandlerDestroy fDestroy,
			   FNetNodeHandleEvent fHandleEvent)
{
  return protocols_register(pNode->pProtocols, uNumber, pHandler,
			    fDestroy, fHandleEvent);
}

// ----- network_nodes_destroy --------------------------------------
void network_nodes_destroy(void ** ppItem)
{
  node_destroy((SNetNode **) ppItem);
}

// ----- network_domains_compare ------------------------------------
/**
 *
 *
 */
int network_domains_compare(void * pItem1, void * pItem2, 
				unsigned int uEltSize)
{
  SNetDomain * pDomain1 = *((SNetDomain **)pItem1);
  SNetDomain * pDomain2 = *((SNetDomain **)pItem2);

  LOG_DEBUG("%x %x\n", pDomain1, pDomain2);
  if (pDomain1->uAS < pDomain2->uAS)
    return -1;
  else if (pDomain1->uAS > pDomain2->uAS)
    return 1;
  else
    return 0;
}

// ----- network_domains_destroy ------------------------------------
/**
 *
 *
 */
void network_domains_destroy(void * pItem)
{
  if (*(SNetNode **)pItem != NULL)
    domain_destroy((SNetDomain **)pItem);
}

// ----- network_create ---------------------------------------------
/**
 *
 */
SNetwork * network_create()
{
  SNetwork * pNetwork= (SNetwork *) MALLOC(sizeof(SNetwork));
  
  pNetwork->pNodes= radix_tree_create(32, network_nodes_destroy);
  pNetwork->pDomains = NULL;
  return pNetwork;
}

// ----- network_destroy --------------------------------------------
/**
 *
 */
void network_destroy(SNetwork ** ppNetwork)
{
  if (*ppNetwork != NULL) {
    radix_tree_destroy(&(*ppNetwork)->pNodes);
    ptr_array_destroy(&(*ppNetwork)->pDomains);
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
int network_add_node(SNetwork * pNetwork, SNetNode * pNode)
{
  pNode->pNetwork= pNetwork;
  return radix_tree_add(pNetwork->pNodes, pNode->tAddr, 32, pNode);
}

// ----- network_find_node ------------------------------------------
/**
 *
 */
SNetNode * network_find_node(SNetwork * pNetwork, net_addr_t tAddr)
{
  return (SNetNode *) radix_tree_get_exact(pNetwork->pNodes, tAddr, 32);
}

// ----- network_forward --------------------------------------------
/**
 * This function forwards a message through the given link. The
 * function first checks if the link is up.
 */
int network_forward(SNetwork * pNetwork, SNetLink * pLink,
		    SNetMessage * pMessage)
{
  SNetNode * pNextHop;

  // Check link's state
  if (!link_get_state(pLink, NET_LINK_FLAG_UP)) {
    LOG_INFO("Info: link is down, message dropped\n");
    message_destroy(&pMessage);
    return NET_ERROR_LINK_DOWN; // Link is down, silently drop
  }

  if (pLink->fForward == NULL) {
    // Node lookup: O(log(n))
    pNextHop= network_find_node(pNetwork, pLink->tAddr);
    assert(pNextHop != NULL);
    
    // Forward to node...
    assert(!simulator_post_event(network_send_callback,
				 network_send_context_destroy,
				 network_send_context_create(pNextHop,
							     pMessage),
				 0, RELATIVE_TIME));
    return NET_SUCCESS;
  } else {
    return pLink->fForward(pLink->tAddr, pLink->pContext, pMessage);
  }
}

// ----- network_nodes_to_file --------------------------------------
int network_nodes_to_file(uint32_t uKey, uint8_t uKeyLen,
			  void * pItem, void * pContext)
{
  SNetNode * pNode= (SNetNode *) pItem;
  FILE * pStream= (FILE *) pContext;
  SNetLink * pLink;
  int iLinkIndex;

  for (iLinkIndex= 0; iLinkIndex < ptr_array_length(pNode->pLinks);
       iLinkIndex++) {
	  pLink= (SNetLink *) pNode->pLinks->data[iLinkIndex];
	  fprintf(pStream, "%d\t%d\t%d\n",
		  pNode->tAddr, pLink->tAddr, pLink->tDelay);
  }
  return 0;
}

// ----- network_to_file --------------------------------------------
/**
 *
 */
int network_to_file(FILE * pStream, SNetwork * pNetwork)
{
  return radix_tree_for_each(pNetwork->pNodes, network_nodes_to_file,
			     pStream);
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

// ----- network_shortest_path_destroy ------------------------------
void network_shortest_path_destroy(void ** pItem)
{
  net_path_destroy((SNetPath **) pItem);
}

// ----- network_shortest_for_each ----------------------------------
int network_shortest_for_each(uint32_t uKey, uint8_t uKeyLen,
			      void * pItem, void * pContext)
{
  SNetPath * pPath= (SNetPath *) pItem;
  FILE * pStream= (FILE *) pContext;
  
  fprintf(pStream, "--> %u\t[", uKey);
  net_path_dump(pStream, pPath);
  fprintf(pStream, "]\n");
  return 0;
}
  
// ----- network_shortest_path --------------------------------------
/**
 * Calculate shortest path from the given source AS towards all the
 * other ASes.
 */
int network_shortest_path(SNetwork * pNetwork, FILE * pStream,
			  net_addr_t tSrcAddr)
{
  SNetLink * pLink;
  SNetNode * pNode;
  SFIFO * pFIFO;
  SRadixTree * pVisited;
  SContext * pContext, * pOldContext;
  int iIndex;

  pVisited= radix_tree_create(32, network_shortest_path_destroy);
  
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
    pNode= network_find_node(pNetwork, pContext->tAddr);

    pOldContext= pContext;

    for (iIndex= 0; iIndex < ptr_array_length(pNode->pLinks);
	 iIndex++) {
      pLink= (SNetLink *) pNode->pLinks->data[iIndex];
      if (radix_tree_get_exact(pVisited, pLink->tAddr, 32) == NULL) {
	pContext= (SContext *) MALLOC(sizeof(SContext));
	pContext->tAddr= pLink->tAddr;
	pContext->pPath= net_path_copy(pOldContext->pPath);
	net_path_append(pContext->pPath, pLink->tAddr);
	radix_tree_add(pVisited, pLink->tAddr, 32,
		       net_path_copy(pContext->pPath));
	assert(fifo_push(pFIFO, pContext) == 0);
      }
    }
    net_path_destroy(&pOldContext->pPath);
    FREE(pOldContext);
  }
  fifo_destroy(&pFIFO);

  radix_tree_for_each(pVisited, network_shortest_for_each, pStream);

  radix_tree_destroy(&pVisited);
  return 0;
}

/////////////////////////////////////////////////////////////////////
// ROUTING TESTS
/////////////////////////////////////////////////////////////////////

#define NET_RECORD_ROUTE_SUCCESS         0
#define NET_RECORD_ROUTE_TOO_LONG       -1
#define NET_RECORD_ROUTE_UNREACH        -2
#define NET_RECORD_ROUTE_TUNNEL_UNREACH -3
#define NET_RECORD_ROUTE_TUNNEL_BROKEN  -4

// ----- node_record_route ------------------------------------------
/**
 *
 */
int node_record_route(SNetNode * pNode, net_addr_t tDstAddr,
		      SNetPath ** ppPath,
		      net_link_delay_t * pDelay, uint32_t * pWeight)
{
  SNetNode * pCurrentNode= pNode;
  SNetLink * pLink= NULL;
  unsigned int uHopCount= 0;
  int iResult= NET_RECORD_ROUTE_UNREACH;
  SNetPath * pPath= net_path_create();
  SStack * pDstStack= stack_create(10);
  net_link_delay_t tTotalDelay= 0;
  uint32_t uTotalWeight= 0;
  net_link_delay_t tLinkDelay= 0;
  uint32_t uLinkWeight= 0;

  while (pCurrentNode != NULL) {

    net_path_append(pPath, pCurrentNode->tAddr);

    tTotalDelay+= tLinkDelay;
    uTotalWeight+= uLinkWeight;

    // Final destination reached ?
    if (pCurrentNode->tAddr == tDstAddr) {

      // Tunnel end-point ?
      if (stack_depth(pDstStack) > 0) {

	// Does the end-point support IP-in-IP protocol ?
	if (protocols_get(pCurrentNode->pProtocols,
			  NET_PROTOCOL_IPIP) == NULL) {
	  iResult= NET_RECORD_ROUTE_TUNNEL_UNREACH;
	  break;
	}
	
	// Pop destination, but current node does not change
	tDstAddr= (net_addr_t) stack_pop(pDstStack);

      } else {

	// Destination reached :-)
	iResult= NET_RECORD_ROUTE_SUCCESS;
	break;

      }

    } else {

      // Lookup the next-hop for this destination
      pLink= node_rt_lookup(pCurrentNode, tDstAddr);
      if (pLink == NULL)
	break;

      tLinkDelay= pLink->tDelay;
      uLinkWeight= pLink->uIGPweight;

      // Handle tunnel encapsulation
      if (link_get_state(pLink, NET_LINK_FLAG_TUNNEL)) {
	// Push destination to stack
	stack_push(pDstStack, (void *) tDstAddr);
	
	tDstAddr= pLink->tAddr;

	// Find new destination (i.e. the tunnel end-point address)
	pLink= node_rt_lookup(pCurrentNode, tDstAddr);
	if (pLink == NULL) {
	  iResult= NET_RECORD_ROUTE_TUNNEL_BROKEN;
	  break;
	}

      }

      pCurrentNode= network_find_node(pNode->pNetwork, pLink->tAddr);
      
    }

    uHopCount++;

    if (uHopCount > NET_OPTIONS_MAX_HOPS) {
      iResult= NET_RECORD_ROUTE_TOO_LONG;
      break;
    }

  }

  *ppPath= pPath;

  stack_destroy(&pDstStack);

  /* Returns the total route delay */
  if (pDelay != NULL) {
    *pDelay= tTotalDelay;
  }

  /* Returns the total route weight */
  if (pWeight != NULL) {
    *pWeight= uTotalWeight;
  }

  return iResult;
}

// ----- node_dump_recorded_route -----------------------------------
/**
 *
 */
void node_dump_recorded_route(FILE * pStream, SNetNode * pNode,
			      net_addr_t tDstAddr, int iDelay)
{
  int iResult;
  SNetPath * pPath;
  net_link_delay_t tDelay= 0;
  uint32_t uWeight= 0;

  iResult= node_record_route(pNode, tDstAddr, &pPath,
			     &tDelay, &uWeight);

  ip_address_dump(pStream, pNode->tAddr);
  fprintf(pStream, "\t");
  ip_address_dump(pStream, tDstAddr);
  fprintf(pStream, "\t");
  switch (iResult) {
  case NET_RECORD_ROUTE_SUCCESS: fprintf(pStream, "SUCCESS"); break;
  case NET_RECORD_ROUTE_TOO_LONG: fprintf(pStream, "TOO_LONG"); break;
  case NET_RECORD_ROUTE_UNREACH: fprintf(pStream, "UNREACH"); break;
  case NET_RECORD_ROUTE_TUNNEL_UNREACH:
    fprintf(pStream, "TUNNEL_UNREACH"); break;
  case NET_RECORD_ROUTE_TUNNEL_BROKEN:
    fprintf(pStream, "TUNNEL_BROKEN"); break;
  default:
    fprintf(pStream, "UNKNOWN_ERROR");
  }
  fprintf(pStream, "\t");
  net_path_dump(pStream, pPath);
  if (iDelay) {
    fprintf(pStream, "\t%u\t%u", tDelay, uWeight);
  }
  fprintf(pStream, "\n");

  net_path_destroy(&pPath);
}

/////////////////////////////////////////////////////////////////////
// TEST
/////////////////////////////////////////////////////////////////////

// ----- node_handle_event ------------------------------------------
int node_handle_event(void * pHandler, SNetMessage * pMessage)
{
  SNetNode * pNode= (SNetNode *) pHandler;

  printf("node_event[%d]: from %u, %d\n",
	 pNode->tAddr,
	 pMessage->tSrcAddr,
	 (int) pMessage->pPayLoad);
  return 0;
}

// ----- test_network -----------------------------------------------
void test_network()
{
  SNetwork * pNetwork= network_create();
  SNetNode * pNodeA0= node_create(0);
  SNetNode * pNodeA1= node_create(1);
  SNetNode * pNodeB0= node_create(65536);
  SNetNode * pNodeB1= node_create(65537);
  SNetNode * pNodeC0= node_create(131072);
  FILE * pNetFile;

  node_register_protocol(pNodeA0, NET_PROTOCOL_ICMP, pNodeA0, NULL,
			 node_handle_event);
  node_register_protocol(pNodeA1, NET_PROTOCOL_ICMP, pNodeA1, NULL,
			 node_handle_event);
  node_register_protocol(pNodeB0, NET_PROTOCOL_ICMP, pNodeB0, NULL,
			 node_handle_event);
  node_register_protocol(pNodeB1, NET_PROTOCOL_ICMP, pNodeB1, NULL,
			 node_handle_event);

  simulator_init();

  log_set_level(pMainLog, LOG_LEVEL_EVERYTHING);
  log_set_stream(pMainLog, stderr);

  assert(!network_add_node(pNetwork, pNodeA0));
  assert(!network_add_node(pNetwork, pNodeA1));
  assert(!network_add_node(pNetwork, pNodeB0));
  assert(!network_add_node(pNetwork, pNodeB1));
  assert(!network_add_node(pNetwork, pNodeC0));

  LOG_DEBUG("nodes attached.\n");

  assert(!node_add_link(pNodeA0, pNodeA1, 100, 1));
  assert(!node_add_link(pNodeA1, pNodeB0, 1000, 1));
  assert(!node_add_link(pNodeB0, pNodeB1, 100, 1));
  assert(!node_add_link(pNodeA1, pNodeC0, 400, 1));
  assert(!node_add_link(pNodeC0, pNodeB0, 400, 1));

  LOG_DEBUG("links attached.\n");

  pNetFile= fopen("net-file", "w");
  assert(pNetFile != NULL);
  assert(!network_to_file(pNetFile, pNetwork));
  fclose(pNetFile);

  LOG_DEBUG("network to file.\n");

  assert(!node_send(pNodeA0, 1, 0, (void *) 5, NULL));
  simulator_run();

  LOG_DEBUG("message sent.\n");

  network_shortest_path(pNetwork, stderr, pNodeA0->tAddr);

  LOG_DEBUG("shortest-path computed.\n");

  //network_dijkstra(pNetwork, stderr, pNodeA0->tAddr);

  LOG_DEBUG("dijkstra computed.\n");

  network_destroy(&pNetwork);

  LOG_DEBUG("network done.\n");

  /*
  pNetFile= fopen("net-file", "r");
  assert(pNetFile != NULL);
  pNetwork= network_from_file(pNetFile);
  assert(pNetwork != NULL);
  fclose(pNetFile);

  LOG_DEBUG("network from file.\n");
  */

  simulator_done();

  exit(0);
}

/////////////////////////////////////////////////////////////////////
// INITIALIZATION AND FINALIZATION SECTION
/////////////////////////////////////////////////////////////////////

void _network_destroy() __attribute__((destructor));

// ----- _network_destroy -------------------------------------------
/**
 *
 */
void _network_destroy()
{
  if (pTheNetwork != NULL) {
    //fprintf(stderr, "free network: ");
    //fflush(stderr);
    network_destroy(&pTheNetwork);
    //fprintf(stderr, "done.\n");
  }
}

// ----- network_domain_add ------------------------------------------
/**
 *
 *
 */
int network_domain_add(SNetwork * pNetwork, uint32_t uAS, 
						      char * pcName)
{
  SNetDomain * pDomain = domain_create(uAS, pcName);
  if (pNetwork->pDomains == NULL)
    pNetwork->pDomains = ptr_array_create(ARRAY_OPTION_SORTED|ARRAY_OPTION_SORTED, 
					network_domains_compare, 
					network_domains_destroy);

  return ptr_array_add(pNetwork->pDomains, &pDomain);
}

// ----- network_domain_get ------------------------------------------
/**
 *
 *
 */
SNetDomain * network_domain_get (SNetwork * pNetwork, uint32_t uAS)
{
  unsigned int uIndex;
  SNetDomain * pDomain = domain_create(uAS, "");

  if (ptr_array_sorted_find_index(pNetwork->pDomains, &pDomain, &uIndex)) {
    LOG_DEBUG("network_domain_get>domain not found.\n");
    domain_destroy(&pDomain);
    return NULL;
  }
  domain_destroy(&pDomain);

  return (SNetDomain *)pNetwork->pDomains->data[uIndex];
}
