// ==================================================================
// @(#)routes_list.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 20/02/2004
// @lastdate 13/07/2007
// ==================================================================

#ifndef __ROUTES_LIST_H__
#define __ROUTES_LIST_H__

#include <libgds/array.h>

#define ROUTES_LIST_OPTION_REF 0x01

typedef SPtrArray SRoutes;

#ifdef __cplusplus
extern "C" {
#endif

  // ----- routes_list_create ---------------------------------------
  SRoutes * routes_list_create(uint8_t uOptions);
  // ----- routes_list_destroy --------------------------------------
  void routes_list_destroy(SRoutes ** ppRoutes);
  // ----- routes_list_append ---------------------------------------
  void routes_list_append(SRoutes * pRoutes, SRoute * pRoute);
  // ----- routes_list_remove_at ------------------------------------
  void routes_list_remove_at(SRoutes * pRoutes, int iIndex);
  // ----- routes_list_get_at ---------------------------------------
  SRoute * routes_list_get_at(SRoutes * pRoutes, const int iIndex);
  // ----- routes_list_get_num --------------------------------------
  int routes_list_get_num(SRoutes * pRoutes);
  // ----- routes_list_dump -----------------------------------------
  void routes_list_dump(SLogStream * pStream, SRoutes * pRoutes);
  // -----[ routes_list_for_each ]-----------------------------------
  int routes_list_for_each(SRoutes * pRoutes, FArrayForEach fForEach,
			   void * pContext);
  
#ifdef __cplusplus
}
#endif

#endif /* __ROUTES_LIST_H__ */
