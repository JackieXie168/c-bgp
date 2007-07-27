// ==================================================================
// @(#)error.c
//
// List of error codes.
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 30/05/2007
// @lastdate 30/05/2007
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <net/error.h>

// ----- network_perror ---------------------------------------------
/**
 * Dump an error message for the given error code.
 */
void network_perror(SLogStream * pStream, int iErrorCode)
{
#define LOG(M) log_printf(pStream, M); break;
  switch (iErrorCode) {
  case NET_SUCCESS:
    LOG("success");
  case NET_ERROR_UNSUPPORTED:
    LOG("unexpected error");
  case NET_ERROR_NET_UNREACH:
    LOG("network unreachable");
  case NET_ERROR_HOST_UNREACH:
    LOG("host unreachable");
  case NET_ERROR_PROTO_UNREACH:
    LOG("protocol unreachable");
  case NET_ERROR_TIME_EXCEEDED:
    LOG("time exceeded");
  case NET_ERROR_ICMP_NET_UNREACH:
    LOG("icmp error (network-unreachable)");
  case NET_ERROR_ICMP_HOST_UNREACH:
    LOG("icmp error (host-unreachable)");
  case NET_ERROR_ICMP_PROTO_UNREACH:
    LOG("icmp error (proto-unreachable)");
  case NET_ERROR_ICMP_TIME_EXCEEDED:
    LOG("icmp error (time-exceeded)");
  case NET_ERROR_NO_REPLY:
    LOG("no reply");
  case NET_ERROR_LINK_DOWN:
    LOG("link down");
  case NET_ERROR_PROTOCOL_ERROR:
    LOG("protocol error");
  case NET_ERROR_IF_UNKNOWN:
    LOG("unknown interface");
  case NET_ERROR_MGMT_INVALID_NODE:
    LOG("invalid node");
  case NET_ERROR_MGMT_INVALID_LINK:
    LOG("invalid link");
  case NET_ERROR_MGMT_INVALID_SUBNET:
    LOG("invalid subnet");
  case NET_ERROR_MGMT_NODE_ALREADY_EXISTS:
    LOG("node already exists");
  case NET_ERROR_MGMT_LINK_ALREADY_EXISTS:
    LOG("link already exists");
  case NET_ERROR_MGMT_LINK_LOOP:
    LOG("link endpoints are equal");
  case NET_ERROR_MGMT_INVALID_OPERATION:
    LOG("invalid operation");
  default:
    log_printf(pStream, "unknown error (%i)", iErrorCode);
  }
#undef LOG
}

