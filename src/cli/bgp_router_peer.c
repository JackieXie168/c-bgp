// ==================================================================
// @(#)bgp_router_peer.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be),
// @date 27/02/2008
// @lastdate 27/02/2008
// ==================================================================

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <string.h>

#include <libgds/cli_ctx.h>

#include <bgp/filter.h>
#include <bgp/filter_parser.h>
#include <bgp/mrtd.h>
#include <bgp/peer.h>
#include <bgp/as.h>
#include <cli/bgp_filter.h>
#include <cli/common.h>
#include <cli/enum.h>
#include <net/util.h>

// -----[ _peer_from_context ]---------------------------------------
static inline SBGPPeer * _peer_from_context(SCliContext * pContext) {
  SBGPPeer * pPeer=
    (SBGPPeer *) cli_context_get_item_at_top(pContext);
  assert(pPeer != NULL);
  return pPeer;
}

// -----[ cli_ctx_create_peer ]--------------------------------------
/**
 * context: {router} -> {router, peer}
 * tokens: {addr}
 */
static int cli_ctx_create_peer(SCliContext * pContext,
					  void ** ppItem)
{
  SBGPRouter * pRouter= (SBGPRouter *) cli_context_get_item_at_top(pContext);
  bgp_peer_t * pPeer;
  char * pcPeerAddr;
  net_addr_t tAddr;

  assert(pRouter != NULL);

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

// -----[ cli_ctx_destroy_peer ]-------------------------------------
static void cli_ctx_destroy_peer(void ** ppItem)
{
} 

// -----[ cli_peer_infilter_set ]------------------------------------
/**
 * context: {router, peer}
 * tokens: {addr, addr, filter}
 */
/*static int cli_peer_infilter_set(SCliContext * pContext,
				 SCliCmd * pCmd)
{
  bgp_peer_t * pPeer= _peer_from_context(pContext);
  SFilter * pFilter= NULL;

  bgp_peer_set_in_filter(pPeer, pFilter);

  return CLI_SUCCESS;
  }*/

// -----[ cli_peer_infilter_add ]------------------------------------
/**
 * context: {router, peer}
 * tokens: {filter}
 */
 /*static int cli_peer_infilter_add(SCliContext * pContext,
				 SCliCmd * pCmd)
{
  bgp_peer_t * pPeer= _peer_from_context(pContext);
  SFilterRule * pRule;
  SFilter * pFilter;

  if (filter_parser_rule(tokens_get_string_at(pCmd->pParamValues, 0), &pRule)
      != FILTER_PARSER_SUCCESS)
    return CLI_ERROR_COMMAND_FAILED;
  pFilter= bgp_peer_in_filter_get(pPeer);
  if (pFilter == NULL) {
    pFilter= filter_create();
    bgp_peer_set_in_filter(pPeer, pFilter);
  }
  filter_add_rule2(pFilter, pRule);
  return CLI_SUCCESS;
  }*/

// -----[ cli_peer_outfilter_set ]-----------------------------------
/**
 * context: {router, peer}
 * tokens: {filter}
 */
  /*static int cli_peer_outfilter_set(SCliContext * pContext,
					     SCliCmd * pCmd)
{
  bgp_peer_t * pPeer= _peer_from_context(pContext);
  SFilter * pFilter= NULL;

  bgp_peer_set_out_filter(pPeer, pFilter);
  return CLI_SUCCESS;
  }*/

// -----[ cli_peer_outfilter_add ]-----------------------------------
/**
 * context: {router, peer}
 * tokens: {filter}
 */
   /*static int cli_peer_outfilter_add(SCliContext * pContext,
				  SCliCmd * pCmd)
{
  bgp_peer_t * pPeer= _peer_from_context(pContext);
  SFilterRule * pRule;
  SFilter * pFilter;

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
  }*/

// -----[ cli_ctx_create_peer_filter ]-------------------------------
/**
 * context: {router, peer}
 * tokens: {in|out}
 */
static int cli_ctx_create_peer_filter(SCliContext * pContext,
				      void ** ppItem)
{
  bgp_peer_t * pPeer= _peer_from_context(pContext);
  char * pcFilter;

  // Create filter context
  pcFilter= tokens_get_string_at(pContext->pCmd->pParamValues, 0);
  if (!strcmp("in", pcFilter))
    *ppItem= &(pPeer->pFilter[FILTER_IN]);
  else if (!strcmp("out", pcFilter))
    *ppItem= &(pPeer->pFilter[FILTER_OUT]);
  else {
    cli_set_user_error(cli_get(), "invalid filter type \"%s\"", pcFilter);
    return CLI_ERROR_CTX_CREATE;
  }

  return CLI_SUCCESS;
}

// -----[ cli_ctx_destroy_peer_filter ]------------------------------
static void cli_ctx_destroy_peer_filter(void ** ppItem)
{
}

// -----[ cli_peer_show_filter ]-------------------------------------
/**
 * context: {router, peer}
 * tokens: {in|out}
 */
/*static int cli_peer_show_filter(SCliContext * pContext,
				SCliCmd * pCmd)
{
  bgp_peer_t * pPeer= _peer_from_context(pContext);
  char * pcFilter;

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
  }*/

// -----[ cli_peer_rrclient ]----------------------------------------
/**
 * context: {router, peer}
 * tokens: {}
 */
static int cli_peer_rrclient(SCliContext * pContext, SCliCmd * pCmd)
{
  bgp_peer_t * pPeer= _peer_from_context(pContext);

  pPeer->pLocalRouter->iRouteReflector= 1;
  bgp_peer_flag_set(pPeer, PEER_FLAG_RR_CLIENT, 1);
  return CLI_SUCCESS;
}

// -----[ cli_peer_nexthop ]-----------------------------------------
/**
 * context: {router, peer}
 * tokens: {addr}
 */
static int cli_peer_nexthop(SCliContext * pContext,
			    SCliCmd * pCmd)
{
  bgp_peer_t * pPeer= _peer_from_context(pContext);
  char * pcNextHop;
  net_addr_t tNextHop;
  char * pcEndChar;

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

// -----[ cli_peer_nexthopself ]-------------------------------------
/**
 * context: {router, peer}
 * tokens: {}
 */
static int cli_peer_nexthopself(SCliContext * pContext,
				    SCliCmd * pCmd)
{
  bgp_peer_t * pPeer= _peer_from_context(pContext);

  bgp_peer_flag_set(pPeer, PEER_FLAG_NEXT_HOP_SELF, 1);
  return CLI_SUCCESS;
}

// -----[ cli_peer_record ]------------------------------------------
/**
 * context: {router, peer}
 * tokens: {file|-}
 */
static int cli_peer_record(SCliContext * pContext,
			   SCliCmd * pCmd)
{
  bgp_peer_t * pPeer= _peer_from_context(pContext);
  char * pcFileName;
  SLogStream * pStream;
  
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

// -----[ cli_peer_recv ]--------------------------------------------
/**
 * context: {router, peer}
 * tokens: {mrt-record}
 */
static int cli_peer_recv(SCliContext * pContext,
			 SCliCmd * pCmd)
{
  bgp_peer_t * pPeer= _peer_from_context(pContext);
  char * pcRecord;
  SBGPMsg * pMsg;

  /* Check that the peer is virtual */
  if (!bgp_peer_flag_get(pPeer, PEER_FLAG_VIRTUAL)) {
    cli_set_user_error(cli_get(), "only virtual peers can do that");
    return CLI_ERROR_COMMAND_FAILED;
  }

  /* Get the MRT-record */
  pcRecord= tokens_get_string_at(pCmd->pParamValues, 0);

  /* Build a message from the MRT-record */
  pMsg= mrtd_msg_from_line(pPeer->pLocalRouter, pPeer, pcRecord);

  if (pMsg == NULL) {
    cli_set_user_error(cli_get(), "could not build message from input");
    return CLI_ERROR_COMMAND_FAILED;
  }

  bgp_peer_handle_message(pPeer, pMsg);

  return CLI_SUCCESS;
}

// -----[ cli_peer_walton_limit ]------------------------------------
/**
 * context: {router, peer}
 * tokens: {announce-limit}
 */
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
static int cli_peer_walton_limit(SCliContext * pContext, 
				 SCliCmd * pCmd)
{
  bgp_peer_t * pPeer= _peer_from_context(pContext);
  unsigned int uWaltonLimit;

  if (tokens_get_uint_at(pCmd->pParamValues, 0, &uWaltonLimit)) {
    cli_set_user_error(cli_get(), "invalid walton limitation");
    return CLI_ERROR_COMMAND_FAILED;
  }
  peer_set_walton_limit(pPeer, uWaltonLimit);

  return CLI_SUCCESS;

  
}
#endif

// -----[ cli_peer_up ]----------------------------------------------
/**
 * context: {router, peer}
 * tokens: {}
 */
static int cli_peer_up(SCliContext * pContext, SCliCmd * pCmd)
{
  bgp_peer_t * pPeer= _peer_from_context(pContext);

  // Try to open session
  if (bgp_peer_open_session(pPeer)) {
    cli_set_user_error(cli_get(), "could not open session");
    return CLI_ERROR_COMMAND_FAILED;
  }
    
  return CLI_SUCCESS;
}

// -----[ cli_peer_update_source ]-----------------------------------
/**
 * context: {router, peer}
 * tokens: {src-addr}
 */
static int cli_peer_update_source(SCliContext * pContext, SCliCmd * pCmd)
{
  bgp_peer_t * pPeer= _peer_from_context(pContext);
  char * pcSrcAddr, * pcEndChar;
  net_addr_t tSrcAddr;

  // Get source address
  pcSrcAddr= tokens_get_string_at(pCmd->pParamValues, 0);
  if (ip_string_to_address(pcSrcAddr, &pcEndChar, &tSrcAddr) ||
      (*pcEndChar != 0)) {
    cli_set_user_error(cli_get(), "invalid source address \"%s\".", pcSrcAddr);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Try to open session
  if (bgp_peer_set_source(pPeer, tSrcAddr)) {
    cli_set_user_error(cli_get(), "could not set source address");
    return CLI_ERROR_COMMAND_FAILED;
  }
    
  return CLI_SUCCESS;
}

// -----[ cli_peer_softrestart ]-------------------------------------
/**
 * context: {router, peer}
 * tokens: {}
 */
static int cli_peer_softrestart(SCliContext * pContext, SCliCmd * pCmd)
{
  bgp_peer_t * pPeer= _peer_from_context(pContext);

  // Set the virtual flag of this peer
  if (!bgp_peer_flag_get(pPeer, PEER_FLAG_VIRTUAL)) {
    cli_set_user_error(cli_get(), "soft-restart option only available to virtual peers");
    return CLI_ERROR_COMMAND_FAILED;
  }
  bgp_peer_flag_set(pPeer, PEER_FLAG_SOFT_RESTART, 1);
    
  return CLI_SUCCESS;
}

// -----[ cli_peer_virtual ]-----------------------------------------
/**
 * context: {router, peer}
 * tokens: {}
 */
static int cli_peer_virtual(SCliContext * pContext, SCliCmd * pCmd)
{
  bgp_peer_t * pPeer= _peer_from_context(pContext);

  // Set the virtual flag of this peer
  bgp_peer_flag_set(pPeer, PEER_FLAG_VIRTUAL, 1);
    
  return CLI_SUCCESS;
}

// -----[ cli_peer_down ]--------------------------------------------
/**
 * context: {router, peer}
 * tokens: {}
 */
static int cli_peer_down(SCliContext * pContext, SCliCmd * pCmd)
{
  bgp_peer_t * pPeer= _peer_from_context(pContext);

  // Try to close session
  if (bgp_peer_close_session(pPeer)) {
    cli_set_user_error(cli_get(), "could not close session");
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// -----[ cli_peer_readv ]-------------------------------------------
/**
 * context: {router, peer}
 * tokens: {prefix|*}
 */
#ifdef __EXPERIMENTAL__
static int cli_peer_readv(SCliContext * pContext, SCliCmd * pCmd)
{
  bgp_peer_t * pPeer= _peer_from_context(pContext);
  SBGPRouter * pRouter= pPeer->pLocalRouter;
  char * pcPrefix;
  char * pcEndChar;
  SPrefix sPrefix;

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
#endif /* __EXPERIMENTAL__ */

// -----[ cli_peer_reset ]-------------------------------------------
/**
 * context: {router, peer}
 * tokens: {}
 */
static int cli_peer_reset(SCliContext * pContext, SCliCmd * pCmd)
{
  bgp_peer_t * pPeer= _peer_from_context(pContext);

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

// -----[ _cli_register_peer_filter ]--------------------------------
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
static int _cli_register_peer_filter(SCliCmds * pCmds)
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
  /*pParams= cli_params_create();
  cli_params_add(pParams, "<index>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create_ctx("rule",
					    cli_ctx_create_bgp_filter_rule,
					    cli_ctx_destroy_bgp_filter_rule,
					    cli_register_bgp_filter_rule(),
					    pParams));*/
  cli_cmds_add(pSubCmds, cli_cmd_create("show",
					cli_bgp_filter_show,
					NULL, NULL));
  pParams= cli_params_create();
  cli_params_add(pParams, "<in|out>", NULL);
  return cli_cmds_add(pCmds,
		      cli_cmd_create_ctx("filter",
					 cli_ctx_create_peer_filter,
					 cli_ctx_destroy_peer_filter,
					 pSubCmds, pParams));
}

// -----[ _cli_register_peer_filters ]-------------------------------
/*static int _cli_register_peer_filters(SCliCmds * pCmds)
{
  SCliCmds * pSubCmds;
  SCliParams * pParams;

  pSubCmds= cli_cmds_create();
  pParams= cli_params_create();
  cli_params_add(pParams, "<filter>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("set",
					cli_peer_infilter_set,
					NULL, pParams));
  pParams= cli_params_create();
  cli_params_add(pParams, "<filter>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("add",
					cli_peer_infilter_add,
					NULL, pParams));
  cli_cmds_add(pCmds, cli_cmd_create("in-filter", NULL,
				     pSubCmds, NULL));
  pSubCmds= cli_cmds_create();
  pParams= cli_params_create();
  cli_params_add(pParams, "<filter>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("set",
					cli_peer_outfilter_set,
					NULL, pParams));
  pParams= cli_params_create();
  cli_params_add(pParams, "<filter>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("add",
					cli_peer_outfilter_add,
					NULL, pParams));
  return cli_cmds_add(pCmds, cli_cmd_create("out-filter", NULL,
					    pSubCmds, NULL));
					    }*/

// -----[ cli_register_peer_show ]-----------------------------------
/*static int cli_register_peer_show(SCliCmds * pCmds)
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
					    }*/

// -----[ cli_register_bgp_router_peer ]-----------------------------
int cli_register_bgp_router_peer(SCliCmds * pCmds)
{
  SCliCmds * pSubCmds;
  SCliParams * pParams;

  pSubCmds= cli_cmds_create();
  _cli_register_peer_filter(pSubCmds);
  //_cli_register_peer_filters(pSubCmds);
  //_cli_register_peer_show(pSubCmds);
  cli_cmds_add(pSubCmds, cli_cmd_create("down", cli_peer_down,
					NULL, NULL));
  pParams= cli_params_create();
  cli_params_add(pParams, "<next-hop>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("next-hop",
					cli_peer_nexthop,
					NULL, pParams));
  cli_cmds_add(pSubCmds, cli_cmd_create("next-hop-self",
					cli_peer_nexthopself,
					NULL, NULL));
#ifdef __EXPERIMENTAL__
  pParams= cli_params_create();
  cli_params_add(pParams, "<prefix|*>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("re-adv",
					cli_peer_readv,
					NULL, pParams));
#endif
  pParams= cli_params_create();
  cli_params_add(pParams, "<file>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("record",
					cli_peer_record,
					NULL, pParams));
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  pParams = cli_params_create();
  cli_params_add(pParams, "<announce-limit>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("walton-limit", 
					cli_peer_walton_limit,
					NULL, pParams));
#endif
  pParams= cli_params_create();
  cli_params_add(pParams, "<mrt-record>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("recv",
					cli_peer_recv,
					NULL, pParams));
  cli_cmds_add(pSubCmds, cli_cmd_create("reset", cli_peer_reset,
					NULL, NULL));
  cli_cmds_add(pSubCmds, cli_cmd_create("rr-client",
					cli_peer_rrclient,
					NULL, NULL));
  cli_cmds_add(pSubCmds, cli_cmd_create("soft-restart",
					cli_peer_softrestart,
					NULL, NULL));
  cli_cmds_add(pSubCmds, cli_cmd_create("up", cli_peer_up,
					NULL, NULL));
  pParams= cli_params_create();
  cli_params_add(pParams, "<iface-address>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("update-source",
					cli_peer_update_source,
					NULL, pParams));
  cli_cmds_add(pSubCmds, cli_cmd_create("virtual",
					cli_peer_virtual,
					NULL, NULL));
  pParams= cli_params_create();
  cli_params_add2(pParams, "<addr>", NULL, cli_enum_bgp_peers_addr);
  return cli_cmds_add(pCmds, cli_cmd_create_ctx("peer",
						cli_ctx_create_peer,
						cli_ctx_destroy_peer,
						pSubCmds, pParams));
}
