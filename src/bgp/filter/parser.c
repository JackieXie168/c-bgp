// ==================================================================
// @(#)parser.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 11/08/2003
// $Id: parser.c,v 1.1 2009-03-24 13:42:45 bqu Exp $
// ==================================================================
// Syntax:
//   Script     ::= Rules
//   Rules      ::= Rule [ ("\n" | ";") Rules ]
//   Rule       ::= [ Predicates ] ">" Actions
//   Predicates ::= Predicate [ "," Predicates ]
//   Actions    ::= Action [ "," Action ]
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <string.h>

#include <libgds/cli.h>
#include <libgds/cli_ctx.h>
#include <libgds/stream.h>
#include <libgds/types.h>

#include <bgp/filter/filter.h>
#include <bgp/filter/types.h>
#include <bgp/filter/parser.h>
#include <bgp/filter/registry.h>

// ----- filter_parser_action ---------------------------------------
int filter_parser_action(const char * actions, bgp_ft_action_t ** action_ref)
{
  gds_tokenizer_t * tokenizer;
  const gds_tokens_t * tokens;
  int index;
  bgp_ft_action_t * action;
  bgp_ft_action_t ** ppNewAction= NULL;
  int result= 0;

  tokenizer= tokenizer_create(",", NULL, NULL);

  *action_ref= NULL;

  if (tokenizer_run(tokenizer, actions) == 0) {
    tokens= tokenizer_get_tokens(tokenizer);
    for (index= 0; index < tokens_get_num(tokens); index++) {
      if (ft_registry_action_parser(tokens_get_string_at(tokens, index),
				    (void *) &action) == 0) {
	if (*action_ref == NULL) {
	  *action_ref= action;
	  ppNewAction= &(action->next_action);
	} else {
	  *ppNewAction= action;
	  ppNewAction= &(action->next_action);
	}
      } else {
	filter_action_destroy(action_ref);
	*action_ref= NULL;
	result= -1;
	break;
      }
    }
  } else {
    result= -1;
    *action_ref= NULL;
  }

  tokenizer_destroy(&tokenizer);

  return result;
}

// ----- filter_parser_rule -----------------------------------------
int filter_parser_rule(const char * rule, bgp_ft_rule_t ** rule_ref)
{
  bgp_ft_matcher_t * matcher= NULL;
  bgp_ft_action_t * action= NULL;

  if (filter_parser_action(rule, &action) == FILTER_PARSER_SUCCESS)
    if (action != NULL) {
      *rule_ref= filter_rule_create(matcher, action);
      return FILTER_PARSER_SUCCESS;
    }
  return FILTER_PARSER_ERROR_UNEXPECTED;
}

