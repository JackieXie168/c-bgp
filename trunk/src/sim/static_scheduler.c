// ==================================================================
// @(#)static_scheduler.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 30/07/2003
// @lastdate 03/04/2008
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <stdio.h>
#include <sys/time.h>

#include <libgds/fifo.h>
#include <libgds/log.h>
#include <libgds/memory.h>
#include <sim/static_scheduler.h>
#include <net/network.h>

#define EVENT_QUEUE_DEPTH 256

typedef struct {
  sim_event_ops_t * ops;
  void            * ctx;
} _event_t;

typedef struct {
  sched_type_t   type;
  sched_ops_t    ops;
  simulator_t  * sim;
  SFIFO        * events;
  unsigned int   cur_time;
  SLogStream   * pProgressLogStream;
} sched_static_t;
typedef sched_static_t SStaticScheduler;

// -----[ _event_create ]--------------------------------------------
static inline _event_t * _event_create(sim_event_ops_t * ops,
				       void * ctx)
{
  _event_t * event= (_event_t *) MALLOC(sizeof(_event_t));
  event->ops= ops;
  event->ctx= ctx;
  return event;
}

// -----[ _event_destroy ]-------------------------------------------
static void _event_destroy(_event_t ** event_ref)
{
  if (*event_ref != NULL) {
    FREE(*event_ref);
    *event_ref= NULL;
  }
}

// -----[ _fifo_event_destroy ]--------------------------------------
static void _fifo_event_destroy(void ** item_ref)
{
  _event_t ** event_ref= (_event_t **) item_ref;

  if (*event_ref != NULL) {
    if ((*event_ref)->ops->destroy != NULL)
      (*event_ref)->ops->destroy((*event_ref)->ctx);
    _event_destroy(event_ref);
  }
}

// -----[ _log_progress ]--------------------------------------------
static inline void _log_progress(sched_static_t * sched)
{
  struct timeval tv;

  if (sched->pProgressLogStream == NULL)
    return;

  assert(gettimeofday(&tv, NULL) >= 0);
  log_printf(sched->pProgressLogStream, "%d\t%.0f\t%d\n",
	     sched->cur_time,
	     ((double) tv.tv_sec)*1000000 + (double) tv.tv_usec,
	     fifo_depth(sched->events));
}

// -----[ _destroy ]-------------------------------------------------
static void _destroy(sched_t ** sched_ref)
{
  sched_static_t * sched;
  if (*sched_ref != NULL) {

    sched= (sched_static_t *) *sched_ref;

    /* Free private part */
    if (sched->events->uCurrentDepth > 0)
      LOG_ERR(LOG_LEVEL_WARNING, "Warning: %d events still in queue.\n",
	      sched->events->uCurrentDepth);
    fifo_destroy(&sched->events);
    if (sched->pProgressLogStream != NULL)
      log_destroy(&sched->pProgressLogStream);

    FREE(*sched_ref);
    *sched_ref= NULL;
  }
}

// -----[ _clear ]---------------------------------------------------
static void _clear(sched_t * self)
{
  sched_static_t * sched= (sched_static_t *) self;
  fifo_destroy(&sched->events);
  sched->events= fifo_create(EVENT_QUEUE_DEPTH, _fifo_event_destroy);
  fifo_set_option(sched->events, FIFO_OPTION_GROW_EXPONENTIAL, 1);
}

// -----[ _run ]-----------------------------------------------------
/**
 * Process the events in the global linear queue.
 *
 * Arguments:
 * - iNumSteps: number of events to process during this run (if -1,
 *   process events until queue is empty).
 */
static net_error_t _run(sched_t * self, unsigned int num_steps)
{
  sched_static_t * sched= (sched_static_t *) self;
  _event_t * event;

  while ((event= (_event_t *) fifo_pop(sched->events)) != NULL) {

    LOG_DEBUG(LOG_LEVEL_DEBUG, "=====<<< EVENT %2.2f >>>=====\n",
	      (double) sched->cur_time);
    event->ops->callback(sched->sim, event->ctx);
    _event_destroy(&event);
    LOG_DEBUG(LOG_LEVEL_DEBUG, "\n");

    // Update simulation time
    sched->cur_time++;

    // Limit on simulation time ??
    if ((sched->sim->max_time > 0) &&
	(sched->cur_time >= sched->sim->max_time)) {
      LOG_ERR(LOG_LEVEL_WARNING, "WARNING: Simulation stopped @ %2.2f.\n",
	      (double) sched->cur_time);
      return ESIM_TIME_LIMIT;
    }

    // Limit on number of steps
    if (num_steps > 0) {
      num_steps--;
      if (num_steps == 0)
	break;
    }

    _log_progress(sched);
  }
  return ESUCCESS;
}

// -----[ _num_events ]----------------------------------------------
/**
 * Return the number of queued events.
 */
static unsigned int _num_events(sched_t * self)
{
  sched_static_t * sched= (sched_static_t *) self;
  return sched->events->uCurrentDepth;
}

// -----[ _event_at ]------------------------------------------------
void * _event_at(sched_t * self, unsigned int index)
{
  sched_static_t * sched= (sched_static_t *) self;
  uint32_t depth;
  uint32_t max_depth;
  uint32_t start;

  depth= sched->events->uCurrentDepth;
  if (index >= depth)
    return NULL;

  max_depth= sched->events->uMaxDepth;
  start= sched->events->uStartIndex;
  return ((_event_t *) sched->events->ppItems[(start+index) % max_depth])->ctx;

}

// -----[ _post ]----------------------------------------------------
static int _post(sched_t * self, sim_event_ops_t * ops,
		 void * ctx, double time, sim_time_t time_type)
{
  sched_static_t * sched= (sched_static_t *) self;
  _event_t * event;

  assert((time == 0) && (time_type == SIM_TIME_REL));
  event= _event_create(ops, ctx);
  return fifo_push(sched->events, event);
}

// -----[ _dump_events ]---------------------------------------------
/**
 * Return information 
 */
static void _dump_events(SLogStream * stream, sched_t * self)
{
  sched_static_t * sched= (sched_static_t *) self;
  _event_t * event;
  uint32_t depth;
  uint32_t max_depth;
  uint32_t start;
  unsigned int index;

  depth= sched->events->uCurrentDepth;
  max_depth= sched->events->uMaxDepth;
  start= sched->events->uStartIndex;
  log_printf(stream, "Number of events queued: %u (%u)\n",
	     depth, max_depth);
  for (index= 0; index < depth; index++) {
    event= (_event_t *) sched->events->ppItems[(start+index) % max_depth];
    log_printf(stream, "(%d) ", (start+index) % max_depth);
    log_flush(stream);
    if (event->ops->dump != NULL) {
      event->ops->dump(stream, event->ctx);
    } else {
      log_printf(stream, "unknown");
    }
    log_printf(stream, "\n");
  }
}

// -----[ _set_log_progress ]----------------------------------------
/**
 *
 */
static void _set_log_progress(sched_t * self, const char * file_name)
{
  sched_static_t * sched= (sched_static_t *) self;

  if (sched->pProgressLogStream != NULL)
    log_destroy(&sched->pProgressLogStream);

  if (file_name != NULL) {
    sched->pProgressLogStream= log_create_file(file_name);
    log_printf(sched->pProgressLogStream, "# C-BGP Queue Progress\n");
    log_printf(sched->pProgressLogStream, "# <step> <time (us)> <depth>\n");
  }
}

// -----[ _cur_time ]------------------------------------------------
static double _cur_time(sched_t * self)
{
  sched_static_t * sched= (sched_static_t *) self;
  return (double) sched->cur_time;
}

// -----[ static_scheduler_create ]---------------------------------
/**
 *
 */
sched_t * sched_static_create(simulator_t * sim)
{
  sched_static_t * sched=
    (sched_static_t *) MALLOC(sizeof(sched_static_t));

  /* Initialize public part (type + ops) */
  sched->type= SCHEDULER_STATIC;
  sched->ops.destroy        = _destroy;
  sched->ops.clear          = _clear;
  sched->ops.run            = _run;
  sched->ops.post           = _post;
  sched->ops.num_events     = _num_events;
  sched->ops.event_at       = _event_at;
  sched->ops.dump_events    = _dump_events;
  sched->ops.set_log_process= _set_log_progress;
  sched->ops.cur_time       = _cur_time;
  sched->sim= sim;

  /* Initialize private part */
  sched->events= fifo_create(EVENT_QUEUE_DEPTH, _fifo_event_destroy);
  fifo_set_option(sched->events, FIFO_OPTION_GROW_EXPONENTIAL, 1);
  sched->cur_time= 0;
  sched->pProgressLogStream= NULL;

  return (sched_t *) sched;
}
