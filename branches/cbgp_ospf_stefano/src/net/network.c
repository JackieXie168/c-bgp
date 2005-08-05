// ==================================================================
// @(#)network.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 4/07/2003
// @lastdate 07/04/2005
// ==================================================================

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <string.h>
#include <libgds/log.h>
#include <libgds/memory.h>
#include <libgds/stack.h>
#include <net/net_types.h>
#include <net/prefix.h>
#include <net/icmp.h>
#include <net/ipip.h>
#include <net/network.h>
#include <net/net_path.h>
#include <net/ospf.h>
#include <net/ospf_rt.h>
#include <net/subnet.h>
#include <net/domain.h>
#include <ui/output.h>
#include <bgp/message.h>
#include <libgds/fifo.h>
#include <bgp/as_t.h>
#include <bgp/rib.h>


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

// ----- network_send_dump ------------------------------------------
/**
 * Callback function used to dump the content of a message event. See
 * also 'simulator_dump_events' (sim/simulator.c).
 */
void network_send_dump(FILE * pStream, void * pContext)
{
  SNetSendContext * pSendContext= (SNetSendContext *) pContext;

  fprintf(pStream, "net-msg [");
  message_dump(pStream, pSendContext->pMessage);
  fprintf(pStream, "]");
  if (pSendContext->pMessage->uProtocol == NET_PROTOCOL_BGP) {
    bgp_msg_dump(pStream, NULL, pSendContext->pMessage->pPayLoad);
  }
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

// ----- node_compare -----------------------------------------
int node_compare(void * pItem1, void * pItem2,
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
  SNetLink * pLink= create_link_toRouter_byAddr(tToIf);//SHOULD BE A DIFFERENT TYPE OF LINK?
  SPtrArray * pLinks;
  unsigned int uIndex;

  pInterface->tAddr = tFromIf;

  if (ptr_array_sorted_find_index(pNode->aInterfaces, &pInterface, &uIndex)) {
    LOG_DEBUG("node_interface_link_add>interface not found %ul\n", tFromIf);
    return -1;
  }
  pLinks = ((SNetInterface *)pNode->aInterfaces->data[uIndex])->pLinks;

  //pLink->tAddr= tToIf;
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
  
  /*if I'm not care on link type*/
//   if (pLink1->uDestinationType  == NET_LINK_TYPE_ANY || 
//                        pLink2->uDestinationType  == NET_LINK_TYPE_ANY) {
//     fprintf(stdout, "un link è ANY confronto il prefisso: " );  
    SPrefix sPrefixL1, sPrefixL2, * pP1, * pP2;
    link_get_prefix(pLink1, &sPrefixL1);
    link_get_prefix(pLink2, &sPrefixL2);

    pP1 = &sPrefixL1;
    pP2 = &sPrefixL2;
    int pfxCmp = ip_prefixes_compare(&pP1, &pP2, 0);

    return pfxCmp;
//   } 

//   if ((pLink1->uDestinationType == pLink2->uDestinationType)){

//     if (pLink1->uDestinationType == NET_LINK_TYPE_ROUTER) {

//       if (link_get_address(pLink1) < link_get_address(pLink2))
//         return -1;
//       else if (link_get_address(pLink1) > link_get_address(pLink2))
//         return 1;
//       else
//         return 0;
//      }
//      else {

//        SPrefix sPrefixL1, sPrefixL2, * pP1, * pP2;

//        link_get_prefix(pLink1, &sPrefixL1);
//        link_get_prefix(pLink2, &sPrefixL2);
//        pP1 = &sPrefixL1;
//        pP2 = &sPrefixL2;
//        int pfxCmp = ip_prefixes_compare(&pP1, &pP2, 0);
// 
//        return pfxCmp;
//      }
//    }
//    else if (pLink1->uDestinationType == NET_LINK_TYPE_TRANSIT)
//      return 1;
//    else if (pLink2->uDestinationType == NET_LINK_TYPE_TRANSIT)
//      return -1;
//    else if (pLink1->uDestinationType == NET_LINK_TYPE_STUB)
//      return 1;
// //    else
//      return -1;  
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

// ----- node_get_prefix----------------------------------------------
void node_get_prefix(SNetNode * pNode, SPrefix * pPrefix){
  pPrefix->tNetwork = pNode->tAddr;
  pPrefix->uMaskLen = 32;
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
  
  pNode->pLinks = ptr_array_create(ARRAY_OPTION_SORTED|ARRAY_OPTION_UNIQUE,
				   node_links_compare,
				   node_links_destroy);
  pNode->pRT= rt_create();
#ifdef OSPF_SUPPORT
  pNode->pOSPFAreas  = uint32_array_create(ARRAY_OPTION_SORTED|ARRAY_OPTION_UNIQUE);
  pNode->pOspfRT     = OSPF_rt_create();
  pNode->pIGPDomains = uint16_array_create(ARRAY_OPTION_SORTED|ARRAY_OPTION_UNIQUE);
#endif
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
    ptr_array_destroy(&(*ppNode)->aInterfaces);
    
    //SUBNET MUST BE DESTROYED
//     int iIndex;
//     SNetLink * pLink;
//     for (iIndex = 0; iIndex < ptr_array_length((*ppNode)->pLinks); iIndex++){
//       ptr_array_get_at(&(*ppNode)->pLinks, iIndex, &pLink);
//       if (link_check_type(pLink, NET_LINK_TYPE_TRANSIT) || link_check_type(pLink, NET_LINK_TYPE_STUB))
//         subnet_destroy(&(link_get_subnet(pLink)));
//     }
    ptr_array_destroy(&(*ppNode)->pLinks);
    
#ifdef OSPF_SUPPORT
    _array_destroy((SArray **)(&(*ppNode)->pOSPFAreas));
    OSPF_rt_destroy(&(*ppNode)->pOspfRT);
    uint16_array_destroy(&(*ppNode)->pIGPDomains);
#endif
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
  SNetLink * pLink= create_link_toRouter(pNodeB);//(SNetLink *) MALLOC(sizeof(SNetLink));
  
  if (iRecurse)
    if (node_add_link(pNodeB, pNodeA, tDelay, 0) < 0)
      return -1;

  //pLink->tAddr= pNodeB->tAddr;
  pLink->tDelay= tDelay;
  pLink->uFlags= NET_LINK_FLAG_UP | NET_LINK_FLAG_IGP_ADV;//NET_LINK_FLAG_IGP_ADV OBSOLETE 
  pLink->uIGPweight= tDelay;
  pLink->pContext= NULL;
  pLink->fForward= NULL;
  return ptr_array_add(pNodeA->pLinks, &pLink);
}

// ----- node_add_link_toSubnet ----------------------------------------------
/**
 *
 */
int node_add_link_toSubnet(SNetNode * pNode, SNetSubnet * pSubnet, 
                                             net_link_delay_t tDelay, int iMutual)
{
  SNetLink * pLink= create_link_toSubnet(pSubnet);
  pLink->tDelay= tDelay;
  pLink->uFlags= NET_LINK_FLAG_UP | NET_LINK_FLAG_IGP_ADV; //NET_LINK_FLAG_IGP_ADV OBSOLETE 
  pLink->uIGPweight= tDelay;
  pLink->pContext= NULL;
  pLink->fForward= NULL;
  
  if (ptr_array_add(pNode->pLinks, &pLink) < 0){
    return -1;//TODO define error constant
  }
  else if (iMutual)
    return subnet_link_toNode(pSubnet, pNode);//add node to subnet's node list  
  else 
    return 0;
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
  SNetLink * pLink= create_link_toRouter_byAddr(tDstPoint);
  
  //pLink->tAddr= tDstPoint;
  pLink->tDelay= 0;
  pLink->uFlags= NET_LINK_FLAG_UP | NET_LINK_FLAG_TUNNEL;
  pLink->uIGPweight= 0;
  pLink->pContext= pNode;
  pLink->fForward= ipip_link_forward;
  return ptr_array_add(pNode->pLinks, &pLink);
}

// ----- node_find_link_to_router ---------------------------------------------
/**
 *
 */
SNetLink * node_find_link_to_router(SNetNode * pNode, net_addr_t tAddr)
{
  unsigned int iIndex;
  SNetLink * pLink= NULL, * wrapLink;
//   char ipS[20], ipD[20];
//   char * pIpS = &ipS[0], *pIpD = &ipD[0];
  wrapLink = create_link_toRouter_byAddr(tAddr);
  
//   ip_address_to_string(ipS, pNode->tAddr);
//   ip_address_to_string(ipD, tAddr);
// LOG_SEVERE("find_link_to_router %s ->%s \n", ipS, ipD);
  if (ptr_array_sorted_find_index(pNode->pLinks, &wrapLink, &iIndex) == 0)
    pLink= (SNetLink *) pNode->pLinks->data[iIndex];
  link_destroy(&wrapLink);
  return pLink;
}

// ----- node_find_link_to_subnet ---------------------------------------------
/**
 *
 */
SNetLink * node_find_link_to_subnet(SNetNode * pNode, SNetSubnet * pSubnet)
{
  unsigned int iIndex;
  SNetLink * pLink= NULL, * wrapLink;
  
  wrapLink = create_link_toSubnet(pSubnet);
    
  if (ptr_array_sorted_find_index(pNode->pLinks, &wrapLink, &iIndex) == 0)
    pLink= (SNetLink *) pNode->pLinks->data[iIndex];
  
  link_destroy(&wrapLink);
  return pLink;
}

// ----- node_find_link ---------------------------------------------
/**
 *
 */
SNetLink * node_find_link(SNetNode * pNode, SPrefix * pPrefix)
{

//   char ipS[20], ipD[20];
  
//    ip_address_to_string(ipS, pNode->tAddr);
//   ip_prefix_to_string(ipD, pPrefix);
// LOG_SEVERE("find_link %s ->%s \n", ipS, ipD);

  if (pPrefix->uMaskLen == 32)
    return node_find_link_to_router(pNode, pPrefix->tNetwork);
  else  {
    SNetSubnet * pWrapSubnet = subnet_create(pPrefix->tNetwork, pPrefix->uMaskLen, 0);
    return node_find_link_to_subnet(pNode, pWrapSubnet);
  }
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


// ----- node_igp_domain_add -------------------------------------------
extern int node_igp_domain_add(SNetNode * pNode, uint16_t uDomainNumber){
  return _array_add((SArray*)(pNode->pIGPDomains), &uDomainNumber);
}

// ----- node_igp_domain_add -------------------------------------------
/** Return TRUE (1) if node belongs to igp domain tDomainNumber
 * FALSE (0) otherwise.
 */
extern int node_belongs_to_igp_domain(SNetNode * pNode, uint16_t uDomainNumber){
  unisgned int iIndex;
  if  (_array_sorted_find_index((SArray*)(pNode->pIGPDomains), &uDomainNumber, &iIndex) == 0)
    return 1;
  return 0;
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
  pRouteInfo= route_info_create(sPrefix, pLink, uWeight, uType);

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
void node_rt_dump(FILE * pStream, SNetNode * pNode, SNetDest sDest)
{
  if (pNode->pRT != NULL)
    rt_dump(pStream, pNode->pRT, sDest);

  flushir(pStream);
}

// ----- node_rt_dump_string -----------------------------------------------
/**
 *
 */
/*char * node_rt_dump_string(SNetNode * pNode, SPrefix sPrefix)
{
  char * cRT = NULL;

  if (pNode->pRT != NULL)
    cRT = rt_dump_string(pNode->pRT, sPrefix);

  return cRT;
}*/


// ----- node_rt_lookup ---------------------------------------------
/**
 * This function looks for the next-hop that must be used to reach a
 * destination address. The function looks up the static/IGP/BGP
 * routing table.
 */
SNetLink * node_rt_lookup(SNetNode * pNode, net_addr_t tDstAddr)
{
  SNetLink * pLink= NULL;
  SNetRouteInfo * pRouteInfo;

  // Is there a static or IGP route ?
  if (pNode->pRT != NULL) {
    pRouteInfo= rt_find_best(pNode->pRT, tDstAddr, NET_ROUTE_ANY);
    if (pRouteInfo != NULL)
      pLink= pRouteInfo->pNextHopIf;
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
  if (pLink == NULL) {
    if (fDestroy != NULL)
      fDestroy(&pPayLoad);
    return -1;
  }

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



/////////////////////////////////////////////////////////////////////
/////// Network methods
/////////////////////////////////////////////////////////////////////

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

// ----- network_subnets_destroy -----------------------------------------
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
  
#ifdef __EXPERIMENTAL__
  pNetwork->pNodes= trie_create(network_nodes_destroy);
#else
  pNetwork->pNodes= radix_tree_create(32, network_nodes_destroy);
#endif
  pNetwork->pDomains = NULL;
  pNetwork->pSubnets = ptr_array_create(ARRAY_OPTION_SORTED | ARRAY_OPTION_UNIQUE,
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
#ifdef __EXPERIMENTAL__
    trie_destroy(&(*ppNetwork)->pNodes);
#else
    radix_tree_destroy(&(*ppNetwork)->pNodes);
#endif
    ptr_array_destroy(&(*ppNetwork)->pDomains);
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
int network_add_node(SNetwork * pNetwork, SNetNode * pNode)
{
  pNode->pNetwork= pNetwork;
#ifdef __EXPERIMENTAL__
  return trie_insert(pNetwork->pNodes, pNode->tAddr, 32, pNode);
#else
  return radix_tree_add(pNetwork->pNodes, pNode->tAddr, 32, pNode);
#endif
}

// ----- network_add_subnet -------------------------------------------
/**
 *
 */
int network_add_subnet(SNetwork * pNetwork, SNetSubnet * pSubnet)
{
//   pNode->pNetwork = pSubnet; //NO FOR NOW
  return ptr_array_add(pNetwork->pSubnets, &pSubnet);
}

// ----- network_find_node ------------------------------------------
/**
 *
 */
SNetNode * network_find_node(SNetwork * pNetwork, net_addr_t tAddr)
{
#ifdef __EXPERIMENTAL__
  return (SNetNode *) trie_find_exact(pNetwork->pNodes, tAddr, 32);
# else
  return (SNetNode *) radix_tree_get_exact(pNetwork->pNodes, tAddr, 32);
#endif
}

// ----- network_find_node ------------------------------------------
/**
 *
 */
SNetSubnet * network_find_subnet(SNetwork * pNetwork, SPrefix sPrefix)
{ 
  unsigned int iIndex;
  SNetSubnet * pSubnet = NULL, * pWrapSubnet = subnet_create(sPrefix.tNetwork, sPrefix.uMaskLen, 0);
  if (ptr_array_sorted_find_index(pNetwork->pSubnets, &pWrapSubnet, &iIndex) == 0) 
    ptr_array_get_at(pNetwork->pSubnets, iIndex, &pSubnet);
  
  subnet_destroy(&pWrapSubnet);
  return pSubnet;
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
    fprintf(stderr, "*** Message lost ***\n");
    fprintf(stderr, "message: ");
    message_dump(stderr, pMessage);
    if (pMessage->uProtocol == NET_PROTOCOL_BGP)
      bgp_msg_dump(stderr, NULL, pMessage->pPayLoad);
    fprintf(stderr, "\n");
    fprintf(stderr, "link: ");
    link_dump(stderr, pLink);
    fprintf(stderr, "\n");
    LOG_INFO("Info: link is down, message dropped\n");
    message_destroy(&pMessage);
    return NET_ERROR_LINK_DOWN; // Link is down, silently drop
  }

  if (pLink->fForward == NULL) {
    // Node lookup: O(log(n))
    pNextHop= network_find_node(pNetwork, link_get_address(pLink));
    assert(pNextHop != NULL);
    
    // Forward to node...
    assert(!simulator_post_event(network_send_callback,
				 network_send_dump,
				 network_send_context_destroy,
				 network_send_context_create(pNextHop,
							     pMessage),
				 0, RELATIVE_TIME));
    return NET_SUCCESS;
  } else {
    return pLink->fForward(link_get_address(pLink), pLink->pContext, pMessage);
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
	  fprintf(pStream, "%d\t%d\t%u\n",
		  pNode->tAddr, link_get_address(pLink), pLink->tDelay);
  }
  return 0;
}

// ----- network_to_file --------------------------------------------
/**
 *
 */
int network_to_file(FILE * pStream, SNetwork * pNetwork)
{
#ifdef __EXPERIMENTAL__
  return trie_for_each(pNetwork->pNodes, network_nodes_to_file,
		       pStream);
#else
  return radix_tree_for_each(pNetwork->pNodes, network_nodes_to_file,
			     pStream);
#endif
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

  radix_tree_for_each(pVisited, network_shortest_for_each, pStream);

  radix_tree_destroy(&pVisited);
  return 0;
}

//---- network_dump_subnets ---------------------------------------------
void network_dump_subnets(FILE * pStream, SNetwork *pNetwork){
  int iIndex, totSub;
  SNetSubnet * pSubnet = NULL;
  totSub = ptr_array_length(pNetwork->pSubnets);
  for (iIndex = 0; iIndex < totSub; iIndex++){
    ptr_array_get_at(pNetwork->pSubnets, iIndex, &pSubnet);
    ip_prefix_dump(pStream, pSubnet->sPrefix);
    fprintf(pStream, "\n");
  }
}
/////////////////////////////////////////////////////////////////////
// ROUTING TESTS
/////////////////////////////////////////////////////////////////////

#define NET_RECORD_ROUTE_SUCCESS         0
#define NET_RECORD_ROUTE_TOO_LONG       -1
#define NET_RECORD_ROUTE_UNREACH        -2
#define NET_RECORD_ROUTE_DOWN           -3
#define NET_RECORD_ROUTE_TUNNEL_UNREACH -4
#define NET_RECORD_ROUTE_TUNNEL_BROKEN  -5
#define NET_RECORD_ROUTE_LOOP		-6

// ----- node_record_route -------------------------------------------
/**
 *
 */
int node_record_route(SNetNode * pNode, SNetDest sDest,
		      SNetPath ** ppPath,
		      net_link_delay_t * pDelay, uint32_t * pWeight, 
		      const uint8_t uDeflection, SNetPath ** ppDeflectedPath)
{
  SNetNode * pCurrentNode= pNode;
  SNetLink * pLink= NULL;
  unsigned int uHopCount= 0;
  int iResult= NET_RECORD_ROUTE_UNREACH;
  SNetPath * pPath= net_path_create(), * pDeflectedPath=net_path_create();
  SStack * pDstStack= stack_create(10);
  net_link_delay_t tTotalDelay= 0;
  uint32_t uTotalWeight= 0;
  net_link_delay_t tLinkDelay= 0;
  uint32_t uLinkWeight= 0;
  int iReached;
  SNetDest * pDestCopy;
  SNetRouteInfo * pRouteInfo;

  net_addr_t tInitialBGPNextHopAddr = 0, tCurrentBGPNextHopAddr = 0;
  SNetProtocol * pProtocol = NULL;
  SBGPRouter * pRouter = NULL;
  SRoute * pRoute = NULL;
  uint8_t uDeflectionOccurs = 0;

  assert((sDest.tType == NET_DEST_PREFIX) ||
	 (sDest.tType == NET_DEST_ADDRESS));

  while (pCurrentNode != NULL) {
      
    if (uDeflection) {
      pProtocol = protocols_get(pCurrentNode->pProtocols, NET_PROTOCOL_BGP);
      if (pProtocol != NULL)
        pRouter = (SBGPRouter *)pProtocol->pHandler;
    }
    
   /* check for a loop */
    if (uDeflection && net_path_search(pPath, pCurrentNode->tAddr)) {
	iResult = NET_RECORD_ROUTE_LOOP;
	net_path_append(pPath, pCurrentNode->tAddr);
	break;
    }

    net_path_append(pPath, pCurrentNode->tAddr);

    tTotalDelay+= tLinkDelay;
    uTotalWeight+= uLinkWeight;

    // Final destination reached ?
    iReached= 0;
    switch (sDest.tType) {
    case NET_DEST_ADDRESS:
      iReached= (pCurrentNode->tAddr == sDest.uDest.tAddr);
      break;
    case NET_DEST_PREFIX:
      iReached= ip_address_in_prefix(pCurrentNode->tAddr,
				     sDest.uDest.sPrefix);
    }

    if (iReached) {

      // Tunnel end-point ?
      if (stack_depth(pDstStack) > 0) {

	// Does the end-point support IP-in-IP protocol ?
	if (protocols_get(pCurrentNode->pProtocols,
			  NET_PROTOCOL_IPIP) == NULL) {
	  iResult= NET_RECORD_ROUTE_TUNNEL_UNREACH;
	  break;
	}
	
	// Pop destination, but current node does not change
	pDestCopy= (SNetDest *) stack_pop(pDstStack);
	sDest= *pDestCopy;
	FREE(pDestCopy);

      } else {

	// Destination reached :-)
	iResult= NET_RECORD_ROUTE_SUCCESS;
	break;

      }

    } else {

	  
      // Lookup the next-hop for this destination
      switch (sDest.tType) {
      case NET_DEST_ADDRESS:
	pLink= node_rt_lookup(pCurrentNode, sDest.uDest.tAddr);
	if (uDeflection) 
	  pRoute = rib_find_best(pRouter->pLocRIB, uint32_to_prefix(sDest.uDest.tAddr, 32));
	break;
      case NET_DEST_PREFIX:
	pRouteInfo= rt_find_exact(pCurrentNode->pRT, sDest.uDest.sPrefix,
			     NET_ROUTE_ANY);
	if (uDeflection)
	  pRoute = rib_find_exact(pRouter->pLocRIB, sDest.uDest.sPrefix);
	if (pRouteInfo != NULL)
	  pLink= pRouteInfo->pNextHopIf;
	else
	  pLink= NULL;
	break;
      }

      /* No route: return UNREACH */
      if (pLink == NULL)
	break;

      /* Link down: return DOWN */
      if (!link_get_state(pLink, NET_LINK_FLAG_UP)) {
	iResult= NET_RECORD_ROUTE_DOWN;
	break;
      }


      //Check if deflection happens on the forwarding path. We check it by the
      //following test: If the Last known BGP NextHop is different from the BGP
      //NH of the current Node it means that there is deflection. In this case
      //we retain the new BGP NH as the last known BGP NH.
      if (uDeflection && pRoute != NULL) {
	tCurrentBGPNextHopAddr = route_nexthop_get(pRoute);
	//We check that 
	//1) it's not the initial node
	//2) bypass the next-hop-self
	//3) finally, the BGP NH is different between the current node and the
	//   previous one
	if (tInitialBGPNextHopAddr != 0 && 
	    pCurrentNode->tAddr != tInitialBGPNextHopAddr &&
	    tInitialBGPNextHopAddr != tCurrentBGPNextHopAddr) { 
	  if (!uDeflectionOccurs){
	    net_path_append(pDeflectedPath, pNode->tAddr);
	    net_path_append(pDeflectedPath, tInitialBGPNextHopAddr);
	    uDeflectionOccurs = 1;
	  }
	    /*** record deflection or print it directly ... ***/
	    //sta : todo We must record it and print it in the previous function that calls this function.
	    //if (uDeflectionOccurs == 0)
	      
	  net_path_append(pDeflectedPath, pCurrentNode->tAddr);
	  net_path_append(pDeflectedPath, tCurrentBGPNextHopAddr);
	    //else
	      //net_path_append(pPath, tCurrentBGP
	}
	tInitialBGPNextHopAddr = tCurrentBGPNextHopAddr;
      }

      tLinkDelay= pLink->tDelay;
      uLinkWeight= pLink->uIGPweight;

      // Handle tunnel encapsulation
      if (link_get_state(pLink, NET_LINK_FLAG_TUNNEL)) {

	/*
	// Push destination to stack
	pDestCopy= (SNetDest *) MALLOC(sizeof(sDest));
	memcpy(pDestCopy, &sDest, sizeof(sDest));
	stack_push(pDstStack, pDestCopy);
	
	tDstAddr= pLink->tAddr;

	// Find new destination (i.e. the tunnel end-point address)
	pLink= node_rt_lookup(pCurrentNode, tDstAddr);
	if (pLink == NULL) {
	  iResult= NET_RECORD_ROUTE_TUNNEL_BROKEN;
	  break;
	}
	*/

      }

      pCurrentNode= network_find_node(pNode->pNetwork, link_get_address(pLink));
      
    }

    uHopCount++;

    if (uHopCount > NET_OPTIONS_MAX_HOPS) {
      iResult= NET_RECORD_ROUTE_TOO_LONG;
      break;
    }

  }

  *ppPath= pPath;
  *ppDeflectedPath = pDeflectedPath;

  stack_destroy(&pDstStack);

  /* Returns the total route delay */
  if (pDelay != NULL)
    *pDelay= tTotalDelay;

  /* Returns the total route weight */
  if (pWeight != NULL)
    *pWeight= uTotalWeight;

  return iResult;
}

typedef struct {
  FILE * pStream;
  // 0 -> Node Addr
  // 1 -> NH Addr
  uint8_t uAddrType;
}SDeflectedDump;

int print_deflected_path_for_each(void * pItem, void * pContext) 
{
  SDeflectedDump * pDump = (SDeflectedDump *)pContext;
  net_addr_t tAddr = *((net_addr_t *)pItem);

  if (pDump->uAddrType == 0) {
    ip_address_dump(pDump->pStream, tAddr);
    fprintf(pDump->pStream, "->");
    pDump->uAddrType = 1;
  } else {
    fprintf(pDump->pStream, "NH:");
    ip_address_dump(pDump->pStream, tAddr);
    fprintf(pDump->pStream, " " );
    pDump->uAddrType = 0;
  }
  return 0;
}
  

// ----- node_dump_recorded_route -----------------------------------
/**
 *
 */
void node_dump_recorded_route(FILE * pStream, SNetNode * pNode,
			      SNetDest sDest, int iDelay, const uint8_t uDeflection)
{
  int iResult;
  SNetPath * pPath, * pDeflectedPath;
  net_link_delay_t tDelay= 0;
  uint32_t uWeight= 0;
  SDeflectedDump pDeflectedDump;

  iResult= node_record_route(pNode, sDest, &pPath,
			     &tDelay, &uWeight, uDeflection, &pDeflectedPath);

  ip_address_dump(pStream, pNode->tAddr);
  fprintf(pStream, "\t");
  ip_dest_dump(pStream, sDest);
  fprintf(pStream, "\t");
  switch (iResult) {
  case NET_RECORD_ROUTE_SUCCESS: fprintf(pStream, "SUCCESS"); break;
  case NET_RECORD_ROUTE_TOO_LONG: fprintf(pStream, "TOO_LONG"); break;
  case NET_RECORD_ROUTE_UNREACH: fprintf(pStream, "UNREACH"); break;
  case NET_RECORD_ROUTE_DOWN: fprintf(pStream, "DOWN"); break;
  case NET_RECORD_ROUTE_TUNNEL_UNREACH:
    fprintf(pStream, "TUNNEL_UNREACH"); break;
  case NET_RECORD_ROUTE_TUNNEL_BROKEN:
    fprintf(pStream, "TUNNEL_BROKEN"); break;
  case NET_RECORD_ROUTE_LOOP:
    fprintf(pStream, "LOOP"); break;
  default:
    fprintf(pStream, "UNKNOWN_ERROR");
  }
  fprintf(pStream, "\t");
  net_path_dump(pStream, pPath);
  if (iDelay) {
    fprintf(pStream, "\t%u\t%u", tDelay, uWeight);
  }
  if (uDeflection && net_path_length(pDeflectedPath) > 0) {
    fprintf(pStream, "\tDEFLECTION\t");
    pDeflectedDump.pStream = pStream;
    pDeflectedDump.uAddrType = 0;
    net_path_for_each(pDeflectedPath, print_deflected_path_for_each, &pDeflectedDump);
  }
  fprintf(pStream, "\n");

  net_path_destroy(&pPath);
  net_path_destroy(&pDeflectedPath);

  flushir(pStream);
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

  assert(node_add_link(pNodeA0, pNodeA1, 100, 1) >= 0);
  assert(node_add_link(pNodeA1, pNodeB0, 1000, 1) >= 0);
  assert(node_add_link(pNodeB0, pNodeB1, 100, 1) >= 0);
  assert(node_add_link(pNodeA1, pNodeC0, 400, 1) >= 0);
  assert(node_add_link(pNodeC0, pNodeB0, 400, 1) >= 0);

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











