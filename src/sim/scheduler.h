// ==================================================================
// @(#)scheduler.h
//
// @author Sebastien Tandel (sta@infonet.fundp.ac.be)
// @date 12/06/2003
// @lastdate 13/06/2003
// ==================================================================

#ifndef __SCHEDULER_H__
#define __SCHEDULER_H__

#include <libgds/types.h>
#include <libgds/list.h>
#include <libgds/log.h>
#include <libgds/memory.h>
//#include <dispatcher.h>
#include <libgds/fifo.h>

// ----- FSchedulerEventCallback ------------------------------------
typedef void (*FSchedEventCallback) (void * pItem);

typedef struct {
	FSchedEventCallback fCallback;
	void * pContext;
} SSchedEvent;

// ----- scheduler_init ---------------------------------------------
extern int scheduler_init();
// ----- scheduler_run ----------------------------------------------
extern int scheduler_run(float);
// ----- scheduler_done ---------------------------------------------
extern void scheduler_done();
// ----- scheduler_post_event ---------------------------------------
extern int scheduler_post_event(FSchedEventCallback fCallback, void * pContext, float uSchedulingTime);
// ----- scheduler_isEmpty ------------------------------------------
extern int scheduler_isEmpty();

#endif

