// ==================================================================
// @(#)icmp.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 25/02/2004
// @lastdate 18/04/2007
// ==================================================================

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>

#include <net/icmp.h>
#include <net/prefix.h>
#include <net/protocol.h>
#include <net/network.h>
#include <net/net_types.h>

#define ICMP_ECHO_REQUEST 0
#define ICMP_ECHO_REPLY   1
#define ICMP_ERROR        2

#define ICMP_MSG_TYPE(M) (((int) (M->pPayLoad)) & 0xff)
#define ICMP_MSG_SUBTYPE(M) (((int) (M->pPayLoad)) >> 8)

typedef struct {
  SNetNode * pNode;      // Node that received the message
  net_addr_t tSrcAddr;   // Source address of the message
  net_addr_t tDstAddr;   // Destination address of the message
  uint8_t    uType;      // ICMP type
  uint8_t    uSubType;   // ICMP subtype
  int        iReceived;
} SICMPMessage;
static SICMPMessage sICMPRecv= { .iReceived= 0 };


// -----[ icmp_perror ]----------------------------------------------
/**
 * Report comprehensive ICMP error description.
 */
void icmp_perror(SLogStream * pStream, uint8_t uErrorCode)
{
  switch (uErrorCode) {
  case ICMP_ERROR_NET_UNREACH:
    log_printf(pStream, "network unreachable"); break;
  case ICMP_ERROR_DST_UNREACH:
    log_printf(pStream, "destination unreachable"); break;
  case ICMP_ERROR_PORT_UNREACH:
    log_printf(pStream, "port unreachable"); break;
  case ICMP_ERROR_TIME_EXCEEDED:
    log_printf(pStream, "time exceeded"); break;
  default:
    log_printf(pStream, "unknown error code (%d)", uErrorCode);
  }
}

// -----[ is_icmp_error ]--------------------------------------------
/**
 * Check if the message is an ICMP error. This is used in the node
 * forwarding function to avoid sending an ICMP error message when
 * we fail to forward/deliver another ICMP error message.
 */
int is_icmp_error(SNetMessage * pMessage)
{
  return ((pMessage->uProtocol == NET_PROTOCOL_ICMP) &&
	  (ICMP_MSG_TYPE(pMessage) == ICMP_ERROR));
}

// -----[ icmp_send_error ]------------------------------------------
/**
 * Send an ICMP error message.
 */
int icmp_send_error(SNetNode * pNode, net_addr_t tSrcAddr,
		    net_addr_t tDstAddr, uint8_t uErrorCode,
		    SSimulator * pSimulator)
{
  return node_send_msg(pNode, tSrcAddr, tDstAddr, NET_PROTOCOL_ICMP,
		       255, (void *) ICMP_ERROR+(uErrorCode << 8),
		       NULL, pSimulator);
}

// -----[ icmp_send_echo_request ]-----------------------------------
/**
 * Send an ICMP echo request to the given destination address.
 */
int icmp_send_echo_request(SNetNode * pNode, net_addr_t tSrcAddr,
			   net_addr_t tDstAddr, uint8_t uTTL,
			   SSimulator * pSimulator)
{
  return node_send_msg(pNode, tSrcAddr, tDstAddr, NET_PROTOCOL_ICMP,
		       uTTL, (void *) ICMP_ECHO_REQUEST,
		       NULL, pSimulator);
}

// -----[ icmp_send_echo_reply ]-------------------------------------
/**
 * Send an ICMP echo reply to the given destination address. The
 * source address of the ICMP message is set to the given source
 * address. Depending on the implementation, the source address might
 * be the same as the destination address in the ICMP echo request
 * message instead of the address of the outgoing interface.
 */
int icmp_send_echo_reply(SNetNode * pNode, net_addr_t tSrcAddr,
			 net_addr_t tDstAddr, uint8_t uTTL,
			 SSimulator * pSimulator)
{
  // Source address is fixed
  return node_send_msg(pNode, tSrcAddr, tDstAddr, NET_PROTOCOL_ICMP,
		       uTTL, (void *) ICMP_ECHO_REPLY, NULL, pSimulator);
  /*
  // Source address depends on outgoing interface
  return node_send(pNode, NET_ADDR_ANY, tDstAddr, NET_PROTOCOL_ICMP,
  uTTL, (void *) ICMP_ECHO_REPLY, NULL, pSimulator);
  */
}

// -----[ icmp_event_handler ]---------------------------------------
/**
 * This function handles ICMP messages received by a node.
 */
int icmp_event_handler(SSimulator * pSimulator,
		       void * pHandler,
		       SNetMessage * pMessage)
{
  SNetNode * pNode= (SNetNode *) pHandler;

  // Record received ICMP messages. This is used by the icmp_ping()
  // and icmp_traceroute() functions to check for replies.
  sICMPRecv.iReceived= 1;
  sICMPRecv.pNode= pNode;
  sICMPRecv.tSrcAddr= pMessage->tSrcAddr;
  sICMPRecv.tDstAddr= pMessage->tDstAddr;
  sICMPRecv.uType= ICMP_MSG_TYPE(pMessage);
  sICMPRecv.uSubType= ICMP_MSG_SUBTYPE(pMessage);

  switch (ICMP_MSG_TYPE(pMessage)) {
  case ICMP_ECHO_REQUEST:
    icmp_send_echo_reply(pNode, pMessage->tDstAddr, pMessage->tSrcAddr,
			 255, pSimulator);
    break;

  case ICMP_ECHO_REPLY:
  case ICMP_ERROR:
    break;

  default:
    LOG_ERR(LOG_LEVEL_FATAL, "Error: unsupported ICMP message type (%d)\n",
	    ICMP_MSG_TYPE(pMessage));
    abort();
  }

  return 0;
}

// -----[ icmp_ping_send_recv ]--------------------------------------
/**
 * This function sends an ICMP echo-request and checks that an answer
 * is received. It can be used to silently check that a remote host
 * is reachable (e.g. in BGP session establishment and refresh
 * checks).
 *
 * For this purpose, the function instanciates a local simulator that
 * is used to schedule the messages exchanged in reaction to the ICMP
 * echo-request.
 */
int icmp_ping_send_recv(SNetNode * pSrcNode, net_addr_t tSrcAddr,
			net_addr_t tDstAddr, uint8_t uTTL)
{
  SSimulator * pSimulator= NULL;
  int iResult;

  pSimulator= simulator_create(SCHEDULER_STATIC);

  sICMPRecv.iReceived= 0;
  iResult= icmp_send_echo_request(pSrcNode, tSrcAddr, tDstAddr,
				  uTTL, pSimulator);
  if (iResult == NET_SUCCESS) {
    simulator_run(pSimulator);

    if ((sICMPRecv.iReceived) && (sICMPRecv.pNode == pSrcNode)) {
      switch (sICMPRecv.uType) {
      case ICMP_ECHO_REPLY:
	iResult= NET_SUCCESS;
	break;
      case ICMP_ERROR:
	switch (sICMPRecv.uSubType) {
	case ICMP_ERROR_NET_UNREACH:
	  iResult= NET_ERROR_ICMP_NET_UNREACH; break;
	case ICMP_ERROR_DST_UNREACH:
	  iResult= NET_ERROR_ICMP_DST_UNREACH; break;
	case ICMP_ERROR_PORT_UNREACH:
	  iResult= NET_ERROR_ICMP_PORT_UNREACH; break;
	case ICMP_ERROR_TIME_EXCEEDED:
	  iResult= NET_ERROR_ICMP_TIME_EXCEEDED; break;
	default:
	  abort();
	}
	break;
      default:
	iResult= NET_ERROR_NO_REPLY;
      }
    } else {
      iResult= NET_ERROR_NO_REPLY;
    }
  }

  simulator_destroy(&pSimulator);
  return iResult;
}

// -----[ icmp_ping ]------------------------------------------------
/**
 * Send an ICMP echo-request to the given destination and checks if a
 * reply is received. This function mimics the well-known 'ping'
 * application.
 */
int icmp_ping(SLogStream * pStream,
	      SNetNode * pSrcNode, net_addr_t tSrcAddr,
	      net_addr_t tDstAddr, uint8_t uTTL)
{
  int iResult;

  if (uTTL == 0)
    uTTL= 255;

  iResult= icmp_ping_send_recv(pSrcNode, tSrcAddr, tDstAddr, uTTL);

  log_printf(pStream, "ping: ");
  switch (iResult) {
  case NET_SUCCESS:
    log_printf(pStream, "reply from ");
    ip_address_dump(pStream, sICMPRecv.tSrcAddr);
    break;
  case NET_ERROR_ICMP_NET_UNREACH:
  case NET_ERROR_ICMP_DST_UNREACH:
  case NET_ERROR_ICMP_PORT_UNREACH:
  case NET_ERROR_ICMP_TIME_EXCEEDED:
    network_perror(pStream, iResult);
    log_printf(pStream, " from ");
    ip_address_dump(pStream, sICMPRecv.tSrcAddr);
    break;
  default:
    network_perror(pStream, iResult);
  }
  log_printf(pStream, "\n");
  log_flush(pStream);

  return iResult;
}

// -----[ icmp_trace_route ]-----------------------------------------
/**
 * Send a ICMP echo-request with increasing TTL to the given
 * destination and checks if a 'time-exceeded' error or an echo-reply
 * is received. This function mimics the well-known 'traceroute'
 * application.
 */
int icmp_trace_route(SLogStream * pStream,
		     SNetNode * pSrcNode, net_addr_t tSrcAddr,
		     net_addr_t tDstAddr, uint8_t uMaxTTL)
{
  int iResult= NET_ERROR_UNKNOWN;
  uint8_t uTTL= 1;

  if (uMaxTTL == 0)
    uMaxTTL= 255;

  while (uTTL <= uMaxTTL) {

    iResult= icmp_ping_send_recv(pSrcNode, tSrcAddr, tDstAddr, uTTL);

    log_printf(pStream, "%3d\t", uTTL);
    switch (iResult) {
    case NET_SUCCESS:
    case NET_ERROR_ICMP_NET_UNREACH:
    case NET_ERROR_ICMP_DST_UNREACH:
    case NET_ERROR_ICMP_PORT_UNREACH:
    case NET_ERROR_ICMP_TIME_EXCEEDED:
      ip_address_dump(pStream, sICMPRecv.tSrcAddr);
      break;
    default:
      log_printf(pStream, "*");
    }

    log_printf(pStream, "\t");
    if (iResult == NET_SUCCESS)
      log_printf(pStream, "reply");
    else
      network_perror(pStream, iResult);
    log_printf(pStream, "\n");
    
    if (iResult != NET_ERROR_ICMP_TIME_EXCEEDED)
      return iResult;
      
    uTTL++;
  }
  log_flush(pStream);

  return iResult;
}
