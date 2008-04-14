// ==================================================================
// @(#)cisco.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 15/05/2007
// $Id: cisco.c,v 1.2 2008-04-14 09:14:35 bqu Exp $
// ==================================================================

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libgds/tokenizer.h>
#include <libgds/tokens.h>

#include <bgp/cisco.h>

// ----- Example input file -----------------------------------------
// BGP table version is 253386, local router ID is 198.32.162.100
// Status codes: s suppressed, d damped, h history, * valid, > best, i - internal
// Origin codes: i - IGP, e - EGP, ? - incomplete
// 
//    Network          Next Hop          Metric LocPrf Weight Path
// *  3.0.0.0          206.157.77.11        165             0 1673 701 80 ?
// *                   144.228.240.93         4             0 1239 701 80 ?
// *                   205.238.48.3                         0 2914 1280 701 80 ?
// ------------------------------------------------------------------

// ----- Parser states -----
#define CISCO_STATE_HEADER  0
#define CISCO_STATE_RECORDS 1

// -----[ cisco_perror ]---------------------------------------------
void cisco_perror(SLogStream * pStream, int iErrorCode)
{
#define LOG(M) log_printf(pStream, M); break;
  switch (iErrorCode) {
  case CISCO_SUCCESS:
    LOG("success");
  case CISCO_ERROR_UNEXPECTED:
    LOG("unexpected error");
  case CISCO_ERROR_FILE_OPEN:
    LOG("unable to open file");
  case CISCO_ERROR_NUM_FIELDS:
    LOG("incorrect number of fields");
  default:
    log_printf(pStream, "unknown error (%i)", iErrorCode);
  }
#undef LOG
}

// -----[ cisco_record_parser ]--------------------------------------
int cisco_record_parser(const char * pcLine)
{
  // First field
  switch (pcLine[0]) {
  case '*':
    break;
  case ' ':
    break;
  default:
    return CISCO_ERROR_UNEXPECTED;
  }

  // Second field
  switch (pcLine[1]) {
  case '>':
    break;
  case ' ':
    break;
  default:
    return CISCO_ERROR_UNEXPECTED;
  }

  // Space separator
  if (pcLine[2] != ' ')
    return CISCO_ERROR_UNEXPECTED;

  // Get "Network" field (length=19)

  // Get "Next Hop" field (length=15)
  
  // Get "Metric" field (length=?)

  // Get "LocPrf" field (length=?)

  // Get "Weight" field (length=?)

  // Get "Path" field (length=variable)

  // Get "Origin" field

  return CISCO_SUCCESS;
}

// -----[ cisco_parser ]---------------------------------------------
int cisco_parser(FILE * pStream)
{
  STokenizer * pTokenizer;
  STokens * pTokens;
  char acFileLine[80];
  unsigned int uLineNumber= 0;
  int iError= CISCO_SUCCESS;

  pTokenizer= tokenizer_create(" \t", 0, NULL, NULL);

  while ((!feof(pStream)) && (!iError)) {
    if (fgets(acFileLine, sizeof(acFileLine), pStream) == NULL)
      break;
    uLineNumber++;

    // Skip comments starting with '#'
    if (acFileLine[0] == '#')
      continue;

    
    pTokens= tokenizer_get_tokens(pTokenizer);

  }
  tokenizer_destroy(&pTokenizer);

  return iError;
}

// -----[ cisco_load ]-----------------------------------------------
int cisco_load(const char * pcFileName)
{
  FILE * pFile;
  int iResult;

  pFile= fopen(pcFileName, "r");
  if (pFile == NULL)
    return -1;

  iResult= cisco_parser(pFile);

  fclose(pFile);
  return iResult;
}
