// ==================================================================
// @(#)rexford.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 28/07/2003
// $Id: rexford.h,v 1.1 2009-03-24 13:39:08 bqu Exp $
// ==================================================================

#ifndef __BGP_ASLEVEL_REXFORD_H__
#define __BGP_ASLEVEL_REXFORD_H__

#include <stdio.h>

#include <bgp/aslevel/types.h>

// ----- AS relationships and policies -----
#define REXFORD_REL_PEER_PEER 0
#define REXFORD_REL_PROV_CUST 1

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ rexford_parser ]-----------------------------------------
  int rexford_parser(FILE * file, as_level_topo_t * topo,
		     unsigned int * line_number);

#ifdef __cplusplus
}
#endif

#endif /* __BGP_ASLEVEL_REXFORD_H__ */

