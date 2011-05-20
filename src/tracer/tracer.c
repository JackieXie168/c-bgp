

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include <time.h>


#include <assert.h>
#include <libgds/str_util.h>
#include <libgds/memory.h>
#include <libgds/stack.h>
#include <libgds/fifo.h>
#include <libgds/lifo.h>


#include <libgds/memory.h>
#include <libgds/trie.h>


#include <net/error.h>
#include <net/icmp.h>
#include <net/link.h>
#include <net/link-list.h>
#include <net/net_types.h>
#include <net/netflow.h>
#include <net/network.h>
#include <net/node.h>
#include <net/subnet.h>

#include <tracer/transition.h>
#include <tracer/tracer.h>
#include <tracer/graph.h>
#include <tracer/state.h>
#include <tracer/transition.h>

#include "tracer.h"
#include "state.h"
#include "transition.h"
#include "graph.h"


#define IMAGE_GRAPHVIZ_FORMAT "eps"




static tracer_t  * _default_tracer= NULL;

/*
int FOR_TESTING_PURPOSE_tracer_go_one_step(tracer_t * self )
{
    if(self->started == 0)
        return -1;

    //actuellement graphe est une simple chaine
    // injecter l'état actuel dans le système (scheduler ou simulator)
    // faire simuler un pas (le premier message sera celui traité)
    // capturer l'état actuel, le mettre dans un nouvel état et le lier au précédent.
    // noter le nouvel état comme l'état actuel.

    state_t * current_state = self->graph->FOR_TESTING_PURPOSE_current_state;
    // injecter l'état
    // ici ne rien faire car c'est juste une liste, la file n'a pas bougé
    //construire la transition avec le prochain event qui sera simulé
    transition_t * transition = transition_create_from(get_event_at((struct sched_t *)tracer_get_tunable_scheduler(self),0),current_state, 0);
    state_add_output_transition(current_state,transition);
    //simuler le prochain événement.
    sim_step(tracer_get_simulator(self), 1);
    //construire l'état résultant.
    state_t * new_state = state_create(self,transition);

    self->graph->FOR_TESTING_PURPOSE_current_state = new_state;    

    return 1;
}
 * */


typedef struct state_trans_t {
  unsigned int state;
  unsigned int trans;
} state_trans_t;

void destroy_struct_state_trans(void ** structstatetrans)
{
    //FREE(structstatetrans);
}


static unsigned int MAXDEPTH = 10000;

int tracer_trace_whole_graph(tracer_t * self)
{
   return  tracer_trace_whole_graph_v1bis(self);
}


int tracer_trace_whole_graph_v0(tracer_t * self)
{
    if(self->started == 1)
        return -1;

    unsigned int work = 0;
    unsigned int nbtrans;
    unsigned int current_state_id;
    unsigned int i;
    state_t * current_state;
    gds_fifo_t * list_of_state_trans;

    state_trans_t * state_trans;

    _tracer_start(self);

    list_of_state_trans = fifo_create(MAXDEPTH, destroy_struct_state_trans);

    //amorce :
    // ajouter toutes les transitions dispo de l'état 0 dans la fifo
    current_state_id=0;
    current_state = self->graph->list_of_states[current_state_id];
    nbtrans = state_calculate_allowed_output_transitions(current_state);

    for( i = 0 ; i < nbtrans ; i++ )
    {
        state_trans = (struct state_trans_t *) MALLOC ( sizeof(struct state_trans_t));
        state_trans->state=current_state_id;
        state_trans->trans=i;
        fifo_push(list_of_state_trans, state_trans);
    }
//  fin de l'amorce


    while(fifo_depth(list_of_state_trans)!=0)
    {
        state_trans = (state_trans_t *) fifo_pop(list_of_state_trans);
        work++;
        printf("Generated transition : %u\n",work);
       // if(work>18)
       //         tracer_graph_export_dot(gdsout,self);
if(work==200)
{
return;
}
        //printf("2: before tracing");
        tracer_trace_from_state_using_transition(self, state_trans->state,state_trans->trans);
        //printf("2: after tracing");
        // si nouvel état créé, on l'ajoute dans la file, sinon on passe!
        // c'est un nouvel état si
        // a partir de l'état créé, on prend la transition créée, on prend l'état au bout de la transition
        // si cet état a une seule input_transition alors c'est un nouveau.
        current_state = self->graph->list_of_states[state_trans->state];
        // trouver la transition qui porte le numéro state_trans->trans
        for(i=current_state->nb_output-1; i>=0; i--)
        {
            if(current_state->output_transitions[i]->num_trans==state_trans->trans)
                break;
        }
        assert(i>=0);
        if( current_state->output_transitions[i]->to->nb_input == 1)
        {
            current_state = current_state->output_transitions[i]->to;
            current_state_id = current_state->id;
            nbtrans = state_calculate_allowed_output_transitions(current_state);

            for( i = 0 ; i < nbtrans ; i++ )
            {
                state_trans = (state_trans_t *) MALLOC ( sizeof(state_trans_t));
                state_trans->state=current_state_id;
                state_trans->trans=i;
                fifo_push(list_of_state_trans, state_trans);
            }
        }
    }
    return self->graph->nb_states;
}

// same as v0 but use a lifo instead of a fifo
// should be refactored with the v0 (because it's just a copy/paste)
int tracer_trace_whole_graph_v1(tracer_t * self)
{
    if(self->started == 1)
        return -1;

    unsigned int work = 0;
    unsigned int nbtrans;
    unsigned int current_state_id;
    unsigned int i;
    state_t * current_state;
    gds_lifo_t * list_of_state_trans;

    state_trans_t * state_trans;

    _tracer_start(self);

    list_of_state_trans = lifo_create(MAXDEPTH, destroy_struct_state_trans);

    //amorce :
    // ajouter toutes les transitions dispo de l'état 0 dans la fifo
    current_state_id=0;
    current_state = self->graph->list_of_states[current_state_id];
    nbtrans = state_calculate_allowed_output_transitions(current_state);

    for( i = 0 ; i < nbtrans ; i++ )
    {
        state_trans = (struct state_trans_t *) MALLOC ( sizeof(struct state_trans_t));
        state_trans->state=current_state_id;
        state_trans->trans=i;
        lifo_push(list_of_state_trans, state_trans);
    }
//  fin de l'amorce


    while(lifo_depth(list_of_state_trans)!=0)
    {
        state_trans = (state_trans_t *) lifo_pop(list_of_state_trans);
        work++;
        printf("Generated transition : %u\n",work);
       // if(work>18)
       //         tracer_graph_export_dot(gdsout,self);
if(work==200)
{
return;
}
        //printf("2: before tracing");
        tracer_trace_from_state_using_transition(self, state_trans->state,state_trans->trans);
        //printf("2: after tracing");
        // si nouvel état créé, on l'ajoute dans la file, sinon on passe!
        // c'est un nouvel état si
        // a partir de l'état créé, on prend la transition créée, on prend l'état au bout de la transition
        // si cet état a une seule input_transition alors c'est un nouveau.
        current_state = self->graph->list_of_states[state_trans->state];
        // trouver la transition qui porte le numéro state_trans->trans
        for(i=current_state->nb_output-1; i>=0; i--)
        {
            if(current_state->output_transitions[i]->num_trans==state_trans->trans)
                break;
        }
        assert(i>=0);
        if( current_state->output_transitions[i]->to->nb_input == 1)
        {
            current_state = current_state->output_transitions[i]->to;
            current_state_id = current_state->id;
            nbtrans = state_calculate_allowed_output_transitions(current_state);

            for( i = 0 ; i < nbtrans ; i++ )
            {
                state_trans = (state_trans_t *) MALLOC ( sizeof(state_trans_t));
                state_trans->state=current_state_id;
                state_trans->trans=i;
                lifo_push(list_of_state_trans, state_trans);
            }
        }
    }
    return self->graph->nb_states;
}


// same as v1 but with a max graph depth.
// should be refactored with the v1
int tracer_trace_whole_graph_v1bis(tracer_t * self)
{
    if(self->started == 1)
        return -1;

    unsigned int work = 0;
    unsigned int nbtrans;
    unsigned int current_state_id;
    unsigned int i;
    state_t * current_state;
    gds_lifo_t * list_of_state_trans;

    struct state_trans_t * state_trans;

    _tracer_start(self);

    list_of_state_trans = lifo_create(MAXDEPTH, destroy_struct_state_trans);
    
    //amorce :
    // ajouter toutes les transitions dispo de l'état 0 dans la fifo
    current_state_id=0;
    current_state = self->graph->list_of_states[current_state_id];
    nbtrans = state_calculate_allowed_output_transitions(current_state);

    for( i = 0 ; i < nbtrans ; i++ )
    {
        state_trans = (struct state_trans_t *) MALLOC ( sizeof(struct state_trans_t));
        state_trans->state=current_state_id;
        state_trans->trans=i;
        lifo_push(list_of_state_trans, state_trans);
    }
//  fin de l'amorce
    
    unsigned int MAX_GRAPH_DEPTH = 38;
    while(lifo_depth(list_of_state_trans)!=0)
    {   
        state_trans = (struct state_trans_t *) lifo_pop(list_of_state_trans);
        work++;
        printf("Treated transitions : %u, From state %u at depth %u using transition nb %u\n",work, state_trans->state,  self->graph->list_of_states[state_trans->state]->depth, state_trans->trans);
       // if(work>18)
       //         tracer_graph_export_dot(gdsout,self);
if(work==10000)
{
return;
} 
        //printf("2: before tracing :  state %u , trans %u\n",state_trans->state ,state_trans->trans);
        tracer_trace_from_state_using_transition(self, state_trans->state, state_trans->trans);
        //printf("2: after tracing\n");
        // si nouvel état créé, on l'ajoute dans la file, sinon on passe!
        // c'est un nouvel état si
        // a partir de l'état créé, on prend la transition créée, on prend l'état au bout de la transition
        // si cet état a une seule input_transition alors c'est un nouveau.
        //printf("ici\n");//,current_state->depth);
        current_state = self->graph->list_of_states[state_trans->state];
        // trouver la transition qui porte le numéro state_trans->trans
        for(i=current_state->nb_output-1; i>=0; i--)
        {
            if(current_state->output_transitions[i]->num_trans==state_trans->trans)
                break;
        }
        assert(i>=0);
        if( current_state->output_transitions[i]->to->nb_input == 1)
        {
            current_state = current_state->output_transitions[i]->to;
            current_state_id = current_state->id;
            nbtrans = state_calculate_allowed_output_transitions(current_state);
            printf("new state : %u - depth :%u\n",current_state->id, current_state->depth);

            if(current_state->depth < MAX_GRAPH_DEPTH)
            {
              for( i = 0 ; i < nbtrans ; i++ )
              {
                state_trans = (state_trans_t *) MALLOC ( sizeof(state_trans_t));
                state_trans->state=current_state_id;
                state_trans->trans=i;
                lifo_push(list_of_state_trans, state_trans);
              }
            }
        }
    }
   if(lifo_depth(list_of_state_trans)==0)
   {
       printf("All transitions treated untill depth %u \n", MAX_GRAPH_DEPTH);
   }

    return self->graph->nb_states;
}



int tracer_trace_whole_graph_v2(tracer_t * self)
{
    if(self->started == 1)
        return -1;

    _tracer_start(self);

    unsigned int nbtrans;
    unsigned int i;
    unsigned int transition = 0;
    state_t * min_state;

    unsigned int nb_trans_limit = 1000;
    while( transition < nb_trans_limit
            && (min_state = get_state_with_mininum_bigger_number_of_msg_in_session(self->graph))
             != NULL )
    {
        //printf("Min_state = %u \n",min_state->id);

        // traiter toutes ses transitions.
        // ici quand on prend un état, on traite toutes ses transitions !
        nbtrans = state_calculate_allowed_output_transitions(min_state);

        for( i = 0 ; i < nbtrans ; i++)
        {
           // printf("\t Transition num %u \n",i);

            tracer_trace_from_state_using_transition(self,min_state->id,i);
            transition++;
        }
    }
    printf("nb transition : %u \n",transition);
    return self->graph->nb_states;
}


int tracer_trace_whole_branch_from_state(tracer_t * self, unsigned int state_id)
{
    if(self->started == 0)
        return -1;


}


int tracer_trace_from_state_using_transition(tracer_t * self, unsigned int state_id, unsigned int transition_id )
{
    state_t * origin_state;

    int debug = 0;
    
    if(self->started == 0)
        return -1;

    //chercher l'état,
    //l'injecter dans cbgp
    //choisir de simuler l'événement num_event
    //créer la transition
    //prendre copie de l'état courant de cbgp
    //attacher cet état correctement dans le graphe.

    if(state_id >= graph_max_num_state())
    {
        if(debug==1) printf("ATTENTION ! nombre max d'état = %d\n",graph_max_num_state());
        return 1;
    }
    assert(self->graph->list_of_states != NULL );
    origin_state = self->graph->list_of_states[state_id];
    if(origin_state == NULL)
    {
        if(debug==1) printf("Unexisting state !\n");
        return 1;
    }

    state_inject(origin_state);

    transition_t * transition = state_generate_transition(origin_state,transition_id);
    // TO DO TODO
    //vérifier que la transition n'a pas déjà été visitée !
    // transition->to == NULL
    if(transition == NULL) // actuellement si transition déjà générée --> return NULL
        return 10;

    unsigned int num_event =  origin_state->allowed_output_transitions[transition_id];

    //let the num_event be the next event to run
    if(debug==1) printf("2: tracing : before setfirst\n");
    tracer_get_simulator(self)->sched->ops.set_first(tracer_get_simulator(self)->sched, num_event );
    if(debug==1) printf("2: tracing : before sim step 1\n");

    sim_step(tracer_get_simulator(self), 1);
    if(debug==1) printf("2: tracing : before create state\n");

    state_t * new_state = state_create_isolated(self);

    // vérifier si l'état n'est pas déjà présent
     if(debug==1) printf("2: tracing : before check identical state\n");

    state_t * identical_state = graph_search_identical_state(self->graph, new_state);
     if(debug==1) printf("2: tracing : after check if there is an identical state\n");

    if( identical_state == NULL )
    {
        // attacher l'état au graphe  (et valider le numéro state-id)
        if(debug==1) printf("2: tracing : before attach state to graph\n");

        state_attach_to_graph(new_state, transition);
                if(debug==1) printf("2: tracing : after attach state to graph\n");

    }
    else
    {
                if(debug==1) printf("2: tracing : before add input transition\n");

        state_add_input_transition(identical_state,transition);
                if(debug==1) printf("2: tracing : after add input trans\n");

        // récupérer l'état identique
        // libérer l'état nouvellement créé :
        // free(state) ???
        // ajouter une transition entrante
        // ne pas valider le numéro state-id
        FREE(new_state);
    }

     if(debug==1) printf("2: tracing : end of tracing\n");



    return 0;
}

int _tracer_dump(gds_stream_t * stream, tracer_t * tracer)
{
    if(tracer->started == 0)
    {
        stream_printf(stream,"Tracer not yet started\n");        
        return 0;
    }
    else
    {
        stream_printf(stream,"Tracer Ready : \n");
        return state_dump(stream,tracer->graph->state_root);
        //return 1;
    }
}

/*
int FOR_TESTING_PURPOSE_tracer_graph_state_dump(gds_stream_t * stream, tracer_t * self , unsigned int num_state)
{
    return FOR_TESTING_PURPOSE_graph_state_dump(stream,self->graph,num_state);
}*/

/*int FOR_TESTING_PURPOSE_tracer_graph_current_state_dump(gds_stream_t * stream, tracer_t * self)
{
    return FOR_TESTING_PURPOSE_graph_current_state_dump(stream,self->graph);
}
*/

   int tracer_graph_allstates_dump(gds_stream_t * stream,tracer_t *  tracer)
   {
          return graph_allstates_dump(stream,tracer->graph);
   }

/*
int FOR_TESTING_PURPOSE_tracer_inject_state(tracer_t * tracer , unsigned int num_state)
{
    return FOR_TESTING_PURPOSE_graph_inject_state(tracer->graph , num_state);
}
 * */

sched_tunable_t * tracer_get_tunable_scheduler(tracer_t * tracer)
{
    return (sched_tunable_t *) tracer->network->sim->sched;
}

simulator_t * tracer_get_simulator(tracer_t * tracer)
{
    return tracer->network->sim;
}

  int tracer_inject_state(tracer_t * tracer , unsigned int num_state)
  {
      return graph_inject_state(tracer->graph , num_state);
  }

   int tracer_state_dump(gds_stream_t * stream, tracer_t * tracer , unsigned int num_state)
   {
      return graph_state_dump(stream, tracer->graph , num_state);

   }

   void tracer_graph_export_dot(gds_stream_t * stream,tracer_t * tracer)
   {
       graph_export_dot(stream,tracer->graph);
   }




/////////////////////////////////////////////////////////////////////
//
// INITIALIZATION AND FINALIZATION SECTION
//
/////////////////////////////////////////////////////////////////////

// -----[ _network_init ]--------------------------------------------

// concider the simulator current state as the initial state.


tracer_t * tracer_create(network_t * network)
{
  tracer_t * tracer;
  tracer=(tracer_t *) MALLOC(sizeof(tracer_t));
  tracer->network = network;
  tracer->graph = graph_create(tracer);
  tracer->started = 0;
  tracer->nodes = NULL;
  tracer->nb_nodes = 0;
          //(net_node_t **) MALLOC(sizeof(net_node_t *) * trie_num_nodes(network->nodes, 1));

  time_t curtime;
  struct tm * loctime;
  /* Get the current time.  */
  curtime = time (NULL);
  /* Convert it to local time representation.  */
  loctime = localtime (&curtime);
  //tracer->base_output_file_name = (char *) MALLOC ( 256 * sizeof(char));

  sprintf(tracer->base_output_directory,"/home/yo/tmp/tracer_cbgp/%d_%d_%d_%d:%d:%d/",
      loctime->tm_year,loctime->tm_mon,loctime->tm_mday,loctime->tm_hour,loctime->tm_min,loctime->tm_sec);
  char commande[1024];
  sprintf(commande,"mkdir %s",tracer->base_output_directory);
  system(commande);

  sprintf(tracer->base_output_file_name,"tracer_");
  sprintf(tracer->IMAGE_FORMAT,IMAGE_GRAPHVIZ_FORMAT);
  
  return tracer;
}

tracer_t * _tracer_create()
{
  return tracer_create( network_get_default());
}

tracer_t * tracer_get_default()
{
  assert(_default_tracer != NULL);
  return _default_tracer;
}


void _tracer_init()
{
  _default_tracer= _tracer_create();
}

int _tracer_start(tracer_t * self)
{
     if(self->started == 1)
         return 0;

     self->nodes = (net_node_t **) _trie_get_array(self->network->nodes)->data ;
     self->nb_nodes = trie_num_nodes(self->network->nodes, 1);

     graph_init(self->graph);

     //self->graph->state_root->graph=self->graph;
     // the first state doesn't have the graph.
     
     self->started = 1;
     return 1;
}



/*
// -----[ _network_done ]--------------------------------------------
void _tracer_done()
{
  if (_default_tracer != NULL)
    tracer_destroy(&_default_tracer);
}
*/


void tracer_graph_export_allStates_to_file(tracer_t * tracer)
{
    graph_export_allStates_to_file(tracer->graph);
}

void tracer_graph_export_dot_to_file(tracer_t * tracer)
{
    graph_export_dot_to_file(tracer->graph);
}

    void tracer_graph_export_dot_allStates_to_file(tracer_t * tracer)
    {
         graph_export_allStates_dot_to_file(tracer->graph);
    }
