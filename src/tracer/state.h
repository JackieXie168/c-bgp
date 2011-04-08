/* 
 * File:   state.h
 * Author: yo
 *
 * Created on 25 novembre 2010, 11:48
 */

#ifndef __STATE_H__
#define	__STATE_H__

#include <libgds/memory.h>
#include <libgds/stack.h>
#include <libgds/types.h>
#include <libgds/stream.h>
#include <net/error.h>
#include <libgds/fifo_tunable.h>
#include <net/routing_t.h>
#include <bgp/types.h>



struct routing_state_t;
struct queue_state_t;
struct state_t;

#define DUMP_ONLY_CONTENT 1
#define DUMP_STATE 0
#define DUMP_DEFAULT DUMP_STATE


#define STATE_DEBBUG_YES 1
#define STATE_DEBBUG_NO 2
#define STATE_DEBBUG STATE_DEBBUG_NO



#include <tracer/transition.h>
#include <tracer/graph.h>
#include <tracer/tracer.h>
#include <tracer/queue_state.h>
#include <tracer/routing_state.h>




typedef struct session_waiting_time_t
{
    net_addr_t  from;
    net_addr_t  to;
    int         max_waiting_time;
    int         min_waiting_time;
}session_waiting_time_t;

// -----[ sched_t ]--------------------------------------------------
/** Definition of a state. */
typedef struct state_t {
  routing_state_t *    routing_state;
  queue_state_t   *    queue_state;
  unsigned int *                allowed_output_transitions;
  unsigned int                  nb_allowed_output_transitions;
  struct transition_t    **   output_transitions;
  struct transition_t    **   input_transitions;
  unsigned int nb_output;
  unsigned int nb_input;
  struct graph_t         *    graph;
  unsigned int          id;
  uint8_t                type;

  unsigned int          marking_sequence_number;

  uint8_t                blocked;

  unsigned int          depth;

  session_waiting_time_t   ** session_waiting_time;

} state_t;

// ----- Global BGP options --------
#define STATE_ROOT                      0x01
#define STATE_FINAL                     0x02
#define STATE_MEMBER_OF_A_CYCLE         0x04
#define STATE_CAN_LEAD_TO_A_CYCLE       0x08
#define STATE_CAN_LEAD_TO_A_FINAL_STATE 0x10

// ----- Global BGP options --------
#define STATE_NOT_BLOCKED                   0x00
#define STATE_DEFINITELY_BLOCKED            0x01
#define STATE_BLOCKED_BECAUSE_OF_TOO_DEEP   0x02
#define STATE_TOTALY_EXPLORED               0x04



#ifdef	__cplusplus
extern "C" {
#endif

    


    //state_t * state_create(struct tracer_t * tracer, struct transition_t * the_input_transition);

    state_t * state_create_isolated(struct tracer_t * tracer);


    void state_add_output_transition(state_t * state,  struct transition_t * the_output_transition);

    int state_dump(gds_stream_t * stream, state_t * state);

    int state_inject(state_t * state);

    unsigned int state_calculate_allowed_output_transitions(state_t * state);

    struct transition_t * state_generate_transition(state_t * state, unsigned int trans);

    //int state_generate_all_transitions(state_t * state);

    int state_identical(state_t * state1 , state_t * state2 );

    void state_mark_for_can_lead_to_final_state(state_t * state , unsigned int marking_sequence_number );

    int state_export_to_file(state_t * state);

    int state_export_dot_to_file(state_t * state);


    int get_state_type_of_dump();
    session_waiting_time_t * search_waiting_time_value_for_this_session(state_t * state, net_addr_t from, net_addr_t to);

void state_tag_waiting_time_HTML_dump(gds_stream_t * stream, state_t * state);

#ifdef	__cplusplus
}
#endif

#endif	/* __STATE_H__ */

