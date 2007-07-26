// ==================================================================
// @(#)comm.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 23/05/2003
// @lastdate 22/07/2007
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <libgds/memory.h>
#include <libgds/str_util.h>
#include <libgds/tokenizer.h>

#include <stdlib.h>
#include <string.h>

#include <comm.h>

static STokenizer * pCommTokenizer= NULL;

// ----- comm_create ------------------------------------------------
/**
 *
 */
SCommunities * comm_create()
{
  SCommunities * pComms= (SCommunities*) MALLOC(sizeof(SCommunities));
  pComms->uNum= 0;
  return pComms;
}

// ----- comm_destroy -----------------------------------------------
/**
 *
 */
void comm_destroy(SCommunities ** ppComms)
{
  if (*ppComms != NULL) {
    FREE(*ppComms);
    *ppComms= NULL;
  }
}

// ----[ comm_copy ]-------------------------------------------------
/**
 *
 */
SCommunities * comm_copy(SCommunities * pComms)
{
  SCommunities * pNewComms=
    (SCommunities *) MALLOC(sizeof(SCommunities)+
			     sizeof(comm_t)*pComms->uNum);
  memcpy(pNewComms, pComms, sizeof(SCommunities)+
	 sizeof(comm_t)*pComms->uNum);
  return pNewComms;
}

// ----- comm_add ---------------------------------------------------
/**
 * Add a community value to the given Communities attribute.
 *
 * Return value:
 *    0   in case of success
 *   -1   in case of failure
 */
int comm_add(SCommunities ** ppComms, comm_t tComm)
{
  SCommunities * pComms=
    (SCommunities *) REALLOC(*ppComms, sizeof(SCommunities)+
			     sizeof(comm_t)*((*ppComms)->uNum+1));
  if (pComms == NULL)
    return -1;
  (*ppComms)= pComms;
  (*ppComms)->uNum++;
  (*ppComms)->asComms[(*ppComms)->uNum-1]= tComm;
  return 0;
}

// ----- comm_remove ------------------------------------------------
/**
 * Remove a given community value from the set.
 */
void comm_remove(SCommunities ** ppComms, comm_t tComm)
{
  int iIndex;
  int iLastIndex= 0;

  if (*ppComms != NULL) {
    /* Remove the given community value. */
    for (iIndex= 0; iIndex < (*ppComms)->uNum; iIndex++) {
      if ((*ppComms)->asComms[iIndex] != tComm) {
	if (iIndex != iLastIndex)
	  memcpy(&(*ppComms)->asComms[iLastIndex],
		 &(*ppComms)->asComms[iIndex], sizeof(comm_t));
	iLastIndex++;
      }
    }
    /* destroy or resize the set of communities, if needed. */
    if ((*ppComms)->uNum != iLastIndex) {
      (*ppComms)->uNum= iLastIndex;
      if ((*ppComms)->uNum == 0) {
	FREE(*ppComms);
	*ppComms= NULL;
      } else
	*ppComms=
	  (SCommunities *) REALLOC(*ppComms,
				   (*ppComms)->uNum*sizeof(comm_t));
    }
  }
}

// ----- comm_contains ----------------------------------------------
/**
 * Check if a given community value belongs to the communities
 * attribute.
 *
 * Return value:
 *   1  the community was found and return value is the index.
 *   0  otherwise
 */
int comm_contains(SCommunities * pComms, comm_t tComm)
{
  int iIndex= 0;

  if (pComms == NULL)
    return 0;

  while (iIndex < pComms->uNum) {
    if (pComms->asComms[iIndex] == tComm)
      return 1;
    iIndex++;
  }
  return 0;
}

// ----- comm_equals ------------------------------------------------
/**
 *
 */
int comm_equals(SCommunities * pComms1, SCommunities * pComms2)
{
  if (pComms1 == pComms2)
    return 1;
  if ((pComms1 == NULL) || (pComms2 == NULL))
    return 0;
  if (pComms1->uNum != pComms2->uNum)
    return 0;
  return (memcmp(pComms1->asComms, pComms2->asComms,
		 pComms1->uNum*sizeof(comm_t)) == 0);
}

// -----[ comm_value_from_string ]-----------------------------------
/**
 *
 */
int comm_value_from_string(char * pcComm, comm_t * pCommunity)
{
  unsigned long ulComm;
  char * pcComm2;

  if (!strcmp(pcComm, "no-export")) {
    *pCommunity= COMM_NO_EXPORT;
  } else if (!strcmp(pcComm, "no-advertise")) {
    *pCommunity= COMM_NO_ADVERTISE;
#ifdef __EXPERIMENTAL__
  } else if (!strcmp(pcComm, "depref")) {
    *pCommunity= COMM_DEPREF;
#endif
  } else {

    if (strchr(pcComm, ':') == NULL) {

      // Single part (0-UINT32_MAX)
      if ((str_as_ulong(pcComm, &ulComm) < 0) ||
	  (ulComm > UINT32_MAX))
	return -1;

      *pCommunity= ulComm;

    } else {
      
      // Two parts (0-65535):(0-65535)
      ulComm= strtoul(pcComm, &pcComm2, 0);
      if (*pcComm2 != ':')
	return -1;
      if (ulComm > 65535)
	return -1;
      *pCommunity= ulComm << 16;
      ulComm= strtoul(pcComm2+1, &pcComm2, 0);
      if (*pcComm2 != '\0')
	return -1;
      if (ulComm > 65535)
	return -1;
      *pCommunity+= ulComm;

    }
  }
  return 0;
}

// -----[ comm_value_dump ]------------------------------------------
/**
 *
 */
void comm_value_dump(SLogStream * pStream, comm_t tCommunity, int iText)
{
  if (iText == COMM_DUMP_TEXT) {
    switch (tCommunity) {
    case COMM_NO_EXPORT:
      log_printf(pStream, "no-export");
      break;
    case COMM_NO_ADVERTISE:
      log_printf(pStream, "no-advertise");
      break;
    default:
      log_printf(pStream, "%u:%u", (tCommunity >> 16), (tCommunity & 65535));
    }
  } else {
    log_printf(pStream, "%u:%u", (tCommunity >> 16), (tCommunity & 65535));
    //log_printf(pStream, "%u", tCommunity);
  }
}

// ----- comm_dump --------------------------------------------------
/*
 *
 */
void comm_dump(SLogStream * pStream, SCommunities * pComms,
	       int iText)
{
  int iIndex;
  comm_t tCommunity;

  for (iIndex= 0; iIndex < pComms->uNum; iIndex++) {
    if (iIndex > 0)
      log_printf(pStream, " ");
    tCommunity= (comm_t) pComms->asComms[iIndex];
    comm_value_dump(pStream, tCommunity, iText);
  }
}

// -----[ comm_to_string ]-------------------------------------------
/**
 * Convert the given Communities to a string. The string memory MUST
 * have been allocated before. The function will not write outside of
 * the allocated buffer, based on the provided destination buffer
 * size.
 *
 * Return value:
 *   -1    not enough space in destination buffer
 *   >= 0  number of characters written
 */
int comm_to_string(SCommunities * pComms, char * pBuffer,
		   size_t tBufferSize)
{
  size_t tInitialSize= tBufferSize;
  int iWritten= 0;
  unsigned int uIndex;
  comm_t tCommunity;

  if ((pComms == NULL) || (pComms->uNum == 0)) {
    if (tBufferSize < 1)
      return -1;
    pBuffer[0]= '\0';
    return 0;
  }
 
  for (uIndex= 0; uIndex < pComms->uNum; uIndex++) {
    tCommunity= (comm_t) pComms->asComms[uIndex];

    if (uIndex > 0) {
      iWritten= snprintf(pBuffer, tBufferSize, " %u", (uint32_t) tCommunity);
    } else {
      iWritten= snprintf(pBuffer, tBufferSize, "%u", (uint32_t) tCommunity);
    }
    if (iWritten >= tBufferSize)
      return -1;
    tBufferSize-= iWritten;
    pBuffer+= iWritten;
  }
  return tInitialSize-tBufferSize;
}

// -----[ comm_from_string ]-----------------------------------------
/**
 *
 */
SCommunities * comm_from_string(const char * pcCommunities)
{
  int iIndex;
  SCommunities * pComm= NULL;
  STokens * pTokens;
  comm_t uComm;

  if (pCommTokenizer == NULL)
    pCommTokenizer= tokenizer_create(" ", 0, NULL, NULL);

  if (tokenizer_run(pCommTokenizer, (char *) pcCommunities)
      != TOKENIZER_SUCCESS)
    return NULL;

  pTokens= tokenizer_get_tokens(pCommTokenizer);

  pComm= comm_create();
  for (iIndex= 0; iIndex < tokens_get_num(pTokens); iIndex++) {
    if (comm_value_from_string(tokens_get_string_at(pTokens, iIndex),
			       &uComm)) {
      comm_destroy(&pComm);
      return NULL;
    }
    comm_add(&pComm, uComm);
  }
  
  return pComm;
}

// -----[ comm_cmp ]-------------------------------------------------
/**
 * Compare two Communities. This is just used for storing different
 * communities in a sorted array, not for testing that two Communities
 * are equal. Indeed, individual community values are not sorted in the
 * Communities which means that this function can say that two
 * Communities are different even if they contain the same set of
 * community values, but in different orderings.
 *
 * Return value:
 *   0  communities are equal
 *   <0 comm1 < comm2
 *   >0 comm1 > comm2
 */
int comm_cmp(SCommunities * pComm1, SCommunities * pComm2)
{
  unsigned int uIndex;

  // Pointers are equal means Communities are equal
  if (pComm1 == pComm2)
    return 0;

  // Empty and null Communities are equal
  if (((pComm1 == NULL) && (pComm2->uNum == 0)) ||
      ((pComm2 == NULL) && (pComm1->uNum == 0)))
    return 0;

  if (pComm1 == NULL)
    return -1;
  if (pComm2 == NULL)
    return 1;

  // Check Communities length
  if (pComm1->uNum < pComm2->uNum)
    return -1;
  else if (pComm1->uNum > pComm2->uNum)
    return 1;

  // Check individual community values
  for (uIndex= 0; uIndex < pComm1->uNum;uIndex++) {
    if (pComm1->asComms[uIndex] < pComm2->asComms[uIndex])
      return -1;
    else if (pComm1->asComms[uIndex] > pComm2->asComms[uIndex])
      return 1;
  }

  // Both Communities are equal
  return 0;
}

// -----[ _comm_destroy ]--------------------------------------------
void _comm_destroy()
{
  tokenizer_destroy(&pCommTokenizer);
}
