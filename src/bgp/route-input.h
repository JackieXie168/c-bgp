// ==================================================================
// @(#)route-input.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 21/05/2007
// $Id: route-input.h,v 1.3 2008-05-20 11:58:15 bqu Exp $
// ==================================================================

#ifndef __BGP_ROUTE_INPUT_H__
#define __BGP_ROUTE_INPUT_H__

#include <net/prefix.h>
#include <bgp/route_t.h>
#include <bgp/routes_list.h>

// ----- BGP Routes Input Formats -----
typedef enum {
  BGP_ROUTES_INPUT_MRT_ASC,
#ifdef HAVE_BGPDUMP
  BGP_ROUTES_INPUT_MRT_BIN,
#endif /* HAVE_BGPDUMP */
  BGP_ROUTES_INPUT_CISCO,
  BGP_ROUTES_INPUT_MAX
} bgp_input_type_t;

// ----- Error codes -----
#define BGP_ROUTES_INPUT_SUCCESS           0
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
typedef int (*FBGPRouteHandler)(int status,
				bgp_route_t * route, net_addr_t peer_addr,
				unsigned int peer_asn, void * context);

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ bgp_routes_str2format ]----------------------------------
  int bgp_routes_str2format(const char * format_str,
			    bgp_input_type_t * format);
  // -----[ bgp_routes_load ]----------------------------------------
  int bgp_routes_load(const char * filename,
		      bgp_input_type_t format,
		      FBGPRouteHandler handler, void * context);
  // -----[ bgp_routes_load_list ]-----------------------------------
  bgp_routes_t * bgp_routes_load_list(const char * filename,
				      bgp_input_type_t format);

#ifdef __cplusplus
}
#endif

#endif /* __BGP_ROUTE_INPUT_H__ */
