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
#include <net/network.h>
#include <net/subnet.h>
#include <net/link.h>

#define X_AREA 1
#define Y_AREA 2
#define K_AREA 3

#define DJK_NODE_TYPE_ROUTER 0
#define DJK_NODE_TYPE_SUBNET    1

//////////////////////////////////////////////////////////////////////////
///////  intra_route_computation
//////////////////////////////////////////////////////////////////////////

/*
   Assumption: pSubNet is valid only for destination type Subnet.
   The filed can be used only if pPrefix has a 32 bit netmask.
*/

typedef struct {
  SNetLink * pLink;
  net_addr_t tAddr;
} SOSPFNextHop;

// ----- ospf_next_hop_create ---------------------------------------
/* A Next Hop is a couple Link + IpAddress.
   IpAddress is used to distinguish beetween more than one node reachable
   towards the same link (typically when link is toward transit network)
*/
SOSPFNextHop * ospf_next_hop_create(SNetLink * pLink, net_addr_t tAddr)
{
  SOSPFNextHop * pNH = (SOSPFNextHop *) MALLOC(sizeof(SOSPFNextHop));
  pNH->pLink = pLink;
  pNH->tAddr = tAddr;
  return pNH;
}

void ospf_next_hop_destroy(SOSPFNextHop ** ppNH)
{
  FREE(*ppNH);
  *ppNH = NULL;
}
//TODO completa next hop function
// ----- ospf_next_hops_compare -----------------------------------------
int ospf_next_hops_compare(void * pItem1, void * pItem2, unsigned int uEltSize)
{
  SNetNode * pNH1= *((SOSPFNextHop **) pItem1);
  SNetNode * pNH2= *((SOSPFNextHop **) pItem2);

  if (pNode1->tAddr < pNode2->tAddr)
    return -1;
  else if (pNode1->tAddr > pNode2->tAddr)
    return 1;
  else
    return 0;
}

next_hop_list_s * ospf_nh_list_create(){
  return ptr_array_create(ARRAY_OPTION_UNIQUE,  ospf_next_hops_compare, NULL);
}




typedef struct {
  uint8_t              uDestinationType;    /* TNET_LINK_TYPE_ROUTER , 
                                               NET_LINK_TYPE_TRANSIT, NET_LINK_TYPE_STUB */
  UDestinationId       UDestId;   
  SDijkNextHop       * aNextHops;
  net_link_delay_t     uIGPweight;  
} SSptVertex;

// ----- dijkstra_info_create ---------------------------------------
SSptVertex * spt_vertex_create_byRouter(SNetNode * Node)
{
  SSptVertex * pVertex = (SSptVertex *) MALLOC(sizeof(SSptVertex));
  
  pInfo->uIGPweight = 0;
  pInfo->aNextHops = NULL;/*ptr_array_create(ARRAY_OPTION_UNIQUE,
				  node_links_compare, 
				  NULL);*/
  return pInfo;
}


typedef struct {
  SPrefix            * pPrefix; 
  SNetSubnet	     * pSubnet;
  //SPtrArray        * aNextHops;  /* list to manage Equal Cost Multi Path */
  SDijkNextHop       * aNextHops;
  net_link_delay_t     uIGPweight;  
} SDijkContext;


typedef struct {
  next_hops_list_s * aNextHops;
  net_link_delay_t   uIGPweight;
} SDijkstraInfo;

// ----- dijkstra_info_create ---------------------------------------
SDijkstraInfo * OSPF_dijkstra_info_create(net_link_delay_t uIGPweight)
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
}

// ----- dijkstra ---------------------------------------------------
/**
 * Compute the Shortest Path Tree from the given source router
 * towards all the other routers that belong to the given prefix.
 */
SRadixTree * OSPF_dijkstra(SNetwork * pNetwork, net_addr_t tSrcAddr,
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
  pContext->aNextHops = NULL;/*ptr_array_create(ARRAY_OPTION_UNIQUE,
				  ip_prefixes_compare, 
				  ip_prefixes_destroy);*/
  pContext->uIGPweight = 0;
  
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
       /*Consider only the links that have the following properties:
	 - NET_LINK_FLAG_IGP_ADV
	 - NET_LINK_FLAG_UP (the link must be UP)
	 - the end-point belongs to the given prefix */
       /*if (link_get_state(pLink, NET_LINK_FLAG_IGP_ADV) &&
	   link_get_state(pLink, NET_LINK_FLAG_UP) &&
	   ((!NET_OPTIONS_IGP_INTER && ip_address_in_prefix(link_get_addr(pLink), sPrefix)) ||
	   (NET_OPTIONS_IGP_INTER && ip_address_in_prefix(pNode->tAddr, sPrefix)))) {*/
	
	   
	uint32_t uNetmaskLen;
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
	 /* if (pOldContext->tNextHop == tSrcAddr) {
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
	}
	
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
}




//////////////////////////////////////////////////////////////////////////
///////  route info list method
//////////////////////////////////////////////////////////////////////////

// ----- OSPF_rt_route_info_destroy --------------------------------------
void OSPF_rt_route_info_destroy(void ** ppItem)
{
  OSPF_route_info_destroy((SOSPFRouteInfo **) ppItem);
}

// ----- OSPF_rt_info_list_cmp -------------------------------------------
/**
 * Compare two routes towards the same destination
 * - prefer the route with the lowest type (STATIC > IGP > BGP)
 * - prefer the route with the lowest metric
 * - prefer the route with the lowest next-hop address
 */
int OSPF_rt_info_list_cmp(void * pItem1, void * pItem2, unsigned int uEltSize)
{
  SOSPFRouteInfo * pRI1= *((SOSPFRouteInfo **) pItem1);
  SOSPFRouteInfo * pRI2= *((SOSPFRouteInfo **) pItem2);

  // Prefer lowest routing protocol type STATIC > IGP > BGP
  if (pRI1->tType > pRI2->tType)
    return 1;
  if (pRI1->tType < pRI2->tType)
    return -1;

  // Prefer route with lowest metric
  if (pRI1->uWeight > pRI2->uWeight)
    return 1;
  if (pRI1->uWeight < pRI2->uWeight)
    return -1;

  // Tie-break: prefer route with lowest next-hop address
  /*if (link_get_addr(pRI1->pNextHopIf) > link_get_addr(pRI2->pNextHopIf))
    return 1;
  if (link_get_addr(pRI1->pNextHopIf) < link_get_addr(pRI2->pNextHopIf))
    return- 1;*/
  
  return 0;
}

// ----- OSPF_rt_info_list_dst -------------------------------------------
void OSPF_rt_info_list_dst(void * pItem)
{
  SOSPFRouteInfo * pRI= *((SOSPFRouteInfo **) pItem);

  OSPF_route_info_destroy(&pRI);
}

// ----- OSPF_rt_info_list_create ----------------------------------------
SOSPFRouteInfoList * OSPF_rt_info_list_create()
{
  return (SOSPFRouteInfoList *) ptr_array_create(ARRAY_OPTION_SORTED|
						ARRAY_OPTION_UNIQUE,
						OSPF_rt_info_list_cmp,
						OSPF_rt_info_list_dst);
}

// ----- OSPF_rt_info_list_destroy ---------------------------------------
void OSPF_rt_info_list_destroy(SOSPFRouteInfoList ** ppRouteInfoList)
{
  ptr_array_destroy((SPtrArray **) ppRouteInfoList);
}

// ----- OSPF_rt_info_list_length ----------------------------------------
int OSPF_rt_info_list_length(SOSPFRouteInfoList * pRouteInfoList)
{
  return _array_length((SArray *) pRouteInfoList);
}

// ----- rt_info_list_get -------------------------------------------
SOSPFRouteInfo * OSPF_rt_info_list_get(SOSPFRouteInfoList * pRouteInfoList,
				 int iIndex)
{
  if (iIndex < ptr_array_length((SPtrArray *) pRouteInfoList))
    return pRouteInfoList->data[iIndex];
  return NULL;
}

// ----- OSPF_rt_info_list_add -------------------------------------------
int OSPF_rt_info_list_add(SOSPFRouteInfoList * pRouteInfoList,
			SOSPFRouteInfo * pRouteInfo)
{
  if (ptr_array_add((SPtrArray *) pRouteInfoList,
		    &pRouteInfo)) {
    return NET_RT_ERROR_ADD_DUP;
  }
  return NET_RT_SUCCESS;
}



//////////////////////////////////////////////////////////////////////////
/////// OSPF methods for routing table
//////////////////////////////////////////////////////////////////////////
//
// ----- OSPF_nextHops_compare -----------------------------------------
/*int OSPF_nextHops_compare(void * pItem1, void * pItem2,
		       unsigned int uEltSize)
{
  SNetLink * pLink1= *((SNetLink **) pItem1);
  SNetLink * pLink2= *((SNetLink **) pItem2);

  if (link_get_addr(pLink1) < link_get_addr(pLink2))
    return -1;
  else if (link_get_addr(pLink1) > link_get_addr(pLink2))
    return 1;
  else
    return 0;
}*/

// ----- OSPF_route_info_create -----------------------------------------------
/**
 *ospf_dest_type_t  uOSPFDestinationType;
  SPrefix           sPrefix;
  uint32_t          uWeight;
  ospf_area_t       uOSPFArea;
  ospf_path_type_t  uOSPFPathType;
  SPtrArray *       pNextHops;
  net_route_type_t  tType 
 */
SOSPFRouteInfo * OSPF_route_info_create(ospf_dest_type_t tOSPFDestinationType,
                                       SPrefix           sPrefix,
				       uint32_t          uWeight,
				       ospf_area_t       tOSPFArea,
				       ospf_path_type_t  tOSPFPathType,
				       SNetLink *        pNextHopIf)
{
  SOSPFRouteInfo * pRouteInfo=
    (SOSPFRouteInfo *) MALLOC(sizeof(SOSPFRouteInfo));
  
  pRouteInfo->tOSPFDestinationType = tOSPFDestinationType;
  pRouteInfo->sPrefix= sPrefix;
  pRouteInfo->uWeight= uWeight;
  pRouteInfo->tOSPFArea = tOSPFArea;
  pRouteInfo->tType= NET_ROUTE_IGP;
  
  pRouteInfo->aNextHops= ptr_array_create(ARRAY_OPTION_UNIQUE,
				  node_links_compare, //TODO sure?
				  NULL);
  
  if (ptr_array_add(pRouteInfo->aNextHops, &pNextHopIf) < 0)
   return NULL;
  
  return pRouteInfo;
  
}

// ----- route_info_destroy -----------------------------------------
int OSPF_route_info_add_nextHop(SOSPFRouteInfo * pRouteInfo, SNetLink * pNextHopIf) 
{
  return ptr_array_add(pRouteInfo->aNextHops, &pNextHopIf);
}

// ----- OSPF_route_info_destroy -----------------------------------------
/**
 *
 */
void OSPF_route_info_destroy(SOSPFRouteInfo ** ppOSPFRouteInfo)
{
  if (*ppOSPFRouteInfo != NULL) {
    if ((*ppOSPFRouteInfo)->aNextHops != NULL)
      ptr_array_destroy(&((*ppOSPFRouteInfo)->aNextHops));
    FREE(*ppOSPFRouteInfo);
    *ppOSPFRouteInfo= NULL;
  }
}

// ----- OSPF_dest_type_dump -----------------------------------------
void OSPF_dest_type_dump(FILE * pStream, ospf_dest_type_t tDestType)
{
  switch (tDestType) {
    case OSPF_DESTINATION_TYPE_NETWORK : 
           fprintf(pStream, "NETWORK");
	   break;
    case OSPF_DESTINATION_TYPE_ROUTER : 
           fprintf(pStream, "NETWORK");
	   break;
    default : 
           fprintf(pStream, "???");
  }
}

// ----- OSPF_area_dump -----------------------------------------
void OSPF_area_dump(FILE * pStream, ospf_area_t tOSPFArea)
{
  if (tOSPFArea == 0)
    fprintf(pStream,"BACKBONE");
  else
    fprintf(pStream,"%d", tOSPFArea);
}

// ----- OSPF_path_type_dump -----------------------------------------
void OSPF_path_type_dump(FILE * pStream, ospf_path_type_t tOSPFPathType)
{
  switch (tOSPFPathType) {
    case OSPF_PATH_TYPE_INTRA : 
           fprintf(pStream, "INTRA");
	   break;
    case OSPF_PATH_TYPE_INTER : 
           fprintf(pStream, "INTER");
	   break;
    case OSPF_PATH_TYPE_EXTERNAL_1 : 
           fprintf(pStream, "EXT1");
	   break;
    case OSPF_PATH_TYPE_EXTERNAL_2 : 
           fprintf(pStream, "EXT2");
	   break;
    default : 
           fprintf(pStream, "???");
  }
}

// ----- OSPF_route_type_dump --------------------------------------------
/**
 *
 */
void OSPF_route_type_dump(FILE * pStream, net_route_type_t tType)
{
  switch (tType) {
  case NET_ROUTE_STATIC:
    fprintf(pStream, "STATIC");
    break;
  case NET_ROUTE_IGP:
    fprintf(pStream, "IGP");
    break;
  case NET_ROUTE_BGP:
    fprintf(pStream, "BGP");
    break;
  default:
    fprintf(pStream, "???");
  }
}

typedef struct { 
  FILE *   pStream;
  uint32_t uCount;
} SNextHop_Dump_Context;

// ----- OSPF_next_hop_dump --------------------------------------------
int OSPF_next_hop_dump(void * pItem, void * pContext)
{ 
  SNextHop_Dump_Context * pCtxt = (SNextHop_Dump_Context *) pContext;
  SNetLink * pNextHop = *((SNetLink **) pItem);
  pCtxt->uCount++;
  if (pCtxt->uCount > 1)
    fprintf(pCtxt->pStream,"...\t\t\t\t\t\t");
  ip_address_dump(pCtxt->pStream, link_get_addr(pNextHop));
  if (!link_get_state(pNextHop, NET_LINK_FLAG_UP)) 
    fprintf(pCtxt->pStream, "\t[DOWN]\n");
  else
    fprintf(pCtxt->pStream, "\n");
  return 0;
}

// ----- OSPF_rt_find_exact ----------------------------------------------
/**
 * Find the route that exactly matches the given prefix. If a
 * particular route type is given, returns only the route with the
 * requested type.
 *
 * Parameters:
 * - routing table
 * - prefix
 * - route type (can be NET_ROUTE_ANY if any type is ok)
 */
SOSPFRouteInfo * OSPF_rt_find_exact(SOSPFRT * pRT, SPrefix sPrefix,
			      net_route_type_t tType)
{
  SOSPFRouteInfoList * pRIList;
  int iIndex;
  SOSPFRouteInfo * pRouteInfo;

  /* First, retrieve the list of routes that exactly match the given
     prefix */
#ifdef __EXPERIMENTAL__
  pRIList= (SNetRouteInfoList *)
    trie_find_exact((STrie *) pRT, sPrefix.tNetwork, sPrefix.uMaskLen);
#else
  pRIList= (SNetRouteInfoList *)
    radix_tree_get_exact((SRadixTree *) pRT, sPrefix.tNetwork,
			 sPrefix.uMaskLen);
#endif

  /* Then, select the first returned route that matches the given
     route-type (if requested) */
  if (pRIList != NULL) {

    assert(OSPF_rt_info_list_length(pRIList) != 0);

    for (iIndex= 0; iIndex < ptr_array_length(pRIList); iIndex++) {
      pRouteInfo= OSPF_rt_info_list_get(pRIList, iIndex);
      if (pRouteInfo->tType & tType)
	return pRouteInfo;
    }
  }

  return NULL;
}

// ----- OSPF_route_info_dump --------------------------------------------
/**
 * Output format:
 * <dst-prefix> <link/if> <weight> <type> [ <state> ]
 */
void OSPF_route_info_dump(FILE * pStream, SOSPFRouteInfo * pRouteInfo)
{
  OSPF_dest_type_dump(pStream, pRouteInfo->tOSPFDestinationType);
  fprintf(pStream, "\t");
  ip_prefix_dump(pStream, pRouteInfo->sPrefix);
  fprintf(pStream, "\t");
  OSPF_area_dump(pStream, pRouteInfo->tOSPFArea);
  fprintf(pStream, "\t");
  OSPF_path_type_dump(pStream, pRouteInfo->tOSPFPathType);
  fprintf(pStream, "\t");
  OSPF_route_type_dump(pStream, pRouteInfo->tType);
  fprintf(pStream, "\t%u\t", pRouteInfo->uWeight);
  SNextHop_Dump_Context * pContext = MALLOC(sizeof(SNextHop_Dump_Context));
  pContext->pStream = pStream;
  pContext->uCount = 0;
  if (_array_for_each((SArray *) (pRouteInfo->aNextHops), OSPF_next_hop_dump, pContext))
    fprintf(pStream, "\tNEXT HOP DUMP ERROR");
}


// ----- OSPF_rt_il_dst --------------------------------------------------
void OSPF_rt_il_dst(void ** ppItem)
{
  OSPF_rt_info_list_destroy((SOSPFRouteInfoList **) ppItem);
}

// ----- OSPF_rt_create --------------------------------------------------
/**
 *
 */
SOSPFRT * OSPF_rt_create()
{
#ifdef __EXPERIMENTAL__
  return (SOSPFRT *) trie_create(OSPF_rt_il_dst);
#else
  return (SOSPFRT *) radix_tree_create(32, OSPF_rt_il_dst);
#endif
}

// ----- OSPF_rt_destroy -------------------------------------------------
/**
 *
 */
void OSPF_rt_destroy(SOSPFRT ** ppRT)
{
#ifdef __EXPERIMENTAL__
  trie_destroy((STrie **) ppRT);
#else
  radix_tree_destroy((SRadixTree **) ppRT);
#endif
}


// ----- OSPF_rt_add_route -----------------------------------------------
/**
 *
 */
int OSPF_rt_add_route(SOSPFRT * pRT, SPrefix sPrefix,
		 SOSPFRouteInfo * pRouteInfo)
{
  SOSPFRouteInfoList * pRIList;

#ifdef __EXPERIMENTAL__
  pRIList=
    (SOSPFRouteInfoList *) trie_find_exact((STrie *) pRT,
					  sPrefix.tNetwork,
					  sPrefix.uMaskLen);
#else
  pRIList=
    (SOSPFRouteInfoList *) radix_tree_get_exact((SRadixTree *) pRT,
					       sPrefix.tNetwork,
					       sPrefix.uMaskLen);
#endif

  if (pRIList == NULL) {
    pRIList= OSPF_rt_info_list_create();
#ifdef __EXPERIMENTAL__
    trie_insert((STrie *) pRT, sPrefix.tNetwork, sPrefix.uMaskLen, pRIList);
#else
    radix_tree_add((SRadixTree *) pRT, sPrefix.tNetwork,
		   sPrefix.uMaskLen, pRIList);
#endif
  }

  return OSPF_rt_info_list_add(pRIList, pRouteInfo);
}



// ----- OSPF_rt_perror -------------------------------------------------------
/**
 *
 */
void OSPF_rt_perror(FILE * pStream, int iErrorCode)
{
  switch (iErrorCode) {
  case NET_RT_SUCCESS:
    fprintf(pStream, "Success"); break;
  case NET_RT_ERROR_NH_UNREACH:
    fprintf(pStream, "Next-Hop is unreachable"); break;
  case NET_RT_ERROR_IF_UNKNOWN:
    fprintf(pStream, "Interface is unknown"); break;
  case NET_RT_ERROR_ADD_DUP:
    fprintf(pStream, "Route already exists"); break;
  case NET_RT_ERROR_DEL_UNEXISTING:
    fprintf(pStream, "Route does not exist"); break;
  default:
    fprintf(pStream, "Unknown error");
  }
}

// ----- OSPF_rt_info_list_dump ------------------------------------------
void OSPF_rt_info_list_dump(FILE * pStream, SPrefix sPrefix,
		       SOSPFRouteInfoList * pRouteInfoList)
{
  int iIndex;

  if (OSPF_rt_info_list_length(pRouteInfoList) == 0) {

    fprintf(pStream, "\033[1;31mERROR: empty info-list for ");
    ip_prefix_dump(pStream, sPrefix);
    fprintf(pStream, "\033[0m\n");
    abort();

  } else {
    for (iIndex= 0; iIndex < ptr_array_length((SPtrArray *) pRouteInfoList);
	 iIndex++) {
      //ip_prefix_dump(pStream, sPrefix);
      //fprintf(pStream, "\t");
      OSPF_route_info_dump(pStream, (SOSPFRouteInfo *) pRouteInfoList->data[iIndex]);
      fprintf(pStream, "\n");
    }
  }
}

// ----- OSPF_rt_dump_for_each -------------------------------------------
int OSPF_rt_dump_for_each(uint32_t uKey, uint8_t uKeyLen, void * pItem,
		     void * pContext)
{
  FILE * pStream= (FILE *) pContext;
  SNetRouteInfoList * pRIList= (SNetRouteInfoList *) pItem;
  SPrefix sPrefix;

  sPrefix.tNetwork= uKey;
  sPrefix.uMaskLen= uKeyLen;
  
  OSPF_rt_info_list_dump(pStream, sPrefix, pRIList);
  return 0;
}

// ----- OSPF_rt_dump ----------------------------------------------------
/**
 * Dump the routing table for the given destination. The destination
 * can be of type NET_DEST_ANY, NET_DEST_ADDRESS and NET_DEST_PREFIX.
 */
void OSPF_rt_dump(FILE * pStream, SOSPFRT * pRT)
{

#ifdef __EXPERIMENTAL__
    trie_for_each((STrie *) pRT, OSPF_rt_dump_for_each, pStream);
#else
    radix_tree_for_each((SRadixTree *) pRT, OSPF_rt_dump_for_each, pStream);
#endif
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
  
  SNetSubnet * pSubnetTB1= subnet_create_byAddr(4, 30, NET_SUBNET_TYPE_TRANSIT);
  SNetSubnet * pSubnetTB2= subnet_create_byAddr(8, 30, NET_SUBNET_TYPE_TRANSIT);
  
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
  /*TODO meglio aggiungere a Link un puntatore al nodo: verrà usato nella procedura
         che linka il nodo alla subnet.
	 
	 cicla con ptr_array_get_at
	 prendi il link e aggiungi in array
	 
	 potresti scrivere una funzione che prende in un parametro la posizione
	 del link e lo restituisce...
	 */
  //subnet_get_links(pSubnetTB1);
  return 1;
}

// ----- ospf_rt_test() -----------------------------------------------
int ospf_rt_test(){
  
  SOSPFRT * pRt= OSPF_rt_create();
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
  
  return 1;
}

// ----- test_ospf -----------------------------------------------
int ospf_test()
{
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
  
  SNetSubnet * pSubnetTB1= subnet_create_byAddr(4, 30, NET_SUBNET_TYPE_TRANSIT);
  SNetSubnet * pSubnetTX1= subnet_create_byAddr(8, 30, NET_SUBNET_TYPE_TRANSIT);
  SNetSubnet * pSubnetTY1= subnet_create_byAddr(12, 30, NET_SUBNET_TYPE_TRANSIT);
  SNetSubnet * pSubnetTK1= subnet_create_byAddr(16, 30, NET_SUBNET_TYPE_TRANSIT);
  
  SNetSubnet * pSubnetSB1= subnet_create_byAddr(20, 30, NET_SUBNET_TYPE_STUB);
  SNetSubnet * pSubnetSB2= subnet_create_byAddr(24, 30, NET_SUBNET_TYPE_STUB);
  SNetSubnet * pSubnetSX1= subnet_create_byAddr(28, 30, NET_SUBNET_TYPE_STUB);
  SNetSubnet * pSubnetSX2= subnet_create_byAddr(32, 30, NET_SUBNET_TYPE_STUB);
  SNetSubnet * pSubnetSX3= subnet_create_byAddr(36, 30, NET_SUBNET_TYPE_STUB);
  SNetSubnet * pSubnetSY1= subnet_create_byAddr(40, 30, NET_SUBNET_TYPE_STUB);
  SNetSubnet * pSubnetSY2= subnet_create_byAddr(44, 30, NET_SUBNET_TYPE_STUB);
  SNetSubnet * pSubnetSY3= subnet_create_byAddr(48, 30, NET_SUBNET_TYPE_STUB);
  SNetSubnet * pSubnetSK1= subnet_create_byAddr(52, 30, NET_SUBNET_TYPE_STUB);
  SNetSubnet * pSubnetSK2= subnet_create_byAddr(56, 30, NET_SUBNET_TYPE_STUB);
  SNetSubnet * pSubnetSK3= subnet_create_byAddr(60, 30, NET_SUBNET_TYPE_STUB);
  
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

  LOG_DEBUG("all subnet OSPF methods tested ... they seem ok!\n");
  
  //ospf_rt_test();
  //LOG_DEBUG("all routing table OSPF methods tested ... they seem ok!\n");
  
  ospf_djk_test();
  
  LOG_DEBUG("all dijkstra OSPF methods tested ... they seem ok!\n");
  return 1;
}

