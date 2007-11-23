// ==================================================================
// @(#)filter.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 27/11/2002
// @lastdate 23/11/2007
// ==================================================================

#ifndef __BGP_FILTER_H__
#define __BGP_FILTER_H__

#include <stdio.h>

#include <libgds/array.h>
#include <libgds/hash.h>
#include <libgds/log.h>

#include <bgp/as_t.h>
#include <bgp/comm_t.h>
#include <bgp/ecomm_t.h>
#include <bgp/filter_t.h>
#include <bgp/route_t.h>

#include <util/regex.h>

// Matcher codes
#define FT_MATCH_ANY            0
#define FT_MATCH_OP_AND         1
#define FT_MATCH_OP_OR          2
#define FT_MATCH_OP_NOT         3
#define FT_MATCH_COMM_CONTAINS  10
#define FT_MATCH_PATH_MATCHES   20
#define FT_MATCH_NEXTHOP_IS     25
#define FT_MATCH_NEXTHOP_IN     26
#define FT_MATCH_PREFIX_IS      30
#define FT_MATCH_PREFIX_IN      31

// Action codes
#define FT_ACTION_NOP              0
#define FT_ACTION_ACCEPT           1
#define FT_ACTION_DENY             2
#define FT_ACTION_COMM_APPEND      10
#define FT_ACTION_COMM_STRIP       11
#define FT_ACTION_COMM_REMOVE      12
#define FT_ACTION_PATH_APPEND      20
#define FT_ACTION_PATH_PREPEND     21
#define FT_ACTION_PATH_REM_PRIVATE 22
#define FT_ACTION_PREF_SET         30
#define FT_ACTION_ECOMM_APPEND     40
#define FT_ACTION_METRIC_SET       50
#define FT_ACTION_METRIC_INTERNAL  51
#define FT_ACTION_JUMP		   60
#define FT_ACTION_CALL		   61


#define FTM_AND(fm1, fm2) filter_match_and(fm1, fm2)
#define FTM_OR(fm1, fm2) filter_match_or(fm1, fm2)
#define FTM_NOT(fm) filter_match_not(fm)

#define FTA_ACCEPT filter_action_accept()
#define FTA_DENY filter_action_deny()

typedef struct {
  char * pcPattern;
  SRegEx * pRegEx;
  uint32_t uArrayPos;
} SPathMatch;

#ifdef __cplusplus
extern "C" {
#endif

  // ----- filter_rule_create ---------------------------------------
  SFilterRule * filter_rule_create(SFilterMatcher * pMatcher,
				   SFilterAction * pAction);
  // ----- filter_create --------------------------------------------
  SFilter * filter_create();
  // ----- filter_destroy -------------------------------------------
  void filter_destroy(SFilter ** ppFilter);
  // ----- filter_matcher_destroy -----------------------------------
  void filter_matcher_destroy(SFilterMatcher ** ppMatcher);
  // ----- filter_action_destroy ------------------------------------
  void filter_action_destroy(SFilterAction ** ppAction);
  // ----- filter_add_rule ------------------------------------------
  int filter_add_rule(SFilter * pFilter,
		      SFilterMatcher * pMatcher,
		      SFilterAction * pAction);
  // ----- filter_add_rule2 -----------------------------------------
  int filter_add_rule2(SFilter * pFilter,
		       SFilterRule * pRule);
  // ----- filter_insert_rule ---------------------------------------
  int filter_insert_rule(SFilter * pFilter, unsigned int uIndex,
			 SFilterRule * pRule);
  // ----- filter_remove_rule ---------------------------------------
  int filter_remove_rule(SFilter * pFilter, unsigned int uIndex);
  // ----- filter_apply ---------------------------------------------
  int filter_apply(SFilter * pFilter, SBGPRouter * pRouter,
		   SRoute * pRoute);
  // ----- filter_matcher_apply -------------------------------------
  int filter_matcher_apply(SFilterMatcher * pMatcher,
			   SBGPRouter * pRouter,
			   SRoute * pRoute);
  // ----- filter_action_apply --------------------------------------
  int filter_action_apply(SFilterAction * pAction,
			  SBGPRouter * pRouter,
			  SRoute * pRoute);
  // ----- filter_match_and -----------------------------------------
  SFilterMatcher * filter_match_and(SFilterMatcher * pMatcher1,
				    SFilterMatcher * pMatcher2);
  // ----- filter_match_or ------------------------------------------
  SFilterMatcher * filter_match_or(SFilterMatcher * pMatcher1,
				   SFilterMatcher * pMatcher2);
  // ----- filter_match_not -----------------------------------------
  SFilterMatcher * filter_match_not(SFilterMatcher * pMatcher);
  // ----- filter_match_comm_contains -------------------------------
  SFilterMatcher * filter_match_comm_contains(uint32_t uCommunity);
  // ----- filter_match_nexthop_equals ------------------------------
  SFilterMatcher * filter_match_nexthop_equals(net_addr_t tNextHop);
  // ----- filter_match_nexthop_in ----------------------------------
  SFilterMatcher * filter_match_nexthop_in(SPrefix sPrefix);
  // ----- filter_match_prefix_equals -------------------------------
  SFilterMatcher * filter_match_prefix_equals(SPrefix sPrefix);
  // ----- filter_match_prefix_in -----------------------------------
  SFilterMatcher * filter_match_prefix_in(SPrefix sPrefix);
  // ----- filter_math_path -----------------------------------------
  SFilterMatcher * filter_match_path(int iArrayPathRegExPos);
  // ----- filter_action_accept -------------------------------------
  SFilterAction * filter_action_accept();
  // ----- filter_action_deny ---------------------------------------
  SFilterAction * filter_action_deny();
  // ----- filter_action_jump ---------------------------------------
  SFilterAction * filter_action_jump(SFilter * pFilter);
  // ----- filter_action_call ---------------------------------------
  SFilterAction * filter_action_call(SFilter * pFilter);
  // ----- filter_action_pref_set -----------------------------------
  SFilterAction * filter_action_pref_set(uint32_t uPref);
  // ----- filter_action_metric_set ---------------------------------
  SFilterAction * filter_action_metric_set(uint32_t uMetric);
  // ----- filter_action_metric_internal ----------------------------
  SFilterAction * filter_action_metric_internal();
  // ----- filter_action_comm_append --------------------------------
  SFilterAction * filter_action_comm_append(comm_t uCommunity);
  // ----- filter_action_comm_remove --------------------------------
  SFilterAction * filter_action_comm_remove(comm_t uCommunity);
  // ----- filter_action_comm_strip ---------------------------------
  SFilterAction * filter_action_comm_strip();
  // ----- filter_action_ecomm_append -------------------------------
  SFilterAction * filter_action_ecomm_append(SECommunity * pComm);
  // ----- filter_action_path_prepend -------------------------------
  SFilterAction * filter_action_path_prepend(uint8_t uAmount);
  // ----- filter_action_path_rem_private ---------------------------
  SFilterAction * filter_action_path_rem_private();
  // ----- filter_dump ----------------------------------------------
  void filter_dump(SLogStream * pStream, SFilter * pFilter);
  // ----- filter_matcher_dump --------------------------------------
  void filter_matcher_dump(SLogStream * pStream,
			   SFilterMatcher * pMatcher);
  // ----- filter_action_dump ---------------------------------------
  void filter_action_dump(SLogStream * pStream,
			  SFilterAction * pAction);
  
  // ----- filter_path_regex_init -----------------------------------
  void _filter_path_regex_init();
  // ----- filter_path_regex_destroy --------------------------------
  void _filter_path_regex_destroy();

#ifdef __cplusplus
}
#endif

#endif /* __BGP_FILTER_H__ */
