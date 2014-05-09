// ==================================================================
// @(#)comm_hash.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 14/10/2005
// $Id: comm_hash.h,v 1.1 2009-03-24 13:41:26 bqu Exp $
// ==================================================================

#ifndef __BGP_COMM_HASH_H__
#define __BGP_COMM_HASH_H__

#include <stdlib.h>
#include <libgds/stream.h>

#include <bgp/attr/types.h>

#define COMM_HASH_METHOD_STRING 0
#define COMM_HASH_METHOD_ZEBRA 1

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ comm_hash_add ]------------------------------------------
  void * comm_hash_add(bgp_comms_t * comms);
  // -----[ comm_hash_get ]------------------------------------------
  bgp_comms_t * comm_hash_get(bgp_comms_t * comms);
  // -----[ comm_hash_remove ]---------------------------------------
  int comm_hash_remove(bgp_comms_t * comms);
  // -----[ comm_hash_refcnt ]---------------------------------------
  unsigned int comm_hash_refcnt(bgp_comms_t * comms);

  // -----[ comm_hash_get_size ]-------------------------------------
  unsigned int comm_hash_get_size();
  // -----[ comm_hash_set_size ]-------------------------------------
  int comm_hash_set_size(unsigned int size);
  // -----[ path_hash_get_method ]-----------------------------------
  uint8_t comm_hash_get_method();
  // -----[ comm_hash_set_method ]-----------------------------------
  int comm_hash_set_method(uint8_t method);
  
  // -----[ comm_hash_content ]--------------------------------------
  void comm_hash_content(gds_stream_t * stream);
  // -----[ comm_hash_statistics ]-----------------------------------
  void comm_hash_statistics(gds_stream_t * stream);
  
  // -----[ _comm_hash_init ]----------------------------------------
  void _comm_hash_init();
  // -----[ _comm_hash_destroy ]-------------------------------------
  void _comm_hash_destroy();

#ifdef __cplusplus
}
#endif

#endif /* __BGP_COMM_HASH_H__ */
