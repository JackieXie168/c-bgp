// ==================================================================
// @(#)filter_registry.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 01/03/2004
// @lastdate 28/08/2007
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
				   SFilterMatcher ** ppMatcher);
  // -----[ ft_registry_action_parser ]------------------------------
  int ft_registry_action_parser(char * pcExpr,
				SFilterAction ** ppAction);

  // -----[ _ft_registry_init ]--------------------------------------
  void _ft_registry_init();
  // -----[ _ft_registry_destroy ]-----------------------------------
  void _ft_registry_destroy();

#ifdef __cplusplus
}
#endif

#endif
