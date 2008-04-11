// ==================================================================
// @(#)net_node_iface.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 14/02/2008
// $Id: net_node_iface.c,v 1.3 2008-04-11 11:03:06 bqu Exp $
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>

#include <libgds/cli.h>
#include <libgds/cli_ctx.h>
#include <libgds/str_util.h>

#include <cli/common.h>
#include <net/iface.h>
#include <net/net_types.h>
#include <net/node.h>
#include <net/error.h>

// -----[ _iface_from_context ]--------------------------------------
static inline net_iface_t *_iface_from_context(SCliContext * pContext) {
  net_iface_t * pIface= (net_iface_t *) cli_context_get_item_at_top(pContext);
  assert(pIface != NULL);
  return pIface;
}

// -----[ cli_ctx_create_iface ]-------------------------------------
/**
 * context: {node}
 * tokens: {prefix|address}
 */
static int cli_ctx_create_iface(SCliContext * pContext, void ** ppItem)
{
  net_node_t * pNode= (net_node_t *) cli_context_get_item_at_top(pContext);
  net_iface_t * pIface= NULL;
  net_iface_id_t tIfaceID;
  char * pcTmp;
  int iResult;

  assert(pNode != NULL);
  
  // Get interface identifier
  pcTmp= tokens_get_string_at(pContext->pCmd->pParamValues, 0);
  iResult= net_iface_str2id(pcTmp, &tIfaceID);
  if (iResult != ESUCCESS) {
    cli_set_user_error(cli_get(), "invalid input \"%s\" (%s)", pcTmp,
		       network_strerror(iResult));
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Find interface in current node
  pIface= node_find_iface(pNode, tIfaceID);
  if (pIface == NULL) {
    cli_set_user_error(cli_get(), "no such interface \"%s\".", pcTmp);
    return CLI_ERROR_COMMAND_FAILED;
  }

  *ppItem= pIface;
  return CLI_SUCCESS;
}

// -----[ cli_ctx_destroy_iface ]------------------------------------
static void cli_ctx_destroy_iface(void ** ppItem)
{
  /* Nothing to do here (context is only a reference). */
}

// -----[ cli_iface_igpweight ]--------------------------------------
/**
 * context: {iface}
 * tokens: {weight}
 */
static int cli_iface_igpweight(SCliContext * pContext, SCliCmd * pCmd)
{
  net_iface_t * pIface= _iface_from_context(pContext);
  unsigned int uWeight;
  igp_weight_t tWeight;
  int iResult;

  // Get IGP weight
  if (tokens_get_uint_at(pCmd->pParamValues, 0, &uWeight) != 0) {
    cli_set_user_error(cli_get(), "invalid weight \"%s\"",
		       tokens_get_string_at(pCmd->pParamValues, 0));
    return CLI_ERROR_COMMAND_FAILED;
  }
  tWeight= (igp_weight_t) uWeight;

  // Change link weight in forward direction
  iResult= net_iface_set_metric(pIface, 0, tWeight, 0);
  if (iResult != ESUCCESS) {
    cli_set_user_error(cli_get(), "cannot set weight (%s)",
		       network_strerror(iResult));
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// -----[ cli_iface_connect ]----------------------------------------
/**
 * context: {iface}
 * tokens: {}
 */
static int cli_iface_connect(SCliContext * pContext, SCliCmd * pCmd)
{
  net_iface_t * pIface= _iface_from_context(pContext);

  pIface= NULL;

  return CLI_SUCCESS;
}

// -----[ cli_iface_load_clear ]-------------------------------------
/**
 * context: {iface}
 * tokens: {}
 */
static int cli_iface_load_clear(SCliContext * pContext, SCliCmd * pCmd)
{
  net_iface_t * pIface= _iface_from_context(pContext);

  net_iface_set_load(pIface, 0);

  return CLI_SUCCESS;
}

// -----[ cli_iface_load_add ]---------------------------------------
/**
 * context: {iface}
 * tokens: {load}
 */
static int cli_iface_load_add(SCliContext * pContext, SCliCmd * pCmd)
{
  net_iface_t * pIface= _iface_from_context(pContext);
  char * pcValue;
  unsigned int uValue;
  net_link_load_t tLoad;

  // Get load
  pcValue= tokens_get_string_at(pCmd->pParamValues, 0);
  if (str_as_uint(pcValue, &uValue) < 0) {
    cli_set_user_error(cli_get(), "invalid load \"%s\"", pcValue);
    return CLI_ERROR_COMMAND_FAILED;
  }
  tLoad= uValue;

  net_iface_set_load(pIface, tLoad);

  return CLI_SUCCESS;
}

// -----[ cli_iface_load_show ]--------------------------------------
/**
 * context: {iface}
 * tokens: {}
 */
static int cli_iface_load_show(SCliContext * pContext, SCliCmd * pCmd)
{
  net_iface_t * pIface= _iface_from_context(pContext);

  net_link_dump_load(pLogOut, pIface);

  return CLI_SUCCESS;
}

// -----[ cli_net_node_add_iface ]----------------------------------
/**
 * context: {node}
 * tokens: {address|prefix, type}
 * type: {rtr, ptp, ptmp, virtual}
 */
static int cli_net_node_add_iface(SCliContext * pContext, SCliCmd * pCmd)
{
  net_node_t * pNode= (net_node_t *) cli_context_get_item_at_top(pContext);
  char * pcTmp;
  net_error_t error;
  net_iface_id_t tIfaceID;
  net_iface_type_t tIfaceType;

  // Get interface identifier
  pcTmp= tokens_get_string_at(pContext->pCmd->pParamValues, 0);
  error= net_iface_str2id(pcTmp, &tIfaceID);
  if (error != ESUCCESS) {
    cli_set_user_error(cli_get(), "could not add interface (%s)",
		       network_strerror(error));
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Get interface type
  pcTmp= tokens_get_string_at(pContext->pCmd->pParamValues, 1);
  error= net_iface_str2type(pcTmp, &tIfaceType);
  if (error != ESUCCESS) {
    cli_set_user_error(cli_get(), "could not add interface (%s)",
		       network_strerror(error));
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Add new interface
  error= node_add_iface(pNode, tIfaceID, tIfaceType);
  if (error != ESUCCESS) {
    cli_set_user_error(cli_get(), "could not add interface (%s)",
		       network_strerror(error));
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// -----[ cli_register_net_node_add_iface ]-------------------------
int cli_register_net_node_add_iface(SCliCmds * pCmds)
{
  SCliParams * pParams;

  pParams= cli_params_create();
  cli_params_add(pParams, "address|prefix", NULL);
  cli_params_add(pParams, "type", NULL);
  return cli_cmds_add(pCmds, cli_cmd_create("iface",
					    cli_net_node_add_iface,
					    NULL, pParams));
}

// -----[ cli_register_net_node_iface_load --------------------------
static int cli_register_net_node_iface_load(SCliCmds * pCmds)
{
  SCliCmds * pSubCmds;
  SCliParams * pParams;
  
  pSubCmds= cli_cmds_create();
  pParams= cli_params_create();
  cli_params_add(pParams, "<load>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("add", cli_iface_load_add,
					NULL, pParams));
  cli_cmds_add(pSubCmds, cli_cmd_create("clear", cli_iface_load_clear,
					NULL, NULL));
  cli_cmds_add(pSubCmds, cli_cmd_create("show", cli_iface_load_show,
					NULL, NULL));
  return cli_cmds_add(pCmds, cli_cmd_create("load", NULL, pSubCmds, NULL));
}

// -----[ cli_register_net_node_iface ]------------------------------
int cli_register_net_node_iface(SCliCmds * pCmds)
{
  SCliCmds * pSubCmds= cli_cmds_create();
  SCliParams * pParams;

  pParams= cli_params_create();
  cli_cmds_add(pSubCmds, cli_cmd_create("connect",
					cli_iface_connect,
					NULL, pParams));
  pParams= cli_params_create();
  cli_params_add(pParams, "<weight>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("igp-weight",
					cli_iface_igpweight,
					NULL, pParams));
  cli_register_net_node_iface_load(pSubCmds);
  pParams= cli_params_create();
  cli_params_add(pParams, "<address|prefix>", NULL);
  return cli_cmds_add(pCmds, cli_cmd_create_ctx("iface",
						cli_ctx_create_iface,
						cli_ctx_destroy_iface,
						pSubCmds, pParams));
}
