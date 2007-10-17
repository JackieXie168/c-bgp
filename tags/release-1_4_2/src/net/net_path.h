// ==================================================================
// @(#)net_path.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 09/07/2003
// @lastdate 21/07/2007
// ==================================================================

#ifndef __NET_PATH_H__
#define __NET_PATH_H__

#include <libgds/array.h>
#include <libgds/log.h>
#include <net/prefix.h>
#include <stdio.h>

typedef SUInt32Array SNetPath;

#ifdef __cplusplus
extern "C" {
#endif

  // ----- net_path_create ------------------------------------------
  SNetPath * net_path_create();
  // ----- path_destroy ---------------------------------------------
  void net_path_destroy(SNetPath ** ppPath);
  // ----- path_append ----------------------------------------------
  int net_path_append(SNetPath * pPath, net_addr_t tAddr);
  // ----- path_copy ------------------------------------------------
  SNetPath * net_path_copy(SNetPath * pPath);
  // ----- net_path_length ------------------------------------------
  int net_path_length(SNetPath * pPath);
  // ----- net_path_for_each ----------------------------------------
  int net_path_for_each(SNetPath *pPath, FArrayForEach fForEach,
			void * pContext);
  // ----- path_dump ------------------------------------------------
  void net_path_dump(SLogStream * pStream, SNetPath * pPath);
  // ----- net_path_search ------------------------------------------
  int net_path_search(SNetPath * pPath, net_addr_t tAddr);

#ifdef __cplusplus
}
#endif

#endif /* __NET_PATH_H__ */
