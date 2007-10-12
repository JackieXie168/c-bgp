// ==================================================================
// @(#)common.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 15/07/2003
// @lastdate 19/07/2007
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include <libgds/gds.h>
#include <libgds/log.h>
#include <libgds/tokenizer.h>

#include <bgp/comm_hash.h>
#include <bgp/mrtd.h>
#include <bgp/path_hash.h>
#include <bgp/predicate_parser.h>
#include <bgp/routes_list.h>
#include <cli/bgp.h>
#include <cli/common.h>
#include <cli/net.h>
#include <cli/sim.h>
#include <net/prefix.h>
#include <net/util.h>
#include <ui/output.h>
#include <ui/rl.h>

#if defined(HAVE_SETRLIMIT) || defined(HAVE_GETRLIMIT)
# include <sys/resource.h>
# ifdef RLIMIT_AS
#  define _RLIMIT_RESOURCE RLIMIT_AS
# else
#  ifdef RLIMIT_VMEM
#   define _RLIMIT_RESOURCE RLIMIT_VMEM
#  else
#   undef HAVE_SETRLIMIT
#   undef HAVE_GETRLIMIT
#  endif
# endif
#endif

static SCli * pTheCli= NULL;

// ----- str2boolean ------------------------------------------------
int str2boolean(const char * pcValue, int * piOptionValue)
{
  if (!strcmp(pcValue, "on")) {
    *piOptionValue= 1;
    return 0;
  } else if (!strcmp(pcValue, "off")) {
    *piOptionValue= 0;
    return 0;
  }
  return -1;
}

// ----- parse_version ----------------------------------------------
int parse_version(char * pcVersion, unsigned int * puVersion)
{
  STokenizer * pTokenizer;
  STokens * pTokens;
  int iResult= 0;
  unsigned int uSubVersion;
  unsigned int uVersion= 0;
  unsigned int uFactor= 1000000;
  unsigned int uIndex;

  pTokenizer= tokenizer_create("-", 1, NULL, NULL);
  if ((tokenizer_run(pTokenizer, pcVersion) != TOKENIZER_SUCCESS) ||
      (tokenizer_get_num_tokens(pTokenizer) < 1)) {
    iResult= -1;
  } else {
    pTokens= tokenizer_get_tokens(pTokenizer);
    pcVersion= strdup(tokens_get_string_at(pTokens, 0));
  }
  tokenizer_destroy(&pTokenizer);
  if (iResult)
    return iResult;

  pTokenizer= tokenizer_create(".", 1, NULL, NULL);
  if (tokenizer_run(pTokenizer, pcVersion) != TOKENIZER_SUCCESS) {
    iResult= -1;
  } else {
    pTokens= tokenizer_get_tokens(pTokenizer);
    for (uIndex= 0; uIndex < tokens_get_num(pTokens); uIndex++) {
      if (tokens_get_uint_at(pTokens, uIndex, &uSubVersion) ||
	  (uSubVersion >= 100)) {
	iResult= -1;
	break;
      }
      uVersion+= uFactor * uSubVersion;
      uFactor/= 100;
    }
    if (!iResult)
      *puVersion= uVersion;
  }
  free(pcVersion);
  tokenizer_destroy(&pTokenizer);
  return iResult;
}

// -----[ cli_net_node_by_addr ]-------------------------------------
SNetNode * cli_net_node_by_addr(char * pcAddr)
{
  net_addr_t tAddr;

  if (str2address(pcAddr, &tAddr))
    return NULL;
  return network_find_node(tAddr);
}

// -----[ cli_params_add_file ]--------------------------------------
int cli_params_add_file(SCliParams * pParams, const char * pcName,
			FCliCheckParam fCheck)
{
#ifdef _FILENAME_COMPLETION_FUNCTION
  return cli_params_add2(pParams, (char *) pcName, fCheck,
			 _FILENAME_COMPLETION_FUNCTION);
#else
  return cli_params_add(pParams, (char *) pcName, fCheck);
#endif
}

// ----- cli_require_version ----------------------------------------
int cli_require_version(SCliContext * pContext, SCliCmd * pCmd)
{
  char * pcVersion;
  unsigned int uRequiredVersion;
  unsigned int uVersion;
  
  // Get required version
  pcVersion= tokens_get_string_at(pCmd->pParamValues, 0);

  if (parse_version(pcVersion, &uRequiredVersion)) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid version \"\".\n", pcVersion);
    return CLI_ERROR_COMMAND_FAILED;
  }
  if (parse_version(PACKAGE_VERSION, &uVersion)) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid version \"\".\n",
	    PACKAGE_VERSION);
    return CLI_ERROR_COMMAND_FAILED;
  }
  if (uRequiredVersion > uVersion) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: version %s > version %s.\n",
	    pcVersion, PACKAGE_VERSION);
    return CLI_ERROR_COMMAND_FAILED;
  }
  
  return CLI_SUCCESS;
}

// ----- cli_set_autoflush ------------------------------------------
int cli_set_autoflush(SCliContext * pContext, SCliCmd * pCmd)
{
  char * pcTemp;

  pcTemp= tokens_get_string_at(pCmd->pParamValues, 0);
  if (str2boolean(pcTemp, &iOptionAutoFlush) != 0) {
    LOG_ERR(LOG_LEVEL_SEVERE,
	    "Error: invalid value \"%s\" for option \"autoflush\"\n",
	    pcTemp);
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// ----- cli_set_exitonerror ----------------------------------------
int cli_set_exitonerror(SCliContext * pContext, SCliCmd * pCmd)
{
  char * pcTemp;

  pcTemp= tokens_get_string_at(pCmd->pParamValues, 0);
  if (str2boolean(pcTemp, &iOptionExitOnError) != 0) {
    LOG_ERR(LOG_LEVEL_SEVERE,
	    "Error: invalid value \"%s\" for option \"exit-on-error\"\n",
	    pcTemp);
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// ----- cli_set_comm_hash_size -------------------------------------
int cli_set_comm_hash_size(SCliContext * pContext, SCliCmd * pCmd)
{
  unsigned long ulSize;

  /* Get the hash size */
  if (tokens_get_ulong_at(pCmd->pParamValues, 0, &ulSize) < 0) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid size \"%s\"\n",
	    tokens_get_string_at(pCmd->pParamValues, 0));
    return CLI_ERROR_COMMAND_FAILED;
  }

  /* Set the hash size */
  if (comm_hash_set_size(ulSize) != 0) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: could not set comm-hash size\n");
    return CLI_ERROR_COMMAND_FAILED;
  }
  
  return CLI_SUCCESS;
}

// -----[ cli_show_comm_hash_content ]-------------------------------
int cli_show_comm_hash_content(SCliContext * pContext, SCliCmd * pCmd)
{
  char * pcFileName;
  SLogStream * pStream= pLogOut;

  pcFileName= tokens_get_string_at(pCmd->pParamValues, 0);
  if (strcmp("stdout", pcFileName)) {
    pStream= log_create_file(pcFileName);
    if (pStream == NULL) {
      LOG_ERR(LOG_LEVEL_SEVERE, "Error: unable to create file \"%s\"\n",
	      pcFileName);
      return CLI_ERROR_COMMAND_FAILED;
    }
  }

  comm_hash_content(pStream);
  if (pStream != pLogOut)
    log_destroy(&pStream);
  return CLI_SUCCESS;
}

// ----- cli_show_comm_hash_size ------------------------------------
int cli_show_comm_hash_size(SCliContext * pContext, SCliCmd * pCmd)
{
  log_printf(pLogOut, "%u\n", comm_hash_get_size());
  return CLI_SUCCESS;
}

// -----[ cli_show_comm_hash_stat ]----------------------------------
int cli_show_comm_hash_stat(SCliContext * pContext, SCliCmd * pCmd)
{
  char * pcFileName;
  SLogStream * pStream= pLogOut;

  pcFileName= tokens_get_string_at(pCmd->pParamValues, 0);
  if (strcmp("stdout", pcFileName)) {
    pStream= log_create_file(pcFileName);
    if (pStream == NULL) {
      LOG_ERR(LOG_LEVEL_SEVERE, "Error: unable to create file \"%s\"\n",
	      pcFileName);
      return CLI_ERROR_COMMAND_FAILED;
    }
  }

  comm_hash_statistics(pStream);
  if (pStream != pLogOut)
    log_destroy(&pStream);
  return CLI_SUCCESS;
}

// -----[ cli_show_commands ]----------------------------------------
int cli_show_commands(SCliContext * pContext, SCliCmd * pCmd)
{
  cli_cmd_dump(pLogOut, "  ", pTheCli->pBaseCommand);
  return CLI_SUCCESS;
}

// ----- cli_set_debug ----------------------------------------------
int cli_set_debug(SCliContext * pContext, SCliCmd * pCmd)
{
  char * pcTemp;

  pcTemp= tokens_get_string_at(pCmd->pParamValues, 0);
  if (str2boolean(pcTemp, &iOptionDebug) != 0) {
    LOG_ERR(LOG_LEVEL_SEVERE,
	    "Error: invalid value \"%s\" for option \"debug\"\n",
	    pcTemp);
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// -----[ cli_show_mrt ]---------------------------------------------
/**
 * context: {}
 * tokens: {filename, predicate}
 */
int cli_show_mrt(SCliContext * pContext, SCliCmd * pCmd)
{
#ifdef HAVE_BGPDUMP
  /*
  char * pcPredicate;
  SFilterMatcher * pMatcher;

  // Parse predicate
  pcPredicate= tokens_get_string_at(pCmd->pParamValues, 1);
  if (predicate_parse(&pcPredicate, &pMatcher)) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid predicate \"%s\"\n",
	    pcPredicate);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Dump routes that match the given predicate
  mrtd_load_routes(tokens_get_string_at(pCmd->pParamValues, 0), 1, pMatcher);
*/
  return CLI_SUCCESS;
#else
  LOG_ERR(LOG_LEVEL_SEVERE, "Error: compiled without bgpdump.\n");
  return CLI_ERROR_COMMAND_FAILED;
#endif
}

// ----- cli_show_mem_limit -----------------------------------------
int cli_show_mem_limit(SCliContext * pContext, SCliCmd * pCmd)
{
#ifdef HAVE_GETRLIMIT
  struct rlimit rlim;

  if (getrlimit(_RLIMIT_RESOURCE, &rlim) < 0) {
    log_perror(pLogErr, "Error: getrlimit, ");
    return CLI_ERROR_COMMAND_FAILED;
  }

  log_printf(pLogOut, "soft limit: ");
  if (rlim.rlim_cur == RLIM_INFINITY) {
    log_printf(pLogOut, "unlimited\n");
  } else {
    log_printf(pLogOut, "%u byte(s)\n", (unsigned int) rlim.rlim_cur);
  }
  log_printf(pLogOut, "hard limit: ");
  if (rlim.rlim_max == RLIM_INFINITY) {
    log_printf(pLogOut, "unlimited\n");
  } else {
    log_printf(pLogOut, "%u byte(s)\n", (unsigned int) rlim.rlim_max);
  }

  log_flush(pLogOut);

  return CLI_SUCCESS;
#else
  LOG_ERR(LOG_LEVEL_SEVERE,
	  "Error: getrlimit() is not supported by your system\n");
  return CLI_ERROR_COMMAND_FAILED;
#endif
}

// ----- cli_set_mem_limit ------------------------------------------
int cli_set_mem_limit(SCliContext * pContext, SCliCmd * pCmd)
{
#if defined(HAVE_SETRLIMIT) && defined(HAVE_GETRLIMIT)
  unsigned long ulLimit;
  rlim_t tLimit;
  struct rlimit rlim;

  /* Get the value of the memory-limit */
  if (tokens_get_ulong_at(pCmd->pParamValues, 0, &ulLimit) < 0) {
    if (!strcmp(tokens_get_string_at(pCmd->pParamValues, 0), "unlimited")) {
      tLimit= RLIM_INFINITY;
    } else {
      LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid mem limit \"%s\"\n",
	      tokens_get_string_at(pCmd->pParamValues, 0));
      return CLI_ERROR_COMMAND_FAILED;
    }
  } else {
    if (sizeof(tLimit) < sizeof(ulLimit)) {
      LOG_ERR(LOG_LEVEL_WARNING,
	      "Warning: limit may be larger than supported by system.\n");
    }
    tLimit= (rlim_t) ulLimit;
  }

  /* Get the soft limit on the process's size of virtual memory */
  if (getrlimit(_RLIMIT_RESOURCE, &rlim) < 0) {
    log_perror(pLogErr, "Error: getrlimit, ");
    return CLI_ERROR_COMMAND_FAILED;
  }

  rlim.rlim_cur= tLimit;

  /* Set new soft limit on the process's size of virtual memory */
  if (setrlimit(_RLIMIT_RESOURCE, &rlim) < 0) {
    log_perror(pLogErr, "Error: setrlimit, ");
    return CLI_ERROR_COMMAND_FAILED;
  }
  
  return CLI_SUCCESS;
#else
  LOG_ERR(LOG_LEVEL_SEVERE,
	  "Error: setrlimit() is not supported by your system.\n");
  return CLI_ERROR_COMMAND_FAILED;
#endif
}

// ----- cli_set_path_hash_size -------------------------------------
int cli_set_path_hash_size(SCliContext * pContext, SCliCmd * pCmd)
{
  unsigned long ulSize;

  /* Get the hash size */
  if (tokens_get_ulong_at(pCmd->pParamValues, 0, &ulSize) < 0) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid size \"%s\"\n",
	    tokens_get_string_at(pCmd->pParamValues, 0));
    return CLI_ERROR_COMMAND_FAILED;
  }

  /* Set the hash size */
  if (path_hash_set_size(ulSize) != 0) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: could not set path-hash size\n");
    return CLI_ERROR_COMMAND_FAILED;
  }
  
  return CLI_SUCCESS;
}

// -----[ cli_show_path_hash_content ]-------------------------------
int cli_show_path_hash_content(SCliContext * pContext, SCliCmd * pCmd)
{
  char * pcFileName;
  SLogStream * pStream= pLogOut;

  pcFileName= tokens_get_string_at(pCmd->pParamValues, 0);
  if (strcmp("stdout", pcFileName)) {
    pStream= log_create_file(pcFileName);
    if (pStream == NULL) {
      LOG_ERR(LOG_LEVEL_SEVERE, "Error: unable to create file \"%s\"\n",
	      pcFileName);
      return CLI_ERROR_COMMAND_FAILED;
    }
  }

  path_hash_content(pStream);
  if (pStream != pLogOut)
    log_destroy(&pStream);
  return CLI_SUCCESS;
}

// ----- cli_show_path_hash_size ------------------------------------
int cli_show_path_hash_size(SCliContext * pContext, SCliCmd * pCmd)
{
  log_printf(pLogOut, "%u\n", path_hash_get_size());
  return CLI_SUCCESS;
}

// -----[ cli_show_path_hash_stat ]----------------------------------
int cli_show_path_hash_stat(SCliContext * pContext, SCliCmd * pCmd)
{
  char * pcFileName;
  SLogStream * pStream= pLogOut;

  pcFileName= tokens_get_string_at(pCmd->pParamValues, 0);
  if (strcmp("stdout", pcFileName)) {
    pStream= log_create_file(pcFileName);
    if (pStream == NULL) {
      LOG_ERR(LOG_LEVEL_SEVERE, "Error: unable to create file \"%s\"\n",
	      pcFileName);
      return CLI_ERROR_COMMAND_FAILED;
    }
  }

  path_hash_statistics(pStream);
  if (pStream != pLogOut)
    log_destroy(&pStream);
  return CLI_SUCCESS;
}

// ----- cli_show_version -------------------------------------------
int cli_show_version(SCliContext * pContext, SCliCmd * pCmd)
{
  log_printf(pLogOut, "%s version: %s", PACKAGE_NAME, PACKAGE_VERSION);
#ifdef __EXPERIMENTAL__ 
  log_printf(pLogOut, " [experimental]");
#endif
#ifdef HAVE_LIBZ
  log_printf(pLogOut, " [zlib]");
#endif
#ifdef HAVE_JNI
  log_printf(pLogOut, " [jni]");
#endif
#ifdef HAVE_BGPDUMP
  log_printf(pLogOut, " [bgpdump]");
#endif
#ifdef __ROUTER_LIST_ENABLE__
  log_printf(pLogOut, " [router-list]");
#endif
#ifdef __EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
  log_printf(pLogOut, " [external-best]");
#endif
  log_printf(pLogOut, "\n");

  log_printf(pLogOut, "libgds version: %s\n", gds_version());

  log_flush(pLogOut);

  return CLI_SUCCESS;
}

// ----- cli_include ------------------------------------------------
int cli_include(SCliContext * pContext, SCliCmd * pCmd)
{
  FILE * pFile;
  int iResult= CLI_ERROR_COMMAND_FAILED;
  char * pcFileName= tokens_get_string_at(pCmd->pParamValues, 0);

  pFile= fopen(pcFileName, "r");
  if (pFile != NULL) {
    iResult= cli_execute_file(pTheCli, pFile);
    fclose(pFile);
  } else
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: Unable to load file \"%s\".\n",
	    pcFileName);
  return iResult;
}

// ----- cli_pause --------------------------------------------------
int cli_pause(SCliContext * pContext, SCliCmd * pCmd)
{
  log_printf(pLogOut, "Paused: hit 'Enter' to continue...");
  log_flush(pLogOut);
  fgetc(stdin);
  log_printf(pLogOut, "\n");

  return CLI_SUCCESS;
}

// ----- cli_print --------------------------------------------------
int cli_print(SCliContext * pContext, SCliCmd * pCmd)
{
  log_printf(pLogOut, tokens_get_string_at(pCmd->pParamValues, 0));
  
  log_flush(pLogOut);

  return CLI_SUCCESS;
}

// ----- cli_quit ---------------------------------------------------
int cli_quit(SCliContext * pContext, SCliCmd * pCmd)
{
  return CLI_SUCCESS_TERMINATE;
}

// -----[ cli_time_diff ]--------------------------------------------
static time_t tSavedTime= 0;
int cli_time_diff(SCliContext * pContext, SCliCmd * pCmd)
{
  time_t tCurrentTime= time(NULL);

  if (tSavedTime == 0) {
    return CLI_ERROR_COMMAND_FAILED;
  }

  log_printf(pLogOut, "%f\n", difftime(tCurrentTime, tSavedTime));
  return CLI_SUCCESS;
}

// -----[ cli_time_save ]--------------------------------------------
int cli_time_save(SCliContext * pContext, SCliCmd * pCmd)
{
  tSavedTime= time(NULL);
  return CLI_SUCCESS;
}

// void cli_register_set --------------------------------------------
void cli_register_set(SCli * pCli)
{
  SCliCmds * pSubCmds;
  SCliParams * pParams;

  pSubCmds= cli_cmds_create();
  pParams= cli_params_create();
  cli_params_add(pParams, "<on|off>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("autoflush", cli_set_autoflush,
					NULL, pParams));
  pParams= cli_params_create();
  cli_params_add(pParams, "<size>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("comm-hash-size",
					cli_set_comm_hash_size,
					NULL, pParams));
  pParams= cli_params_create();
  cli_params_add(pParams, "<on|off>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("debug", cli_set_debug,
					NULL, pParams));
  pParams= cli_params_create();
  cli_params_add(pParams, "<on|off>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("exit-on-error", cli_set_exitonerror,
					NULL, pParams));
  pParams= cli_params_create();
  cli_params_add(pParams, "<value>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("mem-limit", cli_set_mem_limit,
					NULL, pParams));
  pParams= cli_params_create();
  cli_params_add(pParams, "<size>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("path-hash-size",
					cli_set_path_hash_size,
					NULL, pParams));
//  pParams= cli_params_create();
//  cli_params_add(pParams, "<time>", NULL);
//  cli_cmds_add(pSubCmds, cli_cmd_create("time-limit", cli_set_time_limit,
//					NULL, pParams));
  cli_register_cmd(pCli, cli_cmd_create("set", NULL, pSubCmds, NULL));
}

// void cli_register_show -------------------------------------------
void cli_register_show(SCli * pCli)
{
  SCliCmds * pSubCmds;
  SCliParams * pParams;

  pSubCmds= cli_cmds_create();
  pParams= cli_params_create();
  cli_params_add_file(pParams, "<filename>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("comm-hash-content",
					cli_show_comm_hash_content,
					NULL, pParams));
  cli_cmds_add(pSubCmds, cli_cmd_create("comm-hash-size",
					cli_show_comm_hash_size,
					NULL, NULL));
  pParams= cli_params_create();
  cli_params_add_file(pParams, "<filename>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("comm-hash-stat",
					cli_show_comm_hash_stat,
					NULL, pParams));
  cli_cmds_add(pSubCmds, cli_cmd_create("commands",
					cli_show_commands,
					NULL, NULL));  
  pParams= cli_params_create();
  cli_params_add_file(pParams, "<filename>", NULL);
  cli_params_add(pParams, "<predicate>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("mrt", cli_show_mrt,
					NULL, pParams));
  cli_cmds_add(pSubCmds, cli_cmd_create("mem-limit", cli_show_mem_limit,
					NULL, NULL));
//  cli_cmds_add(pSubCmds, cli_cmd_create("mem-limit", cli_show_time_limit,
//					NULL, NULL));
  pParams= cli_params_create();
  cli_params_add_file(pParams, "<filename>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("path-hash-content",
					cli_show_path_hash_content,
					NULL, pParams));
  cli_cmds_add(pSubCmds, cli_cmd_create("path-hash-size",
					cli_show_path_hash_size,
					NULL, NULL));
  pParams= cli_params_create();
  cli_params_add_file(pParams, "<filename>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("path-hash-stat",
					cli_show_path_hash_stat,
					NULL, pParams));
  cli_cmds_add(pSubCmds, cli_cmd_create("version", cli_show_version,
					NULL, NULL));
  cli_register_cmd(pCli, cli_cmd_create("show", NULL, pSubCmds, NULL));
}

// ----- cli_register_include ---------------------------------------
void cli_register_include(SCli * pCli)
{
  SCliParams * pParams= cli_params_create();

  cli_params_add_file(pParams, "<file>", NULL);
  cli_register_cmd(pCli, cli_cmd_create("include", cli_include,
					NULL, pParams));
}

// ----- cli_register_pause -----------------------------------------
void cli_register_pause(SCli * pCli)
{
  cli_register_cmd(pCli, cli_cmd_create("pause", cli_pause,
					NULL, NULL));
}

// ----- cli_register_print -----------------------------------------
void cli_register_print(SCli * pCli)
{
  SCliParams * pParams;

  pParams= cli_params_create();
  cli_params_add(pParams, "<message>", NULL);
  cli_register_cmd(pCli, cli_cmd_create("print", cli_print,
					NULL, pParams));
}

// ----- cli_register_quit ------------------------------------------
void cli_register_quit(SCli * pCli)
{
  cli_register_cmd(pCli, cli_cmd_create("quit", cli_quit,
					NULL, NULL));
}

// ----- cli_register_require ---------------------------------------
void cli_register_require(SCli * pCli)
{
  SCliCmds * pSubCmds;
  SCliParams * pParams;

  pSubCmds= cli_cmds_create();
  pParams= cli_params_create();
  cli_params_add(pParams, "<version>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("version", cli_require_version,
					NULL, pParams));
  cli_register_cmd(pCli, cli_cmd_create("require", cli_quit,
					pSubCmds, NULL));
}

// ----- cli_register_time ------------------------------------------
void cli_register_time(SCli * pCli)
{
  SCliCmds * pSubCmds;

  pSubCmds= cli_cmds_create();
  cli_cmds_add(pSubCmds, cli_cmd_create("diff", cli_time_diff,
					NULL, NULL));
  cli_cmds_add(pSubCmds, cli_cmd_create("save", cli_time_save,
					NULL, NULL));
  cli_register_cmd(pCli, cli_cmd_create("time", cli_quit,
					pSubCmds, NULL));
}

// ----- cli_exit_on_error ------------------------------------------
/**
 *
 */
static int _cli_exit_on_error(int iResult)
{
  return (iOptionExitOnError?iResult:CLI_SUCCESS);
}

// ----- cli_get ----------------------------------------------------
/**
 *
 */
SCli * cli_get()
{
  if (pTheCli == NULL) {
    pTheCli= cli_create();
    cli_set_exit_callback(pTheCli, _cli_exit_on_error);

    /* Command classes */
    cli_register_bgp(pTheCli);
    cli_register_net(pTheCli);
    cli_register_sim(pTheCli);

    /* Miscelaneous commands */
    cli_register_include(pTheCli);
    cli_register_pause(pTheCli);
    cli_register_print(pTheCli);
    cli_register_quit(pTheCli);
    cli_register_require(pTheCli);
    cli_register_set(pTheCli);
    cli_register_show(pTheCli);
    cli_register_time(pTheCli);
  }
  return pTheCli;
}

// ----- cli_common_check_addr --------------------------------------
/**
 *
 */
int cli_common_check_addr(char * pcValue)
{
  net_addr_t tAddr;
  char * pcEndPtr;

  if (!ip_string_to_address(pcValue, &pcEndPtr, &tAddr) && (*pcEndPtr == 0))
    return CLI_SUCCESS;
  else
    return CLI_ERROR_BAD_PARAM;
}

// ----- cli_common_check_prefix ------------------------------------
/**
 *
 */
int cli_common_check_prefix(char * pcValue)
{
  SPrefix sPrefix;
  char * pcEndPtr;

  if (!ip_string_to_prefix(pcValue, &pcEndPtr, &sPrefix) && (*pcEndPtr == 0))
    return CLI_SUCCESS;
  else
    return CLI_ERROR_BAD_PARAM;
}

// ----- cli_common_check_uint --------------------------------------
/**
 *
 */
int cli_common_check_uint(char * pcValue)
{
  return CLI_SUCCESS;
}

/////////////////////////////////////////////////////////////////////
// INITIALIZATION AND FINALIZATION SECTION
/////////////////////////////////////////////////////////////////////

// ----- _cli_common_destroy ----------------------------------------
/**
 *
 */
void _cli_common_destroy()
{
  cli_destroy(&pTheCli);
}
