// ==================================================================
// @(#)ospf.c
//
// @author Stefano Iasi (stefanoia@tin.it)
// @date 14/06/2005
// @lastdate 14/05/2005
// ==================================================================

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <net/net_types.h>
#include <net/ospf.h>
#include <net/ospf_rt.h>
#include <net/network.h>
#include <net/subnet.h>
#include <net/link.h>
#include <net/prefix.h>
#include <net/spt_vertex.h>
#include <net/igp_domain.h>
#include <string.h>

#define X_AREA 1
#define Y_AREA 2
#define K_AREA 3


/////////////////////////////////////////////////////////////////////
/////// OSPF methods for node object
/////////////////////////////////////////////////////////////////////
// ----- node_add_OSPFArea ------------------------------------------
int node_add_OSPFArea(SNetNode * pNode, uint32_t OSPFArea)
{
  return int_array_add(pNode->pOSPFAreas, &OSPFArea);
}

// ----- node_belongs_to_area ------------------------------------------
int node_belongs_to_area(SNetNode * pNode, uint32_t tArea)
{
  int iIndex;
  
  return !_array_sorted_find_index((SArray *)(pNode->pOSPFAreas), &tArea, &iIndex);
}

// ----- node_is_BorderRouter ------------------------------------------
int node_is_BorderRouter(SNetNode * pNode)
{
  return (int_array_length(pNode->pOSPFAreas) > 1);
}

// ----- node_is_InternalRouter ------------------------------------------
int node_is_InternalRouter(SNetNode * pNode) 
{
  return (int_array_length(pNode->pOSPFAreas) == 1);
}

// ----- node_ospf_rt_add_route ------------------------------------------
extern int node_ospf_rt_add_route(SNetNode     * pNode,     ospf_dest_type_t  tOSPFDestinationType,
                       SPrefix        sPrefix,   uint32_t          uWeight,
		       ospf_area_t    tOSPFArea, ospf_path_type_t  tOSPFPathType,
		       next_hops_list_t * pNHList)
{
  SOSPFRouteInfo * pRouteInfo;
  //we should be do it?
  /*pLink= node_links_lookup(pNode, tNextHop);
  if (pLink == NULL) {
    return NET_RT_ERROR_NH_UNREACH;
  }*/

  pRouteInfo = OSPF_route_info_create(tOSPFDestinationType,  sPrefix,
				       uWeight, tOSPFArea, tOSPFPathType, pNHList);
//  LOG_DEBUG("OSPF_route_info_create pass...\n");  
  return OSPF_rt_add_route(pNode->pOspfRT, sPrefix, pRouteInfo);
 return 0;
}


/////////////////////////////////////////////////////////////////////
/////// OSPF methods for subnet object
/////////////////////////////////////////////////////////////////////
// ----- subnet_OSPFArea -------------------------------------------------
void subnet_set_OSPFArea(SNetSubnet * pSubnet, uint32_t uOSPFArea)
{
  pSubnet->uOSPFArea = uOSPFArea;
}

// ----- subnet_getOSPFArea ----------------------------------------------
uint32_t subnet_get_OSPFArea(SNetSubnet * pSubnet)
{
  return pSubnet->uOSPFArea;
}

// ----- subnet_belongs_to_area ------------------------------------------
int subnet_belongs_to_area(SNetSubnet * pSubnet, uint32_t tArea)
{
  return pSubnet->uOSPFArea == tArea;
}

/////////////////////////////////////////////////////////////////////
/////// routing table computation from spt (intra route)
/////////////////////////////////////////////////////////////////////

typedef struct {
  SNetNode    * pNode;
  ospf_area_t   tArea;
} SOspfIntraRouteContext;

// ----- ospf_build_route_for_each --------------------------------
int ospf_intra_route_for_each(uint32_t uKey, uint8_t uKeyLen,
				void * pItem, void * pContext)
{
  SOspfIntraRouteContext * pMyContext= (SOspfIntraRouteContext *) pContext;
  SSptVertex * pVertex= (SSptVertex *) pItem;
  SPrefix sPrefix;
  next_hops_list_t * pNHList;
  ospf_dest_type_t   tDestType;
  int iIndex;
// LOG_DEBUG("ospf_build_route_for_each\n"); 
  
 
  /* removes the previous route for this node/prefix if it already
     exists */     
   // TODO  node_ospf_rt_del_route(pNode, &sPrefix, NULL, NET_ROUTE_IGP);
   
   //adding route towards subnet linked to vertex
   int iTotSubnet = ptr_array_length(pVertex->aSubnets);
   if (iTotSubnet > 0){
     SNetLink * pLinkToSub;
     next_hops_list_t * pNHListToSubnet;
     for (iIndex = 0; iIndex < iTotSubnet; iIndex++){
       ptr_array_get_at(pVertex->aSubnets, iIndex, &pLinkToSub); 
       link_get_prefix(pLinkToSub, &sPrefix); 
//        next_hops_list_t * pRootNHList = ospf_nh_list_create();ù
       //must duplicate next hop to define a separate route correctly
       pNHListToSubnet = ospf_nh_list_copy(pVertex->pNextHops);
//        ospf_nh_list_add(pNHListToSubnet, pNHtoSubnet);
       node_ospf_rt_add_route(pMyContext->pNode,  OSPF_DESTINATION_TYPE_NETWORK, 
                                sPrefix,
				pVertex->uIGPweight + pLinkToSub->uIGPweight,
				pMyContext->tArea, 
				OSPF_PATH_TYPE_INTRA,
				pNHListToSubnet);
     }
   }
   
   // Skip route to itself
   if (pMyContext->pNode->tAddr == (net_addr_t) uKey)
    return 0;
    
   //adding OSPF route towards vertex
   sPrefix.tNetwork= uKey;
   sPrefix.uMaskLen= uKeyLen;

   tDestType = OSPF_DESTINATION_TYPE_NETWORK;
   if (spt_vertex_is_router(pVertex))
     if (node_is_BorderRouter(spt_vertex_to_router(pVertex))){
//        LOG_DEBUG("Trovato Border Router\n");
       tDestType = OSPF_DESTINATION_TYPE_ROUTER;
     }
   
  //to prevent NextHopList destroy when spt is destroyed
  //note: next hops list are dynamically allocated during spt computation
  //      and linked to routing table during rt computation
  pNHList = pVertex->pNextHops;
  pVertex->pNextHops = NULL;
  
  return node_ospf_rt_add_route(pMyContext->pNode, tDestType, 
                                sPrefix,
				pVertex->uIGPweight,
				pMyContext->tArea, 
				OSPF_PATH_TYPE_INTRA,
				pNHList);
				
  
}

// ----- igp_compute_prefix -----------------------------------------
/**
 *
 */
int node_ospf_intra_route_single_area(SNetNode * pNode, uint16_t uIGPDomainNumber, ospf_area_t tArea)
{
  int iResult;
  SRadixTree * pTree;
  SOspfIntraRouteContext * pContext;
  
  if (!node_belongs_to_area(pNode, tArea))
    return -1; //TODO define opportune error code
// LOG_DEBUG("Enter ospf_node_build_intra_route()\n");
  /* Remove all OSPF routes from node */
  //TODO   node_rt_del_route(pNode, NULL, NULL, NET_ROUTE_IGP);
  
  /* Compute Minimum Spanning Tree */
  pTree= node_ospf_compute_spt(pNode, uIGPDomainNumber, tArea);
  if (pTree == NULL)
    return -1;
// LOG_DEBUG("start visit of spt\n");
  /* Visit spt and set route in routing table */
  pContext = (SOspfIntraRouteContext *) MALLOC(sizeof(SOspfIntraRouteContext));
  pContext->pNode = pNode;
  pContext->tArea = tArea;
  iResult= radix_tree_for_each(pTree, ospf_intra_route_for_each, pContext);
  radix_tree_destroy(&pTree);
  
  return iResult;
}

// ----- igp_compute_prefix ------------------------------------------------------------------
int node_ospf_intra_route(SNetNode * pNode, uint16_t uIGPDomainsNumber)
{
  int iIndex, iStatus = 0;
  ospf_area_t tCurrentArea;
  for (iIndex = 0; iIndex < _array_length((SArray *)pNode->pOSPFAreas); iIndex++){
     _array_get_at((SArray *)(pNode->pOSPFAreas), iIndex, &tCurrentArea);
     iStatus = node_ospf_intra_route_single_area(pNode, uIGPDomainsNumber, tCurrentArea );
     if (iStatus < 0) 
       break;
  } 
  return iStatus;
}

// ----- ospf_node_rt_dump ------------------------------------------------------------------
/**  Option:
  *  NET_OSPF_RT_OPTION_SORT_AREA : dump routing table gouping route by area
  */
void ospf_node_rt_dump(FILE * pStream, SNetNode * pNode, int iOption)
{
  int iIndex;
  ospf_area_t tCurrentArea;
  for( iIndex = 0; iIndex < _array_length((SArray *)(pNode->pOSPFAreas)); iIndex++){
    _array_get_at((SArray *)(pNode->pOSPFAreas), iIndex, &tCurrentArea);
    OSPF_rt_dump(stdout, pNode->pOspfRT, iOption, tCurrentArea);
    fprintf(stdout, "-------------------------------------------------------------------------------------\n");
  }
}


  
int ospf_test_rfc2328()
{
  LOG_DEBUG("ospf_test_rfc2328(): START\n");
  LOG_DEBUG("ospf_test_rfc2328(): building the sample network of RFC2328 for test...");
  const int startAddr = 1024;
//   LOG_DEBUG("point-to-point links attached.\n");
  SNetwork * pNetworkRFC2328= network_create();
  SNetNode * pNodeRT1= node_create(startAddr);
  SNetNode * pNodeRT2= node_create(startAddr + 1);
  SNetNode * pNodeRT3= node_create(startAddr + 2);
  SNetNode * pNodeRT4= node_create(startAddr + 3);
  SNetNode * pNodeRT5= node_create(startAddr + 4);
  SNetNode * pNodeRT6= node_create(startAddr + 5);
  SNetNode * pNodeRT7= node_create(startAddr + 6);
  SNetNode * pNodeRT8= node_create(startAddr + 7);
  SNetNode * pNodeRT9= node_create(startAddr + 8);
  SNetNode * pNodeRT10= node_create(startAddr + 9);
  SNetNode * pNodeRT11= node_create(startAddr + 10);
  SNetNode * pNodeRT12= node_create(startAddr + 11);
  
  SNetSubnet * pSubnetSN1=  subnet_create( 4, 30, NET_SUBNET_TYPE_STUB);
  SNetSubnet * pSubnetSN2=  subnet_create( 8, 30, NET_SUBNET_TYPE_STUB);
  SNetSubnet * pSubnetTN3=  subnet_create(12, 30, NET_SUBNET_TYPE_TRANSIT);
  SNetSubnet * pSubnetSN4=  subnet_create(16, 30, NET_SUBNET_TYPE_STUB);
  SNetSubnet * pSubnetTN6=  subnet_create(20, 30, NET_SUBNET_TYPE_TRANSIT);
  SNetSubnet * pSubnetSN7=  subnet_create(24, 30, NET_SUBNET_TYPE_STUB);
  SNetSubnet * pSubnetTN8=  subnet_create(28, 30, NET_SUBNET_TYPE_TRANSIT);
  SNetSubnet * pSubnetTN9=  subnet_create(32, 30, NET_SUBNET_TYPE_TRANSIT);
  SNetSubnet * pSubnetSN10= subnet_create(36, 30, NET_SUBNET_TYPE_STUB);
  SNetSubnet * pSubnetSN11= subnet_create(40, 30, NET_SUBNET_TYPE_STUB);
  
  assert(!network_add_node(pNetworkRFC2328, pNodeRT1));
  assert(!network_add_node(pNetworkRFC2328, pNodeRT2));
  assert(!network_add_node(pNetworkRFC2328, pNodeRT3));
  assert(!network_add_node(pNetworkRFC2328, pNodeRT4));
  assert(!network_add_node(pNetworkRFC2328, pNodeRT5));
  assert(!network_add_node(pNetworkRFC2328, pNodeRT6));
  assert(!network_add_node(pNetworkRFC2328, pNodeRT7));
  assert(!network_add_node(pNetworkRFC2328, pNodeRT8));
  assert(!network_add_node(pNetworkRFC2328, pNodeRT9));
  assert(!network_add_node(pNetworkRFC2328, pNodeRT10));
  assert(!network_add_node(pNetworkRFC2328, pNodeRT11));
  assert(!network_add_node(pNetworkRFC2328, pNodeRT12));

  assert(node_add_link_toSubnet(pNodeRT1, pSubnetSN1, 3, 1) >= 0);
  assert(node_add_link_toSubnet(pNodeRT1, pSubnetTN3, 1, 1) >= 0);
  assert(node_add_link_toSubnet(pNodeRT2, pSubnetSN2, 3, 1) >= 0);
  assert(node_add_link_toSubnet(pNodeRT2, pSubnetTN3, 1, 1) >= 0);
  
  assert(node_add_link_toSubnet(pNodeRT3, pSubnetSN4, 2, 1) >= 0);
  assert(node_add_link_toSubnet(pNodeRT3, pSubnetTN3, 1, 1) >= 0);
  assert(node_add_link(pNodeRT3, pNodeRT6, 8, 0) >= 0);
  assert(node_add_link(pNodeRT6, pNodeRT3, 6, 0) >= 0);
  assert(node_add_link_toSubnet(pNodeRT4, pSubnetTN3, 1, 1) >= 0);
 
  assert(node_add_link(pNodeRT4, pNodeRT5, 8, 1) >= 0);
  assert(node_add_link(pNodeRT5, pNodeRT6, 7, 0) >= 0);
  assert(node_add_link(pNodeRT6, pNodeRT5, 6, 0) >= 0);
  
  assert(node_add_link(pNodeRT5, pNodeRT7, 6, 1) >= 0);
  assert(node_add_link(pNodeRT6, pNodeRT10, 7, 0) >= 0);
  assert(node_add_link(pNodeRT10, pNodeRT6, 5, 0) >= 0);
  assert(node_add_link_toSubnet(pNodeRT7, pSubnetTN6, 1, 1) >= 0);

  assert(node_add_link_toSubnet(pNodeRT8, pSubnetTN6, 1, 1) >= 0);
  assert(node_add_link_toSubnet(pNodeRT8, pSubnetSN7, 4, 1) >= 0);
  assert(node_add_link_toSubnet(pNodeRT10, pSubnetTN6, 1, 1) >= 0);
  assert(node_add_link_toSubnet(pNodeRT10, pSubnetTN8, 3, 1) >= 0);
  assert(node_add_link_toSubnet(pNodeRT11, pSubnetTN8, 2, 1) >= 0);
  assert(node_add_link_toSubnet(pNodeRT11, pSubnetTN9, 1, 1) >= 0);
  assert(node_add_link_toSubnet(pNodeRT9, pSubnetSN11, 3, 1) >= 0);
  assert(node_add_link_toSubnet(pNodeRT9, pSubnetTN9, 1, 1) >= 0);
  assert(node_add_link_toSubnet(pNodeRT12, pSubnetTN9, 1, 1) >= 0);
  assert(node_add_link_toSubnet(pNodeRT12, pSubnetSN10, 2, 1) >= 0);
  
  LOG_DEBUG(" done.\n");
  
  LOG_DEBUG("ospf_test_rfc2328(): Adding other nodes to test igp domain check...");
  SNetNode * pNodeRTE1= node_create(startAddr + 30);
  SNetNode * pNodeRTE2= node_create(startAddr + 31);
  SNetNode * pNodeRTE3= node_create(startAddr + 32);
  SNetNode * pNodeRTE4= node_create(startAddr + 33);
  
  assert(!network_add_node(pNetworkRFC2328, pNodeRTE1));
  assert(!network_add_node(pNetworkRFC2328, pNodeRTE2));
  assert(!network_add_node(pNetworkRFC2328, pNodeRTE3));
  assert(!network_add_node(pNetworkRFC2328, pNodeRTE4));
  
  assert(node_add_link_toSubnet(pNodeRTE1, pSubnetTN3, 1, 1) >= 0);
  assert(node_add_link(pNodeRTE2, pNodeRT4, 1, 1) >= 0);
  assert(node_add_link(pNodeRTE3, pNodeRT3, 1, 1) >= 0);
  assert(node_add_link(pNodeRTE4, pNodeRT6, 1, 1) >= 0);
  LOG_DEBUG(" done.\n");
  
  LOG_DEBUG("ospf_test_rfc2328(): Assigning areas...");
  assert(node_add_OSPFArea(pNodeRT1, 1) >= 0);
  assert(node_add_OSPFArea(pNodeRT2, 1) >= 0);
  assert(node_add_OSPFArea(pNodeRT3, 1) >= 0);
  assert(node_add_OSPFArea(pNodeRT4, 1) >= 0);
  
  assert(node_add_OSPFArea(pNodeRT3,  BACKBONE_AREA) >= 0);
  assert(node_add_OSPFArea(pNodeRT4,  BACKBONE_AREA) >= 0);
  assert(node_add_OSPFArea(pNodeRT5,  BACKBONE_AREA) >= 0);
  assert(node_add_OSPFArea(pNodeRT6,  BACKBONE_AREA) >= 0);
  assert(node_add_OSPFArea(pNodeRT7,  BACKBONE_AREA) >= 0);
  assert(node_add_OSPFArea(pNodeRT10, BACKBONE_AREA) >= 0);
  
  assert(node_add_OSPFArea(pNodeRT7,  2) >= 0);
  assert(node_add_OSPFArea(pNodeRT10, 2) >= 0);
  assert(node_add_OSPFArea(pNodeRT11, 2) >= 0);
  
  assert(node_add_OSPFArea(pNodeRT9,  3) >= 0);
  assert(node_add_OSPFArea(pNodeRT11, 3) >= 0);
  assert(node_add_OSPFArea(pNodeRT12, 3) >= 0);
  
  subnet_set_OSPFArea(pSubnetSN1, 1);
  subnet_set_OSPFArea(pSubnetSN2, 1);
  subnet_set_OSPFArea(pSubnetTN3, 1);
  subnet_set_OSPFArea(pSubnetSN4, 1);
  
  subnet_set_OSPFArea(pSubnetTN6, 2);
  subnet_set_OSPFArea(pSubnetSN7, 2);
  subnet_set_OSPFArea(pSubnetTN8, 2);
  
  subnet_set_OSPFArea(pSubnetTN9, 3);
  subnet_set_OSPFArea(pSubnetSN10, 3);
  subnet_set_OSPFArea(pSubnetSN11, 3);
  LOG_DEBUG(" done.\n");
  
  LOG_DEBUG("ospf_test_rfc2328(): Creating igp domain...");
  uint16_t uIGPDomain = 10;
  SIGPDomain * pIGPDomain = get_igp_domain(uIGPDomain); //creating and registering domain
  igp_domain_add_router(pIGPDomain, pNodeRT1);
  igp_domain_add_router(pIGPDomain, pNodeRT2);
  igp_domain_add_router(pIGPDomain, pNodeRT3);
  igp_domain_add_router(pIGPDomain, pNodeRT4);
  igp_domain_add_router(pIGPDomain, pNodeRT5);
  igp_domain_add_router(pIGPDomain, pNodeRT6);
  igp_domain_add_router(pIGPDomain, pNodeRT7);
  igp_domain_add_router(pIGPDomain, pNodeRT9);
  igp_domain_add_router(pIGPDomain, pNodeRT10);
  igp_domain_add_router(pIGPDomain, pNodeRT11);
  igp_domain_add_router(pIGPDomain, pNodeRT12);
  LOG_DEBUG(" done.\n");
  
  LOG_DEBUG("ospf_test_rfc2328: Computing SPT for RT4 (.dot output in spt.dot)...");
  
  SRadixTree * pSpt = node_ospf_compute_spt(pNodeRT4, uIGPDomain, BACKBONE_AREA);
  FILE * pOutDump = fopen("spt.RFC2328.backbone.dot", "w");
  spt_dump_dot(pOutDump, pSpt, pNodeRT4->tAddr);
  fclose(pOutDump);
  radix_tree_destroy(&pSpt);
  
  pSpt = node_ospf_compute_spt(pNodeRT4, uIGPDomain, 1);
  pOutDump = fopen("spt.RFC2328.1.dot", "w");
  spt_dump_dot(pOutDump, pSpt, pNodeRT4->tAddr);
  fclose(pOutDump);
  radix_tree_destroy(&pSpt);
  
  LOG_DEBUG(" ok!\n");
  
  LOG_DEBUG("ospf_test_rfc2328: Computing Routing Table for RT3-BACKBONE...");
  node_ospf_intra_route_single_area(pNodeRT3, uIGPDomain, BACKBONE_AREA);
  fprintf(stdout, "\n");
  OSPF_rt_dump(stdout, pNodeRT3->pOspfRT, 0, 0);
  LOG_DEBUG(" ok!\n");
  
  OSPF_rt_destroy(&(pNodeRT3->pOspfRT));
  pNodeRT3->pOspfRT = OSPF_rt_create();
  LOG_DEBUG("ospf_test_rfc2328: Computing Routing Table for RT3-AREA 1...");
  node_ospf_intra_route_single_area(pNodeRT3, uIGPDomain, 1);
  fprintf(stdout, "\n");
  OSPF_rt_dump(stdout, pNodeRT3->pOspfRT, 0, 0);
  LOG_DEBUG(" ok!\n");

  LOG_DEBUG("ospf_test_rfc2328: Computing Routing Table for RT6-AREA 1 (should fail)...");
  assert(node_ospf_intra_route_single_area(pNodeRT6, uIGPDomain, 1) < 0);
  LOG_DEBUG(" ok!\n");
    
  LOG_DEBUG("ospf_test_rfc2328: Try computing intra route for each area on RT4...\n");
  fprintf(stdout, "\n");
  node_ospf_intra_route(pNodeRT4, uIGPDomain);
  
  ospf_node_rt_dump(stdout, pNodeRT4, NET_OSPF_RT_OPTION_SORT_AREA);
  LOG_DEBUG(" ok!\n");
  
  LOG_DEBUG("ospf_test_rfc2328: STOP\n");
  return 1;
}


int ospf_test_sample_net()
{
  LOG_DEBUG("ospf_djk_test(): START\n");
  LOG_DEBUG("ospf_djk_test(): building network for test...");
  const int startAddr = 1024;
//   LOG_DEBUG("point-to-point links attached.\n");
  SNetwork * pNetwork= network_create();
  SNetNode * pNodeB1= node_create(startAddr);
  SNetNode * pNodeB2= node_create(startAddr + 1);
  SNetNode * pNodeB3= node_create(startAddr + 2);
  SNetNode * pNodeB4= node_create(startAddr + 3);
  SNetNode * pNodeX1= node_create(startAddr + 4);
  SNetNode * pNodeX2= node_create(startAddr + 5);
  SNetNode * pNodeX3= node_create(startAddr + 6);
  SNetNode * pNodeY1= node_create(startAddr + 7);
  SNetNode * pNodeY2= node_create(startAddr + 8);
  SNetNode * pNodeK1= node_create(startAddr + 9);
  SNetNode * pNodeK2= node_create(startAddr + 10);
  SNetNode * pNodeK3= node_create(startAddr + 11);
  
  SNetSubnet * pSubnetTB1= subnet_create(4, 30, NET_SUBNET_TYPE_TRANSIT);
  SNetSubnet * pSubnetTX1= subnet_create(8, 30, NET_SUBNET_TYPE_TRANSIT);
  SNetSubnet * pSubnetTY1= subnet_create(12, 30, NET_SUBNET_TYPE_TRANSIT);
  SNetSubnet * pSubnetTK1= subnet_create(16, 30, NET_SUBNET_TYPE_TRANSIT);
  
//   SNetSubnet * pSubnetSB1= subnet_create(20, 30, NET_SUBNET_TYPE_STUB);
//   SNetSubnet * pSubnetSB2= subnet_create(24, 30, NET_SUBNET_TYPE_STUB);
//   SNetSubnet * pSubnetSX1= subnet_create(28, 30, NET_SUBNET_TYPE_STUB);
//   SNetSubnet * pSubnetSX2= subnet_create(32, 30, NET_SUBNET_TYPE_STUB);
// //   SNetSubnet * pSubnetSX3= subnet_create(36, 30, NET_SUBNET_TYPE_STUB);
//   SNetSubnet * pSubnetSY1= subnet_create(40, 30, NET_SUBNET_TYPE_STUB);
// //   SNetSubnet * pSubnetSY2= subnet_create(44, 30, NET_SUBNET_TYPE_STUB);
//   SNetSubnet * pSubnetSY3= subnet_create(48, 30, NET_SUBNET_TYPE_STUB);
//   SNetSubnet * pSubnetSK1= subnet_create(52, 30, NET_SUBNET_TYPE_STUB);
//   SNetSubnet * pSubnetSK2= subnet_create(56, 30, NET_SUBNET_TYPE_STUB);
//   SNetSubnet * pSubnetSK3= subnet_create(60, 30, NET_SUBNET_TYPE_STUB);
  
  assert(!network_add_node(pNetwork, pNodeB1));
  assert(!network_add_node(pNetwork, pNodeB2));
  assert(!network_add_node(pNetwork, pNodeB3));
  assert(!network_add_node(pNetwork, pNodeB4));
  assert(!network_add_node(pNetwork, pNodeX1));
  assert(!network_add_node(pNetwork, pNodeX2));
  assert(!network_add_node(pNetwork, pNodeX3));
  assert(!network_add_node(pNetwork, pNodeY1));
  assert(!network_add_node(pNetwork, pNodeY2));
  assert(!network_add_node(pNetwork, pNodeK1));
  assert(!network_add_node(pNetwork, pNodeK2));
  assert(!network_add_node(pNetwork, pNodeK3));

  assert(node_add_link(pNodeB1, pNodeB2, 100, 1) >= 0);
  assert(node_add_link(pNodeB2, pNodeB3, 100, 1) >= 0);
  assert(node_add_link(pNodeB3, pNodeB4, 100, 1) >= 0);
  assert(node_add_link(pNodeB4, pNodeB1, 100, 1) >= 0);
  
  assert(node_add_link(pNodeB2, pNodeX2, 100, 1) >= 0);
  assert(node_add_link(pNodeB3, pNodeX2, 100, 1) >= 0);
  assert(node_add_link(pNodeX2, pNodeX1, 100, 1) >= 0);
  assert(node_add_link(pNodeX2, pNodeX3, 100, 1) >= 0);
 
  assert(node_add_link(pNodeB3, pNodeY1, 100, 1) >= 0);
  assert(node_add_link(pNodeY1, pNodeY2, 100, 1) >= 0);
  
  assert(node_add_link(pNodeB4, pNodeK1, 100, 1) >= 0);
  assert(node_add_link(pNodeB4, pNodeK2, 100, 1) >= 0);
  assert(node_add_link(pNodeB1, pNodeK3, 100, 1) >= 0);
  assert(node_add_link(pNodeK3, pNodeK2, 100, 1) >= 0);
  
   assert(node_add_link_toSubnet(pNodeB1, pSubnetTB1, 100, 1) >= 0);
   assert(node_add_link_toSubnet(pNodeB3, pSubnetTB1, 100, 1) >= 0);
   assert(node_add_link_toSubnet(pNodeX1, pSubnetTX1, 100, 1) >= 0);
   assert(node_add_link_toSubnet(pNodeX3, pSubnetTX1, 100, 1) >= 0);
   assert(node_add_link_toSubnet(pNodeB3, pSubnetTY1, 100, 1) >= 0);
   assert(node_add_link_toSubnet(pNodeY2, pSubnetTY1, 100, 1) >= 0);
   assert(node_add_link_toSubnet(pNodeK2, pSubnetTK1, 100, 1) >= 0);
   assert(node_add_link_toSubnet(pNodeK1, pSubnetTK1, 100, 1) >= 0);

//   assert(node_add_link_toSubnet(pNodeB2, pSubnetSB1, 100, 1) >= 0);
//   assert(node_add_link_toSubnet(pNodeB1, pSubnetSB2, 100, 1) >= 0);
//   assert(node_add_link_toSubnet(pNodeB2, pSubnetSX1, 100, 1) >= 0);
//   assert(node_add_link_toSubnet(pNodeX2, pSubnetSX2, 100, 1) >= 0);
//   assert(node_add_link_toSubnet(pNodeX3, pSubnetSX3, 100, 1) >= 0);
//   assert(node_add_link_toSubnet(pNodeB3, pSubnetSY1, 100, 1) >= 0);
//   assert(node_add_link_toSubnet(pNodeY1, pSubnetSY2, 100, 1) >= 0);
//   assert(node_add_link_toSubnet(pNodeY1, pSubnetSY3, 100, 1) >= 0);
//   assert(node_add_link_toSubnet(pNodeK3, pSubnetSK1, 100, 1) >= 0);
//   assert(node_add_link_toSubnet(pNodeK2, pSubnetSK2, 100, 1) >= 0);
//   assert(node_add_link_toSubnet(pNodeK2, pSubnetSK3, 100, 1) >= 0);

  LOG_DEBUG(" ok!\n");

  LOG_DEBUG("ospf_djk_test(): CHECK Vertex functions...");
  
  SSptVertex * pV1 = NULL; pV1 = spt_vertex_create_byRouter(pNodeB1, 0);
  assert(pV1 != NULL);
  SSptVertex * pV2 = NULL; pV2 = spt_vertex_create_bySubnet(pSubnetTB1, 0);
  assert(pV2 != NULL);
  
//   spt_vertex_get_links(pV1);
  SPtrArray * pLinks;
//   SNetLink * pLink;
  pLinks = spt_vertex_get_links(pV1);
  assert(pLinks != NULL);
//   int iIndex;
  /*
  for (iIndex = 0; iIndex < ptr_array_length(pLinks); iIndex++){
    ptr_array_get_at(pLinks, iIndex, &pLink);
    link_dump(stdout, pLink);
    fprintf(stdout, "\n");
  }
  */
  
  pLinks = spt_vertex_get_links(pV2);
  assert(pLinks != NULL);
//   for (iIndex = 0; iIndex < ptr_array_length(pLinks); iIndex++){
//     ptr_array_get_at(pLinks, iIndex, &pLink);
//     link_dump(stdout, pLink);
//     fprintf(stdout, "\n");
//   }
  
//   ip_prefix_dump(stdout, spt_vertex_get_prefix(pV1));
//   fprintf(stdout, "\n");
//   ip_prefix_dump(stdout, spt_vertex_get_prefix(pV2));
//   fprintf(stdout, "\n");
   spt_vertex_destroy(&pV1);
  assert(pV1 == NULL);
  spt_vertex_destroy(&pV2);
  assert(pV2 == NULL);
  LOG_DEBUG(" ok!\n");
  
  LOG_DEBUG("ospf_test_rfc2328(): Creating igp domain...");
  uint16_t uIGPDomain = 1;
  SIGPDomain * pIGPDomain = get_igp_domain(uIGPDomain); //creating and registering domain
  igp_domain_add_router(pIGPDomain, pNodeB1);
  igp_domain_add_router(pIGPDomain, pNodeB2);
  igp_domain_add_router(pIGPDomain, pNodeB3);
  igp_domain_add_router(pIGPDomain, pNodeB4);
  igp_domain_add_router(pIGPDomain, pNodeX1);
  igp_domain_add_router(pIGPDomain, pNodeX2);
  igp_domain_add_router(pIGPDomain, pNodeX3);
  igp_domain_add_router(pIGPDomain, pNodeY1);
  igp_domain_add_router(pIGPDomain, pNodeY2);
  igp_domain_add_router(pIGPDomain, pNodeK1);
  igp_domain_add_router(pIGPDomain, pNodeK2);
  igp_domain_add_router(pIGPDomain, pNodeK3);
  
  LOG_DEBUG(" done.\n");
  
  LOG_DEBUG("ospf_djk_test(): CHECK Dijkstra function...");
  
  SRadixTree * pSpt = node_ospf_compute_spt(pNodeB1, uIGPDomain, BACKBONE_AREA);
  
//spt_dump(stdout, pSpt, pNodeB1->tAddr);
  FILE * pOutDump = fopen("spt.test.dot", "w");
  spt_dump_dot(pOutDump, pSpt, pNodeB1->tAddr);
  fclose(pOutDump);
  radix_tree_destroy(&pSpt);
  
  LOG_DEBUG(" ok!\n");
  
  LOG_DEBUG("ospf_djk_test(): Computing Routing Table...");
  node_ospf_intra_route_single_area(pNodeB1, uIGPDomain, BACKBONE_AREA);
  
  LOG_DEBUG(" ok!\n");
  OSPF_rt_dump(stdout, pNodeB1->pOspfRT, 0, 0);
  LOG_DEBUG("ospf_djk_test(): STOP\n");
  return 1;
}

int ospf_info_test() {
  const int startAddr = 1024;
  LOG_DEBUG("point-to-point links attached.\n");
  SNetwork * pNetwork= network_create();
  SNetNode * pNodeB1= node_create(startAddr);
  SNetNode * pNodeB2= node_create(startAddr + 1);
  SNetNode * pNodeB3= node_create(startAddr + 2);
  SNetNode * pNodeB4= node_create(startAddr + 3);
  SNetNode * pNodeX1= node_create(startAddr + 4);
  SNetNode * pNodeX2= node_create(startAddr + 5);
  SNetNode * pNodeX3= node_create(startAddr + 6);
  SNetNode * pNodeY1= node_create(startAddr + 7);
  SNetNode * pNodeY2= node_create(startAddr + 8);
  SNetNode * pNodeK1= node_create(startAddr + 9);
  SNetNode * pNodeK2= node_create(startAddr + 10);
  SNetNode * pNodeK3= node_create(startAddr + 11);
  
  SNetSubnet * pSubnetTB1= subnet_create(4, 30, NET_SUBNET_TYPE_TRANSIT);
  SNetSubnet * pSubnetTX1= subnet_create(8, 30, NET_SUBNET_TYPE_TRANSIT);
  SNetSubnet * pSubnetTY1= subnet_create(12, 30, NET_SUBNET_TYPE_TRANSIT);
  SNetSubnet * pSubnetTK1= subnet_create(16, 30, NET_SUBNET_TYPE_TRANSIT);
  
  SNetSubnet * pSubnetSB1= subnet_create(20, 30, NET_SUBNET_TYPE_STUB);
  SNetSubnet * pSubnetSB2= subnet_create(24, 30, NET_SUBNET_TYPE_STUB);
  SNetSubnet * pSubnetSX1= subnet_create(28, 30, NET_SUBNET_TYPE_STUB);
  SNetSubnet * pSubnetSX2= subnet_create(32, 30, NET_SUBNET_TYPE_STUB);
  SNetSubnet * pSubnetSX3= subnet_create(36, 30, NET_SUBNET_TYPE_STUB);
  SNetSubnet * pSubnetSY1= subnet_create(40, 30, NET_SUBNET_TYPE_STUB);
  SNetSubnet * pSubnetSY2= subnet_create(44, 30, NET_SUBNET_TYPE_STUB);
  SNetSubnet * pSubnetSY3= subnet_create(48, 30, NET_SUBNET_TYPE_STUB);
  SNetSubnet * pSubnetSK1= subnet_create(52, 30, NET_SUBNET_TYPE_STUB);
  SNetSubnet * pSubnetSK2= subnet_create(56, 30, NET_SUBNET_TYPE_STUB);
  SNetSubnet * pSubnetSK3= subnet_create(60, 30, NET_SUBNET_TYPE_STUB);

  assert(!network_add_node(pNetwork, pNodeB1));
  assert(!network_add_node(pNetwork, pNodeB2));
  assert(!network_add_node(pNetwork, pNodeB3));
  assert(!network_add_node(pNetwork, pNodeB4));
  assert(!network_add_node(pNetwork, pNodeX1));
  assert(!network_add_node(pNetwork, pNodeX2));
  assert(!network_add_node(pNetwork, pNodeX3));
  assert(!network_add_node(pNetwork, pNodeY1));
  assert(!network_add_node(pNetwork, pNodeY2));
  assert(!network_add_node(pNetwork, pNodeK1));
  assert(!network_add_node(pNetwork, pNodeK2));
  assert(!network_add_node(pNetwork, pNodeK3));
  
  LOG_DEBUG("nodes attached.\n");

  assert(node_add_link(pNodeB1, pNodeB2, 100, 1) >= 0);
  assert(node_add_link(pNodeB2, pNodeB3, 100, 1) >= 0);
  assert(node_add_link(pNodeB3, pNodeB4, 100, 1) >= 0);
  assert(node_add_link(pNodeB4, pNodeB1, 100, 1) >= 0);
  
  assert(node_add_link(pNodeB2, pNodeX2, 100, 1) >= 0);
  assert(node_add_link(pNodeB3, pNodeX2, 100, 1) >= 0);
  assert(node_add_link(pNodeX2, pNodeX1, 100, 1) >= 0);
  assert(node_add_link(pNodeX2, pNodeX3, 100, 1) >= 0);

  assert(node_add_link(pNodeB3, pNodeY1, 100, 1) >= 0);
  assert(node_add_link(pNodeY1, pNodeY2, 100, 1) >= 0);
  
  assert(node_add_link(pNodeB4, pNodeK1, 100, 1) >= 0);
  assert(node_add_link(pNodeB4, pNodeK2, 100, 1) >= 0);
  assert(node_add_link(pNodeB1, pNodeK3, 100, 1) >= 0);
  assert(node_add_link(pNodeK3, pNodeK2, 100, 1) >= 0);
  
  
  assert(node_add_link_toSubnet(pNodeB1, pSubnetTB1, 100, 1) >= 0);
  assert(node_add_link_toSubnet(pNodeB3, pSubnetTB1, 100, 1) >= 0);
  assert(node_add_link_toSubnet(pNodeX1, pSubnetTX1, 100, 1) >= 0);
  assert(node_add_link_toSubnet(pNodeX3, pSubnetTX1, 100, 1) >= 0);
  assert(node_add_link_toSubnet(pNodeB3, pSubnetTY1, 100, 1) >= 0);
  assert(node_add_link_toSubnet(pNodeY2, pSubnetTY1, 100, 1) >= 0);
  assert(node_add_link_toSubnet(pNodeK2, pSubnetTK1, 100, 1) >= 0);
  assert(node_add_link_toSubnet(pNodeK1, pSubnetTK1, 100, 1) >= 0);
  
  LOG_DEBUG("transit-network links attached.\n");
  
  assert(node_add_link_toSubnet(pNodeB2, pSubnetSB1, 100, 1) >= 0);
  assert(node_add_link_toSubnet(pNodeB1, pSubnetSB2, 100, 1) >= 0);
  assert(node_add_link_toSubnet(pNodeB2, pSubnetSX1, 100, 1) >= 0);
  assert(node_add_link_toSubnet(pNodeX2, pSubnetSX2, 100, 1) >= 0);
  assert(node_add_link_toSubnet(pNodeX3, pSubnetSX3, 100, 1) >= 0);
  assert(node_add_link_toSubnet(pNodeB3, pSubnetSY1, 100, 1) >= 0);
  assert(node_add_link_toSubnet(pNodeY1, pSubnetSY2, 100, 1) >= 0);
  assert(node_add_link_toSubnet(pNodeY1, pSubnetSY3, 100, 1) >= 0);
  assert(node_add_link_toSubnet(pNodeK3, pSubnetSK1, 100, 1) >= 0);
  assert(node_add_link_toSubnet(pNodeK2, pSubnetSK2, 100, 1) >= 0);
  assert(node_add_link_toSubnet(pNodeK2, pSubnetSK3, 100, 1) >= 0);
  
  LOG_DEBUG("stub-network links attached.\n");
  
  assert(node_add_OSPFArea(pNodeB1, BACKBONE_AREA) >= 0);
  assert(node_add_OSPFArea(pNodeB1, K_AREA) >= 0);
  assert(node_add_OSPFArea(pNodeB2, BACKBONE_AREA) >= 0);
  assert(node_add_OSPFArea(pNodeB2, X_AREA) >= 0);
  assert(node_add_OSPFArea(pNodeB3, BACKBONE_AREA) >= 0);
  assert(node_add_OSPFArea(pNodeB3, X_AREA) >= 0);
  assert(node_add_OSPFArea(pNodeB3, Y_AREA) >= 0);
  assert(node_add_OSPFArea(pNodeB4, BACKBONE_AREA) >= 0);
  assert(node_add_OSPFArea(pNodeB4, K_AREA) >= 0);
  
  assert(node_add_OSPFArea(pNodeX1, X_AREA) >= 0);
  assert(node_add_OSPFArea(pNodeX2, X_AREA) >= 0);
  assert(node_add_OSPFArea(pNodeX3, X_AREA) >= 0);
  
  assert(node_add_OSPFArea(pNodeY1, Y_AREA) >= 0);
  assert(node_add_OSPFArea(pNodeY2, Y_AREA) >= 0);
  
  assert(node_add_OSPFArea(pNodeK1, K_AREA) >= 0);
  assert(node_add_OSPFArea(pNodeK2, K_AREA) >= 0);
  assert(node_add_OSPFArea(pNodeK3, K_AREA) >= 0);
  
  subnet_set_OSPFArea(pSubnetTB1, BACKBONE_AREA);
  subnet_set_OSPFArea(pSubnetTX1, X_AREA);
  subnet_set_OSPFArea(pSubnetTY1, Y_AREA);
  subnet_set_OSPFArea(pSubnetTK1, K_AREA);
  
  subnet_set_OSPFArea(pSubnetSB1, BACKBONE_AREA);
  subnet_set_OSPFArea(pSubnetSB2, BACKBONE_AREA);
  subnet_set_OSPFArea(pSubnetSX1, X_AREA);
  subnet_set_OSPFArea(pSubnetSX2, X_AREA);
  subnet_set_OSPFArea(pSubnetSX3, X_AREA);
  subnet_set_OSPFArea(pSubnetSY1, Y_AREA);
  subnet_set_OSPFArea(pSubnetSY2, Y_AREA);
  subnet_set_OSPFArea(pSubnetSY3, Y_AREA);
  subnet_set_OSPFArea(pSubnetSK1, K_AREA);
  subnet_set_OSPFArea(pSubnetSK2, K_AREA);
  subnet_set_OSPFArea(pSubnetSK3, K_AREA);
  
  LOG_DEBUG("OSPF area assigned.\n");
  
  assert(node_is_BorderRouter(pNodeB1));
  assert(node_is_BorderRouter(pNodeB2));
  assert(node_is_BorderRouter(pNodeB3));
  assert(node_is_BorderRouter(pNodeB4));
  
  assert(!node_is_BorderRouter(pNodeX1));
  assert(!node_is_BorderRouter(pNodeX2));
  assert(!node_is_BorderRouter(pNodeX3));
  assert(!node_is_BorderRouter(pNodeY1));
  assert(!node_is_BorderRouter(pNodeY2));
  assert(!node_is_BorderRouter(pNodeK1));
  assert(!node_is_BorderRouter(pNodeK2));
  assert(!node_is_BorderRouter(pNodeK3));
  
  assert(!node_is_InternalRouter(pNodeB1));
  assert(!node_is_InternalRouter(pNodeB2));
  assert(!node_is_InternalRouter(pNodeB3));
  assert(!node_is_InternalRouter(pNodeB4));
    
  assert(node_is_InternalRouter(pNodeX1));
  assert(node_is_InternalRouter(pNodeX2));
  assert(node_is_InternalRouter(pNodeX3));
  assert(node_is_InternalRouter(pNodeY1));
  assert(node_is_InternalRouter(pNodeY2));
  assert(node_is_InternalRouter(pNodeK1));
  assert(node_is_InternalRouter(pNodeK2));
  assert(node_is_InternalRouter(pNodeK3));
  
  LOG_DEBUG("all node OSPF method tested... they seem ok!\n");
  
  assert(subnet_get_OSPFArea(pSubnetTX1) == X_AREA);
  assert(subnet_get_OSPFArea(pSubnetTB1) != X_AREA);
  assert(subnet_get_OSPFArea(pSubnetTB1) == BACKBONE_AREA);
  
  assert(subnet_get_OSPFArea(pSubnetSB1) == BACKBONE_AREA);
  assert(subnet_get_OSPFArea(pSubnetSB2) != X_AREA);
  assert(subnet_get_OSPFArea(pSubnetSY1) == Y_AREA);
  assert(subnet_get_OSPFArea(pSubnetSK2) != X_AREA);
  assert(subnet_get_OSPFArea(pSubnetSX2) == X_AREA);
    
  assert(subnet_is_transit(pSubnetTK1));
  assert(!subnet_is_stub(pSubnetTK1));
  return 1; 
}

// ----- test_ospf -----------------------------------------------
int ospf_test()
{
  log_set_level(pMainLog, LOG_LEVEL_EVERYTHING);
  log_set_stream(pMainLog, stderr);

    
//   subnet_test();
//   LOG_DEBUG("all info OSPF methods tested ... they seem ok!\n");
  
//   ospf_info_test();
//   LOG_DEBUG("all subnet OSPF methods tested ... they seem ok!\n");
  
//   ospf_rt_test();
//   LOG_DEBUG("all routing table OSPF methods tested ... they seem ok!\n");
  
  
//   IGPdomain_test();
//   ospf_test_sample_net();
//   LOG_DEBUG("test on sample network... seem ok!\n");
  
  
   ospf_test_rfc2328();
//   LOG_DEBUG("test on sample network in RFC2328... seems ok!\n");
  return 1;
}

