// ==================================================================
// @(#)net.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 15/07/2003
// @lastdate 05/08/2004
// ==================================================================

#include <string.h>

#include <libgds/cli_ctx.h>
#include <cli/common.h>
#include <cli/net.h>
#include <net/prefix.h>
#include <libgds/log.h>
#include <net/icmp.h>
#include <net/igp.h>
#include <net/network.h>
#include <net/net_path.h>
#include <net/routing.h>

// ----- cli_net_node_by_addr ---------------------------------------
/**
 *
 */
SNetNode * cli_net_node_by_addr(char * pcAddr)
{
  net_addr_t tAddr;
  char * pcEndPtr;

  if (ip_string_to_address(pcAddr, &pcEndPtr, &tAddr) || (*pcEndPtr != 0))
    return NULL;
  return network_find_node(network_get(), tAddr);
}

// ----- cli_net_add_node -------------------------------------------
int cli_net_add_node(SCliContext * pContext, STokens * pTokens)
{
  net_addr_t tAddr;
  char * pcEndPtr;

  if (ip_string_to_address(tokens_get_string_at(pTokens, 0),
			   &pcEndPtr, &tAddr) ||
      (*pcEndPtr != 0))
    return CLI_ERROR_COMMAND_FAILED;
  if (network_find_node(network_get(), tAddr) != NULL) {
    LOG_SEVERE("Error: could not add node (already exists)\n");
    return CLI_ERROR_COMMAND_FAILED;
  }
  if (network_add_node(network_get(), node_create(tAddr))) {
    LOG_SEVERE("Error: could not add node (unknown reason)\n");
    return CLI_ERROR_COMMAND_FAILED;
  }
  return CLI_SUCCESS;
}

// ----- cli_net_add_link -------------------------------------------
/**
 * context: {}
 * tokens: {addr-src, addr-dst, delay}
 */
int cli_net_add_link(SCliContext * pContext, STokens * pTokens)
{
  SNetNode * pNodeSrc, * pNodeDst;
  unsigned int uDelay;
  char * pcNodeAddr;

  pcNodeAddr= tokens_get_string_at(pTokens, 0);
  pNodeSrc= cli_net_node_by_addr(pcNodeAddr);
  if (pNodeSrc == NULL) {
    LOG_SEVERE("Error: could not find node \"%s\"\n", pcNodeAddr);
    return CLI_ERROR_COMMAND_FAILED;
  }
  pcNodeAddr= tokens_get_string_at(pTokens, 1);
  pNodeDst= cli_net_node_by_addr(pcNodeAddr);
  if (pNodeDst == NULL) {
    LOG_SEVERE("Error: could not find node \"%s\"\n", pcNodeAddr);
    return CLI_ERROR_COMMAND_FAILED;
  }
  if (tokens_get_uint_at(pTokens, 2, &uDelay))
    return CLI_ERROR_COMMAND_FAILED;
  if (node_add_link(pNodeSrc, pNodeDst, (net_link_delay_t) uDelay, 1)) {
    LOG_SEVERE("Error: could not add link (already exists)\n");
    return CLI_ERROR_COMMAND_FAILED;
  }
  return CLI_SUCCESS;
}

// ----- cli_ctx_create_net_link ------------------------------------
int cli_ctx_create_net_link(SCliContext * pContext, void ** ppItem)
{
  SNetNode * pNodeSrc, * pNodeDst;
  char * pcNodeSrcAddr, * pcNodeDstAddr;

  pcNodeSrcAddr= tokens_get_string_at(pContext->pTokens, 0);
  pNodeSrc= cli_net_node_by_addr(pcNodeSrcAddr);
  if (pNodeSrc == NULL) {
    LOG_SEVERE("Error: unable to find node \"%s\"\n", pcNodeSrcAddr);
    return CLI_ERROR_CTX_CREATE;
  }
  pcNodeDstAddr= tokens_get_string_at(pContext->pTokens, 1);
  pNodeDst= cli_net_node_by_addr(pcNodeDstAddr);
  if (pNodeDst == NULL) {
    LOG_SEVERE("Error: unable to find node \"%s\"\n", pcNodeDstAddr);
    return CLI_ERROR_CTX_CREATE;
  }
  *ppItem= node_find_link(pNodeSrc, pNodeDst->tAddr);
  if (*ppItem == NULL) {
    LOG_SEVERE("Error: unable to find link \"%s-%s\"\n",
	       pcNodeSrcAddr, pcNodeDstAddr);
    return CLI_ERROR_CTX_CREATE;
  }
  return CLI_SUCCESS;
}

// ----- cli_ctx_destroy_net_link -----------------------------------
void cli_ctx_destroy_net_link(void ** ppItem)
{
}

// ----- cli_net_node_ipip_enable -----------------------------------
/**
 * context: {node}
 * tokens: {addr}
 */
int cli_net_node_ipip_enable(SCliContext * pContext, STokens * pTokens)
{
  SNetNode * pNode;

  // Get node from context
  pNode= (SNetNode *) cli_context_get_item_at_top(pContext);

  // Enabled IP-in-IP
  if (node_ipip_enable(pNode)) {
    LOG_SEVERE("Error: unexpected error.\n");
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// ----- cli_net_node_spfprefix -------------------------------------
/**
 * context: {node}
 * tokens: {addr, prefix}
 */
int cli_net_node_spfprefix(SCliContext * pContext, STokens * pTokens)
{
  SNetNode * pNode;
  char * pcPrefix;
  SPrefix sPrefix;
  char * pcEndChar;

  // Get node from the CLI's context
  pNode= (SNetNode *) cli_context_get_item_at_top(pContext);
  if (pNode == NULL)
    return CLI_ERROR_COMMAND_FAILED;
  
  // Get the prefix
  pcPrefix= tokens_get_string_at(pTokens, 1);
  if (ip_string_to_prefix(pcPrefix, &pcEndChar, &sPrefix) ||
      (*pcEndChar != 0)) {
    LOG_SEVERE("Error: invalid prefix \"%s\"\n", pcPrefix);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Compute the SPF from the node towards all the nodes in the prefix
  if (igp_compute_prefix(network_get(), pNode, sPrefix))
    return CLI_ERROR_COMMAND_FAILED;

  return CLI_SUCCESS;
}

// ----- cli_net_link_up --------------------------------------------
int cli_net_link_up(SCliContext * pContext, STokens * pTokens)
{
  SNetLink * pLink;

  pLink= (SNetLink *) cli_context_get_item_at_top(pContext);
  if (pLink == NULL)
    return CLI_ERROR_COMMAND_FAILED;
  link_set_state(pLink, NET_LINK_FLAG_UP, 1);
  return CLI_SUCCESS;
}

// ----- cli_net_link_down ------------------------------------------
int cli_net_link_down(SCliContext * pContext, STokens * pTokens)
{
  SNetLink * pLink;

  pLink= (SNetLink *) cli_context_get_item_at_top(pContext);
  if (pLink == NULL)
    return CLI_ERROR_COMMAND_FAILED;
  link_set_state(pLink, NET_LINK_FLAG_UP, 0);
  return CLI_SUCCESS;
}

// ----- cli_net_link_igpweight -------------------------------------
/**
 * context: {link}
 * tokens: {addr-src, addr-dst, igp-weight}
 */
int cli_net_link_igpweight(SCliContext * pContext, STokens * pTokens)
{
  SNetLink * pLink;
  unsigned int uWeight;

  pLink= (SNetLink *) cli_context_get_item_at_top(pContext);
  if (pLink == NULL)
    return CLI_ERROR_COMMAND_FAILED;
  if (tokens_get_uint_at(pTokens, 2, &uWeight) != 0)
    return CLI_ERROR_COMMAND_FAILED;
  link_set_igp_weight(pLink, uWeight);
  return CLI_SUCCESS;
}

// ----- cli_ctx_create_net_node ------------------------------------
int cli_ctx_create_net_node(SCliContext * pContext, void ** ppItem)
{
  char * pcNodeAddr;
  SNetNode * pNode;

  pcNodeAddr= tokens_get_string_at(pContext->pTokens, 0);
  pNode= cli_net_node_by_addr(pcNodeAddr);
  if (pNode == NULL) {
    LOG_SEVERE("Error: unable to find node \"%s\"\n", pcNodeAddr);
    return CLI_ERROR_CTX_CREATE;
  }
  *ppItem= pNode;
  return CLI_SUCCESS;
}

// ----- cli_ctx_destroy_net_node -----------------------------------
void cli_ctx_destroy_net_node(void ** ppItem)
{
}

// ----- cli_net_node_ping ------------------------------------------
/**
 * context: {node}
 * tokens: {addr, addr}
 */
int cli_net_node_ping(SCliContext * pContext, STokens * pTokens)
{
  SNetNode * pNode;
  char * pcDstAddr;
  char * pcEndChar;
  net_addr_t tDstAddr;

 // Get node from the CLI'scontext
  pNode= (SNetNode *) cli_context_get_item_at_top(pContext);
  if (pNode == NULL)
    return CLI_ERROR_COMMAND_FAILED;

  // Get destination address
  pcDstAddr= tokens_get_string_at(pContext->pTokens, 1);
  if (ip_string_to_address(pcDstAddr, &pcEndChar, &tDstAddr) ||
      (*pcEndChar != 0)) {
    LOG_SEVERE("Error: invalid address \"%s\"\n",
	       pcDstAddr);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Send ICMP echo request
  if (icmp_echo_request(pNode, tDstAddr)) {
    LOG_SEVERE("Error: ICMP echo request failed\n");
    return CLI_ERROR_COMMAND_FAILED;
  }
  return CLI_SUCCESS;
}

// ----- cli_net_node_recordroute -----------------------------------
/**
 * context: {node}
 * tokens: {addr, addr}
 */
int cli_net_node_recordroute(SCliContext * pContext, STokens * pTokens)
{
  SNetNode * pNode;
  char * pcDstAddr;
  char * pcEndChar;
  net_addr_t tDstAddr;

 // Get node from the CLI'scontext
  pNode= (SNetNode *) cli_context_get_item_at_top(pContext);
  if (pNode == NULL)
    return CLI_ERROR_COMMAND_FAILED;

  // Get destination address
  pcDstAddr= tokens_get_string_at(pContext->pTokens, 1);
  if (ip_string_to_address(pcDstAddr, &pcEndChar, &tDstAddr) ||
      (*pcEndChar != 0)) {
    LOG_SEVERE("Error: invalid address \"%s\"\n",
	       pcDstAddr);
    return CLI_ERROR_COMMAND_FAILED;
  }

  node_dump_recorded_route(stdout, pNode, tDstAddr, 0);

  return CLI_SUCCESS;
}

// ----- cli_net_node_recordroutedelay ------------------------------
/**
 * context: {node}
 * tokens: {addr, addr}
 */
int cli_net_node_recordroutedelay(SCliContext * pContext,
				  STokens * pTokens)
{
  SNetNode * pNode;
  char * pcDstAddr;
  char * pcEndChar;
  net_addr_t tDstAddr;

 // Get node from the CLI'scontext
  pNode= (SNetNode *) cli_context_get_item_at_top(pContext);
  if (pNode == NULL)
    return CLI_ERROR_COMMAND_FAILED;

  // Get destination address
  pcDstAddr= tokens_get_string_at(pContext->pTokens, 1);
  if (ip_string_to_address(pcDstAddr, &pcEndChar, &tDstAddr) ||
      (*pcEndChar != 0)) {
    LOG_SEVERE("Error: invalid address \"%s\"\n",
	       pcDstAddr);
    return CLI_ERROR_COMMAND_FAILED;
  }

  node_dump_recorded_route(stdout, pNode, tDstAddr, 1);

  return CLI_SUCCESS;
}

// ----- cli_net_node_route_add -------------------------------------
/**
 * This function adds a new static route to the given node.
 *
 * context: {node}
 * tokens: {addr, prefix, next-hop, weight}
 */
int cli_net_node_route_add(SCliContext * pContext, STokens * pTokens)
{
  SNetNode * pNode;
  char * pcEndChar;
  char * pcPrefix;
  SPrefix sPrefix;
  char * pcNextHopAddr;
  net_addr_t tNextHopAddr;
  unsigned long ulWeight;

  // Get node from the CLI'scontext
  pNode= (SNetNode *) cli_context_get_item_at_top(pContext);
  if (pNode == NULL)
    return CLI_ERROR_COMMAND_FAILED;
  
  // Get the route's prefix
  pcPrefix= tokens_get_string_at(pTokens, 1);
  if (ip_string_to_prefix(pcPrefix, &pcEndChar, &sPrefix) ||
      (*pcEndChar != 0)) {
    LOG_SEVERE("Error: invalid prefix \"%s\"\n", pcPrefix);
    return CLI_ERROR_COMMAND_FAILED;
  }
  
  // Get the next-hop's address
  pcNextHopAddr= tokens_get_string_at(pTokens, 2);
  if (ip_string_to_address(pcNextHopAddr, &pcEndChar, &tNextHopAddr) ||
      (*pcEndChar != 0)) {
    LOG_SEVERE("Error: invalid next-hop address \"%s\"\n",
	       pcNextHopAddr);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Get the weight
  if (tokens_get_ulong_at(pTokens, 3, &ulWeight)) {
    LOG_SEVERE("Error: invalid weight\n");
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Add the route
  if (node_rt_add_route(pNode, sPrefix, tNextHopAddr,
			ulWeight, NET_ROUTE_STATIC)) {
    LOG_SEVERE("Error: could not add static route\n");
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// ----- cli_net_node_route_del -------------------------------------
/**
 * This function removes a static route from the given node.
 *
 * context: {node}
 * tokens: {addr, prefix, next-hop, weight}
 */
int cli_net_node_route_del(SCliContext * pContext, STokens * pTokens)
{
  SNetNode * pNode;
  char * pcEndChar;
  char * pcPrefix;
  SPrefix sPrefix;
  char * pcNextHopAddr;
  net_addr_t tNextHopAddr;
  net_addr_t * pNextHopAddr= &tNextHopAddr;

  // Get node from the CLI'scontext
  pNode= (SNetNode *) cli_context_get_item_at_top(pContext);
  if (pNode == NULL)
    return CLI_ERROR_COMMAND_FAILED;
  
  // Get the route's prefix
  pcPrefix= tokens_get_string_at(pTokens, 1);
  if (ip_string_to_prefix(pcPrefix, &pcEndChar, &sPrefix) ||
      (*pcEndChar != 0)) {
    LOG_SEVERE("Error: invalid prefix \"%s\"\n", pcPrefix);
    return CLI_ERROR_COMMAND_FAILED;
  }
  
  // Get the next-hop's address
  pcNextHopAddr= tokens_get_string_at(pTokens, 2);
  if (!strcmp(pcNextHopAddr, "*")) {
    pNextHopAddr= NULL;
  } else if (ip_string_to_address(pcNextHopAddr, &pcEndChar, pNextHopAddr) ||
	     (*pcEndChar != 0)) {
    LOG_SEVERE("Error: invalid next-hop address \"%s\"\n",
	       pcNextHopAddr);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Remove the route
  if (node_rt_del_route(pNode, &sPrefix, pNextHopAddr, NET_ROUTE_STATIC)) {
    LOG_SEVERE("Error: could not remove static route\n");
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// ----- cli_net_node_show_links ------------------------------------
/**
 * context: {node}
 * tokens: {addr}
 */
int cli_net_node_show_links(SCliContext * pContext, STokens * pTokens)
{
  SNetNode * pNode;

  // Get node from the CLI'scontext
  pNode= (SNetNode *) cli_context_get_item_at_top(pContext);
  if (pNode == NULL)
    return CLI_ERROR_COMMAND_FAILED;

  // Dump list of direct links
  node_links_dump(stdout, pNode);
  
  return CLI_SUCCESS;
}

// ----- cli_net_node_show_rt ---------------------------------------
/**
 * context: {node}
 * tokens: {addr}
 */
int cli_net_node_show_rt(SCliContext * pContext, STokens * pTokens)
{
  SNetNode * pNode;
  char * pcPrefix;
  SPrefix sPrefix;

  // Get node from the CLI'scontext
  pNode= (SNetNode *) cli_context_get_item_at_top(pContext);
  if (pNode == NULL)
    return CLI_ERROR_COMMAND_FAILED;

  // Get the prefix/address/*
  pcPrefix= tokens_get_string_at(pTokens, 1);
  if (cli_common_get_dest(pcPrefix, &sPrefix)) {
    LOG_SEVERE("Error: invalid prefix|address|* \"%s\"\n", pcPrefix);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Dump routing table
  node_rt_dump(stdout, pNode, sPrefix);

  return CLI_SUCCESS;
}

// ----- cli_net_node_tunnel_add ------------------------------------
/**
 * context: {node}
 * tokens: {addr, end-point}
 */
int cli_net_node_tunnel_add(SCliContext * pContext, STokens * pTokens)
{
  SNetNode * pNode;
  char * pcEndPointAddr;
  net_addr_t tEndPointAddr;
  char * pcEndChar;

  // Get node from context
  pNode= (SNetNode *) cli_context_get_item_at_top(pContext);

  // Get tunnel end-point
  pcEndPointAddr= tokens_get_string_at(pTokens, 1);
  if (ip_string_to_address(pcEndPointAddr, &pcEndChar, &tEndPointAddr) ||
      (*pcEndChar != 0)) {
    LOG_SEVERE("Error: invalid end-point address \"%s\"\n",
	       pcEndPointAddr);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Add tunnel
  if (node_add_tunnel(pNode, tEndPointAddr)) {
    LOG_SEVERE("Error: unexpected error\n");
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// ----- cli_net_options_maxhops ------------------------------------
/**
 * Change the maximum number of hops used in IP-level
 * record-routes. The maximum number of hops must be in the range
 * [0, 255].
 *
 * context: {}
 * tokens: {maxhops}
 */
int cli_net_options_maxhops(SCliContext * pContext, STokens * pTokens)
{
  unsigned int uMaxHops;

  if (tokens_get_uint_at(pTokens, 0, &uMaxHops)) {
    LOG_SEVERE("Error: invalid value for max-hops (%s)\n",
	       tokens_get_string_at(pTokens, 0));
    return CLI_ERROR_COMMAND_FAILED;
  }

  if (uMaxHops > 255) {
    LOG_SEVERE("Error: maximum number of hops is 255\n");
    return CLI_ERROR_COMMAND_FAILED;
  }

  NET_OPTIONS_MAX_HOPS= uMaxHops;

  return CLI_SUCCESS;
}

// ----- cli_register_net_add ---------------------------------------
int cli_register_net_add(SCliCmds * pCmds)
{
  SCliCmds * pSubCmds;
  SCliParams * pParams;

  pSubCmds= cli_cmds_create();
  pParams= cli_params_create();
  cli_params_add(pParams, "<addr>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("node", cli_net_add_node,
					NULL, pParams));
  pParams= cli_params_create();
  cli_params_add(pParams, "<addr-src>", NULL);
  cli_params_add(pParams, "<addr-dst>", NULL);
  cli_params_add(pParams, "<delay>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("link", cli_net_add_link,
					NULL, pParams));
  return cli_cmds_add(pCmds, cli_cmd_create("add", NULL, pSubCmds, NULL));
}

// ----- cli_register_net_link --------------------------------------
int cli_register_net_link(SCliCmds * pCmds)
{
  SCliCmds * pSubCmds;
  SCliParams * pParams;

  pSubCmds= cli_cmds_create();
  cli_cmds_add(pSubCmds, cli_cmd_create("up", cli_net_link_up,
					NULL, NULL));
  cli_cmds_add(pSubCmds, cli_cmd_create("down", cli_net_link_down,
					NULL, NULL));
  pParams= cli_params_create();
  cli_params_add(pParams, "", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("igp-weight", cli_net_link_igpweight,
					NULL, pParams));
  pParams= cli_params_create();
  cli_params_add(pParams, "<addr-src>", NULL);
  cli_params_add(pParams, "<addr-dst>", NULL);
  return cli_cmds_add(pCmds, cli_cmd_create_ctx("link",
						cli_ctx_create_net_link,
						cli_ctx_destroy_net_link,
						pSubCmds, pParams));
}

// ----- cli_register_net_node_show ---------------------------------
int cli_register_net_node_show(SCliCmds * pCmds)
{
  SCliCmds * pSubCmds;
  SCliParams * pParams;

  pSubCmds= cli_cmds_create();
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
void cli_register_net_node_route(SCliCmds * pCmds)
{
  SCliCmds * pSubCmds= cli_cmds_create();
  SCliParams * pParams;

  pParams= cli_params_create();
  cli_params_add(pParams, "<prefix>", NULL);
  cli_params_add(pParams, "<next-hop>", NULL);
  cli_params_add(pParams, "<weight>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("add",
					cli_net_node_route_add,
					NULL, pParams));
  pParams= cli_params_create();
  cli_params_add(pParams, "<prefix>", NULL);
  cli_params_add(pParams, "<next-hop>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("del",
					cli_net_node_route_del,
					NULL, pParams));
  cli_cmds_add(pCmds, cli_cmd_create("route", NULL, pSubCmds, NULL));
}

// ----- cli_register_net_node_tunnel -------------------------------
int cli_register_net_node_tunnel(SCliCmds * pCmds)
{
  SCliCmds * pSubCmds= cli_cmds_create();
  SCliParams * pParams;

  pParams= cli_params_create();
  cli_params_add(pParams, "<end-point>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("add",
					cli_net_node_tunnel_add,
					NULL, pParams));
  return cli_cmds_add(pCmds, cli_cmd_create("tunnel", NULL,
					   pSubCmds, NULL));
}

// ----- cli_register_net_node --------------------------------------
int cli_register_net_node(SCliCmds * pCmds)
{
  SCliCmds * pSubCmds;
  SCliParams * pParams;

  pSubCmds= cli_cmds_create();
  cli_register_net_node_route(pSubCmds);
  cli_cmds_add(pSubCmds, cli_cmd_create("ipip-enable",
					cli_net_node_ipip_enable,
					NULL, NULL));
  pParams= cli_params_create();
  cli_params_add(pParams, "<address>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("ping",
					cli_net_node_ping,
					NULL, pParams));
  pParams= cli_params_create();
  cli_params_add(pParams, "<address>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("record-route",
					cli_net_node_recordroute,
					NULL, pParams));
  pParams= cli_params_create();
  cli_params_add(pParams, "<address>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("record-route-delay",
					cli_net_node_recordroutedelay,
					NULL, pParams));
  pParams= cli_params_create();
  cli_params_add(pParams, "<prefix>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("spf-prefix",
					cli_net_node_spfprefix,
					NULL, pParams));
  cli_register_net_node_show(pSubCmds);
  cli_register_net_node_tunnel(pSubCmds);
  pParams= cli_params_create();
  cli_params_add(pParams, "<addr>", NULL);
  return cli_cmds_add(pCmds, cli_cmd_create_ctx("node",
						cli_ctx_create_net_node,
						cli_ctx_destroy_net_node,
						pSubCmds, pParams));
}

// ----- cli_register_net_options -----------------------------------
/**
 *
 */
int cli_register_net_options(SCliCmds * pCmds)
{
  SCliCmds * pSubCmds;
  SCliParams * pParams;

  pSubCmds= cli_cmds_create();
  pParams= cli_params_create();
  cli_params_add(pParams, "<max-hops>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("max-hops",
					cli_net_options_maxhops,
					NULL, pParams));
  return cli_cmds_add(pCmds, cli_cmd_create_ctx("options",
						NULL, NULL,
						pSubCmds, NULL));
}

// ----- cli_register_net -------------------------------------------
/**
 *
 */
int cli_register_net(SCli * pCli)
{
  SCliCmds * pCmds;

  pCmds= cli_cmds_create();
  cli_register_net_add(pCmds);
  cli_register_net_link(pCmds);
  cli_register_net_node(pCmds);
  cli_register_net_options(pCmds);
  cli_register_cmd(pCli, cli_cmd_create("net", NULL, pCmds, NULL));
  return 0;
}
