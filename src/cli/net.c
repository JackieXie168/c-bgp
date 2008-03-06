// ==================================================================
// @(#)net.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 15/07/2003
// @lastdate 25/02/2008
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
#include <cli/enum.h>
#include <cli/net.h>
#include <cli/net_domain.h>
#include <cli/net_node.h>
#include <cli/net_ospf.h>
#include <net/error.h>
#include <net/export.h>
#include <net/node.h>
#include <net/ntf.h>
#include <net/prefix.h>
#include <net/subnet.h>
#include <net/igp.h>
#include <net/igp_domain.h>
#include <net/network.h>
#include <net/ospf.h>
#include <net/ospf_rt.h>
#include <net/tm.h>
#include <net/util.h>
#include <ui/rl.h>

// -----[ _cli_net_subnet_by_prefix ]--------------------------------
static inline SNetSubnet * _cli_net_subnet_by_prefix(char * pcAddr)
{
  SPrefix sPrefix;

  if (str2prefix(pcAddr, &sPrefix))
    return NULL;
  return network_find_subnet(sPrefix);
}

// -----[ _link_from_context ]---------------------------------------
static inline SNetLink * _link_from_context(SCliContext * pContext)
{
  SNetLink * pLink= cli_context_get_item_at_top(pContext);
  assert(pLink != NULL);
  return pLink;
}


/////////////////////////////////////////////////////////////////////
//
// CLI COMMANDS
//
/////////////////////////////////////////////////////////////////////

// ----- cli_net_add_node -------------------------------------------
/**
 * context: {}
 * tokens: {addr}
 */
int cli_net_add_node(SCliContext * pContext, SCliCmd * pCmd)
{
  net_addr_t tAddr;
  SNetNode * pNode;
  int iResult;

  // Node address ?
  if (str2address(tokens_get_string_at(pCmd->pParamValues, 0), &tAddr)) {
    cli_set_user_error(cli_get(), "could not add node (invalid address)");
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Create new node
  pNode= node_create(tAddr);

  // Add node
  iResult= network_add_node(pNode);
  if (iResult != NET_SUCCESS) {
    node_destroy(&pNode);
    cli_set_user_error(cli_get(), "could not add node (%s)",
		       network_strerror(iResult));
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
  int iResult;

  // Subnet prefix
  if ((str2prefix(tokens_get_string_at(pCmd->pParamValues, 0), &sPrefix))) {
    cli_set_user_error(cli_get(), "could not add subnet (invalid prefix)");
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Check the prefix length (!= 32)
  // THE CHECK FOR PREFIX LENGTH SHOULD BE MOVED TO THE NETWORK MGMT PART.
  if (sPrefix.uMaskLen == 32) {
    cli_set_user_error(cli_get(), "could not add subnet (invalid subnet)");
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Subnet type: transit / stub ?
  pcType = tokens_get_string_at(pCmd->pParamValues, 1);
  if (strcmp(pcType, "transit") == 0)
    uType = NET_SUBNET_TYPE_TRANSIT;
  else if (strcmp(pcType, "stub") == 0)
    uType = NET_SUBNET_TYPE_STUB;
  else {
    cli_set_user_error(cli_get(), "wrong subnet type");
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Create new subnet
  pSubnet= subnet_create(sPrefix.tNetwork, sPrefix.uMaskLen, uType);

  // Add the subnet
  iResult= network_add_subnet(pSubnet);
  if (iResult != NET_SUCCESS) {
    subnet_destroy(&pSubnet);
    cli_set_user_error(cli_get(), "could not add subnet (%s)",
		       network_strerror(iResult));
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// ----- cli_net_add_link -------------------------------------------
/**
 * context: {}
 * tokens: {addr-src, prefix-dst, delay}
 * options: [--depth, --capacity]
 */
int cli_net_add_link(SCliContext * pContext, SCliCmd * pCmd)
{
  SNetNode * pNodeSrc;
  unsigned int uValue;
  char * pcNodeSrcAddr, * pcDest;
  SNetDest sDest;
  int iResult;
  char * pcValue;
  net_link_delay_t tDelay= 0;
  uint8_t tDepth= 1;
  net_link_load_t tCapacity= 0;
  SNetIface * pIface;
  EIfaceDir eDir;

  // Get optional link depth
  pcValue= cli_options_get_value(pCmd->pOptions, "depth");
  if (pcValue != NULL) {
    if ((str_as_uint(pcValue, &uValue) < 0) ||
	(uValue > NET_LINK_MAX_DEPTH)) {
      cli_set_user_error(cli_get(), "invalid depth \"%s\"", pcValue);
      return CLI_ERROR_COMMAND_FAILED;
    }
    tDepth= uValue;
  }
  
  // Get optional link capacity
  pcValue= cli_options_get_value(pCmd->pOptions, "bw");
  if (pcValue != NULL) {
    if (str_as_uint(pcValue, &uValue) < 0) {
      cli_set_user_error(cli_get(), "invalid capacity \"%s\"", pcValue);
      return CLI_ERROR_COMMAND_FAILED;
    }
    tCapacity= uValue;
  }
  
  // Get source node
  pcNodeSrcAddr= tokens_get_string_at(pCmd->pParamValues, 0);
  pNodeSrc= cli_net_node_by_addr(pcNodeSrcAddr);
  if (pNodeSrc == NULL) {
    cli_set_user_error(cli_get(), "could not find node \"%s\"", pcNodeSrcAddr);
    return CLI_ERROR_COMMAND_FAILED;
  }
  
  // Get destination: can be a node (router) or a subnet
  pcDest= tokens_get_string_at(pCmd->pParamValues, 1);
  if ((ip_string_to_dest(pcDest, &sDest) < 0) ||
      (sDest.tType == NET_DEST_ANY)) {
    cli_set_user_error(cli_get(), "invalid destination \"%s\".", pcDest);
    return CLI_ERROR_COMMAND_FAILED;
  }
  
  // Get delay
  if (tokens_get_uint_at(pCmd->pParamValues, 2, &uValue)) {
    cli_set_user_error(cli_get(), "invalid delay %s.",
		       tokens_get_string_at(pCmd->pParamValues, 2));
    return CLI_ERROR_COMMAND_FAILED;
  }
  tDelay= uValue;
  
  // Add link (RTR / PTMP)
  if (sDest.tType == NET_DEST_ADDRESS) {
    SNetNode * pNodeDst= network_find_node(sDest.uDest.tAddr);
    if (pNodeDst == NULL) {
      cli_set_user_error(cli_get(), "tail-end \"%s\" does not exist.", pcDest);
      return CLI_ERROR_COMMAND_FAILED;
    }
    eDir= BIDIR;
    iResult= net_link_create_rtr(pNodeSrc, pNodeDst, eDir, &pIface);
  } else {
    SNetSubnet * pSubnet= network_find_subnet(sDest.uDest.sPrefix);
    if (pSubnet == NULL) {
      cli_set_user_error(cli_get(), "tail-end \"%s\" does not exist.", pcDest);
      return CLI_ERROR_COMMAND_FAILED;
    }
    eDir= UNIDIR;
    iResult= net_link_create_ptmp(pNodeSrc, pSubnet,
				  sDest.uDest.sPrefix.tNetwork,
				  &pIface);
  }
  if (iResult != NET_SUCCESS) {
    cli_set_user_error(cli_get(), "could not add link %s -> %s (%s)",
		       pcNodeSrcAddr, pcDest, network_strerror(iResult));
    return CLI_ERROR_COMMAND_FAILED;
  }

  iResult= net_link_set_phys_attr(pIface, tDelay, tCapacity, eDir);
  if (iResult != NET_SUCCESS) {
    cli_set_user_error(cli_get(), "could not set physical attributes (%s)",
		       network_strerror(iResult));
    return CLI_ERROR_COMMAND_FAILED;
  }  

  return CLI_SUCCESS;
}

// -----[ cli_net_add_linkptp ]--------------------------------------
/**
 * context: {}
 * tokens: {src-addr, src-iface-id, dst-addr, dst-iface-id}
 */
int cli_net_add_linkptp(SCliContext * pContext, SCliCmd * pCmd)
{
  SNetNode * pSrcNode;
  SNetNode * pDstNode;
  net_iface_id_t tSrcIfaceID;
  net_iface_id_t tDstIfaceID;
  char * pcTmp;
  int iResult;
  SNetIface * pIface;

  // Get source node
  pcTmp= tokens_get_string_at(pCmd->pParamValues, 0);
  pSrcNode= cli_net_node_by_addr(pcTmp);
  if (pSrcNode == NULL) {
    cli_set_user_error(cli_get(), "could not find node \"%s\"", pcTmp);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Get source address
  pcTmp= tokens_get_string_at(pCmd->pParamValues, 1);
  iResult= net_iface_str2id(pcTmp, &tSrcIfaceID);
  if (iResult != NET_SUCCESS) {
    cli_set_user_error(cli_get(), "invalid interface id \"%\"", pcTmp);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Get destination node
  pcTmp= tokens_get_string_at(pCmd->pParamValues, 2);
  pDstNode= cli_net_node_by_addr(pcTmp);
  if (pDstNode == NULL) {
    cli_set_user_error(cli_get(), "could not find node \"%s\"", pcTmp);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // get destination address
  pcTmp= tokens_get_string_at(pCmd->pParamValues, 3);
  iResult= net_iface_str2id(pcTmp, &tDstIfaceID);
  if (iResult != NET_SUCCESS) {
    cli_set_user_error(cli_get(), "invalid interface id \"%\"", pcTmp);
    return CLI_ERROR_COMMAND_FAILED;
  }
  
  // Create link
  iResult= net_link_create_ptp(pSrcNode, tSrcIfaceID,
			       pDstNode, tDstIfaceID,
			       BIDIR, &pIface);
  if (iResult != NET_SUCCESS) {
    cli_set_user_error(cli_get(), "could not create ptp link "
		       "from %s (%s) to %s (%s) (%s)",
		       tokens_get_string_at(pCmd->pParamValues, 0),
		       tokens_get_string_at(pCmd->pParamValues, 1),
		       tokens_get_string_at(pCmd->pParamValues, 2),
		       tokens_get_string_at(pCmd->pParamValues, 3),
		       network_strerror(iResult));
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
  SNetIface * pIface = NULL;

  pcNodeSrcAddr= tokens_get_string_at(pContext->pCmd->pParamValues, 0);
  pNodeSrc= cli_net_node_by_addr(pcNodeSrcAddr);
  if (pNodeSrc == NULL) {
    cli_set_user_error(cli_get(), "unable to find node \"%s\"",
		       pcNodeSrcAddr);
    return CLI_ERROR_CTX_CREATE;
  }

  pcVertexDstPrefix= tokens_get_string_at(pContext->pCmd->pParamValues, 1);
  if (ip_string_to_dest(pcVertexDstPrefix, &sDest) < 0 ||
      sDest.tType == NET_DEST_ANY) {
    cli_set_user_error(cli_get(), "destination id is wrong \"%s\"",
		       pcVertexDstPrefix);
    return CLI_ERROR_CTX_CREATE;
  }

  pIface= node_find_iface(pNodeSrc, net_dest_to_prefix(sDest));
  if (pIface == NULL) {
    cli_set_user_error(cli_get(), "unable to find link %s -> %s",
		       pcNodeSrcAddr, pcVertexDstPrefix);
    return CLI_ERROR_COMMAND_FAILED;
  }

  *ppItem= pIface;

  return CLI_SUCCESS;
}

// ----- cli_ctx_destroy_net_link -----------------------------------
void cli_ctx_destroy_net_link(void ** ppItem)
{
}

// -----[ cli_net_export ]-------------------------------------------
/**
 * Context: {}
 * tokens: {file}
 */
int cli_net_export(SCliContext * pContext, SCliCmd * pCmd)
{
  char * pcFileName;

  // Get name of the output file
  pcFileName= tokens_get_string_at(pCmd->pParamValues, 0);
  
  if (net_export_file(pcFileName) != NET_SUCCESS) {
    cli_set_user_error(cli_get(), "could not export to \"%s\"",
		       pcFileName);
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
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
    cli_set_user_error(cli_get(), ": unable to load NTF file \"%s\"",
		       pcFileName);
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// ----- cli_net_link_up --------------------------------------------
/**
 * context: {link}
 * tokens: {}
 */
int cli_net_link_up(SCliContext * pContext, SCliCmd * pCmd)
{
  SNetLink * pLink= _link_from_context(pContext);

  net_iface_set_enabled(pLink, 1);

  return CLI_SUCCESS;
}

// ----- cli_net_link_down ------------------------------------------
/**
 * context: {link}
 * tokens: {}
 */
int cli_net_link_down(SCliContext * pContext, SCliCmd * pCmd)
{
  SNetLink * pLink= _link_from_context(pContext);

  net_iface_set_enabled(pLink, 0);

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
  SNetLink * pLink= _link_from_context(pContext);
  //SNetNode * pNode;
  unsigned int uWeight;
  net_igp_weight_t tWeight;
  net_tos_t tTOS= 0;
  char * pcValue;
  int iBidir= 0;
  int iResult;

  // Get optional TOS
  pcValue= cli_options_get_value(pCmd->pOptions, "tos");
  if (pcValue != NULL)
    if (!str2tos(pcValue, &tTOS)) {
      cli_set_user_error(cli_get(), "invalid TOS \"%s\"", pcValue);
      return CLI_ERROR_COMMAND_FAILED;
    }

  // Check option --bidir
  if (cli_options_has_value(pCmd->pOptions, "bidir")) {
    if (pLink->tType != NET_IFACE_RTR) {
      cli_set_user_error(cli_get(), ": --bidir only works with ptp links");
      return CLI_ERROR_COMMAND_FAILED;
    }
    iBidir= 1;
  }

  // Get new IGP weight
  if (tokens_get_uint_at(pCmd->pParamValues, 0, &uWeight) != 0) {
    cli_set_user_error(cli_get(), "invalid weight \"%s\"",
		       tokens_get_string_at(pCmd->pParamValues, 0));
    return CLI_ERROR_COMMAND_FAILED;
  }
  tWeight= (net_igp_weight_t) uWeight;

  // Change link weight
  iResult= net_iface_set_metric(pLink, tTOS, tWeight, iBidir);
  if (iResult != NET_SUCCESS) {
    cli_set_user_error(cli_get(), "cannot set metric (%s)",
		       network_strerror(iResult));
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// ----- cli_net_link_show_info -------------------------------------
/**
 * context: {link}
 * tokens: {}
 */
int cli_net_link_show_info(SCliContext * pContext, SCliCmd * pCmd)
{
  SNetLink * pLink= _link_from_context(pContext);

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
  network_ifaces_load_clear();
  return CLI_SUCCESS;
}

// ----- cli_ctx_create_net_subnet ----------------------------------
int cli_ctx_create_net_subnet(SCliContext * pContext, void ** ppItem)
{
  char * pcSubnetPfx;
  SNetSubnet * pSubnet;

  pcSubnetPfx= tokens_get_string_at(pContext->pCmd->pParamValues, 0);
  pSubnet = _cli_net_subnet_by_prefix(pcSubnetPfx);
  
  if (pSubnet == NULL) {
    cli_set_user_error(cli_get(), "unable to find subnet \"%s\"",
		       pcSubnetPfx);
    return CLI_ERROR_CTX_CREATE;
  }
  *ppItem= pSubnet;
  return CLI_SUCCESS;
}

// ----- cli_ctx_destroy_net_subnet -----------------------------------
void cli_ctx_destroy_net_subnet(void ** ppItem)
{
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
    cli_set_user_error(cli_get(), "invalid value for max-hops (%s)",
		       tokens_get_string_at(pCmd->pParamValues, 0));
    return CLI_ERROR_COMMAND_FAILED;
  }

  if (uMaxHops > 255) {
    cli_set_user_error(cli_get(), "maximum number of hops is 255");
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

// ----- cli_net_show_subnets ---------------------------------------
/**
 * Display all subnets matching the given criterion, which is a
 * prefix or an asterisk (meaning "all subnets").
 *
 * context: {}
 * tokens: {}
 */
int cli_net_show_subnets(SCliContext * pContext, SCliCmd * pCmd)
{
  network_dump_subnets(pLogOut, network_get());
  return CLI_SUCCESS;
}

// ----- cli_net_traffic_load ---------------------------------------
/**
 * context: {}
 * tokens: {file}
 */
int cli_net_traffic_load(SCliContext * pContext, SCliCmd * pCmd)
{
  int iResult;
  char * pcFileName;

  pcFileName= tokens_get_string_at(pCmd->pParamValues, 0);
  iResult= net_tm_load(pcFileName);
  if (iResult != NET_TM_SUCCESS) {
    cli_set_user_error(cli_get(), "could not load traffic matrix \"%s\" (%s)",
		       pcFileName, net_tm_strerror(iResult));
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// ----- cli_net_traffic_save ---------------------------------------
/**
 * context: {}
 * tokens: {file}
 */
int cli_net_traffic_save(SCliContext * pContext, SCliCmd * pCmd)
{
  char * pcFileName= NULL;
  SLogStream * pStream= pLogOut;
  int iResult;

  // Get the optional parameter
  if ((pCmd->pParamValues != NULL) &&
      (tokens_get_num(pCmd->pParamValues) > 0)) {
    pcFileName= tokens_get_string_at(pCmd->pParamValues, 0);
    log_printf(pLogOut, "filename: %s\n", pcFileName);
    pStream= log_create_file(pcFileName);
    if (pStream == NULL) {
      cli_set_user_error(cli_get(), "could not create \"%d\"",
			 pcFileName);
      return CLI_ERROR_COMMAND_FAILED;
    }
  }

  iResult= network_links_save(pStream);

  if (pStream != pLogOut)
    log_destroy(&pStream);

  if (iResult != 0) {
    cli_set_user_error(cli_get(), "could not save traffic matrix");
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}


/////////////////////////////////////////////////////////////////////
//
// CLI REGISTRATION
//
/////////////////////////////////////////////////////////////////////

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
  cli_params_add2(pParams, "<addr-src>", NULL, cli_enum_net_nodes_addr);
  cli_params_add2(pParams, "<addr-dst>", NULL, cli_enum_net_nodes_addr);
  cli_params_add(pParams, "<delay>", NULL);
  pCmd= cli_cmd_create("link", cli_net_add_link, NULL, pParams);
  cli_cmd_add_option(pCmd, "bw", NULL);
  cli_cmd_add_option(pCmd, "depth", NULL);
  cli_cmds_add(pSubCmds, pCmd);

  pParams= cli_params_create();
  cli_params_add2(pParams, "<addr-src>", NULL, cli_enum_net_nodes_addr);
  cli_params_add(pParams, "<iface-id>", NULL);
  cli_params_add2(pParams, "<addr-dst>", NULL, cli_enum_net_nodes_addr);
  cli_params_add(pParams, "<iface-id>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("link-ptp", cli_net_add_linkptp,
					NULL, pParams));

  return cli_cmds_add(pCmds, cli_cmd_create("add", NULL, pSubCmds, NULL));
}

// -----[ cli_register_net_export ]----------------------------------
int cli_register_net_export(SCliCmds * pCmds)
{
  SCliParams * pParams= cli_params_create();
  cli_params_add(pParams, "<file>", NULL);
  return cli_cmds_add(pCmds, cli_cmd_create("export", cli_net_export,
					    NULL, pParams));
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
  cli_register_net_link_show(pSubCmds);
  pParams= cli_params_create();
  cli_params_add2(pParams, "<addr-src>", NULL, cli_enum_net_nodes_addr);
  cli_params_add2(pParams, "<addr-dst>", NULL, cli_enum_net_nodes_addr);
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
  cli_params_add_file(pParams, "<filename>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("load",
					cli_net_ntf_load,
					NULL, pParams));
  return cli_cmds_add(pCmds, cli_cmd_create_ctx("ntf", NULL, NULL,
						pSubCmds, NULL));
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

// ----- cli_register_net_traffic -----------------------------------
int cli_register_net_traffic(SCliCmds * pCmds)
{
  SCliCmds * pSubCmds;
  SCliParams * pParams;

  pSubCmds= cli_cmds_create();
  pParams= cli_params_create();
  cli_params_add(pParams, "<file>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("load", cli_net_traffic_load,
					NULL, pParams));
  pParams= cli_params_create();
  cli_params_add_vararg(pParams, "<file>", 1, NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("save", cli_net_traffic_save,
					NULL, pParams));
  return cli_cmds_add(pCmds, cli_cmd_create("traffic", NULL, pSubCmds, NULL));
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
  cli_register_net_export(pCmds);
  cli_register_net_link(pCmds);
  cli_register_net_links(pCmds);
  cli_register_net_ntf(pCmds);
  cli_register_net_node(pCmds);
  cli_register_net_subnet(pCmds);
  cli_register_net_options(pCmds);
  cli_register_net_show(pCmds);
  cli_register_net_traffic(pCmds);
//#ifdef OSPF_SUPPORT
//  cli_register_net_ospf(pCmds);
//#endif
  cli_register_cmd(pCli, cli_cmd_create("net", NULL, pCmds, NULL));
  return 0;
}
