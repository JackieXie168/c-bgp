// ==================================================================
// @(#)route_map.h
//
// @author Sebastien Tandel (sta@info.ucl.ac.be)
// @date 13/12/2004
// @lastdate 14/12/2004
// ==================================================================


#ifndef _CBGP_ROUTE_MAP_
#define _CBGP_ROUTE_MAP_

#include <bgp/filter.h>

// ------ route_map_add ----------------------------------------------
int route_map_add(char * pcRouteMapName, SFilter * pFilter);
// ------ route_map_del ----------------------------------------------
int route_map_del(char * pcRouteMapName);
// ----- route_map_get -----------------------------------------------
SFilter * route_map_get(char * pcRouteMapName);
// ----- route_map_replace -------------------------------------------
int route_map_replace(char * pcRouteMapName, SFilter * pFilter);
#endif //_CBGP_ROUTE_MAP_
