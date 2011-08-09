// ==================================================================
// @(#)icmp_options.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 01/04/2008
// $Id: icmp_options.c,v 1.6 2009-08-31 09:48:28 bqu Exp $
// ==================================================================

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include <net/icmp.h>
#include <net/icmp_options.h>
#include <net/ip_trace.h>
#include <net/node.h>
#include <util/str_format.h>

// Limit on the number of parallel paths
#define MAX_TRACE_FIFO_DEPTH 100

//#define IP_OPT_DEBUG

#ifdef IP_OPT_DEBUG
static int _debug_for_each(gds_stream_t * stream, void * context,
			   char format)
{
  va_list * ap= (va_list*) context;
  net_node_t * node;
  net_addr_t addr;
  net_msg_t * msg;
  net_iface_t * iface;
  ip_opt_t * opts;
  char * s;

  switch (format) {
  case 'a':
    addr= va_arg(*ap, net_addr_t);
    ip_address_dump(stream, addr);
    break;
  case 'i':
    iface= va_arg(*ap, net_iface_t *);
    net_iface_dump_id(stream, iface);
    break;
  case 'm':
    msg= va_arg(*ap, net_msg_t *);
    stream_printf(stream, "[");
    message_dump(stream, msg);
    stream_printf(stream, "]");
    break;
  case 'n':
    node= va_arg(*ap, net_node_t *);
    node_dump_id(stream, node);
    break;
  case 'o':
    opts= va_arg(*ap, ip_opt_t *);
    if (opts == NULL) {
      stream_printf(stream, "null");
      break;
    }
    stream_printf(stream, " %p {", opts);
    stream_flush(stream);
    if (opts->flags & IP_OPT_ALT_DEST) {
      stream_printf(stream, "alt-dest:");
      ip_prefix_dump(stream, opts->alt_dest);
      stream_printf(stream, " ");
    }
    if (opts->flags & IP_OPT_TRACE) {
      stream_printf(stream, "trace:(%p)");
      stream_flush(stream);
      if (opts->trace != NULL) {
	stream_printf(stream, "[");
	ip_trace_dump(stream, opts->trace, 0);
	stream_printf(stream, "]");
      }
      stream_printf(stream, " ");
    }
    stream_printf(stream, "}");
    break;
  case 's':
    s= va_arg(*ap, char *);
    stream_printf(stream, "%s", s);
    break;
  }
  return 0;
}
#endif /* IP_OPT_DEBUG */

#ifdef IP_OPT_DEBUG
static inline void ___ip_opt_debug(const char * msg, ...)
{
  va_list ap;

  va_start(ap, msg);
  stream_printf(gdsout, "IP_OPT_DBG::");
  str_format_for_each(gdsout, _debug_for_each, &ap, msg);
  stream_flush(gdsout);
  va_end(ap);
}
#else
#define ___ip_opt_debug(M...)
#endif /* IP_OPT_DEBUG */


// -----[ _info_update_qos ]-----------------------------------------
/**
 * Update the record-route info with the traversed link's QoS info.
 */
static inline void _info_update_qos(net_msg_t * msg,
				    net_iface_t * oif)
{
  net_link_load_t capacity;
  ip_trace_t * trace= msg->opts->trace;

  // Keep total propagation delay
  trace->delay+= net_iface_get_delay(oif);

  // Keep total IGP weight (for the given topology, i.e. TOS)
  trace->weight+= net_iface_get_metric(oif, 0);

  // Keep maximum possible capacity (=> keep minimum along the path)
  capacity= net_iface_get_capacity(oif);
  if (capacity < trace->capacity)
    trace->capacity= capacity;

  // Load the outgoing link, if requested.
  if (msg->opts->flags & IP_OPT_LOAD)
    net_iface_add_load(oif, msg->opts->load);
}

// -----[ _check_loop ]----------------------------------------------
/**
 * Checks if the current trace contains a loop.
 *
 * Note: will this work with encapsulation ?
 */
static inline net_error_t
_check_loop(net_msg_t * msg, net_node_t * node)
{
  if (!(msg->opts->flags & IP_OPT_QUICK_LOOP))
    return ESUCCESS;

  if (ip_trace_search(msg->opts->trace, node)) {
    msg->opts->trace->status= ENET_FWD_LOOP;
    return ENET_FWD_LOOP;
  }

  return ESUCCESS;
}

static inline
net_error_t _process_msg_alt_fwd(net_node_t * node,
				 net_msg_t * msg,
				 const rt_entries_t ** rtentries)
{
  rt_info_t * rtinfo= NULL;
  net_error_t error= ESUCCESS;

  if (*rtentries != NULL)
    return ESUCCESS;

  if (msg->opts->flags & IP_OPT_ALT_DEST) {

    // Perform exact match with provided destination prefix (alt_dest).
    rtinfo= rt_find_exact(node->rt, msg->opts->alt_dest, NET_ROUTE_ANY);
    if (rtinfo == NULL) {
      error= ENET_NET_UNREACH;
      if (msg->opts->flags & IP_OPT_TRACE)
	msg->opts->trace->status= error;
    } else
      *rtentries= rtinfo->entries;
	
  }

  return error;
}

// -----[ _has_option ]----------------------------------------------
static inline int _has_option(net_msg_t * msg)
{
  return ((msg != NULL) && (msg->opts != NULL));
}

// -----[ ip_opt_hook_msg_error ]---------------------------------
net_error_t ip_opt_hook_msg_error(net_msg_t * msg, net_error_t error)
{
  // Skip if no option
  if (!_has_option(msg))
    return ESUCCESS;

  ___ip_opt_debug("ERROR\n");
  
  if (!(msg->opts->flags & IP_OPT_TRACE))
    return ESUCCESS;

  msg->opts->trace->status= error;
  // ???? What is the goal of the following code ????
  /*while (msg->protocol == NET_PROTOCOL_IPIP) {
    ip_trace_add_trace(((net_msg_t *) msg->payload)->opts->trace,
		       msg->opts->trace);
    msg= (net_msg_t *) msg->payload;
    msg->opts->trace->status= error;
    }*/
  return error;
}

// -----[ ip_opt_hook_msg_sent ]----------------------------------
net_error_t ip_opt_hook_msg_sent(net_node_t * node,
				 net_msg_t * msg,
				 const rt_entries_t ** rtentries)
{
  net_error_t error;
  ip_trace_item_t * last_item;

  // Skip if no option
  if (!_has_option(msg))
    return ESUCCESS;

  ___ip_opt_debug("SENT node=%n opts=%o\n", node, msg->opts);

  // Forward according to alternate dest ?
  error= _process_msg_alt_fwd(node, msg, rtentries);
  if (error != ESUCCESS)
    return error;
  
  /* Need to update trace ? */
  if (msg->opts->flags & IP_OPT_TRACE) {
    last_item= ip_trace_last_item(msg->opts->trace);
    if ((last_item == NULL) ||
	(last_item->elt.type != NODE) ||
	(last_item->elt.node != node))
      ip_trace_add_node(msg->opts->trace, node, NET_ADDR_ANY, NET_ADDR_ANY);
  }
  return ESUCCESS;
}

// -----[ ip_opt_hook_msg_rcvd ]----------------------------------
net_error_t ip_opt_hook_msg_rcvd(net_node_t * node,
				 net_iface_t * iif,
				 net_msg_t * msg)
{
  ip_trace_item_t * last_item;

  // Skip if no option
  if (!_has_option(msg))
    return ESUCCESS;

  ___ip_opt_debug("RCVD node=%n, iif=%i opts=%o\n", node, iif, msg->opts);

  /* Need to update trace ? */
  if (msg->opts->flags & IP_OPT_TRACE) {
    last_item= ip_trace_last_item(msg->opts->trace);
    assert(last_item != NULL);
    assert((last_item->elt.type == NODE) &&
	   (last_item->elt.node == node));
    last_item->iif= iif;
  }
  return ESUCCESS;
}

// -----[ ip_opt_hook_msg_in ]------------------------------------
/**
 * The role of this function is to process incoming ICMP message
 * options. There are basically two operations performed:
 * - lookup based on alternate destination
 * - update IP trace
 */
net_error_t ip_opt_hook_msg_in(net_node_t * node,
			       net_iface_t * iif,
			       net_msg_t * msg,
			       const rt_entries_t ** rtentries)
{
  ip_trace_item_t * last_item;
  net_error_t error;

  // Skip if no option
  if (!_has_option(msg))
    return ESUCCESS;

  ___ip_opt_debug("IN node=%n, iif=%i opts=%o\n", node, iif, msg->opts);

  if (msg->opts->flags & IP_OPT_TRACE) {
    last_item= ip_trace_last_item(msg->opts->trace);
    if ((last_item != NULL) &&
	(last_item->elt.type == NODE) &&
	(last_item->elt.node == node)) {

    } else {

      ip_trace_add_node(msg->opts->trace, node, iif, NET_ADDR_ANY);

      // Check if the packet is looping back to an already traversed node
      error= _check_loop(msg, node);
      if (error != ESUCCESS)
	return error;
    }

  }

  // Forward according to alternate dest ?
  error= _process_msg_alt_fwd(node, msg, rtentries);
  if (error != ESUCCESS)
    return error;

  return ESUCCESS;
}

// -----[ ip_opt_hook_msg_out ]-----------------------------------
/**
 * The role of this function is to process outgoing ICMP message
 * options. The single operation performed is to update the IP trace.
 */
net_error_t ip_opt_hook_msg_out(net_node_t * node,
				net_iface_t * oif,
				net_msg_t * msg)
{
  ip_trace_item_t * last_item;

  // Skip if no option
  if (!_has_option(msg))
    return ESUCCESS;

  ___ip_opt_debug("OUT node=%n, oif=%i\n", node, oif);
  
  /* Need to update trace ? */
  if (msg->opts->flags & IP_OPT_TRACE) {
    last_item= ip_trace_last_item(msg->opts->trace);
    assert(last_item != NULL);
    assert((last_item->elt.type == NODE) &&
	   (last_item->elt.node == node));
    last_item->oif= oif;
    _info_update_qos(msg, oif);
  }

  return ESUCCESS;
}

// -----[ ip_opt_hook_msg_subnet ]--------------------------------
net_error_t ip_opt_hook_msg_subnet(net_subnet_t * subnet,
				   net_msg_t * msg,
				   int * reached)
{
  // Skip if no option
  if (!_has_option(msg))
    return ESUCCESS;

  ___ip_opt_debug("SUBNET\n");

  // Add subnet to trace
  if (msg->opts->flags & IP_OPT_TRACE)
    ip_trace_add_subnet(msg->opts->trace, subnet);

  if (msg->opts->flags & IP_OPT_ALT_DEST) {
    if (!ip_prefix_cmp(&msg->opts->alt_dest, &subnet->prefix)) {
      if (msg->opts->flags & IP_OPT_TRACE)
	msg->opts->trace->status= ESUCCESS;
      *reached= 1;
      return ESUCCESS;
    }
  }

  return ESUCCESS;
}

// -----[ ip_opt_hook_msg_encap ]---------------------------------
net_error_t ip_opt_hook_msg_encap(net_node_t * node,
				  net_msg_t * outer_msg,
				  net_msg_t * inner_msg)
{
  // Skip if no option
  if (!_has_option(inner_msg))
    return ESUCCESS;

  ___ip_opt_debug("ENCAP node=%n msg=%m opts=%o\n", node, inner_msg,
		  inner_msg->opts);

  if (!(inner_msg->opts->flags & IP_OPT_TUNNEL))
    return ESUCCESS;

  // Create new inside trace. Set status to no-reply.
  outer_msg->opts->trace= ip_trace_create();
  outer_msg->opts->trace->status= ENET_NO_REPLY;

  return ESUCCESS;
}

// -----[ ip_opt_hook_msg_decap ]---------------------------------
net_error_t ip_opt_hook_msg_decap(net_node_t * node,
				  net_msg_t * outer_msg,
				  net_msg_t * inner_msg)
{
  // Skip if no option
  if (!_has_option(inner_msg))
    return ESUCCESS;

  ___ip_opt_debug("DECAP node=%n opts=%o\n", node, inner_msg->opts);

  if (!(inner_msg->opts->flags & IP_OPT_TUNNEL))
    return ESUCCESS;

  // Signal that tunnel trace is successfull
  assert(outer_msg->opts->trace != NULL);
  outer_msg->opts->trace->status= ESUCCESS;

  // Connect inside trace (outer msg)  to outside trace (inner msg)
  ip_trace_add_trace(inner_msg->opts->trace, outer_msg->opts->trace);

  return ESUCCESS;
}

typedef struct _ecmp_ctx_t {
  net_node_t   * node;
  net_msg_t    * msg;
  rt_entries_t * rtentries;
} _ecmp_ctx_t;

// -----[ ip_opt_hook_msg_ecmp ]----------------------------------
net_error_t ip_opt_hook_msg_ecmp(net_node_t * node,
				 net_msg_t * msg,
				 const rt_entries_t ** rtentries)
{
  unsigned int num_entries;
  unsigned int index;
  net_msg_t * msg_copy;

  // Skip if no option
  if (!_has_option(msg))
    return ESUCCESS;
  
  if (!(msg->opts->flags & IP_OPT_ECMP))
    return ESUCCESS;

  num_entries= rt_entries_size(*rtentries);
  if (num_entries <= 1)
    return ESUCCESS;

  ___ip_opt_debug("ECMP node=%n, msg=%m, opts=%o\n", node, msg, msg->opts);

  msg->opts->load/= num_entries;

  for (index= 1; index < num_entries; index++) {
    msg_copy= message_copy(msg);
    if (msg->opts->trace != NULL)
      msg_copy->opts->trace= ip_trace_copy(msg->opts->trace);
    ip_opt_ecmp_push(msg->opts, node, msg_copy,
		     rt_entries_get_at(*rtentries, index));
  }

  return ESUCCESS;
}

// -----[ ip_opt_ecmp_push ]-----------------------------------------
void ip_opt_ecmp_push(ip_opt_t * opts, net_node_t * node,
		      net_msg_t * msg, rt_entry_t * rtentry)
{
  _ecmp_ctx_t * ctx= (_ecmp_ctx_t *) MALLOC(sizeof(_ecmp_ctx_t));
  ctx->msg= msg;
  ctx->node= node;
  if (rtentry != NULL) {
    ctx->rtentries= rt_entries_create();
    rt_entry_add_ref(rtentry);
    rt_entries_add(ctx->rtentries, rtentry);
  } else
    ctx->rtentries= NULL;
  fifo_push(opts->fifo_trace, ctx);
}

// -----[ ip_opt_ecmp_has_next ]-------------------------------------
int ip_opt_ecmp_has_next(ip_opt_t * opts)
{
  return (fifo_depth(opts->fifo_trace) > 0);
}

// -----[ ip_opt_ecmp_get_next ]-------------------------------------
ip_trace_t ** ip_opt_ecmp_get_next(ip_opt_t * opts)
{
  simulator_t * sim;
  _ecmp_ctx_t * ctx;
  net_msg_t * msg;
  ip_trace_t ** trace_ptr;

  // Process next ECMP path ?
  if (fifo_depth(opts->fifo_trace) <= 0)
    return NULL;

  ctx= fifo_pop(opts->fifo_trace);

  ___ip_opt_debug("process ECMP trace %o\n", ctx->msg->opts);

  msg= ctx->msg;
  while ((msg != NULL) && (msg->protocol == NET_PROTOCOL_IPIP))
    msg= (net_msg_t *) msg->payload;
  trace_ptr= &msg->opts->trace;

  ip_options_add_ref(ctx->msg->opts);
  sim= sim_create(SCHEDULER_STATIC);
  net_error_t error= node_send(ctx->node, ctx->msg, ctx->rtentries, sim);
  if (error != ESUCCESS)
    ___ip_opt_debug("could not send (%s)\n", network_strerror(error));

  sim_run(sim);
  sim_destroy(&sim);
  rt_entries_destroy(&ctx->rtentries);
  FREE(ctx);

  return trace_ptr;
}


/////////////////////////////////////////////////////////////////////
//
// IP OPTIONS SETUP
//
/////////////////////////////////////////////////////////////////////

// -----[ ip_options_init ]----------------------------------------
/**
 * Initialize an IP option data structure with a reference count
 * equal to 0.
 */
void ip_options_init(ip_opt_t * opts)
{
  opts->flags= 0;
  opts->load= 0;
  opts->ref_cnt= 0;
  opts->trace= NULL;
  opts->fifo_trace= NULL;
}

// -----[ ip_options_create ]------------------------------------------
/**
 * Create an IP option data structure.
 */
ip_opt_t * ip_options_create(void)
{
  ip_opt_t * opts= MALLOC(sizeof(ip_opt_t));
  ip_options_init(opts);
  opts->ref_cnt= 1;
  return opts;
}

// -----[ ip_options_destroy ]---------------------------------------
void ip_options_destroy(ip_opt_t ** opts_ref)
{
  ip_opt_t * opts= *opts_ref;

  if (opts != NULL) {
    assert(opts->ref_cnt > 0);
    opts->ref_cnt--;

    if (opts->ref_cnt > 0)
      return;

    FREE(opts);
    *opts_ref= NULL;
  }
}

// -----[ ip_options_copy ]-----------------------------------------
ip_opt_t * ip_options_copy(ip_opt_t * opts)
{
  ip_opt_t * new_opts= MALLOC(sizeof(ip_opt_t));
  new_opts->flags= opts->flags;
  new_opts->alt_dest= opts->alt_dest;
  new_opts->load= opts->load;
  new_opts->ref_cnt= 1;
  if (opts->trace != NULL)
    new_opts->trace= ip_trace_copy(opts->trace);
  else
    new_opts->trace= NULL;
  new_opts->fifo_trace= opts->fifo_trace;
  return new_opts;
}

// -----[ ip_options_add_ref ]---------------------------------------
void ip_options_add_ref(ip_opt_t * opts)
{
  assert(opts->ref_cnt < UINT_MAX);
  opts->ref_cnt++;
}

// -----[ ip_options_load ]------------------------------------------
void ip_options_load(ip_opt_t * opts, net_link_load_t load)
{
  opts->flags|= IP_OPT_LOAD;
  opts->load= load;
}

// -----[ ip_options_set ]-------------------------------------------
void ip_options_set(ip_opt_t * opts, uint16_t flag)
{
  switch (flag) {
  case IP_OPT_CAPACITY:
  case IP_OPT_DEFLECTION:
  case IP_OPT_DELAY:
  case IP_OPT_ECMP:
  case IP_OPT_TUNNEL:
  case IP_OPT_QUICK_LOOP:
  case IP_OPT_WEIGHT:
    opts->flags|= flag;
    break;
  default:
    cbgp_fatal("cannot set this option (%u) with ip_options_set()", flag);
  }
}

// -----[ ip_options_alt_dest ]--------------------------------------
void ip_options_alt_dest(ip_opt_t * opts, ip_pfx_t alt_dest)
{
  opts->flags|= IP_OPT_ALT_DEST;
  opts->alt_dest= alt_dest;
}

// -----[ _fifo_trace_destroy ]--------------------------------------
static void _fifo_trace_destroy(void ** item)
{
  // TODO: free stored ECMP trace context
}

// -----[ ip_options_trace ]-----------------------------------------
void ip_options_trace(ip_opt_t * opts)
{
  opts->flags|= IP_OPT_TRACE;
  opts->trace= ip_trace_create();
  opts->fifo_trace= fifo_create(MAX_TRACE_FIFO_DEPTH, _fifo_trace_destroy);
}
