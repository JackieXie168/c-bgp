// ==================================================================
// @(#)static_scheduler.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 30/07/2003
// @lastdate 16/04/2007
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <libgds/memory.h>
#include <sim/static_scheduler.h>
#include <net/network.h>

#define EVENT_QUEUE_DEPTH 256

typedef struct {
  FSimEventCallback fCallback;
  FSimEventDump fDump;
  FSimEventDestroy fDestroy;
  void * pContext;
} SStaticEvent;

// -----[ _static_sched_event_create ]-------------------------------
static SStaticEvent * _static_sched_event_create(FSimEventCallback fCallback,
						 FSimEventDump fDump,
						 FSimEventDestroy fDestroy,
						 void * pContext)
{
  SStaticEvent * pEvent= (SStaticEvent *) MALLOC(sizeof(SStaticEvent));
  pEvent->fCallback= fCallback;
  pEvent->fDump= fDump;
  pEvent->fDestroy= fDestroy;
  pEvent->pContext= pContext;
  return pEvent;
}

// -----[ _static_sched_event_destroy ]------------------------------
/**
 *
 */
static void _static_sched_event_destroy(SStaticEvent ** ppEvent)
{
  if (*ppEvent != NULL) {
    FREE(*ppEvent);
    *ppEvent= NULL;
  }
}

// -----[ _static_sched_event_fifo_destroy ]-------------------------
/**
 *
 */
static void _static_sched_event_fifo_destroy(void ** ppItem)
{
  SStaticEvent ** ppEvent= (SStaticEvent **) ppItem;

  if (((*ppEvent) != NULL) && ((*ppEvent)->fDestroy != NULL))
    (*ppEvent)->fDestroy((*ppEvent)->pContext);
  _static_sched_event_destroy(ppEvent);
}

// -----[ static_scheduler_create ]---------------------------------
/**
 *
 */
SStaticScheduler * static_scheduler_create()
{
  SStaticScheduler * pScheduler=
    (SStaticScheduler *) MALLOC(sizeof(SStaticScheduler));
  pScheduler->pEvents=
    fifo_create(EVENT_QUEUE_DEPTH,
		_static_sched_event_fifo_destroy);
  fifo_set_option(pScheduler->pEvents,
		  FIFO_OPTION_GROW_EXPONENTIAL, 1);
  return pScheduler;
}

// -----[ static_scheduler_destroy ]--------------------------------
/**
 *
 */
void static_scheduler_destroy(SStaticScheduler ** ppScheduler)
{
  if (*ppScheduler != NULL) {
    if ((*ppScheduler)->pEvents->uCurrentDepth > 0)
      LOG_ERR(LOG_LEVEL_WARNING, "Warning: %d events still in queue.\n",
	      (*ppScheduler)->pEvents->uCurrentDepth);

    fifo_destroy(&(*ppScheduler)->pEvents);

    FREE(*ppScheduler);
    *ppScheduler= NULL;
  }
}

// ----- static_scheduler_run ---------------------------------------
/**
 * Process the events in the global linear queue.
 *
 * Arguments:
 * - iNumSteps: number of events to process during this run (if -1,
 *   process events until queue is empty).
 */
int static_scheduler_run(void * pSchedCtx, void * pContext,
			 int iNumSteps)
{
  SStaticScheduler * pScheduler= (SStaticScheduler *) pSchedCtx;
  SStaticEvent * pEvent;
  SSimulator * pSimulator= (SSimulator *) pContext;

  while ((pEvent= (SStaticEvent *) fifo_pop(pScheduler->pEvents))
	  != NULL) {

    LOG_DEBUG(LOG_LEVEL_DEBUG, "=====<<< EVENT %2.2f >>>=====\n",
	      pSimulator->dCurrentTime);

    //if (pEvent->fDump != NULL) {
    //  pEvent->fDump(log_get_stream(pMainLog), pEvent->pContext);
    //} else {
    //  fprintf(log_get_stream(pMainLog), "unknown");
    //}
    //fprintf(log_get_stream(pMainLog), "\n");

    pEvent->fCallback(pSimulator, pEvent->pContext);
    _static_sched_event_destroy(&pEvent);

    LOG_DEBUG(LOG_LEVEL_DEBUG, "\n");

    // Update simulation time
    (pSimulator->dCurrentTime)+= 1;

    // Limit on simulation time ??
    if ((pSimulator->dMaximumTime > 0) &&
	(pSimulator->dCurrentTime >= pSimulator->dMaximumTime)) {
      LOG_ERR(LOG_LEVEL_WARNING, "WARNING: Simulation stopped @ %2.2f.\n",
	      pSimulator->dCurrentTime);
      break;
    }

    // Limit on number of steps
    if (iNumSteps > 0) {
      iNumSteps--;
      if (iNumSteps == 0)
	break;
    }

  }

  return 0;
}

// ----- static_scheduler_post --------------------------------------
int static_scheduler_post(void * pSchedCtx,
			  FSimEventCallback fCallback,
			  FSimEventDump fDump,
			  FSimEventDestroy fDestroy,
			  void * pContext,
			  double uSchedulingTime,
			  uint8_t uDeltaType)
{
  SStaticScheduler * pScheduler= (SStaticScheduler *) pSchedCtx;
  SStaticEvent * pEvent;

  assert((uSchedulingTime == 0) && (uDeltaType == RELATIVE_TIME));
  pEvent= _static_sched_event_create(fCallback, fDump, fDestroy, pContext);

  return fifo_push(pScheduler->pEvents, pEvent);
}

// ----- static_scheduler_get_num_events ----------------------------
/**
 * Return the number of queued events.
 */
uint32_t static_scheduler_get_num_events(void * pSchedCtx)
{
  SStaticScheduler * pScheduler= (SStaticScheduler *) pSchedCtx;
  return pScheduler->pEvents->uCurrentDepth;
}

// ----- static_scheduler_dump_events -------------------------------
/**
 * Return information 
 */
void static_scheduler_dump_events(SLogStream * pStream, void * pSchedCtx)
{
  SStaticScheduler * pScheduler= (SStaticScheduler *) pSchedCtx;
  SStaticEvent * pEvent;
  uint32_t uDepth;
  uint32_t uMaxDepth;
  uint32_t uStart;
  int iIndex;

  uDepth= pScheduler->pEvents->uCurrentDepth;
  uMaxDepth= pScheduler->pEvents->uMaxDepth;
  uStart= pScheduler->pEvents->uStartIndex;
  log_printf(pStream, "Number of events queued: %u (%u)\n",
	     uDepth, uMaxDepth);
  for (iIndex= 0; iIndex < uDepth; iIndex++) {
    pEvent= (SStaticEvent *) pScheduler->pEvents->ppItems[(uStart+iIndex) % uMaxDepth];
    log_printf(pStream, "(%d) ", (uStart+iIndex) % uMaxDepth);
    log_flush(pStream);
    if (pEvent->fDump != NULL) {
      pEvent->fDump(pStream, pEvent->pContext);
    } else {
      log_printf(pStream, "unknown");
    }
    log_printf(pStream, "\n");
  }
}
