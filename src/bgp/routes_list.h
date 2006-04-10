// ==================================================================
// @(#)routes_list.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 20/02/2004
// @lastdate 03/03/2006
// ==================================================================

#ifndef __ROUTES_LIST_H__
#define __ROUTES_LIST_H__

#include <libgds/array.h>

#define ROUTES_LIST_OPTION_REF 0x01

typedef SPtrArray SRoutes;

// ----- routes_list_create -----------------------------------------
extern SRoutes * routes_list_create(uint8_t uOptions);
// ----- routes_list_destroy ----------------------------------------
extern void routes_list_destroy(SRoutes ** ppRoutes);
// ----- routes_list_append -----------------------------------------
extern void routes_list_append(SRoutes * pRoutes, SRoute * pRoute);
// ----- routes_list_remove_at --------------------------------------
extern void routes_list_remove_at(SRoutes * pRoutes, int iIndex);
// ----- routes_list_get_at -----------------------------------------
SRoute * routes_list_get_at(SRoutes * pRoutes, const int iIndex);
// ----- routes_list_get_num ----------------------------------------
extern int routes_list_get_num(SRoutes * pRoutes);
// ----- routes_list_dump -------------------------------------------
extern void routes_list_dump(SLogStream * pStream, SRoutes * pRoutes);
// -----[ routes_list_for_each ]-------------------------------------
extern int routes_list_for_each(SRoutes * pRoutes, FArrayForEach fForEach,
				void * pContext);

#endif
