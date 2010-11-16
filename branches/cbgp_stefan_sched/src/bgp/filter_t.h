// ==================================================================
// @(#)filter_t.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 27/11/2002
// $Id: filter_t.h,v 1.6 2008-06-11 15:14:52 bqu Exp $
// ==================================================================

#ifndef __FILTER_T_H__
#define __FILTER_T_H__

#include <libgds/sequence.h>

typedef enum {
  FILTER_IN,
  FILTER_OUT,
  FILTER_MAX
} bgp_filter_dir_t;

// Matcher codes
typedef enum {
  FT_MATCH_ANY,
  FT_MATCH_OP_AND,
  FT_MATCH_OP_OR,
  FT_MATCH_OP_NOT,
  FT_MATCH_COMM_CONTAINS,
  FT_MATCH_PATH_MATCHES,
  FT_MATCH_NEXTHOP_IS,
  FT_MATCH_NEXTHOP_IN,
  FT_MATCH_PREFIX_IS,
  FT_MATCH_PREFIX_IN,
  FT_MATCH_PREFIX_GE,
  FT_MATCH_PREFIX_LE,
} bgp_ft_matcher_code_t;

// Action codes
typedef enum {
  FT_ACTION_NOP,
  FT_ACTION_ACCEPT,
  FT_ACTION_DENY,
  FT_ACTION_COMM_APPEND,
  FT_ACTION_COMM_STRIP,
  FT_ACTION_COMM_REMOVE,
  FT_ACTION_PATH_APPEND,
  FT_ACTION_PATH_PREPEND,
  FT_ACTION_PATH_REM_PRIVATE,
  FT_ACTION_PREF_SET,
  FT_ACTION_ECOMM_APPEND,
  FT_ACTION_METRIC_SET,
  FT_ACTION_METRIC_INTERNAL,
  FT_ACTION_JUMP,
  FT_ACTION_CALL,
} bgp_ft_action_code_t;

/* Maximum size of the parameter field of a filter matcher */
#define BGP_FT_MATCHER_SIZE_MAX 255

/** Warning: the following structures must be "packed" as we might
 *           access their fields in a non-aligned manner.
 * We use the GCC specific __attribute__((packed)) construct. */
typedef struct {
  bgp_ft_matcher_code_t uCode;
  unsigned char         uSize;
  unsigned char         auParams[0];
} __attribute__((packed)) bgp_ft_matcher_t ;

typedef struct bgp_ft_action_t {
  bgp_ft_action_code_t     uCode;
  unsigned char            uSize;
  struct bgp_ft_action_t * pNextAction;
  unsigned char            auParams[0];
} __attribute__((packed)) bgp_ft_action_t;

typedef struct {
  bgp_ft_matcher_t * pMatcher;
  bgp_ft_action_t * pAction;
} bgp_ft_rule_t;

typedef struct {
  SSequence * pSeqRules;
} bgp_filter_t;

#endif
