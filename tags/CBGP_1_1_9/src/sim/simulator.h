// ==================================================================
// @(#)simulator.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be), Sebastien Tandel
// @date 28/11/2002
// @lastdate 12/02/2004
// ==================================================================

#ifndef __SIMULATOR_H__
#define __SIMULATOR_H__

#include <libgds/types.h>
#include <libgds/log.h>
#include <sim/scheduler.h>

#define SCHEDULER_STATIC  1
#define SCHEDULER_DYNAMIC 2

uint8_t SIM_OPTIONS_SCHEDULER;

#define ABSOLUTE_TIME	0x00
#define RELATIVE_TIME	0x01

// ----- FSimEventCallback ------------------------------------------
typedef int (*FSimEventCallback)(void * pContext);
// ----- FSimEventDestroy -------------------------------------------
typedef void (*FSimEventDestroy)(void * pContext);

// ----- FSimSchedulerRun -------------------------------------------
typedef int (*FSimSchedulerRun)(void * pContext);
// ----- FSimSchedulerPost ------------------------------------------
typedef int (*FSimSchedulerPost)(FSimEventCallback fCallback,
				 FSimEventDestroy fDestroy,
				 void * pContext,
				 double uSchedulingTime,
				 uint8_t uDeltaType);

typedef struct {
  FSimSchedulerRun fSchedulerRun;
  FSimSchedulerPost fSchedulerPost;
  double dCurrentTime;
  double dMaximumTime;
} SSimulator;

// ----- simulator_create -------------------------------------------
extern void simulator_init();
// ----- simulator_destroy ------------------------------------------
extern void simulator_done();
// ----- simulator_dun ----------------------------------------------
extern int simulator_run();
// ----- simulator_post_event ---------------------------------------
extern int simulator_post_event(FSimEventCallback fCallback,
				FSimEventDestroy fDestroy,
				void * pContext, double uSchedulingTime,
				uint8_t uDeltaType);
// ----- simulator_get_time -----------------------------------------
extern double simulator_get_time();
// ----- simulator_set_max_time -------------------------------------
extern void simulator_set_max_time(double dMaximumTime);

#endif

