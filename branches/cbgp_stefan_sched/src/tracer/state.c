

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <time.h>

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
#include <tracer/routing_state.h>
#include <tracer/queue_state.h>


#include <sim/tunable_scheduler.h>

#include <libgds/fifo_tunable.h>


#include <libgds/trie.h>
#include <../net/net_types.h>

#include "state.h"
#include "transition.h"

#define DUMP_ONLY_CONTENT 1
#define DUMP_STATE 0
#define DUMP_DEFAULT DUMP_STATE

static int STATE_DEBBUG = 0;
static int TYPE_OF_DUMP = DUMP_DEFAULT;


static unsigned int state_next_available_id= 0;
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

if(TYPE_OF_DUMP!=DUMP_ONLY_CONTENT)  
		  stream_printf(stream, "\tQueue State :\n\t\tNumber of events queued: %u (%u)\n",
	     depth, max_depth);
          
  for (index= 0; index < depth; index++) {
    event= (_event_t *) queue_state->events->items[(start+index) % max_depth];
    if(TYPE_OF_DUMP!=DUMP_ONLY_CONTENT) 
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

unsigned int  _queue_state_calculate_max_nb_of_msg_in_oriented_bgp_session(queue_state_t * qs)
{
    if(qs->max_nb_of_msg_in_one_oriented_session != 0 || qs->events->current_depth == 0)
    {
        return qs->max_nb_of_msg_in_one_oriented_session;
    }


    net_addr_t src;
    net_addr_t dst;

    unsigned int index1;
    unsigned int nb_msg = 0;
    unsigned int temp = 0;
    int tab1_visited[qs->events->current_depth];

    for(index1=0; index1<qs->events->current_depth ; index1++)
    {
        tab1_visited[index1] = 0;
    }


    while( nb_msg != qs->events->current_depth)
    {
        //trouver le premier message de la session bgp (dirigée) suivante dans file 1:
        index1 = 0;
        while(index1<qs->events->current_depth && tab1_visited[index1]==1)
        {
            index1++;
        }
        assert(index1<qs->events->current_depth);

        // en index1 : premier message d'une session bgp dirigée
        // identifier la session bgp dirigée :
        src = get_src_addr((_event_t *) qs->events->items[(qs->events->start_index + index1)% qs->events->max_depth]);
        dst = get_dst_addr((_event_t *) qs->events->items[(qs->events->start_index + index1)% qs->events->max_depth]);
        tab1_visited[index1] = 1;

        nb_msg++;
        temp = 1;

        // la suite de cette session bgp dirigée
        index1 = _next_of_bgp_session_oriented(src,dst,qs->events,index1, tab1_visited);

        while(index1<qs->events->current_depth)
        {
            // dans index1= le suivant de la session bgp dirigée.
            nb_msg++;
            temp++;
            tab1_visited[index1]=1;
            index1 = _next_of_bgp_session_oriented(src,dst,qs->events,index1, tab1_visited);
        }

        assert(index1 == qs->events->current_depth);

        if(temp>qs->max_nb_of_msg_in_one_oriented_session)
            qs->max_nb_of_msg_in_one_oriented_session = temp;
    }
    return qs->max_nb_of_msg_in_one_oriented_session;
}


// ----- state_create ------------------------------------------------
queue_state_t * _queue_state_create(state_t * state, sched_tunable_t * tunable_scheduler)
{
  queue_state_t * queue_state;
  queue_state= (queue_state_t *) MALLOC(sizeof(queue_state_t));

  queue_state->state = state;
  queue_state->events=fifo_tunable_copy( tunable_scheduler->events , fifo_tunable_event_deep_copy   ) ;
  queue_state->max_nb_of_msg_in_one_oriented_session=0;
  _queue_state_calculate_max_nb_of_msg_in_oriented_bgp_session(queue_state);
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

    if(state->nb_allowed_output_transitions == 0)
    {
        state->type = state->type | STATE_FINAL;
        graph_add_final_state(state->graph, state);
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
  state->type = 0x0;
  state->type = 0x0;
  state->marking_sequence_number = 0;

  
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
  state->type = 0x0;
  state->marking_sequence_number = 0;
  return state;
}

void state_attach_to_graph(state_t * state, struct transition_t * the_input_transition)
{
  if(the_input_transition != NULL)
  { if(STATE_DEBBUG==1) printf("3: state attach to graph : 1 \n");

    assert(state->input_transitions == NULL);
     if(STATE_DEBBUG==1) printf("3: state attach to graph : 2 \n");
    state->input_transitions = (struct transition_t **) MALLOC( 1 * sizeof(struct transition_t *));
    if(STATE_DEBBUG==1) printf("3: state attach to graph : 3 \n");
    state->nb_input=1; if(STATE_DEBBUG==1) printf("3: state attach to graph : 4 \n");
    state->input_transitions[0]=the_input_transition; if(STATE_DEBBUG==1) printf("3: state attach to graph : 5 \n");
    state->input_transitions[0]->to = state; if(STATE_DEBBUG==1) printf("3: state attach to graph : 6 \n");
  }
  else
  { if(STATE_DEBBUG==1) printf("3: state attach to graph : 7 \n");
    state->input_transitions = NULL; if(STATE_DEBBUG==1) printf("3: state attach to graph :8 \n");
    state->nb_input=0; if(STATE_DEBBUG==1) printf("3: state attach to graph : 9 \n");
  }

  state->output_transitions = NULL;
  state->nb_output=0;
 if(STATE_DEBBUG==1) printf("3: state attach to graph : 10 \n");
  //state->allowed_output_transitions=NULL;
  //state->nb_allowed_output_transitions=0;

  state_calculate_allowed_output_transitions(state);
 if(STATE_DEBBUG==1) printf("3: state attach to graph : 11 \n");
  state_next_available_id++;
   if(STATE_DEBBUG==1) printf("3: state attach to graph : 12 \n");
  graph_add_state(state->graph,state,state->id);
   if(STATE_DEBBUG==1) printf("3: state attach to graph : 13 \n");

}

static int _queue_state_inject( queue_state_t * queue_state , sched_tunable_t * tunable_scheduler)
{

    // destroy le tunable_scheduler->events,
    // copier l'état actuel dans le scheduler.
  int level=1;     int ilevel; for(ilevel = 0 ; ilevel < level ; ilevel++) if(STATE_DEBBUG==1) printf("  ");
    if(STATE_DEBBUG==1) printf("queue state_inject \n");

     tunable_scheduler->events = fifo_tunable_copy( queue_state->events , fifo_tunable_event_deep_copy  ) ;

     return 1;
}

int state_inject(state_t * state)
{
    // inject queue_state
    if(STATE_DEBBUG==1) printf("State_inject :    %d\n",state->id);
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
             (struct _event_t *) fifo_tunable_get_at(state->queue_state->events,
              state->allowed_output_transitions[trans]),(struct state_t *) state, trans);

      state_add_output_transition(state,transition);

      return transition;
  }

/*
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
 */

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

    if(TYPE_OF_DUMP!=DUMP_ONLY_CONTENT)  
		_allowed_transition_dump(stream,state);
    _routing_state_dump(stream,state->routing_state);

	if(TYPE_OF_DUMP!=DUMP_ONLY_CONTENT)  
		_transitions_dump(stream,state);
    return 1;
}

int state_flat_dump(gds_stream_t * stream, state_t * state)
{
    _queue_state_dump(stream,state->queue_state);
//    state_calculate_allowed_output_transitions(state);
   _routing_state_flat_dump(stream,state->routing_state);

    return 1;
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

// 0 = identical
// other value, otherwise
int _queue_states_equivalent_v2(queue_state_t * qs1, queue_state_t  * qs2 )
{
    if(qs1->events->current_depth != qs2->events->current_depth )
    {
        return -1;
    }
    if(qs1->max_nb_of_msg_in_one_oriented_session != qs2->max_nb_of_msg_in_one_oriented_session)
    {
        return -2;
    }

    net_addr_t src;
    net_addr_t dst;

    unsigned int index1;
    unsigned int index2;
    unsigned int nb_msg = 0;
    //int * tab1_visited = (int *) MALLOC ( qs1->events->current_depth * sizeof(int));
    //int * tab2_visited = (int *) MALLOC ( qs1->events->current_depth * sizeof(int));
    int tab1_visited[qs1->events->current_depth];
    int tab2_visited[qs1->events->current_depth];

    for(index1=0; index1<qs1->events->current_depth ; index1++)
    {
        tab1_visited[index1] = 0;
        tab2_visited[index1] = 0;
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

void state_mark_for_can_lead_to_final_state(state_t * state , unsigned int marking_sequence_number )
{
    // si je suis déjà marqué avec ce numéro, je ne fais rien !
    //printf("State id %u,  marking sequence number : %u", state->id,state->marking_sequence_number  );
    if(state->marking_sequence_number == marking_sequence_number)
        return;

    state->marking_sequence_number = marking_sequence_number;
    state->type = state->type | STATE_CAN_LEAD_TO_A_FINAL_STATE;
    //printf("   new seq num : %u \n", state->marking_sequence_number  );

    // marquer tout ceux qui mènent à moi !
    unsigned int i;
    for( i = 0 ; i < state->nb_input ; i++)
    {
        state_mark_for_can_lead_to_final_state(state->input_transitions[i]->from, marking_sequence_number );
    }
    
}

int state_export_to_file(state_t * state)
{

	TYPE_OF_DUMP = DUMP_ONLY_CONTENT;

    char file_name[256];

    time_t curtime;
    struct tm *loctime;

    /* Get the current time.  */
    curtime = time (NULL);

    /* Convert it to local time representation.  */
    loctime = localtime (&curtime);

    /* Print out the date and time in the standard format.  */



    //sprintf(file_name,"/home/yo/tmp/tracer_cbgp/tracer_state_%u",state->id);
    sprintf(file_name,"/home/yo/tmp/tracer_cbgp/%d_%d_%d_%d:%d:%d_state_%u.txt",loctime->tm_year,loctime->tm_mon,loctime->tm_mday,loctime->tm_hour,loctime->tm_min,loctime->tm_sec,state->id);
    
    gds_stream_t * stream = stream_create_file(file_name);



    //FILE* fichier = NULL;
    //char file_name[256];
    //sprintf(file_name,"/home/yo/tmp/tracer_cbgp/tracer_state_%u",state->id);

    //tracer_state_dump( stream, tracer , state->id);
    state_flat_dump(stream,state);
    stream_destroy(&stream);

	TYPE_OF_DUMP = DUMP_DEFAULT;
    return 0;
}
