// ==================================================================
// @(#)dijkstra.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 23/02/2004
// @lastdate 27/01/2005
// ==================================================================

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include <net/dijkstra.h>
#include <net/network.h>

typedef struct {
  net_addr_t tAddr;
  net_addr_t tNextHop;
  net_link_delay_t uIGPweight;
} SContext;

// ----- dijkstra_info_create ---------------------------------------
SDijkstraInfo * dijkstra_info_create(net_link_delay_t uIGPweight)
{
  SDijkstraInfo * pInfo=
    (SDijkstraInfo *) MALLOC(sizeof(SDijkstraInfo));
  pInfo->uIGPweight= uIGPweight;
  pInfo->tNextHop= 0;
  return pInfo;
}

// ----- dijkstra_info_destroy --------------------------------------
void dijkstra_info_destroy(void ** pItem)
{
  SDijkstraInfo * pInfo= *((SDijkstraInfo **) pItem);

  FREE(pInfo);
}

// ----- dijkstra ---------------------------------------------------
/**
 * Compute the Shortest Path Tree from the given source router
 * towards all the other routers that belong to the given prefix.
 */
SRadixTree * dijkstra(SNetwork * pNetwork, net_addr_t tSrcAddr,
		      SPrefix sPrefix)
{
  SNetLink * pLink;
  SNetNode * pNode;
  SFIFO * pFIFO;
  SRadixTree * pVisited;
  SContext * pContext, * pOldContext;
  SDijkstraInfo * pInfo;
  int iIndex;

  pVisited= radix_tree_create(32, dijkstra_info_destroy);
  pFIFO= fifo_create(100000, NULL);
  pContext= (SContext *) MALLOC(sizeof(SContext));
  pContext->tAddr= tSrcAddr;
  pContext->tNextHop= tSrcAddr;
  pContext->uIGPweight= 0;
  fifo_push(pFIFO, pContext);
  radix_tree_add(pVisited, tSrcAddr, 32, dijkstra_info_create(0));

  // Breadth-first search
  while (1) {
    pContext= (SContext *) fifo_pop(pFIFO);
    if (pContext == NULL)
      break;
    pNode= network_find_node(pNetwork, pContext->tAddr);
    assert(pNode != NULL);

    pOldContext= pContext;

    for (iIndex= 0; iIndex < ptr_array_length(pNode->pLinks);
	 iIndex++) {
      pLink= (SNetLink *) pNode->pLinks->data[iIndex];

      /* Consider only the links that have the following properties:
	 - NET_LINK_FLAG_IGP_ADV
	 - NET_LINK_FLAG_UP (the link must be UP)
	 - the end-point belongs to the given prefix */
      if (link_get_state(pLink, NET_LINK_FLAG_IGP_ADV) &&
	  link_get_state(pLink, NET_LINK_FLAG_UP) &&
	  ((!NET_OPTIONS_IGP_INTER && ip_address_in_prefix(pLink->tAddr, sPrefix)) ||
	   (NET_OPTIONS_IGP_INTER && ip_address_in_prefix(pNode->tAddr, sPrefix)))) {

	pInfo=
	  (SDijkstraInfo *) radix_tree_get_exact(pVisited, pLink->tAddr, 32);
	if ((pInfo == NULL) ||
	    (pInfo->uIGPweight > pOldContext->uIGPweight+pLink->uIGPweight)) {
	  pContext= (SContext *) MALLOC(sizeof(SContext));
	  pContext->tAddr= pLink->tAddr;
	  if (pOldContext->tNextHop == tSrcAddr)
	    pContext->tNextHop= pLink->tAddr;
	  else
	    pContext->tNextHop= pOldContext->tNextHop;
	  pContext->uIGPweight= pOldContext->uIGPweight+pLink->uIGPweight;
	  if (pInfo == NULL) {
	    pInfo= dijkstra_info_create(pOldContext->uIGPweight+pLink->uIGPweight);
	    pInfo->tNextHop= pContext->tNextHop;
	    radix_tree_add(pVisited, pLink->tAddr, 32, pInfo);
	  } else {
	    pInfo->uIGPweight= pOldContext->uIGPweight+pLink->uIGPweight;
	    pInfo->tNextHop= pOldContext->tNextHop;
	  }
	  assert(fifo_push(pFIFO, pContext) == 0);
	}
      }
    }
    FREE(pOldContext);
  }
  fifo_destroy(&pFIFO);

  return pVisited;
}
