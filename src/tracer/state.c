

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

#include "state.h"



static int state_next_available_id= 0;

bgp_session_info_t * _one_session_info_create(bgp_peer_t * peer )
{

    bgp_session_info_t * thesessioninfo = (bgp_session_info_t *) MALLOC( sizeof(bgp_session_info_t) );
    thesessioninfo->recv_seq_num=peer->recv_seq_num;
    thesessioninfo->send_seq_num=peer->send_seq_num;
    thesessioninfo->neighbor_addr=peer->addr;

    thesessioninfo->adj_rib_IN_routes =  (bgp_route_t **) _trie_get_array(peer->adj_rib[RIB_IN])->data ;
    thesessioninfo->adj_rib_OUT_routes =  (bgp_route_t **) _trie_get_array(peer->adj_rib[RIB_OUT])->data ;

    thesessioninfo->nb_adj_rib_in_routes = trie_num_nodes(peer->adj_rib[RIB_IN],1);
    thesessioninfo->nb_adj_rib_out_routes = trie_num_nodes(peer->adj_rib[RIB_OUT],1);

    return thesessioninfo;
}

bgp_sessions_info_t * _bgp_sessions_info_create(net_node_t * node)
{
    bgp_sessions_info_t * sessions_info = (bgp_sessions_info_t *) MALLOC(sizeof(bgp_sessions_info_t) );

    //pour chaque peer, creer le bgp_session_info
    bgp_peer_t * peer;
    unsigned int index;
    net_protocol_t * protocol;
    bgp_router_t * router;
    
    protocol= protocols_get(node->protocols, NET_PROTOCOL_BGP);
    if (protocol == NULL)
    {
        printf("ouille ouille ouille ..., ce noeud n'est pas un bgp router");
        return NULL;        
    }
    router = (bgp_router_t *) protocol->handler;

    sessions_info->bgp_session_info = (bgp_session_info_t **) MALLOC( bgp_peers_size( router->peers) * sizeof(bgp_session_info_t *) );
    sessions_info->nb_bgp_session_info_ = bgp_peers_size( router->peers);

    for (index= 0; index < bgp_peers_size( router->peers); index++) {
        peer= bgp_peers_at(router->peers, index);
        sessions_info->bgp_session_info[index] = _one_session_info_create(peer);
    }
    return  sessions_info;
}

static void _bgp_one_session_info_dump(gds_stream_t * stream, bgp_session_info_t * bgp_session_info)
{/*
   net_addr_t            neighbor_addr;

       unsigned int          send_seq_num;
   unsigned int          recv_seq_num;
   bgp_route_t **        adj_rib_IN_routes;
   bgp_route_t **        adj_rib_OUT_routes;
   unsigned int          nb_adj_rib_in_routes;
   unsigned int          nb_adj_rib_out_routes;
  * */
    unsigned int index;
   stream_printf(stream, "\t\t\tpeer :" );
         ip_address_dump(stream, bgp_session_info->neighbor_addr);
         stream_printf(stream, "\n" );
   stream_printf(stream, "\t\t\t\ttcp send/recv seq num : %d/%d\n",bgp_session_info->send_seq_num , bgp_session_info->recv_seq_num );
   stream_printf(stream, "\t\t\t\tAdj-Rib IN : \n");
   for (index= 0; index < bgp_session_info->nb_adj_rib_in_routes; index++) {
       stream_printf(stream, "\t\t\t\t\t");
       route_dump(stream, bgp_session_info->adj_rib_IN_routes[index]);
       stream_printf(stream, "\n");
   }
   stream_printf(stream, "\t\t\t\tAdj-Rib OUT : \n");
   for (index= 0; index < bgp_session_info->nb_adj_rib_out_routes; index++) {
       stream_printf(stream, "\t\t\t\t\t");
       route_dump(stream, bgp_session_info->adj_rib_OUT_routes[index]);
       stream_printf(stream, "\n");
   }

}

static void _bgp_sessions_info_dump(gds_stream_t * stream, bgp_sessions_info_t * sessions_info)
{
     stream_printf(stream, "\t\t\tBGP sessions \n");
    //pour chaque session, afficher les rib in et out :
    unsigned int index;
    for (index= 0; index < sessions_info->nb_bgp_session_info_; index++) {
        _bgp_one_session_info_dump(stream,sessions_info->bgp_session_info[index]);
    }
}

routing_info_t * _routing_info_create(net_node_t * node)
{
  routing_info_t * info = (routing_info_t *) MALLOC( sizeof(routing_info_t) );


  //info->node_rt_t = rt_deep_copy(node->rt);

  //info->bgp_router_loc_rib_t = ;

  //info->bgp_router_peers = ;
  
  info->bgp_sessions_info = _bgp_sessions_info_create(node);

  return info;
}

static void _routing_info_dump(gds_stream_t * stream, routing_info_t * info )
{
     //stream_printf(stream, " Node : %d" , coupleNR->node->rid );
     _bgp_sessions_info_dump(stream,info->bgp_sessions_info);

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
    stream_printf(stream, "\t\tNode : " );
    node_dump_id(stream,coupleNR->node);
    stream_printf(stream, "\n" );

    _routing_info_dump(stream,coupleNR->routing_info);
}


static void _routing_state_dump(gds_stream_t * stream, routing_state_t * routing_state)
{
  stream_printf(stream, "\tRouting State : \n");
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
    _routing_state_dump(stream,state->routing_state);
    return 1;
}

