// ==================================================================
// @(#)export_cli.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 15/10/07
// @lastdate 22/02/08
// ==================================================================
// Note:
//   - tunnels and tunnel-based routes are not suppported
//     (there is a potential interdependency between
//     routing/forwarding in that case)
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* HAVE_CONFIG_H */

#include <time.h>

#include <libgds/enumerator.h>

#include <net/error.h>
#include <net/export_cli.h>
#include <net/link-list.h>
#include <net/igp_domain.h>
#include <net/network.h>
#include <net/node.h>
#include <net/subnet.h>

#define COMM_SEP1 \
  "# ===================================================================\n"
#define COMM_SEP2 \
  "# -------------------------------------------------------------------\n"

// -----[ _net_export_cli_header ]-----------------------------------
static void _net_export_cli_header(SLogStream * pStream)
{
  time_t tCurrentTime= time(NULL);

  log_printf(pStream, COMM_SEP1);
  log_printf(pStream, "# C-BGP Export file (CLI)\n");
  log_printf(pStream, "# generated on %s", ctime(&tCurrentTime));
  log_printf(pStream, COMM_SEP1);
}

// -----[ _net_export_cli_comment ]----------------------------------
static void _net_export_cli_comment(SLogStream * pStream,
				    const char * pcComment)
{
  log_printf(pStream, COMM_SEP2);
  log_printf(pStream, "# %s\n", pcComment);
  log_printf(pStream, COMM_SEP2);
}

// -----[ _net_export_cli_phys ]-------------------------------------
/**
 * Save all nodes, links, subnets
 */
static void _net_export_cli_phys(SLogStream * pStream,
				 SNetwork * pNetwork)
{
  SEnumerator * pEnum, * pEnumLinks;
  SNetNode * pNode;
  SNetSubnet * pSubnet;
  SNetLink * pLink;

  // *** all nodes ***
  pEnum= trie_get_enum(pNetwork->pNodes);
  while (enum_has_next(pEnum)) {
    pNode= *(SNetNode **) enum_get_next(pEnum);
    log_printf(pStream, "net add node ");
    ip_address_dump(pStream, pNode->tAddr);
    log_printf(pStream, "\n");
  }
  enum_destroy(&pEnum);

  // *** all subnets ***
  pEnum= _array_get_enum((SArray*) pNetwork->pSubnets);
  while (enum_has_next(pEnum)) {
    pSubnet= *(SNetSubnet **) enum_get_next(pEnum);

    log_printf(pStream, "net add subnet ");
    ip_prefix_dump(pStream, pSubnet->sPrefix);
    log_printf(pStream, " ");
    switch (pSubnet->uType) {
    case NET_SUBNET_TYPE_TRANSIT: log_printf(pStream, "transit"); break;
    case NET_SUBNET_TYPE_STUB: log_printf(pStream, "stub"); break;
    }
    log_printf(pStream, "\n");
  }
  enum_destroy(&pEnum);

  // *** all links ***
  pEnum= trie_get_enum(pNetwork->pNodes);
  while (enum_has_next(pEnum)) {
    pNode= *(SNetNode **) enum_get_next(pEnum);

    pEnumLinks= net_links_get_enum(pNode->pLinks);
    while (enum_has_next(pEnumLinks)) {
      pLink= *(SNetLink **) enum_get_next(pEnumLinks);

      if ((pLink->tType == NET_IFACE_VIRTUAL) ||
	  ((pLink->tType == NET_IFACE_RTR) &&
	   (pLink->tDest.pIface->pSrcNode->tAddr < pNode->tAddr)))
	continue;

      log_printf(pStream, "net add link ");
      ip_address_dump(pStream, pNode->tAddr);
      log_printf(pStream, " ");
      switch (pLink->tType) {
      case NET_IFACE_RTR:
	ip_address_dump(pStream, pLink->tDest.pIface->pSrcNode->tAddr);
	break;
      case NET_IFACE_PTP:
      case NET_IFACE_PTMP:
	ip_prefix_dump(pStream, pLink->tDest.pSubnet->sPrefix);
	break;
      case NET_IFACE_VIRTUAL:
	abort();
	break;
      default:
	abort();
      }
      log_printf(pStream, " %u\n", pLink->tDelay);
    }
    enum_destroy(&pEnumLinks);
  }
  enum_destroy(&pEnum);
}

// -----[ _net_export_cli_static ]-----------------------------------
/**
 * Static routes
 */
static void _net_export_cli_static(SLogStream * pStream,
				   SNetwork * pNetwork)
{
  SEnumerator * pEnumNodes, * pEnumRILists, * pEnumRIs;
  SNetNode * pNode;
  SNetRouteInfoList * pRIList;
  SNetRouteInfo * pRI;

  pEnumNodes= trie_get_enum(pNetwork->pNodes);
  while (enum_has_next(pEnumNodes)) {
    pNode= *(SNetNode **) enum_get_next(pEnumNodes);

    pEnumRILists= trie_get_enum(pNode->pRT);
    while (enum_has_next(pEnumRILists)) {
      pRIList= *(SNetRouteInfoList **) enum_get_next(pEnumRILists);

      pEnumRIs= _array_get_enum((SArray *) pRIList);
      while (enum_has_next(pEnumRIs)) {
	pRI= *(SNetRouteInfo **) enum_get_next(pEnumRIs);

	// Constraints
	// - Only static routes
	// - Static routes based on tunnel interfaces not supported
	if ((pRI->tType != NET_ROUTE_STATIC) ||
	    (pRI->sNextHop.pIface->tType == NET_IFACE_VIRTUAL))
	  continue;

	log_printf(pStream, "net node ");
	ip_address_dump(pStream, pNode->tAddr);
	log_printf(pStream, " route add ");

	// Destination prefix
	ip_prefix_dump(pStream, pRI->sPrefix);
	log_printf(pStream, " ");

	// Gateway ?
	if (pRI->sNextHop.tGateway != NET_ADDR_ANY)
	  ip_address_dump(pStream, pRI->sNextHop.tGateway);
	else
	  log_printf(pStream, "*");

	// Outgoing interface
	log_printf(pStream, " ");
	switch (pRI->sNextHop.pIface->tType) {
	case NET_IFACE_LOOPBACK:
	case NET_IFACE_RTR:
	case NET_IFACE_VIRTUAL:
	  ip_address_dump(pStream, pRI->sNextHop.pIface->tDest.pIface->pSrcNode->tAddr);
	  break;
	case NET_IFACE_PTMP:
	  ip_prefix_dump(pStream, pRI->sNextHop.pIface->tDest.pSubnet->sPrefix);
	  break;
	default:
	  abort();
	}

	// Metric
	log_printf(pStream, " %d\n", pRI->uWeight);

      }
      enum_destroy(&pEnumRIs);

    }
    enum_destroy(&pEnumRILists);
    
  }
  enum_destroy(&pEnumNodes);
}

static int _igp_domain_fe(SIGPDomain * pDomain, void * pContext)
{
  SEnumerator * pEnumRouters, * pEnumLinks;
  SLogStream * pStream= (SLogStream *) pContext;
  SNetNode * pRouter;
  SNetLink * pLink;


  log_printf(pStream, "net add domain %d igp\n", pDomain->uNumber);

  pEnumRouters= trie_get_enum(pDomain->pRouters);
  while (enum_has_next(pEnumRouters)) {
    pRouter= *(SNetNode **) enum_get_next(pEnumRouters);
    log_printf(pStream, "net node ");
    ip_address_dump(pStream, pRouter->tAddr);
    log_printf(pStream, " domain %d\n", pDomain->uNumber);
  }
  enum_destroy(&pEnumRouters);

  pEnumRouters= trie_get_enum(pDomain->pRouters);
  while (enum_has_next(pEnumRouters)) {
    pRouter= *(SNetNode **) enum_get_next(pEnumRouters);
    
    pEnumLinks= net_links_get_enum(pRouter->pLinks);
    while (enum_has_next(pEnumLinks)) {
      pLink= *(SNetLink **) enum_get_next(pEnumLinks);
      
      if (pLink->tType == NET_IFACE_VIRTUAL)
	continue;
      
      log_printf(pStream, "net link ");
      ip_address_dump(pStream, pRouter->tAddr);
      log_printf(pStream, " ");
      switch (pLink->tType) {
      case NET_IFACE_LOOPBACK:
      case NET_IFACE_RTR:
      case NET_IFACE_VIRTUAL:
	ip_address_dump(pStream, pLink->tDest.pIface->pSrcNode->tAddr);
	break;
      case NET_IFACE_PTMP:
	ip_prefix_dump(pStream, pLink->tDest.pSubnet->sPrefix);
	break;
      default:
	abort();
      }
      log_printf(pStream, " igp-weight %u\n", net_iface_get_metric(pLink, 0));
    }
    enum_destroy(&pEnumLinks);

  }
  enum_destroy(&pEnumRouters);
  
  log_printf(pStream, "net domain %d compute\n", pDomain->uNumber);

  return 0;
}

// -----[ _net_export_cli_igp ]--------------------------------------
/**
 * IGP configuration (domains, weights)
 */
static void _net_export_cli_igp(SLogStream * pStream,
				SNetwork * pNetwork)
{
  igp_domains_for_each(_igp_domain_fe, pStream);
}

// -----[ _net_export_cli_bgp ]--------------------------------------
/**
 * BGP configuration
 */
static void _net_export_cli_bgp(SLogStream * pStream,
				SNetwork * pNetwork)
{
}

// -----[ net_export_cli ]-------------------------------------------
/**
 *
 */
int net_export_cli(SLogStream * pStream, SNetwork * pNetwork)
{
  _net_export_cli_header(pStream);
  log_printf(pStream, "\n");

  _net_export_cli_comment(pStream, "Physical topology");
  _net_export_cli_phys(pStream, pNetwork);
  log_printf(pStream, "\n");

  _net_export_cli_comment(pStream, "Static routing");
  _net_export_cli_static(pStream, pNetwork);
  log_printf(pStream, "\n");

  _net_export_cli_comment(pStream, "IGP routing"); 
  _net_export_cli_igp(pStream, pNetwork);
  log_printf(pStream, "\n");

  _net_export_cli_comment(pStream, "BGP routing"); 
  _net_export_cli_bgp(pStream, pNetwork);

  return NET_SUCCESS;
}

