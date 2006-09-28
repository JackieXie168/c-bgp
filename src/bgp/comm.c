// ==================================================================
// @(#)comm.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 23/05/2003
// @lastdate 28/09/2006
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <libgds/memory.h>

#include <stdlib.h>
#include <string.h>

#include <comm.h>

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
 *   >= 0   the community was found and return value is the index.
 *   -1     the given community could not be found.
 */
int comm_contains(SCommunities * pComms, comm_t tComm)
{
  int iIndex= 0;

  while (iIndex < pComms->uNum) {
    if (pComms->asComms[iIndex] == tComm)
      return iIndex;
    iIndex++;
  }
  return -1;
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

// ----- comm_from_string -------------------------------------------
/**
 *
 */
int comm_from_string(char * pcComm, comm_t * pCommunity)
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
    *pCommunity= 0;
    ulComm= strtoul(pcComm, &pcComm2, 0);
    if (*pcComm2 == ':') {
      if (ulComm > 65535)
	return -1;
      *pCommunity= ulComm << 16;
      ulComm= strtoul(pcComm2+1, &pcComm2, 0);
    }
    if (*pcComm2 != 0)
      return -1;
    if (ulComm > 65535)
      return -1;
    *pCommunity+= ulComm;
  }
  return 0;
}

// ----- comm_dump2 -------------------------------------------------
/**
 *
 */
void comm_dump2(SLogStream * pStream, comm_t tCommunity, int iText)
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
    comm_dump2(pStream, tCommunity, iText);
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
 *   The function returns a valid pointer if the Communities could be
 *   written in the provided buffer. The returned pointer is the first
 *   character that follows the conversion. If there was not enough
 *   space in the buffer to write the Communities, then NULL is
 *   returned.
 */
int comm_to_string(SCommunities * pComms, char * pBuffer,
		   size_t tBufferSize)
{
  size_t tInitialSize= tBufferSize;
  int iWritten;
  int iIndex;
  comm_t tCommunity;
  
  for (iIndex= 0; iIndex < pComms->uNum; iIndex++) {
    tCommunity= (comm_t) pComms->asComms[iIndex];
    iWritten= snprintf(pBuffer, tBufferSize, "%ul", (unsigned int) tCommunity);
    if (iWritten == tBufferSize)
      return tInitialSize;
    tBufferSize-= iWritten;
    pBuffer+= iWritten;
  }
  return tInitialSize-tBufferSize;
}
