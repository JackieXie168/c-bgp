// ==================================================================
// @(#)bgp.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 15/07/2003
// @lastdate 24/05/2004
// ==================================================================

#include <bgp/as.h>
#include <bgp/bgp_assert.h>
#include <bgp/bgp_debug.h>
#include <bgp/filter.h>
#include <bgp/filter_parser.h>
#include <bgp/predicate_parser.h>
#include <bgp/message.h>
#include <bgp/mrtd.h>
#include <bgp/peer.h>
#include <bgp/qos.h>
#include <bgp/rexford.h>
#include <bgp/tie_breaks.h>
#include <cli/bgp.h>
#include <cli/common.h>
#include <cli/net.h>
#include <libgds/cli_ctx.h>
#include <libgds/log.h>
#include <net/prefix.h>
#include <string.h>

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
  SAS * pAS;
  unsigned int uASNum;

  // Find node
  pcNodeAddr= tokens_get_string_at(pTokens, 1);
  pNode= cli_net_node_by_addr(pcNodeAddr);
  if (pNode == NULL) {
    LOG_SEVERE("Error: invalid node address \"%s\"\n",
	       pcNodeAddr);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Check AS-Number
  if (tokens_get_uint_at(pTokens, 0, &uASNum) || (uASNum > MAX_AS)) {
    LOG_SEVERE("Error: invalid AS_Number\n");
    return CLI_ERROR_COMMAND_FAILED;
  }

  pAS= as_create(uASNum, pNode, 0);

  // Register BGP protocol into the node
  if (node_register_protocol(pNode, NET_PROTOCOL_BGP, pAS,
			     (FNetNodeHandlerDestroy) as_destroy,
			     as_handle_message)) {
    LOG_SEVERE("Error: could not register BGP protocol on node \"%s\"\n",
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
    LOG_SEVERE("Error: peerings assertion failed.\n");
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
    LOG_SEVERE("Error: reachability assertion failed.\n");
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// ----- cli_bgp_topology_load --------------------------------------
/**
 * context: {}
 * tokens: {file}
 */
int cli_bgp_topology_load(SCliContext * pContext, STokens * pTokens)
{
  if (rexford_load(tokens_get_string_at(pTokens, 0), network_get()))
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
  FILE * fOutput;
  int iResult= CLI_SUCCESS;

  if (ip_string_to_prefix(tokens_get_string_at(pTokens, 0), &pcEndPtr,
			  &sPrefix) || (*pcEndPtr != '\0'))
    return CLI_ERROR_COMMAND_FAILED;
  if ((fOutput= fopen(tokens_get_string_at(pTokens, 2), "w")) == NULL)
    return CLI_ERROR_COMMAND_FAILED;
  if (rexford_record_route(fOutput, tokens_get_string_at(pTokens, 1), sPrefix))
    iResult= CLI_ERROR_COMMAND_FAILED;
  fclose(fOutput);
  return iResult;
}

// ----- cli_bgp_topology_recordroute_bm ----------------------------
/**
 * context: {}
 * tokens: {prefix, bound, file-in, file-out}
 */
int cli_bgp_topology_recordroute_bm(SCliContext * pContext,
				    STokens * pTokens)
{
  char * pcPrefix;
  SPrefix sPrefix;
  char * pcEndPtr;
  FILE * fOutput;
  int iResult= CLI_SUCCESS;
  unsigned int uBound;

  // Get prefix
  pcPrefix= tokens_get_string_at(pTokens, 0);
  if (ip_string_to_prefix(pcPrefix, &pcEndPtr, &sPrefix) ||
      (*pcEndPtr != '\0')) {
    LOG_SEVERE("Error: invalid prefix \"%s\"\n", pcPrefix);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Get prefix bound
  if (tokens_get_uint_at(pTokens, 1, &uBound) || (uBound > 32)) {
    LOG_SEVERE("Error: invalid prefix bound \"%s\"\n",
	       tokens_get_string_at(pTokens, 1));
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Create output
  if ((fOutput= fopen(tokens_get_string_at(pTokens, 3), "w")) == NULL) {
    LOG_SEVERE("Error: unable to create \"%s\"\n",
	       tokens_get_string_at(pTokens, 3));
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Record route
  if (rexford_record_route_bm(fOutput, tokens_get_string_at(pTokens, 2),
			      sPrefix, (uint8_t) uBound))
    iResult= CLI_ERROR_COMMAND_FAILED;

  fclose(fOutput);

  return iResult;
}

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
    LOG_SEVERE("Error: invalid node address \"%s\"\n",
	       pcNodeAddr);
    return CLI_ERROR_CTX_CREATE;
  }

  // Get BGP protocol instance
  pProtocol= protocols_get(pNode->pProtocols, NET_PROTOCOL_BGP);
  if (pProtocol == NULL) {
    LOG_SEVERE("Error: BGP is not supported on node \"%s\"\n",
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

// ----- cli_bgp_options_tiebreak ------------------------------------
/**
 * context: {}
 * tokens: {function}
 */
int cli_bgp_options_tiebreak(SCliContext * pContext, STokens * pTokens)
{
  char * pcParam;

  pcParam= tokens_get_string_at(pTokens, 0);
  if (!strcmp(pcParam, "hash"))
    BGP_OPTIONS_TIE_BREAK= tie_break_hash;
  else if (!strcmp(pcParam, "low-isp"))
    BGP_OPTIONS_TIE_BREAK= tie_break_low_ISP;
  else if (!strcmp(pcParam, "high-isp"))
    BGP_OPTIONS_TIE_BREAK= tie_break_high_ISP;
  else
    return CLI_ERROR_COMMAND_FAILED;
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

  if (tokens_get_ulong_at(pTokens, 0, &ulLocalPref))
    return CLI_ERROR_COMMAND_FAILED;
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
  SAS * pAS;
  unsigned int uClusterID;

  // Get the BGP instance from the context
  pAS= (SAS *) cli_context_get_item_at_top(pContext);

  // Get the cluster-id
  if (tokens_get_uint_at(pTokens, 1, &uClusterID)) {
    LOG_SEVERE("Error: invalid cluster-id\n");
    return CLI_ERROR_COMMAND_FAILED;
  }

  pAS->tClusterID= uClusterID;
  pAS->iRouteReflector= 1;
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
    LOG_SEVERE("Error: invalid prefix \"%s\"\n", pcPrefix);
    return CLI_ERROR_COMMAND_FAILED;
  }

  bgp_debug_dp(stdout, pRouter, sPrefix);

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
  SAS * pAS;

  // Get the BGP instance from the context
  pAS= (SAS *) cli_context_get_item_at_top(pContext);

  // Get the MRTD file name
  pcFileName= tokens_get_string_at(pTokens, 1);

  // Load the MRTD file 
  if (as_load_rib(pcFileName, pAS)) {
    LOG_SEVERE("Error: could not load \"%s\"\n", pcFileName);
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
int cli_bgp_router_set_tiebreak(SCliContext * pContext,
				STokens * pTokens)
{
  char * pcParam;
  SAS * pAS;

  // Get the BGP instance from the context
  pAS= (SAS *) cli_context_get_item_at_top(pContext);

  // Get the tie-breaking function name
  pcParam= tokens_get_string_at(pTokens, 1);
  if (!strcmp(pcParam, "hash"))
    pAS->fTieBreak= tie_break_hash;
  else if (!strcmp(pcParam, "low-isp"))
    pAS->fTieBreak= tie_break_low_ISP;
  else if (!strcmp(pcParam, "high-isp"))
    pAS->fTieBreak= tie_break_high_ISP;
  else {
    LOG_SEVERE("Error: invalid tie-breaking function \"%s\"\n", pcParam);
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

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
  SAS * pRouter;
  char * pcFileName;

  // Get the BGP instance from the context
  pRouter= (SAS *) cli_context_get_item_at_top(pContext);

  // Get the file name
  pcFileName= tokens_get_string_at(pTokens, 1);

  // Save the Loc-RIB
  if (bgp_router_save_rib(pcFileName, pRouter)) {
    LOG_SEVERE("Error: unable to save into \"%s\"\n", pcFileName);
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
  LOG_SEVERE("Error: sorry, not implemented\n");
  return CLI_ERROR_COMMAND_FAILED;
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
  SAS * pRouter;

  // Get the BGP instance from the context
  pRouter= (SAS *) cli_context_get_item_at_top(pContext);

  bgp_router_dump_networks(stdout, pRouter);

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
  SAS * pRouter;

  // Get the BGP instance from the context
  pRouter= (SAS *) cli_context_get_item_at_top(pContext);

  // Dump peers
  bgp_router_dump_peers(stdout, pRouter);

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
  SAS * pRouter;
  char * pcPrefix;
  char * pcEndChar;
  SPrefix sPrefix;

  // Get the BGP instance from the context
  pRouter= (SAS *) cli_context_get_item_at_top(pContext);

  // Get the prefix/address/*
  pcPrefix= tokens_get_string_at(pTokens, 1);
  if (!strcmp(pcPrefix, "*")) {
    bgp_router_dump_rib(stdout, pRouter);
  } else if (!ip_string_to_prefix(pcPrefix, &pcEndChar, &sPrefix) &&
	     (*pcEndChar == 0)) {
    bgp_router_dump_rib_prefix(stdout, pRouter, sPrefix);
  } else if (!ip_string_to_address(pcPrefix, &pcEndChar,
				   &sPrefix.tNetwork) &&
	     (*pcEndChar == 0)) {
    bgp_router_dump_rib_address(stdout, pRouter, sPrefix.tNetwork);
  } else {
    LOG_SEVERE("Error: invalid prefix|address|* \"%s\"\n", pcPrefix);
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
  SAS * pRouter;
  char * pcPeerAddr;
  char * pcEndChar;
  net_addr_t tPeerAddr;
  SPeer * pPeer;
  char * pcPrefix;
  SPrefix sPrefix;

  // Get the BGP instance from the context
  pRouter= (SAS *) cli_context_get_item_at_top(pContext);

  // Get the peer/*
  pcPeerAddr= tokens_get_string_at(pTokens, 1);
  if (!strcmp(pcPeerAddr, "*")) {
    pPeer= NULL;
  } else if (!ip_string_to_address(pcPeerAddr, &pcEndChar, &tPeerAddr)
	     && (*pcEndChar == 0)) {
    pPeer= as_find_peer(pRouter, tPeerAddr);
    if (pPeer == NULL) {
      LOG_SEVERE("Error: unknown peer \"%s\"\n", pcPeerAddr);
      return CLI_ERROR_COMMAND_FAILED;
    }
  } else {
    LOG_SEVERE("Error: invalid peer address \"%s\"\n", pcPeerAddr);
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
    LOG_SEVERE("Error: invalid prefix|address|* \"%s\"\n", pcPrefix);
    return CLI_ERROR_COMMAND_FAILED;
  }

  bgp_router_dump_ribin(stdout, pRouter, pPeer, sPrefix);
  
  return CLI_SUCCESS;
}

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
  SPath * pPath= NULL;

  // Get BGP instance
  pRouter= (SBGPRouter *) cli_context_get_item_at_top(pContext);

  // Get prefix
  pcPrefix= tokens_get_string_at(pTokens, 1);
  if (ip_string_to_prefix(pcPrefix, &pcEndPtr,
			  &sPrefix) || (*pcEndPtr != '\0')) {
    LOG_SEVERE("Error: invalid prefix \"%s\"\n", pcPrefix);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Record route
  iResult= bgp_router_record_route(pRouter, sPrefix, &pPath, 0);

  // Display recorded-route
  bgp_router_dump_recorded_route(stdout, pRouter, sPrefix, pPath, iResult);

  path_destroy(&pPath);

  return CLI_SUCCESS;
}

// ----- cli_bgp_router_recordroute_bm ------------------------------
/**
 * context: {router}
 * tokens: {addr, prefix, bound}
 */
int cli_bgp_router_recordroute_bm(SCliContext * pContext,
				  STokens * pTokens)
{
  SBGPRouter * pRouter;
  char * pcPrefix;
  SPrefix sPrefix;
  char * pcEndPtr;
  unsigned int uBound;
  int iResult;
  SPath * pPath= NULL;

  // Get BGP instance
  pRouter= (SBGPRouter *) cli_context_get_item_at_top(pContext);

  // Get prefix
  pcPrefix= tokens_get_string_at(pTokens, 1);
  if (ip_string_to_prefix(pcPrefix, &pcEndPtr,
			  &sPrefix) || (*pcEndPtr != '\0')) {
    LOG_SEVERE("Error: invalid prefix \"%s\"\n", pcPrefix);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Get bound
  if (tokens_get_uint_at(pTokens, 2, &uBound) ||
      (uBound > 32)) {
    LOG_SEVERE("Error: invalid prefix bound \"%s\"\n",
	       tokens_get_string_at(pTokens, 2));
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Call record-route
  iResult= bgp_router_record_route_bounded_match(pRouter, sPrefix,
						 (uint8_t) uBound,
						 &pPath, 0);

  // Display record-route results
  bgp_router_dump_recorded_route(stdout, pRouter, sPrefix, pPath, iResult);

  path_destroy(&pPath);

  return CLI_SUCCESS;
}

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
    LOG_SEVERE("Error: RIB scan failed\n");
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
    LOG_SEVERE("Error: reset failed.\n");
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
    LOG_SEVERE("Error: invalid AS number \"%s\"\n",
	       tokens_get_string_at(pTokens, 1));
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Get the new peer's address
  pcAddr= tokens_get_string_at(pTokens, 2);
  if (ip_string_to_address(pcAddr, &pcEndPtr, &tAddr) ||
      (*pcEndPtr != 0)) {
    LOG_SEVERE("Error: invalid address \"%s\"\n",
	       pcAddr);
    return CLI_ERROR_COMMAND_FAILED;
  }

  if (as_add_peer(pRouter, uASNum, tAddr, 0)) {
    LOG_SEVERE("Error: peer already exists\n");
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
    LOG_SEVERE("Error: invalid prefix \"%s\"\n", pcPrefix);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Add the network
  if (as_add_network(pRouter, sPrefix))
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
    LOG_SEVERE("Error: invalid prefix \"%s\"\n", pcPrefix);
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
  SAS * pAS;
  SPrefix sPrefix;
  char * pcEndPtr;
  unsigned int uDelay;

  /*LOG_DEBUG("bgp router %s add network %s\n",
	    tokens_get_string_at(pTokens, 0),
	    tokens_get_string_at(pTokens, 1));*/
  pAS= (SAS *) cli_context_get_item_at_top(pContext);
  if (ip_string_to_prefix(tokens_get_string_at(pTokens, 1),
			  &pcEndPtr, &sPrefix) || (*pcEndPtr != '\0'))
    return CLI_ERROR_COMMAND_FAILED;
  if (tokens_get_uint_at(pTokens, 2, &uDelay))
    return CLI_ERROR_COMMAND_FAILED;
  if (as_add_qos_network(pAS, sPrefix, (net_link_delay_t) uDelay))
    return CLI_ERROR_COMMAND_FAILED;
  return CLI_SUCCESS;
}
#endif

// ----- cli_ctx_create_bgp_router_peer -----------------------------
/**
 * context: {router} -> {router, peer}
 * tokens: {addr, addr}
 */
int cli_ctx_create_bgp_router_peer(SCliContext * pContext,
				   void ** ppItem)
{
  SAS * pAS;
  SPeer * pPeer;
  char * pcPeerAddr;
  char * pcEndPtr;
  net_addr_t tAddr;

  // Get the BGP instance from context
  pAS= (SAS *) cli_context_get_item_at_top(pContext);

  // Get the peer address
  pcPeerAddr= tokens_get_string_at(pContext->pTokens, 1);
  if (ip_string_to_address(pcPeerAddr, &pcEndPtr, &tAddr) ||
      (*pcEndPtr != 0)) {
    LOG_SEVERE("Error: invalid peer address \"%s\"\n",
	       pcPeerAddr);
    return CLI_ERROR_CTX_CREATE;
  }

  // Get the peer
  pPeer= as_find_peer(pAS, tAddr);
  if (pPeer == NULL) {
    LOG_SEVERE("Error: unknown peer\n");
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
    LOG_SEVERE("Error: invalid filter type \"%s\"\n", pcFilter);
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
    LOG_SEVERE("Error: invalid predicate \"%s\"\n", pcPredicate);
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
    LOG_SEVERE("Error: invalid action \"%s\"\n", pcAction);
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
    LOG_SEVERE("Error: invalid index\n");
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
    LOG_SEVERE("Error: filter contains no rule\n");
    return CLI_ERROR_COMMAND_FAILED;
  }
  
  // Get index of rule to be removed
  if (tokens_get_uint_at(pTokens,
			 tokens_get_num(pTokens)-1,
			 &uIndex)) {
    LOG_SEVERE("Error: invalid index\n");
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Remove rule
  if (filter_remove_rule(*ppFilter, uIndex)) {
    LOG_SEVERE("Error: could not remove rule %u\n", uIndex);
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
  LOG_SEVERE("Error: not yet implemented, sorry.\n");
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

  filter_dump(stdout, *ppFilter);

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
  if (filter_parser_run(tokens_get_string_at(pTokens, 2), &pFilter) !=
      FILTER_PARSER_SUCCESS) {
    LOG_SEVERE("Error: invalid filter\n");
    return CLI_ERROR_COMMAND_FAILED;
  }

  peer_set_in_filter(pPeer, pFilter);

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
  pFilter= peer_in_filter_get(pPeer);
  if (pFilter == NULL) {
    pFilter= filter_create();
    peer_set_in_filter(pPeer, pFilter);
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
  if (filter_parser_run(tokens_get_string_at(pTokens, 2), &pFilter) !=
      FILTER_PARSER_SUCCESS)
    return CLI_ERROR_COMMAND_FAILED;
  peer_set_out_filter(pPeer, pFilter);
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
  pFilter= peer_out_filter_get(pPeer);
  if (pFilter == NULL) {
    pFilter= filter_create();
    peer_set_out_filter(pPeer, pFilter);
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
    bgp_peer_dump_in_filters(stdout, pPeer);
  else if (!strcmp(pcFilter, "out"))
    bgp_peer_dump_out_filters(stdout, pPeer);
  else {
    LOG_SEVERE("Error: invalid filter type \"%s\"\n", pcFilter);
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
  SAS * pAS;
  SPeer * pPeer;

  pAS= (SAS *) cli_context_get_item_at(pContext, 0);
  pAS->iRouteReflector= 1;
  pPeer= (SPeer *) cli_context_get_item_at_top(pContext);
  peer_flag_set(pPeer, PEER_FLAG_RR_CLIENT, 1);
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
  SPeer * pPeer;

  pPeer= (SPeer *) cli_context_get_item_at_top(pContext);
  peer_flag_set(pPeer, PEER_FLAG_NEXT_HOP_SELF, 1);
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
  if (!peer_flag_get(pPeer, PEER_FLAG_VIRTUAL)) {
    LOG_SEVERE("Error: only virtual peers can do that\n");
    return CLI_ERROR_COMMAND_FAILED;
  }

  /* Get the router instance from context */
  pRouter= (SBGPRouter *) cli_context_get_item_at(pContext, 0);

  /* Get the MRT-record */
  pcRecord= tokens_get_string_at(pTokens, 2);

  /* Build a message from the MRT-record */
  pMsg= mrtd_msg_from_line(pRouter, pPeer, pcRecord);

  if (pMsg == NULL) {
    LOG_SEVERE("Error: could not build message from input\n");
    return CLI_ERROR_COMMAND_FAILED;
  }

  peer_handle_message(pPeer, pMsg);

  return CLI_SUCCESS;
}

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
  if (peer_open_session(pPeer)) {
    LOG_SEVERE("Error: could not open session\n");
    return CLI_ERROR_COMMAND_FAILED;
  }
    
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
  peer_flag_set(pPeer, PEER_FLAG_VIRTUAL, 1);
    
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
  if (peer_close_session(pPeer)) {
    LOG_SEVERE("Error: could not close session\n");
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
  if (peer_close_session(pPeer)) {
    LOG_SEVERE("Error: could not close session\n");
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Try to open session
  if (peer_open_session(pPeer)) {
    LOG_SEVERE("Error: could not close session\n");
    return CLI_ERROR_COMMAND_FAILED;
  }

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

// ----- cli_register_bgp_topology ----------------------------------
int cli_register_bgp_topology(SCliCmds * pCmds)
{
  SCliCmds * pSubCmds;
  SCliParams * pParams;

  pSubCmds= cli_cmds_create();
  pParams= cli_params_create();
  cli_params_add(pParams, "<file>", NULL);
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
  pParams= cli_params_create();
  cli_params_add(pParams, "<prefix>", NULL);
  cli_params_add(pParams, "<bound>", NULL);
  cli_params_add(pParams, "<input>", NULL);
  cli_params_add(pParams, "<output>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("record-route-bm",
					cli_bgp_topology_recordroute_bm,
					NULL, pParams));
  cli_cmds_add(pSubCmds, cli_cmd_create("show-rib",
					cli_bgp_topology_showrib,
					NULL, NULL));
  pParams= cli_params_create();
  cli_params_add(pParams, "<prefix>", NULL);
  cli_params_add(pParams, "<output>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("route-dp-rule",
					cli_bgp_topology_route_dp_rule,
					NULL, pParams));
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
  cli_cmds_add(pSubCmds, cli_cmd_create("next-hop-self",
					cli_bgp_router_peer_nexthopself,
					NULL, NULL));
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
  cli_cmds_add(pSubCmds, cli_cmd_create("up", cli_bgp_router_peer_up,
					NULL, NULL));
  cli_cmds_add(pSubCmds, cli_cmd_create("virtual",
					cli_bgp_router_peer_virtual,
					NULL, NULL));
  pParams= cli_params_create();
  cli_params_add(pParams, "<addr>", NULL);
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
  cli_params_add(pParams, "<nlri>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("nlri",
					cli_bgp_options_nlri,
					NULL, pParams));
#ifdef BGP_QOS
  pParams= cli_params_create();
  cli_params_add(pParams, "<limit>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("qos-aggr-limit",
					cli_bgp_options_qosaggrlimit,
					NULL, pParams));
#endif
  pParams= cli_params_create();
  cli_params_add(pParams, "<function>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("tie-break",
					cli_bgp_options_tiebreak,
					NULL, pParams));
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
  cli_params_add(pParams, "<file>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("rib",
					cli_bgp_router_load_rib,
					NULL, pParams));
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
  cli_params_add(pParams, "<file>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("rib",
					cli_bgp_router_save_rib,
					NULL, pParams));
  pParams= cli_params_create();
  cli_params_add(pParams, "<peer>", NULL);
  cli_params_add(pParams, "<file>", NULL);
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
  pParams= cli_params_create();
  cli_params_add(pParams, "<function>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("tie-break",
				     cli_bgp_router_set_tiebreak,
				     NULL, pParams));
  return cli_cmds_add(pCmds, cli_cmd_create("set", NULL,
					    pSubCmds, NULL));
}

// ----- cli_register_bgp_router_show -------------------------------
int cli_register_bgp_router_show(SCliCmds * pCmds)
{
  SCliCmds * pSubCmds;
  SCliParams * pParams;

  pSubCmds= cli_cmds_create();
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
  cli_params_add(pParams, "<prefix|address|*>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("rib",
					cli_bgp_router_show_rib,
					NULL, pParams));
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
  cli_register_bgp_router_debug(pSubCmds);
  cli_register_bgp_router_del(pSubCmds);
  cli_register_bgp_router_peer(pSubCmds);
  pParams= cli_params_create();
  cli_params_add(pParams, "<prefix>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("record-route",
					cli_bgp_router_recordroute,
					NULL, pParams));
  pParams= cli_params_create();
  cli_params_add(pParams, "<prefix>", NULL);
  cli_params_add(pParams, "<bound>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("record-route-bm",
					cli_bgp_router_recordroute_bm,
					NULL, pParams));
  cli_cmds_add(pSubCmds, cli_cmd_create("rescan",
					cli_bgp_router_rescan,
					NULL, NULL));
  cli_cmds_add(pSubCmds, cli_cmd_create("reset",
					cli_bgp_router_reset,
					NULL, NULL));
  cli_register_bgp_router_load(pSubCmds);
  cli_register_bgp_router_save(pSubCmds);
  cli_register_bgp_router_set(pSubCmds);
  cli_register_bgp_router_show(pSubCmds);
  pParams= cli_params_create();
  cli_params_add(pParams, "<addr>", NULL);
  return cli_cmds_add(pCmds, cli_cmd_create_ctx("router",
						cli_ctx_create_bgp_router,
						cli_ctx_destroy_bgp_router, 
						pSubCmds, pParams));
}

// ----- cli_register_bgp -------------------------------------------
/**
 *
 */
int cli_register_bgp(SCli * pCli)
{
  SCliCmds * pCmds;

  pCmds= cli_cmds_create();
  cli_register_bgp_add(pCmds);
  cli_register_bgp_assert(pCmds);
  cli_register_bgp_options(pCmds);
  cli_register_bgp_topology(pCmds);
  cli_register_bgp_router(pCmds);
  cli_register_cmd(pCli, cli_cmd_create("bgp", NULL, pCmds, NULL));
  return 0;
}
