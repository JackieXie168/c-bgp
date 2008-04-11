// ==================================================================
// @(#)message.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @author Sebastien Tandel (standel@info.ucl.ac.be)
// @date 19/05/2003
// $Id: message.c,v 1.24 2008-04-11 11:03:06 bqu Exp $
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <string.h>
#include <time.h>

#include <libgds/log.h>
#include <libgds/memory.h>
#include <libgds/log.h>

#include <bgp/as.h>
#include <bgp/message.h>
#include <net/network.h>
#include <net/protocol.h>
#include <sim/simulator.h>

typedef struct {
  uint16_t    uRemoteAS;
  bgp_msg_t * msg;
} bgp_msg_send_ctx_t;

static char * bgp_msg_names[BGP_MSG_TYPE_MAX]= {
  "A",
  "W",
  "CLOSE",
  "OPEN",
};

static SLogStream * pMonitor= NULL;

// ----- bgp_msg_update_create --------------------------------------
/**
 *
 */
bgp_msg_t * bgp_msg_update_create(uint16_t uPeerAS,
				  bgp_route_t * pRoute)
{
  bgp_msg_update_t * msg=
    (bgp_msg_update_t *) MALLOC(sizeof(bgp_msg_update_t));
  msg->header.type= BGP_MSG_TYPE_UPDATE;
  msg->header.uPeerAS= uPeerAS;
  msg->pRoute= pRoute;
  return (bgp_msg_t *) msg;
}

// ----- bgp_msg_withdraw_create ------------------------------------
/**
 *
 */
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
bgp_msg_t * bgp_msg_withdraw_create(uint16_t uPeerAS,
				    SPrefix sPrefix,
				    net_addr_t * tNextHop)
#else
bgp_msg_t * bgp_msg_withdraw_create(uint16_t uPeerAS,
				    SPrefix sPrefix)
#endif
{
  bgp_msg_withdraw_t * msg=
    (bgp_msg_withdraw_t *) MALLOC(sizeof(bgp_msg_withdraw_t));
  msg->header.type= BGP_MSG_TYPE_WITHDRAW;
  msg->header.uPeerAS= uPeerAS;
  memcpy(&(msg->sPrefix), &sPrefix, sizeof(SPrefix));
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  //It corresponds to the path identifier described in the walton draft 
  //... nevertheless, we keep the variable name NextHop!
//  LOG_DEBUG("creation of withdraw : ");
//  LOG_ENABLED_DEBUG() ip_address_dump(log_get_stream(pMainLog), tNextHop);
//  LOG_DEBUG("\n");
  if (tNextHop != NULL) {
    msg->tNextHop = MALLOC(sizeof(net_addr_t));
    memcpy(msg->tNextHop, tNextHop, sizeof(net_addr_t));
  } else {
    msg->tNextHop = NULL;
  }
#endif
  return (bgp_msg_t *) msg;
}

// ----- bgp_msg_close_create ---------------------------------------
bgp_msg_t * bgp_msg_close_create(uint16_t uPeerAS)
{
  bgp_msg_close_t * msg=
    (bgp_msg_close_t *) MALLOC(sizeof(bgp_msg_close_t));
  msg->header.type= BGP_MSG_TYPE_CLOSE;
  msg->header.uPeerAS= uPeerAS;
  return (bgp_msg_t *) msg;
}

// ----- bgp_msg_open_create ----------------------------------------
bgp_msg_t * bgp_msg_open_create(uint16_t uPeerAS, net_addr_t router_id)
{
  bgp_msg_open_t * msg=
    (bgp_msg_open_t *) MALLOC(sizeof(bgp_msg_open_t));
  msg->header.type= BGP_MSG_TYPE_OPEN;
  msg->header.uPeerAS= uPeerAS;
  msg->router_id= router_id;
  return (bgp_msg_t *) msg;
}

// ----- bgp_msg_destroy --------------------------------------------
/**
 *
 */
void bgp_msg_destroy(bgp_msg_t ** msg_ref)
{
  if (*msg_ref != NULL) {
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
    if (((*msg_ref)->type == BGP_MSG_TYPE_WITHDRAW) &&
	((bgp_msg_withdraw_t *)(*msg_ref))->tNextHop != NULL)
      FREE( ((SBGPMsgWithdraw *)(*msg_ref))->tNextHop );
#endif
    FREE(*msg_ref);
    *msg_ref= NULL;
  }
}

// ----- bgp_msg_send -----------------------------------------------
/**
 *
 */
int bgp_msg_send(net_node_t * node, net_addr_t src_addr,
		 net_addr_t dst_addr, bgp_msg_t * msg)
{
  bgp_msg_monitor_write(msg, node, dst_addr);

  //fprintf(stdout, "(");
  //ip_address_dump(stdout, node->addr);
  //fprintf(stdout, ") bgp-msg-send from ");
  //ip_address_dump(stdout, node->addr);
  //fprintf(stdout, " to ");
  //ip_address_dump(stdout, dst_addr);
  //fprintf(stdout, "\n");
  
  return node_send_msg(node, src_addr, dst_addr, NET_PROTOCOL_BGP, 255,
		       msg, (FPayLoadDestroy) bgp_msg_destroy,
		       network_get_simulator(node->network));
}

// -----[ _bgp_msg_header_dump ]-------------------------------------
static inline void _bgp_msg_header_dump(SLogStream * stream,
					net_node_t * node,
					bgp_msg_t * msg)
{
  log_printf(stream, "|%s", bgp_msg_names[msg->type]);
  // Peer IP
  log_printf(stream, "|");
  if (node != NULL) {
    ip_address_dump(stream, node->addr);
  } else {
    log_printf(stream, "?");
  }
  // Peer AS
  log_printf(stream, "|%d", msg->uPeerAS);
}

// -----[ _bgp_msg_update_dump ]-------------------------------------
static inline void _bgp_msg_update_dump(SLogStream * stream,
					bgp_msg_update_t * msg)
{
  unsigned int index;
  uint32_t uCommunity;
  bgp_route_t * route= msg->pRoute;

  // Prefix
  log_printf(stream, "|");
  ip_prefix_dump(stream, route->sPrefix);
  // AS-PATH
  log_printf(stream, "|");
  path_dump(stream, route_get_path(route), 1);
  // ORIGIN
  log_printf(stream, "|");
  switch (route->pAttr->tOrigin) {
  case ROUTE_ORIGIN_IGP:
    log_printf(stream, "IGP"); break;
  case ROUTE_ORIGIN_EGP:
    log_printf(stream, "EGP"); break;
  case ROUTE_ORIGIN_INCOMPLETE:
    log_printf(stream, "INCOMPLETE"); break;
  default:
    log_printf(stream, "?");
  }
  // NEXT-HOP
  log_printf(stream, "|");
  ip_address_dump(stream, route->pAttr->tNextHop);
  // LOCAL-PREF
  log_printf(stream, "|%u", route->pAttr->uLocalPref);
  // MULTI-EXIT-DISCRIMINATOR
  if (route->pAttr->uMED == ROUTE_MED_MISSING)
    log_printf(stream, "|");
  else
    log_printf(stream, "|%u", route->pAttr->uMED);
  // COMMUNITY
  log_printf(stream, "|");
  if (route->pAttr->pCommunities != NULL) {
    for (index= 0; index < route->pAttr->pCommunities->uNum; index++) {
      uCommunity= (uint32_t) route->pAttr->pCommunities->asComms[index];
      log_printf(stream, "%u ", uCommunity);
    }
  }
  
  // Route-reflectors: Originator
  if (route->pAttr->pOriginator != NULL) {
    log_printf(stream, "originator:");
    ip_address_dump(stream, *route->pAttr->pOriginator);
  }
  log_printf(stream, "|");
  
  if (route->pAttr->pClusterList != NULL) {
    log_printf(stream, "cluster_id_list:");
    cluster_list_dump(stream, route->pAttr->pClusterList);
  }
  log_printf(stream, "|");
}

// -----[ _bgp_msg_withdraw_dump ]-----------------------------------
static inline void _bgp_msg_withdraw_dump(SLogStream * stream,
					  bgp_msg_withdraw_t * msg)
{
  // Prefix
  log_printf(stream, "|");
  ip_prefix_dump(stream, ((SBGPMsgWithdraw *) msg)->sPrefix);
}

// -----[ _bgp_msg_close_dump ]--------------------------------------
static inline void _bgp_msg_close_dump(SLogStream * stream,
				       bgp_msg_close_t * msg)
{
}

// -----[ _bgp_msg_open_dump ]---------------------------------------
static inline void _bgp_msg_open_dump(SLogStream * stream,
				      bgp_msg_open_t * msg)
{
  /* Router ID */
  log_printf(stream, "|");
  ip_address_dump(stream, msg->router_id);
}

// ----- bgp_msg_dump -----------------------------------------------
/**
 *
 */
void bgp_msg_dump(SLogStream * stream,
		  net_node_t * node,
		  bgp_msg_t * msg)
{
  assert(msg->type < BGP_MSG_TYPE_MAX);

  /* Dump header */
  _bgp_msg_header_dump(stream, node, msg);

  /* Dump message content */
  switch (msg->type) {
  case BGP_MSG_TYPE_UPDATE:
    _bgp_msg_update_dump(stream, (bgp_msg_update_t *) msg);
    break;
  case BGP_MSG_TYPE_WITHDRAW:
    _bgp_msg_withdraw_dump(stream, (bgp_msg_withdraw_t *) msg);
    break;
  case BGP_MSG_TYPE_OPEN:
    _bgp_msg_open_dump(stream, (bgp_msg_open_t *) msg);
    break;
  case BGP_MSG_TYPE_CLOSE:
    _bgp_msg_close_dump(stream, (bgp_msg_close_t *) msg);
    break;
  default:
    log_printf(stream, "should never reach this code");
  } 
}

/////////////////////////////////////////////////////////////////////
//
// BGP MESSAGES MONITORING SECTION
//
/////////////////////////////////////////////////////////////////////
//
// This is enabled through the CLI with the following command
// (see cli/bgp.c): bgp options msg-monitor <output-file>

// ----- bgp_msg_monitor_open ---------------------------------------
/**
 *
 */
int bgp_msg_monitor_open(const char * file_name)
{
  time_t tTime= time(NULL);

  bgp_msg_monitor_close();

  // Create new monitor
  pMonitor= log_create_file((char *) file_name);
  if (pMonitor == NULL) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Unable to create monitor file\n");
    return -1;
  }

  // Write monitor file header
  log_set_level(pMonitor, LOG_LEVEL_EVERYTHING);
  log_printf(pMonitor, "# BGP message trace\n");
  log_printf(pMonitor, "# generated by C-BGP on %s",
	  ctime(&tTime));
  log_printf(pMonitor, "# <dest-ip>|BGP4|<event-time>|<type>|"
	  "<peer-ip>|<peer-as>|<prefix>|...\n");
  return 0;
}

// -----[ bgp_msg_monitor_close ]------------------------------------
/**
 *
 */
void bgp_msg_monitor_close()
{
  // If monitor was previously enabled, stop it.
  if (pMonitor != NULL)
    log_destroy(&pMonitor);
}

// ----- bgp_msg_monitor_write --------------------------------------
/**
 * Write the given BGP update/withdraw message in MRTD format, i.e.
 * a BGP update message is written as
 *
 *   BGP4|<time>|A|<peer-ip>|<peer-as>|<prefix>|<as-path>|<origin>|
 *   <next-hop>|<local-pref>|<med>|<communities>
 *
 * and a BGP withdraw message is written as
 *
 *   BGP4|<time>|W|<peer-ip>|<peer-as>|<prefix>
 *
 * In addition to this, each message is prefixed with the
 * destination's IP address
 *
 *   <dest-ip>|
 */
void bgp_msg_monitor_write(bgp_msg_t * msg, net_node_t * node,
			   net_addr_t addr)
{
  if (pMonitor != NULL) {

    // Destination router (): this is not MRTD format but required to
    // identify the destination of the messages
    ip_address_dump(pMonitor, addr);
    // Protocol and Time
    log_printf(pMonitor, "|BGP4|%.2f",
	       sim_get_time(network_get_simulator(node->network)));

    bgp_msg_dump(pMonitor, node, msg);

    log_printf(pMonitor, "\n");
  }
}

// ----- bgp_msg_monitor_destroy ------------------------------------
/**
 *
 */
void bgp_msg_monitor_destroy(SLogStream ** ppStream)
{
  log_destroy(ppStream);
}

/////////////////////////////////////////////////////////////////////
// INITIALIZATION AND FINALIZATION SECTION
/////////////////////////////////////////////////////////////////////

// -----[ _message_destroy ]-----------------------------------------
void _message_destroy()
{
  bgp_msg_monitor_destroy(&pMonitor);
}
