// ==================================================================
// @(#)main.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 28/01/2005
// @lastdate 31/01/2005
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <cli/common.h>
#include <sim/simulator.h>

/////////////////////////////////////////////////////////////////////
// CHECKING MACROS
/////////////////////////////////////////////////////////////////////

#define MSG_CHECKING(MSG) \
  printf("\033[37;1m%s\033[0m", MSG)
#define MSG_RESULT_SUCCESS() \
  printf("\033[70G[\033[32;1mSUCCESS\033[0m]\n")
#define MSG_RESULT_FAIL() \
  printf("\033[70G[\033[31;1mFAIL\033[0m]\n")
#define ASSERT_RETURN(TEST, INFO) \
  if (!(TEST)) { \
    MSG_RESULT_FAIL(); \
    printf("  Reason: [%s]\n", INFO); \
    return -1; \
  }

/////////////////////////////////////////////////////////////////////
// DECISION PROCESS TEST
/////////////////////////////////////////////////////////////////////
int check_decision_process()
{
  MSG_CHECKING("* Decision process");
  MSG_RESULT_FAIL();

  return -1;
}

/////////////////////////////////////////////////////////////////////
// VARIOUS SCRIPTS
////////////////////////////////////////////////////////////////////
int check_scripts()
{
  FILE * pInCli;

  log_set_level(pMainLog, LOG_LEVEL_FATAL);
  log_set_stream(pMainLog, stderr);

  MSG_CHECKING("* Script 1");
  simulator_init();
  pInCli= fopen("valid/valid-igp-change.cli", "r");
  if (pInCli != NULL) {
    ASSERT_RETURN(cli_execute_file(cli_get(), pInCli) == CLI_SUCCESS,
		  "script execution failed");
    fclose(pInCli);
  }
  simulator_done();
  MSG_RESULT_SUCCESS();

  return 0;
}

/////////////////////////////////////////////////////////////////////
// MAIN PART
/////////////////////////////////////////////////////////////////////

// -----[ main ]-----------------------------------------------------
int main(int argc, char * argv[])
{
  //check_decision_process();

  check_scripts();

  return 0;
}
