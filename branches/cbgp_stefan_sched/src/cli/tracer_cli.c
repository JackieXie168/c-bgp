// ==================================================================
// @(#)tracer_cli.c
//
// @author Stefan Beauport (stefan.beauport@umons.Ac.be)


// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <libgds/cli.h>
#include <libgds/cli_ctx.h>
#include <libgds/cli_params.h>
#include <libgds/stream.h>
#include <libgds/str_util.h>

#include <cli/common.h>
#include <cli/sim.h>
#include <net/network.h>
#include <tracer/tracer.h>



// ----- cli_tracer_start ------------------------------------------------
int cli_tracer_start(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  int result = _tracer_start(tracer_get_default());

  if (!result) {
    cli_set_user_error(cli_get(), "Cound not start the tracer. (Tracer is probably already started ! )");
    return CLI_ERROR_COMMAND_FAILED;
  }
  return CLI_SUCCESS;
}
/*
// ----- cli_tracer_start ------------------------------------------------
int cli_tracer_dump(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  int result = _tracer_dump(gdsout ,tracer_get_default());

  if (!result) {
    cli_set_user_error(cli_get(), "Cound not dump the tracer");
    return CLI_ERROR_COMMAND_FAILED;
  }
  return CLI_SUCCESS;
}*/

// ----- cli_tracer_start ------------------------------------------------
int cli_tracer_graph_dump(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  int result = _tracer_dump(gdsout ,tracer_get_default());

  if (!result) {
    cli_set_user_error(cli_get(), "Cound not dump the tracer");
    return CLI_ERROR_COMMAND_FAILED;
  }
  return CLI_SUCCESS;
}

// ----- cli_tracer_start ------------------------------------------------
int cli_tracer_go1step(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  int result = FOR_TESTING_PURPOSE_tracer_go_one_step(tracer_get_default());

  if (!result) {
    cli_set_user_error(cli_get(), "Cound not go 1 step!");
    return CLI_ERROR_COMMAND_FAILED;
  }
  return CLI_SUCCESS;
}

int cli_tracer_graph_inject_state(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
const char * arg= cli_get_arg_value(cmd, 0);
   unsigned int nb;
  // Get number of steps
  if (str_as_uint(arg, &nb)) {
    cli_set_user_error(cli_get(), "invalid nb\"%s\".", arg);
    return CLI_ERROR_COMMAND_FAILED;
  }
   int result =  FOR_TESTING_PURPOSE_tracer_inject_state(tracer_get_default(),nb);

  if (!result) {
    cli_set_user_error(cli_get(), "Cound not inject the state number : %d",nb);
    return CLI_ERROR_COMMAND_FAILED;
  }
  return CLI_SUCCESS;

}

int cli_tracer_graph_dump_state(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
const char * arg= cli_get_arg_value(cmd, 0);
   unsigned int nb;
  // Get number of steps
  if (str_as_uint(arg, &nb)) {
    cli_set_user_error(cli_get(), "invalid nb\"%s\".", arg);
    return CLI_ERROR_COMMAND_FAILED;
  }
   int result =  FOR_TESTING_PURPOSE_tracer_graph_state_dump(gdsout ,tracer_get_default(),nb);

  if (!result) {
    cli_set_user_error(cli_get(), "Cound not dump the state number : %d",nb);
    return CLI_ERROR_COMMAND_FAILED;
  }
  return CLI_SUCCESS;

}

// ----- cli_tracer_start ------------------------------------------------
int cli_tracer_graph_dump_root(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  int result =  FOR_TESTING_PURPOSE_tracer_graph_state_dump(gdsout ,tracer_get_default(),0);

  if (!result) {
    cli_set_user_error(cli_get(), "Cound not dump the root state");
    return CLI_ERROR_COMMAND_FAILED;
  }
  return CLI_SUCCESS;
}

// ----- cli_tracer_start ------------------------------------------------
int cli_tracer_graph_dump_currentState(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  int result = FOR_TESTING_PURPOSE_tracer_graph_current_state_dump(gdsout ,tracer_get_default());

  if (!result) {
    cli_set_user_error(cli_get(), "Cound not dump the current state");
    return CLI_ERROR_COMMAND_FAILED;
  }
  return CLI_SUCCESS;
}

// ----- cli_tracer_start ------------------------------------------------
int cli_tracer_graph_dump_allStates(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  int result = FOR_TESTING_PURPOSE_tracer_graph_allstates_dump(gdsout ,tracer_get_default());

  if (!result) {
    cli_set_user_error(cli_get(), "Cound not dump the current state");
    return CLI_ERROR_COMMAND_FAILED;
  }
  return CLI_SUCCESS;
}

// -----[ _register_sim_run ]----------------------------------------
static void _register_tracer_start(cli_cmd_t * parent)
{
  cli_add_cmd(parent, cli_cmd("start", cli_tracer_start));
}


// -----[ _register_sim_run ]----------------------------------------
static void _register_tracer_graph_dump(cli_cmd_t * parent)
{
  cli_cmd_t * group= cli_add_cmd(parent, cli_cmd_group("dump"));
  cli_add_cmd(group, cli_cmd("root", cli_tracer_graph_dump_root));
  cli_add_cmd(group, cli_cmd("currentState", cli_tracer_graph_dump_currentState));
  cli_cmd_t * cmd= cli_add_cmd(group, cli_cmd("state", cli_tracer_graph_dump_state));
  cli_add_arg(cmd, cli_arg("state number", NULL));
  cli_add_cmd(group, cli_cmd("allStates", cli_tracer_graph_dump_allStates));

}
// -----[ _register_sim_run ]----------------------------------------
static void _register_tracer_graph_inject(cli_cmd_t * parent)
{
  cli_cmd_t * group= cli_add_cmd(parent, cli_cmd_group("inject"));
  cli_cmd_t * cmd= cli_add_cmd(group, cli_cmd("state", cli_tracer_graph_inject_state));
  cli_add_arg(cmd, cli_arg("state number", NULL));

  
}

// -----[ _register_sim_run ]----------------------------------------
static void _register_tracer_graph(cli_cmd_t * parent)
{
  cli_cmd_t * group= cli_add_cmd(parent, cli_cmd_group("graph"));
  _register_tracer_graph_dump(group);
  _register_tracer_graph_inject(group);

}


// -----[ _register_sim_run ]----------------------------------------
static void _register_tracer_go1step(cli_cmd_t * parent)
{
  cli_add_cmd(parent, cli_cmd("go1step", cli_tracer_go1step));
}


// ----- cli_register_sim -------------------------------------------
void cli_register_tracer(cli_cmd_t * parent)
{
  cli_cmd_t * group= cli_add_cmd(parent, cli_cmd_group("tracer"));

  _register_tracer_start(group);
  //_register_tracer_dump(group);
  _register_tracer_graph(group);
  _register_tracer_go1step(group);


/*  _register_sim_debug(group);
  _register_sim_event(group);
  _register_sim_options(group);
  _register_sim_queue(group);
  _register_sim_run(group);
  _register_sim_step(group);
  _register_sim_stop(group);
 * */

}
