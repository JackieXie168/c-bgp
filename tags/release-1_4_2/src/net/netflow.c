// ==================================================================
// @(#)netflow.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 14/05/2007
// @lastdate 16/05/2007
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <string.h>

#include <libgds/log.h>
#include <libgds/tokenizer.h>
#include <libgds/tokens.h>

#include <net/netflow.h>
#include <net/prefix.h>
#include <net/util.h>

// ----- Parser's states -----
#define NETFLOW_STATE_HEADER  0
#define NETFLOW_STATE_RECORDS 1

#ifdef HAVE_LIBZ
#include <zlib.h>
typedef gzFile FILE_TYPE;
#define FILE_OPEN(N,A) gzopen(N, A)
#define FILE_CLOSE(F) gzclose(F)
#define FILE_GETS(F,B,L) gzgets(F, B, L)
#define FILE_EOF(F) gzeof(F)
#else
typedef FILE FILE_TYPE;
#define FILE_OPEN(N,A) fopen(N, A)
#define FILE_CLOSE(F) fclose(F)
#define FILE_GETS(F,B,L) fgets(B,L,F)
#define FILE_EOF(F) feof(F)
#endif


// -----[ netflow_perror ]-----------------------------------------
/**
 *
 */
void netflow_perror(SLogStream * pStream, int iErrorCode)
{
#define LOG(M) log_printf(pStream, M); return;
  switch (iErrorCode) {
  case NETFLOW_SUCCESS:
    LOG("success");
  case NETFLOW_ERROR_UNEXPECTED:
    LOG("unexpected error");
  case NETFLOW_ERROR_OPEN:
    LOG("unable to open file");
  case NETFLOW_ERROR_NUM_FIELDS:
    LOG("incorrect number of fields");
  case NETFLOW_ERROR_INVALID_HEADER:
    LOG("invalid (flow-print) header");
  case NETFLOW_ERROR_INVALID_SRC_ADDR:
    LOG("invalid source address");
  case NETFLOW_ERROR_INVALID_DST_ADDR:
    LOG("invalid destination address");
  case NETFLOW_ERROR_INVALID_OCTETS:
    LOG("invalid octets field");
  default:
    LOG("unknown error");
  }
#undef LOG
}

// -----[ netflow_parser ]-------------------------------------------
/**
 * Parse Netflow data produced by the flow-print utility (flow-tools)
 * with the default output format. The expected format is as follows.
 * - lines starting with '#' are ignored (comments)
 * - first non-comment line is header and must contain
 *    "srcIP  dstIP  prot  srcPort  dstPort  octets  packets"
 * - following lines contain a flow description
 *    <srcIP> <dstIP> <prot> <srcPort> <dstPort> <octets> <packets>
 *
 * For each Netflow record, the handler is called with the Netflow
 * fields (src, dst, octets) and the given context pointer.
 */
int netflow_parser(FILE_TYPE * pStream, FNetflowHandler fHandler,
		   void * pContext)
{
  STokenizer * pTokenizer;
  STokens * pTokens;
  char acFileLine[100];
  unsigned int uLineNumber= 0;
  int iError= NETFLOW_SUCCESS;
  int iState= NETFLOW_STATE_HEADER; // 0: parse header, 1: parse records
  char * pcField;
  net_addr_t tSrcAddr, tDstAddr;
  unsigned int uOctets;
  int iResult;

  pTokenizer= tokenizer_create(" \t", 0, NULL, NULL);

  while ((!FILE_EOF(pStream)) && (!iError)) {
    if (FILE_GETS(pStream, acFileLine, sizeof(acFileLine)) == NULL)
      break;
    uLineNumber++;

    // Skip comments starting with '#'
    if (acFileLine[0] == '#')
      continue;

    // Parse a single line
    if (tokenizer_run(pTokenizer, acFileLine) != TOKENIZER_SUCCESS) {
      iError= NETFLOW_ERROR_UNEXPECTED;
      break;
    }
    
    pTokens= tokenizer_get_tokens(pTokenizer);

    // Get and check fields
    if (tokens_get_num(pTokens) != 7) {
      iError= NETFLOW_ERROR_NUM_FIELDS;
      break;
    }

    // Check header (state == 0), then records (state == 1)
    if (iState == NETFLOW_STATE_HEADER) {
      if (strcmp("srcIP", tokens_get_string_at(pTokens, 0)) ||
	  strcmp("dstIP", tokens_get_string_at(pTokens, 1)) ||
	  strcmp("prot", tokens_get_string_at(pTokens, 2)) ||
	  strcmp("srcPort", tokens_get_string_at(pTokens, 3)) ||
	  strcmp("dstPort", tokens_get_string_at(pTokens, 4)) ||
	  strcmp("octets", tokens_get_string_at(pTokens, 5)) ||
	  strcmp("packets", tokens_get_string_at(pTokens, 6)))
	return NETFLOW_ERROR_INVALID_HEADER;
      iState= NETFLOW_STATE_RECORDS;

    } else if (iState == NETFLOW_STATE_RECORDS) {
      
      // Get src IP
      pcField= tokens_get_string_at(pTokens, 0);
      if (str2address(pcField, &tSrcAddr) < 0)
	return NETFLOW_ERROR_INVALID_SRC_ADDR;
      
      // Get dst IP
      pcField= tokens_get_string_at(pTokens, 1);
      if (str2address(pcField, &tDstAddr) < 0)
	return NETFLOW_ERROR_INVALID_DST_ADDR;

      // Get number of octets
      if (tokens_get_uint_at(pTokens, 5, &uOctets) != 0)
	return NETFLOW_ERROR_INVALID_OCTETS;

      iResult= fHandler(tSrcAddr, tDstAddr, uOctets, pContext);
      if (iResult != 0)
	return NETFLOW_ERROR_UNEXPECTED;

    } else
      abort();
    
  }
  tokenizer_destroy(&pTokenizer);

  return iError;
}

// -----[ netflow_load ]---------------------------------------------
int netflow_load(const char * pcFileName, FNetflowHandler fHandler,
		 void * pContext)
{
  FILE_TYPE * pFile;
  int iResult;

  pFile= FILE_OPEN(pcFileName, "r");
  if (pFile == NULL)
    return NETFLOW_ERROR_OPEN;

  iResult= netflow_parser(pFile, fHandler, pContext);

  FILE_CLOSE(pFile);
  return iResult;
}
