// ==================================================================
// @(#)protocol.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 25/02/2004
// $Id: protocol.c,v 1.6 2008-06-11 15:13:45 bqu Exp $
// ==================================================================

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include <libgds/array.h>
#include <libgds/memory.h>
#include <string.h>

#include <net/error.h>
#include <net/net_types.h>
#include <net/protocol.h>

// -----[ Protocol names ]-----
const char * PROTOCOL_NAMES[NET_PROTOCOL_MAX]= {
  "raw",
  "icmp",
  "bgp",
  "ipip"
};

// ----- protocol_create --------------------------------------------
/**
 *
 */
net_protocol_t * protocol_create(void * handler,
				 FNetProtoHandlerDestroy destroy,
				 FNetProtoHandleEvent handle_event)
{
  net_protocol_t * proto=
    (net_protocol_t *) MALLOC(sizeof(net_protocol_t));
  proto->pHandler= handler;
  proto->fDestroy= destroy;
  proto->fHandleEvent= handle_event;
  return proto;
}

// ----- protocol_destroy -------------------------------------------
/**
 *
 */
void protocol_destroy(net_protocol_t ** proto_ref)
{
  if (*proto_ref != NULL) {
    if ((*proto_ref)->fDestroy != NULL)
      (*proto_ref)->fDestroy(&(*proto_ref)->pHandler);
    FREE(*proto_ref);
    *proto_ref= NULL;
  }
}

// -----[ protocol_recv ]------------------------------------------
net_error_t protocol_recv(net_protocol_t * proto, net_msg_t * msg,
			  simulator_t * sim)
{
  assert(proto->fHandleEvent != NULL);
  return proto->fHandleEvent(sim, proto->pHandler, msg);

}

// ----- protocols_create -------------------------------------------
/**
 *
 */
net_protocols_t * protocols_create()
{
  net_protocols_t * pProtocols=
    (net_protocols_t *) MALLOC(sizeof(net_protocols_t));

  memset(pProtocols->data, 0, sizeof(pProtocols->data));
  return pProtocols;
}

// ----- protocols_destroy ------------------------------------------
/**
 * This function destroys the list of protocols and destroys all the
 * related protocol instances if required.
 */
void protocols_destroy(net_protocols_t ** protocols_ref)
{
  unsigned int index;

  if (*protocols_ref != NULL) {
    
    // Destroy protocols (also clear the protocol instance if
    // required, see 'protocol_destroy')
    for (index= 0; index < NET_PROTOCOL_MAX; index++)
      if ((*protocols_ref)->data[index] != NULL)
	protocol_destroy(&(*protocols_ref)->data[index]);

    FREE(*protocols_ref);
    *protocols_ref= NULL;
  }
}

// ----- protocols_register -----------------------------------------
/**
 * Add a protocol handler to the protocol list.
 */
net_error_t protocols_register(net_protocols_t * protocols,
			       net_protocol_id_t id,
			       void * handler,
			       FNetProtoHandlerDestroy destroy,
			       FNetProtoHandleEvent handle_event)
{
  if (id >= NET_PROTOCOL_MAX)
    return ENET_PROTO_UNKNOWN;

  if (protocols->data[id] != NULL)
    return ENET_PROTO_DUPLICATE;

  protocols->data[id]= protocol_create(handler, destroy, handle_event);
  return ESUCCESS;
}

// ----- protocols_get ----------------------------------------------
/**
 *
 */
net_protocol_t * protocols_get(net_protocols_t * protocols,
			       net_protocol_id_t id)
{
  if (id >= NET_PROTOCOL_MAX)
    return NULL;
  return protocols->data[id];
}

// -----[ net_protocol2str ]-----------------------------------------
const char * net_protocol2str(net_protocol_id_t id)
{
  if (id >= NET_PROTOCOL_MAX)
    return "unknown";
  return PROTOCOL_NAMES[id];
}

// -----[ net_str2protocol ]-----------------------------------------
net_error_t net_str2protocol(const char * str, net_protocol_id_t * id_ref)
{
  net_protocol_id_t id= 0;
  while (id < NET_PROTOCOL_MAX) {
    if (!strcmp(PROTOCOL_NAMES[id], str)) {
      *id_ref= id;
      return ESUCCESS;
    }
    id++;
  }
  return ENET_PROTO_UNKNOWN;
}
