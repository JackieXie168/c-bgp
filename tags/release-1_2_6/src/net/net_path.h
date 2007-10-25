// ==================================================================
// @(#)net_path.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 09/07/2003
// @lastdate 03/03/2006
// ==================================================================

#ifndef __NET_PATH_H__
#define __NET_PATH_H__

#include <libgds/array.h>
#include <libgds/log.h>
#include <net/prefix.h>
#include <stdio.h>

typedef SUInt32Array SNetPath;

// ----- net_path_create --------------------------------------------
extern SNetPath * net_path_create();
// ----- path_destroy -------------------------------------------
extern void net_path_destroy(SNetPath ** ppPath);
// ----- path_append --------------------------------------------
extern int net_path_append(SNetPath * pPath, net_addr_t tAddr);
// ----- path_copy ----------------------------------------------
extern SNetPath * net_path_copy(SNetPath * pPath);
// ----- net_path_length --------------------------------------------
extern int net_path_length(SNetPath * pPath);
// ----- net_path_for_each ------------------------------------------
extern int net_path_for_each(SNetPath *pPath, FArrayForEach fForEach,
			     void * pContext);
// ----- path_dump ----------------------------------------------
extern void net_path_dump(SLogStream * pStream, SNetPath * pPath);
// ----- net_path_dump_string ---------------------------------------
char * net_path_dump_string(SNetPath * pPath);
// ----- net_path_search ---------------------------------------------
int net_path_search(SNetPath * pPath, net_addr_t tAddr);

#endif