// ==================================================================
// @(#)net_domain.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @author Stefano Iasi (stefanoia@tin.it)
// @date 29/07/2005
// @lastdate 10/10/2005
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <string.h>
#include <libgds/cli_ctx.h>
#include <net/igp_domain.h>
#include <assert.h>
#include <net/ospf_deflection.h>

// ----- cli_net_add_domain -----------------------------------------
/**
 * context: {}
 * tokens: {id, type}
 */
int cli_net_add_domain(SCliContext * pContext, STokens * pTokens)
{
  unsigned int uId;
  char * pcType;
  EDomainType tType;
  SIGPDomain * pDomain;

  /* Check domain id */
  if (tokens_get_uint_at(pTokens, 0, &uId) ||
      (uId > 65535)) {
    LOG_SEVERE("Error: invalid domain id %s\n",
	       tokens_get_string_at(pTokens, 0));
    return CLI_ERROR_COMMAND_FAILED;
  }
  
  /* Check domain type */
  pcType= tokens_get_string_at(pTokens, 1);
  if (!strcmp(pcType, "igp")) {
    tType= DOMAIN_IGP;
  } else if (!strcmp(pcType, "ospf")) {
#ifdef OSPF_SUPPORT 
    tType= DOMAIN_OSPF;
#else 
    LOG_SEVERE("To use OSPF model you must compile cbgp with --enable-ospf option\n");
    return CLI_ERROR_COMMAND_FAILED;
#endif    
  } else {
    LOG_SEVERE("Error: unknown domain type %s\n",
	       pcType);
    return CLI_ERROR_COMMAND_FAILED;
  }

  /* Check that a domain with the same Id does not exist */
  if (exists_igp_domain(uId)) {
    LOG_SEVERE("Error: domain %d already exists\n",
	       uId);
    return CLI_ERROR_COMMAND_FAILED;
  }

  /* Create and register domain */
  pDomain= igp_domain_create(uId, tType);
  register_igp_domain(pDomain);

  return CLI_SUCCESS;
}

// ----- cli_net_node_domain ----------------------------------------
/**
 * context: {node}
 * tokens: {addr, id}
 */
int cli_net_node_domain(SCliContext * pContext, STokens * pTokens)
{
  SNetNode * pNode;
  unsigned int uId;
  SIGPDomain * pDomain;

  // Get node from context
  pNode= (SNetNode *) cli_context_get_item_at_top(pContext);
  
  // Get domain id
  if (tokens_get_uint_at(pTokens, 1, &uId) ||
      (uId > 65535)) {
    LOG_SEVERE("Error: invalid domain id \"%s\"\n",
	       tokens_get_string_at(pTokens, 1));
    return CLI_ERROR_COMMAND_FAILED;
  }


  pDomain= get_igp_domain(uId);
  
  if (igp_domain_contains_router(pDomain, pNode)) {
    LOG_SEVERE("Error: could not add to domain \"%d\"\n",
	       uId);
    return CLI_ERROR_COMMAND_FAILED;
  }

  igp_domain_add_router(pDomain, pNode);
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
  SIGPDomain * pDomain;

  /* Get domain id */
  if (tokens_get_uint_at(pContext->pTokens, 0, &uId) ||
      (uId > 65535)) {
    LOG_SEVERE("Error: invalid domain id \"%s\"\n",
	       tokens_get_string_at(pContext->pTokens, 0));
    return CLI_ERROR_CTX_CREATE;
  }

  pDomain= get_igp_domain(uId);
  if (pDomain == NULL) {
    LOG_SEVERE("Error: unable to find domain \"%d\"\n", uId);
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
 * tokens: {id, state}
 */
int cli_net_domain_set_ecmp(SCliContext * pContext, STokens * pTokens)
{
  SIGPDomain * pDomain;
  int iState;

  // Get domain from context
  pDomain= cli_context_get_item_at_top(pContext);

  // Get the option's state ("yes"/"no")
  if (!strcmp(tokens_get_string_at(pTokens, 1), "yes")) {
    iState= 1;
  } else if (!strcmp(tokens_get_string_at(pTokens, 1), "no")) {
    iState= 0;
  } else {
    LOG_SEVERE("Error: invalid value for 'ecmp': \"%s\"\n",
	       tokens_get_string_at(pTokens, 1));
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Set state
  if (igp_domain_set_ecmp(pDomain, iState)) {
    LOG_SEVERE("Error: this model does not accept changes to 'ecmp'\n");
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// ----- cli_net_domain_show_info -----------------------------------
/**
 * context: {domain}
 * tokens: {}
 */
int cli_net_domain_show_info(SCliContext * pContext, STokens * pTokens)
{
  SIGPDomain * pDomain;

  // Get domain from context
  pDomain= cli_context_get_item_at_top(pContext);

  igp_domain_info(stdout, pDomain);
  return CLI_SUCCESS;
}

// ----- cli_net_domain_show_nodes ----------------------------------
/**
 * context: {domain}
 * tokens: {}
 */
int cli_net_domain_show_nodes(SCliContext * pContext, STokens * pTokens)
{
  SIGPDomain * pDomain;

  // Get domain from context
  pDomain= cli_context_get_item_at_top(pContext);

  igp_domain_dump(stdout, pDomain);
  return CLI_SUCCESS;
}

// ----- cli_net_domain_show_subnets --------------------------------
/**
 * context: {domain}
 * tokens: {}
 */
/*int cli_net_domain_show_subnets(SCliContext * pContext, STokens * pTokens)
{
  SIGPDomain * pDomain;

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
int cli_net_domain_compute(SCliContext * pContext, STokens * pTokens)
{
  SIGPDomain * pDomain;

  // Get domain from context
  pDomain= cli_context_get_item_at_top(pContext);

  igp_domain_compute(pDomain);
  return CLI_SUCCESS;
}

#ifdef OSPF_SUPPORT
// ----- cli_net_domain_check-deflection -------------------------------------
/**
 * context: {domain}
 * tokens: {}
 */
int cli_net_domain_check_deflection(SCliContext * pContext, STokens * pTokens)
{
  SIGPDomain * pDomain;

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
