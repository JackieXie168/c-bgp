// ==================================================================
// @(#)net_node.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 15/05/2007
// $Id: net_node.c,v 1.8 2008-04-11 11:03:06 bqu Exp $
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>

#include <libgds/cli.h>
#include <libgds/cli_ctx.h>
#include <libgds/str_util.h>

#include <cli/common.h>
#include <cli/enum.h>
#include <cli/net_node_iface.h>
#include <net/error.h>
#include <net/icmp.h>
#include <net/igp_domain.h>
#include <net/net_types.h>
#include <net/netflow.h>
#include <net/node.h>
#include <net/record-route.h>
#include <net/util.h>


// -----[ _node_from_context ]---------------------------------------
static inline net_node_t * _node_from_context(SCliContext * pContext)
{
  net_node_t * pNode= (net_node_t *) cli_context_get_item_at_top(pContext);
  assert(pNode != NULL);
  return pNode;
}

/////////////////////////////////////////////////////////////////////
//
// CONTEXT CREATION/DESTRUCTION
//
/////////////////////////////////////////////////////////////////////

// ----- cli_ctx_create_net_node ------------------------------------
static int cli_ctx_create_net_node(SCliContext * pContext, void ** ppItem)
{
  char * pcNodeAddr;
  net_node_t * pNode;

  pcNodeAddr= tokens_get_string_at(pContext->pCmd->pParamValues, 0);
  pNode= cli_net_node_by_addr(pcNodeAddr);
  if (pNode == NULL) {
    cli_set_user_error(cli_get(), "unable to find node \"%s\"", pcNodeAddr);
    return CLI_ERROR_CTX_CREATE;
  }
  *ppItem= pNode;
  return CLI_SUCCESS;
}

// ----- cli_ctx_destroy_net_node -----------------------------------
static void cli_ctx_destroy_net_node(void ** ppItem)
{
  // Nothing to do (context is only a reference)
}


/////////////////////////////////////////////////////////////////////
//
// CLI COMMANDS
//
/////////////////////////////////////////////////////////////////////

// ----- cli_net_node_coord -----------------------------------------
/**
 * context: {node}
 * tokens: {latitude,longitude}
 */
static int cli_net_node_coord(SCliContext * pContext, SCliCmd * pCmd)
{
  net_node_t * node= _node_from_context(pContext);
  double latitude, longitude;

  // Get latitude
  if (tokens_get_double_at(pCmd->pParamValues, 0, &latitude)) {
    cli_set_user_error(cli_get(), "invalid latitude \"%s\"",
		       tokens_get_string_at(pCmd->pParamValues, 0));
    return CLI_ERROR_COMMAND_FAILED;
  }
  
  // Get latitude
  if (tokens_get_double_at(pCmd->pParamValues, 1, &longitude)) {
    cli_set_user_error(cli_get(), "invalid longitude \"%s\"",
		       tokens_get_string_at(pCmd->pParamValues, 1));
    return CLI_ERROR_COMMAND_FAILED;
  }

  node->coord.latitude= latitude;
  node->coord.longitude= longitude;

  return CLI_SUCCESS;
}

// ----- cli_net_node_domain ----------------------------------------
/**
 * context: {node}
 * tokens: {id}
 */
static int cli_net_node_domain(SCliContext * pContext, SCliCmd * pCmd)
{
  net_node_t * pNode= _node_from_context(pContext);
  unsigned int uId;
  igp_domain_t * pDomain;

  // Get domain ID
  if (tokens_get_uint_at(pCmd->pParamValues, 0, &uId) ||
      (uId > 65535)) {
    cli_set_user_error(cli_get(), "invalid domain id \"%s\"",
		       tokens_get_string_at(pCmd->pParamValues, 0));
    return CLI_ERROR_COMMAND_FAILED;
  }

  /* Get domain from ID. Check if domain exists... */
  pDomain= get_igp_domain(uId);
  if (pDomain == NULL) {
    cli_set_user_error(cli_get(), "unknown domain \"%d\"", uId);
    return CLI_ERROR_COMMAND_FAILED;    
  }
  
  /* Check if domain already contains this node */
  if (igp_domain_contains_router(pDomain, pNode)) {
    cli_set_user_error(cli_get(), "could not add to domain \"%d\"", uId);
    return CLI_ERROR_COMMAND_FAILED;
  }

  igp_domain_add_router(pDomain, pNode);
  return CLI_SUCCESS;
}

// ----- cli_net_node_ipip_enable -----------------------------------
/**
 * context: {node}
 * tokens: {}
 */
static int cli_net_node_ipip_enable(SCliContext * pContext, SCliCmd * pCmd)
{
  net_node_t * pNode= _node_from_context(pContext);
  int iResult;

  // Enable IP-in-IP
  iResult= node_ipip_enable(pNode);
  if (iResult != ESUCCESS) {
    cli_set_user_error(cli_get(), "could not enable ip-ip (%s)",
		       network_strerror(iResult));
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// ----- cli_net_node_name ------------------------------------------
/**
 * context: {node}
 * tokens: {string}
 */
static int cli_net_node_name(SCliContext * pContext, SCliCmd * pCmd)
{
  net_node_t * pNode= _node_from_context(pContext);

  node_set_name(pNode, tokens_get_string_at(pCmd->pParamValues, 0));

  return CLI_SUCCESS;
}

// ----- cli_net_node_ping ------------------------------------------
/**
 * context: {node}
 * tokens : {addr}
 * options: ttl=TTL
 */
static int cli_net_node_ping(SCliContext * pContext, SCliCmd * pCmd)
{
  net_node_t * pNode= _node_from_context(pContext);
  char * pcValue;
  net_addr_t tDstAddr;
  int iErrorCode;
  unsigned int uValue;
  uint8_t uTTL= 0;

  // Get destination address
  pcValue= tokens_get_string_at(pContext->pCmd->pParamValues, 0);
  if (str2address(pcValue, &tDstAddr) != 0) {
    cli_set_user_error(cli_get(), "invalid address \"%s\"", pcValue);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Optional: TTL ?
  if (cli_options_has_value(pCmd->pOptions, "ttl")) {
    pcValue= cli_options_get_value(pCmd->pOptions, "ttl");
    if (pcValue == NULL) {
      cli_set_user_error(cli_get(), "TTL value must be specified");
      return CLI_ERROR_COMMAND_FAILED;
    }
    if ((str_as_uint(pcValue, &uValue) < 0) || (uValue > 255)) {
      cli_set_user_error(cli_get(), "invalid TTL \"%s\"", pcValue);
      return CLI_ERROR_COMMAND_FAILED;
    }
    uTTL= uValue;
  }

  // Perform ping (note that errors are ignored)
  iErrorCode= icmp_ping(pLogOut, pNode, NET_ADDR_ANY, tDstAddr, uTTL);

  return CLI_SUCCESS;
}

// ----- cli_net_node_recordroute -----------------------------------
/**
 * context: {node}
 * options: [--capacity]
 *          [--check-loop]
 *          [--deflection]
 *          [--delay]
 *          [--load=V]
 *          [--tos]
 *          [--weight]
 * tokens: {prefix|address|*}
 */
static int cli_net_node_recordroute(SCliContext * pContext, SCliCmd * pCmd)
{
  net_node_t * pNode= _node_from_context(pContext);
  char * pcDest;
  SNetDest sDest;
  net_tos_t tTOS= 0;
  uint8_t uOptions= 0;
  char * pcValue;
  unsigned int uValue;
  net_link_load_t tLoad= 0;

  // Optional capacity ?
  if (cli_options_has_value(pCmd->pOptions, "capacity"))
    uOptions|= NET_RECORD_ROUTE_OPTION_CAPACITY;

  // Optional loop-check ?
  if (cli_options_has_value(pCmd->pOptions, "check-loop"))
    uOptions|= NET_RECORD_ROUTE_OPTION_QUICK_LOOP;

  // Optional deflection-check ?
  if (cli_options_has_value(pCmd->pOptions, "deflection"))
    uOptions|= NET_RECORD_ROUTE_OPTION_DEFLECTION;

  // Optional delay ?
  if (cli_options_has_value(pCmd->pOptions, "delay"))
    uOptions|= NET_RECORD_ROUTE_OPTION_DELAY;

  // Load path with value ?
  pcValue= cli_options_get_value(pCmd->pOptions, "load");
  if (pcValue != NULL) {
    if (str_as_uint(pcValue, &uValue) < 0) {
      cli_set_user_error(cli_get(), "invalid load \"%s\"", pcValue);
      return CLI_ERROR_COMMAND_FAILED;
    }
    uOptions|= NET_RECORD_ROUTE_OPTION_LOAD;
    tLoad= uValue;
  }

  // Perform record-route in particular plane (based on TOS) ?
  pcValue= cli_options_get_value(pCmd->pOptions, "tos");
  if (pcValue != NULL)
    if (!str2tos(pcValue, &tTOS)) {
      cli_set_user_error(cli_get(), "invalid TOS \"%s\"", pcValue);
      return CLI_ERROR_COMMAND_FAILED;
    }

  // Optional weight ?
  if (cli_options_has_value(pCmd->pOptions, "weight"))
    uOptions|= NET_RECORD_ROUTE_OPTION_WEIGHT;

  // Get destination address
  pcDest= tokens_get_string_at(pCmd->pParamValues, 0);
  if (ip_string_to_dest(pcDest, &sDest)) {
    cli_set_user_error(cli_get(), "invalid prefix|address|* \"%s\"", pcDest);
    return CLI_ERROR_COMMAND_FAILED;
  }

  /* Check that the destination type is adress/prefix */
  if ((sDest.tType != NET_DEST_ADDRESS) &&
      (sDest.tType != NET_DEST_PREFIX)) {
    cli_set_user_error(cli_get(), "can not use such destination");
    return CLI_ERROR_COMMAND_FAILED;
  }

  node_dump_recorded_route(pLogOut, pNode, sDest, tTOS, uOptions, tLoad);

  return CLI_SUCCESS;
}

// ----- cli_net_node_traceroute ------------------------------------
/**
 * context: {node}
 * tokens : {addr}
 */
static int cli_net_node_traceroute(SCliContext * pContext, SCliCmd * pCmd)
{
  net_node_t * pNode= _node_from_context(pContext);
  char * pcValue;
  net_addr_t tDstAddr;
  int iErrorCode;

  // Get destination address
  pcValue= tokens_get_string_at(pContext->pCmd->pParamValues, 0);
  if (str2address(pcValue, &tDstAddr) != 0) {
    cli_set_user_error(cli_get(), "invalid address \"%s\"", pcValue);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Perform traceroute (note that errors are ignored)
  iErrorCode= icmp_trace_route(pLogOut, pNode, NET_ADDR_ANY, tDstAddr, 0, NULL);

  return CLI_SUCCESS;
}

// ----- cli_net_node_route_add -------------------------------------
/**
 * This function adds a new static route to the given node.
 *
 * context: {node}
 * tokens : {prefix, addr|*, addr|prefix (iface-id), weight}
 * options: --gw=addr
 */
static int cli_net_node_route_add(SCliContext * pContext, SCliCmd * pCmd)
{
  net_node_t * pNode= _node_from_context(pContext);
  SPrefix sPrefix;
  char * pcValue;
  net_addr_t tGateway= NET_ADDR_ANY;
  unsigned long ulWeight;
  net_iface_id_t tOutIfaceID;
  int iResult;

  // Get the route's prefix
  pcValue= tokens_get_string_at(pCmd->pParamValues, 0);
  if (str2prefix(pcValue, &sPrefix) != 0) {
    cli_set_user_error(cli_get(), "invalid prefix \"%s\"", pcValue);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Get the gateway (optional)
  pcValue= cli_options_get_value(pCmd->pOptions, "gw");
  if (pcValue != NULL) {
    if (str2address(pcValue, &tGateway) != 0) {
      cli_set_user_error(cli_get(), "invalid gateway \"%s\"", pcValue);
      return CLI_ERROR_COMMAND_FAILED;
    }
  }

  // Get the outgoing interface identifier (address/prefix)
  pcValue= tokens_get_string_at(pCmd->pParamValues, 1);
  iResult= net_iface_str2id(pcValue, &tOutIfaceID);
  if (iResult != ESUCCESS) {
    cli_set_user_error(cli_get(), "invalid interface \"%s\" (%s)", pcValue,
		       network_strerror(iResult));
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Get the weight
  if (tokens_get_ulong_at(pCmd->pParamValues, 2, &ulWeight)) {
    cli_set_user_error(cli_get(), "invalid weight");
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Add the route
  iResult= node_rt_add_route(pNode, sPrefix, tOutIfaceID, tGateway,
			     ulWeight, NET_ROUTE_STATIC);
  if (iResult != ESUCCESS) {
    cli_set_user_error(cli_get(), "could not add static route (%s)",
		       network_strerror(iResult));
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// ----- cli_net_node_route_del -------------------------------------
/**
 * This function removes a static route from the given node.
 *
 * context: {node}
 * tokens: {prefix, addr|*, addr|prefix|*, iface}
 */
static int cli_net_node_route_del(SCliContext * pContext, SCliCmd * pCmd)
{
  net_node_t * pNode= _node_from_context(pContext);
  char * pcValue;
  SPrefix sPrefix;
  SNetDest sDest;
  net_addr_t tGateway;
  net_iface_id_t tOutIfaceID;
  net_iface_id_t * ptOutIfaceID= &tOutIfaceID;
  int iResult;

  // Get the route's prefix
  pcValue= tokens_get_string_at(pCmd->pParamValues, 0);
  if (str2prefix(pcValue, &sPrefix) != 0) {
    cli_set_user_error(cli_get(), "invalid prefix \"%s\"", pcValue);
    return CLI_ERROR_COMMAND_FAILED;
  }
  
  // Get the gateway
  pcValue= tokens_get_string_at(pCmd->pParamValues, 1);
  if (ip_string_to_dest(pcValue, &sDest) ||
      (sDest.tType == NET_DEST_PREFIX)) {
    cli_set_user_error(cli_get(), "invalid gateway \"%s\"", pcValue);
    return CLI_ERROR_COMMAND_FAILED;
  }
  switch (sDest.tType) {
  case NET_DEST_ADDRESS:
    tGateway= sDest.uDest.tAddr; break;
  case NET_DEST_ANY:
    tGateway= 0; break;
  default:
    abort();
  }

  // Get the outgoing link identifier (address/prefix)
  pcValue= tokens_get_string_at(pCmd->pParamValues, 2);
  if (!strcmp(pcValue, "*")) {
    ptOutIfaceID= NULL;
  } else {
    iResult= net_iface_str2id(pcValue, ptOutIfaceID);
    if (iResult != ESUCCESS) {
      cli_set_user_error(cli_get(), "invalid interface \"%s\" (%s)", pcValue,
			 network_strerror(iResult));
      return CLI_ERROR_COMMAND_FAILED;
    }
  }

  // Remove the route
  if (node_rt_del_route(pNode, &sPrefix, ptOutIfaceID, &tGateway,
			NET_ROUTE_STATIC)) {
    cli_set_user_error(cli_get(), "could not remove static route");
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// ----- cli_net_node_show_ifaces -----------------------------------
/**
 * context: {node}
 * tokens: {}
 */
static int cli_net_node_show_ifaces(SCliContext * pContext, SCliCmd * pCmd)
{
  net_node_t * pNode= _node_from_context(pContext);

  node_ifaces_dump(pLogOut, pNode);

  return CLI_SUCCESS;
}

// ----- cli_net_node_show_info -------------------------------------
/**
 * context: {node}
 * tokens: {}
 */
static int cli_net_node_show_info(SCliContext * pContext, SCliCmd * pCmd)
{
  net_node_t * pNode= _node_from_context(pContext);

  node_info(pLogOut, pNode);

  return CLI_SUCCESS;
}

// ----- cli_net_node_show_links ------------------------------------
/**
 * context: {node}
 * tokens: {}
 */
static int cli_net_node_show_links(SCliContext * pContext, SCliCmd * pCmd)
{
  net_node_t * pNode= _node_from_context(pContext);

  // Dump list of direct links
  node_links_dump(pLogOut, pNode);
  
  return CLI_SUCCESS;
}

// ----- cli_net_node_show_rt ---------------------------------------
/**
 * context: {node}
 * tokens: {prefix|address|*}
 */
static int cli_net_node_show_rt(SCliContext * pContext, SCliCmd * pCmd)
{
  net_node_t * pNode= _node_from_context(pContext);
  char * pcPrefix;
  SNetDest sDest;

  // Get the prefix/address/*
  pcPrefix= tokens_get_string_at(pCmd->pParamValues, 0);
  if (ip_string_to_dest(pcPrefix, &sDest)) {
    cli_set_user_error(cli_get(), "invalid prefix|address|* \"%s\"", pcPrefix);
    return CLI_ERROR_COMMAND_FAILED;
  }

#ifndef OSPF_SUPPORT
  // Dump routing table
  node_rt_dump(pLogOut, pNode, sDest);
#else
  // Dump OSPF routing table
  // Options: OSPF_RT_OPTION_SORT_AREA : dump routing table grouping routes by area
  ospf_node_rt_dump(pLogOut, pNode, OSPF_RT_OPTION_SORT_AREA | OSPF_RT_OPTION_SORT_PATH_TYPE);
#endif
  return CLI_SUCCESS;
}

// ----- cli_net_node_tunnel_add ------------------------------------
/**
 * context: {node}
 * tokens: {end-point, if-addr}
 * option: oif=<addr>, src=<addr>
 */
static int cli_net_node_tunnel_add(SCliContext * pContext, SCliCmd * pCmd)
{
  net_node_t * pNode= _node_from_context(pContext);
  char * pcValue;
  net_addr_t tEndPointAddr;
  net_addr_t tAddr;
  net_iface_id_t tOutIfaceID;
  net_iface_id_t * ptOutIfaceID= &tOutIfaceID;
  net_addr_t tSrcAddr= 0;
  int iResult;

  // Get tunnel end-point (remote)
  pcValue= tokens_get_string_at(pCmd->pParamValues, 0);
  if (str2address(pcValue, &tEndPointAddr) != 0) {
    cli_set_user_error(cli_get(), "invalid end-point address \"%s\"", pcValue);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Get tunnel identifier: address (local)
  pcValue= tokens_get_string_at(pCmd->pParamValues, 1);
  if (str2address(pcValue, &tAddr) != 0) {
    cli_set_user_error(cli_get(), "invalid local address \"%s\"", pcValue);
    return CLI_ERROR_COMMAND_FAILED;    
  }

  // get optional outgoing interface
  pcValue= cli_options_get_value(pCmd->pOptions, "oif");
  if (pcValue != NULL) {
    iResult= net_iface_str2id(pcValue, ptOutIfaceID);
    if (iResult != ESUCCESS) {
      cli_set_user_error(cli_get(), "invalid outgoing interface \"%s\" (%s)",
			 pcValue, network_strerror(iResult));
      return CLI_ERROR_COMMAND_FAILED;
    }
  } else
    ptOutIfaceID= NULL;

  // get optional source address
  pcValue= cli_options_get_value(pCmd->pOptions, "src");
  if (pcValue != NULL) {
    if (str2address(pcValue, &tSrcAddr) < 0) {
      cli_set_user_error(cli_get(), "invalid source address \"%s\"", pcValue);
      return CLI_ERROR_COMMAND_FAILED;
    }
  }

  // Add tunnel
  iResult= node_add_tunnel(pNode, tEndPointAddr, tAddr, ptOutIfaceID, tSrcAddr);
  if (iResult != ESUCCESS) {
    cli_set_user_error(cli_get(), "could not add tunnel (%s)",
		       network_strerror(iResult));
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// -----[ cli_net_node_traffic_load ] -------------------------------
/**
 * context: {node}
 * tokens: {<filename>}
 * option: --summary, --details
 */
static int cli_net_node_traffic_load(SCliContext * pContext, SCliCmd * pCmd)
{
  net_node_t * pNode= _node_from_context(pContext);
  char * pcFileName;
  int iResult;
  uint8_t tOptions= 0;

  // Get option "--summary" ?
  if (cli_options_has_value(pCmd->pOptions, "summary"))
    tOptions|= NET_NODE_NETFLOW_OPTIONS_SUMMARY;

  // Get option "--details" ?
  if (cli_options_has_value(pCmd->pOptions, "details"))
    tOptions|= NET_NODE_NETFLOW_OPTIONS_DETAILS;

  // Load Netflow from file
  pcFileName= tokens_get_string_at(pCmd->pParamValues, 0);
  iResult= node_load_netflow(pNode, pcFileName, tOptions);
  if (iResult != NETFLOW_SUCCESS) {
    cli_set_user_error(cli_get(), "could not load traffic (%s)",
		       netflow_strerror(iResult));
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}


/////////////////////////////////////////////////////////////////////
//
// CLI REGISTRATION
//
/////////////////////////////////////////////////////////////////////

// -----[ cli_register_net_node_add ]--------------------------------
static int cli_register_net_node_add(SCliCmds * pCmds)
{
  SCliCmds * pSubCmds= cli_cmds_create();

  cli_register_net_node_add_iface(pSubCmds);
  return cli_cmds_add(pCmds, cli_cmd_create("add", NULL, pSubCmds, NULL));
}

// ----- cli_register_net_node_traffic ------------------------------
static int cli_register_net_node_traffic(SCliCmds * pCmds)
{
  SCliCmds * pSubCmds;
  SCliParams * pParams;
  SCliCmd * pCmd;

  pSubCmds= cli_cmds_create();
  pParams= cli_params_create();
  cli_params_add_file(pParams, "<filename>", NULL);
  pCmd= cli_cmd_create("load", cli_net_node_traffic_load, NULL, pParams);
  cli_cmd_add_option(pCmd, "details", NULL);
  cli_cmd_add_option(pCmd, "summary", NULL);
  cli_cmds_add(pSubCmds, pCmd);
  return cli_cmds_add(pCmds, cli_cmd_create("traffic", NULL, pSubCmds, NULL));
}

// ----- cli_register_net_node_show ---------------------------------
static int cli_register_net_node_show(SCliCmds * pCmds)
{
  SCliCmds * pSubCmds;
  SCliParams * pParams;

  pSubCmds= cli_cmds_create();
  cli_cmds_add(pSubCmds, cli_cmd_create("ifaces",
					cli_net_node_show_ifaces,
					NULL, NULL));
  cli_cmds_add(pSubCmds, cli_cmd_create("info",
					cli_net_node_show_info,
					NULL, NULL));
  cli_cmds_add(pSubCmds, cli_cmd_create("links",
					cli_net_node_show_links,
					NULL, NULL));
  pParams= cli_params_create();
  cli_params_add(pParams, "<prefix|address|*>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("rt",
					cli_net_node_show_rt,
					NULL, pParams));
  return cli_cmds_add(pCmds, cli_cmd_create("show", NULL,
					    pSubCmds, NULL));
}

// ----- cli_register_net_node_route --------------------------------
static void cli_register_net_node_route(SCliCmds * pCmds)
{
  SCliCmds * pSubCmds= cli_cmds_create();
  SCliCmd * pCmd;
  SCliParams * pParams;

  pParams= cli_params_create();
  cli_params_add(pParams, "<prefix>", NULL);
  cli_params_add(pParams, "<iface>", NULL);
  cli_params_add(pParams, "<weight>", NULL);
  pCmd= cli_cmd_create("add", cli_net_node_route_add, NULL, pParams);
  cli_cmd_add_option(pCmd, "gw", NULL);
  cli_cmds_add(pSubCmds, pCmd);
  pParams= cli_params_create();
  cli_params_add(pParams, "<prefix>", NULL);
  cli_params_add(pParams, "<gateway>", NULL);
  cli_params_add(pParams, "<iface>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("del",
					cli_net_node_route_del,
					NULL, pParams));
  cli_cmds_add(pCmds, cli_cmd_create("route", NULL, pSubCmds, NULL));
}

// ----- cli_register_net_node_tunnel -------------------------------
static int cli_register_net_node_tunnel(SCliCmds * pCmds)
{
  SCliCmds * pSubCmds= cli_cmds_create();
  SCliParams * pParams;
  SCliCmd * pCmd;

  pParams= cli_params_create();
  cli_params_add(pParams, "<end-point>", NULL);
  cli_params_add(pParams, "<addr>", NULL);
  pCmd= cli_cmd_create("add", cli_net_node_tunnel_add, NULL, pParams);
  cli_cmd_add_option(pCmd, "oif", NULL);
  cli_cmd_add_option(pCmd, "src", NULL);
  cli_cmds_add(pSubCmds, pCmd);
  return cli_cmds_add(pCmds, cli_cmd_create("tunnel", NULL,
					   pSubCmds, NULL));
}

// ----- cli_register_net_node --------------------------------------
int cli_register_net_node(SCliCmds * pCmds)
{
  SCliCmds * pSubCmds;
  SCliParams * pParams;
  SCliCmd * pCmd;

  pSubCmds= cli_cmds_create();

  cli_register_net_node_add(pSubCmds);  
  cli_register_net_node_iface(pSubCmds);
  cli_register_net_node_route(pSubCmds);
  pParams= cli_params_create();
  cli_params_add(pParams, "<latitude>", NULL);
  cli_params_add(pParams, "<longitude>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("coord", cli_net_node_coord,
					NULL, pParams));
  pParams= cli_params_create();
  cli_params_add(pParams, "<id>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("domain", cli_net_node_domain,
					NULL, pParams));
  cli_cmds_add(pSubCmds, cli_cmd_create("ipip-enable",
					cli_net_node_ipip_enable,
					NULL, NULL));
  pParams= cli_params_create();
  cli_params_add(pParams, "<name>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("name", cli_net_node_name,
					NULL, pParams));

  pParams= cli_params_create();
  cli_params_add2(pParams, "<addr>", NULL, cli_enum_net_nodes_addr);
  pCmd= cli_cmd_create("ping", cli_net_node_ping, NULL, pParams);
  cli_cmd_add_option(pCmd, "ttl", NULL);
  cli_cmds_add(pSubCmds, pCmd);

  pParams= cli_params_create();
  cli_params_add(pParams, "<address|prefix>", NULL);
  pCmd= cli_cmd_create("record-route",
		       cli_net_node_recordroute,
		       NULL, pParams);
  cli_cmd_add_option(pCmd, "capacity", NULL);
  cli_cmd_add_option(pCmd, "check-loop", NULL);
  cli_cmd_add_option(pCmd, "deflection", NULL);
  cli_cmd_add_option(pCmd, "delay", NULL);
  cli_cmd_add_option(pCmd, "load", NULL);
  cli_cmd_add_option(pCmd, "tos", NULL);
  cli_cmd_add_option(pCmd, "weight", NULL);
  cli_cmds_add(pSubCmds, pCmd);

  pParams= cli_params_create();
  cli_params_add2(pParams, "<addr>", NULL, cli_enum_net_nodes_addr);
  pCmd= cli_cmd_create("traceroute", cli_net_node_traceroute, NULL, pParams);
  cli_cmds_add(pSubCmds, pCmd);

  cli_register_net_node_show(pSubCmds);
  cli_register_net_node_traffic(pSubCmds);
  cli_register_net_node_tunnel(pSubCmds);
#ifdef OSPF_SUPPORT
  cli_register_net_node_ospf(pSubCmds);
#endif
  pParams= cli_params_create();
  cli_params_add2(pParams, "<addr>", NULL, cli_enum_net_nodes_addr);
  return cli_cmds_add(pCmds, cli_cmd_create_ctx("node",
						cli_ctx_create_net_node,
						cli_ctx_destroy_net_node,
						pSubCmds, pParams));
}
