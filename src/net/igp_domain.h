// ==================================================================
// @(#)domain.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 13/02/2002
// @lastdate 17/05/2005
// ==================================================================

#ifndef __IGP_DOMAIN_H__
#define __IGP_DOMAIN_H__

#include <libgds/radix-tree.h>
#include <net/network.h>
typedef struct {
  uint16_t uNumber;
  char * pcName;
  SRadixTree * pRouters;
} SIGPDomain;

// ----- igp_domain_create ------------------------------------------
extern SIGPDomain * ospf_domain_create(uint16_t uNumber);


// ----- igp_domain_destroy -----------------------------------------
extern void igp_domain_destroy(SIGPDomain ** ppDomain);

// ----- igp_domain_add_router --------------------------------------
extern void igp_domain_add_router(SIGPDomain * pDomain, SNetNode * pRouter);

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


// ----- _domain_init -----------------------------------------------
extern void _domain_init();
// ----- _domain_destroy --------------------------------------------
extern void _domain_destroy();
// ----- IGPdomain_test() --------------------------------------------
extern int IGPdomain_test();
#endif
