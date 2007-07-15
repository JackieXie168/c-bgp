// ==================================================================
// @(#)rexford.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 28/07/2003
// @lastdate 03/05/2007
// ==================================================================

#ifndef __BGP_REXFORD_H__
#define __BGP_REXFORD_H__

#include <libgds/log.h>

#include <bgp/as-level.h>
#include <net/prefix.h>
#include <net/network.h>
#include <stdio.h>

// ----- AS relationships and policies -----
#define REXFORD_REL_PEER_PEER 0
#define REXFORD_REL_PROV_CUST 1

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ rexford_parser ]-----------------------------------------
  int rexford_parser(FILE * pStream, SASLevelTopo * pTopo,
		     unsigned int * puLineNumber);

#ifdef __cplusplus
}
#endif

#endif /* __BGP_REXFORD_H__ */

