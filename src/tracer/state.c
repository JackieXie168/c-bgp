

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libgds/memory.h>
#include <libgds/stack.h>

#include <assert.h>
#include <libgds/str_util.h>

#include <net/error.h>
#include <net/icmp.h>
#include <net/link.h>
#include <net/link-list.h>
#include <net/net_types.h>
#include <net/netflow.h>
#include <net/network.h>
#include <net/node.h>
#include <net/subnet.h>

#include <tracer/state.h>
#include <tracer/tracer.h>
#include <tracer/graph.h>

#include <sim/tunable_scheduler.h>

#include <libgds/fifo_tunable.h>



// ----- state_create ------------------------------------------------
routing_state_t * _routing_state_create(state_t * state)
{
  routing_state_t * routing_state;
  routing_state= (routing_state_t *) MALLOC(sizeof(routing_state_t));

  routing_state->state = state;
  routing_state->couples_node_rib = (couple_node_rib_t **) MALLOC(state->graph->nb_net_nodes * sizeof(couple_node_rib_t *));

  return routing_state;
}



// ----- state_create ------------------------------------------------
queue_state_t * _queue_state_create(state_t * state, sched_tunable_t * tunable_scheduler)
{
  queue_state_t * queue_state;
  queue_state= (queue_state_t *) MALLOC(sizeof(queue_state_t));

  queue_state->state = state;

  queue_state->events=fifo_tunable_copy( tunable_scheduler->events , fifo_tunable_event_copy   ) ;

  return queue_state;
}

// ----- state_create ------------------------------------------------
state_t * state_create(sched_tunable_t * tunable_scheduler)
{
  state_t * state;   
  state=(state_t *) MALLOC(sizeof(state_t));

  state->queue_state = _queue_state_create(state, tunable_scheduler);


  //state->routing_state = _routing_state_create(state);
  //state->output_transitions = _output_transitions_create();

  return state;
}

