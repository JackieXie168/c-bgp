// ==================================================================
// @(#)sim.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 24/07/2003
// @lastdate 02/08/2005
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <cli/common.h>
#include <cli/sim.h>
#include <libgds/cli_ctx.h>
#include <libgds/log.h>
#include <sim/simulator.h>
#include <libgds/str_util.h>

SCli * pTheCli= NULL;

// ----- cli_sim_debug_queue ----------------------------------------
/**
 * context: {}
 * tokens: {}
 */
int cli_sim_debug_queue(SCliContext * pContext, STokens * pTokens)
{
  simulator_dump_events(stderr);
  return CLI_SUCCESS;
}

// ----- cli_sim_event_callback -------------------------------------
int cli_sim_event_callback(void * pContext)
{
  char * pcCommand= (char *) pContext;
  int iResult= 0;

  if (pTheCli != NULL)
    if (cli_execute(pTheCli, pcCommand) < 0)
      iResult= -1;
  str_destroy(&pcCommand);
  return iResult;
}

// ----- cli_sim_event_destroy --------------------------------------
void cli_sim_event_destroy(void * pContext)
{
  char * pCharContext= (char *) pContext;
  str_destroy(&pCharContext);
}

// ----- cli_sim_event_dump -----------------------------------------
void cli_sim_event_dump(FILE * pStream, void * pContext)
{
  char * pcCommand= (char *) pContext;

  fprintf(pStream, "cli-event [%s]\n", pcCommand);
}

// ----- cli_sim_event ----------------------------------------------
/**
 * context: {}
 * tokens: {time, command}
 */
int cli_sim_event(SCliContext * pContext, STokens * pTokens)
{
  double dTime;

  if (tokens_get_double_at(pTokens, 0, &dTime))
    return CLI_ERROR_COMMAND_FAILED;
  simulator_post_event(cli_sim_event_callback,
		       cli_sim_event_dump,
		       cli_sim_event_destroy,
		       str_create(tokens_get_string_at(pTokens, 1)),
		       dTime,
		       ABSOLUTE_TIME);
  return CLI_SUCCESS;
}

// ----- cli_sim_options_loglevel -----------------------------------
/**
 * context: {}
 * tokens: {log-level}
 */
int cli_sim_options_loglevel(SCliContext * pContext, STokens * pTokens)
{
  char * pcParam;

  pcParam= tokens_get_string_at(pTokens, 0);
  log_set_level(pMainLog, log_str2level(pcParam));
  return CLI_SUCCESS;
}

// ----- cli_sim_options_scheduler ----------------------------------
/**
 * context: {}
 * tokens: {function}
 */
int cli_sim_options_scheduler(SCliContext * pContext, STokens * pTokens)
{
  char * pcParam;

  pcParam= tokens_get_string_at(pTokens, 0);
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
int cli_sim_queue_info(SCliContext * pContext, STokens * pTokens)
{
  simulator_show_infos();
  return CLI_SUCCESS;
}

// ----- cli_sim_queue_show -----------------------------------------
/**
 *
 */
int cli_sim_queue_show(SCliContext * pContext, STokens * pTokens)
{
  simulator_dump_events(stdout);
  return CLI_SUCCESS;
}


// ----- cli_sim_run ------------------------------------------------
int cli_sim_run(SCliContext * pContext, STokens * pTokens)
{
  if (simulator_run())
    return CLI_ERROR_COMMAND_FAILED;
  return CLI_SUCCESS;
}

// ----- cli_sim_step -----------------------------------------------
/**
 * context: {}
 * tokens: {num-steps}
 */
int cli_sim_step(SCliContext * pContext, STokens * pTokens)
{
  unsigned int uNumSteps;

  // Get number of steps
  if (tokens_get_uint_at(pTokens, 0, &uNumSteps)) {
    LOG_SEVERE("Error: invalid number of steps \"%s\".\n",
	       tokens_get_string_at(pTokens, 0));
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Run the simulator for the given number of steps
  if (simulator_step((int) uNumSteps))
    return CLI_ERROR_COMMAND_FAILED;

  return CLI_SUCCESS;
}

// ----- cli_sim_stop_at --------------------------------------------
/**
 * context: {}
 * tokens: {max-time}
 */
int cli_sim_stop_at(SCliContext * pContext, STokens * pTokens)
{
  double dMaxTime;

  if (tokens_get_double_at(pTokens, 0, &dMaxTime))
    return CLI_ERROR_COMMAND_FAILED;
  simulator_set_max_time(dMaxTime);
  return CLI_SUCCESS;
}

// ----- cli_sim_stop -----------------------------------------------
int cli_sim_stop(SCliContext * pContext, STokens * pTokens)
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

  pSubCmds= cli_cmds_create();
  cli_cmds_add(pSubCmds, cli_cmd_create("info",
					cli_sim_queue_info,
					NULL, NULL));
  cli_cmds_add(pSubCmds, cli_cmd_create("show",
					cli_sim_queue_show,
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
