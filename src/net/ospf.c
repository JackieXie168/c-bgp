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

// ----- spt_vertex_are_equal ---------------------------------------
int spt_vertex_compare(void * pItem1, void * pItem2, unsigned int uEltSize);

//////////////////////////////////////////////////////////////////////////
///////  intra_route_computation
//////////////////////////////////////////////////////////////////////////

/*typedef union {
  SNetNode * pNode;
  SNetSubnet * pSubnet;
} USPtDestId;*/

typedef SPtrArray spt_vertex_list_t;

 typedef struct vertex_t {
   uint8_t              uDestinationType;    /* TNET_LINK_TYPE_ROUTER , 
                                                NET_LINK_TYPE_TRANSIT, NET_LINK_TYPE_STUB */ 
   void               * pObject;
   next_hops_list_t   * pNextHops;
   net_link_delay_t     uIGPweight;  
   
   SPtrArray          * fathers;    //ONLY FOR DEBUG we can remove
   SPtrArray          * sons;      //ONLY FOR DEBUG we can remove
 } SSptVertex;

// ----- spt_vertex_create_byRouter ---------------------------------------
SSptVertex * spt_vertex_create_byRouter(SNetNode * pNode, net_link_delay_t uIGPweight)
{
  SSptVertex * pVertex = (SSptVertex *) MALLOC(sizeof(SSptVertex));
  pVertex->uDestinationType = NET_LINK_TYPE_ROUTER;
//   (pVertex->UDestId).pNode = pNode;
  pVertex->pObject = pNode;
  pVertex->uIGPweight = uIGPweight;
  pVertex->pNextHops = ospf_nh_list_create();
  
  pVertex->fathers = ptr_array_create(ARRAY_OPTION_UNIQUE, spt_vertex_compare, NULL);
  pVertex->sons = ptr_array_create(ARRAY_OPTION_UNIQUE, spt_vertex_compare, NULL);
  return pVertex;
}

// ----- spt_vertex_create_bySubnet ---------------------------------------
SSptVertex * spt_vertex_create_bySubnet(SNetSubnet * pSubnet, net_link_delay_t uIGPweight)
{
  SSptVertex * pVertex = (SSptVertex *) MALLOC(sizeof(SSptVertex));
  pVertex->uDestinationType = NET_LINK_TYPE_TRANSIT;
//   (pVertex->UDestId).pSubnet = pSubnet;
  pVertex->pObject = pSubnet;
  pVertex->uIGPweight = uIGPweight;
  pVertex->pNextHops = ospf_nh_list_create();
  
  pVertex->fathers = ptr_array_create(ARRAY_OPTION_UNIQUE, spt_vertex_compare, NULL);
  pVertex->sons = ptr_array_create(ARRAY_OPTION_UNIQUE, spt_vertex_compare, NULL);
  return pVertex;
}

// ----- spt_vertex_create -----------------------------------------------------------------
SSptVertex * spt_vertex_create(SNetwork * pNetwork, SNetLink * pLink, SSptVertex * pVFather)
{
  SSptVertex * pVertex;
  
  if (pLink->uDestinationType == NET_LINK_TYPE_ROUTER){
    SNetNode * pNode;
    fprintf(stdout, "creo vertex da router\n");
    pNode = network_find_node(pNetwork, link_get_addr(pLink));
    assert(pNode != NULL);
    pVertex = spt_vertex_create_byRouter(pNode, pVFather->uIGPweight + pLink->uIGPweight);
  }
  else if (pLink->uDestinationType == NET_LINK_TYPE_TRANSIT ||
           pLink->uDestinationType == NET_LINK_TYPE_STUB){
    SNetSubnet * pSubnet;
    fprintf(stdout, "creo vertex da subnet\n");
    pSubnet = link_get_subnet(pLink);
    pVertex = spt_vertex_create_bySubnet(pSubnet, pVFather->uIGPweight + pLink->uIGPweight);
  }
  else
    return NULL;
    
  return pVertex;
}

// ----- spt_vertex_destroy ---------------------------------------
void spt_vertex_destroy(SSptVertex ** ppVertex){
  if (*ppVertex != NULL){
    if ((*ppVertex)->pNextHops != NULL)
      ospf_nh_list_destroy(&((*ppVertex)->pNextHops));
     if ((*ppVertex)->sons != NULL)
       ptr_array_destroy(&((*ppVertex)->sons));
    FREE(*ppVertex);
    *ppVertex = NULL;
  }    
}

// ----- spt_vertex_dst ---------------------------------------
void spt_vertex_dst(void ** ppItem){
  SSptVertex * pVertex = *((SSptVertex **)(ppItem));
  spt_vertex_destroy(&pVertex);    
}

// ----- spt_vertex_get_links ---------------------------------------
SPtrArray * spt_vertex_get_links(SSptVertex * pVertex)
{
  if (pVertex->uDestinationType == NET_LINK_TYPE_ROUTER){
//     LOG_DEBUG("è un router\n");
    return  ((SNetNode *)(pVertex->pObject))->pLinks;
  }
  else{
//     LOG_DEBUG("è una subnet\n");
    return subnet_get_links((SNetSubnet *)(pVertex->pObject));
  }
}

// ----- spt_vertex_get_links ---------------------------------------
SPrefix spt_vertex_get_prefix(SSptVertex * pVertex)
{     
  SPrefix p;
  if (pVertex->uDestinationType == NET_LINK_TYPE_ROUTER) {

    p.tNetwork = ((SNetNode *)(pVertex->pObject))->tAddr;
    p.uMaskLen = 32;

  }
  else {
    p = *(((SNetSubnet *)(pVertex->pObject))->pPrefix);
  }
  return p;
}

// ----- spt_vertex_compare ---------------------------------------
int spt_vertex_compare(void * pItem1, void * pItem2, unsigned int uEltSize)
{
  SSptVertex * pVertex1= *((SSptVertex **) pItem1);
  SSptVertex * pVertex2= *((SSptVertex **) pItem2); 
  
  if (pVertex1->uDestinationType == pVertex2->uDestinationType) {
    if (pVertex1->uDestinationType == NET_LINK_TYPE_ROUTER){
//        fprintf(stdout, "spt_vertex_compare: sono router\n");
//       ip_address_dump(stdout, ((SNetNode *)(pVertex1->pObject))->tAddr);
      if (((SNetNode *)(pVertex1->pObject))->tAddr < ((SNetNode *)(pVertex2->pObject))->tAddr)
        return -1;
      else if (((SNetNode *)(pVertex1->pObject))->tAddr > ((SNetNode *)(pVertex2->pObject))->tAddr)
        return 1;
      else
	return 0;
    }
    else {
//       fprintf(stdout, "spt_vertex_compare: sono subnet\n");
      SPrefix sPrefV1 = spt_vertex_get_prefix(pVertex1); 
      SPrefix sPrefV2 = spt_vertex_get_prefix(pVertex2); 
      SPrefix * pPrefV1 = &sPrefV1;
      SPrefix * pPrefV2 = &sPrefV2;
//       ip_prefix_dump(stdout, sPrefV1);
//       ip_prefix_dump(stdout, sPrefV2);
//       fprintf(stdout, "spt_vertex_compare: prefissi presi\n");
      return ip_prefixes_compare(&pPrefV1, &pPrefV2, 0);
    }
  }
  else if (pVertex1->uDestinationType == NET_LINK_TYPE_ROUTER)
   return -1;
  else 
   return  1;
}



// ----- spt_get_best_candidate -----------------------------------------------
/*
  Search for the best candidate to add to SPT.
  - vertex with smallest cost are preferred
  - transit network are preferred on router
*/
SSptVertex * spt_get_best_candidate(SPtrArray * paGrayVertexes)
{
  SSptVertex * pBestVertex = NULL;
  SSptVertex * pCurrentVertex = NULL;
  
  int iBestIndex = 0, iIndex;
  
  if (ptr_array_length(paGrayVertexes) > 0) {
     fprintf(stdout, "vertex in aGray : %d\n", ptr_array_length(paGrayVertexes));
    //search for best candidate in frontier
    
//       ptr_array_get_at(aGrayVertexes, 0, &pCurrentVertex);
//       ip_prefix_dump(stdout, spt_vertex_get_prefix(pNewVertex));
// 	  fprintf(stdout, "\n");
    for (iIndex = 0; iIndex < ptr_array_length(paGrayVertexes); iIndex++) {
      ptr_array_get_at(paGrayVertexes, iIndex, &pCurrentVertex);
      assert(pCurrentVertex != NULL);
      if (pBestVertex == NULL){
// 	fprintf(stdout, "prendo il primo : \n");
	ip_prefix_dump(stdout, spt_vertex_get_prefix(pCurrentVertex));
	fprintf(stdout, "\n");
	pBestVertex = pCurrentVertex;
	iBestIndex = iIndex;
      }
      else if (pCurrentVertex->uIGPweight <  pBestVertex->uIGPweight){
// 	fprintf(stdout, "trovato migliore : \n");
	ip_prefix_dump(stdout, spt_vertex_get_prefix(pCurrentVertex));
	fprintf(stdout, "\n");
	pBestVertex = pCurrentVertex;
	iBestIndex = iIndex;
     }
   }  
   fprintf(stdout, "rimuovo da aGrayVertex ...\n");
   ptr_array_remove_at(paGrayVertexes, iBestIndex);
  } 
  return pBestVertex;
}


// ----- dijkstra ---------------------------------------------------
/**
 * Compute the Shortest Path Tree from the given source router
 * towards all the other routers that belong to the given prefix.
 */
SRadixTree * OSPF_dijkstra(SNetwork * pNetwork, net_addr_t tSrcAddr,
		      SPrefix sPrefix)
{
  SPrefix      sDestPrefix; 

  int iIndex = 0;
  int iOldVertexPos;

//   SNetSubnet * pSubnet = NULL;
  SPtrArray  * aLinks = NULL;
  SNetLink   * pCurrentLink = NULL;
  SSptVertex * pCurrentVertex = NULL, * pNewVertex = NULL, * pOldVertex;
  SRadixTree * pSpt = radix_tree_create(32, spt_vertex_dst);
//   SRadixTree * pFrontier= radix_tree_create(32, NULL);
   SNetNode * pRootNode = network_find_node(pNetwork, tSrcAddr);//, * pNode = NULL;
  assert(pRootNode != NULL);
  spt_vertex_list_t * aGrayVertexes = ptr_array_create(ARRAY_OPTION_SORTED|ARRAY_OPTION_UNIQUE,
                                                     spt_vertex_compare,
     	    				            NULL);
  //ADD root to SPT
  pCurrentVertex = spt_vertex_create_byRouter(pRootNode, 0);
  assert(pCurrentVertex != NULL);
  while (pCurrentVertex != NULL) {
    sDestPrefix = spt_vertex_get_prefix(pCurrentVertex);
    fprintf(stdout, "Current vertex is ");
    ip_prefix_dump(stdout, sDestPrefix);
    fprintf(stdout, "\n");
    
    radix_tree_add(pSpt, sDestPrefix.tNetwork, sDestPrefix.uMaskLen, pCurrentVertex);
    
    aLinks = spt_vertex_get_links(pCurrentVertex);
    for (iIndex = 0; iIndex < ptr_array_length(aLinks); iIndex++){
      ptr_array_get_at(aLinks, iIndex, &pCurrentLink);
      //TODO add check to link flags
      
//       debug...
      fprintf(stdout, "ospf_djk(): current Link is ");
      link_get_prefix(pCurrentLink, &sDestPrefix);
      ip_prefix_dump(stdout, sDestPrefix);
      fprintf(stdout, "\n");
      
      //first time consider only Transit Link
      if (pCurrentLink->uDestinationType == NET_LINK_TYPE_STUB){
        fprintf(stdout, "è una stub... passo al prossimo\n");
        continue;
      }
      //check if vertex is in spt	
      link_get_prefix(pCurrentLink, &sDestPrefix);
      pOldVertex = (SSptVertex *)radix_tree_get_exact(pSpt, 
                                                          sDestPrefix.tNetwork, 
							  sDestPrefix.uMaskLen);
      
      if(pOldVertex != NULL){
       fprintf(stdout, "è già nel spt... passo al prossimo\n");
       continue;
      }
      
      
      //create a vertex object to compare with grayVertex
      pNewVertex = spt_vertex_create(pNetwork, pCurrentLink, pCurrentVertex);
      assert(pNewVertex != NULL);
      
      fprintf(stdout, "nuovo vertex creato\n");
      //check if vertex is in grayVertexes
      int srchRes = -1;
      if (ptr_array_length(aGrayVertexes) > 0)
        srchRes = ptr_array_sorted_find_index(aGrayVertexes, &pNewVertex, &iOldVertexPos);
      else
        fprintf(stdout, "aGrays è vuoto\n");
      //srchres == -1 ==> item not found
      //srchres == 0  ==> item found
      pOldVertex = NULL;
      if(srchRes == 0) {
        fprintf(stdout, "vertex è già in aGrayVertexes\n");
        ptr_array_get_at(aGrayVertexes, iOldVertexPos, &pOldVertex);
      }
      
      if (pOldVertex == NULL){
        fprintf(stdout, "vertex aggiunto\n");
// TODO        SOSPFNextHop * pNH = compute_next_hop();
//             aggiungere pOldVertex a Grayvertexes
        ptr_array_add(aGrayVertexes, &pNewVertex);
  	ptr_array_add(pCurrentVertex->sons, &pNewVertex);
	ptr_array_add(pNewVertex->fathers, &pCurrentVertex);
// TODO	ospf_nh_list_add(pOldVertex->pNextHops, pNH);
      }
      else if (pOldVertex->uIGPweight > pNewVertex->uIGPweight) {
        int iPos;
// TODO        SNextHop * pNH = compute_next_hop();
        //TODO implement nh_list_remove
	pOldVertex->uIGPweight = pNewVertex->uIGPweight;
//         ospf_nh_list_destroy(&(pOldVertex->pNextHops));
// 	pOldVertex->pNextHops = ospf_nh_list_create();
// TODO	ospf_nh_list_add(pOldVertex->pNextHops, pNH);
        int iIndex;
	SSptVertex * pFather;
	for( iIndex = 0; iIndex < ptr_array_length(pOldVertex->fathers); iIndex++){
	  ptr_array_get_at(pOldVertex->fathers, iIndex, &pFather);
	  assert(ptr_array_sorted_find_index(pFather->sons, &pOldVertex, &iPos) == 0);
	  ptr_array_remove_at(pFather->sons, iPos);
	}
	
	for( iIndex = 0; iIndex < ptr_array_length(pOldVertex->fathers); iIndex++){
	  ptr_array_remove_at(pOldVertex->fathers, iIndex);
	}
	
        ptr_array_add(pOldVertex->fathers, &pCurrentVertex);
        ptr_array_add(pCurrentVertex->sons, &pOldVertex);
        spt_vertex_destroy(&pNewVertex);
	fprintf(stdout, "vertex aggiornato(peso minore)\n");
      }
      else if (pOldVertex->uIGPweight == pNewVertex->uIGPweight) {
// TODO        SNextHop * pNH = compute_next_hop();
//         ospf_nh_list_add(pVertex->aNextHops, pNH);
        ptr_array_add(pOldVertex->fathers, &pCurrentVertex);
        ptr_array_add(pCurrentVertex->sons, &pOldVertex);
        spt_vertex_destroy(&pNewVertex);
	fprintf(stdout, "vertex aggiornato (peso uguale)\n");
      }
      else {
        fprintf(stdout, "new vertex eliminato\n");
        spt_vertex_destroy(&pNewVertex);
      }  
    }//end for on links
    if (pCurrentVertex->uDestinationType == NET_LINK_TYPE_TRANSIT ||
        pCurrentVertex->uDestinationType == NET_LINK_TYPE_STUB)
	ptr_array_destroy(&aLinks);
    
    pCurrentVertex = spt_get_best_candidate(aGrayVertexes);
  }
  return pSpt;
}

void visit_vertex(SSptVertex * pVertex, int level, FILE * pStream){
//   pVertex->color = GRAY;
  SSptVertex * pChild;
  int iIndex; 
  if (level != 0){
    for (iIndex = 0; iIndex < level; iIndex++)
    	fprintf(pStream, "            ");
    fprintf(pStream, "|");
  }
//   for (iIndex = 0; iIndex < level; iIndex++)
  fprintf(pStream, "---");
  fprintf(pStream, "> [ ");
  ip_prefix_dump(pStream, spt_vertex_get_prefix(pVertex));
  fprintf(pStream, " ]-[ %d ]\n", pVertex->uIGPweight);
  level++;
  for(iIndex = 0; iIndex < ptr_array_length(pVertex->sons); iIndex++){
     ptr_array_get_at(pVertex->sons, iIndex, &pChild);
     visit_vertex(pChild, level, pStream);
  }
}
// ----- spt_dump ------------------------------------------
void spt_dump(FILE * pStream, SRadixTree * pSpt, net_addr_t tRadixAddr)
{
  SSptVertex * pRadix = (SSptVertex *)radix_tree_get_exact(pSpt, tRadixAddr, 32);
//   SSptVertex * pNext = NULL;
  
  visit_vertex(pRadix, 0, pStream);
}
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


int ospf_djk_test()
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
//   SNetNode * pNodeX1= node_create(startAddr + 4);
//   SNetNode * pNodeX2= node_create(startAddr + 5);
//   SNetNode * pNodeX3= node_create(startAddr + 6);
//   SNetNode * pNodeY1= node_create(startAddr + 7);
//   SNetNode * pNodeY2= node_create(startAddr + 8);
//   SNetNode * pNodeK1= node_create(startAddr + 9);
//   SNetNode * pNodeK2= node_create(startAddr + 10);
//   SNetNode * pNodeK3= node_create(startAddr + 11);
  
  SNetSubnet * pSubnetTB1= subnet_create(4, 30, NET_SUBNET_TYPE_TRANSIT);
//   SNetSubnet * pSubnetTX1= subnet_create(8, 30, NET_SUBNET_TYPE_TRANSIT);
//   SNetSubnet * pSubnetTY1= subnet_create(12, 30, NET_SUBNET_TYPE_TRANSIT);
//   SNetSubnet * pSubnetTK1= subnet_create(16, 30, NET_SUBNET_TYPE_TRANSIT);
  
//   SNetSubnet * pSubnetSB1= subnet_create(20, 30, NET_SUBNET_TYPE_STUB);
//   SNetSubnet * pSubnetSB2= subnet_create(24, 30, NET_SUBNET_TYPE_STUB);
//   SNetSubnet * pSubnetSX1= subnet_create(28, 30, NET_SUBNET_TYPE_STUB);
//   SNetSubnet * pSubnetSX2= subnet_create(32, 30, NET_SUBNET_TYPE_STUB);
//   SNetSubnet * pSubnetSX3= subnet_create(36, 30, NET_SUBNET_TYPE_STUB);
//   SNetSubnet * pSubnetSY1= subnet_create(40, 30, NET_SUBNET_TYPE_STUB);
//   SNetSubnet * pSubnetSY2= subnet_create(44, 30, NET_SUBNET_TYPE_STUB);
//   SNetSubnet * pSubnetSY3= subnet_create(48, 30, NET_SUBNET_TYPE_STUB);
//   SNetSubnet * pSubnetSK1= subnet_create(52, 30, NET_SUBNET_TYPE_STUB);
//   SNetSubnet * pSubnetSK2= subnet_create(56, 30, NET_SUBNET_TYPE_STUB);
//   SNetSubnet * pSubnetSK3= subnet_create(60, 30, NET_SUBNET_TYPE_STUB);
  
  assert(!network_add_node(pNetwork, pNodeB1));
  assert(!network_add_node(pNetwork, pNodeB2));
  assert(!network_add_node(pNetwork, pNodeB3));
  assert(!network_add_node(pNetwork, pNodeB4));
//   assert(!network_add_node(pNetwork, pNodeX1));
//   assert(!network_add_node(pNetwork, pNodeX2));
//   assert(!network_add_node(pNetwork, pNodeX3));
//   assert(!network_add_node(pNetwork, pNodeY1));
//   assert(!network_add_node(pNetwork, pNodeY2));
//   assert(!network_add_node(pNetwork, pNodeK1));
//   assert(!network_add_node(pNetwork, pNodeK2));
//   assert(!network_add_node(pNetwork, pNodeK3));
//   LOG_DEBUG("nodes attached.\n");
  assert(node_add_link(pNodeB1, pNodeB2, 1000, 1) >= 0);
  assert(node_add_link(pNodeB2, pNodeB3, 100, 1) >= 0);
  assert(node_add_link(pNodeB3, pNodeB4, 100, 1) >= 0);
  assert(node_add_link(pNodeB4, pNodeB1, 100, 1) >= 0);
  
//   assert(node_add_link(pNodeB2, pNodeX2, 100, 1) >= 0);
//   assert(node_add_link(pNodeB3, pNodeX2, 100, 1) >= 0);
//   assert(node_add_link(pNodeX2, pNodeX1, 100, 1) >= 0);
//   assert(node_add_link(pNodeX2, pNodeX3, 100, 1) >= 0);
// 
//   assert(node_add_link(pNodeB3, pNodeY1, 100, 1) >= 0);
//   assert(node_add_link(pNodeY1, pNodeY2, 100, 1) >= 0);
  
//   assert(node_add_link(pNodeB4, pNodeK1, 100, 1) >= 0);
//   assert(node_add_link(pNodeB4, pNodeK2, 100, 1) >= 0);
//   assert(node_add_link(pNodeB1, pNodeK3, 100, 1) >= 0);
//   assert(node_add_link(pNodeK3, pNodeK2, 100, 1) >= 0);
  
  assert(node_add_link_toSubnet(pNodeB1, pSubnetTB1, 200, 1) >= 0);
  assert(node_add_link_toSubnet(pNodeB3, pSubnetTB1, 100, 1) >= 0);
//   assert(node_add_link_toSubnet(pNodeX1, pSubnetTX1, 100, 1) >= 0);
//   assert(node_add_link_toSubnet(pNodeX3, pSubnetTX1, 100, 1) >= 0);
//   assert(node_add_link_toSubnet(pNodeB3, pSubnetTY1, 100, 1) >= 0);
//   assert(node_add_link_toSubnet(pNodeY2, pSubnetTY1, 100, 1) >= 0);
//   assert(node_add_link_toSubnet(pNodeK2, pSubnetTK1, 100, 1) >= 0);
//   assert(node_add_link_toSubnet(pNodeK1, pSubnetTK1, 100, 1) >= 0);
//   LOG_DEBUG("transit-network links attached.\n");
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
//   LOG_DEBUG("stub-network links attached.\n");
  LOG_DEBUG(" ok!\n");

  LOG_DEBUG("ospf_djk_test(): CHECK Vertex functions...");
  
  SSptVertex * pV1 = NULL; pV1 = spt_vertex_create_byRouter(pNodeB1, 0);
  assert(pV1 != NULL);
  SSptVertex * pV2 = NULL; pV2 = spt_vertex_create_bySubnet(pSubnetTB1, 0);
  assert(pV2 != NULL);
  
//   spt_vertex_get_links(pV1);
  SPtrArray * pLinks;
  SNetLink * pLink;
  pLinks = spt_vertex_get_links(pV1);
  assert(pLinks != NULL);
  int iIndex;
 /* for (iIndex = 0; iIndex < ptr_array_length(pLinks); iIndex++){
    ptr_array_get_at(pLinks, iIndex, &pLink);
    link_dump(stdout, pLink);
    fprintf(stdout, "\n");
  }*/
  
  pLinks = spt_vertex_get_links(pV2);
  assert(pLinks != NULL);
  for (iIndex = 0; iIndex < ptr_array_length(pLinks); iIndex++){
    ptr_array_get_at(pLinks, iIndex, &pLink);
    link_dump(stdout, pLink);
    fprintf(stdout, "\n");
  }
  
  ip_prefix_dump(stdout, spt_vertex_get_prefix(pV1));
  fprintf(stdout, "\n");
  ip_prefix_dump(stdout, spt_vertex_get_prefix(pV2));
  fprintf(stdout, "\n");
  spt_vertex_destroy(&pV1);
  assert(pV1 == NULL);
  spt_vertex_destroy(&pV2);
  assert(pV2 == NULL);
  LOG_DEBUG(" ok!\n");
  
  LOG_DEBUG("ospf_djk_test(): CHECK Dijkstra function...");
  SPrefix sPrefix; 
  sPrefix.tNetwork = 1024;
  sPrefix.uMaskLen = 22;
  
  SNetNode * pFindNode = network_find_node(pNetwork, pNodeB1->tAddr);
  assert(pFindNode != NULL);
  LOG_DEBUG("ospf_djk_test(): find node is ok...");
  SRadixTree * pSpt = OSPF_dijkstra(pNetwork, pNodeB1->tAddr, sPrefix);
  
  spt_dump(stdout, pSpt, pNodeB1->tAddr);
  radix_tree_destroy(&pSpt);
  
  LOG_DEBUG(" ok!\n");
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
  
  subnet_test();
  LOG_DEBUG("all info OSPF methods tested ... they seem ok!\n");
  
  ospf_info_test();
  LOG_DEBUG("all subnet OSPF methods tested ... they seem ok!\n");
  
//     ospf_rt_test();
//     LOG_DEBUG("all routing table OSPF methods tested ... they seem ok!\n");
  
   ospf_djk_test();
   LOG_DEBUG("all dijkstra OSPF methods tested ... they seem ok!\n");
  return 1;
}

