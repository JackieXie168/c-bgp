// ==================================================================
// @(#)igp.c
//
// Simple model of an intradomain routing protocol (IGP).
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 23/02/2004
// @lastdate 04/07/2007
// ==================================================================

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <libgds/array.h>
#include <net/igp.h>
#include <net/igp_domain.h>
#include <net/network.h>
#include <net/node.h>
#include <net/routing.h>

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
  SNetLink * pIface;
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
  net_addr_t tAddr;
  SNetLink * pIface;
  net_addr_t tNextHop;
  net_igp_weight_t tWeight;
} SSPTContext;

// ----- _spt_info_create -------------------------------------------
static SSPTInfo * _spt_info_create(net_igp_weight_t tWeight)
{
  SSPTInfo * pInfo=
    (SSPTInfo *) MALLOC(sizeof(SSPTInfo));
  pInfo->tWeight= tWeight;
  pInfo->pIface= NULL;
  pInfo->tNextHop= 0;
  return pInfo;
}

// ----- _spt_info_destroy ------------------------------------------
static void _spt_info_destroy(void ** pItem)
{
  SSPTInfo * pInfo= *((SSPTInfo **) pItem);

  FREE(pInfo);
}

// ----- _spt_info_update -------------------------------------------
/**
 * Create/update SPT info for visited node
 */
static int _spt_info_update(SRadixTree * pVisited, SPrefix sPrefix,
			    SNetLink * pIface, net_addr_t tNextHop,
			    net_igp_weight_t tNewWeight)
{
  int iPushNode= 1;
  SSPTInfo * pInfo= (SSPTInfo *) radix_tree_get_exact(pVisited,
						      sPrefix.tNetwork,
						      sPrefix.uMaskLen);
  if (pInfo == NULL) {
    // Not yet visited: create SPT info
    pInfo= _spt_info_create(tNewWeight);
    pInfo->pIface= pIface;
    pInfo->tNextHop= tNextHop;
    radix_tree_add(pVisited, sPrefix.tNetwork, sPrefix.uMaskLen, pInfo);
  } else if (pInfo->tWeight > tNewWeight) {
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
	link_dst_dump(stdout, pLink);
	fprintf(stdout, "\n");*/
    iPushNode= 0; // do not continue traversal through this node
  } else {
    iPushNode= 0; // do not continue traversal through this node
  }
  return iPushNode;
}

// ----- spt_bfs ----------------------------------------------------
/**
 * Compute the Shortest Path Tree (SPT) from the given source router
 * towards all the other routers in the same domain. The algorithm
 * uses a breadth-first-search (BFS) approach.
 *
 * TODO: replace by Bellman-Ford or Dijkstra.
 */
SRadixTree * spt_bfs(SNetwork * pNetwork, net_addr_t tSrcAddr,
		     SIGPDomain * pDomain)
{
  SNetLink * pLink;
  SNetNode * pNode;
  SFIFO * pFIFO;
  SRadixTree * pVisited;
  SSPTContext * pContext, * pOldContext;
  SPrefix sInfoPrefix;
  int iIndex;
  unsigned int uIndex;
  SNetLink * pIface;
  net_addr_t tNextHop;
  SPtrArray * pLinks;
  net_igp_weight_t tWeight;
  net_igp_weight_t tNewWeight;

  pVisited= radix_tree_create(32, _spt_info_destroy);
  pFIFO= fifo_create(100000, NULL);
  pContext= (SSPTContext *) MALLOC(sizeof(SSPTContext));
  pContext->tAddr= tSrcAddr;
  pContext->pIface= NULL;
  pContext->tNextHop= 0;
  pContext->tWeight= 0;
  fifo_push(pFIFO, pContext);
  radix_tree_add(pVisited, tSrcAddr, 32, _spt_info_create(0));

  // -----| Breadth-first search |-----
  while (1) {

    pContext= (SSPTContext *) fifo_pop(pFIFO);
    if (pContext == NULL)
      break;
    pNode= network_find_node(pContext->tAddr);
    assert(pNode != NULL);

    // Save context for further references during iteration
    pOldContext= pContext;

    // Traverse all the outbound links of the current node
    for (iIndex= 0; iIndex < ptr_array_length(pNode->pLinks); iIndex++) {
      pLink= (SNetLink *) pNode->pLinks->data[iIndex];

      // Filter unacceptable links. Consider only the links that have
      // the following properties:
      // - NET_LINK_FLAG_IGP_ADV (adjacency can be used)
      // - NET_LINK_FLAG_UP (the link must be UP)
      if (!net_link_get_state(pLink, NET_LINK_FLAG_IGP_ADV) ||
	  !net_link_get_state(pLink, NET_LINK_FLAG_UP))
	continue;

      // Ignore link if its weight is 0 or NET_IGP_MAX_WEIGHT
      tWeight= net_link_get_weight(pLink, 0);
      if ((tWeight == 0) || (tWeight == NET_IGP_MAX_WEIGHT)) {
	//LOG_ERR(LOG_LEVEL_WARNING, "Warning: link weight is 0 or infinity !!!\n");
	continue;
      }

      // Determine next-hop iface to be used (this is an interface of
      // the root node).
      if (pOldContext->pIface != NULL) {
	pIface= pOldContext->pIface;
	tNextHop= pOldContext->tNextHop;
      } else {
	pIface= pLink;
	tNextHop= 0;
      }

      // Compute weight to reach destination through this link
      tNewWeight= net_igp_add_weights(pOldContext->tWeight, tWeight);

      // Retrieve SPT info for destination depending on link type...
      sInfoPrefix= net_link_get_prefix(pLink);
      if (!_spt_info_update(pVisited, sInfoPrefix, pIface, tNextHop, tNewWeight))
	continue;

      // Push current destination onto stack if further exploration
      // is required from here.
      switch (pLink->uType) {
      case NET_LINK_TYPE_ROUTER:
	if (igp_domain_contains_router_by_addr(pDomain,
					       net_link_get_address(pLink))) {
	  pContext= (SSPTContext *) MALLOC(sizeof(SSPTContext));
	  pContext->tAddr= net_link_get_address(pLink);
	  pContext->pIface= pIface;
	  pContext->tNextHop= tNextHop;
	  pContext->tWeight= tNewWeight;
	  assert(fifo_push(pFIFO, pContext) == 0);
	}
	break;
      case NET_LINK_TYPE_STUB:
	break;
      case NET_LINK_TYPE_TRANSIT:

	pLinks= pLink->tDest.pSubnet->pLinks;
	for (uIndex= 0; uIndex < ptr_array_length(pLinks); uIndex++) {
	  SNetLink * pSubLink= (SNetLink *) pLinks->data[uIndex];

	  // Skip if we reached the subnet through this link
	  if (pSubLink == pLink)
	    continue;

	  // Skip if sub-link is not UP
	  if (!net_link_get_state(pSubLink, NET_LINK_FLAG_UP))
	    continue;

	  // We traverse the sub-link in reverse direction (i.e. from
	  // subnet to node) => don't take weight into account

	  sInfoPrefix.tNetwork= pSubLink->pSrcNode->tAddr;
	  sInfoPrefix.uMaskLen= 32;
	  if (igp_domain_contains_router(pDomain, pSubLink->pSrcNode)) {

	    // Update the next-hop if the subnet is a direct link from
	    // the root node. Otherwise, use previously computed
	    // next-hop (from context).
	    if (pIface == pLink) {
	      tNextHop= pSubLink->tIfaceAddr;
	    }

	    if (_spt_info_update(pVisited, sInfoPrefix, pIface, tNextHop, tNewWeight)) {
	      pContext= (SSPTContext *) MALLOC(sizeof(SSPTContext));
	      pContext->tAddr= sInfoPrefix.tNetwork;
	      pContext->pIface= pIface;
	      pContext->tNextHop= tNextHop;
	      pContext->tWeight= tNewWeight;
	      assert(fifo_push(pFIFO, pContext) == 0);
	    }
	  }
	}
	break;
      default:
	abort();
      }
    }
    FREE(pOldContext);
  }
  fifo_destroy(&pFIFO);

  return pVisited;
}

// ----- igp_compute_prefix_for_each --------------------------------
int igp_compute_prefix_for_each(uint32_t uKey, uint8_t uKeyLen,
				void * pItem, void * pContext)
{
  SNetNode * pNode= (SNetNode *) pContext;
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
int igp_compute_rt_for_each(uint32_t uKey, uint8_t uKeyLen,
			    void * pItem, void * pContext)
{
  int iResult;
  SIGPDomain * pDomain= (SIGPDomain *) pContext;
  SNetNode * pNode= (SNetNode *) pItem;
  SRadixTree * pTree;

  /* Remove all IGP routes from node */
  node_rt_del_route(pNode, NULL, NULL, NULL, NET_ROUTE_IGP);

  /* Compute shortest-path tree (SPT) */
  pTree= spt_bfs(network_get(), pNode->tAddr, pDomain);
  if (pTree == NULL)
    return -1;

  /* Add routes corresponding to the SPT */
  iResult= radix_tree_for_each(pTree, igp_compute_prefix_for_each, pNode);

  radix_tree_destroy(&pTree);
  return iResult;
}

// ----- igp_compute_domain -----------------------------------------
/**
 * Compute the shortest-path tree (SPT) and build the routing table
 * for each router within the given domain.
 */
int igp_compute_domain(SIGPDomain * pDomain)
{
  return trie_for_each(pDomain->pRouters,
		       igp_compute_rt_for_each,
		       pDomain);
}
