// ==================================================================
// @(#)static_scheduler.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 30/07/2003
// @lastdate 16/04/2007
// ==================================================================

#ifndef __STATIC_SCHEDULER_H__
#define __STATIC_SCHEDULER_H__

#include <stdio.h>
#include <libgds/fifo.h>
#include <libgds/log.h>
#include <sim/simulator.h>

typedef struct {
  SFIFO * pEvents;
} SStaticScheduler;

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ static_scheduler_create ]--------------------------------
  SStaticScheduler * static_scheduler_create();
  // -----[ static_scheduler_destroy ]-------------------------------
  void static_scheduler_destroy(SStaticScheduler ** ppScheduler);
  // ----- static_scheduler_run -------------------------------------
  int static_scheduler_run(void * pSchedCtx, void * pContext,
			   int iNumSteps);
  // ----- static_scheduler_post ------------------------------------
  int static_scheduler_post(void * pSchedCtx,
			    FSimEventCallback fCallback,
			    FSimEventDump fDump,
			    FSimEventDestroy fDestroy,
			    void * pContext,
			    double uSchedulingTime,
			    uint8_t uDeltaType);
  // ----- static_scheduler_get_num_events --------------------------
  uint32_t static_scheduler_get_num_events(void * pSchedCtx);
  // ----- static_scheduler_dump_events -----------------------------
  void static_scheduler_dump_events(SLogStream * pStream, void * pSchedCtx);

#ifdef __cplusplus
}
#endif

#endif
