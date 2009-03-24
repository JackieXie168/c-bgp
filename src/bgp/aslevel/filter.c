// ==================================================================
// @(#)filter.c
//
// Provide filtering capabilities to prune AS-level topologies from
// specific classes of nodes / edges. Example of such filters include
// - remove all stubs
// - remove all single-homed stubs
// - remove all peer-to-peer links
// - remove all but top-level domains
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 21/06/2007
// $Id: filter.c,v 1.1 2009-03-24 13:39:08 bqu Exp $
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <string.h>

#include <bgp/aslevel/as-level.h>
#include <bgp/aslevel/filter.h>
#include <bgp/aslevel/types.h>
#include <bgp/aslevel/util.h>

// -----[ aslevel_filter_str2filter ]--------------------------------
/**
 * Convert a filter name to its identifier.
 */
int aslevel_filter_str2filter(const char * pcFilter,
			      aslevel_filter_t * puFilter)
{
  if (!strcmp(pcFilter, "stubs")) {
    *puFilter= ASLEVEL_FILTER_STUBS;
    return ASLEVEL_SUCCESS;
  } else if (!strcmp(pcFilter, "single-homed-stubs")) {
    *puFilter= ASLEVEL_FILTER_SHSTUBS;
    return ASLEVEL_SUCCESS;
  } else if (!strcmp(pcFilter, "peer-to-peer")) {
    *puFilter= ASLEVEL_FILTER_P2P;
    return ASLEVEL_SUCCESS;
  } else if (!strcmp(pcFilter, "keep-top")) {
    *puFilter= ASLEVEL_FILTER_KEEP_TOP;
    return ASLEVEL_SUCCESS;
  }
  return ASLEVEL_ERROR_UNKNOWN_FILTER;
}

// -----[ aslevel_filter_topo ]--------------------------------------
/**
 * Filter the given topology with 
 */
int aslevel_filter_topo(as_level_topo_t * topo, aslevel_filter_t filter)
{
  unsigned int index, index2;
  as_level_domain_t * domain;
  as_level_link_t * link;
  unsigned int num_customers, num_providers;
  AS_SET_DEFINE(auSet);
  unsigned int num_nodes_removed= 0, num_edges_removed= 0;

  AS_SET_INIT(auSet);

  // Identify domains to be removed
  for (index= 0; index < aslevel_topo_num_nodes(topo); index++) {
    domain= (as_level_domain_t *) topo->domains->data[index];
    num_customers= aslevel_as_num_customers(domain);

    switch (filter) {

    case ASLEVEL_FILTER_STUBS:
      // Remove the domain if it has no customers and at least one
      // provider (to avoid removing top domains)
      if (num_customers == 0) {
	/*uNumProviders= aslevel_as_num_providers(pDomain);
	  if (uNumProviders > 0)*/
	  AS_SET_PUT(auSet, domain->asn);
      }
      break;

    case ASLEVEL_FILTER_SHSTUBS:
      // Remove the domain if it has no customers and exactly one single
      // provider
      if (num_customers == 0) {
	num_providers= aslevel_as_num_providers(domain);
	if (num_providers == 1)
	  AS_SET_PUT(auSet, domain->asn);
      }
      break;

    case ASLEVEL_FILTER_P2P:
      // Remove peer-to-peer links 
      num_providers= aslevel_as_num_providers(domain);
      if (num_providers == 0)
	break;
      index2= 0;
      while (index2 < ptr_array_length(domain->neighbors)) {
	link= (as_level_link_t *) domain->neighbors->data[index2];
	num_providers= aslevel_as_num_providers(link->neighbor);
	if (num_providers == 0)
	  break;
	if (link->peer_type == ASLEVEL_PEER_TYPE_PEER) {
	  ptr_array_remove_at(domain->neighbors, index2);
	  continue;
	}
	index2++;
      }
      if (ptr_array_length(domain->neighbors) == 0)
	AS_SET_PUT(auSet, domain->asn);
      break;

    case ASLEVEL_FILTER_KEEP_TOP:
      // Remove the domain if it has providers
      num_providers= aslevel_as_num_providers(domain);
      if (num_providers > 0)
	AS_SET_PUT(auSet, domain->asn);
      break;

    default:
      return ASLEVEL_ERROR_UNKNOWN_FILTER;

    }
  }

  // Remove links to/from identified domains
  for (index= 0; index < aslevel_topo_num_nodes(topo); index++) {
    domain= (as_level_domain_t *) topo->domains->data[index];
    index2= 0;
    while (index2 < ptr_array_length(domain->neighbors)) {
      link= (as_level_link_t *) domain->neighbors->data[index2];
      if (IS_IN_AS_SET(auSet, link->neighbor->asn)) {
	ptr_array_remove_at(domain->neighbors, index2);
	num_edges_removed++;
	continue;
      }
      index2++;
    }
  }

  // Remove identified domains
  index= 0;
  while (index < aslevel_topo_num_nodes(topo)) {
    domain= (as_level_domain_t *) topo->domains->data[index];
    if (IS_IN_AS_SET(auSet, domain->asn)) {
      ptr_array_remove_at(topo->domains, index);
      num_nodes_removed++;
      continue;
    }
    index++;
  }

  stream_printf(gdserr, "\tnumber of nodes removed: %d\n", num_nodes_removed);
  stream_printf(gdserr, "\tnumber of edges removed: %d\n", num_edges_removed);

  return ASLEVEL_SUCCESS;
}
