// ==================================================================
// @(#)error.c
//
// List of error codes.
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 30/05/2007
// @lastdate 19/10/2007
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
  char * pcErrorStr= network_strerror(iErrorCode);
  if (pcErrorStr != NULL)
    log_printf(pStream, pcErrorStr);
  else
    log_printf(pStream, "unknown error (%i)", iErrorCode);    
}

// ----- network_perror ---------------------------------------------
char * network_strerror(int iErrorCode)
{
  switch (iErrorCode) {
  case NET_SUCCESS:
    return "success";
  case NET_ERROR_UNSUPPORTED:
    return "unexpected error";
  case NET_ERROR_NET_UNREACH:
    return "network unreachable";
  case NET_ERROR_HOST_UNREACH:
    return "host unreachable";
  case NET_ERROR_PROTO_UNREACH:
    return "protocol unreachable";
  case NET_ERROR_TIME_EXCEEDED:
    return "time exceeded";
  case NET_ERROR_ICMP_NET_UNREACH:
    return "icmp error (network-unreachable)";
  case NET_ERROR_ICMP_HOST_UNREACH:
    return "icmp error (host-unreachable)";
  case NET_ERROR_ICMP_PROTO_UNREACH:
    return "icmp error (proto-unreachable)";
  case NET_ERROR_ICMP_TIME_EXCEEDED:
    return "icmp error (time-exceeded)";
  case NET_ERROR_NO_REPLY:
    return "no reply";
  case NET_ERROR_LINK_DOWN:
    return "link down";
  case NET_ERROR_PROTOCOL_ERROR:
    return "protocol error";
  case NET_ERROR_IF_UNKNOWN:
    return "unknown interface";
  case NET_ERROR_MGMT_INVALID_NODE:
    return "invalid node";
  case NET_ERROR_MGMT_INVALID_LINK:
    return "invalid link";
  case NET_ERROR_MGMT_INVALID_SUBNET:
    return "invalid subnet";
  case NET_ERROR_MGMT_NODE_ALREADY_EXISTS:
    return "node already exists";
  case NET_ERROR_MGMT_SUBNET_ALREADY_EXISTS:
    return "subnet already exists";
  case NET_ERROR_MGMT_LINK_ALREADY_EXISTS:
    return "link already exists";
  case NET_ERROR_MGMT_LINK_LOOP:
    return "link endpoints are equal";
  case NET_ERROR_MGMT_INVALID_OPERATION:
    return "invalid operation";
  }
  return NULL;
}

