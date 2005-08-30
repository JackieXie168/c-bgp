// ==================================================================
// @(#)domain.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 13/02/2002
// @lastdate 04/08/2005
// ==================================================================

#ifndef __BGP_DOMAIN_H__
#define __BGP_DOMAIN_H__

#include <libgds/radix-tree.h>

#include <bgp/as_t.h>

// ----- bgp_domain_create ------------------------------------------
extern SBGPDomain * bgp_domain_create(uint16_t uNumber);

// ----- bgp_domain_destroy -----------------------------------------
extern void bgp_domain_destroy(SBGPDomain ** ppDomain);

// ----- bgp_domain_add_router --------------------------------------
extern void bgp_domain_add_router(SBGPDomain * pDomain, SBGPRouter * pRouter);

// ----- bgp_domain_routers_for_each --------------------------------
extern int bgp_domain_routers_for_each(SBGPDomain * pDomain,
				       FRadixTreeForEach fForEach,
				       void * pContext);

// ----- exists_bgp_domain ------------------------------------------
extern int exists_bgp_domain(uint16_t uNumber);

// ----- get_bgp_domain ---------------------------------------------
extern SBGPDomain * get_bgp_domain(uint16_t uNumber);

// ----- register_bgp_domain ----------------------------------------
extern void register_bgp_domain(SBGPDomain * pDomain);

// ----- bgp_domain_rescan ------------------------------------------
extern int bgp_domain_rescan(SBGPDomain * pDomain);
// ----- bgp_domain_dump_recorded_route -----------------------------
extern int bgp_domain_dump_recorded_route(FILE * pStream,
					  SBGPDomain * pDomain, 
					  SNetDest sDest,
					  const uint8_t uOptions);
// ----- bgp_domain_full_mesh ---------------------------------------
extern int bgp_domain_full_mesh(SBGPDomain * pDomain);

// ----- _bgp_domain_init -------------------------------------------
extern void _bgp_domain_init();
// ----- _bgp_domain_destroy ----------------------------------------
extern void _bgp_domain_destroy();

#endif
