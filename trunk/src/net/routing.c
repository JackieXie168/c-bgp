// ==================================================================
// @(#)routing.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 24/02/2004
// @lastdate 29/03/2005
// ==================================================================

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libgds/log.h>
#include <libgds/memory.h>
#include <net/network.h>
#include <net/routing.h>
#include <ui/output.h>

#include <assert.h>
#include <string.h>

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
SNetRouteInfo * route_info_create(SPrefix sPrefix,
				  SNetLink * pNextHopIf,
				  uint32_t uWeight,
				  net_route_type_t tType)
{
  SNetRouteInfo * pRouteInfo=
    (SNetRouteInfo *) MALLOC(sizeof(SNetRouteInfo));
  pRouteInfo->sPrefix= sPrefix;
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

// ----- route_type_dump_string --------------------------------------------
/**
 *
 */
/*char * route_type_dump_string(net_route_type_t tType)
{
  char * cType = MALLOC(7);

  switch (tType) {
  case NET_ROUTE_STATIC:
    strcpy(cType, "STATIC");
    break;
  case NET_ROUTE_IGP:
    strcpy(cType, "IGP");
    break;
  case NET_ROUTE_BGP:
    strcpy(cType, "BGP");
    break;
  default:
    strcpy(cType, "???");
  }
  return cType;
}*/

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

// ----- route_info_dump_string --------------------------------------------
/**
 *
 */
/*char * route_info_dump_string(SNetRouteInfo * pRouteInfo)
{
  char * cRoute = MALLOC(1024), * cCharTmp;
  uint32_t icRoutePtr = 0;

  cCharTmp = ip_address_dump_string(pRouteInfo->pNextHopIf->tAddr);
  strcpy(cRoute, cCharTmp);
  icRoutePtr = strlen(cRoute);
  FREE(cCharTmp);
  icRoutePtr += sprintf(cRoute+icRoutePtr, "\t%u\t", pRouteInfo->uWeight);
  cCharTmp = route_type_dump_string(pRouteInfo->tType);
  strcpy(cRoute+icRoutePtr, cCharTmp);
  FREE(cCharTmp);

  return cRoute;
}*/

// ----- route_info_dump --------------------------------------------
/**
 * Output format:
 * <dst-prefix> <link/if> <weight> <type> [ <state> ]
 */
void route_info_dump(FILE * pStream, SNetRouteInfo * pRouteInfo)
{
  ip_prefix_dump(pStream, pRouteInfo->sPrefix);
  fprintf(pStream, "\t");
  ip_address_dump(pStream, pRouteInfo->pNextHopIf->tAddr);
  fprintf(pStream, "\t%u\t", pRouteInfo->uWeight);
  route_type_dump(pStream, pRouteInfo->tType);
  if (!link_get_state(pRouteInfo->pNextHopIf, NET_LINK_FLAG_UP)) {
    fprintf(pStream, "\t[DOWN]");
  }
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

// ----- rt_info_list_length ----------------------------------------
int rt_info_list_length(SNetRouteInfoList * pRouteInfoList)
{
  return _array_length((SArray *) pRouteInfoList);
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
  SPtrArray * pRemovalList;
} SNetRouteInfoDel;

// ----- net_info_prefix_dst -----------------------------------------
void net_info_prefix_dst(void * pItem)
{
  ip_prefix_destroy((SPrefix **) pItem);
}

// ----- net_info_schedule_removal -----------------------------------
/**
 * This function adds a prefix whose entry in the routing table must
 * be removed. This is used when a route-info-list has been emptied.
 */
void net_info_schedule_removal(SNetRouteInfoDel * pRIDel,
			       SPrefix * pPrefix)
{
  if (pRIDel->pRemovalList == NULL)
    pRIDel->pRemovalList= ptr_array_create(0, NULL, net_info_prefix_dst);

  pPrefix= ip_prefix_copy(pPrefix);
  ptr_array_append(pRIDel->pRemovalList, pPrefix);
}

// ----- net_info_removal_for_each ----------------------------------
/**
 *
 */
int net_info_removal_for_each(void * pItem, void * pContext)
{
  SNetRT * pRT= (SNetRT *) pContext;
  SPrefix * pPrefix= *(SPrefix **) pItem;

  return radix_tree_remove(pRT, pPrefix->tNetwork, pPrefix->uMaskLen, 1);
}

// ----- net_info_removal -------------------------------------------
/**
 * Remove the scheduled prefixes from the routing table.
 */
int net_info_removal(SNetRouteInfoDel * pRIDel,
			SNetRT * pRT)
{
  int iResult= 0;

  if (pRIDel->pRemovalList != NULL) {
    iResult= _array_for_each((SArray *) pRIDel->pRemovalList,
			     net_info_removal_for_each,
			     pRT);
    ptr_array_destroy(&pRIDel->pRemovalList);
  }
  return iResult;
}

// ----- rt_info_list_del -------------------------------------------
/**
 * This function removes from the route-list all the routes that match
 * the given attributes: next-hop and route type. A wildcard can be
 * used for the next-hop attribute (by specifying a NULL next-hop
 * link).
 */
int rt_info_list_del(SNetRouteInfoList * pRouteInfoList,
		     SNetRouteInfoDel * pRIDel,
		     SPrefix * pPrefix)
{
  int iIndex;
  SNetRouteInfo * pRI;
  int iRemoved= 0;
  int iResult= NET_RT_ERROR_DEL_UNEXISTING;

  /* Lookup the whole list of routes... */
  for (iIndex= 0; iIndex < ptr_array_length((SPtrArray *) pRouteInfoList);
       iIndex++) {
    pRI= (SNetRouteInfo *) pRouteInfoList->data[iIndex];

    /* If the type matches and the next-hop matches or we do not care
       about the next-hop, the remove the route */
    if (((pRIDel->pNextHopIf == NULL) ||
	 (pRI->pNextHopIf == pRIDel->pNextHopIf)) &&
	(pRI->tType == pRIDel->tType)) {
      
      ptr_array_remove_at((SPtrArray *) pRouteInfoList, iIndex);
      iRemoved++;

      /* If we don't use wildcards for the next-hop, we can stop here
	 since there can not be multiple routes in the list with the
	 same next-hop */
      if (pRIDel->pNextHopIf == NULL) {
	iResult= NET_RT_SUCCESS;
	break;
      }

    }

  }
  
  /* If the list of routes does not contain any route, it should be
     destroyed. Schedule the prefix for removal... */
  if (rt_info_list_length(pRouteInfoList) == 0)
    net_info_schedule_removal(pRIDel, pPrefix);

  if (iRemoved > 0)
    iResult= NET_RT_SUCCESS;

  return iResult;
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

  if (rt_info_list_length(pRouteInfoList) == 0) {

    fprintf(pStream, "\033[1;31mERROR: empty info-list for ");
    ip_prefix_dump(pStream, sPrefix);
    fprintf(pStream, "\033[0m\n");
    abort();

  } else {
    for (iIndex= 0; iIndex < ptr_array_length((SPtrArray *) pRouteInfoList);
	 iIndex++) {
      //ip_prefix_dump(pStream, sPrefix);
      //fprintf(pStream, "\t");
      route_info_dump(pStream, (SNetRouteInfo *) pRouteInfoList->data[iIndex]);
      fprintf(pStream, "\n");
    }
  }
}

// ----- rt_info_list_dump_string ------------------------------------------
/*char * rt_info_list_dump_string(SPrefix sPrefix,
		       SNetRouteInfoList * pRouteInfoList)
{
  char * cInfo = MALLOC(1024), * cCharTmp;
  int iIndex;
  uint32_t icInfoPtr = 0;

  for (iIndex= 0; iIndex < ptr_array_length((SPtrArray *) pRouteInfoList);
       iIndex++) {
    cCharTmp = ip_prefix_dump_string(sPrefix);
    strcpy(cInfo+icInfoPtr, cCharTmp);
    icInfoPtr += strlen(cCharTmp);
    FREE(cCharTmp);

    strcpy(cInfo+icInfoPtr++, "\t");

    cCharTmp = route_info_dump_string((SNetRouteInfo *) pRouteInfoList->data[iIndex]);
    strcpy(cInfo+icInfoPtr, cCharTmp);
    icInfoPtr += strlen(cCharTmp);
    FREE(cCharTmp);

    strcpy(cInfo+icInfoPtr++, "\n");
  }
  return cInfo;
}*/


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
#ifdef __EXPERIMENTAL__
  trie_destroy((STrie **) ppRT);
#else
  radix_tree_destroy((SRadixTree **) ppRT);
#endif
}

// ----- rt_find_best -----------------------------------------------
/**
 * Find the route that best matches the given prefix. If a particular
 * route type is given, returns only the route with the requested
 * type.
 *
 * Parameters:
 * - routing table
 * - prefix
 * - route type (can be NET_ROUTE_ANY if any type is ok)
 */
SNetRouteInfo * rt_find_best(SNetRT * pRT, net_addr_t tAddr,
			     net_route_type_t tType)
{
  SNetRouteInfoList * pRIList;
  int iIndex;
  SNetRouteInfo * pRouteInfo;

  if (iOptionDebug) {
    fprintf(stderr, "rt_find_best(");
    ip_address_dump(stderr, tAddr);
    fprintf(stderr, ")\n");
  }

  /* First, retrieve the list of routes that best match the given
     prefix */
#ifdef __EXPERIMENTAL__
  pRIList= (SNetRouteInfoList *) trie_find_best((STrie *) pRT, tAddr, 32);
#else
  pRIList= (SNetRouteInfoList *) radix_tree_get_best((SRadixTree *) pRT,
						     tAddr, 32);
#endif

  /* Then, select the first returned route that matches the given
     route-type (if requested) */
  if (pRIList != NULL) {

    assert(rt_info_list_length(pRIList) != 0);

    for (iIndex= 0; iIndex < ptr_array_length(pRIList); iIndex++) {
      pRouteInfo= rt_info_list_get(pRIList, iIndex);
      if (pRouteInfo->tType & tType)
	return pRouteInfo;
    }
  }

  return NULL;
}

// ----- rt_find_exact ----------------------------------------------
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
SNetRouteInfo * rt_find_exact(SNetRT * pRT, SPrefix sPrefix,
			      net_route_type_t tType)
{
  SNetRouteInfoList * pRIList;
  int iIndex;
  SNetRouteInfo * pRouteInfo;

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

    assert(rt_info_list_length(pRIList) != 0);

    for (iIndex= 0; iIndex < ptr_array_length(pRIList); iIndex++) {
      pRouteInfo= rt_info_list_get(pRIList, iIndex);
      if (pRouteInfo->tType & tType)
	return pRouteInfo;
    }
  }

  return NULL;
}

// ----- rt_add_route -----------------------------------------------
/**
 *
 */
int rt_add_route(SNetRT * pRT, SPrefix sPrefix,
		 SNetRouteInfo * pRouteInfo)
{
  SNetRouteInfoList * pRIList;

#ifdef __EXPERIMENTAL__
  pRIList=
    (SNetRouteInfoList *) trie_find_exact((STrie *) pRT,
					  sPrefix.tNetwork,
					  sPrefix.uMaskLen);
#else
  pRIList=
    (SNetRouteInfoList *) radix_tree_get_exact((SRadixTree *) pRT,
					       sPrefix.tNetwork,
					       sPrefix.uMaskLen);
#endif

  if (pRIList == NULL) {
    pRIList= rt_info_list_create();
#ifdef __EXPERIMENTAL__
    trie_insert((STrie *) pRT, sPrefix.tNetwork, sPrefix.uMaskLen, pRIList);
#else
    radix_tree_add((SRadixTree *) pRT, sPrefix.tNetwork,
		   sPrefix.uMaskLen, pRIList);
#endif
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
  SPrefix sPrefix;
  int iResult;

  if (pRIList == NULL)
    return -1;

  sPrefix.tNetwork= uKey;
  sPrefix.uMaskLen= uKeyLen;

  /* Remove from the list all the routes that match the given
     attributes */
  iResult= rt_info_list_del(pRIList, pRIDel, &sPrefix);

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
  int iResult;

  /* Prepare the attributes of the routes to be removed (context) */
  sRIDel.pPrefix= pPrefix;
  sRIDel.pNextHopIf= pNextHopIf;
  sRIDel.tType= tType;
  sRIDel.pRemovalList= NULL;

  /* Prefix specified ? or wildcard ? */
  if (pPrefix != NULL) {

    /* Get the list of routes towards the given prefix */
#ifdef __EXPERIMENTAL__
    pRIList=
      (SNetRouteInfoList *) trie_find_exact((STrie *) pRT,
					    pPrefix->tNetwork,
					    pPrefix->uMaskLen);
#else
    pRIList=
      (SNetRouteInfoList *) radix_tree_get_exact((SRadixTree *) pRT,
						 pPrefix->tNetwork,
						 pPrefix->uMaskLen);
#endif

    iResult= rt_del_for_each(pPrefix->tNetwork, pPrefix->uMaskLen,
			     pRIList, &sRIDel);

  } else {

    /* Remove all the routes that match the given attributes, whatever
       the prefix is */
#ifdef __EXPERIMENTAL__
    iResult= trie_for_each((STrie *) pRT, rt_del_for_each, &sRIDel);
#else
    iResult= radix_tree_for_each((SRadixTree *) pRT, rt_del_for_each, &sRIDel);
#endif

  }

  net_info_removal(&sRIDel, pRT);

  return iResult;
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

typedef struct {
  char * cDump;
} SRTDump;

// ----- rt_dump_string_for_each -------------------------------------------
/*int rt_dump_string_for_each(uint32_t uKey, uint8_t uKeyLen, void * pItem,
		     void * pContext)
{
  SRTDump * pCtx = (SRTDump *) pContext;
  SNetRouteInfoList * pRIList= (SNetRouteInfoList *) pItem;
  SPrefix sPrefix;
  uint32_t iLen, icDumpPtr;
  char * cCharTmp;

  sPrefix.tNetwork= uKey;
  sPrefix.uMaskLen= uKeyLen;
  
  if (pCtx->cDump == NULL) {
    icDumpPtr = 0;
    pCtx->cDump = MALLOC(1024);
  } else {
    icDumpPtr = iLen = strlen(pCtx->cDump);
    iLen += 1024;
    pCtx->cDump = REALLOC(pCtx->cDump, iLen);
  }

  cCharTmp = rt_info_list_dump_string(sPrefix, pRIList);
  strcpy((pCtx->cDump)+icDumpPtr, cCharTmp);
  FREE(cCharTmp);

  return 0;
}
*/

typedef struct {
  FRadixTreeForEach fForEach;
  void * pContext;
} SRTForEachCtx;

// ----- rt_for_each_function ----------------------------------------------
/**
 *
 */
int rt_for_each_function(uint32_t uKey, uint8_t uKeyLen,
			 void * pItem, void * pContext)
{
  SRTForEachCtx * pCtx= (SRTForEachCtx *) pContext;
  SNetRouteInfoList * pInfoList= (SNetRouteInfoList *) pItem;
  int iIndex;

  for (iIndex= 0; iIndex < ptr_array_length(pInfoList); iIndex++) {
    if (pCtx->fForEach(uKey, uKeyLen, pInfoList->data[iIndex], pCtx->pContext) != 0)
      return -1;
  }

  return 0;
}

// ----- rt_for_each -------------------------------------------------------
/**
 *
 */
int rt_for_each(SNetRT * pRT, FRadixTreeForEach fForEach, void * pContext)
{
  SRTForEachCtx sCtx;

  sCtx.fForEach= fForEach;
  sCtx.pContext= pContext;

#ifdef __EXPERIMENTAL__
  return trie_for_each((STrie *) pRT, rt_for_each_function, &sCtx);
#else
  return radix_tree_for_each((SRadixTree *) pRT, rt_for_each_function, &sCtx);
#endif
}

// ----- rt_dump_string ----------------------------------------------------
/**
 *
 */
/*char * rt_dump_string(SNetRT * pRT, SPrefix sPrefix)
{
  SNetRouteInfo * pRouteInfo;
  SRTDump pCtx ;
  char * cCharTmp;
  uint32_t iLen;

  pCtx.cDump = NULL;

  if (sPrefix.uMaskLen == 0) {
#ifdef __EXPERIMENTAL__
    trie_for_each((STrie *) pRT, rt_dump_string_for_each, &pCtx);
#else
    radix_tree_for_each((SRadixTree *) pRT, rt_dump_string_for_each, &pCtx);
#endif
  } else if (sPrefix.uMaskLen == 32) {
    pRouteInfo= rt_find_best(pRT, sPrefix.tNetwork, NET_ROUTE_ANY);
    iLen = 0;
    if (pRouteInfo != NULL) {
      cCharTmp = route_info_dump_string(pRouteInfo);
      iLen = strlen(cCharTmp);
      pCtx.cDump = MALLOC(iLen+1);
      strcpy(pCtx.cDump, cCharTmp);
      FREE(cCharTmp);
    }
    strcpy((pCtx.cDump)+iLen, "\n");
  } else {
    pRouteInfo= rt_find_best(pRT, sPrefix.tNetwork, NET_ROUTE_ANY);
    iLen = 0;
    if (pRouteInfo != NULL) {
      cCharTmp = route_info_dump_string(pRouteInfo);
      iLen = strlen(cCharTmp);
      pCtx.cDump = MALLOC(iLen+1);
      strcpy(pCtx.cDump, cCharTmp);
      FREE(cCharTmp);
    }
    strcpy((pCtx.cDump)+iLen, "\n");
  }
  return pCtx.cDump;
}*/


// ----- rt_dump ----------------------------------------------------
/**
 * Dump the routing table for the given destination. The destination
 * can be of type NET_DEST_ANY, NET_DEST_ADDRESS and NET_DEST_PREFIX.
 */
void rt_dump(FILE * pStream, SNetRT * pRT, SNetDest sDest)
{
  SNetRouteInfo * pRouteInfo;

  switch (sDest.tType) {

  case NET_DEST_ANY:
#ifdef __EXPERIMENTAL__
    trie_for_each((STrie *) pRT, rt_dump_for_each, pStream);
#else
    radix_tree_for_each((SRadixTree *) pRT, rt_dump_for_each, pStream);
#endif
    break;

  case NET_DEST_ADDRESS:
    pRouteInfo= rt_find_best(pRT, sDest.sPrefix.tNetwork, NET_ROUTE_ANY);
    if (pRouteInfo != NULL) {
      //ip_address_dump(pStream, sDest.sPrefix.tNetwork);
      //fprintf(pStream, "\t");
      route_info_dump(pStream, pRouteInfo);
      fprintf(pStream, "\n");
    }
    break;

  case NET_DEST_PREFIX:
    pRouteInfo= rt_find_exact(pRT, sDest.sPrefix, NET_ROUTE_ANY);
    if (pRouteInfo != NULL) {
      //ip_prefix_dump(pStream, sDest.sPrefix);
      //fprintf(pStream, "\t");
      route_info_dump(pStream, pRouteInfo);
      fprintf(pStream, "\n");
    }
    break;

  default:
    abort();
  }
}

/////////////////////////////////////////////////////////////////////
// TESTS
/////////////////////////////////////////////////////////////////////
