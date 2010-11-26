// ==================================================================
// @(#)state.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 08/08/2005
// $Id: node.c,v 1.19 2009-08-31 09:48:28 bqu Exp $
// ==================================================================

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

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


#include <state.h>


// ----- state_create ------------------------------------------------
routing_state_t * _routing_state_create(state_t * state)
{
  routing_state_t * routing_state;

  routing_state= (routing_state *) MALLOC(sizeof(routing_state));

  routing_state->state = state;
  routing_state->couples_node_rib = (couple_node_rib_t **) MALLOC( state->graph->nb_nodes * sizeof(couple_node_rib_t *));



  return state;
}

// ----- state_create ------------------------------------------------
state_t * state_create(graph_t * graph)
{
  state_t * state;
   
  state=(state_t *) MALLOC(sizeof(state_t));
  state->routing_state = _routing_state_create(state);
  state->queue_state = _queue_state_create();
  state->output_transitions = _output_transitions_create();

  return state;
}


// ----- state_create ------------------------------------------------
graph_t * graph_create(sched_tunable_t tunable_scheduler)
{
  graph_t * graph;

  graph=(graph_t *) MALLOC(sizeof(graph_t));
  state->routing_state = _routing_state_create(state);
  state->queue_state = _queue_state_create();
  state->output_transitions = _output_transitions_create();

  return graph;
}