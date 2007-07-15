// ==================================================================
// @(#)bgp_assert.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @author Sebastien Tandel (standel@info.ucl.ac.be)
// @date 08/03/2004
// @lastdate 22/05/2007
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <libgds/array.h>
#include <libgds/patricia-tree.h>
#include <libgds/types.h>
#include <bgp/as.h>
#include <bgp/bgp_assert.h>
#include <net/network.h>
#include <bgp/peer.h>
#include <net/protocol.h>
#include <bgp/record-route.h>
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
  trie_for_each(pNetwork->pNodes, build_router_list_rtfe, pRL);

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
  SBGPPath * pPath;
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
	  if (bgp_record_route(pRouterSrc, pRoute->sPrefix, &pPath, 0)) {
	    log_printf(pLogOut, "Assert: ");
	    ip_address_dump(pLogOut, pRouterSrc->pNode->tAddr);
	    log_printf(pLogOut, " can not reach ");
	    ip_prefix_dump(pLogOut, pRoute->sPrefix);
	    log_printf(pLogOut, "\n");
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
  //SNetNode * pNode;
  //SNetProtocol * pProtocol;
  SBGPPeer * pPeer;
  int iBadPeerings= 0;

  pRL= build_router_list();

  // For all BGP instances...
  for (iIndex= 0; iIndex < ptr_array_length(pRL); iIndex++) {
    pRouter= (SBGPRouter *) pRL->data[iIndex];

    log_printf(pLogOut, "check router ");
    bgp_router_dump_id(pLogOut, pRouter);
    log_printf(pLogOut, "\n");

    // For all peerings...
    for (iPeerIndex= 0; iPeerIndex < ptr_array_length(pRouter->pPeers);
	 iPeerIndex++) {
      pPeer= (SBGPPeer *) pRouter->pPeers->data[iPeerIndex];

      log_printf(pLogOut, "\tcheck peer ");
      bgp_peer_dump_id(pLogOut, pPeer);
      log_printf(pLogOut, "\n");

      // Check existence of BGP peer
      /*pNode= network_find_node(pPeer->tAddr);
      if (pNode != NULL) {

	// Check support for BGP protocol
	pProtocol= protocols_get(pNode->pProtocols, NET_PROTOCOL_BGP);
	if (pProtocol != NULL) {*/
	  
	  // Check for reachability
	  if (!bgp_peer_session_ok(pPeer)) {
	    log_printf(pLogOut, "Assert: ");
	    ip_address_dump(pLogOut, pRouter->pNode->tAddr);
	    log_printf(pLogOut, "'s peer ");
	    ip_address_dump(pLogOut, pPeer->tAddr);
	    log_printf(pLogOut, " is not reachable\n");
	    iResult= -1;
	    iBadPeerings++;
	  }

	  /*} else {
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
	}*/
      
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

// ----- bgp_router_assert_best -------------------------------------
/**
 * This function checks that the router has a best route towards the
 * given prefix. In addition, the function checks that this route has
 * the given next-hop.
 *
 * Return: 0 on success, -1 on failure
 */
int bgp_router_assert_best(SBGPRouter * pRouter, SPrefix sPrefix,
			   net_addr_t tNextHop)
{
  SRoute * pRoute;

  // Get the best route
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  pRoute= rib_find_one_exact(pRouter->pLocRIB, sPrefix, NULL);
#else
  pRoute= rib_find_exact(pRouter->pLocRIB, sPrefix);
#endif

  // Check that it exists
  if (pRoute == NULL)
    return -1;

  // Check the next-hop
  if (route_get_nexthop(pRoute) != tNextHop)
    return -1;

  return 0;
}

// ----- bgp_router_assert_feasible ---------------------------------
/**
 * This function checks that the router has a route towards the given
 * prefix with the given next-hop.
 *
 * Return: 0 on success, -1 on failure
 */
int bgp_router_assert_feasible(SBGPRouter * pRouter, SPrefix sPrefix,
			       net_addr_t tNextHop)
{
  SRoutes * pRoutes;
  SRoute * pRoute;
  int iIndex;
  int iResult= -1;

  // Get the feasible routes
#ifdef __EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
  pRoutes = bgp_router_get_feasible_routes(pRouter, sPrefix, 0);
#else
  pRoutes= bgp_router_get_feasible_routes(pRouter, sPrefix);
#endif

  // Find a route with the given next-hop
  for (iIndex= 0; iIndex < routes_list_get_num(pRoutes); iIndex++) {
    pRoute= (SRoute *) pRoutes->data[iIndex];

    if (route_get_nexthop(pRoute) == tNextHop) {
      iResult= 0;
      break;
    }
  }

  routes_list_destroy(&pRoutes);

  return iResult;
}
