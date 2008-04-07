// ==================================================================
// @(#)protocol.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 25/02/2004
// $Id: protocol.h,v 1.5 2008-04-07 09:44:48 bqu Exp $
// ==================================================================
// Note: the number of supported protocols will often be low, so that
// the list of protocols is implemented by a fixed-size array at this
// time. Later, if the number of protocols increases or if the
// protocols supported on different nodes varies a lot, we will switch
// to another data structure such as a sorted heap with a O(log(n))
// complexity.


#ifndef __NET_PROTOCOL_H__
#define __NET_PROTOCOL_H__

#include <net/error.h>
#include <net/net_types.h>

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ protocol_recv ]------------------------------------------
  net_error_t protocol_recv(net_protocol_t * proto, net_msg_t * msg,
			    simulator_t * sim);
  
  // ----- protocols_create -------------------------------------------
  net_protocols_t * protocols_create();
  // ----- protocols_destroy ----------------------------------------
  void protocols_destroy(net_protocols_t ** ppProtocols);
  // ----- protocols_register ---------------------------------------
  int protocols_register(net_protocols_t * pProtocols,
			 net_protocol_id_t id, void * pHandler,
			 FNetProtoHandlerDestroy fDestroy,
			 FNetProtoHandleEvent fHandleEvent);
  // ----- protocols_get --------------------------------------------
  net_protocol_t * protocols_get(net_protocols_t * pProtocols,
				 net_protocol_id_t id);
  // -----[ net_protocol2str ]---------------------------------------
  const char * net_protocol2str(net_protocol_id_t id);
  // -----[ net_str2protocol ]---------------------------------------
  net_error_t net_str2protocol(const char * str, net_protocol_id_t * pID);

#ifdef __cplusplus
}
#endif

#endif /* __NET_PROTOCOL_H__ */
