
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
graph_t * graph_create(sched_tunable_t * tunable_scheduler)
{
  graph_t * graph;
  graph=(graph_t *) MALLOC(sizeof(graph_t));


  // seulement la file d'événements !

  graph->state_root= state_create(tunable_scheduler);
  graph->FOR_TESTING_PURPOSE_current_state = graph->state_root;
  graph->nb_states = 1;

  //net_node_t     **    list_of_nodes;
  //struct state_t        **    list_of_states;
  //unsigned int         nb_net_nodes;
  

  return graph;
}
