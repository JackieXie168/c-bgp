// ==================================================================
// @(#)filter.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 27/11/2002
// @lastdate 01/03/2004
// ==================================================================

#ifndef __FILTER_H__
#define __FILTER_H__

#include <stdio.h>

#include <libgds/sequence.h>

#include <bgp/as_t.h>
#include <bgp/comm_t.h>
#include <bgp/ecomm_t.h>
#include <bgp/filter_t.h>
#include <bgp/route_t.h>

#define FTM_AND(fm1, fm2) filter_match_and(fm1, fm2)
#define FTM_OR(fm1, fm2) filter_match_or(fm1, fm2)
#define FTM_NOT(fm) filter_match_not(fm)

#define FTA_ACCEPT filter_action_accept()
#define FTA_DENY filter_action_deny()

// ----- filter_rule_create -----------------------------------------
extern SFilterRule * filter_rule_create(SFilterMatcher * pMatcher,
					SFilterAction * pAction);
// ----- filter_create ----------------------------------------------
extern SFilter * filter_create();
// ----- filter_destroy ---------------------------------------------
extern void filter_destroy(SFilter ** ppFilter);
// ----- filter_matcher_destroy -------------------------------------
extern void filter_matcher_destroy(SFilterMatcher ** ppMatcher);
// ----- filter_action_destroy --------------------------------------
extern void filter_action_destroy(SFilterAction ** ppAction);
// ----- filter_add_rule --------------------------------------------
extern int filter_add_rule(SFilter * pFilter,
			   SFilterMatcher * pMatcher,
			   SFilterAction * pAction);
// ----- filter_add_rule2 -------------------------------------------
extern int filter_add_rule2(SFilter * pFilter,
			    SFilterRule * pRule);
// ----- filter_insert_rule -----------------------------------------
extern int filter_insert_rule(SFilter * pFilter, unsigned int uIndex,
			      SFilterRule * pRule);
// ----- filter_remove_rule -----------------------------------------
extern int filter_remove_rule(SFilter * pFilter, unsigned int uIndex);
// ----- filter_apply -----------------------------------------------
extern int filter_apply(SFilter * pFilter, SAS * pAS,
			SRoute * pRoute);
// ----- filter_match_and -------------------------------------------
extern SFilterMatcher * filter_match_and(SFilterMatcher * pMatcher1,
					 SFilterMatcher * pMatcher2);
// ----- filter_match_or --------------------------------------------
extern SFilterMatcher * filter_match_or(SFilterMatcher * pMatcher1,
					SFilterMatcher * pMatcher2);
// ----- filter_match_not -------------------------------------------
extern SFilterMatcher * filter_match_not(SFilterMatcher * pMatcher);
// ----- filter_match_comm_contains ---------------------------------
extern SFilterMatcher * filter_match_comm_contains(uint32_t uCommunity);
// ----- filter_match_prefix_equals ---------------------------------
extern SFilterMatcher * filter_match_prefix_equals(SPrefix sPrefix);
// ----- filter_match_prefix_in -------------------------------------
extern SFilterMatcher * filter_match_prefix_in(SPrefix sPrefix);
// ----- filter_action_accept ---------------------------------------
extern SFilterAction * filter_action_accept();
// ----- filter_action_deny -----------------------------------------
extern SFilterAction * filter_action_deny();
// ----- filter_action_pref_set -------------------------------------
extern SFilterAction * filter_action_pref_set(uint32_t uPref);
// ----- filter_action_comm_append ----------------------------------
extern SFilterAction * filter_action_comm_append(comm_t uCommunity);
// ----- filter_action_comm_strip -----------------------------------
extern SFilterAction * filter_action_comm_strip();
// ----- filter_action_ecomm_append ---------------------------------
extern SFilterAction * filter_action_ecomm_append(SECommunity * pComm);
// ----- filter_action_path_prepend ---------------------------------
extern SFilterAction * filter_action_path_prepend(uint8_t uAmount);
// ----- filter_dump ------------------------------------------------
extern void filter_dump(FILE * pStream, SFilter * pFilter);
// ----- filter_matcher_dump ----------------------------------------
extern void filter_matcher_dump(FILE * pStream,
				SFilterMatcher * pMatcher);
// ----- filter_action_dump -----------------------------------------
extern void filter_action_dump(FILE * pStream,
			       SFilterAction * pAction);

#endif
