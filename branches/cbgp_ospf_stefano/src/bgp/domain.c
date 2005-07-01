// ==================================================================
// @(#)domain.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 13/02/2002
// @lastdate 13/04/2005
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <libgds/memory.h>
#include <libgds/radix-tree.h>

#include <bgp/as.h>
#include <bgp/domain.h>
#include <net/network.h>

#define MAX_DOMAINS 65536

SBGPDomain * apDomains[MAX_DOMAINS];

// ----- bgp_domain_create ------------------------------------------
/**
 * Create a BGP domain (an Autonomous System).
 */
SBGPDomain * bgp_domain_create(uint16_t uNumber)
{
  SBGPDomain * pDomain= (SBGPDomain *) MALLOC(sizeof(SBGPDomain));
  pDomain->uNumber= uNumber;
  pDomain->pcName= NULL;

  /* Radix-tree with all routers. Destroy function is NULL. */
  pDomain->pRouters= radix_tree_create(32, NULL);

  return pDomain;
}

// ----- bgp_domain_destroy -----------------------------------------
/**
 * Destroy a BGP domain.
 */
void bgp_domain_destroy(SBGPDomain ** ppDomain)
{
  if (*ppDomain != NULL) {
    radix_tree_destroy(&((*ppDomain)->pRouters));
    FREE(*ppDomain);
    *ppDomain= NULL;
  }
}

// ----- bgp_domain_add_router --------------------------------------
/**
 * Add a reference to a router in the domain.
 *
 * Preconditions:
 * - the router must be non NULL;
 * - the router must not exist in the domain. Otherwise, it will be
 *   replaced by the new one and memory leaks as well as unexpected
 *   results may occur.
 */
void bgp_domain_add_router(SBGPDomain * pDomain, SBGPRouter * pRouter)
{
  radix_tree_add(pDomain->pRouters, pRouter->pNode->tAddr, 32, pRouter);
  pRouter->pDomain= pDomain;
}

// ----- bgp_domain_routers_for_each --------------------------------
/**
 * Call the given callback function for all routers registered in the
 * domain.
 */
int bgp_domain_routers_for_each(SBGPDomain * pDomain,
				FRadixTreeForEach fForEach,
				void * pContext)
{
  return radix_tree_for_each(pDomain->pRouters, fForEach, pContext);
}

// ----- exists_bgp_domain ------------------------------------------
/**
 * Return true (1) if the domain identified by the given AS number
 * exists. Otherwise, return false (0).
 */
int exists_bgp_domain(uint16_t uNumber)
{
  return (apDomains[uNumber] != NULL);
}

// ----- get_bgp_domain ---------------------------------------------
/**
 * Get the reference of a domain identified by its AS number. If the
 * domain does not exist, it is created and registered.
 */
SBGPDomain * get_bgp_domain(uint16_t uNumber)
{
  if (apDomains[uNumber] == NULL)
    apDomains[uNumber]= bgp_domain_create(uNumber);
  return apDomains[uNumber];
}

// ----- register_bgp_domain ----------------------------------------
/**
 * Register a new domain.
 *
 * Preconditions:
 * - the router must be non NULL;
 * - the router must not exist.
 */
void register_bgp_domain(SBGPDomain * pDomain)
{
  assert(apDomains[pDomain->uNumber] == NULL);
  apDomains[pDomain->uNumber]= pDomain;
}

// ----- bgp_domain_routers_rescan_fct ------------------------------
int bgp_domain_routers_rescan_fct_for_each(uint32_t uKey, uint8_t uKeyLen,
					   void * pItem, void * pContext)
{
  return bgp_router_scan_rib((SBGPRouter *) pItem);
}

// ----- bgp_domain_rescan ------------------------------------------
/**
 * Run the RIB rescan process in each router of the BGP domain.
 */
int bgp_domain_rescan(SBGPDomain * pDomain)
{
  return bgp_domain_routers_for_each(pDomain,
				     bgp_domain_routers_rescan_fct_for_each,
				     NULL);
}

typedef struct {
  FILE * pStream;
  SNetDest sDest;
  int iDelay;
  uint8_t uDeflection;
} SRecordRoute;

// ----- bgp_domain_routers_record_route_for_each --------------------
int bgp_domain_routers_record_route_for_each(uint32_t uKey, uint8_t uKeyLen,
					      void * pItem, void * pContext)
{
  SNetNode * pNode = (SNetNode *)((SBGPRouter *)pItem)->pNode;
  SRecordRoute * pCont = (SRecordRoute *)pContext;
  
  node_dump_recorded_route(pCont->pStream, pNode, pCont->sDest,
				      pCont->iDelay, pCont->uDeflection);

  return 0;
}


// ----- bgp_domain_record_route -------------------------------------
/**
 *
 */
int bgp_domain_dump_recorded_route(FILE * pStream, SBGPDomain * pDomain, 
			      SNetDest  sDest, int iDelay, 
			      const uint8_t uDeflection)
{
  SRecordRoute pContext;
  pContext.pStream = pStream;
  pContext.sDest = sDest;
  pContext.iDelay = iDelay;
  pContext.uDeflection = uDeflection;

  return bgp_domain_routers_for_each(pDomain, bgp_domain_routers_record_route_for_each,
				     &pContext);
}

#ifdef __EXPERIMENTAL__
int bgp_domain_build_router_list_rtfe(uint32_t uKey, uint8_t uKeyLen,
			   void * pItem, void * pContext)
{
  SPtrArray * pRL= (SPtrArray *) pContext;
  SBGPRouter * pRouter= (SBGPRouter *) pItem;

  ptr_array_append(pRL, pRouter);

  return 0;
}

// ----- build_router_list ------------------------------------------
static SPtrArray * bgp_domain_routers_list(SBGPDomain * pDomain)
{
  SPtrArray * pRL= ptr_array_create_ref(0);

  // Build list of BGP routers
  radix_tree_for_each(pDomain->pRouters,
		      bgp_domain_build_router_list_rtfe, pRL);

  return pRL;
}

// ----- bgp_domain_full_mesh ---------------------------------------
/**
 * Generate a full-mesh of iBGP sessions in the domain.
 */
int bgp_domain_full_mesh(SBGPDomain * pDomain)
{
  int iIndex1, iIndex2;
  SPtrArray * pRouters;
  SBGPRouter * pRouter1, * pRouter2;

  /* Get the list of routers */
  pRouters= bgp_domain_routers_list(pDomain);

  /* Build the full-mesh of sessions */
  for (iIndex1= 0; iIndex1 < ptr_array_length(pRouters); iIndex1++) {
    pRouter1= pRouters->data[iIndex1];

    for (iIndex2= 0; iIndex2 < ptr_array_length(pRouters); iIndex2++) {

      if (iIndex1 == iIndex2)
	continue;

      pRouter2= pRouters->data[iIndex2];

      bgp_router_add_peer(pRouter1, pDomain->uNumber,
			  pRouter2->pNode->tAddr, 0);

    }

    bgp_router_start(pRouter1);

  }

  ptr_array_destroy(&pRouters);

  return 0;
}
#endif /* __EXPERIMENTAL__ */

/////////////////////////////////////////////////////////////////////
//
// INITIALIZATION AND FINALIZATION SECTION
//
/////////////////////////////////////////////////////////////////////

// ----- _domain_init -----------------------------------------------
/**
 * Initialize the array of BGP domains.
 */
void _domain_init()
{
  int iIndex;

  for (iIndex= 0; iIndex < MAX_DOMAINS; iIndex++) {
    apDomains[iIndex]= NULL;
  }
}

// ----- _domain_destroy --------------------------------------------
/**
 * Clean the array of domains: destroy the existing domains.
 */
void _domain_destroy()
{
  int iIndex;

  for (iIndex= 0; iIndex < MAX_DOMAINS; iIndex++) {
    bgp_domain_destroy(&apDomains[iIndex]);
  }
}
