// ==================================================================
// @(#)net_path.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 09/07/2003
// @lastdate 05/03/2004
// ==================================================================

#include <net/net_path.h>
#include <libgds/memory.h>
#include <string.h>

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

// ----- net_path_dump_string ----------------------------------------------
/**
 *
 */
char * net_path_dump_string(SNetPath * pPath)
{
  int iIndex;
  char * cPath = MALLOC(255), * cCharTmp;
  uint8_t icPathPtr = 0;

  for (iIndex= 0; iIndex < _array_length((SArray *) pPath); iIndex++) {
    if (iIndex > 0)
      strcpy(cPath+icPathPtr++, " ");
    cCharTmp = ip_address_dump_string(pPath->data[iIndex]);
    strcpy(cPath+icPathPtr, cCharTmp);
    icPathPtr += strlen(cCharTmp);
    FREE(cCharTmp);
  }
  return cPath;
}


// ----- net_path_dump ----------------------------------------------
/**
 *
 */
void net_path_dump(FILE * pStream, SNetPath * pPath)
{
  int iIndex;

  for (iIndex= 0; iIndex < _array_length((SArray *) pPath); iIndex++) {
    if (iIndex > 0)
      fprintf(pStream, " ");
    ip_address_dump(pStream, pPath->data[iIndex]);
  }
}


