// ==================================================================
// @(#)domain.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 13/02/2002
// @lastdate 11/03/2008
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
#include <net/record-route.h>

#define BGP_DOMAINS_MAX 65536

SBGPDomain * apDomains[BGP_DOMAINS_MAX];

// ----- bgp_domain_create ------------------------------------------
/**
 * Create a BGP domain (an Autonomous System).
 */
SBGPDomain * bgp_domain_create(uint16_t uASN)
{
  SBGPDomain * pDomain= (SBGPDomain *) MALLOC(sizeof(SBGPDomain));
  pDomain->uASN= uASN;
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

// -----[ bgp_domains_for_each ]-------------------------------------
/**
 *
 */
int bgp_domains_for_each(FBGPDomainsForEach fForEach, void * pContext)
{
  uint32_t uIndex;
  int iResult;

  for (uIndex= 0; uIndex < BGP_DOMAINS_MAX; uIndex++) {
    if (apDomains[uIndex] != NULL) {
      iResult= fForEach(apDomains[uIndex], pContext);
      if (iResult != 0)
	return iResult;
    }
  }
  return 0;
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
void bgp_domain_add_router(SBGPDomain * pDomain, bgp_router_t * pRouter)
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
int exists_bgp_domain(uint16_t uASN)
{
  return (apDomains[uASN] != NULL);
}

// ----- get_bgp_domain ---------------------------------------------
/**
 * Get the reference of a domain identified by its AS number. If the
 * domain does not exist, it is created and registered.
 */
SBGPDomain * get_bgp_domain(uint16_t uASN)
{
  if (apDomains[uASN] == NULL)
    apDomains[uASN]= bgp_domain_create(uASN);
  return apDomains[uASN];
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
  assert(apDomains[pDomain->uASN] == NULL);
  apDomains[pDomain->uASN]= pDomain;
}

// ----- bgp_domain_routers_rescan_fct ------------------------------
int bgp_domain_routers_rescan_fct_for_each(uint32_t uKey, uint8_t uKeyLen,
					   void * pItem, void * pContext)
{
  return bgp_router_scan_rib((bgp_router_t *) pItem);
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
  SLogStream * pStream;
  SNetDest sDest;
  uint8_t uOptions;
} SRecordRoute;

// ----- _bgp_domain_routers_record_route_for_each --------------------
static int bgp_domain_routers_record_route_for_each(uint32_t uKey,
						    uint8_t uKeyLen,
						    void * pItem,
						    void * pContext)
{
  net_node_t * pNode = (net_node_t *)((bgp_router_t *)pItem)->pNode;
  SRecordRoute * pCont = (SRecordRoute *)pContext;
  
  node_dump_recorded_route(pCont->pStream, pNode,
			   pCont->sDest, 0, pCont->uOptions, 0);
  return 0;
}

// ----- bgp_domain_record_route -------------------------------------
/**
 *
 */
int bgp_domain_dump_recorded_route(SLogStream * pStream, SBGPDomain * pDomain, 
				   SNetDest  sDest, const uint8_t uOptions)
{
  SRecordRoute pContext;
  pContext.pStream= pStream;
  pContext.sDest= sDest;
  pContext.uOptions= uOptions;

  return bgp_domain_routers_for_each(pDomain,
				     bgp_domain_routers_record_route_for_each,
				     &pContext);
}

int bgp_domain_build_router_list_rtfe(uint32_t uKey, uint8_t uKeyLen,
				      void * pItem, void * pContext)
{
  SPtrArray * pRL= (SPtrArray *) pContext;
  bgp_router_t * pRouter= (bgp_router_t *) pItem;
  
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
  bgp_router_t * pRouter1, * pRouter2;

  /* Get the list of routers */
  pRouters= bgp_domain_routers_list(pDomain);

  /* Build the full-mesh of sessions */
  for (iIndex1= 0; iIndex1 < ptr_array_length(pRouters); iIndex1++) {
    pRouter1= pRouters->data[iIndex1];

    for (iIndex2= 0; iIndex2 < ptr_array_length(pRouters); iIndex2++) {

      if (iIndex1 == iIndex2)
	continue;

      pRouter2= pRouters->data[iIndex2];

      bgp_router_add_peer(pRouter1, pDomain->uASN,
			  pRouter2->pNode->tAddr, NULL);

    }

    bgp_router_start(pRouter1);

  }

  ptr_array_destroy(&pRouters);

  return 0;
}

/////////////////////////////////////////////////////////////////////
//
// INITIALIZATION AND FINALIZATION SECTION
//
/////////////////////////////////////////////////////////////////////

// ----- _bgp__domain_init ------------------------------------------
/**
 * Initialize the array of BGP domains.
 */
void _bgp_domain_init()
{
  int iIndex;

  for (iIndex= 0; iIndex < BGP_DOMAINS_MAX; iIndex++) {
    apDomains[iIndex]= NULL;
  }
}

// ----- _bgp_domain_destroy ----------------------------------------
/**
 * Clean the array of domains: destroy the existing domains.
 */
void _bgp_domain_destroy()
{
  int iIndex;

  for (iIndex= 0; iIndex < BGP_DOMAINS_MAX; iIndex++) {
    bgp_domain_destroy(&apDomains[iIndex]);
  }
}
