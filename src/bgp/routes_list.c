// ==================================================================
// @(#)routes_list.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 20/02/2004
// @lastdate 23/02/2005
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>

#include <bgp/route.h>
#include <bgp/routes_list.h>

// -----[ routes_list_item_destroy ]---------------------------------
void routes_list_item_destroy(void * pItem)
{
  route_destroy((SRoute **) pItem);
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
SRoutes * routes_list_create(uint8_t uOptions)
{
  if (uOptions & ROUTES_LIST_OPTION_REF) {
    return (SRoutes *) ptr_array_create_ref(0);
  } else {
    return (SRoutes *) ptr_array_create(0, NULL, routes_list_item_destroy);
  }
}

// ----- routes_list_destroy ----------------------------------------
/**
 *
 */
void routes_list_destroy(SRoutes ** ppRoutes)
{
  ptr_array_destroy((SPtrArray **) ppRoutes);
}

// ----- routes_list_append -----------------------------------------
/**
 *
 */
void routes_list_append(SRoutes * pRoutes, SRoute * pRoute)
{
  assert(ptr_array_append((SPtrArray *) pRoutes, pRoute) >= 0);
}

// ----- routes_list_remove_at --------------------------------------
/**
 *
 */
void routes_list_remove_at(SRoutes * pRoutes, int iIndex)
{
  ptr_array_remove_at((SPtrArray *) pRoutes, iIndex);
}

// ----- routes_list_get_at -----------------------------------------
/**
 *
 **/
SRoute * routes_list_get_at(SRoutes * pRoutes, const int iIndex)
{
  SRoute * pRoute = MALLOC(sizeof(SRoute));

  ptr_array_get_at((SPtrArray *) pRoutes, iIndex, pRoute);

  return pRoute;
}

// ----- routes_list_get_num ----------------------------------------
/**
 *
 */
int routes_list_get_num(SRoutes * pRoutes)
{
  return ptr_array_length((SPtrArray *) pRoutes);
}

// ----- routes_list_dump -------------------------------------------
/**
 *
 */
void routes_list_dump(FILE * pStream, SRoutes * pRoutes)
{
  int iIndex;

  for (iIndex= 0; iIndex < routes_list_get_num(pRoutes); iIndex++) {
    route_dump(pStream, (SRoute *) pRoutes->data[iIndex]);
    fprintf(pStream, "\n");
  }
}

// -----[ routes_list_for_each ]-------------------------------------
/**
 * Call the given function for each route contained in the list.
 */
int routes_list_for_each(SRoutes * pRoutes, FArrayForEach fForEach,
			 void * pContext)
{
  return _array_for_each((SArray *) pRoutes, fForEach, pContext);
}
