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


// -----[ sched_t ]--------------------------------------------------
/** Definition of a state. */
typedef struct graph_t {
  net_node_t     **    list_of_nodes;
  struct state_t        **    list_of_states;
  struct state_t        *    state_root;

  struct state_t        * FOR_TESTING_PURPOSE_current_state;

  unsigned int         nb_net_nodes;
  unsigned int         nb_states;
} graph_t;



#ifdef	__cplusplus
extern "C" {
#endif

    graph_t * graph_create(sched_tunable_t * tunable_scheduler);



#ifdef	__cplusplus
}
#endif

#endif	/* __GRAPH_H__ */

