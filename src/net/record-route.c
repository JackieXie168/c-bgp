// ==================================================================
// @(#)record-route.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @author Sebastien Tandel (sta@info.ucl.ac.be)
// @date 04/08/2003
// @lastdate 31/05/2007
// ==================================================================

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include <libgds/log.h>
#include <libgds/memory.h>

#include <net/error.h>
#include <net/network.h>
#include <net/node.h>
#include <net/record-route.h>
#include <ui/output.h>

// ----- net_record_route_info_create -------------------------------
SNetRecordRouteInfo * net_record_route_info_create(const uint8_t uOptions)
{
  SNetRecordRouteInfo * pRRInfo=
    (SNetRecordRouteInfo *) MALLOC(sizeof(SNetRecordRouteInfo));
  pRRInfo->uOptions= uOptions;
  pRRInfo->iResult= NET_ERROR_UNEXPECTED;
  pRRInfo->pPath= net_path_create();
  pRRInfo->tDelay= 0;
  pRRInfo->tWeight= 0;
  pRRInfo->tCapacity= NET_LINK_MAX_CAPACITY;

  if (uOptions & NET_RECORD_ROUTE_OPTION_DEFLECTION)
    pRRInfo->pDeflectedPath= net_path_create();
  else
    pRRInfo->pDeflectedPath= NULL;

  return pRRInfo;
}

// ----- net_record_route_info_destroy ------------------------------
void net_record_route_info_destroy(SNetRecordRouteInfo ** ppRRInfo)
{
  if (*ppRRInfo != NULL) {
    net_path_destroy(&(*ppRRInfo)->pPath);
    net_path_destroy(&(*ppRRInfo)->pDeflectedPath);
    FREE(*ppRRInfo);
    *ppRRInfo= NULL;
  }
}

// ----- _node_record_route_nexthop ----------------------------------
static int _node_record_route_nexthop(SNetNode * pCurrentNode,
				      SNetDest sDest,
				      net_tos_t tTOS,
				      SNetNode ** ppNextHopNode,
				      net_link_delay_t * ptLinkDelay,
				      net_igp_weight_t * ptLinkWeight,
				      net_link_load_t * ptLinkCapacity,
				      net_link_load_t tLoad,
				      uint8_t uOptions)
{
  net_addr_t tNextHop;
  SNetRouteInfo * pRouteInfo;
  SNetRouteNextHop * pNextHop;
  int iResult;

  // Lookup the next-hop for this destination
  switch (sDest.tType) {
  case NET_DEST_ADDRESS:
    pNextHop= node_rt_lookup(pCurrentNode, sDest.uDest.tAddr);
    /*
      if (uDeflection) 
      pRoute = rib_find_best(pRouter->pLocRIB, uint32_to_prefix(sDest.uDest.tAddr, 32));
    */
    break;
  case NET_DEST_PREFIX:
    pRouteInfo= rt_find_exact(pCurrentNode->pRT, sDest.uDest.sPrefix,
			      NET_ROUTE_ANY);
    /*
      if (uDeflection)
      pRoute = rib_find_exact(pRouter->pLocRIB, sDest.uDest.sPrefix);
    */
    if (pRouteInfo != NULL)
      pNextHop= &pRouteInfo->sNextHop;
    else
      pNextHop= NULL;
    break;
  default:
    abort();
  }

  // No route: return UNREACH
  if (pNextHop == NULL)
    return NET_ERROR_NET_UNREACH;
  
  // Link down: return DOWN
  if (!net_link_get_state(pNextHop->pIface, NET_LINK_FLAG_UP))
    return NET_ERROR_LINK_DOWN;
  
  /*
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
  //record deflection or print it directly ...
  //sta : todo We must record it and print it in the previous function that calls this function.
  //if (uDeflectionOccurs == 0)
  
  net_path_append(pDeflectedPath, pCurrentNode->tAddr);
  net_path_append(pDeflectedPath, tCurrentBGPNextHopAddr);
  //else
  //net_path_append(pPath, tCurrentBGP
  }
  tInitialBGPNextHopAddr = tCurrentBGPNextHopAddr;
  }
  */

  // Keep total propagation delay
  *ptLinkDelay= net_link_get_delay(pNextHop->pIface);

  // Keep total IGP weight (for the given topology, i;e. TOS)
  *ptLinkWeight= net_link_get_weight(pNextHop->pIface, tTOS);

  // Keep maximum possible capacity (=> keep minimum along the path)
  *ptLinkCapacity= net_link_get_capacity(pNextHop->pIface);

  // Load the outgoing link, if requested.
  if (uOptions & NET_RECORD_ROUTE_OPTION_LOAD)
    net_link_add_load(pNextHop->pIface, tLoad);


  // Handle tunnel encapsulation
  /*
    if (link_get_state(pLink, NET_LINK_FLAG_TUNNEL)) {
    
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
    
    }
  */

  // Next-hop address on point-to-multipoint links
  // (this is a simplified model of ARP resolution)
  if (pNextHop->tGateway == 0)
    tNextHop= sDest.uDest.tAddr;
  else
    tNextHop= pNextHop->tGateway;

  // Forward along the link (using the link's method). The result
  // is the outgoing interface (link) and an optional next-hop (???)
  iResult= pNextHop->pIface->fForward(tNextHop,
				      pNextHop->pIface->pContext,
				      ppNextHopNode, NULL);
  if (iResult != NET_SUCCESS)
    return iResult;
   
  return 0;
}

// ----- node_record_route ------------------------------------------
/**
 *
 */
SNetRecordRouteInfo * node_record_route(SNetNode * pNode,
					SNetDest sDest,
					net_tos_t tTOS,
					const uint8_t uOptions,
					net_link_load_t tLoad)
{
  SNetRecordRouteInfo * pRRInfo;
  SNetNode * pCurrentNode= pNode;
  unsigned int uHopCount= 0;
  int iReached;
  int iResult;
  // Keep path performance metrics
  net_link_delay_t tLinkDelay= 0;
  net_igp_weight_t tLinkWeight= 0;
  net_link_load_t tLinkCapacity= NET_LINK_MAX_CAPACITY;
  /*
  SStack * pDstStack= stack_create(10);
  SNetDest * pDestCopy;
  */

  pRRInfo= net_record_route_info_create(uOptions);
  
  
  /* DEFLECTION CHECK */
/*  net_addr_t tInitialBGPNextHopAddr = 0, tCurrentBGPNextHopAddr = 0;
  SNetProtocol * pProtocol = NULL;
  SBGPRouter * pRouter = NULL;
  SRoute * pRoute = NULL;
  uint8_t uDeflectionOccurs = 0;*/
  

  assert((sDest.tType == NET_DEST_PREFIX) ||
	 (sDest.tType == NET_DEST_ADDRESS));

  while (pCurrentNode != NULL) {

          
/*    if (uOptions & NET_RECORD_ROUTE_OPTION_DEFLECTION) {
      pProtocol = protocols_get(pCurrentNode->pProtocols, NET_PROTOCOL_BGP);
      if (pProtocol != NULL)
        pRouter = (SBGPRouter *)pProtocol->pHandler;
    }*/

    /* Check for a loop */
    if ((uOptions & NET_RECORD_ROUTE_OPTION_QUICK_LOOP) &&
	net_path_search(pRRInfo->pPath, pCurrentNode->tAddr)) {
      pRRInfo->iResult = NET_ERROR_FWD_LOOP;
      net_path_append(pRRInfo->pPath, pCurrentNode->tAddr);
      break;
    }
    
    net_path_append(pRRInfo->pPath, pCurrentNode->tAddr);

    // Update path performance metrics
    pRRInfo->tDelay+= tLinkDelay;
    pRRInfo->tWeight+= tLinkWeight;
    if (tLinkCapacity < pRRInfo->tCapacity)
      pRRInfo->tCapacity= tLinkCapacity;

    // Final destination reached ?
    iReached= 0;
    switch (sDest.tType) {
    case NET_DEST_ADDRESS:
      iReached= node_has_address(pCurrentNode, sDest.uDest.tAddr);
      break;
    case NET_DEST_PREFIX:
      iReached= ip_address_in_prefix(pCurrentNode->tAddr,
				     sDest.uDest.sPrefix);
    }

    if (iReached) {

      // Tunnel end-point ?
      /*
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

	} else {*/

	// Destination reached :-)
	pRRInfo->iResult= NET_SUCCESS;
	break;

	//}

    } else {

      iResult= _node_record_route_nexthop(pCurrentNode,
					  sDest,
					  tTOS,
					  &pCurrentNode,
					  &tLinkDelay,
					  &tLinkWeight,
					  &tLinkCapacity,
					  tLoad,
					  uOptions);
      if (iResult != 0) {
	pRRInfo->iResult= iResult;
	break;
      }
	  
    }

    uHopCount++;

    if (uHopCount > NET_OPTIONS_MAX_HOPS) {
      pRRInfo->iResult= NET_ERROR_TIME_EXCEEDED;
      break;
    }

  }

  //stack_destroy(&pDstStack);

  return pRRInfo;
}

typedef struct {
  SLogStream * pStream;
  // 0 -> Node Addr
  // 1 -> NH Addr
  uint8_t uAddrType;
} SDeflectedDump;

int print_deflected_path_for_each(void * pItem, void * pContext) 
{
  SDeflectedDump * pDump = (SDeflectedDump *)pContext;
  net_addr_t tAddr = *((net_addr_t *)pItem);

  if (pDump->uAddrType == 0) {
    ip_address_dump(pDump->pStream, tAddr);
    log_printf(pDump->pStream, "->");
    pDump->uAddrType = 1;
  } else {
    log_printf(pDump->pStream, "NH:");
    ip_address_dump(pDump->pStream, tAddr);
    log_printf(pDump->pStream, " " );
    pDump->uAddrType = 0;
  }
  return 0;
}

// -----[ net_record_route_perror ]----------------------------------
static void net_record_route_perror(SLogStream * pStream, int iErrorCode)
{
#define LOG(M) log_printf(pStream, M); break;
  switch (iErrorCode) {
  case NET_SUCCESS:
    LOG("SUCCESS");
  case NET_ERROR_NET_UNREACH:
    LOG("UNREACH");
  case NET_ERROR_HOST_UNREACH:
    LOG("UNREACH");
  case NET_ERROR_LINK_DOWN:
    LOG("DOWN");
  case NET_ERROR_FWD_LOOP:
    LOG("LOOP");
    /*
  case NET_RECORD_ROUTE_TUNNEL_UNREACH:
    LOG("TUNNEL_UNREACH");
  case NET_RECORD_ROUTE_TUNNEL_BROKEN:
    LOG("TUNNEL_BROKEN");
    */
  default:
    LOG("UNKNOWN");
  }
}

// ----- node_dump_recorded_route -----------------------------------
/**
 * Dump the recorded route.
 *
 * Format:
 *   <src> <dst> <status> <num-hops> <hop-1>...<hop-N> [delay:<delay> ...]
 */
void node_dump_recorded_route(SLogStream * pStream, SNetNode * pNode,
			      SNetDest sDest, net_tos_t tTOS,
			      const uint8_t uOptions,
			      net_link_load_t tLoad)
{
  SDeflectedDump pDeflectedDump;
  SNetRecordRouteInfo * pRRInfo;

  pRRInfo= node_record_route(pNode, sDest, tTOS, uOptions, tLoad);
  assert(pRRInfo != NULL);

  ip_address_dump(pStream, pNode->tAddr);
  log_printf(pStream, "\t");
  ip_dest_dump(pStream, sDest);
  log_printf(pStream, "\t");
  net_record_route_perror(pStream, pRRInfo->iResult);
  log_printf(pStream, "\t%u\t", net_path_length(pRRInfo->pPath));
  net_path_dump(pStream, pRRInfo->pPath);

  // Total propagation delay requested ?
  if (uOptions & NET_RECORD_ROUTE_OPTION_DELAY)
    log_printf(pStream, "\tdelay:%u", pRRInfo->tDelay);

  // Total IGP weight requested ?
  if (uOptions & NET_RECORD_ROUTE_OPTION_WEIGHT)
    log_printf(pStream, "\tweight:%u", pRRInfo->tWeight);

  // Maximum capacity requested ?
  if (uOptions & NET_RECORD_ROUTE_OPTION_CAPACITY)
    log_printf(pStream, "\tcapacity:%u", pRRInfo->tCapacity);

  if ((uOptions & NET_RECORD_ROUTE_OPTION_DEFLECTION) &&
      (net_path_length(pRRInfo->pDeflectedPath) > 0)) {
    log_printf(pStream, "\tDEFLECTION\t");
    pDeflectedDump.pStream= pStream;
    pDeflectedDump.uAddrType= 0;
    net_path_for_each(pRRInfo->pDeflectedPath,
		      print_deflected_path_for_each,
		      &pDeflectedDump);
  }

  log_printf(pStream, "\n");

  net_record_route_info_destroy(&pRRInfo);

  log_flush(pStream);
}

