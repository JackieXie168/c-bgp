// ==================================================================
// @(#)filter_registry.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 01/03/2004
// @lastdate 01/03/2004
// ==================================================================

#include <libgds/cli.h>
#include <libgds/cli_ctx.h>

#include <bgp/comm.h>
#include <bgp/ecomm.h>
#include <bgp/filter.h>
#include <bgp/filter_registry.h>

static SCli * pFtPredicateCLI= NULL;
static SCli * pFtActionCLI= NULL;

// ----- ft_registry_predicate_parse ----------------------------
/**
 * This function takes a string, parses it and if it matches the
 * definition of a predicate, creates and returns it.
 */
int ft_registry_predicate_parse(char * pcExpr,
				SFilterMatcher ** ppMatcher)
{
  int iResult;

  iResult= cli_execute_ctx(pFtPredicateCLI, pcExpr, (void *) ppMatcher);
  if (iResult != CLI_SUCCESS) {
    cli_perror(stderr, iResult);
    return FILTER_PARSER_ERROR_UNEXPECTED;
  }
  return FILTER_PARSER_SUCCESS;

  return -1;
}

// ----- ft_registry_action_parse -----------------------------------
/**
 * This function takes a string, parses it and if it matches the
 * definition of an action, creates and returns it.
 */
int ft_registry_action_parse(char * pcExpr,
			     SFilterAction ** ppAction)
{
  int iResult;

  iResult= cli_execute_ctx(pFtActionCLI, pcExpr, (void *) ppAction);
  if (iResult != CLI_SUCCESS) {
    cli_perror(stderr, iResult);
    return FILTER_PARSER_ERROR_UNEXPECTED;
  }
  return FILTER_PARSER_SUCCESS;

  return -1;
}

// ----- ft_cli_predicate_get ---------------------------------------
static SCli * ft_cli_predicate_get()
{
  if (pFtPredicateCLI == NULL)
    pFtPredicateCLI= cli_create();
  return pFtPredicateCLI;
}

// ----- ft_cli_action_get ------------------------------------------
static SCli * ft_cli_action_get()
{
  if (pFtActionCLI == NULL)
    pFtActionCLI= cli_create();
  return pFtActionCLI;
}

/////////////////////////////////////////////////////////////////////
// CLI PREDICATE COMMANDS
/////////////////////////////////////////////////////////////////////

// ----- ft_cli_predicate_any ---------------------------------------
static int ft_cli_predicate_any(SCliContext * pContext,
				STokens * pTokens)
{
  return CLI_SUCCESS;
}

// ----- ft_cli_predicate_community_is ------------------------------
static int ft_cli_predicate_community_is(SCliContext * pContext,
					 STokens * pTokens)
{
  SFilterMatcher ** ppMatcher=
    (SFilterMatcher **) cli_context_get(pContext);
  char * pcCommunity;
  comm_t tCommunity;

  // Get community
  pcCommunity= tokens_get_string_at(pTokens, 0);
  if (comm_from_string(pcCommunity, &tCommunity)) {
    LOG_SEVERE("Error: invalid community \"%s\"\n", pcCommunity);
    return CLI_ERROR_COMMAND_FAILED;
  }

  *ppMatcher= filter_match_comm_contains(tCommunity);

  return CLI_SUCCESS;
}

// ----- ft_cli_predicate_prefix_is ---------------------------------
static int ft_cli_predicate_prefix_is(SCliContext * pContext,
				      STokens * pTokens)
{
  SFilterMatcher ** ppMatcher=
    (SFilterMatcher **) cli_context_get(pContext);
  char * pcPrefix;
  char * pcEndChar;
  SPrefix sPrefix;
  
  // Get prefix
  pcPrefix= tokens_get_string_at(pTokens, 0);
  if (ip_string_to_prefix(pcPrefix, &pcEndChar, &sPrefix) ||
      (*pcEndChar != 0)) {
    LOG_SEVERE("Error: invalid prefix \"%s\"\n", pcPrefix);
    return CLI_ERROR_COMMAND_FAILED;
  }

  *ppMatcher= filter_match_prefix_equals(sPrefix);

  return CLI_SUCCESS;
}

// ----- ft_cli_predicate_prefix_in ---------------------------------
static int ft_cli_predicate_prefix_in(SCliContext * pContext,
				      STokens * pTokens)
{
  SFilterMatcher ** ppMatcher=
    (SFilterMatcher **) cli_context_get(pContext);
  char * pcPrefix;
  char * pcEndChar;
  SPrefix sPrefix;
  
  // Get prefix
  pcPrefix= tokens_get_string_at(pTokens, 0);
  if (ip_string_to_prefix(pcPrefix, &pcEndChar, &sPrefix) ||
      (*pcEndChar != 0)) {
    LOG_SEVERE("Error: invalid prefix \"%s\"\n", pcPrefix);
    return CLI_ERROR_COMMAND_FAILED;
  }

  *ppMatcher= filter_match_prefix_in(sPrefix);

  return CLI_SUCCESS;
}

/////////////////////////////////////////////////////////////////////
// CLI PREDICATE REGISTRATION
/////////////////////////////////////////////////////////////////////

// ----- ft_cli_register_predicate_any ------------------------------
static void ft_cli_register_predicate_any()
{
  cli_register_cmd(ft_cli_predicate_get(),
		   cli_cmd_create("any", ft_cli_predicate_any,
				  NULL, NULL));
}

// ----- ft_cli_register_predicate_community ------------------------
static void ft_cli_register_predicate_community()
{
  SCli * pCli= ft_cli_predicate_get();
  SCliCmds * pSubCmds= cli_cmds_create();
  SCliParams * pParams;

  pParams= cli_params_create();
  cli_params_add(pParams, "<community>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("is",
					ft_cli_predicate_community_is,
					NULL, pParams));
  cli_register_cmd(pCli, cli_cmd_create("community", NULL,
					pSubCmds, NULL));
}

// ----- ft_cli_register_predicate_prefix ---------------------------
static void ft_cli_register_predicate_prefix()
{
  SCli * pCli= ft_cli_predicate_get();
  SCliCmds * pSubCmds= cli_cmds_create();
  SCliParams * pParams;

  pParams= cli_params_create();
  cli_params_add(pParams, "<prefix>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("is",
					ft_cli_predicate_prefix_is,
					NULL, pParams));
  pParams= cli_params_create();
  cli_params_add(pParams, "<prefix>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("in",
					ft_cli_predicate_prefix_in,
					NULL, pParams));
  cli_register_cmd(pCli, cli_cmd_create("prefix", NULL,
					pSubCmds, NULL));
}

/////////////////////////////////////////////////////////////////////
// CLI ACTION COMMANDS
/////////////////////////////////////////////////////////////////////

// ----- ft_cli_action_accept ---------------------------------------
int ft_cli_action_accept(SCliContext * pContext,
			 STokens * pTokens)
{
  SFilterAction ** ppAction=
    (SFilterAction **) cli_context_get(pContext);

  *ppAction= filter_action_accept();

  return CLI_SUCCESS;
}

// ----- ft_cli_action_deny -----------------------------------------
int ft_cli_action_deny(SCliContext * pContext,
		       STokens * pTokens)
{
  SFilterAction ** ppAction=
    (SFilterAction **) cli_context_get(pContext);

  *ppAction= filter_action_deny();

  return CLI_SUCCESS;
}

// ----- ft_cli_action_local_pref -----------------------------------
int ft_cli_action_local_pref(SCliContext * pContext,
			     STokens * pTokens)
{
  SFilterAction ** ppAction=
    (SFilterAction **) cli_context_get(pContext);
  unsigned long int ulPref;

  if (tokens_get_ulong_at(pTokens, 0, &ulPref)) {
    *ppAction= NULL;
    LOG_SEVERE("Error: invalid local-pref\n");
    return CLI_ERROR_COMMAND_FAILED;
  }

  *ppAction= filter_action_pref_set(ulPref);

  return CLI_SUCCESS;
}

// ----- ft_cli_action_as_path_prepend ------------------------------
int ft_cli_action_as_path_prepend(SCliContext * pContext,
				  STokens * pTokens)
{
  SFilterAction ** ppAction=
    (SFilterAction **) cli_context_get(pContext);
  unsigned long int ulAmount;

  if (tokens_get_ulong_at(pTokens, 0, &ulAmount)) {
    *ppAction= NULL;
    LOG_SEVERE("Error: invalid prepending amount\n");
    return CLI_ERROR_COMMAND_FAILED;
  }

  *ppAction= filter_action_path_prepend(ulAmount);

   return CLI_SUCCESS;
}

// ----- ft_cli_action_community_add --------------------------------
int ft_cli_action_community_add(SCliContext * pContext,
				STokens * pTokens)
{
  SFilterAction ** ppAction=
    (SFilterAction **) cli_context_get(pContext);
  char * pcCommunity;
  comm_t tCommunity;

  pcCommunity= tokens_get_string_at(pTokens, 0);
  if (comm_from_string(pcCommunity, &tCommunity)) {
    *ppAction= NULL;
    LOG_SEVERE("Error: invalid community \"%s\"\n", pcCommunity);
    return CLI_ERROR_COMMAND_FAILED;
  }

  *ppAction= filter_action_comm_append(tCommunity);

  return CLI_SUCCESS;
}

// ----- ft_cli_action_community_strip ------------------------------
int ft_cli_action_community_strip(SCliContext * pContext,
				  STokens * pTokens)
{
  SFilterAction ** ppAction=
    (SFilterAction **) cli_context_get(pContext);

  *ppAction= filter_action_comm_strip();

  return CLI_SUCCESS;
}

// ----- ft_cli_action_red_comm_ignore ------------------------------
int ft_cli_action_red_comm_ignore(SCliContext * pContext,
				  STokens * pTokens)
{
  SFilterAction ** ppAction=
    (SFilterAction **) cli_context_get(pContext);
  unsigned int uTarget;

  if (tokens_get_uint_at(pTokens, 0, &uTarget)) {
    LOG_SEVERE("Error: invalid target\n");
    return CLI_ERROR_COMMAND_FAILED;
  }

  *ppAction=
    filter_action_ecomm_append(ecomm_red_create_as(ECOMM_RED_ACTION_IGNORE,
						   0, uTarget));
  return CLI_SUCCESS;
}

// ----- ft_cli_action_red_comm_prepend -----------------------------
int ft_cli_action_red_comm_prepend(SCliContext * pContext,
				   STokens * pTokens)
{
  SFilterAction ** ppAction=
    (SFilterAction **) cli_context_get(pContext);
  unsigned int uAmount;
  unsigned int uTarget;
  
  if (tokens_get_uint_at(pTokens, 0, &uAmount)) {
    LOG_SEVERE("Error: invalid prepending amount\n");
    return CLI_ERROR_COMMAND_FAILED;
  }

  if (tokens_get_uint_at(pTokens, 1, &uTarget)) {
    LOG_SEVERE("Error: invalid target\n");
    return CLI_ERROR_COMMAND_FAILED;
  }

  *ppAction=
    filter_action_ecomm_append(ecomm_red_create_as(ECOMM_RED_ACTION_PREPEND,
						   uAmount, uTarget));
  return CLI_SUCCESS;
}

/////////////////////////////////////////////////////////////////////
// CLI ACTION REGISTRATION
/////////////////////////////////////////////////////////////////////

// ----- ft_cli_register_action_accept_deny -------------------------
void ft_cli_register_action_accept_deny()
{
  SCli * pCli= ft_cli_action_get();

  cli_register_cmd(pCli, cli_cmd_create("accept",
					ft_cli_action_accept,
					NULL, NULL));
  cli_register_cmd(pCli, cli_cmd_create("deny",
					ft_cli_action_deny,
					NULL, NULL));
}

// ----- ft_cli_register_action_local_pref --------------------------
void ft_cli_register_action_local_pref()
{
  SCli * pCli= ft_cli_action_get();
  SCliParams * pParams;
  
  pParams= cli_params_create();
  cli_params_add(pParams, "<preference>", NULL);
  cli_register_cmd(pCli, cli_cmd_create("local-pref",
					ft_cli_action_local_pref,
					NULL, pParams));
}

// ----- ft_cli_register_action_as_path -----------------------------
void ft_cli_register_action_as_path()
{
  SCli * pCli= ft_cli_action_get();
  SCliCmds * pSubCmds= cli_cmds_create();
  SCliParams * pParams;

  pParams= cli_params_create();
  cli_params_add(pParams, "<amount>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("prepend",
					ft_cli_action_as_path_prepend,
					NULL, pParams));
  cli_register_cmd(pCli, cli_cmd_create("as-path", NULL, pSubCmds, NULL));
}

// ----- ft_cli_register_action_community ---------------------------
void ft_cli_register_action_community()
{
  SCli * pCli= ft_cli_action_get();
  SCliCmds * pSubCmds= cli_cmds_create();
  SCliParams * pParams;

  pParams= cli_params_create();
  cli_params_add(pParams, "<community>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("add",
					ft_cli_action_community_add,
					NULL, pParams));
  cli_register_cmd(pCli, cli_cmd_create("community", NULL,
					pSubCmds, NULL));
}

// ----- ft_cli_register_action_red_community -----------------------
void ft_cli_register_action_red_community()
{
  SCli * pCli= ft_cli_action_get();
  SCliCmds * pSubCmds= cli_cmds_create();
  SCliCmds * pSubSubCmds= cli_cmds_create();
  SCliParams * pParams;

  pParams= cli_params_create();
  cli_params_add(pParams, "<as-num|address>", NULL);
  cli_cmds_add(pSubSubCmds, cli_cmd_create("ignore",
					   ft_cli_action_red_comm_ignore,
					   NULL, pParams));
  pParams= cli_params_create();
  cli_params_add(pParams, "<amount>", NULL);
  cli_params_add(pParams, "<as-num|address>", NULL);
  cli_cmds_add(pSubSubCmds, cli_cmd_create("prepend",
					   ft_cli_action_red_comm_prepend,
					   NULL, pParams));
  cli_cmds_add(pSubCmds, cli_cmd_create("add", NULL,
					pSubSubCmds, NULL));
  cli_register_cmd(pCli, cli_cmd_create("red-community", NULL,
					pSubCmds, NULL));
}

/////////////////////////////////////////////////////////////////////
// INITIALIZATION AND FINALIZATION SECTION
/////////////////////////////////////////////////////////////////////

void _ft_registry_init() __attribute__((constructor));
void _ft_registry_destroy() __attribute__((destructor));

// ----- _ft_registry_init --------------------------------------
/**
 *
 */
void _ft_registry_init()
{
  // Register predicates
  ft_cli_register_predicate_any();
  ft_cli_register_predicate_community();
  ft_cli_register_predicate_prefix();

  // Register actions
  ft_cli_register_action_accept_deny();
  ft_cli_register_action_local_pref();
  ft_cli_register_action_as_path();
  ft_cli_register_action_community();
  ft_cli_register_action_red_community();
}

// ----- _ft_registry_destroy -----------------------------------
/**
 *
 */
void _ft_registry_destroy()
{
  cli_destroy(&pFtPredicateCLI);
  cli_destroy(&pFtActionCLI);
}
