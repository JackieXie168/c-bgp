// ==================================================================
// @(#)spt_vertex.c
//
// @author Stefano Iasi (stefanoia@tin.it), (C) 2005
// @date 4 July 2005
// @lastdate 4 July 2005
// ==================================================================

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <net/spt_vertex.h>
#include <net/ospf.h>
#include <net/igp_domain.h>
#include <string.h>


//////////////////////////////////////////////////////////////////////////
///////  spt computation: vertex object 
//////////////////////////////////////////////////////////////////////////

// ----- spt_vertex_create_byRouter ---------------------------------------
SSptVertex * spt_vertex_create_byRouter(SNetNode * pNode, net_link_delay_t uIGPweight)
{
  SSptVertex * pVertex = (SSptVertex *) MALLOC(sizeof(SSptVertex));
  pVertex->uDestinationType = NET_LINK_TYPE_ROUTER;
//   (pVertex->UDestId).pNode = pNode;
  pVertex->pObject = pNode;
  pVertex->uIGPweight = uIGPweight;
  pVertex->pNextHops = ospf_nh_list_create();
  pVertex->aSubnets = ptr_array_create(0, NULL, NULL);
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
  pVertex->aSubnets = ptr_array_create(0, NULL, NULL);
  pVertex->fathers = ptr_array_create(ARRAY_OPTION_UNIQUE | ARRAY_OPTION_SORTED, spt_vertex_compare, NULL);
  pVertex->sons = ptr_array_create(ARRAY_OPTION_UNIQUE, spt_vertex_compare, NULL);
  return pVertex;
}

// ----- spt_vertex_create -----------------------------------------------------------------
SSptVertex * spt_vertex_create(SNetwork * pNetwork, SNetLink * pLink, SSptVertex * pVFather)
{
  SSptVertex * pVertex;
  
  if (pLink->uDestinationType == NET_LINK_TYPE_ROUTER){
    SNetNode * pNode;
//     fprintf(stdout, "creo vertex da router\n");
    pNode = network_find_node(pNetwork, link_get_address(pLink));
    assert(pNode != NULL);
    pVertex = spt_vertex_create_byRouter(pNode, pVFather->uIGPweight + pLink->uIGPweight);
  }
  else if (pLink->uDestinationType == NET_LINK_TYPE_TRANSIT ||
           pLink->uDestinationType == NET_LINK_TYPE_STUB){
    SNetSubnet * pSubnet;
//     fprintf(stdout, "creo vertex da subnet\n");
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
    if ((*ppVertex)->aSubnets != NULL)
      ptr_array_destroy(&((*ppVertex)->aSubnets));
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
    return  ((SNetNode *)(pVertex->pObject))->pLinks;
  }
  else{
    return subnet_get_links((SNetSubnet *)(pVertex->pObject));
  }
}

// ----- spt_vertex_belongs_to_area ---------------------------------------
int spt_vertex_belongs_to_area(SSptVertex * pVertex, ospf_area_t tArea)
{
  if (spt_vertex_is_router(pVertex))
    return  node_belongs_to_area(spt_vertex_to_router(pVertex), tArea);
  
  return  subnet_belongs_to_area(spt_vertex_to_subnet(pVertex), tArea);
}

// ----- spt_vertex_get_prefix ---------------------------------------
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
      if (((SNetNode *)(pVertex1->pObject))->tAddr < ((SNetNode *)(pVertex2->pObject))->tAddr)
        return -1;
      else if (((SNetNode *)(pVertex1->pObject))->tAddr > ((SNetNode *)(pVertex2->pObject))->tAddr)
        return 1;
      else
	return 0;
    }
    else {
      SPrefix sPrefV1 = spt_vertex_get_prefix(pVertex1); 
      SPrefix sPrefV2 = spt_vertex_get_prefix(pVertex2); 
      SPrefix * pPrefV1 = &sPrefV1;
      SPrefix * pPrefV2 = &sPrefV2;
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
    
    for (iIndex = 0	; iIndex < ptr_array_length(paGrayVertexes); iIndex++) {
      ptr_array_get_at(paGrayVertexes, iIndex, &pCurrentVertex);
      assert(pCurrentVertex != NULL);
      if (pBestVertex == NULL){
// 	ip_prefix_dump(stdout, spt_vertex_get_prefix(pCurrentVertex));
// 	fprintf(stdout, "\n");
	pBestVertex = pCurrentVertex;
	iBestIndex = iIndex;
      }
      else if (pCurrentVertex->uIGPweight <  pBestVertex->uIGPweight){
// 	ip_prefix_dump(stdout, spt_vertex_get_prefix(pCurrentVertex));
// 	fprintf(stdout, "\n");
	pBestVertex = pCurrentVertex;
	iBestIndex = iIndex;
      }
      else if (pCurrentVertex->uIGPweight == pBestVertex->uIGPweight){ 
        if (pCurrentVertex->uDestinationType > pBestVertex->uDestinationType){ 
// 	  ip_prefix_dump(stdout, spt_vertex_get_prefix(pCurrentVertex));
// 	  fprintf(stdout, "\n");
	  pBestVertex = pCurrentVertex;
	  iBestIndex = iIndex;
	}
      }
   }  
//    fprintf(stdout, "rimuovo da aGrayVertex ...\n");
   ptr_array_remove_at(paGrayVertexes, iBestIndex);
  } 
  return pBestVertex;
}

// ----- spt_vertex_has_father -----------------------------------------------------------
int spt_vertex_has_father(SSptVertex * pParent, SSptVertex * pRoot){
  int iIndex;
  if (ptr_array_sorted_find_index(pParent->fathers, &pRoot, &iIndex) == 0)
    return 1;
  return 0;
}


// ----- calculate_next_hop -----------------------------------------------------------
/**
 * Calculates next hop as explained in RFC2328 (p. 167)
 */
 
//TODO write functions for next hop
void spt_calculate_next_hop(SSptVertex * pRoot, SSptVertex * pParent, 
                                      SSptVertex * pDestination, SNetLink * pLink){
  SOSPFNextHop * pNH = NULL, * pNHCopy = NULL;
  int iLink;

  if (pRoot == pParent){
//     fprintf(stdout, "spt_calculate_next_hop(): parent == root\n");
    pNH = ospf_next_hop_create(pLink, OSPF_NO_IP_NEXT_HOP);
    ospf_nh_list_add(pDestination->pNextHops, pNH);
  }
  else if (pParent->uDestinationType == NET_LINK_TYPE_TRANSIT && 
           spt_vertex_has_father(pParent, pRoot)) {
//     fprintf(stdout, "spt_calculate_next_hop(): parent == transit && fathers contains root\n");
//devo trovare il link verso la subnet in root
// fprintf(stdout, "cerco il link...\n");
    //TODO ma il link non è già nella subnet (oggetto vertex)?
    SNetLink * pLinkToSub = node_find_link_to_subnet(((SNetNode *)pRoot->pObject), ((SNetSubnet *)pParent->pObject));
    
    pNH = ospf_next_hop_create(pLinkToSub, spt_vertex_get_prefix(pDestination).tNetwork);
    ospf_nh_list_add(pDestination->pNextHops, pNH);
  }
  else {//there is at least a router beetween root and destination
//     fprintf(stdout, "spt_calculate_next_hop(): inherited from parent\n");
    for(iLink = 0; iLink < ptr_array_length(pParent->pNextHops); iLink++){
      ptr_array_get_at(pParent->pNextHops, iLink, &pNHCopy);
      pNH = ospf_next_hop_create(pNHCopy->pLink, pNHCopy->tAddr);
      ospf_nh_list_add(pDestination->pNextHops, pNH);
    }
  }
}

// ----- spt_vertex_add_subnet ---------------------------------------------------
int spt_vertex_add_subnet(SSptVertex * pCurrentVertex, SNetLink * pCurrentLink){
  return ptr_array_add(pCurrentVertex->aSubnets, &pCurrentLink);
}

// ----- node_ospf_compute_spt ---------------------------------------------------
/**
 * Compute the Shortest Path Tree from the given source router
 * towards all the other routers that belong to the given prefix.
 * TODO 2. consider ospf domain (set of router)
 *      3. improve computation 
 *
 * PREREQUISITE:
 *      1. igp domain MUST exist
 *      2. ospf areas are setted right
 */
SRadixTree * node_ospf_compute_spt(SNetNode * pNode, uint16_t IGPDomainNumber, ospf_area_t tArea)
{
  SPrefix      sDestPrefix; 
// LOG_DEBUG("entro in OSPF_dijkstra\n");
  int iIndex = 0;
  int iOldVertexPos;
  SNetwork * pNetwork = pNode->pNetwork;
  SIGPDomain * pIGPDomain = get_igp_domain(IGPDomainNumber);
  
//   SNetSubnet * pSubnet = NULL;
  SPtrArray  * aLinks = NULL;
  SNetLink   * pCurrentLink = NULL;
  SSptVertex * pCurrentVertex = NULL, * pNewVertex = NULL;
  SSptVertex * pOldVertex = NULL, * pRootVertex = NULL;
  SRadixTree * pSpt = radix_tree_create(32, spt_vertex_dst);
//   SNetSubnet * pStubSubnet;
  SNetNode * pRootNode = pNode;//network_find_node(pNetwork, tSrcAddr);
  assert(pRootNode != NULL);
  spt_vertex_list_t * aGrayVertexes = ptr_array_create(ARRAY_OPTION_SORTED|ARRAY_OPTION_UNIQUE,
                                                     spt_vertex_compare,
     	    				            NULL);
  //ADD root to SPT
  pRootVertex = spt_vertex_create_byRouter(pRootNode, 0);
  pCurrentVertex = pRootVertex;
  assert(pCurrentVertex != NULL);
  while (pCurrentVertex != NULL) {
     sDestPrefix = spt_vertex_get_prefix(pCurrentVertex);
//     fprintf(stdout, "Current vertex is ");
//     ip_prefix_dump(stdout, sDestPrefix);
//     fprintf(stdout, "\n");
    
    radix_tree_add(pSpt, sDestPrefix.tNetwork, sDestPrefix.uMaskLen, pCurrentVertex);
    
    aLinks = spt_vertex_get_links(pCurrentVertex);
    for (iIndex = 0; iIndex < ptr_array_length(aLinks); iIndex++){
      ptr_array_get_at(aLinks, iIndex, &pCurrentLink);
      
      /* Consider only the links that have the following properties:
	 - NET_LINK_FLAG_UP (the link must be UP)
	 - the end-point belongs to the given prefix */
      if (!(link_get_state(pCurrentLink, NET_LINK_FLAG_UP)))
        continue;

//       debug...
//       fprintf(stdout, "ospf_djk(): current Link is ");
//       link_get_prefix(pCurrentLink, &sDestPrefix);
//       ip_prefix_dump(stdout, sDestPrefix);
//       fprintf(stdout, "\n");

      //first time consider only Transit Link... but we store subnet in a list
      //so we have not to recheck all the links durign routing table computation
      //TODO write macro link_to_subnet , link_is_towards_stub
      if (pCurrentLink->uDestinationType == NET_LINK_TYPE_STUB &&
          subnet_belongs_to_area(link_get_subnet(pCurrentLink), tArea)){
        spt_vertex_add_subnet(pCurrentVertex, pCurrentLink);
        continue;
      }
      
      link_get_prefix(pCurrentLink, &sDestPrefix);
      
      //TODO network should belongs to isis???
      //ROUTER should belongs to ospf domain (check is not performed for network)
       if (sDestPrefix.uMaskLen == 32) {
         if (!igp_domain_contains_router(pIGPDomain, sDestPrefix)) {
//            fprintf(stdout, "trovato router che non appartiene al dominio\n");
//  	  ip_prefix_dump(stdout, sDestPrefix);
//  	  fprintf(stdout, "\n");
           continue;
         }
       } 
      //check if vertex is in spt	
      pOldVertex = (SSptVertex *)radix_tree_get_exact(pSpt, 
                                                          sDestPrefix.tNetwork, 
							  sDestPrefix.uMaskLen);
      
      if(pOldVertex != NULL){
//        fprintf(stdout, "è già nel spt... passo al prossimo\n");
       continue;
      }
      
      //create a vertex object to compare with grayVertex
      pNewVertex = spt_vertex_create(pNetwork, pCurrentLink, pCurrentVertex);
      assert(pNewVertex != NULL);
      
//       fprintf(stdout, "nuovo vertex creato\n");
      //check if vertex is in grayVertexes
      int srchRes = -1;
      if (ptr_array_length(aGrayVertexes) > 0)
        srchRes = ptr_array_sorted_find_index(aGrayVertexes, &pNewVertex, &iOldVertexPos);
      else ;
//         fprintf(stdout, "aGrays è vuoto\n");
      //srchres == -1 ==> item not found
      //srchres == 0  ==> item found
      pOldVertex = NULL;
      if(srchRes == 0) {
//         fprintf(stdout, "vertex è già in aGrayVertexes\n");
        ptr_array_get_at(aGrayVertexes, iOldVertexPos, &pOldVertex);
      }
      
      if (pOldVertex == NULL){
//         fprintf(stdout, "vertex da aggiungere\n");
 	if (spt_vertex_belongs_to_area(pNewVertex, tArea)){
          spt_calculate_next_hop(pRootVertex, pCurrentVertex, pNewVertex, pCurrentLink);
        
	  ptr_array_add(aGrayVertexes, &pNewVertex);
	  //// THIS IS GOOD IF WE WOULD TO STORE SPT (FOR PRINT OR FOR FASTER RECOMPUTATION) - START
	  //set father and sons relationship
  	  ptr_array_add(pCurrentVertex->sons, &pNewVertex);
	  ptr_array_add(pNewVertex->fathers, &pCurrentVertex);
	  //// THIS IS GOOD IF WE WOULD TO STORE SPT (FOR PRINT OR FOR FASTER RECOMPUTATION) - STOP
 	}
 	else
 	  spt_vertex_destroy(&pNewVertex);
	
      }
      else if (pOldVertex->uIGPweight > pNewVertex->uIGPweight) {
//         fprintf(stdout, "vertex da aggiornare (peso minore)\n");
        int iPos, iIndex;
	SSptVertex * pFather;
	pOldVertex->uIGPweight = pNewVertex->uIGPweight;
// 	Remove old next hops
        if (ptr_array_length(pOldVertex->pNextHops) > 0){
          ospf_nh_list_destroy(&(pOldVertex->pNextHops));
 	  pOldVertex->pNextHops = ospf_nh_list_create();
        }
        spt_calculate_next_hop(pRootVertex, pCurrentVertex, pOldVertex, pCurrentLink);
        //// THIS IS GOOD IF WE WOULD TO STORE SPT (FOR PRINT OR FOR FASTER RECOMPUTATION) - START
	//remove old father->sons relationship
	for( iIndex = 0; iIndex < ptr_array_length(pOldVertex->fathers); iIndex++){
	  ptr_array_get_at(pOldVertex->fathers, iIndex, &pFather);
	  assert(ptr_array_sorted_find_index(pFather->sons, &pOldVertex, &iPos) == 0);
	  ptr_array_remove_at(pFather->sons, iPos);
	}
	//remove sons->father relationship
	for( iIndex = 0; iIndex < ptr_array_length(pOldVertex->fathers); iIndex++){
	  ptr_array_remove_at(pOldVertex->fathers, iIndex);
	}
	//set new sons->father and father->sons relationship
        ptr_array_add(pOldVertex->fathers, &pCurrentVertex);
        ptr_array_add(pCurrentVertex->sons, &pOldVertex);
        //// THIS IS GOOD IF WE WOULD TO STORE SPT (FOR PRINT OR FOR FASTER RECOMPUTATION) - STOP
	spt_vertex_destroy(&pNewVertex);
      }
      else if (pOldVertex->uIGPweight == pNewVertex->uIGPweight) {
//         fprintf(stdout, "vertex da aggiornare (peso uguale)\n");
        spt_calculate_next_hop(pRootVertex, pCurrentVertex, pOldVertex, pCurrentLink);
// 	fprintf(stdout, "next hops calcolati\n");
        //// THIS IS GOOD IF WE WOULD TO STORE SPT (FOR PRINT OR FOR FASTER RECOMPUTATION) - START
        ptr_array_add(pOldVertex->fathers, &pCurrentVertex);
        ptr_array_add(pCurrentVertex->sons, &pOldVertex);
	//// THIS IS GOOD IF WE WOULD TO STORE SPT (FOR PRINT OR FOR FASTER RECOMPUTATION) - STOP
        spt_vertex_destroy(&pNewVertex);
      }
      else {
//         fprintf(stdout, "new vertex eliminato\n");
        spt_vertex_destroy(&pNewVertex);
      }  
    }//end for on links
    
    //links for subnet are dinamically created and MUST be removed
    if (pCurrentVertex->uDestinationType == NET_LINK_TYPE_TRANSIT ||
        pCurrentVertex->uDestinationType == NET_LINK_TYPE_STUB)
	ptr_array_destroy(&aLinks);
    //select node with smallest weight to add to spt
    pCurrentVertex = spt_get_best_candidate(aGrayVertexes);
  }
  return pSpt;
}

//------ compute_vertical_bars -------------------------------------------
/**
 *  Compute string to dump a text-based graphics spt
 */
void compute_vertical_bars(FILE * pStream, int iLevel, char * pcBars){
  int iIndex;
//   strcpy(pcPrefix, "");
  strcpy(pcBars, "");
  if (iLevel != 0){
    for (iIndex = 0; iIndex < iLevel; iIndex++)
    	strcat(pcBars, "            |");
//     strcat(pcSpace, "|");
    
  }
}

//------ visit_vertex -------------------------------------------
/**
 *  Visit a node of the spt to dump it
 */
void visit_vertex(SSptVertex * pVertex, int iLevel, int lastChild, char * pcSpace, 
                  SRadixTree * pVisited, FILE * pStream){
  char * pcPrefix = MALLOC(30);
  SPrefix sPrefix, sChildPfx;
  SSptVertex * pChild;
  int iIndex; 
  strcpy(pcSpace, "");
  strcpy(pcPrefix, "");
  compute_vertical_bars(pStream, iLevel, pcSpace);
  

  
  fprintf(pStream, "%s\n%s", pcSpace, pcSpace);
  
  
  sPrefix = spt_vertex_get_prefix(pVertex);
  ip_prefix_to_string(pcPrefix, &sPrefix);
  
  fprintf(pStream, "---> [ %s ]-[ %d ] -- NH >> ", pcPrefix, pVertex->uIGPweight);
  
  if (pVertex->pNextHops != NULL){
//     if (lastChild)
//       pcSpace[strlen(pcSpace) - 1] = ' ';
      for(iIndex = 0; iIndex < strlen(pcPrefix) + 24 + 3; iIndex++) //last term should be dynamic
         strcat(pcSpace, " ");
    ospf_nh_list_dump(pStream, pVertex->pNextHops, pcSpace);
  }
  radix_tree_add(pVisited, sPrefix.tNetwork, sPrefix.uMaskLen, pVertex);
  iLevel++;
  for(iIndex = 0; iIndex < ptr_array_length(pVertex->sons); iIndex++){
     ptr_array_get_at(pVertex->sons, iIndex, &pChild);
     sChildPfx = spt_vertex_get_prefix(pChild);
     if (radix_tree_get_exact(pVisited, sChildPfx.tNetwork, sChildPfx.uMaskLen) != NULL)
       continue;
//      if (iIndex == ptr_array_length(pVertex->sons) - 1)
       visit_vertex(pChild, iLevel, 1, pcSpace, pVisited, pStream);
//      else
//        visit_vertex(pChild, iLevel, 0, pcSpace, pVisited, pStream);
  }
  
  FREE(pcPrefix);
}
#define startAddr 1024

char * ip_to_name(int tAddr){
// const int startAddr = 1024;

 switch(tAddr){
    case startAddr      : return "RT1"; break;
    case startAddr + 1  : return "RT2"; break;
    case startAddr + 2  : return "RT3"; break;
    case startAddr + 3  : return "RT4"; break;
    case startAddr + 4  : return "RT5"; break;
    case startAddr + 5  : return "RT6"; break;
    case startAddr + 6  : return "RT7"; break;
    case startAddr + 7  : return "RT8"; break;
    case startAddr + 8  : return "RT9"; break;
    case startAddr + 9  : return "RT10"; break;
    case startAddr + 10 : return "RT11"; break;
    case startAddr + 11 : return "RT12"; break;
    case 4              : return "N1"; break;
    case 8              : return "N2"; break;
    case 12             : return "N3"; break;
    case 16             : return "N4"; break;
    case 20             : return "N6"; break;
    case 24             : return "N7"; break;
    case 28             : return "N8"; break;
    case 32             : return "N9"; break;
    case 36             : return "N10"; break;
    case 40             : return "N11"; break;
    default             : return "?" ; 
  }
}
//------ visit_vertex -------------------------------------------
/**
 *  Visit a node of the spt to dump it
 */
void visit_vertex_dot(SSptVertex * pVertex, char * pcPfxFather, SRadixTree * pVisited, net_link_delay_t tWFather, FILE * pStream){
  char * pcMyPrefix = MALLOC(30);
  char cNextHop[25] = "", cNextHops[300] = "", * pcNextHops = cNextHops, * pcNH = cNextHop;
  
  SSptVertex * pChild;
  SPrefix sMyPrefix = spt_vertex_get_prefix(pVertex), sChildPfx;
  SOSPFNextHop * pNH;
  
  ip_prefix_to_string(pcMyPrefix, &sMyPrefix);
  int iIndex;
  
  
  //print node
  fprintf(pStream, "\"%s\" -> \"%s\" [label=\"%d\"];\n", pcPfxFather, pcMyPrefix, pVertex->uIGPweight - tWFather);
    
  if (ptr_array_length(pVertex->pNextHops) > 0){
    //print next hop
    for (iIndex = 0; iIndex < ptr_array_length(pVertex->pNextHops); iIndex++){
      ptr_array_get_at(pVertex->pNextHops, iIndex, &pNH);
      ospf_next_hop_to_string(pcNH, pNH);
      strcat(pcNextHops, pcNH);
      strcat(pcNextHops, "\\n");
    }
    fprintf(pStream, "\"%sNH\" [shape = box, label=\"%s\", style=filled, color=\".7 .3 1.0\"] \n", pcMyPrefix, pcNextHops);
    fprintf(pStream, "\"%s\" -> \"%sNH\" [style = dotted];\n", pcMyPrefix, pcMyPrefix);
  }
  
  
  
  radix_tree_add(pVisited, sMyPrefix.tNetwork, sMyPrefix.uMaskLen, pVertex);
  for(iIndex = 0; iIndex < ptr_array_length(pVertex->sons); iIndex++){
     ptr_array_get_at(pVertex->sons, iIndex, &pChild);
     sChildPfx = spt_vertex_get_prefix(pChild);
     if (radix_tree_get_exact(pVisited, sChildPfx.tNetwork, sChildPfx.uMaskLen) != NULL)
       continue;
     visit_vertex_dot(pChild, pcMyPrefix, pVisited, pVertex->uIGPweight, pStream);
  }
  
  FREE(pcMyPrefix);
}

// ----- spt_dump ------------------------------------------
void spt_dump(FILE * pStream, SRadixTree * pSpt, net_addr_t tRadixAddr)
{
  SSptVertex * pRadix = (SSptVertex *)radix_tree_get_exact(pSpt, tRadixAddr, 32);
  char * pcSpace = MALLOC(500);
  SRadixTree * pVisited =  radix_tree_create(32, NULL);
  visit_vertex(pRadix, 0, 1, pcSpace, pVisited, pStream);
}

// ----- spt_dump_dot ------------------------------------------
void spt_dump_dot(FILE * pStream, SRadixTree * pSpt, net_addr_t tRadixAddr)
{
  SSptVertex * pRadix = (SSptVertex *)radix_tree_get_exact(pSpt, tRadixAddr, 32);
//   char * pcSpace = MALLOC(500);
  SPrefix sPref;
  SRadixTree * pVisited =  radix_tree_create(32, NULL);
  sPref.tNetwork = tRadixAddr;
  sPref.uMaskLen = 32;
  char pcRootPrefix[30];
  ip_prefix_to_string(pcRootPrefix, &sPref);
  fprintf(pStream, "digraph G {\n");
  visit_vertex_dot(pRadix, pcRootPrefix, pVisited, 0, pStream);
  fprintf(pStream, "}\n");
}

