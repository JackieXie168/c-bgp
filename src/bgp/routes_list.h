// ==================================================================
// @(#)routes_list.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 20/02/2004
// @lastdate 11/03/2008
// ==================================================================

#ifndef __ROUTES_LIST_H__
#define __ROUTES_LIST_H__

#include <libgds/array.h>

#define ROUTES_LIST_OPTION_REF 0x01

#ifdef __cplusplus
extern "C" {
#endif

  // ----- routes_list_create ---------------------------------------
  bgp_routes_t * routes_list_create(uint8_t uOptions);
  // ----- routes_list_destroy --------------------------------------
  void routes_list_destroy(bgp_routes_t ** ppRoutes);
  // ----- routes_list_append ---------------------------------------
  void routes_list_append(bgp_routes_t * pRoutes, bgp_route_t * pRoute);
  // ----- routes_list_remove_at ------------------------------------
  void routes_list_remove_at(bgp_routes_t * pRoutes, unsigned int uIndex);
  // ----- routes_list_dump -----------------------------------------
  void routes_list_dump(SLogStream * pStream, bgp_routes_t * pRoutes);
  // -----[ routes_list_for_each ]-----------------------------------
  int routes_list_for_each(bgp_routes_t * pRoutes, FArrayForEach fForEach,
			   void * pContext);

  // -----[ bgp_routes_size ]----------------------------------------
  static inline unsigned int bgp_routes_size(bgp_routes_t * pRoutes) {
    return ptr_array_length(pRoutes);
  }
  // -----[ bgp_routes_at ]------------------------------------------
  static inline bgp_route_t * bgp_routes_at(bgp_routes_t * pRoutes,
					    unsigned int uIndex) {
    return pRoutes->data[uIndex];
  }
  
#ifdef __cplusplus
}
#endif

#endif /* __ROUTES_LIST_H__ */
