// ==================================================================
// @(#)static_scheduler.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 30/07/2003
// @lastdate 04/01/2005
// ==================================================================

#include <assert.h>
#include <libgds/memory.h>
#include <sim/static_scheduler.h>
#include <net/network.h>

#define EVENT_QUEUE_DEPTH 256

static SStaticScheduler * pStaticScheduler= NULL;

typedef struct {
  FSimEventCallback fCallback;
  FSimEventDump fDump;
  FSimEventDestroy fDestroy;
  void * pContext;
} SStaticEvent;

// ----- static_scheduler_event_create ------------------------------
SStaticEvent * static_scheduler_event_create(FSimEventCallback fCallback,
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

// ----- static_scheduler_event_destroy -----------------------------
/**
 *
 */
void static_scheduler_event_destroy(SStaticEvent ** ppEvent)
{
  if (*ppEvent != NULL) {
    FREE(*ppEvent);
    *ppEvent= NULL;
  }
}

// ----- static_scheduler_event_fifo_destroy ------------------------
/**
 *
 */
void static_scheduler_event_fifo_destroy(void ** ppItem)
{
  SStaticEvent ** ppEvent= (SStaticEvent **) ppItem;

  if (((*ppEvent) != NULL) && ((*ppEvent)->fDestroy != NULL))
    (*ppEvent)->fDestroy((*ppEvent)->pContext);
  static_scheduler_event_destroy(ppEvent);
}

// ----- static_scheduler_init -------------------------------------
/**
 *
 */
SStaticScheduler * static_scheduler_init()
{
  assert(pStaticScheduler == NULL);
  pStaticScheduler=
    (SStaticScheduler *) MALLOC(sizeof(SStaticScheduler));
  pStaticScheduler->pEvents=
    fifo_create(EVENT_QUEUE_DEPTH, static_scheduler_event_fifo_destroy);
  fifo_set_option(pStaticScheduler->pEvents, FIFO_OPTION_GROW_EXPONENTIAL, 1);
  return pStaticScheduler;
}

// ----- static_scheduler_done --------------------------------------
/**
 *
 */
void static_scheduler_done()
{
  if (pStaticScheduler != NULL) {
    if (pStaticScheduler->pEvents->uCurrentDepth > 0)
      LOG_WARNING("Warning: %d events still in queue.\n",
		  pStaticScheduler->pEvents->uCurrentDepth);
    fifo_destroy(&pStaticScheduler->pEvents);
    FREE(pStaticScheduler);
    pStaticScheduler= NULL;
  }
}

// ----- static_scheduler_run ---------------------------------------
/**
 *
 */
int static_scheduler_run(void * pContext)
{
  SStaticEvent * pEvent;
  SSimulator * pSimulator= (SSimulator *) pContext;

  while ((pEvent= (SStaticEvent *) fifo_pop(pStaticScheduler->pEvents))
	 != NULL) {

    pEvent->fCallback(pEvent->pContext);

    static_scheduler_event_destroy(&pEvent);

    // Update simulation time
    (pSimulator->dCurrentTime)+= 1;

    // Limit on simulation time ??
    if ((pSimulator->dMaximumTime > 0) &&
	(pSimulator->dCurrentTime >= pSimulator->dMaximumTime)) {
      LOG_WARNING("WARNING: Simulation stopped @ %2.2f.\n",
		  pSimulator->dCurrentTime);
      break;
    }
  }
  return 0;
}

// ----- static_scheduler_post --------------------------------------
int static_scheduler_post(FSimEventCallback fCallback,
			  FSimEventDump fDump,
			  FSimEventDestroy fDestroy,
			  void * pContext,
			  double uSchedulingTime,
			  uint8_t uDeltaType)
{
  SStaticEvent * pEvent;

  assert((uSchedulingTime == 0) && (uDeltaType == RELATIVE_TIME));
  pEvent= static_scheduler_event_create(fCallback, fDump, fDestroy, pContext);

  return fifo_push(pStaticScheduler->pEvents, pEvent);
}

// ----- static_scheduler_dump_events -------------------------------
/**
 * Return information 
 */
void static_scheduler_dump_events(FILE * pStream)
{
  SStaticEvent * pEvent;
  uint32_t uDepth;
  uint32_t uMaxDepth;
  uint32_t uStart;
  int iIndex;

  uDepth= pStaticScheduler->pEvents->uCurrentDepth;
  uMaxDepth= pStaticScheduler->pEvents->uMaxDepth;
  uStart= pStaticScheduler->pEvents->uStartIndex;
  fprintf(pStream, "Number of events queued: %u (%u)\n",
	  uDepth, uMaxDepth);
  for (iIndex= 0; iIndex < uDepth; iIndex++) {
    pEvent= (SStaticEvent *) pStaticScheduler->pEvents->ppItems[(uStart+iIndex) % uMaxDepth];
    fprintf(pStream, "(%d) ", (uStart+iIndex) % uMaxDepth);
    fflush(pStream);
    if (pEvent->fDump != NULL) {
      pEvent->fDump(pStream, pEvent->pContext);
    } else {
      fprintf(pStream, "unknown");
    }
    fprintf(pStream, "\n");
  }
}
