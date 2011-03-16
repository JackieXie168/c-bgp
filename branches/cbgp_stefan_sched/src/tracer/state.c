

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

#include <bgp/peer.h>


#include <tracer/state.h>
#include <tracer/tracer.h>
#include <tracer/graph.h>

#include <sim/tunable_scheduler.h>

#include <libgds/fifo_tunable.h>


#include <libgds/trie.h>
#include <../net/net_types.h>

#include "state.h"
#include "transition.h"




static unsigned int state_next_available_id= 0;

bgp_session_info_t * _one_session_info_create(bgp_peer_t * peer )
{

    bgp_session_info_t * thesessioninfo = (bgp_session_info_t *) MALLOC( sizeof(bgp_session_info_t) );
    bgp_route_t ** bgp_route_array;
    unsigned int index;

    thesessioninfo->recv_seq_num=peer->recv_seq_num;
    thesessioninfo->send_seq_num=peer->send_seq_num;
    thesessioninfo->neighbor_addr=peer->addr;

    thesessioninfo->nb_adj_rib_in_routes = trie_num_nodes(peer->adj_rib[RIB_IN],1);
    bgp_route_array = (bgp_route_t **) (_trie_get_array(peer->adj_rib[RIB_IN])->data) ;
   // if (bgp_route_array == NULL)
   //     printf("il y a %d élément, mais get array   est NULL !  \n",trie_num_nodes(peer->adj_rib[RIB_IN],1));
    thesessioninfo->adj_rib_IN_routes =  (bgp_route_t **)
            MALLOC( thesessioninfo->nb_adj_rib_in_routes * sizeof(bgp_route_t *) );
    for (index= 0; index < thesessioninfo->nb_adj_rib_in_routes; index++) {
        thesessioninfo->adj_rib_IN_routes[index] = route_copy(bgp_route_array[index]);
    }

    thesessioninfo->nb_adj_rib_out_routes = trie_num_nodes(peer->adj_rib[RIB_OUT],1);
    bgp_route_array = (bgp_route_t **) (_trie_get_array(peer->adj_rib[RIB_OUT])->data) ;
    //if (bgp_route_array == NULL)
    //    printf("il y a %d élément, mais get array   est NULL !  \n",trie_num_nodes(peer->adj_rib[RIB_OUT],1));
    thesessioninfo->adj_rib_OUT_routes =  (bgp_route_t **)
            MALLOC( thesessioninfo->nb_adj_rib_out_routes * sizeof(bgp_route_t *) );
    for (index= 0; index < thesessioninfo->nb_adj_rib_out_routes; index++) {
        thesessioninfo->adj_rib_OUT_routes[index] = route_copy(bgp_route_array[index]);
    }
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
{
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

static void _loc_rib_info_dump(gds_stream_t * stream, local_rib_info_t * info )
{
    unsigned int i;
    for ( i = 0 ; i<  info->nb_local_rib_elem ; i++)
    {
       stream_printf(stream, "\t\t\t\t\t");
        route_dump(stream, info->bgp_route_[i]);
       stream_printf(stream, "\n");
    }
}

local_rib_info_t * _routing_local_rib_create(net_node_t * node)
{
    unsigned int index;
    net_protocol_t * protocol;
    bgp_router_t * router;
    local_rib_info_t * loc_rib_info = (local_rib_info_t *) MALLOC(sizeof(local_rib_info_t) );
    bgp_route_t ** bgp_route_array ;

    protocol= protocols_get(node->protocols, NET_PROTOCOL_BGP);
    if (protocol == NULL)
    {
        printf("ouille ouille ouille ..., ce noeud n'est pas un bgp router");
        return NULL;
    }
    router = (bgp_router_t *) protocol->handler;

    loc_rib_info->nb_local_rib_elem = trie_num_nodes(router->loc_rib,1);

    bgp_route_array = (bgp_route_t **) (_trie_get_array(router->loc_rib)->data) ;
    loc_rib_info->bgp_route_ =  (bgp_route_t **)
            MALLOC(loc_rib_info->nb_local_rib_elem * sizeof(bgp_route_t *) );
    for (index= 0; index < loc_rib_info->nb_local_rib_elem; index++) {
        loc_rib_info->bgp_route_[index] = route_copy(bgp_route_array[index]);
    }
    return loc_rib_info;
}

routing_info_t * _routing_info_create(net_node_t * node)
{
  routing_info_t * info = (routing_info_t *) MALLOC( sizeof(routing_info_t) );


  //info->node_rt_t = rt_deep_copy(node->rt);

  info->bgp_router_loc_rib_t = _routing_local_rib_create(node);

  //info->bgp_router_peers = ;
  
  info->bgp_sessions_info = _bgp_sessions_info_create(node);

  return info;
}

static void _routing_info_dump(gds_stream_t * stream, routing_info_t * info )
{
     //stream_printf(stream, " Node : %d" , coupleNR->node->rid );
     _loc_rib_info_dump(stream,info->bgp_router_loc_rib_t );
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

static int bgp_router_inject_bgp_session_information(bgp_peer_t * peer, bgp_session_info_t * info)
{
    unsigned int i;

    peer->send_seq_num = info->send_seq_num;
    peer->recv_seq_num = info->recv_seq_num;

    rib_destroy(&(peer->adj_rib[RIB_IN]));
    peer->adj_rib[RIB_IN] = rib_create(0);

    for(i=0 ; i < info->nb_adj_rib_in_routes; i++)
    {
        // ajouter une copie de la route que le tracer mémorise !
        //printf("ATTENTION, placer une copie de la route ");
        rib_add_route(peer->adj_rib[RIB_IN], route_copy(info->adj_rib_IN_routes[i]));
    }
    
    rib_destroy(&(peer->adj_rib[RIB_OUT]));
    peer->adj_rib[RIB_OUT] = rib_create(0);

    for(i=0 ; i < info->nb_adj_rib_out_routes; i++)
    {
        // ajouter une copie de la route que le tracer mémorise !
        //printf("ATTENTION, placer une copie de la route ");
        rib_add_route(peer->adj_rib[RIB_OUT], route_copy(info->adj_rib_OUT_routes[i]));
    }
    
    return 1;
}

static int bgp_router_inject_loc_rib_info(bgp_router_t * router, local_rib_info_t * loc_rib_info)
    {
        unsigned int i;
        rib_destroy(&(router->loc_rib));
        router->loc_rib = rib_create(0);

        for(i=0 ; i < loc_rib_info->nb_local_rib_elem; i++)
        {        
            rib_add_route(router->loc_rib, route_copy(loc_rib_info->bgp_route_[i]));
        }
        return i;
    }

static int _node_inject_routing_info(net_node_t * node, routing_info_t * routing_info )
{
    // inject bgp sessions
    bgp_sessions_info_t * sessions_info = routing_info->bgp_sessions_info;
    local_rib_info_t * loc_rib_info = routing_info->bgp_router_loc_rib_t;

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

    //pour chaque peer, donner les infos de session bgp

    if(bgp_peers_size( router->peers) != sessions_info->nb_bgp_session_info_)
    {
        printf("ATTENTION !!! pas autant de peer dans le noeud de cbgp que de session qu'on a mémorisé plus tôt pour ce même noeud!!\n");
        return -1;
    }

    for (index= 0; index < bgp_peers_size( router->peers); index++) {
        peer= bgp_peers_at(router->peers, index);
        if(peer->addr !=  sessions_info->bgp_session_info[index]->neighbor_addr)
        {
            printf("ATTENTION !!! les peer ne sont pas dans le même ordre qu'avant !?\n");
            return -2;
        }
        bgp_router_inject_bgp_session_information(peer,sessions_info->bgp_session_info[index]);
    }

    // local_rib
    bgp_router_inject_loc_rib_info(router,loc_rib_info);

    return index;
}

static int _routing_state_inject(routing_state_t * routing_state)
{
  unsigned int i = 0;

  int level=1;     int ilevel; for(ilevel = 0 ; ilevel < level ; ilevel++) printf("  ");
  printf("Routing state inject\n");

  for(i=0 ; i< routing_state->state->graph->tracer->nb_nodes ; i++)
  {
      level=2;      for( ilevel = 0 ; ilevel < level ; ilevel++) printf("  ");
      printf("node ");
      node_dump_id(gdsout,routing_state->couples_node_routing_info[i]->node );
      printf("\n");
      
      _node_inject_routing_info(routing_state->couples_node_routing_info[i]->node ,
                                    routing_state->couples_node_routing_info[i]->routing_info);
  }
  return i;
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

  stream_printf(stream, "\tQueue State :\n\t\tNumber of events queued: %u (%u)\n",
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

int same_tcp_session(_event_t * event1 , _event_t * event2 )
{
// returns 1 if same tcp session
// 0 otherwise
  net_send_ctx_t * send_ctx1 = (net_send_ctx_t *) event1->ctx;
  net_send_ctx_t * send_ctx2 = (net_send_ctx_t *) event2->ctx;

  net_msg_t * msg1 = send_ctx1->msg;
  net_msg_t * msg2 = send_ctx2->msg;

  if( msg1->src_addr == msg2->src_addr && msg1->dst_addr == msg2->dst_addr)
      return 1;
  else
      return 0;
}

unsigned int state_calculate_allowed_output_transitions(state_t * state)
{
    if(state->allowed_output_transitions!=NULL)
    {
        // already done !
        return state->nb_allowed_output_transitions;
    }

    unsigned int max_output_transition = state->queue_state->events->current_depth;
    
    state->allowed_output_transitions = (unsigned int *) MALLOC( max_output_transition * sizeof(unsigned int));
    state->nb_allowed_output_transitions = 0;

    // pour chaque événement de la file :
    //      vérifier s'il n'est pas meme source/dest qu'un message deja mis dans les allowedoutputtransition
    //      pour chaque message dans allowed output transition
    //          si meme src/dest , alors sortir
    //      si pas sorti de la boucle, alors ajouter !s

    unsigned int i;
    _event_t * event;
    // pour chaque événement(msg) de la file
    for (i = 0 ; i < state->queue_state->events->current_depth ; i++)
    {
         event = (_event_t *) fifo_tunable_get_at(state->queue_state->events, i);
         //      vérifier s'il n'est pas meme source/dest qu'un message deja mis dans les allowedoutputtransition

         //      pour chaque message dans allowed output transition
         int event_a_consider = 1;
         unsigned int j;
         for(j = 0; event_a_consider == 1 && j < state->nb_allowed_output_transitions ; j++ )
         {
              // si meme src/dest , alors sortir
             if ( 1 == same_tcp_session(event, (_event_t *)
                     fifo_tunable_get_at(state->queue_state->events,
                         state->allowed_output_transitions[j]   )))
             {
                 // meme tcp session ==> cet event n'est pas a considérer.
                 event_a_consider = 0;                 
             }
         }

         if(event_a_consider == 1)
         {
             //ajouter l'événement dans la liste
             state->allowed_output_transitions[state->nb_allowed_output_transitions] = i;
             state->nb_allowed_output_transitions = state->nb_allowed_output_transitions + 1;
         }
    }
    return state->nb_allowed_output_transitions;
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

  state->allowed_output_transitions=NULL;
  state->nb_allowed_output_transitions=0;

  
  state_calculate_allowed_output_transitions(state);
  graph_add_state(state->graph,state,state->id);
  

  return state;
}

// ----- state_create ------------------------------------------------
state_t * state_create_isolated(struct tracer_t * tracer)
{
  state_t * state;
  state=(state_t *) MALLOC(sizeof(state_t));

  state->id= state_next_available_id;
  state->graph = tracer->graph;

  state->queue_state = _queue_state_create(state, tracer_get_tunable_scheduler(tracer));
  state->routing_state = _routing_state_create(state);

  state->allowed_output_transitions=NULL;
  state->nb_allowed_output_transitions = 0;

  state->input_transitions=NULL;
  state->output_transitions=NULL;
  state->nb_input = 0;
  state->nb_output = 0;

  return state;
}

void state_attach_to_graph(state_t * state, struct transition_t * the_input_transition)
{
  if(the_input_transition != NULL)
  { printf("3: state attach to graph : 1 \n");

    assert(state->input_transitions == NULL);
     printf("3: state attach to graph : 2 \n");
    state->input_transitions = (struct transition_t **) MALLOC( 1 * sizeof(struct transition_t *));
    printf("3: state attach to graph : 3 \n");
    state->nb_input=1; printf("3: state attach to graph : 4 \n");
    state->input_transitions[0]=the_input_transition; printf("3: state attach to graph : 5 \n");
    state->input_transitions[0]->to = state; printf("3: state attach to graph : 6 \n");
  }
  else
  { printf("3: state attach to graph : 7 \n");
    state->input_transitions = NULL; printf("3: state attach to graph :8 \n");
    state->nb_input=0; printf("3: state attach to graph : 9 \n");
  }

  state->output_transitions = NULL;
  state->nb_output=0;
 printf("3: state attach to graph : 10 \n");
  //state->allowed_output_transitions=NULL;
  //state->nb_allowed_output_transitions=0;

  state_calculate_allowed_output_transitions(state);
 printf("3: state attach to graph : 11 \n");
  state_next_available_id++;
   printf("3: state attach to graph : 12 \n");
  graph_add_state(state->graph,state,state->id);
   printf("3: state attach to graph : 13 \n");

}

static int _queue_state_inject( queue_state_t * queue_state , sched_tunable_t * tunable_scheduler)
{

    // destroy le tunable_scheduler->events,
    // copier l'état actuel dans le scheduler.
  int level=1;     int ilevel; for(ilevel = 0 ; ilevel < level ; ilevel++) printf("  ");
    printf("queue state_inject \n");

     tunable_scheduler->events = fifo_tunable_copy( queue_state->events , fifo_tunable_event_deep_copy  ) ;

     return 1;
}

int state_inject(state_t * state)
{
    // inject queue_state
    printf("State_inject :    %d\n",state->id);
    _queue_state_inject(state->queue_state ,  tracer_get_tunable_scheduler(state->graph->tracer));
    _routing_state_inject(state->routing_state);
    return 1;    
}

void state_add_output_transition(state_t * state,  struct transition_t * the_output_transition)
{
    // TO DO  TODO
    // vérifier que la transition n'est pas déjà présente.


    if(state->output_transitions == NULL)
    {
        assert(state->nb_output==0);
        state->output_transitions = (struct transition_t **) MALLOC( 1 * sizeof(struct transition_t *));
    }
    else
    {
        state->output_transitions = (struct transition_t **) REALLOC( state->output_transitions, (state->nb_output + 1) * sizeof(struct transition_t *));
    }

    state->nb_output = state->nb_output + 1 ;
    state->output_transitions[state->nb_output-1]=the_output_transition;

    if(the_output_transition->from ==NULL)
            the_output_transition->from=state;
}

void state_add_input_transition(state_t * state,  struct transition_t * the_input_transition)
{
    // TODO !
    // vérifier que la transition n'est pas déjà présente.


    if(state->input_transitions == NULL)
    {
        assert(state->nb_input==0);
        state->input_transitions = (struct transition_t **) MALLOC( 1 * sizeof(struct transition_t *));
    }
    else
    {
        state->input_transitions = (struct transition_t **) REALLOC( state->input_transitions, (state->nb_input + 1) * sizeof(struct transition_t *));
    }

    state->nb_input = state->nb_input + 1 ;
    state->input_transitions[state->nb_input-1]=the_input_transition;
    state->input_transitions[state->nb_input-1]->to=state;
}

struct transition_t * state_generate_transition(state_t * state, unsigned int trans)
  {
      if( trans >= state->nb_allowed_output_transitions)
          return NULL;

      //regarder si on n'a pas déjà généré la transition plus tôt :
      if(state->nb_output>=1)
      {
          unsigned int i =0;
          for (i=0; i< state->nb_output ; i++)
          {
              if( trans == state->output_transitions[i]->num_trans)
                  return NULL;
          }
      }


      struct transition_t * transition = transition_create_from(
             (_event_t *) fifo_tunable_get_at(state->queue_state->events,
              state->allowed_output_transitions[trans]),(struct state_t *) state, trans);

      state_add_output_transition(state,transition);

      return transition;
  }

int state_generate_all_transitions(state_t * state)
  {
      unsigned int nbtrans = state_calculate_allowed_output_transitions(state);
      unsigned int i;
      for(i = 0 ; i < nbtrans ; i++)
      {
          state_generate_transition(state,i);
      }
      return i;
  }

static void _allowed_transition_dump(gds_stream_t * stream, state_t * state)
{
    unsigned int i;
    stream_printf(stream, "\tAllowed output transitions :");
    for(i = 0 ; i < state->nb_allowed_output_transitions ; i++)
    {
       stream_printf(stream, "\t%u", state->allowed_output_transitions[i]);
    }
    stream_printf(stream, "\n");

}

static void _one_transition_dump(gds_stream_t * stream, transition_t * trans)
{
    stream_printf(stream, "\t\t transition id : %u", trans->num_trans );
    if(trans->to!=NULL)
        stream_printf(stream, " leads to state %u", trans->to->id );
    stream_printf(stream, "\n");
}

static void _transitions_dump(gds_stream_t * stream, state_t * state)
{
    unsigned int i;

    stream_printf(stream, "\tOutput transitions :\n");
    for(i = 0 ; i < state->nb_output ; i++)
    {
      _one_transition_dump( stream, state->output_transitions[i]);
    }
}

int state_dump(gds_stream_t * stream, state_t * state)
{
    stream_printf(stream, "State id : %u\n",state->id);
    
    _queue_state_dump(stream,state->queue_state);

    state_calculate_allowed_output_transitions(state);

    _allowed_transition_dump(stream,state);
    _routing_state_dump(stream,state->routing_state);

    _transitions_dump(stream,state);
    return 1;
}

// 0 --> identical
//other valuer -->otherwize
int _one_bgp_session_compare( bgp_session_info_t * bgpi1, bgp_session_info_t * bgpi2)
{
    unsigned int index;
    if(bgpi1->nb_adj_rib_in_routes != bgpi2->nb_adj_rib_in_routes
            || bgpi1->nb_adj_rib_out_routes != bgpi2->nb_adj_rib_out_routes )
        return -710;

    //adj-rib-in
    for (index= 0; index < bgpi1->nb_adj_rib_in_routes; index++) {
       if( 1 != route_equals(bgpi1->adj_rib_IN_routes[index],bgpi2->adj_rib_IN_routes[index]))
       {
           return -720;
       }
   }

        //adj-rib-out
    for (index= 0; index < bgpi1->nb_adj_rib_out_routes; index++) {
       if( 1 != route_equals(bgpi1->adj_rib_OUT_routes[index],bgpi2->adj_rib_OUT_routes[index]))
       {
           return -730;
       }
   }
    return 0;

}

//0=identical
// other value  --> otherwise
int _bgp_sessions_compare(bgp_sessions_info_t * bgp_i_1, bgp_sessions_info_t * bgp_i_2  )
{
   unsigned int i;

   if(bgp_i_1->nb_bgp_session_info_ != bgp_i_2->nb_bgp_session_info_)
       return -660;

   for(i = 0; i < bgp_i_1->nb_bgp_session_info_ ; i++)
   {
       if ( 0 != _one_bgp_session_compare(bgp_i_1->bgp_session_info[i],bgp_i_2->bgp_session_info[i]  ) )
           return -670;
   }
   return  0;
}

// 0 = identical
// other value, otherwise
int _rib_compare(local_rib_info_t * rib1, local_rib_info_t * rib2)
{
// 1rst version :
//  have to have the same route in the same order !

    unsigned int i;

    if(rib1->nb_local_rib_elem != rib2->nb_local_rib_elem)
        return -700;

    for(i = 0; i < rib1->nb_local_rib_elem ; i++)
    {
        if ( route_equals(rib1->bgp_route_[i], rib2->bgp_route_[i] ) !=1 )
            return -800;
    }
    return 0;
}

// 0 = identical
// other value, otherwise
int _routing_info_compare(int nb_nodes, routing_info_t * ri1, routing_info_t * ri2)
{
    // local rib :
    if ( 0 !=  _rib_compare(ri1->bgp_router_loc_rib_t, ri2->bgp_router_loc_rib_t))
        return -600;

    //compare bgp session!

    if( _bgp_sessions_compare(ri1->bgp_sessions_info, ri2->bgp_sessions_info  ) !=0)
        return -650;

    return 0;
}
// 0 = identical
// other value, otherwise
int _routing_states_equivalent(routing_state_t * rs1, routing_state_t * rs2 )
{
  unsigned int i = 0;

  for(i=0 ; i< rs1->state->graph->tracer->nb_nodes ; i++)
  {
     if(_routing_info_compare(rs1->state->graph->tracer->nb_nodes,
             rs1->couples_node_routing_info[i]->routing_info
             ,rs2->couples_node_routing_info[i]->routing_info )
             !=0)
         return -500;
  }
    return 0;
}


// 0-->equivalent
// other value otherwize
int _msgs_equivalent(net_msg_t * msg1,net_msg_t * msg2  )
{
    if(msg1->dst_addr != msg2->dst_addr)
        return -1;

    if(msg1->src_addr != msg2->src_addr)
        return -2;

    if( msg1->protocol != msg2->protocol)
        return -3;

    if( msg1->tos != msg2->tos)
        return -4;

    if( msg1->ttl != msg2->ttl)
        return -5;

    const net_protocol_def_t * proto_def;
    proto_def= net_protocols_get_def(msg1->protocol);
    if (proto_def->ops.compare_msg != NULL)
        return proto_def->ops.compare_msg(msg1, msg2);
    else
        return -10;
}

int _events_equivalent( _event_t * event1, _event_t * event2   )
{
    net_send_ctx_t * send_ctx1= (net_send_ctx_t *) event1->ctx;
    net_send_ctx_t * send_ctx2= (net_send_ctx_t *) event2->ctx;

    if( send_ctx1->dst_iface->addr != send_ctx2->dst_iface->addr)
        return -1;

    return _msgs_equivalent(send_ctx1->msg, send_ctx2->msg);
}


// 0 = identical
// other value, otherwise
int _queue_states_equivalent(queue_state_t * qs1, queue_state_t  * qs2 )
{
    if(qs1->events->current_depth != qs2->events->current_depth )
    {
        return -1;
    }
    
    unsigned int i;
    
    for(i = 0 ; i < qs1->events->current_depth ; i++)
    {
        if( 0 != _events_equivalent(
                (_event_t *) qs1->events->items[(qs1->events->start_index + i)% qs1->events->max_depth],
                (_event_t *) qs2->events->items[(qs2->events->start_index + i)% qs2->events->max_depth]
                ))
            return -1;
    }
    return 0;
}

net_addr_t get_dst_addr(_event_t * ev)
{
     return ((net_send_ctx_t *) ev->ctx)->msg->dst_addr;
}
net_addr_t get_src_addr(_event_t * ev)
{
     return ((net_send_ctx_t *) ev->ctx)->msg->src_addr;
}


// 0 --> different
// 1 --> same
int _same_bgp_session_oriented( _event_t * event1, _event_t * event2 )
{
    return (get_dst_addr(event1) == get_dst_addr(event2) &&
            get_src_addr(event1) == get_src_addr(event2));
}


// return the index of next
// or max value if does not exist.
int _next_of_bgp_session_oriented(net_addr_t src, net_addr_t dst ,
            gds_fifo_tunable_t * file, unsigned int index, int * visited )
{
    while(index < file->current_depth &&
           (
                visited[index]==1 ||
                get_src_addr((_event_t *) file->items[(file->start_index + index)% file->max_depth]) != src ||
                get_dst_addr((_event_t *) file->items[(file->start_index + index)% file->max_depth]) != dst
           )
         )
        index++;
    return index;
}

// 0 = identical
// other value, otherwise
int _queue_states_equivalent_v2(queue_state_t * qs1, queue_state_t  * qs2 )
{
    if(qs1->events->current_depth != qs2->events->current_depth )
    {
        return -1;
    }

    net_addr_t src;
    net_addr_t dst;

    unsigned int index1;
    unsigned int index2;
    unsigned int nb_msg = 0;
    int * tab1_visited = (int *) MALLOC ( qs1->events->current_depth * sizeof(int));
    int * tab2_visited = (int *) MALLOC ( qs1->events->current_depth * sizeof(int));

    for(index1=0; index1<qs1->events->current_depth ; index1++);
    {
        tab1_visited[index1]=0;
        tab2_visited[index1]=0;
    }




    while( nb_msg != qs1->events->current_depth)
    {
        //trouver le premier message de la session bgp (dirigée) suivante dans file 1:
        index1 = 0;
        while(index1<qs1->events->current_depth && tab1_visited[index1]==1)
        {
            index1++;
        }
        assert(index1<qs1->events->current_depth);

        // en index1 : premier message d'une session bgp dirigée
        // identifier la session bgp dirigée :
        src = get_src_addr((_event_t *) qs1->events->items[(qs1->events->start_index + index1)% qs1->events->max_depth]);
        dst = get_dst_addr((_event_t *) qs1->events->items[(qs1->events->start_index + index1)% qs1->events->max_depth]);
        tab1_visited[index1] = 1;
        // trouver le premier message de file2 qui est de cette session bgp.
        index2 = 0;
        while(index2<qs1->events->current_depth && 
                (tab2_visited[index2]==1 || !_same_bgp_session_oriented(
                     (_event_t *) qs1->events->items[(qs1->events->start_index + index1)% qs1->events->max_depth],
                     (_event_t *) qs2->events->items[(qs2->events->start_index + index2)% qs2->events->max_depth]))
             )
        {
           index2++;
        }
        // we go out of the while wheter index2 too high, or we found the msg !
        if(index2==qs1->events->current_depth)
        {
            return -800;
        }

        if(0 != _events_equivalent( (_event_t *) qs1->events->items[(qs1->events->start_index + index1)% qs1->events->max_depth],
                                    (_event_t *) qs2->events->items[(qs2->events->start_index + index2)% qs2->events->max_depth]))
        {
            return -810;
        }

        nb_msg++;
        tab2_visited[index2]=1;

        // la suite de cette session bgp dirigée
        index1 = _next_of_bgp_session_oriented(src,dst,qs1->events,index1, tab1_visited);
        index2 = _next_of_bgp_session_oriented(src,dst,qs2->events,index2, tab2_visited);

        while(index1<qs1->events->current_depth && index2<qs2->events->current_depth)
        {
            // dans index1= le suivant de la session bgp dirigée.
            // dans index2= le suivant de la session bgp dirigée.
            if(0 != _events_equivalent( (_event_t *) qs1->events->items[(qs1->events->start_index + index1)% qs1->events->max_depth],
                                    (_event_t *) qs2->events->items[(qs2->events->start_index + index2)% qs2->events->max_depth]))
            {
                return -820;
            }
            nb_msg++;
            tab1_visited[index1]=1;
            tab2_visited[index2]=1;
            index1 = _next_of_bgp_session_oriented(src,dst,qs1->events,index1, tab1_visited);
            index2 = _next_of_bgp_session_oriented(src,dst,qs2->events,index2, tab2_visited);
        }

        // soit il n'y en a plus dans les deux files, et c'est ok
        if( index1 != qs1->events->current_depth || index2 != qs2->events->current_depth)
            return -830;

    }
    return 0;
}



// 0 = identical
// other value, otherwise
int state_identical(state_t * state1 , state_t * state2 )
{
// équivalent en terme de file, et d'information de routage ...

    if( _queue_states_equivalent_v2(state1->queue_state,state2->queue_state) == 0 &&
            _routing_states_equivalent(state1->routing_state, state2->routing_state ) == 0   )
        return 0;
    else
        return 1;
}