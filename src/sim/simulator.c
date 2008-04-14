// ==================================================================
// @(#)simulator.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 13/06/2003
// $Id: simulator.c,v 1.13 2008-04-14 09:12:14 bqu Exp $
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>

#include <net/error.h>
#include <sim/simulator.h>
#include <sim/scheduler.h>
#include <sim/static_scheduler.h>

#define SIMULATOR_TIME_STEP 0.01

sched_type_t SIM_OPTIONS_SCHEDULER= SCHEDULER_STATIC;

// -----[ sim_create ]-----------------------------------------------
simulator_t * sim_create(sched_type_t type)
{
  simulator_t * sim= (simulator_t *) MALLOC(sizeof(simulator_t));

  switch (type) {
  case SCHEDULER_STATIC:
    sim->sched= sched_static_create(sim);
    break;
  default:
    fatal("The requested scheduler type (%d) is not supported", type);
  }
  sim->max_time= 0;
  return sim;
}

// -----[ sim_destroy ]----------------------------------------------
void sim_destroy(simulator_t ** sim_ref)
{
  if (*sim_ref != NULL) {
    (*sim_ref)->sched->ops.destroy(&(*sim_ref)->sched);
    FREE(*sim_ref);
    *sim_ref= NULL;
  }
}

// -----[ sim_clear ]------------------------------------------------
void sim_clear(simulator_t * sim)
{
  sim->sched->ops.clear(sim->sched);
}

// -----[ sim_run ]--------------------------------------------------
/**
 *
 */
int sim_run(simulator_t * sim)
{
  return sim->sched->ops.run(sim->sched, -1);
}

// -----[ sim_step ]-------------------------------------------------
/**
 *
 */
int sim_step(simulator_t * sim, unsigned int num_steps)
{
  return sim->sched->ops.run(sim->sched, num_steps);
}

// -----[ sim_get_num_events ]---------------------------------------
/**
 *
 */
uint32_t sim_get_num_events(simulator_t * sim)
{
  return sim->sched->ops.num_events(sim->sched);
}

// -----[ sim_get_event ]--------------------------------------------
void * sim_get_event(simulator_t * sim, unsigned int index)
{
  return sim->sched->ops.event_at(sim->sched, index);
}

// -----[ sim_post_event ]-------------------------------------------
/**
 *
 */
int sim_post_event(simulator_t * sim, sim_event_ops_t * ops,
		   void * ctx, double time, sim_time_t time_type)
{
  return sim->sched->ops.post(sim->sched, ops, ctx, time, time_type);
}

// -----[ sim_get_time ]---------------------------------------------
/**
 *
 */
double sim_get_time(simulator_t * sim)
{
  return sim->sched->ops.cur_time(sim->sched);
}

// -----[ sim_set_max_time ]-----------------------------------------
/**
 *
 */
void sim_set_max_time(simulator_t * sim, double max_time)
{
  sim->max_time= max_time;
}

// -----[ sim_dump_events ]------------------------------------------
/**
 *
 */
void sim_dump_events(SLogStream * stream, simulator_t * sim)
{
  sim->sched->ops.dump_events(stream, sim->sched);
}

// -----[ sim_show_infos ]-------------------------------------------
/**
 *
 */
void sim_show_infos(SLogStream * stream, simulator_t * sim)
{
  log_printf(stream, "scheduler   : ");
  switch (sim->sched->type) {
  case SCHEDULER_STATIC:
    log_printf(stream, "static"); break;
  default:
    log_printf(stream, "unknown");
  }
  log_printf(stream, "\n");
  log_printf(stream, "current time: %f\n", sim_get_time(sim));
  log_printf(stream, "maximum time: %f\n", sim->max_time);
}

// -----[ sim_set_log_progress ]-------------------------------------
void sim_set_log_progress(simulator_t * sim, const char * file_name)
{
  sim->sched->ops.set_log_process(sim->sched, file_name);
}
