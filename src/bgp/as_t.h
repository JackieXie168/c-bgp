// ==================================================================
// @(#)as_t.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 23/05/2003
// @lastdate 27/02/2004
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
  uint32_t uTBID;
  SPtrArray * pPeers;
  SRIB * pLocRIB;
  SPtrArray * pLocalNetworks;
  SNetNode * pNode;
  FTieBreakFunction fTieBreak;
  cluster_id_t tClusterID;
  int iRouteReflector;
} SAS;

typedef SAS SBGPRouter;

#endif
