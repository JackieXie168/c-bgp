// ==================================================================
// @(#)error.h
//
// List of error codes.
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 30/05/2007
// @lastdate 15/02/2008
// ==================================================================

#ifndef __NET_ERROR_H__
#define __NET_ERROR_H__

#include <libgds/log.h>

// -----[ Error codes ]----------------------------------------------
#define NET_SUCCESS                  0    /* No error */
#define NET_ERROR_UNEXPECTED         -1   /* Unexpected error (generic) */
#define NET_ERROR_UNSUPPORTED        -2   /* Unsupported feature */
#define NET_ERROR_NET_UNREACH        -3   /* Network unreachable */
#define NET_ERROR_HOST_UNREACH       -4   /* Host unreachable */
#define NET_ERROR_PROTO_UNREACH      -5   /* Protocol unreachable */
#define NET_ERROR_TIME_EXCEEDED      -6   /* Time exceeded (TTL = 0) */
#define NET_ERROR_ICMP_NET_UNREACH   -7   /* ICMP net-unreach received */
#define NET_ERROR_ICMP_HOST_UNREACH  -8   /* ICMP host-unreach received */
#define NET_ERROR_ICMP_PROTO_UNREACH -9   /* ICMP proto-unreach received */
#define NET_ERROR_ICMP_TIME_EXCEEDED -10  /* ICMP time-exceeded received */
#define NET_ERROR_LINK_DOWN          -11
#define NET_ERROR_PROTOCOL_ERROR     -12
#define NET_ERROR_IF_UNKNOWN         -13
#define NET_ERROR_NO_REPLY           -14
#define NET_ERROR_FWD_LOOP           -15

#define NET_ERROR_MGMT_INVALID_NODE          -100
#define NET_ERROR_MGMT_INVALID_LINK          -101
#define NET_ERROR_MGMT_INVALID_SUBNET        -102
#define NET_ERROR_MGMT_NODE_ALREADY_EXISTS   -110
#define NET_ERROR_MGMT_SUBNET_ALREADY_EXISTS -111
#define NET_ERROR_MGMT_LINK_ALREADY_EXISTS   -112
#define NET_ERROR_MGMT_LINK_LOOP             -113
#define NET_ERROR_MGMT_INVALID_OPERATION     -114
#define NET_ERROR_MGMT_TOO_MANY_PROTOCOLS    -115
#define NET_ERROR_MGMT_DUPLICATE_PROTOCOL    -116
#define NET_ERROR_MGMT_DUPLICATE_IFACE       -117
#define NET_ERROR_MGMT_INVALID_IFACE_ID      -118
#define NET_ERROR_MGMT_INVALID_IFACE_TYPE    -119
#define NET_ERROR_MGMT_IFACE_NOT_SUPPORTED   -120
#define NET_ERROR_MGMT_IFACE_INCOMPATIBLE    -121
#define NET_ERROR_MGMT_IFACE_CONNECTED       -122
#define NET_ERROR_MGMT_IFACE_NOT_CONNECTED   -123

#ifdef __cplusplus
extern "C" {
#endif

  // ----- network_perror -------------------------------------------
  void network_perror(SLogStream * pStream, int iErrorCode);
  // ----- network_strerror -----------------------------------------
  char * network_strerror(int iErrorCode);

#ifdef __cplusplus
}
#endif

#endif /* __NET_ERROR_H__ */
