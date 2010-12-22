

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


#include <libgds/trie.h>



static int state_next_available_id= 0;



routing_info_t * _routing_info_create(net_node_t * node)
{
  routing_info_t * info = (routing_info_t *) MALLOC(sizeof(routing_info_t));


  info->node_rt_t = rt_deep_copy(node->rt);


  //info->bgp_router_loc_rib_t = ;
  //info->bgp_router_peers = ;
  //info->node_rt_t = ;
  

  return info;
}
couple_node_routinginfo_t * _couple_node_routinginfo_create(net_node_t * node)
{
 couple_node_routinginfo_t * couple;
 couple = (couple_node_routinginfo_t *) MALLOC(sizeof(couple_node_routinginfo_t));
 
 couple->node=node;
 couple->routing_info = _routing_info_create(node);
  
 return couple;
 
}

// ----- state_create ------------------------------------------------
routing_state_t * _routing_state_create(state_t * state)
{
  routing_state_t * routing_state;
  routing_state= (routing_state_t *) MALLOC(sizeof(routing_state_t));

  routing_state->state = state;
  routing_state->couples_node_routing_info = (couple_node_routinginfo_t **) MALLOC(state->graph->tracer->nb_nodes * sizeof(couple_node_routinginfo_t *));

  unsigned int i = 0;
  for(i=0 ; i< state->graph->tracer->nb_nodes ; i++)
  {
      routing_state->couples_node_routing_info[i] = _couple_node_routinginfo_create(state->graph->tracer->nodes[i]);
  }

  return routing_state;
}

static void _couple_node_routinginfo_dump(gds_stream_t * stream, couple_node_routinginfo_t * coupleNR)
{
    stream_printf(stream, " " );
}


static void _routing_state_dump(gds_stream_t * stream, routing_state_t * routing_state)
{
  unsigned int i = 0;
  for(i=0 ; i< routing_state->state->graph->tracer->nb_nodes ; i++)
  {
      _couple_node_routinginfo_dump(stream,routing_state->couples_node_routing_info[i]);
  }

}


static void _queue_state_dump(gds_stream_t * stream, queue_state_t * queue_state)
{
  _event_t * event;
  uint32_t depth;
  uint32_t max_depth;
  uint32_t start;
  unsigned int index;

  depth= queue_state->events->current_depth;
  max_depth= queue_state->events->max_depth;
  start= queue_state->events->start_index;

  stream_printf(stream, "\tQueue State :\n\tNumber of events queued: %u (%u)\n",
	     depth, max_depth);

          
  for (index= 0; index < depth; index++) {
    event= (_event_t *) queue_state->events->items[(start+index) % max_depth];
    stream_printf(stream, "%d\t(%d) ", index, (start+index) % max_depth);
    //stream_printf(stream, "-- Event:%p - ctx:%p - msg:%p - bgpmsg:%p --\n\t\t", event, event->ctx, ((net_send_ctx_t *)event->ctx)->msg,((net_msg_t *)((net_send_ctx_t *)event->ctx)->msg)->payload);
    stream_flush(stream);
    if (event->ops->dump != NULL) {
      event->ops->dump(stream, event->ctx);
    } else {
      stream_printf(stream, "unknown");
    }
    stream_printf(stream, "\n");
  }
}


// ----- state_create ------------------------------------------------
queue_state_t * _queue_state_create(state_t * state, sched_tunable_t * tunable_scheduler)
{
  queue_state_t * queue_state;
  queue_state= (queue_state_t *) MALLOC(sizeof(queue_state_t));

  queue_state->state = state;

  queue_state->events=fifo_tunable_copy( tunable_scheduler->events , fifo_tunable_event_deep_copy   ) ;
  
  return queue_state;
}

// ----- state_create ------------------------------------------------
state_t * state_create(struct tracer_t * tracer, struct transition_t * the_input_transition)
{
  state_t * state;   
  state=(state_t *) MALLOC(sizeof(state_t));

  state->id= state_next_available_id;
  state_next_available_id++;
  
  state->graph = tracer->graph;

  state->queue_state = _queue_state_create(state, tracer_get_tunable_scheduler(tracer));
  state->routing_state = _routing_state_create(state);

  if(the_input_transition != NULL)
  {
    state->input_transitions = (struct transition_t **) MALLOC( 1 * sizeof(struct transition_t *));
    state->nb_input=1;
    state->input_transitions[0]=the_input_transition;
    state->input_transitions[0]->to = state;
  }
  else
  {
    state->input_transitions = NULL;
    state->nb_input=0;
  }
  
  state->output_transitions = NULL;
  state->nb_output=0;


  return state;
}


int _queue_state_inject( queue_state_t * queue_state , sched_tunable_t * tunable_scheduler)
{

    // destroy le tunable_scheduler->events,
    // copier l'état actuel dans le scheduler.
    printf("state_inject :    %d\n",queue_state->state->id);

     tunable_scheduler->events = fifo_tunable_copy( queue_state->events , fifo_tunable_event_deep_copy  ) ;

     return 1;
}

int state_inject(state_t * state)
{
    // inject queue_state
    printf("state_inject :    %d\n",state->id);
    return  _queue_state_inject(state->queue_state ,  tracer_get_tunable_scheduler(state->graph->tracer));

    // inject route_state
    
}



void state_add_output_transition(state_t * state,  struct transition_t * the_output_transition)
{
    if(state->output_transitions == NULL)
    {
        assert(state->nb_output==0);
        state->output_transitions = (struct transition_t **) MALLOC( 1 * sizeof(struct transition_t *));
        state->nb_output=1;
        state->output_transitions[0]=the_output_transition;
        state->output_transitions[0]->from = state;
    }
    else
    {
        state->output_transitions = (struct transition_t **) REALLOC( state->output_transitions, (state->nb_output + 1) * sizeof(struct transition_t *));
        state->nb_output = state->nb_output + 1 ;
        state->output_transitions[state->nb_output-1]=the_output_transition;
        state->output_transitions[state->nb_output-1]->from = state;
    }
}


int state_dump(gds_stream_t * stream, state_t * state)
{
    stream_printf(stream, "State id : %d\n",state->id);

    _queue_state_dump(stream,state->queue_state);
    //_routing_state_dump(stream,state->routing_state);
    return 1;
}

