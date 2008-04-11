// ==================================================================
// @(#)domain.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 13/02/2002
// $Id: domain.c,v 1.10 2008-04-11 11:03:06 bqu Exp $
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
static bgp_domain_t * _domains[BGP_DOMAINS_MAX];

// ----- bgp_domain_create ------------------------------------------
/**
 * Create a BGP domain (an Autonomous System).
 */
bgp_domain_t * bgp_domain_create(asn_t asn)
{
  bgp_domain_t * domain= (bgp_domain_t *) MALLOC(sizeof(bgp_domain_t));
  domain->asn= asn;
  domain->name= NULL;

  /* Radix-tree with all routers. Destroy function is NULL. */
  domain->routers= radix_tree_create(32, NULL);

  return domain;
}

// ----- bgp_domain_destroy -----------------------------------------
/**
 * Destroy a BGP domain.
 */
void bgp_domain_destroy(bgp_domain_t ** domain_ref)
{
  if (*domain_ref != NULL) {
    radix_tree_destroy(&((*domain_ref)->routers));
    FREE(*domain_ref);
    *domain_ref= NULL;
  }
}

// -----[ bgp_domains_for_each ]-------------------------------------
/**
 *
 */
int bgp_domains_for_each(FBGPDomainsForEach for_each, void * ctx)
{
  unsigned int index;
  int result;

  for (index= 0; index < BGP_DOMAINS_MAX; index++) {
    if (_domains[index] != NULL) {
      result= for_each(_domains[index], ctx);
      if (result != 0)
	return result;
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
void bgp_domain_add_router(bgp_domain_t * domain, bgp_router_t * router)
{
  radix_tree_add(domain->routers, router->pNode->addr, 32, router);
  router->domain= domain;
}

// ----- bgp_domain_routers_for_each --------------------------------
/**
 * Call the given callback function for all routers registered in the
 * domain.
 */
int bgp_domain_routers_for_each(bgp_domain_t * domain,
				FRadixTreeForEach for_each,
				void * ctx)
{
  return radix_tree_for_each(domain->routers, for_each, ctx);
}

// ----- exists_bgp_domain ------------------------------------------
/**
 * Return true (1) if the domain identified by the given AS number
 * exists. Otherwise, return false (0).
 */
int exists_bgp_domain(asn_t asn)
{
  return (_domains[asn] != NULL);
}

// ----- get_bgp_domain ---------------------------------------------
/**
 * Get the reference of a domain identified by its AS number. If the
 * domain does not exist, it is created and registered.
 */
bgp_domain_t * get_bgp_domain(asn_t asn)
{
  if (_domains[asn] == NULL)
    _domains[asn]= bgp_domain_create(asn);
  return _domains[asn];
}

// ----- register_bgp_domain ----------------------------------------
/**
 * Register a new domain.
 *
 * Preconditions:
 * - the router must be non NULL;
 * - the router must not exist.
 */
void register_bgp_domain(bgp_domain_t * domain)
{
  assert(_domains[domain->asn] == NULL);
  _domains[domain->asn]= domain;
}

// -----[ _rescan_for_each ]-----------------------------------------
static int _rescan_for_each(uint32_t key, uint8_t key_len,
			    void * item, void * ctx)
{
  return bgp_router_scan_rib((bgp_router_t *) item);
}

// ----- bgp_domain_rescan ------------------------------------------
/**
 * Run the RIB rescan process in each router of the BGP domain.
 */
int bgp_domain_rescan(bgp_domain_t * domain)
{
  return bgp_domain_routers_for_each(domain,
				     _rescan_for_each,
				     NULL);
}

typedef struct {
  SLogStream * stream;
  SNetDest     dest;
  uint8_t      options;
} _record_route_ctx_t;

// -----[ _record_route_for_each ]-----------------------------------
static int _record_route_for_each(uint32_t key, uint8_t key_len,
				  void * item, void * ctx)
{
  net_node_t * node = ((bgp_router_t *)item)->pNode;
  _record_route_ctx_t * rr_ctx = (_record_route_ctx_t *) ctx;
  
  node_dump_recorded_route(rr_ctx->stream, node,
			   rr_ctx->dest, 0, rr_ctx->options, 0);
  return 0;
}

// ----- bgp_domain_record_route -------------------------------------
/**
 *
 */
int bgp_domain_dump_recorded_route(SLogStream * stream,
				   bgp_domain_t * domain, 
				   SNetDest dest,
				   uint8_t options)
{
  _record_route_ctx_t ctx=  {
    .stream = stream,
    .dest   = dest,
    .options= options
  };

  return bgp_domain_routers_for_each(domain,
				     _record_route_for_each,
				     &ctx);
}

// -----[ _build_router_list ]---------------------------------------
static int _build_router_list(uint32_t key, uint8_t key_len,
			      void * item, void * ctx)
{
  SPtrArray * list= (SPtrArray *) ctx;
  bgp_router_t * router= (bgp_router_t *) item;
  
  ptr_array_append(list, router);
  
  return 0;
}

// -----[ _bgp_domain_routers_list ]---------------------------------
static inline SPtrArray * _bgp_domain_routers_list(bgp_domain_t * domain)
{
  SPtrArray * list= ptr_array_create_ref(0);

  // Build list of BGP routers
  radix_tree_for_each(domain->routers, _build_router_list, list);

  return list;
}

// ----- bgp_domain_full_mesh ---------------------------------------
/**
 * Generate a full-mesh of iBGP sessions in the domain.
 */
int bgp_domain_full_mesh(bgp_domain_t * domain)
{
  unsigned int index1, index2;
  SPtrArray * routers;
  bgp_router_t * router1, * router2;

  /* Get the list of routers */
  routers= _bgp_domain_routers_list(domain);

  /* Build the full-mesh of sessions */
  for (index1= 0; index1 < ptr_array_length(routers); index1++) {
    router1= routers->data[index1];
    for (index2= 0; index2 < ptr_array_length(routers); index2++) {
      if (index1 == index2)
	continue;
      router2= routers->data[index2];
      bgp_router_add_peer(router1, domain->asn,
			  router2->pNode->addr, NULL);
    }
    bgp_router_start(router1);
  }
  ptr_array_destroy(&routers);
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
  unsigned int index;

  for (index= 0; index < BGP_DOMAINS_MAX; index++) {
    _domains[index]= NULL;
  }
}

// ----- _bgp_domain_destroy ----------------------------------------
/**
 * Clean the array of domains: destroy the existing domains.
 */
void _bgp_domain_destroy()
{
  unsigned int index;

  for (index= 0; index < BGP_DOMAINS_MAX; index++) {
    bgp_domain_destroy(&_domains[index]);
  }
}
