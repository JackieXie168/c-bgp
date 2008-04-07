// ==================================================================
// @(#)icmp_options.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 01/04/2008
// $Id: icmp_options.c,v 1.1 2008-04-07 09:32:47 bqu Exp $
// ==================================================================

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include <net/icmp.h>
#include <net/icmp_options.h>
#include <net/ip_trace.h>
#include <net/node.h>

// -----[ _info_update_qos ]-----------------------------------------
/**
 * Update the record-route info with the traversed link's QoS info.
 */
static inline void _info_update_qos(icmp_msg_t * icmp_msg,
				    net_iface_t * oif)
{
  net_link_load_t capacity;
  ip_trace_t * trace= icmp_msg->opts.trace;

  // Keep total propagation delay
  trace->delay+= net_iface_get_delay(oif);

  // Keep total IGP weight (for the given topology, i.e. TOS)
  trace->weight+= net_iface_get_metric(oif, 0/*pInfo->tTOS*/);

  // Keep maximum possible capacity (=> keep minimum along the path)
  capacity= net_iface_get_capacity(oif);
  if (capacity < trace->capacity)
    trace->capacity= capacity;

  // Load the outgoing link, if requested.
  if (icmp_msg->opts.options & ICMP_RR_OPTION_LOAD)
    net_iface_add_load(oif, trace->load);
}

// -----[ _check_loop ]----------------------------------------------
/**
 * Checks if the current trace contains a loop.
 *
 * Note: will this work with encapsulation ?
 */
static inline net_error_t
_check_loop(icmp_msg_t * msg, net_node_t * node)
{
  if (!(msg->opts.options & ICMP_RR_OPTION_QUICK_LOOP))
    return ESUCCESS;

  if (ip_trace_search(msg->opts.trace, node)) {
    msg->opts.trace->status= ENET_FWD_LOOP;
    return ENET_FWD_LOOP;
  }

  return ESUCCESS;
}

// -----[ _icmp_process_in_options ]---------------------------------
/**
 * The role of this function is to process incoming ICMP message
 * options. There are basically two operations performed:
 * - lookup based on alternate destination
 * - update IP trace
 */
static inline net_error_t 
_icmp_process_in_options(net_node_t * node,
			 net_iface_t * iface,
			 icmp_msg_t * icmp_msg,
			 const rt_entry_t ** rtentry_ref)
{
  net_error_t error;
  ip_trace_item_t * last_item;
  rt_info_t * rtinfo;

  /* Perform lookup with alternate destination ? */
  if (icmp_msg->opts.options & ICMP_RR_OPTION_ALT_DEST) {
    /* Destination reached ? */
    if (node_has_prefix(node, icmp_msg->opts.alt_dest)) {
      if (icmp_msg->opts.trace != NULL)
	icmp_msg->opts.trace->status= ESUCCESS;
      return EUNSPECIFIED;
    }

    /* Perform exact match with provided destination prefix (alt_dest) */
    rtinfo= rt_find_exact(node->rt, icmp_msg->opts.alt_dest,
			  NET_ROUTE_ANY);
    if (rtinfo == NULL)
      return ENET_NET_UNREACH;
    *rtentry_ref= &rtinfo->sNextHop;
  }

  /* Need to update trace ? */
  if (icmp_msg->opts.trace != NULL) {
    last_item= ip_trace_add_node(icmp_msg->opts.trace, node,
				 NET_ADDR_ANY, NET_ADDR_ANY);
    if (iface != NULL)
      last_item->iif_addr= iface->tIfaceAddr;
  
    /* Check if the packet is looping back to an already traversed node */
    error= _check_loop(icmp_msg, node);
    if (error != ESUCCESS)
      return error;

  }

  return ESUCCESS;
}

// -----[ _icmp_process_out_options ]--------------------------------
/**
 * The role of this function is to process outgoing ICMP message
 * options. The single operation performed is to update the IP trace.
 */
static inline net_error_t
_icmp_process_out_options(net_node_t * node,
			  net_iface_t * iface,
			  icmp_msg_t * icmp_msg)
{
  ip_trace_item_t * last_item;
  unsigned int trace_len;
  
  /* Need to update trace ? */
  if (icmp_msg->opts.trace != NULL) {
    trace_len= ip_trace_length(icmp_msg->opts.trace);
    assert(trace_len > 0);
    last_item= ip_trace_item_at(icmp_msg->opts.trace, trace_len-1);
    assert(last_item->elt.node == node);
    last_item->oif_addr= iface->tIfaceAddr;
    _info_update_qos(icmp_msg, iface);

  }

  return ESUCCESS;
}

// -----[ _icmp_process_encap_options ]------------------------------
static inline net_error_t _icmp_process_encap_options()
{
  return EUNSUPPORTED;
}

// -----[ _icmp_process_decap_options ]------------------------------
static inline net_error_t _icmp_process_decap_options()
{
  return EUNSUPPORTED;
}

// -----[ icmp_process_options ]-------------------------------------
/**
 * Process the options of an ICMP message. The function can be
 * invoked at different times of the IP processing (states):
 *   1). when the message is received by the IP forwarding process;
 *       in this case, the state is ICMP_OPT_STATE_INCOMING.
 *   2). when the message is send by the IP forwarding process;
 *       in this case, the state is ICMP_OPT_STATE_OUTGOING.
 *   3-4). when the message is en-/decapsulated, the state is
 *       respectively ICMP_OPT_STATE_ENCAP/DECAP. These states are
 *       NOT SUPPORTED YET.
 *
 * The parameters are as follows:
 *   - node   : is the node where the IP processing occurs
 *   - iface  : is the incoming/outgoing interface (depending on
 *              state)
 *   - msg    : is the ICMP message
 *   - rtentry: is a reference to the current routing table entry;
 *              (it can be modified in order to perform RT lookups
 *              based on exact prefix matching)
 */
net_error_t icmp_process_options(icmp_opt_state_t state,
				 net_node_t * node,
				 net_iface_t * iface,
				 net_msg_t * msg,
				 const rt_entry_t ** rtentry_ref)
{
  icmp_msg_t * icmp_msg= (icmp_msg_t *) msg->payload;

  switch (state) {
  case ICMP_OPT_STATE_INCOMING:
    return _icmp_process_in_options(node, iface, icmp_msg, rtentry_ref);

  case ICMP_OPT_STATE_OUTGOING:
    return _icmp_process_out_options(node, iface, icmp_msg);

  case ICMP_OPT_STATE_ENCAP:
    return _icmp_process_encap_options();

  case ICMP_OPT_STATE_DECAP:
    return _icmp_process_decap_options();

  default:
    fatal("state (%d) is not supported by icmp_process_options()");
  }
  return EUNSUPPORTED;
}
