// ==================================================================
// @(#)protocol.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 25/02/2004
// $Id: protocol.c,v 1.7 2008-06-13 14:26:23 bqu Exp $
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

// -----[ _protocol_create ]-----------------------------------------
/**
 *
 */
static inline
net_protocol_t * _protocol_create(net_protocol_id_t id,
				  const char * name,
				  void * handler,
				  FNetProtoHandlerDestroy destroy,
				  FNetProtoHandleEvent handle_event)
{
  net_protocol_t * proto=
    (net_protocol_t *) MALLOC(sizeof(net_protocol_t));
  proto->id= id;
  proto->name= name;
  proto->handler= handler;
  proto->ops.handle= handle_event;
  proto->ops.destroy= destroy;
  proto->ops.dump= NULL;
  return proto;
}

// -----[ _protocol_destroy ]----------------------------------------
/**
 *
 */
static inline
void _protocol_destroy(net_protocol_t ** proto_ref)
{
  if (*proto_ref != NULL) {
    if ((*proto_ref)->ops.destroy != NULL)
      (*proto_ref)->ops.destroy(&(*proto_ref)->handler);
    FREE(*proto_ref);
    *proto_ref= NULL;
  }
}

// -----[ protocol_recv ]------------------------------------------
net_error_t protocol_recv(net_protocol_t * proto, net_msg_t * msg,
			  simulator_t * sim)
{
  assert(proto->ops.handle != NULL);
  return proto->ops.handle(sim, proto->handler, msg);
}

// -----[ _protocols_cmp ]-------------------------------------------
static int _protocols_cmp(void * item1, void * item2,
			  unsigned int item_size)
{
  net_protocol_t * proto1= *((net_protocol_t **) item1);
  net_protocol_t * proto2= *((net_protocol_t **) item2);
  
  if (proto1->id > proto2->id)
    return 1;
  else if (proto1->id < proto2->id)
    return -1;
  return 0;
}

// -----[ _protocols_destroy ]---------------------------------------
static void _protocols_destroy(void * item)
{
  _protocol_destroy((net_protocol_t **) item);
}

// ----- protocols_create -------------------------------------------
/**
 *
 */
net_protocols_t * protocols_create()
{
  net_protocols_t * pProtocols=
    (net_protocols_t *) ptr_array_create(ARRAY_OPTION_SORTED|
					 ARRAY_OPTION_UNIQUE,
					 _protocols_cmp,
					 _protocols_destroy);
  return pProtocols;
}

// ----- protocols_destroy ------------------------------------------
/**
 * This function destroys the list of protocols and destroys all the
 * related protocol instances if required.
 */
void protocols_destroy(net_protocols_t ** protocols_ref)
{
  ptr_array_destroy(protocols_ref);
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
  net_protocol_t * protocol=
    _protocol_create(id, NULL, handler, destroy, handle_event);

  if (ptr_array_add(protocols, &protocol) < 0) {
    _protocol_destroy(&protocol);
    return ENET_PROTO_DUPLICATE;
  }

  return ESUCCESS;
}

// ----- protocols_get ----------------------------------------------
/**
 *
 */
net_protocol_t * protocols_get(net_protocols_t * protocols,
			       net_protocol_id_t id)
{
  net_protocol_t proto= { .id= id };
  net_protocol_t * proto_ref= &proto;
  unsigned int index;

  if (ptr_array_sorted_find_index(protocols, &proto_ref, &index) < 0)
    return NULL;
  return protocols->data[index];
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
