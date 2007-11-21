// ==================================================================
// @(#)tm.h
//
// Traffic matrix management.
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 23/01/2007
// @lastdate 20/11/2007
// ==================================================================

#ifndef __NET_TM_H__
#define __NET_TM_H__

#include <libgds/log.h>

#define NET_TM_SUCCESS            0
#define NET_TM_ERROR_UNEXPECTED   -1
#define NET_TM_ERROR_OPEN         -2
#define NET_TM_ERROR_NUM_PARAMS   -3
#define NET_TM_ERROR_INVALID_SRC  -4
#define NET_TM_ERROR_UNKNOWN_SRC  -5
#define NET_TM_ERROR_INVALID_DST  -6
#define NET_TM_ERROR_INVALID_LOAD -7

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ net_tm_strerror ]----------------------------------------
  char * net_tm_strerror(int iError);
  // -----[ net_tm_perror ]------------------------------------------
  void net_tm_perror(SLogStream * pStream, int iError);
  // -----[ net_tm_parser ]------------------------------------------
  int net_tm_parser(FILE * pStream);
  // -----[ net_tm_load ]--------------------------------------------
  int net_tm_load(const char * pcFileName);

#ifdef __cplusplus
}
#endif

#endif /* __NET_TM_H__ */
