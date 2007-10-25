// ==================================================================
// @(#)igp_domain.c
//
// @author Stefano Iasi (stefanoia@tin.it)
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 5/07/2005
// @lastdate 15/10/2007
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <libgds/log.h>
#include <libgds/memory.h>
#include <libgds/radix-tree.h>
#include <net/igp.h>
#include <net/igp_domain.h>
#include <net/network.h>
#include <net/ospf.h>

#define IGP_MAX_DOMAINS 65536

SIGPDomain * apIGPDomains[IGP_MAX_DOMAINS];

// ----- igp_domain_create ------------------------------------------
/**
 * Create an IGP domain (set of routers to compute IGP routes).
 * Note: routers in the domain should support the same IGP model.
 */
SIGPDomain * igp_domain_create(uint16_t uNumber, EDomainType tType)
{
  SIGPDomain * pDomain= (SIGPDomain *) MALLOC(sizeof(SIGPDomain));
  pDomain->uNumber= uNumber;
  pDomain->pcName= NULL;
  pDomain->tType= tType;
  pDomain->uECMP= 0;

  /* Radix-tree with all routers. Destroy function is NULL. */
  pDomain->pRouters= trie_create(NULL);

  return pDomain;
}

// ----- igp_domain_destroy -----------------------------------------
/**
 * Destroy a IGP domain.
 */
void igp_domain_destroy(SIGPDomain ** ppDomain)
{
  if (*ppDomain != NULL) {
    trie_destroy(&((*ppDomain)->pRouters));
    FREE(*ppDomain);
    *ppDomain= NULL;
  }
}

// ----- igp_domain_set_ecmp ----------------------------------------
/**
 * Set the support for Equal Cost Multi-Paths (ECMP).
 */
int igp_domain_set_ecmp(SIGPDomain * pDomain, int iState)
{
  switch (pDomain->tType) {
  case DOMAIN_IGP:
    return -1;
  case DOMAIN_OSPF:
    pDomain->uECMP= (iState?1:0);
    break;
  }
  return 0;
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
int igp_domain_add_router(SIGPDomain * pDomain, SNetNode * pNode)
{
  trie_insert(pDomain->pRouters, pNode->tAddr, 32, pNode);
  return node_igp_domain_add(pNode, pDomain->uNumber);
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
  trie_for_each(pDomain->pRouters, fForEach, pContext);
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
  /* Remove implicit domain creation (since we need to know the
     domain's type)
  if (apIGPDomains[uNumber] == NULL)
    apIGPDomains[uNumber]= igp_domain_create(uNumber);
  */
  return apIGPDomains[uNumber];
}

// -----[ igp_domains_for_each ]-------------------------------------
/**
 *
 */
int igp_domains_for_each(FIGPDomainsForEach fForEach, void * pContext)
{
  uint32_t uIndex;
  int iResult;

  for (uIndex= 0; uIndex < IGP_MAX_DOMAINS; uIndex++) {
    if (apIGPDomains[uIndex] != NULL) {
      iResult= fForEach(apIGPDomains[uIndex], pContext);
      if (iResult != 0)
	return iResult;
    }
  }
  return 0;
}

// ----- register_igp_domain ----------------------------------------
/**
 * Register a new domain.
 *
 * Preconditions:
 * - the domain must be non NULL;
 * - the domain must not exist.
 */
int register_igp_domain(SIGPDomain * pDomain)
{
  if (apIGPDomains[pDomain->uNumber] != NULL) {
    return -1;
  }
  apIGPDomains[pDomain->uNumber]= pDomain;
  return 0;
}

// ----- _igp_domain_dump_for_each ----------------------------------
/**
 *  Helps igp_domain_dump to dump the ip address of each router 
 *  in the igp domain.
*/
static int _igp_domain_dump_for_each(uint32_t uKey, uint8_t uKeyLen,
				     void * pItem, void * pContext)
{
  SLogStream * pStream= (SLogStream *) (pContext);
  
  ip_address_dump(pStream, ((SNetNode *) pItem)->tAddr);
  log_printf(pStream, "\n");
  return 0;
}

// ----- igp_domain_dump --------------------------------------------
int igp_domain_dump(SLogStream * pStream, SIGPDomain * pDomain)
{
  return trie_for_each(pDomain->pRouters,
		       _igp_domain_dump_for_each,
		       pStream);
}

// ----- igp_domain_info --------------------------------------------
void igp_domain_info(SLogStream * pStream, SIGPDomain * pDomain)
{
  // IGP model
  log_printf(pStream, "model: ");
  switch (pDomain->tType) {
  case DOMAIN_IGP: log_printf(pStream, "igp"); break;
  case DOMAIN_OSPF: log_printf(pStream, "ospf"); break;
  default:
    log_printf(pStream, "???");
  }
  log_printf(pStream, "\n");

  // Support for ECMP
  if (pDomain->uECMP)
    log_printf(pStream, "ecmp : yes\n");
  else
    log_printf(pStream, "ecmp : no\n");
}

// ----- igp_domain_compute -----------------------------------------
/**
 * Compute the routing tables of all the routers within the given
 * domain.
 *
 * Returns: 0 on success, -1 on error.
 */
int igp_domain_compute(SIGPDomain * pDomain)
{
  switch (pDomain->tType) {
  case DOMAIN_IGP:
    return igp_compute_domain(pDomain);
    break;
  case DOMAIN_OSPF:
#ifdef OSPF_SUPPORT
    if (ospf_domain_build_route(pDomain->uNumber) >= 0)
      return 0;
#endif
    return -1;
    
    break;
  default:
    return -1;
  }
  return 0;
}

// ----- igp_domain_contains_router ---------------------------------
/**
 * Return TRUE (1) if node is in radix tree. FALSE (0) otherwise.
 *
 */
int igp_domain_contains_router(SIGPDomain * pDomain, SNetNode * pNode)
{
  if (trie_find_exact(pDomain->pRouters, pNode->tAddr, 32) == NULL)
    return 0;
  return 1;
}

// ----- igp_domain_contains_router_by_addr -------------------------
/**
 * Return TRUE (1) if node is in radix tree. FALSE (0) otherwise.
 *
 */
int igp_domain_contains_router_by_addr(SIGPDomain * pDomain, net_addr_t tAddr)
{
  if (trie_find_exact(pDomain->pRouters, tAddr, 32) == NULL)
    return 0;
  return 1;
}

/////////////////////////////////////////////////////////////////////
//
// INITIALIZATION AND FINALIZATION SECTION
//
/////////////////////////////////////////////////////////////////////

// ----- _igp_domain_init -------------------------------------------
/**
 * Initialize the array of IGP domains.
 */
void _igp_domain_init()
{
  int iIndex;

  for (iIndex= 0; iIndex < IGP_MAX_DOMAINS; iIndex++) {
    apIGPDomains[iIndex]= NULL;
  }
}

// ----- _igp__domain_destroy ---------------------------------------
/**
 * Clean the array of domains: destroy the existing domains.
 */
void _igp_domain_destroy()
{
  int iIndex;

  for (iIndex= 0; iIndex < IGP_MAX_DOMAINS; iIndex++) {
    igp_domain_destroy(&apIGPDomains[iIndex]);
  }
}

/////////////////////////////////////////////////////////////////////
//
// TEST FUNCTIONS
//
/////////////////////////////////////////////////////////////////////

int _igp_domain_test(){
#ifdef OSPF_SUPPORT
  LOG_DEBUG("test_igp_domain(): START\n");

  LOG_DEBUG("test_igp_domain(): create domains... "); 
  _igp_domain_init();
  
  SIGPDomain * pDomain1 = ospf_domain_create(1);
  SIGPDomain * pDomain2 = ospf_domain_create(2);
  SIGPDomain * pDomain3 = ospf_domain_create(3);
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
  _igp_domain_destroy();
  node_destroy(&pNode1);
  node_destroy(&pNode2);
  LOG_DEBUG("done!");
  LOG_DEBUG("test_igp_domain(): DONE!\n");
#endif /* OSPF_SUPPORT */
  return 1;
}
