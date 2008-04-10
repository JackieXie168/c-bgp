// ==================================================================
// @(#)network.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 4/07/2003
// $Id: network.c,v 1.56 2008-04-10 11:27:00 bqu Exp $
// ==================================================================

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <string.h>

#include <libgds/fifo.h>
#include <libgds/log.h>
#include <libgds/memory.h>
#include <libgds/patricia-tree.h>
#include <libgds/stack.h>
#include <libgds/str_util.h>

#include <net/error.h>
#include <net/net_types.h>
#include <net/prefix.h>
#include <net/icmp.h>
#include <net/ipip.h>
#include <net/link-list.h>
#include <net/network.h>
#include <net/net_path.h>
#include <net/node.h>
#include <net/ospf.h>
#include <net/ospf_rt.h>
#include <net/subnet.h>
#include <bgp/message.h>
#include <bgp/as_t.h>
#include <bgp/rib.h>
#include <ui/output.h>

static network_t  * _default_network= NULL;
static simulator_t * _thread_sim= NULL;

/////////////////////////////////////////////////////////////////////
//
// NETWORK PHYSICAL MESSAGE PROPAGATION FUNCTIONS
//
/////////////////////////////////////////////////////////////////////

// -----[ _thread_set_simulator ]------------------------------------
/**
 * Set the current thread's simulator context. This function
 * currently supports a single thread.
 */
static inline void _thread_set_simulator(simulator_t * sim)
{
  _thread_sim= sim;
}

// -----[ thread_set_simulator ]-------------------------------------
void thread_set_simulator(simulator_t * sim)
{
  _thread_set_simulator(sim);
}

// -----[ _thread_get_simulator ]------------------------------------
/**
 * Return the current thread's simulator context. This function
 * currently supports a single thread.
 */
static inline simulator_t * _thread_get_simulator()
{
  assert(_thread_sim != NULL);
  return _thread_sim;
}

// -----[ _network_send_callback ]-------------------------------------
/**
 * This function is used to receive messages "from the wire". The
 * function handles an event from the simulator. When such an event is
 * received, the event's message is delivered to the event's network
 * interface.
 */
static int _network_send_callback(simulator_t * sim,
				  void * ctx)
{
  net_send_ctx_t * send_ctx= (net_send_ctx_t *) ctx;
  net_error_t error;

  // Deliver message to the destination interface
  _thread_set_simulator(sim);
  assert(send_ctx->dst_iface != NULL);
  error= net_iface_recv(send_ctx->dst_iface, send_ctx->msg);
  _thread_set_simulator(NULL);

  // Free the message context. The message MUST be freed by
  // node_recv_msg() if the message has been delivered or in case
  // the message cannot be forwarded.
  FREE(ctx);

  return (error == ESUCCESS)?0:-1;
}

// -----[ _network_send_ctx_dump ]-----------------------------------
/**
 * Callback function used to dump the content of a message event. See
 * also 'simulator_dump_events' (sim/simulator.c).
 */
static void _network_send_ctx_dump(SLogStream * pStream, void * ctx)
{
  net_send_ctx_t * send_ctx= (net_send_ctx_t *) ctx;

  log_printf(pStream, "net-msg n-h:");
  ip_address_dump(pStream, send_ctx->dst_iface->src_node->tAddr);
  log_printf(pStream, " [");
  message_dump(pStream, send_ctx->msg);
  log_printf(pStream, "]");

  switch (send_ctx->msg->protocol) {
  case NET_PROTOCOL_BGP:
    bgp_msg_dump(pStream, NULL, send_ctx->msg->payload);
    break;
  default:
    log_printf(pStream, "opaque");
  }
}

// -----[ _network_send_ctx_create ]---------------------------------
/**
 * This function creates a message context to be pushed on the
 * simulator's events queue. The context contains the message and the
 * destination interface.
 */
static inline net_send_ctx_t *
_network_send_ctx_create(net_iface_t * dst_iface, net_msg_t * msg)
{
  net_send_ctx_t * send_ctx=
    (net_send_ctx_t *) MALLOC(sizeof(net_send_ctx_t));
  
  send_ctx->dst_iface= dst_iface;
  send_ctx->msg= msg;
  return send_ctx;
}

// -----[ _network_send_ctx_destroy ]--------------------------------
/**
 * This function is used by the simulator to free events which will
 * not be processed.
 */
static void _network_send_ctx_destroy(void * ctx)
{
  net_send_ctx_t * send_ctx= (net_send_ctx_t *) ctx;
  message_destroy(&send_ctx->msg);
  FREE(send_ctx);
}

static sim_event_ops_t _network_send_ops= {
  .callback= _network_send_callback,
  .destroy = _network_send_ctx_destroy,
  .dump    = _network_send_ctx_dump,
};

// -----[ network_drop ]----------------------------------------------
/**
 * This function is used to drop a message. The function is used when
 * a message is sent to an interface that is disabled or not
 * connected.
 *
 * The function can write an optional error message on the standard
 * output stream.
 */
void network_drop(net_msg_t * msg)
{
  /*
    LOG_ERR_ENABLED(LOG_LEVEL_SEVERE) {
    log_printf(pLogErr, "*** \033[31;1mMESSAGE DROPPED\033[0m ***\n");
    log_printf(pLogErr, "message: ");
    message_dump(pLogErr, msg);
    log_printf(pLogErr, "\n");
    }
  */
  message_destroy(&msg);
}

// -----[ network_send ]---------------------------------------------
/**
 * This function is used to send a message "on the wire" between two
 * interfaces or between an interface and a subnet. The function
 * pushes the message to the simulator's scheduler.
 */
void network_send(net_iface_t * dst_iface, net_msg_t * msg)
{
  assert(!sim_post_event(_thread_get_simulator(),
			 &_network_send_ops,
			 _network_send_ctx_create(dst_iface, msg),
			 0, SIM_TIME_REL));
}

// -----[ network_get_simulator ]------------------------------------
simulator_t * network_get_simulator(network_t * network)
{
  if (network->sim == NULL)
    network->sim= sim_create(SCHEDULER_STATIC);
  return network->sim;
}


/////////////////////////////////////////////////////////////////////
//
// NETWORK TOPOLOGY MANAGEMENT FUNCTIONS
//
/////////////////////////////////////////////////////////////////////

// -----[ node_add_tunnel ]------------------------------------------
/**
 * Add a tunnel interface on this node.
 *
 * Arguments:
 *   node
 *   tunnel remote end-point
 *   tunnel identifier (local interface address)
 *   outgoing interface (optional)
 *   source address (optional)
 */
net_error_t node_add_tunnel(net_node_t * node,
			    net_addr_t dst_point,
			    net_addr_t addr,
			    net_iface_id_t * oif_id,
			    net_addr_t src_addr)
{
  net_iface_t * oif= NULL;
  net_iface_t * tunnel;
  net_error_t error;

  // If an outgoing interface is specified, check that it exists
  if (oif_id != NULL) {
    oif= node_find_iface(node, *oif_id);
    if (oif == NULL)
      return ENET_IFACE_UNKNOWN;
  }

  // Create tunnel
  error= ipip_link_create(node, dst_point, addr, oif, src_addr, &tunnel);
  if (error != ESUCCESS)
    return error;

  // Add interface (interface is destroyed by node_add_iface2()
  // in case of error)
  return node_add_iface2(node, tunnel);
}

// -----[ node_del_tunnel ]------------------------------------------
/**
 *
 */
int node_del_tunnel(net_node_t * node, net_addr_t addr)
{
  return EUNSUPPORTED;
}

// -----[ node_ipip_enable ]-----------------------------------------
/**
 *
 */
int node_ipip_enable(net_node_t * node)
{
  return node_register_protocol(node, NET_PROTOCOL_IPIP,
				node, NULL,
				ipip_event_handler);
}

// ----- node_igp_domain_add -------------------------------------------
int node_igp_domain_add(net_node_t * pNode, uint16_t uDomainNumber)
{
  return _array_add((SArray*)(pNode->pIGPDomains), &uDomainNumber);
}

// ----- node_belongs_to_igp_domain ---------------------------------
/**
 * Test if a node belongs to a given IGP domain.
 *
 * Return value:
 *   TRUE (1) if node belongs to the given IGP domain
 *   FALSE (0) otherwise.
 */
int node_belongs_to_igp_domain(net_node_t * node, uint16_t uDomainNumber)
{
  unsigned int index;

  if (_array_sorted_find_index((SArray*)(node->pIGPDomains),
			       &uDomainNumber, &index) == 0)
    return 1;
  return 0;
}

// ----- node_links_for_each ----------------------------------------
int node_links_for_each(net_node_t * node, FArrayForEach for_each,
			void * ctx)
{
  return _array_for_each((SArray *) node->ifaces, for_each, ctx);
}

// ----- node_rt_lookup ---------------------------------------------
/**
 * This function looks for the next-hop that must be used to reach a
 * destination address. The function looks up the static/IGP/BGP
 * routing table.
 */
const rt_entry_t * node_rt_lookup(net_node_t * node, net_addr_t dst_addr)
{
  rt_entry_t * rtentry= NULL;
  rt_info_t * rtinfo;

  if (node->rt != NULL) {
    rtinfo= rt_find_best(node->rt, dst_addr, NET_ROUTE_ANY);
    if (rtinfo != NULL)
      rtentry= &rtinfo->next_hop;
  }
  return rtentry;
}


/////////////////////////////////////////////////////////////////////
//
// IP MESSAGE HANDLING
//
/////////////////////////////////////////////////////////////////////

// -----[ _node_error_dump ]-----------------------------------------
static inline void _node_error_dump(net_node_t * node, net_error_t error)
{
  SLogStream * syslog= node_syslog(node);

  log_printf(syslog, "@");
  ip_address_dump(syslog, node->tAddr);
  log_printf(syslog, ": ");
  network_perror(syslog, error);
  log_printf(syslog, "\n");
}

// -----[ _node_ip_fwd_error ]---------------------------------------
/**
 * This function is responsible for handling errors that occur during
 * the procedding of IP messages. An error message is displayed
 * (delegated to _node_error_dump()).
 *
 * If the IP message that caused the error is not ICMP and if an ICMP
 * error message code is provided, an ICMP error message is sent to
 * the source.
 */
static inline net_error_t
_node_ip_fwd_error(net_node_t * node,
		   net_msg_t * msg,
		   net_error_t error,
		   uint8_t uICMPError)
{
  _node_error_dump(node, error);
  if ((uICMPError != 0) && !is_icmp_error(msg))
    icmp_send_error(node, NET_ADDR_ANY, msg->src_addr,
		    uICMPError, _thread_get_simulator());
  message_destroy(&msg);
  return error;
}

// -----[ _node_ip_process_msg ]-------------------------------------
/**
 * This function is responsible for handling messages that are
 * addressed locally.
 *
 * If there is no protocol handler for the message, an ICMP error
 * with type "protocol-unreachable" is sent to the source.
 */
static inline net_error_t
_node_ip_process_msg(net_node_t * node, net_msg_t * msg)
{
  net_protocol_t * proto;
  net_error_t error;

  // Find protocol (based on message field)
  proto= protocols_get(node->protocols, msg->protocol);
  if (proto == NULL)
    return _node_ip_fwd_error(node, msg,
			      ENET_PROTO_UNREACH,
			      ICMP_ERROR_PROTO_UNREACH);

  // Call protocol handler
  error= protocol_recv(proto, msg, _thread_get_simulator());

  // Free only the message. Dealing with the payload was the
  // responsability of the protocol handler.
  msg->payload= NULL;
  message_destroy(&msg);
  return error;
}

// -----[ _node_ip_output ]------------------------------------------
/**
 * This function forwards a message through the given output
 * interface.
 */
static inline net_error_t
_node_ip_output(net_node_t * node, const rt_entry_t * rtentry,
		net_msg_t * msg)
{
  net_addr_t l2_addr= rtentry->gateway;

  // Note: we don't support recursive routing table lookups in C-BGP
  //       resolving the real outgoing interface is done by the
  //       routing protocols themselves (e.g. BGP)

  // Fix source address (if not already set)
  if (msg->src_addr == NET_ADDR_ANY)
    msg->src_addr= net_iface_src_address(rtentry->oif);

  if (rtentry->oif->type == NET_IFACE_PTMP) {
    // For point-to-multipoint interfaces, need to get identifier of
    // next-hop (role played by ARP in the case of Ethernet)
    // In C-BGP, the layer-2 address is equal to the destination's
    // interface IP address. Note: this will not work with unnumbered
    // interfaces !
    if (l2_addr == NET_ADDR_ANY)
      l2_addr= msg->dst_addr;
    // should be written as
    //   l2_addr= net_iface_map_l2(msg->tDstAddr);
  }

  // Process ICMP options
  if (msg->protocol == NET_PROTOCOL_ICMP)
    icmp_process_options(1, node, rtentry->oif, msg, NULL);

  // Forward along this link...
  return net_iface_send(rtentry->oif, l2_addr, msg);
}

// -----[ node_recv_msg ]--------------------------------------------
/**
 * This function handles a message received at this node. If the node
 * is the destination, the message is delivered locally. Otherwize,
 * the function looks up if it has a route towards the destination
 * and, if so, the function forwards the message to the next-hop.
 *
 * Important: this function (or any of its delegates) is responsible
 * for freeing the message in case it is delivered or in case it can
 * not be forwarded.
 *
 * Note: the function also decreases the TTL of the message. If the
 * message is not delivered locally and if the TTL is 0, the message
 * is discarded.
 */
net_error_t node_recv_msg(net_node_t * node,
			  net_iface_t * iif,
			  net_msg_t * msg)
{
  const rt_entry_t * rtentry= NULL;
  net_error_t error;

  // Incoming interface must be fixed
  assert(iif != NULL);

  // A node should never receive an IP datagram with a TTL of 0
  assert(msg->ttl > 0);

  // Process ICMP options
  if (msg->protocol == NET_PROTOCOL_ICMP)
    if ((error= icmp_process_options(ICMP_OPT_STATE_INCOMING, node,
				     iif, msg, &rtentry)) != ESUCCESS)
      return _node_ip_fwd_error(node, msg, error, 0);

  /********************
   * Local delivery ? *
   ********************/

  // Check list of interface addresses to see if the packet
  // is for this node.
  if (node_has_address(node, msg->dst_addr))
    return _node_ip_process_msg(node, msg);

  /**********************
   * IP Forwarding part *
   **********************/

  // Decrement TTL (if TTL is less than or equal to 1,
  // discard and raise ICMP time exceeded message)
  if (msg->ttl <= 1)
    return _node_ip_fwd_error(node, msg,
			      ENET_TIME_EXCEEDED,
			      ICMP_ERROR_TIME_EXCEEDED);
  msg->ttl--;

  // Find route to destination (if no route is found,
  // discard and raise ICMP host unreachable message)
  if (rtentry == NULL) {
    rtentry= node_rt_lookup(node, msg->dst_addr);
    if (rtentry == NULL)
      return _node_ip_fwd_error(node, msg,
				ENET_HOST_UNREACH,
				ICMP_ERROR_HOST_UNREACH);
  }

  // Check that the outgoing interface is different from the
  // incoming interface. Anormal router should send an ICMP Redirect
  // if it is the first hop. We don't handle this case in C-BGP.
  if (iif == rtentry->oif)
    return _node_ip_fwd_error(node, msg, EUNEXPECTED, 0);

  return _node_ip_output(node, rtentry, msg);
}

// -----[ node_send_msg ]--------------------------------------------
/**
 * Send a message to a node.
 * The node must be directly connected to the sender node or there
 * must be a route towards the node in the sender's routing table.
 */
net_error_t node_send_msg(net_node_t * node,
			  net_addr_t src_addr,
			  net_addr_t dst_addr,
			  net_protocol_id_t proto,
			  uint8_t ttl,
			  void * payload,
			  FPayLoadDestroy f_destroy,
			  simulator_t * sim)
{
  const rt_entry_t * rtentry= NULL;
  net_msg_t * msg;
  net_error_t error;

  if (sim != NULL)
    _thread_set_simulator(sim);

  if (ttl == 0)
    ttl= 255;

  // Build "IP" message
  msg= message_create(src_addr, dst_addr, proto, ttl, payload, f_destroy);

  // Process ICMP options
  if (msg->protocol == NET_PROTOCOL_ICMP)
    if ((error= icmp_process_options(ICMP_OPT_STATE_INCOMING, node,
				     NULL, msg, &rtentry)) != ESUCCESS)
      return _node_ip_fwd_error(node, msg, error, 0);

  // Local delivery ?
  if (node_has_address(node, msg->dst_addr)) {
    // Set source address as the node's loopback address (if unspecified)
    if (msg->src_addr == NET_ADDR_ANY)
      msg->src_addr= node->tAddr;
    return _node_ip_process_msg(node, msg);
  }
  
  // Find outgoing interface & next-hop
  if (rtentry == NULL) {
    rtentry= node_rt_lookup(node, dst_addr);
    if (rtentry == NULL)
      return _node_ip_fwd_error(node, msg, ENET_HOST_UNREACH, 0);
  }

  return _node_ip_output(node, rtentry, msg);
}


/////////////////////////////////////////////////////////////////////
//
// NETWORK METHODS
//
/////////////////////////////////////////////////////////////////////

// ----- network_nodes_destroy --------------------------------------
void network_nodes_destroy(void ** ppItem)
{
  node_destroy((net_node_t **) ppItem);
}

// ----- network_create ---------------------------------------------
/**
 *
 */
network_t * network_create()
{
  network_t * network= (network_t *) MALLOC(sizeof(network_t));
  
  network->nodes= trie_create(network_nodes_destroy);
  network->subnets= subnets_create();
  network->sim= NULL;
  return network;
}

// ----- network_destroy --------------------------------------------
/**
 *
 */
void network_destroy(network_t ** network_ref)
{
  if (*network_ref != NULL) {
    trie_destroy(&(*network_ref)->nodes);
    subnets_destroy(&(*network_ref)->subnets);
    if ((*network_ref)->sim != NULL)
      sim_destroy(&(*network_ref)->sim);
    FREE(*network_ref);
    *network_ref= NULL;
  }
}

// -----[ network_get_default ]--------------------------------------
/**
 * Get the default network.
 */
network_t * network_get_default()
{
  assert(_default_network != NULL);
  return _default_network;
}

// ----- network_add_node -------------------------------------------
/**
 * Add a node to the given network.
 */
net_error_t network_add_node(network_t * network, net_node_t * node)
{
   // Check that node does not already exist
  if (network_find_node(network, node->tAddr) != NULL)
    return ENET_NODE_DUPLICATE;

  node->network= network;
  if (trie_insert(network->nodes, node->tAddr, 32, node) != 0)
    return EUNEXPECTED;
  return ESUCCESS;
}

// ----- network_add_subnet -------------------------------------------
/**
 * Add a subnet to the given network.
 */
net_error_t network_add_subnet(network_t * network, net_subnet_t * subnet)
{
  // Check that subnet does not already exist
  if (network_find_subnet(network, subnet->sPrefix) != NULL)
    return ENET_SUBNET_DUPLICATE;

  if (subnets_add(network->subnets, subnet) < 0)
    return EUNEXPECTED;
  return ESUCCESS;
}

// ----- network_find_node ------------------------------------------
/**
 *
 */
net_node_t * network_find_node(network_t * network, net_addr_t tAddr)
{
  return (net_node_t *) trie_find_exact(network->nodes, tAddr, 32);
}

// ----- network_find_subnet ----------------------------------------
/**
 *
 */
net_subnet_t * network_find_subnet(network_t * network, SPrefix sPrefix)
{ 
  return subnets_find(network->subnets, sPrefix);
}

// ----- _network_nodes_to_file -------------------------------------
/*
static int _network_nodes_to_file(uint32_t uKey, uint8_t uKeyLen,
				  void * pItem, void * pContext)
{
  net_node_t * pNode= (net_node_t *) pItem;
  SLogStream * pStream= (SLogStream *) pContext;
  net_iface_t * pLink;
  int iLinkIndex;

  for (iLinkIndex= 0; iLinkIndex < ptr_array_length(pNode->ifaces);
       iLinkIndex++) {
    pLink= (net_iface_t *) pNode->ifaces->data[iLinkIndex];
    link_dump(pStream, pLink);
    log_printf(pStream, "\n");
  }
  return 0;
}
*/

// ----- network_to_file --------------------------------------------
/**
 *
 */
int network_to_file(SLogStream * pStream, network_t * network)
{
  enum_t * pEnum= trie_get_enum(network->nodes);
  net_node_t * node;
  /*
  return trie_for_each(pNetwork->nodes, _network_nodes_to_file,
		       pStream);
  */
  while (enum_has_next(pEnum)) {
    node= *(net_node_t **) enum_get_next(pEnum);
    node_dump(pStream, node);
    log_printf(pStream, "\n");
  }
  enum_destroy(&pEnum);
  return 0;
}

typedef struct {
  net_addr_t tAddr;
  SNetPath * pPath;
  net_link_delay_t tDelay;
} SContext;

//---- network_dump_subnets ---------------------------------------------
void network_dump_subnets(SLogStream * pStream, network_t * network)
{
  //  int iIndex, /*totSub*/;
  net_subnet_t * subnet = NULL;
  enum_t * pEnum= _array_get_enum((SArray*) network->subnets);
  while (enum_has_next(pEnum)) {
    subnet= *(net_subnet_t **) enum_get_next(pEnum);
    ip_prefix_dump(pStream, subnet->sPrefix);
    log_printf(pStream, "\n");
  }
  enum_destroy(&pEnum);
}

// -----[ network_ifaces_load_clear ]----------------------------------
/**
 * Clear the load of all links in the topology.
 */
void network_ifaces_load_clear(network_t * network)
{
  enum_t * pEnum= NULL;
  net_node_t * node;
  
  pEnum= trie_get_enum(network->nodes);
  while (enum_has_next(pEnum)) {
    node= *((net_node_t **) enum_get_next(pEnum));
    node_ifaces_load_clear(node);
  }
  enum_destroy(&pEnum);
}

// ----- network_links_save -----------------------------------------
/**
 * Save the load of all links in the topology.
 */
int network_links_save(SLogStream * pStream)
{
  enum_t * pEnum= NULL;
  net_node_t * node;

  pEnum= trie_get_enum(_default_network->nodes);
  while (enum_has_next(pEnum)) {
    node= *((net_node_t **) enum_get_next(pEnum));
    node_links_save(pStream, node);
  }
  enum_destroy(&pEnum);
  return 0;
}


/////////////////////////////////////////////////////////////////////
//
// INITIALIZATION AND FINALIZATION SECTION
//
/////////////////////////////////////////////////////////////////////

// ----- _network_create --------------------------------------------
void _network_create()
{
  _default_network= network_create();
}

// ----- _network_destroy -------------------------------------------
/**
 *
 */
void _network_destroy()
{
  if (_default_network != NULL)
    network_destroy(&_default_network);
}

