// ==================================================================
// @(#)path_segment.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 28/10/2003
// @lastdate 05/09/2007
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <libgds/log.h>
#include <libgds/memory.h>
#include <libgds/tokenizer.h>
#include <bgp/path.h>
#include <bgp/path_segment.h>

// -----[ path segment delimiters ]-----
/**
 * These are the delimiters used in route_btoa.
 */
#define PATH_SEG_SET_DELIMS            "[]"
/**
 * These are the delimiters used in libbgpdump.
 */
//#define PATH_SEG_SET_DELIMS            "{}"
//#define PATH_SEG_CONFED_SET_DELIM      "[]"
//#define PATH_SEG_CONFED_SEQUENCE_DELIM "()"

static STokenizer * pSegmentTokenizer= NULL;

// ----- path_segment_create ----------------------------------------
/**
 * Create an AS-Path segment of the given type and length.
 */
SPathSegment * path_segment_create(uint8_t uType, uint8_t uLength)
{
  SPathSegment * pSegment=
    (SPathSegment *) MALLOC(sizeof(SPathSegment)+
			     (uLength*sizeof(uint16_t)));
  pSegment->uType= uType;
  pSegment->uLength= uLength;
  return pSegment;
}

// ----- path_segment_destroy ---------------------------------------
/**
 * Destroy an AS-Path segment.
 */
void path_segment_destroy(SPathSegment ** ppSegment)
{
  if (*ppSegment != NULL) {
    FREE(*ppSegment);
    *ppSegment= NULL;
  }
}

// ----- path_segment_copy ------------------------------------------
/**
 * Duplicate an existing AS-Path segment.
 */
SPathSegment * path_segment_copy(SPathSegment * pSegment)
{
  SPathSegment * pNewSegment=
    path_segment_create(pSegment->uType, pSegment->uLength);

  memcpy(pNewSegment->auValue, pSegment->auValue,
	 pNewSegment->uLength*sizeof(uint16_t));
  return pNewSegment;
}

// -----[ path_segment_to_string ]-----------------------------------
/**
 * Convert the given AS-Path segment to a string. The string memory
 * MUST have been allocated before. The function will not write
 * outside of the allocated buffer, based on the provided destination
 * buffer size.
 *
 * Return value:
 *   -1    if the destination buffer is too small
 *   >= 0  otherwise (number of characters written without \0)
 *
 * Note: the function uses snprintf() in order to write into the
 * destination buffer. The return value of snprintf() is important. A
 * return value equal or larger that the maximum destination size
 *  means that the output was truncated.
 */
int path_segment_to_string(SPathSegment * pSegment,
			   uint8_t uReverse,
			   char * pcDst,
			   size_t tDstSize)
{
  size_t tInitialSize= tDstSize;
  int iIndex;
  int iWritten= 0;

  assert(((pSegment->uType == AS_PATH_SEGMENT_SET) ||
	  (pSegment->uType == AS_PATH_SEGMENT_SEQUENCE) ||
	  (pSegment->uType == AS_PATH_SEGMENT_CONFED_SET) ||
	  (pSegment->uType == AS_PATH_SEGMENT_CONFED_SEQUENCE)) &&
	 (pSegment->uLength > 0));

  if (pSegment->uType != AS_PATH_SEGMENT_SEQUENCE) {
    iWritten= snprintf(pcDst, tDstSize, "}");
    if (iWritten == tDstSize)
      return -1;
    tDstSize-= iWritten;
    pcDst+= iWritten;
  }

  if (uReverse) {
    for (iIndex= pSegment->uLength; iIndex > 0; iIndex--) {
      if (iIndex < pSegment->uLength)
	iWritten= snprintf(pcDst, tDstSize, " %u",
			   pSegment->auValue[iIndex-1]);
      else
	iWritten= snprintf(pcDst, tDstSize, "%u",
			   pSegment->auValue[iIndex-1]);	  
      if (iWritten >= tDstSize)
	return -1;
      tDstSize-= iWritten;
      pcDst+= iWritten;
    }
  } else {
    for (iIndex= 0; iIndex < pSegment->uLength; iIndex++) {
      if (iIndex > 0)
	iWritten= snprintf(pcDst, tDstSize, " %u",
			   pSegment->auValue[iIndex-1]);
      else
	iWritten= snprintf(pcDst, tDstSize, "%u",
			   pSegment->auValue[iIndex-1]);	  
      if (iWritten >= tDstSize)
	return -1;
      tDstSize-= iWritten;
      pcDst+= iWritten;
    }
  }

  if (pSegment->uType != AS_PATH_SEGMENT_SEQUENCE) {
    iWritten= snprintf(pcDst, tDstSize, "}");
    if (iWritten >= tDstSize)
      return -1;
    tDstSize-= iWritten;
    pcDst+= iWritten;
  }

  return tInitialSize-tDstSize;
}

// -----[ path_segment_from_string ]---------------------------------
/**
 * Convert a string to an AS-Path segment.
 *
 * Return value:
 *   NULL  if the string does not contain a valid AS-Path
 *   a new AS-Path segment otherwise
 */
SPathSegment * path_segment_from_string(const char * pcPathSegment)
{
  int iIndex;
  STokens * pTokens;
  SPathSegment * pSegment;
  unsigned long ulASNum;

  if (pSegmentTokenizer == NULL)
    pSegmentTokenizer= tokenizer_create(" ", 0, NULL, NULL);

  if (tokenizer_run(pSegmentTokenizer, (char *) pcPathSegment)
      != TOKENIZER_SUCCESS) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: parse error in 'path_segment_from_string'\n");
    return NULL;
  }

  pTokens= tokenizer_get_tokens(pSegmentTokenizer);
  pSegment= path_segment_create(AS_PATH_SEGMENT_SET, 0);
  for (iIndex= tokens_get_num(pTokens); iIndex > 0; iIndex--) {
    if (tokens_get_ulong_at(pTokens, iIndex-1, &ulASNum) || (ulASNum > 65535)) {
      LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid AS-Num \"%s\"\n",
		 tokens_get_string_at(pTokens, iIndex-1));
      path_segment_destroy(&pSegment);
      break;
    }
    path_segment_add(&pSegment, ulASNum);
  }
  return pSegment;
}

// ----- path_segment_dump_string ------------------------------------------
/**
 * Dump an AS-Path segment.
 */
char * path_segment_dump_string(SPathSegment * pSegment,
		       uint8_t uReverse)
{
  char * cSegment = MALLOC(255);
  uint8_t icSegPtr = 0;
  int iIndex;

  switch (pSegment->uType) {
  case AS_PATH_SEGMENT_SEQUENCE:
    if (uReverse) {
      for (iIndex= pSegment->uLength; iIndex > 0; iIndex--) {
	icSegPtr += sprintf(cSegment+icSegPtr, "%u", pSegment->auValue[iIndex-1]);
	if (iIndex > 1)
	  strcpy(cSegment+icSegPtr++, " ");
      }
    } else {
      for (iIndex= 0; iIndex < pSegment->uLength; iIndex++) {
	if (iIndex > 0)
	  strcpy(cSegment+icSegPtr++, " ");
	icSegPtr += sprintf(cSegment+icSegPtr, "%u", pSegment->auValue[iIndex]);
      }
    }
    break;
  case AS_PATH_SEGMENT_SET:
    strcpy(cSegment+icSegPtr++, "{");
    if (uReverse) {
      for (iIndex= pSegment->uLength; iIndex > 0; iIndex--) {
	icSegPtr += sprintf(cSegment+icSegPtr, "%u", pSegment->auValue[iIndex-1]);
	if (iIndex > 1)
	  strcpy(cSegment+icSegPtr++, " ");
      }
    } else {
      for (iIndex= 0; iIndex < pSegment->uLength; iIndex++) {
	if (iIndex > 0)
	  strcpy(cSegment+icSegPtr++, " ");
	icSegPtr += sprintf(cSegment+icSegPtr, "%u", pSegment->auValue[iIndex]);
      }
    }
    strcpy(cSegment+icSegPtr++, "}");
    break;
  default:
    abort();
  }
  return cSegment;
}


// ----- path_segment_dump ------------------------------------------
/**
 * Dump an AS-Path segment.
 */
void path_segment_dump(SLogStream * pStream, SPathSegment * pSegment,
		       uint8_t uReverse)
{
  int iIndex;

  switch (pSegment->uType) {
  case AS_PATH_SEGMENT_SEQUENCE:
    if (uReverse) {
      for (iIndex= pSegment->uLength; iIndex > 0; iIndex--) {
	log_printf(pStream, "%u", pSegment->auValue[iIndex-1]);
	if (iIndex > 1)
	  log_printf(pStream, " ");
      }
    } else {
      for (iIndex= 0; iIndex < pSegment->uLength; iIndex++) {
	if (iIndex > 0)
	  log_printf(pStream, " ");
	log_printf(pStream, "%u", pSegment->auValue[iIndex]);
      }
    }
    break;
  case AS_PATH_SEGMENT_SET:
    log_printf(pStream, "{");
    if (uReverse) {
      for (iIndex= pSegment->uLength; iIndex > 0; iIndex--) {
	log_printf(pStream, "%u", pSegment->auValue[iIndex-1]);
	if (iIndex > 1)
	  log_printf(pStream, " ");
      }
    } else {
      for (iIndex= 0; iIndex < pSegment->uLength; iIndex++) {
	if (iIndex > 0)
	  log_printf(pStream, " ");
	log_printf(pStream, "%u", pSegment->auValue[iIndex]);
      }
    }
    log_printf(pStream, "}");
    break;
  default:
    abort();
  }
}

// -----[ _path_segment_resize ]-------------------------------------
/**
 * Resizes an existing segment.
 */
static inline void _path_segment_resize(SPathSegment ** ppSegment,
					uint8_t uNewLength)
{
  if (uNewLength != (*ppSegment)->uLength) {
    *ppSegment= (SPathSegment *) REALLOC(*ppSegment,
					 sizeof(SPathSegment)+
					 uNewLength*sizeof(uint16_t));
    (*ppSegment)->uLength= uNewLength;
  }
}

// ----- path_segment_add -------------------------------------------
/**
 * Add a new AS number to the given segment. Since the size of the
 * segment will be changed by this function, the segment's location in
 * memory can change. Thus, the pointer to the segment can change !!!
 *
 * The segment's size is limited to 255 AS numbers. If the segment can
 * not hold a additional AS number, the function returns
 * -1. Otherwize, the new AS number is added and the function returns
 * 0.
 */
int path_segment_add(SPathSegment ** ppSegment, uint16_t uAS)
{
  if ((*ppSegment)->uLength < MAX_PATH_SEQUENCE_LENGTH) {
    _path_segment_resize(ppSegment, (*ppSegment)->uLength+1);
    (*ppSegment)->auValue[(*ppSegment)->uLength-1]= uAS;
    return 0;
  } else
    return -1;
}

// ----- path_segment_contains --------------------------------------
/**
 * Test if the given segment contains the given AS number.
 *
 * Return value:
 *   1  if the segment contains the ASN
 *   0  otherwise
 */
inline int path_segment_contains(SPathSegment * pSegment, uint16_t uAS)
{
  unsigned int uIndex;

  for (uIndex= 0; uIndex < pSegment->uLength; uIndex++) {
    if (pSegment->auValue[uIndex] == uAS)
      return 1;
  }

  return 0;
}

// ----- path_segment_equals ----------------------------------------
/**
 * Test if two segments are equal. If they are equal, the function
 * returns 1 otherwize, the function returns 0.
 */
inline int path_segment_equals(SPathSegment * pSegment1,
			       SPathSegment * pSegment2)
{
  int iIndex;

  if (pSegment1 == pSegment2)
    return 1;

  if ((pSegment1->uType != pSegment2->uType) ||
      (pSegment1->uLength != pSegment2->uLength))
    return 0;
  
  switch (pSegment1->uType) {
  case AS_PATH_SEGMENT_SEQUENCE:
    // In the case of an AS-SEQUENCE, we simply compare the memory
    // regions since the ordering of the AS numbers must be equal.
    if (memcmp(pSegment1->auValue, pSegment2->auValue,
	       pSegment1->uLength*sizeof(uint16_t)) == 0)
      return 1;
    break;
  case AS_PATH_SEGMENT_SET:
    // In the case of an AS-SET, there is no ordering of the AS
    // numbers. We must thus check if each element of the first set is
    // in the second set. This is sufficient since we have already
    // compared the segment lengths.
    for (iIndex= 0; iIndex < pSegment1->uLength; iIndex++)
      if (path_segment_contains(pSegment2,
				pSegment1->auValue[iIndex]) < -1)
	return 0;
    return 1;
    break;
  default:
    abort();
  }
  return 0;
}

// -----[ path_segment_cmp ]-----------------------------------------
/**
 * Compare two path segments. This function is aimed at sorting path
 * segments, not at measuring their length in the decision process !
 *
 * Pre-condition: path segments must not be NULL.
 *
 * Return value:
 *   -1  if seg1 < seg2
 *   0   if seg1 == seg2
 *   1   if seg1 > seg2
 *
 * Note: in the case of segments of type SEQUENCE, two segments with
 *       the same content might be considered different when their
 *       content is ordered differently.
 */
int path_segment_cmp(SPathSegment * pSeg1, SPathSegment * pSeg2)
{
  unsigned int uIndex;

  assert((pSeg1 != NULL) && (pSeg2 != NULL));

  // Longest is first
  if (pSeg1->uLength < pSeg2->uLength)
    return -1;
  else if (pSeg1->uLength > pSeg2->uLength)
    return 1;

  // Equal size segments, compare each AS number individually
  for (uIndex= 0; uIndex < pSeg1->uLength; uIndex++) {
    if (pSeg1->auValue[uIndex] < pSeg2->auValue[uIndex])
      return -1;
    else if (pSeg1->auValue[uIndex] > pSeg2->auValue[uIndex])
      return 1;
  }

  return 0;
}

// -----[ _path_segment_destroy ]------------------------------------
void _path_segment_destroy()
{
  tokenizer_destroy(&pSegmentTokenizer);
}
