// ==================================================================
// @(#)rib.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 04/12/2002
// @lastdate 05/04/2005
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <libgds/log.h>

#include <bgp/as.h>
#include <bgp/rib.h>

// ----- rib_route_destroy ------------------------------------------
/**
 *
 */
void rib_route_destroy(void ** ppItem)
{
  SRoute ** ppRoute= (SRoute **) ppItem;

  route_destroy(ppRoute);
}

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

  if (uOptions & RIB_OPTION_ALIAS)
    fDestroy= NULL;
  else
    fDestroy= rib_route_destroy;

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
SRoute * rib_find_best(SRIB * pRIB, SPrefix sPrefix)
{
#ifdef __EXPERIMENTAL__
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
SRoute * rib_find_exact(SRIB * pRIB, SPrefix sPrefix)
{
#ifdef __EXPERIMENTAL__
  return (SRoute *) trie_find_exact((STrie *) pRIB,
				    sPrefix.tNetwork,
				    sPrefix.uMaskLen);
#else
  return (SRoute *) radix_tree_get_exact((SRadixTree *) pRIB,
					 sPrefix.tNetwork,
					 sPrefix.uMaskLen);
#endif
}

// ----- rib_add_route ----------------------------------------------
/**
 *
 */
int rib_add_route(SRIB * pRIB, SRoute * pRoute)
{
#ifdef __EXPERIMENTAL__
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
#ifdef __EXPERIMENTAL__
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
int rib_remove_route(SRIB * pRIB, SPrefix sPrefix)
{
#ifdef __EXPERIMENTAL__
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
#ifdef __EXPERIMENTAL__
  return trie_for_each((STrie *) pRIB, fForEach,
		       pContext);
#else
  return radix_tree_for_each((SRadixTree *) pRIB, fForEach,
			     pContext);
#endif
}
