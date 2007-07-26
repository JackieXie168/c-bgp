// ==================================================================
// @(#)path_hash.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 10/10/2005
// @lastdate 18/07/2007
// ==================================================================

#ifndef __BGP_PATH_HASH_H__
#define __BGP_PATH_HASH_H__

#include <libgds/log.h>
#include <stdlib.h>

#define PATH_HASH_METHOD_STRING 0
#define PATH_HASH_METHOD_ZEBRA  1
#define PATH_HASH_METHOD_OAT    2

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ path_hash_add ]------------------------------------------
  void * path_hash_add(SBGPPath * pPath);
  // -----[ path_hash_get ]------------------------------------------
  SBGPPath * path_hash_get(SBGPPath * pPath);
  // -----[ path_hash_remove ]---------------------------------------
  int path_hash_remove(SBGPPath * pPath);
  // -----[ path_hash_get_size ]-------------------------------------
  uint32_t path_hash_get_size();
  // -----[ path_hash_set_size ]-------------------------------------
  int path_hash_set_size(uint32_t uSize);
  // -----[ path_hash_get_method ]-----------------------------------
  uint8_t path_hash_get_method();
  // -----[ path_hash_set_method ]-----------------------------------
  int path_hash_set_method(uint8_t uMethod);
  
  // -----[ path_hash_content ]--------------------------------------
  void path_hash_content(SLogStream * pStream);
  // -----[ path_hash_statistics ]-----------------------------------
  void path_hash_statistics(SLogStream * pStream);
  
  // -----[ _path_hash_init ]----------------------------------------
  void _path_hash_init();
  // -----[ _path_hash_destroy ]-------------------------------------
  void _path_hash_destroy();

#ifdef __cplusplus
}
#endif

#endif /* __BGP_PATH_HASH_H__ */
