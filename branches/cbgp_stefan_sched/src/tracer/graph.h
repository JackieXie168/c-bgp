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

static unsigned int MAX_STATE = 100;

// -----[ sched_t ]--------------------------------------------------
/** Definition of a state. */
typedef struct graph_t {
  struct tracer_t       *   tracer;
  net_node_t            **  list_of_nodes;
  struct state_t        **  list_of_states;
  struct state_t        *   state_root;

  struct state_t        *   FOR_TESTING_PURPOSE_current_state;
  unsigned int         nb_states;
} graph_t;



#ifdef	__cplusplus
extern "C" {
#endif

    graph_t * graph_create(struct tracer_t * tracer);
    int graph_add_state(graph_t * the_graph, struct state_t * the_state, unsigned int index);

    int FOR_TESTING_PURPOSE_graph_state_dump(gds_stream_t * stream, graph_t * graph, unsigned int num_state);
    int FOR_TESTING_PURPOSE_graph_current_state_dump(gds_stream_t * stream, graph_t * graph);
    int graph_allstates_dump(gds_stream_t * stream, graph_t * graph);
    //int FOR_TESTING_PURPOSE_graph_inject_state(graph_t * graph , unsigned int num_state);

    int graph_inject_state(graph_t * graph , unsigned int num_state);
    int graph_state_dump(gds_stream_t * stream, graph_t * graph, unsigned int num_state);


    void graph_export_dot(gds_stream_t * stream, graph_t * graph);

    struct state_t * graph_search_identical_state(graph_t * graph, struct state_t * state);

#ifdef	__cplusplus
}
#endif

#endif	/* __GRAPH_H__ */
