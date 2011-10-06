/* 
 * File:   tracer.h
 * Author: yo
 *
 * Created on 26 novembre 2010, 12:16
 */

#ifndef __TRACER_H__
#define	__TRACER_H__

#include <libgds/memory.h>
#include <libgds/stack.h>
#include <libgds/types.h>
#include <libgds/stream.h>
#include <net/error.h>
#include <libgds/fifo_tunable.h>
#include <libgds/lifo.h>
#include <net/network.h>

struct tracer_t;

#include <tracer/graph.h>
#include <tracer/state.h>

// -----[ tracing scheduler type ]-------------------------------------------
/** BGP message types. */
typedef enum {
  /** Update message. */
  TRACING_SCHEDULER_1_FIFO,
  /** Withdraw message. */
  TRACING_SCHEDULER_2_LIFO,
  /** Close message. */
  TRACING_SCHEDULER_3_MIN_MAX_NBMSGBGPSESSION,
  /** Open message. */
  TRACING_SCHEDULER_4,
  TRACING_SCHEDULER_MAX,
} tracing_scheduler_type_t;


// -----[ bgp_msg_t ]------------------------------------------------
/** Definition of a BGP message (abstract). */
typedef struct {
  /** Message type. */
  tracing_scheduler_type_t type;
  
  /** ASN of the source. */
  //uint16_t       peer_asn;
  /** "TCP" sequence number (used for checking ordering). */
  //unsigned int   seq_num;
} tracing_scheduler_t;



typedef struct {  
  tracing_scheduler_t     header;  
  gds_fifo_t *           list_of_state_trans;
  
} tracing_scheduler_1_fifo_t;


typedef struct {
  tracing_scheduler_t       header;  
  gds_lifo_t * list_of_state_trans;  
} tracing_scheduler_2_lifo_t;

typedef struct {
  tracing_scheduler_t header;
  struct state_t *    current_working_state;
  unsigned int        next_trans ;
  struct tracer_t *    tracer;
  
  struct state_t **   list_of_states;
  int          nb_states;
  int          max_states;
  int           started;
  int           change_state;
  
} tracing_scheduler_3_MIN_MAX_NBMSGBGPSESSION_t;


// -----[ bgp_msg_open_t ]-------------------------------------------
/** Definition of a BGP open message. */
typedef struct {
  /** Common BGP message header. */
  tracing_scheduler_t  header;
  /** Router-ID of the source. */
  //net_addr_t router_id;
} tracing_scheduler_4_t;


typedef struct filter_t {    
   struct tracer_t            * tracer;
    
  unsigned int       depth;
  unsigned int       diff_depth;
  unsigned int       nb_max_msg;
  unsigned int       maxNbOfTreatedTransitions;
  unsigned int       rejectWithdrawMsg;
  unsigned int       rejectmultipleMsgInSameSession;
}filter_t ;



typedef struct tracer_t {
  struct graph_t         * graph;
  network_t              * network;
  net_node_t             ** nodes;
  unsigned int           nb_nodes;
  int                    started ;
  char                   base_output_file_name[256];
  char                   base_output_directory[256];
  char                   IMAGE_FORMAT[10];

  
  unsigned int          initial_state_for_tracing;
  tracing_scheduler_t *   tracing_scheduler;
  tracing_scheduler_type_t tracing_scheduler_type;
  filter_t              * filter;
  
} tracer_t;


#define FILTER_OK 1
#define FILTER_NOK 0
#define TRUE 1
#define FALSE 0


#ifdef	__cplusplus
extern "C" {
#endif


   void _tracer_init();

// concider the simulator current state as the initial state.
   int _tracer_start(tracer_t * self);


   int FOR_TESTING_PURPOSE_tracer_go_one_step(tracer_t * self );

   int FOR_TESTING_PURPOSE_tracer_graph_state_dump(gds_stream_t * stream, tracer_t * self , unsigned int num_state);

   int tracer_trace_from_state_using_transition(tracer_t * self, unsigned int state_id, unsigned int transition_id );

   int FOR_TESTING_PURPOSE_tracer_graph_current_state_dump(gds_stream_t * stream, tracer_t * self);

   int tracer_graph_allstates_dump(gds_stream_t * stream,tracer_t *  tracer);

   int _tracer_dump(gds_stream_t * stream, tracer_t * self);

   int tracer_inject_state(tracer_t * tracer , unsigned int num_state);

   int tracer_state_dump(gds_stream_t * stream, tracer_t * tracer , unsigned int num_state);

    sched_tunable_t * tracer_get_tunable_scheduler(tracer_t * tracer);

    simulator_t * tracer_get_simulator(tracer_t * tracer);

    tracer_t * tracer_get_default();



    void tracer_graph_export_dot(gds_stream_t * stream,tracer_t * tracer);

    void tracer_graph_export_dot_allStates_to_file(tracer_t * tracer);

    void tracer_graph_export_allStates_to_file(tracer_t * tracer);

	void tracer_graph_export_dot_to_file(tracer_t * tracer);

    int tracer_trace_whole_graph(tracer_t * self);


   int tracer_trace_whole_graph_v2_TYPE3_MINBGPSESSION(tracer_t * self);
   int tracer_trace_whole_graph_v1bis_LIFO_AND_MAX_GRAPH_DEPTH(tracer_t * self);
   int tracer_trace_whole_graph___GENERIC(tracer_t * self);

   void tracer_graph_detect_every_cycle(gds_stream_t * stream,tracer_t * tracer);
   void tracer_graph_cycle_dump(gds_stream_t * stream,tracer_t * tracer);
   void tracer_graph_final_state_dump(gds_stream_t * stream,tracer_t * tracer);

   void filter_set_limit_on_depth(tracer_t * self, unsigned int depth);

   void filter_unset_limit_on_depth(tracer_t * self);
   
   void filter_set_limit_on_diff_depth(tracer_t * self, unsigned int depth);

   void filter_unset_limit_on_diff_depth(tracer_t * self);

   void filter_set_limit_on_max_nb_in_oriented_session(tracer_t * self, unsigned int nb_msg);

   void filter_unset_limit_on_max_nb_in_oriented_session(tracer_t * self);

   void filter_set_maxNbOfTreatedTransitions(tracer_t * self, unsigned int max);
   void filter_unset_maxNbOfTreatedTransitions(tracer_t * self);
   
        void filter_withdrawMsg_accept(tracer_t * tracer);
        void filter_withdrawMsg_reject(tracer_t * tracer);
   
   void tracer_scheduler_set(tracer_t * self, tracing_scheduler_type_t type);

   void tracer_set_initial_state_for_tracing(tracer_t * self, unsigned int numstate);
   unsigned int tracer_howManyStatesCurrently(tracer_t * self);
   int tracer_trace_auto(tracer_t * self);

#ifdef	__cplusplus
}
#endif

#endif	/* __TRACER_H__ */

