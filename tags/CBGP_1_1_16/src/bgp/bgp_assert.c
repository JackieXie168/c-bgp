// ==================================================================
// @(#)bgp_assert.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 08/03/2004
// @lastdate 09/04/2004
// ==================================================================

#include <libgds/array.h>
#include <libgds/radix-tree.h>
#include <libgds/types.h>
#include <bgp/as.h>
#include <bgp/bgp_assert.h>
#include <net/network.h>
#include <bgp/peer.h>
#include <net/protocol.h>
#include <bgp/route_t.h>

int build_router_list_rtfe(uint32_t uKey, uint8_t uKeyLen,
			   void * pItem, void * pContext)
{
  SPtrArray * pRL= (SPtrArray *) pContext;
  SNetNode * pNode= (SNetNode *) pItem;
  SNetProtocol * pProtocol;

  pProtocol= protocols_get(pNode->pProtocols, NET_PROTOCOL_BGP);
  if (pProtocol != NULL)
    ptr_array_append(pRL, pProtocol->pHandler);

  return 0;
}

// ----- build_router_list ------------------------------------------
static SPtrArray * build_router_list()
{
  SPtrArray * pRL= ptr_array_create_ref(0);
  SNetwork * pNetwork= network_get();

  // Build list of BGP routers
  radix_tree_for_each(pNetwork->pNodes, build_router_list_rtfe, pRL);

  return pRL;
}

// ----- bgp_assert_reachability ------------------------------------
/**
 * This function checks that all the advertised networks are reachable
 * from all the BGP routers.
 */
int bgp_assert_reachability()
{
  SPtrArray * pRL;
  int iIndexSrc, iIndexDst, iIndexNet;
  SBGPRouter * pRouterSrc, * pRouterDst;
  SPath * pPath;
  SRoute * pRoute;
  int iResult= 0;

  pRL= build_router_list();

  // All routers as source...
  for (iIndexSrc= 0; iIndexSrc < ptr_array_length(pRL); iIndexSrc++) {
    pRouterSrc= (SBGPRouter *) pRL->data[iIndexSrc];

    // All routers as destination...
    for (iIndexDst= 0; iIndexDst < ptr_array_length(pRL); iIndexDst++) {
      pRouterDst= (SBGPRouter *) pRL->data[iIndexDst];
      
      if (pRouterSrc != pRouterDst) {

	// For all advertised networks in the destination router
	for (iIndexNet= 0;
	     iIndexNet < ptr_array_length(pRouterDst->pLocalNetworks);
	     iIndexNet++) {
	  pRoute= (SRoute *) pRouterDst->pLocalNetworks->data[iIndexNet];

	  // Check BGP reachability
	  if (bgp_router_record_route(pRouterSrc, pRoute->sPrefix,
				      &pPath, 0)) {
	    fprintf(stdout, "Assert: ");
	    ip_address_dump(stdout, pRouterSrc->pNode->tAddr);
	    fprintf(stdout, " can not reach ");
	    ip_prefix_dump(stdout, pRoute->sPrefix);
	    fprintf(stdout, "\n");
	    iResult= -1;
	  }
	  path_destroy(&pPath);

	}
	
      }
    }
  }

  ptr_array_destroy(&pRL);

  return iResult;
}

// ----- bgp_assert_peerings ----------------------------------------
/**
 * This function checks that all defined peerings are valid, i.e. the
 * peers exist and are reachable.
 */
int bgp_assert_peerings()
{
  int iIndex, iPeerIndex;
  SPtrArray * pRL;
  SBGPRouter * pRouter;
  int iResult= 0;
  SNetNode * pNode;
  SNetProtocol * pProtocol;
  SPeer * pPeer;
  int iBadPeerings= 0;

  pRL= build_router_list();

  // For all BGP instances...
  for (iIndex= 0; iIndex < ptr_array_length(pRL); iIndex++) {
    pRouter= (SBGPRouter *) pRL->data[iIndex];

    // For all peerings...
    for (iPeerIndex= 0; iPeerIndex < ptr_array_length(pRouter->pPeers);
	 iPeerIndex++) {
      pPeer= (SPeer *) pRouter->pPeers->data[iPeerIndex];

      // Check existence of BGP peer
      pNode= network_find_node(network_get(), pPeer->tAddr);
      if (pNode != NULL) {

	// Check support for BGP protocol
	pProtocol= protocols_get(pNode->pProtocols, NET_PROTOCOL_BGP);
	if (pProtocol != NULL) {
	  
	  // Check for reachability
	  if (node_rt_lookup(pRouter->pNode, pPeer->tAddr) == NULL) {
	    fprintf(stdout, "Assert: ");
	    ip_address_dump(stdout, pRouter->pNode->tAddr);
	    fprintf(stdout, "'s peer ");
	    ip_address_dump(stdout, pPeer->tAddr);
	    fprintf(stdout, " is not reachable\n");
	    iResult= -1;
	    iBadPeerings++;
	  }

	} else {
	  fprintf(stdout, "Assert: ");
	  ip_address_dump(stdout, pRouter->pNode->tAddr);
	  fprintf(stdout, "'s peer ");
	  ip_address_dump(stdout, pPeer->tAddr);
	  fprintf(stdout, " does not support BGP\n");
	  iResult= -1;
	  iBadPeerings++;
	}

      } else {
	fprintf(stdout, "Assert: ");
	ip_address_dump(stdout, pRouter->pNode->tAddr);
	fprintf(stdout, "'s peer ");
	ip_address_dump(stdout, pPeer->tAddr);
	fprintf(stdout, " does not exist\n");
	iResult= -1;
	iBadPeerings++;
      }
      
    }
  }

  ptr_array_destroy(&pRL);

  return iResult;
}

// ----- bgp_assert_full_mesh ---------------------------------------
int bgp_assert_full_mesh(uint16_t uAS)
{
  int iResult= 0;

  return iResult;
}

// ----- bgp_assert_sessions ----------------------------------------
int bgp_assert_sessions()
{
  int iResult= 0;

  return iResult;
}
