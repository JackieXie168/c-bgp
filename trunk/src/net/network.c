// ==================================================================
// @(#)network.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 4/07/2003
// @lastdate 19/10/2007
// ==================================================================

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <string.h>

#include <libgds/fifo.h>
#include <libgds/log.h>
#include <libgds/memory.h>
#include <libgds/patricia-tree.h>
#include <libgds/stack.h>
#include <libgds/str_util.h>

#include <net/error.h>
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
#include <bgp/message.h>
#include <bgp/as_t.h>
#include <bgp/rib.h>
#include <ui/output.h>

SNetwork * pTheNetwork= NULL;

// ----- options -----
uint8_t NET_OPTIONS_MAX_HOPS= 30;
uint8_t NET_OPTIONS_IGP_INTER= 0;

// ----- Forward declarations ---------------------------------------
static SNetSendContext * _network_send_context_create(SNetNode * pNode,
						      SNetMessage * pMessage);
static void _network_send_context_destroy(void * pContext);
static int _network_send_callback(SSimulator * pSimulator,
				  void * pContext);
static void _network_send_dump(SLogStream * pStream, void * pContext);
static int _network_forward(SNetwork * pNetwork, SNetLink * pLink,
			    net_addr_t tNextHop, SNetMessage * pMessage,
			    SSimulator * pSimulator);

// -----[ network_get_simulator ]------------------------------------
SSimulator * network_get_simulator()
{
  if (pTheNetwork->pSimulator == NULL)
    pTheNetwork->pSimulator= simulator_create(SCHEDULER_STATIC);
  return pTheNetwork->pSimulator;
}

// ----- node_add_tunnel --------------------------------------------
/**
 * Add a tunnel interface on this node.
 *
 * Arguments:
 *   node
 *   tunnel remote end-point
 *   tunnel identifier (local interface address)
 *   outgoing interface (optional)
 *   source address (optional)
 */
int node_add_tunnel(SNetNode * pNode,
		    net_addr_t tDstPoint,
		    net_addr_t tAddr,
		    SNetDest * pOutIfaceDest,
		    net_addr_t tSrcAddr)
{
  SNetLink * pOutIface= NULL;
  SNetLink * pLink;
  int iResult;

  // If an outgoing interface is specified, check that it exists
  if (pOutIfaceDest != NULL) {
    pOutIface= node_find_iface(pNode, *pOutIfaceDest);
    if (pOutIface == NULL)
      return NET_ERROR_IF_UNKNOWN;
  }

  iResult= ipip_link_create(pNode, tDstPoint, tAddr,
			    pOutIface, tSrcAddr,
			    &pLink);
  if (iResult != NET_SUCCESS)
    return iResult;

  if (net_links_add(pNode->pLinks, pLink) < 0) {
    net_link_destroy(&pLink);
    return NET_ERROR_MGMT_LINK_ALREADY_EXISTS;
  }

  return NET_SUCCESS;
}

// -----[ node_del_tunnel ]------------------------------------------
/**
 *
 */
int node_del_tunnel(SNetNode * pNode, net_addr_t tAddr)
{
  return NET_ERROR_UNSUPPORTED;
}

// -----[ node_ipip_enable ]-----------------------------------------
/**
 *
 */
int node_ipip_enable(SNetNode * pNode)
{
  return node_register_protocol(pNode, NET_PROTOCOL_IPIP,
				pNode, NULL,
				ipip_event_handler);
}

// ----- node_igp_domain_add -------------------------------------------
int node_igp_domain_add(SNetNode * pNode, uint16_t uDomainNumber)
{
  return _array_add((SArray*)(pNode->pIGPDomains), &uDomainNumber);
}

// ----- node_belongs_to_igp_domain ---------------------------------
/**
 * Test if a node belongs to a given IGP domain.
 *
 * Return value:
 *   TRUE (1) if node belongs to the given IGP domain
 *   FALSE (0) otherwise.
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
int node_links_for_each(SNetNode * pNode, FArrayForEach fForEach,
			void * pContext)
{
  return _array_for_each((SArray *) pNode->pLinks, fForEach, pContext);
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

// -----[ _node_error_dump ]-----------------------------------------
static void _node_error_dump(int iErrorCode)
{
  /*LOG_ERR_ENABLED(LOG_LEVEL_SEVERE) {
    log_printf(pLogErr, "@");
    ip_address_dump(pLogErr, pNode->tAddr);
    log_printf(pLogErr, ": ");
    network_perror(pLogErr, iErrorCode);
    log_printf(pLogErr, " (");
    message_dump(pLogErr, pMessage);
    log_printf(pLogErr, ")\n");
    }*/
}

// -----[ _node_fwd_error ]------------------------------------------
static int _node_fwd_error(SNetNode * pNode,
			   SNetMessage * pMessage,
			   SSimulator * pSimulator,
			   int iErrorCode, uint8_t uICMPError)
{
  _node_error_dump(iErrorCode);
  if ((uICMPError != 0) && !is_icmp_error(pMessage))
    icmp_send_error(pNode, NET_ADDR_ANY, pMessage->tSrcAddr,
		    uICMPError, pSimulator);
  message_destroy(&pMessage);
  return iErrorCode;
}

// -----[ _node_process_msg ]----------------------------------------
int _node_process_msg(SNetNode * pNode, SNetMessage * pMessage,
		      SSimulator * pSimulator)
{
  SNetProtocol * pProtocol;
  int iResult;

  assert(pNode->pProtocols != NULL);
  pProtocol= protocols_get(pNode->pProtocols, pMessage->uProtocol);
  if (pProtocol == NULL)
    return _node_fwd_error(pNode, pMessage, pSimulator,
			   NET_ERROR_PROTO_UNREACH,
			   ICMP_ERROR_PROTO_UNREACH);
    
  iResult= pProtocol->fHandleEvent(pSimulator, 
				   pProtocol->pHandler,
				   pMessage);
    
  // Free only the message. Dealing with the payload is now the
  // responsability of the protocol handler.
  pMessage->pPayLoad= NULL;
  message_destroy(&pMessage);
  return iResult;
}

// -----[ node_recv_msg ]--------------------------------------------
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
int node_recv_msg(SNetNode * pNode, SNetMessage * pMessage,
		  SSimulator * pSimulator)
{
  SNetRouteNextHop * pNextHop;
  int iResult;

  assert(pMessage->uTTL > 0);

  // Local delivery ?
  if (node_has_address(pNode, pMessage->tDstAddr))
    return _node_process_msg(pNode, pMessage, pSimulator);

  // Time-To-Live expired ?
  pMessage->uTTL--;
  if (pMessage->uTTL == 0)
    return _node_fwd_error(pNode, pMessage, pSimulator,
			   NET_ERROR_TIME_EXCEEDED,
			   ICMP_ERROR_TIME_EXCEEDED);

  // Find route to destination
  pNextHop= node_rt_lookup(pNode, pMessage->tDstAddr);
  if (pNextHop == NULL)
    return _node_fwd_error(pNode, pMessage, pSimulator,
			   NET_ERROR_NET_UNREACH,
			   ICMP_ERROR_NET_UNREACH);

  // In the future, we should check that the outgoing interface is
  // different from the incoming interface
  // ...

  // Forward...
  iResult= _network_forward(pNode->pNetwork,
			    pNextHop->pIface,
			    pNextHop->tGateway,
			    pMessage,
			    pSimulator);
  if (iResult == NET_ERROR_HOST_UNREACH) {
    return _node_fwd_error(pNode, pMessage, pSimulator,
			   NET_ERROR_HOST_UNREACH,
			   ICMP_ERROR_HOST_UNREACH);
  }

  return iResult;
}

// -----[ node_send ]------------------------------------------------
/**
 * Send a message to a node.
 * The node must be directly connected to the sender node or there
 * must be a route towards the node in the sender's routing table.
 */
int node_send_msg(SNetNode * pNode, net_addr_t tSrcAddr,
		  net_addr_t tDstAddr, uint8_t uProtocol, uint8_t uTTL,
		  void * pPayLoad, FPayLoadDestroy fDestroy,
		  SSimulator * pSimulator)
{
  SNetRouteNextHop * pNextHop;
  SNetMessage * pMessage;

  if (uTTL == 0)
    uTTL= 255;

  // Build "IP" message
  pMessage= message_create(tSrcAddr, tDstAddr,
			   uProtocol, uTTL,
			   pPayLoad, fDestroy);

  // Local delivery ?
  if (node_has_address(pNode, pMessage->tDstAddr)) {
    // Set source address as the node's loopback address (if unspecified)
    if (pMessage->tSrcAddr == NET_ADDR_ANY)
      pMessage->tSrcAddr= pNode->tAddr;
    return _node_process_msg(pNode, pMessage, pSimulator);
  }
  
  // Find outgoing interface & next-hop
  pNextHop= node_rt_lookup(pNode, tDstAddr);
  if (pNextHop == NULL)
    return _node_fwd_error(pNode, pMessage, pSimulator,
			   NET_ERROR_NET_UNREACH, 0);

  if (pMessage->tSrcAddr == 0) {
    if (pNextHop->pIface->tIfaceAddr != 0)
      pMessage->tSrcAddr= pNextHop->pIface->tIfaceAddr;
    else
      pMessage->tSrcAddr= pNode->tAddr;
  }

  // Forward
  return _network_forward(pNode->pNetwork,
			  pNextHop->pIface,
			  pNextHop->tGateway,
			  pMessage,
			  pSimulator);
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

// ----- network_create ---------------------------------------------
/**
 *
 */
SNetwork * network_create()
{
  SNetwork * pNetwork= (SNetwork *) MALLOC(sizeof(SNetwork));
  
  pNetwork->pNodes= trie_create(network_nodes_destroy);
  pNetwork->pSubnets= subnets_create();
  pNetwork->pSimulator= NULL;
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
    subnets_destroy(&(*ppNetwork)->pSubnets);
    if ((*ppNetwork)->pSimulator != NULL)
      simulator_destroy(&(*ppNetwork)->pSimulator);
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
  assert(pTheNetwork != NULL);
  return pTheNetwork;
}

// ----- network_add_node -------------------------------------------
/**
 *
 */
int network_add_node(SNetNode * pNode)
{
  // Check that node does not already exist
  if (network_find_node(pNode->tAddr) != NULL)
    return NET_ERROR_MGMT_NODE_ALREADY_EXISTS;

  pNode->pNetwork= pTheNetwork;
  return trie_insert(pTheNetwork->pNodes, pNode->tAddr, 32, pNode);
}

// ----- network_add_subnet -------------------------------------------
/**
 *
 */
int network_add_subnet(SNetSubnet * pSubnet)
{
  // Check that subnet does not already exist
  if (network_find_subnet(pSubnet->sPrefix) != NULL)
    return NET_ERROR_MGMT_SUBNET_ALREADY_EXISTS;

  return subnets_add(pTheNetwork->pSubnets, pSubnet);
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
  return subnets_find(pTheNetwork->pSubnets, sPrefix);
}

// ----- _network_drop ----------------------------------------------
/**
 * Drop a message with an error message on STDERR.
 */
static void _network_drop(SNetMessage * pMsg)
{
  /*LOG_ERR_ENABLED(LOG_LEVEL_SEVERE) {
    log_printf(pLogErr, "*** \033[31;1mMESSAGE DROPPED\033[0m ***\n");
    log_printf(pLogErr, "message: ");
    message_dump(pLogErr, pMsg);
    log_printf(pLogErr, "\n");
    }*/
  message_destroy(&pMsg);
}

// ----- _network_forward -------------------------------------------
/**
 * This function forwards a message through the given link.
 */
static int _network_forward(SNetwork * pNetwork, SNetLink * pLink,
			    net_addr_t tNextHop, SNetMessage * pMsg,
			    SSimulator * pSimulator)
{
  SNetNode * pNextHop= NULL;
  int iResult;

  if (tNextHop == 0)
    tNextHop= pMsg->tDstAddr;

  // Forward along this link...
  assert(pLink->fForward != NULL);
  iResult= pLink->fForward(tNextHop, pLink->pContext, &pNextHop, &pMsg);

  // If destination is unreachable
  if (iResult == NET_ERROR_HOST_UNREACH)
    return iResult;

  // If one link was DOWN, drop the message.
  if (iResult == NET_ERROR_LINK_DOWN) {
    _network_drop(pMsg);
    return iResult;
  }

  // Forward to node...
  if (iResult == NET_SUCCESS) {
    assert(pNextHop != NULL);

    if (pSimulator == NULL)
      pSimulator= network_get_simulator();

    assert(!simulator_post_event(pSimulator,
				 _network_send_callback,
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
    log_printf(pStream, "\n");
  }
  enum_destroy(&pEnum);
  return 0;
}

typedef struct {
  net_addr_t tAddr;
  SNetPath * pPath;
  net_link_delay_t tDelay;
} SContext;

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

// ----- network_links_clear ----------------------------------------
/**
 * Clear the load of all links in the topology.
 */
void network_links_clear()
{
  SEnumerator * pEnum= NULL;
  SNetNode * pNode;
  
  pEnum= trie_get_enum(pTheNetwork->pNodes);
  while (enum_has_next(pEnum)) {
    pNode= *((SNetNode **) enum_get_next(pEnum));
    node_links_clear(pNode);
  }
  enum_destroy(&pEnum);
}

// ----- network_links_save -----------------------------------------
/**
 * Save the load of all links in the topology.
 */
int network_links_save(SLogStream * pStream)
{
  SEnumerator * pEnum= NULL;
  SNetNode * pNode;

  pEnum= trie_get_enum(pTheNetwork->pNodes);
  while (enum_has_next(pEnum)) {
    pNode= *((SNetNode **) enum_get_next(pEnum));
    node_links_save(pStream, pNode);
  }
  enum_destroy(&pEnum);

  return 0;
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
static int _network_send_callback(SSimulator * pSimulator,
				  void * pContext)
{
  SNetSendContext * pSendContext= (SNetSendContext *) pContext;
  int iResult;

  // Deliver message to next-hop which will use the message localy or
  // forward it...
  iResult= node_recv_msg(pSendContext->pNode, pSendContext->pMessage,
			 pSimulator);

  // Free the message context. The message MUST be freed by
  // node_recv_msg() if the message has been delivered or in case
  // the message can not be forwarded.
  FREE(pContext);
  return iResult;
}

// ----- _network_send_dump -----------------------------------------
/**
 * Callback function used to dump the content of a message event. See
 * also 'simulator_dump_events' (sim/simulator.c).
 */
static void _network_send_dump(SLogStream * pStream, void * pContext)
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
static SNetSendContext * _network_send_context_create(SNetNode * pNode,
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
static void _network_send_context_destroy(void * pContext)
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
  if (pTheNetwork != NULL)
    network_destroy(&pTheNetwork);
}


/////////////////////////////////////////////////////////////////////
//
// TESTS
//
/////////////////////////////////////////////////////////////////////

// -----[ _test_node_handle_event ]----------------------------------
static int _test_node_handle_event(SSimulator * pSimulator,
				   void * pHandler,
				   SNetMessage * pMessage)
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
			 _test_node_handle_event);
  node_register_protocol(pNodeA1, NET_PROTOCOL_ICMP, pNodeA1, NULL,
			 _test_node_handle_event);
  node_register_protocol(pNodeB0, NET_PROTOCOL_ICMP, pNodeB0, NULL,
			 _test_node_handle_event);
  node_register_protocol(pNodeB1, NET_PROTOCOL_ICMP, pNodeB1, NULL,
			 _test_node_handle_event);

  assert(!network_add_node(pNodeA0));
  assert(!network_add_node(pNodeA1));
  assert(!network_add_node(pNodeB0));
  assert(!network_add_node(pNodeB1));
  assert(!network_add_node(pNodeC0));

  LOG_DEBUG(LOG_LEVEL_DEBUG, "nodes attached.\n");

  assert(node_add_link_ptp(pNodeA0, pNodeA1, 100, 0, 1, 1) >= 0);
  assert(node_add_link_ptp(pNodeA1, pNodeB0, 1000, 0, 1, 1) >= 0);
  assert(node_add_link_ptp(pNodeB0, pNodeB1, 100, 0, 1, 1) >= 0);
  assert(node_add_link_ptp(pNodeA1, pNodeC0, 400, 0, 1, 1) >= 0);
  assert(node_add_link_ptp(pNodeC0, pNodeB0, 400, 0, 1, 1) >= 0);

  LOG_DEBUG(LOG_LEVEL_DEBUG, "links attached.\n");

  pNetStream= log_create_file("net-file");
  assert(pNetStream != NULL);
  assert(!network_to_file(pNetStream, pNetwork));
  log_destroy(&pNetStream);

  LOG_DEBUG(LOG_LEVEL_DEBUG, "network to file.\n");

  assert(!node_send_msg(pNodeA0, 0, 1, 0, 255, (void *) 5, NULL, NULL));
  simulator_run(network_get_simulator());

  LOG_DEBUG(LOG_LEVEL_DEBUG, "message sent.\n");

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

  exit(EXIT_SUCCESS);
}

