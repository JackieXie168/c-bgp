

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
#include "graph.h"
#include "tracer.h"




static unsigned int state_next_available_id= 0;
static int Type_Of_Dump = DUMP_DEFAULT;

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

/*
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
  state->depth = 0;
  
  state_calculate_allowed_output_transitions(state);
  graph_add_state(state->graph,state,state->id);
  

  return state;
}
*/


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
  state->depth = 0;
  return state;
}


void state_tag_newly_added_state_session_waiting_time(state_t * state)
{
    assert(state->session_waiting_time == NULL);

    state->session_waiting_time = create_session_waiting_time_container(state->queue_state);
    
    if(state->nb_input == 0)
    {
        assert(state->input_transitions == NULL);
        unsigned int i;
        for (i = 0 ; i < state->queue_state->nb_oriented_bgp_session ; i++)
        {
            state->session_waiting_time[i]->max_waiting_time=0;
            state->session_waiting_time[i]->min_waiting_time=0;
        }
    }
    else
    {
        assert(state->nb_input == 1 && state->input_transitions != NULL);
        unsigned int i;
        for (i = 0 ; i < state->queue_state->nb_oriented_bgp_session ; i++)
        {
            if(  state->session_waiting_time[i]->from == get_src_addr(state->input_transitions[0]->event)
                    && state->session_waiting_time[i]->to == get_dst_addr(state->input_transitions[0]->event))
            {
                state->session_waiting_time[i]->max_waiting_time=0;
                state->session_waiting_time[i]->min_waiting_time=0;
            }
            else
            {
                // chercher cette session dans l'état d'ou on vient
                session_waiting_time_t * swt = search_waiting_time_value_for_this_session(
                   state->input_transitions[0]->from, state->session_waiting_time[i]->from,state->session_waiting_time[i]->to);
                if(swt == NULL) // c'est donc un nouveau msg
                {
                    state->session_waiting_time[i]->max_waiting_time=0;
                    state->session_waiting_time[i]->min_waiting_time=0;
                }
                else
                {
                    state->session_waiting_time[i]->max_waiting_time = swt->max_waiting_time + 1;
                    state->session_waiting_time[i]->min_waiting_time = swt->min_waiting_time + 1;
                }
            }
        }
    }
}

void state_attach_to_graph(state_t * state, struct transition_t * the_input_transition)
{
  if(the_input_transition != NULL)
  { 

    assert(state->input_transitions == NULL);
    state->input_transitions = (struct transition_t **) MALLOC( 1 * sizeof(struct transition_t *));
    state->nb_input=1; 
    state->input_transitions[0]=the_input_transition; 
    state->input_transitions[0]->to = state;
    state->depth = the_input_transition->depth;
  }
  else
  {
    state->input_transitions = NULL; 
    state->nb_input=0;
    state->depth = 0;
  }

  state->output_transitions = NULL;
  state->nb_output=0;
 
  //state->allowed_output_transitions=NULL;
  //state->nb_allowed_output_transitions=0;

  state_calculate_allowed_output_transitions(state);
 
  state_next_available_id++;
 
  graph_add_state(state->graph,state,state->id);

  state_tag_newly_added_state_session_waiting_time(state);
  

}

int state_inject(state_t * state)
{
    // inject queue_state
    if(STATE_DEBBUG==STATE_DEBBUG_YES) printf("State_inject :    %d\n",state->id);
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
    the_output_transition->depth = state->depth + 1;
}

void state_with_new_input_tag_session_waiting_time_(state_t * state, struct transition_t * the_input_transition)
{
    assert(state->session_waiting_time != NULL);

    // si c'est root, c'est possible que le nb de transition soit = 1

        assert(state->nb_input >= 1 && state->input_transitions != NULL);
        unsigned int i;
        for (i = 0 ; i < state->queue_state->nb_oriented_bgp_session ; i++)
        {
            if(  state->session_waiting_time[i]->from == get_src_addr(the_input_transition->event)
                    && state->session_waiting_time[i]->to == get_dst_addr(the_input_transition->event))
            {
                state->session_waiting_time[i]->min_waiting_time=0;
            }
            else
            {
                // chercher cette session dans l'état d'ou on vient
                session_waiting_time_t * swt = search_waiting_time_value_for_this_session(
                   the_input_transition->from, state->session_waiting_time[i]->from,state->session_waiting_time[i]->to);
                if(swt == NULL) // c'est donc un nouveau msg
                {
                    state->session_waiting_time[i]->min_waiting_time=0;
                }
                else
                {
                    if( state->session_waiting_time[i]->max_waiting_time < swt->max_waiting_time + 1)
                        state->session_waiting_time[i]->max_waiting_time = swt->max_waiting_time + 1;
                    if( state->session_waiting_time[i]->min_waiting_time > swt->min_waiting_time + 1)
                        state->session_waiting_time[i]->min_waiting_time = swt->min_waiting_time + 1;
                }
            }
        }
    
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

    if(the_input_transition->depth > 0  &&
            the_input_transition->depth < state->depth)
        state->depth = the_input_transition->depth;

   state_with_new_input_tag_session_waiting_time_(state, the_input_transition);
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

    if(Type_Of_Dump!=DUMP_ONLY_CONTENT)
		_allowed_transition_dump(stream,state);
    _routing_state_dump(stream,state->routing_state);

	if(Type_Of_Dump!=DUMP_ONLY_CONTENT)
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

    Type_Of_Dump = DUMP_ONLY_CONTENT;

    char file_name[256];

    time_t curtime;
    struct tm *loctime;

    /* Get the current time.  */
    curtime = time (NULL);

    /* Convert it to local time representation.  */
    loctime = localtime (&curtime);

    /* Print out the date and time in the standard format.  */



    //sprintf(file_name,"/home/yo/tmp/tracer_cbgp/tracer_state_%u",state->id);
    sprintf(file_name,"%s%s_state_%u.txt",state->graph->tracer->base_output_directory,state->graph->tracer->base_output_file_name,state->id);
    
    gds_stream_t * stream = stream_create_file(file_name);



    //FILE* fichier = NULL;
    //char file_name[256];
    //sprintf(file_name,"/home/yo/tmp/tracer_cbgp/tracer_state_%u",state->id);

    //tracer_state_dump( stream, tracer , state->id);
    state_flat_dump(stream,state);
    stream_destroy(&stream);

    Type_Of_Dump = DUMP_DEFAULT;
    return 0;
}

    int get_state_type_of_dump()
    {
        return Type_Of_Dump;
    }


/*
    int state_export_dot(state_t * state)
    {
        state->graph->list_of_nodes
        state->routing_state->

    }
*/


int state_export_dot_to_file(state_t * state)
{
    char file_name[256];
    sprintf(file_name,"%s%s_state_%u.dot",state->graph->tracer->base_output_directory,state->graph->tracer->base_output_file_name,state->id);

    gds_stream_t * stream = stream_create_file(file_name);

    routing_state_export_dot(stream,state->routing_state);

    stream_destroy(&stream);
    char commande[1024];
    //sprintf(commande,"neato -Tpng %s -o%s.png",file_name, file_name);
    //system(commande);

    sprintf(commande,"neato -T%s %s -o%s.%s",state->graph->tracer->IMAGE_FORMAT,file_name, file_name,state->graph->tracer->IMAGE_FORMAT);
    system(commande);
    return 0;
}


session_waiting_time_t * search_waiting_time_value_for_this_session(state_t * state, net_addr_t from, net_addr_t to)
{
    unsigned int i = 0;
    if(state->session_waiting_time == NULL)
    {
        return NULL;
    }

    for(i = 0 ; i < state->queue_state->nb_oriented_bgp_session ; i++)
    {
        if( state->session_waiting_time[i]->from == from
                && state->session_waiting_time[i]->to == to )
            return state->session_waiting_time[i];
    }
    return NULL;
}


void state_tag_waiting_time_HTML_dump(gds_stream_t * stream, state_t * state)
{
    unsigned int i;

    if(state->queue_state->nb_oriented_bgp_session == 0)
        return;

    stream_printf(stream, "<TABLE>");
    for(i = 0 ; i < state->queue_state->nb_oriented_bgp_session ; i++)
    {
        stream_printf(stream, "<TR>");
        stream_printf(stream, "<TD>");
        ip_address_dump(stream,state->session_waiting_time[i]->from);
        stream_printf(stream, "</TD>");
        stream_printf(stream, "<TD>--&gt;</TD>");
        stream_printf(stream, "<TD>");
        ip_address_dump(stream,state->session_waiting_time[i]->to);
        stream_printf(stream, "</TD>");
        stream_printf(stream, "<TD>%u</TD>",state->session_waiting_time[i]->min_waiting_time );
        stream_printf(stream, "<TD>%u</TD>",state->session_waiting_time[i]->max_waiting_time );
        stream_printf(stream, "</TR>");
    }
    stream_printf(stream, "</TABLE>");
}