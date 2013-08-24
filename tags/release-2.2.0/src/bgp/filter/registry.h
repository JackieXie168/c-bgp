// ==================================================================
// @(#)registry.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 01/03/2004
// $Id: registry.h,v 1.1 2009-03-24 13:42:45 bqu Exp $
// ==================================================================

#ifndef __BGP_FILTER_REGISTRY_H__
#define __BGP_FILTER_REGISTRY_H__

#include <libgds/cli.h>
#include <libgds/types.h>

#include <bgp/filter/types.h>

#define FILTER_PARSER_SUCCESS           0
#define FILTER_PARSER_ERROR_UNEXPECTED -1
#define FILTER_PARSER_ERROR_END        -2

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ ft_registry_strerror() ]---------------------------------
  void ft_registry_get_error(cli_error_t * error);
  // -----[ ft_registry_predicate_parser ]---------------------------
  int ft_registry_predicate_parser(const char * expr,
				   bgp_ft_matcher_t ** matcher);
  // -----[ ft_registry_action_parser ]------------------------------
  int ft_registry_action_parser(const char * expr,
				bgp_ft_action_t ** action);

  // -----[ _ft_registry_init ]--------------------------------------
  void _ft_registry_init();
  // -----[ _ft_registry_done ]--------------------------------------
  void _ft_registry_done();

#ifdef __cplusplus
}
#endif

#endif /* __BGP_FILTER_REGISTRY_H__ */
