// ==================================================================
// @(#)record-route.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @author Sebastien Tandel (sta@info.ucl.ac.be)
// @date 04/08/2003
// $Id: record-route.c,v 1.11 2008-04-11 11:03:07 bqu Exp $
// ==================================================================

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include <libgds/log.h>
#include <libgds/memory.h>

#include <net/error.h>
#include <net/iface.h>
#include <net/ip_trace.h>
#include <net/network.h>
#include <net/node.h>
#include <net/record-route.h>
#include <ui/output.h>

// -----[ _info_create ]---------------------------------------------
static inline SNetRecordRouteInfo * _info_create(const uint8_t uOptions,
						 net_link_load_t tLoad,
						 net_tos_t tTOS)
{
  SNetRecordRouteInfo * pInfo=
    (SNetRecordRouteInfo *) MALLOC(sizeof(SNetRecordRouteInfo));
  pInfo->uOptions= uOptions;
  pInfo->iResult= EUNEXPECTED;
  pInfo->pTrace= ip_trace_create();
  pInfo->tDelay= 0;
  pInfo->tWeight= 0;
  pInfo->tCapacity= NET_LINK_MAX_CAPACITY;
  pInfo->tLoad= tLoad;
  pInfo->tTOS= tTOS;
  pInfo->pDeflectedPath= NULL;
  return pInfo;
}

// -----[ _check_deflection ]----------------------------------------
static inline int _check_deflection(SNetRecordRouteInfo * pInfo,
				    net_node_t * pCNode)
{
  if (!(pInfo->uOptions & NET_RECORD_ROUTE_OPTION_QUICK_LOOP))
    return ESUCCESS;

  /*
    pRRInfo->pDeflectedPath= net_path_create();
    if (uDeflection) 
    pRoute = rib_find_best(pRouter->pLocRIB,
    uint32_to_prefix(sDest.uDest.tAddr, 32));
    if (uDeflection)
    pRoute = rib_find_exact(pRouter->pLocRIB, sDest.uDest.sPrefix);
    
    // Check if deflection happens on the forwarding path. We check it
    // by the following test: if the last known BGP NextHop is
    // different from the BGP NH of the current node it means that
    // there is deflection. In this case we remember the new BGP NH as
    // the last known BGP NH.
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
    
    net_addr_t tInitialBGPNextHopAddr = 0, tCurrentBGPNextHopAddr = 0;
    net_protocol_t * pProtocol = NULL;
    SBGPRouter * pRouter = NULL;
    SRoute * pRoute = NULL;
    uint8_t uDeflectionOccurs = 0;
    
    if (uOptions & NET_RECORD_ROUTE_OPTION_DEFLECTION) {
    pProtocol = protocols_get(pCurrentNode->pProtocols, NET_PROTOCOL_BGP);
    if (pProtocol != NULL)
    pRouter = (SBGPRouter *)pProtocol->pHandler;
    }
  */
  return ESUCCESS;
}
  
// -----[ _check_loop ]----------------------------------------------
/**
 * Checks if the current trace contains a loop.
 *
 * Note: will this work with encapsulation ?
 */
static inline int _check_loop(SNetRecordRouteInfo * pInfo,
			      net_node_t * pCNode)
{
  if (!(pInfo->uOptions & NET_RECORD_ROUTE_OPTION_QUICK_LOOP))
    return ESUCCESS;

  if (ip_trace_search(pInfo->pTrace, pCNode)) {
    pInfo->iResult= ENET_FWD_LOOP;
    return pInfo->iResult;
  }

  return ESUCCESS;
}

// -----[ _info_update_qos ]-----------------------------------------
/**
 * Update the record-route info with the traversed link's QoS info.
 */
static inline void _info_update_qos(SNetRecordRouteInfo * pInfo,
				    net_iface_t * pIface)
{
  net_link_load_t tCapacity;
  
  // Keep total propagation delay
  pInfo->tDelay+= net_iface_get_delay(pIface);

  // Keep total IGP weight (for the given topology, i.e. TOS)
  pInfo->tWeight+= net_iface_get_metric(pIface, pInfo->tTOS);

  // Keep maximum possible capacity (=> keep minimum along the path)
  tCapacity= net_iface_get_capacity(pIface);
  if (tCapacity < pInfo->tCapacity)
    pInfo->tCapacity= tCapacity;

  // Load the outgoing link, if requested.
  if (pInfo->uOptions & NET_RECORD_ROUTE_OPTION_LOAD)
    net_iface_add_load(pIface, pInfo->tLoad);
}

// -----[ _find_nexthop ]--------------------------------------------
static inline int _find_nexthop(net_node_t * pCNode,
				SNetDest sDest,
				net_iface_t ** ppDstIface,
				SNetRecordRouteInfo * pInfo,
				net_msg_t ** ppDummyMsg)
{
  net_addr_t tNextHop;
  rt_info_t * rtinfo;
  const rt_entry_t * pNextHop= NULL;
  //int iResult;

  // Lookup the next-hop for this destination
  switch (sDest.tType) {
  case NET_DEST_ADDRESS:
    pNextHop= node_rt_lookup(pCNode, sDest.uDest.tAddr);
    break;

  case NET_DEST_PREFIX:
    rtinfo= rt_find_exact(pCNode->rt, sDest.uDest.sPrefix,
			  NET_ROUTE_ANY);
    if (rtinfo != NULL)
      pNextHop= &rtinfo->next_hop;
    break;

  default:
    abort();
  }

  // No route: return UNREACH
  if (pNextHop == NULL)
    return ENET_NET_UNREACH;

  // Link down: return DOWN
  if (!net_iface_is_enabled(pNextHop->oif))
    return ENET_LINK_DOWN;

  // Update QoS information
  _info_update_qos(pInfo, pNextHop->oif);

  // Next-hop address on point-to-multipoint links
  // (this is a simplified model of ARP)
  if (pNextHop->gateway == 0)
    tNextHop= sDest.uDest.tAddr;
  else
    tNextHop= pNextHop->gateway;

  // Forward along the link (using the link's method). The result
  // is the destination's incoming interface
  //
  // Note/Idea: we could use the returned message as the stack of
  // destinations.
  /*iResult= net_iface_send(pNextHop->pIface, ppDstIface,
			  tNextHop, ppDummyMsg);
  if (iResult != ESUCCESS)
  return iResult;*/
   
  return ESUCCESS;
}

// -----[ _is_final_destination ]------------------------------------
static inline int _is_final_destination(net_node_t * pNode, SNetDest sDest)
{
  switch (sDest.tType) {
  case NET_DEST_ADDRESS:
    // Check if the current node has the destination address
    return node_has_address(pNode, sDest.uDest.tAddr);

  case NET_DEST_PREFIX:
    // Check if any interface on the current node has the same prefix
    // as the destination
    return node_has_prefix(pNode, sDest.uDest.sPrefix);

  default:
    abort();
  }
}

// ----- node_record_route ------------------------------------------
SNetRecordRouteInfo * node_record_route(net_node_t * pNode,
					SNetDest sDest,
					net_tos_t tTOS,
					const uint8_t uOptions,
					net_link_load_t tLoad)
{
  SNetRecordRouteInfo * pRRInfo;
  net_node_t * pCNode= pNode;           /* Current node */
  net_addr_t tCInAddr= NET_ADDR_ANY;  /* Current incoming iface addr */
  net_addr_t tCOutAddr= NET_ADDR_ANY; /* Current outgoing iface addr */
  int iResult;
  net_msg_t * pDummyMsg= NULL;
  net_iface_t * pDstIface;

  return NULL;

  pRRInfo= _info_create(uOptions, tLoad, tTOS);

  assert((sDest.tType == NET_DEST_PREFIX) ||
	 (sDest.tType == NET_DEST_ADDRESS));

  while (pCNode != NULL) {

    ip_trace_add_node(pRRInfo->pTrace, pCNode, tCInAddr, tCOutAddr);

    if (_check_loop(pRRInfo, pCNode) != ESUCCESS)
      break;
    if (_check_deflection(pRRInfo, pCNode) != ESUCCESS)
      break;
    
    // Final destination reached ?
    if (_is_final_destination(pCNode, sDest)) {
      pRRInfo->iResult= ESUCCESS;
      break;
    }

    // Find next-hop towards destination
    pDstIface= NULL;
    iResult= _find_nexthop(pCNode, sDest, &pDstIface, pRRInfo, &pDummyMsg);
    if (iResult != ESUCCESS) {
      pRRInfo->iResult= iResult;
      break;
    }

    switch (pDstIface->type) {
    case NET_IFACE_PTP:
    case NET_IFACE_PTMP:
      tCInAddr= pDstIface->tIfaceAddr;
      break;
    case NET_IFACE_RTR:
      tCInAddr= NET_ADDR_ANY;
      break;
    default:
      abort();
    }

    pCNode= pDstIface->src_node;
	  
  }
  return pRRInfo;
}

typedef struct {
  SLogStream * pStream;
  // 0 -> Node Addr
  // 1 -> NH Addr
  uint8_t uAddrType;
} SDeflectedDump;

// -----[ _print_deflected_path_for_each ]---------------------------
/*static int _print_deflected_path_for_each(void * pItem,
					  void * pContext)
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
  }*/

// ----- node_dump_recorded_route -----------------------------------
/**
 * Dump the recorded route.
 *
 * Format:
 *   <src> <dst> <status> <num-hops> <hop-1>...<hop-N> [delay:<delay> ...]
 */
void node_dump_recorded_route(SLogStream * stream,
			      net_node_t * node,
			      SNetDest sDest,
			      net_tos_t tOS,
			      uint8_t options,
			      net_link_load_t load)
{
  unsigned int index;
  ip_trace_item_t * trace_item;
  ip_trace_t * trace;
  net_error_t error;

  if (sDest.tType == NET_DEST_PREFIX)
    options|= ICMP_RR_OPTION_ALT_DEST;

  error= icmp_record_route(node, sDest.uDest.tAddr,
			   &sDest.uDest.sPrefix,
			   255, options, &trace, load);

  ip_address_dump(stream, node->addr);
  log_printf(stream, "\t");
  ip_dest_dump(stream, sDest);
  log_printf(stream, "\t");

  switch (trace->status) {
  case ESUCCESS:
    log_printf(stream, "SUCCESS"); break;
  case ENET_FWD_LOOP:
    log_printf(stream, "LOOP"); break;
  default:
    log_printf(stream, "UNREACH");
  }

  log_printf(stream, "\t%u\t", ip_trace_length(trace));
  for (index= 0; index < ip_trace_length(trace); index++) {
    if (index > 0)
      log_printf(stream, " ");
    trace_item= ip_trace_item_at(trace, index);
    switch (trace_item->elt.type) {
    case NODE:
      ip_address_dump(stream, trace_item->elt.node->addr);
      break;
    case SUBNET: 
      ip_prefix_dump(stream, trace_item->elt.subnet->prefix);
      break;
    default:
      fatal("invalid network-element type (%d)\n",
	    trace_item->elt.type);
    }
  }

  // Total propagation delay requested ?
  if (options & NET_RECORD_ROUTE_OPTION_DELAY)
    log_printf(stream, "\tdelay:%u", trace->delay);

  // Total IGP weight requested ?
  if (options & NET_RECORD_ROUTE_OPTION_WEIGHT)
    log_printf(stream, "\tweight:%u", trace->weight);

  // Maximum capacity requested ?
  if (options & NET_RECORD_ROUTE_OPTION_CAPACITY)
    log_printf(stream, "\tcapacity:%u", trace->capacity);

  /*if ((uOptions & NET_RECORD_ROUTE_OPTION_DEFLECTION) &&
      (net_path_length(pRRInfo->pDeflectedPath) > 0)) {
    log_printf(pStream, "\tDEFLECTION\t");
    pDeflectedDump.pStream= pStream;
    pDeflectedDump.uAddrType= 0;
    net_path_for_each(pRRInfo->pDeflectedPath,
		      _print_deflected_path_for_each,
		      &pDeflectedDump);
		      }*/

  log_printf(stream, "\n");

  //net_record_route_info_destroy(&pRRInfo);

  log_flush(stream);
}

// -----[ net_record_route_info_destroy ]----------------------------
void net_record_route_info_destroy(SNetRecordRouteInfo ** ppInfo)
{
  if (*ppInfo != NULL) {
    net_path_destroy(&(*ppInfo)->pDeflectedPath);
    ip_trace_destroy(&(*ppInfo)->pTrace);
    FREE(*ppInfo);
    *ppInfo= NULL;
  }
}
