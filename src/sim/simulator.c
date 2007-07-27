// ==================================================================
// @(#)simulator.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 13/06/2003
// @lastdate 16/04/2007
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

// -----[ simulator_create ]-----------------------------------------
SSimulator * simulator_create(uint8_t tType)
{
  SSimulator * pSimulator= (SSimulator *) MALLOC(sizeof(SSimulator));

  switch (tType) {
  case SCHEDULER_STATIC:
    pSimulator->pScheduler= static_scheduler_create();
    pSimulator->fSchedulerRun= static_scheduler_run;
    pSimulator->fSchedulerPost= static_scheduler_post;
    break;
  default:
    abort();
  }

  pSimulator->dCurrentTime= 0;
  pSimulator->dMaximumTime= 0;
  pSimulator->tType= tType;
  return pSimulator;
}

// -----[ simulator_destroy ]----------------------------------------
void simulator_destroy(SSimulator ** ppSimulator)
{
  SStaticScheduler * pScheduler;

  if (*ppSimulator != NULL) {

    switch((*ppSimulator)->tType) {
    case SCHEDULER_STATIC:
      pScheduler= (SStaticScheduler *) (*ppSimulator)->pScheduler;
      static_scheduler_destroy(&pScheduler);
      break;
    default:
      abort();
    }

    FREE(*ppSimulator);
    *ppSimulator= NULL;
  }
}

// -----[ simulator_run ]--------------------------------------------
/**
 *
 */
int simulator_run(SSimulator * pSimulator)
{
  return pSimulator->fSchedulerRun(pSimulator->pScheduler,
				   pSimulator, -1);
}

// -----[ simulator_step ]-------------------------------------------
/**
 *
 */
int simulator_step(SSimulator * pSimulator, int iNumSteps)
{
  return pSimulator->fSchedulerRun(pSimulator->pScheduler,
				   pSimulator, iNumSteps);
}

// ----- simulator_get_num_events -----------------------------------
/**
 *
 */
uint32_t simulator_get_num_events(SSimulator * pSimulator)
{
  switch (pSimulator->tType) {
  case SCHEDULER_STATIC:
    return static_scheduler_get_num_events(pSimulator->pScheduler);
  default:
    return 0;
  }
}

// ----- simulator_get_event --------------------------------------
void * simulator_get_event(SSimulator * pSimulator, unsigned int uIndex)
{
  switch (pSimulator->tType) {
  case SCHEDULER_STATIC:
    return static_scheduler_get_event(pSimulator->pScheduler, uIndex);
  default:
    return 0;
  }  
}

// ----- simulator_post_event ---------------------------------------
/**
 *
 */
int simulator_post_event(SSimulator * pSimulator,
			 FSimEventCallback fCallback,
			 FSimEventDump fDump,
			 FSimEventDestroy fDestroy,
			 void * pContext,
			 double dSchedulingTime,
			 const uint8_t uDeltaType)
{
  return pSimulator->fSchedulerPost(pSimulator->pScheduler,
				    fCallback, fDump, fDestroy, pContext,
				    dSchedulingTime, uDeltaType);
}

// ----- simulator_get_time -----------------------------------------
/**
 *
 */
double simulator_get_time(SSimulator * pSimulator)
{
  return pSimulator->dCurrentTime;
}

// ----- simulator_set_max_time -------------------------------------
/**
 *
 */
void simulator_set_max_time(SSimulator * pSimulator, double dMaximumTime)
{
  pSimulator->dMaximumTime= dMaximumTime;
}

// ----- simulator_dump_events --------------------------------------
/**
 *
 */
void simulator_dump_events(SLogStream * pStream, SSimulator * pSimulator)
{
  switch (pSimulator->tType) {
  case SCHEDULER_STATIC:
    static_scheduler_dump_events(pStream, pSimulator->pScheduler); break;
  default:
    abort();
  }
}

// ----- simulator_show_infos ---------------------------------------
/**
 *
 */
void simulator_show_infos(SLogStream * pStream, SSimulator * pSimulator)
{
  log_printf(pStream, "scheduler   : ");
  switch (pSimulator->tType) {
  case SCHEDULER_STATIC:
    log_printf(pStream, "static"); break;
  default:
    log_printf(pStream, "unknown");
  }
  log_printf(pStream, "\n");
  log_printf(pStream, "current time: %f\n", pSimulator->dCurrentTime);
  log_printf(pStream, "maximum time: %f\n", pSimulator->dMaximumTime);
}

