// ==================================================================
// @(#)routes_list.c
//
// A set of functions to manage a list of routes in a RIB. A list of
// routes is a wrapper around an array of pointers to BGP routes.
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 20/02/2004
// @lastdate 21/12/2004
// ==================================================================

#ifdef UNDER_DEVELOPMENT

// ----- rib_routes_destroy_route -----------------------------------
/**
 * Utility function used to destroy a BGP route stored in a list of
 * routes (ptr_array).
 */
void rib_routes_destroy_route(void * pItem)
{
  SRoute * pRoute= *((SRoute **) pItem);

  route_destroy(&pRoute);
}

// ----- rib_routes_create ------------------------------------------
/**
 *
 */
SRIBRoutes * rib_routes_create()
{
  SRIBRoutes * pRoutes=
    (SRIBRoutes *) ptr_array_create(NULL, NULL, rib_routes_destroy_route);
}

// ----- rib_routes_destroy -----------------------------------------
/**
 *
 */
void rib_routes_destroy(SRIBRoutes ** ppRoutes)
{
  ptr_array_destroy((SPtrArray **) ppRoutes);
}

// ----- rib_routes_add ---------------------------------------------

#endif
