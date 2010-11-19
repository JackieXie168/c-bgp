// ==================================================================
// @(#)static_scheduler.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 30/07/2003
// $Id: static_scheduler.h,v 1.12 2009-03-10 13:13:25 bqu Exp $
// ==================================================================

/**
 * \file
 * Provide data structures and functions to handle a static
 * scheduler. The static scheduler is basically a single
 * linear queue. New events are inserted at the end of the queue
 * while events to be proccessed are found at the beginning of the
 * queue (FIFO discipline).
 *
 * Insertion and extraction times are obviously O(1) in theory.
 * In practice, memory management activities can make the insertion
 * time longer, especially when the scheduler's queue must be
 * expanded.
 *
 * In addition, there is no real notion of time in a static
 * scheduler. In this implementation, the simulation time corresponds
 * to the index of the last processed event.
 */

#ifndef __TUNABLE_SCHEDULER_H__
#define __TUNABLE_SCHEDULER_H__

#include <sim/simulator.h>

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ sched_static_create ]------------------------------------
  /**
   * Create a tunable scheduler instance.
   *
   * \param sim is the parent simulator.
   * \retval a static scheduler.
   */
  sched_t * sched_tunable_create(simulator_t * sim);

#ifdef __cplusplus
}
#endif

#endif /* __TUNABLE_SCHEDULER_H__ */
