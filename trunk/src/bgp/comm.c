// ==================================================================
// @(#)comm.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 23/05/2003
// @lastdate 04/01/2005
// ==================================================================

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

// ----- comm_add ---------------------------------------------------
/**
 *
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
void comm_dump2(FILE * pStream, comm_t tCommunity, int iText)
{
  if (iText == COMM_DUMP_TEXT) {
    switch (tCommunity) {
    case COMM_NO_EXPORT:
      fprintf(pStream, "no-export");
      break;
    case COMM_NO_ADVERTISE:
      fprintf(pStream, "no-advertise");
      break;
    default:
      fprintf(pStream, "%u:%u", (tCommunity >> 16), (tCommunity & 65535));
    }
  } else {
    fprintf(pStream, "%u", tCommunity);
  }
}

// ----- comm_dump --------------------------------------------------
/*
 *
 */
void comm_dump(FILE * pStream, SCommunities * pCommunities,
	       int iText)
{
  int iIndex;
  comm_t uCommunity;

  for (iIndex= 0; iIndex < pCommunities->iSize; iIndex++) {
    if (iIndex > 0)
      fprintf(pStream, " ");
    uCommunity= (uint32_t) pCommunities->ppItems[iIndex];
    comm_dump2(pStream, uCommunity, iText);
  }
}

