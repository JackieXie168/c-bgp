// ==================================================================
// @(#)ospf_rt.c
//
// @author Stefano Iasi (stefanoia@tin.it)
// @date 14/06/2005
// @lastdate 14/05/2005
// ==================================================================

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <net/ospf_rt.h>
#include <net/routing_t.h>
#include <net/prefix.h>
#include <assert.h>

/*only for test function*/
#include <net/network.h>


#define NET_OSPF_RT_SUCCESS               0
#define NET_OSPF_RT_ERROR_NH_UNREACH     -1
#define NET_OSPF_RT_ERROR_IF_UNKNOWN     -2
#define NET_OSPF_RT_ERROR_ADD_DUP        -3
#define NET_OSPF_RT_ERROR_DEL_UNEXISTING -4

//////////////////////////////////////////////////////////////////////////
///////  next hop object & next hop list object
//////////////////////////////////////////////////////////////////////////
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
  if (*ppNH != NULL){
    FREE(*ppNH);
    *ppNH = NULL;
  }
}

// ----- ospf_next_hops_compare -----------------------------------------------
int ospf_next_hops_compare(void * pItem1, void * pItem2, unsigned int uEltSize)
{
  SOSPFNextHop * pNH1= *((SOSPFNextHop **) pItem1);
  SOSPFNextHop * pNH2= *((SOSPFNextHop **) pItem2);
  SPrefix pL1Prefix; //= MALLOC(sizeof(SPrefix));
  SPrefix pL2Prefix; //= MALLOC(sizeof(SPrefix));
  link_get_prefix(pNH1->pLink, &pL1Prefix);
  link_get_prefix(pNH2->pLink, &pL2Prefix);
  int prefixCmp = ip_prefixes_compare(&pL2Prefix, &pL2Prefix, 0);
  if (prefixCmp == 0)
    if (pNH1->tAddr > pNH2->tAddr)
      return 1;
    else if (pNH1->tAddr < pNH2->tAddr)
      return -1;
    else 
      return  0;
  else
    return prefixCmp;     
}

// ----- ospf_next_hop_dst -----------------------------------------
void ospf_next_hop_dst(void * pItem)
{
  ospf_next_hop_destroy((SOSPFNextHop **) pItem);
}

// ----- OSPF_next_hop_dump --------------------------------------------
void ospf_next_hop_dump(FILE* pStream, SOSPFNextHop * pNH)
{ 
  SPrefix sPrefix;
  fprintf(pStream, "IF <");
  link_get_prefix(pNH->pLink, &sPrefix);
  ip_prefix_dump(stdout, sPrefix);
  fprintf(pStream, "> IP <");
  if (pNH->tAddr != OSPF_NO_IP_NEXT_HOP)
    ip_address_dump(pStream, pNH->tAddr);
  else
    fprintf(pStream, "-DIRECT-");
  fprintf(pStream, ">");
}

// ----- ospf_nh_list_length --------------------------------------------
int ospf_nh_list_length(next_hops_list_t * pNHList)
{
  return ptr_array_length(pNHList);
}

// ----- ospf_nh_list_get_at --------------------------------------------------
void ospf_nh_list_get_at(next_hops_list_t * pNHList, int iIndex, SOSPFNextHop ** ppNH)
{
  ptr_array_get_at(pNHList, iIndex, ppNH);
} 
/*void ospf_next_hop_dump(SOSPFNextHop * pNH)
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
}*/

// ----- ospf_nh_list_create --------------------------------------------------
next_hops_list_t * ospf_nh_list_create()
{
  return ptr_array_create(ARRAY_OPTION_UNIQUE, ospf_next_hops_compare, 
                                               ospf_next_hop_dst);
}

// ----- ospf_nh_list_destroy --------------------------------------------------
void ospf_nh_list_destroy(next_hops_list_t ** pNHList)
{
  ptr_array_destroy((SPtrArray **) pNHList);
}

// ----- ospf_nh_list_add --------------------------------------------------
int ospf_nh_list_add(next_hops_list_t * pNHList, SOSPFNextHop * pNH)
{
  return ptr_array_add(pNHList, &pNH);
}

// ----- ospf_nh_list_dump --------------------------------------------------
/*
   pcSpace should not be NULL! 
   USAGE pcSapace = "" or 
         pcSpace = "\t"
*/
void ospf_nh_list_dump(FILE * pStream, next_hops_list_t * pNHList, char * pcSpace)
{
  int iIndex;
  SOSPFNextHop  sNH, * pNH;
  pNH = &sNH;
  for (iIndex = 0; iIndex < ptr_array_length(pNHList); iIndex++)
  {
    ptr_array_get_at(pNHList, iIndex, &pNH);
    ospf_next_hop_dump(pStream, pNH);
    fprintf(pStream, "\n%s", pcSpace);
  }
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
 * [for details see chap. 11 of RFC2328]
 TODO 
 * - prefer the route with the lowest type (STATIC > IGP > BGP)
 * - prefer route with smallest area ?????? 
 * - prefer route with smallest path-thype (INTRA < INTER < EXT1 < EXT2)
 * - prefer route with smallest cost
 * - prefer route with smallest next-hop
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
    return NET_OSPF_RT_ERROR_ADD_DUP;
  }
  return NET_OSPF_RT_SUCCESS;
}



//////////////////////////////////////////////////////////////////////////
/////// OSPF methods for routing table
//////////////////////////////////////////////////////////////////////////

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
SOSPFRouteInfo * OSPF_route_info_create(ospf_dest_type_t  tOSPFDestinationType,
                                       SPrefix            sPrefix,
				       uint32_t           uWeight,
				       ospf_area_t        tOSPFArea,
				       ospf_path_type_t   tOSPFPathType,
				       SOSPFNextHop     * pNextHop)
{
  SOSPFRouteInfo * pRouteInfo=
    (SOSPFRouteInfo *) MALLOC(sizeof(SOSPFRouteInfo));
  
  pRouteInfo->tOSPFDestinationType = tOSPFDestinationType;
  pRouteInfo->sPrefix              = sPrefix;
  pRouteInfo->uWeight              = uWeight;
  pRouteInfo->tOSPFArea            = tOSPFArea;
  pRouteInfo->tType                = NET_ROUTE_IGP;
  
  pRouteInfo->aNextHops = ospf_nh_list_create();
  
  if (ospf_nh_list_add(pRouteInfo->aNextHops, pNextHop) < 0){
   ospf_nh_list_destroy(&(pRouteInfo->aNextHops));
   return NULL;
  }
  
  return pRouteInfo;
}

// ----- route_info_destroy -----------------------------------------
int OSPF_route_info_add_nextHop(SOSPFRouteInfo * pRouteInfo, SOSPFNextHop * pNH) 
{
  return ospf_nh_list_add(pRouteInfo->aNextHops, pNH);
}

// ----- OSPF_route_info_destroy -----------------------------------------
/**
 *
 */
void OSPF_route_info_destroy(SOSPFRouteInfo ** ppOSPFRouteInfo)
{
  if (*ppOSPFRouteInfo != NULL) {
    if ((*ppOSPFRouteInfo)->aNextHops != NULL){
      LOG_DEBUG("nh list destroy in route info\n");
      ospf_nh_list_destroy(&((*ppOSPFRouteInfo)->aNextHops));
      LOG_DEBUG("nh list destroy in route info\n");
    }
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
  pRIList= (SOSPFRouteInfoList *)
    trie_find_exact((STrie *) pRT, sPrefix.tNetwork, sPrefix.uMaskLen);
#else
  pRIList= (SOSPFRouteInfoList *)
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
  ospf_nh_list_dump(stdout, pRouteInfo->aNextHops, "\t\t\t\t\t\t");
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
  case NET_OSPF_RT_SUCCESS:
    fprintf(pStream, "Success"); break;
  case NET_OSPF_RT_ERROR_NH_UNREACH:
    fprintf(pStream, "Next-Hop is unreachable"); break;
  case NET_OSPF_RT_ERROR_IF_UNKNOWN:
    fprintf(pStream, "Interface is unknown"); break;
  case NET_OSPF_RT_ERROR_ADD_DUP:
    fprintf(pStream, "Route already exists"); break;
  case NET_OSPF_RT_ERROR_DEL_UNEXISTING:
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
  SOSPFRouteInfoList * pRIList= (SOSPFRouteInfoList *) pItem;
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
  
  SNetSubnet * pSubTx = subnet_create(sSubnetPfxTx.tNetwork,
                                              sSubnetPfxTx.uMaskLen, NET_SUBNET_TYPE_TRANSIT);
//   SNetSubnet * pSubTx1 = subnet_create(sSubnetPfxTx1.tNetwork,
//                                                sSubnetPfxTx.uMaskLen,
// 					       NET_SUBNET_TYPE_TRANSIT);
  //subnet_dump(stdout, pSubTx);
  //subnet_dump(stdout, pSubTx1);
 
  //Small network for test: 3 router A,B,C on a subnet Tx
  assert(node_add_link_toSubnet(pNodeA, pSubTx, 100, 1) >= 0);
  assert(node_add_link_toSubnet(pNodeB, pSubTx, 100, 1) >= 0);
  assert(node_add_link_toSubnet(pNodeC, pSubTx, 300, 1) >= 0);
  assert(node_add_link(pNodeA, pNodeC, 100, 1) >= 0);
  
  //CHECK node_find_link
  SNetLink * pLinkAS = node_find_link_to_subnet(pNodeA, pSubTx);
  assert(pLinkAS != NULL);
  SNetLink * pLinkAC = node_find_link_to_router(pNodeA, tAddrC);
  assert(pLinkAC != NULL);
  
  //CHECK next hop capability
  SOSPFNextHop * pNHB = ospf_next_hop_create(pLinkAS, tAddrB);
  assert(pNHB != NULL);
  assert(pNHB->pLink == pLinkAS && pNHB->tAddr == tAddrB);
  SOSPFNextHop * pNHC1 = ospf_next_hop_create(pLinkAS, tAddrC);
  assert(pNHB != NULL);
  SOSPFNextHop * pNHC2 = ospf_next_hop_create(pLinkAC, 0);
  assert(pNHB != NULL);
  SOSPFNextHop * pNHTx = ospf_next_hop_create(pLinkAS, 0);
  assert(pNHTx != NULL);
  
  //CHECK next hop list
  next_hops_list_t * pNHList = ospf_nh_list_create();
  assert(pNHList != NULL);
  
  assert(ospf_nh_list_add(pNHList, pNHC1) >= 0);
  assert(ospf_nh_list_add(pNHList, pNHC2) >= 0);
//   ospf_nh_list_dump(stdout, pNHList, "");
  
  //CHECK route info functions
  SPrefix sPrefixB, sPrefixC;
  sPrefixB.tNetwork = tAddrB;
  sPrefixB.uMaskLen = 32;
  sPrefixC.tNetwork = tAddrC;
  sPrefixC.uMaskLen = 32;
  SOSPFRouteInfo * pRiB = NULL; 
  pRiB = OSPF_route_info_create(OSPF_DESTINATION_TYPE_ROUTER,
                                       sPrefixB,
				       100,
				       BACKBONE_AREA,
				       OSPF_PATH_TYPE_INTRA ,
				       pNHB);
  assert(pRiB != NULL);      
  SOSPFRouteInfo * pRiC = NULL; 
  pRiC = OSPF_route_info_create(OSPF_DESTINATION_TYPE_ROUTER,
                                       sPrefixC,
				       100,
				       BACKBONE_AREA,
				       OSPF_PATH_TYPE_INTRA ,
				       pNHC1);	       
       
  SOSPFRouteInfo * pRiTx = NULL; 
  pRiTx = OSPF_route_info_create(OSPF_DESTINATION_TYPE_NETWORK,
                                       sSubnetPfxTx,
				       100,
				       BACKBONE_AREA,
				       OSPF_PATH_TYPE_INTRA ,
				       pNHTx);
  assert(pRiTx != NULL);  
   
  assert(OSPF_route_info_add_nextHop(pRiC, pNHC2) >= 0);
//   OSPF_route_info_dump(stdout, pRiB);
//   OSPF_route_info_dump(stdout, pRiC);
  
  //CHECK routing table function
  SOSPFRT * pRT = OSPF_rt_create();
  assert(pRT != NULL);
  
  assert(OSPF_rt_add_route(pRT, sPrefixB, pRiB) >= 0);
  assert(OSPF_rt_add_route(pRT, sPrefixC, pRiC) >= 0);
  assert(OSPF_rt_add_route(pRT, sSubnetPfxTx, pRiTx) >= 0);
  OSPF_rt_dump(stdout, pRT);
  
  //TODO add advertising router
  
  OSPF_rt_destroy(&pRT);
  assert(pRT == NULL);
  
  ospf_nh_list_destroy(&pNHList);
  assert(pNHList == NULL);
 
  ospf_next_hop_destroy(&pNHB);
  assert(pNHB == NULL);
  ospf_next_hop_destroy(&pNHC1);
  assert(pNHC1 == NULL);
  ospf_next_hop_destroy(&pNHC2);
  assert(pNHC2 == NULL);
  
  subnet_destroy(&pSubTx);
//subnet_destroy(&pSubTx1);
  
  node_destroy(&pNodeA);
  node_destroy(&pNodeB);
  node_destroy(&pNodeC);
  
/*
  
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

  
 
  OSPF_route_info_dump(stdout, pRiB);
  OSPF_route_info_dump(stdout, pRiC);
  
  */  
  return 1;
}

