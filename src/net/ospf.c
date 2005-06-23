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
#include <net/ospf.h>
#include <net/ospf_rt.h>
#include <net/network.h>
#include <net/subnet.h>
#include <net/link.h>
#include <net/prefix.h>

#define X_AREA 1
#define Y_AREA 2
#define K_AREA 3

#define DJK_NODE_TYPE_ROUTER 0
#define DJK_NODE_TYPE_SUBNET 1


//////////////////////////////////////////////////////////////////////////
///////  intra_route_computation
//////////////////////////////////////////////////////////////////////////

// typedef struct {
//   uint8_t              uDestinationType;    /* TNET_LINK_TYPE_ROUTER , 
//                                                NET_LINK_TYPE_TRANSIT, NET_LINK_TYPE_STUB */
//   UDestinationId       UDestId;   
//   SDijkNextHop       * aNextHops;
//   net_link_delay_t     uIGPweight;  
// } SSptVertex;

// ----- dijkstra_info_create ---------------------------------------
/*SSptVertex * spt_vertex_create_byRouter(SNetNode * Node)
{
  SSptVertex * pVertex = (SSptVertex *) MALLOC(sizeof(SSptVertex));
  
  pInfo->uIGPweight = 0;
  pInfo->aNextHops = NULL;  ptr_array_create(ARRAY_OPTION_UNIQUE,
				  node_links_compare, 
// 				  NULL);*/
//   return pInfo;
// }


// typedef struct {
//   SPrefix            * pPrefix; 
//   SNetSubnet	     * pSubnet;
  //SPtrArray        * aNextHops;  /* list to manage Equal Cost Multi Path */
//   SDijkNextHop       * aNextHops;
//   net_link_delay_t     uIGPweight;  
// } SDijkContext;


// typedef struct {
//   next_hops_list_s * aNextHops;
//   net_link_delay_t   uIGPweight;
// } SDijkstraInfo;

// ----- dijkstra_info_create ---------------------------------------
/*SDijkstraInfo * OSPF_dijkstra_info_create(net_link_delay_t uIGPweight)
{
  SDijkstraInfo * pInfo=
    (SDijkstraInfo *) MALLOC(sizeof(SDijkstraInfo));
  pInfo->uIGPweight= uIGPweight;
  pInfo->aNextHops= ptr_array_create(ARRAY_OPTION_UNIQUE,
				  node_links_compare, 
				  NULL);
  return pInfo;
}

// ----- dijkstra_info_destroy --------------------------------------
void OSPF_dijkstra_info_destroy(void ** ppItem)
{  
  SDijkstraInfo * pInfo= *((SDijkstraInfo **) ppItem);
  if (pInfo->aNextHops != NULL)
      ptr_array_destroy(&(pInfo->aNextHops));
  FREE(pInfo);
}*/

// ----- dijkstra ---------------------------------------------------
/**
 * Compute the Shortest Path Tree from the given source router
 * towards all the other routers that belong to the given prefix.
 */
/*SRadixTree * OSPF_dijkstra(SNetwork * pNetwork, net_addr_t tSrcAddr,
		      SPrefix sPrefix)
{
  SNetLink * pLink;
  SNetNode * pNode = NULL;
  SNetSubnet * pSubnet;
  SFIFO * pFIFO;
  SRadixTree * pVisited;
  SDijkContext * pContext = NULL, * pOldContext =  NULL;
  SPtrArray * aLinks;
  
  SDijkstraInfo * pInfo;
  SPrefix  * pPrefix = (SPrefix *) MALLOC(sizeof(SPrefix));;
  int iIndex;//, iIndexN;//, iIndexL;

  pVisited= radix_tree_create(32, OSPF_dijkstra_info_destroy);
  pFIFO= fifo_create(100000, NULL);
  pContext= (SDijkContext *) MALLOC(sizeof(SDijkContext));
  
  
  pContext->pSubnet = NULL;
  
  pPrefix->tNetwork = tSrcAddr;
  pPrefix->uMaskLen = 32;
  
  pContext->pPrefix = pPrefix;
  pContext->pSubnet = NULL;
  pContext->aNextHops = NULL;ptr_array_create(ARRAY_OPTION_UNIQUE,
				  ip_prefixes_compare, 
				  ip_prefixes_destroy);*/
/*  pContext->uIGPweight = 0;
  
  fifo_push(pFIFO, pContext);
  radix_tree_add(pVisited, tSrcAddr, 32, OSPF_dijkstra_info_create(0));

  // Breadth-first search
  while (1) {
     pContext= (SDijkContext *) fifo_pop(pFIFO);

     //pNode= network_find_node(pNetwork, pContext->pPrefix->tNetwork);//REMOVE
     //assert(pNode != NULL);//REMOVE
     //ptr_array_get_at(pNode->pLinks, 0, &(pLink));//REMOVE
     //pContext->pSubnet = (pLink->UDestId).pSubnet;
     //pContext->pPrefix->uMaskLen = 31;//REMOVE
   
     if (pContext == NULL)
       break;
     if (pContext->pPrefix->uMaskLen == 32) {//if it's a router
        pNode= network_find_node(pNetwork, pContext->pPrefix->tNetwork);
       assert(pNode != NULL);
       aLinks = pNode->pLinks;
     }
     else {
       pSubnet= pContext->pSubnet;
       assert(pSubnet != NULL);
       //subnet_dump(stdout, pSubnet);
       aLinks = ptr_array_create(0,//ARRAY_OPTION_SORTED,
 				  node_links_compare,
 				  NULL);
       aLinks = subnet_get_links(pSubnet);
       assert(aLinks != NULL);
       }
      

     pOldContext= pContext;
     for (iIndex= 0; iIndex < ptr_array_length(aLinks); iIndex++) {
       ptr_array_get_at(aLinks, iIndex, &(pLink));
       link_dump(stdout, pLink);
       fprintf(stdout, "\n");
       Consider only the links that have the following properties:
	 - NET_LINK_FLAG_IGP_ADV
	 - NET_LINK_FLAG_UP (the link must be UP)
	 - the end-point belongs to the given prefix */
       /*if (link_get_state(pLink, NET_LINK_FLAG_IGP_ADV) &&
	   link_get_state(pLink, NET_LINK_FLAG_UP) &&
	   ((!NET_OPTIONS_IGP_INTER && ip_address_in_prefix(link_get_addr(pLink), sPrefix)) ||
	   (NET_OPTIONS_IGP_INTER && ip_address_in_prefix(pNode->tAddr, sPrefix)))) {*/
	
	   
/*	uint32_t uNetmaskLen;
	if (pLink->uDestinationType == NET_LINK_TYPE_ROUTER)
	  uNetmaskLen = 32;
	else
	  uNetmaskLen = (((pLink->UDestId).pSubnet)->pPrefix)->uMaskLen;
	  
	//fprintf(stdout, "uNetmaskLen %d\n", uNetmaskLen);
	
	
	pInfo = (SDijkstraInfo *) radix_tree_get_exact(pVisited, link_get_addr(pLink), uNetmaskLen);
	if ((pInfo == NULL) ||
	    (pInfo->uIGPweight > pOldContext->uIGPweight+pLink->uIGPweight)) {
	    fprintf(stdout, "pInfo è NULL o ha peso da aggiornare\n");
	  pContext= (SDijkContext *) MALLOC(sizeof(SDijkContext));
	  
	  pContext->pPrefix = (SPrefix *) MALLOC(sizeof(SPrefix)) ;
	  link_get_prefix(pLink, pContext->pPrefix);
	  if (pLink->uDestinationType != NET_LINK_TYPE_ROUTER)
	    pContext->pSubnet = (pLink->UDestId).pSubnet;
	  
	  //fprintf(stdout, "recuperato id per subnet...\n");
	  //subnet_dump(stdout, pContext->pSubnet);  
	  if (pOldContext->tNextHop == tSrcAddr) {
	    link_get_prefix(pLink, &nextHopPrefix);
	    pContext->tNextHop= link_get_prefix(pLink);
	    
	  }
	  else
	    pContext->tNextHop= pOldContext->tNextHop;
	    
	  pContext->uIGPweight= pOldContext->uIGPweight+pLink->uIGPweight;
	  if (pInfo == NULL) {
	    pInfo= dijkstra_info_create(pOldContext->uIGPweight+pLink->uIGPweight);
	    pInfo->tNextHop= pContext->tNextHop;
	    radix_tree_add(pVisited, link_get_addr(pLink), 32, pInfo);
	  } else {
	    pInfo->uIGPweight= pOldContext->uIGPweight+pLink->uIGPweight;
	    pInfo->tNextHop= pOldContext->tNextHop;
	  }
	  assert(fifo_push(pFIFO, pContext) == 0);*/
/*	}
	
      }
     break;
      
     
     ptr_array_destroy(&aLinks);
     FREE(pOldContext->pPrefix);
     pOldContext->pPrefix = NULL;
     FREE(pOldContext);
     pOldContext = NULL;
     }
  fifo_destroy(&pFIFO);

  return pVisited;
}*/

/////////////////////////////////////////////////////////////////////
/////// OSPF methods for node object
/////////////////////////////////////////////////////////////////////

// ----- node_add_OSPFArea ------------------------------------------
int node_add_OSPFArea(SNetNode * pNode, uint32_t OSPFArea)
{
  return int_array_add(pNode->pOSPFAreas, &OSPFArea);
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


/////////////////////////////////////////////////////////////////////
/////// OSPF methods for node object
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


/*int ospf_djk_test()
{
  const int startAddr = 1024;

  SDijkstraInfo * pInfo = OSPF_dijkstra_info_create(0);
  void * pVoid = (void *)pInfo;
  OSPF_dijkstra_info_destroy(&pVoid);
  //assert(pVoid == NULL);
  
  SNetwork * pNetwork= network_create();
  SNetNode * pNodeB1= node_create(startAddr);
  SNetNode * pNodeB2= node_create(startAddr + 1);
  SNetNode * pNodeB3= node_create(startAddr + 2);
  SNetNode * pNodeB4= node_create(startAddr + 3);
  
  SNetSubnet * pSubnetTB1= subnet_create(4, 30, NET_SUBNET_TYPE_TRANSIT);
  SNetSubnet * pSubnetTB2= subnet_create(8, 30, NET_SUBNET_TYPE_TRANSIT);
  
  assert(!network_add_node(pNetwork, pNodeB1));
  assert(!network_add_node(pNetwork, pNodeB2));
  assert(!network_add_node(pNetwork, pNodeB3));
  assert(!network_add_node(pNetwork, pNodeB4));
  
  assert(node_add_link_toSubnet(pNodeB1, pSubnetTB1, 100, 1) >= 0);
  assert(node_add_link_toSubnet(pNodeB2, pSubnetTB1, 100, 1) >= 0);
  assert(node_add_link_toSubnet(pNodeB3, pSubnetTB1, 100, 1) >= 0);
  assert(node_add_link_toSubnet(pNodeB4, pSubnetTB1, 100, 1) >= 0);
  
  assert(node_add_link_toSubnet(pNodeB1, pSubnetTB2, 100, 1) >= 0);
  
  SPrefix sPrefix;
  sPrefix.tNetwork = startAddr;
  sPrefix.uMaskLen = 22;
  
  OSPF_dijkstra(pNetwork, pNodeB1->tAddr, sPrefix);
  TODO meglio aggiungere a Link un puntatore al nodo: verrà usato nella procedura
         che linka il nodo alla subnet.
	 
	 cicla con ptr_array_get_at
	 prendi il link e aggiungi in array
	 
	 potresti scrivere una funzione che prende in un parametro la posizione
	 del link e lo restituisce...
	 
  //subnet_get_links(pSubnetTB1);
  return 1;
}*/

// ----- ospf_rt_test() -----------------------------------------------
int ospf_rt_test(){
  char * pcEndPtr;
  SPrefix  sSubnetPfxTx;
  SPrefix  sSubnetPfxTx1;
  net_addr_t tAddrA, tAddrB, tAddrC;
  
  assert(!ip_string_to_address("192.168.0.2", &pcEndPtr, &tAddrA));
  assert(!ip_string_to_address("192.168.0.3", &pcEndPtr, &tAddrB));
  assert(!ip_string_to_address("192.168.0.4", &pcEndPtr, &tAddrC));
  
  SNetNode * pNodeA= node_create(tAddrA);
  SNetNode * pNodeB= node_create(tAddrB);
  SNetNode * pNodeC= node_create(tAddrC);
  
  assert(!ip_string_to_prefix("192.168.0.0/24", &pcEndPtr, &sSubnetPfxTx));
  assert(!ip_string_to_prefix("192.120.0.0/24", &pcEndPtr, &sSubnetPfxTx1));
  
  //ip_prefix_dump(stdout, sSubnetPfxTx);
  //ip_prefix_dump(stdout, sSubnetPfxTx1);
  
  SNetSubnet * pSubTx = subnet_create(sSubnetPfxTx.tNetwork,
                                              sSubnetPfxTx.uMaskLen, NET_SUBNET_TYPE_TRANSIT);
//   SNetSubnet * pSubTx1 = subnet_create(sSubnetPfxTx1.tNetwork,
//                                                sSubnetPfxTx.uMaskLen,
// 					       NET_SUBNET_TYPE_TRANSIT);
  //subnet_dump(stdout, pSubTx);
  //subnet_dump(stdout, pSubTx1);
 
  assert(node_add_link_toSubnet(pNodeA, pSubTx, 100, 1) >= 0);
  assert(node_add_link_toSubnet(pNodeB, pSubTx, 100, 1) >= 0);
  assert(node_add_link_toSubnet(pNodeC, pSubTx, 100, 1) >= 0);
  assert(node_add_link(pNodeA, pNodeC, 100, 1) >= 0);
  
  SNetLink * pLinkAS = node_find_link_to_subnet(pNodeA, pSubTx);
  assert(pLinkAS != NULL);
  SNetLink * pLinkAC = node_find_link_to_router(pNodeA, tAddrC);
  assert(pLinkAC != NULL);
  
  SOSPFNextHop * pNHB = ospf_next_hop_create(pLinkAS, tAddrB);
  assert(pNHB != NULL);
  assert(pNHB->pLink == pLinkAS && pNHB->tAddr == tAddrB);
  SOSPFNextHop * pNHC1 = ospf_next_hop_create(pLinkAS, tAddrC);
  assert(pNHB != NULL);
  SOSPFNextHop * pNHC2 = ospf_next_hop_create(pLinkAC, tAddrC);
  assert(pNHB != NULL);
  
  
  next_hops_list_s * pNHList = ospf_nh_list_create();
  assert(pNHList != NULL);
  
  ospf_nh_list_add(pNHList, pNHB);
  //TODO ospf_nh_list_dump
  //TODO collegare nh_list a rt... vedi sotto 
  ospf_nh_list_destroy(&pNHList);
  assert(pNHList == NULL);
  
  ospf_next_hop_destroy(&pNHB);
  assert(pNHB == NULL);
  ospf_next_hop_destroy(&pNHC1);
  assert(pNHC1 == NULL);
  ospf_next_hop_destroy(&pNHC2);
  assert(pNHC2 == NULL);
  
  subnet_destroy(&pSubTx);
//   subnet_destroy(&pSubTx1);
  
  node_destroy(&pNodeA);
  node_destroy(&pNodeB);
  node_destroy(&pNodeC);
  
    
  /**/
  
/*SOSPFRT * pRt= OSPF_rt_create();
  assert(pRt != NULL);
  
//   SNetNode * pNodeA= node_create(1);
  SNetNode * pNodeB= node_create(2);
  SNetNode * pNodeC= node_create(2);
  
//   SNetLink * pLinkToA = create_link_toRouter(pNodeA);
  SNetLink * pLinkToB = create_link_toRouter(pNodeB);
  SNetLink * pLinkToC2 = create_link_toRouter(pNodeC);
  SNetLink * pLinkToC1 = create_link_toRouter(pNodeC);
  
  SPrefix  sPrefixB, sPrefixC;
  sPrefixB.tNetwork = 2048;
  sPrefixB.uMaskLen = 28;
  sPrefixC.tNetwork = 4096;
  sPrefixC.uMaskLen = 28;
  
  SOSPFRouteInfo * pRiB = OSPF_route_info_create(OSPF_DESTINATION_TYPE_ROUTER,
                                       sPrefixB,
				       100,
				       BACKBONE_AREA,
				       OSPF_PATH_TYPE_INTRA ,
				       pLinkToB);
  assert(pRiB != NULL);
  SOSPFRouteInfo * pRiC = OSPF_route_info_create(OSPF_DESTINATION_TYPE_ROUTER,
                                       sPrefixC,
				       100,
				       BACKBONE_AREA,
				       OSPF_PATH_TYPE_INTRA ,
				       pLinkToC1);

  assert(pRiC != NULL);
  assert(OSPF_rt_add_route(pRt, sPrefixB, pRiB) >= 0);
  assert(OSPF_rt_add_route(pRt, sPrefixC, pRiC) >= 0);
 
  OSPF_route_info_dump(stdout, pRiB);
  OSPF_route_info_dump(stdout, pRiC);
  
  assert(OSPF_route_info_add_nextHop(pRiC, pLinkToC2) >= 0); 
  OSPF_route_info_dump(stdout, pRiC);
  
  assert(OSPF_rt_find_exact(pRt, sPrefixB, NET_ROUTE_ANY) == pRiB);
  
  OSPF_rt_dump(stdout, pRt);
  
  OSPF_rt_destroy(&pRt);
  assert(pRt == NULL);
*/  
  return 1;
}


int ospf_info_test() {
  
  const int startAddr = 1024;
  
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
  
//  FILE * pNetFile;
  /*
  simulator_init();
  */
  log_set_level(pMainLog, LOG_LEVEL_EVERYTHING);
  log_set_stream(pMainLog, stderr);

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
  
  LOG_DEBUG("point-to-point links attached.\n");
  
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
  subnet_test();
  LOG_DEBUG("all info OSPF methods tested ... they seem ok!\n");
  
  ospf_info_test();
  LOG_DEBUG("all subnet OSPF methods tested ... they seem ok!\n");
  
  ospf_rt_test();
  LOG_DEBUG("all routing table OSPF methods tested ... they seem ok!\n");
  
//  ospf_djk_test();
  
//  LOG_DEBUG("all dijkstra OSPF methods tested ... they seem ok!\n");
  return 1;
}

