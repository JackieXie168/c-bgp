// ==================================================================
// @(#)scheduler.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @author Sebastien Tandel (sta@infonet.fundp.ac.be)
// @date 12/06/2003
// $Id: scheduler.h,v 1.3 2009-03-10 13:13:25 bqu Exp $
// ==================================================================

/**
 * \file
 * Provide data structures and functions to handle a dynamic
 * scheduler. This is a highly experimental and inefficient
 * implementation.
 */

#ifndef __SCHEDULER_H__
#define __SCHEDULER_H__

#include <sim/simulator.h>

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ sched_dynamic_create ]-----------------------------------
  /**
   * Create a dynamic scheduler instance.
   *
   * \param sim is the parent simulator.
   * \retval a dynamic scheduler.
   */
  sched_t * sched_dynamic_create(simulator_t * sim);

#ifdef __cplusplus
}
#endif

#endif /* __SCHEDULER_H__ */
