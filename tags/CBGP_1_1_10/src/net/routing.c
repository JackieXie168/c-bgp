// ==================================================================
// @(#)routing.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 24/02/2004
// @lastdate 08/03/2004
// ==================================================================

#include <libgds/log.h>
#include <libgds/memory.h>
#include <net/network.h>
#include <net/routing.h>

// ----- rt_perror -------------------------------------------------------
/**
 *
 */
void rt_perror(FILE * pStream, int iErrorCode)
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

// ----- route_info_create -----------------------------------------------
/**
 *
 */
SNetRouteInfo * route_info_create(SNetLink * pNextHopIf,
				  uint32_t uWeight,
				  net_route_type_t tType)
{
  SNetRouteInfo * pRouteInfo=
    (SNetRouteInfo *) MALLOC(sizeof(SNetRouteInfo));
  pRouteInfo->pNextHopIf= pNextHopIf;
  pRouteInfo->uWeight= uWeight;
  pRouteInfo->tType= tType;
  return pRouteInfo;
}

// ----- route_info_destroy -----------------------------------------
/**
 *
 */
void route_info_destroy(SNetRouteInfo ** ppRouteInfo)
{
  if (*ppRouteInfo != NULL) {
    FREE(*ppRouteInfo);
    *ppRouteInfo= NULL;
  }
}

// ----- route_info_dump --------------------------------------------
/**
 *
 */
void route_info_dump(FILE * pStream, SNetRouteInfo * pRouteInfo)
{
  ip_address_dump(pStream, pRouteInfo->pNextHopIf->tAddr);
  fprintf(pStream, "\t%u\t", pRouteInfo->uWeight);
  switch (pRouteInfo->tType) {
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

// ----- rt_route_info_destroy --------------------------------------
void rt_route_info_destroy(void ** ppItem)
{
  route_info_destroy((SNetRouteInfo **) ppItem);
}

// ----- rt_info_list_cmp -------------------------------------------
int rt_info_list_cmp(void * pItem1, void * pItem2, unsigned int uEltSize)
{
  SNetRouteInfo * pRI1= *((SNetRouteInfo **) pItem1);
  SNetRouteInfo * pRI2= *((SNetRouteInfo **) pItem2);

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
  if (pRI1->pNextHopIf->tAddr > pRI2->pNextHopIf->tAddr)
    return 1;
  if (pRI1->pNextHopIf->tAddr < pRI2->pNextHopIf->tAddr)
    return- 1;
  
  return 0;
}

// ----- rt_info_list_dst -------------------------------------------
void rt_info_list_dst(void * pItem)
{
  SNetRouteInfo * pRI= *((SNetRouteInfo **) pItem);

  route_info_destroy(&pRI);
}

// ----- rt_info_list_create ----------------------------------------
SNetRouteInfoList * rt_info_list_create()
{
  return (SNetRouteInfoList *) ptr_array_create(ARRAY_OPTION_SORTED|
						ARRAY_OPTION_UNIQUE,
						rt_info_list_cmp,
						rt_info_list_dst);
}

// ----- rt_info_list_destroy ---------------------------------------
void rt_info_list_destroy(SNetRouteInfoList ** ppRouteInfoList)
{
  ptr_array_destroy((SPtrArray **) ppRouteInfoList);
}

// ----- rt_info_list_add -------------------------------------------
int rt_info_list_add(SNetRouteInfoList * pRouteInfoList,
			SNetRouteInfo * pRouteInfo)
{
  if (ptr_array_add((SPtrArray *) pRouteInfoList,
		    &pRouteInfo)) {
    return NET_RT_ERROR_ADD_DUP;
  }
  return NET_RT_SUCCESS;
}

// ----- rt_info_list_del -------------------------------------------
int rt_info_list_del(SNetRouteInfoList * pRouteInfoList,
		     SNetLink * pNextHopIf,
		     net_route_type_t tType)
{
  int iIndex;
  SNetRouteInfo * pRI;
  int iRemoved= 0;

  for (iIndex= 0; iIndex < ptr_array_length((SPtrArray *) pRouteInfoList);
       iIndex++) {
    pRI= (SNetRouteInfo *) pRouteInfoList->data[iIndex];
    if (((pNextHopIf == NULL) || (pRI->pNextHopIf == pNextHopIf)) &&
	(pRI->tType == tType)) {
      ptr_array_remove_at((SPtrArray *) pRouteInfoList, iIndex);
      iRemoved++;
      if (pNextHopIf == NULL)
	return NET_RT_SUCCESS;
    }
  }
  
  if (iRemoved > 0)
    return NET_RT_SUCCESS;

  return NET_RT_ERROR_DEL_UNEXISTING;
}

// ----- rt_info_list_get -------------------------------------------
SNetRouteInfo * rt_info_list_get(SNetRouteInfoList * pRouteInfoList,
				 int iIndex)
{
  if (iIndex < ptr_array_length((SPtrArray *) pRouteInfoList))
    return pRouteInfoList->data[iIndex];
  return NULL;
}

// ----- rt_info_list_dump ------------------------------------------
void rt_info_list_dump(FILE * pStream, SPrefix sPrefix,
		       SNetRouteInfoList * pRouteInfoList)
{
  int iIndex;

  for (iIndex= 0; iIndex < ptr_array_length((SPtrArray *) pRouteInfoList);
       iIndex++) {
    ip_prefix_dump(pStream, sPrefix);
    fprintf(pStream, "\t");
    route_info_dump(pStream, (SNetRouteInfo *) pRouteInfoList->data[iIndex]);
    fprintf(pStream, "\n");
  }
}

// ----- rt_il_dst --------------------------------------------------
void rt_il_dst(void ** ppItem)
{
  rt_info_list_destroy((SNetRouteInfoList **) ppItem);
}

// ----- rt_create --------------------------------------------------
/**
 *
 */
SNetRT * rt_create()
{
  return (SNetRT *) radix_tree_create(32, rt_il_dst);
}

// ----- rt_destroy -------------------------------------------------
/**
 *
 */
void rt_destroy(SNetRT ** ppRT)
{
  radix_tree_destroy((SRadixTree **) ppRT);
}

// ----- rt_find_best -----------------------------------------------
/**
 *
 */
SNetRouteInfo * rt_find_best(SNetRT * pRT, net_addr_t tAddr)
{
  SNetRouteInfoList * pRIList=
    (SNetRouteInfoList *) radix_tree_get_best((SRadixTree *) pRT,
					      tAddr, 32);

  if (pRIList != NULL)
    return rt_info_list_get(pRIList, 0);

  return NULL;
}

// ----- rt_find_exact ----------------------------------------------
/**
 *
 */
SNetRouteInfo * rt_find_exact(SNetRT * pRT, SPrefix sPrefix)
{
  SNetRouteInfoList * pRIList=
    (SNetRouteInfoList *) radix_tree_get_exact((SRadixTree *) pRT,
					       sPrefix.tNetwork,
					       sPrefix.uMaskLen);
  if (pRIList != NULL)
    return rt_info_list_get(pRIList, 0);

  return NULL;
}

// ----- rt_add_route -----------------------------------------------
/**
 *
 */
int rt_add_route(SNetRT * pRT, SPrefix sPrefix,
		 SNetRouteInfo * pRouteInfo)
{
  SNetRouteInfoList * pRIList=
    (SNetRouteInfoList *) radix_tree_get_exact((SRadixTree *) pRT,
					       sPrefix.tNetwork,
					       sPrefix.uMaskLen);
  if (pRIList == NULL) {
    pRIList= rt_info_list_create();
    radix_tree_add((SRadixTree *) pRT, sPrefix.tNetwork,
		   sPrefix.uMaskLen, pRIList);
  }

  return rt_info_list_add(pRIList, pRouteInfo);
}

// ----- rt_del_route -----------------------------------------------
/**
 *
 */
int rt_del_route(SNetRT * pRT, SPrefix sPrefix, SNetLink * pNextHopIf,
		 net_route_type_t tType)
{
  SNetRouteInfoList * pRIList=
    (SNetRouteInfoList *) radix_tree_get_exact((SRadixTree *) pRT,
					       sPrefix.tNetwork,
					       sPrefix.uMaskLen);
  if (pRIList == NULL)
    return -1;

  return rt_info_list_del(pRIList, pNextHopIf, tType);
}

// ----- rt_dump_for_each -------------------------------------------
int rt_dump_for_each(uint32_t uKey, uint8_t uKeyLen, void * pItem,
		     void * pContext)
{
  FILE * pStream= (FILE *) pContext;
  SNetRouteInfoList * pRIList= (SNetRouteInfoList *) pItem;
  SPrefix sPrefix;

  sPrefix.tNetwork= uKey;
  sPrefix.uMaskLen= uKeyLen;
  
  rt_info_list_dump(pStream, sPrefix, pRIList);
  return 0;
}

// ----- rt_dump ----------------------------------------------------
/**
 *
 */
void rt_dump(FILE * pStream, SNetRT * pRT, SPrefix sPrefix)
{
  SNetRouteInfo * pRouteInfo;

  if (sPrefix.uMaskLen == 0) {
    radix_tree_for_each((SRadixTree *) pRT, rt_dump_for_each, pStream);
  } else if (sPrefix.uMaskLen == 32) {
    pRouteInfo= rt_find_best(pRT, sPrefix.tNetwork);
    if (pRouteInfo != NULL)
      route_info_dump(pStream, pRouteInfo);
    fprintf(pStream, "\n");
  } else {
    pRouteInfo= rt_find_best(pRT, sPrefix.tNetwork);
    if (pRouteInfo != NULL)
      route_info_dump(pStream, pRouteInfo);
    fprintf(pStream, "\n");
  }
}

/////////////////////////////////////////////////////////////////////
// TESTS
/////////////////////////////////////////////////////////////////////

