// ==================================================================
// @(#)static_scheduler.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 30/07/2003
// @lastdate 02/08/2005
// ==================================================================

#ifndef __STATIC_SCHEDULER_H__
#define __STATIC_SCHEDULER_H__

#include <stdio.h>
#include <libgds/fifo.h>
#include <sim/simulator.h>

typedef struct {
  SFIFO * pEvents;
} SStaticScheduler;

// ----- static_scheduler_init --------------------------------------
extern SStaticScheduler * static_scheduler_init();
// ----- static_scheduler_done --------------------------------------
extern void static_scheduler_done();
// ----- static_scheduler_run ---------------------------------------
extern int static_scheduler_run(void * pContext, int iNumSteps);
// ----- static_scheduler_post --------------------------------------
extern int static_scheduler_post(FSimEventCallback fCallback,
				 FSimEventDump fDump,
				 FSimEventDestroy fDestroy,
				 void * pContext,
				 double uSchedulingTime,
				 uint8_t uDeltaType);
// ----- static_scheduler_dump_events -------------------------------
extern void static_scheduler_dump_events(FILE * pStream);

#endif
