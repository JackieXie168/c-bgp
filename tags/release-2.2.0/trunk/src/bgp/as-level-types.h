// ==================================================================
// @(#)as-level-type.h
//
// Provides internal type definitions for AS-level topology
// structures.
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 30/04/2007
// $Id: as-level-types.h,v 1.2 2008-04-11 11:03:06 bqu Exp $
// ==================================================================

#ifndef __BGP_ASLEVEL_TYPES_H__
#define __BGP_ASLEVEL_TYPES_H__

// ----- AS-level domain structure -----
struct TASLevelDomain {
  asn_t          uASN;
  SPtrArray    * pNeighbors;
  bgp_router_t * pRouter;
};

// ----- AS-level link structure -----
struct TASLevelLink {
  SASLevelDomain * pNeighbor;
  peer_type_t      tPeerType;
};

// ----- AS-level topology structure -----
#define ASLEVEL_TOPO_AS_CACHE_SIZE 2
struct TASLevelTopo {
  SPtrArray          * pDomains;
  SASLevelDomain     * pCacheDomains[ASLEVEL_TOPO_AS_CACHE_SIZE];
  FASLevelAddrMapper   fAddrMapper;
  uint8_t              uState;
};

#endif /* __BGP_ASLEVEL_TYPES_H__ */
