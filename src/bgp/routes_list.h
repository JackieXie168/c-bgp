// ==================================================================
// @(#)routes_list.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 20/02/2004
// @lastdate 27/02/2004
// ==================================================================

#ifndef __ROUTES_LIST_H__
#define __ROUTES_LIST_H__

#include <libgds/array.h>

typedef SPtrArray SRoutes;

// ----- routes_list_create -----------------------------------------
extern SRoutes * routes_list_create();
// ----- routes_list_destroy ----------------------------------------
extern void routes_list_destroy(SRoutes ** ppRoutes);
// ----- routes_list_append -----------------------------------------
extern void routes_list_append(SRoutes * pRoutes, SRoute * pRoute);
// ----- routes_list_remove_at --------------------------------------
extern void routes_list_remove_at(SRoutes * pRoutes, int iIndex);
// ----- routes_list_get_num ----------------------------------------
extern int routes_list_get_num(SRoutes * pRoutes);
// ----- routes_list_dump -------------------------------------------
extern void routes_list_dump(FILE * pStream, SRoutes * pRoutes);


#endif
