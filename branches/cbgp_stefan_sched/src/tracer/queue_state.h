/* 
 * File:   queue_state.h
 * Author: yo
 *
 * Created on 24 mars 2011, 11:03
 */

#ifndef QUEUE_STATE_H
#define	QUEUE_STATE_H




struct routing_state_t;
struct queue_state_t;
struct state_t;

#include <tracer/transition.h>
#include <tracer/graph.h>
#include <tracer/tracer.h>
#include <tracer/state.h>



typedef struct {
const sim_event_ops_t * ops;
void                  * ctx;
//int                     id;
//int                     generator;
} _event_t;

typedef struct  queue_state_t {
    struct state_t              * state;
    gds_fifo_tunable_t          * events;
    unsigned int                max_nb_of_msg_in_one_oriented_session;
    unsigned int                nb_oriented_bgp_session;
}  queue_state_t;




#ifdef	__cplusplus
extern "C" {
#endif



void _queue_state_dump(gds_stream_t * stream, queue_state_t * queue_state);

unsigned int  _queue_state_calculate_max_nb_of_msg_in_oriented_bgp_session(queue_state_t * qs);

// ----- state_create ------------------------------------------------
queue_state_t * _queue_state_create(struct state_t * state, sched_tunable_t * tunable_scheduler);

int _queue_state_inject( queue_state_t * queue_state , sched_tunable_t * tunable_scheduler);

int _queue_states_equivalent(queue_state_t * qs1, queue_state_t  * qs2 );
// 0 = identical
// other value, otherwise
int _queue_states_equivalent_v2(queue_state_t * qs1, queue_state_t  * qs2 );

int same_tcp_session(_event_t * event1 , _event_t * event2 );

void _queue_state_flat_simple_HTML_dump(gds_stream_t * stream, queue_state_t * queue_state);

net_addr_t get_dst_addr(_event_t * ev);

net_addr_t get_src_addr(_event_t * ev);

unsigned int nb_of_msg_of_this_oriented_bgp_session(net_addr_t src, net_addr_t dst, queue_state_t * qs );

#ifdef	__cplusplus
}
#endif

#endif	/* QUEUE_STATE_H */
