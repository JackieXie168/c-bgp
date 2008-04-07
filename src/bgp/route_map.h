// ==================================================================
// @(#)route_map.h
//
// @author Sebastien Tandel (sta@info.ucl.ac.be)
// @date 13/12/2004
// @lastdate 17/05/2005
// ==================================================================


#ifndef __BGP_ROUTE_MAP_H__
#define __BGP_ROUTE_MAP_H__

#include <bgp/filter.h>

#ifdef __cplusplus
extern "C" {
#endif

  // ------ route_map_add ----------------------------------------------
  int route_map_add(char * pcRouteMapName, bgp_filter_t * pFilter);
  // ------ route_map_del ----------------------------------------------
  int route_map_del(char * pcRouteMapName);
  // ----- route_map_get -----------------------------------------------
  bgp_filter_t * route_map_get(char * pcRouteMapName);
  // ----- route_map_replace -------------------------------------------
  int route_map_replace(char * pcRouteMapName, bgp_filter_t * pFilter);
  
  // ----- route_map_init ----------------------------------------------
  void _route_map_init();
  // ----- route_map_destroy ------------------------------------------
  void _route_map_destroy();

#ifdef __cplusplus
}
#endif

#endif /* __BGP_ROUTE_MAP_H__ */
