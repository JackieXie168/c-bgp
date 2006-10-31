// ==================================================================
// @(#)filter.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @author Sebastien Tandel (standel@info.ucl.ac.be)
// @date 27/11/2002
// @lastdate 18/05/2005
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

// ----- filter_path_regex_compare -----------------------------------
int filter_path_regex_compare(void * pItem1, void * pItem2, 
				unsigned int uEltSize)
{
  SPathMatch * pRegEx1 = (SPathMatch *) pItem1;
  SPathMatch * pRegEx2 = (SPathMatch *) pItem2;
  int iComp;

  //printf("pattern1 : %s\tpattern2: %s\n", pRegEx1->pcPattern, pRegEx2->pcPattern);
  iComp = strcmp(pRegEx1->pcPattern, pRegEx2->pcPattern);
  if (iComp < 0)
    return -1;
  else if (iComp > 0)
    return 1;
  else
    return 0;
}

// ----- filter_path_regex_destroy -----------------------------------
void filter_path_regex_destroy(void * pItem)
{
  SPathMatch * pRegEx = (SPathMatch *)pItem;

  if ( pRegEx != NULL) {
    if (pRegEx->pcPattern != NULL)
      FREE(pRegEx->pcPattern);

    if (pRegEx->pRegEx != NULL)
      regex_finalize(&(pRegEx->pRegEx));
    FREE(pItem);
  }
}

// ----- filter_path_regex_hash --------------------------------------------------
/**
 * Universal hash function for string keys (discussed in Sedgewick's
 * "Algorithms in C, 3rd edition") and adapted.
 */
uint32_t filter_path_regex_hash(void * pItem)
{
  SPathMatch * pRegEx = (SPathMatch *)pItem;
  
  if (pRegEx == NULL)
    return 0;

  return hash_utils_key_compute_string(pRegEx->pcPattern, uHashPathRegExSize);
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

  if (pMatcher != NULL) {
    switch (pMatcher->uCode) {
    case FT_MATCH_OP_AND:
      return filter_matcher_apply((SFilterMatcher *) pMatcher->auParams,
				  pRouter, pRoute) &&
	filter_matcher_apply((SFilterMatcher *)
			     &pMatcher->auParams[sizeof(SFilterMatcher)+
						pMatcher->auParams[1]],
			     pRouter, pRoute);
      break;
    case FT_MATCH_OP_OR:
      return filter_matcher_apply((SFilterMatcher *) pMatcher->auParams,
				  pRouter, pRoute) ||
	filter_matcher_apply((SFilterMatcher *)
			     &pMatcher->auParams[sizeof(SFilterMatcher)+
						pMatcher->auParams[1]],
			     pRouter, pRoute);
      break;
    case FT_MATCH_OP_NOT:
      return !filter_matcher_apply((SFilterMatcher *) pMatcher->auParams,
				   pRouter, pRoute);
    case FT_MATCH_COMM_CONTAINS:
      memcpy(&tCommunity, pMatcher->auParams, sizeof(tCommunity));
      return route_comm_contains(pRoute, tCommunity)?1:0;
      break;
    case FT_MATCH_NEXTHOP_EQUALS:
      return (pRoute->tNextHop == *((net_addr_t *) pMatcher->auParams))?1:0;
      break;
    case FT_MATCH_NEXTHOP_IN:
      return ip_address_in_prefix(pRoute->tNextHop,
				  *((SPrefix*) pMatcher->auParams))?1:0;
      break;
    case FT_MATCH_PREFIX_EQUALS:
      return ip_prefix_equals(pRoute->sPrefix,
			      *((SPrefix*) pMatcher->auParams))?1:0;
      break;
    case FT_MATCH_PREFIX_IN:
      return ip_prefix_in_prefix(pRoute->sPrefix,
				 *((SPrefix*) pMatcher->auParams))?1:0;
      break;
    case FT_MATCH_AS_PATH:
      return path_match(pRoute->pASPath, *((int *) pMatcher->auParams))?1:0;
      break;
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
    case FT_ACTION_PREF_SET:
      route_localpref_set(pRoute, *((uint32_t *) pAction->auParams));
      break;
    case FT_ACTION_METRIC_SET:
      route_med_set(pRoute, *((uint32_t *) pAction->auParams));
      break;
    case FT_ACTION_METRIC_INTERNAL:
      if (pRoute->tNextHop != pRouter->pNode->tAddr) {
	pRouteInfo= rt_find_best(pRouter->pNode->pRT, pRoute->tNextHop,
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
  if (pMatcher1 == NULL)
    return pMatcher2;
  else if (pMatcher2 == NULL)
    return pMatcher1;

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
  if ((pMatcher1 == NULL) || (pMatcher2 == NULL)) {
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
  int iSize= sizeof(SFilterMatcher)+pMatcher->uSize;
  SFilterMatcher * pNewMatcher= filter_matcher_create(FT_MATCH_OP_NOT,
						   iSize);
  memcpy(pNewMatcher->auParams, pMatcher, iSize);
  filter_matcher_destroy(&pMatcher);
  return pNewMatcher;
}

// ----- filter_match_nexthop_equals --------------------------------
SFilterMatcher * filter_match_nexthop_equals(net_addr_t tNextHop)
{
  SFilterMatcher * pMatcher=
    filter_matcher_create(FT_MATCH_NEXTHOP_EQUALS,
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
    filter_matcher_create(FT_MATCH_PREFIX_EQUALS,
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
    filter_matcher_create(FT_MATCH_AS_PATH,
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

// ----- filter_matcher_dump ----------------------------------------
/**
 *
 */
void filter_matcher_dump(FILE * pStream, SFilterMatcher * pMatcher)
{
  comm_t tCommunity;

  if (pMatcher != NULL) {
    switch (pMatcher->uCode) {
    case FT_MATCH_OP_AND:
      fprintf(pStream, "(");
      filter_matcher_dump(pStream, (SFilterMatcher *) pMatcher->auParams);
      fprintf(pStream, ")AND(");
      filter_matcher_dump(pStream,
			  (SFilterMatcher *)
			  &pMatcher->auParams[sizeof(SFilterMatcher)+
					      pMatcher->auParams[1]]);
      fprintf(pStream, ")");
      break;
    case FT_MATCH_OP_OR:
      fprintf(pStream, "(");
      filter_matcher_dump(pStream, (SFilterMatcher *) pMatcher->auParams);
      fprintf(pStream, ")OR(");
      filter_matcher_dump(pStream,
			  (SFilterMatcher *)
			  &pMatcher->auParams[sizeof(SFilterMatcher)+
					      pMatcher->auParams[1]]);
      fprintf(pStream, ")");
      break;
    case FT_MATCH_OP_NOT:
      fprintf(pStream, "NOT(");
      filter_matcher_dump(pStream, (SFilterMatcher *) pMatcher->auParams);
      fprintf(pStream, ")");
      break;
    case FT_MATCH_COMM_CONTAINS:
      memcpy(&tCommunity, pMatcher->auParams, sizeof(tCommunity));
      fprintf(pStream, "comm contains %u", tCommunity);
      break;
    case FT_MATCH_NEXTHOP_EQUALS:
      fprintf(pStream, "next-hop is ");
      ip_address_dump(pStream, *((net_addr_t *) pMatcher->auParams));
      break;
    case FT_MATCH_NEXTHOP_IN:
      fprintf(pStream, "next-hop in ");
      ip_prefix_dump(pStream, *((SPrefix *) pMatcher->auParams));
      break;
    case FT_MATCH_PREFIX_EQUALS:
      fprintf(pStream, "prefix is ");
      ip_prefix_dump(pStream, *((SPrefix *) pMatcher->auParams));
      break;
    case FT_MATCH_PREFIX_IN:
      fprintf(pStream, "prefix in ");
      ip_prefix_dump(pStream, *((SPrefix *) pMatcher->auParams));
      break;
    default:
      fprintf(pStream, "?");
    }
  } else
    fprintf(pStream, "any");
}

// ----- filter_action_dump -----------------------------------------
/**
 *
 */
void filter_action_dump(FILE * pStream, SFilterAction * pAction)
{
  while (pAction != NULL) {
    switch (pAction->uCode) {
    case FT_ACTION_ACCEPT: fprintf(pStream, "ACCEPT"); break;
    case FT_ACTION_DENY: fprintf(pStream, "DENY"); break;
    case FT_ACTION_COMM_APPEND:
      fprintf(pStream, "append community ");
      comm_dump2(pStream, *((uint32_t *) pAction->auParams), COMM_DUMP_TEXT);
      break;
    case FT_ACTION_COMM_STRIP: fprintf(pStream, "comm strip"); break;
    case FT_ACTION_COMM_REMOVE:
      fprintf(pStream, "remove community %u", *((uint32_t *) pAction->auParams));
      break;
    case FT_ACTION_PATH_PREPEND:
      fprintf(pStream, "prepend as-path %u", *((uint8_t *) pAction->auParams));
      break;
    case FT_ACTION_PREF_SET:
      fprintf(pStream, "set local-pref %u", *((uint32_t *) pAction->auParams));
      break;
    case FT_ACTION_METRIC_SET:
      fprintf(pStream, "set metric %u", *((uint32_t *) pAction->auParams));
      break;
    case FT_ACTION_METRIC_INTERNAL:
      fprintf(pStream, "set metric internal");
      break;
    case FT_ACTION_ECOMM_APPEND:
      fprintf(pStream, "append ext-community ");
      ecomm_val_dump(pStream, (SECommunity *) pAction->auParams,
		     ECOMM_DUMP_TEXT);
      break;
    default:
      fprintf(pStream, "?");
    }
    pAction= pAction->pNextAction;
    if (pAction != NULL) {
      fprintf(pStream, ", ");
    }
  }
}

// ----- filter_rule_dump -------------------------------------------
/**
 *
 */
void filter_rule_dump(FILE * pStream, SFilterRule * pRule)
{
  filter_matcher_dump(pStream, pRule->pMatcher);
  fprintf(pStream, " --> ");
  filter_action_dump(pStream, pRule->pAction);
  fprintf(pStream, "\n");
}

// ----- filter_dump ------------------------------------------------
/**
 *
 */
void filter_dump(FILE * pStream, SFilter * pFilter)
{
  int iIndex;

  if (pFilter != NULL)
    for (iIndex= 0; iIndex < pFilter->pSeqRules->iSize; iIndex++) {
      fprintf(pStream, "%u. ", iIndex);
      filter_rule_dump(pStream,
		       (SFilterRule *) pFilter->pSeqRules->ppItems[iIndex]);
    }
  fprintf(pStream, "default. any --> ACCEPT\n");
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
  pHashPathExpr = hash_init(uHashPathRegExSize, .5, filter_path_regex_compare, 
				filter_path_regex_destroy, 
				filter_path_regex_hash);
  paPathExpr = ptr_array_create(0, NULL, NULL);
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