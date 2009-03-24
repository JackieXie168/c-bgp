// ==================================================================
// @(#)route_map.h
//
// @author Sebastien Tandel (sta@info.ucl.ac.be)
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 13/12/2004
// $Id: route_map.h,v 1.4 2009-03-24 15:52:01 bqu Exp $
// ==================================================================

#ifndef __BGP_ROUTE_MAP_H__
#define __BGP_ROUTE_MAP_H__

#include <bgp/filter/types.h>

typedef struct {
  char         * name;
  bgp_filter_t * filter;
} _route_map_t;

#ifdef __cplusplus
extern "C" {
#endif

  // ------[ route_map_add ]-----------------------------------------
  int route_map_add(const char * name, bgp_filter_t * filter);
  // ------[ route_map_remove ]--------------------------------------
  int route_map_remove(const char * name);
  // -----[ route_map_get ]------------------------------------------
  bgp_filter_t * route_map_get(const char * name);
  // -----[ route_map_enum ]-----------------------------------------
  gds_enum_t * route_map_enum();


  ///////////////////////////////////////////////////////////////////
  // INITIALIZATION AND FINALIZATION
  ///////////////////////////////////////////////////////////////////
  
  // -----[ _route_maps_init ]---------------------------------------
  void _route_maps_init();
  // -----[ _route_maps_destroy ]------------------------------------
  void _route_maps_destroy();

#ifdef __cplusplus
}
#endif

#endif /* __BGP_ROUTE_MAP_H__ */
