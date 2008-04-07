// ==================================================================
// @(#)igp.c
//
// Simple model of an intradomain routing protocol (IGP).
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 23/02/2004
// @lastdate 11/03/2008
// ==================================================================

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <libgds/array.h>
#include <net/igp.h>
#include <net/igp_domain.h>
#include <net/link-list.h>
#include <net/network.h>
#include <net/node.h>
#include <net/routing.h>
#include <net/subnet.h>

// ----- SPTInfo ----------------------------------------------------
/**
 * Information stored in the Shortest-Paths Tree (SPT). For each
 * destination, the following information is stored:
 * - outgoing interface (this is an interface of the root node)
 * - next-hop address (if destination is reachable through a transit
 *   subnet)
 * - weight
 */
typedef struct {
  net_iface_t * pIface;
  net_addr_t tNextHop;
  net_igp_weight_t   tWeight;
} SSPTInfo;

// ----- SPTContext -------------------------------------------------
/**
 * Structure used to push information on the BFS's stack. The
 * following informations are stored:
 * - addr (address of the node to be visited)
 * - iface (outgoing interface used to reach it)
 * - next-hop
 * - weight
 */
typedef struct {
  net_node_t * pNode;
  net_iface_t * pIface;
  net_addr_t tNextHop;
  net_igp_weight_t tWeight;
} SSPTContext;

// ----- _spt_info_create -------------------------------------------
static inline SSPTInfo * _spt_info_create(net_igp_weight_t tWeight)
{
  SSPTInfo * pInfo=
    (SSPTInfo *) MALLOC(sizeof(SSPTInfo));
  pInfo->tWeight= tWeight;
  pInfo->pIface= NULL;
  pInfo->tNextHop= 0;
  return pInfo;
}

// ----- _spt_info_destroy ------------------------------------------
static inline void _spt_info_destroy(void ** pItem)
{
  SSPTInfo * pInfo= *((SSPTInfo **) pItem);

  FREE(pInfo);
}

// ----- _spt_info_update -------------------------------------------
/**
 * Create/update SPT info for visited node
 *
 * To-do:
 * - handle ECMP (problem is the RT structure, not the SPT algo)
 */
static inline int _spt_info_update(SRadixTree * pVisited,
				   SPrefix sPrefix,
				   net_iface_t * pIface,
				   net_addr_t tNextHop,
				   net_igp_weight_t tNewWeight)
{
  int iPushNode= 1;
  SSPTInfo * pInfo= (SSPTInfo *) radix_tree_get_exact(pVisited,
						      sPrefix.tNetwork,
						      sPrefix.uMaskLen);
  if (pInfo == NULL) {
    /*log_printf(pLogErr, "first visit of ");
    ip_prefix_dump(pLogErr, sPrefix);
    log_printf(pLogErr, " (w=%u)\n", tNewWeight);*/

    // Not yet visited: create SPT info
    pInfo= _spt_info_create(tNewWeight);
    pInfo->pIface= pIface;
    pInfo->tNextHop= tNextHop;
    radix_tree_add(pVisited, sPrefix.tNetwork, sPrefix.uMaskLen, pInfo);
  } else if (pInfo->tWeight > tNewWeight) {
    /*log_printf(pLogErr, "update visit of ");
    ip_prefix_dump(pLogErr, sPrefix);
    log_printf(pLogErr, " (w=%u)\n", tNewWeight);*/

    // Update with new weight, interface & next-hop
    pInfo->tWeight= tNewWeight;
    pInfo->pIface= pIface;
    pInfo->tNextHop= tNextHop;
  } else if (pInfo->tWeight == tNewWeight) {
    // Add equal-cost path (ECMP)
    /*	fprintf(stdout, "*** \033[32;1mECMP\033[0m: not yet supported ***\n");
      fprintf(stdout, "node: ");
      ip_address_dump(stdout, tSrcAddr);
      fprintf(stdout, ", dst: ");
	net_iface_dump(pLogOut, pLink);
	fprintf(stdout, "\n");*/
    iPushNode= 0; // do not continue traversal through this node
  } else {
    iPushNode= 0; // do not continue traversal through this node
  }
  return iPushNode;
}

// -----[ _push ]----------------------------------------------------
/**
 * Push additional nodes on the stack. These nodes will be processed
 * later.
 */
static inline void _push(SFIFO * pFIFO,
			 SIGPDomain * pDomain,
			 net_node_t * pNode,
			 net_iface_t * pIface,
			 net_addr_t tNextHop,
			 net_igp_weight_t tWeight)
{
  SSPTContext * pContext;

  /*log_printf(pLogErr, "__push(");
  ip_address_dump(pLogErr, pNode->tAddr);
  log_printf(pLogErr, ")\n");*/

  if (!igp_domain_contains_router(pDomain, pNode)) {
    /*log_printf(pLogErr, "  (node not in domain)\n");*/
    return;
  }

  pContext= (SSPTContext *) MALLOC(sizeof(SSPTContext));
  pContext->pNode= pNode;
  pContext->pIface= pIface;
  pContext->tNextHop= tNextHop;
  pContext->tWeight= tWeight;
  assert(fifo_push(pFIFO, pContext) == 0);
}

// -----[ _is_iface_acceptable ]-------------------------------------
/**
 * Filter unacceptable links. Consider only the links that have the
 * following properties:
 * - link must be enabled (UP)
 * - link must be connected
 * - the IGP weight must be greater than 0
 *   and lower than NET_IGP_MAX_WEIGHT
 */
static inline int _is_iface_acceptable(net_iface_t * pIface)
{
  net_igp_weight_t tWeight;

  if (!net_iface_is_enabled(pIface) ||
      !net_iface_is_connected(pIface)) {
    /*log_printf(pLogErr, "link not connected : ");
    net_iface_dump_id(pLogErr, pIface);
    log_printf(pLogErr, "\n");*/
    return 0;
  }

  tWeight= net_iface_get_metric(pIface, 0);
  if ((tWeight == 0) || (tWeight == NET_IGP_MAX_WEIGHT)) {
    /*log_printf(pLogErr, "link metric is O or max-metric : ");
    net_iface_dump_id(pLogErr, pIface);
    log_printf(pLogErr, "\n");*/
    //LOG_ERR(LOG_LEVEL_WARNING, "Warning: link weight is 0 or infinity !!!\n");
    return 0;
  }
  return 1;
}

// -----[ _spt_bfs ]-------------------------------------------------
/**
 * Compute the Shortest Path Tree (SPT) from the given source router
 * towards all the other routers in the same domain. The algorithm
 * uses a breadth-first-search (BFS) approach.
 *
 * To-do:
 * - replace by Bellman-Ford or Dijkstra ?
 * - clean code (make it easier to read !)
 * - separate SPT computation from routing table generation (i.e. do
 *   not look at the local interfaces at the moment)
 */
static SRadixTree * _spt_bfs(net_node_t * pSrcNode, SIGPDomain * pDomain)
{
  net_iface_t * pLink;
  net_node_t * pNode;
  SFIFO * pFIFO;
  SRadixTree * pVisited;
  SSPTContext * pContext;
  SPrefix sInfoPrefix;
  int iIndex;
  unsigned int uIndex;
  net_iface_t * pIface;   // Root node's outgoing interface along current path
  net_addr_t tNextHop; // Root node's nexthop along current path
  net_ifaces_t * ifaces;
  net_igp_weight_t tWeight;
  net_igp_weight_t tNewWeight;

  /*log_printf(pLogErr, "** starting from ");
  ip_address_dump(pLogErr, pSrcNode->tAddr);
  log_printf(pLogErr, " **\n");*/

  pVisited= radix_tree_create(32, _spt_info_destroy);
  pFIFO= fifo_create(100000, NULL);
  _push(pFIFO, pDomain, pSrcNode, NULL, 0, 0);
  _spt_info_update(pVisited, net_iface_id_addr(pSrcNode->tAddr),
		   NULL, 0, 0);

  // -----| Breadth-first search |-----
  while (1) {

    pContext= (SSPTContext *) fifo_pop(pFIFO);
    if (pContext == NULL)
      break;
    pNode= pContext->pNode;

    // Update loopback interface (interface metric is always 0)
    // Determine the root node's outgoing interface/next-hop
    // to be used for current path
    if (pContext->pIface != NULL) {
      pIface= pContext->pIface;
      tNextHop= pContext->tNextHop;
    } else {
      pIface= NULL; // No interface for loopback
      tNextHop= 0;
    }
    sInfoPrefix= net_iface_id_addr(pNode->tAddr);
    _spt_info_update(pVisited, sInfoPrefix, pIface, tNextHop, pContext->tWeight);
    
    // Traverse all the outbound links of the current node
    for (iIndex= 0; iIndex < net_ifaces_size(pNode->ifaces); iIndex++) {
      pLink= net_ifaces_at(pNode->ifaces, iIndex);

      /*log_printf(pLogErr, "from ");
      ip_address_dump(pLogErr, pNode->tAddr);
      log_printf(pLogErr, " traverse interface ");
      net_iface_dump_id(pLogErr, pLink);
      log_printf(pLogErr, "  (w=%u)\n", net_iface_get_metric(pLink, 0));*/

      if (!_is_iface_acceptable(pLink))
	continue;

      tWeight= net_iface_get_metric(pLink, 0);

      // Determine the root node's outgoing interface/next-hop
      // to be used for current path
      if (pContext->pIface != NULL) {
	pIface= pContext->pIface;
	tNextHop= pContext->tNextHop;
      } else {
	pIface= pLink;
	tNextHop= 0;
      }

      // Compute weight to reach destination through this link
      tNewWeight= net_igp_add_weights(pContext->tWeight, tWeight);

      // Retrieve SPT info for destination depending on link type...
      sInfoPrefix= net_iface_dst_prefix(pLink);
      if (!_spt_info_update(pVisited, sInfoPrefix, pIface, tNextHop, tNewWeight)) {
	/*log_printf(pLogErr, "  (path towards ");
	ip_prefix_dump(pLogErr, sInfoPrefix);
	log_printf(pLogErr, " is not better)\n");*/
	continue;
      }

      // Push current destination onto stack if further exploration
      // is required from here. This is depending on the iface's type
      switch (pLink->type) {
      case NET_IFACE_RTR:
      case NET_IFACE_PTP:
	_push(pFIFO, pDomain, pLink->dest.iface->src_node,
	      pIface, tNextHop, tNewWeight);
	break;

      case NET_IFACE_PTMP:
	if (!subnet_is_transit(pLink->dest.subnet))
	  continue;
	ifaces= pLink->dest.subnet->ifaces;
	for (uIndex= 0; uIndex < ptr_array_length(ifaces); uIndex++) {
	  net_iface_t * pSubLink= net_ifaces_at(ifaces, uIndex);

	  // Skip if we reached the subnet through this link
	  if (pSubLink == pLink)
	    continue;

	  // Skip if sub-link is not UP
	  if (!net_iface_is_enabled(pSubLink)) {
	    /*log_printf(pLogErr, "subnet link is disabled : ");
	    net_iface_dump_id(pLogErr, pSubLink);
	    log_printf(pLogErr, "\n");*/
	    continue;
	  }

	  // We traverse the sub-link in reverse direction (i.e. from
	  // subnet to node) => don't take weight into account

	  sInfoPrefix.tNetwork= pSubLink->src_node->tAddr;
	  sInfoPrefix.uMaskLen= 32;
	  if (igp_domain_contains_router(pDomain, pSubLink->src_node)) {

	    // Update the next-hop if the subnet is a direct link from
	    // the root node. Otherwise, use previously computed
	    // next-hop (from context).
	    if (pIface == pLink) {
	      tNextHop= pSubLink->tIfaceAddr;
	    }

	    if (_spt_info_update(pVisited, sInfoPrefix, pIface, tNextHop, tNewWeight))
	      _push(pFIFO, pDomain, pSubLink->src_node,
		    pIface, tNextHop, tNewWeight);
	  }
	}
	break;
      default:
	log_printf(pLogErr, "WARNING: unsupported link type (%d), "
		   "ignored by IGP\n", pLink->type);
      }
    }
    FREE(pContext);
  }
  fifo_destroy(&pFIFO);

  return pVisited;
}

// ----- _igp_compute_prefix_for_each -------------------------------
static int _igp_compute_prefix_for_each(uint32_t uKey, uint8_t uKeyLen,
					void * pItem, void * pContext)
{
  net_node_t * pNode= (net_node_t *) pContext;
  SSPTInfo * pInfo= (SSPTInfo *) pItem;
  SPrefix sPrefix;
  int iResult;

  // Skip route to itself
  if ((pNode->tAddr == (net_addr_t) uKey) || (pInfo->pIface == NULL))
    return 0;

  //log_printf(pLogErr, "*** node: ");
  //ip_address_dump(pLogErr, pNode->tAddr);
  //log_printf(pLogErr, ", prefix: ");
  //ip_address_dump(pLogErr, (net_addr_t) uKey);
  //log_printf(pLogErr, "/%u, iface: ", uKeyLen);
  //link_dump(pLogErr, pInfo->pIface);
  //log_printf(pLogErr, " (next-hop: ");
  //ip_address_dump(pLogErr, pInfo->tNextHop);
  //log_printf(pLogErr, ")\n");

  // Add IGP route
  sPrefix.tNetwork= uKey;
  sPrefix.uMaskLen= uKeyLen;

  /* removes the previous route for this node/prefix if it already
     exists */
  node_rt_del_route(pNode, &sPrefix, NULL, NULL, NET_ROUTE_IGP);

  iResult= node_rt_add_route_link(pNode, sPrefix,
				  pInfo->pIface, pInfo->tNextHop,
				  pInfo->tWeight, NET_ROUTE_IGP);
  /*if (iResult != 0)
    rt_perror(pLogErr, iResult);*/
  return iResult;
}

// ----- igp_compute_rt_for_each --------------------------------
/**
 *
 */
static int _igp_compute_rt_for_each(uint32_t uKey, uint8_t uKeyLen,
				    void * pItem, void * pContext)
{
  int iResult;
  SIGPDomain * pDomain= (SIGPDomain *) pContext;
  net_node_t * pNode= (net_node_t *) pItem;
  SRadixTree * pTree;

  /* Remove all IGP routes from node */
  node_rt_del_route(pNode, NULL, NULL, NULL, NET_ROUTE_IGP);

  /* Compute shortest-path tree (SPT) */
  pTree= _spt_bfs(pNode, pDomain);
  if (pTree == NULL)
    return -1;

  /* Add routes corresponding to the SPT */
  iResult= radix_tree_for_each(pTree, _igp_compute_prefix_for_each, pNode);

  radix_tree_destroy(&pTree);
  return iResult;
}

// ----- igp_compute_domain -----------------------------------------
/**
 * Compute the shortest-path tree (SPT) and build the routing table
 * for each router within the given domain.
 *
 * To-do:
 * - use enumerator instead of for-each function.
 */
int igp_compute_domain(SIGPDomain * pDomain)
{
  return trie_for_each(pDomain->pRouters,
		       _igp_compute_rt_for_each,
		       pDomain);
}
