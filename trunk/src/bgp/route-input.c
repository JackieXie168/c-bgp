// ==================================================================
// @(#)route-input.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 21/05/2007
// @lastdate 21/05/2007
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <string.h>

#include <bgp/cisco.h>
#include <bgp/mrtd.h>
#include <bgp/route-input.h>
#include <bgp/routes_list.h>

// -----[ bgp_routes_str2format ]------------------------------------
int bgp_routes_str2format(const char * pcFormat, uint8_t * puFormat)
{
  if (!strcmp(pcFormat, "mrt-ascii") || !strcmp(pcFormat, "default")) {
    *puFormat= BGP_ROUTES_INPUT_MRT_ASC;
    return 0;
#ifdef HAVE_BGPDUMP
  } else if (!strcmp(pcFormat, "mrt-binary")) {
    *puFormat= BGP_ROUTES_INPUT_MRT_BIN;
    return 0;
#endif
  } else if (!strcmp(pcFormat, "cisco")) {
    *puFormat= BGP_ROUTES_INPUT_CISCO;
    return 0;
  }
  return -1;
}


// -----[ bgp_routes_load ]------------------------------------------
int bgp_routes_load(const char * pcFileName, uint8_t uFormat,
		    FBGPRouteHandler fHandler, void * pContext)
{
  switch (uFormat) {
  case BGP_ROUTES_INPUT_MRT_ASC:
    return mrtd_ascii_load(pcFileName, fHandler, pContext);
#ifdef HAVE_BGPDUMP
  case BGP_ROUTES_INPUT_MRT_BIN:
    return mrtd_binary_load(pcFileName, fHandler, pContext);
#endif
  case BGP_ROUTES_INPUT_CISCO:
    return -1;//cisco_load(pcFileName, fHandler, pContext);
  default:
    return -1;
  }
  return -1;
}

// -----[ _bgp_routes_load_list_handler ]----------------------------
static int _bgp_routes_load_list_handler(int iStatus,
					 SRoute * pRoute,
					 net_addr_t tPeerAddr,
					 unsigned int uPeerAS,
					 void * pContext)
{
  SRoutes * pRoutes= (SRoutes *) pContext;

  if (iStatus == BGP_ROUTES_INPUT_STATUS_OK)
    routes_list_append(pRoutes, pRoute);

  return BGP_ROUTES_INPUT_SUCCESS;
}

// -----[ bgp_routes_load_list ]-------------------------------------
/**
 *
 */
SRoutes * bgp_routes_load_list(const char * pcFileName, uint8_t uFormat)
{
  SRoutes * pRoutes= routes_list_create(ROUTES_LIST_OPTION_REF);
  int iResult;

  iResult= bgp_routes_load(pcFileName, uFormat, _bgp_routes_load_list_handler,
			   pRoutes);  
  if (iResult != 0) {
    // TODO: We MUST destroy all routes here !!!
    routes_list_destroy(&pRoutes);
    return NULL;
  }

  return pRoutes;
}

