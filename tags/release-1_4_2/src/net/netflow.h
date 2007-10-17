// ==================================================================
// @(#)netflow.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 14/05/2007
// @lastdate 15/05/2007
// ==================================================================

#ifndef __NET_NETFLOW_H__
#define __NET_NETFLOW_H__

#include <stdio.h>

#include <libgds/log.h>

#include <net/prefix.h>

// ----- Error codes -----
#define NETFLOW_SUCCESS                0
#define NETFLOW_ERROR_UNEXPECTED       -1
#define NETFLOW_ERROR_OPEN             -2
#define NETFLOW_ERROR_NUM_FIELDS       -3
#define NETFLOW_ERROR_INVALID_HEADER   -4
#define NETFLOW_ERROR_INVALID_SRC_ADDR -5
#define NETFLOW_ERROR_INVALID_DST_ADDR -6
#define NETFLOW_ERROR_INVALID_OCTETS   -7

// ----- Netflow record handler -----
typedef int (*FNetflowHandler)(net_addr_t tSrc, net_addr_t tDst,
			       unsigned int uOctets, void * pContext);

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ netflow_perror ]-----------------------------------------
  void netflow_perror(SLogStream * pStream, int iErrorCode);
  // -----[ netflow_load ]-------------------------------------------
  int netflow_load(const char * pcFileName, FNetflowHandler fHandler,
		   void * pContext);

#ifdef __cplusplus
}
#endif

#endif /* __NET_NETFLOW_H__ */
