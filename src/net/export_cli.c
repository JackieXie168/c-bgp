// ==================================================================
// @(#)export_cli.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 15/10/07
// $Id: export_cli.c,v 1.5 2008-04-11 11:03:06 bqu Exp $
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
static void _net_export_cli_header(SLogStream * stream)
{
  time_t tCurrentTime= time(NULL);

  log_printf(stream, COMM_SEP1);
  log_printf(stream, "# C-BGP Export file (CLI)\n");
  log_printf(stream, "# generated on %s", ctime(&tCurrentTime));
  log_printf(stream, COMM_SEP1);
}

// -----[ _net_export_cli_comment ]----------------------------------
static void _net_export_cli_comment(SLogStream * stream,
				    const char * comment)
{
  log_printf(stream, COMM_SEP2);
  log_printf(stream, "# %s\n", comment);
  log_printf(stream, COMM_SEP2);
}

// -----[ _net_export_cli_phys ]-------------------------------------
/**
 * Save all nodes, links, subnets
 */
static void _net_export_cli_phys(SLogStream * stream,
				 network_t * network)
{
  enum_t * pEnum, * pEnumLinks;
  net_node_t * node;
  net_subnet_t * subnet;
  net_iface_t * link;

  // *** all nodes ***
  pEnum= trie_get_enum(network->nodes);
  while (enum_has_next(pEnum)) {
    node= *(net_node_t **) enum_get_next(pEnum);
    log_printf(stream, "net add node ");
    ip_address_dump(stream, node->addr);
    log_printf(stream, "\n");
  }
  enum_destroy(&pEnum);

  // *** all subnets ***
  pEnum= _array_get_enum((SArray*) network->subnets);
  while (enum_has_next(pEnum)) {
    subnet= *(net_subnet_t **) enum_get_next(pEnum);

    log_printf(stream, "net add subnet ");
    ip_prefix_dump(stream, subnet->prefix);
    log_printf(stream, " ");
    switch (subnet->type) {
    case NET_SUBNET_TYPE_TRANSIT: log_printf(stream, "transit"); break;
    case NET_SUBNET_TYPE_STUB: log_printf(stream, "stub"); break;
    default:
      abort();
    }
    log_printf(stream, "\n");
  }
  enum_destroy(&pEnum);

  // *** all links ***
  pEnum= trie_get_enum(network->nodes);
  while (enum_has_next(pEnum)) {
    node= *(net_node_t **) enum_get_next(pEnum);

    pEnumLinks= net_links_get_enum(node->ifaces);
    while (enum_has_next(pEnumLinks)) {
      link= *(net_iface_t **) enum_get_next(pEnumLinks);

      if ((link->type == NET_IFACE_VIRTUAL) ||
	  ((link->type == NET_IFACE_RTR) &&
	   (link->dest.iface->src_node->addr < node->addr)))
	continue;

      log_printf(stream, "net add link ");
      ip_address_dump(stream, node->addr);
      log_printf(stream, " ");
      switch (link->type) {
      case NET_IFACE_RTR:
	ip_address_dump(stream, link->dest.iface->src_node->addr);
	break;
      case NET_IFACE_PTP:
      case NET_IFACE_PTMP:
	ip_prefix_dump(stream, link->dest.subnet->prefix);
	break;
      case NET_IFACE_VIRTUAL:
	abort();
	break;
      default:
	abort();
      }
      log_printf(stream, " %u\n", link->phys.delay);
    }
    enum_destroy(&pEnumLinks);
  }
  enum_destroy(&pEnum);
}

// -----[ _net_export_cli_static ]-----------------------------------
/**
 * Static routes
 */
static void _net_export_cli_static(SLogStream * stream,
				   network_t * network)
{
  enum_t * pEnumNodes, * pEnumRILists, * pEnumRIs;
  net_node_t * node;
  rt_info_list_t * list;
  rt_info_t * rtinfo;

  pEnumNodes= trie_get_enum(network->nodes);
  while (enum_has_next(pEnumNodes)) {
    node= *(net_node_t **) enum_get_next(pEnumNodes);

    pEnumRILists= trie_get_enum(node->rt);
    while (enum_has_next(pEnumRILists)) {
      list= *(rt_info_list_t **) enum_get_next(pEnumRILists);

      pEnumRIs= _array_get_enum((SArray *) list);
      while (enum_has_next(pEnumRIs)) {
	rtinfo= *(rt_info_t **) enum_get_next(pEnumRIs);

	// Constraints
	// - Only static routes
	// - Static routes based on tunnel interfaces not supported
	if ((rtinfo->type != NET_ROUTE_STATIC) ||
	    (rtinfo->next_hop.oif->type == NET_IFACE_VIRTUAL))
	  continue;

	log_printf(stream, "net node ");
	ip_address_dump(stream, node->addr);
	log_printf(stream, " route add ");

	// Destination prefix
	ip_prefix_dump(stream, rtinfo->prefix);
	log_printf(stream, " ");

	// Gateway ?
	if (rtinfo->next_hop.gateway != NET_ADDR_ANY)
	  ip_address_dump(stream, rtinfo->next_hop.gateway);
	else
	  log_printf(stream, "*");

	// Outgoing interface
	log_printf(stream, " ");
	switch (rtinfo->next_hop.oif->type) {
	case NET_IFACE_LOOPBACK:
	case NET_IFACE_RTR:
	case NET_IFACE_VIRTUAL:
	  ip_address_dump(stream, rtinfo->next_hop.oif->dest.iface->src_node->addr);
	  break;
	case NET_IFACE_PTMP:
	  ip_prefix_dump(stream, rtinfo->next_hop.oif->dest.subnet->prefix);
	  break;
	default:
	  abort();
	}

	// Metric
	log_printf(stream, " %d\n", rtinfo->metric);

      }
      enum_destroy(&pEnumRIs);

    }
    enum_destroy(&pEnumRILists);
    
  }
  enum_destroy(&pEnumNodes);
}

static int _igp_domain_fe(igp_domain_t * domain, void * ctx)
{
  enum_t * pEnumRouters, * pEnumLinks;
  SLogStream * stream= (SLogStream *) ctx;
  net_node_t * router;
  net_iface_t * link;


  log_printf(stream, "net add domain %d igp\n", domain->id);

  pEnumRouters= trie_get_enum(domain->routers);
  while (enum_has_next(pEnumRouters)) {
    router= *(net_node_t **) enum_get_next(pEnumRouters);
    log_printf(stream, "net node ");
    ip_address_dump(stream, router->addr);
    log_printf(stream, " domain %d\n", domain->id);
  }
  enum_destroy(&pEnumRouters);

  pEnumRouters= trie_get_enum(domain->routers);
  while (enum_has_next(pEnumRouters)) {
    router= *(net_node_t **) enum_get_next(pEnumRouters);
    
    pEnumLinks= net_links_get_enum(router->ifaces);
    while (enum_has_next(pEnumLinks)) {
      link= *(net_iface_t **) enum_get_next(pEnumLinks);
      
      if (link->type == NET_IFACE_VIRTUAL)
	continue;
      
      log_printf(stream, "net link ");
      ip_address_dump(stream, router->addr);
      log_printf(stream, " ");
      switch (link->type) {
      case NET_IFACE_LOOPBACK:
      case NET_IFACE_RTR:
      case NET_IFACE_VIRTUAL:
	ip_address_dump(stream, link->dest.iface->src_node->addr);
	break;
      case NET_IFACE_PTMP:
	ip_prefix_dump(stream, link->dest.subnet->prefix);
	break;
      default:
	abort();
      }
      log_printf(stream, " igp-weight %u\n", net_iface_get_metric(link, 0));
    }
    enum_destroy(&pEnumLinks);

  }
  enum_destroy(&pEnumRouters);
  
  log_printf(stream, "net domain %d compute\n", domain->id);

  return 0;
}

// -----[ _net_export_cli_igp ]--------------------------------------
/**
 * IGP configuration (domains, weights)
 */
static void _net_export_cli_igp(SLogStream * stream,
				network_t * network)
{
  igp_domains_for_each(_igp_domain_fe, stream);
}

// -----[ _net_export_cli_bgp ]--------------------------------------
/**
 * BGP configuration
 */
static void _net_export_cli_bgp(SLogStream * stream,
				network_t * network)
{
}

// -----[ net_export_cli ]-------------------------------------------
/**
 *
 */
int net_export_cli(SLogStream * stream, network_t * network)
{
  _net_export_cli_header(stream);
  log_printf(stream, "\n");

  _net_export_cli_comment(stream, "Physical topology");
  _net_export_cli_phys(stream, network);
  log_printf(stream, "\n");

  _net_export_cli_comment(stream, "Static routing");
  _net_export_cli_static(stream, network);
  log_printf(stream, "\n");

  _net_export_cli_comment(stream, "IGP routing"); 
  _net_export_cli_igp(stream, network);
  log_printf(stream, "\n");

  _net_export_cli_comment(stream, "BGP routing"); 
  _net_export_cli_bgp(stream, network);

  return ESUCCESS;
}

