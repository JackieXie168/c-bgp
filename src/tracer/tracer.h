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
#include <net/network.h>

struct tracer_t;

#include <tracer/graph.h>
#include <tracer/state.h>




typedef struct tracer_t {
  struct graph_t         * graph;
  network_t              * network;
  net_node_t             ** nodes;
  unsigned int              nb_nodes;
  int                      started ;
  char                  base_output_file_name[256];
  char                  base_output_directory[256];
  char                  IMAGE_FORMAT[10];
} tracer_t;


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


   int tracer_trace_whole_graph_v2(tracer_t * self);
   int tracer_trace_whole_graph_v1bis(tracer_t * self);

   void tracer_graph_detect_every_cycle(gds_stream_t * stream,tracer_t * tracer);

#ifdef	__cplusplus
}
#endif

#endif	/* __TRACER_H__ */

