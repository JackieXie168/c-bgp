// ==================================================================
// @(#)net_domain.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @author Stefano Iasi (stefanoia@tin.it)
// @date 29/07/2005
// $Id: net_domain.c,v 1.10 2008-04-11 11:03:06 bqu Exp $
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <string.h>

#include <libgds/cli.h>
#include <libgds/cli_ctx.h>
#include <libgds/log.h>

#include <cli/common.h>
#include <net/igp_domain.h>
#include <net/ospf_deflection.h>

// ----- cli_net_add_domain -----------------------------------------
/**
 * context: {}
 * tokens: {id, type}
 */
int cli_net_add_domain(SCliContext * pContext, SCliCmd * pCmd)
{
  unsigned int uId;
  char * pcType;
  igp_domain_type_t tType;
  igp_domain_t* pDomain;

  /* Check domain id */
  if (tokens_get_uint_at(pCmd->pParamValues, 0, &uId) ||
      (uId > 65535)) {
    cli_set_user_error(cli_get(), "invalid domain id %s",
		       tokens_get_string_at(pCmd->pParamValues, 0));
    return CLI_ERROR_COMMAND_FAILED;
  }
  
  /* Check domain type */
  pcType= tokens_get_string_at(pCmd->pParamValues, 1);
  if (!strcmp(pcType, "igp")) {
    tType= IGP_DOMAIN_IGP;
  } else if (!strcmp(pcType, "ospf")) {
#ifdef OSPF_SUPPORT 
    tType= IGP_DOMAIN_OSPF;
#else 
    cli_set_user_error(cli_get(), "not compiled with OSPF support");
    return CLI_ERROR_COMMAND_FAILED;
#endif    
  } else {
    cli_set_user_error(cli_get(), "unknown domain type %s", pcType);
    return CLI_ERROR_COMMAND_FAILED;
  }

  /* Check that a domain with the same Id does not exist */
  if (exists_igp_domain(uId)) {
    cli_set_user_error(cli_get(), "domain %d already exists", uId);
    return CLI_ERROR_COMMAND_FAILED;
  }

  /* Create and register domain */
  pDomain= igp_domain_create(uId, tType);
  register_igp_domain(pDomain);

  return CLI_SUCCESS;
}

// ----- cli_ctx_create_net_domain ----------------------------------
/*
 * context: {}
 * tokens: {id}
 */
int cli_ctx_create_net_domain(SCliContext * pContext, void ** ppItem)
{
  unsigned int uId;
  igp_domain_t* pDomain;

  /* Get domain id */
  if (tokens_get_uint_at(pContext->pCmd->pParamValues, 0, &uId) ||
      (uId > 65535)) {
    cli_set_user_error(cli_get(), "invalid domain id \"%s\"",
		       tokens_get_string_at(pContext->pCmd->pParamValues, 0));
    return CLI_ERROR_CTX_CREATE;
  }

  pDomain= get_igp_domain(uId);
  if (pDomain == NULL) {
    cli_set_user_error(cli_get(), "unable to find domain \"%d\"", uId);
    return CLI_ERROR_CTX_CREATE;
  }
  *ppItem= pDomain;
  return CLI_SUCCESS;
}

// ----- cli_ctx_destroy_net_domain ---------------------------------
void cli_ctx_destroy_net_domain(void ** ppItem)
{
}

// ----- cli_net_domain_set_ecmp ------------------------------------
/**
 * context: {domain}
 * tokens: {ecmp-state}
 */
int cli_net_domain_set_ecmp(SCliContext * pContext, SCliCmd * pCmd)
{
  igp_domain_t* pDomain;
  char * pcState;
  int iState;

  // Get domain from context
  pDomain= cli_context_get_item_at_top(pContext);

  // Get the option's state ("yes"/"no")
  pcState= tokens_get_string_at(pCmd->pParamValues, 0);
  if (!strcmp(pcState, "yes")) {
    iState= 1;
  } else if (!strcmp(pcState, "no")) {
    iState= 0;
  } else {
    cli_set_user_error(cli_get(), "invalid value for 'ecmp': \"%s\"",
		       pcState);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Set state
  if (igp_domain_set_ecmp(pDomain, iState)) {
    cli_set_user_error(cli_get(), "ecmp options not supported");
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// ----- cli_net_domain_show_info -----------------------------------
/**
 * context: {domain}
 * tokens: {}
 */
int cli_net_domain_show_info(SCliContext * pContext, SCliCmd * pCmd)
{
  igp_domain_t* pDomain;

  // Get domain from context
  pDomain= cli_context_get_item_at_top(pContext);

  igp_domain_info(pLogOut, pDomain);
  return CLI_SUCCESS;
}

// ----- cli_net_domain_show_nodes ----------------------------------
/**
 * context: {domain}
 * tokens: {}
 */
int cli_net_domain_show_nodes(SCliContext * pContext, SCliCmd * pCmd)
{
  igp_domain_t* pDomain;

  // Get domain from context
  pDomain= cli_context_get_item_at_top(pContext);

  igp_domain_dump(pLogOut, pDomain);
  return CLI_SUCCESS;
}

// ----- cli_net_domain_show_subnets --------------------------------
/**
 * context: {domain}
 * tokens: {}
 */
/*int cli_net_domain_show_subnets(SCliContext * pContext, SCliCmd * pCmd)
{
  igp_domain_t* pDomain;

  // Get domain from context
  pDomain= cli_context_get_item_at_top(pContext);

  igp_domain_dump(stdout, pDomain);
  return CLI_SUCCESS;
  }*/

// ----- cli_net_domain_compute -------------------------------------
/**
 * context: {domain}
 * tokens: {}
 */
int cli_net_domain_compute(SCliContext * pContext, SCliCmd * pCmd)
{
  igp_domain_t* pDomain;

  // Get domain from context
  pDomain= cli_context_get_item_at_top(pContext);
  if (pDomain  == NULL)
    return CLI_ERROR_COMMAND_FAILED;

  if (igp_domain_compute(pDomain) != CLI_SUCCESS) {
    cli_set_user_error(cli_get(), "IGP routes computation failed.\n");
    return CLI_ERROR_COMMAND_FAILED;
  }
  return CLI_SUCCESS;
}

#ifdef OSPF_SUPPORT
// ----- cli_net_domain_check-deflection -------------------------------------
/**
 * context: {domain}
 * tokens: {}
 */
int cli_net_domain_check_deflection(SCliContext * pContext, SCliCmd * pCmd)
{
  igp_domain_t* pDomain;

  // Get domain from context
  pDomain= cli_context_get_item_at_top(pContext);

  ospf_domain_deflection(pDomain);
  return CLI_SUCCESS;
}
#endif

// ----- cli_register_net_domain_set --------------------------------
int cli_register_net_domain_set(SCliCmds * pCmds)
{
  SCliCmds * pSubCmds;
  SCliParams * pParams;

  pSubCmds= cli_cmds_create();
  pParams= cli_params_create();
  cli_params_add(pParams, "<state>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("ecmp", cli_net_domain_set_ecmp,
					NULL, pParams));
  return cli_cmds_add(pCmds, cli_cmd_create("set", NULL,
					    pSubCmds, NULL));
}

// ----- cli_register_net_domain_show -------------------------------
int cli_register_net_domain_show(SCliCmds * pCmds)
{
  SCliCmds * pSubCmds;

  pSubCmds= cli_cmds_create();
  cli_cmds_add(pSubCmds, cli_cmd_create("info", cli_net_domain_show_info,
					NULL, NULL));
  cli_cmds_add(pSubCmds, cli_cmd_create("nodes", cli_net_domain_show_nodes,
					NULL, NULL));
  /*  cli_cmds_add(pSubCmds, cli_cmd_create("subnets", cli_net_domain_show_subnets,
      NULL, NULL));*/
  return cli_cmds_add(pCmds, cli_cmd_create("show", NULL,
					    pSubCmds, NULL));
}

// ----- cli_register_net_domain ------------------------------------
int cli_register_net_domain(SCliCmds * pCmds)
{
  SCliCmds * pSubCmds;
  SCliParams * pParams;

  pSubCmds= cli_cmds_create();
  cli_cmds_add(pSubCmds, cli_cmd_create("compute", cli_net_domain_compute,
					NULL, NULL));
#ifdef OSPF_SUPPORT
  cli_cmds_add(pSubCmds, cli_cmd_create("check-deflection",
					cli_net_domain_check_deflection,
					NULL, NULL));
#endif
  cli_register_net_domain_set(pSubCmds);
  cli_register_net_domain_show(pSubCmds);
  pParams= cli_params_create();
  cli_params_add(pParams, "<id>", NULL);
  return cli_cmds_add(pCmds, cli_cmd_create_ctx("domain",
						cli_ctx_create_net_domain,
						cli_ctx_destroy_net_domain,
						pSubCmds, pParams));
}
