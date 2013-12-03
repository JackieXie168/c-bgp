// ==================================================================
// @(#)predicate_parser.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 29/02/2004
// $Id: predicate_parser.h,v 1.1 2009-03-24 13:42:45 bqu Exp $
// ==================================================================

#ifndef __BGP_FILTER_PREDICATE_PARSER_H__
#define __BGP_FILTER_PREDICATE_PARSER_H__

#include <libgds/stream.h>
#include <bgp/filter/types.h>

#define PREDICATE_PARSER_END                     1
#define PREDICATE_PARSER_SUCCESS                 0
#define PREDICATE_PARSER_ERROR_UNEXPECTED        -1
#define PREDICATE_PARSER_ERROR_UNFINISHED_STRING -2
#define PREDICATE_PARSER_ERROR_ATOM              -3
#define PREDICATE_PARSER_ERROR_UNEXPECTED_EOF    -4
#define PREDICATE_PARSER_ERROR_LEFT_EXPR         -5
#define PREDICATE_PARSER_ERROR_PAR_MISMATCH      -6
#define PREDICATE_PARSER_ERROR_STATE_TOO_LARGE   -7
#define PREDICATE_PARSER_ERROR_UNARY_OP          -8

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ predicate_parser ]---------------------------------------
  int predicate_parser(const char * expr, bgp_ft_matcher_t ** matcher_ref);
  // -----[ predicate_parser_perror ]--------------------------------
  void predicate_parser_perror(gds_stream_t * stream, int error);
  // -----[ predicate_parser_strerror ]------------------------------
  char * predicate_parser_strerror(int error);

#ifdef __cplusplus
}
#endif

#endif /* __BGP_FILTER_PREDICATE_PARSER_H__ */
