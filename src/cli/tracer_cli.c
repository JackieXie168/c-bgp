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
  int result = 0 ; //FOR_TESTING_PURPOSE_tracer_go_one_step(tracer_get_default());

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
   //int result = FOR_TESTING_PURPOSE_tracer_inject_state(tracer_get_default(),nb);
   int result = tracer_inject_state(tracer_get_default(),nb);

  if (!result) {
    cli_set_user_error(cli_get(), "Cound not inject the state number : %d",nb);
    return CLI_ERROR_COMMAND_FAILED;
  }
  return CLI_SUCCESS;

}

int cli_tracer_trace_whole_graph(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  if( tracer_trace_whole_graph(tracer_get_default()) >0 )
    return CLI_SUCCESS;
  return CLI_ERROR_COMMAND_FAILED;
}

// ----- cli_tracer_start ------------------------------------------------
int cli_tracer_graph_export_dot(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  tracer_graph_export_dot(gdsout,tracer_get_default());
  
  return CLI_SUCCESS;
}
// ----- cli_tracer_start ------------------------------------------------
int cli_tracer_graph_cycle_detectAll(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  tracer_graph_detect_every_cycle(gdsout,tracer_get_default());
  return CLI_SUCCESS;
}

int cli_tracer_graph_export_dot_to_file(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  tracer_graph_export_dot_to_file(tracer_get_default());
  
  return CLI_SUCCESS;
}

// ----- cli_tracer_start ------------------------------------------------
int cli_tracer_graph_export_allStates_to_file(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
   tracer_graph_export_allStates_to_file(tracer_get_default());

  return CLI_SUCCESS;
}

// ----- cli_tracer_start ------------------------------------------------
int cli_tracer_graph_export_dot_allStates_to_file(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
   tracer_graph_export_dot_allStates_to_file(tracer_get_default());

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
   //int result =  FOR_TESTING_PURPOSE_tracer_graph_state_dump(gdsout ,tracer_get_default(),nb);
   int result =  tracer_state_dump(gdsout ,tracer_get_default(),nb);

  if (!result) {
    cli_set_user_error(cli_get(), "Cound not dump the state number : %d",nb);
    return CLI_ERROR_COMMAND_FAILED;
  }
  return CLI_SUCCESS;

}

// ----- cli_tracer_start ------------------------------------------------
int cli_tracer_graph_dump_root(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  //int result =  FOR_TESTING_PURPOSE_tracer_graph_state_dump(gdsout ,tracer_get_default(),0);
    int result = tracer_state_dump(gdsout ,tracer_get_default(),0);

  if (!result) {
    cli_set_user_error(cli_get(), "Cound not dump the root state");
    return CLI_ERROR_COMMAND_FAILED;
  }
  return CLI_SUCCESS;
}

// ----- cli_tracer_start ------------------------------------------------
int cli_tracer_graph_dump_currentState(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  int result = 0; //FOR_TESTING_PURPOSE_tracer_graph_current_state_dump(gdsout ,tracer_get_default());

  if (!result) {
    cli_set_user_error(cli_get(), "Cound not dump the current state");
    return CLI_ERROR_COMMAND_FAILED;
  }
  return CLI_SUCCESS;
}

// ----- cli_tracer_start ------------------------------------------------
int cli_tracer_graph_dump_allStates(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  int result = tracer_graph_allstates_dump(gdsout ,tracer_get_default());

  if (!result) {
    cli_set_user_error(cli_get(), "Cound not dump the current state");
    return CLI_ERROR_COMMAND_FAILED;
  }
  return CLI_SUCCESS;
}

int cli_tracer_filter_bgpsessionqueue_unset(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
    filter_unset_limit_on_max_nb_in_oriented_session(tracer_get_default());
    return CLI_SUCCESS;
}
int cli_tracer_filter_depth_unset(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
    filter_unset_limit_on_depth(tracer_get_default());
    return CLI_SUCCESS;
}
int cli_tracer_filter_bgpsessionqueue_set(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
    const char * arg1= cli_get_arg_value(cmd, 0);
    unsigned int nummmm;
    if (str_as_uint(arg1, &nummmm)) {
        cli_set_user_error(cli_get(), "invalid bgpsessionqueue value \"%s\".", arg1);
    return CLI_ERROR_COMMAND_FAILED;
  }
    filter_set_limit_on_max_nb_in_oriented_session(tracer_get_default(), nummmm);
    return CLI_SUCCESS;
}
int cli_tracer_filter_depth_set(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
    const char * arg1= cli_get_arg_value(cmd, 0);
    unsigned int nummmm;
    if (str_as_uint(arg1, &nummmm)) {
        cli_set_user_error(cli_get(), "invalid depth value \"%s\".", arg1);
    return CLI_ERROR_COMMAND_FAILED;
  }
    filter_set_limit_on_depth(tracer_get_default(), nummmm);
    return CLI_SUCCESS;
}
int cli_tracer_filter_diffDepth_set(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
    const char * arg1= cli_get_arg_value(cmd, 0);
    unsigned int nummmm;
    if (str_as_uint(arg1, &nummmm)) {
        cli_set_user_error(cli_get(), "invalid diff depth value \"%s\".", arg1);
    return CLI_ERROR_COMMAND_FAILED;
  }
    filter_set_limit_on_diff_depth(tracer_get_default(), nummmm);
    return CLI_SUCCESS;
}
int cli_tracer_filter_diffDepth_unset(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
    filter_unset_limit_on_diff_depth(tracer_get_default());
    return CLI_SUCCESS;
}


int cli_tracer_filter_maxTransition_set(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
    const char * arg1= cli_get_arg_value(cmd, 0);
    unsigned int nummmm;
    if (str_as_uint(arg1, &nummmm)) {
        cli_set_user_error(cli_get(), "invalid max transition value \"%s\".", arg1);
    return CLI_ERROR_COMMAND_FAILED;
  }
    filter_set_maxNbOfTreatedTransitions(tracer_get_default(), nummmm);
    return CLI_SUCCESS;
}
int cli_tracer_filter_maxTransition_unset(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
    filter_unset_maxNbOfTreatedTransitions(tracer_get_default());
    return CLI_SUCCESS;
}

int cli_tracer_filter_withdrawmsg_accept(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
    filter_withdrawMsg_accept(tracer_get_default());
    return CLI_SUCCESS;
}
int cli_tracer_filter_withdrawmsg_reject(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
    filter_withdrawMsg_reject(tracer_get_default());
    return CLI_SUCCESS;
}


int cli_tracer_filter_multipleMsgInSameSession_accept(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
    filter_multipleMsgInSameSession_accept(tracer_get_default());
    return CLI_SUCCESS;
}
int cli_tracer_filter_multipleMsgInSameSession_reject(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
    filter_multipleMsgInSameSession_reject(tracer_get_default());
    return CLI_SUCCESS;
}



int cli_tracer_set_initial_state_for_tracing(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
 const char * arg1= cli_get_arg_value(cmd, 0);
    unsigned int nummmm;
    if (str_as_uint(arg1, &nummmm)) {
        cli_set_user_error(cli_get(), "invalid num for initial state \"%s\".", arg1);
    return CLI_ERROR_COMMAND_FAILED;
  }
    if( nummmm >= tracer_howManyStatesCurrently(tracer_get_default()) )
    {
        cli_set_user_error(cli_get(), "this state \"%s\" doesn't exists.", arg1);
    return CLI_ERROR_COMMAND_FAILED;
    }
    
    tracer_set_initial_state_for_tracing(tracer_get_default(), nummmm);
   
    return CLI_SUCCESS;
}

int cli_tracer_trace_auto(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  if( tracer_trace_auto(tracer_get_default()) >0 )
    return CLI_SUCCESS;
  return CLI_ERROR_COMMAND_FAILED;
}


int cli_tracer_scheduler_set_type1(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
    tracer_scheduler_set(tracer_get_default(),TRACING_SCHEDULER_1_FIFO);
    return CLI_SUCCESS;
}
int cli_tracer_scheduler_set_type2(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
    tracer_scheduler_set(tracer_get_default(),TRACING_SCHEDULER_2_LIFO);
    return CLI_SUCCESS;
}
int cli_tracer_scheduler_set_type3(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
    tracer_scheduler_set(tracer_get_default(),TRACING_SCHEDULER_3_MIN_MAX_NBMSGBGPSESSION);
    return CLI_SUCCESS;
}
int cli_tracer_scheduler_set_type4(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
    tracer_scheduler_set(tracer_get_default(),TRACING_SCHEDULER_4);
    return CLI_SUCCESS;
}

int cli_tracer_trace_from(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  const char * arg1= cli_get_arg_value(cmd, 0);
  unsigned int state_id;
  const char * arg2= cli_get_arg_value(cmd, 1);
  unsigned int transition_id;

  // Get state_id
  if (str_as_uint(arg1, &state_id)) {
    cli_set_user_error(cli_get(), "invalid state id \"%s\".", arg1);
    return CLI_ERROR_COMMAND_FAILED;
  }
  // Get transition id
  if (str_as_uint(arg2, &transition_id)) {
    cli_set_user_error(cli_get(), "invalid transition id \"%s\".", arg2);
    return CLI_ERROR_COMMAND_FAILED;
  }


  if( tracer_trace_from_state_using_transition(tracer_get_default(), state_id, transition_id ))
    return CLI_ERROR_COMMAND_FAILED;

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
static void _register_tracer_graph_cycle(cli_cmd_t * parent)
{
  cli_cmd_t * group= cli_add_cmd(parent, cli_cmd_group("cycle"));
  cli_cmd_t * cmd= cli_add_cmd(group, cli_cmd("detectAll", cli_tracer_graph_cycle_detectAll));
  //cli_add_arg(cmd, cli_arg("state number", NULL));

}

// -----[ _register_sim_run ]----------------------------------------
static void _register_tracer_graph(cli_cmd_t * parent)
{
  cli_cmd_t * group= cli_add_cmd(parent, cli_cmd_group("graph"));
  _register_tracer_graph_dump(group);
  cli_add_cmd(group, cli_cmd("exportDot", cli_tracer_graph_export_dot));
  cli_add_cmd(group, cli_cmd("export_dot_to_file", cli_tracer_graph_export_dot_to_file));
  cli_add_cmd(group, cli_cmd("export_all_states_to_file", cli_tracer_graph_export_allStates_to_file));
  cli_add_cmd(group, cli_cmd("export_dot_all_states_to_file", cli_tracer_graph_export_dot_allStates_to_file));

  _register_tracer_graph_inject(group);
  _register_tracer_graph_cycle(group);

}


// -----[ _register_sim_run ]----------------------------------------
static void _register_tracer_filter_depth(cli_cmd_t * parent)
{
  cli_cmd_t * group= cli_add_cmd(parent, cli_cmd_group("depth"));
  cli_cmd_t * cmd ;
  cmd = cli_add_cmd(group, cli_cmd("unset", cli_tracer_filter_depth_unset));
  cmd= cli_add_cmd(group, cli_cmd("set", cli_tracer_filter_depth_set));
  cli_add_arg(cmd, cli_arg("depth", NULL));
}

// -----[ _register_sim_run ]----------------------------------------
static void _register_tracer_filter_withdrawMsg(cli_cmd_t * parent)
{
  cli_cmd_t * group= cli_add_cmd(parent, cli_cmd_group("withdraw_msg"));
  cli_cmd_t * cmd ;
  cmd = cli_add_cmd(group, cli_cmd("accept", cli_tracer_filter_withdrawmsg_accept));
  cmd= cli_add_cmd(group, cli_cmd("reject", cli_tracer_filter_withdrawmsg_reject));
  
}

// -----[ _register_sim_run ]----------------------------------------
static void _register_tracer_filter_multipleMsgInSameSession(cli_cmd_t * parent)
{
  cli_cmd_t * group= cli_add_cmd(parent, cli_cmd_group("multiple_msg_in_same_session"));
  cli_cmd_t * cmd ;
  cmd = cli_add_cmd(group, cli_cmd("accept", cli_tracer_filter_multipleMsgInSameSession_accept));
  cmd= cli_add_cmd(group, cli_cmd("reject", cli_tracer_filter_multipleMsgInSameSession_reject));
  
}

// -----[ _register_sim_run ]----------------------------------------
static void _register_tracer_filter_diff_depth(cli_cmd_t * parent)
{
  cli_cmd_t * group= cli_add_cmd(parent, cli_cmd_group("diff_depth"));
  cli_cmd_t * cmd ;
  cmd = cli_add_cmd(group, cli_cmd("unset", cli_tracer_filter_diffDepth_unset));
  cmd= cli_add_cmd(group, cli_cmd("set", cli_tracer_filter_diffDepth_set));
  cli_add_arg(cmd, cli_arg("depth", NULL));
}

// -----[ _register_sim_run ]----------------------------------------
static void _register_tracer_filter_max_transitions(cli_cmd_t * parent)
{
  cli_cmd_t * group= cli_add_cmd(parent, cli_cmd_group("max_transition"));
  cli_cmd_t * cmd ;
  cmd = cli_add_cmd(group, cli_cmd("unset", cli_tracer_filter_maxTransition_unset));
  cmd= cli_add_cmd(group, cli_cmd("set", cli_tracer_filter_maxTransition_set));
  cli_add_arg(cmd, cli_arg("nb transition", NULL));
}

// -----[ _register_sim_run ]----------------------------------------
static void _register_tracer_filter_bgpsessionqueue(cli_cmd_t * parent)
{
  cli_cmd_t * group= cli_add_cmd(parent, cli_cmd_group("bgpsessionqueue"));
  cli_cmd_t * cmd ;
  cmd = cli_add_cmd(group, cli_cmd("unset", cli_tracer_filter_bgpsessionqueue_unset));
  cmd= cli_add_cmd(group, cli_cmd("set", cli_tracer_filter_bgpsessionqueue_set));
  cli_add_arg(cmd, cli_arg("depth", NULL));
}

// -----[ _register_sim_run ]----------------------------------------
static void _register_tracer_filter(cli_cmd_t * parent)
{
  cli_cmd_t * group= cli_add_cmd(parent, cli_cmd_group("filter"));
  //_register_tracer_graph_dump(group);
  /*cli_add_cmd(group, cli_cmd("exportDot", cli_tracer_graph_export_dot));
  cli_add_cmd(group, cli_cmd("export_dot_to_file", cli_tracer_graph_export_dot_to_file));
  cli_add_cmd(group, cli_cmd("export_all_states_to_file", cli_tracer_graph_export_allStates_to_file));
  cli_add_cmd(group, cli_cmd("export_dot_all_states_to_file", cli_tracer_graph_export_dot_allStates_to_file));
  */
  _register_tracer_filter_depth(group);
  _register_tracer_filter_diff_depth(group);
  _register_tracer_filter_bgpsessionqueue(group);
  _register_tracer_filter_max_transitions(group);
  _register_tracer_filter_withdrawMsg(group);
  _register_tracer_filter_multipleMsgInSameSession(group);


}
 static void  _register_tracer_scheduler(cli_cmd_t * parent)
 {
       cli_cmd_t * group= cli_add_cmd(parent, cli_cmd_group("scheduler"));
       cli_cmd_t * cmd;
       cmd = cli_add_cmd(group, cli_cmd("fifo", cli_tracer_scheduler_set_type1));
       cmd = cli_add_cmd(group, cli_cmd("lifo", cli_tracer_scheduler_set_type2));
       cmd = cli_add_cmd(group, cli_cmd("minBgpSession", cli_tracer_scheduler_set_type3));
       cmd = cli_add_cmd(group, cli_cmd("_type4<unimplemented>", cli_tracer_scheduler_set_type4));
 }


// -----[ _register_sim_run ]----------------------------------------
static void _register_tracer_state_dump(cli_cmd_t * parent)
{
  cli_cmd_t * cmd= cli_add_cmd(parent, cli_cmd("dump", cli_tracer_graph_dump_state));
}


// -----[ _register_sim_run ]----------------------------------------
static void _register_tracer_state(cli_cmd_t * parent)
{
  cli_cmd_t * group= cli_add_cmd(parent, cli_cmd_group("state"));
  cli_add_arg(group, cli_arg("state_id", NULL));
  _register_tracer_state_dump(group);

}


// -----[ _register_sim_run ]----------------------------------------
static void _register_tracer_go1step(cli_cmd_t * parent)
{
  cli_add_cmd(parent, cli_cmd("go1step", cli_tracer_go1step));
}

// -----[ _register_sim_run ]----------------------------------------
static void _register_tracer_trace_from(cli_cmd_t * parent)
{
  cli_cmd_t * cmd= cli_add_cmd(parent, cli_cmd("from", cli_tracer_trace_from));
  cli_add_arg(cmd, cli_arg("state_id", NULL));
  cli_add_arg(cmd, cli_arg("transition-id", NULL));
}
// -----[ _register_sim_run ]----------------------------------------
static void _register_tracer_trace(cli_cmd_t * parent)
{
  cli_cmd_t * group= cli_add_cmd(parent, cli_cmd_group("trace"));
  _register_tracer_trace_from(group);
  cli_add_cmd(group, cli_cmd("whole_graph", cli_tracer_trace_whole_graph));
  cli_add_cmd(group, cli_cmd("auto", cli_tracer_trace_auto));
  
}



// ----- cli_register_sim -------------------------------------------
void cli_register_tracer(cli_cmd_t * parent)
{
  cli_cmd_t * group= cli_add_cmd(parent, cli_cmd_group("tracer"));

  _register_tracer_start(group);
  //_register_tracer_dump(group);
  _register_tracer_graph(group);
  _register_tracer_go1step(group);
  _register_tracer_trace(group);
  _register_tracer_filter(group);
  _register_tracer_scheduler(group);

  cli_cmd_t * cmd = cli_add_cmd(group, cli_cmd("set_initial_state",cli_tracer_set_initial_state_for_tracing));
  cli_add_arg(cmd, cli_arg("state_id", NULL));


/*  _register_sim_debug(group);
  _register_sim_event(group);
  _register_sim_options(group);
  _register_sim_queue(group);
  _register_sim_run(group);
  _register_sim_step(group);
  _register_sim_stop(group);
 * */

}
