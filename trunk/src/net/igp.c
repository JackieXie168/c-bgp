// ==================================================================
// @(#)igp.c
//
// Simple model of an intradomain routing protocol (IGP).
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 23/02/2004
// $Id: igp.c,v 1.15 2008-04-11 11:03:06 bqu Exp $
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
  net_iface_t  * iface;
  net_addr_t     next_hop;
  igp_weight_t   weight;
} spt_info_t;

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
  net_node_t   * node;
  net_iface_t  * iface;
  net_addr_t     next_hop;
  igp_weight_t   weight;
} SSPTContext;

// ----- _spt_info_create -------------------------------------------
static inline spt_info_t * _spt_info_create(igp_weight_t weight)
{
  spt_info_t * info=
    (spt_info_t *) MALLOC(sizeof(spt_info_t));
  info->weight= weight;
  info->iface= NULL;
  info->next_hop= 0;
  return info;
}

// ----- _spt_info_destroy ------------------------------------------
static inline void _spt_info_destroy(void ** item)
{
  spt_info_t * info= *((spt_info_t **) item);
  FREE(info);
}

// ----- _spt_info_update -------------------------------------------
/**
 * Create/update SPT info for visited node. Three different cases are
 * possible:
 *   1). no info is known about this node => create
 *   2). info is known
 *     2.1). new_weight < weight => update
 *     2.2). new_weight == weight => update (ECMP)
 *
 * The ECMP case (2.2) is currently not handled due to a limitation
 * in the routing table data structure.
 *
 * If an info node was created or updated (i.e. we were in one of the
 * three above cases), then the function returns TRUE (1), meaning
 * that the link tail end must be pushed on the queue for further
 * exploration...
 */
static inline int _spt_info_update(SRadixTree * visited,
				   ip_pfx_t prefix,
				   net_iface_t * iface,
				   net_addr_t next_hop,
				   igp_weight_t new_weight)
{
  int push_node= 1;
  spt_info_t * info= (spt_info_t *) radix_tree_get_exact(visited,
							 prefix.tNetwork,
							 prefix.uMaskLen);
  if (info == NULL) {

    // ------
    // CASE 1
    // ------

    /*log_printf(pLogErr, "first visit of ");
    ip_prefix_dump(pLogErr, sPrefix);
    log_printf(pLogErr, " (w=%u)\n", tNewWeight);*/

    // Not yet visited: create SPT info
    info= _spt_info_create(new_weight);
    info->iface= iface;
    info->next_hop= next_hop;
    radix_tree_add(visited, prefix.tNetwork, prefix.uMaskLen, info);

  } else if (info->weight > new_weight) {

    // --------
    // CASE 2.1
    // --------

    /*log_printf(pLogErr, "update visit of ");
    ip_prefix_dump(pLogErr, sPrefix);
    log_printf(pLogErr, " (w=%u)\n", tNewWeight);*/

    // Update with new weight, interface & next-hop
    info->weight= new_weight;
    info->iface= iface;
    info->next_hop= next_hop;

  } else if (info->weight == new_weight) {

    // ---------------
    // CASE 2.2 (ECMP)
    // ---------------

    // Add equal-cost path (ECMP)
    /*	fprintf(stdout, "*** \033[32;1mECMP\033[0m: not yet supported ***\n");
      fprintf(stdout, "node: ");
      ip_address_dump(stdout, tSrcAddr);
      fprintf(stdout, ", dst: ");
	net_iface_dump(pLogOut, pLink);
	fprintf(stdout, "\n");*/
    push_node= 0; // do not continue traversal through this node

  } else {

    push_node= 0; // do not continue traversal through this node

  }
  return push_node;
}

// -----[ _push ]----------------------------------------------------
/**
 * Push additional nodes on the stack. These nodes will be processed
 * later.
 */
static inline void _push(SFIFO * pFIFO,
			 igp_domain_t * domain,
			 net_node_t * node,
			 net_iface_t * iface,
			 net_addr_t next_hop,
			 igp_weight_t weight)
{
  SSPTContext * ctx;

  /*log_printf(pLogErr, "__push(");
  ip_address_dump(pLogErr, pNode->tAddr);
  log_printf(pLogErr, ")\n");*/

  if (!igp_domain_contains_router(domain, node)) {
    /*log_printf(pLogErr, "  (node not in domain)\n");*/
    return;
  }

  ctx= (SSPTContext *) MALLOC(sizeof(SSPTContext));
  ctx->node= node;
  ctx->iface= iface;
  ctx->next_hop= next_hop;
  ctx->weight= weight;
  assert(fifo_push(pFIFO, ctx) == 0);
}

// -----[ _is_iface_acceptable ]-------------------------------------
/**
 * Filter unacceptable links. Consider only the links that have the
 * following properties:
 * - link must be enabled (UP)
 * - link must be connected
 * - the IGP weight must be greater than 0
 *   and lower than IGP_MAX_WEIGHT
 */
static inline int _is_iface_acceptable(net_iface_t * iface)
{
  igp_weight_t weight;

  if (!net_iface_is_enabled(iface) ||
      !net_iface_is_connected(iface)) {
    /*log_printf(pLogErr, "link not connected : ");
    net_iface_dump_id(pLogErr, pIface);
    log_printf(pLogErr, "\n");*/
    return 0;
  }

  weight= net_iface_get_metric(iface, 0);
  if ((weight == 0) || (weight == IGP_MAX_WEIGHT)) {
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
static SRadixTree * _spt_bfs(net_node_t * src_node, igp_domain_t * domain)
{
  net_iface_t * pLink;
  net_node_t * pNode;
  SFIFO * pFIFO;
  SRadixTree * visited;
  SSPTContext * pContext;
  ip_pfx_t sInfoPrefix;
  unsigned int index, index2;
  net_iface_t * pIface;   // Root node's outgoing interface along current path
  net_addr_t tNextHop; // Root node's nexthop along current path
  net_ifaces_t * ifaces;
  igp_weight_t weight;
  igp_weight_t new_weight;

  /*log_printf(pLogErr, "** starting from ");
  ip_address_dump(pLogErr, pSrcNode->tAddr);
  log_printf(pLogErr, " **\n");*/

  visited= radix_tree_create(32, _spt_info_destroy);
  pFIFO= fifo_create(100000, NULL);
  _push(pFIFO, domain, src_node, NULL, 0, 0);
  _spt_info_update(visited, net_iface_id_addr(src_node->addr),
		   NULL, 0, 0);

  // -----| Breadth-first search |-----
  while (1) {

    pContext= (SSPTContext *) fifo_pop(pFIFO);
    if (pContext == NULL)
      break;
    pNode= pContext->node;

    // Update loopback interface (interface metric is always 0)
    // Determine the root node's outgoing interface/next-hop
    // to be used for current path
    if (pContext->iface != NULL) {
      pIface= pContext->iface;
      tNextHop= pContext->next_hop;
    } else {
      pIface= NULL; // No interface for loopback
      tNextHop= 0;
    }
    sInfoPrefix= net_iface_id_addr(pNode->addr);
    _spt_info_update(visited, sInfoPrefix, pIface, tNextHop, pContext->weight);
    
    // Traverse all the outbound links of the current node
    for (index= 0; index < net_ifaces_size(pNode->ifaces); index++) {
      pLink= net_ifaces_at(pNode->ifaces, index);

      /*log_printf(pLogErr, "from ");
      ip_address_dump(pLogErr, pNode->addr);
      log_printf(pLogErr, " traverse interface ");
      net_iface_dump_id(pLogErr, pLink);
      log_printf(pLogErr, "  (w=%u)\n", net_iface_get_metric(pLink, 0));*/

      if (!_is_iface_acceptable(pLink))
	continue;

      weight= net_iface_get_metric(pLink, 0);

      // Determine the root node's outgoing interface/next-hop
      // to be used for current path
      if (pContext->iface != NULL) {
	pIface= pContext->iface;
	tNextHop= pContext->next_hop;
      } else {
	pIface= pLink;
	tNextHop= 0;
      }

      // Compute weight to reach destination through this link
      new_weight= net_igp_add_weights(pContext->weight, weight);

      // Retrieve SPT info for destination depending on link type...
      sInfoPrefix= net_iface_dst_prefix(pLink);
      if (!_spt_info_update(visited, sInfoPrefix, pIface, tNextHop, new_weight)) {
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
	_push(pFIFO, domain, pLink->dest.iface->src_node,
	      pIface, tNextHop, new_weight);
	break;

      case NET_IFACE_PTMP:
	if (!subnet_is_transit(pLink->dest.subnet))
	  continue;
	ifaces= pLink->dest.subnet->ifaces;
	for (index2= 0; index2 < ptr_array_length(ifaces); index2++) {
	  net_iface_t * pSubLink= net_ifaces_at(ifaces, index2);

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

	  sInfoPrefix.tNetwork= pSubLink->src_node->addr;
	  sInfoPrefix.uMaskLen= 32;
	  if (igp_domain_contains_router(domain, pSubLink->src_node)) {

	    // Update the next-hop if the subnet is a direct link from
	    // the root node. Otherwise, use previously computed
	    // next-hop (from context).
	    if (pIface == pLink) {
	      tNextHop= pSubLink->tIfaceAddr;
	    }

	    if (_spt_info_update(visited, sInfoPrefix, pIface, tNextHop, new_weight))
	      _push(pFIFO, domain, pSubLink->src_node,
		    pIface, tNextHop, new_weight);
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

  return visited;
}

// ----- _igp_compute_prefix_for_each -------------------------------
static int _igp_compute_prefix_for_each(uint32_t key, uint8_t key_len,
					void * item, void * ctx)
{
  net_node_t * node= (net_node_t *) ctx;
  spt_info_t * info= (spt_info_t *) item;
  ip_pfx_t prefix;
  int result;

  // Skip route to itself
  if ((node->addr == (net_addr_t) key) || (info->iface == NULL))
    return 0;

  //log_printf(pLogErr, "*** node: ");
  //ip_address_dump(pLogErr, pNode->addr);
  //log_printf(pLogErr, ", prefix: ");
  //ip_address_dump(pLogErr, (net_addr_t) uKey);
  //log_printf(pLogErr, "/%u, iface: ", uKeyLen);
  //link_dump(pLogErr, pInfo->pIface);
  //log_printf(pLogErr, " (next-hop: ");
  //ip_address_dump(pLogErr, pInfo->tNextHop);
  //log_printf(pLogErr, ")\n");

  // Add IGP route
  prefix.tNetwork= key;
  prefix.uMaskLen= key_len;

  /* removes the previous route for this node/prefix if it already
     exists */
  node_rt_del_route(node, &prefix, NULL, NULL, NET_ROUTE_IGP);

  result= node_rt_add_route_link(node, prefix,
				 info->iface, info->next_hop,
				 info->weight, NET_ROUTE_IGP);
  /*if (iResult != 0)
    rt_perror(pLogErr, iResult);*/
  return result;
}

// ----- igp_compute_rt_for_each --------------------------------
/**
 *
 */
static int _igp_compute_rt_for_each(uint32_t key, uint8_t key_len,
				    void * item, void * ctx)
{
  int result;
  igp_domain_t * domain= (igp_domain_t *) ctx;
  net_node_t * node= (net_node_t *) item;
  SRadixTree * tree;

  /* Remove all IGP routes from node */
  node_rt_del_route(node, NULL, NULL, NULL, NET_ROUTE_IGP);

  /* Compute shortest-path tree (SPT) */
  tree= _spt_bfs(node, domain);
  if (tree == NULL)
    return -1;

  /* Add routes corresponding to the SPT */
  result= radix_tree_for_each(tree, _igp_compute_prefix_for_each, node);

  radix_tree_destroy(&tree);
  return result;
}

// ----- igp_compute_domain -----------------------------------------
/**
 * Compute the shortest-path tree (SPT) and build the routing table
 * for each router within the given domain.
 *
 * To-do:
 * - use enumerator instead of for-each function.
 */
int igp_compute_domain(igp_domain_t * domain)
{
  return trie_for_each(domain->routers,
		       _igp_compute_rt_for_each,
		       domain);
}
