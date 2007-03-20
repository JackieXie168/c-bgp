// ==================================================================
// @(#)net.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 15/07/2003
// @lastdate 20/03/2007
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <errno.h>
#include <string.h>

#include <libgds/cli_ctx.h>
#include <libgds/log.h>
#include <libgds/str_util.h>

#include <cli/common.h>
#include <cli/net.h>
#include <cli/net_domain.h>
#include <cli/net_ospf.h>
#include <net/node.h>
#include <net/ntf.h>
#include <net/prefix.h>
#include <net/subnet.h>
#include <net/icmp.h>
#include <net/igp.h>
#include <net/igp_domain.h>
#include <net/network.h>
#include <net/net_path.h>
#include <net/record-route.h>
#include <net/routing.h>
#include <net/ospf.h>
#include <net/ospf_rt.h>
#include <net/tm.h>
#include <net/tm.h>
#include <net/util.h>
#include <ui/rl.h>

// ----- cli_net_enum_nodes -----------------------------------------
char * cli_net_enum_nodes(const char * pcText, int state)
{
  return network_enum_nodes(pcText, state);
}

// ----- cli_net_node_by_addr ---------------------------------------
/**
 *
 */
SNetNode * cli_net_node_by_addr(char * pcAddr)
{
  net_addr_t tAddr;

  if (str2address(pcAddr, &tAddr))
    return NULL;
  return network_find_node(tAddr);
}

// ----- cli_net_subnet_by_prefix ---------------------------------------
/**
 *
 */
SNetSubnet * cli_net_subnet_by_prefix(char * pcAddr)
{
  SPrefix sPrefix;

  if (str2prefix(pcAddr, &sPrefix))
    return NULL;
  return network_find_subnet(sPrefix);
}

// ----- cli_net_add_node -------------------------------------------
/**
 * context: {}
 * tokens: {addr}
 */
int cli_net_add_node(SCliContext * pContext, SCliCmd * pCmd)
{
  net_addr_t tAddr;

  // Node address ?
  if (str2address(tokens_get_string_at(pCmd->pParamValues, 0), &tAddr)) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: could not add node (invalid address)");
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Check that node does not already exist
  // THIS CHECK SHOULD BE MOVED TO THE NETWORK MGMT PART.
  if (network_find_node(tAddr) != NULL) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: could not add node (");
    node_mgmt_perror(pLogErr, NET_ERROR_MGMT_NODE_ALREADY_EXISTS);
    LOG_ERR(LOG_LEVEL_SEVERE, ")\n");
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Add node.
  // NOTE: IF THAT FAILS, THE NEW NODE SHOULD BE DISPOSED.
  if (network_add_node(node_create(tAddr))) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: could not add node (unknown reason)\n");
    return CLI_ERROR_COMMAND_FAILED;
  }
  return CLI_SUCCESS;
}

// ----- cli_net_add_subnet -------------------------------------------
/**
 * context: {}
 * tokens: {prefix, type}
 */
int cli_net_add_subnet(SCliContext * pContext, SCliCmd * pCmd)
{
  SPrefix sPrefix;
  char * pcType;
  uint8_t uType;
  SNetSubnet * pSubnet;

  // Subnet prefix
  if ((str2prefix(tokens_get_string_at(pCmd->pParamValues, 0), &sPrefix))) {
    LOG_ERR(LOG_LEVEL_SEVERE,
	    "Error: could not add subnet (invalid prefix)\n");
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Check the prefix length (!= 32)
  // THE CHECK FOR PREFIX LENGTH SHOULD BE MOVED TO THE NETWORK MGMT PART.
  if (sPrefix.uMaskLen == 32) {
    LOG_ERR(LOG_LEVEL_SEVERE,
	    "Error: could not add subnet (invalid subnet)\n");
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Check that subnet does not already exist
  // THIS CHECK SHOULD BE MOVED TO THE NETWORK MGMT PART.
  if (network_find_subnet(sPrefix) != NULL) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: could not add subnet (already exists)\n");
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Subnet type: transit / stub ?
  pcType = tokens_get_string_at(pCmd->pParamValues, 1);
  if (strcmp(pcType, "transit") == 0)
    uType = NET_SUBNET_TYPE_TRANSIT;
  else if (strcmp(pcType, "stub") == 0)
    uType = NET_SUBNET_TYPE_STUB;
  else {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: wrong subnet type\n");
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Add the subnet
  // NOTE: IF THAT FAILS, THE NEW SUBNET SHOULD BE DISPOSED.
  pSubnet= subnet_create(sPrefix.tNetwork, sPrefix.uMaskLen, uType);
  if (network_add_subnet(pSubnet) < 0) {
    LOG_ERR(LOG_LEVEL_SEVERE,
	    "Error: could not add subnet (unknown reason)\n");
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// ----- cli_net_add_link -------------------------------------------
/**
 * context: {}
 * tokens: {addr-src, prefix-dst, delay}
 * options: [--depth]
 */
int cli_net_add_link(SCliContext * pContext, SCliCmd * pCmd)
{
  SNetNode * pNodeSrc;
  unsigned int uValue;
  char * pcNodeSrcAddr, * pcDest;
  SNetDest sDest;
  int iErrorCode;
  char * pcValue;
  net_link_delay_t tDelay= 0;
  uint8_t tDepth= 1;
  net_link_load_t tCapacity= 0;

  // Get optional link depth
  pcValue= cli_options_get_value(pCmd->pOptions, "depth");
  if (pcValue != NULL) {
    if ((str_as_uint(pcValue, &uValue) < 0) ||
	(uValue > NET_LINK_MAX_DEPTH)) {
      LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid depth \"%s\"\n", pcValue);
    }
    tDepth= uValue;
  }

  // Get optional link capacity
  pcValue= cli_options_get_value(pCmd->pOptions, "bw");
  if (pcValue != NULL) {
    if (str_as_uint(pcValue, &uValue) < 0) {
      LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid capacity \"%s\"\n", pcValue);
    }
    tCapacity= uValue;
  }

  
  // Get source node
  pcNodeSrcAddr= tokens_get_string_at(pCmd->pParamValues, 0);
  pNodeSrc= cli_net_node_by_addr(pcNodeSrcAddr);
    if (pNodeSrc == NULL) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: could not find node \"%s\"\n", pcNodeSrcAddr);
    return CLI_ERROR_COMMAND_FAILED;
  }
  
  // Get destination: can be a node (router) or a subnet
  pcDest= tokens_get_string_at(pCmd->pParamValues, 1);
  if ((ip_string_to_dest(pcDest, &sDest) < 0) ||
      (sDest.tType == NET_DEST_ANY)) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid destination \"%s\".\n",
	       pcDest);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Get delay
  if (tokens_get_uint_at(pCmd->pParamValues, 2, &uValue)) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid delay %s.\n",
	       tokens_get_string_at(pCmd->pParamValues, 2));
    return CLI_ERROR_COMMAND_FAILED;
  }
  tDelay= uValue;

  // Add link
  if ((iErrorCode= node_add_link(pNodeSrc, sDest, tDelay, tCapacity,
				 tDepth))) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: could not add link %s -> %s (",
	       pcNodeSrcAddr, pcDest);
    node_mgmt_perror(pLogErr, iErrorCode);
    LOG_ERR(LOG_LEVEL_SEVERE, ")\n");
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// ----- cli_ctx_create_net_link ------------------------------------
/**
 * context: {}
 * tokens: {src-addr, dst-addr}
 */
int cli_ctx_create_net_link(SCliContext * pContext, void ** ppItem)
{
  SNetNode * pNodeSrc;
  SNetDest sDest;
  char     * pcNodeSrcAddr, * pcVertexDstPrefix; 
  SNetLink * pLink = NULL;

  pcNodeSrcAddr= tokens_get_string_at(pContext->pCmd->pParamValues, 0);
  pNodeSrc= cli_net_node_by_addr(pcNodeSrcAddr);
  if (pNodeSrc == NULL) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: unable to find node \"%s\"\n", pcNodeSrcAddr);
    return CLI_ERROR_CTX_CREATE;
  }

  pcVertexDstPrefix= tokens_get_string_at(pContext->pCmd->pParamValues, 1);
  if (ip_string_to_dest(pcVertexDstPrefix, &sDest) < 0 ||
      sDest.tType == NET_DEST_ANY) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: destination id is wrong \"%s\"\n", pcVertexDstPrefix);
    return CLI_ERROR_CTX_CREATE;
  }

  pLink = node_find_link(pNodeSrc, sDest);
  if (pLink == NULL) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: unable to find link %s -> %s\n",
	       pcNodeSrcAddr, pcVertexDstPrefix);
    return CLI_ERROR_COMMAND_FAILED;
  }

  *ppItem= pLink;

  return CLI_SUCCESS;
}

// ----- cli_ctx_destroy_net_link -----------------------------------
void cli_ctx_destroy_net_link(void ** ppItem)
{
}

// ----- cli_net_ntf_load -------------------------------------------
/**
 * context: {}
 * tokens: {ntf_file}
 */
int cli_net_ntf_load(SCliContext * pContext, SCliCmd * pCmd)
{
  char * pcFileName;

  // Get name of the NTF file
  pcFileName= tokens_get_string_at(pCmd->pParamValues, 0);

  // Load given NTF file
  if (ntf_load(pcFileName) != NTF_SUCCESS) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: unable to load NTF file \"%s\"\n",
	       pcFileName);
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// ----- cli_net_node_ipip_enable -----------------------------------
/**
 * context: {node}
 * tokens: {}
 */
int cli_net_node_ipip_enable(SCliContext * pContext, SCliCmd * pCmd)
{
  SNetNode * pNode;

  // Get node from context
  pNode= (SNetNode *) cli_context_get_item_at_top(pContext);

  // Enabled IP-in-IP
  if (node_ipip_enable(pNode)) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: unexpected error.\n");
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// ----- cli_net_node_name ------------------------------------------
/**
 * context: {node}
 * tokens: {string}
 */
int cli_net_node_name(SCliContext * pContext, SCliCmd * pCmd)
{
  SNetNode * pNode;

  // Get node from context
  pNode= (SNetNode *) cli_context_get_item_at_top(pContext);

  node_set_name(pNode, tokens_get_string_at(pCmd->pParamValues, 0));

  return CLI_SUCCESS;
}

// ----- cli_net_link_up --------------------------------------------
/**
 * context: {link}
 * tokens: {}
 */
int cli_net_link_up(SCliContext * pContext, SCliCmd * pCmd)
{
  SNetLink * pLink;

  pLink= (SNetLink *) cli_context_get_item_at_top(pContext);
  if (pLink == NULL)
    return CLI_ERROR_COMMAND_FAILED;

  net_link_set_state(pLink, NET_LINK_FLAG_UP, 1);
  return CLI_SUCCESS;
}

// ----- cli_net_link_down ------------------------------------------
/**
 * context: {link}
 * tokens: {}
 */
int cli_net_link_down(SCliContext * pContext, SCliCmd * pCmd)
{
  SNetLink * pLink;

  pLink= (SNetLink *) cli_context_get_item_at_top(pContext);
  if (pLink == NULL)
    return CLI_ERROR_COMMAND_FAILED;

  net_link_set_state(pLink, NET_LINK_FLAG_UP, 0);
  return CLI_SUCCESS;
}

// ----- cli_net_link_ipprefix -------------------------------------
/**
 * context: {link}
 * tokens: {iface-prefix}
 */
/* COMMENT ON 17/01/2007
int cli_net_link_ipprefix(SCliContext * pContext, SCliCmd * pCmd)
{
  SNetLink * pLink;
  char * pcIfaceAddr;
  char * pcEndChar;
  SPrefix sIfacePrefix;
  
  pLink= (SNetLink *) cli_context_get_item_at_top(pContext);
  if (pLink == NULL)
    return CLI_ERROR_COMMAND_FAILED;

  if (!link_is_to_router(pLink))
    return CLI_ERROR_COMMAND_FAILED;
  
  // Get src prefix
  pcIfaceAddr= tokens_get_string_at(pContext->pCmd->pParamValues, 0);
  if (ip_string_to_prefix(pcIfaceAddr, &pcEndChar, &sIfacePrefix) ||
      (*pcEndChar != 0)) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid prefix \"%s\"\n",
	       pcIfaceAddr);
    return CLI_ERROR_COMMAND_FAILED;
  }
  

  link_set_ip_prefix(pLink, sIfacePrefix);
  return CLI_SUCCESS;
  }*/

// ----- cli_net_link_igpweight -------------------------------------
/**
 * context: {link}
 * tokens: {[--bidir] [--tos] igp-weight}
 */
int cli_net_link_igpweight(SCliContext * pContext, SCliCmd * pCmd)
{
  SNetNode * pNode;
  SNetLink * pLink;
  net_igp_weight_t tWeight;
  net_tos_t tTOS= 0;
  char * pcValue;
  int iBidir= 0;

  // Get link from context
  pLink= (SNetLink *) cli_context_get_item_at_top(pContext);
  if (pLink == NULL)
    return CLI_ERROR_COMMAND_FAILED;

  // Get optional TOS
  pcValue= cli_options_get_value(pCmd->pOptions, "tos");
  if (pcValue != NULL)
    if (!str2tos(pcValue, &tTOS) ||
	(tTOS > net_link_get_depth(pLink))) {
      LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid TOS \"%s\"\n", pcValue);
      return CLI_ERROR_COMMAND_FAILED;
    }

  // Check option --bidir
  if (cli_options_has_value(pCmd->pOptions, "bidir")) {
    if (pLink->uType != NET_LINK_TYPE_ROUTER) {
      LOG_ERR(LOG_LEVEL_SEVERE, "Error: --bidir only works with ptp links\n");
      return CLI_ERROR_COMMAND_FAILED;
    }
    iBidir= 1;
  }

  // Get new IGP weight
  if (tokens_get_uint_at(pCmd->pParamValues, 0, &tWeight) != 0) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid weight \"%s\"\n",
	    tokens_get_string_at(pCmd->pParamValues, 0));
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Change link weight in forward direction
  net_link_set_weight(pLink, tTOS, tWeight);

  // Change link weight in backward direction (if requested)
  if (iBidir) {
    pNode= network_find_node(pLink->tDest.tAddr);
    assert(pNode != NULL);
    pLink= node_find_link_ptp(pNode, pLink->pSrcNode->tAddr);
    assert(pLink != NULL);
    net_link_set_weight(pLink, tTOS, tWeight);
  }

  return CLI_SUCCESS;
}

// ----- cli_net_link_load_clear ------------------------------------
/**
 * context: {link}
 * tokens: {}
 */
int cli_net_link_load_clear(SCliContext * pContext, SCliCmd * pCmd)
{
  SNetLink * pLink;

  // Get link from context
  pLink= (SNetLink *) cli_context_get_item_at_top(pContext);
  if (pLink == NULL)
    return CLI_ERROR_COMMAND_FAILED;

  net_link_set_load(pLink, 0);

  return CLI_SUCCESS;
}

// ----- cli_net_link_load_add --------------------------------------
/**
 * context: {link}
 * tokens: {load}
 */
int cli_net_link_load_add(SCliContext * pContext, SCliCmd * pCmd)
{
  SNetLink * pLink;
  char * pcValue;
  unsigned int uValue;
  net_link_load_t tLoad;

  // Get link from context
  pLink= (SNetLink *) cli_context_get_item_at_top(pContext);
  if (pLink == NULL)
    return CLI_ERROR_COMMAND_FAILED;

  // Get load
  pcValue= tokens_get_string_at(pCmd->pParamValues, 0);
  if (str_as_uint(pcValue, &uValue) < 0) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid load \"%s\"\n", pcValue);
    return CLI_ERROR_COMMAND_FAILED;
  }
  tLoad= uValue;

  net_link_set_load(pLink, tLoad);

  return CLI_SUCCESS;
}

// ----- cli_net_link_load_show -------------------------------------
/**
 * context: {link}
 * tokens: {}
 */
int cli_net_link_load_show(SCliContext * pContext, SCliCmd * pCmd)
{
  SNetLink * pLink;

  // Get link from context
  pLink= (SNetLink *) cli_context_get_item_at_top(pContext);
  if (pLink == NULL)
    return CLI_ERROR_COMMAND_FAILED;

  net_link_dump_load(pLogOut, pLink);

  return CLI_SUCCESS;
}

// ----- cli_net_link_show_info -------------------------------------
/**
 * context: {link}
 * tokens: {}
 */
int cli_net_link_show_info(SCliContext * pContext, SCliCmd * pCmd)
{
  SNetLink * pLink;

  // Get link from context
  pLink= (SNetLink *) cli_context_get_item_at_top(pContext);
  if (pLink == NULL)
    return CLI_ERROR_COMMAND_FAILED;

  net_link_dump_info(pLogOut, pLink);

  return CLI_SUCCESS;
}

// ----- cli_net_links_load_clear -----------------------------------
/**
 * context: {}
 * tokens: {}
 */
int cli_net_links_load_clear(SCliContext * pContext, SCliCmd * pCmd)
{
  network_links_clear();
  return CLI_SUCCESS;
}

// ----- cli_ctx_create_net_node ------------------------------------
int cli_ctx_create_net_node(SCliContext * pContext, void ** ppItem)
{
  char * pcNodeAddr;
  SNetNode * pNode;

  pcNodeAddr= tokens_get_string_at(pContext->pCmd->pParamValues, 0);
  pNode= cli_net_node_by_addr(pcNodeAddr);
  if (pNode == NULL) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: unable to find node \"%s\"\n", pcNodeAddr);
    return CLI_ERROR_CTX_CREATE;
  }
  *ppItem= pNode;
  return CLI_SUCCESS;
}

// ----- cli_ctx_destroy_net_node -----------------------------------
void cli_ctx_destroy_net_node(void ** ppItem)
{
}

// ----- cli_ctx_create_net_node ------------------------------------
int cli_ctx_create_net_subnet(SCliContext * pContext, void ** ppItem)
{
  char * pcSubnetPfx;
  SNetSubnet * pSubnet;

  pcSubnetPfx= tokens_get_string_at(pContext->pCmd->pParamValues, 0);
  pSubnet = cli_net_subnet_by_prefix(pcSubnetPfx);
  
  if (pSubnet == NULL) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: unable to find subnet \"%s\"\n", pcSubnetPfx);
    return CLI_ERROR_CTX_CREATE;
  }
  *ppItem= pSubnet;
  return CLI_SUCCESS;
}

// ----- cli_ctx_destroy_net_subnet -----------------------------------
void cli_ctx_destroy_net_subnet(void ** ppItem)
{
}

// ----- cli_ctx_create_net_node_link ------------------------------------
/**
 * context: {node}
 * tokens: {prefix|address|*}
 */
int cli_ctx_create_net_node_link(SCliContext * pContext, void ** ppItem)
{
   char * pcPrefix;
  SNetLink * pLink = NULL;
  SNetDest sDest;
  
  SNetNode * pNodeSource = (SNetNode *) cli_context_get_item_at_top(pContext);

  pcPrefix = tokens_get_string_at(pContext->pCmd->pParamValues, 0);
  if (ip_string_to_dest(pcPrefix, &sDest) ||
      (sDest.tType == NET_DEST_ANY)) {
    LOG_ERR(LOG_LEVEL_SEVERE,
	    "Error: invalid destination prefix|address \"%s\"\n", pcPrefix);
    return CLI_ERROR_COMMAND_FAILED;
  }

  if (sDest.tType == NET_DEST_ADDRESS)
    (sDest.uDest).sPrefix.uMaskLen= 32;
  
  pLink= node_find_link(pNodeSource, sDest);
  if (pLink == NULL) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: unable to find link \"%s\"\n", pcPrefix);
    return CLI_ERROR_CTX_CREATE;
  }

  *ppItem= pLink;
  return CLI_SUCCESS;
}

// ----- cli_ctx_destroy_net_node_link -----------------------------------
void cli_ctx_destroy_net_node_link(void ** ppItem)
{
}

// ----- cli_net_node_ping ------------------------------------------
/**
 * context: {node}
 * tokens: {addr}
 */
int cli_net_node_ping(SCliContext * pContext, SCliCmd * pCmd)
{
  SNetNode * pNode;
  char * pcDstAddr;
  char * pcEndChar;
  net_addr_t tDstAddr;
  int iErrorCode;

 // Get node from the CLI'scontext
  pNode= (SNetNode *) cli_context_get_item_at_top(pContext);
  if (pNode == NULL)
    return CLI_ERROR_COMMAND_FAILED;

  // Get destination address
  pcDstAddr= tokens_get_string_at(pContext->pCmd->pParamValues, 0);
  if (ip_string_to_address(pcDstAddr, &pcEndChar, &tDstAddr) ||
      (*pcEndChar != 0)) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid address \"%s\"\n",
	       pcDstAddr);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Send ICMP echo request
  iErrorCode= icmp_echo_request(pNode, tDstAddr);
  if (iErrorCode != NET_SUCCESS) {
    LOG_ERR_ENABLED(LOG_LEVEL_SEVERE) {
      log_printf(pLogErr, "Error: ICMP echo request failed: ");
      network_perror(pLogErr, iErrorCode);
      log_printf(pLogErr, "\n");
    }
    return CLI_ERROR_COMMAND_FAILED;
  }
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
int cli_net_node_recordroute(SCliContext * pContext, SCliCmd * pCmd)
{
  SNetNode * pNode;
  char * pcDest;
  SNetDest sDest;
  net_tos_t tTOS= 0;
  uint8_t uOptions= 0;
  char * pcValue;
  unsigned int uValue;
  net_link_load_t tLoad= 0;

 // Get node from the CLI'scontext
  pNode= (SNetNode *) cli_context_get_item_at_top(pContext);
  if (pNode == NULL)
    return CLI_ERROR_COMMAND_FAILED;

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
      LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid load \"%s\"\n", pcValue);
      return CLI_ERROR_COMMAND_FAILED;
    }
    uOptions|= NET_RECORD_ROUTE_OPTION_LOAD;
    tLoad= uValue;
  }

  // Perform record-route in particular plane (based on TOS) ?
  pcValue= cli_options_get_value(pCmd->pOptions, "tos");
  if (pcValue != NULL)
    if (!str2tos(pcValue, &tTOS)) {
      LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid TOS \"%s\"\n", pcValue);
      return CLI_ERROR_COMMAND_FAILED;
    }

  // Optional weight ?
  if (cli_options_has_value(pCmd->pOptions, "weight"))
    uOptions|= NET_RECORD_ROUTE_OPTION_WEIGHT;

  // Get destination address
  pcDest= tokens_get_string_at(pCmd->pParamValues, 0);
  if (ip_string_to_dest(pcDest, &sDest)) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid prefix|address|* \"%s\"\n", pcDest);
    return CLI_ERROR_COMMAND_FAILED;
  }

  /* Check that the destination type is adress/prefix */
  if ((sDest.tType != NET_DEST_ADDRESS) &&
      (sDest.tType != NET_DEST_PREFIX)) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: can not use this destination type with record-route\n");
    return CLI_ERROR_COMMAND_FAILED;
  }

  node_dump_recorded_route(pLogOut, pNode, sDest, tTOS, uOptions, tLoad);

  return CLI_SUCCESS;
}

// ----- cli_net_node_route_add -------------------------------------
/**
 * This function adds a new static route to the given node.
 *
 * context: {node}
 * tokens: {prefix, addr|*, addr|prefix, weight, iface, weight}
 */
int cli_net_node_route_add(SCliContext * pContext, SCliCmd * pCmd)
{
  SNetNode * pNode;
  char * pcEndChar;
  char * pcPrefix;
  SPrefix sPrefix;
  char * pcIface, * pcNextHop;
  net_addr_t tNextHop;
  SNetDest sDest, sIface;
  unsigned long ulWeight;
  int iErrorCode;

  // Get node from the CLI'scontext
  pNode= (SNetNode *) cli_context_get_item_at_top(pContext);
  if (pNode == NULL)
    return CLI_ERROR_COMMAND_FAILED;
  
  // Get the route's prefix
  pcPrefix= tokens_get_string_at(pCmd->pParamValues, 0);
  if (ip_string_to_prefix(pcPrefix, &pcEndChar, &sPrefix) ||
      (*pcEndChar != 0)) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid prefix \"%s\"\n", pcPrefix);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Get the next-hop
  pcNextHop= tokens_get_string_at(pCmd->pParamValues, 1);
  if (ip_string_to_dest(pcNextHop, &sDest) ||
      (sDest.tType == NET_DEST_PREFIX)) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid next-hop \"%s\"\n",
	       pcNextHop);
    return CLI_ERROR_COMMAND_FAILED;
  }
  switch (sDest.tType) {
  case NET_DEST_ADDRESS:
    tNextHop= sDest.uDest.tAddr; break;
  case NET_DEST_ANY:
    tNextHop= 0; break;
  default:
    abort();
  }
  
  // Get the outgoing link identifier (address/prefix)
  pcIface= tokens_get_string_at(pCmd->pParamValues, 2);
  if (ip_string_to_dest(pcIface, &sIface) ||
      (sIface.tType == NET_DEST_ANY)) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid interface \"%s\"\n",
	       pcIface);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Get the weight
  if (tokens_get_ulong_at(pCmd->pParamValues, 3, &ulWeight)) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid weight\n");
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Add the route
  if ((iErrorCode= node_rt_add_route_dest(pNode, sPrefix, sIface, tNextHop,
					  ulWeight, NET_ROUTE_STATIC))) {
    LOG_ERR_ENABLED(LOG_LEVEL_SEVERE) {
      log_printf(pLogErr, "Error: could not add static route (");
      rt_perror(pLogErr, iErrorCode);
      log_printf(pLogErr, ").\n");
    }
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
int cli_net_node_route_del(SCliContext * pContext, SCliCmd * pCmd)
{
  SNetNode * pNode;
  char * pcEndChar;
  char * pcPrefix;
  SPrefix sPrefix;
  char * pcNextHop;
  SNetDest sDest;
  net_addr_t tNextHop;
  char * pcIface;
  SNetDest sIface;

  // Get node from the CLI'scontext
  pNode= (SNetNode *) cli_context_get_item_at_top(pContext);
  if (pNode == NULL)
    return CLI_ERROR_COMMAND_FAILED;
  
  // Get the route's prefix
  pcPrefix= tokens_get_string_at(pCmd->pParamValues, 0);
  if (ip_string_to_prefix(pcPrefix, &pcEndChar, &sPrefix) ||
      (*pcEndChar != 0)) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid prefix \"%s\"\n", pcPrefix);
    return CLI_ERROR_COMMAND_FAILED;
  }
  
  // Get the next-hop
  pcNextHop= tokens_get_string_at(pCmd->pParamValues, 1);
  if (ip_string_to_dest(pcNextHop, &sDest) ||
      (sDest.tType == NET_DEST_PREFIX)) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid next-hop \"%s\"\n",
	       pcNextHop);
    return CLI_ERROR_COMMAND_FAILED;
  }
  switch (sDest.tType) {
  case NET_DEST_ADDRESS:
    tNextHop= sDest.uDest.tAddr; break;
  case NET_DEST_ANY:
    tNextHop= 0; break;
  default:
    abort();
  }

  // Get the outgoing link identifier (address/prefix)
  pcIface= tokens_get_string_at(pCmd->pParamValues, 2);
  if (ip_string_to_dest(pcIface, &sIface)) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid next-hop address \"%s\"\n",
	       pcIface);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Remove the route
  if (node_rt_del_route(pNode, &sPrefix, &sIface, &tNextHop,
			NET_ROUTE_STATIC)) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: could not remove static route\n");
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// ----- cli_net_node_show_ifaces -----------------------------------
/**
 * context: {node}
 * tokens: {}
 */
int cli_net_node_show_ifaces(SCliContext * pContext, SCliCmd * pCmd)
{
  SNetNode * pNode;

  // Get node from the CLI'scontext
  pNode= (SNetNode *) cli_context_get_item_at_top(pContext);
  if (pNode == NULL)
    return CLI_ERROR_COMMAND_FAILED;

  node_ifaces_dump(pLogOut, pNode);
  return CLI_SUCCESS;
}

// ----- cli_net_node_show_info -------------------------------------
/**
 * context: {node}
 * tokens: {}
 */
int cli_net_node_show_info(SCliContext * pContext, SCliCmd * pCmd)
{
  SNetNode * pNode;

  // Get node from the CLI'scontext
  pNode= (SNetNode *) cli_context_get_item_at_top(pContext);
  if (pNode == NULL)
    return CLI_ERROR_COMMAND_FAILED;

  node_info(pLogOut, pNode);
  return CLI_SUCCESS;
}

// ----- cli_net_node_show_links ------------------------------------
/**
 * context: {node}
 * tokens: {}
 */
int cli_net_node_show_links(SCliContext * pContext, SCliCmd * pCmd)
{
  SNetNode * pNode;

  // Get node from the CLI'scontext
  pNode= (SNetNode *) cli_context_get_item_at_top(pContext);
  if (pNode == NULL)
    return CLI_ERROR_COMMAND_FAILED;

  // Dump list of direct links
  node_links_dump(pLogOut, pNode);
  
  return CLI_SUCCESS;
}

// ----- cli_net_node_show_rt ---------------------------------------
/**
 * context: {node}
 * tokens: {prefix|address|*}
 */
int cli_net_node_show_rt(SCliContext * pContext, SCliCmd * pCmd)
{
  SNetNode * pNode;
  char * pcPrefix;
  SNetDest sDest;

  // Get node from the CLI'scontext
  pNode= (SNetNode *) cli_context_get_item_at_top(pContext);
  if (pNode == NULL)
    return CLI_ERROR_COMMAND_FAILED;

  // Get the prefix/address/*
  pcPrefix= tokens_get_string_at(pCmd->pParamValues, 0);
  if (ip_string_to_dest(pcPrefix, &sDest)) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid prefix|address|* \"%s\"\n", pcPrefix);
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
 * tokens: {end-point}
 */
int cli_net_node_tunnel_add(SCliContext * pContext, SCliCmd * pCmd)
{
  SNetNode * pNode;
  char * pcEndPointAddr;
  net_addr_t tEndPointAddr;
  char * pcEndChar;

  // Get node from context
  pNode= (SNetNode *) cli_context_get_item_at_top(pContext);

  // Get tunnel end-point
  pcEndPointAddr= tokens_get_string_at(pCmd->pParamValues, 0);
  if (ip_string_to_address(pcEndPointAddr, &pcEndChar, &tEndPointAddr) ||
      (*pcEndChar != 0)) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid end-point address \"%s\"\n",
	       pcEndPointAddr);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Add tunnel
  if (node_add_tunnel(pNode, tEndPointAddr)) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: unexpected error\n");
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
int cli_net_options_maxhops(SCliContext * pContext, SCliCmd * pCmd)
{
  unsigned int uMaxHops;

  if (tokens_get_uint_at(pCmd->pParamValues, 0, &uMaxHops)) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid value for max-hops (%s)\n",
	       tokens_get_string_at(pCmd->pParamValues, 0));
    return CLI_ERROR_COMMAND_FAILED;
  }

  if (uMaxHops > 255) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: maximum number of hops is 255\n");
    return CLI_ERROR_COMMAND_FAILED;
  }

  NET_OPTIONS_MAX_HOPS= uMaxHops;

  return CLI_SUCCESS;
}

// ----- cli_net_show_nodes -----------------------------------------
/**
 * Display all nodes matching the given criterion, which is a prefix
 * or an asterisk (meaning "all nodes").
 *
 * context: {}
 * tokens: {prefix}
 */
int cli_net_show_nodes(SCliContext * pContext, SCliCmd * pCmd)
{
  network_to_file(pLogOut, network_get());
  return CLI_SUCCESS;
}

// ----- cli_net_show_nodes -----------------------------------------
/**
 * Display all nodes matching the given criterion, which is a prefix
 * or an asterisk (meaning "all nodes").
 *
 * context: {}
 * tokens: {}
 */
int cli_net_show_subnets(SCliContext * pContext, SCliCmd * pCmd)
{
  network_dump_subnets(pLogOut, network_get());
  return CLI_SUCCESS;
}

// ----- cli_net_tm_load --------------------------------------------
/**
 * context: {}
 * tokens: {file}
 */
int cli_net_tm_load(SCliContext * pContext, SCliCmd * pCmd)
{
  int iResult;
  char * pcFileName;

  pcFileName= tokens_get_string_at(pCmd->pParamValues, 0);
  iResult= net_tm_load(pcFileName);
  if (iResult != NET_TM_SUCCESS) {
    LOG_ERR(LOG_LEVEL_SEVERE,
	    "Error: could not load traffic matrix from \"%s\"", pcFileName);
    net_tm_perror(pLogErr, iResult);
    LOG_ERR(LOG_LEVEL_SEVERE, "\n");
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// ----- cli_register_net_add ---------------------------------------
int cli_register_net_add(SCliCmds * pCmds)
{
  SCliCmds * pSubCmds;
  SCliParams * pParams;
  SCliCmd * pCmd;

  pSubCmds= cli_cmds_create();
  pParams= cli_params_create();
  cli_params_add(pParams, "<id>", NULL);
  cli_params_add(pParams, "<type>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("domain", cli_net_add_domain,
					NULL, pParams));
  pParams= cli_params_create();
  cli_params_add(pParams, "<addr>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("node", cli_net_add_node,
					NULL, pParams));
  pParams= cli_params_create();
  cli_params_add(pParams, "<prefix>", NULL);
  cli_params_add(pParams, "<transit|stub>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("subnet", cli_net_add_subnet,
					NULL, pParams));
  pParams= cli_params_create();
  cli_params_add2(pParams, "<addr-src>", NULL, cli_net_enum_nodes);
  cli_params_add2(pParams, "<addr-dst>", NULL, cli_net_enum_nodes);
  cli_params_add(pParams, "<delay>", NULL);
  pCmd= cli_cmd_create("link", cli_net_add_link, NULL, pParams);
  cli_cmd_add_option(pCmd, "bw", NULL);
  cli_cmd_add_option(pCmd, "depth", NULL);
  cli_cmds_add(pSubCmds, pCmd);
  return cli_cmds_add(pCmds, cli_cmd_create("add", NULL, pSubCmds, NULL));
}

// ----- cli_register_net_link_load ---------------------------------
int cli_register_net_link_load(SCliCmds * pCmds)
{
  SCliCmds * pSubCmds;
  SCliParams * pParams;
  
  pSubCmds= cli_cmds_create();
  pParams= cli_params_create();
  cli_params_add(pParams, "<load>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("add", cli_net_link_load_add,
					NULL, pParams));
  cli_cmds_add(pSubCmds, cli_cmd_create("clear", cli_net_link_load_clear,
					NULL, NULL));
  cli_cmds_add(pSubCmds, cli_cmd_create("show", cli_net_link_load_show,
					NULL, NULL));
  return cli_cmds_add(pCmds, cli_cmd_create("load", NULL, pSubCmds, NULL));
}

// ----- cli_register_net_link_show ---------------------------------
int cli_register_net_link_show(SCliCmds * pCmds)
{
  SCliCmds * pSubCmds;
  
  pSubCmds= cli_cmds_create();
  cli_cmds_add(pSubCmds, cli_cmd_create("info", cli_net_link_show_info,
					NULL, NULL));
  return cli_cmds_add(pCmds, cli_cmd_create("show", NULL, pSubCmds, NULL));
}

// ----- cli_register_net_link --------------------------------------
int cli_register_net_link(SCliCmds * pCmds)
{
  SCliCmds * pSubCmds;
  SCliParams * pParams;
  SCliCmd * pCmd;

  pSubCmds= cli_cmds_create();
  cli_cmds_add(pSubCmds, cli_cmd_create("up", cli_net_link_up,
					NULL, NULL));
  cli_cmds_add(pSubCmds, cli_cmd_create("down", cli_net_link_down,
					NULL, NULL));
  pParams= cli_params_create();
  cli_params_add(pParams, "", NULL);
  pCmd= cli_cmd_create("igp-weight", cli_net_link_igpweight, NULL, pParams);
  cli_cmd_add_option(pCmd, "bidir", NULL);
  cli_cmd_add_option(pCmd, "tos", NULL);
  cli_cmds_add(pSubCmds, pCmd);
  cli_register_net_link_load(pSubCmds);
  cli_register_net_link_show(pSubCmds);
  pParams= cli_params_create();
  cli_params_add2(pParams, "<addr-src>", NULL, cli_net_enum_nodes);
  cli_params_add2(pParams, "<addr-dst>", NULL, cli_net_enum_nodes);
  return cli_cmds_add(pCmds, cli_cmd_create_ctx("link",
						cli_ctx_create_net_link,
						cli_ctx_destroy_net_link,
						pSubCmds, pParams));
}

// ----- cli_register_net_links -------------------------------------
int cli_register_net_links(SCliCmds * pCmds)
{
  SCliCmds * pSubCmds;

  pSubCmds= cli_cmds_create();
  cli_cmds_add(pSubCmds, cli_cmd_create("load-clear",
					cli_net_links_load_clear,
					NULL, NULL));
  return cli_cmds_add(pCmds, cli_cmd_create("links", NULL, pSubCmds, NULL));
}

// ----- cli_register_net_ntf ---------------------------------------
int cli_register_net_ntf(SCliCmds * pCmds)
{
  SCliCmds * pSubCmds;
  SCliParams * pParams;

  pSubCmds= cli_cmds_create();
  pParams= cli_params_create();
#ifdef _FILENAME_COMPLETION_FUNCTION
  cli_params_add2(pParams, "<filename>", NULL,
		  _FILENAME_COMPLETION_FUNCTION);
#else
  cli_params_add(pParams, "<filename>", NULL);
#endif
  cli_cmds_add(pSubCmds, cli_cmd_create("load",
					cli_net_ntf_load,
					NULL, pParams));
  return cli_cmds_add(pCmds, cli_cmd_create_ctx("ntf", NULL, NULL,
						pSubCmds, NULL));
}

// ----- cli_register_net_node_show ---------------------------------
int cli_register_net_node_show(SCliCmds * pCmds)
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
void cli_register_net_node_route(SCliCmds * pCmds)
{
  SCliCmds * pSubCmds= cli_cmds_create();
  SCliParams * pParams;

  pParams= cli_params_create();
  cli_params_add(pParams, "<prefix>", NULL);
  cli_params_add(pParams, "<next-hop>", NULL);
  cli_params_add(pParams, "<iface>", NULL);
  cli_params_add(pParams, "<weight>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("add",
					cli_net_node_route_add,
					NULL, pParams));
  pParams= cli_params_create();
  cli_params_add(pParams, "<prefix>", NULL);
  cli_params_add(pParams, "<next-hop>", NULL);
  cli_params_add(pParams, "<iface>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("del",
					cli_net_node_route_del,
					NULL, pParams));
  cli_cmds_add(pCmds, cli_cmd_create("route", NULL, pSubCmds, NULL));
}

// ----- cli_register_net_node_link --------------------------------
int cli_register_net_node_link(SCliCmds * pCmds)
{
  SCliCmds * pSubCmds= cli_cmds_create();
  SCliParams * pParams;
  
#ifdef OSPF_SUPPORT
  cli_register_net_node_link_ospf(pSubCmds);
#endif  
  pParams= cli_params_create();
  cli_params_add(pParams, "<prefix>", NULL);			
//   cli_cmds_add(pCmds, cli_cmd_create("link", NULL, pSubCmds, pParams));
  return cli_cmds_add(pCmds, cli_cmd_create_ctx("link",
						cli_ctx_create_net_node_link,
						cli_ctx_destroy_net_node_link,
						pSubCmds, pParams));
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
  SCliCmd * pCmd;

  pSubCmds= cli_cmds_create();
  
    cli_register_net_node_link(pSubCmds);
  cli_register_net_node_route(pSubCmds);
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
  cli_params_add2(pParams, "<addr>", NULL, cli_net_enum_nodes);
  cli_cmds_add(pSubCmds, cli_cmd_create("ping",
					cli_net_node_ping,
					NULL, pParams));
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
  cli_register_net_node_show(pSubCmds);
  cli_register_net_node_tunnel(pSubCmds);
#ifdef OSPF_SUPPORT
  cli_register_net_node_ospf(pSubCmds);
#endif
  pParams= cli_params_create();
  cli_params_add2(pParams, "<addr>", NULL, cli_net_enum_nodes);
  return cli_cmds_add(pCmds, cli_cmd_create_ctx("node",
						cli_ctx_create_net_node,
						cli_ctx_destroy_net_node,
						pSubCmds, pParams));
}

// ----- cli_register_net_subnet --------------------------------------
int cli_register_net_subnet(SCliCmds * pCmds)
{
  SCliCmds * pSubCmds;
  SCliParams * pParams;

  pSubCmds= cli_cmds_create();
#ifdef OSPF_SUPPORT  
  cli_register_net_subnet_ospf(pSubCmds);
#endif
  pParams= cli_params_create();
  cli_params_add(pParams, "<prefix>", NULL);
  return cli_cmds_add(pCmds, cli_cmd_create_ctx("subnet",
						cli_ctx_create_net_subnet,
						cli_ctx_destroy_net_subnet,
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

// ----- cli_register_net_show --------------------------------------
/**
 *
 */
int cli_register_net_show(SCliCmds * pCmds)
{
  SCliCmds * pSubCmds;
  SCliParams * pParams;

  pSubCmds= cli_cmds_create();
  pParams= cli_params_create();
  cli_params_add(pParams, "<prefix|*>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("nodes", cli_net_show_nodes,
					NULL, pParams));
  
  cli_cmds_add(pSubCmds, cli_cmd_create("subnets", cli_net_show_subnets,
					NULL, NULL));
  return cli_cmds_add(pCmds, cli_cmd_create("show", NULL,
					    pSubCmds, NULL));
}

// ----- cli_register_net_tm ----------------------------------------
int cli_register_net_tm(SCliCmds * pCmds)
{
  SCliCmds * pSubCmds;
  SCliParams * pParams;

  pSubCmds= cli_cmds_create();
  pParams= cli_params_create();
  cli_params_add(pParams, "<file>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("load", cli_net_tm_load,
					NULL, pParams));
  return cli_cmds_add(pCmds, cli_cmd_create("tm", NULL, pSubCmds, NULL));
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
  cli_register_net_domain(pCmds);
  cli_register_net_link(pCmds);
  cli_register_net_links(pCmds);
  cli_register_net_ntf(pCmds);
  cli_register_net_node(pCmds);
  cli_register_net_subnet(pCmds);
  cli_register_net_options(pCmds);
  cli_register_net_show(pCmds);
  cli_register_net_tm(pCmds);
//#ifdef OSPF_SUPPORT
//  cli_register_net_ospf(pCmds);
//#endif
  cli_register_cmd(pCli, cli_cmd_create("net", NULL, pCmds, NULL));
  return 0;
}
