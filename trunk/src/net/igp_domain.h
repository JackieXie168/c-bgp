// ==================================================================
// @(#)domain.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 13/02/2002
// $Id: igp_domain.h,v 1.8 2008-04-11 11:03:06 bqu Exp $
// ==================================================================

#ifndef __IGP_DOMAIN_H__
#define __IGP_DOMAIN_H__

#include <libgds/log.h>
#include <libgds/radix-tree.h>
#include <net/net_types.h>
#include <net/network.h>

// -----[ FIGPDomainsForEach ]---------------------------------------
typedef int (*FIGPDomainsForEach)(igp_domain_t * domain, void * ctx);

#define ospf_domain_create(N) igp_domain_create(N, IGP_DOMAIN_OSPF)

#ifdef __cplusplus
extern "C" {
#endif

  // ----- igp_domain_create ----------------------------------------
  igp_domain_t * igp_domain_create(uint16_t id, igp_domain_type_t type);
  // ----- igp_domain_set_ecmp --------------------------------------
  int igp_domain_set_ecmp(igp_domain_t * domain, int state);
  // ----- igp_domain_destroy ---------------------------------------
  void igp_domain_destroy(igp_domain_t ** domain_ref);
  // ----- igp_domain_add_router ------------------------------------
  int igp_domain_add_router(igp_domain_t * domain, net_node_t * router);
  // -----[ igp_domains_for_each ]-----------------------------------
  int igp_domains_for_each(FIGPDomainsForEach for_each, void * ctx);
  // ----- igp_domain_routers_for_each ------------------------------
  int igp_domain_routers_for_each(igp_domain_t * domain,
				  FRadixTreeForEach for_each,
				  void * ctx);
  // ----- exists_igp_domain ----------------------------------------
  int exists_igp_domain(uint16_t id);
  // ----- get_igp_domain -------------------------------------------
  igp_domain_t * get_igp_domain(uint16_t id);
  // ----- register_igp_domain --------------------------------------
  int register_igp_domain(igp_domain_t * domain);
  // ----- igp_domain_contains_router -------------------------------
  int igp_domain_contains_router(igp_domain_t * domain,
				 net_node_t * node);
  // ----- igp_domain_contains_router_by_addr -----------------------
  int igp_domain_contains_router_by_addr(igp_domain_t * domain,
					 net_addr_t addr);
  // ----- igp_domain_dump ------------------------------------------
  int igp_domain_dump(SLogStream * stream, igp_domain_t * domain);
  // ----- igp_domain_info ------------------------------------------
  void igp_domain_info(SLogStream * stream, igp_domain_t * domain);
  // ----- igp_domain_compute ---------------------------------------
  int igp_domain_compute(igp_domain_t * domain);
  

  ///////////////////////////////////////////////////////////////////
  // INITIALIZATION AND FINALIZATION
  ///////////////////////////////////////////////////////////////////

  // ----- _igp__domain_init ----------------------------------------
  void _igp_domain_init();
  // ----- _igp__domain_destroy -------------------------------------
  void _igp_domain_destroy();
  
#ifdef __cplusplus
}
#endif

#endif
