// ==================================================================
// @(#)bgp_topology.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be), 
// @date 30/04/2007
// @lastdate 14/05/2007
// ==================================================================

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include <libgds/types.h>

#include <bgp/as-level.h>
#include <bgp/caida.h>
#include <bgp/rexford.h>
#include <cli/common.h>
#include <net/util.h>

// -----[ cli_bgp_topology_load ]------------------------------------
/**
 * context: {}
 * tokens: {file}
 * options:
 *   --addr-sch=default|local
 *   --format=default|caida
 */
int cli_bgp_topology_load(SCliContext * pContext, SCliCmd * pCmd)
{
  char * pcValue;
  uint8_t uAddrScheme= ASLEVEL_ADDR_SCH_DEFAULT;
  uint8_t uFormat= ASLEVEL_FORMAT_DEFAULT;
  int iResult;

  // Optional addressing scheme specified ?
  pcValue= cli_options_get_value(pCmd->pOptions, "addr-sch");
  if ((pcValue != NULL) &&
      (aslevel_str2addr_sch(pcValue, &uAddrScheme) != ASLEVEL_SUCCESS)) {
    LOG_ERR(LOG_LEVEL_SEVERE, "invalid addressing scheme \"%s\"\n", pcValue);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Optional format specified ?
  pcValue= cli_options_get_value(pCmd->pOptions, "format");
  if ((pcValue != NULL) &&
      (aslevel_topo_str2format(pcValue, &uFormat) != ASLEVEL_SUCCESS)) {
    LOG_ERR(LOG_LEVEL_SEVERE, "invalid format \"%s\"\n", pcValue);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Load AS-level topology
  pcValue= tokens_get_string_at(pCmd->pParamValues, 0);
  iResult= aslevel_topo_load(pcValue, uFormat, uAddrScheme);
  if (iResult != ASLEVEL_SUCCESS) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: ");
    aslevel_perror(pLogErr, iResult);
    LOG_ERR(LOG_LEVEL_SEVERE, "\n");
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// -----[ cli_bgp_topology_check ]-----------------------------------
/**
 * context: {}
 * tokens: {}
 * options: {--verbose}
 */
int cli_bgp_topology_check(SCliContext * pContext, SCliCmd * pCmd)
{
  int iResult;
  int iVerbose= 0;

  // Verbose check ?
  if (cli_options_has_value(pCmd->pOptions, "verbose"))
    iVerbose= 1;

  iResult= aslevel_topo_check(iVerbose);
  if (iResult != ASLEVEL_SUCCESS) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: ");
    aslevel_perror(pLogErr, iResult);
    LOG_ERR(LOG_LEVEL_SEVERE, "\n");
    return CLI_ERROR_COMMAND_FAILED;
  }
  return CLI_SUCCESS;
}

// -----[ cli_bgp_topology_filter ]----------------------------------
/**
 * context: {}
 * tokens: {<what>}
 */
int cli_bgp_topology_filter(SCliContext * pContext, SCliCmd * pCmd)
{
  int iResult;
  uint8_t uFilter;

  // Get filter parameter
  iResult= aslevel_topo_str2filter(tokens_get_string_at(pCmd->pParamValues, 0),
				   &uFilter);
  if (iResult != ASLEVEL_SUCCESS) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: ");
    aslevel_perror(pLogErr, iResult);
    LOG_ERR(LOG_LEVEL_SEVERE, "\n");
    return CLI_ERROR_COMMAND_FAILED;
  }
  
  // Filter topology
  iResult= aslevel_topo_filter(uFilter);
  if (iResult != ASLEVEL_SUCCESS) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: ");
    aslevel_perror(pLogErr, iResult);
    LOG_ERR(LOG_LEVEL_SEVERE, "\n");
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// -----[ cli_bgp_topology_policies ]--------------------------------
/**
 * context: {}
 * tokens: {}
 */
int cli_bgp_topology_policies(SCliContext * pContext, SCliCmd * pCmd)
{
  int iResult= aslevel_topo_policies();

  if (iResult != ASLEVEL_SUCCESS) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: ");
    aslevel_perror(pLogErr, iResult);
    LOG_ERR(LOG_LEVEL_SEVERE, "\n");
    return CLI_ERROR_COMMAND_FAILED;
  }
  return CLI_SUCCESS;
}

// -----[ cli_bgp_topology_info ]------------------------------------
/**
 * context: {}
 * tokens: {}
 */
int cli_bgp_topology_info(SCliContext * pContext, SCliCmd * pCmd)
{
  int iResult= aslevel_topo_info(pLogOut);

  if (iResult != ASLEVEL_SUCCESS) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: ");
    aslevel_perror(pLogErr, iResult);
    LOG_ERR(LOG_LEVEL_SEVERE, "\n");
    return CLI_ERROR_COMMAND_FAILED;
  }
  return CLI_SUCCESS;
}

// -----[ cli_bgp_topology_recordroute ]-----------------------------
/**
 * context: {}
 * tokens: {prefix, file-in, file-out}
 */
/*
int cli_bgp_topology_recordroute(SCliContext * pContext,
				 SCliCmd * pCmd)
{
  SPrefix sPrefix;
  char * pcPrefix;
  SLogStream * pOutput;
  int iResult= CLI_SUCCESS;
  char * pcOutput, * pcInput;

  pcPrefix= tokens_get_string_at(pCmd->pParamValues, 0);
  if (str2prefix(pcPrefix, &sPrefix) < 0)
    return CLI_ERROR_COMMAND_FAILED;

  pcOutput= tokens_get_string_at(pCmd->pParamValues, 2);
  if ((pOutput= log_create_file(pcOutput)) == NULL)
    return CLI_ERROR_COMMAND_FAILED;

  pcInput= tokens_get_string_at(pCmd->pParamValues, 1);
  if (aslevel_record_route(pOutput, pcInput, sPrefix) != ASLEVEL_SUCCESS)
    iResult= CLI_ERROR_COMMAND_FAILED;

  log_destroy(&pOutput);
  return iResult;
}
*/

// -----[ cli_bgp_topology_showrib ]---------------------------------
/**
 * context: {}
 * tokens: {}
 */
 /*
int cli_bgp_topology_showrib(SCliContext * pContext,
			     SCliCmd * pCmd)
{
  return CLI_ERROR_COMMAND_FAILED;
}
 */

// -----[ cli_bgp_topology_route_dp_rule ]---------------------------
/**
 * context: {}
 * tokens: {prefix, file-out}
 */
/*
int cli_bgp_topology_route_dp_rule(SCliContext * pContext,
				   SCliCmd * pCmd)
{
  SPrefix sPrefix;
  FILE * fOutput;
  int iResult= CLI_SUCCESS;

  if (str2prefix(tokens_get_string_at(pTokens, 0), &sPrefix) < 0)
    return CLI_ERROR_COMMAND_FAILED;
  if ((fOutput= fopen(tokens_get_string_at(pTokens, 1), "w")) == NULL)
    return CLI_ERROR_COMMAND_FAILED;
  if (rexford_route_dp_rule(fOutput, sPrefix))
    iResult= CLI_ERROR_COMMAND_FAILED;
  fclose(fOutput);
  return iResult;
}
*/

// -----[ cli_bgp_topology_install ]---------------------------------
/**
 * context: {}
 * tokens: {}
 */
int cli_bgp_topology_install(SCliContext * pContext, SCliCmd * pCmd)
{
  int iResult= aslevel_topo_install();

  if (iResult != ASLEVEL_SUCCESS) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: ");
    aslevel_perror(pLogErr, iResult);
    LOG_ERR(LOG_LEVEL_SEVERE, "\n");
    return CLI_ERROR_COMMAND_FAILED;
  }
  return CLI_SUCCESS;
}

// -----[ cli_bgp_topology_run ]-------------------------------------
/**
 * context: {}
 * tokens: {}
 */
int cli_bgp_topology_run(SCliContext * pContext, SCliCmd * pCmd)
{
  int iResult= aslevel_topo_run();

  if (iResult != ASLEVEL_SUCCESS) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: ");
    aslevel_perror(pLogErr, iResult);
    LOG_ERR(LOG_LEVEL_SEVERE, "\n");
    return CLI_ERROR_COMMAND_FAILED;
  }
  return CLI_SUCCESS;
}

// ----- cli_register_bgp_topology ----------------------------------
int cli_register_bgp_topology(SCliCmds * pCmds)
{
  SCliCmds * pSubCmds;
  SCliParams * pParams;
  SCliCmd * pCmd;

  pSubCmds= cli_cmds_create();
  pParams= cli_params_create();
  cli_params_add_file(pParams, "<file>", NULL);
  pCmd= cli_cmd_create("load", cli_bgp_topology_load, NULL, pParams);
  cli_cmd_add_option(pCmd, "addr-sch", NULL);
  cli_cmd_add_option(pCmd, "format", NULL);
  cli_cmds_add(pSubCmds, pCmd);
  pCmd= cli_cmd_create("check", cli_bgp_topology_check, NULL, NULL);
  cli_cmd_add_option(pCmd, "verbose", NULL);
  cli_cmds_add(pSubCmds, pCmd);
  pParams= cli_params_create();
  cli_params_add(pParams, "<what>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("filter",
					cli_bgp_topology_filter,
					NULL, pParams));
  cli_cmds_add(pSubCmds, cli_cmd_create("info",
					cli_bgp_topology_info,
					NULL, NULL));
  cli_cmds_add(pSubCmds, cli_cmd_create("install",
					cli_bgp_topology_install,
					NULL, NULL));
  cli_cmds_add(pSubCmds, cli_cmd_create("policies",
					cli_bgp_topology_policies,
					NULL, NULL));
  /*
  pParams= cli_params_create();
  cli_params_add(pParams, "<prefix>", NULL);
  cli_params_add(pParams, "<input>", NULL);
  cli_params_add(pParams, "<output>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("record-route",
					cli_bgp_topology_recordroute,
					NULL, pParams));
*/
  /*
  cli_cmds_add(pSubCmds, cli_cmd_create("show-rib",
					cli_bgp_topology_showrib,
					NULL, NULL));
  */
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
