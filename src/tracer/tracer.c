

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <libgds/str_util.h>
#include <libgds/memory.h>
#include <libgds/stack.h>


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






static tracer_t  * _default_tracer= NULL;

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
    transition_t * transition = transition_create(get_event_at(tracer_get_tunable_scheduler(self), 0));
    state_add_output_transition(current_state,transition);
    //simuler le prochain événement.
    sim_step(tracer_get_simulator(self), 1);
    //construire l'état résultant.
    state_t * new_state = state_create(self,transition);

    self->graph->FOR_TESTING_PURPOSE_current_state = new_state;    

    return 1;
}



int tracer_trace_from_state_using_transition(tracer_t * self, unsigned int num_state, unsigned int num_event )
{
    state_t * origin_state;

    if(self->started == 0)
        return -1;

    //chercher l'état,
    //l'injecter dans cbgp
    //choisir de simuler l'événement num_event
    //créer la transition
    //prendre copie de l'état courant de cbgp
    //attacher cet état correctement dans le graphe.

    if(num_state >= graph_max_num_state())
    {
        printf("ATTENTION ! nombre max d'état = %d\n",graph_max_num_state());
        return -1;
    }
    assert(self->graph->list_of_states != NULL );
    origin_state = self->graph->list_of_states[num_state];
    if(origin_state == NULL)
    {
        printf("Unexisting state !\n");
        return -1;
    }

    //// continue from here !!!!


   //////////////////////





    //actuellement graphe est une simple chaine
    // injecter l'état actuel dans le système (scheduler ou simulator)
    // faire simuler un pas (le premier message sera celui traité)
    // capturer l'état actuel, le mettre dans un nouvel état et le lier au précédent.
    // noter le nouvel état comme l'état actuel.

    state_t * current_state = self->graph->FOR_TESTING_PURPOSE_current_state;
    // injecter l'état
    // ici ne rien faire car c'est juste une liste, la file n'a pas bougé
    //construire la transition avec le prochain event qui sera simulé
    transition_t * transition = transition_create(get_event_at(tracer_get_tunable_scheduler(self), 0));
    state_add_output_transition(current_state,transition);
    //simuler le prochain événement.
    sim_step(tracer_get_simulator(self), 1);
    //construire l'état résultant.
    state_t * new_state = state_create(self,transition);

    self->graph->FOR_TESTING_PURPOSE_current_state = new_state;

    return 1;
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

int FOR_TESTING_PURPOSE_tracer_graph_state_dump(gds_stream_t * stream, tracer_t * self , unsigned int num_state)
{
    return FOR_TESTING_PURPOSE_graph_state_dump(stream,self->graph,num_state);
}

int FOR_TESTING_PURPOSE_tracer_graph_current_state_dump(gds_stream_t * stream, tracer_t * self)
{
    return FOR_TESTING_PURPOSE_graph_current_state_dump(stream,self->graph);
}

   int FOR_TESTING_PURPOSE_tracer_graph_allstates_dump(gds_stream_t * stream,tracer_t *  tracer)
   {
          return FOR_TESTING_PURPOSE_graph_allstates_dump(stream,tracer->graph);
   }


int FOR_TESTING_PURPOSE_tracer_inject_state(tracer_t * tracer , unsigned int num_state)
{
    return FOR_TESTING_PURPOSE_graph_inject_state(tracer->graph , num_state);
}

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
