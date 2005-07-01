// ==================================================================
// @(#)filter_registry.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @author Sebastien Tandel (standel@info.ucl.ac.be)
// @date 01/03/2004
// @lastdate 17/05/2005
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <libgds/cli.h>
#include <libgds/cli_ctx.h>
#include <libgds/hash.h>

#include <bgp/comm.h>
#include <bgp/ecomm.h>
#include <bgp/filter.h>
#include <bgp/filter_registry.h>
#include <bgp/route_map.h>
#include <util/regex.h>

#include <string.h>

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

// ----- ft_cli_predicate_nexthop_in --------------------------------
static int ft_cli_predicate_nexthop_in(SCliContext * pContext,
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

  *ppMatcher= filter_match_nexthop_in(sPrefix);

  return CLI_SUCCESS;
}

// ----- ft_cli_predicate_nexthop_is --------------------------------
static int ft_cli_predicate_nexthop_is(SCliContext * pContext,
				       STokens * pTokens)
{
  SFilterMatcher ** ppMatcher=
    (SFilterMatcher **) cli_context_get(pContext);
  char * pcNextHop;
  char * pcEndChar;
  net_addr_t tNextHop;

  // Get IP address
  pcNextHop= tokens_get_string_at(pTokens, 0);
  if (ip_string_to_address(pcNextHop, &pcEndChar, &tNextHop) ||
      (*pcEndChar != 0)) {
    LOG_SEVERE("Error: invalid next-hop \"%s\"\n", pcNextHop);
    return CLI_ERROR_COMMAND_FAILED;
  }

  *ppMatcher= filter_match_nexthop_equals(tNextHop);

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

extern SHash * pHashPathExpr;
extern SPtrArray * paPathExpr;
// ----- ft_cli_predicate_path ---------------------------------------
/**
 *
 *
 */
static int ft_cli_predicate_path (SCliContext * pContext, 
				    STokens * pTokens)
{
  SFilterMatcher ** ppMatcher=
    (SFilterMatcher **) cli_context_get(pContext);
  int iPos = 0;
  char * pcPattern;
  SPathMatch * pFilterRegEx, * pHashFilterRegEx;

  pcPattern= tokens_get_string_at(pTokens, 0);
  if (pcPattern == NULL) {
    LOG_SEVERE("Error: No Regular Expression.\n");
    return CLI_ERROR_COMMAND_FAILED;
  }
  
  pFilterRegEx = MALLOC(sizeof(SPathMatch));
  pFilterRegEx->pcPattern = MALLOC(strlen(pcPattern)+1);
  strcpy(pFilterRegEx->pcPattern, pcPattern);
  /* Try to find the expression in the hash table 
   * if not found, insert it in the hash table and in the array.
   * else the we have found the structure added in the hash table. 
   * This structure contains the position in the array.
   */
  if ( (pHashFilterRegEx = hash_search(pHashPathExpr, pFilterRegEx)) == NULL) {
    pHashFilterRegEx = pFilterRegEx;
    if ( (pHashFilterRegEx->pRegEx = regex_init(pcPattern, 20)) == NULL) {
      LOG_SEVERE("Error: Invalid Regular Expression : \"%s\"\n", pcPattern);
      return CLI_ERROR_COMMAND_FAILED;
    }
    if (hash_add(pHashPathExpr, pHashFilterRegEx) == -1) {
      LOG_SEVERE("Error: Could'nt insert the path reg exp in the hash table\n");
      return CLI_ERROR_COMMAND_FAILED;
    }
    if ( (iPos = ptr_array_add(paPathExpr, &pHashFilterRegEx)) == -1) {
      LOG_SEVERE("Error: Could not insert path reg exp in the array\n");
      return CLI_ERROR_COMMAND_FAILED;
    }
    pHashFilterRegEx->uArrayPos = iPos;
  } else {
    if (pFilterRegEx->pcPattern != NULL)
      FREE(pFilterRegEx->pcPattern);
    FREE(pFilterRegEx);
    
    iPos = pHashFilterRegEx->uArrayPos;
  }

  *ppMatcher= filter_match_path(iPos);

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

// ----- ft_cli_register_predicate_nexthop --------------------------
static void ft_cli_register_predicate_nexthop()
{
  SCli * pCli= ft_cli_predicate_get();
  SCliCmds * pSubCmds= cli_cmds_create();
  SCliParams * pParams;

  pParams= cli_params_create();
  cli_params_add(pParams, "<prefix>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("in",
					ft_cli_predicate_nexthop_in,
					NULL, pParams));
  pParams= cli_params_create();
  cli_params_add(pParams, "<next-hop>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("is",
					ft_cli_predicate_nexthop_is,
					NULL, pParams));
  cli_register_cmd(pCli, cli_cmd_create("next-hop", NULL,
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

// ----- ft_cli_register_predicate_path ------------------------------
static void ft_cli_register_predicate_path()
{
  SCli * pCli= ft_cli_predicate_get();
  SCliParams * pParams;

  pParams= cli_params_create();
  cli_params_add(pParams, "<path regular expression>", NULL);
  cli_register_cmd(pCli, cli_cmd_create("path", 
					    ft_cli_predicate_path, 
					    NULL, pParams));
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

// ----- ft_cli_action_jump -----------------------------------------
/**
 *
 */
int ft_cli_action_jump(SCliContext * pContext,
			STokens * pTokens)
{
  SFilterAction ** ppAction=
    (SFilterAction **) cli_context_get(pContext);
  char * pcRouteMapName;
  SFilter * pFilter;

  if ( (pcRouteMapName = tokens_get_string_at(pTokens, 0)) == NULL) {
    *ppAction= NULL;
    LOG_SEVERE("Error: No Route Map name.\n");
    return CLI_ERROR_COMMAND_FAILED;
  }

  if ( (pFilter = route_map_get(pcRouteMapName)) == NULL) {
    *ppAction= NULL;
    LOG_SEVERE("Error: No Route Map %s defined.\n", pcRouteMapName);
    return CLI_ERROR_COMMAND_FAILED;
  }


  *ppAction= filter_action_jump(pFilter);

  return CLI_SUCCESS;
}

// ----- ft_cli_action_call -----------------------------------------
/**
 *
 *
 */
int ft_cli_action_call(SCliContext * pContext,
			STokens * pTokens)
{
  SFilterAction ** ppAction=
    (SFilterAction **) cli_context_get(pContext);
  char * pcRouteMapName;
  SFilter * pFilter;

  if ( (pcRouteMapName = tokens_get_string_at(pTokens, 0)) == NULL) {
    *ppAction= NULL;
    LOG_SEVERE("Error: No Route Map name.\n");
    return CLI_ERROR_COMMAND_FAILED;
  }

  if ( (pFilter = route_map_get(pcRouteMapName)) == NULL) {
    *ppAction= NULL;
    LOG_SEVERE("Error: No Route Map %s defined.\n", pcRouteMapName);
    return CLI_ERROR_COMMAND_FAILED;
  }

  *ppAction= filter_action_call(pFilter);

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

// ----- ft_cli_action_metric ---------------------------------------
int ft_cli_action_metric(SCliContext * pContext,
			 STokens * pTokens)
{
  SFilterAction ** ppAction=
    (SFilterAction **) cli_context_get(pContext);
  unsigned long int ulMetric;

  if (!strcmp(tokens_get_string_at(pTokens, 0), "internal")) {
    *ppAction= filter_action_metric_internal();
  } else {
    if (tokens_get_ulong_at(pTokens, 0, &ulMetric)) {
      *ppAction= NULL;
      LOG_SEVERE("Error: invalid metric\n");
      return CLI_ERROR_COMMAND_FAILED;
    }

    *ppAction= filter_action_metric_set(ulMetric);
  }


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

// ----- ft_cli_action_community_remove -----------------------------
int ft_cli_action_community_remove(SCliContext * pContext,
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

  *ppAction= filter_action_comm_remove(tCommunity);

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

#ifdef __EXPERIMENTAL__
// ----- ft_cli_action_pref_comm ------------------------------------
int ft_cli_action_pref_comm(SCliContext * pContext,
			    STokens * pTokens)
{
  SFilterAction ** ppAction=
    (SFilterAction **) cli_context_get(pContext);
  unsigned int uPref;
  
  if (tokens_get_uint_at(pTokens, 0, &uPref)) {
    LOG_SEVERE("Error: invalid preference value\n");
    return CLI_ERROR_COMMAND_FAILED;
  }

  *ppAction=
    filter_action_ecomm_append(ecomm_pref_create(uPref));
  return CLI_SUCCESS;
}
#endif

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

// ----- ft_cli_register_action_jump ---------------------------------
/**
 *
 *
 */
void ft_cli_register_action_jump()
{
  SCli * pCli = ft_cli_action_get();
  SCliParams * pParams;

  pParams= cli_params_create();
  cli_params_add(pParams, "<route-map name>", NULL);
  cli_register_cmd(pCli, cli_cmd_create("jump", 
					ft_cli_action_jump,
					NULL, pParams));
}

// ----- ft_cli_register_action_call ---------------------------------
/**
 *
 *
 */
void ft_cli_register_action_call()
{
  SCli * pCli= ft_cli_action_get();
  SCliParams * pParams;

  pParams= cli_params_create();
  cli_params_add(pParams, "<route-map name>", NULL);
  cli_register_cmd(pCli, cli_cmd_create("call", 
					ft_cli_action_call,
					NULL, pParams));
}

// ----- ft_cli_register_action_metric ------------------------------
void ft_cli_register_action_metric()
{
  SCli * pCli= ft_cli_action_get();
  SCliParams * pParams;
  
  pParams= cli_params_create();
  cli_params_add(pParams, "<metric>", NULL);
  cli_register_cmd(pCli, cli_cmd_create("metric",
					ft_cli_action_metric,
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

  cli_cmds_add(pSubCmds, cli_cmd_create("strip",
					ft_cli_action_community_strip,
					NULL, NULL));
  pParams= cli_params_create();
  cli_params_add(pParams, "<community>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("add",
					ft_cli_action_community_add,
					NULL, pParams));
  pParams= cli_params_create();
  cli_params_add(pParams, "<community>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("remove",
					ft_cli_action_community_remove,
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

#ifdef __EXPERIMENTAL__
// ----- ft_cli_register_action_pref_community ----------------------
void ft_cli_register_action_pref_community()
{
  SCli * pCli= ft_cli_action_get();
  SCliCmds * pSubCmds= cli_cmds_create();
  SCliParams * pParams;
  
  pParams= cli_params_create();
  cli_params_add(pParams, "<pref>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("add", ft_cli_action_pref_comm,
					NULL, pParams));
  cli_register_cmd(pCli, cli_cmd_create("pref-community", NULL,
					pSubCmds, NULL));
}
#endif

// ----- ft_cli_register_action_ext_community -----------------------
void ft_cli_register_action_ext_community()
{
  SCli * pCli= ft_cli_action_get();
  SCliCmds * pSubCmds= cli_cmds_create();
  SCliCmds * pSubSubCmds= cli_cmds_create();

  cli_cmds_add(pSubCmds, cli_cmd_create("add", NULL,
					pSubSubCmds, NULL));
  cli_register_cmd(pCli, cli_cmd_create("ext-community", NULL,
					pSubCmds, NULL));
}

/////////////////////////////////////////////////////////////////////
// INITIALIZATION AND FINALIZATION SECTION
/////////////////////////////////////////////////////////////////////

// ----- _ft_registry_init --------------------------------------
/**
 *
 */
void _ft_registry_init()
{
  // Register predicates
  ft_cli_register_predicate_any();
  ft_cli_register_predicate_community();
  ft_cli_register_predicate_nexthop();
  ft_cli_register_predicate_prefix();
  ft_cli_register_predicate_path();

  // Register actions
  ft_cli_register_action_accept_deny();
  ft_cli_register_action_local_pref();
  ft_cli_register_action_metric();
  ft_cli_register_action_as_path();
  ft_cli_register_action_community();
  ft_cli_register_action_ext_community();
  ft_cli_register_action_red_community();
#ifdef __EXPERIMENTAL__
  ft_cli_register_action_pref_community();
#endif
  ft_cli_register_action_call();
  ft_cli_register_action_jump();
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
