// ==================================================================
// @(#)tie_breaks.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 28/07/2003
// @lastdate 04/11/2003
// ==================================================================
// These tie-break functions MUST satisfy the following constraints:
// * return (1) if ROUTE1 is prefered over ROUTE2
// * return (-1) if ROUTE2 is prefered over ROUTE1
// * return (0) if the function can not decide
//
// The later case SHOULD NOT occur since non-determinism is then
// introduced. The decision process WILL ABORT the simulator in
// case the tie-break function can not decide.
// ==================================================================

#include <assert.h>

#include <libgds/log.h>

#include <bgp/path.h>
#include <bgp/path_segment.h>
#include <bgp/tie_breaks.h>

// ----- tie_break_router_id ----------------------------------------
/**
 * This function compares the router-id of the sender of the
 * routes. The route learned from the sender with the lowest router-id
 * is prefered.
 */
int tie_break_router_id(SRoute * pRoute1, SRoute * pRoute2)
{
  if (pRoute1->tNextHop < pRoute2->tNextHop)
    return 1;
  else if (pRoute1->tNextHop > pRoute2->tNextHop)
    return -1;

  return 0;
}

// ----- tie_break_hash ---------------------------------------------
/**
 * This function computes hashes of the paths contained in both routes
 * and decides on this basis which route is prefered. If the hash
 * based decision is not sufficient then, the function tries to find
 * the first
 *
 * PRE-CONDITION: Paths should have the same length.
 */
int tie_break_hash(SRoute * pRoute1, SRoute * pRoute2)
{
  int iHash1= path_hash(pRoute1->pASPath);
  int iHash2= path_hash(pRoute2->pASPath);
  int iIndex1, iIndex2, iSegIndex1, iSegIndex2;
  uint16_t uASNumber1, uASNumber2;
  SPathSegment * pSegment1= NULL, * pSegment2= NULL;

  // Compare hashes
  if (iHash1 > iHash2)
    return 1;
  else if (iHash1 < iHash2)
    return -1;

  // If hashes are equal, find the first different segment in each path
  // and prefer the longuest onlowest-one (hyp: length of AS-Paths
  // should be equal, but a little assert can avoid bad surprises)
  assert(path_length(pRoute1->pASPath) == path_length(pRoute2->pASPath));

  // The following code finds the first position in both paths for
  // which the corresponding AS differ. The route for which this AS
  // number is the lowest is prefered.
  // For efficiency reasons, this code directly accesses the internal
  // structure of the AS-Paths.

  iIndex1= 0;
  iSegIndex1= 0;
  iIndex2= 0;
  iSegIndex2= 0;

  while ((iIndex1 < path_num_segments(pRoute1->pASPath)) &&
	 (iIndex2 < path_num_segments(pRoute2->pASPath))) {
    if (pSegment1 == NULL)
      pSegment1= (SPathSegment *) pRoute1->pASPath->data[iIndex1];
    if (pSegment2 == NULL)
      pSegment2= (SPathSegment *) pRoute2->pASPath->data[iIndex2];
    if (iSegIndex1 >= pSegment1->uLength) {
      iIndex1++;
      if (iIndex1 >= path_num_segments(pRoute1->pASPath))
	break;
      pSegment1= pRoute1->pASPath->data[iIndex1];
      iSegIndex1= 0;
    }
    if (iSegIndex2 >= pSegment2->uLength) {
      iIndex2++;
      if (iIndex2 >= path_num_segments(pRoute2->pASPath))
	break;
      pSegment2= pRoute2->pASPath->data[iIndex2];
      iSegIndex2= 0;
    }
    switch (pSegment1->uType) {
    case AS_PATH_SEGMENT_SEQUENCE:
      uASNumber1= pSegment1->auValue[iSegIndex1];
      iSegIndex1++;
      break;
    case AS_PATH_SEGMENT_SET:
      uASNumber1= 65535;
      while (iSegIndex1 < pSegment1->uLength)
	if (pSegment1->auValue[iSegIndex1] < uASNumber1)
	  uASNumber1= pSegment1->auValue[iSegIndex1];
      break;
    default:
      abort();
    }
    switch (pSegment2->uType) {
    case AS_PATH_SEGMENT_SEQUENCE:
      uASNumber2= pSegment2->auValue[iSegIndex2];
      iSegIndex2++;
      break;
    case AS_PATH_SEGMENT_SET:
      uASNumber2= 65535;
      while (iSegIndex2 < pSegment2->uLength)
	if (pSegment2->auValue[iSegIndex2] < uASNumber2)
	  uASNumber2= pSegment2->auValue[iSegIndex2];
      break;
    default:
      abort();
    }
    if (uASNumber1 < uASNumber2)
      return 1;
    else if (uASNumber1 > uASNumber2)
      return -1;
  }

  // If the function was not able to decide on the basis of the above
  // rules, it relies on a final decision function which is based on
  // the router-id.
  return tie_break_router_id(pRoute1, pRoute2);
}

// ----- tie_break_low_ISP ------------------------------------------
/**
 *
 */
int tie_break_low_ISP(SRoute * pRoute1, SRoute * pRoute2)
{
  if ((path_length(pRoute1->pASPath) > 1) &&
      (path_length(pRoute2->pASPath) > 1)) {
    /*
    if (pRoute1->pASPath->asPath[1] < pRoute2->pASPath->asPath[1])
      return 1;
    else if (pRoute1->pASPath->asPath[1] > pRoute2->pASPath->asPath[1])
      return -1;
    */
  }
  return tie_break_hash(pRoute1, pRoute2);
}

// ----- tie_break_high_ISP -----------------------------------------
/**
 *
 */
int tie_break_high_ISP(SRoute * pRoute1, SRoute * pRoute2)
{
  if ((path_length(pRoute1->pASPath) > 1) &&
      (path_length(pRoute2->pASPath) > 1)) {
  /*
    if (pRoute1->pASPath->asPath[1] > pRoute2->pASPath->asPath[1])
      return 1;
    else if (pRoute1->pASPath->asPath[1] < pRoute2->pASPath->asPath[1])
      return -1;
  */
  }
  return tie_break_hash(pRoute1, pRoute2);
}
