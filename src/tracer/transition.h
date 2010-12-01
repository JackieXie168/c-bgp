/* 
 * File:   transition.h
 * Author: yo
 *
 * Created on 26 novembre 2010, 16:03
 */

#ifndef __TRANSITION_H__
#define	__TRANSITION_H__


#include <libgds/memory.h>
#include <libgds/stack.h>
#include <libgds/types.h>
#include <libgds/stream.h>
#include <net/error.h>
#include <libgds/fifo_tunable.h>

#include <sim/tunable_scheduler.h>
#include <sim/simulator.h>


struct transition_t;

#include <tracer/state.h>


// -----[ sched_t ]--------------------------------------------------
/** Definition of a state. */
typedef struct transition_t{
  struct _event_t               * event;
  struct state_t         * from;
  struct state_t         * to;
} transition_t;

#ifdef	__cplusplus
extern "C" {
#endif

transition_t * transition_create(  struct _event_t * event);


#ifdef	__cplusplus
}
#endif

#endif	/* __TRANSITION_H__ */

