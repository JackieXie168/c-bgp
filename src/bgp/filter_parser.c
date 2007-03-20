// ==================================================================
// @(#)filter_parser.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 11/08/2003
// @lastdate 09/01/2007
// ==================================================================
// Syntax:
//   Script     ::= Rules
//   Rules      ::= Rule [ ("\n" | ";") Rules ]
//   Rule       ::= [ Predicates ] ">" Actions
//   Predicates ::= Predicate [ "," Predicates ]
//   Actions    ::= Action [ "," Action ]

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <string.h>

#include <libgds/cli.h>
#include <libgds/cli_ctx.h>
#include <libgds/log.h>
#include <libgds/types.h>

#include <bgp/ecomm.h>
#include <bgp/ecomm_t.h>
#include <bgp/filter.h>
#include <bgp/filter_parser.h>
#include <bgp/filter_registry.h>

SCli * pMatcherCli= NULL;
SCli * pActionCli= NULL;

// ----- filter_parser_matcher_comm_contains ------------------------
/**
 * context: [&matcher] {}
 * tokens: {community}
 */
int filter_parser_matcher_comm_contains(SCliContext * pContext,
					SCliCmd * pCmd)
{
  return CLI_ERROR_COMMAND_FAILED;
}

// ----- filter_parser_matcher_cli ----------------------------------
/**
 *
 */
SCli * filter_parser_matcher_cli()
{
  SCliParams * pParams;
  SCliCmds * pSubCmds;

  if (pMatcherCli == NULL) {
    pSubCmds= cli_cmds_create();
    pParams= cli_params_create();
    cli_params_add(pParams, "<Community>", NULL);
    cli_cmds_add(pSubCmds, cli_cmd_create("contains",
					  filter_parser_matcher_comm_contains,
					  NULL, pParams));
    cli_register_cmd(pMatcherCli, cli_cmd_create("comm", NULL,
						 pSubCmds, NULL));
  }
  return pMatcherCli;
}

// ----- filter_parser_action ---------------------------------------
/**
 *
 */
int filter_parser_action(char * pcActions, SFilterAction ** ppAction)
{
  STokenizer * pTokenizer;
  STokens * pTokens;
  int iIndex;
  SFilterAction * pAction;
  SFilterAction ** ppNewAction= NULL;
  int iResult= 0;

  pTokenizer= tokenizer_create(",", 0, NULL, NULL);

  *ppAction= NULL;

  if (tokenizer_run(pTokenizer, pcActions) == 0) {
    pTokens= tokenizer_get_tokens(pTokenizer);
    for (iIndex= 0; iIndex < tokens_get_num(pTokens); iIndex++) {
      if (ft_registry_action_parse(tokens_get_string_at(pTokens, iIndex),
				   (void *) &pAction) == 0) {
	if (*ppAction == NULL) {
	  *ppAction= pAction;
	  ppNewAction= &(pAction->pNextAction);
	} else {
	  *ppNewAction= pAction;
	  ppNewAction= &(pAction->pNextAction);
	}
      } else {
	filter_action_destroy(ppAction);
	*ppAction= NULL;
	iResult= -1;
	break;
      }
    }
  } else {
    iResult= -1;
    *ppAction= NULL;
  }

  tokenizer_destroy(&pTokenizer);

  return iResult;
}

// ----- filter_parser_rule -----------------------------------------
/**
 *
 */
int filter_parser_rule(char * pcRule, SFilterRule ** ppRule)
{
  SFilterMatcher * pMatcher= NULL;
  SFilterAction * pAction= NULL;

  if (filter_parser_action(pcRule, &pAction) == FILTER_PARSER_SUCCESS)
    if (pAction != NULL) {
      *ppRule= filter_rule_create(pMatcher, pAction);
      return FILTER_PARSER_SUCCESS;
    }
  return FILTER_PARSER_ERROR_UNEXPECTED;
}

// ----- filter_parser_run ------------------------------------------
/**
 *
 */
/*
int filter_parser_run(char * pcScript, SFilter ** ppFilter)
{
  SFilter * pFilter= NULL;
  int iResult= FILTER_PARSER_SUCCESS;
  SFilterRule * pRule= NULL;
  STokenizer * pTokenizer= tokenizer_create(";\n", 0, NULL, NULL);
  STokens * pTokens;
  int iIndex;

  pFilter= filter_create();
  if (tokenizer_run(pTokenizer, pcScript) == 0) {
    pTokens= tokenizer_get_tokens(pTokenizer);
    for (iIndex= 0; iIndex < tokens_get_num(pTokens); iIndex++) {
      iResult= filter_parser_rule(tokens_get_string_at(pTokens, iIndex),
				  &pRule);
      if (iResult)
	break;
      if (pRule != NULL) {
	filter_add_rule2(pFilter, pRule);
	pRule= NULL;
      }
    }
  } else
    iResult= FILTER_PARSER_ERROR_UNEXPECTED;

  tokenizer_destroy(&pTokenizer);

  if (iResult != FILTER_PARSER_SUCCESS) {
    FREE(&pFilter);
    pFilter= NULL;
  }

  *ppFilter= pFilter;
  return iResult;
}
*/
