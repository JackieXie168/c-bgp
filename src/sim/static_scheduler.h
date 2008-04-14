// ==================================================================
// @(#)static_scheduler.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 30/07/2003
// $Id: static_scheduler.h,v 1.11 2008-04-14 09:12:14 bqu Exp $
// ==================================================================

#ifndef __STATIC_SCHEDULER_H__
#define __STATIC_SCHEDULER_H__

#include <sim/simulator.h>

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ sched_static_create ]------------------------------------
  sched_t * sched_static_create(simulator_t * sim);

#ifdef __cplusplus
}
#endif

#endif /* __STATIC_SCHEDULER_H__ */
