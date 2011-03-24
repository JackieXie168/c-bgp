/* 
 * File:   graph.h
 * Author: yo
 *
 * Created on 26 novembre 2010, 12:18
 */

#ifndef __GRAPH_H__
#define	__GRAPH_H__

#include <libgds/memory.h>
#include <libgds/stack.h>

#include <libgds/types.h>
#include <libgds/stream.h>
#include <net/error.h>

#include <sim/tunable_scheduler.h>

struct graph_t;

#include <tracer/tracer.h>
#include <tracer/state.h>
#include <tracer/transition.h>

static unsigned int MAX_STATE = 1000000;
static unsigned int MAX_FINAL_STATES = 100;

#define STATE_FINAL_DOT_STYLE  "peripheries=3,style=filled, colorscheme=blues4,color=4"
#define STATE_ROOT_DOT_STYLE  "shape=box,peripheries=2"
#define STATE_CAN_LEAD_TO_A_FINAL_STATE_DOT_STYLE  "style=filled, colorscheme=blues4,color=2"
#define GRAPH_EDGE_DOT_STYLE  "color=\"#666666\",colorscheme=blues4,labelfontsize=10,labelfontcolor=4"
#define STATE_NOT_COMPLETELY_TREATED_DOT_STYLE  "peripheries=2"


#define TRANSITION_DOT_DUMP_VERSION_ID 1
#define TRANSITION_DOT_DUMP_VERSION_SHORT_DUMP 2
#define TRANSITION_DOT_DUMP_VERSION TRANSITION_DOT_DUMP_VERSION_SHORT_DUMP

/*\definecolor{turquoise}{rgb}{0,0.6666,0.8}
\definecolor{turquoiseLight}{rgb}{0,0.7333,0.88}
\definecolor{bordeau}{rgb}{0.65882353,0,0.22352941}*/


// -----[ sched_t ]--------------------------------------------------
/** Definition of a state. */
typedef struct graph_t {
  struct tracer_t       *   tracer;
  net_node_t            **  list_of_nodes;
  struct state_t        **  list_of_states;
  struct state_t        *   state_root;
  struct state_t        **  list_of_final_states;
  //struct state_t        *   FOR_TESTING_PURPOSE_current_state;
  unsigned int         nb_states;
  unsigned int         nb_final_states;

  unsigned int          marking_sequence_number;

  struct state_t        **  list_of_active_states;
  unsigned int              nb_of_active_states;

} graph_t;



#ifdef	__cplusplus
extern "C" {
#endif

    graph_t * graph_create(struct tracer_t * tracer);
    int graph_add_state(graph_t * the_graph, struct state_t * the_state, unsigned int index);

    int FOR_TESTING_PURPOSE_graph_state_dump(gds_stream_t * stream, graph_t * graph, unsigned int num_state);
    //int FOR_TESTING_PURPOSE_graph_current_state_dump(gds_stream_t * stream, graph_t * graph);
    int graph_allstates_dump(gds_stream_t * stream, graph_t * graph);
    //int FOR_TESTING_PURPOSE_graph_inject_state(graph_t * graph , unsigned int num_state);

    int graph_inject_state(graph_t * graph , unsigned int num_state);
    int graph_state_dump(gds_stream_t * stream, graph_t * graph, unsigned int num_state);


    void graph_export_dot(gds_stream_t * stream, graph_t * graph);

    struct state_t * graph_search_identical_state(graph_t * graph, struct state_t * state);

    void graph_export_allStates_to_file(graph_t * graph);

    void graph_export_dot_allStates_to_file(graph_t * graph);


    int graph_export_dot_to_file(graph_t * graph);

    struct state_t * get_state_with_mininum_bigger_number_of_msg_in_session(graph_t * graph);


#ifdef	__cplusplus
}
#endif

#endif	/* __GRAPH_H__ */

