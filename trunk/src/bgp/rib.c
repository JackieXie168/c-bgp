// ==================================================================
// @(#)rib.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @author Sebastien Tandel (standel@info.ucl.ac.be)
//
// @date 04/12/2002
// @lastdate 28/02/2006
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>

#include <libgds/log.h>

#include <bgp/as.h>
#include <bgp/rib.h>
#include <bgp/routes_list.h>

// ----- rib_route_destroy ------------------------------------------
/**
 *
 */
void rib_route_destroy(void ** ppItem)
{
  SRoute ** ppRoute= (SRoute **) ppItem;

  route_destroy(ppRoute);
}

#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
void rib_routes_list_destroy(void ** ppItem)
{
  SRoutes ** ppRoutes = (SRoutes **) ppItem;

  routes_list_destroy(ppRoutes);
}

typedef struct {
  FRadixTreeForEach fForEach;
  void * pContext;
}SRIBCtx;

//TODO : propagate the result!!!
int rib_trie_for_each(trie_key_t uKey, trie_key_len_t uKeyLen, void * pItem, void * pContext)
{
  uint16_t uIndex;
  SRoutes * pRoutes = (SRoutes *) pItem;
  SRIBCtx * pRIBCtx = (SRIBCtx *) pContext;

  for (uIndex = 0; uIndex < routes_list_get_num(pRoutes); uIndex++) {
    pRIBCtx->fForEach(uKey, uKeyLen, 
		      (void *)routes_list_get_at(pRoutes, uIndex), 
		      (void *)pRIBCtx->pContext);
  }
  return 0;
}
#endif

// ----- rib_create -------------------------------------------------
/**
 *
 */
SRIB * rib_create(uint8_t uOptions)
{
#ifdef __EXPERIMENTAL__
  FTrieDestroy fDestroy;
#else
  FRadixTreeDestroy fDestroy;
#endif

#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  if (uOptions & RIB_OPTION_ALIAS)
    fDestroy = NULL;
  else
    fDestroy = rib_routes_list_destroy;
#else
  if (uOptions & RIB_OPTION_ALIAS)
    fDestroy= NULL;
  else
    fDestroy= rib_route_destroy;
#endif

#ifdef __EXPERIMENTAL__
  return (SRIB *) trie_create(fDestroy);
#else
  return (SRIB *) radix_tree_create(32, fDestroy);
#endif
}

// ----- rib_destroy ------------------------------------------------
/**
 *
 */
void rib_destroy(SRIB ** ppRIB)
{

#ifdef __EXPERIMENTAL__
  STrie ** ppTrie= (STrie **) ppRIB;

  trie_destroy(ppTrie);
#else
  SRadixTree ** ppRadixTree= (SRadixTree **) ppRIB;

  radix_tree_destroy(ppRadixTree);
#endif
}

// ----- rib_find_best ----------------------------------------------
/**
 *
 */
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
SRoutes * rib_find_best(SRIB * pRIB, SPrefix sPrefix)
#else
SRoute * rib_find_best(SRIB * pRIB, SPrefix sPrefix)
#endif
{
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  return (SRoutes *) trie_find_best((STrie *) pRIB,
				   sPrefix.tNetwork,
				   sPrefix.uMaskLen);
#elif defined __EXPERIMENTAL__
  return (SRoute *) trie_find_best((STrie *) pRIB,
				   sPrefix.tNetwork,
				   sPrefix.uMaskLen);
#else
  return (SRoute *) radix_tree_get_best((SRadixTree *) pRIB,
					sPrefix.tNetwork,
					sPrefix.uMaskLen);
#endif
}


// ----- rib_find_exact ---------------------------------------------
/**
 *
 */
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
SRoutes * rib_find_exact(SRIB * pRIB, SPrefix sPrefix)
#else
SRoute * rib_find_exact(SRIB * pRIB, SPrefix sPrefix)
#endif
{
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  return (SRoutes *) trie_find_exact((STrie *) pRIB,
				    sPrefix.tNetwork,
				    sPrefix.uMaskLen);
#elif defined __EXPERIMENTAL__
  return (SRoute *) trie_find_exact((STrie *) pRIB,
				    sPrefix.tNetwork,
				    sPrefix.uMaskLen);
#else
  return (SRoute *) radix_tree_get_exact((SRadixTree *) pRIB,
					 sPrefix.tNetwork,
					 sPrefix.uMaskLen);
#endif
}

#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
// ----- rib_find_one_best -------------------------------------------
/**
 *
 */
SRoute * rib_find_one_best(SRIB * pRIB, SPrefix sPrefix)
{
  SRoutes * pRoutes = rib_find_best(pRIB, sPrefix);

  if (pRoutes != NULL) {
    assert(routes_list_get_num(pRoutes) == 0 
	|| routes_list_get_num(pRoutes) == 1);
    if (routes_list_get_num(pRoutes) == 1)
      return routes_list_get_at(pRoutes, 0);
  }
  return NULL;
}

// ----- rib_find_onex_exact -----------------------------------------
/**
 *
 */
SRoute * rib_find_one_exact(SRIB * pRIB, SPrefix sPrefix, 
					  net_addr_t * tNextHop)
{
  uint16_t uIndex;
  SRoutes * pRoutes = rib_find_exact(pRIB, sPrefix);
  SRoute * pRoute;

  if (pRoutes != NULL) {
    if (tNextHop != NULL) {
  //LOG_DEBUG("removal : \n");
  //LOG_ENABLED_DEBUG() ip_prefix_dump(log_get_stream(pMainLog), sPrefix);
  //LOG_ENABLED_DEBUG() ip_address_dump(log_get_stream(pMainLog), *tNextHop);
  //LOG_DEBUG("\n");
      for (uIndex = 0; uIndex < routes_list_get_num(pRoutes); uIndex++) {
	pRoute = routes_list_get_at(pRoutes, uIndex);
	if (pRoute->tNextHop == *tNextHop)
	  return pRoute;
      }
    //the next hop is not known ... we must be sure there is only one route in
    //the RIB
    } else {
      assert(routes_list_get_num(pRoutes) == 0 
	  || routes_list_get_num(pRoutes) == 1);
      if (routes_list_get_num(pRoutes) == 1)
	return routes_list_get_at(pRoutes, 0);
    }
  }
  return NULL;
}

// ----- _rib_insert_new_route ---------------------------------------
/**
 *
 */
int _rib_insert_new_route(SRIB * pRIB, SRoute * pRoute)
{
  SRoutes * pRoutes;
  //TODO: Verify that the option is good!
  pRoutes = routes_list_create(0);
  routes_list_append(pRoutes, pRoute);
  return trie_insert((STrie *) pRIB, pRoute->sPrefix.tNetwork,
	  pRoute->sPrefix.uMaskLen, pRoutes);
}

// ----- _rib_replace_route ------------------------------------------
/**
 *
 */
int _rib_replace_route(SRIB * pRIB, SRoutes * pRoutes, SRoute * pRoute)
{
  uint16_t uIndex;
  SRoute * pRouteSeek;

  for (uIndex = 0; uIndex < routes_list_get_num(pRoutes); uIndex++) {
    pRouteSeek = routes_list_get_at(pRoutes, uIndex);
    //It's the same route ... OK ... update it!
    if (pRouteSeek->tNextHop == pRoute->tNextHop) {
      routes_list_remove_at(pRoutes, uIndex);
      routes_list_append(pRoutes, pRoute);
      return 0;
    }
  }
  //The route has not been found ... insert it
  routes_list_append(pRoutes, pRoute);
  return 0;
}

// ----- _rib_remove_route -------------------------------------------
/**
 *
 */
int _rib_remove_route(SRoutes * pRoutes, net_addr_t tNextHop)
{
  uint16_t uIndex;
  SRoute * pRouteSeek;

  for (uIndex = 0; uIndex < routes_list_get_num(pRoutes); uIndex++) {
    pRouteSeek = routes_list_get_at(pRoutes, uIndex);
    if (pRouteSeek->tNextHop == tNextHop) {
      //LOG_DEBUG("remove route ");
      //LOG_ENABLED_DEBUG() ip_address_dump(log_get_stream(pMainLog), tNextHop);
      //LOG_DEBUG("\n");
      routes_list_remove_at(pRoutes, uIndex);
      return 0;
    } 
   // else {
   //   LOG_DEBUG("not removed ");
   //   LOG_ENABLED_DEBUG() ip_address_dump(log_get_stream(pMainLog), tNextHop);
   //   LOG_DEBUG("\n");
   // }

      
  }
  return 0;
}
#endif

// ----- rib_add_route ----------------------------------------------
/**
 *
 */
int rib_add_route(SRIB * pRIB, SRoute * pRoute)
{
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  SRoutes * pRoutes = trie_find_exact((STrie *) pRIB,
				      pRoute->sPrefix.tNetwork,
				      pRoute->sPrefix.uMaskLen);
  if (pRoutes == NULL) {
    return _rib_insert_new_route(pRIB, pRoute);
  } else {
    return _rib_replace_route(pRIB, pRoutes, pRoute);
  }
#elif defined __EXPERIMENTAL__
  return trie_insert((STrie *) pRIB, pRoute->sPrefix.tNetwork,
		     pRoute->sPrefix.uMaskLen, pRoute);
#else
  return radix_tree_add((SRadixTree *) pRIB, pRoute->sPrefix.tNetwork,
			pRoute->sPrefix.uMaskLen, pRoute);
#endif
}

// ----- rib_replace_route ------------------------------------------
/**
 *
 */
int rib_replace_route(SRIB * pRIB, SRoute * pRoute)
{
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  SRoutes * pRoutes = trie_find_exact((STrie *) pRIB,
				      pRoute->sPrefix.tNetwork,
				      pRoute->sPrefix.uMaskLen);
  if (pRoutes == NULL) {
    return _rib_insert_new_route(pRIB, pRoute);
  } else {
    return _rib_replace_route(pRIB, pRoutes, pRoute);
  }
#elif defined __EXPERIMENTAL__
  return trie_insert((STrie *) pRIB, pRoute->sPrefix.tNetwork,
		     pRoute->sPrefix.uMaskLen, pRoute);
#else
  return radix_tree_add((SRadixTree *) pRIB, pRoute->sPrefix.tNetwork,
			pRoute->sPrefix.uMaskLen, pRoute);
#endif
}

// ----- rib_remove_route -------------------------------------------
/**
 *
 */
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
int rib_remove_route(SRIB * pRIB, SPrefix sPrefix, net_addr_t * tNextHop)
#else
int rib_remove_route(SRIB * pRIB, SPrefix sPrefix)
#endif
{
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  SRoutes * pRoutes = trie_find_exact((STrie *) pRIB,
				      sPrefix.tNetwork,
				      sPrefix.uMaskLen);
  if (pRoutes != NULL) {
    // Next-Hop == NULL => corresponds to an explicit withdraw!
    // we may destroy the list directly.
    if (tNextHop != NULL) {
      _rib_remove_route(pRoutes, *tNextHop);
      if (routes_list_get_num(pRoutes) == 0) {
        return trie_remove((STrie *) pRIB, sPrefix.tNetwork,
				sPrefix.uMaskLen);
      }
    } else {
      return trie_remove((STrie *) pRIB, sPrefix.tNetwork,
				sPrefix.uMaskLen);
    }
  }
  return 0;
#elif defined __EXPERIMENTAL__
  return trie_remove((STrie *) pRIB,
		     sPrefix.tNetwork,
		     sPrefix.uMaskLen);
#else
  return radix_tree_remove((SRadixTree *) pRIB,
			   sPrefix.tNetwork,
			   sPrefix.uMaskLen, 1);
#endif
}

// ----- rib_for_each -----------------------------------------------
/**
 *
 */
int rib_for_each(SRIB * pRIB, FRadixTreeForEach fForEach,
		 void * pContext)
{

#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  int iRet;
  SRIBCtx * pCtx = MALLOC(sizeof(SRIBCtx));

  pCtx->fForEach = fForEach;
  pCtx->pContext = pContext;

  iRet = trie_for_each((STrie *) pRIB, rib_trie_for_each, pCtx);
  FREE(pCtx);
  return iRet;
#elif defined __EXPERIMENTAL__
  return trie_for_each((STrie *) pRIB, fForEach,
		       pContext);
#else
  return radix_tree_for_each((SRadixTree *) pRIB, fForEach,
			     pContext);
#endif
}
