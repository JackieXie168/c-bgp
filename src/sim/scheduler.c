// ==================================================================
// @(#)scheduler.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be), Sebastien Tandel
// @date 12/06/2003
// @lastdate 20/04/2004
// ==================================================================

#include <sim/scheduler.h>

typedef struct {
  SList * pEvents;
} SScheduler;

typedef struct {
  float uTime;
  SFIFO * pFifoEvents;
} SSchedulerFifoEvent;

SScheduler * pScheduler = NULL;

#define EVENT_QUEUE_DEPTH 40000000

uint32_t uCountEvent = 0;
uint32_t uCountPost = 0;

// ----- scheduler_event_compare ------------------------------------
/**
 *
 */
int scheduler_event_compare(void * pItem1, void * pItem2)
{
  SSchedulerFifoEvent * pEvent1= (SSchedulerFifoEvent *) pItem1;
  SSchedulerFifoEvent * pEvent2= (SSchedulerFifoEvent *) pItem2;

  if (pEvent1->uTime < pEvent2->uTime) 
    return 1;
  else if (pEvent1->uTime > pEvent2->uTime) 
    return -1;
  else 
    return 0;
}


/*void scheduler_event_fifo_destroy(void ** pItem)
  {
  SSchedEvent ** ppEvent = (SSchedulerEvent **) ppItem;

  scheduler_event_destroy(ppEvent);
  }*/

// ----- scheduler_event_create -------------------------------------
/**
 *
 */
SSchedEvent * scheduler_event_create(FSchedEventCallback fCallback,
				     void * pContext)
{
  SSchedEvent * pEvent= (SSchedEvent *) MALLOC(sizeof(SSchedEvent));

  pEvent->fCallback= fCallback;
  pEvent->pContext= pContext;
  uCountEventCreated++;
  return pEvent;
}

// ----- scheduler_insert_event -------------------------------------
/**
 *
 */
int scheduler_event_fifo_insert(SFIFO * pFifoEvents,
				FSchedEventCallback fCallback,
				void * pContext)
{
  SSchedEvent * pEvent= scheduler_event_create(fCallback, pContext);
		
  return fifo_push(pFifoEvents, pEvent);
}

// ----- scheduler_event_destroy ------------------------------------
/**
 *
 */
void scheduler_event_destroy(SSchedEvent ** ppEvent)
{
  if (*ppEvent != NULL) {
    uCountEventDestroyed++;
    FREE(*ppEvent);
    *ppEvent= NULL;
  }
}

// ----- scheduler_event_destroy_wrapper ----------------------------
/**
 *
 */
inline void scheduler_event_destroy_wrapper(void ** ppItem)
{
  scheduler_event_destroy((SSchedEvent **) ppItem);
}

// ----- scheduler_event_create -------------------------------------
/**
 *
 */
SSchedulerFifoEvent * scheduler_event_fifo_create(float uSchedulingTime)
{
  SSchedulerFifoEvent * pEvent=
    (SSchedulerFifoEvent *) MALLOC(sizeof(SSchedulerFifoEvent));
  pEvent->pFifoEvents= fifo_create(EVENT_QUEUE_DEPTH,
				   scheduler_event_destroy_wrapper);
  pEvent->uTime= uSchedulingTime;
  uCountFifoEventCreated++;
  return pEvent;
}


// ----- scheduler_event_fifo_destroy -------------------------------
/**
 *
 */
void scheduler_event_fifo_destroy(SSchedulerFifoEvent ** ppFifoEvent)
{
  if (*ppFifoEvent != NULL) {
    if ((*ppFifoEvent)->pFifoEvents != NULL) 
      fifo_destroy(&(*ppFifoEvent)->pFifoEvents);
    uCountFifoEventDestroyed++;
    FREE(*ppFifoEvent);
    *ppFifoEvent= NULL;
  }
}

// ----- scheduler_list_event_destroy -------------------------------
/**
 *
 */
void scheduler_list_event_destroy(void ** ppItem)
{
  SSchedulerFifoEvent ** ppFifoEvent= (SSchedulerFifoEvent **) ppItem;

  scheduler_event_fifo_destroy(ppFifoEvent);
}

// ----- scheduler_create -------------------------------------------
/**
 *
 */
SScheduler * scheduler_create()
{
  SScheduler * pScheduler= (SScheduler *) MALLOC(sizeof(SScheduler));
  pScheduler->pEvents= list_create(scheduler_event_compare,
				   scheduler_list_event_destroy, 50);
  return pScheduler;
}

// ----- scheduler_init ---------------------------------------------
/**
 * Rem : We can imagine initializing the scheduler with a function representing
 * the intelligence of the scheduler.
 */
int scheduler_init()
{
  uCountFifoEventCreated= 0;
  uCountFifoEventDestroyed= 0;
  uCountEventCreated= 0;
  uCountEventDestroyed= 0;
  if (pScheduler == NULL)
    pScheduler= scheduler_create();
  return 0;
}

// ----- scheduler_run ----------------------------------------------
/**
 *
 */
int scheduler_run(float uSimulatorTime)
{
  int iIndexFirstEvent= list_get_nbr_element(pScheduler->pEvents)-1;
  SSchedulerFifoEvent * pFifoEvent= list_get_index(pScheduler->pEvents, 
						    iIndexFirstEvent);
  SSchedEvent * pEvent;
	
  LOG_DEBUG("scheduler> nbr events before scheduling %d\t",
	    iIndexFirstEvent + 1);
  if (pFifoEvent->uTime == uSimulatorTime) {
		
    while ((pEvent = fifo_pop(pFifoEvent->pFifoEvents)) != NULL) {
      uCountEvent++;
      pEvent->fCallback(pEvent->pContext);
      scheduler_event_destroy(&pEvent);
      iIndexFirstEvent = list_get_nbr_element(pScheduler->pEvents) - 1;
      pFifoEvent = list_get_index(pScheduler->pEvents, iIndexFirstEvent);
    }

    list_delete(pScheduler->pEvents, iIndexFirstEvent);
  } else if (pFifoEvent->uTime < uSimulatorTime) {
    LOG_FATAL("Existence of events which will never occur %f", pFifoEvent->uTime);
    exit(1);
  }
  LOG_DEBUG("after %d\n", iIndexFirstEvent + 1);
  return 0;
}

// ----- scheduler_destroy ------------------------------------------
/**
 *
 */
void scheduler_done()
{
  LOG_DEBUG("scheduler> uCountEvent : %d\n", uCountEvent);
  LOG_EVERYTHING("scheduler> uCountPost : %d\n", uCountPost);
  if (pScheduler != NULL) {
    list_destroy(&(pScheduler->pEvents));
    FREE(pScheduler);
    pScheduler = NULL;
  }
}

// ----- scheduler_post_event ---------------------------------------
/**
 *
 */
int scheduler_post_event(FSchedEventCallback fCallback,
			 void * pContext, float uSchedulingTime)
{
  int iIndex, iRet;
  SSchedulerFifoEvent * pFifoEvent;

  LOG_DEBUG("scheduler_post_event\n");
	
  pFifoEvent = scheduler_event_fifo_create(uSchedulingTime);
  uCountPost++;
  if (list_find_index(pScheduler->pEvents, pFifoEvent, &iIndex) == 0) {
    scheduler_event_fifo_destroy((void *)&pFifoEvent);
    pFifoEvent = list_get_index(pScheduler->pEvents, iIndex);
    iRet = scheduler_event_fifo_insert(pFifoEvent->pFifoEvents, fCallback, pContext);
  } else {
    scheduler_event_fifo_insert(pFifoEvent->pFifoEvents, fCallback, pContext);
    iRet = list_insert_index(pScheduler->pEvents, iIndex, pFifoEvent);
  }
  return iRet;
}

// ----- scheduler_isEmpty ------------------------------------------
/**
 *
 */
int scheduler_isEmpty()
{
  if (pScheduler != NULL) 
    return (list_get_nbr_element(pScheduler->pEvents) == 0 ? 1 : 0);
  else
    return -1;
}
