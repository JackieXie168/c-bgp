// ==================================================================
// @(#)caida.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 30/04/2007
// @lastdate 02/05/2007
// ==================================================================

#ifndef __BGP_CAIDA_H__
#define __BGP_CAIDA_H__

#include <libgds/log.h>

#include <bgp/as_t.h>
#include <net/prefix.h>
#include <net/network.h>
#include <stdio.h>

// ----- AS relationships and policies -----
#define CAIDA_REL_CUST_PROV -1
#define CAIDA_REL_PEER_PEER 0
#define CAIDA_REL_PROV_CUST 1
#define CAIDA_REL_SIBLINGS  2

#ifdef __cplusplus
extern "C" {
#endif

  // ----- caida_load ---------------------------------------------
  int caida_parser(FILE * pStream, SASLevelTopo * pTopo,
		   unsigned int * puLineNumber);

#ifdef __cplusplus
}
#endif

#endif /* __BGP_CAIDA_H__ */

