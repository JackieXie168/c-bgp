// ==================================================================
// @(#)error.c
//
// List of error codes.
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 30/05/2007
// @lastdate 13/03/2008
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdarg.h>
#include <stdlib.h>

#include <net/error.h>

// ----- network_perror ---------------------------------------------
/**
 * Dump an error message for the given error code.
 */
void network_perror(SLogStream * pStream, int iErrorCode)
{
  const char * pcErrorStr= network_strerror(iErrorCode);
  if (pcErrorStr != NULL)
    log_printf(pStream, pcErrorStr);
  else
    log_printf(pStream, "unknown error (%i)", iErrorCode);    
}

// ----- network_strerror -------------------------------------------
const char * network_strerror(int iErrorCode)
{
  switch (iErrorCode) {
  case ESUCCESS:
    return "success";
  case EUNEXPECTED:
    return "unexpected error";
  case EUNSUPPORTED:
    return "not implemented";
  case EUNSPECIFIED:
    return "unspecified error";

  case ENET_NET_UNREACH:
    return "network unreachable";
  case ENET_HOST_UNREACH:
    return "host unreachable";
  case ENET_PROTO_UNREACH:
    return "protocol unreachable";
  case ENET_TIME_EXCEEDED:
    return "time exceeded";
  case ENET_ICMP_NET_UNREACH:
    return "icmp error (network-unreachable)";
  case ENET_ICMP_HOST_UNREACH:
    return "icmp error (host-unreachable)";
  case ENET_ICMP_PROTO_UNREACH:
    return "icmp error (proto-unreachable)";
  case ENET_ICMP_TIME_EXCEEDED:
    return "icmp error (time-exceeded)";
  case ENET_NO_REPLY:
    return "no reply";
  case ENET_LINK_DOWN:
    return "link down";
  case ENET_PROTOCOL_ERROR:
    return "protocol error";
  case ENET_NODE_DUPLICATE:
    return "node already exists";
  case ENET_SUBNET_DUPLICATE:
    return "subnet already exists";
  case ENET_LINK_DUPLICATE:
    return "link already exists";
  case ENET_LINK_LOOP:
    return "link endpoints are equal";
  case ENET_PROTO_UNKNOWN:
    return "invalid protocol ID";
  case ENET_PROTO_DUPLICATE:
    return "protocol already registered";
  case ENET_IFACE_UNKNOWN:
    return "unknown interface";
  case ENET_IFACE_DUPLICATE:
    return "interface already exists";
  case ENET_IFACE_INVALID_ID:
    return "invalid interface ID";
  case ENET_IFACE_INVALID_TYPE:
    return "invalid interface type";
  case ENET_IFACE_NOT_SUPPORTED:
    return "operation not supported by interface";
  case ENET_IFACE_INCOMPATIBLE:
    return "incompatible interface";
  case ENET_IFACE_CONNECTED:
    return "interface is connected";
  case ENET_IFACE_NOT_CONNECTED:
    return "interface is not connected";
  case EBGP_PEER_DUPLICATE:
    return "peer already exists";
  case EBGP_PEER_INVALID_ADDR:
    return "cannot add peer with this address";
  case EBGP_PEER_UNKNOWN:
    return "peer does not exist";
  case EBGP_PEER_INCOMPATIBLE:
    return "peer not compatible with operation";
  case EBGP_PEER_UNREACHABLE:
    return "peer could not be reached";
  case EBGP_PEER_INVALID_STATE:
    return "peer state incompatible with operation";
  case ENET_RT_NH_UNREACH:
    return "next-hop is unreachable";
  case ENET_RT_IFACE_UNKNOWN:
    return "interface is unknown";
  case ENET_RT_DUPLICATE:
    return "route already exists";
  case ENET_RT_UNKNOWN:
    return "route does not exist";
  case EBGP_NETWORK_DUPLICATE:
    return "network already exists";
  case ENET_NODE_INVALID_ID:
    return "invalid identifier";
  }
  return NULL;
}

// -----[ fatal ]----------------------------------------------------
void fatal(const char * msg, ...)
{
  va_list ap;

  va_start(ap, msg);
  vfprintf(stderr, msg, ap);
  va_end(ap);
  abort();
}

