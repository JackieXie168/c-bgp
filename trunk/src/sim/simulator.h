// ==================================================================
// @(#)simulator.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @author Sebastien Tandel
//
// @date 28/11/2002
// $Id: simulator.h,v 1.13 2008-04-14 09:12:14 bqu Exp $
// ==================================================================

#ifndef __SIMULATOR_H__
#define __SIMULATOR_H__

#include <libgds/types.h>
#include <libgds/log.h>
#include <net/error.h>
#include <sim/scheduler.h>

struct sched_t;
struct simulator_t;

typedef enum {
  SIM_TIME_ABS,
  SIM_TIME_REL,
  SIM_TIME_MAX
} sim_time_t;

typedef enum {
  SCHEDULER_STATIC,
  SCHEDULER_DYNAMIC,
  SCHEDULER_MAX
} sched_type_t;

typedef struct {
  int  (*callback) (struct simulator_t * sim, void * ctx);
  void (*destroy) (void * ctx);
  void (*dump)(SLogStream * stream, void * ctx);
} sim_event_ops_t;

typedef struct sched_ops_t {
  void         (*destroy) (struct sched_t ** self_ref);
  void         (*clear) (struct sched_t * self);
  net_error_t  (*run) (struct sched_t * self, unsigned int num_steps);
  int          (*post) (struct sched_t * self, sim_event_ops_t * ops,
			void * ctx, double time, sim_time_t time_type);
  unsigned int (*num_events) (struct sched_t * self);
  void *       (*event_at) (struct sched_t * self, unsigned int index);
  void         (*dump_events) (SLogStream * stream, struct sched_t * self);
  void         (*set_log_process) (struct sched_t * self,
				   const char * file_name);
  double       (*cur_time) (struct sched_t * self);
} sched_ops_t;

typedef struct sched_t {
  sched_type_t         type;
  sched_ops_t          ops;
  struct simulator_t * sim;
} sched_t;

typedef struct simulator_t {
  sched_t * sched;
  double    max_time;
} simulator_t;

extern sched_type_t SIM_OPTIONS_SCHEDULER;

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ sim_create ]---------------------------------------------
  simulator_t * sim_create(sched_type_t type);
  // -----[ sim_destroy ]--------------------------------------------
  void sim_destroy(simulator_t ** sim_ref);
  // -----[ sim_clear ]----------------------------------------------
  void sim_clear(simulator_t * sim);
  // -----[ sim_run ]------------------------------------------------
  int sim_run(simulator_t * sim);
  // -----[ sim_step ]-----------------------------------------------
  int sim_step(simulator_t * sim, unsigned int num_steps);
  // -----[ sim_post_event ]-----------------------------------------
  int sim_post_event(simulator_t * sim, sim_event_ops_t * ops,
		     void * ctx, double time, sim_time_t time_type);
  // -----[ sim_get_num_events ]-------------------------------------
  uint32_t sim_get_num_events(simulator_t * sim);
  // -----[ sim_get_event ]------------------------------------------
  void * sim_get_event(simulator_t * sim, unsigned int index);
  // -----[ sim_get_time ]-------------------------------------------
  double sim_get_time(simulator_t * sim);
  // -----[ sim_set_max_time ]---------------------------------------
  void sim_set_max_time(simulator_t * sim, double max_time);
  // -----[ sim_dump_events ]----------------------------------------
  void sim_dump_events(SLogStream * stream, simulator_t * sim);
  // -----[ sim_show_infos ]-----------------------------------------
  void sim_show_infos(SLogStream * stream, simulator_t * sim);
  // -----[ sim_set_log_progress ]-----------------------------------
  void sim_set_log_progress(simulator_t * sim, const char * file_name);

#ifdef __cplusplus
}
#endif

#endif /* __SIMULATOR_H__ */

