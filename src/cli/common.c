// ==================================================================
// @(#)common.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 15/07/2003
// @lastdate 01/06/2004
// ==================================================================

#include <string.h>

#include <libgds/log.h>

#include <cli/bgp.h>
#include <cli/common.h>
#include <cli/net.h>
#include <cli/sim.h>
#include <net/prefix.h>
#include <ui/output.h>

static SCli * pTheCli= NULL;

// ----- cli_set_autoflush ------------------------------------------
int cli_set_autoflush(SCliContext * pContext, STokens * pTokens)
{
  char * pcTemp;

  pcTemp= tokens_get_string_at(pTokens, 0);
  if (!strcmp(pcTemp, "on")) {
    iOptionAutoFlush= 1;
  } else if (!strcmp(pcTemp, "off")) {
    iOptionAutoFlush= 0;
  } else {
    LOG_FATAL("Error: invalid value \"%s\" for option \"autoflush\"\n",
	      pcTemp);
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// ----- cli_include ------------------------------------------------
int cli_include(SCliContext * pContext, STokens * pTokens)
{
  FILE * pFile;
  int iResult= CLI_ERROR_COMMAND_FAILED;
  char * pcFileName= tokens_get_string_at(pTokens, 0);

  pFile= fopen(pcFileName, "r");
  if (pFile != NULL) {
    iResult= cli_execute_file(pTheCli, pFile);
    fclose(pFile);
  } else
    LOG_SEVERE("Error: Unable to load file \"%s\".\n", pcFileName);
  return iResult;
}

// ----- cli_print --------------------------------------------------
int cli_print(SCliContext * pContext, STokens * pTokens)
{
  fprintf(stdout, tokens_get_string_at(pTokens, 0));
  
  flushir(stdout);

  return CLI_SUCCESS;
}

// void cli_register_set --------------------------------------------
void cli_register_set(SCli * pCli)
{
  SCliCmds * pSubCmds;
  SCliParams * pParams;

  pParams= cli_params_create();
  cli_params_add(pParams, "<value>", NULL);
  pSubCmds= cli_cmds_create();
  cli_cmds_add(pSubCmds, cli_cmd_create("autoflush", cli_set_autoflush,
					NULL, pParams));
  cli_register_cmd(pCli, cli_cmd_create("set", NULL, pSubCmds, NULL));
}

// ----- cli_register_include ---------------------------------------
void cli_register_include(SCli * pCli)
{
  SCliParams * pParams;

  pParams= cli_params_create();
  cli_params_add(pParams, "<file>", NULL);
  cli_register_cmd(pCli, cli_cmd_create("include", cli_include,
					NULL, pParams));
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

// ----- cli_get ----------------------------------------------------
/**
 *
 */
SCli * cli_get()
{
  if (pTheCli == NULL) {
    pTheCli= cli_create();
    cli_register_bgp(pTheCli);
    cli_register_net(pTheCli);
    cli_register_sim(pTheCli);
    cli_register_include(pTheCli);
    cli_register_print(pTheCli);
    cli_register_set(pTheCli);
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
    return CLI_ERROR_BAD_PARAMETER;
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
    return CLI_ERROR_BAD_PARAMETER;
}

// ----- cli_common_check_uint --------------------------------------
/**
 *
 */
int cli_common_check_uint(char * pcValue)
{
  return CLI_SUCCESS;
}

// ----- cli_common_get_dest ----------------------------------------
int cli_common_get_dest(char * pcPrefix, SPrefix * pPrefix)
{
  char * pcEndChar;

  if (!strcmp(pcPrefix, "*")) {
    pPrefix->uMaskLen= 0;
  } else if (!ip_string_to_prefix(pcPrefix, &pcEndChar, pPrefix) &&
	     (*pcEndChar == 0)) {
  } else if (!ip_string_to_address(pcPrefix, &pcEndChar,
				   &pPrefix->tNetwork) &&
	     (*pcEndChar == 0)) {
    pPrefix->uMaskLen= 32;
  } else {
    return -1;
  }

  return 0;
}


/////////////////////////////////////////////////////////////////////
// INITIALIZATION AND FINALIZATION SECTION
/////////////////////////////////////////////////////////////////////

void _cli_destroy() __attribute__((destructor));

// ----- _cli_destroy -----------------------------------------------
/**
 *
 */
void _cli_destroy()
{
  cli_destroy(&pTheCli);
}
