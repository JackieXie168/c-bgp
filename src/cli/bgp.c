// ==================================================================
// @(#)bgp.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be), 
// @author Sebastien Tandel (standel@info.ucl.ac.be)
// @date 15/07/2003
// @lastdate 10/04/2006
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
#include <bgp/rexford.h>
#include <bgp/route_map.h>
#include <bgp/tie_breaks.h>

#include <cli/bgp.h>
#include <cli/common.h>
#include <cli/net.h>
#include <ui/rl.h>
#include <libgds/cli_ctx.h>
#include <libgds/log.h>
#include <libgds/memory.h>
#include <net/node.h>
#include <net/prefix.h>
#include <net/record-route.h>
#include <string.h>

// ----- cli_bgp_enum_nodes -----------------------------------------
char * cli_bgp_enum_nodes(const char * pcText, int state)
{
  return network_enum_bgp_nodes(pcText, state);
}

// ----- cli_bgp_add_router -----------------------------------------
/**
 * This function adds a BGP protocol instance to the given node
 * (identified by its address).
 *
 * context: {}
 * tokens: {addr, as-num}
 */
int cli_bgp_add_router(SCliContext * pContext, STokens * pTokens)
{
  char * pcNodeAddr;
  SNetNode * pNode;
  SBGPRouter * pRouter;
  unsigned int uASNum;

  // Find node
  pcNodeAddr= tokens_get_string_at(pTokens, 1);
  pNode= cli_net_node_by_addr(pcNodeAddr);
  if (pNode == NULL) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid node address \"%s\"\n",
	    pcNodeAddr);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Check AS-Number
  if (tokens_get_uint_at(pTokens, 0, &uASNum) || (uASNum > MAX_AS)) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid AS_Number\n");
    return CLI_ERROR_COMMAND_FAILED;
  }

  pRouter= bgp_router_create(uASNum, pNode);

  // Register BGP protocol into the node
  if (node_register_protocol(pNode, NET_PROTOCOL_BGP, pRouter,
			     (FNetNodeHandlerDestroy) bgp_router_destroy,
			     bgp_router_handle_message)) {
    LOG_ERR(LOG_LEVEL_SEVERE,
	    "Error: could not register BGP protocol on node \"%s\"\n",
	    pcNodeAddr);
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// ----- cli_bgp_assert_peerings ------------------------------------
/**
 * This function checks that all the BGP peerings defined in BGP
 * instances are valid, i.e. the peer exists and is reachable.
 */
int cli_bgp_assert_peerings(SCliContext * pContext, STokens * pTokens)
{
  // Test the validity of peerings in all BGP instances
  if (bgp_assert_peerings()) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: peerings assertion failed.\n");
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
int cli_bgp_assert_reachability(SCliContext * pContext, STokens * pTokens)
{
  // Test the reachability of all advertised networks from all BGP
  // instances
  if (bgp_assert_reachability()) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: reachability assertion failed.\n");
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
  if (tokens_get_uint_at(pContext->pTokens, 0, &uASNumber) != 0) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid AS number \"%s\"\n",
	    tokens_get_string_at(pContext->pTokens, 0));
    return CLI_ERROR_CTX_CREATE;
  }

  /* Check that the BGP domain exists */
  if (!exists_bgp_domain((uint16_t) uASNumber)) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: domain \"%u\" does not exist.\n",
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
int cli_bgp_domain_full_mesh(SCliContext * pContext, STokens * pTokens)
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
int cli_bgp_domain_rescan(SCliContext * pContext, STokens * pTokens)
{
  SBGPDomain * pDomain;

  /* Get the domain from the context. */
  pDomain= (SBGPDomain *) cli_context_get_item_at_top(pContext);

  /* Rescan all the routers in the domain. */
  bgp_domain_rescan(pDomain);

  return CLI_SUCCESS;
}

// ----- cli_net_node_recordroutedeflection ------------------------------
/**
 * context: {as}
 * tokens: {addr}
 */
int cli_bgp_domain_recordroutedeflection(SCliContext * pContext,
				  STokens * pTokens)
{
  SBGPDomain * pDomain;
  char * pcDest;
  SNetDest sDest;

 // Get node from the CLI'scontext
  pDomain= (SBGPDomain *) cli_context_get_item_at_top(pContext);
  if (pDomain == NULL)
    return CLI_ERROR_COMMAND_FAILED;

  // Get destination address
  pcDest= tokens_get_string_at(pTokens, 1);
  if (ip_string_to_dest(pcDest, &sDest)) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid prefix|address|* \"%s\"\n",
	    pcDest);
    return CLI_ERROR_COMMAND_FAILED;
  }

  /* Check that the destination type is adress/prefix */
  if ((sDest.tType != NET_DEST_ADDRESS) &&
      (sDest.tType != NET_DEST_PREFIX)) {
    LOG_ERR(LOG_LEVEL_SEVERE,
	    "Error: can not use this destination type with record-route\n");
    return CLI_ERROR_COMMAND_FAILED;
  }

  bgp_domain_dump_recorded_route(pLogOut, pDomain, sDest,
				 NET_RECORD_ROUTE_OPTION_DEFLECTION);

  return CLI_SUCCESS;
}

// ----- cli_net_node_recordroute ------------------------------------
/**
 * context: {as}
 * tokens: {addr}
 */
int cli_bgp_domain_recordroute(SCliContext * pContext,
				  STokens * pTokens)
{
  SBGPDomain * pDomain;
  char * pcDest;
  SNetDest sDest;

 // Get node from the CLI'scontext
  pDomain= (SBGPDomain *) cli_context_get_item_at_top(pContext);
  if (pDomain == NULL)
    return CLI_ERROR_COMMAND_FAILED;

  // Get destination address
  pcDest= tokens_get_string_at(pTokens, 1);
  if (ip_string_to_dest(pcDest, &sDest)) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid prefix|address|* \"%s\"\n",
	    pcDest);
    return CLI_ERROR_COMMAND_FAILED;
  }

  /* Check that the destination type is adress/prefix */
  if ((sDest.tType != NET_DEST_ADDRESS) &&
      (sDest.tType != NET_DEST_PREFIX)) {
    LOG_ERR(LOG_LEVEL_SEVERE,
	    "Error: can not use this destination type with record-route\n");
    return CLI_ERROR_COMMAND_FAILED;
  }

  bgp_domain_dump_recorded_route(pLogOut, pDomain, sDest, 0);

  return CLI_SUCCESS;
}


// ----- cli_bgp_topology_load --------------------------------------
/**
 * context: {}
 * tokens: {file}
 */
int cli_bgp_topology_load(SCliContext * pContext, STokens * pTokens)
{
  if (rexford_load(tokens_get_string_at(pTokens, 0)))
    return CLI_ERROR_COMMAND_FAILED;
  return CLI_SUCCESS;
}

// ----- cli_bgp_topology_policies ----------------------------------
/**
 * context: {}
 * tokens: {}
 */
int cli_bgp_topology_policies(SCliContext * pContext, STokens * pTokens)
{
  rexford_setup_policies();
  return CLI_SUCCESS;
}

// ----- cli_bgp_topology_recordroute -------------------------------
/**
 * context: {}
 * tokens: {prefix, file-in, file-out}
 */
int cli_bgp_topology_recordroute(SCliContext * pContext,
				 STokens * pTokens)
{
  SPrefix sPrefix;
  char * pcEndPtr;
  SLogStream * pOutput;
  int iResult= CLI_SUCCESS;

  if (ip_string_to_prefix(tokens_get_string_at(pTokens, 0), &pcEndPtr,
			  &sPrefix) || (*pcEndPtr != '\0'))
    return CLI_ERROR_COMMAND_FAILED;
  if ((pOutput= log_create_file(tokens_get_string_at(pTokens, 2))) == NULL)
    return CLI_ERROR_COMMAND_FAILED;
  if (rexford_record_route(pOutput, tokens_get_string_at(pTokens, 1), sPrefix))
    iResult= CLI_ERROR_COMMAND_FAILED;
  log_destroy(&pOutput);
  return iResult;
}

// ----- cli_bgp_topology_recordroute_bm ----------------------------
/**
 * context: {}
 * tokens: {prefix, bound, file-in, file-out}
 */
#ifdef __EXPERIMENTAL__
int cli_bgp_topology_recordroute_bm(SCliContext * pContext,
				    STokens * pTokens)
{
  char * pcPrefix;
  SPrefix sPrefix;
  char * pcEndPtr;
  SLogStream * pOutput;
  int iResult= CLI_SUCCESS;
  unsigned int uBound;

  // Get prefix
  pcPrefix= tokens_get_string_at(pTokens, 0);
  if (ip_string_to_prefix(pcPrefix, &pcEndPtr, &sPrefix) ||
      (*pcEndPtr != '\0')) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid prefix \"%s\"\n", pcPrefix);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Get prefix bound
  if (tokens_get_uint_at(pTokens, 1, &uBound) || (uBound > 32)) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid prefix bound \"%s\"\n",
	    tokens_get_string_at(pTokens, 1));
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Create output
  if ((pOutput= log_create_file(tokens_get_string_at(pTokens, 3))) == NULL) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: unable to create \"%s\"\n",
	    tokens_get_string_at(pTokens, 3));
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Record route
  if (rexford_record_route_bm(pOutput, tokens_get_string_at(pTokens, 2),
			      sPrefix, (uint8_t) uBound))
    iResult= CLI_ERROR_COMMAND_FAILED;

  log_destroy(&pOutput);

  return iResult;
}
#endif

// ----- cli_bgp_topology_showrib -----------------------------------
/**
 * context: {}
 * tokens: {}
 */
int cli_bgp_topology_showrib(SCliContext * pContext,
			     STokens * pTokens)
{
  return CLI_ERROR_COMMAND_FAILED;
}

// ----- cli_bgp_topology_route_dp_rule -----------------------------
/**
 * context: {}
 * tokens: {prefix, file-out}
 */
/*
int cli_bgp_topology_route_dp_rule(SCliContext * pContext,
				   STokens * pTokens)
{
  SPrefix sPrefix;
  char * pcEndPtr;
  FILE * fOutput;
  int iResult= CLI_SUCCESS;

  if (ip_string_to_prefix(tokens_get_string_at(pTokens, 0), &pcEndPtr,
			  &sPrefix) || (*pcEndPtr != '\0'))
    return CLI_ERROR_COMMAND_FAILED;
  if ((fOutput= fopen(tokens_get_string_at(pTokens, 1), "w")) == NULL)
    return CLI_ERROR_COMMAND_FAILED;
  if (rexford_route_dp_rule(fOutput, sPrefix))
    iResult= CLI_ERROR_COMMAND_FAILED;
  fclose(fOutput);
  return iResult;
}
*/

// ----- cli_bgp_topology_run ---------------------------------------
/**
 * context: {}
 * tokens: {}
 */
int cli_bgp_topology_run(SCliContext * pContext, STokens * pTokens)
{
  rexford_run();
  return CLI_SUCCESS;
}

// ----- cli_ctx_create_bgp_route_map --------------------------------
/**
 * context: {} -> {&filter}
 * tokens: {}
 */
int cli_ctx_create_bgp_route_map(SCliContext * pContext, void ** ppItem)
{
  char * pcRouteMapName, * pcToken;
  SFilter ** ppFilter;

  pcRouteMapName = pcToken = tokens_get_string_at(pContext->pTokens, 0);
  
  if (pcRouteMapName != NULL) {
    if (route_map_get(pcRouteMapName) != NULL) {
      LOG_ERR(LOG_LEVEL_SEVERE, "Error: Route Map %s exists.\n",
	      pcRouteMapName);
      return CLI_ERROR_CTX_CREATE;
    }
    ppFilter = MALLOC(sizeof(SFilter *));
    *ppFilter = filter_create();
    pcRouteMapName = MALLOC(strlen(pcToken)+1);
    strcpy(pcRouteMapName, pcToken);
    route_map_add(pcRouteMapName, *ppFilter);
    *ppItem = ppFilter;
  } else {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: No Route Map name.\n");
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
  pcNodeAddr= tokens_get_string_at(pContext->pTokens, 0);
  pNode= cli_net_node_by_addr(pcNodeAddr);
  if (pNode == NULL) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid node address \"%s\"\n",
	    pcNodeAddr);
    return CLI_ERROR_CTX_CREATE;
  }

  // Get BGP protocol instance
  pProtocol= protocols_get(pNode->pProtocols, NET_PROTOCOL_BGP);
  if (pProtocol == NULL) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: BGP is not supported on node \"%s\"\n",
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
int cli_bgp_options_autocreate(SCliContext * pContext, STokens * pTokens)
{
  char * pcParam;

  pcParam= tokens_get_string_at(pTokens, 0);
  if (!strcmp(pcParam, "on"))
    BGP_OPTIONS_AUTO_CREATE= 1;
  else if (!strcmp(pcParam, "off"))
    BGP_OPTIONS_AUTO_CREATE= 0;
  else {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid value \"%s\"\n", pcParam);
    return CLI_ERROR_COMMAND_FAILED;
  }
  return CLI_SUCCESS;
}

// ----- cli_bgp_options_showmode ------------------------------------
/**
 * context: {}
 * tokens: {mode}
 */
int cli_bgp_options_showmode(SCliContext * pContext, STokens * pTokens)
{
  char * pcParam;

  pcParam= tokens_get_string_at(pTokens, 0);
  if (!strcmp(pcParam, "cisco"))
    BGP_OPTIONS_SHOW_MODE= ROUTE_SHOW_CISCO;
  else if (!strcmp(pcParam, "mrt"))
    BGP_OPTIONS_SHOW_MODE= ROUTE_SHOW_MRT;
  else {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: unknown show mode \"%s\"\n", pcParam);
    return CLI_ERROR_COMMAND_FAILED;
  }
  return CLI_SUCCESS;
}

// ----- cli_bgp_options_tiebreak ------------------------------------
/**
 * context: {}
 * tokens: {function}
 */
/*int cli_bgp_options_tiebreak(SCliContext * pContext, STokens * pTokens)
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
int cli_bgp_options_med(SCliContext * pContext, STokens * pTokens)
{
  char * pcMedType= tokens_get_string_at(pTokens, 0);
  
  if (!strcmp(pcMedType, "deterministic")) {
    BGP_OPTIONS_MED_TYPE= BGP_MED_TYPE_DETERMINISTIC;
  } else if (!strcmp(pcMedType, "always-compare")) {
    BGP_OPTIONS_MED_TYPE= BGP_MED_TYPE_ALWAYS_COMPARE;
  } else {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: unknown med-type \"%s\"\n", pcMedType);
    return CLI_ERROR_COMMAND_FAILED;
  }
  return CLI_SUCCESS;
}

// ----- cli_bgp_options_localpref ----------------------------------
/**
 * context: {}
 * tokens: {local-pref}
 */
int cli_bgp_options_localpref(SCliContext * pContext, STokens * pTokens)
{
  unsigned long int ulLocalPref;

  if (tokens_get_ulong_at(pTokens, 0, &ulLocalPref)) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid default local-pref \"%s\"\n",
	    tokens_get_string_at(pTokens, 0));
    return CLI_ERROR_COMMAND_FAILED;
  }
  BGP_OPTIONS_DEFAULT_LOCAL_PREF= ulLocalPref;
  return CLI_SUCCESS;
}

// ----- cli_bgp_options_msgmonitor ---------------------------------
/**
 * context: {}
 * tokens: {output-file}
 */
int cli_bgp_options_msgmonitor(SCliContext * pContext, STokens * pTokens)
{
  char * pcParam;

  pcParam= tokens_get_string_at(pTokens, 0);
  bgp_msg_monitor_open(pcParam);
  return CLI_SUCCESS;
}

// ----- cli_bgp_options_nlri ---------------------------------------
/**
 * context: {}
 * tokens: {nlri}
 */
int cli_bgp_options_nlri(SCliContext * pContext, STokens * pTokens)
{
  char * pcParam;

  pcParam= tokens_get_string_at(pTokens, 0);
  if (!strcmp(pcParam, "be"))
    BGP_OPTIONS_NLRI= BGP_NLRI_BE;
  else if (!strcmp(pcParam, "qos-delay"))
    BGP_OPTIONS_NLRI= BGP_NLRI_QOS_DELAY;
  else
    return CLI_ERROR_COMMAND_FAILED;
  return CLI_SUCCESS;
}

// ----- cli_bgp_options_qosaggrlimit -------------------------------
/**
 * context: {}
 * tokens: {limit}
 */
#ifdef BGP_QOS
int cli_bgp_options_qosaggrlimit(SCliContext * pContext,
				 STokens * pTokens)
{
  unsigned int uLimit;

  if (tokens_get_uint_at(pTokens, 0, &uLimit))
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
				      STokens * pTokens)
{
  char * pcParam;

  pcParam= tokens_get_string_at(pTokens, 0);
  if (!strcmp(pcParam, "best"))
    BGP_OPTIONS_WALTON_CONVERGENCE_ON_BEST = 1;
  else if (!strcmp(pcParam, "all"))
    BGP_OPTIONS_WALTON_CONVERGENCE_ON_BEST = 0;
  else {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid value \"%s\"\n", pcParam);
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
					STokens * pTokens)
{
  char * pcParam;

  pcParam= tokens_get_string_at(pTokens, 0);
  if (!strcmp(pcParam, "on"))
    BGP_OPTIONS_ADVERTISE_EXTERNAL_BEST= 1;
  else if (!strcmp(pcParam, "off"))
    BGP_OPTIONS_ADVERTISE_EXTERNAL_BEST= 0;
  else {
    LOG_SEVERE("Error: invalid value \"%s\"\n", pcParam);
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
 * tokens: {addr, cluster-id}
 */
int cli_bgp_router_set_clusterid(SCliContext * pContext,
				 STokens * pTokens)
{
  SBGPRouter * pRouter;
  unsigned int uClusterID;

  // Get the BGP instance from the context
  pRouter= (SBGPRouter *) cli_context_get_item_at_top(pContext);

  // Get the cluster-id
  if (tokens_get_uint_at(pTokens, 1, &uClusterID)) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid cluster-id\n");
    return CLI_ERROR_COMMAND_FAILED;
  }

  pRouter->tClusterID= uClusterID;
  pRouter->iRouteReflector= 1;
  return CLI_SUCCESS;
}

// ----- cli_bgp_router_debug_dp ------------------------------------
/**
 * context: {router}
 * tokens: {addr, prefix}
 */
int cli_bgp_router_debug_dp(SCliContext * pContext, STokens * pTokens)
{
  SBGPRouter * pRouter;
  char * pcPrefix;
  char * pcEndPtr;
  SPrefix sPrefix;

  // Get the BGP router from the context
  pRouter= (SBGPRouter *) cli_context_get_item_at_top(pContext);

  // Get the prefix
  pcPrefix= tokens_get_string_at(pTokens, 1);
  if (ip_string_to_prefix(pcPrefix, &pcEndPtr, &sPrefix) ||
      (*pcEndPtr != '\0')) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid prefix \"%s\"\n", pcPrefix);
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
 * tokens: {addr, file}
 */
int cli_bgp_router_load_rib(SCliContext * pContext,
			    STokens * pTokens)
{
  char * pcFileName;
  SBGPRouter * pRouter;

  // Get the BGP instance from the context
  pRouter= (SBGPRouter *) cli_context_get_item_at_top(pContext);

  // Get the MRTD file name
  pcFileName= tokens_get_string_at(pTokens, 1);

  // Load the MRTD file 
  if (bgp_router_load_rib(pcFileName, pRouter)) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: could not load \"%s\"\n", pcFileName);
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
				STokens * pTokens)
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
 * tokens: {addr, file}
 */
int cli_bgp_router_save_rib(SCliContext * pContext,
			    STokens * pTokens)
{
  SBGPRouter * pRouter;
  char * pcFileName;

  // Get the BGP instance from the context
  pRouter= (SBGPRouter *) cli_context_get_item_at_top(pContext);

  // Get the file name
  pcFileName= tokens_get_string_at(pTokens, 1);

  // Save the Loc-RIB
  if (bgp_router_save_rib(pcFileName, pRouter)) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: unable to save into \"%s\"\n",
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
 * tokens: {addr, file}
 */
int cli_bgp_router_save_ribin(SCliContext * pContext,
			      STokens * pTokens)
{
  LOG_ERR(LOG_LEVEL_SEVERE, "Error: sorry, not implemented\n");
  return CLI_ERROR_COMMAND_FAILED;
}

// ----- cli_bgp_router_show_info -----------------------------------
/**
 * context: {router}
 * tokens: {addr}
 */
int cli_bgp_router_show_info(SCliContext * pContext,
			     STokens * pTokens)
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
 * tokens: {addr}
 */
int cli_bgp_router_show_networks(SCliContext * pContext,
				 STokens * pTokens)
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
 * tokens: {addr}
 */
int cli_bgp_router_show_peers(SCliContext * pContext,
			      STokens * pTokens)
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
 * tokens: {addr, prefix|address|*}
 */
int cli_bgp_router_show_rib(SCliContext * pContext,
			    STokens * pTokens)
{
  SBGPRouter * pRouter;
  char * pcPrefix;
  char * pcEndChar;
  SPrefix sPrefix;

  // Get the BGP instance from the context
  pRouter= (SBGPRouter *) cli_context_get_item_at_top(pContext);

  // Get the prefix/address/*
  pcPrefix= tokens_get_string_at(pTokens, 1);
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
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid prefix|address|* \"%s\"\n",
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
 * tokens: {addr, addr, prefix|address|*}
 */
int cli_bgp_router_show_ribin(SCliContext * pContext,
			      STokens * pTokens)
{
  SBGPRouter * pRouter;
  char * pcPeerAddr;
  char * pcEndChar;
  net_addr_t tPeerAddr;
  SPeer * pPeer;
  char * pcPrefix;
  SPrefix sPrefix;

  // Get the BGP instance from the context
  pRouter= (SBGPRouter *) cli_context_get_item_at_top(pContext);

  // Get the peer/*
  pcPeerAddr= tokens_get_string_at(pTokens, 1);
  if (!strcmp(pcPeerAddr, "*")) {
    pPeer= NULL;
  } else if (!ip_string_to_address(pcPeerAddr, &pcEndChar, &tPeerAddr)
	     && (*pcEndChar == 0)) {
    pPeer= bgp_router_find_peer(pRouter, tPeerAddr);
    if (pPeer == NULL) {
      LOG_ERR(LOG_LEVEL_SEVERE, "Error: unknown peer \"%s\"\n", pcPeerAddr);
      return CLI_ERROR_COMMAND_FAILED;
    }
  } else {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid peer address \"%s\"\n", pcPeerAddr);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Get the prefix/address/*
  pcPrefix= tokens_get_string_at(pTokens, 2);
  if (!strcmp(pcPrefix, "*")) {
    sPrefix.uMaskLen= 0;
  } else if (!ip_string_to_prefix(pcPrefix, &pcEndChar, &sPrefix) &&
	     (*pcEndChar == 0)) {
  } else if (!ip_string_to_address(pcPrefix, &pcEndChar,
				   &sPrefix.tNetwork) &&
	     (*pcEndChar == 0)) {
    sPrefix.uMaskLen= 32;
  } else {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid prefix|address|* \"%s\"\n",
	    pcPrefix);
    return CLI_ERROR_COMMAND_FAILED;
  }

  bgp_router_dump_adjrib(pLogOut, pRouter, pPeer, sPrefix, 1);
  
  return CLI_SUCCESS;
}

int cli_bgp_router_show_ribout(SCliContext * pContext,
			      STokens * pTokens)
{
  SBGPRouter * pRouter;
  char * pcPeerAddr;
  char * pcEndChar;
  net_addr_t tPeerAddr;
  SPeer * pPeer;
  char * pcPrefix;
  SPrefix sPrefix;

  // Get the BGP instance from the context
  pRouter= (SBGPRouter *) cli_context_get_item_at_top(pContext);

  // Get the peer/*
  pcPeerAddr= tokens_get_string_at(pTokens, 1);
  if (!strcmp(pcPeerAddr, "*")) {
    pPeer= NULL;
  } else if (!ip_string_to_address(pcPeerAddr, &pcEndChar, &tPeerAddr)
	     && (*pcEndChar == 0)) {
    pPeer= bgp_router_find_peer(pRouter, tPeerAddr);
    if (pPeer == NULL) {
      LOG_ERR(LOG_LEVEL_SEVERE, "Error: unknown peer \"%s\"\n", pcPeerAddr);
      return CLI_ERROR_COMMAND_FAILED;
    }
  } else {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid peer address \"%s\"\n",
	    pcPeerAddr);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Get the prefix/address/*
  pcPrefix= tokens_get_string_at(pTokens, 2);
  if (!strcmp(pcPrefix, "*")) {
    sPrefix.uMaskLen= 0;
  } else if (!ip_string_to_prefix(pcPrefix, &pcEndChar, &sPrefix) &&
	     (*pcEndChar == 0)) {
  } else if (!ip_string_to_address(pcPrefix, &pcEndChar,
				   &sPrefix.tNetwork) &&
	     (*pcEndChar == 0)) {
    sPrefix.uMaskLen= 32;
  } else {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid prefix|address|* \"%s\"\n",
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
 * tokens: {addr, prefix|address|* [, output]}
 */
int cli_bgp_router_show_routeinfo(SCliContext * pContext,
				  STokens * pTokens)
{
  SBGPRouter * pRouter;
  char * pcDest;
  SNetDest sDest;
  char * pcOutput;
  SLogStream * pStream= pLogOut;

  // Get the BGP instance from the context
  pRouter= (SBGPRouter *) cli_context_get_item_at_top(pContext);

  // Get the destination
  pcDest= tokens_get_string_at(pTokens, 1);
  if (ip_string_to_dest(pcDest, &sDest)) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid destination \"%s\"\n", pcDest);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Get the optional parameter
  if (tokens_get_num(pTokens) > 2) {
    pcOutput= tokens_get_string_at(pTokens, 2);
    pStream= log_create_file(pcOutput);
    if (pStream == NULL) {
      LOG_ERR(LOG_LEVEL_SEVERE, "Error: could not create \"%d\"\n", pcOutput);
      return CLI_ERROR_COMMAND_FAILED;
    }
  }

  // Show the route information
  if (bgp_router_show_routes_info(pStream, pRouter, sDest) < 0) {
    LOG_ERR(LOG_LEVEL_SEVERE,
	    "Error: failed to show info for route(s) towards \"%s\"\n",
	    pcDest);
    if (pStream != pLogOut)
      log_destroy(&pStream);
    return CLI_ERROR_COMMAND_FAILED;
  }
  if (pStream != pLogOut)
    log_destroy(&pStream);

  return CLI_SUCCESS;
}

// ----- cli_bgp_router_show_stats ----------------------------------
#ifdef __EXPERIMENTAL__
int cli_bgp_router_show_stats(SCliContext * pContext,
			      STokens * pTokens)
{
  SBGPRouter * pRouter=
    (SBGPRouter *) cli_context_get_item_at_top(pContext);

  bgp_router_show_stats(pLogOut, pRouter);

  return CLI_SUCCESS;
}
#endif

// ----- cli_bgp_router_recordroute ---------------------------------
/**
 * context: {router}
 * tokens: {addr, prefix}
 */
int cli_bgp_router_recordroute(SCliContext * pContext,
			       STokens * pTokens)
{
  SBGPRouter * pRouter;
  char * pcPrefix;
  SPrefix sPrefix;
  char * pcEndPtr;
  int iResult;
  SBGPPath * pPath= NULL;

  // Get BGP instance
  pRouter= (SBGPRouter *) cli_context_get_item_at_top(pContext);

  // Get prefix
  pcPrefix= tokens_get_string_at(pTokens, 1);
  if (ip_string_to_prefix(pcPrefix, &pcEndPtr,
			  &sPrefix) || (*pcEndPtr != '\0')) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid prefix \"%s\"\n", pcPrefix);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Record route
  iResult= bgp_router_record_route(pRouter, sPrefix, &pPath, 0);

  // Display recorded-route
  bgp_router_dump_recorded_route(pLogOut, pRouter, sPrefix, pPath, iResult);

  path_destroy(&pPath);

  return CLI_SUCCESS;
}

// ----- cli_bgp_router_recordroute_bm ------------------------------
/**
 * context: {router}
 * tokens: {addr, prefix, bound}
 */
#ifdef __EXPERIMENTAL__
int cli_bgp_router_recordroute_bm(SCliContext * pContext,
				  STokens * pTokens)
{
  SBGPRouter * pRouter;
  char * pcPrefix;
  SPrefix sPrefix;
  char * pcEndPtr;
  unsigned int uBound;
  int iResult;
  SBGPPath * pPath= NULL;

  // Get BGP instance
  pRouter= (SBGPRouter *) cli_context_get_item_at_top(pContext);

  // Get prefix
  pcPrefix= tokens_get_string_at(pTokens, 1);
  if (ip_string_to_prefix(pcPrefix, &pcEndPtr,
			  &sPrefix) || (*pcEndPtr != '\0')) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid prefix \"%s\"\n", pcPrefix);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Get bound
  if (tokens_get_uint_at(pTokens, 2, &uBound) ||
      (uBound > 32)) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid prefix bound \"%s\"\n",
	    tokens_get_string_at(pTokens, 2));
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Call record-route
  iResult= bgp_router_record_route_bounded_match(pRouter, sPrefix,
						 (uint8_t) uBound,
						 &pPath, 0);

  // Display record-route results
  bgp_router_dump_recorded_route(pLogOut, pRouter, sPrefix, pPath, iResult);

  path_destroy(&pPath);

  return CLI_SUCCESS;
}
#endif

// ----- cli_bgp_router_rescan --------------------------------------
/**
 *
 */
int cli_bgp_router_rescan(SCliContext * pContext, STokens * pTokens)
{
  SBGPRouter * pRouter;

  // Get BGP instance from context
  pRouter= (SBGPRouter *) cli_context_get_item_at_top(pContext);

  if (bgp_router_scan_rib(pRouter)) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: RIB scan failed\n");
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// ----- cli_bgp_router_rerun ---------------------------------------
/**
 * context: {router}
 * tokens: {addr, prefix|*}
 */
int cli_bgp_router_rerun(SCliContext * pContext, STokens * pTokens)
{
  SBGPRouter * pRouter;
  char * pcPrefix;
  char * pcEndChar;
  SPrefix sPrefix;

  // Get BGP instance from context
  pRouter= (SBGPRouter *) cli_context_get_item_at_top(pContext);

  /* Get prefix */
  pcPrefix= tokens_get_string_at(pTokens, 1);
  if (!strcmp(pcPrefix, "*")) {
    sPrefix.uMaskLen= 0;
  } else if (!ip_string_to_prefix(pcPrefix, &pcEndChar, &sPrefix) &&
	     (*pcEndChar == 0)) {
  } else {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid prefix|* \"%s\"\n", pcPrefix);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Rerun the decision process for all known prefixes
  if (bgp_router_rerun(pRouter, sPrefix)) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: reset failed.\n");
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
 * tokens: {addr}
 */
int cli_bgp_router_reset(SCliContext * pContext, STokens * pTokens)
{
  SBGPRouter * pRouter;

  // Get BGP instance from context
  pRouter= (SBGPRouter *) cli_context_get_item_at_top(pContext);

  // Reset the router
  if (bgp_router_reset(pRouter)) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: reset failed.\n");
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// ----- cli_bgp_router_start ---------------------------------------
/**
 * This function opens all the defined peerings of the given router.
 *
 * context: {router}
 * tokens: {addr}
 */
int cli_bgp_router_start(SCliContext * pContext, STokens * pTokens)
{
  SBGPRouter * pRouter;

  // Get BGP instance from context
  pRouter= (SBGPRouter *) cli_context_get_item_at_top(pContext);

  // Start the router
  if (bgp_router_start(pRouter)) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: could not start the router.\n");
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// ----- cli_bgp_router_stop ----------------------------------------
/**
 * This function closes all the defined peerings of the given router.
 *
 * context: {router}
 * tokens: {addr}
 */
int cli_bgp_router_stop(SCliContext * pContext, STokens * pTokens)
{
  SBGPRouter * pRouter;

  // Get BGP instance from context
  pRouter= (SBGPRouter *) cli_context_get_item_at_top(pContext);

  // Stop the router
  if (bgp_router_stop(pRouter)) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: could not stop the router.\n");
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// ----- cli_bgp_router_add_peer ------------------------------------
/**
 * context: {router}
 * tokens: {addr, as-num, addr}
 */
int cli_bgp_router_add_peer(SCliContext * pContext, STokens * pTokens)
{
  SBGPRouter * pRouter;
  unsigned int uASNum;
  char * pcAddr;
  net_addr_t tAddr;
  char * pcEndPtr;

  // Get BGP router from context
  pRouter= (SBGPRouter *) cli_context_get_item_at_top(pContext);

  // Get the new peer's AS number
  if (tokens_get_uint_at(pTokens, 1, &uASNum) || (uASNum > MAX_AS)) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid AS number \"%s\"\n",
	    tokens_get_string_at(pTokens, 1));
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Get the new peer's address
  pcAddr= tokens_get_string_at(pTokens, 2);
  if (ip_string_to_address(pcAddr, &pcEndPtr, &tAddr) ||
      (*pcEndPtr != 0)) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid address \"%s\"\n",
	    pcAddr);
    return CLI_ERROR_COMMAND_FAILED;
  }

  if (bgp_router_add_peer(pRouter, uASNum, tAddr, 0) == NULL) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: peer already exists\n");
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// ----- cli_bgp_router_add_network ---------------------------------
/**
 * This function adds a local network to the BGP instance.
 *
 * context: {router}
 * tokens: {addr, prefix}
 */
int cli_bgp_router_add_network(SCliContext * pContext, STokens * pTokens)
{
  SBGPRouter * pRouter;
  char * pcPrefix;
  SPrefix sPrefix;
  char * pcEndPtr;

  // Get the BGP instance
  pRouter= (SBGPRouter *) cli_context_get_item_at_top(pContext);

  // Get the prefix
  pcPrefix= tokens_get_string_at(pTokens, 1);
  if (ip_string_to_prefix(pcPrefix, &pcEndPtr, &sPrefix) ||
      (*pcEndPtr != '\0')) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid prefix \"%s\"\n", pcPrefix);
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
 * tokens: {addr, prefix}
 */
int cli_bgp_router_del_network(SCliContext * pContext,
			       STokens * pTokens)
{
  SBGPRouter * pRouter;
  char * pcPrefix;
  SPrefix sPrefix;
  char * pcEndPtr;

  // Get BGP instance from context
  pRouter= (SBGPRouter *) cli_context_get_item_at_top(pContext);

  // Get the prefix
  pcPrefix= tokens_get_string_at(pTokens, 1);
  if (ip_string_to_prefix(pcPrefix, &pcEndPtr, &sPrefix) ||
      (*pcEndPtr != '\0')) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid prefix \"%s\"\n", pcPrefix);
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
 * tokens: {addr, prefix, delay}
 */
#ifdef BGP_QOS
int cli_bgp_router_add_qosnetwork(SCliContext * pContext, STokens * pTokens)
{
  SBGPRouter * pRouter;
  SPrefix sPrefix;
  char * pcEndPtr;
  unsigned int uDelay;

  /*LOG_DEBUG("bgp router %s add network %s\n",
	    tokens_get_string_at(pTokens, 0),
	    tokens_get_string_at(pTokens, 1));*/
  pRouter= (SBGPRouter *) cli_context_get_item_at_top(pContext);
  if (ip_string_to_prefix(tokens_get_string_at(pTokens, 1),
			  &pcEndPtr, &sPrefix) || (*pcEndPtr != '\0'))
    return CLI_ERROR_COMMAND_FAILED;
  if (tokens_get_uint_at(pTokens, 2, &uDelay))
    return CLI_ERROR_COMMAND_FAILED;
  if (bgp_router_add_qos_network(pRouter, sPrefix, (net_link_delay_t) uDelay))
    return CLI_ERROR_COMMAND_FAILED;
  return CLI_SUCCESS;
}
#endif

// ----- cli_bgp_router_assert_routes_match --------------------------
/**
 * context: {router, routes}
 * tokens: {addr, prefix, type, predicate}
 */
int cli_bgp_router_assert_routes_match(SCliContext * pContext,
				       STokens * pTokens)
{
  char * pcPredicate;
  char * pcTemp;
  SFilterMatcher * pMatcher;
  SRoutes * pRoutes= (SRoutes *) cli_context_get_item_at_top(pContext);
  int iMatches= 0;
  int iIndex;
  SRoute * pRoute;

  // Parse predicate
  pcPredicate= tokens_get_string_at(pTokens,
				    tokens_get_num(pTokens)-1);
  pcTemp= pcPredicate;
  if (predicate_parse(&pcTemp, &pMatcher)) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid predicate \"%s\"\n", pcPredicate);
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
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: no route matched \"%s\"\n", pcPredicate);
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// ----- cli_bgp_router_assert_routes_show --------------------------
/**
 * context: {router, routes}
 * tokens: {addr, prefix, type}
 */
int cli_bgp_router_assert_routes_show(SCliContext * pContext,
				      STokens * pTokens)
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
 * tokens: {addr, prefix, type}
 */
int cli_ctx_create_bgp_router_assert_routes(SCliContext * pContext,
					    void ** ppItem)
{
  SBGPRouter * pRouter;
  char * pcPrefix;
  char * pcType;
  SPrefix sPrefix;
  char * pcEndPtr;
  SRoutes * pRoutes;

  // Get BGP instance from context
  pRouter= (SBGPRouter *) cli_context_get_item_at_top(pContext);

  // Get the prefix
  pcPrefix= tokens_get_string_at(pContext->pTokens, 1);
  if (ip_string_to_prefix(pcPrefix, &pcEndPtr, &sPrefix) ||
      (*pcEndPtr != '\0')) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid prefix \"%s\"\n", pcPrefix);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Get the routes type
  pcType= tokens_get_string_at(pContext->pTokens, 2);

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
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: unsupported type of routes \"%s\"\n", pcType);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Check that there is at leat one route
  if (routes_list_get_num(pRoutes) < 1) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: no feasible routes towards %s\n", pcPrefix);
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
 * tokens: {addr, addr}
 */
int cli_ctx_create_bgp_router_peer(SCliContext * pContext,
				   void ** ppItem)
{
  SBGPRouter * pRouter;
  SPeer * pPeer;
  char * pcPeerAddr;
  char * pcEndPtr;
  net_addr_t tAddr;

  // Get the BGP instance from context
  pRouter= (SBGPRouter *) cli_context_get_item_at_top(pContext);

  // Get the peer address
  pcPeerAddr= tokens_get_string_at(pContext->pTokens, 1);
  if (ip_string_to_address(pcPeerAddr, &pcEndPtr, &tAddr) ||
      (*pcEndPtr != 0)) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid peer address \"%s\"\n",
	       pcPeerAddr);
    return CLI_ERROR_CTX_CREATE;
  }

  // Get the peer
  pPeer= bgp_router_find_peer(pRouter, tAddr);
  if (pPeer == NULL) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: unknown peer\n");
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
 * tokens: {addr, addr, in|out}
 */
int cli_ctx_create_bgp_router_peer_filter(SCliContext * pContext,
					  void ** ppItem)
{
  SPeer * pPeer;
  char * pcFilter;

  // Get Peer instance
  pPeer= (SPeer *) cli_context_get_item_at_top(pContext);

  // Create filter context
  pcFilter= tokens_get_string_at(pContext->pTokens, 2);
  if (!strcmp("in", pcFilter))
    *ppItem= &(pPeer->pInFilter);
  else if (!strcmp("out", pcFilter))
    *ppItem= &(pPeer->pOutFilter);
  else {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid filter type \"%s\"\n", pcFilter);
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
			      STokens * pTokens)
{
  SFilterRule * pRule;
  char * pcPredicate;
  SFilterMatcher * pMatcher;

  // Get rule from context
  pRule= (SFilterRule *) cli_context_get_item_at_top(pContext);

  // Parse predicate
  pcPredicate= tokens_get_string_at(pTokens,
				    tokens_get_num(pTokens)-1);
  if (predicate_parse(&pcPredicate, &pMatcher)) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid predicate \"%s\"\n", pcPredicate);
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
			       STokens * pTokens)
{
  SFilterRule * pRule;
  char * pcAction;
  SFilterAction * pAction;

  // Get rule from context
  pRule= (SFilterRule *) cli_context_get_item_at_top(pContext);

  // Parse predicate
  pcAction= tokens_get_string_at(pTokens,
				 tokens_get_num(pTokens)-1);
  if (filter_parser_action(pcAction, &pAction)) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid action \"%s\"\n", pcAction);
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
  if (tokens_get_uint_at(pContext->pTokens,
			 tokens_get_num(pContext->pTokens)-1,
			 &uIndex)) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid index\n");
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
			       STokens * pTokens)
{
  SFilter ** ppFilter=
    (SFilter **) cli_context_get_item_at_top(pContext);
  unsigned int uIndex;

  if (*ppFilter == NULL) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: filter contains no rule\n");
    return CLI_ERROR_COMMAND_FAILED;
  }
  
  // Get index of rule to be removed
  if (tokens_get_uint_at(pTokens,
			 tokens_get_num(pTokens)-1,
			 &uIndex)) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid index\n");
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Remove rule
  if (filter_remove_rule(*ppFilter, uIndex)) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: could not remove rule %u\n", uIndex);
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
  LOG_ERR(LOG_LEVEL_SEVERE, "Error: not yet implemented, sorry.\n");
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
			STokens * pTokens)
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
				  STokens * pTokens)
{
  char * pcRouteMapName;
  SFilterRule * pRule;
  SFilter * pFilter= NULL;

  pcRouteMapName = (char *) cli_context_get_item_at_top(pContext);
  
  if (filter_parser_rule(tokens_get_string_at(pTokens, 0), &pRule) != 
      FILTER_PARSER_SUCCESS)
    return CLI_ERROR_COMMAND_FAILED;
  if ( (pFilter = route_map_get(pcRouteMapName)) == NULL) {
    pFilter = filter_create();
    if (route_map_add(pcRouteMapName, pFilter) < 0) {
      LOG_ERR(LOG_LEVEL_SEVERE, "Error: could not add the route-map filter.\n");
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
				     STokens * pTokens)
{
  SPeer * pPeer;
  SFilter * pFilter= NULL;

  pPeer= (SPeer *) cli_context_get_item_at_top(pContext);
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
 * tokens: {addr, addr, filter}
 */
int cli_bgp_router_peer_infilter_add(SCliContext * pContext,
				     STokens * pTokens)
{
  SPeer * pPeer;
  SFilterRule * pRule;
  SFilter * pFilter;

  /*LOG_DEBUG("bgp router %s peer %s in-filter \"%s\"\n",
	    tokens_get_string_at(pTokens, 0),
	    tokens_get_string_at(pTokens, 1),
	    tokens_get_string_at(pTokens, 2));*/
  pPeer= (SPeer *) cli_context_get_item_at_top(pContext);
  if (filter_parser_rule(tokens_get_string_at(pTokens, 2), &pRule) !=
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
 * tokens: {addr, addr, filter}
 */
int cli_bgp_router_peer_outfilter_set(SCliContext * pContext,
				      STokens * pTokens)
{
  SPeer * pPeer;
  SFilter * pFilter= NULL;

  pPeer= (SPeer *) cli_context_get_item_at_top(pContext);
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
 * tokens: {addr, addr, filter}
 */
int cli_bgp_router_peer_outfilter_add(SCliContext * pContext,
				      STokens * pTokens)
{
  SPeer * pPeer;
  SFilterRule * pRule;
  SFilter * pFilter;

  pPeer= (SPeer *) cli_context_get_item_at_top(pContext);
  if (filter_parser_rule(tokens_get_string_at(pTokens, 2), &pRule) !=
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
 * tokens: {addr, addr, in|out}
 */
int cli_bgp_router_peer_show_filter(SCliContext * pContext,
				    STokens * pTokens)
{
  SPeer * pPeer;
  char * pcFilter;

  // Get BGP peer
  pPeer= (SPeer *) cli_context_get_item_at_top(pContext);

  // Create filter context
  pcFilter= tokens_get_string_at(pContext->pTokens, 2);
  if (!strcmp(pcFilter, "in"))
    bgp_peer_dump_in_filters(pLogOut, pPeer);
  else if (!strcmp(pcFilter, "out"))
    bgp_peer_dump_out_filters(pLogOut, pPeer);
  else {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid filter type \"%s\"\n", pcFilter);
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// ----- cli_bgp_router_peer_rrclient -------------------------------
/**
 * context: {router, peer}
 * tokens: {addr, addr}
 */
int cli_bgp_router_peer_rrclient(SCliContext * pContext, STokens * pTokens)
{
  SBGPRouter * pRouter;
  SPeer * pPeer;

  pRouter= (SBGPRouter *) cli_context_get_item_at(pContext, 0);
  pRouter->iRouteReflector= 1;
  pPeer= (SPeer *) cli_context_get_item_at_top(pContext);
  bgp_peer_flag_set(pPeer, PEER_FLAG_RR_CLIENT, 1);
  return CLI_SUCCESS;
}

// ----- cli_bgp_router_peer_nexthop --------------------------------
/**
 * context: {router, peer}
 * tokens: {addr, addr, addr}
 */
int cli_bgp_router_peer_nexthop(SCliContext * pContext,
				STokens * pTokens)
{
  SBGPPeer * pPeer;
  char * pcNextHop;
  net_addr_t tNextHop;
  char * pcEndChar;

  // Get peer from context
  pPeer= (SPeer *) cli_context_get_item_at_top(pContext);

  // Get next-hop address
  pcNextHop= tokens_get_string_at(pTokens, 2);
  if (ip_string_to_address(pcNextHop, &pcEndChar, &tNextHop) ||
      (*pcEndChar != 0)) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid next-hop \"%s\".\n", pcNextHop);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Set the next-hop to be used for routes advertised to this peer
  if (bgp_peer_set_nexthop(pPeer, tNextHop)) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: could not change next-hop (");
    LOG_ERR(LOG_LEVEL_SEVERE, ").\n");
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// ----- cli_bgp_router_peer_nexthopself ----------------------------
/**
 * context: {router, peer}
 * tokens: {addr, addr}
 */
int cli_bgp_router_peer_nexthopself(SCliContext * pContext,
				    STokens * pTokens)
{
  SBGPPeer * pPeer;

  // Get peer from context
  pPeer= (SPeer *) cli_context_get_item_at_top(pContext);

  bgp_peer_flag_set(pPeer, PEER_FLAG_NEXT_HOP_SELF, 1);
  return CLI_SUCCESS;
}

// ----- cli_bgp_router_peer_recv -----------------------------------
/**
 * context: {router, peer}
 * tokens: {addr, addr, mrt-record}
 */
int cli_bgp_router_peer_recv(SCliContext * pContext,
			     STokens * pTokens)
{
  SPeer * pPeer;
  SBGPRouter * pRouter;
  char * pcRecord;
  SBGPMsg * pMsg;

  /* Get the peer instance from context */
  pPeer= (SPeer *) cli_context_get_item_at_top(pContext);

  /* Check that the peer is virtual */
  if (!bgp_peer_flag_get(pPeer, PEER_FLAG_VIRTUAL)) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: only virtual peers can do that\n");
    return CLI_ERROR_COMMAND_FAILED;
  }

  /* Get the router instance from context */
  pRouter= (SBGPRouter *) cli_context_get_item_at(pContext, 0);

  /* Get the MRT-record */
  pcRecord= tokens_get_string_at(pTokens, 2);

  /* Build a message from the MRT-record */
  pMsg= mrtd_msg_from_line(pRouter, pPeer, pcRecord);

  if (pMsg == NULL) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: could not build message from input\n");
    return CLI_ERROR_COMMAND_FAILED;
  }

  bgp_peer_handle_message(pPeer, pMsg);

  return CLI_SUCCESS;
}

#ifdef __EXPERIMENTAL__
// ----- cli_bgp_router_load_ribs_in --------------------------------
/**
 *
 *
 */
int cli_bgp_router_load_ribs_in(SCliContext * pContext,
				STokens * pTokens)
{
  SBGPRouter * pRouter;
  char * pcFileName;


  pRouter = (SBGPRouter *) cli_context_get_item_at(pContext, 0);
  pcFileName = tokens_get_string_at(pTokens, 1);
  
  if (bgp_router_load_ribs_in(pcFileName, pRouter) == -1)
    return CLI_ERROR_COMMAND_FAILED;
  return CLI_SUCCESS;
}
#endif

#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
// ----- cli_bgp_router_peer_walton_limit ----------------------------
/**
 * context: {router, peer}
 * tokens: {addr, addr, announce-limit}
 */
int cli_bgp_router_peer_walton_limit(SCliContext * pContext, 
						  STokens * pTokens)
{
  SPeer * pPeer;
  unsigned int uWaltonLimit;

  pPeer= (SPeer *) cli_context_get_item_at_top(pContext);

  if (tokens_get_uint_at(pTokens, 2, &uWaltonLimit)) {
    LOG_ERR(LOG_LEVEL_SEVERE,"Error: invalid walton limitation\n");
    return CLI_ERROR_COMMAND_FAILED;
  }
  peer_set_walton_limit(pPeer, uWaltonLimit);

  return CLI_SUCCESS;

  
}
#endif

// ----- cli_bgp_router_peer_up -------------------------------------
/**
 * context: {router, peer}
 * tokens: {addr, addr}
 */
int cli_bgp_router_peer_up(SCliContext * pContext, STokens * pTokens)
{
  SPeer * pPeer;

  // Get peer instance from context
  pPeer= (SPeer *) cli_context_get_item_at_top(pContext);

  // Try to open session
  if (bgp_peer_open_session(pPeer)) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: could not open session\n");
    return CLI_ERROR_COMMAND_FAILED;
  }
    
  return CLI_SUCCESS;
}

// ----- cli_bgp_router_peer_softrestart ----------------------------
/**
 * context: {router, peer}
 * tokens: {addr, addr}
 */
int cli_bgp_router_peer_softrestart(SCliContext * pContext, STokens * pTokens)
{
  SPeer * pPeer;

  // Get peer instance from context
  pPeer= (SPeer *) cli_context_get_item_at_top(pContext);

  // Set the virtual flag of this peer
  if (!bgp_peer_flag_get(pPeer, PEER_FLAG_VIRTUAL)) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: soft-restart option only available to virtual peers\n");
    return CLI_ERROR_COMMAND_FAILED;
  }
  bgp_peer_flag_set(pPeer, PEER_FLAG_SOFT_RESTART, 1);
    
  return CLI_SUCCESS;
}

// ----- cli_bgp_router_peer_virtual --------------------------------
/**
 * context: {router, peer}
 * tokens: {addr, addr}
 */
int cli_bgp_router_peer_virtual(SCliContext * pContext, STokens * pTokens)
{
  SPeer * pPeer;

  // Get peer instance from context
  pPeer= (SPeer *) cli_context_get_item_at_top(pContext);

  // Set the virtual flag of this peer
  bgp_peer_flag_set(pPeer, PEER_FLAG_VIRTUAL, 1);
    
  return CLI_SUCCESS;
}

// ----- cli_bgp_router_peer_down -----------------------------------
/**
 * context: {router, peer}
 * tokens: {addr, addr}
 */
int cli_bgp_router_peer_down(SCliContext * pContext, STokens * pTokens)
{
  SPeer * pPeer;

  // Get peer from context
  pPeer= (SPeer *) cli_context_get_item_at_top(pContext);

  // Try to close session
  if (bgp_peer_close_session(pPeer)) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: could not close session\n");
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// -----[ cli_bgp_router_peer_readv ]--------------------------------
/**
 * context: {router, peer}
 * tokens: {addr, addr, prefix|*}
 */
int cli_bgp_router_peer_readv(SCliContext * pContext, STokens * pTokens)
{
  SBGPRouter * pRouter;
  SPeer * pPeer;
  char * pcPrefix;
  char * pcEndChar;
  SPrefix sPrefix;

  /* Get router and peer from context */
  pRouter= (SBGPRouter *) cli_context_get_item_at(pContext, 0);
  pPeer= (SPeer *) cli_context_get_item_at(pContext, 1);

  /* Get prefix */
  pcPrefix= tokens_get_string_at(pTokens, 2);
  if (!strcmp(pcPrefix, "*")) {
    sPrefix.uMaskLen= 0;
  } else if (!ip_string_to_prefix(pcPrefix, &pcEndChar, &sPrefix) &&
	     (*pcEndChar == 0)) {
  } else {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid prefix|* \"%s\"\n", pcPrefix);
    return CLI_ERROR_COMMAND_FAILED;
  }

  /* Re-advertise */
  if (bgp_router_peer_readv_prefix(pRouter, pPeer, sPrefix)) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: could not re-advertise\n");
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// ----- cli_bgp_router_peer_reset ----------------------------------
/**
 * context: {router, peer}
 * tokens: {addr, addr}
 */
int cli_bgp_router_peer_reset(SCliContext * pContext, STokens * pTokens)
{
  SPeer * pPeer;

  // Get peer from context
  pPeer= (SPeer *) cli_context_get_item_at_top(pContext);

  // Try to close session
  if (bgp_peer_close_session(pPeer)) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: could not close session\n");
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Try to open session
  if (bgp_peer_open_session(pPeer)) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: could not close session\n");
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
int cli_bgp_show_routers(SCliContext * pContext, STokens * pTokens)
{
  return CLI_SUCCESS;
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

  cli_cmds_add(pSubCmds, cli_cmd_create("full-mesh",
					cli_bgp_domain_full_mesh,
					NULL, NULL));
  cli_cmds_add(pSubCmds, cli_cmd_create("rescan",
					cli_bgp_domain_rescan,
					NULL, NULL));

pParams= cli_params_create();
  cli_params_add(pParams, "<address|prefix>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("record-route",
					cli_bgp_domain_recordroute,
					NULL, pParams));

//sta : Cli command to check deflection in an entire domain for a prefix or an address.
  pParams= cli_params_create();
  cli_params_add(pParams, "<address|prefix>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("record-route-deflection",
					cli_bgp_domain_recordroutedeflection,
					NULL, pParams));

  pParams= cli_params_create();
  cli_params_add(pParams, "<as-number>", NULL);
  return cli_cmds_add(pCmds, cli_cmd_create_ctx("domain",
						cli_ctx_create_bgp_domain,
						cli_ctx_destroy_bgp_domain,
						pSubCmds, pParams));
}

// ----- cli_register_bgp_topology ----------------------------------
int cli_register_bgp_topology(SCliCmds * pCmds)
{
  SCliCmds * pSubCmds;
  SCliParams * pParams;

  pSubCmds= cli_cmds_create();
  pParams= cli_params_create();
#ifdef _FILENAME_COMPLETION_FUNCTION
  cli_params_add2(pParams, "<file>", NULL,
		  _FILENAME_COMPLETION_FUNCTION);
#else
  cli_params_add(pParams, "<file>", NULL);
#endif
  cli_cmds_add(pSubCmds, cli_cmd_create("load",
					cli_bgp_topology_load,
					NULL, pParams));
  cli_cmds_add(pSubCmds, cli_cmd_create("policies",
					cli_bgp_topology_policies,
					NULL, NULL));
  pParams= cli_params_create();
  cli_params_add(pParams, "<prefix>", NULL);
  cli_params_add(pParams, "<input>", NULL);
  cli_params_add(pParams, "<output>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("record-route",
					cli_bgp_topology_recordroute,
					NULL, pParams));
#ifdef __EXPERIMENTAL__
  pParams= cli_params_create();
  cli_params_add(pParams, "<prefix>", NULL);
  cli_params_add(pParams, "<bound>", NULL);
  cli_params_add(pParams, "<input>", NULL);
  cli_params_add(pParams, "<output>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("record-route-bm",
					cli_bgp_topology_recordroute_bm,
					NULL, pParams));
#endif
  cli_cmds_add(pSubCmds, cli_cmd_create("show-rib",
					cli_bgp_topology_showrib,
					NULL, NULL));
  /*
  pParams= cli_params_create();
  cli_params_add(pParams, "<prefix>", NULL);
  cli_params_add(pParams, "<output>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("route-dp-rule",
					cli_bgp_topology_route_dp_rule,
					NULL, pParams));
  */
  cli_cmds_add(pSubCmds, cli_cmd_create("run",
					cli_bgp_topology_run,
					NULL, NULL));
  return cli_cmds_add(pCmds, cli_cmd_create("topology", NULL, pSubCmds, NULL));
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
  cli_params_add2(pParams, "<addr>", NULL, cli_bgp_enum_nodes);
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
#ifdef __EXPERIMENTAL__
  pParams= cli_params_create();
  cli_params_add(pParams, "<nlri>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("nlri",
					cli_bgp_options_nlri,
					NULL, pParams));
#endif
  pParams= cli_params_create();
  cli_params_add(pParams, "<cisco|mrt>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("show-mode",
					cli_bgp_options_showmode,
					NULL, pParams));
#ifdef BGP_QOS
  pParams= cli_params_create();
  cli_params_add(pParams, "<limit>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("qos-aggr-limit",
					cli_bgp_options_qosaggrlimit,
					NULL, pParams));
#endif

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

  pSubCmds= cli_cmds_create();
  pParams= cli_params_create();
#ifdef _FILENAME_COMPLETION_FUNCTION
  cli_params_add2(pParams, "<file>", NULL,
		  _FILENAME_COMPLETION_FUNCTION);
#else
  cli_params_add(pParams, "<file>", NULL);
#endif
  cli_cmds_add(pSubCmds, cli_cmd_create("rib",
					cli_bgp_router_load_rib,
					NULL, pParams));
#ifdef __EXPERIMENTAL__
  pParams = cli_params_create();
  cli_params_add(pParams, "<file>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("rib-in", 
					cli_bgp_router_load_ribs_in,
					NULL, pParams));
#endif
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
#ifdef _FILENAME_COMPLETION_FUNCTION
  cli_params_add2(pParams, "<file>", NULL,
		  _FILENAME_COMPLETION_FUNCTION);
#else
  cli_params_add(pParams, "<file>", NULL);
#endif
  cli_cmds_add(pSubCmds, cli_cmd_create("rib",
					cli_bgp_router_save_rib,
					NULL, pParams));
  pParams= cli_params_create();
  cli_params_add(pParams, "<peer>", NULL);
#ifdef _FILENAME_COMPLETION_FUNCTION
  cli_params_add2(pParams, "<file>", NULL,
		  _FILENAME_COMPLETION_FUNCTION);
#else
  cli_params_add(pParams, "<file>", NULL);
#endif
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
#ifdef __EXPERIMENTAL__
  cli_cmds_add(pSubCmds, cli_cmd_create("stats",
					cli_bgp_router_show_stats,
					NULL, NULL));
#endif
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
#ifdef __EXPERIMENTAL__
  pParams= cli_params_create();
  cli_params_add(pParams, "<prefix>", NULL);
  cli_params_add(pParams, "<bound>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("record-route-bm",
					cli_bgp_router_recordroute_bm,
					NULL, pParams));
#endif
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
  cli_params_add2(pParams, "<addr>", NULL, cli_bgp_enum_nodes);
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
  cli_register_cmd(pCli, cli_cmd_create("bgp", NULL, pCmds, NULL));
  return 0;
}
