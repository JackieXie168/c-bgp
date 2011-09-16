

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
  state->blocked = STATE_NOT_BLOCKED;

  state->session_waiting_time = NULL;

  state->scolor.r=1;
  state->scolor.g=1;
  state->scolor.b=1;
  return state;
}


void state_block_if_too_long_waiting_time_and_too_many_msg(state_t * state)
{
    // grande proportion des msg sont des messages d'une meme session
    // cette session à un long temps d'attente, et bcp de messages
    unsigned int i;
    unsigned int wt;
    unsigned int nbmsg;
    for(i = 0 ; i < state->queue_state->nb_oriented_bgp_session ; i++)
    {
         wt = state->session_waiting_time[i]->min_waiting_time;
         nbmsg = nb_of_msg_of_this_oriented_bgp_session(
                 state->session_waiting_time[i]->from,
                 state->session_waiting_time[i]->to,
                 state->queue_state);

         if(wt > 4 && nbmsg > 4 )// && nbmsg > state->queue_state->events->current_depth/2 )
         {
             state->blocked = STATE_DEFINITELY_BLOCKED;
             printf("state %u BLOCKED",state->id);
             break;
         }
    }
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

  state_block_if_too_long_waiting_time_and_too_many_msg(state);

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

    assert(state->nb_output < state->nb_allowed_output_transitions);

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

    if(state->nb_output == state->nb_allowed_output_transitions)
    {
        state->blocked = state->blocked | STATE_TOTALY_EXPLORED ;
    }
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
   
   //state_block_if_too_long_waiting_time_and_too_many_msg;
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


unsigned int _state_get_color_for_node(state_t * state, unsigned int node , unsigned int max_value)
{
    // nb peers (voisins)
    net_protocol_t * protocol;
    bgp_router_t * router;
    protocol= protocols_get(state->routing_state->couples_node_routing_info[node]->node->protocols, NET_PROTOCOL_BGP);
    if (protocol == NULL)
    {
        printf("ouille ouille ouille ..., ce noeud n'est pas un bgp router");
        return max_value;
    }
    router = (bgp_router_t *) protocol->handler;

    unsigned int nbVoisin = state->routing_state->couples_node_routing_info[node]->routing_info->bgp_sessions_info->nb_bgp_session_info_;
    unsigned int possibilite = nbVoisin + 1;


    if(state->routing_state->couples_node_routing_info[node]->routing_info->bgp_router_loc_rib_t->nb_local_rib_elem == 0)
        return max_value;


    // normalement une seule info dans loc_rib (puisque un seul reseau annoncé)
    assert(state->routing_state->couples_node_routing_info[node]->routing_info->bgp_router_loc_rib_t->nb_local_rib_elem == 1);

    bgp_peer_t* peer_choosed = state->routing_state->couples_node_routing_info[node]->routing_info->bgp_router_loc_rib_t->bgp_route_[0]->peer;


    unsigned int i = 0;
    while(i < nbVoisin)
    {
        // si c'est le bgp-peer qui est choisi dans la loc-rib, alors stop
        assert(state->routing_state->couples_node_routing_info[node]->routing_info->bgp_sessions_info->bgp_session_info[i]->nb_adj_rib_in_routes<=1);

        if(state->routing_state->couples_node_routing_info[node]->routing_info->bgp_sessions_info->bgp_session_info[i]->nb_adj_rib_in_routes==1
                && state->routing_state->couples_node_routing_info[node]->routing_info->bgp_sessions_info->bgp_session_info[i]->adj_rib_IN_routes[0]->peer == peer_choosed)
        {
            //trouvé !!!
            break;
        }
        i++;
    }
    return i*max_value/possibilite;
}



    /**
     *    retourne valeur entre 0 et 1 
     */
 float _state_compute_based_color(state_t * state, unsigned int num)
 {
     // ceci concerne les noeuds numéros :
     /*
      * si n noeuds :
      * ca concerne les noeuds de numéros  i tels que
      *     i%3 == num
      * 1er noeud h = num
      * et puis les suivants h + 3
      *
      * nb de noeud par couleur :  state->graph->tracer->nb_nodes / 3
      *
      * combien de bits par noeuds :
      *
      *
      */
     unsigned int h = num;
     float color = 1;
     // nb bit en tout pour cette couleur : 16 )


     uint16_t deux_exp_16_moins_1 = 65535;  // ( 2^16 -1)

     uint16_t color_int = 65535;  // ( 2^16 -1)
     unsigned int nb_node_dans_couleur = (state->graph->tracer->nb_nodes -1 ) / 3  +1 ;
     // soit 16 bit dans cette couleur
     unsigned int nbbit_pour_un_noeud = 16 / nb_node_dans_couleur;
     unsigned int nb_valeur_diff = (1 << nbbit_pour_un_noeud) - 1;
     //si 4 bits, ca doit retourner une valeur entre 0 et 2^4 -1, soit 2^4 valeurs au total.
     unsigned int color_for_node;
     while( h < state->graph->tracer->nb_nodes)
     {
        //printf("couleur %u : noeud %u\n",num,h  );
        color_for_node = _state_get_color_for_node(state, h, nb_valeur_diff) ;
        //printf("nb valeur diff possibles : %u,   valeur choisie : %u\n", nb_valeur_diff, color_for_node);

        color_int = color_int << nbbit_pour_un_noeud ;
        color_int = color_int | color_for_node ;
        h = h + 3;
     }

     //printf("total  : %u    sur  %u\n", color_int,deux_exp_16_moins_1);
     return ((float)color_int)/ deux_exp_16_moins_1;
 }

 void state_compute_color_following_routing_info(state_t * state)
 {
     /* soit la décomposition en 3 couleurs de base.
      * un noeud influe sur une seule couleur
      * une couleur peut etre modifiée par plusieurs noeuds.
      * une couleur de bases représentée sur k bits.  (disons 8)
      * 
      * exemple avec 8 noeuds dans la topologie :
      * num de la couleur :     0     |     1     |    2   
      * num des noeuds    :  0  3  6     1  4  7    2  5
      * 
      * soit n noeuds en tout
      * une couleur de base sur k bits 
      * 
      * num de la couleur de base sur laquelle le noeud i influe : 
      * couleur = i % 3 
      * 
      * une couleur peut représenter combien de noeuds ? 
      * nombre de noeuds qui se partagent une couleur pour la représentation :
      *    ((n-1)/3) +1
      * dans une couleur, nombre de bits disponibles pour un noeud : 
      *     k / ((n-1)/3) +1
      * 
      * 
      * pour influencer
      *   
      * 
      * 
      * 
      * 
      */ 

     state->scolor.r =  _state_compute_based_color(state, 0);
     state->scolor.g =  _state_compute_based_color(state, 1);
     state->scolor.b =  _state_compute_based_color(state, 2);
     
     
 }

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

    stream_printf(stream, "<TABLE BORDER=\"0\" CELLSPACING=\"0\" CELLPADDING=\"0\">");
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