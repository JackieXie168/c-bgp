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

#include <tracer/graph.h>
#include <tracer/state.h>



typedef struct {
  struct graph_t         * graph;
  simulator_t            * simulator;
  int                      set ;
} tracer_t;


#ifdef	__cplusplus
extern "C" {
#endif

// concider the simulator current state as the initial state.
   void tracer_init(tracer_t * self);

   int FOR_TESTING_PURPOSE_tracer_go_one_step(tracer_t * self );

   void tracer_dump(gds_stream_t * stream, tracer_t * self);


   tracer_t * tracer_create( simulator_t * simulator);

   

#ifdef	__cplusplus
}
#endif

#endif	/* TRACER_H */

