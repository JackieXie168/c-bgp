// ==================================================================
// @(#)comm_hash.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 14/10/2005
// $Id: comm_hash.h,v 1.4 2008-06-12 09:44:51 bqu Exp $
// ==================================================================

#ifndef __BGP_COMM_HASH_H__
#define __BGP_COMM_HASH_H__

#include <stdlib.h>
#include <libgds/log.h>

#include <bgp/comm_t.h>

#define COMM_HASH_METHOD_STRING 0
#define COMM_HASH_METHOD_ZEBRA 1

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ comm_hash_add ]------------------------------------------
  void * comm_hash_add(SCommunities * pCommunities);
  // -----[ comm_hash_get ]------------------------------------------
  SCommunities * comm_hash_get(SCommunities * pCommunities);
  // -----[ comm_hash_remove ]---------------------------------------
  int comm_hash_remove(SCommunities * pCommunities);
  // -----[ comm_hash_refcnt ]---------------------------------------
  uint32_t comm_hash_refcnt(SCommunities * pCommunities);

  // -----[ comm_hash_get_size ]-------------------------------------
  uint32_t comm_hash_get_size();
  // -----[ comm_hash_set_size ]-------------------------------------
  int comm_hash_set_size(uint32_t uSize);
  // -----[ path_hash_get_method ]-----------------------------------
  uint8_t comm_hash_get_method();
  // -----[ comm_hash_set_method ]-----------------------------------
  int comm_hash_set_method(uint8_t uMethod);
  
  // -----[ comm_hash_content ]--------------------------------------
  void comm_hash_content(SLogStream * pStream);
  // -----[ comm_hash_statistics ]-----------------------------------
  void comm_hash_statistics(SLogStream * pStream);
  
  // -----[ _comm_hash_init ]----------------------------------------
  void _comm_hash_init();
  // -----[ _comm_hash_destroy ]-------------------------------------
  void _comm_hash_destroy();

#ifdef __cplusplus
}
#endif

#endif /* __BGP_COMM_HASH_H__ */
