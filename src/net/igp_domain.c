// ==================================================================
// @(#)igp_domain.c
//
// @author Stefano Iasi (stefanoia@tin.it)
// @date 5/07/2005
// @lastdate 5/07/2005
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <libgds/memory.h>
#include <libgds/radix-tree.h>
#include <net/igp_domain.h>
#include <net/network.h>

#define IGP_MAX_DOMAINS 65536

SIGPDomain * apIGPDomains[IGP_MAX_DOMAINS];

// ----- igp_domain_create ------------------------------------------
/**
 * Create an IGP domain (set of router to comupute IGP route).
 * Note: router should have the same protocol.
 */
SIGPDomain * igp_domain_create(uint16_t uNumber)
{
  SIGPDomain * pDomain= (SIGPDomain *) MALLOC(sizeof(SIGPDomain));
  pDomain->uNumber= uNumber;
  pDomain->pcName= NULL;
  /* Radix-tree with all routers. Destroy function is NULL. */
  pDomain->pRouters= radix_tree_create(32, NULL);
  return pDomain;
}

// ----- igp_domain_destroy -----------------------------------------
/**
 * Destroy a IGP domain.
 */
void igp_domain_destroy(SIGPDomain ** ppDomain)
{
  if (*ppDomain != NULL) {
    radix_tree_destroy(&((*ppDomain)->pRouters));
    FREE(*ppDomain);
    *ppDomain= NULL;
  }
}

// ----- igp_domain_add_router --------------------------------------
/**
 * Add a reference to a router in the domain.
 *
 * Preconditions:
 * - the router must be non NULL;
 * - the router must not exist in the domain. Otherwise, it will be
 *   replaced by the new one and memory leaks as well as unexpected
 *   results may occur.
 */
void igp_domain_add_router(SIGPDomain * pDomain, SNetNode * pNode)
{
  radix_tree_add(pDomain->pRouters, pNode->tAddr, 32, pNode);
  node_igp_domain_add(pNode, pDomain->uNumber);
}

// ----- igp_domain_routers_for_each --------------------------------
/**
 * Call the given callback function for all routers registered in the
 * domain.
 */
int igp_domain_routers_for_each(SIGPDomain * pDomain,
				FRadixTreeForEach fForEach,
				void * pContext)
{
  radix_tree_for_each(pDomain->pRouters, fForEach, pContext);
  return 0;
}

// ----- exists_igp_domain ------------------------------------------
/**
 * Return true (1) if the domain identified by the given IGP domain
 * number exists. Otherwise, return false (0).
 */
int exists_igp_domain(uint16_t uNumber)
{
  return (apIGPDomains[uNumber] != NULL);
}

// ----- get_igp_domain ---------------------------------------------
/**
 * Get the reference of a domain identified by its IGP number. If the
 * domain does not exist, it is created and registered.
 */
SIGPDomain * get_igp_domain(uint16_t uNumber)
{
  if (apIGPDomains[uNumber] == NULL)
    apIGPDomains[uNumber]= igp_domain_create(uNumber);
  return apIGPDomains[uNumber];
}

// ----- register_igp_domain ----------------------------------------
/**
 * Register a new domain.
 *
 * Preconditions:
 * - the domain must be non NULL;
 * - the domain must not exist.
 */
void register_igp_domain(SIGPDomain * pDomain)
{
  assert(apIGPDomains[pDomain->uNumber] == NULL);
  apIGPDomains[pDomain->uNumber]= pDomain;
}

/////////////////////////////////////////////////////////////////////
//
// INITIALIZATION AND FINALIZATION SECTION
//
/////////////////////////////////////////////////////////////////////

// ----- _domain_init -----------------------------------------------
/**
 * Initialize the array of IGP domains.
 */
void _IGPdomain_init()
{
  int iIndex;

  for (iIndex= 0; iIndex < IGP_MAX_DOMAINS; iIndex++) {
    apIGPDomains[iIndex]= NULL;
  }
}

// ----- _domain_destroy --------------------------------------------
/**
 * Clean the array of domains: destroy the existing domains.
 */
void _IGPdomain_destroy()
{
  int iIndex;

  for (iIndex= 0; iIndex < IGP_MAX_DOMAINS; iIndex++) {
    igp_domain_destroy(&apIGPDomains[iIndex]);
  }
}

int test_show_rt(uint32_t uKey, uint8_t uKeyLen,
					   void * pItem, void * pContext)
{
/*	fprintf(stdout, "test_show_rt %p\n", pItem);
	fflush(stdout);
	ip_address_dump(stdout, ((SNetNode *) pItem)->tAddr);
	fflush(stdout);*/
	return 0;
}

// ----- igp_domain_contains_router --------------------------------------------
/**
 * Return TRUE (1) if node is in radix tree. FALSE (0) otherwise.
 */
int igp_domain_contains_router(SIGPDomain * pDomain, SNetNode * pNode){
  if (radix_tree_get_exact(pDomain->pRouters, pNode->tAddr, 32) == NULL)
    return 0;
  return 1;
}

///////////////////////////////////////////////////////////////////////
/// TEST
///////////////////////////////////////////////////////////////////////
int IGPdomain_test(){
  LOG_DEBUG("test_igp_domain(): START\n");

  LOG_DEBUG("test_igp_domain(): create domains... "); 
  _IGPdomain_init();  
  
  SIGPDomain * pDomain1 = igp_domain_create(1);
  SIGPDomain * pDomain2 = igp_domain_create(2);
  SIGPDomain * pDomain3 = igp_domain_create(3);
  LOG_DEBUG("ok!\n"); 
  
  LOG_DEBUG("test_igp_domain(): register domains... "); 
  register_igp_domain(pDomain1);
  assert(exists_igp_domain(1) != 0);
  assert(exists_igp_domain(2) == 0);
  register_igp_domain(pDomain2);
  assert(exists_igp_domain(2) != 0);
  register_igp_domain(pDomain3);
  LOG_DEBUG("ok!\n"); 
  
  SNetNode * pNode1 = node_create(1);
  SNetNode * pNode2 = node_create(2);
  
  igp_domain_add_router(pDomain1, pNode1);
  igp_domain_add_router(pDomain1, pNode2);
  igp_domain_add_router(pDomain2, pNode2);
  
  assert(node_belongs_to_igp_domain(pNode1, pDomain1->uNumber) != 0);
  assert(node_belongs_to_igp_domain(pNode1, pDomain2->uNumber) == 0);
  assert(node_belongs_to_igp_domain(pNode2, pDomain1->uNumber) != 0);
  assert(node_belongs_to_igp_domain(pNode2, pDomain2->uNumber) != 0);
  
  assert(igp_domain_contains_router(pDomain1, pNode1) != 0);
  assert(igp_domain_contains_router(pDomain2, pNode1) == 0);
  
  LOG_DEBUG("test_igp_domain(): destroy domain... ");
  _IGPdomain_destroy();  
  LOG_DEBUG("done!");
  LOG_DEBUG("test_igp_domain(): DONE!\n");
  return 1;
}
