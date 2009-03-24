// ==================================================================
// @(#)netflow.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 14/05/2007
// $Id: netflow.c,v 1.3 2009-03-24 16:19:12 bqu Exp $
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <string.h>

#include <libgds/stream.h>
#include <libgds/str_util.h>

#include <net/netflow.h>
#include <net/prefix.h>
#include <net/util.h>

#define MAX_NETFLOW_LINE_LEN 100
static lrp_t * _parser;

// ----- Parser's states -----
typedef enum {
  NETFLOW_STATE_HEADER,
  NETFLOW_STATE_RECORDS,
} _netflow_parser_state_t;

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
void netflow_perror(gds_stream_t * stream, int error)
{
  char * error_msg= netflow_strerror(error);
  if (error_msg != NULL)
    stream_printf(stream, error_msg);
  else
    stream_printf(stream, "unknown error (%i)", error);
}

// -----[ netflow_strerror ]-----------------------------------------
char * netflow_strerror(int error)
{
  switch (error) {
  case NETFLOW_SUCCESS:
    return "success";
  case NETFLOW_ERROR_OPEN:
    return "unable to open file";
  case NETFLOW_ERROR_NUM_FIELDS:
    return "incorrect number of fields";
  case NETFLOW_ERROR_INVALID_HEADER:
    return "invalid (flow-print) header";
  case NETFLOW_ERROR_INVALID_SRC_ADDR:
    return "invalid source address";
  case NETFLOW_ERROR_INVALID_DST_ADDR:
    return "invalid destination address";
  case NETFLOW_ERROR_INVALID_OCTETS:
    return "invalid octets field";
  }
	return NULL;
}

// -----[ _netflow_parser ]------------------------------------------
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
static inline 
int _parse(lrp_t * parser, net_traffic_handler_f handler,
	   void * ctx)
{
  unsigned int num_fields;
  int state= NETFLOW_STATE_HEADER; // 0: parse header, 1: parse records
  const char * field;
  net_addr_t src_addr, dst_addr;
  unsigned int octets;
  int result;

  while (lrp_get_next_line(parser)) {
    result= lrp_get_num_fields(_parser, &num_fields);
    if (result < 0)
      return result;

    // Get and check fields
    if (num_fields != 7) {
      lrp_set_user_error(parser, "incorrect number of fields (7 expected)");
      return NETFLOW_ERROR_NUM_FIELDS;
    }

    // Check header (state == 0), then records (state == 1)
    switch (state) {
    case NETFLOW_STATE_HEADER:
      if (strcmp("srcIP", lrp_get_field(parser, 0)) ||
	  strcmp("dstIP", lrp_get_field(parser, 1)) ||
	  strcmp("prot", lrp_get_field(parser, 2)) ||
	  strcmp("srcPort", lrp_get_field(parser, 3)) ||
	  strcmp("dstPort", lrp_get_field(parser, 4)) ||
	  strcmp("octets", lrp_get_field(parser, 5)) ||
	  strcmp("packets", lrp_get_field(parser, 6))) {
	lrp_set_user_error(parser, "invalid header");
	return NETFLOW_ERROR_INVALID_HEADER;
      }
      state= NETFLOW_STATE_RECORDS;
      break;

    case NETFLOW_STATE_RECORDS:
      // Get src IP
      field= lrp_get_field(parser, 0);
      if (str2address(field, &src_addr) < 0) {
	lrp_set_user_error(parser, "invalid source address \"%s\"", field);
	return NETFLOW_ERROR_INVALID_SRC_ADDR;
      }
      
      // Get dst IP
      field= lrp_get_field(parser, 1);
      if (str2address(field, &dst_addr) < 0) {
	lrp_set_user_error(parser, "invalid destination address \"%s\"",
			   field);
	return NETFLOW_ERROR_INVALID_DST_ADDR;
      }

      // Get number of octets
      field= lrp_get_field(parser, 5);
      if (str_as_uint(field, &octets) < 0) {
	lrp_set_user_error(parser, "invalid volume \"%s\"", field);
	return NETFLOW_ERROR_INVALID_OCTETS;
      }

      result= handler(src_addr, dst_addr, octets, ctx);
      if (result != 0)
	return NETFLOW_ERROR;
      break;

    default:
      cbgp_fatal("incorrect state number %d", state);
    }
  }
  return NETFLOW_SUCCESS;
}

// -----[ netflow_parser ]-------------------------------------------
int netflow_parser(FILE * stream, net_traffic_handler_f handler,
		   void * ctx)
{
  lrp_set_stream(_parser, stream);
  return _parse(_parser, handler, ctx);
}

// -----[ netflow_load ]---------------------------------------------
int netflow_load(const char * filename, net_traffic_handler_f handler,
		 void * ctx)
{
  int result= lrp_open(_parser, filename);
  if (result < 0)
    return result;
  result= _parse(_parser, handler, ctx);
  lrp_close(_parser);
  return result;
}

// -----[ _netflow_init ]--------------------------------------------
void _netflow_init()
{
  _parser= lrp_create(MAX_NETFLOW_LINE_LEN, " \t");
}

// -----[ _netflow_done ]--------------------------------------------
void _netflow_done()
{
  lrp_destroy(&_parser);
}
