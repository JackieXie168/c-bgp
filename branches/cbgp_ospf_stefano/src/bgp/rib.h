// ==================================================================
// @(#)rib.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 04/12/2002
// @lastdate 05/04/2005
// ==================================================================

#ifndef __RIB_H__
#define __RIB_H__

#include <net/prefix.h>
#include <bgp/rib_t.h>
#include <bgp/route.h>

#define RIB_OPTION_ALIAS 0x01

// ----- rib_create -------------------------------------------------
extern SRIB * rib_create(uint8_t uOptions);
// ----- rib_destroy ------------------------------------------------
extern void rib_destroy(SRIB ** ppRIB);
// ----- rib_find_best ----------------------------------------------
extern SRoute * rib_find_best(SRIB * pRIB, SPrefix sPrefix);
// ----- rib_find_exact ---------------------------------------------
extern SRoute * rib_find_exact(SRIB * pRIB, SPrefix sPrefix);
// ----- rib_add_route ----------------------------------------------
extern int rib_add_route(SRIB * pRIB, SRoute * pRoute);
// ----- rib_replace_route ------------------------------------------
extern int rib_replace_route(SRIB * pRIB, SRoute * pRoute);
// ----- rib_remove_route -------------------------------------------
extern int rib_remove_route(SRIB * pRIB, SPrefix sPrefix);
// ----- rib_for_each -----------------------------------------------
extern int rib_for_each(SRIB * pRIB, FRadixTreeForEach fForEach,
			void * pContext);

#endif