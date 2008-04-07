// ==================================================================
// @(#)domain.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 13/02/2002
// @lastdate 15/10/2007
// ==================================================================

#ifndef __IGP_DOMAIN_H__
#define __IGP_DOMAIN_H__

#include <libgds/log.h>
#include <libgds/radix-tree.h>
#include <net/net_types.h>
#include <net/network.h>

// -----[ FIGPDomainsForEach ]---------------------------------------
typedef int (*FIGPDomainsForEach)(SIGPDomain * pDomain, void * pContext);

#define ospf_domain_create(N) igp_domain_create(N, DOMAIN_OSPF)

#ifdef __cplusplus
extern "C" {
#endif

  // ----- igp_domain_create ----------------------------------------
  SIGPDomain * igp_domain_create(uint16_t uNumber, EDomainType tType);
  // ----- igp_domain_set_ecmp --------------------------------------
  int igp_domain_set_ecmp(SIGPDomain * pDomain, int iState);
  // ----- igp_domain_destroy ---------------------------------------
  void igp_domain_destroy(SIGPDomain ** ppDomain);
  // ----- igp_domain_add_router ------------------------------------
  int igp_domain_add_router(SIGPDomain * pDomain, net_node_t * pRouter);
  // -----[ igp_domains_for_each ]-----------------------------------
  int igp_domains_for_each(FIGPDomainsForEach fForEach, void * pContext);
  // ----- igp_domain_routers_for_each ------------------------------
  int igp_domain_routers_for_each(SIGPDomain * pDomain,
				  FRadixTreeForEach fForEach,
				  void * pContext);
  // ----- exists_igp_domain ----------------------------------------
  int exists_igp_domain(uint16_t uNumber);
  // ----- get_igp_domain -------------------------------------------
  SIGPDomain * get_igp_domain(uint16_t uNumber);
  // ----- register_igp_domain --------------------------------------
  int register_igp_domain(SIGPDomain * pDomain);
  // ----- igp_domain_contains_router -------------------------------
  int igp_domain_contains_router(SIGPDomain * pDomain,
				 net_node_t * pNode);
  // ----- igp_domain_contains_router_by_addr -----------------------
  int igp_domain_contains_router_by_addr(SIGPDomain * pDomain,
					 net_addr_t tAddr);
  // ----- igp_domain_dump ------------------------------------------
  int igp_domain_dump(SLogStream * pStream, SIGPDomain * pDomain);
  // ----- igp_domain_info ------------------------------------------
  void igp_domain_info(SLogStream * pStream, SIGPDomain * pDomain);
  // ----- igp_domain_compute ---------------------------------------
  int igp_domain_compute(SIGPDomain * pDomain);
  
  
  // ----- _igp__domain_init ----------------------------------------
  void _igp_domain_init();
  // ----- _igp__domain_destroy -------------------------------------
  void _igp_domain_destroy();
  // ----- _igp_domain_test -----------------------------------------
  int _igp_domain_test();
  
#ifdef __cplusplus
}
#endif

#endif
