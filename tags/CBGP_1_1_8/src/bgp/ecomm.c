// ==================================================================
// @(#)ecomm.c
//
// @author Bruno Quoitin (bqu@infonet.fundp.ac.be)
// @date 02/12/2002
// @lastdate 23/05/2003
// ==================================================================

#include <stdio.h>
#include <string.h>

#include <libgds/memory.h>

#include <bgp/ecomm.h>
#include <bgp/peer.h>

#define MAX_COMM 255

// ----- ecomm_val_create -------------------------------------------
/**
 *
 */
SECommunity * ecomm_val_create(unsigned char uIANAAuthority,
			       unsigned char uTransitive)
{
  SECommunity * pComm= (SECommunity *) MALLOC(sizeof(SECommunity));
  pComm->uIANAAuthority= uIANAAuthority & 0x01;
  pComm->uTransitive= uTransitive & 0x01;
  return pComm;
}

// ----- ecomm_val_destroy ------------------------------------------
/**
 *
 */
void ecomm_val_destroy(SECommunity ** ppComm)
{
  if (*ppComm != NULL) {
    FREE(*ppComm);
    *ppComm= NULL;
  }
}

// ----- ecomm_val_copy ---------------------------------------------
SECommunity * ecomm_val_copy(SECommunity * pComm)
{
  SECommunity * pNewComm= (SECommunity *) MALLOC(sizeof(SECommunity));
  memcpy(pNewComm, pComm, sizeof(SECommunity));
  return pNewComm;
}

// ----- ecomm_create -----------------------------------------------
/**
 *
 */
SECommunities * ecomm_create()
{
  SECommunities * pComms= (SECommunities *) MALLOC(sizeof(SECommunities));
  pComms->uNum= 0;
  return pComms;
}

// ----- ecomm_destroy ----------------------------------------------
/**
 *
 */
void ecomm_destroy(SECommunities ** ppComms)
{
  if (*ppComms != NULL) {
    FREE(*ppComms);
    *ppComms= NULL;
  }
}

// ----- ecomm_length -----------------------------------------------
/**
 *
 */
unsigned char ecomm_length(SECommunities * pComms)
{
  return pComms->uNum;
}

// ----- ecomm_get_index --------------------------------------------
/**
 *
 */
SECommunity * ecomm_get_index(SECommunities * pComms, int iIndex)
{
  if ((iIndex >= 0) && (iIndex < pComms->uNum))
    return &pComms->asComms[iIndex];
  else
    return NULL;
}

// ----- ecomm_copy -------------------------------------------------
/**
 *
 */
SECommunities * ecomm_copy(SECommunities * pComms)
{
  SECommunities * pNewComms=
    (SECommunities *) MALLOC(sizeof(SECommunities)+
			     sizeof(SECommunity)*pComms->uNum);
  memcpy(pNewComms, pComms, sizeof(SECommunities)+
	 sizeof(SECommunity)*pComms->uNum);
  return pNewComms;
}

// ----- ecomm_add --------------------------------------------------
/**
 *
 */
int ecomm_add(SECommunities ** ppComms, SECommunity * pComm)
{
  if ((*ppComms)->uNum < MAX_COMM) {
    (*ppComms)->uNum++;
    (*ppComms)=
      (SECommunities *) REALLOC(*ppComms, sizeof(SECommunities)+
				sizeof(SECommunity)*(*ppComms)->uNum);
    memcpy(&(*ppComms)->asComms[(*ppComms)->uNum-1], pComm,
	   sizeof(SECommunity));
    ecomm_val_destroy(&pComm);
    return 0;
  } else
    return -1;
}

// ----- ecomm_strip_non_transitive ---------------------------------
/**
 *
 */
void ecomm_strip_non_transitive(SECommunities ** ppComms)
{
  int iIndex;
  int iLastIndex= 0;

  if (*ppComms != NULL) {
    for (iIndex= 0; iIndex < (*ppComms)->uNum; iIndex++) {
      if ((*ppComms)->asComms[iIndex].uTransitive) {
	if (iIndex != iLastIndex)
	  memcpy(&(*ppComms)->asComms[iLastIndex],
		 &(*ppComms)->asComms[iIndex], sizeof(SECommunity));
	iLastIndex++;
      }
    }
    if ((*ppComms)->uNum != iLastIndex) {
      (*ppComms)->uNum= iLastIndex;
      if ((*ppComms)->uNum == 0) {
	FREE(*ppComms);
	*ppComms= NULL;
      } else
	*ppComms=
	  (SECommunities *) REALLOC(*ppComms,
				    (*ppComms)->uNum*sizeof(SECommunity));
    }
  }
}

// ----- ecomm_red_dump ---------------------------------------------
/**
 *
 */
void ecomm_red_dump(FILE * pStream, SECommunity * pComm)
{
  fprintf(pStream, "red ");
  switch ((pComm->uTypeLow >> 3) & 0x07) {
  case ECOMM_RED_ACTION_PREPEND:
    fprintf(pStream, "prepend %u", (pComm->uTypeLow & 0x07));
    break;
  case ECOMM_RED_ACTION_NO_EXPORT:
    fprintf(pStream, "no-export");
    break;
  case ECOMM_RED_ACTION_IGNORE:
    fprintf(pStream, "ignore");
    break;
  default:
    fprintf(pStream, "???");
  }
  fprintf(pStream, " to ");
  switch (pComm->auValue[0]) {
  case ECOMM_RED_FILTER_AS:
    fprintf(pStream, "AS%u", *(uint16_t *) &pComm->auValue[4]);
    break;
  case ECOMM_RED_FILTER_2AS:
    fprintf(pStream, "AS%u/%u", *(uint16_t *) &pComm->auValue[2],
	    *(uint16_t *) &pComm->auValue[4]);
    break;
  case ECOMM_RED_FILTER_CIDR:
    ip_prefix_dump(pStream, *(SPrefix *) &pComm->auValue[1]);
    break;
  case ECOMM_RED_FILTER_AS4:
    fprintf(pStream, "AS%u", *(uint32_t *) &pComm->auValue[2]);
    break;
  default:
    fprintf(pStream, "???");
  }
}

// ----- ecomm_val_dump ---------------------------------------------
/**
 *
 */
void ecomm_val_dump(FILE * pStream, SECommunity * pComm)
{
  switch (pComm->uTypeHigh) {
  case ECOMM_RED: ecomm_red_dump(pStream, pComm); break;
  default:
    fprintf(pStream, "???");
  }
}

// ----- ecomm_dump -------------------------------------------------
/**
 *
 */
void ecomm_dump(FILE * pStream, SECommunities * pComms)
{
  int iIndex;

  for (iIndex= 0; iIndex < pComms->uNum; iIndex++) {
    if (iIndex > 0)
      fprintf(pStream, " ");
    ecomm_val_dump(pStream, &pComms->asComms[iIndex]);
  }
}

// ----- ecomm_equals -----------------------------------------------
/**
 *
 */
int ecomm_equals(SECommunities * pCommunities1,
		 SECommunities * pCommunities2)
{
  if (pCommunities1 == pCommunities2)
    return 1;
  if (pCommunities1->uNum != pCommunities2->uNum)
    return 0;
  return (memcmp(pCommunities1->asComms,
		 pCommunities2->asComms,
		 pCommunities1->uNum*sizeof(SECommunity)) == 0);
}

// ----- ecomm_red_create -------------------------------------------
/**
 *
 */
SECommunity * ecomm_red_create_as(unsigned char uActionType,
			       unsigned char uActionParam,
			       uint16_t uAS)
{
  SECommunity * pComm= ecomm_val_create(0, 0);
  pComm->uTypeHigh= ECOMM_RED;
  pComm->uTypeLow= ((uActionType & 0x07) << 3) + (uActionParam & 0x07);
  pComm->auValue[0]= ECOMM_RED_FILTER_AS;
  memset(&pComm->auValue[1], 0, 3);
  memcpy(&pComm->auValue[4], &uAS, sizeof(uint16_t));
  return pComm;
}

// ----- ecomm_red_match --------------------------------------------
/**
 * 1 => Matches
 * 0 => Does not match
 */
int ecomm_red_match(SECommunity * pComm, SPeer * pPeer)
{
  LOG_DEBUG("ecomm_red_match(AS%u <=> AS%u)\n",
	    *((uint16_t *) &pComm->auValue[4]),
	    pPeer->uRemoteAS);
  
  switch (pComm->auValue[0]) {
  case ECOMM_RED_FILTER_AS:
    if (memcmp(&pComm->auValue[4], &pPeer->uRemoteAS, sizeof(uint16_t)) == 0)
      return 1;
    break;
  case ECOMM_RED_FILTER_2AS:
    break;
  case ECOMM_RED_FILTER_CIDR:
    break;
  case ECOMM_RED_FILTER_AS4:
    break;
  }
  return 0;
}
