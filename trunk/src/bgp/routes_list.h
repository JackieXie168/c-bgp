// ==================================================================
// @(#)routes_list.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 20/02/2004
// $Id: routes_list.h,v 1.7 2009-03-24 15:53:12 bqu Exp $
// ==================================================================

#ifndef __ROUTES_LIST_H__
#define __ROUTES_LIST_H__

#include <libgds/array.h>

#define ROUTES_LIST_OPTION_REF 0x01

#ifdef __cplusplus
extern "C" {
#endif

  // ----- routes_list_create ---------------------------------------
  bgp_routes_t * routes_list_create(uint8_t options);
  // ----- routes_list_destroy --------------------------------------
  void routes_list_destroy(bgp_routes_t ** routes_ref);
  // ----- routes_list_append ---------------------------------------
  void routes_list_append(bgp_routes_t * routes, bgp_route_t * route);
  // ----- routes_list_remove_at ------------------------------------
  void routes_list_remove_at(bgp_routes_t * routes, unsigned int index);
  // ----- routes_list_dump -----------------------------------------
  void routes_list_dump(gds_stream_t * stream, bgp_routes_t * routes);
  // -----[ routes_list_for_each ]-----------------------------------
  int routes_list_for_each(bgp_routes_t * routes,
			   gds_array_foreach_f foreach,
			   void * ctx);

  // -----[ bgp_routes_size ]----------------------------------------
  static inline unsigned int bgp_routes_size(bgp_routes_t * routes) {
    return ptr_array_length(routes);
  }
  // -----[ bgp_routes_at ]------------------------------------------
  static inline bgp_route_t * bgp_routes_at(bgp_routes_t * routes,
					    unsigned int index) {
    return routes->data[index];
  }
  
#ifdef __cplusplus
}
#endif

#endif /* __ROUTES_LIST_H__ */
