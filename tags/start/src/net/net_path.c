// ==================================================================
// @(#)net_path.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 09/07/2003
// @lastdate 24/02/2004
// ==================================================================

#include <net/net_path.h>

// ----- net_path_create --------------------------------------------
/**
 *
 */
SNetPath * net_path_create()
{
  return (SNetPath *) uint32_array_create(0);
}

// ----- net_path_destroy -------------------------------------------
/**
 *
 */
void net_path_destroy(SNetPath ** ppPath)
{
  _array_destroy((SArray **) ppPath);
}

// ----- net_path_append --------------------------------------------
/**
 *
 */
int net_path_append(SNetPath * pPath, net_addr_t tAddr)
{
  return _array_append((SArray *) pPath, &tAddr);
}

// ----- net_path_copy ----------------------------------------------
/**
 *
 */
SNetPath * net_path_copy(SNetPath * pPath)
{
  return (SNetPath *) _array_copy((SArray *) pPath);
}

// ----- net_path_length --------------------------------------------
/**
 *
 */
int net_path_length(SNetPath * pPath)
{
  return _array_length((SArray *) pPath);
}

// ----- net_path_dump ----------------------------------------------
/**
 *
 */
void net_path_dump(FILE * pStream, SNetPath * pPath)
{
  int iIndex;

  for (iIndex= 0; iIndex < _array_length((SArray *) pPath); iIndex++) {
    if (iIndex == 0)
      fprintf(pStream, "%u", pPath->data[iIndex]);
    else
      fprintf(pStream, " %u", pPath->data[iIndex]);
  }
}


