// ==================================================================
// @(#)net_path.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 09/07/2003
// $Id: net_path.h,v 1.8 2009-03-24 16:18:21 bqu Exp $
// ==================================================================

#ifndef __NET_PATH_H__
#define __NET_PATH_H__

#include <libgds/array.h>
#include <libgds/stream.h>
#include <net/prefix.h>
#include <stdio.h>

typedef uint32_array_t net_path_t;

#ifdef __cplusplus
extern "C" {
#endif

  // ----- net_path_create ------------------------------------------
  net_path_t * net_path_create();
  // ----- path_destroy ---------------------------------------------
  void net_path_destroy(net_path_t ** path_ref);
  // ----- path_append ----------------------------------------------
  int net_path_append(net_path_t * path, net_addr_t addr);
  // ----- path_copy ------------------------------------------------
  net_path_t * net_path_copy(net_path_t * path);
  // ----- net_path_length ------------------------------------------
  int net_path_length(net_path_t * path);
  // ----- net_path_for_each ----------------------------------------
  int net_path_for_each(net_path_t * path, gds_array_foreach_f foreach,
			void * ctx);
  // ----- path_dump ------------------------------------------------
  void net_path_dump(gds_stream_t * stream, net_path_t * path);
  // ----- net_path_search ------------------------------------------
  int net_path_search(net_path_t * path, net_addr_t addr);

#ifdef __cplusplus
}
#endif

#endif /* __NET_PATH_H__ */
