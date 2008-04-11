// ==================================================================
// @(#)domain.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 13/02/2002
// $Id: domain.h,v 1.7 2008-04-11 11:03:06 bqu Exp $
// ==================================================================

#ifndef __BGP_DOMAIN_H__
#define __BGP_DOMAIN_H__

#include <libgds/log.h>
#include <libgds/radix-tree.h>

#include <bgp/as_t.h>

// -----[ FBGPDomainsForEach ]---------------------------------------
typedef int (*FBGPDomainsForEach)(bgp_domain_t * domain, void * ctx);

#ifdef __cplusplus
extern "C" {
#endif

  // ----- bgp_domain_create ----------------------------------------
  bgp_domain_t * bgp_domain_create(uint16_t asn);
  // ----- bgp_domain_destroy ---------------------------------------
  void bgp_domain_destroy(bgp_domain_t ** domain_ref);
  // -----[ bgp_domains_for_each ]-----------------------------------
  int bgp_domains_for_each(FBGPDomainsForEach for_each, void * ctx);
  // ----- bgp_domain_add_router ------------------------------------
  void bgp_domain_add_router(bgp_domain_t * domain, bgp_router_t * router);
  // ----- bgp_domain_routers_for_each ------------------------------
  int bgp_domain_routers_for_each(bgp_domain_t * domain,
				  FRadixTreeForEach for_each,
				  void * ctx);
    // ----- exists_bgp_domain --------------------------------------
  int exists_bgp_domain(uint16_t asn);
  // ----- get_bgp_domain -------------------------------------------
  bgp_domain_t * get_bgp_domain(uint16_t asn);
  // ----- register_bgp_domain --------------------------------------
  void register_bgp_domain(bgp_domain_t * domain);
  // ----- bgp_domain_rescan ----------------------------------------
  int bgp_domain_rescan(bgp_domain_t * domain);
  // ----- bgp_domain_dump_recorded_route ---------------------------
  int bgp_domain_dump_recorded_route(SLogStream * stream,
				     bgp_domain_t * domain, 
				     SNetDest sDest,
				     uint8_t options);
  // ----- bgp_domain_full_mesh -------------------------------------
  int bgp_domain_full_mesh(bgp_domain_t * domain);
  

  ///////////////////////////////////////////////////////////////////
  // INITIALIZATION AND FINALIZATION
  ///////////////////////////////////////////////////////////////////

  // ----- _bgp_domain_init -----------------------------------------
  void _bgp_domain_init();
  // ----- _bgp_domain_destroy --------------------------------------
  void _bgp_domain_destroy();

#ifdef __cplusplus
}
#endif

#endif /* __BGP_DOMAIN_H__ */
