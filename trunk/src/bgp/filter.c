// ==================================================================
// @(#)filter.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @author Sebastien Tandel (standel@info.ucl.ac.be)
// @date 27/11/2002
// $Id: filter.c,v 1.22 2008-04-07 09:12:37 bqu Exp $
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
hash_t * pHashPathExpr = NULL;
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

// -----[ _ft_matcher_create ]---------------------------------------
/**
 *
 */
static inline bgp_ft_matcher_t *
_ft_matcher_create(bgp_ft_matcher_code_t code, unsigned char uSize)
{
  bgp_ft_matcher_t * pMatcher=
    (bgp_ft_matcher_t *) MALLOC(sizeof(bgp_ft_matcher_t)+uSize);
  pMatcher->uCode= code;
  pMatcher->uSize= uSize;
  return pMatcher;
}

// ----- filter_matcher_destroy -------------------------------------
/**
 *
 */
void filter_matcher_destroy(bgp_ft_matcher_t ** ppMatcher)
{
  if (*ppMatcher != NULL) {
    FREE(*ppMatcher);
    *ppMatcher= NULL;
  }
}

// -----[ _ft_action_create ]----------------------------------------
/**
 *
 */
static inline bgp_ft_action_t *
_ft_action_create(bgp_ft_action_code_t code, unsigned char uSize)
{
  bgp_ft_action_t * pAction=
    (bgp_ft_action_t *) MALLOC(sizeof(bgp_ft_action_t)+uSize);
  pAction->uCode= code;
  pAction->uSize= uSize;
  pAction->pNextAction= NULL;
  return pAction;
}

// ----- filter_action_destroy --------------------------------------
/**
 *
 */
void filter_action_destroy(bgp_ft_action_t ** ppAction)
{
  bgp_ft_action_t * pAction;
  bgp_ft_action_t * pOldAction;

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
bgp_ft_rule_t * filter_rule_create(bgp_ft_matcher_t * pMatcher,
				   bgp_ft_action_t * pAction)
{
  bgp_ft_rule_t * pRule= (bgp_ft_rule_t *) MALLOC(sizeof(bgp_ft_rule_t));
  pRule->pMatcher= pMatcher;
  pRule->pAction= pAction;
  return pRule;
}

// ----- filter_rule_destroy ----------------------------------------
/**
 *
 */
void filter_rule_destroy(bgp_ft_rule_t ** ppRule)
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
  bgp_ft_rule_t ** ppRule= (bgp_ft_rule_t **) ppItem;

  filter_rule_destroy(ppRule);
}

// ----- filter_create ----------------------------------------------
/**
 *
 */
bgp_filter_t * filter_create()
{
  bgp_filter_t * pFilter= (bgp_filter_t *) MALLOC(sizeof(bgp_filter_t));
  pFilter->pSeqRules= sequence_create(NULL, filter_rule_seq_destroy);
  return pFilter;
}

// ----- filter_destroy ---------------------------------------------
/**
 *
 */
void filter_destroy(bgp_filter_t ** ppFilter)
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
int filter_rule_apply(bgp_ft_rule_t * pRule, bgp_router_t * pRouter,
		      bgp_route_t * pRoute)
{
  if (!filter_matcher_apply(pRule->pMatcher, pRouter, pRoute))
    return 2;
  return filter_action_apply(pRule->pAction, pRouter, pRoute);
}

// ----- filter_apply -----------------------------------------------
/**
 *
 */
int filter_apply(bgp_filter_t * pFilter, bgp_router_t * pRouter, bgp_route_t * pRoute)
{
  int iIndex;
  int iResult;

  if (pFilter != NULL) {
    for (iIndex= 0; iIndex < pFilter->pSeqRules->iSize; iIndex++) {
      iResult= filter_rule_apply((bgp_ft_rule_t *)
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
int filter_jump(bgp_filter_t * pFilter, bgp_router_t * pRouter, bgp_route_t * pRoute)
{
  return filter_apply(pFilter, pRouter, pRoute);
}

// ----- filter_action_call ------------------------------------------
/**
 *
 *
 */
int filter_call(bgp_filter_t * pFilter, bgp_router_t * pRouter, bgp_route_t * pRoute)
{
  int iIndex; 
  int iResult;

  if (pFilter != NULL) {
    for (iIndex= 0; iIndex < pFilter->pSeqRules->iSize; iIndex++) {
      iResult= filter_rule_apply((bgp_ft_rule_t *)
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
int filter_matcher_apply(bgp_ft_matcher_t * pMatcher, bgp_router_t * pRouter,
			 bgp_route_t * pRoute)
{
  comm_t tCommunity;
  SPathMatch * pPathMatcher;
  bgp_ft_matcher_t * pMatcher1;
  bgp_ft_matcher_t * pMatcher2;

  if (pMatcher != NULL) {
    switch (pMatcher->uCode) {
    case FT_MATCH_ANY:
      return 1;
    case FT_MATCH_OP_AND:
      pMatcher1= (bgp_ft_matcher_t *) pMatcher->auParams;
      pMatcher2= (bgp_ft_matcher_t *) &pMatcher->auParams[sizeof(bgp_ft_matcher_t)+
							  pMatcher1->uSize];
      return (filter_matcher_apply(pMatcher1, pRouter, pRoute) &&
	      filter_matcher_apply(pMatcher2, pRouter, pRoute));
    case FT_MATCH_OP_OR:
      pMatcher1= (bgp_ft_matcher_t *) pMatcher->auParams;
      pMatcher2= (bgp_ft_matcher_t *) &pMatcher->auParams[sizeof(bgp_ft_matcher_t)+
							  pMatcher1->uSize];
	return (filter_matcher_apply(pMatcher1, pRouter, pRoute) ||
		filter_matcher_apply(pMatcher2, pRouter, pRoute));
    case FT_MATCH_OP_NOT:
      return !filter_matcher_apply((bgp_ft_matcher_t *) pMatcher->auParams,
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
      fatal("invalid filter matcher byte code (%u)\n", pMatcher->uCode);
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
int filter_action_apply(bgp_ft_action_t * pAction, bgp_router_t * pRouter,
			bgp_route_t * pRoute)
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
      route_path_prepend(pRoute, pRouter->uASN,
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
	pRouteInfo= rt_find_best(pRouter->pNode->rt, pRoute->pAttr->tNextHop,
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
      return filter_jump((bgp_filter_t *)pAction->auParams, pRouter, pRoute);
      break;
    case FT_ACTION_CALL:
      return filter_call((bgp_filter_t *)pAction->auParams, pRouter, pRoute);
    default:
      fatal("invalid filter action byte code (%u)\n", pAction->uCode);
    }
    pAction= pAction->pNextAction;
  }
  return 2;
}

// ----- filter_add_rule --------------------------------------------
/**
 *
 */
int filter_add_rule(bgp_filter_t * pFilter, bgp_ft_matcher_t * pMatcher,
		    bgp_ft_action_t * pAction)
{
  return sequence_add(pFilter->pSeqRules,
		      filter_rule_create(pMatcher, pAction));
}

// ----- filter_add_rule2 -------------------------------------------
/**
 *
 */
int filter_add_rule2(bgp_filter_t * pFilter, bgp_ft_rule_t * pRule)
{
  return sequence_add(pFilter->pSeqRules, pRule);
}

// ----- filter_insert_rule -----------------------------------------
int filter_insert_rule(bgp_filter_t * pFilter, unsigned int uIndex,
		       bgp_ft_rule_t * pRule)
{
  return sequence_insert_at(pFilter->pSeqRules, uIndex, pRule);
}

// ----- filter_remove_rule -----------------------------------------
int filter_remove_rule(bgp_filter_t * pFilter, unsigned int uIndex)
{
  if (uIndex < pFilter->pSeqRules->iSize)
    filter_rule_destroy((bgp_ft_rule_t **) &pFilter->pSeqRules->ppItems[uIndex]);
  return sequence_remove_at(pFilter->pSeqRules, uIndex);
}

// -----[ _ft_match_compound ]---------------------------------------
/**
 * Create a new matcher whose parameters are other matchers. 
 * The function checks if there is enough space in the target matcher
 * to store the children.
 */
static inline bgp_ft_matcher_t *
_ft_match_compound(bgp_ft_matcher_code_t code,
		   bgp_ft_matcher_t * matcher1,
		   bgp_ft_matcher_t * matcher2)
{
  bgp_ft_matcher_t * matcher;
  unsigned int size1, size2;

  size1= matcher1->uSize + sizeof(bgp_ft_matcher_t);
  if (matcher2 != NULL)
    size2= matcher2->uSize + sizeof(bgp_ft_matcher_t);
  else
    size2= 0;

  assert(size1 + size2 <= BGP_FT_MATCHER_SIZE_MAX);
  matcher= _ft_matcher_create(code, size1 + size2);
  memcpy(matcher->auParams, matcher1, size1);
  filter_matcher_destroy(&matcher1);
  if (matcher2 != NULL) {
    memcpy(&matcher->auParams[size1], matcher2, size2);
    filter_matcher_destroy(&matcher2);
  }
  return matcher;
}

// ----- filter_match_and -------------------------------------------
/**
 *
 */
bgp_ft_matcher_t * filter_match_and(bgp_ft_matcher_t * pMatcher1,
				  bgp_ft_matcher_t * pMatcher2)
{
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
  return _ft_match_compound(FT_MATCH_OP_AND, pMatcher1, pMatcher2);
}

// ----- filter_match_or --------------------------------------------
/**
 *
 */
bgp_ft_matcher_t * filter_match_or(bgp_ft_matcher_t * pMatcher1,
				   bgp_ft_matcher_t * pMatcher2)
{
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
  return _ft_match_compound(FT_MATCH_OP_OR, pMatcher1, pMatcher2);
}

// ----- filter_match_not -------------------------------------------
/**
 *
 */
bgp_ft_matcher_t * filter_match_not(bgp_ft_matcher_t * pMatcher)
{
  bgp_ft_matcher_t * new_matcher;

  // Simplify operation: in the case of "not(not(expr))", return "expr"
  if ((pMatcher != NULL) && (pMatcher->uCode == FT_MATCH_OP_NOT)) {

    new_matcher=
      _ft_matcher_create(((bgp_ft_matcher_t *) pMatcher->auParams)->uCode,
			    ((bgp_ft_matcher_t *) pMatcher->auParams)->uSize);
    memcpy(new_matcher->auParams,
	   ((bgp_ft_matcher_t *) pMatcher->auParams)->auParams,
	   ((bgp_ft_matcher_t *) pMatcher->auParams)->uSize);
    filter_matcher_destroy(&pMatcher);
    return new_matcher;
  }

  return _ft_match_compound(FT_MATCH_OP_NOT, pMatcher, NULL);
}

// ----- filter_match_nexthop_equals --------------------------------
bgp_ft_matcher_t * filter_match_nexthop_equals(net_addr_t tNextHop)
{
  bgp_ft_matcher_t * pMatcher=
    _ft_matcher_create(FT_MATCH_NEXTHOP_IS,
		       sizeof(tNextHop));
  memcpy(pMatcher->auParams, &tNextHop, sizeof(tNextHop));
  return pMatcher;
}

// ----- filter_match_nexthop_in ------------------------------------
bgp_ft_matcher_t * filter_match_nexthop_in(SPrefix sPrefix)
{
  bgp_ft_matcher_t * pMatcher=
    _ft_matcher_create(FT_MATCH_NEXTHOP_IN,
		       sizeof(SPrefix));
  memcpy(pMatcher->auParams, &sPrefix, sizeof(SPrefix));
  return pMatcher;
}

// ----- filter_match_prefix_equals ---------------------------------
/**
 *
 */
bgp_ft_matcher_t * filter_match_prefix_equals(SPrefix sPrefix)
{
  bgp_ft_matcher_t * pMatcher=
    _ft_matcher_create(FT_MATCH_PREFIX_IS,
			  sizeof(SPrefix));
  memcpy(pMatcher->auParams, &sPrefix, sizeof(SPrefix));
  return pMatcher;
}

// ----- filter_match_prefix_in -------------------------------------
/**
 *
 */
bgp_ft_matcher_t * filter_match_prefix_in(SPrefix sPrefix)
{
  bgp_ft_matcher_t * pMatcher=
    _ft_matcher_create(FT_MATCH_PREFIX_IN,
			  sizeof(SPrefix));
  memcpy(pMatcher->auParams, &sPrefix, sizeof(SPrefix));
  return pMatcher;
}

// ----- filter_match_prefix_ge -------------------------------------
/**
 *
 */
bgp_ft_matcher_t * filter_match_prefix_ge(SPrefix sPrefix, uint8_t uMaskLen)
{
  bgp_ft_matcher_t * pMatcher=
    _ft_matcher_create(FT_MATCH_PREFIX_GE,
			  sizeof(SPrefix)+sizeof(uint8_t));
  memcpy(pMatcher->auParams, &sPrefix, sizeof(SPrefix));
  memcpy(pMatcher->auParams+sizeof(SPrefix), &uMaskLen, sizeof(uint8_t));
  return pMatcher;
}

// ----- filter_match_prefix_le -------------------------------------
/**
 *
 */
bgp_ft_matcher_t * filter_match_prefix_le(SPrefix sPrefix, uint8_t uMaskLen)
{
  bgp_ft_matcher_t * pMatcher=
    _ft_matcher_create(FT_MATCH_PREFIX_LE,
			  sizeof(SPrefix)+sizeof(uint8_t));
  memcpy(pMatcher->auParams, &sPrefix, sizeof(SPrefix));
  memcpy(pMatcher->auParams+sizeof(SPrefix), &uMaskLen, sizeof(uint8_t));
  return pMatcher;
}

// ----- filter_match_comm_contains ---------------------------------
/**
 *
 */
bgp_ft_matcher_t * filter_match_comm_contains(comm_t uCommunity)
{
  bgp_ft_matcher_t * pMatcher=
    _ft_matcher_create(FT_MATCH_COMM_CONTAINS,
			  sizeof(comm_t));
  memcpy(pMatcher->auParams, &uCommunity, sizeof(uCommunity));
  return pMatcher;
}

// ----- filter_match_path -------------------------------------------
/**
 *
 */
bgp_ft_matcher_t * filter_match_path(int iArrayPathRegExPos)
{
  bgp_ft_matcher_t * pMatcher=
    _ft_matcher_create(FT_MATCH_PATH_MATCHES,
			  sizeof(int));
  
  memcpy(pMatcher->auParams, &iArrayPathRegExPos, sizeof(iArrayPathRegExPos));
  return pMatcher;
}

// ----- filter_action_accept -----------------------------------------
/**
 *
 */
bgp_ft_action_t * filter_action_accept()
{
  return _ft_action_create(FT_ACTION_ACCEPT, 0);
}

// ----- filter_action_deny -----------------------------------------
/**
 *
 */
bgp_ft_action_t * filter_action_deny()
{
  
  return _ft_action_create(FT_ACTION_DENY, 0);
}

// ----- filter_action_jump ------------------------------------------
/**
 *
 *
 */
bgp_ft_action_t * filter_action_jump(bgp_filter_t * pFilter)
{
  bgp_ft_action_t * pAction = _ft_action_create(FT_ACTION_JUMP,
						  sizeof(bgp_filter_t *));
  memcpy(pAction->auParams, pFilter, sizeof(bgp_filter_t *));
  return pAction;
}

// ----- filter_action_call ------------------------------------------
/**
 *
 *
 */
bgp_ft_action_t * filter_action_call(bgp_filter_t * pFilter)
{
  bgp_ft_action_t * pAction = _ft_action_create(FT_ACTION_CALL,
						  sizeof(bgp_filter_t *));
  memcpy(pAction->auParams, pFilter, sizeof(bgp_filter_t *));
  return pAction;
}

// ----- filter_action_pref_set -------------------------------------
/**
 *
 */
bgp_ft_action_t * filter_action_pref_set(uint32_t uPref)
{
  bgp_ft_action_t * pAction= _ft_action_create(FT_ACTION_PREF_SET,
						sizeof(uint32_t));
  memcpy(pAction->auParams, &uPref, sizeof(uint32_t));
  return pAction;
}

// ----- filter_action_metric_set -----------------------------------
/**
 *
 */
bgp_ft_action_t * filter_action_metric_set(uint32_t uMetric)
{
  bgp_ft_action_t * pAction= _ft_action_create(FT_ACTION_METRIC_SET,
						sizeof(uint32_t));
  memcpy(pAction->auParams, &uMetric, sizeof(uint32_t));
  return pAction;
}

// ----- filter_action_metric_internal ------------------------------
/**
 *
 */
bgp_ft_action_t * filter_action_metric_internal()
{
  return _ft_action_create(FT_ACTION_METRIC_INTERNAL, 0);
}


// ----- filter_action_comm_append ----------------------------------
/**
 *
 */
bgp_ft_action_t * filter_action_comm_append(uint32_t uCommunity)
{
  bgp_ft_action_t * pAction= _ft_action_create(FT_ACTION_COMM_APPEND,
						sizeof(uint32_t));
  memcpy(pAction->auParams, &uCommunity, sizeof(uint32_t));
  return pAction;
}

// ----- filter_action_comm_remove ----------------------------------
/**
 *
 */
bgp_ft_action_t * filter_action_comm_remove(uint32_t uCommunity)
{
  bgp_ft_action_t * pAction= _ft_action_create(FT_ACTION_COMM_REMOVE,
						sizeof(uint32_t));
  memcpy(pAction->auParams, &uCommunity, sizeof(uint32_t));
  return pAction;
}

// ----- filter_action_comm_strip -----------------------------------
/**
 *
 */
bgp_ft_action_t * filter_action_comm_strip()
{
  bgp_ft_action_t * pAction= _ft_action_create(FT_ACTION_COMM_STRIP, 0);
  return pAction;
}

// ----- filter_action_ecomm_append ---------------------------------
/**
 *
 */
bgp_ft_action_t * filter_action_ecomm_append(SECommunity * pComm)
{
  bgp_ft_action_t * pAction= _ft_action_create(FT_ACTION_ECOMM_APPEND,
						sizeof(SECommunity));
  memcpy(pAction->auParams, pComm, sizeof(SECommunity));
  ecomm_val_destroy(&pComm);
  return pAction;
}

// ----- filter_action_path_prepend ---------------------------------
/**
 *
 */
bgp_ft_action_t * filter_action_path_prepend(uint8_t uAmount)
{
  bgp_ft_action_t * pAction= _ft_action_create(FT_ACTION_PATH_PREPEND,
						sizeof(uint8_t));
  memcpy(pAction->auParams, &uAmount, sizeof(uint8_t));
  return pAction;
}

// ----- filter_action_path_rem_private -----------------------------
/**
 *
 */
bgp_ft_action_t * filter_action_path_rem_private()
{
  return _ft_action_create(FT_ACTION_PATH_REM_PRIVATE, 0);
}

// ----- filter_matcher_dump ----------------------------------------
/**
 *
 */
void filter_matcher_dump(SLogStream * pStream, bgp_ft_matcher_t * pMatcher)
{
  comm_t tCommunity;
  SPathMatch * pPathMatch;

  if (pMatcher != NULL) {
    switch (pMatcher->uCode) {
    case FT_MATCH_ANY:
      log_printf(pStream, "any");
      break;
    case FT_MATCH_OP_AND:
      log_printf(pStream, "(");
      filter_matcher_dump(pStream, (bgp_ft_matcher_t *) pMatcher->auParams);
      log_printf(pStream, ")AND(");
      filter_matcher_dump(pStream,
			  (bgp_ft_matcher_t *)
			  &pMatcher->auParams[sizeof(bgp_ft_matcher_t)+
					      pMatcher->auParams[1]]);
      log_printf(pStream, ")");
      break;
    case FT_MATCH_OP_OR:
      log_printf(pStream, "(");
      filter_matcher_dump(pStream, (bgp_ft_matcher_t *) pMatcher->auParams);
      log_printf(pStream, ")OR(");
      filter_matcher_dump(pStream,
			  (bgp_ft_matcher_t *)
			  &pMatcher->auParams[sizeof(bgp_ft_matcher_t)+
					      pMatcher->auParams[1]]);
      log_printf(pStream, ")");
      break;
    case FT_MATCH_OP_NOT:
      log_printf(pStream, "NOT(");
      filter_matcher_dump(pStream, (bgp_ft_matcher_t *) pMatcher->auParams);
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
void filter_action_dump(SLogStream * pStream, bgp_ft_action_t * pAction)
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
void filter_rule_dump(SLogStream * pStream, bgp_ft_rule_t * pRule)
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
void filter_dump(SLogStream * pStream, bgp_filter_t * pFilter)
{
  int iIndex;

  if (pFilter != NULL)
    for (iIndex= 0; iIndex < pFilter->pSeqRules->iSize; iIndex++) {
      log_printf(pStream, "%u. ", iIndex);
      filter_rule_dump(pStream,
		       (bgp_ft_rule_t *) pFilter->pSeqRules->ppItems[iIndex]);
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
