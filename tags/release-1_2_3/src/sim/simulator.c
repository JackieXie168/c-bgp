// ==================================================================
// @(#)simulator.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 13/06/2003
// @lastdate 20/04/2006
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>

#include <sim/simulator.h>
#include <sim/scheduler.h>
#include <sim/static_scheduler.h>

#define SIMULATOR_TIME_STEP 0.01

uint8_t SIM_OPTIONS_SCHEDULER= SCHEDULER_STATIC;

static SSimulator * pSimulator= NULL;

// ----- simulator_get_time -----------------------------------------
/**
 *
 */
double simulator_get_time()
{
  return pSimulator->dCurrentTime;
}

// ----- simulator_set_max_time -------------------------------------
/**
 *
 */
void simulator_set_max_time(double dMaximumTime)
{
  pSimulator->dMaximumTime= dMaximumTime;
}

// ----- simulator_create -------------------------------------------
SSimulator * simulator_create(FSimSchedulerRun fSchedulerRun,
			      FSimSchedulerPost fSchedulerPost)
{
  SSimulator * pSimulator= (SSimulator *) MALLOC(sizeof(SSimulator));
  pSimulator->fSchedulerRun= fSchedulerRun;
  pSimulator->fSchedulerPost= fSchedulerPost;
  pSimulator->dCurrentTime= 0;
  pSimulator->dMaximumTime= 0;
  return pSimulator;
}

// ----- simulator_init ---------------------------------------------
void simulator_init()
{
  switch (SIM_OPTIONS_SCHEDULER) {
  case SCHEDULER_STATIC:
    static_scheduler_init();
    if (pSimulator == NULL)
      pSimulator= simulator_create(static_scheduler_run,
				   static_scheduler_post);
    break;
  case SCHEDULER_DYNAMIC:
    break;
  default:
    abort();
  }
}

// ----- simulator_done ---------------------------------------------
/**
 *
 */
void simulator_done()
{
  if (pSimulator != NULL) {
    FREE(pSimulator);
    pSimulator= NULL;
  }
  static_scheduler_done();
  //dynamic_scheduler_done();
}

// ----- simulator_clear --------------------------------------------
/**
 *
 */
void simulator_clear()
{
  simulator_done();
  simulator_init();
}

// ----- simulator_run ----------------------------------------------
/**
 *
 */
int simulator_run()
{
  return pSimulator->fSchedulerRun(pSimulator, -1);
}

// ----- simulator_step ---------------------------------------------
/**
 *
 */
int simulator_step(int iNumSteps)
{
  return pSimulator->fSchedulerRun(pSimulator, iNumSteps);
}

// ----- simulator_get_num_events -----------------------------------
/**
 *
 */
uint32_t simulator_get_num_events()
{
  switch (SIM_OPTIONS_SCHEDULER) {
  case SCHEDULER_STATIC:
    return static_scheduler_get_num_events();
  default:
    return 0;
  }
}

// ----- simulator_post_event ---------------------------------------
int simulator_post_event(FSimEventCallback fCallback,
			 FSimEventDump fDump,
			 FSimEventDestroy fDestroy,
			 void * pContext,
			 double dSchedulingTime,
			 const uint8_t uDeltaType)
{
  return pSimulator->fSchedulerPost(fCallback, fDump, fDestroy, pContext,
				    dSchedulingTime, uDeltaType);
  /*
  LOG_DEBUG("simulator_post_event\n");

  switch (uDeltaType) {
  case ABSOLUTE_TIME:
    if (uSchedulingTime < uSimulatorTime) {
      LOG_FATAL("Time event goes back");
      exit(EXIT_FAILURE);
    }
    break;
  case RELATIVE_TIME:
    if (uSchedulingTime < uStepSimulatorTime && uSchedulingTime != 0)
      LOG_SEVERE("simulator> Time granularity too small\n");
			
    uSchedulingTime += uSimulatorTime;
    if (uSchedulingTime >= MAX_UINT32_T) {
      LOG_FATAL("Scheduling time overflow");
      exit(EXIT_FAILURE);
    }
    break;
  default:
    abort();
  }
	
  return scheduler_post_event((FSchedEventCallback) fCallback, pContext,
  uSchedulingTime);*/
}

// ----- simulator_dump_events --------------------------------------
/**
 *
 */
void simulator_dump_events(SLogStream * pStream)
{
  if (SIM_OPTIONS_SCHEDULER == SCHEDULER_STATIC) {
    static_scheduler_dump_events(pStream);
  }
}

// ----- simulator_show_infos ---------------------------------------
/**
 *
 */
void simulator_show_infos()
{
  log_printf(pLogOut, "current time: %f\n", pSimulator->dCurrentTime);
  log_printf(pLogOut, "maximum time: %f\n", pSimulator->dMaximumTime);
}

