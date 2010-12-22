
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <libgds/str_util.h>

#include <libgds/memory.h>
#include <libgds/stack.h>


#include <net/error.h>
#include <net/icmp.h>
#include <net/link.h>
#include <net/link-list.h>
#include <net/net_types.h>
#include <net/netflow.h>
#include <net/network.h>
#include <net/node.h>
#include <net/subnet.h>

#include <tracer/tracer.h>
#include <tracer/graph.h>
#include <tracer/state.h>

#include "graph.h"


// ----- state_create ------------------------------------------------
graph_t * graph_create(tracer_t * tracer)
{
  graph_t * graph;
  graph=(graph_t *) MALLOC(sizeof(graph_t));

  graph->tracer=tracer;
  graph->state_root= NULL;
  graph->FOR_TESTING_PURPOSE_current_state = NULL;
  graph->nb_states = 0;
  
  //struct state_t        **    list_of_states;
  return graph;
}


// ----- state_create ------------------------------------------------
int graph_init(graph_t * graph)
{
    // nettoyer le reste si déjà été utilisé ... (pas pour l'instant)
  assert(graph->state_root == NULL && graph->nb_states == 0);
  graph->state_root= state_create(graph->tracer,NULL);
  graph->FOR_TESTING_PURPOSE_current_state = graph->state_root;
  graph->nb_states = 1;
  //struct state_t        **    list_of_states;
  return graph;
}

int graph_root_dump(gds_stream_t * stream, graph_t * graph)
{
    return state_dump( stream, graph->state_root);
}


int FOR_TESTING_PURPOSE_graph_state_dump(gds_stream_t * stream, graph_t * graph, unsigned int num_state)
{
    stream_printf(stream,"Imprimer l'état %d\n",num_state);
    state_t * current_state = graph->state_root;
    unsigned int i = 0 ;
    
    while( i < num_state)
    {   
        current_state = current_state->output_transitions[0]->to;
        i++;
    }
    
    return state_dump( stream, current_state);
}

int FOR_TESTING_PURPOSE_graph_current_state_dump(gds_stream_t * stream, graph_t * graph)
{
    return state_dump( stream, graph->FOR_TESTING_PURPOSE_current_state);
}



int FOR_TESTING_PURPOSE_graph_allstates_dump(gds_stream_t * stream, graph_t * graph)
{
    stream_printf(stream,"***************\n\nImprimer tous les états \n");
    state_t * current_state = graph->state_root;
    unsigned int i = 0 ;

    while(current_state != NULL)
    {
        stream_printf(stream,"\nEtat %d :\n ",i);
        state_dump( stream, current_state);

        if(current_state->output_transitions != NULL && current_state->output_transitions[0] != NULL)
            current_state = current_state->output_transitions[0]->to;
        else
            current_state=NULL;
        
        i++;
    }
    stream_printf(stream,"\n***************\n\n");

    return 1;
}


    int FOR_TESTING_PURPOSE_graph_inject_state(graph_t * graph , unsigned int num_state)
    {
        
    state_t * current_state = graph->state_root;
    unsigned int i = 0 ;

    while( i < num_state)
    {
        current_state = current_state->output_transitions[0]->to;
        i++;
    }

    //current_state est l'état à injecter.

     return state_inject(current_state);

    

    }
