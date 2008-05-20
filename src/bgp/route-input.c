// ==================================================================
// @(#)route-input.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 21/05/2007
// $Id: route-input.c,v 1.3 2008-05-20 11:58:15 bqu Exp $
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <string.h>

#include <bgp/cisco.h>
#include <bgp/mrtd.h>
#include <bgp/route-input.h>
#include <bgp/routes_list.h>

static char * INPUT_TYPE_STR[BGP_ROUTES_INPUT_MAX]=
{
  "mrt-ascii",
#ifdef HAVE_BGPDUMP
  "mrt-binary",
#endif /* HAVE_BGPDUMP */
  "cisco"
};

// -----[ bgp_routes_str2format ]------------------------------------
int bgp_routes_str2format(const char * format_str,
			  bgp_input_type_t * format)
{
  unsigned int index;

  for (index= 0; index < BGP_ROUTES_INPUT_MAX; index++) {
    if (!strcmp(format_str, INPUT_TYPE_STR[index])) {
      *format= index;
      return 0;
    }
  }
  return -1;
}

// -----[ bgp_routes_load ]------------------------------------------
int bgp_routes_load(const char * filename, bgp_input_type_t format,
		    FBGPRouteHandler handler, void * context)
{
  switch (format) {
  case BGP_ROUTES_INPUT_MRT_ASC:
    return mrtd_ascii_load(filename, handler, context);
#ifdef HAVE_BGPDUMP
  case BGP_ROUTES_INPUT_MRT_BIN:
    return mrtd_binary_load(filename, handler, context);
#endif
  case BGP_ROUTES_INPUT_CISCO:
    return -1;//cisco_load(filename, handler, context);
  default:
    return -1;
  }
  return -1;
}

// -----[ _bgp_routes_load_list_handler ]----------------------------
static int _bgp_routes_load_list_handler(int status,
					 bgp_route_t * route,
					 net_addr_t peer_addr,
					 unsigned int peer_asn,
					 void * context)
{
  bgp_routes_t * routes= (bgp_routes_t *) context;

  if (status == BGP_ROUTES_INPUT_STATUS_OK)
    routes_list_append(routes, route);

  return BGP_ROUTES_INPUT_SUCCESS;
}

// -----[ bgp_routes_load_list ]-------------------------------------
/**
 *
 */
bgp_routes_t * bgp_routes_load_list(const char * filename,
				    bgp_input_type_t format)
{
  bgp_routes_t * routes= routes_list_create(ROUTES_LIST_OPTION_REF);
  int result;

  result= bgp_routes_load(filename, format, _bgp_routes_load_list_handler,
			  routes);  
  if (result != 0) {
    // TODO: We MUST destroy all routes here !!!
    routes_list_destroy(&routes);
    return NULL;
  }

  return routes;
}

