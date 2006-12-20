// ==================================================================
// @(#)stats.h
//
// @author Bruno Quoitin (bqu@infonet.fundp.ac.be)
// @date 06/08/2003
// @lastdate 06/08/2003
// ==================================================================

#ifndef __BGP_STATS_H__
#define __BGP_STATS_H__

typedef struct {
  uint32_t uCntRulePref;
  uint32_t uCntRulePath;
  uint32_t uCntRuleTieBreak;
} SASDPStats;

// ----- dp_stats_create --------------------------------------------
extern SASDPStats * dp_stats_create();
// ----- dp_stats_destroy -------------------------------------------
extern void dp_stats_destroy(SASDPStats ** ppStats);

#endif
