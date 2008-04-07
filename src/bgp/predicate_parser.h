// ==================================================================
// @(#)predicate_parser.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 29/02/2004
// @lastdate 12/03/2008
// ==================================================================

#ifndef __PREDICATE_PARSER_H__
#define __PREDICATE_PARSER_H__

#include <libgds/log.h>
#include <bgp/filter.h>

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
  int predicate_parser(char * pcExpr, bgp_ft_matcher_t ** ppMatcher);
  // -----[ predicate_parser_perror ]--------------------------------
  void predicate_parser_perror(SLogStream * pStream, int iErrorCode);
  // -----[ predicate_parser_strerror ]------------------------------
  char * predicate_parser_strerror(int iErrorCode);

#ifdef __cplusplus
}
#endif

#endif /* __PREDICATE_PARSER_H__ */
