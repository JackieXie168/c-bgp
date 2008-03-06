// ==================================================================
// @(#)bgp_filter.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be),
// @date 27/02/2008
// @lastdate 27/02/2008
// ==================================================================

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include <libgds/cli_ctx.h>

#include <bgp/filter.h>
#include <bgp/filter_parser.h>
#include <bgp/predicate_parser.h>
#include <cli/common.h>

static inline SFilterRule * _rule_from_context(SCliContext * pContext) {
  SFilterRule * pRule= (SFilterRule *) cli_context_get_item_at_top(pContext);
  assert(pRule != NULL);
  return pRule;
}

static inline SFilter ** _filter_from_context(SCliContext * pContext) {
  SFilter ** ppFilter= (SFilter **) cli_context_get_item_at_top(pContext);
  assert(ppFilter != NULL);
  return ppFilter;
}

// -----[ cli_bgp_filter_rule_match ]--------------------------------
/**
 * context: {?, rule}
 * tokens: {?, predicate}
 */
static int cli_bgp_filter_rule_match(SCliContext * pContext,
				     SCliCmd * pCmd)
{
  SFilterRule * pRule= _rule_from_context(pContext);
  char * pcPredicate;
  SFilterMatcher * pMatcher;
  int iResult;

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

// -----[ cli_bgp_filter_rule_action ]-------------------------------
/**
 * context: {?, rule}
 * tokens: {?, action}
 */
static int cli_bgp_filter_rule_action(SCliContext * pContext,
				      SCliCmd * pCmd)
{
  SFilterRule * pRule= _rule_from_context(pContext);
  char * pcAction;
  SFilterAction * pAction;

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
  SFilter ** ppFilter= _filter_from_context(pContext);
  SFilterRule * pRule;

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
  SFilter ** ppFilter= _filter_from_context(pContext);
  SFilterRule * pRule;
  unsigned int uIndex;

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
  SFilter ** ppFilter= _filter_from_context(pContext);
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
  SFilter ** ppFilter= _filter_from_context(pContext);

  filter_dump(pLogOut, *ppFilter);

  return CLI_SUCCESS;
}

// -----[ cli_register_bgp_filter_rule ]-----------------------------
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
