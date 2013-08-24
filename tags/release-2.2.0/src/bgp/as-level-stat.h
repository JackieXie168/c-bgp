// ==================================================================
// @(#)as-level-stat.h
//
// Provide structures and functions to report some AS-level
// topologies statistics.
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 21/06/2007
// $Id: as-level-stat.h,v 1.2 2008-04-14 09:14:35 bqu Exp $
// ==================================================================

#ifndef __BGP_ASLEVEL_STAT_H__
#define __BGP_ASLEVEL_STAT_H__

#include <libgds/log.h>

#include <bgp/as-level.h>

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ aslevel_stat_degree ]------------------------------------
  void aslevel_stat_degree(SLogStream * pStream, SASLevelTopo * pTopo,
			   int iCumulated, int iNormalized,
			   int iInverse);

#ifdef __cplusplus
}
#endif

#endif /* __BGP_ASLEVEL_STAT_H__ */
