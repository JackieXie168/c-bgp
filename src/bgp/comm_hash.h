// ==================================================================
// @(#)comm_hash.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 14/10/2005
// @lastdate 14/10/2005
// ==================================================================

#ifndef __BGP_COMM_HASH_H__
#define __BGP_COMM_HASH_H__

#include <stdlib.h>

#include <bgp/comm_t.h>

#define COMM_HASH_METHOD_STRING 0
#define COMM_HASH_METHOD_ZEBRA 1

// -----[ comm_hash_add ]--------------------------------------------
extern int comm_hash_add(SCommunities * pCommunities);
// -----[ comm_hash_add ]--------------------------------------------
extern SCommunities * comm_hash_get(SCommunities * pCommunities);
// -----[ comm_hash_add ]--------------------------------------------
extern int comm_hash_remove(SCommunities * pCommunities);
// -----[ comm_hash_get_size ]---------------------------------------
extern uint32_t comm_hash_get_size();
// -----[ comm_hash_set_size ]---------------------------------------
extern int comm_hash_set_size(uint32_t uSize);
// -----[ path_hash_get_method ]-------------------------------------
extern uint8_t comm_hash_get_method();
// -----[ comm_hash_set_method ]-------------------------------------
extern int comm_hash_set_method(uint8_t uMethod);

// -----[ comm_hash_content ]----------------------------------------
extern void comm_hash_content(FILE * pStream);
// -----[ comm_hash_statistics ]-------------------------------------
extern void comm_hash_statistics(FILE * pStream);

// -----[ _comm_hash_init ]------------------------------------------
extern void _comm_hash_init();
// -----[ _comm_hash_destroy ]---------------------------------------
extern void _comm_hash_destroy();

#endif
