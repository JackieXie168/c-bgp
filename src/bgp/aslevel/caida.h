// ==================================================================
// @(#)caida.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 30/04/2007
// $Id: caida.h,v 1.1 2009-03-24 13:39:08 bqu Exp $
// ==================================================================

#ifndef __BGP_ASLEVEL_CAIDA_H__
#define __BGP_ASLEVEL_CAIDA_H__

#include <stdio.h>

#include <libgds/stream.h>

#include <net/network.h>
#include <net/prefix.h>
#include <bgp/aslevel/types.h>

// ----- AS relationships and policies -----
#define CAIDA_REL_CUST_PROV -1
#define CAIDA_REL_PEER_PEER 0
#define CAIDA_REL_PROV_CUST 1
#define CAIDA_REL_SIBLINGS  2

#ifdef __cplusplus
extern "C" {
#endif

  // ----- caida_load ---------------------------------------------
  int caida_parser(FILE * file, as_level_topo_t * topo,
		   unsigned int * line_number);

#ifdef __cplusplus
}
#endif

#endif /* __BGP_ASLEVEL_CAIDA_H__ */

