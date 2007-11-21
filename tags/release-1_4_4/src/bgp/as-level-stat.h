// ==================================================================
// @(#)as-level-stat.h
//
// Provide structures and functions to report some AS-level
// topologies statistics.
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 21/06/2007
// @lastdate 21/06/2007
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
