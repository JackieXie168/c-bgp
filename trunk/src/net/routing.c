// ==================================================================
// @(#)routing.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 24/02/2004
// @lastdate 05/08/2005
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

// ----- route_nexthop_compare -------------------------------------------
/**
 *
 */
int route_nexthop_compare(SNetRouteNextHop sNH1, SNetRouteNextHop sNH2)
{
  if (sNH1.tAddr > sNH2.tAddr)
    return 1;
  if (sNH1.tAddr < sNH2.tAddr)
    return -1;
  
  if (link_get_address(sNH1.pIface) >
      link_get_address(sNH2.pIface))
    return 1;
  if (link_get_address(sNH1.pIface) <
      link_get_address(sNH2.pIface))
    return -1;

  return 0;
}

// ----- rt_perror -------------------------------------------------------
/**
 * Dump an error message corresponding to the given error code.
 */
void rt_perror(FILE * pStream, int iErrorCode)
{
  switch (iErrorCode) {
  case NET_RT_SUCCESS:
    fprintf(pStream, "success"); break;
  case NET_RT_ERROR_NH_UNREACH:
    fprintf(pStream, "next-hop is unreachable"); break;
  case NET_RT_ERROR_IF_UNKNOWN:
    fprintf(pStream, "interface is unknown"); break;
  case NET_RT_ERROR_ADD_DUP:
    fprintf(pStream, "route already exists"); break;
  case NET_RT_ERROR_DEL_UNEXISTING:
    fprintf(pStream, "route does not exist"); break;
  default:
    fprintf(pStream, "unknown error");
  }
}

// ----- net_route_info_create --------------------------------------
/**
 *
 */
SNetRouteInfo * net_route_info_create(SPrefix sPrefix,
				      SNetLink * pIface,
				      net_addr_t tNextHop,
				      uint32_t uWeight,
				      net_route_type_t tType)
{
  SNetRouteInfo * pRouteInfo=
    (SNetRouteInfo *) MALLOC(sizeof(SNetRouteInfo));
  pRouteInfo->sPrefix= sPrefix;
  pRouteInfo->sNextHop.tAddr= tNextHop;
  pRouteInfo->sNextHop.pIface= pIface;
  pRouteInfo->uWeight= uWeight;
  pRouteInfo->tType= tType;
  return pRouteInfo;
}

// ----- net_route_info_destroy -------------------------------------
/**
 *
 */
void net_route_info_destroy(SNetRouteInfo ** ppRouteInfo)
{
  if (*ppRouteInfo != NULL) {
    FREE(*ppRouteInfo);
    *ppRouteInfo= NULL;
  }
}

// ----- rt_route_info_destroy --------------------------------------
void rt_route_info_destroy(void ** ppItem)
{
  net_route_info_destroy((SNetRouteInfo **) ppItem);
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
  return route_nexthop_compare(pRI1->sNextHop, pRI2->sNextHop);
}

// ----- rt_info_list_dst -------------------------------------------
void rt_info_list_dst(void * pItem)
{
  SNetRouteInfo * pRI= *((SNetRouteInfo **) pItem);

  net_route_info_destroy(&pRI);
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
/**
 * Add a new route into the info-list (an info-list corresponds to a
 * destination prefix).
 *
 * Result:
 * - NET_RT_SUCCESS if the insertion succeeded
 * - NET_RT_ERROR_ADD_DUP if the route could not be added (it already
 *   exists)
 */
int rt_info_list_add(SNetRouteInfoList * pRouteInfoList,
			SNetRouteInfo * pRouteInfo)
{
  if (ptr_array_add((SPtrArray *) pRouteInfoList,
		    &pRouteInfo) < 0) {
    return NET_RT_ERROR_ADD_DUP;
  }
  return NET_RT_SUCCESS;
}

// ----- SNetRouteInfoFilter ----------------------------------------
/**
 * This structure defines a filter to remove routes from the routing
 * table. The possible criteria are as follows:
 * - prefix  : exact match of destination prefix (NULL => match any)
 * - iface   : exact match of outgoing link (NULL => match any)
 * - next-hop: exact match of next-hop (NULL => match any)
 * - type    : exact match of type (mandatory)
 * All the given criteria have to match a route in order for it to be
 * deleted.
 */
typedef struct {
  SPrefix * pPrefix;
  SNetLink * pIface;
  net_addr_t * pNextHop;
  net_route_type_t tType;
  SPtrArray * pRemovalList;
} SNetRouteInfoFilter;

// ----- net_route_info_matches_filter ------------------------------
/**
 * Check if the given route-info matches the given filter. The
 * following fields are checked:
 * - next-hop interface
 * - next-hop address (gateway)
 * - type (routing protocol)
 *
 * Return:
 * 0 if the route does not match filter
 * 1 if the route matches the filter
 */
int net_route_info_matches_filter(SNetRouteInfo * pRI,
				  SNetRouteInfoFilter * pRIFilter)
{
  
  if ((pRIFilter->pIface != NULL) &&
      (pRIFilter->pIface != pRI->sNextHop.pIface))
    return 0;
  if ((pRIFilter->pNextHop != NULL) &&
      (*pRIFilter->pNextHop != pRI->sNextHop.tAddr))
    return 0;
  if (pRIFilter->tType != pRI->tType)
    return 0;

  return 1;
}

// ----- net_route_info_del_dump ------------------------------------
/**
 * Dump a route-info filter.
 */
void net_route_info_dump_filter(FILE * pStream,
				SNetRouteInfoFilter * pRIFilter)
{
  // Destination filter
  fprintf(pStream, "prefix : ");
  if (pRIFilter->pPrefix != NULL)
    ip_prefix_dump(pStream, *pRIFilter->pPrefix);
  else
    fprintf(pStream, "*");

  // Next-hop address (gateway)
  fprintf(pStream, ", next-hop: ");
  if (pRIFilter->pNextHop != NULL)
    ip_address_dump(pStream, *pRIFilter->pNextHop);
  else
    fprintf(pStream, "*");

  // Next-hop interface
  fprintf(pStream, ", iface   : ");
  if (pRIFilter->pIface != NULL)
    link_dst_dump(pStream, pRIFilter->pIface);
  else
    fprintf(pStream, "*");

  // Route type (routing protocol)
  fprintf(pStream, ", type: ");
  net_route_type_dump(pStream, pRIFilter->tType);
  fprintf(pStream, "\n");
}

// ----- net_info_prefix_dst ----------------------------------------
void net_info_prefix_dst(void * pItem)
{
  ip_prefix_destroy((SPrefix **) pItem);
}

// ----- net_info_schedule_removal -----------------------------------
/**
 * This function adds a prefix whose entry in the routing table must
 * be removed. This is used when a route-info-list has been emptied.
 */
void net_info_schedule_removal(SNetRouteInfoFilter * pRIFilter,
			       SPrefix * pPrefix)
{
  if (pRIFilter->pRemovalList == NULL)
    pRIFilter->pRemovalList= ptr_array_create(0, NULL, net_info_prefix_dst);

  pPrefix= ip_prefix_copy(pPrefix);
  ptr_array_append(pRIFilter->pRemovalList, pPrefix);
}

// ----- net_info_removal_for_each ----------------------------------
/**
 *
 */
int net_info_removal_for_each(void * pItem, void * pContext)
{
  SNetRT * pRT= (SNetRT *) pContext;
  SPrefix * pPrefix= *(SPrefix **) pItem;

  return trie_remove(pRT, pPrefix->tNetwork, pPrefix->uMaskLen);
}

// ----- net_info_removal -------------------------------------------
/**
 * Remove the scheduled prefixes from the routing table.
 */
int net_info_removal(SNetRouteInfoFilter * pRIFilter,
		     SNetRT * pRT)
{
  int iResult= 0;

  if (pRIFilter->pRemovalList != NULL) {
    iResult= _array_for_each((SArray *) pRIFilter->pRemovalList,
			     net_info_removal_for_each,
			     pRT);
    ptr_array_destroy(&pRIFilter->pRemovalList);
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
		     SNetRouteInfoFilter * pRIFilter,
		     SPrefix * pPrefix)
{
  unsigned int uIndex;
  SNetRouteInfo * pRI;
  unsigned int uRemovedCount= 0;
  int iResult= NET_RT_ERROR_DEL_UNEXISTING;

  /* Lookup the whole list of routes... */
  uIndex= 0;
  while (uIndex < ptr_array_length((SPtrArray *) pRouteInfoList)) {
    pRI= (SNetRouteInfo *) pRouteInfoList->data[uIndex];

    if (net_route_info_matches_filter(pRI, pRIFilter)) {
      ptr_array_remove_at((SPtrArray *) pRouteInfoList, uIndex);
      uRemovedCount++;
    } else {
      uIndex++;
    }
  }
  
  /* If the list of routes does not contain any route, it should be
     destroyed. Schedule the prefix for removal... */
  if (rt_info_list_length(pRouteInfoList) == 0)
    net_info_schedule_removal(pRIFilter, pPrefix);

  if (uRemovedCount > 0)
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

// ----- rt_il_dst --------------------------------------------------
void rt_il_dst(void ** ppItem)
{
  rt_info_list_destroy((SNetRouteInfoList **) ppItem);
}

// ----- rt_create --------------------------------------------------
/**
 * Create a routing table.
 */
SNetRT * rt_create()
{
  return (SNetRT *) trie_create(rt_il_dst);
}

// ----- rt_destroy -------------------------------------------------
/**
 * Free a routing table.
 */
void rt_destroy(SNetRT ** ppRT)
{
  trie_destroy((STrie **) ppRT);
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
  pRIList= (SNetRouteInfoList *) trie_find_best((STrie *) pRT, tAddr, 32);

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
  pRIList= (SNetRouteInfoList *)
    trie_find_exact((STrie *) pRT, sPrefix.tNetwork, sPrefix.uMaskLen);

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
 * Add a route into the routing table.
 */
int rt_add_route(SNetRT * pRT, SPrefix sPrefix,
		 SNetRouteInfo * pRouteInfo)
{
  SNetRouteInfoList * pRIList;
  int iInsert= 0;
  int iResult;

  pRIList=
    (SNetRouteInfoList *) trie_find_exact((STrie *) pRT,
					  sPrefix.tNetwork,
					  sPrefix.uMaskLen);

  // Create a new info-list if none exists for the given prefix
  if (pRIList == NULL) {
    pRIList= rt_info_list_create();
    iInsert= 1;
  }

  if ((iResult= rt_info_list_add(pRIList, pRouteInfo)) == NET_RT_SUCCESS) {
    // We only insert into the trie if the route could be added to the
    // new info-list
    if (iInsert)
      trie_insert((STrie *) pRT, sPrefix.tNetwork, sPrefix.uMaskLen, pRIList);
  } else {
    rt_info_list_destroy(&pRIList);
  }
  
  return iResult;
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
  SNetRouteInfoFilter * pRIFilter= (SNetRouteInfoFilter *) pContext;
  SPrefix sPrefix;
  int iResult;

  if (pRIList == NULL)
    return -1;

  sPrefix.tNetwork= uKey;
  sPrefix.uMaskLen= uKeyLen;

  /* Remove from the list all the routes that match the given
     attributes */
  iResult= rt_info_list_del(pRIList, pRIFilter, &sPrefix);

  /* If we use widlcards, the call never fails, otherwise, it depends
     on the 'rt_info_list_del' call */
  return ((pRIFilter->pPrefix != NULL) &&
	  (pRIFilter->pIface != NULL) &&
	  (pRIFilter->pNextHop)) ? iResult : 0;
}

// ----- rt_del_route -----------------------------------------------
/**
 * Remove route(s) from the given routing table. The route(s) to be
 * removed must match the given attributes: prefix, next-hop and
 * type. However, wildcards can be used for the prefix and next-hop
 * attributes. The NULL value corresponds to the wildcard.
 */
int rt_del_route(SNetRT * pRT, SPrefix * pPrefix,
		 SNetLink * pIface, net_addr_t * ptNextHop,
		 net_route_type_t tType)
{
  SNetRouteInfoList * pRIList;
  SNetRouteInfoFilter sRIFilter;
  int iResult;

  /* Prepare the attributes of the routes to be removed (context) */
  sRIFilter.pPrefix= pPrefix;
  sRIFilter.pIface= pIface;
  sRIFilter.pNextHop= ptNextHop;
  sRIFilter.tType= tType;
  sRIFilter.pRemovalList= NULL;

  /* Prefix specified ? or wildcard ? */
  if (pPrefix != NULL) {

    /* Get the list of routes towards the given prefix */
    pRIList=
      (SNetRouteInfoList *) trie_find_exact((STrie *) pRT,
					    pPrefix->tNetwork,
					    pPrefix->uMaskLen);
    iResult= rt_del_for_each(pPrefix->tNetwork, pPrefix->uMaskLen,
			     pRIList, &sRIFilter);

  } else {

    /* Remove all the routes that match the given attributes, whatever
       the prefix is */
    iResult= trie_for_each((STrie *) pRT, rt_del_for_each, &sRIFilter);

  }

  net_info_removal(&sRIFilter, pRT);

  return iResult;
}

typedef struct {
  FRadixTreeForEach fForEach;
  void * pContext;
} SRTForEachCtx;

// ----- rt_for_each_function ----------------------------------------------
/**
 * Helper function for the 'rt_for_each' function. This function
 * execute the callback function for each entry in the info-list of
 * the current prefix.
 */
int rt_for_each_function(uint32_t uKey, uint8_t uKeyLen,
			 void * pItem, void * pContext)
{
  SRTForEachCtx * pCtx= (SRTForEachCtx *) pContext;
  SNetRouteInfoList * pInfoList= (SNetRouteInfoList *) pItem;
  int iIndex;

  for (iIndex= 0; iIndex < ptr_array_length(pInfoList); iIndex++) {
    if (pCtx->fForEach(uKey, uKeyLen, pInfoList->data[iIndex],
		       pCtx->pContext) != 0)
      return -1;
  }

  return 0;
}

// ----- rt_for_each -------------------------------------------------------
/**
 * Execute the given function for each entry (prefix, type) in the
 * routing table.
 */
int rt_for_each(SNetRT * pRT, FRadixTreeForEach fForEach, void * pContext)
{
  SRTForEachCtx sCtx;

  sCtx.fForEach= fForEach;
  sCtx.pContext= pContext;

  return trie_for_each((STrie *) pRT, rt_for_each_function, &sCtx);
}


/////////////////////////////////////////////////////////////////////
//
// ROUTING TABLE DUMP FUNCTIONS
//
/////////////////////////////////////////////////////////////////////

// ----- route_nexthop_dump -----------------------------------------
/**
 *
 */
void route_nexthop_dump(FILE * pStream, SNetRouteNextHop sNextHop)
{
  ip_address_dump(pStream, sNextHop.tAddr);
  fprintf(pStream, "\t");
  link_dst_dump(pStream, sNextHop.pIface);
}

// ----- net_route_type_dump ----------------------------------------
/**
 *
 */
void net_route_type_dump(FILE * pStream, net_route_type_t tType)
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

// ----- net_route_info_dump ----------------------------------------
/**
 * Output format:
 * <dst-prefix> <link/if> <weight> <type> [ <state> ]
 */
void net_route_info_dump(FILE * pStream, SNetRouteInfo * pRouteInfo)
{
  ip_prefix_dump(pStream, pRouteInfo->sPrefix);
  fprintf(pStream, "\t");
  route_nexthop_dump(pStream, pRouteInfo->sNextHop);
  fprintf(pStream, "\t%u\t", pRouteInfo->uWeight);
  net_route_type_dump(pStream, pRouteInfo->tType);
  if (!link_get_state(pRouteInfo->sNextHop.pIface, NET_LINK_FLAG_UP)) {
    fprintf(pStream, "\t[DOWN]");
  }
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
      net_route_info_dump(pStream,
			  (SNetRouteInfo *) pRouteInfoList->data[iIndex]);
      fprintf(pStream, "\n");
    }
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
 * Dump the routing table for the given destination. The destination
 * can be of type NET_DEST_ANY, NET_DEST_ADDRESS and NET_DEST_PREFIX.
 */
void rt_dump(FILE * pStream, SNetRT * pRT, SNetDest sDest)
{
  SNetRouteInfo * pRouteInfo;

  //fprintf(pStream, "DESTINATION\tNEXTHOP\tIFACE\tMETRIC\tTYPE\n");

  switch (sDest.tType) {

  case NET_DEST_ANY:
    trie_for_each((STrie *) pRT, rt_dump_for_each, pStream);
    break;

  case NET_DEST_ADDRESS:
    pRouteInfo= rt_find_best(pRT, sDest.uDest.sPrefix.tNetwork, NET_ROUTE_ANY);
    if (pRouteInfo != NULL) {
      net_route_info_dump(pStream, pRouteInfo);
      fprintf(pStream, "\n");
    }
    break;

  case NET_DEST_PREFIX:
    pRouteInfo= rt_find_exact(pRT, sDest.uDest.sPrefix, NET_ROUTE_ANY);
    if (pRouteInfo != NULL) {
      net_route_info_dump(pStream, pRouteInfo);
      fprintf(pStream, "\n");
    }
    break;

  default:
    abort();
  }
}

/////////////////////////////////////////////////////////////////////
//
// TESTS
//
/////////////////////////////////////////////////////////////////////

