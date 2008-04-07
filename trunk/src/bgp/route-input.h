// ==================================================================
// @(#)route-input.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 21/05/2007
// @lastdate 11/03/2008
// ==================================================================

#ifndef __BGP_ROUTE_INPUT_H__
#define __BGP_ROUTE_INPUT_H__

#include <net/prefix.h>
#include <bgp/route_t.h>
#include <bgp/routes_list.h>

// ----- BGP Routes Input Formats -----
#define BGP_ROUTES_INPUT_MRT_ASC 0
#define BGP_ROUTES_INPUT_MRT_BIN 1
#define BGP_ROUTES_INPUT_CISCO   2

// ----- Error codes -----
#define BGP_ROUTES_INPUT_SUCCESS          0
#define BGP_ROUTES_INPUT_ERROR_UNEXPECTED -1
#define BGP_ROUTES_INPUT_ERROR_FILE_OPEN  -2
#define BGP_ROUTES_INPUT_ERROR_SYNTAX     -3
#define BGP_ROUTES_INPUT_ERROR_IGNORED    -4
#define BGP_ROUTES_INPUT_ERROR_FILTERED   -5

// ----- Status codes -----
#define BGP_ROUTES_INPUT_STATUS_OK       0
#define BGP_ROUTES_INPUT_STATUS_FILTERED 1
#define BGP_ROUTES_INPUT_STATUS_IGNORED  2

// ----- BGP Route Handler -----
typedef int (*FBGPRouteHandler)(int iStatus,
				bgp_route_t * pRoute, net_addr_t tPeerAddr,
				unsigned int uPeerAS, void * pContext);

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ bgp_routes_str2format ]----------------------------------
  int bgp_routes_str2format(const char * pcFormat, uint8_t * puFormat);
  // -----[ bgp_routes_load ]----------------------------------------
  int bgp_routes_load(const char * pcFileName, uint8_t uFormat,
		      FBGPRouteHandler fHandler, void * pContext);
  // -----[ bgp_routes_load_list ]-----------------------------------
  bgp_routes_t * bgp_routes_load_list(const char * pcFileName, uint8_t uFormat);

#ifdef __cplusplus
}
#endif

#endif /* __BGP_ROUTE_INPUT_H__ */
