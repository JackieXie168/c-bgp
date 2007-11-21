// ==================================================================
// @(#)sim.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 24/07/2003
// @lastdate 21/11/2007
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <libgds/cli.h>
#include <libgds/cli_ctx.h>
#include <libgds/log.h>
#include <libgds/str_util.h>

#include <cli/common.h>
#include <cli/sim.h>
#include <net/network.h>
#include <sim/simulator.h>

SCli * pTheCli= NULL;

// ----- cli_sim_debug_queue ----------------------------------------
/**
 * context: {}
 * tokens: {}
 */
int cli_sim_debug_queue(SCliContext * pContext, SCliCmd * pCmd)
{
  simulator_dump_events(pLogErr, network_get_simulator());
  return CLI_SUCCESS;
}

// -----[ _cli_sim_event_callback ]----------------------------------
static int _cli_sim_event_callback(SSimulator * pSimulator,
				   void * pContext)
{
  char * pcCommand= (char *) pContext;
  int iResult= 0;

  if (pTheCli != NULL)
    if (cli_execute(pTheCli, pcCommand) < 0)
      iResult= -1;
  str_destroy(&pcCommand);
  return iResult;
}

// -----[ _cli_sim_event_destroy ]-----------------------------------
static void _cli_sim_event_destroy(void * pContext)
{
  char * pCharContext= (char *) pContext;
  str_destroy(&pCharContext);
}

// -----[ _cli_sim_event_dump ]--------------------------------------
static void _cli_sim_event_dump(SLogStream * pStream, void * pContext)
{
  char * pcCommand= (char *) pContext;
  log_printf(pStream, "cli-event [%s]\n", pcCommand);
}

// ----- cli_sim_event ----------------------------------------------
/**
 * context: {}
 * tokens: {time, command}
 */
int cli_sim_event(SCliContext * pContext, SCliCmd * pCmd)
{
  double dTime;

  if (tokens_get_double_at(pCmd->pParamValues, 0, &dTime))
    return CLI_ERROR_COMMAND_FAILED;
  simulator_post_event(network_get_simulator(),
		       _cli_sim_event_callback,
		       _cli_sim_event_dump,
		       _cli_sim_event_destroy,
		       str_create(tokens_get_string_at(pCmd->pParamValues, 1)),
		       dTime,
		       ABSOLUTE_TIME);
  return CLI_SUCCESS;
}

// ----- cli_sim_options_loglevel -----------------------------------
/**
 * context: {}
 * tokens: {log-level}
 */
int cli_sim_options_loglevel(SCliContext * pContext, SCliCmd * pCmd)
{
  char * pcParam;

  pcParam= tokens_get_string_at(pCmd->pParamValues, 0);
  log_set_level(pLogDebug, log_str2level(pcParam));
  return CLI_SUCCESS;
}

// ----- cli_sim_options_scheduler ----------------------------------
/**
 * context: {}
 * tokens: {function}
 */
int cli_sim_options_scheduler(SCliContext * pContext, SCliCmd * pCmd)
{
  char * pcParam;

  pcParam= tokens_get_string_at(pCmd->pParamValues, 0);
  if (!strcmp(pcParam, "static"))
    SIM_OPTIONS_SCHEDULER= SCHEDULER_STATIC;
  else if (!strcmp(pcParam, "dynamic"))
    SIM_OPTIONS_SCHEDULER= SCHEDULER_DYNAMIC;
  else
    return CLI_ERROR_COMMAND_FAILED;
  return CLI_SUCCESS;
}

// ----- cli_sim_queue_info -----------------------------------------
/**
 *
 */
int cli_sim_queue_info(SCliContext * pContext, SCliCmd * pCmd)
{
  simulator_show_infos(pLogOut, network_get_simulator());
  return CLI_SUCCESS;
}

// ----- cli_sim_queue_log ------------------------------------------
/**
 *
 */
int cli_sim_queue_log(SCliContext * pContext, SCliCmd * pCmd)
{
  simulator_set_log_progress(network_get_simulator(),
			     tokens_get_string_at(pCmd->pParamValues, 0));
  return CLI_SUCCESS;
}

// ----- cli_sim_queue_show -----------------------------------------
/**
 *
 */
int cli_sim_queue_show(SCliContext * pContext, SCliCmd * pCmd)
{
  simulator_dump_events(pLogOut, network_get_simulator());
  return CLI_SUCCESS;
}


// ----- cli_sim_run ------------------------------------------------
int cli_sim_run(SCliContext * pContext, SCliCmd * pCmd)
{
  if (simulator_run(network_get_simulator()))
    return CLI_ERROR_COMMAND_FAILED;
  return CLI_SUCCESS;
}

// ----- cli_sim_step -----------------------------------------------
/**
 * context: {}
 * tokens: {num-steps}
 */
int cli_sim_step(SCliContext * pContext, SCliCmd * pCmd)
{
  unsigned int uNumSteps;

  // Get number of steps
  if (tokens_get_uint_at(pCmd->pParamValues, 0, &uNumSteps)) {
    cli_set_user_error(cli_get(), "invalid number of steps \"%s\".",
		       tokens_get_string_at(pCmd->pParamValues, 0));
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Run the simulator for the given number of steps
  if (simulator_step(network_get_simulator(), (int) uNumSteps))
    return CLI_ERROR_COMMAND_FAILED;

  return CLI_SUCCESS;
}

// ----- cli_sim_stop_at --------------------------------------------
/**
 * context: {}
 * tokens: {max-time}
 */
int cli_sim_stop_at(SCliContext * pContext, SCliCmd * pCmd)
{
  double dMaxTime;

  if (tokens_get_double_at(pCmd->pParamValues, 0, &dMaxTime))
    return CLI_ERROR_COMMAND_FAILED;
  simulator_set_max_time(network_get_simulator(), dMaxTime);
  return CLI_SUCCESS;
}

// ----- cli_sim_stop -----------------------------------------------
int cli_sim_stop(SCliContext * pContext, SCliCmd * pCmd)
{
  return CLI_ERROR_COMMAND_FAILED;
}

// ----- cli_register_sim_debug -------------------------------------
int cli_register_sim_debug(SCliCmds * pCmds)
{
  SCliCmds * pSubCmds;

  pSubCmds= cli_cmds_create();
  cli_cmds_add(pSubCmds, cli_cmd_create("queue", cli_sim_debug_queue,
					NULL, NULL));
  return cli_cmds_add(pCmds, cli_cmd_create("debug", NULL,
					    pSubCmds, NULL));
}

// ----- cli_register_sim_event -------------------------------------
int cli_register_sim_event(SCliCmds * pCmds)
{
  SCliParams * pParams;

  pParams= cli_params_create();
  cli_params_add(pParams, "<time>", NULL);
  cli_params_add(pParams, "<event>", NULL);
  return cli_cmds_add(pCmds, cli_cmd_create("event", cli_sim_event,
					    NULL, pParams));
}

// ----- cli_register_sim_options -----------------------------------
int cli_register_sim_options(SCliCmds * pCmds)
{
  SCliCmds * pSubCmds;
  SCliParams * pParams;

  pSubCmds= cli_cmds_create();
  pParams= cli_params_create();
  cli_params_add(pParams, "<level>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("log-level",
					cli_sim_options_loglevel,
					NULL, pParams));
  pParams= cli_params_create();
  cli_params_add(pParams, "<scheduler>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("scheduler",
					cli_sim_options_scheduler,
					NULL, pParams));
  return cli_cmds_add(pCmds, cli_cmd_create("options", NULL,
					    pSubCmds, NULL));
}

// ----- cli_register_sim_queue -------------------------------------
int cli_register_sim_queue(SCliCmds * pCmds)
{
  SCliCmds * pSubCmds;
  SCliParams * pParams;

  pSubCmds= cli_cmds_create();
  cli_cmds_add(pSubCmds, cli_cmd_create("info", cli_sim_queue_info,
					NULL, NULL));
  pParams= cli_params_create();
  cli_params_add(pParams, "<file>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("log", cli_sim_queue_log,
					NULL, pParams));
  cli_cmds_add(pSubCmds, cli_cmd_create("show",	cli_sim_queue_show,
					NULL, NULL));
  return cli_cmds_add(pCmds, cli_cmd_create("queue", NULL,
					    pSubCmds, NULL));
}

// ----- cli_register_sim_run ---------------------------------------
int cli_register_sim_run(SCliCmds * pCmds)
{
  return cli_cmds_add(pCmds, cli_cmd_create("run", cli_sim_run,
					    NULL, NULL));
}

// ----- cli_register_sim_step ---------------------------------------
int cli_register_sim_step(SCliCmds * pCmds)
{
  SCliParams * pParams;

  pParams= cli_params_create();
  cli_params_add(pParams, "<num-steps>", NULL);
  return cli_cmds_add(pCmds, cli_cmd_create("step", cli_sim_step,
					    NULL, pParams));
}

// ----- cli_register_sim_stop --------------------------------------
int cli_register_sim_stop(SCliCmds * pCmds)
{
  SCliCmds * pSubCmds;
  SCliParams * pParams;

  pSubCmds= cli_cmds_create();
  pParams= cli_params_create();
  cli_params_add(pParams, "<time>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("at", cli_sim_stop_at,
					NULL, pParams));
  return cli_cmds_add(pCmds, cli_cmd_create("stop", cli_sim_stop,
					    pSubCmds, NULL));
}

// ----- cli_register_sim -------------------------------------------
/**
 *
 */
int cli_register_sim(SCli * pCli)
{
  SCliCmds * pCmds;

  pTheCli= pCli;

  pCmds= cli_cmds_create();
  cli_register_sim_debug(pCmds);
  cli_register_sim_event(pCmds);
  cli_register_sim_options(pCmds);
  cli_register_sim_queue(pCmds);
  cli_register_sim_run(pCmds);
  cli_register_sim_step(pCmds);
  cli_register_sim_stop(pCmds);
  cli_register_cmd(pCli, cli_cmd_create("sim", NULL, pCmds, NULL));
  return 0;
}
