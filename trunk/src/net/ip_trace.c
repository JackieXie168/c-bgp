// ==================================================================
// @(#)ip_trace.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 01/03/2008
// $Id: ip_trace.c,v 1.5 2008-06-11 15:13:45 bqu Exp $
// ==================================================================

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <libgds/array.h>

#include <net/ip_trace.h>

// -----[ ip_trace_item_node ]---------------------------------------
static inline ip_trace_item_t *
ip_trace_item_node(net_node_t * node,
		   net_iface_t * iif,
		   net_iface_t * oif)
{
  ip_trace_item_t * item= (ip_trace_item_t *) MALLOC(sizeof(ip_trace_item_t));
  item->elt.type= NODE;
  item->elt.node= node;
  item->iif= iif;
  item->oif= oif;
  return item;
}

// -----[ ip_trace_item_subnet ]-------------------------------------
static inline ip_trace_item_t *
ip_trace_item_subnet(net_subnet_t * subnet)
{
  ip_trace_item_t * item= (ip_trace_item_t *) MALLOC(sizeof(ip_trace_item_t));
  item->elt.type= SUBNET;
  item->elt.subnet= subnet;
  item->iif= NULL;
  item->oif= NULL;
  return item;
}

// -----[ ip_trace_create ]------------------------------------------
ip_trace_t * ip_trace_create()
{
  ip_trace_t * trace= (ip_trace_t *) MALLOC(sizeof(ip_trace_t));
  trace->items= ptr_array_create(0, NULL, NULL);
  trace->delay= 0;
  trace->capacity= NET_LINK_MAX_CAPACITY;
  trace->weight= 0;
  trace->load= 0;
  return trace;
}

// -----[ ip_trace_destroy ]-----------------------------------------
void ip_trace_destroy(ip_trace_t ** trace_ref)
{
  if (*trace_ref != NULL) {
    ptr_array_destroy(&(*trace_ref)->items);
    FREE(*trace_ref);
    *trace_ref= NULL;
  }
}

// -----[ ip_trace_add ]---------------------------------------------
static inline ip_trace_item_t * _ip_trace_add(ip_trace_t * trace,
					      ip_trace_item_t * item)
{
  assert(ptr_array_add(trace->items, &item) >= 0);
  return item;
}

// -----[ ip_trace_add_node ]----------------------------------------
ip_trace_item_t * ip_trace_add_node(ip_trace_t * trace,
				    net_node_t * node,
				    net_iface_t * iif,
				    net_iface_t * oif)
{
  return _ip_trace_add(trace, ip_trace_item_node(node, iif, oif));
}

// -----[ ip_trace_add_subnet ]--------------------------------------
ip_trace_item_t * ip_trace_add_subnet(ip_trace_t * trace,
				      net_subnet_t * subnet)
{
  return _ip_trace_add(trace, ip_trace_item_subnet(subnet));
}

// -----[ ip_trace_search ]------------------------------------------
int ip_trace_search(ip_trace_t * trace, net_node_t * node)
{
  unsigned int index;
  ip_trace_item_t * item;
  for (index= 0; index < ip_trace_length(trace); index++) {
    item= ip_trace_item_at(trace, index);
    if ((item->elt.type == NODE) && (item->elt.node == node))
      return 1;
  }
  return 0;
}

// -----[ ip_trace_dump ]--------------------------------------------
void ip_trace_dump(SLogStream * stream, ip_trace_t * trace,
		   uint8_t options)
{
  ip_trace_item_t * item;
  int trace_length= 0;
  int index;

  if (options & IP_TRACE_DUMP_LENGTH) {
    // Compute trace length (only nodes)
    for (index= 0; index < ip_trace_length(trace); index++) {
      switch (ip_trace_item_at(trace, index)->elt.type) {
      case NODE:
	trace_length++;
	break;
      case SUBNET:
	if (options & IP_TRACE_DUMP_SUBNETS)
	  trace_length++;
	break;
      }
    }

    // Dump trace length to output
    log_printf(stream, "\t%u\t", trace_length);
  }

  // Dump each (node) hop to output
  for (index= 0; index < ip_trace_length(trace); index++) {
    item= ip_trace_item_at(trace, index);
    switch (item->elt.type) {
    case NODE:
      ip_address_dump(stream, item->elt.node->addr);
      break;
    case SUBNET:
      if (options & IP_TRACE_DUMP_SUBNETS)
	ip_prefix_dump(stream, item->elt.subnet->prefix);
      break;
    default:
      fatal("invalid network-element type (%d)\n",
	    item->elt.type);
    }
    if (index+1 < trace_length)
      log_printf(stream, " ");
  }
}
