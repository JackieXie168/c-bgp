// ==================================================================
// @(#)ip_trace.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 01/03/2008
// @lastdate 11/03/2008
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
		   net_addr_t iif_addr,
		   net_addr_t oif_addr)
{
  ip_trace_item_t * item= (ip_trace_item_t *) MALLOC(sizeof(ip_trace_item_t));
  item->elt.type= NODE;
  item->elt.node= node;
  item->iif_addr= iif_addr;
  item->oif_addr= oif_addr;
  return item;
}

// -----[ ip_trace_item_subnet ]-------------------------------------
static inline ip_trace_item_t *
ip_trace_item_subnet(net_subnet_t * subnet)
{
  ip_trace_item_t * item= (ip_trace_item_t *) MALLOC(sizeof(ip_trace_item_t));
  item->elt.type= SUBNET;
  item->elt.subnet= subnet;
  item->iif_addr= NET_ADDR_ANY;
  item->oif_addr= NET_ADDR_ANY;
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
				    net_addr_t iif_addr,
				    net_addr_t oif_addr)
{
  return _ip_trace_add(trace, ip_trace_item_node(node, iif_addr, oif_addr));
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
