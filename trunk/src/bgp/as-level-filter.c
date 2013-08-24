// ==================================================================
// @(#)as-level-filter.c
//
// Provide filtering capabilities to prune AS-level topologies from
// specific classes of nodes / edges. Example of such filters include
// - remove all stubs
// - remove all single-homed stubs
// - remove all peer-to-peer links
// - remove all but top-level domains
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 21/06/2007
// @lastdate 21/06/2007
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <string.h>

#include <bgp/as-level.h>
#include <bgp/as-level-filter.h>
#include <bgp/as-level-types.h>
#include <bgp/as-level-util.h>

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
int aslevel_filter_topo(SASLevelTopo * pTopo, aslevel_filter_t uFilter)
{
  unsigned int uIndex, uIndex2;
  SASLevelDomain * pDomain;
  SASLevelLink * pLink;
  unsigned int uNumCustomers, uNumProviders;
  AS_SET_DEFINE(auSet);
  unsigned int uNumNodesRemoved= 0, uNumEdgesRemoved= 0;

  AS_SET_INIT(auSet);

  // Identify domains to be removed
  for (uIndex= 0; uIndex < aslevel_topo_num_nodes(pTopo); uIndex++) {
    pDomain= (SASLevelDomain *) pTopo->pDomains->data[uIndex];
    uNumCustomers= aslevel_as_num_customers(pDomain);

    switch (uFilter) {

    case ASLEVEL_FILTER_STUBS:
      // Remove the domain if it has no customers and at least one
      // provider (to avoid removing top domains)
      if (uNumCustomers == 0) {
	/*uNumProviders= aslevel_as_num_providers(pDomain);
	  if (uNumProviders > 0)*/
	  AS_SET_PUT(auSet, pDomain->uASN);
      }
      break;

    case ASLEVEL_FILTER_SHSTUBS:
      // Remove the domain if it has no customers and exactly one single
      // provider
      if (uNumCustomers == 0) {
	uNumProviders= aslevel_as_num_providers(pDomain);
	if (uNumProviders == 1)
	  AS_SET_PUT(auSet, pDomain->uASN);
      }
      break;

    case ASLEVEL_FILTER_P2P:
      // Remove peer-to-peer links 
      uNumProviders= aslevel_as_num_providers(pDomain);
      if (uNumProviders == 0)
	break;
      uIndex2= 0;
      while (uIndex2 < ptr_array_length(pDomain->pNeighbors)) {
	pLink= (SASLevelLink *) pDomain->pNeighbors->data[uIndex2];
	uNumProviders= aslevel_as_num_providers(pLink->pNeighbor);
	if (uNumProviders == 0)
	  break;
	if (pLink->tPeerType == ASLEVEL_PEER_TYPE_PEER) {
	  ptr_array_remove_at(pDomain->pNeighbors, uIndex2);
	  continue;
	}
	uIndex2++;
      }
      if (ptr_array_length(pDomain->pNeighbors) == 0)
	AS_SET_PUT(auSet, pDomain->uASN);
      break;

    case ASLEVEL_FILTER_KEEP_TOP:
      // Remove the domain if it has providers
      uNumProviders= aslevel_as_num_providers(pDomain);
      if (uNumProviders > 0)
	AS_SET_PUT(auSet, pDomain->uASN);
      break;

    default:
      return ASLEVEL_ERROR_UNKNOWN_FILTER;

    }
  }

  // Remove links to/from identified domains
  for (uIndex= 0; uIndex < aslevel_topo_num_nodes(pTopo); uIndex++) {
    pDomain= (SASLevelDomain *) pTopo->pDomains->data[uIndex];
    uIndex2= 0;
    while (uIndex2 < ptr_array_length(pDomain->pNeighbors)) {
      pLink= (SASLevelLink *) pDomain->pNeighbors->data[uIndex2];
      if (IS_IN_AS_SET(auSet, pLink->pNeighbor->uASN)) {
	ptr_array_remove_at(pDomain->pNeighbors, uIndex2);
	uNumEdgesRemoved++;
	continue;
      }
      uIndex2++;
    }
  }

  // Remove identified domains
  uIndex= 0;
  while (uIndex < aslevel_topo_num_nodes(pTopo)) {
    pDomain= (SASLevelDomain *) pTopo->pDomains->data[uIndex];
    if (IS_IN_AS_SET(auSet, pDomain->uASN)) {
      ptr_array_remove_at(pTopo->pDomains, uIndex);
      uNumNodesRemoved++;
      continue;
    }
    uIndex++;
  }

  fprintf(stderr, "\tnumber of nodes removed: %d\n", uNumNodesRemoved);
  fprintf(stderr, "\tnumber of edges removed: %d\n", uNumEdgesRemoved);

  return ASLEVEL_SUCCESS;
}
