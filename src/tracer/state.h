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

#include <tracer/transition.h>
#include <tracer/graph.h>
#include <tracer/tracer.h>


typedef struct bgp_session_info_t{
   net_addr_t            neighbor_addr;
   unsigned int          send_seq_num;
   unsigned int          recv_seq_num;
   bgp_route_t **        adj_rib_IN_routes;
   bgp_route_t **        adj_rib_OUT_routes;
   unsigned int          nb_adj_rib_in_routes;
   unsigned int          nb_adj_rib_out_routes;

   //bgp_rib_t           * adj_rib[RIB_MAX];
   
}bgp_session_info_t;

typedef struct bgp_sessions_info_t{
   unsigned int          nb_bgp_session_info_;
   bgp_session_info_t ** bgp_session_info;
}bgp_sessions_info_t;

typedef struct local_rib_info_t{
   unsigned int          nb_local_rib_elem;
   bgp_route_t ** bgp_route_;
}local_rib_info_t;

typedef struct routing_info_t{
   //net_rt_t        * node_rt_t;
   local_rib_info_t    *  bgp_router_loc_rib_t;
   //bgp_peers_t         * bgp_router_peers;
   bgp_sessions_info_t * bgp_sessions_info;
}routing_info_t;

typedef struct couple_node_routinginfo_t{
    net_node_t * node;
    routing_info_t * routing_info;
} couple_node_routinginfo_t;



typedef struct routing_state_t{
  struct state_t              * state;
  couple_node_routinginfo_t **    couples_node_routing_info;

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
    unsigned int                max_nb_of_msg_in_one_oriented_session;
}  queue_state_t;




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
  unsigned int          blocked;
} state_t;

// ----- Global BGP options --------
#define STATE_ROOT                      0x01
#define STATE_FINAL                     0x02
#define STATE_MEMBER_OF_A_CYCLE         0x04
#define STATE_CAN_LEAD_TO_A_CYCLE       0x08
#define STATE_CAN_LEAD_TO_A_FINAL_STATE 0x10

#ifdef	__cplusplus
extern "C" {
#endif

    


    state_t * state_create(struct tracer_t * tracer, struct transition_t * the_input_transition);

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


#ifdef	__cplusplus
}
#endif

#endif	/* __STATE_H__ */

