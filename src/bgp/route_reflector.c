// ==================================================================
// @(#)route_reflector.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 23/12/2003
// @lastdate 28/09/2006
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <libgds/log.h>
#include <bgp/route_reflector.h>

#include <net/prefix.h>

// ----- cluster_list_destroy ---------------------------------------
void cluster_list_destroy(SClusterList ** ppClusterList)
{
  _array_destroy((SArray **) ppClusterList);
}

// ----- cluster_list_dump ------------------------------------------
/**
 * This function dumps the cluster-IDs contained in the given list to
 * the given stream.
 */
void cluster_list_dump(SLogStream * pStream, SClusterList * pClusterList)
{
  int iIndex;

  for (iIndex= 0; iIndex < _array_length((SArray *) pClusterList); iIndex++) {
    if (iIndex > 0)
      log_printf(pStream, " ");
    ip_address_dump(pStream, pClusterList->data[iIndex]);
  }
}

// ----- cluster_list_equals -------------------------------------------
/**
 * This function compares 2 cluster-ID-lists.
 *
 * Return value:
 *    1  if they are equal
 *    0  otherwise
 */
int cluster_list_equals(SClusterList * pClusterList1,
			SClusterList * pClusterList2)
{
  int iIndex;

  if (pClusterList1 == pClusterList2)
    return 1;
  if ((pClusterList1 == NULL) || (pClusterList2 == NULL))
    return 0;
  if (_array_length((SArray *) pClusterList1) !=
      _array_length((SArray *) pClusterList2))
    return 0;
  for (iIndex= 0; iIndex < _array_length((SArray *) pClusterList1); iIndex++)
    if (pClusterList1->data[iIndex] != pClusterList2->data[iIndex])
      return 0;
  return 1;
}

// ----- cluster_list_contains --------------------------------------
/**
 *
 */
int cluster_list_contains(SClusterList * pClusterList,
			  cluster_id_t tClusterID)
{
  int iIndex;

  /*
  ip_address_dump(stderr, tClusterID);
  fprintf(stderr, " in ");
  cluster_list_dump(stderr, pClusterList);
  fprintf(stderr, " ?\n");*/

  for (iIndex= 0; iIndex < _array_length((SArray *) pClusterList);
       iIndex++) {
    /*ip_address_dump(stderr, pClusterList->data[iIndex]);
      fprintf(stderr, "\n");*/
    if (pClusterList->data[iIndex] == tClusterID)
      return 1;
  }
  return 0;
}

// -----[ originator_equals ]----------------------------------------
/**
 * Test if two Originator-IDs are equal.
 *
 * Return value:
 *   1  if they are equal
 *   0  otherwise
 */
int originator_equals(net_addr_t * pOrig1, net_addr_t * pOrig2)
{  
  if (pOrig1 == pOrig2)
    return 1;
  if ((pOrig1 == NULL) ||
      (pOrig2 == NULL) ||
      (*pOrig1 != *pOrig2))
      return 0;
  return 1;
}
