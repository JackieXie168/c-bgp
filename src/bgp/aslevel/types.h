// ==================================================================
// @(#)type.h
//
// Provides internal type definitions for AS-level topology
// structures.
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 30/04/2007
// $Id: types.h,v 1.1 2009-03-24 13:39:08 bqu Exp $
// ==================================================================

/**
 * \file
 * Provide main BGP AS-level data structures.
 */

#ifndef __BGP_ASLEVEL_TYPES_H__
#define __BGP_ASLEVEL_TYPES_H__

#include <bgp/types.h>

typedef uint8_t peer_type_t;

// -----[ as_level_addr_mapper_f ]-----------------------------------
/** Maps an ASN to a router identifier (IPv4 address). */
typedef net_addr_t (*as_level_addr_mapper_f)(asn_t asn);

// -----[ as_level_domain_t ]----------------------------------------
/** Definition of an AS. */
typedef struct as_level_domain_t {
  asn_t          asn;
  ptr_array_t  * neighbors;
  bgp_router_t * router;
} as_level_domain_t;

// -----[ as_level_link_t ]------------------------------------------
/** Definition of an AS-level relationship. */
typedef struct as_level_link {
  as_level_domain_t * neighbor;
  bgp_peer_t        * peer;
  peer_type_t         peer_type;
} as_level_link_t;

/** Domains cache depth. */
#define ASLEVEL_TOPO_AS_CACHE_SIZE 2

// -----[ as_level_topo_t ]------------------------------------------
/** Definition of an AS-level topology. */
typedef struct as_level_topo_t {
  ptr_array_t            * domains;
  as_level_domain_t      * cached_domains[ASLEVEL_TOPO_AS_CACHE_SIZE];
  as_level_addr_mapper_f   addr_mapper;
  uint8_t                  state;
} as_level_topo_t;

#endif /* __BGP_ASLEVEL_TYPES_H__ */
