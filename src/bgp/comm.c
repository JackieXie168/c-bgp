// ==================================================================
// @(#)comm.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 23/05/2003
// @lastdate 03/03/2006
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <string.h>

#include <comm.h>

// ----- comm_create ------------------------------------------------
/**
 *
 */
SCommunities * comm_create()
{
  return (SCommunities *) sequence_create(NULL, NULL);
}

// ----- comm_destroy -----------------------------------------------
/**
 *
 */
void comm_destroy(SCommunities ** ppCommunities)
{
  sequence_destroy((SSequence **) ppCommunities);
}

// ----[ comm_copy ]-------------------------------------------------
/**
 *
 */
SCommunities * comm_copy(SCommunities * pCommunities)
{
  return sequence_copy(pCommunities, NULL);
}

// ----- comm_add ---------------------------------------------------
/**
 * Add a community value to the given Communities attribute.
 *
 * Return value:
 *   0    in case of success
 *   -1   in case of failure
 */
int comm_add(SCommunities * pCommunity, comm_t uCommunity)
{
  return sequence_add((SSequence *) pCommunity, (void *) uCommunity);
}

// ----- comm_remove ------------------------------------------------
/**
 *
 */
void comm_remove(SCommunities * pCommunities, comm_t uCommunity)
{
  sequence_remove(pCommunities, (void *) uCommunity);
}

// ----- comm_contains ----------------------------------------------
/**
 *
 */
int comm_contains(SCommunities * pCommunities, comm_t uCommunity)
{
  return sequence_find_index(pCommunities, (void *) uCommunity);
}

// ----- comm_equals ------------------------------------------------
/**
 *
 */
int comm_equals(SCommunities * pCommunities1, SCommunities * pCommunities2)
{
  if (pCommunities1 == pCommunities2)
    return 1;
  if ((pCommunities1 == NULL) || (pCommunities2 == NULL))
    return 0;
  if (pCommunities1->iSize != pCommunities2->iSize)
    return 0;
  return (memcmp(pCommunities1->ppItems,
		 pCommunities2->ppItems,
		 pCommunities1->iSize*sizeof(comm_t)) == 0);
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
void comm_dump(SLogStream * pStream, SCommunities * pCommunities,
	       int iText)
{
  int iIndex;
  comm_t tCommunity;

  for (iIndex= 0; iIndex < pCommunities->iSize; iIndex++) {
    if (iIndex > 0)
      log_printf(pStream, " ");
    tCommunity= (comm_t) pCommunities->ppItems[iIndex];
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
int comm_to_string(SCommunities * pCommunities, char * pBuffer,
		   size_t tBufferSize)
{
  size_t tInitialSize= tBufferSize;
  int iWritten;
  int iIndex;
  comm_t tCommunity;
  
  for (iIndex= 0; iIndex < pCommunities->iSize; iIndex++) {
    tCommunity= (comm_t) pCommunities->ppItems[iIndex];
    iWritten= snprintf(pBuffer, tBufferSize, "%ul", (unsigned int) tCommunity);
    if (iWritten == tBufferSize)
      return tInitialSize;
    tBufferSize-= iWritten;
    pBuffer+= iWritten;
  }
  return tInitialSize-tBufferSize;
}
