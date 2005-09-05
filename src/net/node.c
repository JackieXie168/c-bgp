// ==================================================================
// @(#)node.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 08/08/2005
// @lastdate 08/08/2005
// ==================================================================

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <net/link.h>
#include <net/link-list.h>
#include <net/net_types.h>
#include <net/network.h>
#include <net/node.h>
#include <net/subnet.h>

// ----- node_mgmt_perror -------------------------------------------
/**
 *
 */
void node_mgmt_perror(FILE * pStream, int iErrorCode)
{
  switch (iErrorCode) {
  case NET_SUCCESS:
    fprintf(pStream, "success"); break;
  case NET_ERROR_MGMT_INVALID_NODE:
    fprintf(pStream, "invalid node"); break;
  case NET_ERROR_MGMT_INVALID_LINK:
    fprintf(pStream, "invalid link"); break;
  case NET_ERROR_MGMT_INVALID_SUBNET:
    fprintf(pStream, "invalid subnet"); break;
  case NET_ERROR_MGMT_LINK_ALREADY_EXISTS:
    fprintf(pStream, "link already exists"); break;
  case NET_ERROR_MGMT_INVALID_OPERATION:
    fprintf(pStream, "invalid operation"); break;
  default:
    fprintf(pStream, "unknown error (%i)", iErrorCode);
  }
}

// ----- node_add_link_to_router ------------------------------------
/**
 *
 */
int node_add_link_to_router(SNetNode * pNodeA, SNetNode * pNodeB, 
			    net_link_delay_t tDelay, int iMutual)
{
  SNetLink * pLink= create_link_toRouter(pNodeA, pNodeB);
  int iResult;

  if (iMutual) {
    iResult= node_add_link_to_router(pNodeB, pNodeA, tDelay, 0);
    if (iResult)
      return iResult;
  }

  pLink->tDelay= tDelay;
  pLink->uIGPweight= tDelay;
  if (net_links_add(pNodeA->pLinks, pLink) < 0)
    return NET_ERROR_MGMT_LINK_ALREADY_EXISTS;

  return NET_SUCCESS;
}

// ----- node_add_link_to_subnet ------------------------------------
/**
 *
 */
int node_add_link_to_subnet(SNetNode * pNode, SNetSubnet * pSubnet,
			    net_addr_t tIfaceAddr,
			    net_link_delay_t tDelay, int iMutual)
{
  SNetLink * pLink= create_link_toSubnet(pNode, pSubnet, tIfaceAddr);
  pLink->tDelay= tDelay;
  pLink->uIGPweight= tDelay;

  if (net_links_add(pNode->pLinks, pLink) < 0) {
fprintf(stdout, "fallisce net_links_add in node_add_link_to_subnet\n");
    return NET_ERROR_MGMT_LINK_ALREADY_EXISTS;
  }
  else if (iMutual)
    return subnet_add_link(pSubnet, pLink, tIfaceAddr);
  else 
    return NET_SUCCESS;
}

// ----- node_add_link ----------------------------------------------
/**
 * Add a link to a destination. If the destination is an IP address, a
 * point-to-point link is created. If the destination is an IP prefix,
 * a link to a subnet is created.
 */
int node_add_link(SNetNode * pNode, SNetDest sDest,
		  net_link_delay_t tDelay)
{
  SNetNode * pDestNode;
  SNetSubnet * pDestSubnet;

  switch (sDest.tType) {

  case NET_DEST_ADDRESS:
    // Find destination node
    pDestNode= network_find_node(sDest.uDest.tAddr);
    if (pDestNode == NULL)
      return NET_ERROR_MGMT_INVALID_NODE;
    return node_add_link_to_router(pNode, pDestNode, tDelay, 1);

  case NET_DEST_PREFIX:
    // Find destination subnet
    pDestSubnet= network_find_subnet(sDest.uDest.sPrefix);
    if (pDestSubnet == NULL)
      return NET_ERROR_MGMT_INVALID_SUBNET;
    return node_add_link_to_subnet(pNode, pDestSubnet,
				   sDest.uDest.sPrefix.tNetwork,
				   tDelay, 1);

  default:
    return NET_ERROR_MGMT_INVALID_OPERATION;
  }
  return 0;
}

// ----- node_find_link_to_router -----------------------------------
SNetLink * node_find_link_to_router(SNetNode * pNode,
				    net_addr_t tAddr)
{
  unsigned int uIndex;
  SNetLink * pLink= NULL, * pWrapLink;

  pWrapLink = create_link_toRouter_byAddr(pNode, tAddr);
  
  if (ptr_array_sorted_find_index(pNode->pLinks, &pWrapLink, &uIndex) == 0)
    pLink = (SNetLink *) pNode->pLinks->data[uIndex];

  link_destroy(&pWrapLink);
  return pLink;
}

// ----- node_find_link_to_subnet -----------------------------------
SNetLink * node_find_link_to_subnet(SNetNode * pNode,
				    SNetSubnet * pSubnet, net_addr_t tIfaceAddr)
{
  unsigned int uIndex;
  SNetLink * pLink= NULL, * pWrapLink;
  
  pWrapLink= create_link_toSubnet(pNode, pSubnet, tIfaceAddr);
  if (ptr_array_sorted_find_index(pNode->pLinks, &pWrapLink, &uIndex) == 0)
    pLink= (SNetLink *) pNode->pLinks->data[uIndex];
  link_destroy(&pWrapLink);
  
  return pLink;
}

// ----- node_find_link ---------------------------------------------
/**
 *
 */
SNetLink * node_find_link(SNetNode * pNode, SNetDest sDest)
{
  SNetLink * pLink = NULL;
  SNetLink * pWrapLink = NULL;
 

  /*switch (sDest.tType) {
  case NET_DEST_ADDRESS:
    pLink= node_find_link_to_router(pNode, sDest.uDest.tAddr);
    break;
  case NET_DEST_PREFIX:
    pWrapSubnet= subnet_create(sDest.uDest.sPrefix.tNetwork,
					    sDest.uDest.sPrefix.uMaskLen, 0);
    pLink= node_find_link_to_subnet(pNode, pWrapSubnet, tIfaceAddr);
    subnet_destroy(&pWrapSubnet);
    break;
  default:
    return NULL;
  }*/
  unsigned int uIndex;
  pWrapLink= (SNetLink *) MALLOC(sizeof(SNetLink));
  pWrapLink->uDestinationType = NET_LINK_TYPE_TRANSIT;
  if (sDest.tType == NET_DEST_ADDRESS) {
    pWrapLink->tIfaceAddr = sDest.uDest.tAddr;
  } else if (sDest.tType == NET_DEST_PREFIX) {
    pWrapLink->tIfaceAddr = sDest.uDest.sPrefix.tNetwork;
  }
 
  //create_link(pNode, pSubnet, tIfaceAddr);
  if (ptr_array_sorted_find_index(pNode->pLinks, &pWrapLink, &uIndex) == 0)
    pLink= (SNetLink *) pNode->pLinks->data[uIndex];
  link_destroy(&pWrapLink);
  
  return pLink;

//  return pLink;
}


/////////////////////////////////////////////////////////////////////
//
// ROUTING TABLE FUNCTIONS
//
/////////////////////////////////////////////////////////////////////

// ----- node_rt_add_route ------------------------------------------
/**
 * Add a route to the given prefix into the given node. The node must
 * have a direct link towards the given next-hop address. The weight
 * of the route can be specified as it can be different from the
 * outgoing link's weight.
 */
int node_rt_add_route(SNetNode * pNode, SPrefix sPrefix,
		      net_addr_t tNextHopIface, net_addr_t tNextHop,
		      uint32_t uWeight, uint8_t uType)
{
  SNetLink * pIface;

  // Lookup the next-hop's interface (no recursive lookup is allowed,
  // it must be a direct link !)
  pIface= node_links_lookup(pNode, tNextHopIface);
  if (pIface == NULL)
    return NET_RT_ERROR_IF_UNKNOWN;

  return node_rt_add_route_link(pNode, sPrefix, pIface, tNextHop,
				uWeight, uType);
}

// ----- node_rt_add_route_dest -------------------------------------
/**
 * Add a route to the given prefix into the given node. The node must
 * have a direct link towards the given next-hop address. The weight
 * of the route can be specified as it can be different from the
 * outgoing link's weight.
 */
int node_rt_add_route_dest(SNetNode * pNode, SPrefix sPrefix,
			   SNetDest sIfaceDest, net_addr_t tNextHop,
			   uint32_t uWeight, uint8_t uType)
{
  SNetLink * pIface;

  // Lookup the outgoing interface (no recursive lookup is allowed,
  // it must be a direct link !)
  pIface= node_find_link(pNode, sIfaceDest);

  if (pIface == NULL)
    return NET_RT_ERROR_IF_UNKNOWN;

  return node_rt_add_route_link(pNode, sPrefix, pIface, tNextHop,
				uWeight, uType);
}

// ----- node_rt_add_route_link -------------------------------------
/**
 * Add a route to the given prefix into the given node. The outgoing
 * interface is a link. The weight of the route can be specified as it
 * can be different from the outgoing link's weight.
 *
 * Pre: the outgoing link (next-hop interface) must exist in the node.
 */
int node_rt_add_route_link(SNetNode * pNode, SPrefix sPrefix,
			   SNetLink * pIface, net_addr_t tNextHop,
			   uint32_t uWeight, uint8_t uType)
{
  SNetRouteInfo * pRouteInfo;

  // If a next-hop has been specified, check that it is reachable
  // through the given interface.
  if (tNextHop != 0) {
    //fprintf(stdout, "node-rt-debug: next-hop [");
    //ip_address_dump(stdout, tNextHop);
    //fprintf(stdout, ";");
    //link_dst_dump(stdout, pIface);
    //fprintf(stdout, "] must be checked...\n");
  }

  // Build route info
  pRouteInfo= net_route_info_create(sPrefix, pIface, tNextHop,
				    uWeight, uType);

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
		      SNetDest * psIface, net_addr_t * ptNextHop,
		      uint8_t uType)
{
  SNetLink * pIface= NULL;

  // Lookup the outgoing interface (no recursive lookup is allowed,
  // it must be a direct link !)
  if (psIface != NULL)
    pIface= node_find_link(pNode, *psIface);

  return rt_del_route(pNode->pRT, pPrefix, pIface, ptNextHop, uType);
}

