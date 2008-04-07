// ==================================================================
// @(#)sim.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 24/07/2003
// @lastdate 03/04/2008
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

static SCli * _cli= NULL;

// ----- cli_sim_debug_queue ----------------------------------------
/**
 * context: {}
 * tokens: {}
 */
int cli_sim_debug_queue(SCliContext * pContext, SCliCmd * pCmd)
{
  sim_dump_events(pLogErr, network_get_simulator(network_get_default()));
  return CLI_SUCCESS;
}

// -----[ _cli_sim_event_callback ]----------------------------------
static int _cli_sim_event_callback(simulator_t * sim,
				   void * ctx)
{
  char * cmd= (char *) ctx;
  int iResult= 0;

  if (_cli != NULL)
    if (cli_execute(_cli, cmd) < 0)
      iResult= -1;
  str_destroy(&cmd);
  return iResult;
}

// -----[ _cli_sim_event_destroy ]-----------------------------------
static void _cli_sim_event_destroy(void * ctx)
{
  char * cmd= (char *) ctx;
  str_destroy(&cmd);
}

// -----[ _cli_sim_event_dump ]--------------------------------------
static void _cli_sim_event_dump(SLogStream * stream, void * ctx)
{
  char * cmd= (char *) ctx;
  log_printf(stream, "cli-event [%s]\n", cmd);
}

static sim_event_ops_t _sim_event_ops= {
  .callback= _cli_sim_event_callback,
  .destroy = _cli_sim_event_destroy,
  .dump    = _cli_sim_event_dump,
};

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
  sim_post_event(network_get_simulator(network_get_default()),
		 &_sim_event_ops,
		 str_create(tokens_get_string_at(pCmd->pParamValues, 1)),
		 dTime,
		 SIM_TIME_ABS);
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
  sim_show_infos(pLogOut, network_get_simulator(network_get_default()));
  return CLI_SUCCESS;
}

// ----- cli_sim_queue_log ------------------------------------------
/**
 *
 */
int cli_sim_queue_log(SCliContext * pContext, SCliCmd * pCmd)
{
  sim_set_log_progress(network_get_simulator(network_get_default()),
		       tokens_get_string_at(pCmd->pParamValues, 0));
  return CLI_SUCCESS;
}

// ----- cli_sim_queue_show -----------------------------------------
/**
 *
 */
int cli_sim_queue_show(SCliContext * pContext, SCliCmd * pCmd)
{
  sim_dump_events(pLogOut, network_get_simulator(network_get_default()));
  return CLI_SUCCESS;
}


// ----- cli_sim_run ------------------------------------------------
int cli_sim_run(SCliContext * pContext, SCliCmd * pCmd)
{
  if (sim_run(network_get_simulator(network_get_default())))
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
  unsigned int num_steps;

  // Get number of steps
  if (tokens_get_uint_at(pCmd->pParamValues, 0, &num_steps)) {
    cli_set_user_error(cli_get(), "invalid number of steps \"%s\".",
		       tokens_get_string_at(pCmd->pParamValues, 0));
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Run the simulator for the given number of steps
  if (sim_step(network_get_simulator(network_get_default()),
	       num_steps))
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
  double max_time;

  if (tokens_get_double_at(pCmd->pParamValues, 0, &max_time))
    return CLI_ERROR_COMMAND_FAILED;
  sim_set_max_time(network_get_simulator(network_get_default()),
		   max_time);
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

  _cli= pCli;

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
