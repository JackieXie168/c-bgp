// ==================================================================
// @(#)igp.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 23/02/2004
// @lastdate 24/02/2004
// ==================================================================

#include <net/dijkstra.h>
#include <net/igp.h>
#include <net/network.h>
#include <net/routing.h>

// ----- igp_compute_prefix_for_each --------------------------------
int igp_compute_prefix_for_each(uint32_t uKey, uint8_t uKeyLen,
				void * pItem, void * pContext)
{
  SNetNode * pNode= (SNetNode *) pContext;
  SDijkstraInfo * pInfo= (SDijkstraInfo *) pItem;
  SPrefix sPrefix;

  /*
  fprintf(stderr, "node: ");
  ip_address_dump(stderr, pNode->tAddr);
  fprintf(stderr, "\n");
  fprintf(stderr, "prefix: ");
  ip_address_dump(stderr, (net_addr_t) uKey);
  fprintf(stderr, "/%u\n", uKeyLen);
  fprintf(stderr, "next-hop: ");
  ip_address_dump(stderr, pInfo->tNextHop);
  fprintf(stderr, "\n");
  */

  // Skip route to itself
  if (pNode->tAddr == (net_addr_t) uKey)
    return 0;

  // Skip direct route towards a node
  // (next-hop address == destination address)
  /*if (pInfo->tNextHop == (net_addr_t) uKey)
    return 0;*/

  // Add IGP route
  sPrefix.tNetwork= uKey;
  sPrefix.uMaskLen= uKeyLen;
  return node_rt_add_route(pNode, sPrefix, pInfo->tNextHop,
			   pInfo->uIGPweight, NET_ROUTE_IGP);
}

// ----- igp_compute_prefix -----------------------------------------
/**
 *
 */
int igp_compute_prefix(SNetwork * pNetwork, SNetNode * pNode,
		       SPrefix sPrefix)
{
  int iResult;
  SRadixTree * pTree= dijkstra(pNetwork, pNode->tAddr, sPrefix);
  if (pTree == NULL)
    return -1;

  iResult= radix_tree_for_each(pTree, igp_compute_prefix_for_each, pNode);

  radix_tree_destroy(&pTree);
  return iResult;
}
