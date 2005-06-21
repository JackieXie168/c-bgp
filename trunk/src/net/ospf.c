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
//#include <net/prefix.h>

#define X_AREA 1
#define Y_AREA 2
#define K_AREA 3

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
  SNetwork * pNetwork= network_create();
  SNetNode * pNodeB1= node_create(1);
  SNetNode * pNodeB2= node_create(2);
  SNetNode * pNodeB3= node_create(3);
  SNetNode * pNodeB4= node_create(4);
  SNetNode * pNodeX1= node_create(11);
  SNetNode * pNodeX2= node_create(12);
  SNetNode * pNodeX3= node_create(13);
  SNetNode * pNodeY1= node_create(21);
  SNetNode * pNodeY2= node_create(22);
  SNetNode * pNodeK1= node_create(23);
  SNetNode * pNodeK2= node_create(24);
  SNetNode * pNodeK3= node_create(25);
  
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
  assert(node_add_link_toSubnet(pNodeX3, pSubnetTX1, 100, 0) >= 0);
  assert(node_add_link_toSubnet(pNodeB3, pSubnetTY1, 100, 1) >= 0);
  assert(node_add_link_toSubnet(pNodeY2, pSubnetTY1, 100, 0) >= 0);
  assert(node_add_link_toSubnet(pNodeK2, pSubnetTK1, 100, 1) >= 0);
  assert(node_add_link_toSubnet(pNodeK1, pSubnetTK1, 100, 0) >= 0);
  
  LOG_DEBUG("transit-network links attached.\n");
  
  assert(node_add_link_toSubnet(pNodeB2, pSubnetSB1, 100, 0) >= 0);
  assert(node_add_link_toSubnet(pNodeB1, pSubnetSB2, 100, 0) >= 0);
  assert(node_add_link_toSubnet(pNodeB2, pSubnetSX1, 100, 0) >= 0);
  assert(node_add_link_toSubnet(pNodeX2, pSubnetSX2, 100, 0) >= 0);
  assert(node_add_link_toSubnet(pNodeX3, pSubnetSX3, 100, 0) >= 0);
  assert(node_add_link_toSubnet(pNodeB3, pSubnetSY1, 100, 0) >= 0);
  assert(node_add_link_toSubnet(pNodeY1, pSubnetSY2, 100, 0) >= 0);
  assert(node_add_link_toSubnet(pNodeY1, pSubnetSY3, 100, 0) >= 0);
  assert(node_add_link_toSubnet(pNodeK3, pSubnetSK1, 100, 0) >= 0);
  assert(node_add_link_toSubnet(pNodeK2, pSubnetSK2, 100, 0) >= 0);
  assert(node_add_link_toSubnet(pNodeK2, pSubnetSK3, 100, 0) >= 0);
  
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
  
  ospf_rt_test();
  LOG_DEBUG("all routing table OSPF methods tested ... they seem ok!\n");
  return 1;
}

