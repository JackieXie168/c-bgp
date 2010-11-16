// ==================================================================
// @(#)help.c
//
// Provides functions to obtain help from the CLI in interactive mode.
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 22/11/2002
// $Id: help.c,v 1.7 2009-03-24 16:29:41 bqu Exp $
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <libgds/cli_commands.h>
#include <libgds/cli_ctx.h>
#include <libgds/cli_params.h>
#include <libgds/str_util.h>

#include <api.h>
#include <cli/common.h>
#include <ui/pager.h>

#ifdef HAVE_LIBREADLINE
# ifdef HAVE_READLINE_READLINE_H
#  include <readline/readline.h>
# endif
# include <ui/rl.h>
#endif

GDS_ARRAY_TEMPLATE(str_array, char *, 0, NULL, NULL, NULL)
static str_array_t * _help_paths= NULL;

// -----[ cli_cmd_get_path ]-----------------------------------------
/**
 * Return the complete command path for the given command.
 */
char * cli_cmd_get_path(cli_cmd_t * cmd)
{
  char * path= NULL;

  while (cmd != NULL) {
    if (strcmp(cmd->name, "")) {
      if (path == NULL)
	path= str_create(cmd->name);
      else
	path= str_prepend(str_prepend(path, "_"), cmd->name);
    }
    cmd= cmd->parent;
  }
  return path;
}

// -----[ cli_help_file ]--------------------------------------------
int cli_help_file(const char * cmd, char * buf, size_t buf_len)
{
  
  return -1;
}

// -----[ _cli_help_cmd_str ]----------------------------------------
/*static int _cli_help_cmd_str(const char * str)
{
  char * cmd_filename= str_create(str);
  char * filename;
  int result;
  unsigned int index;
  char * r_filename;

  str_translate(cmd_filename, " ", "_");
  cmd_filename= str_append(cmd_filename, ".txt");

  for (index= 0; index < str_array_size(_help_paths); index++) {
    filename= str_create(_help_paths->data[index]);
    filename= str_append(filename, cmd_filename);
    result= params_replace(filename, libcbgp_get_param_lookup(),
			   &r_filename, PARAM_OPT_ALLOW_UNDEF);
    assert(result == 0);
    result= pager_run(r_filename);
    if (result != PAGER_SUCCESS)
      stream_printf(gdserr, "Trying %s... [failed]\n", filename);
    FREE(r_filename);
    str_destroy(&filename);
    if (result == PAGER_SUCCESS)
      break;
  }

  str_destroy(&cmd_filename);
  return result;
  }*/

// -----[ _cli_help_cmd ]--------------------------------------------
/**
 * Display help for a context: gives the available sub-commands and
 * their parameters.
 */
#if defined(HAVE_LIBREADLINE) && defined(HAVE_READLINE_READLINE_H)
/*static void _cli_help_cmd(cli_cmd_t * cmd)
{
  char * cmd_filename= cli_cmd_get_path(cmd);

  fprintf(stdout, "\n");
  _cli_help_cmd_str(cmd_filename);
  str_destroy(&cmd_filename);
#ifdef HAVE_RL_ON_NEW_LINE
  rl_on_new_line();
#endif  
  return;
}*/
#endif /* HAVE_LIBREADLINE */

// -----[ _cli_help_option ]-----------------------------------------
#if defined(HAVE_LIBREADLINE) && defined(HAVE_READLINE_READLINE_H)
/*static void _cli_help_option(cli_opt_t * option)
{
  fprintf(stdout, "\n");
  if (option->info == NULL)
    fprintf(stdout, "No help available for option \"--%s\"\n",
	    option->name);
  else
    fprintf(stdout, "%s\n", option->info);

#ifdef HAVE_RL_ON_NEW_LINE
  rl_on_new_line();
#endif
}*/
#endif /* HAVE_LIBREADLINE */

// -----[ _cli_help_param ]------------------------------------------
#if defined(HAVE_LIBREADLINE) && defined(HAVE_READLINE_READLINE_H)
 /*static void _cli_help_param(cli_arg_t * arg)
{
  fprintf(stdout, "\n");
  if (arg->info == NULL)
    fprintf(stdout, "No help available for parameter \"%s\"\n",
	    arg->name);
  else
    fprintf(stdout, "%s\n", arg->info);

#ifdef HAVE_RL_ON_NEW_LINE
  rl_on_new_line();
#endif
}*/
#endif /* HAVE_LIBREADLINE */

// -----[ cli_help ]-------------------------------------------------
void cli_help(const char * str)
{
  unsigned int index;
  cli_t * cli= cli_get();
  cli_cmd_t * ctx;
  cli_cmd_t * cmd;
  gds_stream_t * os= stream_create_proc("less");
  assert(os != NULL);

  // Get the current command context.
  cli_get_cmd_context(cli, &ctx, NULL);

  if (str == NULL) {
    cli_cmds_t * cmds= cli_matching_subcmds(cli, ctx, "", 1);
    if (cmds == NULL) {
      stream_printf(os, 
		    "\n\n"
		    "Sorry, I fear I can't do much to help you.\n"
		    "Seems you're kinda stuck in a dead end.\n"
		    "How can it be ? Dunno, ask the author...\n\n");
    } else {
      stream_printf(os,
		    "\n\n"
		    "The following commands are available in this context.\n"
		    "Try \"help <cmd-name>\" to learn more about a specific"
		    " command.\n\n");
      for (index= 0; index < cli_cmds_num(cmds); index++) {
	cmd= cli_cmds_at(cmds, index);
	stream_printf(os, "\t%s\n", cmd->name);
      }
      stream_printf(os, "\n");
      cli_cmds_destroy(&cmds);
    }
  } else {

  }

  stream_destroy(&os);

#ifdef HAVE_RL_ON_NEW_LINE
  // rl_on_new_line();
#endif

  return;

  /*  // Try to match it with the CLI command tree. On success, retrieve
  // the current command and parameter index. THIS CURRENTLY DOES NOT
  // WORK WITH OPTIONS !!!
  if (cmd != NULL)
    status= cli_cmd_match(cli, cmd_ctx, cmd, NULL, &ctx);
  else {
    status= CLI_MATCH_COMMAND;
    ctx= cmd_ctx;
  }

  switch (status) {
  case CLI_MATCH_NOTHING:
    fprintf(stdout, "\n");
    fprintf(stdout, "Sorry, no help is available on this topic\n");
    break;
  case CLI_MATCH_COMMAND:
    _cli_help_cmd((cli_cmd_t *) ctx);
    break;
  case CLI_MATCH_OPTION_VALUE:
    _cli_help_option((cli_opt_t *) ctx);
    break;
  case CLI_MATCH_PARAM_VALUE:
    _cli_help_param((cli_arg_t *) ctx);
    break;
  default: abort();
  }*/

}

// -----[ _rl_help ]-------------------------------------------------
/**
 * This function provides help through the 'readline' subsystem.
 * The help is linked to key '?'. The help is provided at the point
 * where the '?' is typed. For example,
 *
 * "bgp router?"
 *   will provide help on the "bgp router" command
 *
 * "bgp router ?"
 *   will provide help on the first parameter of the "bgp router"
 *   command
 *
 * "bgp add? router"
 *   will provide help on the "bgp add" command, not on the full
 *   "bgp add router" command.
 */
#if defined(HAVE_LIBREADLINE) && defined(HAVE_READLINE_READLINE_H)
static int _rl_help(int count, int key)
{
  char * cmd= NULL;
  int pos= rl_point;

  // Retrieve the content of the readline buffer, from the first
  // character to the first occurence of '?'. On this basis, try to
  // match the command for which help was requested.
  if (pos > 0) {

    // This is the command line prefix
    cmd= (char *) malloc((pos+1)*sizeof(char));
    strncpy(cmd, rl_line_buffer, pos);
    cmd[pos]= '\0';

    cli_help(cmd);

    free(cmd);
  } else {
    cli_help(NULL);
  }
  return 0;
}
#endif /* HAVE_LIBREADLINE */

// -----[ _cli_help ]------------------------------------------------
/**
 * This function provides help through the 'cli' subsystem.
 */
static int _cli_help(cli_ctx_t * ctx, struct cli_cmd_t * cmd)
{
  const char * arg= cli_get_arg_value(cmd, 0);
  cli_help(arg);
  return CLI_SUCCESS;
}


/////////////////////////////////////////////////////////////////////
// INITIALIZATION AND FINALIZATION SECTION
/////////////////////////////////////////////////////////////////////

// -----[ _help_init ]-----------------------------------------------
void _help_init()
{
  cli_cmd_t * cmd;

  // Initialize possible help paths
  _help_paths= str_array_create(0);
  str_array_add(_help_paths, "$DOC_PATH/txt/");
  str_array_add(_help_paths, "$EXEC_PATH/../share/doc/cbgp/txt/");
  str_array_add(_help_paths, str_append(str_create(DOCDIR), "/txt/"));

  // Initialize omnipresent CLI "help" command
  cmd= cli_add_cmd(cli_get_omni_cmd(cli_get()),
		   cli_cmd("help", _cli_help));
  cli_add_arg(cmd, cli_arg("cmd", NULL));

  // Initialize 'readline' help key (if available)
#if defined(HAVE_LIBREADLINE) && defined(HAVE_READLINE_READLINE_H)
  if (rl_bind_key('?', _rl_help) != 0)
    fprintf(stderr, "Error: could not bind '?' key to help.\n");
  else
    fprintf(stderr, "help is bound to '?' key\n");
#ifdef HAVE_RL_ON_NEW_LINE
  rl_on_new_line();
#endif /* HAVE_RL_ON_NEW_LINE */
#endif /* HAVE_LIBREADLINE */
}

// -----[ _help_destroy ]--------------------------------------------
void _help_destroy()
{
  str_array_destroy(&_help_paths);
}
