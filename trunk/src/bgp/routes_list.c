// ==================================================================
// @(#)routes_list.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 20/02/2004
// @lastdate 27/01/2005
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>

#include <bgp/route.h>
#include <bgp/routes_list.h>

// ----- routes_list_create -----------------------------------------
/**
 *
 */
SRoutes * routes_list_create()
{
  return (SRoutes *) ptr_array_create_ref(0);
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
