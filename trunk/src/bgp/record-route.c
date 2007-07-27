// ==================================================================
// @(#)record-route.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 22/05/2007
// @lastdate 22/05/2007
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <libgds/log.h>

#include <bgp/as_t.h>
#include <bgp/record-route.h>
#include <bgp/rib.h>
#include <net/prefix.h>
#include <net/protocol.h>

// -----[ bgp_record_route ]-----------------------------------------
/**
 * This function records the AS-path from one BGP router towards a
 * given prefix. The function has two modes:
 * - records all ASes
 * - records ASes once (do not record iBGP session crossing)
 */
int bgp_record_route(SBGPRouter * pRouter,
			    SPrefix sPrefix, SBGPPath ** ppPath,
			    int iPreserveDups)
{
  SBGPRouter * pCurrentRouter= pRouter;
  SBGPRouter * pPreviousRouter= NULL;
  SRoute * pRoute;
  SBGPPath * pPath= path_create();
  SNetNode * pNode;
  SNetProtocol * pProtocol;
  int iResult= AS_RECORD_ROUTE_UNREACH;

  *ppPath= NULL;

  while (pCurrentRouter != NULL) {
    
    // Is there, in the current node, a BGP route towards the given
    // prefix ?
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
    pRoute= rib_find_one_best(pCurrentRouter->pLocRIB, sPrefix);
#else
    pRoute= rib_find_best(pCurrentRouter->pLocRIB, sPrefix);
#endif
    if (pRoute != NULL) {
      
      // Record current node's AS-Num ??
      if ((pPreviousRouter == NULL) ||
	  (iPreserveDups ||
	   (pPreviousRouter->uNumber != pCurrentRouter->uNumber))) {
	if (path_append(&pPath, pCurrentRouter->uNumber) < 0) {
	  iResult= AS_RECORD_ROUTE_TOO_LONG;
	  break;
	}
      }
      
      // If the route's next-hop is this router, then the function
      // terminates.
      if (pRoute->pAttr->tNextHop == pCurrentRouter->pNode->tAddr) {
	iResult= AS_RECORD_ROUTE_SUCCESS;
	break;
      }
      
      // Otherwize, looks for next-hop router
      pNode= network_find_node(pRoute->pAttr->tNextHop);
      if (pNode == NULL)
	break;
      
      // Get the current node's BGP instance
      pProtocol= protocols_get(pNode->pProtocols, NET_PROTOCOL_BGP);
      if (pProtocol == NULL)
	break;
      pPreviousRouter= pCurrentRouter;
      pCurrentRouter= (SBGPRouter *) pProtocol->pHandler;
      
    } else
      break;
  }
  *ppPath= pPath;

  return iResult;
}

// -----[ bgp_dump_recorded_route ]----------------------------------
/**
 * This function dumps the result of a call to
 * 'bgp_router_record_route'.
 */
void bgp_dump_recorded_route(SLogStream * pStream, SBGPRouter * pRouter,
			     SPrefix sPrefix, SBGPPath * pPath,
			     int iResult)
{
  // Display record-route results
  ip_address_dump(pStream, pRouter->pNode->tAddr);
  log_printf(pStream, "\t");
  ip_prefix_dump(pStream, sPrefix);
  log_printf(pStream, "\t");
  switch (iResult) {
  case AS_RECORD_ROUTE_SUCCESS: log_printf(pStream, "SUCCESS"); break;
  case AS_RECORD_ROUTE_TOO_LONG: log_printf(pStream, "TOO_LONG"); break;
  case AS_RECORD_ROUTE_UNREACH: log_printf(pStream, "UNREACHABLE"); break;
  default:
    log_printf(pStream, "UNKNOWN_ERROR");
  }
  log_printf(pStream, "\t");
  path_dump(pStream, pPath, 0);
  log_printf(pStream, "\n");

  log_flush(pStream);
}
