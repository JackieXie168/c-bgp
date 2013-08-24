// ==================================================================
// @(#)filter_parser.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 11/08/2003
// @lastdate 12/03/2008
// ==================================================================

#ifndef __FILTER_PARSER_H__
#define __FILTER_PARSER_H__

#include <bgp/filter.h>

#define FILTER_PARSER_SUCCESS           0
#define FILTER_PARSER_ERROR_UNEXPECTED -1
#define FILTER_PARSER_ERROR_END        -2

#ifdef _cplusplus
extern "C" {
#endif

  // ----- filter_parser_rule ---------------------------------------
  int filter_parser_rule(char * pcRule, bgp_ft_rule_t ** ppRule);
  // ----- filter_parser_action -------------------------------------
  int filter_parser_action(char * pcAction,
			   bgp_ft_action_t ** ppAction);

#ifdef __cplusplus
}
#endif

#endif /* __FILTER_PARSER_H__ */
