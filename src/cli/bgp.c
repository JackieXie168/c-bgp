// ==================================================================
// @(#)bgp.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be),
// @author Sebastien Tandel (standel@info.ucl.ac.be)
// @date 15/07/2003
// @lastdate 10/01/2008
// ==================================================================

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <bgp/as.h>
#include <bgp/bgp_assert.h>
#include <bgp/bgp_debug.h>
#include <bgp/domain.h>
#include <bgp/dp_rules.h>
#include <bgp/filter.h>
#include <bgp/filter_parser.h>
#include <bgp/predicate_parser.h>
#include <bgp/message.h>
#include <bgp/mrtd.h>
#include <bgp/peer.h>
#include <bgp/qos.h>
#include <bgp/record-route.h>
#include <bgp/route_map.h>
#include <bgp/tie_breaks.h>

#include <cli/bgp.h>
#include <cli/bgp_topology.h>
#include <cli/common.h>
#include <cli/enum.h>
#include <cli/net.h>
#include <ui/rl.h>
#include <libgds/cli_ctx.h>
#include <libgds/log.h>
#include <libgds/memory.h>
#include <libgds/str_util.h>
#include <net/error.h>
#include <net/node.h>
#include <net/prefix.h>
#include <net/record-route.h>
#include <net/util.h>
#include <string.h>

// ----- cli_bgp_add_router -----------------------------------------
/**
 * This function adds a BGP protocol instance to the given node
 * (identified by its address).
 *
 * context: {}
 * tokens: {addr, as-num}
 */
int cli_bgp_add_router(SCliContext * pContext, SCliCmd * pCmd)
{
  char * pcNodeAddr;
  SNetNode * pNode;
  SBGPRouter * pRouter;
  unsigned int uASNum;
  int iResult;

  // Find node
  pcNodeAddr= tokens_get_string_at(pCmd->pParamValues, 1);
  pNode= cli_net_node_by_addr(pcNodeAddr);
  if (pNode == NULL) {
    cli_set_user_error(cli_get(), "invalid node address \"%s\"",
		       pcNodeAddr);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Check AS-Number
  if (tokens_get_uint_at(pCmd->pParamValues, 0, &uASNum)
      || (uASNum > MAX_AS)) {
    cli_set_user_error(cli_get(), "invalid AS_Number");
    return CLI_ERROR_COMMAND_FAILED;
  }

  pRouter= bgp_router_create(uASNum, pNode);

  // Register BGP protocol into the node
  iResult= node_register_protocol(pNode, NET_PROTOCOL_BGP, pRouter,
				  (FNetNodeHandlerDestroy) bgp_router_destroy,
				  bgp_router_handle_message);
  if (iResult != NET_SUCCESS) {
    cli_set_user_error(cli_get(), "could not register BGP (%s)",
		       pcNodeAddr, network_strerror(iResult));
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// ----- cli_bgp_assert_peerings ------------------------------------
/**
 * This function checks that all the BGP peerings defined in BGP
 * instances are valid, i.e. the peer exists and is reachable.
 */
int cli_bgp_assert_peerings(SCliContext * pContext, SCliCmd * pCmd)
{
  // Test the validity of peerings in all BGP instances
  if (bgp_assert_peerings()) {
    cli_set_user_error(cli_get(), "peerings assertion failed.");
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// ----- cli_bgp_assert_reachability --------------------------------
/**
 * This function checks that all the BGP instances have at leat one
 * BGP route towards all the advertised networks.
 *
 * context: {}
 * tokens: {}
 */
int cli_bgp_assert_reachability(SCliContext * pContext, SCliCmd * pCmd)
{
  // Test the reachability of all advertised networks from all BGP
  // instances
  if (bgp_assert_reachability()) {
    cli_set_user_error(cli_get(), "reachability assertion failed.");
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// ----- cli_ctx_create_bgp_domain ----------------------------------
/**
 * Create a context for the given BGP domain.
 *
 * context: {}
 * tokens: {as-number}
 */
int cli_ctx_create_bgp_domain(SCliContext * pContext, void ** ppItem)
{
  unsigned int uASNumber;

  /* Get BGP domain from the top of the context */
  if (tokens_get_uint_at(pContext->pCmd->pParamValues, 0, &uASNumber) != 0) {
    cli_set_user_error(cli_get(), "invalid AS number \"%s\"",
		       tokens_get_string_at(pContext->pCmd->pParamValues, 0));
    return CLI_ERROR_CTX_CREATE;
  }

  /* Check that the BGP domain exists */
  if (!exists_bgp_domain((uint16_t) uASNumber)) {
    cli_set_user_error(cli_get(), "domain \"%u\" does not exist.",
		       uASNumber);
    return CLI_ERROR_CTX_CREATE;
  }

  *ppItem= get_bgp_domain(uASNumber);

  return CLI_SUCCESS;
}

// ----- cli_ctx_destroy_bgp_domain ---------------------------------
void cli_ctx_destroy_bgp_domain(void ** ppItem)
{
}

// ----- cli_bgp_domain_full_mesh -----------------------------------
/**
 * Generate a full-mesh of iBGP sessions between the routers of the
 * domain.
 *
 * context: {as}
 * tokens: {}
 */
int cli_bgp_domain_full_mesh(SCliContext * pContext, SCliCmd * pCmd)
{
  SBGPDomain * pDomain;

  /* Get the domain from the context. */
  pDomain= (SBGPDomain *) cli_context_get_item_at_top(pContext);

  /* Generate the full-mesh of iBGP sessions */
  bgp_domain_full_mesh(pDomain);
  
  return CLI_SUCCESS;
}

// ----- cli_bgp_domain_rescan --------------------------------------
/**
 * Rescan all the routers in the given BGP domain.
 *
 * context: {as}
 * tokens: {}
 */
int cli_bgp_domain_rescan(SCliContext * pContext, SCliCmd * pCmd)
{
  SBGPDomain * pDomain;

  /* Get the domain from the context. */
  pDomain= (SBGPDomain *) cli_context_get_item_at_top(pContext);

  /* Rescan all the routers in the domain. */
  bgp_domain_rescan(pDomain);

  return CLI_SUCCESS;
}

// ----- cli_net_node_recordroute ------------------------------------
/**
 * context: {as}
 * tokens: {prefix|address|*}
 */
int cli_bgp_domain_recordroute(SCliContext * pContext,
				  SCliCmd * pCmd)
{
  SBGPDomain * pDomain;
  char * pcDest;
  SNetDest sDest;
  uint8_t uOptions= 0;

 // Get node from the CLI'scontext
  pDomain= (SBGPDomain *) cli_context_get_item_at_top(pContext);
  if (pDomain == NULL)
    return CLI_ERROR_COMMAND_FAILED;

  // Optional deflection ?
  if (cli_options_has_value(pCmd->pOptions, "deflection"))
    uOptions|= NET_RECORD_ROUTE_OPTION_DEFLECTION;

  // Get destination address
  pcDest= tokens_get_string_at(pCmd->pParamValues, 0);
  if (ip_string_to_dest(pcDest, &sDest)) {
    cli_set_user_error(cli_get(), "invalid prefix|address|* \"%s\"",
		       pcDest);
    return CLI_ERROR_COMMAND_FAILED;
  }

  /* Check that the destination type is adress/prefix */
  if ((sDest.tType != NET_DEST_ADDRESS) &&
      (sDest.tType != NET_DEST_PREFIX)) {
    cli_set_user_error(cli_get(), "can not use this destination");
    return CLI_ERROR_COMMAND_FAILED;
  }

  bgp_domain_dump_recorded_route(pLogOut, pDomain, sDest, uOptions);

  return CLI_SUCCESS;
}

// ----- cli_ctx_create_bgp_route_map --------------------------------
/**
 * context: {} -> {&filter}
 * tokens: {}
 */
int cli_ctx_create_bgp_route_map(SCliContext * pContext, void ** ppItem)
{
  char * pcToken;
  char * pcRouteMapName;
  SFilter ** ppFilter;

  pcToken= tokens_get_string_at(pContext->pCmd->pParamValues, 0);
  
  if (pcToken != NULL) {
    if (route_map_get(pcToken) != NULL) {
      cli_set_user_error(cli_get(), "route-map already exists.",
	      pcToken);
      return CLI_ERROR_CTX_CREATE;
    }

    ppFilter= MALLOC(sizeof(SFilter *));
    *ppFilter= filter_create();

    pcRouteMapName= MALLOC(strlen(pcToken)+1);
    strcpy(pcRouteMapName, pcToken);
    route_map_add(pcRouteMapName, *ppFilter);
    *ppItem = ppFilter;
  } else {
    cli_set_user_error(cli_get(), "missing route-map name.");
    return CLI_ERROR_CTX_CREATE;
  }

  return CLI_SUCCESS;
}

// ----- cli_ctx_destroy_bgp_route_map -------------------------------
/**
 *
 *
 */
void cli_ctx_destroy_bgp_route_map(void ** ppItem)
{
  SFilter ** ppFilter = (SFilter **)*ppItem;
  
  if (ppFilter != NULL)
    FREE(ppFilter);
}

// ----- cli_ctx_create_bgp_router ----------------------------------
/**
 * This function adds the BGP instance of the given node (identified
 * by its address) on the CLI context.
 *
 * context: {} -> {router}
 * tokens: {addr}
 */
int cli_ctx_create_bgp_router(SCliContext * pContext, void ** ppItem)
{
  char * pcNodeAddr;
  SNetNode * pNode;
  SNetProtocol * pProtocol;

  // Find node
  pcNodeAddr= tokens_get_string_at(pContext->pCmd->pParamValues, 0);
  pNode= cli_net_node_by_addr(pcNodeAddr);
  if (pNode == NULL) {
    cli_set_user_error(cli_get(), "invalid node address \"%s\"",
		       pcNodeAddr);
    return CLI_ERROR_CTX_CREATE;
  }

  // Get BGP protocol instance
  pProtocol= protocols_get(pNode->pProtocols, NET_PROTOCOL_BGP);
  if (pProtocol == NULL) {
    cli_set_user_error(cli_get(), "BGP is not supported on node \"%s\"",
		       pcNodeAddr);
    return CLI_ERROR_CTX_CREATE;
  }

  *ppItem= pProtocol->pHandler;
  return CLI_SUCCESS;
}

// ----- cli_ctx_destroy_bgp_router ---------------------------------
void cli_ctx_destroy_bgp_router(void ** ppItem)
{
}

// -----[ cli_bgp_options_autocreate ]-------------------------------
/**
 * context: {}
 * tokens: {on/off}
 */
int cli_bgp_options_autocreate(SCliContext * pContext, SCliCmd * pCmd)
{
  char * pcParam;

  pcParam= tokens_get_string_at(pCmd->pParamValues, 0);
  if (!strcmp(pcParam, "on"))
    BGP_OPTIONS_AUTO_CREATE= 1;
  else if (!strcmp(pcParam, "off"))
    BGP_OPTIONS_AUTO_CREATE= 0;
  else {
    cli_set_user_error(cli_get(), "invalid value \"%s\"", pcParam);
    return CLI_ERROR_COMMAND_FAILED;
  }
  return CLI_SUCCESS;
}

// ----- cli_bgp_options_showmode ------------------------------------
/**
 * Change the BGP route "show" mode.
 *
 * context: {}
 * tokens: {mode, [format]}
 *
 * Where mode is one of "cisco", "mrt" and "custom". If mode is
 * "custom", a format specifier must be provided as an additional
 * argument.
 *
 * See function bgp_route_dump_custom() [src/bgp/route.c] for more
 * details on the format specifier string.
 */
int cli_bgp_options_showmode(SCliContext * pContext, SCliCmd * pCmd)
{
  char * pcParam;
  uint8_t uFormat;

  // Get mode
  pcParam= tokens_get_string_at(pCmd->pParamValues, 0);
  if (route_str2format(pcParam, &uFormat) != 0) {
    cli_set_user_error(cli_get(), "unknown BGP show-mode \"%s\"",
		       pcParam);
    return CLI_ERROR_COMMAND_FAILED;
  }

  if ((uFormat == BGP_ROUTES_OUTPUT_CISCO) ||
      (uFormat == BGP_ROUTES_OUTPUT_MRT_ASCII)) {
    if (tokens_get_num(pCmd->pParamValues) > 1) {
      cli_set_user_error(cli_get(), "no additional argument allowed for mode \"%s\"", tokens_get_string_at(pCmd->pParamValues, 0));
      return CLI_ERROR_COMMAND_FAILED;
    }
    BGP_OPTIONS_SHOW_MODE= uFormat;
  } else {
    // Check that an additional argument (format) is provided
    if (tokens_get_num(pCmd->pParamValues) != 2) {
      cli_set_user_error(cli_get(), "mode \"custom\" requires a format specifier");
      return CLI_ERROR_COMMAND_FAILED;
    }

    // Get format specifier string and update global option
    if (BGP_OPTIONS_SHOW_FORMAT != NULL)
      str_destroy(&BGP_OPTIONS_SHOW_FORMAT);
    BGP_OPTIONS_SHOW_FORMAT=
      str_create(tokens_get_string_at(pCmd->pParamValues, 1));
    BGP_OPTIONS_SHOW_MODE= ROUTE_SHOW_CUSTOM;
  }

  return CLI_SUCCESS;
}

// ----- cli_bgp_options_tiebreak ------------------------------------
/**
 * context: {}
 * tokens: {function}
 */
/*int cli_bgp_options_tiebreak(SCliContext * pContext, SCliCmd * pCmd)
{
  char * pcParam;

  pcParam= tokens_get_string_at(pTokens, 0);
  if (!strcmp(pcParam, "hash"))
    BGP_OPTIONS_TIE_BREAK= tie_break_hash;
  else if (!strcmp(pcParam, "low-isp"))
    BGP_OPTIONS_TIE_BREAK= tie_break_low_ISP;
  else if (!strcmp(pcParam, "high-isp"))
    BGP_OPTIONS_TIE_BREAK= tie_break_high_ISP;
  else {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: unknown tiebreak function \"%s\"\n", pcParam);
    return CLI_ERROR_COMMAND_FAILED;
  }
  return CLI_SUCCESS;
  }*/

// ----- cli_bgp_options_med ----------------------------------------
/**
 * context: {}
 * tokens: {med-type}
 */
int cli_bgp_options_med(SCliContext * pContext, SCliCmd * pCmd)
{
  char * pcMedType= tokens_get_string_at(pCmd->pParamValues, 0);
  
  if (!strcmp(pcMedType, "deterministic")) {
    BGP_OPTIONS_MED_TYPE= BGP_MED_TYPE_DETERMINISTIC;
  } else if (!strcmp(pcMedType, "always-compare")) {
    BGP_OPTIONS_MED_TYPE= BGP_MED_TYPE_ALWAYS_COMPARE;
  } else {
    cli_set_user_error(cli_get(), "unknown med-type \"%s\"", pcMedType);
    return CLI_ERROR_COMMAND_FAILED;
  }
  return CLI_SUCCESS;
}

// ----- cli_bgp_options_localpref ----------------------------------
/**
 * context: {}
 * tokens: {local-pref}
 */
int cli_bgp_options_localpref(SCliContext * pContext, SCliCmd * pCmd)
{
  unsigned long int ulLocalPref;

  if (tokens_get_ulong_at(pCmd->pParamValues, 0, &ulLocalPref)) {
    cli_set_user_error(cli_get(), "invalid default local-pref \"%s\"",
		       tokens_get_string_at(pCmd->pParamValues, 0));
    return CLI_ERROR_COMMAND_FAILED;
  }
  BGP_OPTIONS_DEFAULT_LOCAL_PREF= ulLocalPref;
  return CLI_SUCCESS;
}

// ----- cli_bgp_options_msgmonitor ---------------------------------
/**
 * context: {}
 * tokens: {output-file|"-"}
 */
int cli_bgp_options_msgmonitor(SCliContext * pContext, SCliCmd * pCmd)
{
  char * pcParam;

  pcParam= tokens_get_string_at(pCmd->pParamValues, 0);
  if (strcmp(pcParam, "-") == 0) {
    bgp_msg_monitor_close();
  } else {
    bgp_msg_monitor_open(pcParam);
  }
  return CLI_SUCCESS;
}

// ----- cli_bgp_options_qosaggrlimit -------------------------------
/**
 * context: {}
 * tokens: {limit}
 */
#ifdef BGP_QOS
int cli_bgp_options_qosaggrlimit(SCliContext * pContext,
				 SCliCmd * pCmd)
{
  unsigned int uLimit;

  if (tokens_get_uint_at(pCmd->pParamValues, 0, &uLimit))
    return CLI_ERROR_COMMAND_FAILED;
  BGP_OPTIONS_QOS_AGGR_LIMIT= uLimit;
  return CLI_SUCCESS;
}
#endif

// ----- cli_bgp_options_walton_convergence ------------------------
/**
 *
 */
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
int cli_bgp_options_walton_convergence(SCliContext * pContext,
				      SCliCmd * pCmd)
{
  char * pcParam;

  pcParam= tokens_get_string_at(pCmd->pParamValues, 0);
  if (!strcmp(pcParam, "best"))
    BGP_OPTIONS_WALTON_CONVERGENCE_ON_BEST = 1;
  else if (!strcmp(pcParam, "all"))
    BGP_OPTIONS_WALTON_CONVERGENCE_ON_BEST = 0;
  else {
    cli_set_user_error(cli_get(), "invalid value \"%s\"", pcParam);
    return CLI_ERROR_COMMAND_FAILED;
  }
  return CLI_SUCCESS;
}
#endif

// ----- cli_bgp_options_advertise_external_best -------------------
/**
 * context : {}
 * tokens: {}
 */
#ifdef __EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
int cli_bgp_options_advertise_external_best(SCliContext * pContext,
					SCliCmd * pCmd)
{
  char * pcParam;

  pcParam= tokens_get_string_at(pCmd->pParamValues, 0);
  if (!strcmp(pcParam, "on"))
    BGP_OPTIONS_ADVERTISE_EXTERNAL_BEST= 1;
  else if (!strcmp(pcParam, "off"))
    BGP_OPTIONS_ADVERTISE_EXTERNAL_BEST= 0;
  else {
    cli_set_user_error(cli_get(), "invalid value \"%s\"", pcParam);
    return CLI_ERROR_COMMAND_FAILED;
  }
  return CLI_SUCCESS;
}
#endif

// ----- cli_bgp_router_set_clusterid -------------------------------
/**
 * This function changes the router's cluster-id. This also allows
 * route-reflection in this router.
 *
 * context: {router}
 * tokens: {cluster-id}
 */
int cli_bgp_router_set_clusterid(SCliContext * pContext,
				 SCliCmd * pCmd)
{
  SBGPRouter * pRouter;
  unsigned int uClusterID;

  // Get the BGP instance from the context
  pRouter= (SBGPRouter *) cli_context_get_item_at_top(pContext);

  // Get the cluster-id
  if (tokens_get_uint_at(pCmd->pParamValues, 0, &uClusterID)) {
    cli_set_user_error(cli_get(), "invalid cluster-id \"%s\"",
		       tokens_get_string_at(pCmd->pParamValues, 0));
    return CLI_ERROR_COMMAND_FAILED;
  }

  pRouter->tClusterID= uClusterID;
  pRouter->iRouteReflector= 1;
  return CLI_SUCCESS;
}

// ----- cli_bgp_router_debug_dp ------------------------------------
/**
 * context: {router}
 * tokens: {prefix}
 */
int cli_bgp_router_debug_dp(SCliContext * pContext, SCliCmd * pCmd)
{
  SBGPRouter * pRouter;
  char * pcPrefix;
  SPrefix sPrefix;

  // Get the BGP router from the context
  pRouter= (SBGPRouter *) cli_context_get_item_at_top(pContext);

  // Get the prefix
  pcPrefix= tokens_get_string_at(pCmd->pParamValues, 0);
  if (str2prefix(pcPrefix, &sPrefix) < 0) {
    cli_set_user_error(cli_get(), "invalid prefix \"%s\"", pcPrefix);
    return CLI_ERROR_COMMAND_FAILED;
  }

  bgp_debug_dp(pLogOut, pRouter, sPrefix);

  return CLI_SUCCESS;
}

// ----- cli_bgp_router_load_rib ------------------------------------
/**
 * This function loads an MRTD table dump into the given BGP
 * instance.
 *
 * context: {router}
 * tokens: {file}
 * options: {--autoconf,--format,--force,--summary}
 */
int cli_bgp_router_load_rib(SCliContext * pContext,
			    SCliCmd * pCmd)
{
  char * pcFileName;
  char * pcValue;
  SBGPRouter * pRouter;
  uint8_t tFormat= BGP_ROUTES_INPUT_MRT_ASC;
  uint8_t tOptions= 0;

  // Get the BGP instance from the context
  pRouter= (SBGPRouter *) cli_context_get_item_at_top(pContext);

  // Get the MRTD file name
  pcFileName= tokens_get_string_at(pCmd->pParamValues, 0);

  // Get the optional format
  pcValue= cli_options_get_value(pCmd->pOptions, "format");
  if (pcValue != NULL) {
    if (bgp_routes_str2format(pcValue, &tFormat) != 0) {
      cli_set_user_error(cli_get(), "invalid input format \"%s\"",
			 pcValue);
      return CLI_ERROR_COMMAND_FAILED;
    }
  }

  // Get option --autoconf ?
  if (cli_options_has_value(pCmd->pOptions, "autoconf"))
    tOptions|= BGP_ROUTER_LOAD_OPTIONS_AUTOCONF;

  // Get option --force ?
  if (cli_options_has_value(pCmd->pOptions, "force"))
    tOptions|= BGP_ROUTER_LOAD_OPTIONS_FORCE;
    
  // Get option --summary ?
  if (cli_options_has_value(pCmd->pOptions, "summary"))
    tOptions|= BGP_ROUTER_LOAD_OPTIONS_SUMMARY;

  // Load the MRTD file 
  if (bgp_router_load_rib(pRouter, pcFileName, tFormat, tOptions) != 0) {
    cli_set_user_error(cli_get(), "could not load \"%s\"", pcFileName);
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// ----- cli_bgp_router_set_tiebreak --------------------------------
/**
 * This function changes the tie-breaking rule of the given BGP
 * instance.
 *
 * context: {router}
 * tokens: {addr, function}
 */
/*int cli_bgp_router_set_tiebreak(SCliContext * pContext,
				SCliCmd * pCmd)
{
  char * pcParam;
  SBGPRouter * pRouter;

  // Get the BGP instance from the context
  pRouter= (SBGPRouter *) cli_context_get_item_at_top(pContext);

  // Get the tie-breaking function name
  pcParam= tokens_get_string_at(pTokens, 1);
  if (!strcmp(pcParam, "hash"))
    pRouter->fTieBreak= tie_break_hash;
  else if (!strcmp(pcParam, "low-isp"))
    pRouter->fTieBreak= tie_break_low_ISP;
  else if (!strcmp(pcParam, "high-isp"))
    pRouter->fTieBreak= tie_break_high_ISP;
  else {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid tie-breaking function \"%s\"\n", pcParam);
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
  }*/

// ----- cli_bgp_router_save_rib ------------------------------------
/**
 * This function saves the Loc-RIB of the given BGP instance in a
 * file. The file format is MRTD.
 *
 * context: {router}
 * tokens: {file}
 */
int cli_bgp_router_save_rib(SCliContext * pContext,
			    SCliCmd * pCmd)
{
  SBGPRouter * pRouter;
  char * pcFileName;

  // Get the BGP instance from the context
  pRouter= (SBGPRouter *) cli_context_get_item_at_top(pContext);

  // Get the file name
  pcFileName= tokens_get_string_at(pCmd->pParamValues, 0);

  // Save the Loc-RIB
  if (bgp_router_save_rib(pRouter, pcFileName)) {
    cli_set_user_error(cli_get(), "unable to save into \"%s\"",
		       pcFileName);
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// ----- cli_bgp_router_save_ribin ----------------------------------
/**
 * This function saves the Adj-RIB-Ins of the given BGP instance in a
 * file. The file format is MRTD.
 *
 * context: {router}
 * tokens: {file}
 */
int cli_bgp_router_save_ribin(SCliContext * pContext,
			      SCliCmd * pCmd)
{
  cli_set_user_error(cli_get(), "sorry, not implemented");
  return CLI_ERROR_COMMAND_FAILED;
}

// ----- cli_bgp_router_show_info -----------------------------------
/**
 * context: {router}
 * tokens: {}
 */
int cli_bgp_router_show_info(SCliContext * pContext,
			     SCliCmd * pCmd)
{
  SBGPRouter * pRouter;

  // Get the BGP instance from the context
  pRouter= (SBGPRouter *) cli_context_get_item_at_top(pContext);

  // Show information
  bgp_router_info(pLogOut, pRouter);

  return CLI_SUCCESS;
}

// ----- cli_bgp_router_show_networks -------------------------------
/** 
 * This function shows the list of locally originated networks in the
 * given BGP instance.
 *
 * context: {router}
 * tokens: {}
 */
int cli_bgp_router_show_networks(SCliContext * pContext,
				 SCliCmd * pCmd)
{
  SBGPRouter * pRouter;

  // Get the BGP instance from the context
  pRouter= (SBGPRouter *) cli_context_get_item_at_top(pContext);

  bgp_router_dump_networks(pLogOut, pRouter);

  return CLI_SUCCESS;
}

// ----- cli_bgp_router_show_peers ----------------------------------
/**
 * context: {router}
 * tokens: {}
 */
int cli_bgp_router_show_peers(SCliContext * pContext,
			      SCliCmd * pCmd)
{
  SBGPRouter * pRouter;

  // Get the BGP instance from the context
  pRouter= (SBGPRouter *) cli_context_get_item_at_top(pContext);

  // Dump peers
  bgp_router_dump_peers(pLogOut, pRouter);

  return CLI_SUCCESS;
}

// ----- cli_bgp_router_show_rib ------------------------------------
/**
 * This function shows the list of routes in the given BGP instance's
 * Loc-RIB (best routes). The function takes a single parameter which
 * can be
 *   - an address, meaning show only the route which best matches this
 *     address 
 *   - a prefix, meaning show only the route towards this exact prefix
 *   - an asterisk ('*'), meaning show everything
 *
 * context: {router}
 * tokens: {prefix|address|*}
 */
int cli_bgp_router_show_rib(SCliContext * pContext,
			    SCliCmd * pCmd)
{
  SBGPRouter * pRouter;
  char * pcPrefix;
  char * pcEndChar;
  SPrefix sPrefix;

  // Get the BGP instance from the context
  pRouter= (SBGPRouter *) cli_context_get_item_at_top(pContext);

  // Get the prefix/address/*
  pcPrefix= tokens_get_string_at(pCmd->pParamValues, 0);
  if (!strcmp(pcPrefix, "*")) {
    bgp_router_dump_rib(pLogOut, pRouter);
  } else if (!ip_string_to_prefix(pcPrefix, &pcEndChar, &sPrefix) &&
	     (*pcEndChar == 0)) {
    bgp_router_dump_rib_prefix(pLogOut, pRouter, sPrefix);
  } else if (!ip_string_to_address(pcPrefix, &pcEndChar,
				   &sPrefix.tNetwork) &&
	     (*pcEndChar == 0)) {
    bgp_router_dump_rib_address(pLogOut, pRouter, sPrefix.tNetwork);
  } else {
    cli_set_user_error(cli_get(), "invalid prefix|address|* \"%s\"",
		       pcPrefix);
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// ----- cli_bgp_router_show_ribin ----------------------------------
/**
 * This function shows the list of routes in the given BGP instance's
 * Adj-RIB-In. The function takes two parameters. The first one
 * selects the peer(s) for which the Adj-RIB-In will be shown. The
 * second one selects the routes which will be shown. The 'peer'
 * parameter can be 
 *   - a peer address
 *   - an asterisk ('*')
 * The second parameter can be
 *   - an address, meaning show only the route which best matches this
 *     address 
 *   - a prefix, meaning show only the route towards this exact prefix
 *   - an asterisk ('*'), meaning show everything
 *
 * context: {router}
 * tokens: {addr, prefix|address|*}
 */
int cli_bgp_router_show_ribin(SCliContext * pContext,
			      SCliCmd * pCmd)
{
  SBGPRouter * pRouter;
  char * pcPeerAddr;
  char * pcEndChar;
  net_addr_t tPeerAddr;
  SBGPPeer * pPeer;
  char * pcPrefix;
  SPrefix sPrefix;

  // Get the BGP instance from the context
  pRouter= (SBGPRouter *) cli_context_get_item_at_top(pContext);

  // Get the peer/*
  pcPeerAddr= tokens_get_string_at(pCmd->pParamValues, 0);
  if (!strcmp(pcPeerAddr, "*")) {
    pPeer= NULL;
  } else if (!ip_string_to_address(pcPeerAddr, &pcEndChar, &tPeerAddr)
	     && (*pcEndChar == 0)) {
    pPeer= bgp_router_find_peer(pRouter, tPeerAddr);
    if (pPeer == NULL) {
      cli_set_user_error(cli_get(), "unknown peer \"%s\"", pcPeerAddr);
      return CLI_ERROR_COMMAND_FAILED;
    }
  } else {
    cli_set_user_error(cli_get(), "invalid peer address \"%s\"", pcPeerAddr);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Get the prefix/address/*
  pcPrefix= tokens_get_string_at(pCmd->pParamValues, 1);
  if (!strcmp(pcPrefix, "*")) {
    sPrefix.uMaskLen= 0;
  } else if (!ip_string_to_prefix(pcPrefix, &pcEndChar, &sPrefix) &&
	     (*pcEndChar == 0)) {
  } else if (!ip_string_to_address(pcPrefix, &pcEndChar,
				   &sPrefix.tNetwork) &&
	     (*pcEndChar == 0)) {
    sPrefix.uMaskLen= 32;
  } else {
    cli_set_user_error(cli_get(), "invalid prefix|address|* \"%s\"",
		       pcPrefix);
    return CLI_ERROR_COMMAND_FAILED;
  }

  bgp_router_dump_adjrib(pLogOut, pRouter, pPeer, sPrefix, 1);
  
  return CLI_SUCCESS;
}

// ----- cli_bgp_router_show_ribin ----------------------------------
/**
 * context: {router}
 * tokens: {addr, prefix|address|*}
 */
int cli_bgp_router_show_ribout(SCliContext * pContext,
			       SCliCmd * pCmd)
{
  SBGPRouter * pRouter;
  char * pcPeerAddr;
  char * pcEndChar;
  net_addr_t tPeerAddr;
  SBGPPeer * pPeer;
  char * pcPrefix;
  SPrefix sPrefix;

  // Get the BGP instance from the context
  pRouter= (SBGPRouter *) cli_context_get_item_at_top(pContext);

  // Get the peer/*
  pcPeerAddr= tokens_get_string_at(pCmd->pParamValues, 0);
  if (!strcmp(pcPeerAddr, "*")) {
    pPeer= NULL;
  } else if (!ip_string_to_address(pcPeerAddr, &pcEndChar, &tPeerAddr)
	     && (*pcEndChar == 0)) {
    pPeer= bgp_router_find_peer(pRouter, tPeerAddr);
    if (pPeer == NULL) {
      cli_set_user_error(cli_get(), "unknown peer \"%s\"", pcPeerAddr);
      return CLI_ERROR_COMMAND_FAILED;
    }
  } else {
    cli_set_user_error(cli_get(), "invalid peer address \"%s\"",
		       pcPeerAddr);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Get the prefix/address/*
  pcPrefix= tokens_get_string_at(pCmd->pParamValues, 1);
  if (!strcmp(pcPrefix, "*")) {
    sPrefix.uMaskLen= 0;
  } else if (!ip_string_to_prefix(pcPrefix, &pcEndChar, &sPrefix) &&
	     (*pcEndChar == 0)) {
  } else if (!ip_string_to_address(pcPrefix, &pcEndChar,
				   &sPrefix.tNetwork) &&
	     (*pcEndChar == 0)) {
    sPrefix.uMaskLen= 32;
  } else {
    cli_set_user_error(cli_get(), "invalid prefix|address|* \"%s\"",
		       pcPrefix);
    return CLI_ERROR_COMMAND_FAILED;
  }

  bgp_router_dump_adjrib(pLogOut, pRouter, pPeer, sPrefix, 0);
  
  return CLI_SUCCESS;
}

// ----- cli_bgp_router_show_routeinfo ------------------------------
/**
 * Display a detailled information about the best route towards the
 * given prefix. Information includes communities, and so on...
 *
 * context: {router}
 * tokens: {prefix|address|* [, output]}
 */
int cli_bgp_router_show_routeinfo(SCliContext * pContext,
				  SCliCmd * pCmd)
{
  SBGPRouter * pRouter;
  char * pcDest;
  SNetDest sDest;
  char * pcOutput;
  SLogStream * pStream= pLogOut;

  // Get the BGP instance from the context
  pRouter= (SBGPRouter *) cli_context_get_item_at_top(pContext);

  // Get the destination
  pcDest= tokens_get_string_at(pCmd->pParamValues, 0);
  if (ip_string_to_dest(pcDest, &sDest)) {
    cli_set_user_error(cli_get(), "invalid destination \"%s\"", pcDest);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Get the optional parameter
  if (tokens_get_num(pCmd->pParamValues) > 1) {
    pcOutput= tokens_get_string_at(pCmd->pParamValues, 1);
    pStream= log_create_file(pcOutput);
    if (pStream == NULL) {
      cli_set_user_error(cli_get(), "could not create \"%d\"", pcOutput);
      return CLI_ERROR_COMMAND_FAILED;
    }
  }

  // Show the route information
  if (bgp_router_show_routes_info(pStream, pRouter, sDest) != 0) {
    cli_set_user_error(cli_get(), "failed to show info for route(s)");
    if (pStream != pLogOut)
      log_destroy(&pStream);
    return CLI_ERROR_COMMAND_FAILED;
  }
  if (pStream != pLogOut)
    log_destroy(&pStream);

  return CLI_SUCCESS;
}

// ----- cli_bgp_router_show_stats ----------------------------------
/**
 * context: {router}
 * tokens: {}
 */
int cli_bgp_router_show_stats(SCliContext * pContext,
			      SCliCmd * pCmd)
{
  SBGPRouter * pRouter=
    (SBGPRouter *) cli_context_get_item_at_top(pContext);

  bgp_router_show_stats(pLogOut, pRouter);

  return CLI_SUCCESS;
}

// ----- cli_bgp_router_recordroute ---------------------------------
/**
 * context: {router}
 * tokens: {prefix}
 */
int cli_bgp_router_recordroute(SCliContext * pContext,
			       SCliCmd * pCmd)
{
  SBGPRouter * pRouter;
  char * pcPrefix;
  SPrefix sPrefix;
  int iResult;
  SBGPPath * pPath= NULL;

  // Get BGP instance
  pRouter= (SBGPRouter *) cli_context_get_item_at_top(pContext);

  // Get prefix
  pcPrefix= tokens_get_string_at(pCmd->pParamValues, 0);
  if (str2prefix(pcPrefix, &sPrefix) < 0) {
    cli_set_user_error(cli_get(), "invalid prefix \"%s\"", pcPrefix);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Record route
  iResult= bgp_record_route(pRouter, sPrefix, &pPath, 0);

  // Display recorded-route
  bgp_dump_recorded_route(pLogOut, pRouter, sPrefix, pPath, iResult);

  path_destroy(&pPath);

  return CLI_SUCCESS;
}

// ----- cli_bgp_router_rescan --------------------------------------
/**
 * context: {router}
 * tokens: {}
 */
int cli_bgp_router_rescan(SCliContext * pContext, SCliCmd * pCmd)
{
  SBGPRouter * pRouter;

  // Get BGP instance from context
  pRouter= (SBGPRouter *) cli_context_get_item_at_top(pContext);

  if (bgp_router_scan_rib(pRouter)) {
    cli_set_user_error(cli_get(), "RIB scan failed");
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// ----- cli_bgp_router_rerun ---------------------------------------
/**
 * context: {router}
 * tokens: {prefix|*}
 */
int cli_bgp_router_rerun(SCliContext * pContext, SCliCmd * pCmd)
{
  SBGPRouter * pRouter;
  char * pcPrefix;
  char * pcEndChar;
  SPrefix sPrefix;

  // Get BGP instance from context
  pRouter= (SBGPRouter *) cli_context_get_item_at_top(pContext);

  /* Get prefix */
  pcPrefix= tokens_get_string_at(pCmd->pParamValues, 0);
  if (!strcmp(pcPrefix, "*")) {
    sPrefix.uMaskLen= 0;
  } else if (!ip_string_to_prefix(pcPrefix, &pcEndChar, &sPrefix) &&
	     (*pcEndChar == 0)) {
  } else {
    cli_set_user_error(cli_get(), "invalid prefix|* \"%s\"", pcPrefix);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Rerun the decision process for all known prefixes
  if (bgp_router_rerun(pRouter, sPrefix)) {
    cli_set_user_error(cli_get(), "reset failed.");
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// ----- cli_bgp_router_reset ---------------------------------------
/**
 * This function shuts down all the router's peerings then restart
 * them.
 *
 * context: {router}
 * tokens: {}
 */
int cli_bgp_router_reset(SCliContext * pContext, SCliCmd * pCmd)
{
  SBGPRouter * pRouter;

  // Get BGP instance from context
  pRouter= (SBGPRouter *) cli_context_get_item_at_top(pContext);

  // Reset the router
  if (bgp_router_reset(pRouter)) {
    cli_set_user_error(cli_get(), "reset failed.");
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// ----- cli_bgp_router_start ---------------------------------------
/**
 * This function opens all the defined peerings of the given router.
 *
 * context: {router}
 * tokens: {}
 */
int cli_bgp_router_start(SCliContext * pContext, SCliCmd * pCmd)
{
  SBGPRouter * pRouter;

  // Get BGP instance from context
  pRouter= (SBGPRouter *) cli_context_get_item_at_top(pContext);

  // Start the router
  if (bgp_router_start(pRouter)) {
    cli_set_user_error(cli_get(), "could not start the router.");
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// ----- cli_bgp_router_stop ----------------------------------------
/**
 * This function closes all the defined peerings of the given router.
 *
 * context: {router}
 * tokens: {}
 */
int cli_bgp_router_stop(SCliContext * pContext, SCliCmd * pCmd)
{
  SBGPRouter * pRouter;

  // Get BGP instance from context
  pRouter= (SBGPRouter *) cli_context_get_item_at_top(pContext);

  // Stop the router
  if (bgp_router_stop(pRouter)) {
    cli_set_user_error(cli_get(), "could not stop the router.");
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// ----- cli_bgp_router_add_peer ------------------------------------
/**
 * context: {router}
 * tokens: {as-num, addr}
 */
int cli_bgp_router_add_peer(SCliContext * pContext, SCliCmd * pCmd)
{
  SBGPRouter * pRouter;
  unsigned int uASNum;
  char * pcAddr;
  net_addr_t tAddr;

  // Get BGP router from context
  pRouter= (SBGPRouter *) cli_context_get_item_at_top(pContext);

  // Get the new peer's AS number
  if (tokens_get_uint_at(pCmd->pParamValues, 0, &uASNum)
      || (uASNum > MAX_AS)) {
    cli_set_user_error(cli_get(), "invalid AS number \"%s\"",
		       tokens_get_string_at(pCmd->pParamValues, 0));
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Get the new peer's address
  pcAddr= tokens_get_string_at(pCmd->pParamValues, 1);
  if (str2address(pcAddr, &tAddr) < 0) {
    cli_set_user_error(cli_get(), "invalid address \"%s\"", pcAddr);
    return CLI_ERROR_COMMAND_FAILED;
  }

  if (bgp_router_add_peer(pRouter, uASNum, tAddr, NULL) != 0) {
    cli_set_user_error(cli_get(), "peer already exists");
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// ----- cli_bgp_router_add_network ---------------------------------
/**
 * This function adds a local network to the BGP instance.
 *
 * context: {router}
 * tokens: {prefix}
 */
int cli_bgp_router_add_network(SCliContext * pContext, SCliCmd * pCmd)
{
  SBGPRouter * pRouter;
  char * pcPrefix;
  SPrefix sPrefix;

  // Get the BGP instance
  pRouter= (SBGPRouter *) cli_context_get_item_at_top(pContext);

  // Get the prefix
  pcPrefix= tokens_get_string_at(pCmd->pParamValues, 0);
  if (str2prefix(pcPrefix, &sPrefix) < 0) {
    cli_set_user_error(cli_get(), "invalid prefix \"%s\"", pcPrefix);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Add the network
  if (bgp_router_add_network(pRouter, sPrefix))
    return CLI_ERROR_COMMAND_FAILED;

  return CLI_SUCCESS;
}

// ----- cli_bgp_router_del_network ---------------------------------
/**
 * This function removes a previously added local network.
 *
 * context: {router}
 * tokens: {prefix}
 */
int cli_bgp_router_del_network(SCliContext * pContext,
			       SCliCmd * pCmd)
{
  SBGPRouter * pRouter;
  char * pcPrefix;
  SPrefix sPrefix;

  // Get BGP instance from context
  pRouter= (SBGPRouter *) cli_context_get_item_at_top(pContext);

  // Get the prefix
  pcPrefix= tokens_get_string_at(pCmd->pParamValues, 0);
  if (str2prefix(pcPrefix, &sPrefix) < 0) {
    cli_set_user_error(cli_get(), "invalid prefix \"%s\"", pcPrefix);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Remove the network
  if (bgp_router_del_network(pRouter, sPrefix))
    return CLI_ERROR_COMMAND_FAILED;

  return CLI_SUCCESS;
}

// ----- cli_bgp_router_add_qosnetwork ------------------------------
/**
 * context: {router}
 * tokens: {prefix, delay}
 */
#ifdef BGP_QOS
int cli_bgp_router_add_qosnetwork(SCliContext * pContext, SCliCmd * pCmd)
{
  SBGPRouter * pRouter;
  SPrefix sPrefix;
  unsigned int uDelay;

  /*LOG_DEBUG("bgp router %s add network %s\n",
	    tokens_get_string_at(pTokens, 0),
	    tokens_get_string_at(pTokens, 1));*/
  pRouter= (SBGPRouter *) cli_context_get_item_at_top(pContext);

  if (str2prefix(tokens_get_string_at(pTokens, 0), &sPrefix) < 0)
    return CLI_ERROR_COMMAND_FAILED;

  if (tokens_get_uint_at(pTokens, 1, &uDelay))
    return CLI_ERROR_COMMAND_FAILED;

  if (bgp_router_add_qos_network(pRouter, sPrefix, (net_link_delay_t) uDelay))
    return CLI_ERROR_COMMAND_FAILED;
  return CLI_SUCCESS;
}
#endif

// ----- cli_bgp_router_assert_routes_match --------------------------
/**
 * context: {router, routes}
 * tokens: {predicate}
 */
int cli_bgp_router_assert_routes_match(SCliContext * pContext,
				       SCliCmd * pCmd)
{
  char * pcPredicate;
  char * pcTemp;
  SFilterMatcher* pMatcher;
  SRoutes * pRoutes= (SRoutes *) cli_context_get_item_at_top(pContext);
  int iMatches= 0;
  int iIndex;
  SRoute * pRoute;
  int iResult;

  // Parse predicate
  pcPredicate= tokens_get_string_at(pCmd->pParamValues,
				    tokens_get_num(pCmd->pParamValues)-1);
  pcTemp= pcPredicate;
  iResult= predicate_parser(pcTemp, &pMatcher);
  if (iResult != PREDICATE_PARSER_SUCCESS) {
    cli_set_user_error(cli_get(), "invalid predicate \"%s\" (%s)",
		       pcPredicate, predicate_parser_strerror(iResult));
    return CLI_ERROR_COMMAND_FAILED;
  }

  for (iIndex= 0; iIndex < routes_list_get_num(pRoutes); iIndex++) {
    pRoute= (SRoute *) pRoutes->data[iIndex];
    if (filter_matcher_apply(pMatcher, NULL, pRoute)) {
      iMatches++;
    }
  }

  filter_matcher_destroy(&pMatcher);

  if (iMatches < 1) {
    cli_set_user_error(cli_get(), "no route matched \"%s\"", pcPredicate);
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// ----- cli_bgp_router_assert_routes_show --------------------------
/**
 * context: {router, routes}
 * tokens: {}
 */
int cli_bgp_router_assert_routes_show(SCliContext * pContext,
				      SCliCmd * pCmd)
{
  SRoutes * pRoutes= (SRoutes *) cli_context_get_item_at_top(pContext);

  routes_list_dump(pLogOut, pRoutes);

  return CLI_SUCCESS;
}

// ----- cli_ctx_create_bgp_router_assert_routes --------------------
/**
 * Check that the given router has a feasible route towards a given
 * prefix that goes through a given next-hop.
 *
 * context: {router}
 * tokens: {prefix, type}
 */
int cli_ctx_create_bgp_router_assert_routes(SCliContext * pContext,
					    void ** ppItem)
{
  SBGPRouter * pRouter;
  char * pcPrefix;
  char * pcType;
  SPrefix sPrefix;
  SRoutes * pRoutes;

  // Get BGP instance from context
  pRouter= (SBGPRouter *) cli_context_get_item_at_top(pContext);

  // Get the prefix
  pcPrefix= tokens_get_string_at(pContext->pCmd->pParamValues, 0);
  if (str2prefix(pcPrefix, &sPrefix) < 0) {
    cli_set_user_error(cli_get(), "invalid prefix \"%s\"", pcPrefix);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Get the routes type
  pcType= tokens_get_string_at(pContext->pCmd->pParamValues, 1);

  // Get the requested routes of the router towards the prefix
  if (!strcmp(pcType, "best")) {
    pRoutes= bgp_router_get_best_routes(pRouter, sPrefix);
  } else if (!strcmp(pcType, "feasible")) {
#ifdef __EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
    pRoutes= bgp_router_get_feasible_routes(pRouter, sPrefix, 0);
#else
    pRoutes= bgp_router_get_feasible_routes(pRouter, sPrefix);
#endif
  } else {
    cli_set_user_error(cli_get(), "unsupported type of routes \"%s\"", pcType);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Check that there is at leat one route
  if (routes_list_get_num(pRoutes) < 1) {
    cli_set_user_error(cli_get(), "no feasible routes towards %s", pcPrefix);
    routes_list_destroy(&pRoutes);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Store list of routes on context...
  *ppItem= pRoutes;

  return CLI_SUCCESS;
}

// ----- cli_ctx_destroy_bgp_router_assert_routes -----------------
void cli_ctx_destroy_bgp_router_assert_routes(void ** ppItem)
{
  routes_list_destroy((SRoutes **) ppItem);
}

// ----- cli_ctx_create_bgp_router_peer -----------------------------
/**
 * context: {router} -> {router, peer}
 * tokens: {addr}
 */
int cli_ctx_create_bgp_router_peer(SCliContext * pContext,
				   void ** ppItem)
{
  SBGPRouter * pRouter;
  SBGPPeer * pPeer;
  char * pcPeerAddr;
  net_addr_t tAddr;

  // Get the BGP instance from context
  pRouter= (SBGPRouter *) cli_context_get_item_at_top(pContext);

  // Get the peer address
  pcPeerAddr= tokens_get_string_at(pContext->pCmd->pParamValues, 0);
  if (str2address(pcPeerAddr, &tAddr) < 0) {
    cli_set_user_error(cli_get(), "invalid peer address \"%s\"",
		       pcPeerAddr);
    return CLI_ERROR_CTX_CREATE;
  }

  // Get the peer
  pPeer= bgp_router_find_peer(pRouter, tAddr);
  if (pPeer == NULL) {
    cli_set_user_error(cli_get(), "unknown peer");
    return CLI_ERROR_CTX_CREATE;
  }

  *ppItem= pPeer;
  return CLI_SUCCESS;
}

// ----- cli_ctx_destroy_bgp_router_peer ----------------------------
void cli_ctx_destroy_bgp_router_peer(void ** ppItem)
{
} 

// ----- cli_ctx_create_bgp_router_peer_filter ----------------------
/**
 * context: {router, peer}
 * tokens: {in|out}
 */
int cli_ctx_create_bgp_router_peer_filter(SCliContext * pContext,
					  void ** ppItem)
{
  SBGPPeer * pPeer;
  char * pcFilter;

  // Get Peer instance
  pPeer= (SBGPPeer *) cli_context_get_item_at_top(pContext);

  // Create filter context
  pcFilter= tokens_get_string_at(pContext->pCmd->pParamValues, 0);
  if (!strcmp("in", pcFilter))
    *ppItem= &(pPeer->pInFilter);
  else if (!strcmp("out", pcFilter))
    *ppItem= &(pPeer->pOutFilter);
  else {
    cli_set_user_error(cli_get(), "invalid filter type \"%s\"", pcFilter);
    return CLI_ERROR_CTX_CREATE;
  }

  return CLI_SUCCESS;
}

// ----- cli_ctx_destroy_bgp_router_peer_filter ---------------------
void cli_ctx_destroy_bgp_router_peer_filter(void ** ppItem)
{
}

// ----- cli_bgp_filter_rule_match ----------------------------------
/**
 * context: {?, rule}
 * tokens: {?, predicate}
 */
int cli_bgp_filter_rule_match(SCliContext * pContext,
			      SCliCmd * pCmd)
{
  SFilterRule * pRule;
  char * pcPredicate;
  SFilterMatcher * pMatcher;
  int iResult;

  // Get rule from context
  pRule= (SFilterRule *) cli_context_get_item_at_top(pContext);

  // Parse predicate
  pcPredicate= tokens_get_string_at(pCmd->pParamValues,
				    tokens_get_num(pCmd->pParamValues)-1);
  iResult= predicate_parser(pcPredicate, &pMatcher);
  if (iResult != PREDICATE_PARSER_SUCCESS) {
    cli_set_user_error(cli_get(), "invalid predicate \"%s\" (%s)",
		       pcPredicate, predicate_parser_strerror(iResult));
    return CLI_ERROR_COMMAND_FAILED;
  }

  if (pRule->pMatcher != NULL)
    filter_matcher_destroy(&pRule->pMatcher);
  pRule->pMatcher= pMatcher;

  return CLI_SUCCESS;
}

// ----- cli_bgp_filter_rule_action ----------------------------------
/**
 * context: {?, rule}
 * tokens: {?, action}
 */
int cli_bgp_filter_rule_action(SCliContext * pContext,
			       SCliCmd * pCmd)
{
  SFilterRule * pRule;
  char * pcAction;
  SFilterAction * pAction;

  // Get rule from context
  pRule= (SFilterRule *) cli_context_get_item_at_top(pContext);

  // Parse predicate
  pcAction= tokens_get_string_at(pCmd->pParamValues,
				 tokens_get_num(pCmd->pParamValues)-1);
  if (filter_parser_action(pcAction, &pAction)) {
    cli_set_user_error(cli_get(), "invalid action \"%s\"", pcAction);
    return CLI_ERROR_COMMAND_FAILED;
  }

  if (pRule->pAction != NULL)
    filter_action_destroy(&pRule->pAction);
  pRule->pAction= pAction;

  return CLI_SUCCESS;
}

// ----- cli_ctx_create_bgp_filter_add_rule -------------------------
/**
 * context: {?, &filter}
 * tokens: {?}
 */
int cli_ctx_create_bgp_filter_add_rule(SCliContext * pContext,
				       void ** ppItem)
{
  SFilter ** ppFilter;
  SFilterRule * pRule;

  // Get current filter from context
  ppFilter= (SFilter **) cli_context_get_item_at_top(pContext);

  // If the filter is empty, create it
  if (*ppFilter == NULL)
    *ppFilter= filter_create();

  // Create rule and put it on the context
  pRule= filter_rule_create(NULL, NULL);
  filter_add_rule2(*ppFilter, pRule);

  *ppItem= pRule;

  return CLI_SUCCESS;
}

// ----- cli_ctx_create_bgp_filter_insert_rule ----------------------
/**
 * context: {?, &filter}
 * tokens: {?, pos}
 */
int cli_ctx_create_bgp_filter_insert_rule(SCliContext * pContext,
					  void ** ppItem)
{
  SFilter ** ppFilter;
  SFilterRule * pRule;
  unsigned int uIndex;

  // Get current filter from context
  ppFilter= (SFilter **) cli_context_get_item_at_top(pContext);

  // Get insertion position
  if (tokens_get_uint_at(pContext->pCmd->pParamValues,
			 tokens_get_num(pContext->pCmd->pParamValues)-1,
			 &uIndex)) {
    cli_set_user_error(cli_get(), "invalid index");
    return CLI_ERROR_COMMAND_FAILED;
  }

  // If the filter is empty, create it
  if (*ppFilter == NULL)
    *ppFilter= filter_create();

  // Create rule and put it on the context
  pRule= filter_rule_create(NULL, NULL);
  filter_insert_rule(*ppFilter, uIndex, pRule);

  *ppItem= pRule;

  return CLI_SUCCESS;
  return CLI_ERROR_CTX_CREATE;
}

// ----- cli_bgp_filter_remove_rule ---------------------------------
/**
 * context: {?, &filter}
 * tokens: {?, pos}
 */
int cli_bgp_filter_remove_rule(SCliContext * pContext,
			       SCliCmd * pCmd)
{
  SFilter ** ppFilter=
    (SFilter **) cli_context_get_item_at_top(pContext);
  unsigned int uIndex;

  if (*ppFilter == NULL) {
    cli_set_user_error(cli_get(), "filter contains no rule");
    return CLI_ERROR_COMMAND_FAILED;
  }
  
  // Get index of rule to be removed
  if (tokens_get_uint_at(pCmd->pParamValues,
			 tokens_get_num(pCmd->pParamValues)-1,
			 &uIndex)) {
    cli_set_user_error(cli_get(), "invalid index");
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Remove rule
  if (filter_remove_rule(*ppFilter, uIndex)) {
    cli_set_user_error(cli_get(), "could not remove rule %u", uIndex);
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// ----- cli_ctx_create_bgp_filter_rule -----------------------------
/**
 * context: {?, &filter}
 * tokens: {?}
 */
int cli_ctx_create_bgp_filter_rule(SCliContext * pContext,
				   void ** ppItem)
{
  cli_set_user_error(cli_get(), "not yet implemented, sorry.");
  return CLI_ERROR_CTX_CREATE;
}

// ----- cli_ctx_destroy_bgp_filter_rule ----------------------------
/**
 *
 */
void cli_ctx_destroy_bgp_filter_rule(void ** ppItem)
{
}

// ----- cli_bgp_filter_show-----------------------------------------
/**
 * context: {?, &filter}
 * tokens: {?}
 */
int cli_bgp_filter_show(SCliContext * pContext,
			SCliCmd * pCmd)
{
  SFilter ** ppFilter= (SFilter **) cli_context_get_item_at_top(pContext);

  filter_dump(pLogOut, *ppFilter);

  return CLI_SUCCESS;
}

// ----- cli_bgp_route_map_filter_add --------------------------------
/**
 * context: {route-map name}
 * tokens: {filter}
 */
int cli_bgp_route_map_filter_add(SCliContext * pContext, 
				  SCliCmd * pCmd)
{
  char * pcRouteMapName;
  SFilterRule * pRule;
  SFilter * pFilter= NULL;

  pcRouteMapName = (char *) cli_context_get_item_at_top(pContext);
  
  if (filter_parser_rule(tokens_get_string_at(pCmd->pParamValues, 0), &pRule) != 
      FILTER_PARSER_SUCCESS)
    return CLI_ERROR_COMMAND_FAILED;
  if ( (pFilter = route_map_get(pcRouteMapName)) == NULL) {
    pFilter = filter_create();
    if (route_map_add(pcRouteMapName, pFilter) < 0) {
      cli_set_user_error(cli_get(), "could not add the route-map filter.");
      return CLI_ERROR_COMMAND_FAILED;
    }
  }
  filter_add_rule2(pFilter, pRule);
  return CLI_SUCCESS;
}

// ----- cli_bgp_router_peer_infilter_set ---------------------------
/**
 * context: {router, peer}
 * tokens: {addr, addr, filter}
 */
int cli_bgp_router_peer_infilter_set(SCliContext * pContext,
				     SCliCmd * pCmd)
{
  SBGPPeer * pPeer;
  SFilter * pFilter= NULL;

  pPeer= (SBGPPeer *) cli_context_get_item_at_top(pContext);
  /*
  if (filter_parser_run(tokens_get_string_at(pTokens, 2), &pFilter) !=
      FILTER_PARSER_SUCCESS) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid filter\n");
    return CLI_ERROR_COMMAND_FAILED;
  }
  */

  bgp_peer_set_in_filter(pPeer, pFilter);

  return CLI_SUCCESS;
}

// ----- cli_bgp_router_peer_infilter_add ---------------------------
/**
 * context: {router, peer}
 * tokens: {filter}
 */
int cli_bgp_router_peer_infilter_add(SCliContext * pContext,
				     SCliCmd * pCmd)
{
  SBGPPeer * pPeer;
  SFilterRule * pRule;
  SFilter * pFilter;

  /*LOG_DEBUG("bgp router %s peer %s in-filter \"%s\"\n",
	    tokens_get_string_at(pTokens, 0),
	    tokens_get_string_at(pTokens, 1),
	    tokens_get_string_at(pTokens, 2));*/
  pPeer= (SBGPPeer *) cli_context_get_item_at_top(pContext);

  if (filter_parser_rule(tokens_get_string_at(pCmd->pParamValues, 0), &pRule) !=
      FILTER_PARSER_SUCCESS)
    return CLI_ERROR_COMMAND_FAILED;
  pFilter= bgp_peer_in_filter_get(pPeer);
  if (pFilter == NULL) {
    pFilter= filter_create();
    bgp_peer_set_in_filter(pPeer, pFilter);
  }
  filter_add_rule2(pFilter, pRule);
  return CLI_SUCCESS;
}

// ----- cli_bgp_router_peer_outfilter_set --------------------------
/**
 * context: {router, peer}
 * tokens: {filter}
 */
int cli_bgp_router_peer_outfilter_set(SCliContext * pContext,
				      SCliCmd * pCmd)
{
  SBGPPeer * pPeer;
  SFilter * pFilter= NULL;

  pPeer= (SBGPPeer *) cli_context_get_item_at_top(pContext);
  /*
  if (filter_parser_run(tokens_get_string_at(pTokens, 2), &pFilter) !=
      FILTER_PARSER_SUCCESS)
    return CLI_ERROR_COMMAND_FAILED;
  */
  bgp_peer_set_out_filter(pPeer, pFilter);
  return CLI_SUCCESS;
}

// ----- cli_bgp_router_peer_outfilter_add --------------------------
/**
 * context: {router, peer}
 * tokens: {filter}
 */
int cli_bgp_router_peer_outfilter_add(SCliContext * pContext,
				      SCliCmd * pCmd)
{
  SBGPPeer * pPeer;
  SFilterRule * pRule;
  SFilter * pFilter;

  pPeer= (SBGPPeer *) cli_context_get_item_at_top(pContext);

  if (filter_parser_rule(tokens_get_string_at(pCmd->pParamValues, 0), &pRule) !=
      FILTER_PARSER_SUCCESS)
    return CLI_ERROR_COMMAND_FAILED;
  pFilter= bgp_peer_out_filter_get(pPeer);
  if (pFilter == NULL) {
    pFilter= filter_create();
    bgp_peer_set_out_filter(pPeer, pFilter);
  }
  filter_add_rule2(pFilter, pRule);
  return CLI_SUCCESS;
}

// ----- cli_bgp_router_peer_show_filter ----------------------------
/**
 * context: {router, peer}
 * tokens: {in|out}
 */
int cli_bgp_router_peer_show_filter(SCliContext * pContext,
				    SCliCmd * pCmd)
{
  SBGPPeer * pPeer;
  char * pcFilter;

  // Get BGP peer
  pPeer= (SBGPPeer *) cli_context_get_item_at_top(pContext);

  // Create filter context
  pcFilter= tokens_get_string_at(pContext->pCmd->pParamValues, 0);
  if (!strcmp(pcFilter, "in"))
    bgp_peer_dump_in_filters(pLogOut, pPeer);
  else if (!strcmp(pcFilter, "out"))
    bgp_peer_dump_out_filters(pLogOut, pPeer);
  else {
    cli_set_user_error(cli_get(), "invalid filter type \"%s\"", pcFilter);
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// ----- cli_bgp_router_peer_rrclient -------------------------------
/**
 * context: {router, peer}
 * tokens: {}
 */
int cli_bgp_router_peer_rrclient(SCliContext * pContext, SCliCmd * pCmd)
{
  SBGPRouter * pRouter;
  SBGPPeer * pPeer;

  pRouter= (SBGPRouter *) cli_context_get_item_at(pContext, 0);
  pRouter->iRouteReflector= 1;
  pPeer= (SBGPPeer *) cli_context_get_item_at_top(pContext);
  bgp_peer_flag_set(pPeer, PEER_FLAG_RR_CLIENT, 1);
  return CLI_SUCCESS;
}

// ----- cli_bgp_router_peer_nexthop --------------------------------
/**
 * context: {router, peer}
 * tokens: {addr}
 */
int cli_bgp_router_peer_nexthop(SCliContext * pContext,
				SCliCmd * pCmd)
{
  SBGPPeer * pPeer;
  char * pcNextHop;
  net_addr_t tNextHop;
  char * pcEndChar;

  // Get peer from context
  pPeer= (SBGPPeer *) cli_context_get_item_at_top(pContext);

  // Get next-hop address
  pcNextHop= tokens_get_string_at(pCmd->pParamValues, 0);
  if (ip_string_to_address(pcNextHop, &pcEndChar, &tNextHop) ||
      (*pcEndChar != 0)) {
    cli_set_user_error(cli_get(), "invalid next-hop \"%s\".", pcNextHop);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Set the next-hop to be used for routes advertised to this peer
  if (bgp_peer_set_nexthop(pPeer, tNextHop)) {
    cli_set_user_error(cli_get(), "could not change next-hop");
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// ----- cli_bgp_router_peer_nexthopself ----------------------------
/**
 * context: {router, peer}
 * tokens: {}
 */
int cli_bgp_router_peer_nexthopself(SCliContext * pContext,
				    SCliCmd * pCmd)
{
  SBGPPeer * pPeer;

  // Get peer from context
  pPeer= (SBGPPeer *) cli_context_get_item_at_top(pContext);

  bgp_peer_flag_set(pPeer, PEER_FLAG_NEXT_HOP_SELF, 1);
  return CLI_SUCCESS;
}

// -----[ cli_bgp_router_peer_record ]-------------------------------
/**
 * context: {router, peer}
 * tokens: {file|-}
 */
int cli_bgp_router_peer_record(SCliContext * pContext,
			       SCliCmd * pCmd)
{
  SBGPPeer * pPeer;
  char * pcFileName;
  SLogStream * pStream;
  
  /* Get the peer instance from context */
  pPeer= (SBGPPeer *) cli_context_get_item_at_top(pContext);

  /* Get filename */
  pcFileName= tokens_get_string_at(pCmd->pParamValues, 0);
  if (strcmp(pcFileName, "-") == 0) {
    bgp_peer_set_record_stream(pPeer, NULL);
  } else {
    pStream= log_create_file(pcFileName);
    if (pStream == NULL) {
      cli_set_user_error(cli_get(), "could not open \"%s\" for writing",
			 pcFileName);
      return CLI_ERROR_COMMAND_FAILED;
    }
    if (bgp_peer_set_record_stream(pPeer, pStream) < 0) {
      log_destroy(&pStream);
      cli_set_user_error(cli_get(), "could not set the peer record stream");
      return CLI_ERROR_COMMAND_FAILED;
    }
  }

  return CLI_SUCCESS;
}

// ----- cli_bgp_router_peer_recv -----------------------------------
/**
 * context: {router, peer}
 * tokens: {mrt-record}
 */
int cli_bgp_router_peer_recv(SCliContext * pContext,
			     SCliCmd * pCmd)
{
  SBGPPeer * pPeer;
  SBGPRouter * pRouter;
  char * pcRecord;
  SBGPMsg * pMsg;

  /* Get the peer instance from context */
  pPeer= (SBGPPeer *) cli_context_get_item_at_top(pContext);

  /* Check that the peer is virtual */
  if (!bgp_peer_flag_get(pPeer, PEER_FLAG_VIRTUAL)) {
    cli_set_user_error(cli_get(), "only virtual peers can do that");
    return CLI_ERROR_COMMAND_FAILED;
  }

  /* Get the router instance from context */
  pRouter= (SBGPRouter *) cli_context_get_item_at(pContext, 0);

  /* Get the MRT-record */
  pcRecord= tokens_get_string_at(pCmd->pParamValues, 0);

  /* Build a message from the MRT-record */
  pMsg= mrtd_msg_from_line(pRouter, pPeer, pcRecord);

  if (pMsg == NULL) {
    cli_set_user_error(cli_get(), "could not build message from input");
    return CLI_ERROR_COMMAND_FAILED;
  }

  bgp_peer_handle_message(pPeer, pMsg);

  return CLI_SUCCESS;
}

#ifdef __EXPERIMENTAL__
// ----- cli_bgp_router_load_ribs_in --------------------------------
/**
 * context: {router}
 * tokens: {file}
 */
/* DE-ACTIVATED by BQU on 2007/10/04 */
#ifdef COMMENT_BQU
int cli_bgp_router_load_ribs_in(SCliContext * pContext,
				SCliCmd * pCmd)
{
  SBGPRouter * pRouter;
  char * pcFileName;


  pRouter = (SBGPRouter *) cli_context_get_item_at(pContext, 0);

  pcFileName = tokens_get_string_at(pCmd->pParamValues, 0);
  
  if (bgp_router_load_ribs_in(pcFileName, pRouter) == -1)
    return CLI_ERROR_COMMAND_FAILED;
  return CLI_SUCCESS;
}
#endif /* COMMENT_BQU */
#endif /* __EXPERIMENTAL__ */

#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
// ----- cli_bgp_router_peer_walton_limit ----------------------------
/**
 * context: {router, peer}
 * tokens: {announce-limit}
 */
int cli_bgp_router_peer_walton_limit(SCliContext * pContext, 
						  SCliCmd * pCmd)
{
  SBGPPeer * pPeer;
  unsigned int uWaltonLimit;

  pPeer= (SBGPPeer *) cli_context_get_item_at_top(pContext);

  if (tokens_get_uint_at(pCmd->pParamValues, 0, &uWaltonLimit)) {
    cli_set_user_error(cli_get(), "invalid walton limitation");
    return CLI_ERROR_COMMAND_FAILED;
  }
  peer_set_walton_limit(pPeer, uWaltonLimit);

  return CLI_SUCCESS;

  
}
#endif

// ----- cli_bgp_router_peer_up -------------------------------------
/**
 * context: {router, peer}
 * tokens: {}
 */
int cli_bgp_router_peer_up(SCliContext * pContext, SCliCmd * pCmd)
{
  SBGPPeer * pPeer;

  // Get peer instance from context
  pPeer= (SBGPPeer *) cli_context_get_item_at_top(pContext);

  // Try to open session
  if (bgp_peer_open_session(pPeer)) {
    cli_set_user_error(cli_get(), "could not open session");
    return CLI_ERROR_COMMAND_FAILED;
  }
    
  return CLI_SUCCESS;
}

// ----- cli_bgp_router_peer_softrestart ----------------------------
/**
 * context: {router, peer}
 * tokens: {}
 */
int cli_bgp_router_peer_softrestart(SCliContext * pContext, SCliCmd * pCmd)
{
  SBGPPeer * pPeer;

  // Get peer instance from context
  pPeer= (SBGPPeer *) cli_context_get_item_at_top(pContext);

  // Set the virtual flag of this peer
  if (!bgp_peer_flag_get(pPeer, PEER_FLAG_VIRTUAL)) {
    cli_set_user_error(cli_get(), "soft-restart option only available to virtual peers");
    return CLI_ERROR_COMMAND_FAILED;
  }
  bgp_peer_flag_set(pPeer, PEER_FLAG_SOFT_RESTART, 1);
    
  return CLI_SUCCESS;
}

// ----- cli_bgp_router_peer_virtual --------------------------------
/**
 * context: {router, peer}
 * tokens: {}
 */
int cli_bgp_router_peer_virtual(SCliContext * pContext, SCliCmd * pCmd)
{
  SBGPPeer * pPeer;

  // Get peer instance from context
  pPeer= (SBGPPeer *) cli_context_get_item_at_top(pContext);

  // Set the virtual flag of this peer
  bgp_peer_flag_set(pPeer, PEER_FLAG_VIRTUAL, 1);
    
  return CLI_SUCCESS;
}

// ----- cli_bgp_router_peer_down -----------------------------------
/**
 * context: {router, peer}
 * tokens: {}
 */
int cli_bgp_router_peer_down(SCliContext * pContext, SCliCmd * pCmd)
{
  SBGPPeer * pPeer;

  // Get peer from context
  pPeer= (SBGPPeer *) cli_context_get_item_at_top(pContext);

  // Try to close session
  if (bgp_peer_close_session(pPeer)) {
    cli_set_user_error(cli_get(), "could not close session");
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// -----[ cli_bgp_router_peer_readv ]--------------------------------
/**
 * context: {router, peer}
 * tokens: {prefix|*}
 */
int cli_bgp_router_peer_readv(SCliContext * pContext, SCliCmd * pCmd)
{
  SBGPRouter * pRouter;
  SBGPPeer * pPeer;
  char * pcPrefix;
  char * pcEndChar;
  SPrefix sPrefix;

  /* Get router and peer from context */
  pRouter= (SBGPRouter *) cli_context_get_item_at(pContext, 0);
  pPeer= (SBGPPeer *) cli_context_get_item_at(pContext, 1);

  /* Get prefix */
  pcPrefix= tokens_get_string_at(pCmd->pParamValues, 0);
  if (!strcmp(pcPrefix, "*")) {
    sPrefix.uMaskLen= 0;
  } else if (!ip_string_to_prefix(pcPrefix, &pcEndChar, &sPrefix) &&
	     (*pcEndChar == 0)) {
  } else {
    cli_set_user_error(cli_get(), "invalid prefix|* \"%s\"", pcPrefix);
    return CLI_ERROR_COMMAND_FAILED;
  }

  /* Re-advertise */
  if (bgp_router_peer_readv_prefix(pRouter, pPeer, sPrefix)) {
    cli_set_user_error(cli_get(), "could not re-advertise");
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// ----- cli_bgp_router_peer_reset ----------------------------------
/**
 * context: {router, peer}
 * tokens: {}
 */
int cli_bgp_router_peer_reset(SCliContext * pContext, SCliCmd * pCmd)
{
  SBGPPeer * pPeer;

  // Get peer from context
  pPeer= (SBGPPeer *) cli_context_get_item_at_top(pContext);

  // Try to close session
  if (bgp_peer_close_session(pPeer)) {
    cli_set_user_error(cli_get(), "could not close session");
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Try to open session
  if (bgp_peer_open_session(pPeer)) {
    cli_set_user_error(cli_get(), "could not close session");
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// ----- cli_bgp_show_routers ---------------------------------------
/**
 * Show the list of routers matching the given criterion which is a
 * prefix or an asterisk (meaning "all routers").
 *
 * context: {}
 * tokens: {prefix}
 */
int cli_bgp_show_routers(SCliContext * pContext, SCliCmd * pCmd)
{
  char * pcPrefix;
  char * pcAddr;
  int iState= 0;

  // Get prefix
  pcPrefix= tokens_get_string_at(pCmd->pParamValues, 0);
  if (!strcmp(pcPrefix, "*"))
    pcPrefix= NULL;

  while ((pcAddr= cli_enum_bgp_routers_addr(pcPrefix, iState++)) != NULL) {
    log_printf(pLogOut, "%s\n", pcAddr);
  }
  return CLI_SUCCESS;
}

// ----- cli_bgp_show_sessions --------------------------------------
/**
 * Show the list of sessions.
 *
 * context: {}
 * tokens: {}
 */
int cli_bgp_show_sessions(SCliContext * pContext, SCliCmd * pCmd)
{
  SBGPRouter * pRouter;
  SBGPPeer * pPeer;
  int iState= 0, iState2;
  SLogStream * pStream= pLogOut;

  pStream= log_create_file("/tmp/cbgp-sessions.dat");

  while ((pRouter= cli_enum_bgp_routers(NULL, iState++)) != NULL) {
    iState2= 0;
    while ((pPeer= cli_enum_bgp_peers(pRouter, NULL, iState2++)) != NULL) {
      ip_address_dump(pStream, pRouter->pNode->tAddr);
      log_printf(pStream, "\t");
      ip_address_dump(pStream, pPeer->tAddr);
      log_printf(pStream, "\t%d\t%d\n", pPeer->uSendSeqNum,
		 pPeer->uRecvSeqNum);
    }
  }

  if (pStream != pLogOut)
    log_destroy(&pStream);

  return CLI_SUCCESS;
}

// -----[ cli_bgp_clearadjrib ]--------------------------------------
/**
 * context: ---
 * tokens : ---
 */
int cli_bgp_clearadjrib(SCliContext * pContext,
			SCliCmd * pCmd)
{
  SBGPRouter * pRouter;
  int iState= 0;

  while ((pRouter= cli_enum_bgp_routers(NULL, iState++)) != NULL) {
    bgp_router_clear_adjrib(pRouter);
  }

  return CLI_ERROR_COMMAND_FAILED;
}

// ----- cli_register_bgp_add ---------------------------------------
int cli_register_bgp_add(SCliCmds * pCmds)
{
  SCliCmds * pSubCmds;
  SCliParams * pParams;

  pSubCmds= cli_cmds_create();
  pParams= cli_params_create();
  cli_params_add(pParams, "<as-num>", NULL);
  cli_params_add(pParams, "<addr>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("router", cli_bgp_add_router,
					NULL, pParams));
  return cli_cmds_add(pCmds, cli_cmd_create("add", NULL, pSubCmds, NULL));
}

// ----- cli_register_bgp_assert ------------------------------------
int cli_register_bgp_assert(SCliCmds * pCmds)
{
  SCliCmds * pSubCmds= cli_cmds_create();

  cli_cmds_add(pSubCmds, cli_cmd_create("peerings-ok",
					cli_bgp_assert_peerings,
					NULL, NULL));
  cli_cmds_add(pSubCmds, cli_cmd_create("reachability-ok",
					cli_bgp_assert_reachability,
					NULL, NULL));
  return cli_cmds_add(pCmds, cli_cmd_create("assert", NULL,
					    pSubCmds, NULL));
}



// ----- cli_register_bgp_domain ------------------------------------
int cli_register_bgp_domain(SCliCmds * pCmds)
{
  SCliCmds * pSubCmds= cli_cmds_create();
  SCliParams * pParams;
  SCliCmd * pCmd;

  cli_cmds_add(pSubCmds, cli_cmd_create("full-mesh",
					cli_bgp_domain_full_mesh,
					NULL, NULL));
  cli_cmds_add(pSubCmds, cli_cmd_create("rescan",
					cli_bgp_domain_rescan,
					NULL, NULL));
  pParams= cli_params_create();
  cli_params_add(pParams, "<address|prefix>", NULL);
  pCmd= cli_cmd_create("record-route",
		       cli_bgp_domain_recordroute,
		       NULL, pParams);
  cli_cmd_add_option(pCmd, "deflection", NULL);
  cli_cmds_add(pSubCmds, pCmd);
  pParams= cli_params_create();
  cli_params_add(pParams, "<as-number>", NULL);
  return cli_cmds_add(pCmds, cli_cmd_create_ctx("domain",
						cli_ctx_create_bgp_domain,
						cli_ctx_destroy_bgp_domain,
						pSubCmds, pParams));
}

// ----- cli_register_bgp_router_add --------------------------------
int cli_register_bgp_router_add(SCliCmds * pCmds)
{
  SCliCmds * pSubCmds;
  SCliParams * pParams;

  pSubCmds= cli_cmds_create();
  pParams= cli_params_create();
  cli_params_add(pParams, "<as-num>", NULL);
  cli_params_add(pParams, "<addr>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("peer", cli_bgp_router_add_peer,
					NULL, pParams));
  pParams= cli_params_create();
  cli_params_add(pParams, "<prefix>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("network", cli_bgp_router_add_network,
					NULL, pParams));
#ifdef BGP_QOS
  pParams= cli_params_create();
  cli_params_add(pParams, "<prefix>", NULL);
  cli_params_add(pParams, "<delay>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("qos-network",
					cli_bgp_router_add_qosnetwork,
					NULL, pParams));
#endif
  return cli_cmds_add(pCmds, cli_cmd_create("add", NULL, pSubCmds, NULL));
}

// ----- cli_register_bgp_router_assert -----------------------------
int cli_register_bgp_router_assert(SCliCmds * pCmds)
{
  SCliCmds * pSubCmds= cli_cmds_create();
  SCliCmds * pSubSubCmds= cli_cmds_create();
  SCliParams * pParams;

  pParams= cli_params_create();
  cli_params_add(pParams, "<predicate>", NULL);
  cli_cmds_add(pSubSubCmds, cli_cmd_create("match",
					   cli_bgp_router_assert_routes_match,
					   NULL, pParams));
  cli_cmds_add(pSubSubCmds, cli_cmd_create("show",
					   cli_bgp_router_assert_routes_show,
					   NULL, NULL));
  pParams= cli_params_create();
  cli_params_add(pParams, "<prefix>", NULL);
  cli_params_add(pParams, "<best|feasible>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create_ctx("routes",
					    cli_ctx_create_bgp_router_assert_routes,
					    cli_ctx_destroy_bgp_router_assert_routes,
					    pSubSubCmds, pParams));
  return cli_cmds_add(pCmds, cli_cmd_create("assert", NULL,
					    pSubCmds, NULL));
}

// ----- cli_register_bgp_router_debug ------------------------------
int cli_register_bgp_router_debug(SCliCmds * pCmds)
{
  SCliCmds * pSubCmds= cli_cmds_create();
  SCliParams * pParams;

  pParams= cli_params_create();
  cli_params_add(pParams, "<prefix>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("dp",
					cli_bgp_router_debug_dp,
					NULL, pParams));
  return cli_cmds_add(pCmds, cli_cmd_create("debug", NULL,
					    pSubCmds, NULL));
}

// ----- cli_register_bgp_router_del --------------------------------
int cli_register_bgp_router_del(SCliCmds * pCmds)
{
  SCliCmds * pSubCmds;
  SCliParams * pParams;

  pSubCmds= cli_cmds_create();
  pParams= cli_params_create();
  cli_params_add(pParams, "<prefix>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("network",
					cli_bgp_router_del_network,
					NULL, pParams));
  return cli_cmds_add(pCmds, cli_cmd_create("del", NULL, pSubCmds, NULL));
}

// ----- cli_register_bgp_filter_rule -------------------------------
SCliCmds * cli_register_bgp_filter_rule()
{
  SCliCmds * pCmds= cli_cmds_create();
  SCliParams * pParams;

  pParams= cli_params_create();
  cli_params_add(pParams, "<predicate>", NULL);
  cli_cmds_add(pCmds, cli_cmd_create("match",
				     cli_bgp_filter_rule_match,
				     NULL,
				     pParams));
  pParams= cli_params_create();
  cli_params_add(pParams, "<actions>", NULL);
  cli_cmds_add(pCmds, cli_cmd_create("action",
				     cli_bgp_filter_rule_action,
				     NULL,
				     pParams));
  return pCmds;
}

// ----- cli_register_bgp_router_peer_filter ------------------------
/**
 * filter add
 * filter insert <pos>
 * filter remove <pos>
 *
 * filter X
 *   match ""
 *   action ""
 *   (...)
 */
int cli_register_bgp_router_peer_filter(SCliCmds * pCmds)
{
  SCliCmds * pSubCmds;
  SCliParams * pParams;

  pSubCmds= cli_cmds_create();
  cli_cmds_add(pSubCmds, cli_cmd_create_ctx("add-rule",
					    cli_ctx_create_bgp_filter_add_rule,
					    cli_ctx_destroy_bgp_filter_rule,
					    cli_register_bgp_filter_rule(), NULL));
  pParams= cli_params_create();
  cli_params_add(pParams, "<index>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create_ctx("insert-rule",
					    cli_ctx_create_bgp_filter_insert_rule,
					    cli_ctx_destroy_bgp_filter_rule,
					    cli_register_bgp_filter_rule(),
					    pParams));
  pParams= cli_params_create();
  cli_params_add(pParams, "<index>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("remove-rule",
					cli_bgp_filter_remove_rule,
					NULL, pParams));
  pParams= cli_params_create();
  cli_params_add(pParams, "<index>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create_ctx("rule",
					    cli_ctx_create_bgp_filter_rule,
					    cli_ctx_destroy_bgp_filter_rule,
					    cli_register_bgp_filter_rule(),
					    pParams));
  cli_cmds_add(pSubCmds, cli_cmd_create("show",
					cli_bgp_filter_show,
					NULL, NULL));
  pParams= cli_params_create();
  cli_params_add(pParams, "<in|out>", NULL);
  return cli_cmds_add(pCmds,
		      cli_cmd_create_ctx("filter",
					 cli_ctx_create_bgp_router_peer_filter,
					 cli_ctx_destroy_bgp_router_peer_filter,
					 pSubCmds, pParams));
}

// ----- cli_register_route_map_filters ------------------------------
/**
 *
 *
 */
int cli_register_bgp_route_map_filters(SCliCmds * pCmds)
{
  return cli_cmds_add(pCmds, cli_cmd_create_ctx("add-rule",
					    cli_ctx_create_bgp_filter_add_rule,
					    cli_ctx_destroy_bgp_filter_rule,
					    cli_register_bgp_filter_rule(), NULL));
}

// ----- cli_register_bgp_router_peer_filters -----------------------
int cli_register_bgp_router_peer_filters(SCliCmds * pCmds)
{
  SCliCmds * pSubCmds;
  SCliParams * pParams;

  pSubCmds= cli_cmds_create();
  pParams= cli_params_create();
  cli_params_add(pParams, "<filter>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("set",
					cli_bgp_router_peer_infilter_set,
					NULL, pParams));
  pParams= cli_params_create();
  cli_params_add(pParams, "<filter>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("add",
					cli_bgp_router_peer_infilter_add,
					NULL, pParams));
  cli_cmds_add(pCmds, cli_cmd_create("in-filter", NULL,
				     pSubCmds, NULL));
  pSubCmds= cli_cmds_create();
  pParams= cli_params_create();
  cli_params_add(pParams, "<filter>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("set",
					cli_bgp_router_peer_outfilter_set,
					NULL, pParams));
  pParams= cli_params_create();
  cli_params_add(pParams, "<filter>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("add",
					cli_bgp_router_peer_outfilter_add,
					NULL, pParams));
  return cli_cmds_add(pCmds, cli_cmd_create("out-filter", NULL,
					    pSubCmds, NULL));
}

// ----- cli_register_bgp_router_peer_show --------------------------
int cli_register_bgp_router_peer_show(SCliCmds * pCmds)
{
  SCliCmds * pSubCmds;
  SCliParams * pParams;
  
  pSubCmds= cli_cmds_create();
  pParams= cli_params_create();
  cli_params_add(pParams, "<in|out>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("filter",
					cli_bgp_router_peer_show_filter,
					NULL, pParams));
  return cli_cmds_add(pCmds, cli_cmd_create("show", NULL,
					    pSubCmds, NULL));
}

// ----- bgp_route_map -----------------------------------------------
/**
 * route-map route_map_name 
 * 
 */
int cli_register_bgp_route_map(SCliCmds * pCmds)
{
  SCliCmds * pSubCmds;
  SCliParams * pParams;

  pSubCmds= cli_cmds_create();
  cli_register_bgp_route_map_filters(pSubCmds);
  
  pParams=cli_params_create();
  cli_params_add(pParams, "<route-map name>", NULL);
//  cli_params_add(pParams, "<seq>", NULL);

  return cli_cmds_add(pCmds, cli_cmd_create_ctx("route-map", 
					  cli_ctx_create_bgp_route_map, 
					  cli_ctx_destroy_bgp_route_map,
					  pSubCmds, pParams));
}

// ----- cli_register_bgp_router_peer -------------------------------
int cli_register_bgp_router_peer(SCliCmds * pCmds)
{
  SCliCmds * pSubCmds;
  SCliParams * pParams;

  pSubCmds= cli_cmds_create();
  cli_register_bgp_router_peer_filter(pSubCmds);
  cli_register_bgp_router_peer_filters(pSubCmds);
  cli_register_bgp_router_peer_show(pSubCmds);
  cli_cmds_add(pSubCmds, cli_cmd_create("down", cli_bgp_router_peer_down,
					NULL, NULL));
  pParams= cli_params_create();
  cli_params_add(pParams, "<next-hop>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("next-hop",
					cli_bgp_router_peer_nexthop,
					NULL, pParams));
  cli_cmds_add(pSubCmds, cli_cmd_create("next-hop-self",
					cli_bgp_router_peer_nexthopself,
					NULL, NULL));
#ifdef __EXPERIMENTAL__
  pParams= cli_params_create();
  cli_params_add(pParams, "<prefix|*>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("re-adv",
					cli_bgp_router_peer_readv,
					NULL, pParams));
#endif
  pParams= cli_params_create();
  cli_params_add(pParams, "<file>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("record",
					cli_bgp_router_peer_record,
					NULL, pParams));
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  pParams = cli_params_create();
  cli_params_add(pParams, "<announce-limit>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("walton-limit", 
					cli_bgp_router_peer_walton_limit,
					NULL, pParams));
#endif
  pParams= cli_params_create();
  cli_params_add(pParams, "<mrt-record>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("recv",
					cli_bgp_router_peer_recv,
					NULL, pParams));
  cli_cmds_add(pSubCmds, cli_cmd_create("reset", cli_bgp_router_peer_reset,
					NULL, NULL));
  cli_cmds_add(pSubCmds, cli_cmd_create("rr-client",
					cli_bgp_router_peer_rrclient,
					NULL, NULL));
  cli_cmds_add(pSubCmds, cli_cmd_create("soft-restart",
					cli_bgp_router_peer_softrestart,
					NULL, NULL));
  cli_cmds_add(pSubCmds, cli_cmd_create("up", cli_bgp_router_peer_up,
					NULL, NULL));
  cli_cmds_add(pSubCmds, cli_cmd_create("virtual",
					cli_bgp_router_peer_virtual,
					NULL, NULL));
  pParams= cli_params_create();
  cli_params_add2(pParams, "<addr>", NULL, cli_enum_bgp_routers_addr);
  return cli_cmds_add(pCmds, cli_cmd_create_ctx("peer",
						cli_ctx_create_bgp_router_peer,
						cli_ctx_destroy_bgp_router_peer,
						pSubCmds, pParams));
}

// ----- cli_register_bgp_options -----------------------------------
int cli_register_bgp_options(SCliCmds * pCmds)
{
  SCliCmds * pSubCmds;
  SCliParams * pParams;

  pSubCmds= cli_cmds_create();
  pParams= cli_params_create();
  cli_params_add(pParams, "<on-off>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("auto-create",
					cli_bgp_options_autocreate,
					NULL, pParams));
  pParams= cli_params_create();
  cli_params_add(pParams, "<med-type>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("med", cli_bgp_options_med,
					NULL, pParams));
  pParams= cli_params_create();
  cli_params_add(pParams, "<local-pref>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("local-pref",
					cli_bgp_options_localpref,
					NULL, pParams));
  pParams= cli_params_create();
  cli_params_add(pParams, "<output-file>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("msg-monitor",
					cli_bgp_options_msgmonitor,
					NULL, pParams));
  pParams= cli_params_create();
  cli_params_add(pParams, "<cisco|mrt|custom>", NULL);
  cli_params_add_vararg(pParams, "[format]", 1, NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("show-mode",
					cli_bgp_options_showmode,
					NULL, pParams));
#ifdef __EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
  pParams = cli_params_create();
  cli_params_add(pParams, "<on-off>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("advertise-external-best",
					cli_bgp_options_advertise_external_best,
					NULL, pParams));
#endif
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  pParams = cli_params_create();
  cli_params_add(pParams, "<all|best>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("walton-convergence",
					cli_bgp_options_walton_convergence,
					NULL, pParams));
#endif
  /*pParams= cli_params_create();
  cli_params_add(pParams, "<function>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("tie-break",
					cli_bgp_options_tiebreak,
					NULL, pParams));*/
  return cli_cmds_add(pCmds, cli_cmd_create("options", NULL,
					    pSubCmds, NULL));
}



// ----- cli_register_bgp_router_load -------------------------------
int cli_register_bgp_router_load(SCliCmds * pCmds)
{
  SCliCmds * pSubCmds;
  SCliParams * pParams;
  SCliCmd * pCmd;

  pSubCmds= cli_cmds_create();
  pParams= cli_params_create();
  cli_params_add_file(pParams, "<file>", NULL);
  pCmd= cli_cmd_create("rib", cli_bgp_router_load_rib, NULL, pParams);
  cli_cmd_add_option(pCmd, "force", NULL);
  cli_cmd_add_option(pCmd, "format", NULL);
  cli_cmd_add_option(pCmd, "summary", NULL);
  cli_cmds_add(pSubCmds, pCmd);
#ifdef __EXPERIMENTAL__
#ifdef COMMENT_BQU
  pParams = cli_params_create();
  cli_params_add(pParams, "<file>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("rib-in", 
					cli_bgp_router_load_ribs_in,
					NULL, pParams));
#endif /* COMMENT_BQU */
#endif /* __EXPERIMENTAL__ */
  return cli_cmds_add(pCmds, cli_cmd_create("load", NULL,
					    pSubCmds, NULL));
}

// ----- cli_register_bgp_router_save -------------------------------
int cli_register_bgp_router_save(SCliCmds * pCmds)
{
  SCliCmds * pSubCmds;
  SCliParams * pParams;

  pSubCmds= cli_cmds_create();
  pParams= cli_params_create();
  cli_params_add_file(pParams, "<file>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("rib",
					cli_bgp_router_save_rib,
					NULL, pParams));
  pParams= cli_params_create();
  cli_params_add(pParams, "<peer>", NULL);
  cli_params_add_file(pParams, "<file>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("rib-in",
					cli_bgp_router_save_ribin,
					NULL, pParams));
  return cli_cmds_add(pCmds, cli_cmd_create("save", NULL,
					    pSubCmds, NULL));
}

// ----- cli_register_bgp_router_set --------------------------------
int cli_register_bgp_router_set(SCliCmds * pCmds)
{
  SCliCmds * pSubCmds;
  SCliParams * pParams;

  pSubCmds= cli_cmds_create();
  pParams= cli_params_create();
  cli_params_add(pParams, "<cluster-id>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("cluster-id",
					cli_bgp_router_set_clusterid,
					NULL, pParams));
  /*pParams= cli_params_create();
  cli_params_add(pParams, "<function>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("tie-break",
				     cli_bgp_router_set_tiebreak,
				     NULL, pParams));*/
  return cli_cmds_add(pCmds, cli_cmd_create("set", NULL,
					    pSubCmds, NULL));
}

// ----- cli_register_bgp_router_show -------------------------------
int cli_register_bgp_router_show(SCliCmds * pCmds)
{
  SCliCmds * pSubCmds;
  SCliParams * pParams;

  pSubCmds= cli_cmds_create();
  cli_cmds_add(pSubCmds, cli_cmd_create("info",
					cli_bgp_router_show_info,
					NULL, NULL));
  cli_cmds_add(pSubCmds, cli_cmd_create("networks",
					cli_bgp_router_show_networks,
					NULL, NULL));
  cli_cmds_add(pSubCmds, cli_cmd_create("peers",
					cli_bgp_router_show_peers,
					NULL, NULL));
  pParams= cli_params_create();
  cli_params_add(pParams, "<peer>", NULL);
  cli_params_add(pParams, "<prefix|address|*>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("rib-in",
					cli_bgp_router_show_ribin,
					NULL, pParams));
  pParams= cli_params_create();
  cli_params_add(pParams, "<peer>", NULL);
  cli_params_add(pParams, "<prefix|address|*>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("rib-out",
					cli_bgp_router_show_ribout,
					NULL, pParams));
  pParams= cli_params_create();
  cli_params_add(pParams, "<prefix|address|*>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("rib",
					cli_bgp_router_show_rib,
					NULL, pParams));
  pParams= cli_params_create();
  cli_params_add(pParams, "<prefix|address|*>", NULL);
  cli_params_add_vararg(pParams, "<output>", 1, NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("route-info",
					cli_bgp_router_show_routeinfo,
					NULL, pParams));
  cli_cmds_add(pSubCmds, cli_cmd_create("stats",
					cli_bgp_router_show_stats,
					NULL, NULL));
  return cli_cmds_add(pCmds, cli_cmd_create("show", NULL,
					    pSubCmds, NULL));
}

// ----- cli_register_bgp_router ------------------------------------
int cli_register_bgp_router(SCliCmds * pCmds)
{
  SCliCmds * pSubCmds;
  SCliParams * pParams;

  pSubCmds= cli_cmds_create();
  cli_register_bgp_router_add(pSubCmds);
  cli_register_bgp_router_assert(pSubCmds);
  cli_register_bgp_router_debug(pSubCmds);
  cli_register_bgp_router_del(pSubCmds);
  cli_register_bgp_router_peer(pSubCmds);
  pParams= cli_params_create();
  cli_params_add(pParams, "<prefix>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("record-route",
					cli_bgp_router_recordroute,
					NULL, pParams));
  pParams= cli_params_create();
  cli_params_add(pParams, "<prefix|*>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("rerun",
					cli_bgp_router_rerun,
					NULL, pParams));
  cli_cmds_add(pSubCmds, cli_cmd_create("rescan",
					cli_bgp_router_rescan,
					NULL, NULL));
  cli_cmds_add(pSubCmds, cli_cmd_create("reset",
					cli_bgp_router_reset,
					NULL, NULL));
  cli_cmds_add(pSubCmds, cli_cmd_create("start",
					cli_bgp_router_start,
					NULL, NULL));
  cli_cmds_add(pSubCmds, cli_cmd_create("stop",
					cli_bgp_router_stop,
					NULL, NULL));
  cli_register_bgp_router_load(pSubCmds);
  cli_register_bgp_router_save(pSubCmds);
  cli_register_bgp_router_set(pSubCmds);
  cli_register_bgp_router_show(pSubCmds);
  pParams= cli_params_create();
  cli_params_add2(pParams, "<addr>", NULL, cli_enum_bgp_routers_addr);
  return cli_cmds_add(pCmds, cli_cmd_create_ctx("router",
						cli_ctx_create_bgp_router,
						cli_ctx_destroy_bgp_router, 
						pSubCmds, pParams));
}

// ----- cli_register_bgp_show --------------------------------------
int cli_register_bgp_show(SCliCmds * pCmds)
{
  SCliCmds * pSubCmds;
  SCliParams * pParams;

  pSubCmds= cli_cmds_create();
  pParams= cli_params_create();
  cli_params_add(pParams, "<prefix|*>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("routers", cli_bgp_show_routers,
					NULL, pParams));
  cli_cmds_add(pSubCmds, cli_cmd_create("sessions", cli_bgp_show_sessions,
					NULL, NULL));
  return cli_cmds_add(pCmds, cli_cmd_create("show", NULL,
					    pSubCmds, NULL));
}

// ----- cli_register_bgp -------------------------------------------
/**
 *
 */
int cli_register_bgp(SCli * pCli)
{
  SCliCmds * pCmds;

  pCmds= cli_cmds_create();
  cli_register_bgp_route_map(pCmds);
  cli_register_bgp_add(pCmds);
  cli_register_bgp_assert(pCmds);
  cli_register_bgp_domain(pCmds);
  cli_register_bgp_options(pCmds);
  cli_register_bgp_topology(pCmds);
  cli_register_bgp_router(pCmds);
  cli_register_bgp_show(pCmds);
  cli_cmds_add(pCmds, cli_cmd_create("clear-adj-rib", cli_bgp_clearadjrib,
				     NULL, NULL));
  cli_register_cmd(pCli, cli_cmd_create("bgp", NULL, pCmds, NULL));
  return 0;
}
