// ==================================================================
// @(#)as_t.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @author Sebastien Tandel (standel@info.ucl.ac.be)
// @date 23/05/2003
// @lastdate 28/02/2006
// ==================================================================

#ifndef __AS_T_H__
#define __AS_T_H__

#include <libgds/array.h>
#include <bgp/rib_t.h>
#include <bgp/route_reflector.h>
#include <bgp/tie_breaks.h>
#include <net/network.h>

typedef struct {
  uint16_t uNumber;
  char * pcName;
  SRadixTree * pRouters;
} SBGPDomain;

typedef struct {
  uint16_t uNumber;            // AS-number of the router
  net_addr_t tRouterID;        // Router-ID of the router
  uint32_t uTBID;              // Tie-break ID (should be removed...)
  SPtrArray * pPeers;          // List of neighbor routers
  SRIB * pLocRIB;
  SPtrArray * pLocalNetworks;  // List of locally originated prefixes
  FTieBreakFunction fTieBreak; // Cluster-ID
  cluster_id_t tClusterID;
  int iRouteReflector;
  SNetNode * pNode;            // Reference to the node running this BGP router
  SBGPDomain * pDomain;        // Reference to the AS containing this router
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  SPtrArray * pWaltonLimitPeers;  //This is a list of neighbors sorted on 
				  //the walton limit number
				  //TODO : Is it possible to get rid of the SPtrArray
				  //* pPeers when using walton ?
#endif
} SBGPRouter;

#endif
