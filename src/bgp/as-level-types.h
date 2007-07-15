// ==================================================================
// @(#)as-level-type.h
//
// Provides internal type definitions for AS-level topology
// structures.
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 30/04/2007
// @lastdate 24/05/2007
// ==================================================================

#ifndef __BGP_ASLEVEL_TYPES_H__
#define __BGP_ASLEVEL_TYPES_H__

// ----- AS-level domain structure -----
struct TASLevelDomain {
  uint16_t uASN;
  SPtrArray * pNeighbors;
  SBGPRouter * pRouter;
};

// ----- AS-level link structure -----
struct TASLevelLink {
  SASLevelDomain * pNeighbor;
  peer_type_t tPeerType;
};

// ----- AS-level topology structure -----
#define ASLEVEL_TOPO_AS_CACHE_SIZE 2
struct TASLevelTopo {
  SPtrArray * pDomains;
  SASLevelDomain * pCacheDomains[ASLEVEL_TOPO_AS_CACHE_SIZE];
  FASLevelAddrMapper fAddrMapper;
  uint8_t uState;
};

#endif /* __BGP_ASLEVEL_TYPES_H__ */
