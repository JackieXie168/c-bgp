// ==================================================================
// @(#)filter_registry.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 01/03/2004
// @lastdate 12/03/2008
// ==================================================================

#ifndef __FILTER_REGISTRY_H__
#define __FILTER_REGISTRY_H__

#include <libgds/types.h>

#include <bgp/filter.h>

#define FILTER_PARSER_SUCCESS           0
#define FILTER_PARSER_ERROR_UNEXPECTED -1
#define FILTER_PARSER_ERROR_END        -2

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ ft_registry_predicate_parser ]---------------------------
  int ft_registry_predicate_parser(char * pcExpr,
				   bgp_ft_matcher_t ** ppMatcher);
  // -----[ ft_registry_action_parser ]------------------------------
  int ft_registry_action_parser(char * pcExpr,
				bgp_ft_action_t ** ppAction);

  // -----[ _ft_registry_init ]--------------------------------------
  void _ft_registry_init();
  // -----[ _ft_registry_destroy ]-----------------------------------
  void _ft_registry_destroy();

#ifdef __cplusplus
}
#endif

#endif
