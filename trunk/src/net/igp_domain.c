// ==================================================================
// @(#)igp_domain.c
//
// @author Stefano Iasi (stefanoia@tin.it)
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 5/07/2005
// $Id: igp_domain.c,v 1.9 2008-04-11 11:03:06 bqu Exp $
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
static igp_domain_t * _domains[IGP_MAX_DOMAINS];

// ----- igp_domain_create ------------------------------------------
/**
 * Create an IGP domain (set of routers to compute IGP routes).
 * Note: routers in the domain should support the same IGP model.
 */
igp_domain_t * igp_domain_create(uint16_t id, igp_domain_type_t type)
{
  igp_domain_t * domain= (igp_domain_t *) MALLOC(sizeof(igp_domain_t));
  domain->id= id;
  domain->name= NULL;
  domain->type= type;
  domain->ecmp= 0;

  /* Radix-tree with all routers. Destroy function is NULL. */
  domain->routers= trie_create(NULL);

  return domain;
}

// ----- igp_domain_destroy -----------------------------------------
/**
 * Destroy a IGP domain.
 */
void igp_domain_destroy(igp_domain_t ** domain_ref)
{
  if (*domain_ref != NULL) {
    trie_destroy(&((*domain_ref)->routers));
    FREE(*domain_ref);
    *domain_ref= NULL;
  }
}

// ----- igp_domain_set_ecmp ----------------------------------------
/**
 * Set the support for Equal Cost Multi-Paths (ECMP).
 */
int igp_domain_set_ecmp(igp_domain_t * domain, int state)
{
  switch (domain->type) {
  case IGP_DOMAIN_IGP:
    return -1;
  case IGP_DOMAIN_OSPF:
    domain->ecmp= (state?1:0);
    break;
  default:
    abort();
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
int igp_domain_add_router(igp_domain_t * domain, net_node_t * node)
{
  trie_insert(domain->routers, node->addr, 32, node);
  return node_igp_domain_add(node, domain->id);
}

// ----- igp_domain_routers_for_each --------------------------------
/**
 * Call the given callback function for all routers registered in the
 * domain.
 */
int igp_domain_routers_for_each(igp_domain_t * domain,
				FRadixTreeForEach for_each,
				void * ctx)
{
  trie_for_each(domain->routers, for_each, ctx);
  return 0;
}

// ----- exists_igp_domain ------------------------------------------
/**
 * Return true (1) if the domain identified by the given IGP domain
 * number exists. Otherwise, return false (0).
 */
int exists_igp_domain(uint16_t id)
{
  return (_domains[id] != NULL);
}

// ----- get_igp_domain ---------------------------------------------
/**
 * Get the reference of a domain identified by its IGP number. If the
 * domain does not exist, it is created and registered.
 */
igp_domain_t * get_igp_domain(uint16_t id)
{
  return _domains[id];
}

// -----[ igp_domains_for_each ]-------------------------------------
/**
 *
 */
int igp_domains_for_each(FIGPDomainsForEach for_each, void * ctx)
{
  unsigned int index;
  int result;

  for (index= 0; index < IGP_MAX_DOMAINS; index++) {
    if (_domains[index] != NULL) {
      result= for_each(_domains[index], ctx);
      if (result != 0)
	return result;
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
int register_igp_domain(igp_domain_t * domain)
{
  if (_domains[domain->id] != NULL) {
    return -1;
  }
  _domains[domain->id]= domain;
  return 0;
}

// ----- _igp_domain_dump_for_each ----------------------------------
/**
 *  Helps igp_domain_dump to dump the ip address of each router 
 *  in the igp domain.
*/
static int _igp_domain_dump_for_each(uint32_t key, uint8_t key_len,
				     void * item, void * ctx)
{
  SLogStream * stream= (SLogStream *) ctx;
  net_node_t * node= (net_node_t *) item;
  
  ip_address_dump(stream, node->addr);
  log_printf(stream, "\n");
  return 0;
}

// ----- igp_domain_dump --------------------------------------------
int igp_domain_dump(SLogStream * stream, igp_domain_t * domain)
{
  return trie_for_each(domain->routers,
		       _igp_domain_dump_for_each,
		       stream);
}

// ----- igp_domain_info --------------------------------------------
void igp_domain_info(SLogStream * stream, igp_domain_t * domain)
{
  // IGP model
  log_printf(stream, "model: ");
  switch (domain->type) {
  case IGP_DOMAIN_IGP: log_printf(stream, "igp"); break;
  case IGP_DOMAIN_OSPF: log_printf(stream, "ospf"); break;
  default:
    abort();
  }
  log_printf(stream, "\n");

  // Support for ECMP
  if (domain->ecmp)
    log_printf(stream, "ecmp : yes\n");
  else
    log_printf(stream, "ecmp : no\n");
}

// ----- igp_domain_compute -----------------------------------------
/**
 * Compute the routing tables of all the routers within the given
 * domain.
 *
 * Returns: 0 on success, -1 on error.
 */
int igp_domain_compute(igp_domain_t * domain)
{
  switch (domain->type) {
  case IGP_DOMAIN_IGP:
    return igp_compute_domain(domain);
    break;
  case IGP_DOMAIN_OSPF:
#ifdef OSPF_SUPPORT
    if (ospf_domain_build_route(domain->id) >= 0)
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
int igp_domain_contains_router(igp_domain_t * domain, net_node_t * node)
{
  if (trie_find_exact(domain->routers, node->addr, 32) == NULL)
    return 0;
  return 1;
}

// ----- igp_domain_contains_router_by_addr -------------------------
/**
 * Return TRUE (1) if node is in radix tree. FALSE (0) otherwise.
 *
 */
int igp_domain_contains_router_by_addr(igp_domain_t * domain, net_addr_t addr)
{
  if (trie_find_exact(domain->routers, addr, 32) == NULL)
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
  unsigned int index;

  for (index= 0; index < IGP_MAX_DOMAINS; index++) {
    _domains[index]= NULL;
  }
}

// ----- _igp__domain_destroy ---------------------------------------
/**
 * Clean the array of domains: destroy the existing domains.
 */
void _igp_domain_destroy()
{
  unsigned int index;

  for (index= 0; index < IGP_MAX_DOMAINS; index++) {
    igp_domain_destroy(&_domains[index]);
  }
}
