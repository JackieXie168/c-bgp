// ==================================================================
// @(#)filter_parser.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 11/08/2003
// @lastdate 12/03/2008
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

// ----- filter_parser_action ---------------------------------------
/**
 *
 */
int filter_parser_action(char * pcActions, bgp_ft_action_t ** ppAction)
{
  STokenizer * pTokenizer;
  STokens * pTokens;
  int iIndex;
  bgp_ft_action_t * pAction;
  bgp_ft_action_t ** ppNewAction= NULL;
  int iResult= 0;

  pTokenizer= tokenizer_create(",", 0, NULL, NULL);

  *ppAction= NULL;

  if (tokenizer_run(pTokenizer, pcActions) == 0) {
    pTokens= tokenizer_get_tokens(pTokenizer);
    for (iIndex= 0; iIndex < tokens_get_num(pTokens); iIndex++) {
      if (ft_registry_action_parser(tokens_get_string_at(pTokens, iIndex),
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
int filter_parser_rule(char * pcRule, bgp_ft_rule_t ** ppRule)
{
  bgp_ft_matcher_t * pMatcher= NULL;
  bgp_ft_action_t * pAction= NULL;

  if (filter_parser_action(pcRule, &pAction) == FILTER_PARSER_SUCCESS)
    if (pAction != NULL) {
      *ppRule= filter_rule_create(pMatcher, pAction);
      return FILTER_PARSER_SUCCESS;
    }
  return FILTER_PARSER_ERROR_UNEXPECTED;
}

