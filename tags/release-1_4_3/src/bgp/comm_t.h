// ==================================================================
// @(#)comm_t.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 23/05/2003
// @lastdate 19/07/2007
// ==================================================================

#ifndef __BGP_COMM_T_H__
#define __BGP_COMM_T_H__

#include <libgds/types.h>

#define COMM_NO_EXPORT 0xffffff01
#define COMM_NO_ADVERTISE 0xffffff02
#define COMM_NO_EXPORT_SUBCONFED 0xffffff03
#ifdef __EXPERIMENTAL__
#define COMM_DEPREF 0xfffffff0
#endif

#define COMM_DUMP_RAW  0
#define COMM_DUMP_TEXT 1

typedef uint32_t comm_t;

typedef struct {
  unsigned char uNum;
  comm_t asComms[0];
} SCommunities;

#endif /* __BGP_COMM_T_H__ */
