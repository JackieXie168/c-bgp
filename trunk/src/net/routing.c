// ==================================================================
// @(#)routing.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 24/02/2004
// @lastdate 05/03/2004
// ==================================================================

#include <libgds/log.h>
#include <libgds/memory.h>
#include <net/routing.h>

// ----- route_info_create -----------------------------------------------
/**
 *
 */
SNetRouteInfo * route_info_create(SNetLink * pNextHopIf,
				  uint32_t uWeight,
				  uint8_t uType)
{
  SNetRouteInfo * pRouteInfo=
    (SNetRouteInfo *) MALLOC(sizeof(SNetRouteInfo));
  pRouteInfo->pNextHopIf= pNextHopIf;
  pRouteInfo->uWeight= uWeight;
  pRouteInfo->uType= uType;
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
  switch (pRouteInfo->uType) {
  case NET_ROUTE_STATIC:
    fprintf(pStream, "STATIC");
    break;
  case NET_ROUTE_IGP:
    fprintf(pStream, "IGP");
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

// ----- rt_create --------------------------------------------------
/**
 *
 */
SNetRT * rt_create()
{
  return (SNetRT *) radix_tree_create(32, rt_route_info_destroy);
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
  return (SNetRouteInfo *) radix_tree_get_best((SRadixTree *) pRT, tAddr, 32);
}

// ----- rt_find_exact ----------------------------------------------
/**
 *
 */
SNetRouteInfo * rt_find_exact(SNetRT * pRT, SPrefix sPrefix)
{
  return (SNetRouteInfo *) radix_tree_get_exact((SRadixTree *) pRT,
						sPrefix.tNetwork,
						sPrefix.uMaskLen);
}

// ----- rt_add_route -----------------------------------------------
/**
 *
 */
int rt_add_route(SNetRT * pRT, SPrefix sPrefix,
		 SNetRouteInfo * pRouteInfo)
{
  return radix_tree_add((SRadixTree *) pRT, sPrefix.tNetwork,
			sPrefix.uMaskLen, pRouteInfo);
}

// ----- rt_del_route -----------------------------------------------
/**
 *
 */
int rt_del_route(SNetRT * pRT, SPrefix sPrefix)
{
  LOG_SEVERE("Error: not implemented, sorry.\n");
  return -1;
}

// ----- rt_dump_for_each -------------------------------------------
int rt_dump_for_each(uint32_t uKey, uint8_t uKeyLen, void * pItem,
		     void * pContext)
{
  FILE * pStream= (FILE *) pContext;
  SNetRouteInfo * pRouteInfo= (SNetRouteInfo *) pItem;
  
  ip_address_dump(pStream, (net_addr_t) uKey);
  fprintf(pStream, "/%u\t", uKeyLen);
  route_info_dump(pStream, pRouteInfo);
  fprintf(pStream, "\n");
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
    if (pRouteInfo == NULL)
      fprintf(pStream, "NO_ROUTE");
    else
      route_info_dump(pStream, pRouteInfo);
    fprintf(pStream, "\n");
  } else {
    pRouteInfo= rt_find_best(pRT, sPrefix.tNetwork);
    if (pRouteInfo == NULL)
      fprintf(pStream, "NO_ROUTE");
    else
      route_info_dump(pStream, pRouteInfo);
    fprintf(pStream, "\n");
  }
}
