/* 
 * File:   state.h
 * Author: yo
 *
 * Created on 25 novembre 2010, 11:48
 */

#ifndef __STATE_H__
#define	__STATE_H__

struct routing_state_t;
struct queue_state_t;
//struct transition_t;
struct state_t;

#include <libgds/memory.h>
#include <libgds/stack.h>
#include <libgds/types.h>
#include <libgds/stream.h>
#include <net/error.h>
#include <libgds/fifo_tunable.h>

#include <tracer/transition.h>
#include <tracer/graph.h>
#include <tracer/tracer.h>




typedef struct {
    net_node_t *    ptr_to_node;
}  couple_node_rib_t;


typedef struct routing_state_t {
  struct state_t              * state;
  struct couple_node_rib_t **    couples_node_rib;

    // les rib   (des couples de [node,rib]   )
    // adj-rib-in and -out pour chaque voisin de chaque noeud.
    // pour chaque noeud :  - rib
    //                      - pour chaque voisin :
    //                                      -adj-rib-in
    //                                      -adj-rib-out

}  routing_state_t;

typedef struct {
const sim_event_ops_t * ops;
void                  * ctx;
//int                     id;
//int                     generator;
} _event_t;

typedef struct  queue_state_t {
    struct state_t              * state;
    gds_fifo_tunable_t          * events;
}  queue_state_t;




// -----[ sched_t ]--------------------------------------------------
/** Definition of a state. */
typedef struct state_t {
  routing_state_t *    routing_state;
  queue_state_t   *    queue_state;
  struct transition_t    **   output_transitions;
  //transition_t    **   input_transitions;
  struct transition_t    *   input_transition;
  struct graph_t         *    graph;
} state_t;



#ifdef	__cplusplus
extern "C" {
#endif

    state_t * state_create(sched_tunable_t * tunable_scheduler);






#ifdef	__cplusplus
}
#endif

#endif	/* __STATE_H__ */

