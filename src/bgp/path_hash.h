// ==================================================================
// @(#)path_hash.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 10/10/2005
// @lastdate 17/10/2005
// ==================================================================

#ifndef __BGP_PATH_HASH_H__
#define __BGP_PATH_HASH_H__

#include <stdlib.h>

#define PATH_HASH_METHOD_STRING 0
#define PATH_HASH_METHOD_ZEBRA 1

// -----[ path_hash_add ]--------------------------------------------
extern int path_hash_add(SBGPPath * pPath);
// -----[ path_hash_add ]--------------------------------------------
extern SBGPPath * path_hash_get(SBGPPath * pPath);
// -----[ path_hash_add ]--------------------------------------------
extern int path_hash_remove(SBGPPath * pPath);
// -----[ path_hash_get_size ]---------------------------------------
extern uint32_t path_hash_get_size();
// -----[ path_hash_set_size ]---------------------------------------
extern int path_hash_set_size(uint32_t uSize);
// -----[ path_hash_get_method ]-------------------------------------
extern uint8_t path_hash_get_method();
// -----[ path_hash_set_method ]-------------------------------------
extern int path_hash_set_method(uint8_t uMethod);

// -----[ path_hash_content ]----------------------------------------
extern void path_hash_content(FILE * pStream);
// -----[ path_hash_statistics ]-------------------------------------
extern void path_hash_statistics(FILE * pStream);

// -----[ _path_hash_init ]------------------------------------------
extern void _path_hash_init();
// -----[ _path_hash_destroy ]---------------------------------------
extern void _path_hash_destroy();

#endif
