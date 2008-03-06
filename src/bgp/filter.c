// ==================================================================
// @(#)filter.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @author Sebastien Tandel (standel@info.ucl.ac.be)
// @date 27/11/2002
// @lastdate 16/01/2008
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <libgds/memory.h>
#include <libgds/types.h>
#include <libgds/hash_utils.h>

#include <bgp/comm.h>
#include <bgp/ecomm.h>
#include <bgp/path.h>
#include <bgp/filter.h>
#include <net/network.h>


SPtrArray * paPathExpr = NULL;
SHash * pHashPathExpr = NULL;
static uint32_t  uHashPathRegExSize = 64;

// -----[ _filter_path_regex_hash_compare ]--------------------------
static int _filter_path_regex_hash_compare(void * pItem1, void * pItem2, 
				unsigned int uEltSize)
{
  SPathMatch * pRegEx1= (SPathMatch *) pItem1;
  SPathMatch * pRegEx2= (SPathMatch *) pItem2;

  return strcmp(pRegEx1->pcPattern, pRegEx2->pcPattern);
}

// -----[ _filter_path_regex_hash_destroy ]--------------------------
static void _filter_path_regex_hash_destroy(void * pItem)
{
  SPathMatch * pRegEx= (SPathMatch *) pItem;

  if ( pRegEx != NULL) {
    if (pRegEx->pcPattern != NULL)
      FREE(pRegEx->pcPattern);

    if (pRegEx->pRegEx != NULL)
      regex_finalize(&(pRegEx->pRegEx));
    FREE(pItem);
  }
}

// -----[ _filter_path_regex_hash_compute ]--------------------------
/**
 * Universal hash function for string keys (discussed in Sedgewick's
 * "Algorithms in C, 3rd edition") and adapted.
 */
static uint32_t _filter_path_regex_hash_compute(const void * pItem,
						const uint32_t uHashSize)
{
  SPathMatch * pRegEx= (SPathMatch *) pItem;
  
  if (pRegEx == NULL)
    return 0;

  return hash_utils_key_compute_string(pRegEx->pcPattern,
				       uHashPathRegExSize) % uHashSize;
}

// ----- filter_matcher_create --------------------------------------
/**
 *
 */
SFilterMatcher * filter_matcher_create(unsigned char uCode,
				       unsigned char uSize)
{
  SFilterMatcher * pMatcher=
    (SFilterMatcher *) MALLOC(sizeof(SFilterMatcher)+uSize);
  pMatcher->uCode= uCode;
  pMatcher->uSize= uSize;
  return pMatcher;
}

// ----- filter_matcher_destroy -------------------------------------
/**
 *
 */
void filter_matcher_destroy(SFilterMatcher ** ppMatcher)
{
  if (*ppMatcher != NULL) {
    FREE(*ppMatcher);
    *ppMatcher= NULL;
  }
}

// ----- filter_action_create ---------------------------------------
/**
 *
 */
SFilterAction * filter_action_create(unsigned char uCode,
				     unsigned char uSize)
{
  SFilterAction * pAction=
    (SFilterAction *) MALLOC(sizeof(SFilterAction)+uSize);
  pAction->uCode= uCode;
  pAction->uSize= uSize;
  pAction->pNextAction= NULL;
  return pAction;
}

// ----- filter_action_destroy --------------------------------------
/**
 *
 */
void filter_action_destroy(SFilterAction ** ppAction)
{
  SFilterAction * pAction;
  SFilterAction * pOldAction;

  if (*ppAction != NULL) {
    pAction= *ppAction;
    while (pAction != NULL) {
      pOldAction= pAction;
      pAction= pAction->pNextAction;
      FREE(pOldAction);
    }
    *ppAction= NULL;
  }
}

// ----- filter_rule_create -----------------------------------------
/**
 *
 */
SFilterRule * filter_rule_create(SFilterMatcher * pMatcher,
				 SFilterAction * pAction)
{
  SFilterRule * pRule= (SFilterRule *) MALLOC(sizeof(SFilterRule));
  pRule->pMatcher= pMatcher;
  pRule->pAction= pAction;
  return pRule;
}

// ----- filter_rule_destroy ----------------------------------------
/**
 *
 */
void filter_rule_destroy(SFilterRule ** ppRule)
{
  if (*ppRule != NULL) {
    filter_matcher_destroy(&(*ppRule)->pMatcher);
    filter_action_destroy(&(*ppRule)->pAction);
    FREE(*ppRule);
    *ppRule= NULL;
  }
}

// ----- filter_rule_destroy ----------------------------------------
/**
 *
 */
void filter_rule_seq_destroy(void ** ppItem)
{
  SFilterRule ** ppRule= (SFilterRule **) ppItem;

  filter_rule_destroy(ppRule);
}

// ----- filter_create ----------------------------------------------
/**
 *
 */
SFilter * filter_create()
{
  SFilter * pFilter= (SFilter *) MALLOC(sizeof(SFilter));
  pFilter->pSeqRules= sequence_create(NULL, filter_rule_seq_destroy);
  return pFilter;
}

// ----- filter_destroy ---------------------------------------------
/**
 *
 */
void filter_destroy(SFilter ** ppFilter)
{
  if (*ppFilter != NULL) {
    sequence_destroy(&(*ppFilter)->pSeqRules);
    FREE(*ppFilter);
    *ppFilter= NULL;
  }
}

// ----- filter_rule_apply ------------------------------------------
/**
 * result:
 * 0 => DENY
 * 1 => ACCEPT
 * 2 => continue with next rule
 */
int filter_rule_apply(SFilterRule * pRule, SBGPRouter * pRouter,
		      SRoute * pRoute)
{
  if (!filter_matcher_apply(pRule->pMatcher, pRouter, pRoute))
    return 2;
  return filter_action_apply(pRule->pAction, pRouter, pRoute);
}

// ----- filter_apply -----------------------------------------------
/**
 *
 */
int filter_apply(SFilter * pFilter, SBGPRouter * pRouter, SRoute * pRoute)
{
  int iIndex;
  int iResult;

  if (pFilter != NULL) {
    for (iIndex= 0; iIndex < pFilter->pSeqRules->iSize; iIndex++) {
      iResult= filter_rule_apply((SFilterRule *)
				 pFilter->pSeqRules->ppItems[iIndex],
				 pRouter, pRoute);
      if ((iResult == 0) || (iResult == 1))
	return iResult;
    }
  }
  return 1; // ACCEPT
}


// ----- filter_action_jump ------------------------------------------
/**
 *
 *
 */
int filter_jump(SFilter * pFilter, SBGPRouter * pRouter, SRoute * pRoute)
{
  return filter_apply(pFilter, pRouter, pRoute);
}

// ----- filter_action_call ------------------------------------------
/**
 *
 *
 */
int filter_call(SFilter * pFilter, SBGPRouter * pRouter, SRoute * pRoute)
{
  int iIndex; 
  int iResult;

  if (pFilter != NULL) {
    for (iIndex= 0; iIndex < pFilter->pSeqRules->iSize; iIndex++) {
      iResult= filter_rule_apply((SFilterRule *)
				 pFilter->pSeqRules->ppItems[iIndex],
				 pRouter, pRoute);
      if ((iResult == 0) || (iResult == 1))
	return iResult;
    }
  }
  return 2; // CONTINUE with next rule
}


// ----- filter_matcher_apply ---------------------------------------
/**
 * result:
 * 0 => route does not match the filter
 * 1 => route matches the filter
 */
int filter_matcher_apply(SFilterMatcher * pMatcher, SBGPRouter * pRouter,
			 SRoute * pRoute)
{
  comm_t tCommunity;
  SPathMatch * pPathMatcher;

  if (pMatcher != NULL) {
    switch (pMatcher->uCode) {
    case FT_MATCH_OP_AND:
      return (filter_matcher_apply((SFilterMatcher *) pMatcher->auParams,
				   pRouter, pRoute) &&
	      filter_matcher_apply((SFilterMatcher *)
				   &pMatcher->auParams[sizeof(SFilterMatcher)+
						       pMatcher->auParams[1]],
				   pRouter, pRoute));
    case FT_MATCH_OP_OR:
      return (filter_matcher_apply((SFilterMatcher *) pMatcher->auParams,
				   pRouter, pRoute) ||
	      filter_matcher_apply((SFilterMatcher *)
				   &pMatcher->auParams[sizeof(SFilterMatcher)+
						       pMatcher->auParams[1]],
				   pRouter, pRoute));
    case FT_MATCH_OP_NOT:
      return !filter_matcher_apply((SFilterMatcher *) pMatcher->auParams,
				   pRouter, pRoute);
    case FT_MATCH_COMM_CONTAINS:
      memcpy(&tCommunity, pMatcher->auParams, sizeof(tCommunity));
      return route_comm_contains(pRoute, tCommunity)?1:0;
    case FT_MATCH_NEXTHOP_IS:
      return (pRoute->pAttr->tNextHop == *((net_addr_t *) pMatcher->auParams))?1:0;
    case FT_MATCH_NEXTHOP_IN:
      return ip_address_in_prefix(pRoute->pAttr->tNextHop,
				  *((SPrefix*) pMatcher->auParams))?1:0;
    case FT_MATCH_PREFIX_IS:
      return ip_prefix_cmp(&pRoute->sPrefix,
			    ((SPrefix*) pMatcher->auParams))?0:1;
    case FT_MATCH_PREFIX_IN:
      return ip_prefix_in_prefix(pRoute->sPrefix,
				 *((SPrefix*) pMatcher->auParams))?1:0;
    case FT_MATCH_PREFIX_GE:
      return ip_prefix_ge_prefix(pRoute->sPrefix,
				 *((SPrefix*) pMatcher->auParams),
				 *((uint8_t *) (pMatcher->auParams+
						sizeof(SPrefix))))?1:0;
    case FT_MATCH_PREFIX_LE:
      return ip_prefix_le_prefix(pRoute->sPrefix,
				 *((SPrefix*) pMatcher->auParams),
				 *((uint8_t *) (pMatcher->auParams+
						sizeof(SPrefix))))?1:0;
    case FT_MATCH_PATH_MATCHES:
      assert(ptr_array_get_at(paPathExpr, *((int *) pMatcher->auParams),
			      &pPathMatcher) >= 0);
      return path_match(route_get_path(pRoute), pPathMatcher->pRegEx)?1:0;
    default:
      abort();
    }
  }
  return 1;
}

// ----- filter_action_apply ----------------------------------------
/**
 * result:
 * 0 => DENY route
 * 1 => ACCEPT route
 * 2 => continue with next rule
 */
int filter_action_apply(SFilterAction * pAction, SBGPRouter * pRouter,
			SRoute * pRoute)
{
  SNetRouteInfo * pRouteInfo;

  while (pAction != NULL) {
    switch (pAction->uCode) {
    case FT_ACTION_ACCEPT:
      return 1;
    case FT_ACTION_DENY:
      return 0;
    case FT_ACTION_COMM_APPEND:
      route_comm_append(pRoute, *((uint32_t *) pAction->auParams));
      break;
    case FT_ACTION_COMM_STRIP:
      route_comm_strip(pRoute);
      break;
    case FT_ACTION_COMM_REMOVE:
      route_comm_remove(pRoute, *((uint32_t *) pAction->auParams));
      break;
    case FT_ACTION_PATH_PREPEND:
      route_path_prepend(pRoute, pRouter->uNumber,
			 *((uint8_t *) pAction->auParams));
      break;
    case FT_ACTION_PATH_REM_PRIVATE:
      route_path_rem_private(pRoute);
      break;
    case FT_ACTION_PREF_SET:
      route_localpref_set(pRoute, *((uint32_t *) pAction->auParams));
      break;
    case FT_ACTION_METRIC_SET:
      route_med_set(pRoute, *((uint32_t *) pAction->auParams));
      break;
    case FT_ACTION_METRIC_INTERNAL:
      if (pRoute->pAttr->tNextHop != pRouter->pNode->tAddr) {
	pRouteInfo= rt_find_best(pRouter->pNode->pRT, pRoute->pAttr->tNextHop,
				 NET_ROUTE_ANY);
	assert(pRouteInfo != NULL);
	route_med_set(pRoute, pRouteInfo->uWeight);
      } else {
	route_med_set(pRoute, 0);
      }
      break;
    case FT_ACTION_ECOMM_APPEND:
      route_ecomm_append(pRoute, ecomm_val_copy((SECommunity *)
						pAction->auParams));
      break;
    case FT_ACTION_JUMP:
      return filter_jump((SFilter *)pAction->auParams, pRouter, pRoute);
      break;
    case FT_ACTION_CALL:
      return filter_call((SFilter *)pAction->auParams, pRouter, pRoute);
    default:
      abort();
    }
    pAction= pAction->pNextAction;
  }
  return 2;
}

// ----- filter_add_rule --------------------------------------------
/**
 *
 */
int filter_add_rule(SFilter * pFilter, SFilterMatcher * pMatcher,
		    SFilterAction * pAction)
{
  return sequence_add(pFilter->pSeqRules,
		      filter_rule_create(pMatcher, pAction));
}

// ----- filter_add_rule2 -------------------------------------------
/**
 *
 */
int filter_add_rule2(SFilter * pFilter, SFilterRule * pRule)
{
  return sequence_add(pFilter->pSeqRules, pRule);
}

// ----- filter_insert_rule -----------------------------------------
int filter_insert_rule(SFilter * pFilter, unsigned int uIndex,
		       SFilterRule * pRule)
{
  return sequence_insert_at(pFilter->pSeqRules, uIndex, pRule);
}

// ----- filter_remove_rule -----------------------------------------
int filter_remove_rule(SFilter * pFilter, unsigned int uIndex)
{
  if (uIndex < pFilter->pSeqRules->iSize)
    filter_rule_destroy((SFilterRule **) &pFilter->pSeqRules->ppItems[uIndex]);
  return sequence_remove_at(pFilter->pSeqRules, uIndex);
}

// ----- filter_match_and -------------------------------------------
/**
 *
 */
SFilterMatcher * filter_match_and(SFilterMatcher * pMatcher1,
				  SFilterMatcher * pMatcher2)
{
  SFilterMatcher * pMatcher;
  int iSize1;
  int iSize2;

  // Simplify:
  //   AND(ANY, predicate) = predicate
  //   AND(predicate, ANY) = predicate
  if ((pMatcher1 == NULL) || (pMatcher1->uCode == FT_MATCH_ANY)) {
    filter_matcher_destroy(&pMatcher1);
    return pMatcher2;
  } else if ((pMatcher2 == NULL) || (pMatcher2->uCode == FT_MATCH_ANY)) {
    filter_matcher_destroy(&pMatcher2);
    return pMatcher1;
  }

  // Build AND(predicate1, predicate2)
  iSize1= sizeof(SFilterMatcher)+pMatcher1->uSize;
  iSize2= sizeof(SFilterMatcher)+pMatcher2->uSize;
  pMatcher= filter_matcher_create(FT_MATCH_OP_AND, iSize1+iSize2);
  memcpy(pMatcher->auParams, pMatcher1, iSize1);
  memcpy(&pMatcher->auParams[iSize1], pMatcher2, iSize2);
  filter_matcher_destroy(&pMatcher1);
  filter_matcher_destroy(&pMatcher2);
  return pMatcher;
}

// ----- filter_match_or --------------------------------------------
/**
 *
 */
SFilterMatcher * filter_match_or(SFilterMatcher * pMatcher1,
				 SFilterMatcher * pMatcher2)
{
  SFilterMatcher * pMatcher;
  int iSize1;
  int iSize2;

  // Simplify expression
  //   OR(ANY, ANY) == ANY
  //   OR(ANY, *) = ANY
  //   OR(*, ANY) = ANY
  if ((pMatcher1 == NULL) || (pMatcher1->uCode == FT_MATCH_ANY) ||
      (pMatcher2 == NULL) || (pMatcher2->uCode == FT_MATCH_ANY)) {
    filter_matcher_destroy(&pMatcher2);
    filter_matcher_destroy(&pMatcher1);
    return NULL;
  }

  // Build OR(predicate1, predicate2)
  iSize1= sizeof(SFilterMatcher)+pMatcher1->uSize;
  iSize2= sizeof(SFilterMatcher)+pMatcher2->uSize;
  pMatcher= filter_matcher_create(FT_MATCH_OP_OR, iSize1+iSize2);
  memcpy(pMatcher->auParams, pMatcher1, iSize1);
  memcpy(&pMatcher->auParams[iSize1], pMatcher2, iSize2);
  filter_matcher_destroy(&pMatcher1);
  filter_matcher_destroy(&pMatcher2);
  return pMatcher;
}

// ----- filter_match_not -------------------------------------------
/**
 *
 */
SFilterMatcher * filter_match_not(SFilterMatcher * pMatcher)
{
  SFilterMatcher * pNewMatcher;

  // Simplify operation: in the case of "not(not(expr))", return "expr"
  if ((pMatcher != NULL) && (pMatcher->uCode == FT_MATCH_OP_NOT)) {

    pNewMatcher=
      filter_matcher_create(((SFilterMatcher *) pMatcher->auParams)->uCode,
			    ((SFilterMatcher *) pMatcher->auParams)->uSize);
    memcpy(pNewMatcher->auParams,
	   ((SFilterMatcher *) pMatcher->auParams)->auParams,
	   ((SFilterMatcher *) pMatcher->auParams)->uSize);
    filter_matcher_destroy(&pMatcher);
    return pNewMatcher;
  }

  int iSize= sizeof(SFilterMatcher)+pMatcher->uSize;
  pNewMatcher= filter_matcher_create(FT_MATCH_OP_NOT,
						   iSize);
  memcpy(pNewMatcher->auParams, pMatcher, iSize);
  filter_matcher_destroy(&pMatcher);
  return pNewMatcher;
}

// ----- filter_match_nexthop_equals --------------------------------
SFilterMatcher * filter_match_nexthop_equals(net_addr_t tNextHop)
{
  SFilterMatcher * pMatcher=
    filter_matcher_create(FT_MATCH_NEXTHOP_IS,
			  sizeof(tNextHop));
  memcpy(pMatcher->auParams, &tNextHop, sizeof(tNextHop));
  return pMatcher;
}

// ----- filter_match_nexthop_in ------------------------------------
SFilterMatcher * filter_match_nexthop_in(SPrefix sPrefix)
{
  SFilterMatcher * pMatcher=
    filter_matcher_create(FT_MATCH_NEXTHOP_IN,
			  sizeof(SPrefix));
  memcpy(pMatcher->auParams, &sPrefix, sizeof(SPrefix));
  return pMatcher;
}

// ----- filter_match_prefix_equals ---------------------------------
/**
 *
 */
SFilterMatcher * filter_match_prefix_equals(SPrefix sPrefix)
{
  SFilterMatcher * pMatcher=
    filter_matcher_create(FT_MATCH_PREFIX_IS,
			  sizeof(SPrefix));
  memcpy(pMatcher->auParams, &sPrefix, sizeof(SPrefix));
  return pMatcher;
}

// ----- filter_match_prefix_in -------------------------------------
/**
 *
 */
SFilterMatcher * filter_match_prefix_in(SPrefix sPrefix)
{
  SFilterMatcher * pMatcher=
    filter_matcher_create(FT_MATCH_PREFIX_IN,
			  sizeof(SPrefix));
  memcpy(pMatcher->auParams, &sPrefix, sizeof(SPrefix));
  return pMatcher;
}

// ----- filter_match_prefix_ge -------------------------------------
/**
 *
 */
SFilterMatcher * filter_match_prefix_ge(SPrefix sPrefix, uint8_t uMaskLen)
{
  SFilterMatcher * pMatcher=
    filter_matcher_create(FT_MATCH_PREFIX_GE,
			  sizeof(SPrefix)+sizeof(uint8_t));
  memcpy(pMatcher->auParams, &sPrefix, sizeof(SPrefix));
  memcpy(pMatcher->auParams+sizeof(SPrefix), &uMaskLen, sizeof(uint8_t));
  return pMatcher;
}

// ----- filter_match_prefix_le -------------------------------------
/**
 *
 */
SFilterMatcher * filter_match_prefix_le(SPrefix sPrefix, uint8_t uMaskLen)
{
  SFilterMatcher * pMatcher=
    filter_matcher_create(FT_MATCH_PREFIX_LE,
			  sizeof(SPrefix)+sizeof(uint8_t));
  memcpy(pMatcher->auParams, &sPrefix, sizeof(SPrefix));
  memcpy(pMatcher->auParams+sizeof(SPrefix), &uMaskLen, sizeof(uint8_t));
  return pMatcher;
}

// ----- filter_match_comm_contains ---------------------------------
/**
 *
 */
SFilterMatcher * filter_match_comm_contains(comm_t uCommunity)
{
  SFilterMatcher * pMatcher=
    filter_matcher_create(FT_MATCH_COMM_CONTAINS,
			  sizeof(comm_t));
  memcpy(pMatcher->auParams, &uCommunity, sizeof(uCommunity));
  return pMatcher;
}

// ----- filter_match_path -------------------------------------------
/**
 *
 */
SFilterMatcher * filter_match_path(int iArrayPathRegExPos)
{
  SFilterMatcher * pMatcher=
    filter_matcher_create(FT_MATCH_PATH_MATCHES,
			  sizeof(int));
  
  memcpy(pMatcher->auParams, &iArrayPathRegExPos, sizeof(iArrayPathRegExPos));
  return pMatcher;
}

// ----- filter_action_accept -----------------------------------------
/**
 *
 */
SFilterAction * filter_action_accept()
{
  return filter_action_create(FT_ACTION_ACCEPT, 0);
}

// ----- filter_action_deny -----------------------------------------
/**
 *
 */
SFilterAction * filter_action_deny()
{
  
  return filter_action_create(FT_ACTION_DENY, 0);
}

// ----- filter_action_jump ------------------------------------------
/**
 *
 *
 */
SFilterAction * filter_action_jump(SFilter * pFilter)
{
  SFilterAction * pAction = filter_action_create(FT_ACTION_JUMP,
						  sizeof(SFilter *));
  memcpy(pAction->auParams, pFilter, sizeof(SFilter *));
  return pAction;
}

// ----- filter_action_call ------------------------------------------
/**
 *
 *
 */
SFilterAction * filter_action_call(SFilter * pFilter)
{
  SFilterAction * pAction = filter_action_create(FT_ACTION_CALL,
						  sizeof(SFilter *));
  memcpy(pAction->auParams, pFilter, sizeof(SFilter *));
  return pAction;
}

// ----- filter_action_pref_set -------------------------------------
/**
 *
 */
SFilterAction * filter_action_pref_set(uint32_t uPref)
{
  SFilterAction * pAction= filter_action_create(FT_ACTION_PREF_SET,
						sizeof(uint32_t));
  memcpy(pAction->auParams, &uPref, sizeof(uint32_t));
  return pAction;
}

// ----- filter_action_metric_set -----------------------------------
/**
 *
 */
SFilterAction * filter_action_metric_set(uint32_t uMetric)
{
  SFilterAction * pAction= filter_action_create(FT_ACTION_METRIC_SET,
						sizeof(uint32_t));
  memcpy(pAction->auParams, &uMetric, sizeof(uint32_t));
  return pAction;
}

// ----- filter_action_metric_internal ------------------------------
/**
 *
 */
SFilterAction * filter_action_metric_internal()
{
  return filter_action_create(FT_ACTION_METRIC_INTERNAL, 0);
}


// ----- filter_action_comm_append ----------------------------------
/**
 *
 */
SFilterAction * filter_action_comm_append(uint32_t uCommunity)
{
  SFilterAction * pAction= filter_action_create(FT_ACTION_COMM_APPEND,
						sizeof(uint32_t));
  memcpy(pAction->auParams, &uCommunity, sizeof(uint32_t));
  return pAction;
}

// ----- filter_action_comm_remove ----------------------------------
/**
 *
 */
SFilterAction * filter_action_comm_remove(uint32_t uCommunity)
{
  SFilterAction * pAction= filter_action_create(FT_ACTION_COMM_REMOVE,
						sizeof(uint32_t));
  memcpy(pAction->auParams, &uCommunity, sizeof(uint32_t));
  return pAction;
}

// ----- filter_action_comm_strip -----------------------------------
/**
 *
 */
SFilterAction * filter_action_comm_strip()
{
  SFilterAction * pAction= filter_action_create(FT_ACTION_COMM_STRIP, 0);
  return pAction;
}

// ----- filter_action_ecomm_append ---------------------------------
/**
 *
 */
SFilterAction * filter_action_ecomm_append(SECommunity * pComm)
{
  SFilterAction * pAction= filter_action_create(FT_ACTION_ECOMM_APPEND,
						sizeof(SECommunity));
  memcpy(pAction->auParams, pComm, sizeof(SECommunity));
  ecomm_val_destroy(&pComm);
  return pAction;
}

// ----- filter_action_path_prepend ---------------------------------
/**
 *
 */
SFilterAction * filter_action_path_prepend(uint8_t uAmount)
{
  SFilterAction * pAction= filter_action_create(FT_ACTION_PATH_PREPEND,
						sizeof(uint8_t));
  memcpy(pAction->auParams, &uAmount, sizeof(uint8_t));
  return pAction;
}

// ----- filter_action_path_rem_private -----------------------------
/**
 *
 */
SFilterAction * filter_action_path_rem_private()
{
  return filter_action_create(FT_ACTION_PATH_REM_PRIVATE, 0);
}

// ----- filter_matcher_dump ----------------------------------------
/**
 *
 */
void filter_matcher_dump(SLogStream * pStream, SFilterMatcher * pMatcher)
{
  comm_t tCommunity;
  SPathMatch * pPathMatch;

  if (pMatcher != NULL) {
    switch (pMatcher->uCode) {
    case FT_MATCH_OP_AND:
      log_printf(pStream, "(");
      filter_matcher_dump(pStream, (SFilterMatcher *) pMatcher->auParams);
      log_printf(pStream, ")AND(");
      filter_matcher_dump(pStream,
			  (SFilterMatcher *)
			  &pMatcher->auParams[sizeof(SFilterMatcher)+
					      pMatcher->auParams[1]]);
      log_printf(pStream, ")");
      break;
    case FT_MATCH_OP_OR:
      log_printf(pStream, "(");
      filter_matcher_dump(pStream, (SFilterMatcher *) pMatcher->auParams);
      log_printf(pStream, ")OR(");
      filter_matcher_dump(pStream,
			  (SFilterMatcher *)
			  &pMatcher->auParams[sizeof(SFilterMatcher)+
					      pMatcher->auParams[1]]);
      log_printf(pStream, ")");
      break;
    case FT_MATCH_OP_NOT:
      log_printf(pStream, "NOT(");
      filter_matcher_dump(pStream, (SFilterMatcher *) pMatcher->auParams);
      log_printf(pStream, ")");
      break;
    case FT_MATCH_COMM_CONTAINS:
      memcpy(&tCommunity, pMatcher->auParams, sizeof(tCommunity));
      log_printf(pStream, "community is ");
      comm_value_dump(pStream, tCommunity, COMM_DUMP_TEXT);
      break;
    case FT_MATCH_NEXTHOP_IS:
      log_printf(pStream, "next-hop is ");
      ip_address_dump(pStream, *((net_addr_t *) pMatcher->auParams));
      break;
    case FT_MATCH_NEXTHOP_IN:
      log_printf(pStream, "next-hop in ");
      ip_prefix_dump(pStream, *((SPrefix *) pMatcher->auParams));
      break;
    case FT_MATCH_PREFIX_IS:
      log_printf(pStream, "prefix is ");
      ip_prefix_dump(pStream, *((SPrefix *) pMatcher->auParams));
      break;
    case FT_MATCH_PREFIX_IN:
      log_printf(pStream, "prefix in ");
      ip_prefix_dump(pStream, *((SPrefix *) pMatcher->auParams));
      break;
    case FT_MATCH_PATH_MATCHES:
      ptr_array_get_at(paPathExpr, *((int *) pMatcher->auParams), &pPathMatch);
      if (pPathMatch != NULL)
	log_printf(pStream, "path \\\"%s\\\"", pPathMatch->pcPattern);
      else
	abort();
      break;
    default:
      log_printf(pStream, "?");
    }
  } else
    log_printf(pStream, "any");
}

// ----- filter_action_dump -----------------------------------------
/**
 *
 */
void filter_action_dump(SLogStream * pStream, SFilterAction * pAction)
{
  while (pAction != NULL) {
    switch (pAction->uCode) {
    case FT_ACTION_ACCEPT:
      log_printf(pStream, "ACCEPT");
      break;
    case FT_ACTION_DENY:
      log_printf(pStream, "DENY");
      break;
    case FT_ACTION_COMM_APPEND:
      log_printf(pStream, "append community ");
      comm_value_dump(pStream, *((uint32_t *) pAction->auParams),
		      COMM_DUMP_TEXT);
      break;
    case FT_ACTION_COMM_STRIP:
      log_printf(pStream, "comm strip");
      break;
    case FT_ACTION_COMM_REMOVE:
      log_printf(pStream, "remove community ");
      comm_value_dump(pStream, *((uint32_t *) pAction->auParams),
		      COMM_DUMP_TEXT);
      break;
    case FT_ACTION_PATH_PREPEND:
      log_printf(pStream, "prepend as-path %u", *((uint8_t *) pAction->auParams));
      break;
    case FT_ACTION_PATH_REM_PRIVATE:
      log_printf(pStream, "path remove-private");
      break;
    case FT_ACTION_PREF_SET:
      log_printf(pStream, "set local-pref %u", *((uint32_t *) pAction->auParams));
      break;
    case FT_ACTION_METRIC_SET:
      log_printf(pStream, "set metric %u", *((uint32_t *) pAction->auParams));
      break;
    case FT_ACTION_METRIC_INTERNAL:
      log_printf(pStream, "set metric internal");
      break;
    case FT_ACTION_ECOMM_APPEND:
      log_printf(pStream, "append ext-community ");
      ecomm_val_dump(pStream, (SECommunity *) pAction->auParams,
		     ECOMM_DUMP_TEXT);
      break;
    default:
      log_printf(pStream, "?");
    }
    pAction= pAction->pNextAction;
    if (pAction != NULL) {
      log_printf(pStream, ", ");
    }
  }
}

// ----- filter_rule_dump -------------------------------------------
/**
 *
 */
void filter_rule_dump(SLogStream * pStream, SFilterRule * pRule)
{
  filter_matcher_dump(pStream, pRule->pMatcher);
  log_printf(pStream, " --> ");
  filter_action_dump(pStream, pRule->pAction);
  log_printf(pStream, "\n");
}

// ----- filter_dump ------------------------------------------------
/**
 *
 */
void filter_dump(SLogStream * pStream, SFilter * pFilter)
{
  int iIndex;

  if (pFilter != NULL)
    for (iIndex= 0; iIndex < pFilter->pSeqRules->iSize; iIndex++) {
      log_printf(pStream, "%u. ", iIndex);
      filter_rule_dump(pStream,
		       (SFilterRule *) pFilter->pSeqRules->ppItems[iIndex]);
    }
  log_printf(pStream, "default. any --> ACCEPT\n");
}

/////////////////////////////////////////////////////////////////////
// INITIALIZATION AND FINALIZATION SECTION
/////////////////////////////////////////////////////////////////////

// ----- filter_path_regex_init --------------------------------------
/**
 *
 */
void _filter_path_regex_init()
{
  pHashPathExpr= hash_init(uHashPathRegExSize, .5, 
			   _filter_path_regex_hash_compare, 
			   _filter_path_regex_hash_destroy, 
			   _filter_path_regex_hash_compute);
  paPathExpr= ptr_array_create(0, NULL, NULL);
}

// ----- filter_path_regex_destroy ----------------------------------
/**
 *
 *
 */
void _filter_path_regex_destroy()
{
  ptr_array_destroy(&paPathExpr);
  hash_destroy(&pHashPathExpr);
}
