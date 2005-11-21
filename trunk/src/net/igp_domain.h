// ==================================================================
// @(#)domain.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 13/02/2002
// @lastdate 21/11/2005
// ==================================================================

#ifndef __IGP_DOMAIN_H__
#define __IGP_DOMAIN_H__

#include <libgds/radix-tree.h>
#include <net/net_types.h>
#include <net/network.h>

#define ospf_domain_create(N) igp_domain_create(N, DOMAIN_OSPF)

// ----- igp_domain_create ------------------------------------------
extern SIGPDomain * igp_domain_create(uint16_t uNumber, EDomainType tType);
// ----- igp_domain_set_ecmp ----------------------------------------
extern int igp_domain_set_ecmp(SIGPDomain * pDomain, int iState);
// ----- igp_domain_destroy -----------------------------------------
extern void igp_domain_destroy(SIGPDomain ** ppDomain);
// ----- igp_domain_add_router --------------------------------------
extern int igp_domain_add_router(SIGPDomain * pDomain, SNetNode * pRouter);
// ----- igp_domain_routers_for_each --------------------------------
extern int igp_domain_routers_for_each(SIGPDomain * pDomain,
				       FRadixTreeForEach fForEach,
				       void * pContext);
// ----- exists_igp_domain ------------------------------------------
extern int exists_igp_domain(uint16_t uNumber);
// ----- get_igp_domain ---------------------------------------------
extern SIGPDomain * get_igp_domain(uint16_t uNumber);
// ----- register_igp_domain ----------------------------------------
extern void register_igp_domain(SIGPDomain * pDomain);
// ----- igp_domain_contains_router ---------------------------------
extern int igp_domain_contains_router(SIGPDomain * pDomain,
				      SNetNode * pNode);
// ----- igp_domain_contains_router_by_addr -------------------------
extern int igp_domain_contains_router_by_addr(SIGPDomain * pDomain,
					      net_addr_t tAddr);
// ----- igp_domain_dump --------------------------------------------
extern int igp_domain_dump(FILE * pStream, SIGPDomain * pDomain);
// ----- igp_domain_info --------------------------------------------
extern void igp_domain_info(FILE * pStream, SIGPDomain * pDomain);
// ----- igp_domain_compute -----------------------------------------
extern int igp_domain_compute(SIGPDomain * pDomain);


// ----- _igp__domain_init ------------------------------------------
extern void _igp_domain_init();
// ----- _igp__domain_destroy ---------------------------------------
extern void _igp_domain_destroy();
// ----- _igp_domain_test -------------------------------------------
extern int _igp_domain_test();

#endif
