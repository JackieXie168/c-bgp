// ==================================================================
// @(#)simulator.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be), Sebastien Tandel
// @date 28/11/2002
// @lastdate 16/04/2007
// ==================================================================

#ifndef __SIMULATOR_H__
#define __SIMULATOR_H__

#include <libgds/types.h>
#include <libgds/log.h>
#include <sim/scheduler.h>

#define SCHEDULER_STATIC  1
#define SCHEDULER_DYNAMIC 2

extern uint8_t SIM_OPTIONS_SCHEDULER;

#define ABSOLUTE_TIME	0x00
#define RELATIVE_TIME	0x01

struct TSimulator;

// ----- FSimEventCallback ------------------------------------------
typedef int (*FSimEventCallback)(struct TSimulator * pSimulator,
				 void * pContext);
// ----- FSimEventDump ----------------------------------------------
typedef void (*FSimEventDump)(SLogStream * pStream, void * pContext);
// ----- FSimEventDestroy -------------------------------------------
typedef void (*FSimEventDestroy)(void * pContext);

// ----- FSimSchedulerRun -------------------------------------------
typedef int (*FSimSchedulerRun)(void * pSchedCtx, void * pContext,
				int iNumSteps);
// ----- FSimSchedulerPost ------------------------------------------
typedef int (*FSimSchedulerPost)(void * pSchedCtx,
				 FSimEventCallback fCallback,
				 FSimEventDump fDump,
				 FSimEventDestroy fDestroy,
				 void * pContext,
				 double uSchedulingTime,
				 uint8_t uDeltaType);

struct TSimulator {
  void * pScheduler;
  FSimSchedulerRun fSchedulerRun;
  FSimSchedulerPost fSchedulerPost;
  double dCurrentTime;
  double dMaximumTime;
  uint8_t tType;
};
typedef struct TSimulator SSimulator;

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ simulator_create ]---------------------------------------
  SSimulator * simulator_create(uint8_t tType);
  // -----[ simulator_destroy ]--------------------------------------
  void simulator_destroy(SSimulator ** ppSimulator);
  // ----- simulator_run --------------------------------------------
  int simulator_run(SSimulator * pSimulator);
  // ----- simulator_step -------------------------------------------
  int simulator_step(SSimulator * pSimulator, int iNumSteps);
  // ----- simulator_post_event -------------------------------------
  int simulator_post_event(SSimulator * pSimulator,
			   FSimEventCallback fCallback,
			   FSimEventDump fDump,
			   FSimEventDestroy fDestroy,
			   void * pContext, double uSchedulingTime,
			   uint8_t uDeltaType);
  // ----- simulator_get_num_events ---------------------------------
  uint32_t simulator_get_num_events(SSimulator * pSimulator);
  // ----- simulator_get_event --------------------------------------
  void * simulator_get_event(SSimulator * pSimulator, unsigned int uIndex);
  // ----- simulator_get_time ---------------------------------------
  double simulator_get_time(SSimulator * pSimulator);
  // ----- simulator_set_max_time -----------------------------------
  void simulator_set_max_time(SSimulator * pSimulator, double dMaximumTime);
  // ----- simulator_dump_events ------------------------------------
  void simulator_dump_events(SLogStream * pStream, SSimulator * pSimulator);
  // ----- simulator_show_infos -------------------------------------
  void simulator_show_infos(SLogStream * pStream, SSimulator * pSimulator);
  // -----[ simulator_set_log_progress ]-----------------------------
  int simulator_set_log_progress(SSimulator * pSimulator,
				 const char * pcFileName);

#ifdef __cplusplus
}
#endif

#endif

