// ==================================================================
// @(#)route_reflector.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 23/12/2003
// @lastdate 27/01/2005
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <bgp/route_reflector.h>

#include <net/prefix.h>

// ----- cluster_list_dump ------------------------------------------
/**
 * This function dumps the cluster-IDs contained in the given list to
 * the given stream.
 */
void cluster_list_dump(FILE * pStream, SClusterList * pClusterList)
{
  int iIndex;

  for (iIndex= 0; iIndex < _array_length((SArray *) pClusterList); iIndex++) {
    ip_address_dump(pStream, pClusterList->data[iIndex]);
  }
}

// ----- cluster_list_cmp -------------------------------------------
/**
 * This function compares 2 cluster-ID-lists.
 *
 * If they differ, the function returns -1, otherwise, the function
 * returns 0.
 */
int cluster_list_cmp(SClusterList * pClusterList1,
		     SClusterList * pClusterList2)
{
  int iIndex;

  if (_array_length((SArray *) pClusterList1) !=
      _array_length((SArray *) pClusterList2))
    return -1;
  for (iIndex= 0; iIndex < _array_length((SArray *) pClusterList1); iIndex++)
    if (pClusterList1->data[iIndex] != pClusterList2->data[iIndex])
      return -1;
  return 0;
}

// ----- cluster_list_contains --------------------------------------
/**
 *
 */
int cluster_list_contains(SClusterList * pClusterList,
			  cluster_id_t tClusterID)
{
  int iIndex;
 
  for (iIndex= 0; iIndex < _array_length((SArray *) pClusterList);
       iIndex++)
    if (pClusterList->data[iIndex] == tClusterID)
      return 1;
  return 0;
}
