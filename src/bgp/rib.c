// ==================================================================
// @(#)rib.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 04/12/2002
// @lastdate 21/12/2004
// ==================================================================

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

  route_destroy_count++;
  route_destroy(ppRoute);
}

// ----- rib_create -------------------------------------------------
/**
 *
 */
SRIB * rib_create(uint8_t uOptions)
{
  FRadixTreeDestroy fDestroy;

  if (uOptions & RIB_OPTION_ALIAS)
    fDestroy= NULL;
  else
    fDestroy= rib_route_destroy;

  return (SRIB *) radix_tree_create(32, fDestroy);
}

// ----- rib_destroy ------------------------------------------------
/**
 *
 */
void rib_destroy(SRIB ** ppRIB)
{
  SRadixTree ** ppRadixTree= (SRadixTree **) ppRIB;

  radix_tree_destroy(ppRadixTree);
}

// ----- rib_find_best ----------------------------------------------
/**
 *
 */
SRoute * rib_find_best(SRIB * pRIB, SPrefix sPrefix)
{
  return (SRoute *) radix_tree_get_best((SRadixTree *) pRIB,
					sPrefix.tNetwork,
					sPrefix.uMaskLen);
}

// ----- rib_find_exact ---------------------------------------------
/**
 *
 */
SRoute * rib_find_exact(SRIB * pRIB, SPrefix sPrefix)
{
  return (SRoute *) radix_tree_get_exact((SRadixTree *) pRIB,
					 sPrefix.tNetwork,
					 sPrefix.uMaskLen);
}

// ----- rib_add_route ----------------------------------------------
/**
 *
 */
int rib_add_route(SRIB * pRIB, SRoute * pRoute)
{
  return radix_tree_add((SRadixTree *) pRIB, pRoute->sPrefix.tNetwork,
			pRoute->sPrefix.uMaskLen, pRoute);
}

// ----- rib_replace_route ------------------------------------------
/**
 *
 */
int rib_replace_route(SRIB * pRIB, SRoute * pRoute)
{
  return radix_tree_add((SRadixTree *) pRIB, pRoute->sPrefix.tNetwork,
			pRoute->sPrefix.uMaskLen, pRoute);
}

// ----- rib_remove_route -------------------------------------------
/**
 *
 */
int rib_remove_route(SRIB * pRIB, SPrefix sPrefix)
{
  return radix_tree_remove((SRadixTree *) pRIB,
			   sPrefix.tNetwork,
			   sPrefix.uMaskLen);
}

// ----- rib_for_each -----------------------------------------------
/**
 *
 */
int rib_for_each(SRIB * pRIB, FRadixTreeForEach fForEach,
		 void * pContext)
{
  return radix_tree_for_each((SRadixTree *) pRIB, fForEach,
			     pContext);
}
