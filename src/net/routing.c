// ==================================================================
// @(#)routing.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 24/02/2004
// @lastdate 19/05/2004
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

// ----- route_type_dump --------------------------------------------
/**
 *
 */
void route_type_dump(FILE * pStream, net_route_type_t tType)
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

// ----- route_info_dump --------------------------------------------
/**
 *
 */
void route_info_dump(FILE * pStream, SNetRouteInfo * pRouteInfo)
{
  ip_address_dump(pStream, pRouteInfo->pNextHopIf->tAddr);
  fprintf(pStream, "\t%u\t", pRouteInfo->uWeight);
  route_type_dump(pStream, pRouteInfo->tType);
}

// ----- rt_route_info_destroy --------------------------------------
void rt_route_info_destroy(void ** ppItem)
{
  route_info_destroy((SNetRouteInfo **) ppItem);
}

// ----- rt_info_list_cmp -------------------------------------------
/**
 * Compare two routes towards the same destination
 * - prefer the route with the lowest type (STATIC > IGP > BGP)
 * - prefer the route with the lowest metric
 * - prefer the route with the lowest next-hop address
 */
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

typedef struct {
  SPrefix * pPrefix;
  SNetLink * pNextHopIf;
  net_route_type_t tType;
} SNetRouteInfoDel;

// ----- rt_info_list_del -------------------------------------------
/**
 * This function removes from the route-list all the routes that match
 * the given attributes: next-hop and route type. A wildcard can be
 * used for the next-hop attribute (by specifying a NULL next-hop
 * link).
 */
int rt_info_list_del(SNetRouteInfoList * pRouteInfoList,
		     SNetRouteInfoDel sRIDel)
{
  int iIndex;
  SNetRouteInfo * pRI;
  int iRemoved= 0;

  /* Lookup the whole list of routes... */
  for (iIndex= 0; iIndex < ptr_array_length((SPtrArray *) pRouteInfoList);
       iIndex++) {
    pRI= (SNetRouteInfo *) pRouteInfoList->data[iIndex];

    /* If the type matches and the next-hop matches or we do not care
       about the next-hop, the remove the route */
    if (((sRIDel.pNextHopIf == NULL) ||
	 (pRI->pNextHopIf == sRIDel.pNextHopIf)) &&
	(pRI->tType == sRIDel.tType)) {
      
      ptr_array_remove_at((SPtrArray *) pRouteInfoList, iIndex);
      iRemoved++;

      /* If we don't use wildcards for the next-hop, we can stop here
	 since there can not be multiple routes in the list with the
	 same next-hop */
      if (sRIDel.pNextHopIf == NULL)
	return NET_RT_SUCCESS;

    }

  }
  
  /* If the list of routes does not contain any route, it should be
     destroyed. */
  // *** BLABLABLA ***

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

// ----- rt_del_for_each --------------------------------------------
/**
 * Helper function for the 'rt_del_route' function. Handles the
 * deletion of a single prefix (can be called by
 * 'radix_tree_for_each')
 */
int rt_del_for_each(uint32_t uKey, uint8_t uKeyLen,
		    void * pItem, void * pContext)
{
  SNetRouteInfoList * pRIList= (SNetRouteInfoList *) pItem;
  SNetRouteInfoDel * pRIDel= (SNetRouteInfoDel *) pContext;
  int iResult;

  if (pRIList == NULL)
    return -1;

  /* Remove from the list all the routes that match the given
     attributes */
  iResult= rt_info_list_del(pRIList, *pRIDel);

  /* If we use widlcards, the call never fails, otherwise, it depends
     on the 'rt_info_list_del' call */
  return ((pRIDel->pPrefix != NULL) &&
	  (pRIDel->pNextHopIf != NULL)) ? iResult : 0;
}

// ----- rt_del_route -----------------------------------------------
/**
 * Remove route(s) from the given routing table. The route(s) to be
 * removed must match the given attributes: prefix, next-hop and
 * type. However, wildcards can be used for the prefix and next-hop
 * attributes. The NULL value corresponds to the wildcard.
 */
int rt_del_route(SNetRT * pRT, SPrefix * pPrefix, SNetLink * pNextHopIf,
		 net_route_type_t tType)
{
  SNetRouteInfoList * pRIList;
  SNetRouteInfoDel sRIDel;

  /* Prepare the attributes of the routes to be removed (context) */
  sRIDel.pPrefix= pPrefix;
  sRIDel.pNextHopIf= pNextHopIf;
  sRIDel.tType= tType;

  /* Prefix specified ? or wildcard ? */
  if (pPrefix != NULL) {

    /* Get the list of routes towards the given prefix */
    pRIList=
      (SNetRouteInfoList *) radix_tree_get_exact((SRadixTree *) pRT,
						 pPrefix->tNetwork,
						 pPrefix->uMaskLen);

    return rt_del_for_each(pPrefix->tNetwork, pPrefix->uMaskLen,
			   pRIList, &sRIDel);

  } else {

    /* Remove all the routes that match the given attributes, whatever
       the prefix is */
    return radix_tree_for_each((SRadixTree *) pRT, rt_del_for_each, &sRIDel);

  }
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
