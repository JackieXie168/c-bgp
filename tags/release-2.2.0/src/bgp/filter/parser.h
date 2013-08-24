// ==================================================================
// @(#)parser.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 11/08/2003
// $Id: parser.h,v 1.1 2009-03-24 13:42:45 bqu Exp $
// ==================================================================

#ifndef __BGP_FILTER_PARSER_H__
#define __BGP_FILTER_PARSER_H__

#include <bgp/filter/types.h>

#define FILTER_PARSER_SUCCESS           0
#define FILTER_PARSER_ERROR_UNEXPECTED -1
#define FILTER_PARSER_ERROR_END        -2

#ifdef _cplusplus
extern "C" {
#endif

  // ----- filter_parser_rule ---------------------------------------
  int filter_parser_rule(const char * rule, bgp_ft_rule_t ** rule_ref);
  // ----- filter_parser_action -------------------------------------
  int filter_parser_action(const char * action,
			   bgp_ft_action_t ** action_ref);

#ifdef __cplusplus
}
#endif

#endif /* __BGP_FILTER_PARSER_H__ */
