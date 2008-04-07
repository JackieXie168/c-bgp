// ==================================================================
// @(#)routes_list.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 20/02/2004
// @lastdate 11/03/2008
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>

#include <bgp/route.h>
#include <bgp/routes_list.h>

// -----[ _routes_list_item_destroy ]--------------------------------
static void _routes_list_item_destroy(void * pItem)
{
  route_destroy((bgp_route_t **) pItem);
}

// ----- routes_list_create -----------------------------------------
/**
 * Create an array of routes. The created array of routes support a
 * single option.
 *
 * Options:
 * - ROUTER_LIST_OPTION_REF: if this flag is set, the function creates
 *                           an array of references to existing
 *                           routes.
 */
bgp_routes_t * routes_list_create(uint8_t uOptions)
{
  if (uOptions & ROUTES_LIST_OPTION_REF) {
    return (bgp_routes_t *) ptr_array_create_ref(0);
  } else {
    return (bgp_routes_t *) ptr_array_create(0, NULL,
					     _routes_list_item_destroy);
  }
}

// ----- routes_list_destroy ----------------------------------------
/**
 *
 */
void routes_list_destroy(bgp_routes_t ** ppRoutes)
{
  ptr_array_destroy((SPtrArray **) ppRoutes);
}

// ----- routes_list_append -----------------------------------------
/**
 *
 */
void routes_list_append(bgp_routes_t * pRoutes, bgp_route_t * pRoute)
{
  assert(ptr_array_append((SPtrArray *) pRoutes, pRoute) >= 0);
}

// ----- routes_list_remove_at --------------------------------------
/**
 *
 */
void routes_list_remove_at(bgp_routes_t * pRoutes, unsigned int uIndex)
{
  ptr_array_remove_at((SPtrArray *) pRoutes, uIndex);
}

// ----- routes_list_dump -------------------------------------------
/**
 *
 */
void routes_list_dump(SLogStream * pStream, bgp_routes_t * pRoutes)
{
  unsigned int uIndex;

  for (uIndex= 0; uIndex < bgp_routes_size(pRoutes); uIndex++) {
    route_dump(pStream, bgp_routes_at(pRoutes, uIndex));
    log_printf(pStream, "\n");
  }
}

// -----[ routes_list_for_each ]-------------------------------------
/**
 * Call the given function for each route contained in the list.
 */
int routes_list_for_each(bgp_routes_t * pRoutes, FArrayForEach fForEach,
			 void * pContext)
{
  return _array_for_each((SArray *) pRoutes, fForEach, pContext);
}
